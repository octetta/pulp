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

static void execute_pattern_program(int pattern, const event_program_t *program) {
  if (pattern < 0 || pattern >= PATTERNS_MAX ||
      skode_execute_program_state(program, &test_pattern_voice[pattern],
        SAMPLE_COUNT_GET(), -1) != 0) {
    fail("opcode events", "pattern program execution failed");
  }
}

static void count_pattern_program(int pattern, const event_program_t *program) {
  (void)pattern;
  (void)program;
  pattern_callback_count++;
}

static void ignore_queued_event(const event_t *event) {
  (void)event;
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
             &macro_pattern_voice, SAMPLE_COUNT_GET(), 0), 0,
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
               &dispatched_pattern_voice, SAMPLE_COUNT_GET(), 0), 0,
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
  execute_pattern_program(0, &seq_program[0][0]);
  execute_pattern_program(0, &seq_program[0][1]);
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
             SAMPLE_COUNT_GET(), 0), 0, "execute variable pattern voice");
  expect_int(test, persistent_voice, 6, "persistent variable pattern voice");
  expect_int(test, skode_compile_program("p-.5", &program),
             SKODE_COMPILE_OK, "compile persistent voice follow-up");
  expect_int(test, skode_execute_program_state(&program, &persistent_voice,
             SAMPLE_COUNT_GET(), 0), 0, "execute persistent voice follow-up");
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
  ctx.log[0] = '\0';
  ctx.log_len = 0;
  consume(test, &ctx, "?o");
  if (strstr(ctx.log, "# opcode queue size:1") == NULL ||
      strstr(ctx.log, "tag:17") == NULL ||
      strstr(ctx.log, "voice:$2 MIDI_NOTE $3") == NULL) {
    fail(test, "queued opcode diagnostic output mismatch");
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
  test_opcode_events();
  test_909_sequence_programs();
  test_scalar_voice_opcode_inventory();
  test_parameter_and_buffer_safety();
  test_context_modes();
  test_tempo_and_pattern_reset_limits();

  synth_free();

  if (failures) {
    fprintf(stderr, "%d SKODE state test failure(s)\n", failures);
    return 1;
  }
  printf("SKODE state tests passed\n");
  return 0;
}
