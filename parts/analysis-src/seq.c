#include "synth-types.h"
#include "synth.h"
#include "seq.h"

#include "skqueue.h"

#include <stdio.h>
#include <string.h>

int requested_seq_frames_per_callback = SEQ_FRAMES_PER_CALLBACK;
int seq_frames_per_callback = 0;

char seq_pattern[PATTERNS_MAX][SEQ_STEPS_MAX][STEP_MAX];
event_program_t seq_program[PATTERNS_MAX][SEQ_STEPS_MAX];
int seq_pattern_length[PATTERNS_MAX];

int scope_pattern_pointer = 0;
int seq_pointer[PATTERNS_MAX];  // read-only derived value, kept for external inspection
int seq_state[PATTERNS_MAX];
int seq_modulo[PATTERNS_MAX];
int seq_mute[PATTERNS_MAX];

text_t seq_text[PATTERNS_MAX];

// Per-pattern offset applied to the master clock when deriving step position:
//   step = ((master_tick / modulo) - seq_offset[p]) % length
// Normally zero. seq_step_goto() writes this to force a specific step next tick.
static int64_t seq_offset[PATTERNS_MAX];

// Master clock tick — ever-incrementing, never reset unless seq_rewind() is called.
// All pattern positions are derived from this value so stop/start is always phase-coherent.
static uint64_t master_tick = 0;
static uint64_t seq_sample_origin = 0;
static double seq_tick_origin = 0.0;

float tempo_time_per_step = 60.0f;
float tempo_bpm = 120.0f / 4.0f;
float tempo_base = 0.0f;

void tempo_set(float m) {
  uint64_t now = SAMPLE_COUNT_GET();
  double samples_per_step = tempo_time_per_step * (double)MAIN_SAMPLE_RATE;
  double tick_origin = (double)master_tick;
  if (samples_per_step > 0.0 && now > seq_sample_origin) {
    tick_origin = seq_tick_origin +
      ((double)(now - 1 - seq_sample_origin) / samples_per_step);
    if (tick_origin < (double)master_tick) tick_origin = (double)master_tick;
  }
  seq_sample_origin = now;
  seq_tick_origin = tick_origin;
  tempo_base = m;
  tempo_bpm = m / 4.0;
  float bps = m / 60.f;
  float time_per_step = 1.0f / bps / 4.0f;
  tempo_time_per_step = time_per_step;
}

void seq_rewind(void) {
  seq_sample_origin = SAMPLE_COUNT_GET();
  seq_tick_origin = 0.0;
  master_tick = 0;
}

uint64_t seq_master_tick(void) {
  return master_tick;
}

char *seq_stats(void) {
  return "";
}

static int pattern_length_compute(int p) {
  for (int s = SEQ_STEPS_MAX - 1; s >= 0; s--) {
    if (seq_pattern[p][s][0] != '\0') return s + 1;
  }
  return 0;
}

static queue_t seq_q;

void do_event(uint64_t now, void (*event_fn)(const event_t *event)) {
  // Run expired (ready) queued events.
  item_t item;
  static uint64_t last_ts = 0;
  while (1) {
    if (queue_get_filtered(&seq_q, now, &item)) {
      if (item.timestamp < last_ts) {
        printf("OUT OF ORDER\n");
      }
      last_ts = item.timestamp;
      event_fn(&item.event);
    } else {
      break;
    }
  }
}

void do_pattern(uint64_t now,
    void (*program_fn)(int pattern, const event_program_t *program)) {
  // Advance the master clock from the absolute sample timeline rather than
  // callback cadence so larger buffers do not smear beat timing.
  double samples_per_step = tempo_time_per_step * (double)MAIN_SAMPLE_RATE;
  if (samples_per_step > 0.0 && now > seq_sample_origin) {
    uint64_t target_tick = (uint64_t)(seq_tick_origin +
      ((double)(now - 1 - seq_sample_origin) / samples_per_step));
    while (master_tick < target_tick) {
      master_tick++;
      for (int p = 0; p < PATTERNS_MAX; p++) {
        if (seq_state[p] != SEQ_RUNNING) continue;
        if ((master_tick % (uint64_t)seq_modulo[p]) != 0) continue;

        int len = seq_pattern_length[p];
        if (len == 0) continue;

        int64_t ticks_so_far = (int64_t)(master_tick / (uint64_t)seq_modulo[p]);
        int64_t raw = ticks_so_far - seq_offset[p];
        int step = (int)(((raw % (int64_t)len) + (int64_t)len) % (int64_t)len);

        seq_pointer[p] = step;

        if (seq_pattern[p][step][0] == '-') {
          seq_state[p] = SEQ_STOPPED;
          seq_pointer[p] = 0;
        } else {
          if (seq_mute[p] == 0) program_fn(p, &seq_program[p][step]);
        }
      }
    }
  }

}

void seq(uint64_t now, void (*event_fn)(const event_t *event),
    void (*program_fn)(int pattern, const event_program_t *program)) {

  do_event(now, event_fn);
  do_pattern(now, program_fn);

}

void pattern_reset(int p) {
  seq_pointer[p] = 0;
  seq_state[p] = SEQ_STOPPED;
  seq_modulo[p] = 4;
  seq_mute[p] = 0;
  seq_pattern_length[p] = 0;
  seq_offset[p] = 0;
  for (int s = 0; s < SEQ_STEPS_MAX; s++) {
    seq_pattern[p][s][0] = '\0';
    seq_program[p][s].count = 0;
  }
  seq_text[p][0] = '\0';
}

void seq_init(void) {
  queue_init(&seq_q, QUEUE_SIZE);
  seq_sample_origin = SAMPLE_COUNT_GET();
  seq_tick_origin = 0.0;
  master_tick = 0;
  for (int p = 0; p < PATTERNS_MAX; p++) {
    pattern_reset(p);
  }
}

int queue_event(uint64_t when, const event_t *event, int tag) {
  return queue_put_event(&seq_q, when, tag, NULL, event) ? 0 : -1;
}

void seq_modulo_set(int pattern, int m) {
  if (pattern < 0 || pattern >= PATTERNS_MAX) return;
  if (m < 1) m = 1;
  seq_modulo[pattern] = m;
}

int seq_step_set(int pattern, int step, const char *source,
    const event_program_t *program) {
  if (pattern < 0 || pattern >= PATTERNS_MAX) return -1;
  if (step < 0 || step >= SEQ_STEPS_MAX) return -1;
  if (source && strnlen(source, STEP_MAX) >= STEP_MAX) return -1;
  if (source == NULL || source[0] == '\0') {
    seq_pattern[pattern][step][0] = '\0';
    seq_program[pattern][step].count = 0;
  } else if (strcmp(source, "-") == 0) {
    snprintf(seq_pattern[pattern][step], STEP_MAX, "%s", source);
    seq_program[pattern][step].count = 0;
  } else if (!program || program->count > SEQ_PROGRAM_OP_MAX) {
    return -1;
  } else {
    snprintf(seq_pattern[pattern][step], STEP_MAX, "%s", source);
    seq_program[pattern][step] = *program;
  }
  seq_pattern_length[pattern] = pattern_length_compute(pattern);
  return 0;
}

char *seq_step_get(int pattern, int step) {
  if (pattern < 0 || pattern >= PATTERNS_MAX) return "";
  if (step < 0 || step >= SEQ_STEPS_MAX) return "";
  return seq_pattern[pattern][step];
}

int seq_step_append(int pattern, const char *source,
    const event_program_t *program) {
  return seq_step_set(pattern, seq_pattern_length[pattern], source, program);
}

void seq_pattern_length_set(int pattern, int len) {
  if (pattern < 0 || pattern >= PATTERNS_MAX) return;
  if (len < 0) len = 0;
  if (len > SEQ_STEPS_MAX) len = SEQ_STEPS_MAX;
  seq_pattern_length[pattern] = len;
}

// Jump to a specific step on the next tick. Adjusts seq_offset so that
// (ticks_so_far + 1 - offset) % len == step.
void seq_step_goto(int pattern, int step) {
  if (pattern < 0 || pattern >= PATTERNS_MAX) return;
  int len = seq_pattern_length[pattern];
  if (len == 0) return;
  if (step < 0 || step >= len) return;
  int64_t ticks_so_far = (int64_t)(master_tick / (uint64_t)seq_modulo[pattern]);
  seq_offset[pattern] = (ticks_so_far + 1) - (int64_t)step;
}

void seq_state_set(int p, int state) {
  if (p < 0 || p >= PATTERNS_MAX) return;
  switch (state) {
    case 0: // stop
      seq_state[p] = SEQ_STOPPED;
      break;
    case 1: // start
      seq_state[p] = SEQ_RUNNING;
      break;
    case 2: // pause
      seq_state[p] = SEQ_PAUSED;
      break;
    case 3: // resume
      seq_state[p] = SEQ_RUNNING;
      break;
  }
}

void seq_state_all(int state) {
  for (int p = 0; p < PATTERNS_MAX; p++) seq_state_set(p, state);
}

int seq_queued(void) { return queue_size(&seq_q); }
int seq_capacity(void) { return seq_q.max_size; }

typedef struct {
  int (*fn)(int, uint64_t, uint64_t, int, const event_t *e, void*);
  void *user;
  int count;
} bridge_t;

int seq_foreach_cb(const item_t *item, void *user) {
  bridge_t *b = (bridge_t *)user;
  b->fn(b->count++, item->timestamp, item->id, item->tag,
    &item->event, b->user);
  return 0;
}

int seq_foreach(int (*fn)(int, uint64_t, uint64_t, int, const event_t *e, void*), void *user) {
  bridge_t b;
  b.fn = fn;
  b.user = user;
  b.count = 0;
  queue_foreach(&seq_q, seq_foreach_cb, &b);
  return 0;
}

bool kill_by_tag(const item_t *item, void *user) {
  int *tag = (int *)user;
  if (item->tag == *tag) return true;
  return false;
}

int seq_kill_by_tag(int tag) {
  queue_cancel(&seq_q, kill_by_tag, &tag);
  return 0;
}

int seq_kill_all(void) {
  queue_clear(&seq_q);
  return 0;
}
