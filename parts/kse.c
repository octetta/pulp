#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "vendor/ksynth/ksynth.h"
#include "kse.h"
#include "ks_chan.h"

static pthread_t kse_thread_handle;
static int kse_started;
static ks_ctx *ctx;
static pthread_mutex_t lifecycle_mu = PTHREAD_MUTEX_INITIALIZER;

static K r[KS_WRITERS] = {NULL};
static uint64_t request_seq[KS_WRITERS] = {0};
static uint64_t result_seq[KS_WRITERS] = {0};
static pthread_mutex_t result_mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t result_cv = PTHREAD_COND_INITIALIZER;

static uint64_t kse_next_seq(int writer) {
  pthread_mutex_lock(&result_mu);
  uint64_t seq = ++request_seq[writer];
  pthread_mutex_unlock(&result_mu);
  return seq;
}

uint64_t kse_submit(int writer, const char *cmd, int len) {
  if (writer < 0 || writer >= KS_WRITERS || !cmd || len < 0) return 0;
  pthread_mutex_lock(&lifecycle_mu);
  if (!kse_started) {
    pthread_mutex_unlock(&lifecycle_mu);
    return 0;
  }
  uint64_t seq = kse_next_seq(writer);
  int sent = ks_send_eval(writer, cmd, (size_t)len, seq);
  pthread_mutex_unlock(&lifecycle_mu);
  return sent ? seq : 0;
}

uint64_t kse_bind_vector(int writer, char variable, const double *values,
                         size_t len) {
  if (writer < 0 || writer >= KS_WRITERS ||
      variable < 'A' || variable > 'Z' ||
      (len > 0 && !values) || len > 1000000 || len > INT_MAX) {
    return 0;
  }
  pthread_mutex_lock(&lifecycle_mu);
  if (!kse_started) {
    pthread_mutex_unlock(&lifecycle_mu);
    return 0;
  }
  uint64_t seq = kse_next_seq(writer);
  int sent = ks_send_bind(writer, variable, values, len, seq);
  pthread_mutex_unlock(&lifecycle_mu);
  return sent ? seq : 0;
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
  struct timespec deadline;
  if (timeout_ms >= 0) abstime_from_now(&deadline, timeout_ms);
  while (result_seq[writer] < seq) {
    int rc = timeout_ms < 0
      ? pthread_cond_wait(&result_cv, &result_mu)
      : pthread_cond_timedwait(&result_cv, &result_mu, &deadline);
    if (rc != 0) {
      ok = 0;
      break;
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
  if (r[writer] && !k_is_func(r[writer])) {
    size_t n = (size_t)r[writer]->n;
    if (len) *len = n;
    if (n && n <= SIZE_MAX / sizeof(double)) {
      copy = malloc(n * sizeof(double));
      if (copy) {
        for (size_t i = 0; i < n; i++) copy[i] = r[writer]->f[i];
      }
    }
  }
  pthread_mutex_unlock(&result_mu);
  return copy;
}

void kse_result_free(double *p) {
  free(p);
}

static void kse_complete(int writer, uint64_t seq, K next, int replace_result) {
  pthread_mutex_lock(&result_mu);
  K old = NULL;
  if (replace_result) {
    old = r[writer];
    r[writer] = next;
  }
  result_seq[writer] = seq;
  pthread_cond_broadcast(&result_cv);
  pthread_mutex_unlock(&result_mu);
  if (old) k_free(ctx, old);
}

static void handle_eval(const KS_Job *job) {
  if (job->len >= 2 && strncmp(job->payload.code, "\\X", 2) == 0) {
    ks_clear_vars(ctx);
    pthread_mutex_lock(&result_mu);
    K old[KS_WRITERS];
    for (int i = 0; i < KS_WRITERS; i++) {
      old[i] = r[i];
      r[i] = NULL;
    }
    result_seq[job->writer] = job->seq;
    pthread_cond_broadcast(&result_cv);
    pthread_mutex_unlock(&result_mu);
    for (int i = 0; i < KS_WRITERS; i++) k_free(ctx, old[i]);
    return;
  }

  K next = ks_eval(ctx, job->payload.code, job->len);
  if (ctx->last_status != KS_OK) {
    fprintf(stderr, "# r[%d] status %d error %s\n", job->writer,
            ctx->last_status, ks_strerror(ctx->last_status));
  }
  kse_complete(job->writer, job->seq, next, 1);
}

static void handle_bind(const KS_Job *job) {
  ks_status status = ks_bind_vector(ctx, job->variable, job->payload.values,
                                    job->len);
  if (status != KS_OK) {
    fprintf(stderr, "# bind %c status %d error %s\n", job->variable,
            status, ks_strerror(status));
  }
  kse_complete(job->writer, job->seq, NULL, 0);
}

static void *kse_main(void *arg) {
  (void)arg;
  KS_Job job;
  while (ks_recv(&job)) {
    if (job.type == KS_JOB_EVAL) handle_eval(&job);
    else if (job.type == KS_JOB_BIND) handle_bind(&job);
    ks_job_dispose(&job);
  }

  for (int i = 0; i < KS_WRITERS; i++) {
    k_free(ctx, r[i]);
    r[i] = NULL;
  }
  ks_destroy(ctx);
  ctx = NULL;
  return NULL;
}

int kse_start(void) {
  pthread_mutex_lock(&lifecycle_mu);
  if (kse_started) {
    pthread_mutex_unlock(&lifecycle_mu);
    return 0;
  }
  if (!ks_bus_init()) {
    pthread_mutex_unlock(&lifecycle_mu);
    return -1;
  }
  ctx = ks_create(16 * 1024 * 1024, 10000000);
  if (!ctx) {
    ks_bus_destroy();
    pthread_mutex_unlock(&lifecycle_mu);
    return -1;
  }
  for (int i = 0; i < KS_WRITERS; i++) {
    request_seq[i] = 0;
    result_seq[i] = 0;
    r[i] = NULL;
  }
  pthread_attr_t attr;
  if (pthread_attr_init(&attr) != 0) {
    ks_destroy(ctx);
    ctx = NULL;
    ks_bus_destroy();
    pthread_mutex_unlock(&lifecycle_mu);
    return -1;
  }
  pthread_attr_setstacksize(&attr, 2 * 1024 * 1024);
  int rc = pthread_create(&kse_thread_handle, &attr, kse_main, NULL);
  pthread_attr_destroy(&attr);
  if (rc != 0) {
    ks_destroy(ctx);
    ctx = NULL;
    ks_bus_destroy();
    pthread_mutex_unlock(&lifecycle_mu);
    return -1;
  }
  kse_started = 1;
  pthread_mutex_unlock(&lifecycle_mu);
  return 0;
}

void kse_stop(void) {
  pthread_mutex_lock(&lifecycle_mu);
  if (!kse_started) {
    pthread_mutex_unlock(&lifecycle_mu);
    return;
  }
  kse_started = 0;
  ks_bus_stop();
  pthread_join(kse_thread_handle, NULL);
  ks_bus_destroy();
  pthread_mutex_unlock(&lifecycle_mu);
}
