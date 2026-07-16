#ifndef _SKODE_H_
#define _SKODE_H_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <dirent.h>
#include <sys/types.h>

#define VOICE_STACK_LEN (8)

typedef struct {
  float s[VOICE_STACK_LEN];
  int ptr;
} voice_stack_t;

#include "ands.h"
#include "seq.h"

typedef enum {
  SKODE_OP_NONE = 0,
  SKODE_OP_DELAY,
  SKODE_OP_VOICE,
  SKODE_OP_AMP,
  SKODE_OP_FREQ,
  SKODE_OP_MIDI_NOTE,
  SKODE_OP_PAN,
  SKODE_OP_VELOCITY,
  SKODE_OP_ENVELOPE_VELOCITY,
  SKODE_OP_AMP_MOD,
  SKODE_OP_WAVE_DIRECTION,
  SKODE_OP_WAVE_LOOP,
  SKODE_OP_PHASE_DISTORTION,
  SKODE_OP_PHASE_MOD,
  SKODE_OP_FILTER_ENVELOPE,
  SKODE_OP_FILTER_ENVELOPE_DEPTH,
  SKODE_OP_FREQ_MOD,
  SKODE_OP_FREQ_MOD_MODE,
  SKODE_OP_GLISSANDO,
  SKODE_OP_LINK_MIDI,
  SKODE_OP_SAMPLE_HOLD,
  SKODE_OP_LINK_VELOCITY,
  SKODE_OP_TRIGGER_DELAY,
  SKODE_OP_FILTER_MODE,
  SKODE_OP_FILTER_FREQ,
  SKODE_OP_ENVELOPE_MODE,
  SKODE_OP_MUTE,
  SKODE_OP_MIDI_DETUNE,
  SKODE_OP_PAN_MOD,
  SKODE_OP_QUANTIZE,
  SKODE_OP_FILTER_RESONANCE,
  SKODE_OP_RECORD_TRACK,
  SKODE_OP_SMOOTHER,
  SKODE_OP_VOICE_RESET,
  SKODE_OP_ENVELOPE,
  SKODE_OP_TRIGGER,
  SKODE_OP_WAVE,
  SKODE_OP_VOICE_COPY,
  SKODE_OP_WAVE_DEFAULT,
  SKODE_OP_VARIABLE_SET,
  SKODE_OP_RING_MOD,
  SKODE_OP_WAVE_LOOP_COUNT,
  SKODE_OP_CONTROL_EVENT,
  SKODE_OP_DELAY_PARAMS,
  SKODE_OP_WAVE_RANGE_SET,
  SKODE_OP_WAVE_LOOP_SET,
  SKODE_OP_POLY_NOTE,
  SKODE_OP_POLY_RELEASE,
  SKODE_OP_POLY_BEND,
  SKODE_OP_PHASE_ENVELOPE,
  SKODE_OP_PHASE_ENVELOPE_DEPTH,
} skode_opcode_t;

typedef enum {
  SKODE_COMPILE_OK = 0,
  SKODE_COMPILE_IMMEDIATE_ONLY,
  SKODE_COMPILE_INVALID,
  SKODE_COMPILE_TOO_LARGE,
} skode_compile_result_t;

#define SKODE_LOG_MAX (16384)
#define SKODE_LOG_LINES (128)
#define SKODE_LOG_LINE_MAX (1024)
#define SKODE_EXTRA_MAX (128)
#define SKODE_COMPILE_DEPTH_MAX (16)
#define SKODE_STRING_SLOT_MAX (16)
#define SKODE_STRING_SLOT_LEN (256)

#define SPECTRO_LOG_LINE_BUDGET SKODE_LOG_LINE_MAX - 32

typedef struct skode_s {
  int voice;
  voice_stack_t stack;
  uint64_t defer_sample_time;
  double defer_last;
  int pattern;
  int step;
  int trace;
  int verbose;
  ands_t *parse;
  struct skode_vocab *vocab;
  int quit;
  int (*puts)(struct skode_s *w, const char *s);
  int (*printf)(struct skode_s *w, const char *fmt, ...);
  int log_enable;
  char log[SKODE_LOG_MAX + 1024];
  int log_max;
  int log_len;
  char log_ring[SKODE_LOG_LINES][SKODE_LOG_LINE_MAX];
  int log_head;
  int log_count;
  int log_dropped;
  char log_pending[SKODE_LOG_LINE_MAX];
  int log_pending_len;
  char string_slot[SKODE_STRING_SLOT_MAX][SKODE_STRING_SLOT_LEN];
  int flag;
  struct ks_ctx *ks;
  void *ks_result;
  //
  int udp;
  int which;
  uint32_t ip;
  uint16_t port;
} skode_t;

int skode_consume(char *line, skode_t *w);
int skode_execute_event(const event_t *event, skode_t *ctx);
int skode_execute_voice_opcode(const opcode_event_t *opcode, int voice);
int skode_emit_control_event_opcode(const opcode_event_t *opcode, int voice,
  int pattern, int step, int tag);
int skode_opcode_supported(skode_opcode_t opcode);
const char *skode_opcode_name(uint8_t opcode);
int skode_execute_program(const event_program_t *program, int voice,
  uint64_t now, int tag);
int skode_execute_program_state(const event_program_t *program, int *voice,
  uint64_t now, int tag, int pattern, int step);
skode_compile_result_t skode_compile_program(const char *text,
  event_program_t *program);
struct skode_vocab;
skode_compile_result_t skode_compile_program_ex(const char *text,
  event_program_t *program, struct skode_vocab *vocab);
int skode_extra_copy(int index, char *dst, size_t dst_size);
int skode_queue_program(const event_program_t *program, int voice,
  uint64_t when, int tag);
int skode_queue_program_deferred(const event_program_t *program, int voice,
  uint64_t base, char mode, double delay, int tag);
int skode_midi_note(int voice, float note, float cents);
void skode_envelope_velocity(int voice, float x, uint64_t now);
extern double global_var[ANDS_VAR_MAX];
void show_threads(skode_t *w);
void system_show(skode_t *w);
int audio_show(skode_t *w);
int skode_load(skode_t *w, int voice, int n, int verbose);
int skode_load_name(skode_t *w, const char *name, int verbose);
int wavetable_show(skode_t *w, int n);
char *skode_err_str(int n);

int skode_puts(skode_t *, const char *s);
int skode_printf(skode_t *, const char *fmt, ...);
void skode_log_message(skode_t *ctx, const char *message);

int null_puts(const char *s);
int null_printf(const char *fmt, ...);

#define SKODE_EMPTY() { \
  .voice = 0, \
  .pattern = 0, \
  .step = -1, \
  .trace = 0, \
  .verbose = 0, \
  .parse = NULL, \
  .quit = 0, \
  .puts = skode_puts, \
  .printf = skode_printf, \
  .log_enable = 0, \
  .log_len = 0, \
  .log_max = SKODE_LOG_MAX, \
  .ks = NULL, \
  .ks_result = NULL, \
}

//  .output = 0, 


/* Required by skode-dict.c -- see parts/docs/SKODE_DICT_INTEGRATION.md */
int skode_double_to_int(double value, int *out);
void sk_sleep(int milliseconds);

void skode_init(skode_t *w);
void skode_free(skode_t *w);

#endif
