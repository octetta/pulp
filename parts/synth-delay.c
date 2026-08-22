#include "synth.h"
#include "synth-internal.h"

delay_bus_t delay_bus[DELAY_BUS_COUNT];
#include "util.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "control-events.h"

static float delay_quantize(const delay_bus_t *bus, float x) {
  if (bus->native) {
    // Full float precision, no bit-depth reduction. Still safety-clamped
    // well above unity so a runaway feedback setting can't produce inf/nan.
    return clampf(x, -4.0f, 4.0f);
  }
  x = clampf(x, -1.0f, 1.0f);
  return roundf(x * bus->quant_levels) * (1.0f / bus->quant_levels);
}

static int delay_bus_index(int bus) {
  if (bus < 1 || bus > DELAY_BUS_COUNT) return -1;
  return bus - 1;
}

static float delay_base_ms(const delay_bus_t *bus) {
  int coarse = bus->coarse;
  int fine = bus->fine;
  float base_ms;
  float fine_factor;

  if (coarse < 0) coarse = 0;
  if (coarse > 7) coarse = 7;
  base_ms = 8.0f * (float)(1 << (coarse > 7 ? 7 : coarse));
  fine_factor = 0.5f + ((float)fine / 15.0f) * 0.5f;
  return base_ms * fine_factor;
}

static float delay_feedback_gain(const delay_bus_t *bus) {
  return ((float)bus->feedback / 15.0f) * 0.82f;
}

static float delay_lfo_hz(const delay_bus_t *bus) {
  return ((float)bus->mod_freq / 31.0f) * 10.0f;
}

static float delay_mod_depth_frames(const delay_bus_t *bus) {
  float depth = (float)bus->mod_depth / 31.0f;
  float base_frames = delay_base_ms(bus) * (float)MAIN_SAMPLE_RATE * 0.001f;
  return base_frames * depth * 0.25f;
}

static float delay_stereo_offset_frames(const delay_bus_t *bus) {
  float depth = (float)bus->mod_depth / 31.0f;
  return (2.0f + depth * 8.0f) * (float)MAIN_SAMPLE_RATE * 0.001f;
}

static float delay_hp_coeff(const delay_bus_t *bus);
static float delay_damping_coeff(const delay_bus_t *bus);
void delay_cache_params(skred_engine_t *engine, delay_bus_t *bus) {
  bus->base_frames_target = delay_base_ms(bus) * (float)MAIN_SAMPLE_RATE * 0.001f;
  bus->feedback_gain = delay_feedback_gain(bus);
  bus->lfo_phase_inc = 2.0f * (float)M_PI * delay_lfo_hz(bus) /
                       (float)MAIN_SAMPLE_RATE;
  bus->mod_depth_frames = delay_mod_depth_frames(bus);
  bus->stereo_offset_frames = delay_stereo_offset_frames(bus);
  bus->wet = (float)bus->level / 15.0f;
  bus->damping_coeff = delay_damping_coeff(bus);
  bus->hp_coeff = delay_hp_coeff(bus);
  {
    int effective_bits = bus->bits > 0 ? bus->bits : 12;
    if (effective_bits > 16) effective_bits = 16;
    bus->quant_levels = (float)((1 << (effective_bits - 1)) - 1);
    if (bus->quant_levels < 1.0f) bus->quant_levels = 1.0f;
  }
}

static float delay_read_buf(const float *buf, int write, float delay_frames) {
  if (delay_frames < 1.0f) delay_frames = 1.0f;
  if (delay_frames > (float)(DELAY_MAX_FRAMES - 2))
    delay_frames = (float)(DELAY_MAX_FRAMES - 2);

  float read = (float)write - delay_frames;
  while (read < 0.0f) read += (float)DELAY_MAX_FRAMES;
  int i0 = (int)read;
  int i1 = i0 + 1;
  if (i1 >= DELAY_MAX_FRAMES) i1 = 0;
  float frac = read - (float)i0;
  return buf[i0] + frac * (buf[i1] - buf[i0]);
}

static float delay_damping_coeff(const delay_bus_t *bus) {
  float amount = (float)bus->damping / 15.0f;             // 0..1
  float fc = 20000.0f * powf(500.0f / 20000.0f, amount);   // 20kHz (no damping) -> 500Hz (dark)
  float x = expf(-2.0f * (float)M_PI * fc / (float)MAIN_SAMPLE_RATE);
  return 1.0f - x;
}

static float delay_hp_coeff(const delay_bus_t *bus) {
  float amount = (float)bus->hp / 15.0f;                   // 0..1
  float fc = 20.0f * powf(2000.0f / 20.0f, amount);          // 20Hz (off) -> 2kHz (thin)
  float rc = 1.0f / (2.0f * (float)M_PI * fc);
  float dt = 1.0f / (float)MAIN_SAMPLE_RATE;
  return rc / (rc + dt);
}

// One-pole lowpass followed by one-pole highpass on the feedback path.
// damping = tone/darkening, hp = removes low-end buildup on long feedback chains.
static float delay_filter_feedback(skred_engine_t *engine, delay_bus_t *bus, int ch, float x) {
  bus->lp_state[ch] += bus->damping_coeff * (x - bus->lp_state[ch]);
  float y = bus->hp_coeff * (bus->hp_state[ch] + bus->lp_state[ch] - bus->hp_prev[ch]);
  bus->hp_prev[ch] = bus->lp_state[ch];
  bus->hp_state[ch] = y;
  return y;
}

void delay_process(skred_engine_t *engine, delay_bus_t *bus, float input, float *left, float *right) {
  float lfo = 0.0f;
  float mod_frames;
  float delayed_left;
  float delayed_right;
  float stereo_offset = 0.0f;

  bus->base_frames += DELAY_TIME_SMOOTHING * (bus->base_frames_target - bus->base_frames);

  if (bus->mod_depth > 0) {
    lfo = sinf(bus->lfo_phase);
    if (bus->mod_freq > 0) {
      bus->lfo_phase += bus->lfo_phase_inc;
      if (bus->lfo_phase >= 2.0f * (float)M_PI)
        bus->lfo_phase -= 2.0f * (float)M_PI;
    }
    stereo_offset = bus->stereo_offset_frames;
  }

  mod_frames = lfo * bus->mod_depth_frames;
  delayed_left = delay_read_buf(bus->buffer, bus->write, bus->base_frames + mod_frames);
  delayed_right = bus->pingpong
      ? delay_read_buf(bus->buffer_r, bus->write_r, bus->base_frames - mod_frames + stereo_offset)
      : delay_read_buf(bus->buffer, bus->write, bus->base_frames - mod_frames + stereo_offset);

  float fb_left  = bus->pingpong ? delayed_right : (delayed_left + delayed_right) * 0.5f;
  float fb_right = bus->pingpong ? delayed_left  : fb_left;
  fb_left = delay_filter_feedback(engine, bus, 0, fb_left);
  if (bus->pingpong) fb_right = delay_filter_feedback(engine, bus, 1, fb_right);

  float fb_gain = bus->freeze ? 1.0f : bus->feedback_gain;
  float feed = bus->freeze ? 0.0f : input;
  float write_left = delay_quantize(bus, feed + fb_left * fb_gain);
  bus->buffer[bus->write] = write_left;
  bus->write++;
  if (bus->write >= DELAY_MAX_FRAMES) bus->write = 0;

  if (bus->pingpong) {
    float write_right = delay_quantize(bus, feed + fb_right * fb_gain);
    bus->buffer_r[bus->write_r] = write_right;
    bus->write_r++;
    if (bus->write_r >= DELAY_MAX_FRAMES) bus->write_r = 0;
  }

  if (left) *left = delayed_left * bus->wet;
  if (right) *right = delayed_right * bus->wet;

  if (fabsf(input) + fabsf(delayed_left) + fabsf(delayed_right) +
      fabsf(write_left) > 0.0000001f) {
    bus->active = 1;
    bus->idle_frames = 0;
  } else if (bus->idle_frames >= DELAY_MAX_FRAMES) {
    bus->active = 0;
  } else {
    bus->idle_frames++;
  }
}

int delay_voice_can_send(skred_engine_t *engine, int voice) {
  if (voice_invalid(voice)) return 0;
  if (fabsf(sv.pan[voice]) > 0.0001f) return 0;
  if (sv.pan_mod_osc[voice] >= 0) return 0;
  return 1;
}

int delay_send_set(skred_engine_t *engine, int voice, float amount) {
  if (voice_invalid(voice) || !isfinite(amount))
    return SYNTH_INVALID_VOICE;
  if (amount > 1.0f) amount /= 15.0f;
  sv.delay_send[voice] = clampf(amount, 0.0f, 1.0f);
  return 0;
}

int delay_params_set(skred_engine_t *engine, int bus_number, int coarse, int fine, int feedback, int mod_freq,
                     int mod_depth, int level) {
  int index = delay_bus_index(bus_number);
  if (index < 0) return SYNTH_INVALID_VOICE;
  delay_bus_t *bus = &delay_bus[index];
  bus->coarse = clampi(coarse, 0, 7);
  bus->fine = clampi(fine, 0, 15);
  bus->feedback = clampi(feedback, 0, 15);
  bus->mod_freq = clampi(mod_freq, 0, 31);
  bus->mod_depth = clampi(mod_depth, 0, 31);
  bus->level = clampi(level, 0, 15);
  delay_cache_params(engine, bus);
  return 0;
}

void delay_params_get(skred_engine_t *engine, int bus_number, int *coarse, int *fine, int *feedback, int *mod_freq,
                      int *mod_depth, int *level) {
  int index = delay_bus_index(bus_number);
  if (index < 0) {
    if (coarse) *coarse = 0;
    if (fine) *fine = 0;
    if (feedback) *feedback = 0;
    if (mod_freq) *mod_freq = 0;
    if (mod_depth) *mod_depth = 0;
    if (level) *level = 0;
    return;
  }
  delay_bus_t *bus = &delay_bus[index];
  if (coarse) *coarse = bus->coarse;
  if (fine) *fine = bus->fine;
  if (feedback) *feedback = bus->feedback;
  if (mod_freq) *mod_freq = bus->mod_freq;
  if (mod_depth) *mod_depth = bus->mod_depth;
  if (level) *level = bus->level;
}

int delay_damping_set(skred_engine_t *engine, int bus_number, int damping, int hp) {
  int index = delay_bus_index(bus_number);
  if (index < 0) return SYNTH_INVALID_VOICE;
  delay_bus_t *bus = &delay_bus[index];
  bus->damping = clampi(damping, 0, 15);
  bus->hp = clampi(hp, 0, 15);
  delay_cache_params(engine, bus);
  return 0;
}

int delay_freeze_set(skred_engine_t *engine, int bus_number, int on) {
  int index = delay_bus_index(bus_number);
  if (index < 0) return SYNTH_INVALID_VOICE;
  delay_bus[index].freeze = on ? 1 : 0;
  return 0;
}

int delay_pingpong_set(skred_engine_t *engine, int bus_number, int on) {
  int index = delay_bus_index(bus_number);
  if (index < 0) return SYNTH_INVALID_VOICE;
  delay_bus[index].pingpong = on ? 1 : 0;
  return 0;
}

void delay_damping_get(skred_engine_t *engine, int bus_number, int *damping, int *hp) {
  int index = delay_bus_index(bus_number);
  if (index < 0) {
    if (damping) *damping = 0;
    if (hp) *hp = 0;
    return;
  }
  delay_bus_t *bus = &delay_bus[index];
  if (damping) *damping = bus->damping;
  if (hp) *hp = bus->hp;
}

int delay_freeze_get(skred_engine_t *engine, int bus_number) {
  int index = delay_bus_index(bus_number);
  return index < 0 ? 0 : delay_bus[index].freeze;
}

int delay_pingpong_get(skred_engine_t *engine, int bus_number) {
  int index = delay_bus_index(bus_number);
  return index < 0 ? 0 : delay_bus[index].pingpong;
}

int delay_grit_set(skred_engine_t *engine, int bus_number, int bits, int native) {
  int index = delay_bus_index(bus_number);
  if (index < 0) return SYNTH_INVALID_VOICE;
  delay_bus_t *bus = &delay_bus[index];
  bus->bits = clampi(bits, 0, 16);
  bus->native = native ? 1 : 0;
  delay_cache_params(engine, bus);
  return 0;
}

void delay_grit_get(skred_engine_t *engine, int bus_number, int *bits, int *native) {
  int index = delay_bus_index(bus_number);
  if (index < 0) {
    if (bits) *bits = 0;
    if (native) *native = 0;
    return;
  }
  delay_bus_t *bus = &delay_bus[index];
  if (bits) *bits = bus->bits;
  if (native) *native = bus->native;
}

int delay_time_ms_set(skred_engine_t *engine, int bus_number, float target_ms) {
  int index = delay_bus_index(bus_number);
  if (index < 0) return SYNTH_INVALID_VOICE;
  delay_bus_t *bus = &delay_bus[index];

  float max_ms = 8.0f * (float)(1 << 7);   // 1024ms
  if (target_ms < 4.0f) target_ms = 4.0f;
  if (target_ms > max_ms) target_ms = max_ms;

  int coarse = (int)ceilf(log2f(target_ms / 8.0f) - 1e-6f);
  coarse = clampi(coarse, 0, 7);
  float base = 8.0f * (float)(1 << coarse);
  float fine_factor = clampf(target_ms / base, 0.5f, 1.0f);
  int fine = (int)roundf((fine_factor - 0.5f) / 0.5f * 15.0f);

  bus->coarse = coarse;
  bus->fine = clampi(fine, 0, 15);
  delay_cache_params(engine, bus);
  return 0;
}

// division: 1.0 = quarter note, 0.5 = eighth, 0.75 = dotted eighth, 1.5 = dotted quarter, etc.
int delay_time_sync_set(skred_engine_t *engine, int bus_number, float bpm, float division) {
  if (bpm <= 0.0f || division <= 0.0f) return SYNTH_INVALID_VOICE;
  float quarter_ms = 60000.0f / bpm;
  return delay_time_ms_set(engine, bus_number, quarter_ms * division);
}

void delay_clear(skred_engine_t *engine) {
  for (int i = 0; i < DELAY_BUS_COUNT; i++) {
    memset(delay_bus[i].buffer, 0, sizeof(delay_bus[i].buffer));
    memset(delay_bus[i].buffer_r, 0, sizeof(delay_bus[i].buffer_r));
    delay_bus[i].write = 0;
    delay_bus[i].write_r = 0;
    delay_bus[i].active = 0;
    delay_bus[i].idle_frames = 0;
    delay_bus[i].lfo_phase = 0.0f;
    delay_bus[i].lp_state[0] = delay_bus[i].lp_state[1] = 0.0f;
    delay_bus[i].hp_state[0] = delay_bus[i].hp_state[1] = 0.0f;
    delay_bus[i].hp_prev[0]  = delay_bus[i].hp_prev[1]  = 0.0f;
  }
}

static void delay_format_bus(char *out, size_t out_size, int index) {
  delay_bus_t *bus = &delay_bus[index];
  snprintf(out, out_size,
    "DL%d,%d,%d,%d,%d,%d,%d DD%d,%d DF%d DP%d DG%d,%d\n",
    index + 1, bus->coarse, bus->fine, bus->feedback, bus->mod_freq,
    bus->mod_depth, bus->level, bus->damping, bus->hp, bus->freeze,
    bus->pingpong, bus->bits, bus->native);
}

const char *delay_bus_format(int bus_number) {
  static char out[160];
  int index = delay_bus_index(bus_number);
  if (index < 0) {
    snprintf(out, sizeof(out), "# DL invalid bus=%d range=1..%d\n",
             bus_number, DELAY_BUS_COUNT);
  } else {
    delay_format_bus(out, sizeof(out), index);
  }
  return out;
}

const char *delay_format(void) {
  static char out[640];
  size_t used = 0;
  out[0] = '\0';
  for (int i = 0; i < DELAY_BUS_COUNT; i++) {
    char line[160];
    delay_format_bus(line, sizeof(line), i);
    int wrote = snprintf(out + used, sizeof(out) - used, "%s", line);
    if (wrote < 0) break;
    if ((size_t)wrote >= sizeof(out) - used) {
      used = sizeof(out) - 1;
      break;
    }
    used += (size_t)wrote;
  }
  return out;
}

const char *delay_status(void) {
  static char out[96];
  int active = 0;
  int sends = 0;
  int eligible = 0;

  for (int i = 0; i < DELAY_BUS_COUNT; i++) {
    if (delay_bus[i].active) active++;
  }

  for (int voice = 0; voice < synth_config.voice_max; voice++) {
    if (sv.delay_send[voice] <= 0.0f) continue;
    sends++;
    int track = atomic_load_int(&sv.record_pending[voice]);
    if (track >= 1 && track <= RECORD_TRACK_MAX &&
        delay_voice_can_send(&skred_global_engine, voice)) {
      eligible++;
    }
  }

  snprintf(out, sizeof(out), "delay: active %d/%d sends:%d eligible:%d\n",
           active, DELAY_BUS_COUNT, sends, eligible);
  return out;
}

int volume_set(float v) {
  volume_user = v;
  volume_final = DB_TO_LINEAR(v);
  return 0;
}

float volume_get(void) {
  return volume_user;
}

void audio_rng_init(uint64_t *rng, uint64_t seed);
float audio_rng_float(uint64_t *rng);

