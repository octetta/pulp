#if defined(__linux__) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "api.h"
#include "midi.h"
#include "portable_atomic.h"

#include <ctype.h>
#include <limits.h>
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
    "midi: %s caps:0x%x in:%s%s%d out:%s%s%d mask:0x%08x",
    midi_initialized ? "ready" : "stopped", skred_midi_caps(),
    midi_input_opened ? "open" : "closed",
    midi_input_virtual ? "/virtual:" : "/device:", midi_input_index,
    midi_output_opened ? "open" : "closed",
    midi_output_virtual ? "/virtual:" : "/device:", midi_output_index,
    skred_midi_event_mask());
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
  snprintf(midi_message, sizeof(midi_message), "midi: not built (use MIDI=1)");
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

int skred_midi_command(const char *line) {
  char command[8] = {0};
  char argument[SKRED_MIDI_NAME_MAX] = {0};
  const char *p = line;
  int fields;
  int result;
  midi_message[0] = '\0';
  if (!p) return 0;
  while (isspace((unsigned char)*p)) p++;
  fields = sscanf(p, "%7s %127[^\n]", command, argument);
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
  if (!strcmp(command, "/mi-")) {
    if (fields != 1) goto usage;
    result = skred_midi_input_close();
  } else if (!strcmp(command, "/mo-")) {
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
    "# MIDI usage: /mL | /m? | /mi N | /miV [name] | /mi- | "
    "/mo N | /moV [name] | /mo-");
  return -1;
failed:
  snprintf(midi_message, sizeof(midi_message),
    "# MIDI command failed (%d): %s", result, command);
  return -1;
}
