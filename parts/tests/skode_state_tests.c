#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ands.h"
#include "skode.h"
#include "synth.h"
#include "synth-state.h"

void seq_init(void);

static int failures = 0;

static void fail(const char *test, const char *msg) {
  fprintf(stderr, "FAIL %s: %s\n", test, msg);
  failures++;
}

static void expect_int(const char *test, int got, int want, const char *label) {
  if (got != want) {
    char msg[160];
    snprintf(msg, sizeof(msg), "%s expected %d, got %d", label, want, got);
    fail(test, msg);
  }
}

static void expect_float(const char *test, float got, float want, float eps, const char *label) {
  if (fabsf(got - want) > eps) {
    char msg[160];
    snprintf(msg, sizeof(msg), "%s expected %.8g, got %.8g", label, want, got);
    fail(test, msg);
  }
}

static void expect_u64(const char *test, uint64_t got, uint64_t want, const char *label) {
  if (got != want) {
    char msg[160];
    snprintf(msg, sizeof(msg), "%s expected %llu, got %llu", label,
      (unsigned long long)want, (unsigned long long)got);
    fail(test, msg);
  }
}

static skode_t new_ctx(void) {
  skode_t ctx = SKODE_EMPTY();
  skode_init(&ctx);
  return ctx;
}

static void consume(const char *test, skode_t *ctx, const char *line) {
  char buf[512];
  snprintf(buf, sizeof(buf), "%s", line);
  int r = skode_consume(buf, ctx);
  if (r != 0) {
    char msg[160];
    snprintf(msg, sizeof(msg), "skode_consume returned %d for [%s]", r, line);
    fail(test, msg);
  }
}

static void test_voice_core_commands(void) {
  const char *test = "voice core commands";
  skode_t ctx = new_ctx();
  consume(test, &ctx, "v3 w1 f220 a-6 p0.5 m1 b1 B1");

  expect_int(test, ctx.voice, 3, "ctx.voice");
  expect_int(test, sv.wave_table_index[3], 1, "wave_table_index");
  expect_float(test, sv.freq[3], 220.0f, 0.0001f, "freq");
  expect_float(test, sv.user_amp[3], -6.0f, 0.0001f, "user_amp");
  expect_float(test, sv.pan[3], 0.5f, 0.0001f, "pan");
  expect_float(test, sv.pan_left[3], 0.25f, 0.0001f, "pan_left");
  expect_float(test, sv.pan_right[3], 0.75f, 0.0001f, "pan_right");
  expect_int(test, sv.disconnect[3], 1, "disconnect");
  expect_int(test, sv.direction[3], 1, "direction");
  expect_int(test, sv.loop_enabled[3], 1, "loop_enabled");
}

static void test_invalid_voice_does_not_move_selection(void) {
  const char *test = "invalid voice";
  skode_t ctx = new_ctx();
  consume(test, &ctx, "v2");
  consume(test, &ctx, "v999");
  expect_int(test, ctx.voice, 2, "ctx.voice");
}

static void test_text_and_show_logging(void) {
  const char *test = "text and logging";
  skode_t ctx = new_ctx();
  ctx.log_enable = 1;
  consume(test, &ctx, "v1 [lead beep] vt [scratch text] ?s");

  expect_int(test, ctx.voice, 1, "ctx.voice");
  if (strncmp(sv.text[1], "lead beep", TEXT_MAX) != 0) {
    fail(test, "voice text mismatch");
  }
  if (strstr(ctx.log, "# [scratch text]") == NULL) {
    fail(test, "expected ?s output in log");
  }
}

static void test_data_array_logging(void) {
  const char *test = "data array logging";
  skode_t ctx = new_ctx();
  ctx.log_enable = 1;
  consume(test, &ctx, "(1 2.5 -3) ?d");

  expect_int(test, ands_data_len(ctx.parse), 3, "data length");
  expect_float(test, (float)ands_data(ctx.parse)[0], 1.0f, 0.0001f, "data[0]");
  expect_float(test, (float)ands_data(ctx.parse)[1], 2.5f, 0.0001f, "data[1]");
  expect_float(test, (float)ands_data(ctx.parse)[2], -3.0f, 0.0001f, "data[2]");
  if (strstr(ctx.log, "( 1 2.5 -3 )") == NULL) {
    fail(test, "expected ?d output in log");
  }
}

static void test_midi_and_links(void) {
  const char *test = "midi and links";
  skode_t ctx = new_ctx();
  consume(test, &ctx, "v0 G1 H2 L0.25 N12 5 n60");

  expect_int(test, (int)sv.link_midi_0[0], 1, "link_midi_0");
  expect_int(test, (int)sv.link_velo_0[0], 2, "link_velo_0");
  expect_float(test, sv.link_trig[0], 0.25f, 0.0001f, "link_trig");
  expect_u64(test, sv.link_trig_samp[0], MAIN_SAMPLE_RATE / 4, "link_trig_samp");
  expect_float(test, sv.midi_transpose[0], 12.0f, 0.0001f, "midi_transpose");
  expect_float(test, sv.midi_cents[0], 5.0f, 0.0001f, "midi_cents");
  expect_float(test, sv.last_midi_note[0], 60.0f, 0.0001f, "last_midi_note voice 0");
  expect_float(test, sv.last_midi_note[1], 60.0f, 0.0001f, "last_midi_note linked voice");
}

static void test_trigger_delay_lifecycle(void) {
  const char *test = "trigger delay lifecycle";
  skode_t ctx = new_ctx();

  consume(test, &ctx, "v0 L0.5");
  voice_copy(0, 1);
  expect_float(test, sv.link_trig[1], 0.5f, 0.0001f, "copied link_trig");
  expect_u64(test, sv.link_trig_samp[1], MAIN_SAMPLE_RATE / 2, "copied link_trig_samp");

  wave_reset(0);
  expect_float(test, sv.link_trig[0], -1.0f, 0.0001f, "reset link_trig");
  expect_u64(test, sv.link_trig_samp[0], 0, "reset link_trig_samp");

  consume(test, &ctx, "v1 L-1");
  expect_float(test, sv.link_trig[1], -1.0f, 0.0001f, "disabled link_trig");
  expect_u64(test, sv.link_trig_samp[1], 0, "disabled link_trig_samp");

  consume(test, &ctx, "v0 H999 l1");
}

static void test_context_modes(void) {
  const char *test = "context modes";
  skode_t ctx = new_ctx();
  consume(test, &ctx, "/v1 /t1 /f7 /c1");

  expect_int(test, ctx.verbose, 1, "verbose");
  expect_int(test, ctx.trace, 1, "trace");
  expect_int(test, ctx.flag, 7, "flag");
  expect_int(test, ands_chunk_mode_get(ctx.parse), 1, "chunk mode");
}

int main(void) {
  synth_init(8);
  wave_table_init(0);
  voice_init();
  seq_init();

  test_voice_core_commands();
  test_invalid_voice_does_not_move_selection();
  test_text_and_show_logging();
  test_data_array_logging();
  test_midi_and_links();
  test_trigger_delay_lifecycle();
  test_context_modes();

  synth_free();

  if (failures) {
    fprintf(stderr, "%d SKODE state test failure(s)\n", failures);
    return 1;
  }
  printf("SKODE state tests passed\n");
  return 0;
}
