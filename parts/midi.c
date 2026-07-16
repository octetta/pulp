#if defined(__linux__) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "api.h"
#include "midi.h"
#include "portable_atomic.h"
#include "skode.h"
#include "synth.h"
#include "synth-config.h"
#include "synth-state.h"

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__linux__)
#include <alloca.h>
#endif

#define SKRED_MIDI_NAME_MAX 128
#define SKRED_MIDI_MESSAGE_MAX 4096

static char midi_message[SKRED_MIDI_MESSAGE_MAX];
static atomic_int_t midi_mask = (int)(0xffffffffu & ~(1u << SKRED_MIDI_SYSEX) &
  ~(1u << SKRED_MIDI_ACTIVE_SENSE));

typedef struct {
  int used;
  int channel;
  int target_type;
  int target;
  float bend_semitones;
  int active_key;
  uint8_t held[16][128];
  float bend_cents[16];
} midi_route_t;

static midi_route_t midi_route[SKRED_MIDI_ROUTE_MAX];
typedef struct {
  int used;
  int type;
  int channel;
  int data1;
  char command[SKRED_MIDI_BINDING_COMMAND_MAX];
} midi_binding_t;

static midi_binding_t midi_binding[SKRED_MIDI_BINDING_MAX];
static simple_mutex_t midi_route_mutex;
static atomic_int_t midi_route_init_state;
static char midi_route_message[4096];
static char midi_binding_message[8192];

static void midi_route_init(void) {
  int state = atomic_load_int(&midi_route_init_state);
  if (state == 2) return;
  if (state == 0) {
    int expected = 0;
    if (atomic_compare_exchange_int(&midi_route_init_state, &expected, 1)) {
      simple_mutex_init(&midi_route_mutex);
      for (int i = 0; i < SKRED_MIDI_ROUTE_MAX; i++)
        midi_route[i].active_key = -1;
      atomic_store_int(&midi_route_init_state, 2);
      return;
    }
  }
  while (atomic_load_int(&midi_route_init_state) != 2) {
  }
}

static int midi_route_channel_valid(int channel) {
  return channel == -1 || (channel >= 0 && channel < 16);
}

static int midi_message_type_valid(int type) {
  switch (type) {
    case SKRED_MIDI_NOTE_OFF: case SKRED_MIDI_NOTE_ON:
    case SKRED_MIDI_POLY_PRESSURE: case SKRED_MIDI_CONTROL_CHANGE:
    case SKRED_MIDI_PROGRAM_CHANGE: case SKRED_MIDI_CHANNEL_PRESSURE:
    case SKRED_MIDI_PITCH_BEND: case SKRED_MIDI_MTC_QUARTER_FRAME:
    case SKRED_MIDI_SONG_POSITION: case SKRED_MIDI_SONG_SELECT:
    case SKRED_MIDI_TUNE_REQUEST: case SKRED_MIDI_CLOCK:
    case SKRED_MIDI_START: case SKRED_MIDI_CONTINUE: case SKRED_MIDI_STOP:
    case SKRED_MIDI_RESET:
      return 1;
    default:
      return 0;
  }
}

int skred_midi_binding_set(int type, int channel, int data1,
    const char *command) {
  if (!midi_message_type_valid(type) || !midi_route_channel_valid(channel) ||
      data1 < -1 || data1 > 16383 || !command || !command[0] ||
      strlen(command) >= SKRED_MIDI_BINDING_COMMAND_MAX) return -1;
  midi_route_init();
  simple_mutex_lock(&midi_route_mutex);
  int slot = -1;
  for (int i = 0; i < SKRED_MIDI_BINDING_MAX; i++) {
    if (midi_binding[i].used && midi_binding[i].type == type &&
        midi_binding[i].channel == channel && midi_binding[i].data1 == data1) {
      slot = i;
      break;
    }
    if (slot < 0 && !midi_binding[i].used) slot = i;
  }
  if (slot < 0) {
    simple_mutex_unlock(&midi_route_mutex);
    return -1;
  }
  midi_binding[slot].used = 1;
  midi_binding[slot].type = type;
  midi_binding[slot].channel = channel;
  midi_binding[slot].data1 = data1;
  snprintf(midi_binding[slot].command, sizeof(midi_binding[slot].command),
    "%s", command);
  simple_mutex_unlock(&midi_route_mutex);
  return 0;
}

int skred_midi_binding_remove(int type, int channel, int data1) {
  if (!midi_message_type_valid(type) || !midi_route_channel_valid(channel) ||
      data1 < -1 || data1 > 16383) return -1;
  midi_route_init();
  simple_mutex_lock(&midi_route_mutex);
  int removed = 0;
  for (int i = 0; i < SKRED_MIDI_BINDING_MAX; i++) {
    if (!midi_binding[i].used || midi_binding[i].type != type ||
        midi_binding[i].channel != channel || midi_binding[i].data1 != data1)
      continue;
    memset(&midi_binding[i], 0, sizeof(midi_binding[i]));
    removed++;
  }
  simple_mutex_unlock(&midi_route_mutex);
  return removed;
}

void skred_midi_binding_clear(void) {
  midi_route_init();
  simple_mutex_lock(&midi_route_mutex);
  memset(midi_binding, 0, sizeof(midi_binding));
  simple_mutex_unlock(&midi_route_mutex);
}

int skred_midi_binding_count(void) {
  midi_route_init();
  simple_mutex_lock(&midi_route_mutex);
  int count = 0;
  for (int i = 0; i < SKRED_MIDI_BINDING_MAX; i++)
    if (midi_binding[i].used) count++;
  simple_mutex_unlock(&midi_route_mutex);
  return count;
}

const char *skred_midi_binding_status(void) {
  midi_route_init();
  simple_mutex_lock(&midi_route_mutex);
  size_t used = (size_t)snprintf(midi_binding_message,
    sizeof(midi_binding_message), "# MIDI Skode bindings\n");
  int count = 0;
  for (int i = 0; i < SKRED_MIDI_BINDING_MAX &&
      used < sizeof(midi_binding_message); i++) {
    midi_binding_t *binding = &midi_binding[i];
    if (!binding->used) continue;
    count++;
    char channel[16], data1[16];
    if (binding->channel < 0) snprintf(channel, sizeof(channel), ".");
    else snprintf(channel, sizeof(channel), "%d", binding->channel);
    if (binding->data1 < 0) snprintf(data1, sizeof(data1), ".");
    else snprintf(data1, sizeof(data1), "%d", binding->data1);
    int written = snprintf(midi_binding_message + used,
      sizeof(midi_binding_message) - used, "#   [%s] /mb %d,%s,%s\n",
      binding->command, binding->type, channel, data1);
    if (written < 0 || (size_t)written >=
        sizeof(midi_binding_message) - used) break;
    used += (size_t)written;
  }
  if (!count && used < sizeof(midi_binding_message))
    (void)snprintf(midi_binding_message + used,
      sizeof(midi_binding_message) - used, "#   none\n");
  simple_mutex_unlock(&midi_route_mutex);
  return midi_binding_message;
}

static int midi_route_target_valid(int target_type, int target) {
  if (target_type == SKRED_MIDI_ROUTE_VOICE)
    return target >= 0 && target < VOICE_MAX_HARD_LIMIT;
  if (target_type == SKRED_MIDI_ROUTE_POOL)
    return target >= 0 && target < SKRED_POLY_POOL_MAX;
  return 0;
}

static void midi_route_release_all(midi_route_t *route) {
  if (!route || !route->used) return;
  if (route->target_type == SKRED_MIDI_ROUTE_VOICE) {
    if (route->active_key >= 0)
      skode_envelope_velocity(route->target, 0.0f, SAMPLE_COUNT_GET());
    route->active_key = -1;
    return;
  }
  for (int channel = 0; channel < 16; channel++) {
    for (int note = 0; note < 128; note++) {
      if (route->held[channel][note])
        (void)skred_poly_release(route->target, channel * 128 + note, 0.0f);
    }
  }
}

int skred_midi_route_set(int channel, int target_type, int target,
    float bend_semitones) {
  if (!midi_route_channel_valid(channel) ||
      !midi_route_target_valid(target_type, target) ||
      !isfinite(bend_semitones) || bend_semitones < 0.0f) return -1;
  midi_route_init();
  simple_mutex_lock(&midi_route_mutex);
  int slot = -1;
  for (int i = 0; i < SKRED_MIDI_ROUTE_MAX; i++) {
    if (midi_route[i].used && midi_route[i].channel == channel &&
        midi_route[i].target_type == target_type &&
        midi_route[i].target == target) {
      slot = i;
      break;
    }
    if (slot < 0 && !midi_route[i].used) slot = i;
  }
  if (slot < 0) {
    simple_mutex_unlock(&midi_route_mutex);
    return -1;
  }
  midi_route_release_all(&midi_route[slot]);
  memset(&midi_route[slot], 0, sizeof(midi_route[slot]));
  midi_route[slot].used = 1;
  midi_route[slot].channel = channel;
  midi_route[slot].target_type = target_type;
  midi_route[slot].target = target;
  midi_route[slot].bend_semitones = bend_semitones;
  midi_route[slot].active_key = -1;
  simple_mutex_unlock(&midi_route_mutex);
  return 0;
}

int skred_midi_route_remove(int channel, int target_type, int target) {
  if (!midi_route_channel_valid(channel) ||
      !midi_route_target_valid(target_type, target)) return -1;
  midi_route_init();
  simple_mutex_lock(&midi_route_mutex);
  int removed = 0;
  for (int i = 0; i < SKRED_MIDI_ROUTE_MAX; i++) {
    if (!midi_route[i].used || midi_route[i].channel != channel ||
        midi_route[i].target_type != target_type ||
        midi_route[i].target != target) continue;
    midi_route_release_all(&midi_route[i]);
    memset(&midi_route[i], 0, sizeof(midi_route[i]));
    midi_route[i].active_key = -1;
    removed++;
  }
  simple_mutex_unlock(&midi_route_mutex);
  return removed;
}

void skred_midi_route_clear(void) {
  midi_route_init();
  simple_mutex_lock(&midi_route_mutex);
  for (int i = 0; i < SKRED_MIDI_ROUTE_MAX; i++)
    midi_route_release_all(&midi_route[i]);
  memset(midi_route, 0, sizeof(midi_route));
  for (int i = 0; i < SKRED_MIDI_ROUTE_MAX; i++)
    midi_route[i].active_key = -1;
  simple_mutex_unlock(&midi_route_mutex);
}

int skred_midi_route_count(void) {
  midi_route_init();
  simple_mutex_lock(&midi_route_mutex);
  int count = 0;
  for (int i = 0; i < SKRED_MIDI_ROUTE_MAX; i++)
    if (midi_route[i].used) count++;
  simple_mutex_unlock(&midi_route_mutex);
  return count;
}

const char *skred_midi_route_status(void) {
  midi_route_init();
  simple_mutex_lock(&midi_route_mutex);
  size_t used = 0;
  int count = 0;
  used += (size_t)snprintf(midi_route_message + used,
    sizeof(midi_route_message) - used, "# MIDI routes\n");
  for (int i = 0; i < SKRED_MIDI_ROUTE_MAX &&
      used < sizeof(midi_route_message); i++) {
    midi_route_t *route = &midi_route[i];
    if (!route->used) continue;
    count++;
    int written;
    if (route->channel < 0) {
      written = snprintf(midi_route_message + used,
        sizeof(midi_route_message) - used, "#   /m%c .,%d,%g\n",
        route->target_type == SKRED_MIDI_ROUTE_VOICE ? 'v' : 'p',
        route->target, route->bend_semitones);
    } else {
      written = snprintf(midi_route_message + used,
        sizeof(midi_route_message) - used, "#   /m%c %d,%d,%g\n",
        route->target_type == SKRED_MIDI_ROUTE_VOICE ? 'v' : 'p',
        route->channel, route->target, route->bend_semitones);
    }
    if (written < 0) break;
    if ((size_t)written >= sizeof(midi_route_message) - used) {
      used = sizeof(midi_route_message);
      break;
    }
    used += (size_t)written;
  }
  if (!count && used < sizeof(midi_route_message))
    (void)snprintf(midi_route_message + used,
      sizeof(midi_route_message) - used, "#   none\n");
  simple_mutex_unlock(&midi_route_mutex);
  return midi_route_message;
}

static float midi_bend_cents(int lsb, int msb, float range) {
  int raw = ((msb & 0x7f) << 7) | (lsb & 0x7f);
  float unit = raw < 8192 ? (float)(raw - 8192) / 8192.0f :
    (float)(raw - 8192) / 8191.0f;
  return unit * range * 100.0f;
}

static int midi_template_append(char *out, size_t size, size_t *used,
    const char *text) {
  size_t length = strlen(text);
  if (*used + length >= size) return -1;
  memcpy(out + *used, text, length);
  *used += length;
  out[*used] = '\0';
  return 0;
}

static int midi_expand_command(const char *input, char *out, size_t size,
    int channel, int data1, int data2) {
  size_t used = 0;
  char value[48];
  out[0] = '\0';
  for (size_t i = 0; input[i]; ) {
    const char *replacement = NULL;
    size_t token = 0;
    if (!strncmp(input + i, "{ch}", 4)) {
      snprintf(value, sizeof(value), "%d", channel);
      replacement = value; token = 4;
    } else if (!strncmp(input + i, "{d1}", 4)) {
      snprintf(value, sizeof(value), "%d", data1);
      replacement = value; token = 4;
    } else if (!strncmp(input + i, "{d2}", 4)) {
      snprintf(value, sizeof(value), "%d", data2);
      replacement = value; token = 4;
    } else if (!strncmp(input + i, "{unit}", 6)) {
      snprintf(value, sizeof(value), "%.9g", (double)data2 / 127.0);
      replacement = value; token = 6;
    } else if (!strncmp(input + i, "{bend}", 6)) {
      snprintf(value, sizeof(value), "%.9g",
        (double)midi_bend_cents(data1, data2, 1.0f) / 100.0);
      replacement = value; token = 6;
    }
    if (replacement) {
      if (midi_template_append(out, size, &used, replacement) != 0) return -1;
      i += token;
    } else {
      if (used + 1 >= size) return -1;
      out[used++] = input[i++];
      out[used] = '\0';
    }
  }
  return 0;
}

static int midi_route_pool_event(midi_route_t *route, int type, int channel,
    int data1, int data2) {
  int key = channel * 128 + data1;
  if (type == SKRED_MIDI_NOTE_ON && data2 > 0) {
    route->held[channel][data1] = 1;
    int result = skred_poly_note(route->target, key, (float)data1,
      (float)data2 / 127.0f, 0.0f);
    if (result == 0 && route->bend_cents[channel] != 0.0f)
      result = skred_poly_bend(route->target, key, 0.0f,
        route->bend_cents[channel]);
    return result;
  }
  if (type == SKRED_MIDI_NOTE_OFF ||
      (type == SKRED_MIDI_NOTE_ON && data2 == 0)) {
    route->held[channel][data1] = 0;
    return skred_poly_release(route->target, key, (float)data2 / 127.0f);
  }
  if (type == SKRED_MIDI_PITCH_BEND) {
    int handled = 0;
    route->bend_cents[channel] = midi_bend_cents(data1, data2,
      route->bend_semitones);
    for (int note = 0; note < 128; note++) {
      if (!route->held[channel][note]) continue;
      if (skred_poly_bend(route->target, channel * 128 + note, 0.0f,
          route->bend_cents[channel]) == 0) handled++;
    }
    return handled;
  }
  return 0;
}

static int midi_route_voice_event(midi_route_t *route, int type, int channel,
    int data1, int data2) {
  int key = channel * 128 + data1;
  if (type == SKRED_MIDI_NOTE_ON && data2 > 0) {
    route->active_key = key;
    (void)skode_midi_note(route->target, (float)data1,
      route->bend_cents[channel]);
    skode_envelope_velocity(route->target, (float)data2 / 127.0f,
      SAMPLE_COUNT_GET());
    return 1;
  }
  if (type == SKRED_MIDI_NOTE_OFF ||
      (type == SKRED_MIDI_NOTE_ON && data2 == 0)) {
    if (route->active_key != key) return 0;
    route->active_key = -1;
    skode_envelope_velocity(route->target, 0.0f, SAMPLE_COUNT_GET());
    return 1;
  }
  if (type == SKRED_MIDI_PITCH_BEND) {
    route->bend_cents[channel] = midi_bend_cents(data1, data2,
      route->bend_semitones);
    if (route->active_key < 0 || route->active_key / 128 != channel) return 0;
    return skode_midi_note(route->target,
      (float)(route->active_key % 128), route->bend_cents[channel]) == 0;
  }
  return 0;
}

int skred_midi_route_event(const skred_control_event_t *event) {
  if (!event || event->type != SKRED_CONTROL_EVENT_MIDI ||
      event->value_count < 3) return 0;
  int type = event->id;
  int channel = (int)event->value[0];
  int data1 = (int)event->value[1];
  int data2 = (int)event->value[2];
  if (!midi_message_type_valid(type) || channel < -1 || channel >= 16 ||
      data1 < 0 || data1 > 16383 || data2 < 0 || data2 > 127) return 0;
  midi_route_init();
  simple_mutex_lock(&midi_route_mutex);
  int handled = 0;
  if (channel >= 0 && data1 <= 127 &&
      (type == SKRED_MIDI_NOTE_ON || type == SKRED_MIDI_NOTE_OFF ||
       type == SKRED_MIDI_PITCH_BEND)) {
    for (int i = 0; i < SKRED_MIDI_ROUTE_MAX; i++) {
      midi_route_t *route = &midi_route[i];
      if (!route->used || (route->channel != -1 &&
          route->channel != channel)) continue;
      int result = route->target_type == SKRED_MIDI_ROUTE_VOICE ?
        midi_route_voice_event(route, type, channel, data1, data2) :
        midi_route_pool_event(route, type, channel, data1, data2);
      if (result >= 0) handled += result;
    }
  }
  char commands[SKRED_MIDI_BINDING_MAX][SKRED_MIDI_BINDING_COMMAND_MAX];
  int command_count = 0;
  for (int i = 0; i < SKRED_MIDI_BINDING_MAX; i++) {
    midi_binding_t *binding = &midi_binding[i];
    if (!binding->used || binding->type != type ||
        (binding->channel != -1 && binding->channel != channel) ||
        (binding->data1 != -1 && binding->data1 != data1)) continue;
    snprintf(commands[command_count], sizeof(commands[command_count]), "%s",
      binding->command);
    command_count++;
  }
  simple_mutex_unlock(&midi_route_mutex);
  for (int i = 0; i < command_count; i++) {
    char expanded[SKRED_MIDI_BINDING_COMMAND_MAX];
    if (midi_expand_command(commands[i], expanded, sizeof(expanded), channel,
        data1, data2) == 0 && skred_control_midi_command(expanded) >= 0)
      handled++;
  }
  return handled;
}

static void midi_publish(int type, int channel, int data1, int data2) {
  uint32_t mask = (uint32_t)atomic_load_int(&midi_mask);
  if (type < 0 || type >= 32 || !(mask & (1u << type))) return;
  skred_control_midi_event(type, channel, data1, data2);
}

#if defined(SKRED_MIDI_ENABLED)
#define MINIMIDIO_IMPLEMENTATION
#include "vendor/minimidio/minimidio.h"

static mm_context midi_context;
static mm_device midi_input;
static mm_device midi_output;
static int midi_initialized;
static int midi_input_opened;
static int midi_output_opened;
static int midi_input_virtual;
static int midi_output_virtual;
static int midi_input_index = -1;
static int midi_output_index = -1;
static char midi_client_name[SKRED_MIDI_NAME_MAX] = "pulp";

static int midi_result(mm_result result) {
  return result == MM_SUCCESS ? 0 : (int)result;
}

static void midi_input_callback(mm_device *device, const mm_message *message,
    void *userdata) {
  (void)device;
  (void)userdata;
  if (!message || message->type == MM_SYSEX) return;
  int data1 = message->data[0];
  int data2 = message->data[1];
  if (message->type == MM_SONG_POSITION) {
    data1 = message->song_position;
    data2 = 0;
  }
  midi_publish((int)message->type,
    message->type >= MM_NOTE_OFF && message->type <= MM_PITCH_BEND
      ? (int)message->channel : -1,
    data1, data2);
}

int skred_midi_init(const char *client_name) {
  if (midi_initialized) {
    if (!client_name || !client_name[0] ||
        strcmp(client_name, midi_client_name) == 0) return 0;
    if (midi_input_opened || midi_output_opened) return MM_ALREADY_OPEN;
    (void)mm_context_uninit(&midi_context);
    memset(&midi_context, 0, sizeof(midi_context));
    midi_initialized = 0;
  }
  if (client_name && client_name[0]) {
    snprintf(midi_client_name, sizeof(midi_client_name), "%s", client_name);
  }
  mm_result result = mm_context_init(&midi_context, midi_client_name);
  if (result != MM_SUCCESS) return (int)result;
  midi_initialized = 1;
  return 0;
}

void skred_midi_uninit(void) {
  if (!midi_initialized) return;
  (void)skred_midi_input_close();
  (void)skred_midi_output_close();
  (void)mm_context_uninit(&midi_context);
  memset(&midi_context, 0, sizeof(midi_context));
  midi_initialized = 0;
}

uint32_t skred_midi_caps(void) {
  return midi_initialized ? mm_context_caps(&midi_context) : 0;
}

int skred_midi_input_count(void) {
  return midi_initialized ? (int)mm_in_count(&midi_context) : -1;
}

int skred_midi_output_count(void) {
  return midi_initialized ? (int)mm_out_count(&midi_context) : -1;
}

int skred_midi_input_name(int index, char *name, int name_size) {
  if (!midi_initialized || !name || name_size <= 0 || index < 0)
    return MM_INVALID_ARG;
  return midi_result(mm_in_name(&midi_context, (uint32_t)index, name,
    (size_t)name_size));
}

int skred_midi_output_name(int index, char *name, int name_size) {
  if (!midi_initialized || !name || name_size <= 0 || index < 0)
    return MM_INVALID_ARG;
  return midi_result(mm_out_name(&midi_context, (uint32_t)index, name,
    (size_t)name_size));
}

int skred_midi_input_close(void) {
  if (!midi_input_opened) return 0;
  mm_result stop_result = mm_in_stop(&midi_input);
  mm_result close_result = mm_in_close(&midi_input);
  memset(&midi_input, 0, sizeof(midi_input));
  midi_input_opened = 0;
  midi_input_virtual = 0;
  midi_input_index = -1;
  return close_result != MM_SUCCESS ? (int)close_result :
    (stop_result == MM_SUCCESS || stop_result == MM_NOT_OPEN
      ? 0 : (int)stop_result);
}

static int midi_input_finish_open(mm_result result, int index,
    int is_virtual) {
  if (result != MM_SUCCESS) return (int)result;
  result = mm_in_start(&midi_input);
  if (result != MM_SUCCESS) {
    (void)mm_in_close(&midi_input);
    memset(&midi_input, 0, sizeof(midi_input));
    return (int)result;
  }
  midi_input_opened = 1;
  midi_input_virtual = is_virtual;
  midi_input_index = index;
  return 0;
}

int skred_midi_input_open(int index) {
  if (!midi_initialized || index < 0) return MM_INVALID_ARG;
  (void)skred_midi_input_close();
  return midi_input_finish_open(mm_in_open(&midi_context, &midi_input,
    (uint32_t)index, midi_input_callback, NULL), index, 0);
}

int skred_midi_input_open_virtual(const char *name) {
  if (name && name[0] && strcmp(name, midi_client_name) != 0) {
    if (midi_output_opened) return MM_ALREADY_OPEN;
    (void)skred_midi_input_close();
    int init_result = skred_midi_init(name);
    if (init_result != 0) return init_result;
  }
  if (!midi_initialized) {
    int init_result = skred_midi_init(name && name[0] ? name : "pulp");
    if (init_result != 0) return init_result;
  }
  (void)skred_midi_input_close();
  /* minimidio uses the context name for virtual endpoints. */
  return midi_input_finish_open(mm_in_open_virtual(&midi_context, &midi_input,
    midi_input_callback, NULL), -1, 1);
}

int skred_midi_output_close(void) {
  if (!midi_output_opened) return 0;
  mm_result result = mm_out_close(&midi_output);
  memset(&midi_output, 0, sizeof(midi_output));
  midi_output_opened = 0;
  midi_output_virtual = 0;
  midi_output_index = -1;
  return midi_result(result);
}

int skred_midi_output_open(int index) {
  if (!midi_initialized || index < 0) return MM_INVALID_ARG;
  (void)skred_midi_output_close();
  mm_result result = mm_out_open(&midi_context, &midi_output,
    (uint32_t)index);
  if (result != MM_SUCCESS) return (int)result;
  midi_output_opened = 1;
  midi_output_index = index;
  return 0;
}

int skred_midi_output_open_virtual(const char *name) {
  if (name && name[0] && strcmp(name, midi_client_name) != 0) {
    if (midi_input_opened) return MM_ALREADY_OPEN;
    (void)skred_midi_output_close();
    int init_result = skred_midi_init(name);
    if (init_result != 0) return init_result;
  }
  if (!midi_initialized) {
    int init_result = skred_midi_init(name && name[0] ? name : "pulp");
    if (init_result != 0) return init_result;
  }
  (void)skred_midi_output_close();
  mm_result result = mm_out_open_virtual(&midi_context, &midi_output);
  if (result != MM_SUCCESS) return (int)result;
  midi_output_opened = 1;
  midi_output_virtual = 1;
  return 0;
}

int skred_midi_input_running(void) { return midi_input_opened; }
int skred_midi_output_running(void) { return midi_output_opened; }

int skred_midi_send_raw(const uint8_t *data, int length) {
  if (!midi_output_opened) return MM_NOT_OPEN;
  if (!data || length <= 0) return MM_INVALID_ARG;
  return midi_result(mm_out_send_raw(&midi_output, data, (size_t)length));
}

char *skred_midi_status(void) {
  snprintf(midi_message, sizeof(midi_message),
    "midi: %s caps:0x%x in:%s%s%d out:%s%s%d mask:0x%08x routes:%d bindings:%d",
    midi_initialized ? "ready" : "stopped", skred_midi_caps(),
    midi_input_opened ? "open" : "closed",
    midi_input_virtual ? "/virtual:" : "/device:", midi_input_index,
    midi_output_opened ? "open" : "closed",
    midi_output_virtual ? "/virtual:" : "/device:", midi_output_index,
    skred_midi_event_mask(), skred_midi_route_count(),
    skred_midi_binding_count());
  return midi_message;
}

#else

int skred_midi_init(const char *client_name) { (void)client_name; return -3; }
void skred_midi_uninit(void) {}
uint32_t skred_midi_caps(void) { return 0; }
int skred_midi_input_count(void) { return -1; }
int skred_midi_output_count(void) { return -1; }
int skred_midi_input_name(int i, char *n, int z) { (void)i;(void)n;(void)z;return -3; }
int skred_midi_output_name(int i, char *n, int z) { (void)i;(void)n;(void)z;return -3; }
int skred_midi_input_open(int i) { (void)i; return -3; }
int skred_midi_input_open_virtual(const char *n) { (void)n; return -3; }
int skred_midi_input_close(void) { return 0; }
int skred_midi_output_open(int i) { (void)i; return -3; }
int skred_midi_output_open_virtual(const char *n) { (void)n; return -3; }
int skred_midi_output_close(void) { return 0; }
int skred_midi_input_running(void) { return 0; }
int skred_midi_output_running(void) { return 0; }
int skred_midi_send_raw(const uint8_t *d, int n) { (void)d;(void)n;return -3; }
char *skred_midi_status(void) {
  snprintf(midi_message, sizeof(midi_message),
    "midi: not built (use MIDI=1) routes:%d bindings:%d",
    skred_midi_route_count(), skred_midi_binding_count());
  return midi_message;
}

#endif

void skred_midi_event_mask_set(uint32_t mask) {
  atomic_store_int(&midi_mask, (int)mask);
}
uint32_t skred_midi_event_mask(void) {
  return (uint32_t)atomic_load_int(&midi_mask);
}

void skred_midi_test_inject(int type, int channel, int data1, int data2) {
  midi_publish(type, channel, data1, data2);
}

char *skred_midi_message(void) { return midi_message; }

static void midi_set_status_message(void) {
  char status[512];
  snprintf(status, sizeof(status), "%s", skred_midi_status());
  snprintf(midi_message, sizeof(midi_message), "# %s", status);
}

static void midi_append(size_t *used, const char *format, ...) {
  if (*used >= sizeof(midi_message)) return;
  va_list args;
  va_start(args, format);
  int count = vsnprintf(midi_message + *used, sizeof(midi_message) - *used,
    format, args);
  va_end(args);
  if (count > 0) *used += (size_t)count;
}

static int midi_parse_index(const char *text, int *index) {
  char *end = NULL;
  long value = strtol(text, &end, 10);
  if (!text[0] || end == text || *end || value < 0 || value > INT_MAX)
    return -1;
  *index = (int)value;
  return 0;
}

static int midi_parse_route(const char *text, int *channel, int *target,
    float *bend) {
  char input[SKRED_MIDI_NAME_MAX];
  char channel_text[16];
  char extra;
  snprintf(input, sizeof(input), "%s", text ? text : "");
  for (char *p = input; *p; p++) if (*p == ',') *p = ' ';
  float parsed_bend = 2.0f;
  int fields = sscanf(input, "%15s %d %f %c", channel_text, target,
    &parsed_bend, &extra);
  if (fields < 2 || fields > 3) return -1;
  if (!strcmp(channel_text, ".") || !strcmp(channel_text, "-")) *channel = -1;
  else if (midi_parse_index(channel_text, channel) != 0 || *channel > 15)
    return -1;
  if (bend) *bend = parsed_bend;
  return 0;
}

static int midi_parse_type(const char *text, int *type) {
  return midi_parse_index(text, type) == 0 && midi_message_type_valid(*type)
    ? 0 : -1;
}

static int midi_parse_selector(const char *text, int max, int *value) {
  if (!strcmp(text, ".") || !strcmp(text, "-")) {
    *value = -1;
    return 0;
  }
  return midi_parse_index(text, value) == 0 && *value <= max ? 0 : -1;
}

static int midi_parse_binding(const char *text, int require_command,
    int *type, int *channel, int *data1, const char **command) {
  static char copy[SKRED_MIDI_BINDING_COMMAND_MAX + 96];
  char *space;
  char *first;
  char *second;
  snprintf(copy, sizeof(copy), "%s", text ? text : "");
  space = strpbrk(copy, " \t");
  if (space) {
    *space++ = '\0';
    while (isspace((unsigned char)*space)) space++;
  }
  if (require_command && (!space || !space[0])) return -1;
  first = strchr(copy, ',');
  if (!first) return -1;
  *first++ = '\0';
  second = strchr(first, ',');
  if (!second) return -1;
  *second++ = '\0';
  if (strchr(second, ',') || midi_parse_type(copy, type) != 0 ||
      midi_parse_selector(first, 15, channel) != 0 ||
      midi_parse_selector(second, 16383, data1) != 0) return -1;
  if (command) *command = space;
  return 0;
}

int skred_midi_command(const char *line) {
  char command[8] = {0};
  char argument[SKRED_MIDI_BINDING_COMMAND_MAX + 96] = {0};
  const char *p = line;
  int fields;
  int result;
  midi_message[0] = '\0';
  if (!p) return 0;
  while (isspace((unsigned char)*p)) p++;
  fields = sscanf(p, "%7s %351[^\n]", command, argument);
  if (strncmp(command, "/m", 2) != 0) return 0;

  if (!strcmp(command, "/mL")) {
    if (fields != 1) goto usage;
    result = skred_midi_init("pulp");
    if (result != 0) goto failed;
    size_t used = 0;
    midi_append(&used, "# MIDI inputs\n");
    int count = skred_midi_input_count();
    for (int i = 0; i < count; i++) {
      char name[SKRED_MIDI_NAME_MAX] = {0};
      if (skred_midi_input_name(i, name, sizeof(name)) == 0)
        midi_append(&used, "#   %d %s\n", i, name);
    }
    midi_append(&used, "# MIDI outputs\n");
    count = skred_midi_output_count();
    for (int i = 0; i < count; i++) {
      char name[SKRED_MIDI_NAME_MAX] = {0};
      if (skred_midi_output_name(i, name, sizeof(name)) == 0)
        midi_append(&used, "#   %d %s\n", i, name);
    }
    return 1;
  }
  if (!strcmp(command, "/m?")) {
    if (fields != 1) goto usage;
    midi_set_status_message();
    return 1;
  }
  if (!strcmp(command, "/mR")) {
    if (fields != 1) goto usage;
    snprintf(midi_message, sizeof(midi_message), "%s",
      skred_midi_route_status());
    return 1;
  }
  if (!strcmp(command, "/mC")) {
    if (fields != 1) goto usage;
    skred_midi_route_clear();
    snprintf(midi_message, sizeof(midi_message), "# MIDI routes cleared");
    return 1;
  }
  if (!strcmp(command, "/mb?")) {
    if (fields != 1) goto usage;
    snprintf(midi_message, sizeof(midi_message), "%s",
      skred_midi_binding_status());
    return 1;
  }
  if (!strcmp(command, "/mbC")) {
    if (fields != 1) goto usage;
    skred_midi_binding_clear();
    snprintf(midi_message, sizeof(midi_message),
      "# MIDI Skode bindings cleared");
    return 1;
  }
  if (!strcmp(command, "/mb")) {
    int type, channel, data1;
    const char *skode;
    if (fields != 2 || midi_parse_binding(argument, 1, &type, &channel,
        &data1, &skode) != 0) goto usage;
    result = skred_midi_binding_set(type, channel, data1, skode);
    if (result != 0) goto failed;
    result = skred_control_dispatch_start();
    if (result != 0) goto failed;
    snprintf(midi_message, sizeof(midi_message),
      "# MIDI Skode binding installed");
    return 1;
  }
  if (!strcmp(command, "/mbd")) {
    int type, channel, data1;
    if (fields != 2 || midi_parse_binding(argument, 0, &type, &channel,
        &data1, NULL) != 0) goto usage;
    result = skred_midi_binding_remove(type, channel, data1);
    if (result < 0) goto failed;
    snprintf(midi_message, sizeof(midi_message),
      "# MIDI Skode bindings removed: %d", result);
    return 1;
  }
  if (!strcmp(command, "/mv") || !strcmp(command, "/mp")) {
    int channel, target;
    float bend;
    if (fields != 2 || midi_parse_route(argument, &channel, &target,
        &bend) != 0) goto usage;
    result = skred_midi_route_set(channel,
      command[2] == 'v' ? SKRED_MIDI_ROUTE_VOICE : SKRED_MIDI_ROUTE_POOL,
      target, bend);
    if (result != 0) goto failed;
    result = skred_control_dispatch_start();
    if (result != 0) goto failed;
    if (channel < 0)
      snprintf(midi_message, sizeof(midi_message),
      "# MIDI route /m%c .,%d,%g", command[2], target, bend);
    else
      snprintf(midi_message, sizeof(midi_message),
        "# MIDI route /m%c %d,%d,%g", command[2], channel, target, bend);
    return 1;
  }
  if (!strcmp(command, "/mvd") || !strcmp(command, "/mpd")) {
    int channel, target;
    if (fields != 2 || midi_parse_route(argument, &channel, &target,
        NULL) != 0) goto usage;
    result = skred_midi_route_remove(channel,
      command[2] == 'v' ? SKRED_MIDI_ROUTE_VOICE : SKRED_MIDI_ROUTE_POOL,
      target);
    if (result < 0) goto failed;
    snprintf(midi_message, sizeof(midi_message),
      "# MIDI routes removed: %d", result);
    return 1;
  }
  if (!strcmp(command, "/mic")) {
    if (fields != 1) goto usage;
    result = skred_midi_input_close();
  } else if (!strcmp(command, "/moc")) {
    if (fields != 1) goto usage;
    result = skred_midi_output_close();
  } else if (!strcmp(command, "/miV")) {
    if (fields > 2) goto usage;
    result = skred_midi_init(fields == 2 ? argument : "pulp");
    if (!result) result = skred_midi_input_open_virtual(argument);
  } else if (!strcmp(command, "/moV")) {
    if (fields > 2) goto usage;
    result = skred_midi_init(fields == 2 ? argument : "pulp");
    if (!result) result = skred_midi_output_open_virtual(argument);
  } else if (!strcmp(command, "/mi") || !strcmp(command, "/mo")) {
    int index;
    if (fields != 2 || midi_parse_index(argument, &index) != 0) goto usage;
    result = skred_midi_init("pulp");
    if (!result) result = command[2] == 'i' ? skred_midi_input_open(index) :
      skred_midi_output_open(index);
  } else {
    return 0;
  }
  if (result != 0) goto failed;
  midi_set_status_message();
  return 1;

usage:
  snprintf(midi_message, sizeof(midi_message),
    "# MIDI usage: /mL | /m? | /mi N | /miV [name] | /mic | "
    "/mo N | /moV [name] | /moc | /mv channel,voice[,bend] | "
    "/mp channel,pool[,bend] | /mvd channel,voice | "
    "/mpd channel,pool | /mR | /mC | "
    "/mb type,channel,data1 command | /mbd type,channel,data1 | /mb? | /mbC");
  return -1;
failed:
  snprintf(midi_message, sizeof(midi_message),
    "# MIDI command failed (%d): %s", result, command);
  return -1;
}
