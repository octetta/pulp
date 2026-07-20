#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ands.h"
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

#ifdef SKRED_TEST_KSYNTH
static void consume(const char *test, skode_t *ctx, const char *line) {
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "%s", line);
  if (skode_consume(buffer, ctx) != 0) fail(test, "skode_consume failed");
}

static void test_skode_context_isolation(void) {
  const char *test = "skode ksynth context isolation";
  skode_t a = SKODE_EMPTY();
  skode_t b = SKODE_EMPTY();
  skode_init(&a);
  skode_init(&b);

  consume(test, &a, "(1 2 3) d>k0 [A,A] ks kw>");
  consume(test, &b, "(9) d>k0 [A,A,A] ks kw>");

  const double want_a[] = {1, 2, 3, 1, 2, 3};
  const double want_b[] = {9, 9, 9};
  expect_vector(test, ands_data(a.parse), (size_t)ands_data_len(a.parse),
                want_a, 6);
  expect_vector(test, ands_data(b.parse), (size_t)ands_data_len(b.parse),
                want_b, 3);

  consume(test, &a, "[A] ks kw>");
  expect_vector(test, ands_data(a.parse), (size_t)ands_data_len(a.parse),
                (const double[]){1, 2, 3}, 3);
  consume(test, &b, "[A] ks kw>");
  expect_vector(test, ands_data(b.parse), (size_t)ands_data_len(b.parse),
                (const double[]){9}, 1);

  skode_free(&a);
  skode_free(&b);
}

static void test_skode_wave_round_trip(void) {
  const char *test = "skode wave round trip";
  skode_t ctx = SKODE_EMPTY();
  synth_init(8);
  wave_table_init(0);
  voice_init();
  skode_init(&ctx);

  consume(test, &ctx, "(0 .5 -1) d>k0 [A,A] ks kw>1000");
  const double concatenated[] = {0, 0.5, -1, 0, 0.5, -1};
  expect_vector(test, ands_data(ctx.parse),
                (size_t)ands_data_len(ctx.parse), concatenated, 6);

  consume(test, &ctx, "k>w300");
  if (!sw.data[300] || sw.size[300] != 6) {
    fail(test, "parser data was not loaded into wave 300");
  }
  if (sw.one_shot[300] || sw.loop_start[300] != 0 ||
      sw.loop_end[300] != sw.size[300]) {
    fail(test, "k>w did not install a full-range cycle");
  }
  consume(test, &ctx, "v0 w300,1");
  if (sv.playback_class[0] != OSC_PLAYBACK_CYCLE_SIMPLE)
    fail(test, "cycle wave did not select simple playback");
  consume(test, &ctx, "w300,1,1");
  if (sv.playback_class[0] != OSC_PLAYBACK_GENERAL)
    fail(test, "one-shot override did not select general playback");
  consume(test, &ctx, "w300,1");
  if (sv.one_shot[0] || sv.playback_class[0] != OSC_PLAYBACK_CYCLE_SIMPLE)
    fail(test, "wave reselection did not restore its stored cycle mode");

  consume(test, &ctx, "k>w301,22050,1");
  if (!sw.data[301] || sw.size[301] != 6 || !sw.one_shot[301] ||
      fabsf(sw.rate[301] - 22050.0f) > 0.01f) {
    fail(test, "k>w one-shot options were not applied");
  }
  consume(test, &ctx, "w301,1");
  if (!sv.one_shot[0] || sv.playback_class[0] != OSC_PLAYBACK_GENERAL)
    fail(test, "one-shot wave mode was not inherited");

  consume(test, &ctx, "w>k300,1 [i B] ks kw>1000");
  const double reversed[] = {-1, 0.5, 0, -1, 0.5, 0};
  expect_vector(test, ands_data(ctx.parse),
                (size_t)ands_data_len(ctx.parse), reversed, 6);

  skode_free(&ctx);
  wave_free();
  synth_free();
}
#endif

int main(void) {
  test_owned_eval_result();
  test_generic_vector_binding();
#ifdef SKRED_TEST_KSYNTH
  test_skode_context_isolation();
  test_skode_wave_round_trip();
#endif

  if (failures) {
    fprintf(stderr, "%d Ksynth bridge test failure(s)\n", failures);
    return 1;
  }
  printf("Ksynth bridge tests passed\n");
  return 0;
}
