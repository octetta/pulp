#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ands.h"
#include "kse.h"
#include "vendor/ksynth/ksynth.h"

#ifdef SKRED_TEST_KSYNTH
#include "skode.h"
#include "synth.h"
#include "synth-state.h"
#endif

static int failures;

static void fail(const char *test, const char *message) {
  fprintf(stderr, "FAIL %s: %s\n", test, message);
  failures++;
}

static void expect_vector(const char *test, const double *got, size_t got_len,
                          const double *want, size_t want_len) {
  if (got_len != want_len) {
    char message[128];
    snprintf(message, sizeof(message), "length expected %zu, got %zu",
             want_len, got_len);
    fail(test, message);
    return;
  }
  for (size_t i = 0; i < want_len; i++) {
    if (fabs(got[i] - want[i]) > 1e-9) {
      char message[160];
      snprintf(message, sizeof(message),
               "element %zu expected %.12g, got %.12g", i, want[i], got[i]);
      fail(test, message);
      return;
    }
  }
}

static void test_owned_eval_result(void) {
  const char *test = "owned eval result";
  ks_ctx *ctx = ks_create(1024 * 1024, 1000000);
  if (!ctx) {
    fail(test, "ks_create failed");
    return;
  }

  K first = ks_eval(ctx, "1 2 3", strlen("1 2 3"));
  K overwrite = ks_eval(ctx, "10000#!1", strlen("10000#!1"));
  const double want[] = {1, 2, 3};
  if (!first) fail(test, "first evaluation returned null");
  else expect_vector(test, first->f, (size_t)first->n, want, 3);

  k_free(ctx, overwrite);
  k_free(ctx, first);

  for (int i = 0; i < 100; i++) {
    K rejected = ks_eval(ctx, "1000001#!1", strlen("1000001#!1"));
    if (rejected) fail(test, "invalid expression returned a result");
    if (ctx->last_status != KS_ERR_INVALID_ARGS) {
      fail(test, "invalid expression did not report invalid arguments");
    }
  }
  ks_destroy(ctx);
}

static void test_generic_vector_binding(void) {
  const char *test = "generic vector binding";
  ks_ctx *ctx = ks_create(1024 * 1024, 1000000);
  if (!ctx) {
    fail(test, "ks_create failed");
    return;
  }

  const double input[] = {0.25, -0.5, 1.0};
  const double want[] = {0.25, -0.5, 1.0, 0.25, -0.5, 1.0};
  if (ks_bind_vector(ctx, 'A', input, 3) != KS_OK) {
    fail(test, "ks_bind_vector failed");
  } else {
    K result = ks_eval(ctx, "A,A", strlen("A,A"));
    if (!result) fail(test, "evaluation returned null");
    else expect_vector(test, result->f, (size_t)result->n, want, 6);
    k_free(ctx, result);
  }
  ks_destroy(ctx);
}

static void test_worker_binding_order(void) {
  const char *test = "worker binding order";
  const double input[] = {1, 2, 3};
  const double want[] = {1, 2, 3, 1, 2, 3};

  if (kse_start() != 0) {
    fail(test, "kse_start failed");
    return;
  }
  uint64_t bind_seq = kse_bind_vector(0, 'A', input, 3);
  uint64_t eval_seq = kse_submit(0, "A,A", 3);
  if (!bind_seq || !eval_seq) {
    fail(test, "submission failed");
  } else if (!kse_wait(0, eval_seq, 1000)) {
    fail(test, "worker timed out");
  } else {
    size_t len = 0;
    double *result = kse_result_copy(0, &len, NULL);
    if (!result) fail(test, "result copy failed");
    else expect_vector(test, result, len, want, 6);
    kse_result_free(result);
  }

  for (int iteration = 0; iteration < 1000; iteration++) {
    double changing[] = {iteration, iteration + 0.5, -iteration};
    bind_seq = kse_bind_vector(0, 'A', changing, 3);
    eval_seq = kse_submit(0, "i A", 3);
    if (!bind_seq || !eval_seq || !kse_wait(0, eval_seq, 1000)) {
      fail(test, "repeated bridge operation failed");
      break;
    }
  }
  kse_stop();

  if (kse_start() != 0) fail(test, "worker restart failed");
  else kse_stop();
}

#ifdef SKRED_TEST_KSYNTH
static void consume(const char *test, skode_t *ctx, const char *line) {
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "%s", line);
  if (skode_consume(buffer, ctx) != 0) fail(test, "skode_consume failed");
}

static void test_skode_wave_round_trip(void) {
  const char *test = "skode wave round trip";
  skode_t ctx = SKODE_EMPTY();
  synth_init(8);
  wave_table_init(0);
  voice_init();
  skode_init(&ctx);

  if (kse_start() != 0) {
    fail(test, "kse_start failed");
  } else {
    consume(test, &ctx, "(0 .5 -1) d>k0 [A,A] ks kw>1000");
    const double concatenated[] = {0, 0.5, -1, 0, 0.5, -1};
    expect_vector(test, ands_data(ctx.parse),
                  (size_t)ands_data_len(ctx.parse), concatenated, 6);

    consume(test, &ctx, "/d300,44100");
    if (!sw.data[300] || sw.size[300] != 6) {
      fail(test, "parser data was not loaded into wave 300");
    }

    consume(test, &ctx, "w>k300,1 [i B] ks kw>1000");
    const double reversed[] = {-1, 0.5, 0, -1, 0.5, 0};
    expect_vector(test, ands_data(ctx.parse),
                  (size_t)ands_data_len(ctx.parse), reversed, 6);
    kse_stop();
  }

  ands_free(ctx.parse);
  wave_free();
  synth_free();
}
#endif

int main(void) {
  test_owned_eval_result();
  test_generic_vector_binding();
  test_worker_binding_order();
#ifdef SKRED_TEST_KSYNTH
  test_skode_wave_round_trip();
#endif

  if (failures) {
    fprintf(stderr, "%d Ksynth bridge test failure(s)\n", failures);
    return 1;
  }
  printf("Ksynth bridge tests passed\n");
  return 0;
}
