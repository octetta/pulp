#include <stdio.h>
#include <errno.h>

#ifndef _WIN32
#include <pthread.h>
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ksynth.h"
#include "kse.h"
#include "ks_chan.h"
#include "util.h"

static int kse_running = 1;

static pthread_t kse_thread_handle;
static pthread_attr_t kse_attr;
static ks_ctx *ctx = NULL;

static K r[KS_WRITERS] = {NULL};
static uint64_t request_seq[KS_WRITERS] = {0};
static uint64_t result_seq[KS_WRITERS] = {0};
static pthread_mutex_t result_mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t result_cv = PTHREAD_COND_INITIALIZER;

uint64_t kse_submit(int writer, const char *cmd, int len) {
  if (writer < 0 || writer >= KS_WRITERS) return 0;
  if (cmd == NULL) return 0;

  pthread_mutex_lock(&result_mu);
  uint64_t seq = ++request_seq[writer];
  pthread_mutex_unlock(&result_mu);

  if (!ks_send(writer, cmd, len, seq)) return 0;
  return seq;
}

static void abstime_from_now(struct timespec *ts, int timeout_ms) {
  clock_gettime(CLOCK_REALTIME, ts);
  ts->tv_sec += timeout_ms / 1000;
  ts->tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
  if (ts->tv_nsec >= 1000000000L) {
    ts->tv_sec++;
    ts->tv_nsec -= 1000000000L;
  }
}

int kse_wait(int writer, uint64_t seq, int timeout_ms) {
  if (writer < 0 || writer >= KS_WRITERS || seq == 0) return 0;

  pthread_mutex_lock(&result_mu);
  int ok = 1;
  while (result_seq[writer] < seq) {
    if (timeout_ms < 0) {
      pthread_cond_wait(&result_cv, &result_mu);
    } else {
      struct timespec ts;
      abstime_from_now(&ts, timeout_ms);
      int rc = pthread_cond_timedwait(&result_cv, &result_mu, &ts);
      if (rc != 0) {
        ok = 0;
        break;
      }
    }
  }
  pthread_mutex_unlock(&result_mu);
  return ok;
}

double *kse_result_copy(int writer, size_t *len, uint64_t *seq) {
  if (len) *len = 0;
  if (seq) *seq = 0;
  if (writer < 0 || writer >= KS_WRITERS) return NULL;

  pthread_mutex_lock(&result_mu);
  double *copy = NULL;
  if (seq) *seq = result_seq[writer];
  if (r[writer]) {
    size_t n = r[writer]->n;
    if (len) *len = n;
    if (n) {
      copy = (double *)malloc(n * sizeof(double));
      if (copy) memcpy(copy, r[writer]->f, n * sizeof(double));
    }
  }
  pthread_mutex_unlock(&result_mu);
  return copy;
}

void kse_result_free(double *p) {
  free(p);
}

static void handle_line(ks_ctx *ctx, int writer, uint64_t seq, char *line, size_t len) {
  if (strncmp(line, "\\X", 2) == 0) {
    printf("clearing vars\n");
    ks_clear_vars(ctx);
    printf("clearing r[]\n");
    pthread_mutex_lock(&result_mu);
    K old[KS_WRITERS];
    for (int i=0; i<KS_WRITERS; i++) {
      old[i] = r[i];
      r[i] = NULL;
      result_seq[i] = request_seq[i];
    }
    pthread_cond_broadcast(&result_cv);
    pthread_mutex_unlock(&result_mu);
    for (int i=0; i<KS_WRITERS; i++) {
      if (old[i]) {
        k_free(ctx, old[i]);
      }
    }
    printf("clearing gas\n");
    ctx->gas_used = 0;
    printf("clearing status\n");
    ctx->last_status = KS_OK; // is this good to do?
    return;
  }
  K next = ks_eval(ctx, line, len);
  if (ctx->last_status != KS_OK) {
    ks_status status = ctx->last_status;
    char *err = (char *)ks_strerror(status);
    printf("# r[%d] status %d error %s\n", writer, status, err);
    ctx->last_status = KS_OK; // is this good to do?
  }
  pthread_mutex_lock(&result_mu);
  K old = r[writer];
  r[writer] = next;
  result_seq[writer] = seq;
  pthread_cond_broadcast(&result_cv);
  pthread_mutex_unlock(&result_mu);
  if (old) k_free(ctx, old);
  ctx->gas_used = 0;
}

static void *kse_main(void *arg) {
  //printf("kse_main\n");
  (void)arg;
  // limits       mem          gas
  ctx = ks_create(16*1024*1024, 10000000);
  while (kse_running) {
    int len;
    int writer;
    uint64_t seq;
    //printf("about to recv...\n");
    char *cmd = ks_recv(&len, &writer, &seq);
    //printf("recv[%d]: {%.*s}\n", writer, len, cmd);
    if (!kse_running && len == 0) break;
    handle_line(ctx, writer, seq, cmd, len);
  }
  for (int i=0; i<KS_WRITERS; i++) if (r[i]) k_free(ctx, r[i]);
  ks_clear_vars(ctx);
  ks_destroy(ctx);
  return NULL;
}

int kse_start(void) {
  //printf("ks_start\n");
  ks_bus_init();
  kse_running = 1;
  pthread_attr_init(&kse_attr);
  pthread_attr_setstacksize(&kse_attr, 2 * 1024 * 1024);
  pthread_create(&kse_thread_handle, &kse_attr, kse_main, NULL);
  pthread_detach(kse_thread_handle);
  return 0;
}

void kse_stop(void) {
  kse_running = 0;
  ks_send(0, "", 0, 0);
}
