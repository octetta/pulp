#include <stdio.h>
#include <errno.h>

#ifndef _WIN32
#include <pthread.h>
#endif

#include <stdint.h>

#include "ksynth.h"
#include "kse.h"
#include "ks_chan.h"
#include "util.h"

static int kse_running = 1;

static pthread_t kse_thread_handle;
static pthread_attr_t kse_attr;
static ks_ctx *ctx = NULL;

static K r[KS_WRITERS] = {NULL};

double *kse_get_result(int writer, size_t *len) {
  int thelen = 0;
  double *thearray = NULL;
  if (r[writer]) {
    thearray = r[writer]->f;
    thelen = r[writer]->n;
  }
  if (len) *len = thelen;
  return thearray;
}

static void handle_line(ks_ctx *ctx, int writer, char *line, size_t len) {
  if (r[writer]) {
    //printf("# free r[%d]\n", writer);
    k_free(ctx, r[writer]);
    r[writer] = NULL;
  }
  r[writer] = ks_eval(ctx, line, len);
  if (ctx->last_status != KS_OK) {
    ks_status status = ctx->last_status;
    char *err = (char *)ks_strerror(status);
    //printf("# r[%d] status %d error %s\n", writer, status, err);
  }
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
    //printf("about to recv...\n");
    char *cmd = ks_recv(&len, &writer);
    //printf("recv[%d]: {%.*s}\n", writer, len, cmd);
    handle_line(ctx, writer, cmd, len);
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
}
