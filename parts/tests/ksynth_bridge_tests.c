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
  ks_ctx *ctx = ks_create(1024 * 1024, 1000000, 44100.0);
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
  ks_ctx *ctx = ks_create(1024 * 1024, 1000000, 44100.0);
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

static void test_ksynth_sample_rate_binding_and_scaling(void) {
  const char *test = "ksynth sample rate binding and phase scaling";
  skode_t ctx = SKODE_EMPTY();
  synth_init(8);
  wave_table_init(0);
  voice_init();
  skode_init(&ctx);

  synth_sample_rate_set(48000);
  consume(test, &ctx, "[S] ks kw>");
  if (ands_data_len(ctx.parse) < 1 || fabs(ands_data(ctx.parse)[0] - 48000.0) > 1e-3) {
    fail(test, "scalar S was not bound to system sample rate 48000");
  }

  consume(test, &ctx, "k>w302,22050,1");
  consume(test, &ctx, "v0 w302,1");
  float expected_ratio = 22050.0f / 48000.0f;
  float phase_inc = osc_get_phase_inc(&skred_global_engine,0, 440.0f);
  if (fabsf(phase_inc - expected_ratio) > 0.001f) {
    fail(test, "sample playback rate ratio was not applied for table_rate != system_rate");
  }
  float phase_inc_220 = osc_get_phase_inc(&skred_global_engine,0, 220.0f);
  if (fabsf(phase_inc_220 - (expected_ratio * 0.5f)) > 0.001f) {
    fail(test, "untuned k>w drum sample did not scale phase increment proportionally with frequency");
  }

  skode_free(&ctx);
  wave_free();
  synth_free();
}

static void test_runtime_drum_wavetable_initialization(void) {
  const char *test = "runtime drum wavetable initialization";
  synth_init(8);
  wave_table_init(0);

  const struct {
    int slot;
    const char *name;
    int expected_min_samples;
  } expected_drums[] = {
    { 82, "kick", 10000 },
    { 83, "snare", 10000 },
    { 84, "chh", 5000 },
    { 85, "ohh", 20000 },
    { 86, "clap", 5000 },
    { 87, "rim", 2000 },
    { 88, "tomlo", 20000 },
    { 89, "tommid", 20000 },
    { 90, "tomhi", 15000 },
    { 91, "lotom", 20000 },
    { 92, "midtom", 15000 },
    { 93, "hitom", 15000 },
    { 94, "crash", 50000 },
    { 95, "cymbal", 40000 },
    { 96, "cowbell", 30000 },
    { 97, "clave", 3000 },
    { 98, "maracas", 2500 },
    { 99, "trigger", 20000 },
    { 0, NULL, 0 }
  };

  for (int i = 0; expected_drums[i].slot != 0; i++) {
    int w = expected_drums[i].slot;
    const char *name = expected_drums[i].name;
    if (!sw.data[w]) {
      char msg[128];
      snprintf(msg, sizeof(msg), "drum slot %d (%s) data is null", w, name);
      fail(test, msg);
      continue;
    }
    if (strcmp(sw.name[w], name) != 0) {
      char msg[128];
      snprintf(msg, sizeof(msg), "drum slot %d name expected '%s', got '%s'", w, name, sw.name[w]);
      fail(test, msg);
    }
    if (sw.size[w] < expected_drums[i].expected_min_samples) {
      char msg[128];
      snprintf(msg, sizeof(msg), "drum slot %d size %d less than expected %d",
               w, sw.size[w], expected_drums[i].expected_min_samples);
      fail(test, msg);
    }
    if (sw.rate[w] != 44100.0f) {
      char msg[128];
      snprintf(msg, sizeof(msg), "drum slot %d sample rate expected 44100, got %.1f", w, sw.rate[w]);
      fail(test, msg);
    }
    if (!sw.one_shot[w]) {
      char msg[128];
      snprintf(msg, sizeof(msg), "drum slot %d is not marked one_shot", w);
      fail(test, msg);
    }
    if (!sw.readonly[w]) {
      char msg[128];
      snprintf(msg, sizeof(msg), "drum slot %d is not marked readonly", w);
      fail(test, msg);
    }

    // Check that samples are non-zero
    double energy = 0.0;
    for (int s = 0; s < sw.size[w]; s++) {
      energy += sw.data[w][s] * sw.data[w][s];
    }
    if (energy <= 1e-6) {
      char msg[128];
      snprintf(msg, sizeof(msg), "drum slot %d has no audio energy", w);
      fail(test, msg);
    }
  }

  wave_free();
  synth_free();
}

static void test_runtime_esq1_wavetable_initialization(void) {
  const char *test = "runtime esq1 wavetable initialization";
  synth_init(8);
  wave_table_init(0);

  const struct {
    int slot;
    const char *name;
    int is_oneshot;
  } expected_esq1[] = {
    { 50, "saw", 0 },
    { 51, "bell", 0 },
    { 52, "sine", 0 },
    { 53, "square", 0 },
    { 54, "pulse", 0 },
    { 55, "noise1", 0 },
    { 56, "noise2", 0 },
    { 57, "noise3", 0 },
    { 58, "bass", 0 },
    { 59, "piano", 0 },
    { 60, "elpno", 0 },
    { 61, "voice1", 0 },
    { 62, "voice2", 0 },
    { 63, "kick", 1 },
    { 64, "reed", 0 },
    { 65, "organ", 0 },
    { 66, "synth1", 0 },
    { 67, "synth2", 0 },
    { 68, "synth3", 0 },
    { 69, "formt1", 0 },
    { 70, "formt2", 0 },
    { 71, "formt3", 0 },
    { 72, "formt4", 0 },
    { 73, "formt5", 0 },
    { 74, "pulse2", 0 },
    { 75, "sqr2", 0 },
    { 76, "4octs", 0 },
    { 77, "prime", 0 },
    { 78, "brass", 0 },
    { 79, "string", 0 },
    { 80, "octave", 0 },
    { 81, "oct5th", 0 },
    { 0, NULL, 0 }
  };

  for (int i = 0; expected_esq1[i].slot != 0; i++) {
    int w = expected_esq1[i].slot;
    const char *name = expected_esq1[i].name;
    if (!sw.data[w]) {
      char msg[128];
      snprintf(msg, sizeof(msg), "esq1 slot %d (%s) data is null", w, name);
      fail(test, msg);
      continue;
    }
    if (strcmp(sw.name[w], name) != 0) {
      char msg[128];
      snprintf(msg, sizeof(msg), "esq1 slot %d name expected '%s', got '%s'", w, name, sw.name[w]);
      fail(test, msg);
    }
    if (sw.size[w] != 4096) {
      char msg[128];
      snprintf(msg, sizeof(msg), "esq1 slot %d size expected 4096, got %d", w, sw.size[w]);
      fail(test, msg);
    }
    if (sw.rate[w] != 44100.0f) {
      char msg[128];
      snprintf(msg, sizeof(msg), "esq1 slot %d sample rate expected 44100, got %.1f", w, sw.rate[w]);
      fail(test, msg);
    }
    if (sw.one_shot[w] != expected_esq1[i].is_oneshot) {
      char msg[128];
      snprintf(msg, sizeof(msg), "esq1 slot %d one_shot expected %d, got %d",
               w, expected_esq1[i].is_oneshot, sw.one_shot[w]);
      fail(test, msg);
    }
    if (!sw.readonly[w]) {
      char msg[128];
      snprintf(msg, sizeof(msg), "esq1 slot %d is not marked readonly", w);
      fail(test, msg);
    }

    // Check that samples are non-zero
    double energy = 0.0;
    for (int s = 0; s < sw.size[w]; s++) {
      energy += sw.data[w][s] * sw.data[w][s];
    }
    if (energy <= 1e-6) {
      char msg[128];
      snprintf(msg, sizeof(msg), "esq1 slot %d has no audio energy", w);
      fail(test, msg);
    }
  }

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
  test_ksynth_sample_rate_binding_and_scaling();
  test_runtime_esq1_wavetable_initialization();
  test_runtime_drum_wavetable_initialization();
#endif

  if (failures) {
    fprintf(stderr, "%d Ksynth bridge test failure(s)\n", failures);
    return 1;
  }
  printf("Ksynth bridge tests passed\n");
  return 0;
}
