#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ands.h"
#include "api.h"
#include "skode.h"
#include "seq.h"
#include "synth.h"
#include "synth-state.h"

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

typedef struct {
  int count;
  event_t event;
} event_capture_t;

typedef struct {
  int count;
  uint64_t timestamp[8];
  int tag[8];
  event_t event[8];
} repeat_capture_t;

static int capture_event(int unused, uint64_t timestamp, uint64_t id, int tag,
                         const event_t *event, void *user) {
  (void)unused;
  (void)timestamp;
  (void)id;
  (void)tag;
  event_capture_t *capture = user;
  capture->count++;
  capture->event = *event;
  return 0;
}

static int capture_repeat(int unused, uint64_t timestamp, uint64_t id, int tag,
                          const event_t *event, void *user) {
  (void)unused;
  (void)id;
  repeat_capture_t *capture = user;
  if (capture->count < 8) {
    int n = capture->count;
    capture->timestamp[n] = timestamp;
    capture->tag[n] = tag;
    capture->event[n] = *event;
  }
  capture->count++;
  return 0;
}

static skode_t *queued_event_ctx;
static int test_pattern_voice[PATTERNS_MAX];
static int pattern_callback_count;

static void execute_queued_event(const event_t *event) {
  if (skode_execute_event(event, queued_event_ctx) != 0) {
    fail("opcode events", "queued event execution failed");
  }
}

static void execute_pattern_program(int pattern, int step,
    const event_program_t *program) {
  if (pattern < 0 || pattern >= PATTERNS_MAX ||
      skode_execute_program_state(program, &test_pattern_voice[pattern],
        SAMPLE_COUNT_GET(), -1, pattern, step) != 0) {
    fail("opcode events", "pattern program execution failed");
  }
}

static void count_pattern_program(int pattern, int step,
    const event_program_t *program) {
  (void)pattern;
  (void)step;
  (void)program;
  pattern_callback_count++;
}

static void ignore_queued_event(const event_t *event) {
  (void)event;
}

static void test_voice_core_commands(void) {
  const char *test = "voice core commands";
  skode_t ctx = new_ctx();
  consume(test, &ctx, "v3 w1 f220 a-6 p0.5 m1 b1 BC3");

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
  expect_int(test, sv.loop_count[3], 3, "loop_count");
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

static void test_named_wave_destination(void) {
  const char *test = "named wave destination";
  skode_t ctx = new_ctx();
  char command[512];
  snprintf(command, sizeof(command), "[%s/wav/11.wav] /ws200",
           SKRED_TEST_SOURCE_DIR);
  consume(test, &ctx, command);

  if (!sw.data[200]) fail(test, "wave 200 was not populated");
  if (sw.size[200] <= 0) fail(test, "wave 200 has no samples");
  if (sw.name[200][0] == '\0') fail(test, "wave 200 label is empty");
  expect_int(test, sw.readonly[200], 0, "wave 200 readonly");
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

static void test_envelope_configuration_is_deferred(void) {
  const char *test = "deferred envelope configuration";
  if (!skode_opcode_supported(SKODE_OP_ENVELOPE) ||
      !skode_opcode_supported(SKODE_OP_FILTER_ENVELOPE)) {
    return;
  }

  skode_t ctx = new_ctx();
  char before[2048];
  char after[2048];
  uint64_t saved_sample_count = SAMPLE_COUNT_GET();

  consume(test, &ctx, "v0 t.1,.2,.3,.4 ft.5,.6,.7,.8 l.75");
  voice_format(0, before, sizeof(before), 1);
  if (strstr(before, "amp_env_active:1") == NULL ||
      strstr(before, "amp_env_runtime:4410,8820,0.3,17640") == NULL ||
      strstr(before, "filter_env_active:1") == NULL ||
      strstr(before, "filter_env_runtime:22050,26460,0.7,35280") == NULL) {
    fail(test, "initial runtime envelope snapshot mismatch");
  }

  consume(test, &ctx, "t1,2,.9,4 ft0,0,1,0");
  voice_format(0, after, sizeof(after), 1);
  if (strstr(after, "t1,2,0.9,4") == NULL ||
      strstr(after, "ft 0 0 1 0") == NULL ||
      strstr(after, "amp_env_active:1") == NULL ||
      strstr(after, "amp_env_runtime:4410,8820,0.3,17640") == NULL ||
      strstr(after, "filter_env_active:1") == NULL ||
      strstr(after, "filter_env_runtime:22050,26460,0.7,35280") == NULL) {
    fail(test, "configuration disturbed active envelope");
  }

  SAMPLE_COUNT_PUT(saved_sample_count + 10);
  consume(test, &ctx, "l0");
  voice_format(0, after, sizeof(after), 1);
  char expected_release[80];
  snprintf(expected_release, sizeof(expected_release), "filter_env_release:%llu",
           (unsigned long long)(saved_sample_count + 10));
  if (strstr(after, expected_release) == NULL) {
    fail(test, "disabled filter envelope did not receive release");
  }

  consume(test, &ctx, "l1");
  voice_format(0, after, sizeof(after), 1);
  if (strstr(after, "amp_env_runtime:44100,88200,0.9,176400") == NULL) {
    fail(test, "new amplitude envelope was not applied on retrigger");
  }

  SAMPLE_COUNT_PUT(saved_sample_count);
  wave_reset(0);
}

static void test_envelope_future_timestamps(void) {
  const char *test = "envelope future timestamps";

  envelope_t envelope;
  envelope_init_e(&envelope, .5f, .5f, 0.0f, .5f);
  envelope.is_active = 1;
  envelope.velocity = 1.0f;
  envelope.sample_start = 1024;
  envelope.sample_release = UINT64_MAX;

  expect_float(test, envelope_step_e(&envelope, 1000), 0.0f, 0.0f,
               "future trigger remains at attack start");

  envelope_init_e(&envelope, 0.0f, 0.0f, 1.0f, .5f);
  envelope.is_active = 1;
  envelope.velocity = 1.0f;
  envelope.current_amplitude = 0.75f;
  envelope.amplitude_at_release = 0.75f;
  envelope.sample_start = 0;
  envelope.sample_release = 1024;
  expect_float(test, envelope_step_e(&envelope, 1000), 0.75f, 0.0001f,
               "future release remains at release start");
}

static void configure_loop_test_voice(int voice, int direction) {
  static float table[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  wave_reset(voice);
  sv.table[voice] = table;
  sv.table_size[voice] = 8;
  sv.table_rate[voice] = MAIN_SAMPLE_RATE;
  sv.table_size_rate[voice] = 8.0f / MAIN_SAMPLE_RATE;
  sv.loop_start[voice] = 2;
  sv.loop_end[voice] = 5;
  sv.loop_start_f[voice] = 2.0f;
  sv.loop_end_f[voice] = 5.0f;
  sv.loop_length[voice] = 3;
  sv.loop_valid[voice] = 1;
  sv.one_shot[voice] = 1;
  sv.direction[voice] = direction;
  sv.interpolate[voice] = 0;
  sv.finished[voice] = 1;
}

static void test_bounded_one_shot_loops(void) {
  const char *test = "bounded one-shot loops";
  skode_t ctx = new_ctx();
  const int voice = 5;
  event_program_t program;

  expect_int(test, skode_compile_program("v5 BC4", &program),
             SKODE_COMPILE_OK, "compile BC program");
  expect_int(test, skode_execute_program(&program, 0, SAMPLE_COUNT_GET(), 0),
             0, "execute BC program");
  expect_int(test, sv.loop_count[voice], 4, "scheduled BC count");

  configure_loop_test_voice(voice, 0);
  consume(test, &ctx, "v5 BC2 l1");
  expect_int(test, sv.loop_enabled[voice], 1, "BC enables looping");
  expect_int(test, sv.loop_count[voice], 2, "configured loop count");
  expect_int(test, sv.loop_remaining[voice], 2, "triggered loop count");
  consume(test, &ctx, "BC0");
  expect_int(test, sv.loop_count[voice], 0, "updated next loop count");
  expect_int(test, sv.loop_bounded[voice], 1, "active bounded snapshot");

  for (int i = 0; i < 5; i++) osc_next(voice, 1.0f);
  expect_float(test, sv.phase[voice], 2.0f, 0.0001f, "first wrap phase");
  expect_int(test, sv.loop_remaining[voice], 1, "first wrap remaining");

  for (int i = 0; i < 3; i++) osc_next(voice, 1.0f);
  expect_float(test, sv.phase[voice], 2.0f, 0.0001f, "second wrap phase");
  expect_int(test, sv.loop_remaining[voice], 0, "second wrap remaining");

  for (int i = 0; i < 3; i++) osc_next(voice, 1.0f);
  expect_float(test, sv.phase[voice], 5.0f, 0.0001f, "loop exit phase");
  expect_int(test, sv.loop_active[voice], 0, "loop exhausted");
  expect_int(test, sv.loop_ended[voice], 1, "loop exhaustion event");
  expect_int(test, sv.finished[voice], 0, "tail remains active");

  osc_next(voice, 1.0f);
  osc_next(voice, 1.0f);
  osc_next(voice, 1.0f);
  expect_int(test, sv.finished[voice], 1, "tail completion");

  consume(test, &ctx, "BC0 l1");
  sv.phase[voice] = 4.0f;
  consume(test, &ctx, "l0");
  expect_int(test, sv.loop_stop_requested[voice], 0,
             "unbounded release keeps loop running");
  osc_next(voice, 1.0f);
  expect_int(test, sv.loop_active[voice], 1, "unbounded release loop active");
  expect_float(test, sv.phase[voice], 2.0f, 0.0001f,
               "unbounded release wraps");

  consume(test, &ctx, "BC1 l1");
  sv.phase[voice] = 4.0f;
  consume(test, &ctx, "l0");
  expect_int(test, sv.loop_stop_requested[voice], 1,
             "bounded release stop request");
  osc_next(voice, 1.0f);
  expect_int(test, sv.loop_active[voice], 0, "bounded release loop exit");
  expect_float(test, sv.phase[voice], 5.0f, 0.0001f,
               "bounded release exit phase");

  consume(test, &ctx, "BC1 l1");
  sv.phase[voice] = 4.0f;
  osc_next(voice, 1.0f);
  expect_int(test, sv.loop_remaining[voice], 0,
             "bounded toggle setup remaining");
  consume(test, &ctx, "BC0 B0 B1");
  expect_int(test, sv.loop_bounded[voice], 0,
             "B1 refreshes unbounded loop snapshot");
  sv.phase[voice] = 4.0f;
  osc_next(voice, 1.0f);
  expect_int(test, sv.loop_active[voice], 1,
             "B1 unbounded loop stays active");
  expect_float(test, sv.phase[voice], 2.0f, 0.0001f,
               "B1 unbounded loop wraps");

  configure_loop_test_voice(voice, 1);
  consume(test, &ctx, "BC1 l1");
  sv.phase[voice] = 2.0f;
  osc_next(voice, 1.0f);
  expect_float(test, sv.phase[voice], 4.0f, 0.0001f,
               "backward wrap phase");
  expect_int(test, sv.loop_remaining[voice], 0, "backward wrap remaining");
  sv.phase[voice] = 2.0f;
  osc_next(voice, 1.0f);
  expect_float(test, sv.phase[voice], 1.0f, 0.0001f,
               "backward exit phase");
  expect_int(test, sv.loop_active[voice], 0, "backward loop exhausted");

  configure_loop_test_voice(voice, 0);
  consume(test, &ctx, "BC1 l1");
  osc_next(voice, 10.0f);
  expect_float(test, sv.phase[voice], 7.0f, 0.0001f,
               "multi-boundary exit phase");
  expect_int(test, sv.loop_remaining[voice], 0,
             "multi-boundary remaining");
  expect_int(test, sv.loop_active[voice], 0, "multi-boundary loop exhausted");

  wave_loop_count(voice, 3);
  voice_copy(voice, 4);
  expect_int(test, sv.loop_count[4], 3, "copied loop count");
  configure_loop_test_voice(voice, 0);
  consume(test, &ctx, "v5 BC2 B0");
  voice_copy(voice, 4);
  expect_int(test, sv.loop_enabled[4], 0,
             "copy preserves disabled one-shot loop");
  expect_int(test, sv.loop_active[4], 0,
             "copy preserves inactive one-shot loop");
  expect_int(test, sv.loop_count[4], 2,
             "copy preserves next bounded loop count");
  wave_reset(4);
  expect_int(test, sv.loop_count[4], 0, "reset loop count");

  wave_reset(voice);
}

static void test_bounded_loop_releases_envelopes(void) {
  const char *test = "bounded loop envelope release";
  if (!skode_opcode_supported(SKODE_OP_ENVELOPE) ||
      !skode_opcode_supported(SKODE_OP_FILTER_ENVELOPE)) {
    return;
  }

  skode_t ctx = new_ctx();
  const int voice = 6;
  const uint64_t saved_sample_count = SAMPLE_COUNT_GET();
  float output[AUDIO_CHANNELS] = {0};
  char formatted[2048];

  configure_loop_test_voice(voice, 0);
  consume(test, &ctx,
          "v6 a0 BC1 t0,0,1,.5 ft0,0,1,.5 fd100 l1");
  sv.phase[voice] = 4.0f;
  sv.phase_inc[voice] = 1.0f;
  sv.loop_remaining[voice] = 0;
  SAMPLE_COUNT_PUT(saved_sample_count + 100);
  synth(output, NULL, 1, AUDIO_CHANNELS, NULL);

  voice_format(voice, formatted, sizeof(formatted), 1);
  char expected_amp_release[80];
  char expected_filter_release[80];
  snprintf(expected_amp_release, sizeof(expected_amp_release),
           "amp_env_release:%llu",
           (unsigned long long)(saved_sample_count + 100));
  snprintf(expected_filter_release, sizeof(expected_filter_release),
           "filter_env_release:%llu",
           (unsigned long long)(saved_sample_count + 100));
  if (strstr(formatted, expected_amp_release) == NULL ||
      strstr(formatted, expected_filter_release) == NULL) {
    fail(test, "loop exhaustion did not release active envelopes");
  }

  SAMPLE_COUNT_PUT(saved_sample_count);
  wave_reset(voice);
}

static void test_one_shot_asr_mode(void) {
  const char *test = "one-shot ASR mode";

  skode_t ctx = new_ctx();
  const int voice = 5;
  const uint64_t saved_sample_count = SAMPLE_COUNT_GET();

  configure_loop_test_voice(voice, 0);
  sv.loop_enabled[voice] = 0;
  sv.loop_active[voice] = 0;
  sv.phase_inc[voice] = 1.0f;
  consume(test, &ctx, "v5 k1");
  envelope_set(voice, 0.0f, 0.0f, 1.0f, 3.0f / MAIN_SAMPLE_RATE);
  SAMPLE_COUNT_PUT(1000);
  envelope_velocity(voice, 1.0f);
  expect_u64(test, sv.amp_envelope[voice].sample_release, 1005,
             "non-looping one-shot release");

  configure_loop_test_voice(voice, 0);
  sv.phase_inc[voice] = 1.0f;
  consume(test, &ctx, "v5 BC2 k1");
  envelope_set(voice, 0.0f, 0.0f, 1.0f, 3.0f / MAIN_SAMPLE_RATE);
  SAMPLE_COUNT_PUT(2000);
  envelope_velocity(voice, 1.0f);
  expect_u64(test, sv.amp_envelope[voice].sample_release, 2008,
             "counted one-shot release includes loop repeats");

  consume(test, &ctx, "BC0 B1");
  SAMPLE_COUNT_PUT(3000);
  envelope_velocity(voice, 1.0f);
  expect_u64(test, sv.amp_envelope[voice].sample_release, UINT64_MAX,
             "unbounded one-shot keeps held envelope");

  SAMPLE_COUNT_PUT(saved_sample_count);
  wave_reset(voice);
}

static void test_opcode_events(void) {
  const char *test = "opcode events";
  skode_t ctx = new_ctx();
  seq_kill_all();

  consume(test, &ctx, "v0 L0.25 l0.75");
  expect_int(test, seq_queued(), 1, "queued opcode count");

  event_capture_t capture = {0};
  seq_foreach(capture_event, &capture);
  expect_int(test, capture.count, 1, "captured event count");
  expect_int(test, capture.event.voice, 0, "event voice");
  expect_int(test, capture.event.opcode.code,
             SKODE_OP_ENVELOPE_VELOCITY, "event opcode");
  expect_int(test, capture.event.opcode.argc, 1, "opcode argc");
  expect_float(test, (float)capture.event.opcode.arg[0], 0.75f,
               0.0001f, "opcode velocity");

  event_t invalid = capture.event;
  invalid.voice = 1000000;
  expect_int(test, skode_execute_event(&invalid, &ctx), -1,
             "invalid opcode voice");
  invalid = capture.event;
  invalid.opcode.arg[0] = NAN;
  expect_int(test, skode_execute_event(&invalid, &ctx), -1,
             "non-finite opcode argument");
  invalid = capture.event;
  invalid.voice_var = UINT8_MAX;
  expect_int(test, skode_execute_event(&invalid, &ctx), -1,
             "invalid variable voice");

  event_program_t program;
  expect_int(test, skode_compile_program("v2 a-12 p0.25", &program),
             SKODE_COMPILE_OK, "compile scalar program");
  expect_int(test, program.count, 3, "compiled operation count");
  expect_int(test, skode_execute_program(&program, 0, SAMPLE_COUNT_GET(), 0),
             0, "compiled program execution");
  expect_float(test, sv.user_amp[2], -12.0f, 0.0001f, "compiled amp value");
  expect_float(test, sv.pan[2], 0.25f, 0.0001f, "compiled pan value");
  program.count = SEQ_PROGRAM_OP_MAX + 1;
  expect_int(test, skode_execute_program(&program, 0, SAMPLE_COUNT_GET(), 0),
             -1, "oversized program rejection");
  program.count = 0;
  expect_int(test, skode_execute_program(&program, 0, SAMPLE_COUNT_GET(), 0),
             0, "empty program no-op");

  expect_int(test, skode_compile_program("[name] vt", &program),
             SKODE_COMPILE_IMMEDIATE_ONLY,
             "string command fallback");
  expect_int(test, skode_compile_program("(1 2 3) d>r", &program),
             SKODE_COMPILE_IMMEDIATE_ONLY,
             "array command rejection");
  consume(test, &ctx, "[v2 a-11] e>120");
  expect_int(test, skode_compile_program("e!120 p0.25", &program),
             SKODE_COMPILE_OK, "compile external macro");
  expect_int(test, program.count, 3, "external macro operation count");
  expect_int(test, skode_execute_program(&program, 0, SAMPLE_COUNT_GET(), 0),
             0, "execute external macro");
  expect_float(test, sv.user_amp[2], -11.0f, 0.0001f,
               "external macro amplitude");
  expect_float(test, sv.pan[2], 0.25f, 0.0001f,
               "command after external macro");

  if (strstr(skred_features(), "SEQ ") != NULL) {
    consume(test, &ctx, "[v6 a-4] e>115");
    seq_kill_all();
    consume(test, &ctx, "eR115,3,.01,42");
    consume(test, &ctx, "[v6 a-1] e>115");
    repeat_capture_t repeats = {0};
    seq_foreach(capture_repeat, &repeats);
    expect_int(test, repeats.count, 3, "real-time macro repeat count");
    uint64_t real_time_dt = (uint64_t)(0.01 * MAIN_SAMPLE_RATE);
    for (int i = 0; i < 3; i++) {
      expect_int(test, repeats.tag[i], 42, "real-time macro repeat tag");
      expect_int(test, repeats.event[i].voice, 6,
                 "real-time macro repeat voice");
      expect_float(test, repeats.event[i].opcode.arg[0], -4.0f, 0.0001f,
                   "real-time macro repeat snapshot");
      if (i > 0) {
        expect_u64(test, repeats.timestamp[i] - repeats.timestamp[i - 1],
                   real_time_dt, "real-time macro repeat spacing");
      }
    }
    seq_kill_all();

    expect_int(test, tempo_set(120.0f), 0, "macro repeat tempo");
    consume(test, &ctx, "[a-3] e>114");
    consume(test, &ctx, "eRR114,2,.25,7");
    memset(&repeats, 0, sizeof(repeats));
    seq_foreach(capture_repeat, &repeats);
    expect_int(test, repeats.count, 2, "tempo macro repeat count");
    expect_int(test, repeats.tag[0], 7, "tempo macro repeat first tag");
    expect_int(test, repeats.tag[1], 7, "tempo macro repeat second tag");
    expect_u64(test, repeats.timestamp[1] - repeats.timestamp[0],
               (uint64_t)(0.125 * MAIN_SAMPLE_RATE),
               "tempo macro repeat spacing");
    seq_kill_all();
  }

  consume(test, &ctx, "[e!120 n64] e>121");
  expect_int(test, skode_compile_program("e!121", &program),
             SKODE_COMPILE_OK, "compile nested external macro");
  expect_int(test, program.count, 3, "nested external macro operation count");
  expect_int(test, skode_execute_program(&program, 0, SAMPLE_COUNT_GET(), 0),
             0, "execute nested external macro");
  expect_float(test, sv.last_midi_note[2], 64.0f, 0.0001f,
               "nested external macro note");

  expect_int(test, skode_compile_program("e!120", &program),
             SKODE_COMPILE_OK, "compile macro snapshot");
  expect_int(test, seq_step_set(11, 0, "e!120", &program), 0,
             "store macro snapshot in pattern");
  consume(test, &ctx, "[v2 a-2] e>120");
  int macro_pattern_voice = 0;
  expect_int(test, skode_execute_program_state(&seq_program[11][0],
             &macro_pattern_voice, SAMPLE_COUNT_GET(), 0, -1, -1), 0,
             "execute pattern macro snapshot");
  expect_float(test, sv.user_amp[2], -11.0f, 0.0001f,
               "pattern macro snapshot amplitude");

  consume(test, &ctx, "[v7 a-5] e>122");
  seq_kill_all();
  expect_int(test, skode_compile_program("~0.01 e!122", &program),
             SKODE_COMPILE_OK, "compile deferred external macro");
  expect_int(test, skode_execute_program(&program, 0, SAMPLE_COUNT_GET(), 0),
             0, "queue deferred external macro");
  consume(test, &ctx, "[v7 a-1] e>122");
  memset(&capture, 0, sizeof(capture));
  seq_foreach(capture_event, &capture);
  expect_int(test, capture.count, 1, "deferred macro queued event count");
  expect_int(test, capture.event.voice, 7, "deferred macro snapshot voice");
  expect_int(test, capture.event.opcode.code, SKODE_OP_AMP,
             "deferred macro snapshot opcode");
  expect_float(test, capture.event.opcode.arg[0], -5.0f, 0.0001f,
               "deferred macro snapshot value");
  seq_kill_all();

  if (strstr(skred_features(), "SEQ ") != NULL) {
    consume(test, &ctx, "[v3 a-7] e>118");
    pattern_reset(10);
    ctx.pattern = 10;
    consume(test, &ctx, "[e!118] xa");
    expect_int(test, seq_pattern_length[10], 1,
               "append external macro pattern step");
    expect_int(test, seq_program[10][0].count, 2,
               "compiled external macro pattern step");
    consume(test, &ctx, "[v3 a-1] e>118");
    int dispatched_pattern_voice = 0;
    expect_int(test, skode_execute_program_state(&seq_program[10][0],
               &dispatched_pattern_voice, SAMPLE_COUNT_GET(), 0, -1, -1), 0,
               "execute dispatched pattern macro");
    expect_float(test, sv.user_amp[3], -7.0f, 0.0001f,
                 "dispatched pattern macro snapshot");

    consume(test, &ctx, "[v4 a-6] e>119");
    seq_kill_all();
    consume(test, &ctx, "~0.01 e!119");
    expect_int(test, seq_queued(), 1, "dispatcher deferred macro event");
    consume(test, &ctx, "[v4 a-1] e>119");
    memset(&capture, 0, sizeof(capture));
    seq_foreach(capture_event, &capture);
    expect_int(test, capture.event.voice, 4,
               "dispatcher deferred macro voice");
    expect_float(test, capture.event.opcode.arg[0], -6.0f, 0.0001f,
                 "dispatcher deferred macro snapshot");
    seq_kill_all();
  }

  consume(test, &ctx, "[e!124] e>123");
  consume(test, &ctx, "[e!123] e>124");
  expect_int(test, skode_compile_program("e!123", &program),
             SKODE_COMPILE_INVALID, "reject cyclic external macros");
  consume(test, &ctx,
    "[v0 ~0 n69l1 ~.25 n$1l2 ~.5 l2 ~.25 l3 ~.25 n60l2 ~.25 n$0l2] e>116");
  expect_int(test,
             skode_compile_program(
               "=0,72 =1,65 v0 N-24,0 v1 N-7,5 e!116", &program),
             SKODE_COMPILE_OK, "compile expanded external macro");
  expect_int(test, program.count, 23,
             "expanded external macro operation count");
  char macro_command[32];
  for (int i = 90; i < 105; i++) {
    snprintf(macro_command, sizeof(macro_command),
             "[e!%d] e>%d", i + 1, i);
    consume(test, &ctx, macro_command);
  }
  consume(test, &ctx, "[a-1] e>105");
  expect_int(test, skode_compile_program("e!90", &program),
             SKODE_COMPILE_OK, "accept maximum compile nesting");
  expect_int(test, program.count, 1, "maximum nesting operation count");
  consume(test, &ctx, "[e!106] e>105");
  consume(test, &ctx, "[a-1] e>106");
  expect_int(test, skode_compile_program("e!90", &program),
             SKODE_COMPILE_INVALID, "reject excessive compile nesting");
  consume(test, &ctx,
    "[a-1 a-1 a-1 a-1 a-1 a-1 a-1 a-1 a-1 a-1 a-1 a-1 a-1 a-1 a-1 a-1 "
    "a-1 a-1 a-1 a-1 a-1 a-1 a-1 a-1 a-1 a-1 a-1 a-1 a-1 a-1 a-1 a-1] e>125");
  expect_int(test, skode_compile_program("e!125 p0", &program),
             SKODE_COMPILE_TOO_LARGE, "reject oversized macro expansion");
  expect_int(test, skode_compile_program("e!126", &program),
             SKODE_COMPILE_INVALID, "reject undefined external macro");
  expect_int(test, skode_compile_program("e!999", &program),
             SKODE_COMPILE_INVALID, "reject invalid external macro index");
  consume(test, &ctx, "[#] e>117");
  expect_int(test, skode_compile_program("e!117", &program),
             SKODE_COMPILE_OK, "compile no-op external macro");
  expect_int(test, program.count, 0, "no-op external macro operation count");
  expect_int(test, skode_compile_program("e!", &program),
             SKODE_COMPILE_IMMEDIATE_ONLY,
             "argumentless external macro remains immediate");
  expect_int(test, skode_compile_program("e!$0", &program),
             SKODE_COMPILE_IMMEDIATE_ONLY,
             "variable external macro remains immediate");
  expect_int(test, skode_compile_program("#", &program),
             SKODE_COMPILE_OK, "compile sequence no-op");
  expect_int(test, program.count, 0, "sequence no-op operation count");
  expect_int(test, seq_step_set(0, 3, "#", &program), 0,
             "store sequence no-op");
  expect_int(test, seq_step_set(0, SEQ_STEPS_MAX - 1, "#", &program), 0,
             "store final pattern step");
  expect_int(test, seq_step_set(0, SEQ_STEPS_MAX, "#", &program), -1,
             "reject pattern step beyond limit");
  if (strcmp(seq_pattern[0][3], "#") != 0) {
    fail(test, "sequence no-op source was not stored");
  }
  pattern_reset(0);
  pattern_reset(7);
  expect_int(test, seq_step_append(7, "#", &program), 0,
             "append sequence no-op");
  expect_int(test, seq_pattern_length[7], 1,
             "sequence no-op advances pattern length");

  seq_kill_all();
  expect_int(test, skode_compile_program("v1 a-3 ~0.01 n60", &program),
             SKODE_COMPILE_OK, "compile deferred program");
  uint64_t now = SAMPLE_COUNT_GET();
  expect_int(test, skode_execute_program(&program, 0, now, 9), 0,
             "execute deferred program");
  expect_float(test, sv.user_amp[1], -3.0f, 0.0001f,
               "immediate program operation");
  expect_int(test, seq_queued(), 1, "deferred opcode count");
  memset(&capture, 0, sizeof(capture));
  seq_foreach(capture_event, &capture);
  expect_int(test, capture.event.opcode.code, SKODE_OP_MIDI_NOTE,
             "deferred opcode");
  expect_int(test, capture.event.voice, 1, "deferred opcode voice");

  queued_event_ctx = &ctx;
  seq(now + MAIN_SAMPLE_RATE, execute_queued_event, execute_pattern_program);
  expect_float(test, sv.last_midi_note[1], 60.0f, 0.0001f,
               "deferred MIDI execution");

  memset(test_pattern_voice, 0, sizeof(test_pattern_voice));
  expect_int(test, skode_compile_program("v4 a-8", &program),
             SKODE_COMPILE_OK, "compile sequence step");
  expect_int(test, seq_step_set(0, 0, "v4 a-8", &program), 0,
             "store sequence step");
  expect_int(test, skode_compile_program("p0.5", &program),
             SKODE_COMPILE_OK, "compile sequence follow-up");
  expect_int(test, seq_step_set(0, 1, "p0.5", &program), 0,
             "store sequence follow-up");
  expect_int(test, seq_program[0][0].count, 2, "compiled sequence step");
  expect_int(test, seq_program[0][1].count, 1, "compiled follow-up step");
  execute_pattern_program(0, 0, &seq_program[0][0]);
  execute_pattern_program(0, 1, &seq_program[0][1]);
  expect_float(test, sv.user_amp[4], -8.0f, 0.0001f,
               "sequence program amp");
  expect_float(test, sv.pan[4], 0.5f, 0.0001f,
               "persistent sequence voice");

  ctx.log_enable = 1;
  expect_int(test, skode_compile_program("not schedulable", &program),
             SKODE_COMPILE_IMMEDIATE_ONLY,
             "reject immediate-only sequence command");
  expect_int(test, seq_step_set(0, 2, "not schedulable", NULL), -1,
             "reject uncompiled sequence step");
  expect_int(test, seq_program[0][2].count, 0,
             "rejected sequence operation count");
  if (seq_pattern[0][2][0] != '\0') {
    fail(test, "rejected sequence source was stored");
  }

  seq_kill_all();
}

static void test_909_sequence_programs(void) {
  const char *test = "909 sequence programs";
  if (!skode_opcode_supported(SKODE_OP_FILTER_MODE)) return;

  static const char *programs[] = {
    "v2 n69 l1",
    "v3 l1 +.25 v3 l1 +.25 v3 l1 +.25 v3 l1",
    "+.5 v4 l1 +.25 v4l.5",
    "v12 n$0 l1 +.5 v12 n$0 l.75",
    "=0,$1",
    "=1,40 v12 J0",
    "=1,60 v13 J0",
    "=2,55 v12 J1 v13 J1",
    "=2,45 v0 n$5 l2 v12m0 v13m0",
  };
  event_program_t program;
  for (size_t i = 0; i < sizeof(programs) / sizeof(programs[0]); i++) {
    if (skode_compile_program(programs[i], &program) != SKODE_COMPILE_OK) {
      char msg[256];
      snprintf(msg, sizeof(msg), "failed to compile [%s]", programs[i]);
      fail(test, msg);
    }
  }

  global_var[1] = 52;
  expect_int(test, skode_compile_program("=0,$1 v4 n$0", &program),
             SKODE_COMPILE_OK, "compile variable sequence");
  expect_int(test, skode_execute_program(&program, 0, SAMPLE_COUNT_GET(), 0),
             0, "execute variable sequence");
  expect_float(test, (float)global_var[0], 52, 0.0001f,
               "runtime variable assignment");
  expect_float(test, sv.last_midi_note[4], 52, 0.0001f,
               "runtime variable note");

  seq_kill_all();
  global_var[2] = 3;
  global_var[3] = 45;
  expect_int(test, skode_compile_program("v$2 n$3", &program),
             SKODE_COMPILE_OK, "compile queued variable sequence");
  uint64_t now = SAMPLE_COUNT_GET();
  expect_int(test, skode_queue_program(&program, 0, now + 128, 0),
             0, "queue variable sequence");
  global_var[2] = 5;
  global_var[3] = 47;
  queued_event_ctx = NULL;
  seq(now + 256, execute_queued_event, execute_pattern_program);
  expect_float(test, sv.last_midi_note[5], 47, 0.0001f,
               "queued runtime variable resolution");

  seq_kill_all();
  expect_int(test, skode_compile_program("=0,1 =0,2 =0,3", &program),
             SKODE_COMPILE_OK, "compile equal-time register writes");
  now = SAMPLE_COUNT_GET();
  expect_int(test, skode_queue_program(&program, 0, now + 128, 0),
             0, "queue equal-time register writes");
  seq(now + 256, execute_queued_event, execute_pattern_program);
  expect_float(test, (float)global_var[0], 3, 0.0001f,
               "equal-time opcode order");

  global_var[2] = 6;
  int persistent_voice = 0;
  expect_int(test, skode_compile_program("v$2 a-9", &program),
             SKODE_COMPILE_OK, "compile variable pattern voice");
  expect_int(test, skode_execute_program_state(&program, &persistent_voice,
             SAMPLE_COUNT_GET(), 0, -1, -1), 0,
             "execute variable pattern voice");
  expect_int(test, persistent_voice, 6, "persistent variable pattern voice");
  expect_int(test, skode_compile_program("p-.5", &program),
             SKODE_COMPILE_OK, "compile persistent voice follow-up");
  expect_int(test, skode_execute_program_state(&program, &persistent_voice,
             SAMPLE_COUNT_GET(), 0, -1, -1), 0,
             "execute persistent voice follow-up");
  expect_float(test, sv.pan[6], -.5f, 0.0001f,
               "persistent variable voice follow-up");

  global_var[4] = -1;
  expect_int(test, skode_compile_program("v6 ~$4 a-7", &program),
             SKODE_COMPILE_OK, "compile variable defer");
  expect_int(test, skode_execute_program(&program, 0, SAMPLE_COUNT_GET(), 0),
             0, "execute clamped variable defer");
  expect_float(test, sv.user_amp[6], -7, 0.0001f,
               "clamped variable defer execution");

  pattern_reset(9);
  expect_int(test, skode_compile_program("v$2 +.5 n$3", &program),
             SKODE_COMPILE_OK, "compile diagnostic pattern");
  expect_int(test, seq_step_set(9, 0, "v$2 +.5 n$3", &program), 0,
             "store diagnostic pattern");

  skode_t ctx = new_ctx();
  ctx.log_enable = 1;
  ctx.pattern = 9;
  consume(test, &ctx, "?o-1,0");
  if (strstr(ctx.log, "# pattern:9 step:0 source:[v$2 +.5 n$3]") == NULL ||
      strstr(ctx.log, "VOICE $2") == NULL ||
      strstr(ctx.log, "DELAY + 0.5") == NULL ||
      strstr(ctx.log, "MIDI_NOTE $3") == NULL) {
    fail(test, "compiled pattern diagnostic output mismatch");
  }

  seq_kill_all();
  expect_int(test, skode_compile_program("v$2 ~1 n$3", &program),
             SKODE_COMPILE_OK, "compile diagnostic queue event");
  expect_int(test, skode_execute_program(&program, 0,
             SAMPLE_COUNT_GET(), 17), 0, "queue diagnostic event");
  skred_scheduled_event_t scheduled[2];
  expect_int(test, skred_scheduled_event_count(), 1,
             "scheduled event API count");
  expect_int(test, skred_scheduled_event_snapshot(scheduled, 2), 1,
             "scheduled event API snapshot count");
  expect_int(test, scheduled[0].tag, 17, "scheduled event API tag");
  expect_int(test, scheduled[0].voice_var, 3,
             "scheduled event API variable voice");
  expect_int(test, scheduled[0].opcode, SKODE_OP_MIDI_NOTE,
             "scheduled event API opcode");
  expect_int(test, scheduled[0].opcode_var_mask, 1,
             "scheduled event API variable argument mask");
  expect_float(test, scheduled[0].opcode_arg[0], 3.0f, 0.0f,
               "scheduled event API variable argument");
  expect_int(test, skred_scheduled_event_snapshot(NULL, 1), -1,
             "scheduled event API rejects null buffer");
  ctx.log[0] = '\0';
  ctx.log_len = 0;
  consume(test, &ctx, "?o");
  if (strstr(ctx.log, "# opcode queue size:1") == NULL ||
      strstr(ctx.log, "tag:17") == NULL ||
      strstr(ctx.log, "voice:$2 MIDI_NOTE $3") == NULL) {
    fail(test, "queued opcode diagnostic output mismatch");
  }
  ctx.log[0] = '\0';
  ctx.log_len = 0;
  consume(test, &ctx, "?q");
  if (strstr(ctx.log, "# opcode queue size:1") == NULL ||
      strstr(ctx.log, "tag:17") == NULL) {
    fail(test, "scheduled queue diagnostic alias mismatch");
  }
  seq_kill_all();
}

static void test_scalar_voice_opcode_inventory(void) {
  const char *test = "scalar voice opcode inventory";
  struct {
    const char *command;
    skode_opcode_t opcode;
  } cases[] = {
    {"a-6", SKODE_OP_AMP},
    {"A1,.5,0", SKODE_OP_AMP_MOD},
    {"b1", SKODE_OP_WAVE_DIRECTION},
    {"B1", SKODE_OP_WAVE_LOOP},
    {"BC3", SKODE_OP_WAVE_LOOP_COUNT},
    {"c1,.5", SKODE_OP_PHASE_DISTORTION},
    {"C1,.5", SKODE_OP_PHASE_MOD},
    {"f220", SKODE_OP_FREQ},
    {"ft.01,.2,.5,.3", SKODE_OP_FILTER_ENVELOPE},
    {"fd2", SKODE_OP_FILTER_ENVELOPE_DEPTH},
    {"F1,.5,0", SKODE_OP_FREQ_MOD},
    {"FF1", SKODE_OP_FREQ_MOD_MODE},
    {"g.1", SKODE_OP_GLISSANDO},
    {"G1,2", SKODE_OP_LINK_MIDI},
    {"h4", SKODE_OP_SAMPLE_HOLD},
    {"H1,2", SKODE_OP_LINK_VELOCITY},
    {"L.01", SKODE_OP_TRIGGER_DELAY},
    {"J1", SKODE_OP_FILTER_MODE},
    {"K1200", SKODE_OP_FILTER_FREQ},
    {"k1", SKODE_OP_ENVELOPE_MODE},
    {"l1", SKODE_OP_VELOCITY},
    {"m1", SKODE_OP_MUTE},
    {"n60", SKODE_OP_MIDI_NOTE},
    {"N-12,5", SKODE_OP_MIDI_DETUNE},
    {"p.25", SKODE_OP_PAN},
    {"P1,.5,0", SKODE_OP_PAN_MOD},
    {"q8", SKODE_OP_QUANTIZE},
    {"Q.7", SKODE_OP_FILTER_RESONANCE},
    {"r2", SKODE_OP_RECORD_TRACK},
    {"s.01", SKODE_OP_SMOOTHER},
    {"S3", SKODE_OP_VOICE_RESET},
    {"t.01,.2,.5,.3", SKODE_OP_ENVELOPE},
    {"T", SKODE_OP_TRIGGER},
    {"w2,1,0", SKODE_OP_WAVE},
    {">3", SKODE_OP_VOICE_COPY},
    {"/", SKODE_OP_WAVE_DEFAULT},
    {"=0,$1", SKODE_OP_VARIABLE_SET},
    {"XM1,.5", SKODE_OP_RING_MOD},
    {"ce7,1,2,3", SKODE_OP_CONTROL_EVENT},
  };

  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    event_program_t program;
    skode_compile_result_t result =
      skode_compile_program(cases[i].command, &program);
    skode_compile_result_t expected =
      skode_opcode_supported(cases[i].opcode) ?
      SKODE_COMPILE_OK : SKODE_COMPILE_IMMEDIATE_ONLY;
    if (result != expected) {
      char msg[256];
      snprintf(msg, sizeof(msg), "[%s] expected %d, got %d",
        cases[i].command, expected, result);
      fail(test, msg);
    }
  }

  event_program_t program;
  expect_int(test, skode_compile_program("n-", &program),
             SKODE_COMPILE_OK, "compile MIDI default note");
  expect_int(test, skode_compile_program("N-,7", &program),
             SKODE_COMPILE_OK, "compile MIDI transpose default");
}

static void test_parameter_and_buffer_safety(void) {
  const char *test = "parameter and buffer safety";

  struct {
    skode_t ctx;
    unsigned char guard[32];
  } guarded = { .ctx = SKODE_EMPTY() };
  skode_init(&guarded.ctx);
  guarded.ctx.log_enable = 1;
  memset(guarded.guard, 0x5a, sizeof(guarded.guard));

  char first[2001];
  char second[5001];
  memset(first, 'a', sizeof(first) - 1);
  first[sizeof(first) - 1] = '\0';
  memset(second, 'b', sizeof(second) - 1);
  second[sizeof(second) - 1] = '\0';
  skode_printf(&guarded.ctx, "%s", first);
  skode_printf(&guarded.ctx, "%s", second);
  if (guarded.ctx.log_len >= (int)sizeof(guarded.ctx.log)) {
    fail(test, "log length exceeded buffer");
  }
  for (size_t i = 0; i < sizeof(guarded.guard); i++) {
    if (guarded.guard[i] != 0x5a) {
      fail(test, "log write changed guard bytes");
      break;
    }
  }

  guarded.ctx.log[0] = '\0';
  guarded.ctx.log_len = 0;
  guarded.ctx.log_enable = 1;
  skode_printf(&guarded.ctx, "alpha ");
  skode_printf(&guarded.ctx, "beta\n");
  if (strstr(guarded.ctx.log, "alpha beta") == NULL) {
    fail(test, "fragmented printf did not form one log line");
  }

  guarded.ctx.log[0] = '\0';
  guarded.ctx.log_len = 0;
  guarded.ctx.log_enable = 1;
  for (int i = 0; i < SKODE_LOG_LINES + 5; i++) {
    skode_printf(&guarded.ctx, "line-%02d\n", i);
  }
  if (strstr(guarded.ctx.log, "line-00") != NULL ||
      strstr(guarded.ctx.log, "line-68") == NULL ||
      strstr(guarded.ctx.log, "# log dropped 5 lines") == NULL) {
    fail(test, "ring log overflow did not keep recent lines");
  }

  skode_t ctx = new_ctx();
  consume(test, &ctx, "wait");
  consume(test, &ctx, "100000000 >");
  consume(test, &ctx, "v0 G100000000 n60");
  expect_int(test, (int)sv.link_midi_0[0], -1, "invalid MIDI link");
  consume(test, &ctx, "100000000 0 W@");
  consume(test, &ctx, "100000000 W");

  consume(test, &ctx, "(1 2 3) d>r; -100 w>; w!");
  extern synth_sample_t sampling;
  expect_int(test, sampling.offset, 0, "clamped recording offset");
  expect_int(test, sampling.len, 3, "recording length");

  char long_text[400];
  memset(long_text, 'x', sizeof(long_text));
  long_text[0] = '[';
  long_text[sizeof(long_text) - 4] = ']';
  long_text[sizeof(long_text) - 3] = 'v';
  long_text[sizeof(long_text) - 2] = 't';
  long_text[sizeof(long_text) - 1] = '\0';
  consume(test, &ctx, long_text);
  if (strnlen(sv.text[0], TEXT_MAX) >= TEXT_MAX) {
    fail(test, "voice text was not terminated");
  }
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

static void test_ands_macro_commands(void) {
  const char *test = "ands macro commands";
  skode_t ctx = new_ctx();
  ctx.log_enable = 1;

  consume(test, &ctx, "[ar]: t $$0 0 $$1 0 ; [zz]: f $$0 ;");
  expect_int(test, ands_macro_count(ctx.parse), 2, "macro count after define");

  ctx.log[0] = '\0';
  ctx.log_len = 0;
  consume(test, &ctx, "?m");
  if (!strstr(ctx.log, "[ar]") || !strstr(ctx.log, "[zz]")) {
    fail(test, "macro listing missing definitions");
  }

  consume(test, &ctx, "[ar] /m");
  expect_int(test, ands_macro_count(ctx.parse), 1, "macro count after remove");

  consume(test, &ctx, "/m!");
  expect_int(test, ands_macro_count(ctx.parse), 0, "macro count after clear");
}

static void test_control_composition_primitives(void) {
  const char *test = "control composition primitives";
  skode_t ctx = new_ctx();
  ctx.log_enable = 1;

  consume(test, &ctx, "0 5 swap v");
  expect_int(test, ctx.voice, 5, "swap feeds next command");
  consume(test, &ctx, "7 3 drop v");
  expect_int(test, ctx.voice, 3, "drop feeds next command");
  consume(test, &ctx, "4 2 over v");
  expect_int(test, ctx.voice, 2, "over feeds next command");
  consume(test, &ctx, "1 6 7 rot v");
  expect_int(test, ctx.voice, 6, "rot feeds next command");
  consume(test, &ctx, "5 dup v");
  expect_int(test, ctx.voice, 5, "dup feeds next command");
  consume(test, &ctx, "2 clr v");
  expect_int(test, ctx.voice, 5, "clr leaves next command without args");

  ctx.log[0] = '\0';
  ctx.log_len = 0;
  consume(test, &ctx, "[alpha] 0 s> [beta] ?s");
  if (!strstr(ctx.log, "# [beta]")) fail(test, "parser string baseline missing");
  ctx.log[0] = '\0';
  ctx.log_len = 0;
  consume(test, &ctx, "0 <s ?s");
  if (!strstr(ctx.log, "# [alpha]")) fail(test, "local string slot restore failed");

  ctx.log[0] = '\0';
  ctx.log_len = 0;
  consume(test, &ctx, "[slot-@0-@1] 3 4 s% ?s");
  if (!strstr(ctx.log, "# [slot-3-4]")) fail(test, "runtime string format failed");

  consume(test, &ctx, "0 3 4 a= a");
  expect_float(test, sv.user_amp[ctx.voice], 7.0f, 0.0001f,
               "arithmetic return feeds amp");
  consume(test, &ctx, "11 6 = a");
  expect_float(test, sv.user_amp[ctx.voice], 6.0f, 0.0001f,
               "register set return feeds amp");
  consume(test, &ctx, "(11 12) 1 d@ a");
  expect_float(test, sv.user_amp[ctx.voice], 12.0f, 0.0001f,
               "data read return feeds amp");
}

static void test_tempo_and_pattern_reset_limits(void) {
  const char *test = "tempo and pattern reset limits";
  expect_int(test, tempo_set(120.0f), 0, "set normal tempo");
  expect_float(test, tempo_bpm_get(), 120.0f, 0.001f, "normal tempo");
  expect_int(test, tempo_set(0.0f), -1, "reject zero tempo");
  expect_int(test, tempo_set(-1.0f), -1, "reject negative tempo");
  expect_int(test, tempo_set(NAN), -1, "reject non-finite tempo");
  expect_int(test, tempo_set(961.0f), -1, "reject excessive tempo");
  expect_float(test, tempo_bpm_get(), 120.0f, 0.001f,
               "rejected tempo preserves current value");
  expect_int(test, tempo_set(1.0f), 0, "accept minimum tempo");
  expect_int(test, tempo_set(960.0f), 0, "accept maximum tempo");

  event_program_t program = {0};
  pattern_reset(12);
  int generation = seq_pattern_generation(12);
  expect_int(test, seq_step_set(12, 0, "#", &program), 0,
             "store catch-up pattern");
  seq_modulo_set(12, 1);
  seq_state_set(12, 1);
  seq_rewind();
  pattern_callback_count = 0;
  seq((uint64_t)MAIN_SAMPLE_RATE * 100, ignore_queued_event,
      count_pattern_program);
  expect_int(test, pattern_callback_count, SEQ_MAX_CATCHUP_TICKS,
             "bounded pattern catch-up");

  pattern_reset(12);
  expect_int(test, seq_pattern_generation(12), generation + 1,
             "pattern clear generation");
  expect_int(test, seq_state[12], SEQ_STOPPED, "cleared pattern state");
  expect_int(test, seq_pattern_length[12], 0, "cleared pattern length");
  expect_int(test, tempo_set(120.0f), 0, "restore default tempo");
}

static void test_sample_accurate_sequence_boundaries(void) {
  const char *test = "sample accurate sequence boundaries";
  event_t event = {0};
  uint64_t boundary = 0;

  seq_kill_all();
  expect_int(test, tempo_set(120.0f), 0, "set boundary tempo");
  seq_rewind();
  uint64_t now = SAMPLE_COUNT_GET();
  expect_int(test, queue_event(now + 37, &event, 0), 0,
             "queue in-block event");
  expect_int(test, seq_next_boundary(now, now + 128, &boundary), 1,
             "find queued boundary");
  expect_u64(test, boundary, now + 37, "queued boundary sample");
  seq_kill_all();
  expect_int(test, seq_queued(), 0, "clear queued boundary");

  event_program_t program = {0};
  pattern_reset(13);
  expect_int(test, seq_step_set(13, 0, "#", &program), 0,
             "store boundary pattern");
  seq_modulo_set(13, 1);
  seq_state_set(13, 1);
  seq_rewind();
  now = SAMPLE_COUNT_GET();
  expect_int(test, seq_next_boundary(now, now + 6000, &boundary), 1,
             "find 120 BPM tick");
  expect_u64(test, boundary, now + 5513, "120 BPM tick sample");
  seq(boundary, ignore_queued_event, count_pattern_program);
  expect_u64(test, seq_master_tick(), 1, "120 BPM tick dispatch");

  expect_int(test, tempo_set(123.0f), 0, "set fractional boundary tempo");
  seq_rewind();
  now = SAMPLE_COUNT_GET();
  expect_int(test, seq_next_boundary(now, now + 6000, &boundary), 1,
             "find fractional tick");
  expect_u64(test, boundary, now + 5379, "fractional tick sample");
  seq(boundary, ignore_queued_event, count_pattern_program);
  expect_u64(test, seq_master_tick(), 1, "fractional tick dispatch");
  pattern_reset(13);
  expect_int(test, tempo_set(120.0f), 0, "restore boundary tempo");
}

static void test_silent_voice_fast_path(void) {
  const char *test = "silent voice fast path";
  enum { FRAMES = 16 };
  float output[FRAMES * AUDIO_CHANNELS] = {0};

  wave_reset(7);
  float phase = sv.phase[7];
  synth(output, NULL, FRAMES, AUDIO_CHANNELS, NULL);

  expect_float(test, sv.phase[7], phase, 0.0f,
               "reset voice oscillator remains idle");
  expect_float(test, sv.sample[7], 0.0f, 0.0f,
               "reset voice output remains zero");
}

static void test_control_plane_voice_events(void) {
  const char *test = "control plane voice events";
  skred_control_event_t events[8];
  event_program_t program = {0};
  int voice = 0;
  float output[AUDIO_CHANNELS] = {0};
  uint64_t saved_sample_count = SAMPLE_COUNT_GET();
  skode_t ctx = new_ctx();
  ctx.log_enable = 1;

  skred_control_event_reset();
  voice_trigger(2);
  expect_int(test, skred_control_event_poll(events, 4), 0,
             "voice trigger event default disabled");

  consume(test, &ctx, "v2 vc1");
  voice_trigger(2);
  expect_int(test, skred_control_event_poll(events, 4), 1,
             "voice trigger event count");
  expect_int(test, (int)events[0].type, SKRED_CONTROL_EVENT_VOICE_TRIGGER,
             "voice trigger event type");
  expect_int(test, events[0].voice, 2, "voice trigger event voice");
  expect_int(test, events[0].pattern, -1, "voice trigger default pattern");
  expect_u64(test, events[0].sample, SAMPLE_COUNT_GET(),
             "voice trigger event sample");

  envelope_velocity(2, 0.0f);
  expect_int(test, skred_control_event_poll(events, 4), 1,
             "voice release event count");
  expect_int(test, (int)events[0].type, SKRED_CONTROL_EVENT_VOICE_RELEASE,
             "voice release event type");
  expect_int(test, events[0].voice, 2, "voice release event voice");
  consume(test, &ctx, "vc0");
  voice_trigger(2);
  expect_int(test, skred_control_event_poll(events, 4), 0,
             "voice trigger event disabled after enable");
  ctx.log[0] = '\0';
  ctx.log_len = 0;
  consume(test, &ctx, "?");
  if (strstr(ctx.log, "vc0") != NULL) {
    fail(test, "disabled voice control event flag should be omitted");
  }
  consume(test, &ctx, "vc1");
  ctx.log[0] = '\0';
  ctx.log_len = 0;
  consume(test, &ctx, "?");
  if (strstr(ctx.log, "vc1") == NULL) {
    fail(test, "enabled voice control event flag missing from voice show");
  }

  skred_control_event_reset();
  consume(test, &ctx, "v3 vc1");
  expect_int(test, skode_compile_program("v3 l1", &program),
             SKODE_COMPILE_OK, "compile pattern trigger program");
  expect_int(test, skode_execute_program_state(&program, &voice,
             SAMPLE_COUNT_GET(), 44, 9, 5), 0,
             "execute pattern trigger program");
  expect_int(test, skred_control_event_poll(events, 4), 1,
             "pattern trigger event count");
  expect_int(test, (int)events[0].type, SKRED_CONTROL_EVENT_VOICE_TRIGGER,
             "pattern trigger event type");
  expect_int(test, events[0].voice, 3, "pattern trigger event voice");
  expect_int(test, events[0].pattern, 9, "pattern trigger event pattern");
  expect_int(test, events[0].step, 5, "pattern trigger event step");
  expect_int(test, events[0].tag, 44, "pattern trigger event tag");
  expect_int(test, (int)events[0].opcode, SKODE_OP_VELOCITY,
             "pattern trigger event opcode");

  skred_control_event_reset();
  consume(test, &ctx, "v4 vc1");
  voice_trigger(4);
  consume(test, &ctx, "?ce");
  if (strstr(ctx.log, "# control events:1") == NULL ||
      strstr(ctx.log, "type:VOICE_TRIGGER") == NULL ||
      strstr(ctx.log, "voice:4") == NULL) {
    fail(test, "control-plane event diagnostic output mismatch");
  }
  ctx.log[0] = '\0';
  ctx.log_len = 0;
  consume(test, &ctx, "?ce");
  if (strstr(ctx.log, "# control events empty") == NULL) {
    fail(test, "empty control-plane event diagnostic output mismatch");
  }

  skred_control_event_reset();
  consume(test, &ctx, "v5 ce 42,1.5,2.5");
  expect_int(test, skred_control_event_poll(events, 8), 1,
             "immediate user control event count");
  expect_int(test, (int)events[0].type, SKRED_CONTROL_EVENT_USER,
             "immediate user control event type");
  expect_int(test, events[0].voice, 5, "immediate user control event voice");
  expect_int(test, events[0].id, 42, "immediate user control event id");
  expect_int(test, (int)events[0].value_count, 2,
             "immediate user control event value count");
  expect_float(test, (float)events[0].value[0], 1.5f, 0.0f,
               "immediate user control event value 0");
  expect_float(test, (float)events[0].value[1], 2.5f, 0.0f,
               "immediate user control event value 1");

  skred_control_event_reset();
  expect_int(test, skode_compile_program("ce 9,3,4,5", &program),
             SKODE_COMPILE_OK, "compile user control event program");
  expect_int(test, skode_execute_program_state(&program, &voice,
             SAMPLE_COUNT_GET(), 77, 11, 6), 0,
             "execute user control event program");
  expect_int(test, skred_control_event_poll(events, 8), 1,
             "pattern user control event count");
  expect_int(test, (int)events[0].type, SKRED_CONTROL_EVENT_USER,
             "pattern user control event type");
  expect_int(test, events[0].pattern, 11,
             "pattern user control event pattern");
  expect_int(test, events[0].step, 6, "pattern user control event step");
  expect_int(test, events[0].tag, 77, "pattern user control event tag");
  expect_int(test, events[0].id, 9, "pattern user control event id");
  expect_int(test, (int)events[0].value_count, 3,
             "pattern user control event value count");

  skred_control_event_reset();
  pattern_reset(14);
  ctx.pattern = 14;
  consume(test, &ctx, "yc1");
  expect_int(test, seq_control_events[14], 1, "pattern events enabled");
  expect_int(test, skode_compile_program("ce 12,8", &program),
             SKODE_COMPILE_OK, "compile pattern event step");
  expect_int(test, seq_step_set(14, 0, "ce 12,8", &program), 0,
             "store pattern event step");
  seq_modulo_set(14, 1);
  seq_state_set(14, 1);
  seq_rewind();
  seq_step_goto(14, 0);
  SAMPLE_COUNT_PUT(saved_sample_count + 5513);
  seq(SAMPLE_COUNT_GET(), execute_queued_event, execute_pattern_program);
  expect_int(test, skred_control_event_poll(events, 8), 3,
             "pattern boundary event count");
  expect_int(test, (int)events[0].type, SKRED_CONTROL_EVENT_PATTERN_START,
             "pattern start event type");
  expect_int(test, events[0].pattern, 14, "pattern start event pattern");
  expect_int(test, events[0].step, 0, "pattern start event step");
  expect_int(test, (int)events[1].type, SKRED_CONTROL_EVENT_USER,
             "pattern scheduled user event type");
  expect_int(test, events[1].pattern, 14,
             "pattern scheduled user event pattern");
  expect_int(test, events[1].step, 0, "pattern scheduled user event step");
  expect_int(test, events[1].id, 12, "pattern scheduled user event id");
  expect_int(test, (int)events[2].type, SKRED_CONTROL_EVENT_PATTERN_END,
             "pattern end event type");
  ctx.log[0] = '\0';
  ctx.log_len = 0;
  consume(test, &ctx, "z?");
  if (strstr(ctx.log, "yc1") == NULL) {
    fail(test, "enabled pattern control event flag missing from pattern show");
  }
  consume(test, &ctx, "yc0");
  ctx.log[0] = '\0';
  ctx.log_len = 0;
  consume(test, &ctx, "z?");
  if (strstr(ctx.log, "yc0") != NULL || strstr(ctx.log, "yc1") != NULL) {
    fail(test, "disabled pattern control event flag should be omitted");
  }
  pattern_reset(14);
  SAMPLE_COUNT_PUT(saved_sample_count);

  skred_control_event_reset();
  voice_control_events_set(0, 1);
  amp_set(0, 0.0f);
  envelope_set(0, 1.0f, 1.0f, 0.0f, 0.0f);
  envelope_velocity(0, 1.0f);
  expect_int(test, skred_control_event_poll(events, 4), 1,
             "finite ADSR trigger event count");
  SAMPLE_COUNT_PUT(sv.amp_envelope[0].sample_start +
                   (uint64_t)(2 * MAIN_SAMPLE_RATE));
  synth(output, NULL, 1, AUDIO_CHANNELS, NULL);
  expect_int(test, skred_control_event_poll(events, 4), 1,
             "finite ADSR finished event count");
  expect_int(test, (int)events[0].type, SKRED_CONTROL_EVENT_VOICE_FINISHED,
             "finite ADSR finished event type");
  SAMPLE_COUNT_PUT(saved_sample_count);
  wave_reset(0);
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
  test_named_wave_destination();
  test_midi_and_links();
  test_trigger_delay_lifecycle();
  test_envelope_configuration_is_deferred();
  test_envelope_future_timestamps();
  test_bounded_one_shot_loops();
  test_bounded_loop_releases_envelopes();
  test_one_shot_asr_mode();
  test_opcode_events();
  test_909_sequence_programs();
  test_scalar_voice_opcode_inventory();
  test_parameter_and_buffer_safety();
  test_context_modes();
  test_ands_macro_commands();
  test_control_composition_primitives();
  test_tempo_and_pattern_reset_limits();
  test_sample_accurate_sequence_boundaries();
  test_silent_voice_fast_path();
  test_control_plane_voice_events();

  synth_free();

  if (failures) {
    fprintf(stderr, "%d SKODE state test failure(s)\n", failures);
    return 1;
  }
  printf("SKODE state tests passed\n");
  return 0;
}
