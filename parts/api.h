#ifndef SKRED_API_H
#define SKRED_API_H

#include <stdint.h>
#include "skred-version.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the audio engine, state, and networking
int skred_start(unsigned int req_audio_frames, unsigned int voices, int port);

// Send an ASCII control protocol message to the engine
int skred_command(char* cmd);

// Safely tear down resources
void skred_stop(void);

// RECORD feature: writes master plus four stereo stems to a 10-channel WAV.
int skred_record_start(const char *filename, double max_seconds);
int skred_record_stop(void);
int skred_record_state(void);
uint64_t skred_record_frames_written(void);
uint64_t skred_record_dropped_frames(void);

// SCOPE feature: publishes the 10-channel render bus through shared memory.
int skred_scope_start(const char *name, uint32_t channel_mask,
                      double buffer_seconds);
int skred_scope_stop(void);

// list of included features
char *skred_features(void);

// Version from the repository VERSION file used at build time.
const char *skred_version(void);
int skred_version_major(void);
int skred_version_minor(void);
int skred_version_patch(void);

// did skode have anything to say?
char *skred_log(void);

// enable / disable logging
void skred_logger(int f);

// Audio device management. Selection values are list slots from the latest
// refresh; -1 selects the default device and -2 disables capture.
int skred_audio_refresh(void);
int skred_audio_select(int is_capture, int selection);
int skred_audio_reconnect(void);
int skred_audio_disconnect(void);
int skred_audio_running(void);
char *skred_audio_status(void);
int skred_audio_command(const char *line);
char *skred_audio_message(void);

typedef struct skred_performance_metrics {
  uint64_t callbacks;
  uint64_t frames;
  uint64_t sample_count;
  uint64_t callback_ns_total;
  uint64_t callback_ns_last;
  uint64_t callback_ns_worst;
  uint64_t callback_budget_ns_total;
  uint64_t callback_budget_ns_last;
  uint64_t callback_budget_ns_worst;
  uint64_t callback_overruns;
  uint32_t callback_frames_last;
  uint32_t callback_frames_worst;
} skred_performance_metrics_t;

int skred_performance_metrics(skred_performance_metrics_t *out);
void skred_performance_reset(void);
char *skred_performance_status(void);

typedef enum {
  SKRED_CONTROL_EVENT_NONE = 0,
  SKRED_CONTROL_EVENT_VOICE_TRIGGER = 1,
  SKRED_CONTROL_EVENT_VOICE_RELEASE = 2,
  SKRED_CONTROL_EVENT_VOICE_FINISHED = 3,
  SKRED_CONTROL_EVENT_USER = 4,
  SKRED_CONTROL_EVENT_PATTERN_START = 5,
  SKRED_CONTROL_EVENT_PATTERN_END = 6,
} skred_control_event_type_t;

/*
 * Control-plane notifications are consumed by polling; SKRED does not call
 * host callbacks from its audio thread. Enable voice lifecycle events with
 * Skode "vc1", pattern boundary events with "yc1", and emit host-defined
 * user events with schedulable "ce id[,a,b,c]".
 */
typedef struct skred_control_event {
  uint32_t type;
  uint32_t opcode;
  uint64_t sample;
  uint64_t sequence;
  int voice;
  int pattern;
  int step;
  int tag;
  int id;
  uint32_t value_count;
  double value[3];
} skred_control_event_t;

/* Copies and consumes up to max_events ready notifications. Nonblocking. */
int skred_control_event_poll(skred_control_event_t *events, int max_events);
/* Clears queued notifications, sequence numbering, and dropped count. */
void skred_control_event_reset(void);
/* Cumulative count of notifications dropped because the ring was full. */
uint64_t skred_control_event_dropped(void);

typedef struct skred_scheduled_event {
  int index;
  uint64_t timestamp;
  uint64_t id;
  int tag;
  int voice;
  uint8_t voice_var;
  uint8_t source_valid;
  int pattern;
  int step;
  uint8_t opcode;
  uint8_t opcode_argc;
  char opcode_mode;
  uint8_t opcode_var_mask;
  float opcode_arg[4];
} skred_scheduled_event_t;

int skred_scheduled_event_count(void);
int skred_scheduled_event_snapshot(skred_scheduled_event_t *events,
                                   int max_events);

// Compatibility enumeration API.
int skred_devices(int isCapture);
int skred_device_idx(int isCapture, int idx);
char *skred_device_str(int isCapture, int idx);
int skred_enumerate_devices(int isCapture);
void skred_set_audio_device(int playback_idx, int capture_idx);

#ifdef __cplusplus
}
#endif

#endif // SKRED_API_H
