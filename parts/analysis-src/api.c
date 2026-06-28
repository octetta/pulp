#include "api.h"
#include "synth-state.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include your other internal headers here 
// (e.g., miniaudio.h, udp.h, seq.h, etc.)

#include "util.h"
#include "miniaudio.h"
#include "synth-types.h"
#include "synth.h"
#include "skode.h"
#include "portable_atomic.h"
#include "seq.h"
#include "udp.h"
#include "recorder.h"
#include "scope-ipc.h"

#define ONE_FRAME_MAX (64 * 1024)
#define SKRED_CONTROL_EVENT_CAPACITY 1024
#define SKRED_CONTROL_VOICE_CAPACITY VOICE_MAX_HARD_LIMIT

typedef struct {
  skred_control_event_t event;
  atomic_int_t ready;
} skred_control_event_slot_t;

static skred_control_event_slot_t control_events[SKRED_CONTROL_EVENT_CAPACITY];
static atomic_uint64_t control_write_idx;
static atomic_uint64_t control_read_idx;
static atomic_uint64_t control_sequence;
static atomic_uint64_t control_dropped;
static int control_voice_pattern[SKRED_CONTROL_VOICE_CAPACITY];
static int control_voice_step[SKRED_CONTROL_VOICE_CAPACITY];
static int control_voice_tag[SKRED_CONTROL_VOICE_CAPACITY];
static uint32_t control_voice_opcode[SKRED_CONTROL_VOICE_CAPACITY];
static atomic_int_t control_voice_playing[SKRED_CONTROL_VOICE_CAPACITY];

void skred_control_event_reset(void) {
  atomic_store_uint64(&control_write_idx, 0);
  atomic_store_uint64(&control_read_idx, 0);
  atomic_store_uint64(&control_sequence, 0);
  atomic_store_uint64(&control_dropped, 0);
  for (int i = 0; i < SKRED_CONTROL_EVENT_CAPACITY; i++)
    atomic_store_int(&control_events[i].ready, 0);
  for (int i = 0; i < SKRED_CONTROL_VOICE_CAPACITY; i++) {
    control_voice_pattern[i] = -1;
    control_voice_step[i] = -1;
    control_voice_tag[i] = -1;
    control_voice_opcode[i] = 0;
    atomic_store_int(&control_voice_playing[i], 0);
  }
}

uint64_t skred_control_event_dropped(void) {
  return atomic_load_uint64(&control_dropped);
}

void skred_control_voice_source(int voice, int pattern, int step, int tag,
    uint32_t opcode) {
  if (voice < 0 || voice >= SKRED_CONTROL_VOICE_CAPACITY) return;
  control_voice_pattern[voice] = pattern;
  control_voice_step[voice] = step;
  control_voice_tag[voice] = tag;
  control_voice_opcode[voice] = opcode;
}

void skred_control_voice_event(uint32_t type, uint64_t sample, int voice) {
  if (voice < 0 || voice >= SKRED_CONTROL_VOICE_CAPACITY) return;
  if (type == SKRED_CONTROL_EVENT_VOICE_TRIGGER) {
    atomic_store_int(&control_voice_playing[voice], 1);
  } else if (type == SKRED_CONTROL_EVENT_VOICE_FINISHED) {
    int expected = 1;
    if (!atomic_compare_exchange_int(&control_voice_playing[voice],
          &expected, 0)) {
      return;
    }
  }
  while (1) {
    uint64_t write = atomic_load_uint64(&control_write_idx);
    uint64_t read = atomic_load_uint64(&control_read_idx);
    if (write - read >= SKRED_CONTROL_EVENT_CAPACITY) {
      atomic_fetch_add_uint64(&control_dropped, 1);
      return;
    }
    uint64_t next = write + 1;
    uint64_t expected = write;
    if (atomic_compare_exchange_uint64(&control_write_idx, &expected, next)) {
      skred_control_event_slot_t *slot =
        &control_events[write % SKRED_CONTROL_EVENT_CAPACITY];
      atomic_store_int(&slot->ready, 0);
      slot->event.type = type;
      slot->event.opcode = control_voice_opcode[voice];
      slot->event.sample = sample;
      slot->event.sequence = atomic_fetch_add_uint64(&control_sequence, 1);
      slot->event.voice = voice;
      slot->event.pattern = control_voice_pattern[voice];
      slot->event.step = control_voice_step[voice];
      slot->event.tag = control_voice_tag[voice];
      atomic_store_int(&slot->ready, 1);
      return;
    }
  }
}

int skred_control_event_poll(skred_control_event_t *events, int max_events) {
  if (!events || max_events <= 0) return 0;
  int count = 0;
  while (count < max_events) {
    uint64_t read = atomic_load_uint64(&control_read_idx);
    uint64_t write = atomic_load_uint64(&control_write_idx);
    if (read >= write) break;
    skred_control_event_slot_t *slot =
      &control_events[read % SKRED_CONTROL_EVENT_CAPACITY];
    if (!atomic_load_int(&slot->ready)) break;
    events[count++] = slot->event;
    atomic_store_int(&slot->ready, 0);
    atomic_store_uint64(&control_read_idx, read + 1);
  }
  return count;
}

static atomic_uint64_t perf_callbacks;
static atomic_uint64_t perf_frames;
static atomic_uint64_t perf_callback_ns_total;
static atomic_uint64_t perf_callback_ns_last;
static atomic_uint64_t perf_callback_ns_worst;
static atomic_uint64_t perf_callback_budget_ns_total;
static atomic_uint64_t perf_callback_budget_ns_last;
static atomic_uint64_t perf_callback_budget_ns_worst;
static atomic_uint64_t perf_callback_overruns;
static atomic_uint64_t perf_callback_frames_last;
static atomic_uint64_t perf_callback_frames_worst;
static char perf_status[1024];

void skred_performance_reset(void) {
  atomic_store_uint64(&perf_callbacks, 0);
  atomic_store_uint64(&perf_frames, 0);
  atomic_store_uint64(&perf_callback_ns_total, 0);
  atomic_store_uint64(&perf_callback_ns_last, 0);
  atomic_store_uint64(&perf_callback_ns_worst, 0);
  atomic_store_uint64(&perf_callback_budget_ns_total, 0);
  atomic_store_uint64(&perf_callback_budget_ns_last, 0);
  atomic_store_uint64(&perf_callback_budget_ns_worst, 0);
  atomic_store_uint64(&perf_callback_overruns, 0);
  atomic_store_uint64(&perf_callback_frames_last, 0);
  atomic_store_uint64(&perf_callback_frames_worst, 0);
}

int skred_performance_metrics(skred_performance_metrics_t *out) {
  if (!out) return -1;
  out->callbacks = atomic_load_uint64(&perf_callbacks);
  out->frames = atomic_load_uint64(&perf_frames);
  out->sample_count = SAMPLE_COUNT_GET();
  out->callback_ns_total = atomic_load_uint64(&perf_callback_ns_total);
  out->callback_ns_last = atomic_load_uint64(&perf_callback_ns_last);
  out->callback_ns_worst = atomic_load_uint64(&perf_callback_ns_worst);
  out->callback_budget_ns_total =
    atomic_load_uint64(&perf_callback_budget_ns_total);
  out->callback_budget_ns_last =
    atomic_load_uint64(&perf_callback_budget_ns_last);
  out->callback_budget_ns_worst =
    atomic_load_uint64(&perf_callback_budget_ns_worst);
  out->callback_overruns = atomic_load_uint64(&perf_callback_overruns);
  out->callback_frames_last =
    (uint32_t)atomic_load_uint64(&perf_callback_frames_last);
  out->callback_frames_worst =
    (uint32_t)atomic_load_uint64(&perf_callback_frames_worst);
  return 0;
}

char *skred_performance_status(void) {
  skred_performance_metrics_t m;
  skred_performance_metrics(&m);
  double avg_ms = m.callbacks
    ? (double)m.callback_ns_total / (double)m.callbacks / 1000000.0 : 0.0;
  double last_ms = (double)m.callback_ns_last / 1000000.0;
  double worst_ms = (double)m.callback_ns_worst / 1000000.0;
  double budget_ms = (double)m.callback_budget_ns_last / 1000000.0;
  double worst_budget_ms = (double)m.callback_budget_ns_worst / 1000000.0;
  double avg_load = m.callback_budget_ns_total
    ? (double)m.callback_ns_total / (double)m.callback_budget_ns_total * 100.0
    : 0.0;
  double last_load = m.callback_budget_ns_last
    ? (double)m.callback_ns_last / (double)m.callback_budget_ns_last * 100.0
    : 0.0;
  double worst_load = m.callback_budget_ns_worst
    ? (double)m.callback_ns_worst / (double)m.callback_budget_ns_worst * 100.0
    : 0.0;
  snprintf(perf_status, sizeof(perf_status),
           "perf:\n"
           "  callbacks: %llu frames: %llu samples: %llu overruns: %llu\n"
           "  callback-ms: last %.3f avg %.3f worst %.3f budget %.3f worst-budget %.3f\n"
           "  callback-load: last %.1f%% avg %.1f%% worst %.1f%%\n"
           "  callback-frames: last %u worst %u",
           (unsigned long long)m.callbacks,
           (unsigned long long)m.frames,
           (unsigned long long)m.sample_count,
           (unsigned long long)m.callback_overruns,
           last_ms, avg_ms, worst_ms, budget_ms, worst_budget_ms,
           last_load, avg_load, worst_load,
           m.callback_frames_last, m.callback_frames_worst);
  return perf_status;
}

static struct timespec perf_callback_begin(void) {
  struct timespec start;
  clock_gettime(BENCH_CLOCK, &start);
  return start;
}

static void perf_callback_end(const struct timespec *start,
    ma_device *device, ma_uint32 frame_count) {
  struct timespec end;
  clock_gettime(BENCH_CLOCK, &end);
  uint64_t elapsed_ns = (uint64_t)ts_diff_ns(start, &end);
  uint32_t sample_rate = device && device->sampleRate
    ? device->sampleRate : (uint32_t)MAIN_SAMPLE_RATE;
  uint64_t budget_ns = sample_rate
    ? ((uint64_t)frame_count * 1000000000ULL) / sample_rate : 0;
  uint64_t worst_ns = atomic_load_uint64(&perf_callback_ns_worst);

  atomic_fetch_add_uint64(&perf_callbacks, 1);
  atomic_fetch_add_uint64(&perf_frames, frame_count);
  atomic_fetch_add_uint64(&perf_callback_ns_total, elapsed_ns);
  atomic_fetch_add_uint64(&perf_callback_budget_ns_total, budget_ns);
  atomic_store_uint64(&perf_callback_ns_last, elapsed_ns);
  atomic_store_uint64(&perf_callback_budget_ns_last, budget_ns);
  atomic_store_uint64(&perf_callback_frames_last, frame_count);
  if (elapsed_ns > worst_ns) {
    atomic_store_uint64(&perf_callback_ns_worst, elapsed_ns);
    atomic_store_uint64(&perf_callback_budget_ns_worst, budget_ns);
    atomic_store_uint64(&perf_callback_frames_worst, frame_count);
  }
  if (budget_ns > 0 && elapsed_ns > budget_ns)
    atomic_fetch_add_uint64(&perf_callback_overruns, 1);
}

static int skred_env_enabled(const char *name) {
  const char *value = getenv(name);
  return value && value[0] && strcmp(value, "0") != 0;
}

static void skred_startup_trace(const char *phase) {
  if (skred_env_enabled("SKRED_TRACE_STARTUP"))
    fprintf(stderr, "# startup: %s\n", phase);
}

static int pattern_voice[PATTERNS_MAX];
static int pattern_voice_generation[PATTERNS_MAX];

static void event_cb(const event_t *event) {
  static skode_t w = SKODE_EMPTY();
  skode_execute_event(event, &w);
}
static void pattern_cb(int pattern, int step, const event_program_t *program) {
  if (pattern < 0 || pattern >= PATTERNS_MAX) return;
  int generation = seq_pattern_generation(pattern);
  if (pattern_voice_generation[pattern] != generation) {
    pattern_voice[pattern] = 0;
    pattern_voice_generation[pattern] = generation;
  }
  skode_execute_program_state(program, &pattern_voice[pattern],
    SAMPLE_COUNT_GET(), -1, pattern, step);
}

static void synth_callback(ma_device* pDevice, void* output, const void* input, ma_uint32 frame_count) {
  struct timespec perf_start = perf_callback_begin();
  static int traced_callback = 0;
  if (!traced_callback && skred_env_enabled("SKRED_TRACE_CALLBACK")) {
    traced_callback = 1;
    fprintf(stderr, "# callback: frames=%u playback_channels=%u capture_channels=%u input=%s\n",
            frame_count, pDevice->playback.channels, pDevice->capture.channels,
            input ? "yes" : "no");
  }
  synth_record_bus_t *capture_bus = NULL;
  synth_record_bus_t *record_bus = recorder_begin_block((int)frame_count);
  if (record_bus) capture_bus = record_bus;
  synth_record_bus_t *scope_bus = scope_ipc_begin_block((int)frame_count);
  if (!capture_bus && scope_bus) capture_bus = scope_bus;
  uint64_t block_end = SAMPLE_COUNT_GET() + (uint64_t)frame_count;
  int offset = 0;
  while (offset < (int)frame_count) {
    uint64_t now = SAMPLE_COUNT_GET();
    uint64_t boundary = 0;
    int immediate_passes = 0;
    do {
      seq(now, event_cb, pattern_cb);
      if (!seq_next_boundary(now, block_end, &boundary) || boundary != now) break;
      immediate_passes++;
    } while (immediate_passes < SEQ_MAX_CATCHUP_TICKS);

    int segment_frames = (int)frame_count - offset;
    if (boundary > now && boundary < block_end) {
      uint64_t until_boundary = boundary - now;
      if (until_boundary < (uint64_t)segment_frames)
        segment_frames = (int)until_boundary;
    }

    float *segment_output = (float *)output +
      ((size_t)offset * pDevice->playback.channels);
    float *segment_input = input ? (float *)input +
      ((size_t)offset * pDevice->capture.channels) : NULL;
    synth_record_bus_t segment_bus;
    synth_record_bus_t *segment_capture_bus = NULL;
    if (capture_bus) {
      segment_bus = *capture_bus;
      segment_bus.frames += (size_t)offset * segment_bus.channels;
      segment_capture_bus = &segment_bus;
    }
    synth(segment_output, segment_input, segment_frames,
          (int)pDevice->playback.channels, segment_capture_bus);
    offset += segment_frames;
  }
  if (scope_bus && capture_bus)
    scope_ipc_publish(capture_bus->frames, (int)frame_count);
  recorder_end_block((int)frame_count);
  perf_callback_end(&perf_start, pDevice, frame_count);
}

static skode_t w = SKODE_EMPTY();

// Audio context and device state
static ma_context synth_context;
static int context_initialized = 0;
static ma_device synth_device;
static int device_initialized = 0;
static int engine_started = 0;

static int udp_port = 60440;

static int ensure_context_initialized(void) {
    if (!context_initialized) {
        if (ma_context_init(NULL, 0, NULL, &synth_context) == MA_SUCCESS) {
            context_initialized = 1;
        } else {
            return -1;
        }
    }
    return 0;
}

#define SKRED_AUDIO_DEVICE_MAX 64
#define SKRED_AUDIO_NAME_MAX 256
#define SKRED_AUDIO_MESSAGE_MAX 32768
#define SKRED_AUDIO_DEFAULT (-1)
#define SKRED_AUDIO_OFF (-2)

typedef struct {
  ma_device_id id;
  char name[SKRED_AUDIO_NAME_MAX];
} skred_audio_device_t;

typedef struct {
  int mode;
  ma_device_id id;
  char name[SKRED_AUDIO_NAME_MAX];
} skred_audio_selection_t;

static skred_audio_device_t indev[SKRED_AUDIO_DEVICE_MAX];
static skred_audio_device_t outdev[SKRED_AUDIO_DEVICE_MAX];
static int n_indev = 0;
static int n_outdev = 0;
static skred_audio_selection_t selected_input = {
  .mode = SKRED_AUDIO_OFF,
  .name = "off"
};
static skred_audio_selection_t selected_output = {
  .mode = SKRED_AUDIO_DEFAULT,
  .name = "default"
};
static char active_input[SKRED_AUDIO_NAME_MAX] = "off";
static char active_output[SKRED_AUDIO_NAME_MAX] = "stopped";
static char audio_status[2048];
static char audio_message[SKRED_AUDIO_MESSAGE_MAX];

static void audio_copy_name(char *dst, size_t dst_size, const char *src) {
  if (!dst || dst_size == 0) return;
  snprintf(dst, dst_size, "%s", src ? src : "");
}

int skred_audio_refresh(void) {
  ma_device_info *playback_infos = NULL;
  ma_device_info *capture_infos = NULL;
  ma_uint32 playback_count = 0;
  ma_uint32 capture_count = 0;
  if (ensure_context_initialized() != 0) return -1;
  if (ma_context_get_devices(&synth_context,
                             &playback_infos, &playback_count,
                             &capture_infos, &capture_count) != MA_SUCCESS) {
    return -1;
  }

  n_outdev = (playback_count > SKRED_AUDIO_DEVICE_MAX)
               ? SKRED_AUDIO_DEVICE_MAX : (int)playback_count;
  for (int i = 0; i < n_outdev; i++) {
    outdev[i].id = playback_infos[i].id;
    audio_copy_name(outdev[i].name, sizeof(outdev[i].name),
                    playback_infos[i].name);
  }
  n_indev = (capture_count > SKRED_AUDIO_DEVICE_MAX)
              ? SKRED_AUDIO_DEVICE_MAX : (int)capture_count;
  for (int i = 0; i < n_indev; i++) {
    indev[i].id = capture_infos[i].id;
    audio_copy_name(indev[i].name, sizeof(indev[i].name),
                    capture_infos[i].name);
  }
  return 0;
}

int skred_devices(int isCapture) {
  return isCapture ? n_indev : n_outdev;
}

int skred_device_idx(int isCapture, int idx) {
  int count = isCapture ? n_indev : n_outdev;
  return (idx >= 0 && idx < count) ? idx : -1;
}

char *skred_device_str(int isCapture, int idx) {
  int count = isCapture ? n_indev : n_outdev;
  if (idx < 0 || idx >= count) return "";
  return isCapture ? indev[idx].name : outdev[idx].name;
}

int skred_enumerate_devices(int isCapture) {
  if (skred_audio_refresh() != 0) return -1;
  return skred_devices(isCapture);
}

static int audio_init_selected_device(unsigned int sample_rate) {
  ma_device_type type = selected_input.mode == SKRED_AUDIO_OFF
                          ? ma_device_type_playback : ma_device_type_duplex;
  ma_device_config config = ma_device_config_init(type);
  if (selected_output.mode >= 0) {
    config.playback.pDeviceID = &selected_output.id;
  }
  if (selected_input.mode >= 0) {
    config.capture.pDeviceID = &selected_input.id;
  }
  config.playback.format = ma_format_f32;
  config.playback.channels = 0;
  config.capture.format = ma_format_f32;
  config.capture.channels = AUDIO_CHANNELS;
  config.sampleRate = sample_rate;
  config.dataCallback = synth_callback;
  config.periodSizeInFrames = requested_synth_frames_per_callback;
  config.pUserData = NULL;

  if (ma_device_init(&synth_context, &config, &synth_device) != MA_SUCCESS) {
    return -1;
  }
  device_initialized = 1;
  ma_device_info info;
  if (ma_device_get_info(&synth_device, ma_device_type_playback, &info) == MA_SUCCESS) {
    audio_copy_name(active_output, sizeof(active_output), info.name);
  } else {
    audio_copy_name(active_output, sizeof(active_output), selected_output.name);
  }
  if (selected_input.mode != SKRED_AUDIO_OFF &&
      ma_device_get_info(&synth_device, ma_device_type_capture, &info) == MA_SUCCESS) {
    audio_copy_name(active_input, sizeof(active_input), info.name);
  } else {
    audio_copy_name(active_input, sizeof(active_input), selected_input.name);
  }
  return 0;
}

static int audio_start_selected_device(void) {
  if (!device_initialized) return -1;
  if (ma_device_start(&synth_device) != MA_SUCCESS) {
    ma_device_uninit(&synth_device);
    device_initialized = 0;
    audio_copy_name(active_output, sizeof(active_output), "stopped");
    audio_copy_name(active_input, sizeof(active_input), "stopped");
    return -1;
  }
  return 0;
}

int skred_audio_disconnect(void) {
  if (device_initialized) {
    ma_device_uninit(&synth_device);
    device_initialized = 0;
  }
  audio_copy_name(active_output, sizeof(active_output), "stopped");
  audio_copy_name(active_input, sizeof(active_input), "stopped");
  return 0;
}

int skred_audio_reconnect(void) {
  if (!engine_started) return 0;
  if (ensure_context_initialized() != 0) return -1;
  skred_audio_disconnect();
  if (audio_init_selected_device((unsigned int)MAIN_SAMPLE_RATE) != 0) return -1;
  return audio_start_selected_device();
}

int skred_audio_select(int is_capture, int selection) {
  skred_audio_device_t *devices = is_capture ? indev : outdev;
  int count = is_capture ? n_indev : n_outdev;
  skred_audio_selection_t *selected =
    is_capture ? &selected_input : &selected_output;
  skred_audio_selection_t previous = *selected;

  if (selection == SKRED_AUDIO_OFF && !is_capture) return -1;
  if (selection < SKRED_AUDIO_OFF || selection >= count) return -1;
  if (selection >= 0) {
    selected->mode = selection;
    selected->id = devices[selection].id;
    audio_copy_name(selected->name, sizeof(selected->name),
                    devices[selection].name);
  } else {
    selected->mode = selection;
    audio_copy_name(selected->name, sizeof(selected->name),
                    selection == SKRED_AUDIO_OFF ? "off" : "default");
  }
  if (skred_audio_reconnect() == 0) return 0;

  *selected = previous;
  skred_audio_reconnect();
  return -1;
}

int skred_audio_running(void) {
  return device_initialized && ma_device_is_started(&synth_device);
}

char *skred_audio_status(void) {
  int running = skred_audio_running();
  unsigned int output_channels = device_initialized ? synth_device.playback.channels : 0;
  unsigned int input_channels =
    (device_initialized && selected_input.mode != SKRED_AUDIO_OFF)
      ? synth_device.capture.channels : 0;
  unsigned int sample_rate = device_initialized
    ? synth_device.sampleRate : (unsigned int)synth_sample_rate_get();
  unsigned int output_pairs = output_channels / AUDIO_CHANNELS;
  unsigned int skred_pairs = RECORD_TRACK_COUNT;
  unsigned int stem_pairs = output_pairs > 0 ? output_pairs - 1 : 0;
  if (stem_pairs > RECORD_TRACK_MAX) stem_pairs = RECORD_TRACK_MAX;
  snprintf(audio_status, sizeof(audio_status),
           "audio: %s\n"
           "out: [%s]\n"
           "  requested: [%s]\n"
           "  channels: %u pairs: %u"
           " skred-pairs: %u stem-pairs: %u"
           "\n"
           "in: [%s]\n"
           "  requested: [%s]\n"
           "  channels: %u\n"
           "rate: %u callback-frames: %d\n"
           "%s",
           running ? "running" : "stopped",
           active_output, selected_output.name,
           output_channels, output_pairs
           , skred_pairs, stem_pairs
           , active_input, selected_input.name,
           input_channels,
           sample_rate, requested_synth_frames_per_callback,
           skred_performance_status());
  return audio_status;
}

char *skred_audio_message(void) {
  return audio_message;
}

static int audio_parse_selection(const char *arg, int is_capture, int *selection) {
  char *end = NULL;
  long value;
  if (!arg || arg[0] == '\0') return -1;
  if (strcmp(arg, "default") == 0) {
    *selection = SKRED_AUDIO_DEFAULT;
    return 0;
  }
  if (is_capture && strcmp(arg, "off") == 0) {
    *selection = SKRED_AUDIO_OFF;
    return 0;
  }
  value = strtol(arg, &end, 10);
  if (!end || end == arg || *end != '\0' || value < 0 || value > INT32_MAX) {
    return -1;
  }
  *selection = (int)value;
  return 0;
}

int skred_audio_command(const char *line) {
  char command[16] = {0};
  char argument[64] = {0};
  const char *p = line;
  int selection;
  int fields;
  audio_message[0] = '\0';
  if (!p) return 0;
  while (isspace((unsigned char)*p)) p++;
  fields = sscanf(p, "%15s %63s", command, argument);

  if (strcmp(command, "/als") == 0) {
    if (fields != 1) {
      snprintf(audio_message, sizeof(audio_message), "# usage: /als");
      return -1;
    }
    if (skred_audio_refresh() != 0) {
      snprintf(audio_message, sizeof(audio_message),
               "# audio device refresh failed");
      return -1;
    }
    size_t used = (size_t)snprintf(audio_message, sizeof(audio_message),
                                   "# output devices\n");
    for (int i = 0; i < n_outdev && used < sizeof(audio_message); i++) {
      used += (size_t)snprintf(audio_message + used,
                               sizeof(audio_message) - used,
                               "%d = %s\n", i, outdev[i].name);
    }
    if (used < sizeof(audio_message)) {
      used += (size_t)snprintf(audio_message + used,
                               sizeof(audio_message) - used,
                               "# input devices\n");
    }
    for (int i = 0; i < n_indev && used < sizeof(audio_message); i++) {
      used += (size_t)snprintf(audio_message + used,
                               sizeof(audio_message) - used,
                               "%d = %s\n", i, indev[i].name);
    }
    return 1;
  }

  if (strcmp(command, "/a?") == 0) {
    if (fields != 1) {
      snprintf(audio_message, sizeof(audio_message), "# usage: /a?");
      return -1;
    }
    snprintf(audio_message, sizeof(audio_message), "# %s",
             skred_audio_status());
    return 1;
  }

  int is_capture;
  if (strcmp(command, "/aout") == 0) is_capture = 0;
  else if (strcmp(command, "/ain") == 0) is_capture = 1;
  else return 0;

  if (fields != 2 ||
      audio_parse_selection(argument, is_capture, &selection) != 0) {
    snprintf(audio_message, sizeof(audio_message),
             "# usage: %s <device|default%s>",
             command, is_capture ? "|off" : "");
    return -1;
  }
  if (selection >= 0 &&
      (is_capture ? n_indev : n_outdev) == 0 &&
      skred_audio_refresh() != 0) {
    snprintf(audio_message, sizeof(audio_message),
             "# audio device refresh failed");
    return -1;
  }
  if (skred_audio_select(is_capture, selection) != 0) {
    snprintf(audio_message, sizeof(audio_message),
             "# audio selection failed: %s %s", command, argument);
    return -1;
  }
  snprintf(audio_message, sizeof(audio_message), "# %s",
           skred_audio_status());
  return 1;
}

void skred_set_audio_device(int playback_idx, int capture_idx) {
  if (n_outdev == 0 || n_indev == 0) skred_audio_refresh();
  skred_audio_select(0, playback_idx < 0 ? SKRED_AUDIO_DEFAULT : playback_idx);
  skred_audio_select(1, capture_idx < 0 ? SKRED_AUDIO_OFF : capture_idx);
}

int skred_start(unsigned int req_audio_frames, unsigned int voices, int port) {
    // 1. Initialize synth state
  skred_startup_trace("begin");
  int req = req_audio_frames;
  int vc = voices;
  requested_synth_frames_per_callback = req;
  skred_performance_reset();
  skred_control_event_reset();

  int audio_disabled = skred_env_enabled("SKRED_NO_AUDIO");
  if (audio_disabled) {
    synth_sample_rate_set(SKRED_DEFAULT_SAMPLE_RATE);
  } else {
    skred_startup_trace("audio_context");
    if (ensure_context_initialized() != 0) return -1;
    skred_startup_trace("audio_init");
    if (audio_init_selected_device(0) != 0) return -1;
    synth_sample_rate_set((int)synth_device.sampleRate);
  }

  skred_startup_trace("synth_init");
  synth_init(vc);
  skred_startup_trace("wave_table_init");
  wave_table_init(0);
  skred_startup_trace("voice_init");
  voice_init();
  skred_startup_trace("seq_init");
  seq_init();
  skred_startup_trace("tempo_set");
  tempo_set(120.0);
  memset(pattern_voice, 0, sizeof(pattern_voice));
  for (int p = 0; p < PATTERNS_MAX; p++)
    pattern_voice_generation[p] = seq_pattern_generation(p);

    // 2. Start UDP listening thread
  skred_startup_trace("udp_start");
  udp_port = port;
  if (udp_port > 0) {
    int r = udp_start(udp_port);
    if (r != udp_port) udp_port = 0;
  }

  skred_startup_trace("recorder_init");
  if (recorder_init(ONE_FRAME_MAX, MAIN_SAMPLE_RATE) != 0) {
    if (!audio_disabled) skred_audio_disconnect();
    return -1;
  }
  skred_startup_trace("scope_ipc_init");
  if (scope_ipc_init(ONE_FRAME_MAX, MAIN_SAMPLE_RATE) != 0) {
    if (!audio_disabled) skred_audio_disconnect();
    return -1;
  }

  if (audio_disabled) {
    audio_copy_name(active_output, sizeof(active_output), "disabled");
    audio_copy_name(active_input, sizeof(active_input), "disabled");
    engine_started = 1;
    skred_startup_trace("audio skipped");
    return 0;
  }

  engine_started = 1;
  skred_startup_trace("audio_start");
  if (audio_start_selected_device() != 0) {
    engine_started = 0;
    return -1;
  }
  skred_startup_trace("started");

  return 0;
}

int skred_command(char* line) {
    // Inject a command directly into the skqueue or seq parser
    // This allows local control without hitting the UDP layer
    int n = skode_consume(line, &w);
    // if (w->log_len) printf("%s", w->log);
    // if (n < 0) break; // request to stop or error
    // if (n > 0) printf("# ERR:%d\n", n);
    return n;
}

void skred_stop(void) {
  udp_stop();
  // turn down volume smoothly to avoid clicks
  volume_set(-90);
  skred_audio_disconnect();
  engine_started = 0;
  recorder_uninit();
  scope_ipc_uninit();
  if (context_initialized) {
    ma_context_uninit(&synth_context);
    context_initialized = 0;
  }
  skode_free(&w);
  wave_free();
  synth_free();
}

int skred_record_start(const char *filename, double max_seconds) {
  return recorder_start(filename, max_seconds);
}

int skred_record_stop(void) {
  return recorder_stop();
}

int skred_record_state(void) {
  return recorder_state();
}

uint64_t skred_record_frames_written(void) {
  return recorder_frames_written();
}

uint64_t skred_record_dropped_frames(void) {
  return recorder_dropped_frames();
}

int skred_scope_start(const char *name, uint32_t channel_mask,
                      double buffer_seconds) {
  return scope_ipc_start(name, channel_mask, buffer_seconds);
}

int skred_scope_stop(void) {
  return scope_ipc_stop();
}

static char _features_[65536] = {0};
#define CAT(x) {strcat(_features_, #x);strcat(_features_," ");}
char *skred_features(void) {
CAT(ADSR)
CAT(AM)
CAT(CRUSH)
CAT(FADSR)
CAT(FILT)
CAT(FM)
CAT(GLISS)
CAT(PANMOD)
CAT(PD)
CAT(SAH)
CAT(SEQ)
CAT(SMOOTHER)
CAT(UDP)
CAT(KSYNTH)
CAT(RECORD)
CAT(SCOPE)
return _features_;
}

const char *skred_version(void) {
  return SKRED_VERSION;
}

int skred_version_major(void) {
  return SKRED_VERSION_MAJOR;
}

int skred_version_minor(void) {
  return SKRED_VERSION_MINOR;
}

int skred_version_patch(void) {
  return SKRED_VERSION_PATCH;
}

char *skred_log(void) {
  return w.log;
}

void skred_logger(int f) {
  w.log_enable = f;
}
