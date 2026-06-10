#ifndef _SEQ_H_
#define _SEQ_H_

#include <stdint.h>

#define SEQ_FRAMES_PER_CALLBACK (128)

#define PATTERNS_MAX (128)
#define SEQ_STEPS_MAX (256)
#define STEP_MAX (256)

enum {
  SEQ_STOPPED = 0,
  SEQ_RUNNING = 1,
  SEQ_PAUSED = 2,
};

#define QUEUE_SIZE (2048) // the actual queue max depth

#define SEQ_OPCODE_ARG_MAX (4)
#define SEQ_PROGRAM_OP_MAX (16)

typedef struct {
  uint8_t code;
  uint8_t argc;
  char mode;
  uint8_t var_mask;
  float arg[SEQ_OPCODE_ARG_MAX];
} opcode_event_t;

typedef struct {
  int voice;
  uint8_t voice_var;
  opcode_event_t opcode;
} event_t;

typedef struct {
  opcode_event_t opcode;
} program_op_t;

typedef struct {
  uint8_t count;
  program_op_t op[SEQ_PROGRAM_OP_MAX];
} event_program_t;

void seq(uint64_t now, void (*event_fn)(const event_t *event),
  void (*program_fn)(int pattern, const event_program_t *program));
void seq_init(void);
void seq_rewind(void);
uint64_t seq_master_tick(void);
void pattern_reset(int p);
int queue_event(uint64_t when, const event_t *event, int tag);
void tempo_set(float m);

void seq_modulo_set(int pattern, int m);
int seq_step_set(int pattern, int step, const char *source,
  const event_program_t *program);
char *seq_step_get(int pattern, int step);
int seq_step_append(int pattern, const char *source,
  const event_program_t *program);
void seq_pattern_length_set(int pattern, int len);
void seq_step_goto(int pattern, int step);
void seq_state_set(int p, int state);
void seq_state_all(int state);

extern int requested_seq_frames_per_callback;
extern int seq_frames_per_callback;

extern int seq_pointer[PATTERNS_MAX];
extern int seq_modulo[PATTERNS_MAX];
extern int seq_counter[PATTERNS_MAX];
extern int seq_state[PATTERNS_MAX];
extern int seq_mute[PATTERNS_MAX];
extern int seq_pattern_length[PATTERNS_MAX];
extern char seq_pattern[PATTERNS_MAX][SEQ_STEPS_MAX][STEP_MAX];
extern event_program_t seq_program[PATTERNS_MAX][SEQ_STEPS_MAX];

#define TEXT_MAX (32+1)
typedef char text_t[TEXT_MAX];
extern text_t seq_text[PATTERNS_MAX];

extern float tempo_bpm;
extern float tempo_time_per_step;

int seq_queued(void);
int seq_capacity(void);

int seq_foreach(int (*fn)(int, uint64_t, uint64_t, int, const event_t *e, void*), void *user);

int seq_kill_by_tag(int tag);

char *seq_stats(void);

int seq_kill_all(void);

#endif
