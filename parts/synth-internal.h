#ifndef _SYNTH_INTERNAL_H_
#define _SYNTH_INTERNAL_H_

#include "synth.h"
#include "synth-state.h"
#include "util.h"
#include <math.h>

#define DELAY_MAX_FRAMES (65536)
#define DELAY_BUS_COUNT (4)
#define DELAY_TIME_SMOOTHING (0.0008f)   // one-pole coeff, avoids zipper clicks on time changes
#define FM_TWO_PI (2.0f * (float)M_PI)

typedef struct {
  float buffer[DELAY_MAX_FRAMES];
  float buffer_r[DELAY_MAX_FRAMES];   // only written/read when pingpong == 1
  int write;
  int write_r;
  int active;
  int idle_frames;
  float lfo_phase;
  int coarse;
  int fine;
  int feedback;
  int mod_freq;
  int mod_depth;
  int level;
  int damping;      // 0-15, feedback lowpass amount
  int hp;            // 0-15, feedback highpass amount
  int freeze;         // 0/1, infinite repeat toggle
  int pingpong;         // 0/1, cross-channel feedback
  int bits;              // 0 = default (12-bit), else 1-16 explicit depth
  int native;             // 0 = quantize (default), 1 = bypass entirely
  float base_frames;
  float base_frames_target;   // delay_process ramps base_frames toward this
  float feedback_gain;
  float lfo_phase_inc;
  float mod_depth_frames;
  float stereo_offset_frames;
  float wet;
  float damping_coeff;
  float hp_coeff;
  float lp_state[2];    // [0]=mono/left, [1]=right (pingpong only)
  float hp_state[2];
  float hp_prev[2];
  float quant_levels;    // cached: (2^(effective_bits-1))-1, only used when !native
} delay_bus_t;

extern delay_bus_t delay_bus[DELAY_BUS_COUNT];

void audio_rng_init(uint64_t *rng, uint64_t seed);
uint64_t audio_rng_next(uint64_t *rng);
float audio_rng_float(uint64_t *rng);
float audio_rng_raw_float(uint64_t raw);

float osc_next_at(skred_engine_t *engine, int voice, float phase_inc, float phase_offset, uint64_t current_sample);
void envelope_release_e_at(envelope_t *e, uint64_t current_sample);
float quantize_bits_curve(float v, int bits, int curve, uint64_t *rng);
float mmf_process(skred_engine_t *engine, int n, float input);
float amp_envelope_step(int v, uint64_t current_sample);
int delay_voice_can_send(skred_engine_t *engine, int voice);
void delay_process(skred_engine_t *engine, delay_bus_t *bus, float input, float *left, float *right);

int voice_invalid(int voice);
int wave_invalid(int wave);
void delay_cache_params(skred_engine_t *engine, delay_bus_t *bus);

float clampf(float value, float min_value, float max_value);
int osc_loop_crossings(double distance, double loop_length);
int clampi(int value, int min_value, int max_value);

#endif
#define VOLUME_DEFAULT (-20.0f)
#define DB_TO_LINEAR(v) powf(10.f, (v) / 20.0f)
#define SYNTH_INVALID_VOICE (100)
#define DELAY_MAX_FRAMES (65536)
#define DELAY_BUS_COUNT (4)
#define DELAY_TIME_SMOOTHING (0.0008f)
#define FM_TWO_PI (2.0f * (float)M_PI)
#define SMOOTH_DEFAULT (0.02f)
#define FILTER_UC (16)
int mod_voice_invalid(int voice);
void amp_envelope_schedule_one_shot_release(int v);
