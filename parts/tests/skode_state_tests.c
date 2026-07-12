#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ands.h"
#include "api.h"
#include "control-events.h"
#include "skode.h"
#include "seq.h"
#include "synth.h"
#include "synth-state.h"
#include "miniz_zip.h"

static int failures = 0;

extern int wave_load_string(skode_t *ctx, char *name, int wave_index,
                            int ch, int normalize);
extern int wave_load(skode_t *ctx, int file_num, int wave_index,
                     int ch, int normalize);
extern int rec_load(skode_t *ctx, int wave_slot, int one_shot, float offset);

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

static void expect_substr(const char *test, const char *haystack,
                          const char *needle, const char *label) {
  if (!haystack || !needle || !strstr(haystack, needle)) {
    char msg[160];
    snprintf(msg, sizeof(msg), "%s missing [%s]", label, needle ? needle : "");
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

static void reset_log(skode_t *ctx) {
  ctx->log[0] = '\0';
  ctx->log_len = 0;
  ctx->log_head = 0;
  ctx->log_count = 0;
  ctx->log_dropped = 0;
  ctx->log_pending[0] = '\0';
  ctx->log_pending_len = 0;
}

static void write_u16le(FILE *f, unsigned value) {
  fputc((int)(value & 0xffu), f);
  fputc((int)((value >> 8) & 0xffu), f);
}

static void write_u32le(FILE *f, uint32_t value) {
  fputc((int)(value & 0xffu), f);
  fputc((int)((value >> 8) & 0xffu), f);
  fputc((int)((value >> 16) & 0xffu), f);
  fputc((int)((value >> 24) & 0xffu), f);
}

static void write_tag(FILE *f, const char *tag) {
  fwrite(tag, 1, 4, f);
}

static int write_smpl_test_wav(const char *path, uint32_t frames,
                               uint32_t loop_start, uint32_t loop_end,
                               uint32_t loop_type, uint32_t play_count) {
  const uint32_t data_bytes = frames * 2;
  const uint32_t smpl_bytes = 36 + 24;
  const uint32_t riff_size = 4 + (8 + 16) + (8 + data_bytes) + (8 + smpl_bytes);
  FILE *f = fopen(path, "wb");
  if (!f) return -1;

  write_tag(f, "RIFF");
  write_u32le(f, riff_size);
  write_tag(f, "WAVE");

  write_tag(f, "fmt ");
  write_u32le(f, 16);
  write_u16le(f, 1);
  write_u16le(f, 1);
  write_u32le(f, MAIN_SAMPLE_RATE);
  write_u32le(f, MAIN_SAMPLE_RATE * 2);
  write_u16le(f, 2);
  write_u16le(f, 16);

  write_tag(f, "data");
  write_u32le(f, data_bytes);
  for (uint32_t i = 0; i < frames; i++) {
    write_u16le(f, (unsigned)(i * 1000u));
  }

  write_tag(f, "smpl");
  write_u32le(f, smpl_bytes);
  write_u32le(f, 0);
  write_u32le(f, 0);
  write_u32le(f, 1000000000u / MAIN_SAMPLE_RATE);
  write_u32le(f, 69);
  write_u32le(f, 0);
  write_u32le(f, 0);
  write_u32le(f, 0);
  write_u32le(f, 1);
  write_u32le(f, 0);
  write_u32le(f, 0);
  write_u32le(f, loop_type);
  write_u32le(f, loop_start);
  write_u32le(f, loop_end);
  write_u32le(f, 0);
  write_u32le(f, play_count);

  if (fclose(f) != 0) return -1;
  return 0;
}

static void *read_whole_file(const char *path, size_t *size) {
  FILE *f = fopen(path, "rb");
  void *data = NULL;
  long len;
  if (size) *size = 0;
  if (!f) return NULL;
  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return NULL;
  }
  len = ftell(f);
  if (len < 0 || fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return NULL;
  }
  data = malloc((size_t)len);
  if (!data) {
    fclose(f);
    return NULL;
  }
  if (fread(data, 1, (size_t)len, f) != (size_t)len) {
    free(data);
    data = NULL;
  } else if (size) {
    *size = (size_t)len;
  }
  fclose(f);
  return data;
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
static int foreign_call_count;
static skred_foreign_call_t foreign_last_call;
static double foreign_arg[8];
static double foreign_data[8];
static char foreign_string[128];

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

static int capture_foreign_call(const skred_foreign_call_t *call, void *user) {
  (void)user;
  foreign_call_count++;
  foreign_last_call = *call;
  for (int i = 0; i < call->argc && i < 8; i++) foreign_arg[i] = call->arg[i];
  for (int i = 0; i < call->data_len && i < 8; i++) foreign_data[i] = call->data[i];
  snprintf(foreign_string, sizeof(foreign_string), "%s",
           call->string ? call->string : "");
  foreign_last_call.arg = foreign_arg;
  foreign_last_call.data = foreign_data;
  foreign_last_call.string = foreign_string;
  return 0;
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

static void test_command_help(void) {
  const char *test = "command help";
  skode_t ctx = new_ctx();
  ctx.log_enable = 1;

  consume(test, &ctx, "/h");
  expect_substr(test, ctx.log, "# help categories", "help categories");
  expect_substr(test, ctx.log, "parser", "help parser category");

  reset_log(&ctx);
  consume(test, &ctx, "/h 1");
  expect_substr(test, ctx.log, "# help parser", "numeric category help");
  expect_substr(test, ctx.log, "wait", "numeric category command");

  reset_log(&ctx);
  consume(test, &ctx, "/h 1,1");
  expect_substr(test, ctx.log, "# help wait", "numeric command help");
  expect_substr(test, ctx.log, "blocking msec wait", "numeric command summary");

  reset_log(&ctx);
  consume(test, &ctx, "[files] /h");
  expect_substr(test, ctx.log, "# help files", "string category help");
  expect_substr(test, ctx.log, "/ls", "string category command");

  reset_log(&ctx);
  consume(test, &ctx, "[/ls] /h");
  expect_substr(test, ctx.log, "# help /ls", "string command help");
  expect_substr(test, ctx.log, "skode-load-string filename", "string command summary");
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
  sv.wave_range_start[voice] = 0;
  sv.wave_range_end[voice] = 8;
  sv.wave_range_start_f[voice] = 0.0f;
  sv.wave_range_end_f[voice] = 8.0f;
  sv.wave_range_override[voice] = 0;
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
  float output[AUDIO_CHANNELS] = {0};
  skred_control_event_t events[4];

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
  expect_int(test, sv.loop_release_tail[voice], 1,
             "bounded loop exits into sample tail");

  osc_next(voice, 1.0f);
  osc_next(voice, 1.0f);
  osc_next(voice, 1.0f);
  expect_int(test, sv.finished[voice], 1, "tail completion");

  consume(test, &ctx, "BC0 l1");
  sv.phase[voice] = 4.0f;
  consume(test, &ctx, "l0");
  expect_int(test, sv.loop_stop_requested[voice], 1,
             "unbounded release stop request");
  expect_u64(test, sv.amp_envelope[voice].sample_release, UINT64_MAX,
             "one-shot loop release keeps amp envelope held");
  osc_next(voice, 1.0f);
  expect_int(test, sv.loop_active[voice], 0, "unbounded release loop exit");
  expect_float(test, sv.phase[voice], 5.0f, 0.0001f,
               "unbounded release exit phase");
  expect_int(test, sv.finished[voice], 0,
             "unbounded release tail remains active");
  expect_float(test, amp_envelope_step(voice, SAMPLE_COUNT_GET()), 1.0f,
               0.0001f, "unbounded release tail remains audible");
  expect_u64(test, sv.amp_envelope[voice].sample_release, UINT64_MAX,
             "loop exit keeps amp envelope held for tail");

  configure_loop_test_voice(voice, 1);
  consume(test, &ctx, "BC0 l1");
  sv.phase[voice] = 2.0f;
  consume(test, &ctx, "l0");
  expect_int(test, sv.loop_stop_requested[voice], 1,
             "backward unbounded release stop request");
  osc_next(voice, 1.0f);
  expect_int(test, sv.loop_active[voice], 0,
             "backward unbounded release loop exit");
  expect_float(test, sv.phase[voice], 1.0f, 0.0001f,
               "backward unbounded release exit phase");

  configure_loop_test_voice(voice, 0);
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
  consume(test, &ctx, "v5 a0 vc1 BC1 l1");
  expect_int(test, skred_control_event_clear(), 1,
             "clear trigger before loop exhaustion event");
  sv.phase[voice] = 4.0f;
  sv.phase_inc[voice] = 1.0f;
  sv.loop_remaining[voice] = 0;
  synth(output, NULL, 1, AUDIO_CHANNELS, NULL);
  expect_int(test, skred_control_event_poll(events, 4), 1,
             "loop exhaustion release event count");
  expect_int(test, (int)events[0].type, SKRED_CONTROL_EVENT_VOICE_RELEASE,
             "loop exhaustion release event type");
  expect_int(test, events[0].voice, voice,
             "loop exhaustion release event voice");

  configure_loop_test_voice(voice, 0);
  consume(test, &ctx, "v5 a0 vc1 BC0 l1");
  expect_int(test, skred_control_event_clear(), 1,
             "clear trigger before l0 release event");
  sv.phase[voice] = 4.0f;
  sv.phase_inc[voice] = 1.0f;
  consume(test, &ctx, "l0");
  expect_int(test, skred_control_event_poll(events, 4), 1,
             "l0 immediate release event count");
  expect_int(test, (int)events[0].type, SKRED_CONTROL_EVENT_VOICE_RELEASE,
             "l0 immediate release event type");
  synth(output, NULL, 1, AUDIO_CHANNELS, NULL);
  expect_int(test, skred_control_event_poll(events, 4), 0,
             "l0 boundary release event is not duplicated");

  configure_loop_test_voice(voice, 0);
  consume(test, &ctx, "BC1 l1");
  osc_next(voice, 10.0f);
  expect_float(test, sv.phase[voice], 7.0f, 0.0001f,
               "multi-boundary exit phase");
  expect_int(test, sv.loop_remaining[voice], 0,
             "multi-boundary remaining");
  expect_int(test, sv.loop_active[voice], 0, "multi-boundary loop exhausted");

  configure_loop_test_voice(voice, 0);
  consume(test, &ctx, "b2 BC0 l1");
  sv.phase[voice] = 4.0f;
  osc_next(voice, 1.0f);
  expect_float(test, sv.phase[voice], 5.0f, 0.0001f,
               "ping-pong forward boundary phase");
  expect_int(test, sv.pingpong_reverse[voice], 1,
             "ping-pong reverses at loop end");
  osc_next(voice, 1.0f);
  expect_float(test, sv.phase[voice], 4.0f, 0.0001f,
               "ping-pong backward phase");
  sv.phase[voice] = 2.0f;
  osc_next(voice, 1.0f);
  expect_float(test, sv.phase[voice], 3.0f, 0.0001f,
               "ping-pong reverses at loop start");
  expect_int(test, sv.pingpong_reverse[voice], 0,
             "ping-pong forward after loop start");

  configure_loop_test_voice(voice, 0);
  consume(test, &ctx, "b2 BC2 l1");
  sv.phase[voice] = 4.0f;
  osc_next(voice, 1.0f);
  expect_int(test, sv.loop_remaining[voice], 1,
             "ping-pong first traversal remaining");
  sv.phase[voice] = 2.0f;
  osc_next(voice, 1.0f);
  expect_int(test, sv.loop_remaining[voice], 0,
             "ping-pong second traversal remaining");
  sv.phase[voice] = 4.0f;
  osc_next(voice, 1.0f);
  expect_int(test, sv.loop_active[voice], 0,
             "ping-pong third traversal exits loop");
  expect_int(test, sv.loop_ended[voice], 1,
             "ping-pong bounded loop release event");

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

static void test_wave_loop_points(void) {
  const char *test = "wave loop points";
  static float wave_data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  char formatted[512];
  skode_t ctx = new_ctx();
  const int wave = 300;
  const int voice = 6;

  sw.data[wave] = wave_data;
  sw.size[wave] = 10;
  sw.rate[wave] = MAIN_SAMPLE_RATE;
  sw.one_shot[wave] = 1;
  sw.loop_enabled[wave] = 1;
  sw.loop_start[wave] = 0;
  sw.loop_end[wave] = 10;
  sw.readonly[wave] = 0;
  sw.is_heap[wave] = 0;

  consume(test, &ctx, "v6 w300");
  expect_int(test, sv.loop_start[voice], 0, "initial voice loop start");
  expect_int(test, sv.loop_end[voice], 10, "initial voice loop end");
  expect_int(test, sv.loop_override[voice], 0, "initial voice loop override");

  consume(test, &ctx, "WL300,3,8");
  expect_int(test, sw.loop_start[wave], 3, "wave loop start");
  expect_int(test, sw.loop_end[wave], 8, "wave loop end");
  expect_int(test, sv.loop_start[voice], 3, "voice loop start updated");
  expect_int(test, sv.loop_end[voice], 8, "voice loop end updated");
  expect_int(test, sv.loop_valid[voice], 1, "voice loop valid");
  expect_int(test, sv.loop_length[voice], 5, "voice loop length");

  consume(test, &ctx, "W@300,3");
  expect_float(test, (float)ands_arg(ctx.parse)[0], 3.0f, 0.0001f,
               "W@ loop start");
  consume(test, &ctx, "W@300,4");
  expect_float(test, (float)ands_arg(ctx.parse)[0], 8.0f, 0.0001f,
               "W@ loop end");

  consume(test, &ctx, "WL300,8,3");
  expect_int(test, sw.loop_start[wave], 3, "invalid WL keeps start");
  expect_int(test, sw.loop_end[wave], 8, "invalid WL keeps end");

  ctx.log_enable = 1;
  ctx.log[0] = '\0';
  ctx.log_len = 0;
  consume(test, &ctx, "GS1");
  expect_substr(test, ctx.log, "WL300,3,8",
                "GS1 includes wave loop points");
  ctx.log_enable = 0;

  ctx.log_enable = 1;
  ctx.log[0] = '\0';
  ctx.log_len = 0;
  consume(test, &ctx, "W300,8,2");
  expect_substr(test, ctx.log, "one-shot 1",
                "wave display includes one-shot state");
  expect_substr(test, ctx.log, "loop 3..8 |5|",
                "wave display includes loop points");
  expect_substr(test, ctx.log, "loop [3..8)",
                "wave display includes loop marker legend");
  expect_substr(test, ctx.log, "baseline",
                "wave display includes baseline duration");
  ctx.log_enable = 0;

  extern synth_sample_t sampling;
  static float rec_data[10] = {0, 0, 1, 2, 3, 4, 5, 0, 0, 0};
  sampling.where = rec_data;
  sampling.len = 10;
  sampling.capacity = 10;
  sampling.offset = 2;
  sampling.trim = 3;
  ctx.log_enable = 1;
  ctx.log[0] = '\0';
  ctx.log_len = 0;
  consume(test, &ctx, "W-,8,2");
  expect_substr(test, ctx.log, "# found start 2 end 7 |5|",
                "record display includes found start/end");
  expect_substr(test, ctx.log, "+offset 2 -trim 3 = |5|",
                "record display keeps offset/trim summary");
  ctx.log_enable = 0;
  sampling.where = NULL;
  sampling.len = 0;
  sampling.capacity = 0;
  sampling.offset = 0;
  sampling.trim = 0;

  consume(test, &ctx, "VL2,7");
  expect_int(test, sv.loop_start[voice], 2, "voice loop override start");
  expect_int(test, sv.loop_end[voice], 7, "voice loop override end");
  expect_int(test, sv.loop_override[voice], 1, "voice loop override enabled");

  ctx.log_enable = 1;
  ctx.log[0] = '\0';
  ctx.log_len = 0;
  consume(test, &ctx, "VW8,2");
  expect_substr(test, ctx.log, "voice 6 wave 300",
                "VW shows current voice and wave");
  expect_substr(test, ctx.log, "loop [2..7)",
                "VW uses voice loop override markers");
  ctx.log_enable = 0;

  consume(test, &ctx, "B1 BC0 l1");
  expect_int(test, sv.loop_active[voice], 1, "VL trigger loop active");
  for (int i = 0; i < 7; i++) osc_next(voice, 1.0f);
  expect_float(test, sv.phase[voice], 2.0f, 0.0001f,
               "VL affects l1 loop wrap");
  consume(test, &ctx, "l0");
  for (int i = 0; i < 5; i++) osc_next(voice, 1.0f);
  expect_float(test, sv.phase[voice], 7.0f, 0.0001f,
               "VL release exits at override end");
  expect_int(test, sv.loop_active[voice], 0,
             "VL release clears active loop");
  sv.finished[voice] = 1;
  sv.loop_active[voice] = 0;
  sv.loop_stop_requested[voice] = 0;
  voice_format(voice, formatted, sizeof(formatted), 0);
  if (strstr(formatted, "VL2,7") == NULL)
    fail(test, "voice loop override missing from voice format");

  consume(test, &ctx, "VS2,8");
  expect_int(test, sv.wave_range_start[voice], 2, "voice range override start");
  expect_int(test, sv.wave_range_end[voice], 8, "voice range override end");
  expect_int(test, sv.wave_range_override[voice], 1, "voice range override enabled");
  voice_format(voice, formatted, sizeof(formatted), 0);
  if (strstr(formatted, "VS2,8") == NULL)
    fail(test, "voice range override missing from voice format");

  consume(test, &ctx, "WL300,4,9");
  expect_int(test, sw.loop_start[wave], 4, "updated wave loop start");
  expect_int(test, sw.loop_end[wave], 9, "updated wave loop end");
  expect_int(test, sv.loop_start[voice], 2, "WL preserves override start");
  expect_int(test, sv.loop_end[voice], 7, "WL preserves override end");

  voice_copy(voice, 7);
  expect_int(test, sv.wave_range_start[7], 2, "copy preserves range start");
  expect_int(test, sv.wave_range_end[7], 8, "copy preserves range end");
  expect_int(test, sv.wave_range_override[7], 1, "copy preserves range flag");
  expect_int(test, sv.loop_start[7], 2, "copy preserves override start");
  expect_int(test, sv.loop_end[7], 7, "copy preserves override end");
  expect_int(test, sv.loop_override[7], 1, "copy preserves override flag");

  consume(test, &ctx, "VL");
  expect_int(test, sv.loop_start[voice], 2, "VL reset falls back to range start");
  expect_int(test, sv.loop_end[voice], 8, "VL reset falls back to range end");
  expect_int(test, sv.loop_override[voice], 0, "VL reset override flag");

  consume(test, &ctx, "VS");
  expect_int(test, sv.wave_range_start[voice], 0, "VS reset start");
  expect_int(test, sv.wave_range_end[voice], 10, "VS reset end");
  expect_int(test, sv.wave_range_override[voice], 0, "VS reset override flag");
  consume(test, &ctx, "VL");
  expect_int(test, sv.loop_start[voice], 4, "VL reset wave default start");
  expect_int(test, sv.loop_end[voice], 9, "VL reset wave default end");

  consume(test, &ctx, "WL300,1,6");
  expect_int(test, sv.loop_start[voice], 1, "WL updates reset voice start");
  expect_int(test, sv.loop_end[voice], 6, "WL updates reset voice end");

  consume(test, &ctx, "VL7,2");
  expect_int(test, sv.loop_start[voice], 1, "invalid VL keeps start");
  expect_int(test, sv.loop_end[voice], 6, "invalid VL keeps end");

  consume(test, &ctx, "VL5");
  expect_int(test, sv.loop_start[voice], 1, "partial VL keeps start");
  expect_int(test, sv.loop_end[voice], 6, "partial VL keeps end");

  consume(test, &ctx, "VS3,8");
  expect_int(test, sv.wave_range_start[voice], 3, "VS narrows start");
  expect_int(test, sv.wave_range_end[voice], 8, "VS narrows end");
  expect_int(test, sv.loop_start[voice], 3, "VS rehomes out-of-range loop start");
  expect_int(test, sv.loop_end[voice], 8, "VS rehomes out-of-range loop end");
  consume(test, &ctx, "VL1,6");
  expect_int(test, sv.loop_start[voice], 3, "out-of-range VL keeps start");
  expect_int(test, sv.loop_end[voice], 8, "out-of-range VL keeps end");

  ctx.log_enable = 1;
  ctx.log[0] = '\0';
  ctx.log_len = 0;
  consume(test, &ctx, "VL1,9");
  expect_substr(test, ctx.log, "VL rejected for v6",
                "out-of-range VL explains rejection");
  expect_substr(test, ctx.log, "within VS 3..8",
                "VL rejection includes active voice range");

  consume(test, &ctx,
          "[BOB] : v6 VS 0 10 VL 8 10 B1 ; BOB");
  expect_int(test, sv.wave_range_start[voice], 0,
             "macro updates voice range start");
  expect_int(test, sv.wave_range_end[voice], 10,
             "macro updates voice range end");
  expect_int(test, sv.loop_start[voice], 8,
             "macro updates voice loop start");
  expect_int(test, sv.loop_end[voice], 10,
             "macro updates voice loop end");
  expect_int(test, sv.loop_enabled[voice], 1,
             "macro enables voice loop");
  ctx.log_enable = 0;

  wave_reset(voice);
  wave_reset(7);
  sw.data[wave] = NULL;
  sw.size[wave] = 0;
}

static void test_wave_load_smpl_loop(void) {
  const char *test = "wave load smpl loop";
  char path[128];
  skode_t ctx = new_ctx();
  const int wave = 301;

  snprintf(path, sizeof(path), "/tmp/skode_smpl_loop_%ld.wav", (long)getpid());
  if (write_smpl_test_wav(path, 10, 2, 7, 0, 3) != 0) {
    fail(test, "could not write test wav");
    return;
  }

  ctx.log_enable = 1;
  if (wave_load_string(&ctx, path, wave, -1, 1) != 0) {
    fail(test, "wave_load_string failed");
    unlink(path);
    return;
  }

  expect_int(test, sw.size[wave], 10, "loaded frame count");
  expect_int(test, sw.loop_enabled[wave], 1, "smpl enables wave loop");
  expect_int(test, sw.loop_start[wave], 2, "smpl loop start");
  expect_int(test, sw.loop_end[wave], 8, "smpl loop end is exclusive");
  expect_float(test, sw.direction[wave], 0.0f, 0.0001f, "smpl forward direction");
  expect_substr(test, ctx.log, "# smpl loop 2..8 type:0 play:3",
                "smpl load log");

  consume(test, &ctx, "v2 w301");
  expect_int(test, sv.loop_enabled[2], 1, "voice inherits smpl loop enabled");
  expect_int(test, sv.loop_start[2], 2, "voice inherits smpl loop start");
  expect_int(test, sv.loop_end[2], 8, "voice inherits smpl loop end");

  wave_free_one(wave);
  wave_reset(2);
  ctx.log_len = 0;
  if (write_smpl_test_wav(path, 10, 2, 7, 1, 0) != 0) {
    fail(test, "could not write ping-pong test wav");
    unlink(path);
    return;
  }
  if (wave_load_string(&ctx, path, wave, -1, 1) != 0) {
    fail(test, "ping-pong wave_load_string failed");
    unlink(path);
    return;
  }
  expect_float(test, sw.direction[wave], 2.0f, 0.0001f,
               "smpl ping-pong direction");
  consume(test, &ctx, "v2 w301");
  expect_int(test, sv.direction[2], 2, "voice inherits smpl ping-pong");

  wave_free_one(wave);
  wave_reset(2);
  ctx.log_len = 0;
  if (write_smpl_test_wav(path, 10, 2, 7, 2, 0) != 0) {
    fail(test, "could not write backward test wav");
    unlink(path);
    return;
  }
  if (wave_load_string(&ctx, path, wave, -1, 1) != 0) {
    fail(test, "backward wave_load_string failed");
    unlink(path);
    return;
  }
  expect_float(test, sw.direction[wave], 1.0f, 0.0001f,
               "smpl backward direction");

  wave_reset(2);
  wave_free_one(wave);
  unlink(path);
}

static void test_record_find_trim_command(void) {
  const char *test = "record find trim";
  static float rec_data[16] = {
    0.0f, 0.0002f, 0.02f, 0.0001f, 0.0f,
    0.12f, 0.13f, 0.14f, 0.15f,
    0.0002f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
  };
  skode_t ctx = new_ctx();
  extern synth_sample_t sampling;

  sampling.where = rec_data;
  sampling.len = 16;
  sampling.capacity = 16;
  sampling.offset = 0;
  sampling.trim = 0;

  consume(test, &ctx, "w<>");
  expect_int(test, sampling.offset, 5,
             "default trim ignores noise and one-sample spike");
  expect_int(test, sampling.trim, 6,
             "default trim finds trailing silence");

  sampling.offset = 0;
  sampling.trim = 0;
  consume(test, &ctx, "w<>.001,.001,1");
  expect_int(test, sampling.offset, 4, "margin expands found start");
  expect_int(test, sampling.trim, 6, "margin keeps zero-crossed end");

  sampling.where = NULL;
  sampling.len = 0;
  sampling.capacity = 0;
  sampling.offset = 0;
  sampling.trim = 0;
}

static void test_bounded_loop_preserves_tail_envelopes(void) {
  const char *test = "bounded loop tail envelopes";
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
  if (strstr(formatted, "amp_env_release:18446744073709551615") == NULL ||
      strstr(formatted, "filter_env_release:18446744073709551615") == NULL) {
    fail(test, "loop exhaustion released envelopes before sample tail");
  }
  expect_int(test, sv.finished[voice], 0, "bounded loop tail remains active");

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
    {"DL1,2,3,4,5,6,7", SKODE_OP_DELAY_PARAMS},
    {"VS1,3", SKODE_OP_WAVE_RANGE_SET},
    {"VL1,3", SKODE_OP_WAVE_LOOP_SET},
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
  consume(test, &ctx, "/v1 /t1 /f7");

  expect_int(test, ctx.verbose, 1, "verbose");
  expect_int(test, ctx.trace, 1, "trace");
  expect_int(test, ctx.flag, 7, "flag");
}

static int buffer_channel_nonzero(const float *buffer, int frames,
                                  int channels, int channel) {
  for (int i = 0; i < frames; i++) {
    float value = buffer[(i * channels) + channel];
    if (fabsf(value) > 0.000001f) return 1;
  }
  return 0;
}

static int buffer_channels_differ(const float *buffer, int frames, int channels) {
  for (int i = 0; i < frames; i++) {
    float left = buffer[(i * channels)];
    float right = buffer[(i * channels) + 1];
    if (fabsf(left - right) > 0.000001f) return 1;
  }
  return 0;
}

#ifdef SKRED_TEST_TRACKS
static void configure_delay_test_voice(int voice, int track, float pan) {
  static float table[2] = {1.0f, 1.0f};
  sv.table[voice] = table;
  sv.table_size[voice] = 2;
  sv.table_rate[voice] = (float)MAIN_SAMPLE_RATE;
  sv.table_size_rate[voice] = 2.0f / (float)MAIN_SAMPLE_RATE;
  sv.one_shot[voice] = 0;
  sv.finished[voice] = 0;
  sv.phase_inc[voice] = 0.0f;
  sv.amp[voice] = 1.0f;
  sv.user_amp[voice] = 0.0f;
  sv.use_amp_envelope[voice] = 0;
  pan_set(voice, pan);
  sv.pan_left[voice] = 0.0f;
  sv.pan_right[voice] = 0.0f;
  synth_record_track_set(voice, track);
  delay_send_set(voice, 15.0f);
}

static void test_track_delay_send_requires_route_and_center_pan(void) {
  const char *test = "track delay send requires route and center pan";
  enum { frames = 1024, channels = 2 };
  float buffer[frames * channels];
  skode_t ctx = new_ctx();
  int coarse = -1, fine = -1, feedback = -1;
  int mod_freq = -1, mod_depth = -1, level = -1;

  consume(test, &ctx, "r2 DL2,0,0,0,0,0,15 ds15");
  delay_params_get(2, &coarse, &fine, &feedback, &mod_freq, &mod_depth, &level);
  expect_int(test, coarse, 0, "delay coarse command");
  expect_int(test, fine, 0, "delay fine command");
  expect_int(test, feedback, 0, "delay feedback command");
  expect_int(test, mod_freq, 0, "delay mod freq command");
  expect_int(test, mod_depth, 0, "delay mod depth command");
  expect_int(test, level, 15, "delay level command");
  expect_int(test, synth_record_track_get(ctx.voice), 2, "record track command");
  expect_float(test, sv.delay_send[ctx.voice], 1.0f, 0.0001f,
               "delay send command");

  voice_init();
  delay_params_set(2, 0, 0, 0, 0, 0, 15);
  delay_clear();
  configure_delay_test_voice(0, 2, 0.0f);
  memset(buffer, 0, sizeof(buffer));
  synth(buffer, NULL, frames, channels, NULL);
  expect_int(test, buffer_channel_nonzero(buffer, frames, channels, 0), 1,
             "centered voice feeds delay");

  voice_init();
  delay_params_set(2, 0, 0, 0, 0, 0, 15);
  delay_clear();
  configure_delay_test_voice(0, 0, 0.0f);
  memset(buffer, 0, sizeof(buffer));
  synth(buffer, NULL, frames, channels, NULL);
  expect_int(test, buffer_channel_nonzero(buffer, frames, channels, 0), 0,
             "unrouted voice does not feed track delay");

  voice_init();
  delay_params_set(2, 0, 0, 0, 0, 0, 15);
  delay_clear();
  configure_delay_test_voice(0, 2, 1.0f);
  memset(buffer, 0, sizeof(buffer));
  synth(buffer, NULL, frames, channels, NULL);
  expect_int(test, buffer_channel_nonzero(buffer, frames, channels, 0), 0,
             "panned voice does not feed delay");

  voice_init();
  delay_params_set(3, 0, 0, 0, 0, 31, 15);
  delay_clear();
  configure_delay_test_voice(0, 3, 0.0f);
  memset(buffer, 0, sizeof(buffer));
  synth(buffer, NULL, frames, channels, NULL);
  expect_int(test, buffer_channels_differ(buffer, frames, channels), 1,
             "modulated delay returns stereo");

  voice_init();
  delay_params_set(2, 0, 0, 0, 0, 0, 15);
  delay_clear();
  configure_delay_test_voice(0, 2, 0.0f);
#ifdef SKRED_TEST_RECORD_SCOPE
  float record_frames[frames * RECORD_CHANNELS];
  synth_record_bus_t record_bus = {record_frames, RECORD_CHANNELS};
  memset(buffer, 0, sizeof(buffer));
  memset(record_frames, 0, sizeof(record_frames));
  synth(buffer, NULL, frames, channels, &record_bus);
  expect_int(test, buffer_channel_nonzero(record_frames, frames,
             RECORD_CHANNELS, 4), 1,
             "track delay return is included in matching stem");
#endif

  ctx.log_enable = 1;
  consume(test, &ctx, "GS");
  expect_substr(test, ctx.log, "# skred_version ", "global status version");
  expect_substr(test, ctx.log, "V", "global status master volume");
  expect_substr(test, ctx.log, "M", "global status tempo");
  expect_substr(test, ctx.log, "DL1,", "global status delay bus 1");
  expect_substr(test, ctx.log, "DL4,", "global status delay bus 4");

  consume(test, &ctx, "DL?2");
  expect_substr(test, ctx.log, "DL2,0,0,0,0,0,15",
                "delay query is pasteable");

  delay_params_set(2, 1, 2, 3, 4, 5, 6);
  consume(test, &ctx, "DL2,-,12,-,20,-,10");
  delay_params_get(2, &coarse, &fine, &feedback, &mod_freq, &mod_depth, &level);
  expect_int(test, coarse, 1, "delay dash keeps coarse");
  expect_int(test, fine, 12, "delay updates fine");
  expect_int(test, feedback, 3, "delay dash keeps feedback");
  expect_int(test, mod_freq, 20, "delay updates mod freq");
  expect_int(test, mod_depth, 5, "delay dash keeps mod depth");
  expect_int(test, level, 10, "delay updates level");

  event_program_t program;
  delay_params_set(2, 1, 2, 3, 4, 5, 6);
  expect_int(test, skode_compile_program("DL2,-,11,-,21,-,9", &program),
             SKODE_COMPILE_OK, "compile delay bus params");
  expect_int(test, skode_execute_program(&program, ctx.voice,
             SAMPLE_COUNT_GET(), 0), 0, "execute delay bus params");
  delay_params_get(2, &coarse, &fine, &feedback, &mod_freq, &mod_depth, &level);
  expect_int(test, coarse, 1, "delay opcode dash keeps coarse");
  expect_int(test, fine, 11, "delay opcode updates fine");
  expect_int(test, feedback, 3, "delay opcode dash keeps feedback");
  expect_int(test, mod_freq, 21, "delay opcode updates mod freq");
  expect_int(test, mod_depth, 5, "delay opcode dash keeps mod depth");
  expect_int(test, level, 9, "delay opcode updates level");

  consume(test, &ctx, "GS1");
}
#endif

static void test_ands_macro_commands(void) {
  const char *test = "ands macro commands";
  skode_t ctx = new_ctx();
  ctx.log_enable = 1;
  ands_macro_clear(ctx.parse);

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
  ands_macro_clear(ctx.parse);
}

static void test_load_installs_global_macros_and_registers(void) {
  const char *test = "load installs global macros and registers";
  char cwd[1024];
  char filename[64];
  char named_filename[96];
  char command[160];
  int patch = 910000 + (int)(getpid() % 80000);
  FILE *file;
  skode_t ctx = new_ctx();

  if (!getcwd(cwd, sizeof(cwd))) {
    fail(test, "getcwd failed");
    return;
  }
  if (chdir("/tmp") != 0) {
    fail(test, "chdir /tmp failed");
    return;
  }

  snprintf(filename, sizeof(filename), "%d.sk", patch);
  file = fopen(filename, "w");
  if (!file) {
    fail(test, "could not create temporary patch file");
    chdir(cwd);
    return;
  }
  fputs("[zz]: f $$0 ;\n=12,34\n", file);
  fclose(file);

  ands_macro_clear(ctx.parse);
  global_var[12] = 0.0;

  if (skode_load(&ctx, ctx.voice, patch, 0) != 0) {
    fail(test, "skode_load failed");
  } else {
    consume(test, &ctx, "zz 123");
    expect_float(test, sv.freq[0], 123.0f, 0.0001f,
                 "macro loaded through /l");
    expect_float(test, (float)global_var[12], 34.0f, 0.0001f,
                 "register set through /l");
  }

  snprintf(named_filename, sizeof(named_filename), "named-load-%d.sk",
           (int)getpid());
  file = fopen(named_filename, "w");
  if (!file) {
    fail(test, "could not create named temporary patch file");
  } else {
    fputs("[yy]: f $$0 ;\n=13,56\n", file);
    fclose(file);
    global_var[13] = 0.0;
    snprintf(command, sizeof(command), "[%s] /ls", named_filename);
    consume(test, &ctx, command);
    consume(test, &ctx, "yy 234");
    expect_float(test, sv.freq[0], 234.0f, 0.0001f,
                 "macro loaded through /ls");
    expect_float(test, (float)global_var[13], 56.0f, 0.0001f,
                 "register set through /ls");
  }

  remove(filename);
  remove(named_filename);
  chdir(cwd);
  ands_macro_clear(ctx.parse);
}

static void test_vfs_zip_loads_skode_and_wave_assets(void) {
  const char *test = "vfs zip loads skode and wave assets";
  char cwd[1024];
  char zipname[96];
  char wavname[96];
  char realname[96];
  char ksdir[32] = "ks";
  char skdir[32] = "sk";
  char wavdir[32] = "wav";
  char command[180];
  void *wav_data = NULL;
  size_t wav_size = 0;
  int slot = 350;
  int fallback_slot = 351;
  skode_t ctx = new_ctx();
  FILE *file;

  if (!getcwd(cwd, sizeof(cwd))) {
    fail(test, "getcwd failed");
    return;
  }
  if (chdir("/tmp") != 0) {
    fail(test, "chdir /tmp failed");
    return;
  }
  skred_vfs_unmount();

  snprintf(zipname, sizeof(zipname), "skred-vfs-%d.zip", (int)getpid());
  snprintf(wavname, sizeof(wavname), "skred-vfs-%d.wav", (int)getpid());
  snprintf(realname, sizeof(realname), "real-load-%d.sk", (int)getpid());
  remove(zipname);
  remove(wavname);
  remove(realname);
  mkdir(ksdir, 0777);
  mkdir(skdir, 0777);
  mkdir(wavdir, 0777);

  if (write_smpl_test_wav(wavname, 10, 2, 6, 0, 0) != 0) {
    fail(test, "could not create temporary wav");
    chdir(cwd);
    return;
  }
  wav_data = read_whole_file(wavname, &wav_size);
  if (!wav_data) {
    fail(test, "could not read temporary wav");
    remove(wavname);
    chdir(cwd);
    return;
  }

  const char *patch = "[zx]: f $$0 ;\n=14,78\n";
  const char *note = "hello from zip\n";
  const char *ks = "\\ comment only\n";
  if (!mz_zip_add_mem_to_archive_file_in_place(zipname, "patches/ziptest.sk",
        patch, strlen(patch), NULL, 0, MZ_DEFAULT_COMPRESSION) ||
      !mz_zip_add_mem_to_archive_file_in_place(zipname, "text/readme.txt",
        note, strlen(note), NULL, 0, MZ_DEFAULT_COMPRESSION) ||
      !mz_zip_add_mem_to_archive_file_in_place(zipname, "ks/test.ks",
        ks, strlen(ks), NULL, 0, MZ_DEFAULT_COMPRESSION) ||
      !mz_zip_add_mem_to_archive_file_in_place(zipname, "samples/zip.wav",
        wav_data, wav_size, NULL, 0, MZ_DEFAULT_COMPRESSION)) {
    fail(test, "could not create zip fixture");
    free(wav_data);
    remove(zipname);
    remove(wavname);
    chdir(cwd);
    return;
  }
  free(wav_data);

  ctx.log_enable = 1;
  snprintf(command, sizeof(command), "[%s] %%z", zipname);
  consume(test, &ctx, command);
  expect_substr(test, ctx.log, "# vfs zip:", "zip mount status");

  reset_log(&ctx);
  consume(test, &ctx, "%pwd");
  expect_substr(test, ctx.log, "# vfs zip:", "zip pwd status");

  reset_log(&ctx);
  consume(test, &ctx, "[patches] %cd");
  consume(test, &ctx, "%ls");
  expect_substr(test, ctx.log, "[ziptest.sk]", "zip ls skode file");

  ands_macro_clear(ctx.parse);
  global_var[14] = 0.0;
  consume(test, &ctx, "[ziptest.sk] /ls");
  consume(test, &ctx, "zx 345");
  expect_float(test, sv.freq[0], 345.0f, 0.0001f,
               "macro loaded from zip");
  expect_float(test, (float)global_var[14], 78.0f, 0.0001f,
               "register set from zip");

  file = fopen(realname, "w");
  if (!file) {
    fail(test, "could not create real fallback skode file");
  } else {
    fputs("[zr]: f $$0 ;\n=15,91\n", file);
    fclose(file);
    global_var[15] = 0.0;
    snprintf(command, sizeof(command), "[file:%s] /ls", realname);
    consume(test, &ctx, command);
    consume(test, &ctx, "zr 456");
    expect_float(test, sv.freq[0], 456.0f, 0.0001f,
                 "macro loaded from real file while zip mounted");
    expect_float(test, (float)global_var[15], 91.0f, 0.0001f,
                 "register set from real file while zip mounted");
  }

  file = fopen("sk/fallback.sk", "w");
  if (!file) {
    fail(test, "could not create sk fallback file");
  } else {
    fputs("[zf]: f $$0 ;\n=16,92\n", file);
    fclose(file);
    global_var[16] = 0.0;
    consume(test, &ctx, "[fallback.sk] /ls");
    consume(test, &ctx, "zf 567");
    expect_float(test, sv.freq[0], 567.0f, 0.0001f,
                 "macro loaded from sk fallback");
    expect_float(test, (float)global_var[16], 92.0f, 0.0001f,
                 "register set from sk fallback");
  }

  reset_log(&ctx);
  consume(test, &ctx, "[../text/readme.txt] %cat");
  expect_substr(test, ctx.log, "hello from zip", "zip cat text");

  reset_log(&ctx);
  consume(test, &ctx, "[../ks] %cd");
  consume(test, &ctx, "%ls 3");
  expect_substr(test, ctx.log, "[test.ks]", "zip ls ks file");

  file = fopen("ks/fallback.ks", "w");
  if (!file) {
    fail(test, "could not create ks fallback file");
  } else {
    fputs("\\ comment only\n", file);
    fclose(file);
    consume(test, &ctx, "[fallback.ks] /ks");
  }

  snprintf(command, sizeof(command), "[../samples/zip.wav] /ws%d", slot);
  consume(test, &ctx, command);
  expect_int(test, sw.size[slot], 10, "zip wav frame count");
  expect_int(test, sw.loop_enabled[slot], 1, "zip wav smpl loop enabled");
  expect_int(test, sw.loop_start[slot], 2, "zip wav loop start");
  expect_int(test, sw.loop_end[slot], 7, "zip wav loop end");

  if (write_smpl_test_wav("wav/fallback.wav", 10, 3, 5, 0, 0) != 0) {
    fail(test, "could not create wav fallback file");
  } else {
    snprintf(command, sizeof(command), "[fallback.wav] /ws%d", fallback_slot);
    consume(test, &ctx, command);
    expect_int(test, sw.size[fallback_slot], 10, "wav fallback frame count");
    expect_int(test, sw.loop_start[fallback_slot], 3, "wav fallback loop start");
    expect_int(test, sw.loop_end[fallback_slot], 6, "wav fallback loop end");
  }

  reset_log(&ctx);
  consume(test, &ctx, "%zu");
  expect_substr(test, ctx.log, "# vfs disk:", "zip unmount status");

  skred_vfs_unmount();
  ands_macro_clear(ctx.parse);
  remove(zipname);
  remove(wavname);
  remove(realname);
  remove("sk/fallback.sk");
  remove("ks/fallback.ks");
  remove("wav/fallback.wav");
  chdir(cwd);
}

static void test_load_909_patch_from_source_assets(void) {
#ifdef SKRED_TEST_SOURCE_DIR
  const char *test = "load 909 patch from source assets";
  char cwd[1024];
  skode_t ctx = new_ctx();

  if (!getcwd(cwd, sizeof(cwd))) {
    fail(test, "getcwd failed");
    return;
  }
  if (chdir(SKRED_TEST_SOURCE_DIR) != 0) {
    fail(test, "chdir source dir failed");
    return;
  }

  consume(test, &ctx, "/l909");
  expect_int(test, sw.size[500] > 0, 1, "909 sample wave loaded");
  expect_int(test, sw.size[400] > 0, 1, "909 chh wave loaded");
  expect_int(test, sw.size[401] > 0, 1, "909 kick wave loaded");
  expect_int(test, sw.size[402] > 0, 1, "909 snare wave loaded");

  chdir(cwd);
#endif
}

static void test_909_load_rejects_too_small_wave_table(void) {
#ifdef SKRED_TEST_SOURCE_DIR
  const char *test = "909 load rejects too-small wave table";
  char cwd[1024];
  skode_t ctx = new_ctx();

  synth_free();
  synth_config_set_waves(64);
  synth_init(8);
  wave_table_init(0);
  voice_init();
  seq_init();

  if (!getcwd(cwd, sizeof(cwd))) {
    fail(test, "getcwd failed");
    return;
  }
  if (chdir(SKRED_TEST_SOURCE_DIR) != 0) {
    fail(test, "chdir source dir failed");
    return;
  }

  reset_log(&ctx);
  expect_int(test, wave_load(&ctx, 24, 500, 0, 0), -1,
             "high sample wave load status");

  reset_log(&ctx);
  expect_int(test, rec_load(&ctx, 400, 0, 0.0f), -1,
             "high recording wave load status");

  reset_log(&ctx);
  consume(test, &ctx, "/l909");

  chdir(cwd);
#endif
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
  #if defined(_WIN32) || defined(_WIN64)
  if (skred_control_event_wait_handle() == NULL) {
    fail(test, "control event wait handle unavailable");
  }
  #else
  if (skred_control_event_wait_fd() < 0) {
    fail(test, "control event wait fd unavailable");
  }
  #endif
  expect_int(test, skred_control_event_wait(0), 0,
             "empty control event wait");
  skred_control_user_event(SAMPLE_COUNT_GET(), -1, -1, -1, -1,
                           SKODE_OP_CONTROL_EVENT, 123, 0, NULL);
  expect_int(test, skred_control_event_wait(0), 1,
             "published control event wait");
  expect_int(test, skred_control_event_snapshot(events, 8), 1,
             "control event snapshot count");
  expect_int(test, (int)events[0].type, SKRED_CONTROL_EVENT_USER,
             "control event snapshot type");
  expect_int(test, skred_control_event_snapshot(NULL, 1), -1,
             "control event snapshot rejects null buffer");
  ctx.log[0] = '\0';
  ctx.log_len = 0;
  consume(test, &ctx, "?ce");
  if (strstr(ctx.log, "# control events:1") == NULL ||
      strstr(ctx.log, "snapshot") == NULL) {
    fail(test, "control-plane event snapshot diagnostic output mismatch");
  }
  expect_int(test, skred_control_event_clear(), 1,
             "control event clear count");
  expect_int(test, skred_control_event_wait(0), 0,
             "cleared control event wait");

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
  if (strstr(ctx.log, "# control events:1") == NULL ||
      strstr(ctx.log, "type:VOICE_TRIGGER") == NULL ||
      strstr(ctx.log, "voice:4") == NULL) {
    fail(test, "non-consuming control-plane event diagnostic mismatch");
  }
  ctx.log[0] = '\0';
  ctx.log_len = 0;
  consume(test, &ctx, "?ce!");
  if (strstr(ctx.log, "# control events cleared:1") == NULL) {
    fail(test, "control-plane event clear diagnostic output mismatch");
  }
  ctx.log[0] = '\0';
  ctx.log_len = 0;
  consume(test, &ctx, "?ce");
  if (strstr(ctx.log, "# control events empty") == NULL) {
    fail(test, "empty control-plane event after clear mismatch");
  }

  configure_loop_test_voice(5, 0);
  consume(test, &ctx, "[/cex1,2,5 VS1,7 VL2,6 BC1 l1] e>0");
  consume(test, &ctx, "[/ce! 2 5 VS2,8 VL3,7 BC1 l1] e>1");
  consume(test, &ctx, "/cex0,2,5");
  consume(test, &ctx, "v5 a0 vc1 BC1 l1");
  skred_control_event_clear();
  sv.phase[5] = 4.0f;
  sv.phase_inc[5] = 1.0f;
  sv.loop_remaining[5] = 0;
  synth(output, NULL, 1, AUDIO_CHANNELS, NULL);
  (void)skred_control_dispatch_pump(8);
  expect_int(test, sv.wave_range_start[5], 1,
             "release response updates same voice range start");
  expect_int(test, sv.wave_range_end[5], 7,
             "release response updates same voice range end");
  expect_int(test, sv.loop_start[5], 2,
             "release response updates same voice loop start");
  expect_int(test, sv.loop_end[5], 6,
             "release response updates same voice loop end");

  sv.phase[5] = (double)sv.loop_end[5] - 1.0;
  sv.phase_inc[5] = 1.0f;
  sv.loop_remaining[5] = 0;
  synth(output, NULL, 1, AUDIO_CHANNELS, NULL);
  (void)skred_control_dispatch_pump(8);
  expect_int(test, sv.wave_range_start[5], 2,
             "second release response updates same voice range start");
  expect_int(test, sv.wave_range_end[5], 8,
             "second release response updates same voice range end");
  expect_int(test, sv.loop_start[5], 3,
             "second release response updates same voice loop start");
  expect_int(test, sv.loop_end[5], 7,
             "second release response updates same voice loop end");
  skred_control_response_clear();
  skred_control_event_reset();
  wave_reset(5);
  SAMPLE_COUNT_PUT(saved_sample_count);

  configure_loop_test_voice(5, 0);
  consume(test, &ctx, "[/ce! 3 5 /cex1,2,5 VS2,5 VL2,5 BC2 l1] e>0");
  consume(test, &ctx, "[/ce! 2 5 B0 VS5,8 l1] e>1");
  consume(test, &ctx, "/cex0,3,5");
  consume(test, &ctx, "v5 a0 vc1 B0 VS0,2 l1");
  skred_control_event_clear();
  sv.phase[5] = 1.0;
  sv.phase_inc[5] = 1.0f;
  synth(output, NULL, 1, AUDIO_CHANNELS, NULL);
  (void)skred_control_dispatch_pump(8);
  expect_int(test, sv.wave_range_start[5], 2,
             "finished response sets middle range start");
  expect_int(test, sv.wave_range_end[5], 5,
             "finished response sets middle range end");
  expect_int(test, sv.loop_start[5], 2,
             "finished response sets middle loop start");
  expect_int(test, sv.loop_end[5], 5,
             "finished response sets middle loop end");
  expect_int(test, sv.loop_count[5], 2,
             "finished response sets three-play loop count");

  sv.phase[5] = 4.0;
  sv.phase_inc[5] = 1.0f;
  sv.loop_remaining[5] = 0;
  synth(output, NULL, 1, AUDIO_CHANNELS, NULL);
  (void)skred_control_dispatch_pump(8);
  expect_int(test, sv.wave_range_start[5], 5,
             "release response sets outro range start");
  expect_int(test, sv.wave_range_end[5], 8,
             "release response sets outro range end");
  expect_int(test, sv.loop_enabled[5], 0,
             "release response disables looping for outro");
  skred_control_response_clear();
  skred_control_event_reset();
  wave_reset(5);
  SAMPLE_COUNT_PUT(saved_sample_count);

  foreign_call_count = 0;
  memset(&foreign_last_call, 0, sizeof(foreign_last_call));
  skred_foreign_function_clear(3);
  consume(test, &ctx, "[quiet] /ff3 1,2");
  expect_int(test, foreign_call_count, 0, "unbound foreign function no-op");
  expect_int(test, skred_foreign_function_bind(3, capture_foreign_call, NULL),
             0, "bind foreign function");
  ctx.voice = 6;
  ctx.pattern = 7;
  ctx.step = 8;
  consume(test, &ctx, "(4,5,6) [hello] /ff3 1.5,2.5");
  expect_int(test, foreign_call_count, 1, "foreign function call count");
  expect_int(test, foreign_last_call.index, 3, "foreign function index");
  expect_int(test, foreign_last_call.argc, 2, "foreign function argc");
  expect_float(test, (float)foreign_last_call.arg[0], 1.5f, 0.0f,
               "foreign function arg0");
  expect_float(test, (float)foreign_last_call.arg[1], 2.5f, 0.0f,
               "foreign function arg1");
  if (strcmp(foreign_last_call.string, "hello") != 0) {
    fail(test, "foreign function string mismatch");
  }
  expect_int(test, foreign_last_call.data_len, 3, "foreign function data len");
  expect_float(test, (float)foreign_last_call.data[0], 4.0f, 0.0f,
               "foreign function data0");
  expect_int(test, foreign_last_call.voice, 6, "foreign function voice");
  expect_int(test, foreign_last_call.pattern, 7, "foreign function pattern");
  expect_int(test, foreign_last_call.step, 8, "foreign function step");
  skred_foreign_function_clear(3);

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
  test_command_help();
  test_named_wave_destination();
  test_midi_and_links();
  test_trigger_delay_lifecycle();
  test_envelope_configuration_is_deferred();
  test_envelope_future_timestamps();
  test_bounded_one_shot_loops();
  test_wave_loop_points();
  test_wave_load_smpl_loop();
  test_record_find_trim_command();
  test_bounded_loop_preserves_tail_envelopes();
  test_one_shot_asr_mode();
  test_opcode_events();
  test_909_sequence_programs();
  test_scalar_voice_opcode_inventory();
  test_parameter_and_buffer_safety();
  test_context_modes();
#ifdef SKRED_TEST_TRACKS
  test_track_delay_send_requires_route_and_center_pan();
#endif
  test_ands_macro_commands();
  test_load_installs_global_macros_and_registers();
  test_vfs_zip_loads_skode_and_wave_assets();
  test_control_composition_primitives();
  test_tempo_and_pattern_reset_limits();
  test_sample_accurate_sequence_boundaries();
  test_silent_voice_fast_path();
  test_control_plane_voice_events();
  test_load_909_patch_from_source_assets();
  test_909_load_rejects_too_small_wave_table();

  synth_free();

  if (failures) {
    fprintf(stderr, "%d SKODE state test failure(s)\n", failures);
    return 1;
  }
  printf("SKODE state tests passed\n");
  return 0;
}
