#include <float.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "synth-types.h"

#include "synth-config.h"
#include "synth-state.h"
#include "synth-alloc.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "portable_atomic.h"
#include "control-events.h"

atomic_uint64_t synth_sample_count;
int synth_sample_rate = SKRED_DEFAULT_SAMPLE_RATE;

#define SAMPLE_COUNT_PUT(n) atomic_store_uint64(&synth_sample_count, n)
#define SAMPLE_COUNT_GET() atomic_load_uint64(&synth_sample_count)
#define SAMPLE_COUNT_ADD(n) atomic_fetch_add_uint64(&synth_sample_count, n)

#define VOLUME_DEFAULT (-20.0f)
#define DB_TO_LINEAR(v) powf(10.f, (v) / 20.0f)
#define SYNTH_INVALID_VOICE (100)
#define DELAY_MAX_FRAMES (65536)
#define DELAY_BUS_COUNT (4)

typedef struct {
  float buffer[DELAY_MAX_FRAMES];
  int write;
  int active;
  int idle_frames;
  float lfo_phase;
  int coarse;
  int fine;
  int feedback;
  int mod_freq;
  int mod_depth;
  int level;
} delay_bus_t;

static delay_bus_t delay_bus[DELAY_BUS_COUNT];

static int verbose = 0;

int volume_set(float v);
float volume_get(void);
extern float volume_user;
extern float volume_final;
static int voice_invalid(int voice);
static void synth_track_defaults(void);

int synth_sample_rate_set(int sample_rate) {
  if (sample_rate < 1) sample_rate = SKRED_DEFAULT_SAMPLE_RATE;
  synth_sample_rate = sample_rate;
  return synth_sample_rate;
}

int synth_sample_rate_get(void) {
  return synth_sample_rate;
}

static int mod_voice_invalid(int voice) {
  return voice < -1 || voice >= synth_config.voice_max;
}

void synth_init(int vc) {
  SAMPLE_COUNT_PUT(0);

  if (synth_config.wave_table_max == 0)
    synth_config_defaults();

  synth_config_set_voices(vc);
  if (verbose) printf("# vc = %d\n", vc);

  if (verbose) printf("# synth_config.voice_max = %d\n", synth_config.voice_max);
  if (verbose) printf("# synth_config.wave_table_max = %d\n", synth_config.wave_table_max);

  synth_alloc_voices(synth_config.voice_max);
  synth_alloc_waves(synth_config.wave_table_max);

  if (verbose) printf("# synth_config.voice_max = %d\n", synth_config.voice_max);
  if (verbose) printf("# synth_config.wave_table_max = %d\n", synth_config.wave_table_max);

  volume_set(VOLUME_DEFAULT);
  memset(delay_bus, 0, sizeof(delay_bus));
  synth_track_defaults();
}


void synth_free(void) {
  synth_free_voices();
  synth_free_waves();
}

static float track_volume_db[RECORD_TRACK_COUNT];
static float track_volume_linear[RECORD_TRACK_COUNT];
static text_t track_name[RECORD_TRACK_COUNT];
static int track_defaults_loaded;

static int synth_track_invalid(int track) {
  return track < 0 || track > RECORD_TRACK_MAX;
}

static void synth_track_defaults(void) {
  for (int track = 0; track <= RECORD_TRACK_MAX; track++) {
    track_volume_db[track] = VOLUME_DEFAULT;
    track_volume_linear[track] = DB_TO_LINEAR(VOLUME_DEFAULT);
    track_name[track][0] = '\0';
  }
  snprintf(track_name[0], TEXT_MAX, "master");
  track_defaults_loaded = 1;
}

static void synth_track_ensure_defaults(void) {
  if (!track_defaults_loaded) synth_track_defaults();
}

int synth_record_track_set(int voice, int track) {
  if (voice < 0 || voice >= synth_config.voice_max ||
      track < 0 || track > RECORD_TRACK_MAX) {
    return -1;
  }
  atomic_store_int(&sv.record_pending[voice], track);
  return 0;
}

int synth_record_track_get(int voice) {
  if (voice < 0 || voice >= synth_config.voice_max) return -1;
  return atomic_load_int(&sv.record_pending[voice]);
}

int synth_track_volume_set(int track, float db) {
  if (synth_track_invalid(track) || !isfinite(db)) return -1;
  synth_track_ensure_defaults();
  track_volume_db[track] = db;
  track_volume_linear[track] = DB_TO_LINEAR(db);
  return 0;
}

float synth_track_volume_db_get(int track) {
  if (synth_track_invalid(track)) return 0.0f;
  synth_track_ensure_defaults();
  if (track == 0) return volume_user;
  return track_volume_db[track];
}

float synth_track_volume_linear_get(int track) {
  if (synth_track_invalid(track)) return 0.0f;
  synth_track_ensure_defaults();
  if (track == 0) return volume_final;
  return track_volume_linear[track];
}

int synth_track_name_set(int track, const char *name) {
  if (synth_track_invalid(track) || !name) return -1;
  synth_track_ensure_defaults();
  snprintf(track_name[track], TEXT_MAX, "%s", name);
  return 0;
}

const char *synth_track_name_get(int track) {
  if (synth_track_invalid(track)) return "";
  synth_track_ensure_defaults();
  return track_name[track];
}


int requested_synth_frames_per_callback = SYNTH_FRAMES_PER_CALLBACK;
int synth_frames_per_callback = 0;

#define SMOOTH_DEFAULT (0.02f)

float volume_user = VOLUME_DEFAULT;
float volume_final = 1.0f;
float volume_smoother_gain = 0.0f;
float volume_smoother_smoothing = 0.002f;
float volume_threshold = 0.05f;
float volume_smoother_higher_smoothing = 0.3f;

static float clampf(float value, float min_value, float max_value) {
  if (value < min_value) return min_value;
  if (value > max_value) return max_value;
  return value;
}

static int clampi(int value, int min_value, int max_value) {
  if (value < min_value) return min_value;
  if (value > max_value) return max_value;
  return value;
}

static float delay_12bit_clip(float x) {
  x = clampf(x, -1.0f, 1.0f);
  return roundf(x * 2047.0f) * (1.0f / 2047.0f);
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
  base_ms = 8.0f * (float)(1 << (coarse > 6 ? 6 : coarse));
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

static float delay_read(const delay_bus_t *bus, float delay_frames) {
  if (delay_frames < 1.0f) delay_frames = 1.0f;
  if (delay_frames > (float)(DELAY_MAX_FRAMES - 2))
    delay_frames = (float)(DELAY_MAX_FRAMES - 2);

  float read = (float)bus->write - delay_frames;
  while (read < 0.0f) read += (float)DELAY_MAX_FRAMES;
  int i0 = (int)read;
  int i1 = i0 + 1;
  if (i1 >= DELAY_MAX_FRAMES) i1 = 0;
  float frac = read - (float)i0;
  return bus->buffer[i0] + frac * (bus->buffer[i1] - bus->buffer[i0]);
}

static void delay_process(delay_bus_t *bus, float input, float *left, float *right) {
  float base_frames = delay_base_ms(bus) * (float)MAIN_SAMPLE_RATE * 0.001f;
  float lfo = 0.0f;
  float mod_frames;
  float delayed_left;
  float delayed_right;
  float delayed_feedback;
  float write_sample;
  float wet;
  float stereo_offset = 0.0f;

  if (bus->mod_depth > 0) {
    lfo = sinf(bus->lfo_phase);
    if (bus->mod_freq > 0) {
      bus->lfo_phase += 2.0f * (float)M_PI *
        delay_lfo_hz(bus) / (float)MAIN_SAMPLE_RATE;
      if (bus->lfo_phase >= 2.0f * (float)M_PI)
        bus->lfo_phase -= 2.0f * (float)M_PI;
    }
    stereo_offset = delay_stereo_offset_frames(bus);
  }

  mod_frames = lfo * delay_mod_depth_frames(bus);
  delayed_left = delay_read(bus, base_frames + mod_frames);
  delayed_right = delay_read(bus, base_frames - mod_frames + stereo_offset);
  delayed_feedback = (delayed_left + delayed_right) * 0.5f;
  write_sample = delay_12bit_clip(input + delayed_feedback * delay_feedback_gain(bus));
  bus->buffer[bus->write] = write_sample;
  bus->write++;
  if (bus->write >= DELAY_MAX_FRAMES) bus->write = 0;

  wet = (float)bus->level / 15.0f;
  if (left) *left = delayed_left * wet;
  if (right) *right = delayed_right * wet;

  if (fabsf(input) + fabsf(delayed_left) + fabsf(delayed_right) +
      fabsf(write_sample) > 0.0000001f) {
    bus->active = 1;
    bus->idle_frames = 0;
  } else if (bus->idle_frames >= DELAY_MAX_FRAMES) {
    bus->active = 0;
  } else {
    bus->idle_frames++;
  }
}

static int delay_voice_can_send(int voice) {
  if (voice_invalid(voice)) return 0;
  if (fabsf(sv.pan[voice]) > 0.0001f) return 0;
  if (sv.pan_mod_osc[voice] >= 0) return 0;
  return 1;
}

int delay_send_set(int voice, float amount) {
  if (voice_invalid(voice) || !isfinite(amount))
    return SYNTH_INVALID_VOICE;
  if (amount > 1.0f) amount /= 15.0f;
  sv.delay_send[voice] = clampf(amount, 0.0f, 1.0f);
  return 0;
}

int delay_params_set(int bus_number, int coarse, int fine, int feedback, int mod_freq,
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
  return 0;
}

void delay_params_get(int bus_number, int *coarse, int *fine, int *feedback, int *mod_freq,
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

void delay_clear(void) {
  for (int i = 0; i < DELAY_BUS_COUNT; i++) {
    memset(delay_bus[i].buffer, 0, sizeof(delay_bus[i].buffer));
    delay_bus[i].write = 0;
    delay_bus[i].active = 0;
    delay_bus[i].idle_frames = 0;
    delay_bus[i].lfo_phase = 0.0f;
  }
}

static void delay_format_bus(char *out, size_t out_size, int index) {
  delay_bus_t *bus = &delay_bus[index];
  snprintf(out, out_size,
    "DL%d,%d,%d,%d,%d,%d,%d\n",
    index + 1, bus->coarse, bus->fine, bus->feedback, bus->mod_freq,
    bus->mod_depth, bus->level);
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
        delay_voice_can_send(voice)) {
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

void audio_rng_init(uint64_t *rng, uint64_t seed) {
  *rng = seed ? seed : 1; // Ensure non-zero seed
}

// Generate next random number (full 64-bit range)
uint64_t audio_rng_next(uint64_t *rng) {
    // High-quality LCG parameters (Knuth's MMIX)
    *rng = *rng * 6364136223846793005ULL + 1442695040888963407ULL;
    return *rng;
}

// Generate random float in range [-1.0, 1.0] for audio
float audio_rng_float(uint64_t *rng) {
    uint64_t raw = audio_rng_next(rng);
    // Use upper 32 bits for better quality
    uint32_t val = (uint32_t)(raw >> 32);
    // Convert to signed float [-1.0, 1.0]
    return (float)((int32_t)val) / 2147483648.0f;
}

static inline float audio_rng_raw_float(uint64_t raw) {
    uint32_t val = (uint32_t)(raw >> 32);
    return (float)((int32_t)val) / 2147483648.0f;
}

float osc_get_phase_inc(int v, float f) {
  // Compute the frequency in "table samples per system sample"
  // This works even if table_rate ≠ system rate
  float g = f;
  if (sv.one_shot[v] && sv.offset_hz[v] > 0.0f) g /= sv.offset_hz[v];
  float phase_inc = (g * (float)sv.table_size[v]) / sv.table_rate[v] * (sv.table_rate[v] / MAIN_SAMPLE_RATE);
  return phase_inc;
}

void osc_set_freq(int v, float f) {
  sv.phase_inc[v] = osc_get_phase_inc(v, f);
}

// Fast power approximation using bit manipulation
// About 10x faster than powf, ~1-2% max error for your use case
static inline float fast_pow(float a, float b) {
    // Clamp input to avoid undefined behavior
    if (a <= 0.0f) return 0.0f;

    union { float f; int i; } u = { a };
    u.i = (int)(b * (u.i - 1065353216) + 1065353216);
    return u.f;
}

float cz_phasor(int n, float p, float d, int table_size) {
    const float table_size_f = (float)table_size;
    float phase = p / table_size_f;

    // Clamp d to safe range [0, 1)
    d = (d < 0.0f) ? 0.0f : (d > 0.999f ? 0.999f : d);

    switch (n) {
        case 1: { // saw -> pulse
            const float inv_d = 0.5f / d;
            const float inv_1_minus_d = 0.5f / (1.0f - d);
            if (phase < d) {
                phase *= inv_d;
            } else {
                phase = 0.5f + (phase - d) * inv_1_minus_d;
            }
            break;
        }
        case 2: { // square (folded sine)
            const float half_d = d * 0.5f;
            const float scale = 0.5f / (0.5f - half_d);
            if (phase < 0.5f) {
                phase *= scale;
            } else {
                phase = 1.0f - (1.0f - phase) * scale;
            }
            break;
        }
        case 3: { // triangle
            const float half_d = d * 0.5f;
            const float scale = 0.5f / (0.5f - half_d);
            if (phase < 0.5f) {
                phase *= scale;
            } else {
                phase = 0.5f + (phase - 0.5f) * scale;
            }
            break;
        }
        case 4: { // double sine
            // phase *= 2.0f;
            // if (phase >= 1.0f) phase -= 1.0f;
            phase = fmodf(phase * 2.0f, 1.0f);
            break;
        }
        case 5: { // saw -> triangle
            const float half_d = d * 0.5f;
            const float scale1 = 0.5f / (0.5f - half_d);
            const float scale2 = 0.5f / (0.5f + half_d);
            if (phase < 0.5f) {
                phase *= scale1;
            } else {
                phase = 0.5f + (phase - 0.5f) * scale2;
            }
            break;
        }
        case 6: // resonant 1
            phase = fast_pow(phase, 1.0f + 4.0f * d);
            break;
        case 7: // resonant 2
            phase = fast_pow(phase, 1.0f + 8.0f * d);
            break;
        default:
            return p;
    }

    return phase * table_size_f;
}

static inline int osc_loop_crossings(double distance, double loop_length) {
    double crossings = distance / loop_length;
    return crossings >= (double)(INT_MAX - 1) ?
        INT_MAX : (int)crossings + 1;
}

float osc_next(int voice, float phase_inc) {
    if (sv.finished[voice]) return 0.0f;

    const int table_size = sv.table_size[voice];
    const bool one_shot = sv.one_shot[voice];
    const bool loop_active = sv.loop_active[voice];

    double phase_step = phase_inc;
    if (sv.direction[voice]) phase_step = -phase_step;
    const bool reverse_step = phase_step < 0.0;

    double phase = sv.phase[voice] + phase_step;

    if (!isfinite(phase)) {
        sv.phase[voice] = 0.0f;
        sv.finished[voice] = one_shot;
        return 0.0f;
    }

    // Get loop boundaries (precomputed if available)
    const double loop_start = loop_active && sv.loop_valid[voice]
        ? (double)sv.loop_start_f[voice] : 0.0;
    const double loop_end = loop_active && sv.loop_valid[voice]
        ? (double)sv.loop_end_f[voice] : (double)table_size;
    const double loop_length = loop_end - loop_start;

    // Wrap phase
    if (!one_shot) {
        if (phase >= loop_end) {
            phase = loop_start + fmod(phase - loop_start, loop_length);
        } else if (phase < loop_start) {
            phase = loop_end - fmod(loop_start - phase, loop_length);
        }
    } else if (!reverse_step && phase >= loop_end) {
        if (!loop_active) {
            phase = loop_end - 1e-6;
            sv.finished[voice] = 1;
        } else if (sv.loop_stop_requested[voice]) {
            sv.loop_active[voice] = 0;
            sv.loop_stop_requested[voice] = 0;
            sv.loop_ended[voice] = 1;
            if (phase >= (double)table_size) {
                phase = (double)table_size - 1e-6;
                sv.finished[voice] = 1;
            }
        } else if (sv.loop_bounded[voice]) {
            int crossings = osc_loop_crossings(phase - loop_end, loop_length);
            int wraps = crossings < sv.loop_remaining[voice] ?
                crossings : sv.loop_remaining[voice];
            phase -= (double)wraps * loop_length;
            sv.loop_remaining[voice] -= wraps;
            if (wraps < crossings) {
                sv.loop_active[voice] = 0;
                sv.loop_stop_requested[voice] = 0;
                sv.loop_ended[voice] = 1;
                if (phase >= (double)table_size) {
                    phase = (double)table_size - 1e-6;
                    sv.finished[voice] = 1;
                }
            }
        } else {
            phase = loop_start + fmod(phase - loop_start, loop_length);
        }
    } else if (reverse_step && phase < loop_start) {
        if (!loop_active) {
            phase = loop_start;
            sv.finished[voice] = 1;
        } else if (sv.loop_stop_requested[voice]) {
            sv.loop_active[voice] = 0;
            sv.loop_stop_requested[voice] = 0;
            sv.loop_ended[voice] = 1;
            if (phase < 0.0f) {
                phase = 0.0f;
                sv.finished[voice] = 1;
            }
        } else if (sv.loop_bounded[voice]) {
            int crossings = osc_loop_crossings(loop_start - phase, loop_length);
            int wraps = crossings < sv.loop_remaining[voice] ?
                crossings : sv.loop_remaining[voice];
            phase += (double)wraps * loop_length;
            sv.loop_remaining[voice] -= wraps;
            if (wraps < crossings) {
                sv.loop_active[voice] = 0;
                sv.loop_stop_requested[voice] = 0;
                sv.loop_ended[voice] = 1;
                if (phase < 0.0f) {
                    phase = 0.0f;
                    sv.finished[voice] = 1;
                }
            }
        } else {
            phase = loop_end - fmod(loop_start - phase, loop_length);
        }
    }

    sv.phase[voice] = phase;

    // Get sample

    double final_phase;

    if (sv.cz_mode[voice] && sv.cz_mod_osc[voice] >= 0) {
      int dv = sv.cz_mod_osc[voice];
      float dm = (dv >= 0) ? sv.sample[dv] * sv.cz_mod_depth[voice] : 1.0f;
      final_phase = cz_phasor(sv.cz_mode[voice], (float)phase, sv.cz_distortion[voice] + dm, table_size);
    } else {
      final_phase = phase;
    }

    int idx = (int)final_phase;

    if (idx >= table_size) idx = table_size - 1;
    if (idx < 0) idx = 0;

    // Check if interpolation is enabled for this voice
    if (sv.interpolate[voice]) {
        // Linear interpolation
        float frac = (float)(final_phase - (double)idx);

        int next_idx = idx + 1;
        //if (next_idx >= table_size) next_idx = 0;  // Wrap for seamless loops
        if (next_idx >= table_size) next_idx = sv.one_shot[voice] ? table_size - 1 : 0;

        float sample1 = sv.table[voice][idx];
        float sample2 = sv.table[voice][next_idx];

        return sample1 + frac * (sample2 - sample1);
    } else {
        // No interpolation - just return the sample
        return sv.table[voice][idx];
    }
}

void osc_set_wave_table_index(int voice, int wave) {
  // if we were using a r/w wave table, adjust ref count
  int old = sv.wave_table_index[voice];
  if (old == wave) return;
  // old == -1 means voice was never assigned a wave table yet (e.g. first
  // call from voice_reset).  Guard before indexing to avoid sw.readonly[-1].
  if (old >= 0) sw.refcount[old]--;
  if (wave >= 0) sw.refcount[wave]++;
  if (sw.data[wave] && sw.size[wave] && sw.rate[wave] > 0.0) {
    sv.wave_table_index[voice] = wave;
    int update_freq = 0;
    if (sw.one_shot[wave]) sv.finished[voice] = 1;
    else sv.finished[voice] = 0;
    if (
      sv.table_rate[voice] != sw.rate[wave] ||
      sv.table_size[voice] != sw.size[wave]
      ) update_freq = 1;
    sv.table_rate[voice] = sw.rate[wave];
    sv.table_size[voice] = sw.size[wave];
    sv.table_size_rate[voice] = (float)sv.table_size[voice] / MAIN_SAMPLE_RATE;
    sv.table[voice] = sw.data[wave];
    sv.one_shot[voice] = sw.one_shot[wave];
    sv.loop_start[voice] = sw.loop_start[wave];
    sv.loop_enabled[voice] = sw.loop_enabled[wave];
    sv.loop_active[voice] = sv.loop_enabled[voice];
    sv.loop_bounded[voice] = sv.loop_count[voice] > 0;
    sv.loop_remaining[voice] = sv.loop_count[voice];
    sv.loop_stop_requested[voice] = 0;
    sv.loop_ended[voice] = 0;
    sv.loop_end[voice] = sw.loop_end[wave];
    sv.midi_note[voice] = sw.midi_note[wave];
    sv.offset_hz[voice] = sw.offset_hz[wave];
    sv.direction[voice] = sw.direction[wave];
    //
    int start = sv.loop_start[voice];
    int end = sv.loop_end[voice];
    sv.loop_start_f[voice] = (float)start;
    sv.loop_end_f[voice] = (float)end;
    if (end > start) {
      sv.loop_valid[voice] = 1;
      sv.loop_length[voice] = (float)(end - start);
    } else {
      sv.loop_valid[voice] = 0;
      sv.loop_length[voice] = (float)sv.table_size[voice];
    }
    //
    // sv.phase[voice] = 0; // need to decide how to sync/reset phase???
    if (update_freq) {
      osc_set_freq(voice, sv.freq[voice]);
    }
  }
}

void osc_trigger(int voice) {
    sv.finished[voice] = 0;
    sv.loop_active[voice] = sv.loop_enabled[voice];
    sv.loop_bounded[voice] = sv.loop_count[voice] > 0;
    sv.loop_remaining[voice] = sv.loop_count[voice];
    sv.loop_stop_requested[voice] = 0;
    sv.loop_ended[voice] = 0;

    if (sv.one_shot[voice]) {
        if (sv.direction[voice]) {
            sv.phase[voice] = (double)(sv.table_size[voice] - 1);
        } else {
            sv.phase[voice] = 0.0;
        }
    } else {
        // Preserve direction, but start at appropriate boundary
        if (sv.direction[voice]) {
            // Backward playback: start at loop end
            sv.phase[voice] = sv.loop_active[voice]
                ? (double)sv.loop_end[voice] - 1e-6  // or sv.loop_end_f[voice]
                : (double)(sv.table_size[voice] - 1);
        } else {
            // Forward playback: start at loop start
            sv.phase[voice] = sv.loop_active[voice]
                ? (double)sv.loop_start[voice]  // or sv.loop_start_f[voice]
                : 0.0;
        }
    }
}

float quantize_bits_int(float v, int bits) {
  int levels = (1 << bits) - 1;
  int iv = (int)(v * (float)levels + 0.5);
  return (float)iv * (1.0f / (float)levels);
}

// Process a single sample through the filter - VERY FAST
// Only multiplication and addition, no transcendental functions
float mmf_process(int n, float input) {
    // Calculate output using Direct Form II - only 5 multiplies, 4 adds
    float output = sv.filter[n].b0 * input +
                  sv.filter[n].b1 * sv.filter[n].x1 +
                  sv.filter[n].b2 * sv.filter[n].x2 -
                  sv.filter[n].a1 * sv.filter[n].y1 -
                  sv.filter[n].a2 * sv.filter[n].y2;

    // Update delay lines
    sv.filter[n].x2 = sv.filter[n].x1;
    sv.filter[n].x1 = input;
    sv.filter[n].y2 = sv.filter[n].y1;
    sv.filter[n].y1 = output;

    return output;
}

static void envelope_snapshot_config(envelope_t *e) {
    e->attack_time   = e->a * MAIN_SAMPLE_RATE;
    e->decay_time    = e->d * MAIN_SAMPLE_RATE;
    e->sustain_level = fmaxf(0, fminf(1.0f, e->s));
    e->release_time  = e->r * MAIN_SAMPLE_RATE;
}

// Configure the next trigger without disturbing an envelope in progress.
void envelope_configure_e(envelope_t *e, float a, float d, float s, float r) {
    e->a = a;
    e->d = d;
    e->s = s;
    e->r = r;
}

// Initialize both configured parameters and runtime state.
void envelope_init_e(envelope_t *e, float a, float d, float s, float r) {
    envelope_configure_e(e, a, d, s, r);
    envelope_snapshot_config(e);
    e->sample_start         = 0;
    e->sample_release       = UINT64_MAX;
    e->is_active            = 0;
    e->velocity             = 1.0f;
    e->amplitude_at_release = 0.0f;
    e->amplitude_at_trigger = 0.0f;
    e->current_amplitude    = 0.0f;
}

void envelope_init(int v, float a, float d, float s, float r) {
    envelope_init_e(&sv.amp_envelope[v], a, d, s, r);
}

void envelope_trigger_e(envelope_t *e, float f) {
    envelope_snapshot_config(e);
    if (e->is_active)
        e->amplitude_at_trigger = e->current_amplitude;
    else
        e->amplitude_at_trigger = 0.0f;
    e->sample_start   = SAMPLE_COUNT_GET();
    e->sample_release = UINT64_MAX;
    e->velocity       = f;
    e->is_active      = 1;
}

void amp_envelope_trigger(int v, float f) {
    envelope_trigger_e(&sv.amp_envelope[v], f);
}

void envelope_release_e_at(envelope_t *e, uint64_t current_sample) {
    if (e->is_active && e->sample_release == UINT64_MAX) {
        e->sample_release       = current_sample;
        e->amplitude_at_release = e->current_amplitude;
    }
}

void envelope_schedule_release_e_at(envelope_t *e, uint64_t release_sample) {
    if (e->is_active && e->sample_release == UINT64_MAX) {
        e->sample_release = release_sample;
        e->amplitude_at_release = -1.0f;
    }
}

void envelope_release_e(envelope_t *e) {
    envelope_release_e_at(e, SAMPLE_COUNT_GET());
}

void amp_envelope_release(int v) {
    envelope_release_e(&sv.amp_envelope[v]);
}

float envelope_step_e(envelope_t *e, uint64_t current_sample) {
    if (!e->is_active) return 0.0f;

    float held_out = 0.0f;
    float out = 0.0f;
    float samples_since_start = current_sample >= e->sample_start
        ? (float)(current_sample - e->sample_start) : 0.0f;

    if (samples_since_start < e->attack_time) {
        float attack_progress = samples_since_start / e->attack_time;
        float start_val = e->amplitude_at_trigger;
        float curved_progress = attack_progress * attack_progress;
        held_out = start_val + (curved_progress * (1.0f - start_val));
    }
    else if (samples_since_start < (e->attack_time + e->decay_time)) {
        float samples_in_decay = samples_since_start - e->attack_time;
        float decay_progress = samples_in_decay / e->decay_time;
        held_out = 1.0f - decay_progress * (1.0f - e->sustain_level);
    }
    else {
        held_out = e->sustain_level;
    }

    if (e->sample_release == UINT64_MAX) {
        out = held_out;
        if (e->sustain_level <= 0.0f &&
            samples_since_start >= e->attack_time + e->decay_time) {
            e->is_active = 0;
            out = 0.0f;
        }
    } else if (current_sample < e->sample_release) {
        out = e->amplitude_at_release < 0.0f ? held_out : e->amplitude_at_release;
    } else {
        if (e->amplitude_at_release < 0.0f) {
            e->amplitude_at_release = held_out;
        }
        if (e->release_time <= 0.0f) {
            e->is_active = 0;
            out = 0.0f;
        } else {
            float samples_since_release = (float)(current_sample - e->sample_release);
            if (samples_since_release < e->release_time) {
                float release_progress = samples_since_release / e->release_time;
                out = e->amplitude_at_release * (1.0f - release_progress);
            } else {
                e->is_active = 0;
                out = 0.0f;
            }
        }
    }

    e->current_amplitude = out;
    return out * e->velocity;
}

float amp_envelope_step(int v, uint64_t current_sample) {
    return envelope_step_e(&sv.amp_envelope[v], current_sample);
}

static uint64_t one_shot_natural_frames(int voice) {
    if (!sv.one_shot[voice] || (sv.loop_active[voice] && !sv.loop_bounded[voice]))
        return 0;
    double inc = fabs((double)sv.phase_inc[voice]);
    if (!isfinite(inc) || inc <= 0.0) return 0;

    bool reverse_step = sv.direction[voice] != 0;
    bool loop_active = sv.loop_active[voice] != 0;
    bool loop_bounded = loop_active && sv.loop_bounded[voice] != 0;
    int remaining = loop_bounded ? sv.loop_remaining[voice] : 0;
    double phase = sv.phase[voice];
    double loop_start = loop_active && sv.loop_valid[voice]
        ? (double)sv.loop_start_f[voice] : 0.0;
    double loop_end = loop_active && sv.loop_valid[voice]
        ? (double)sv.loop_end_f[voice] : (double)sv.table_size[voice];
    double loop_length = loop_end - loop_start;

    if (!isfinite(phase) || !isfinite(loop_start) || !isfinite(loop_end) ||
        !isfinite(loop_length) || loop_length <= 0.0)
        return 0;

    uint64_t total = 0;
    int safety = remaining + 2;
    while (safety-- > 0) {
        double distance;
        uint64_t frames;
        if (reverse_step) {
            distance = phase - loop_start;
            if (!isfinite(distance) || distance < 0.0) distance = 0.0;
            double frames_f = floor(distance / inc) + 1.0;
            if (!isfinite(frames_f) || frames_f >= (double)(UINT64_MAX - total))
                return 0;
            frames = (uint64_t)frames_f;
            phase -= (double)frames * inc;
            if (!loop_bounded) return total + frames;

            int crossings = osc_loop_crossings(loop_start - phase, loop_length);
            int wraps = crossings < remaining ? crossings : remaining;
            phase += (double)wraps * loop_length;
            remaining -= wraps;
            total += frames;
            if (wraps < crossings) return total;
        } else {
            distance = loop_end - phase;
            if (!isfinite(distance) || distance <= 0.0) distance = inc;
            double frames_f = ceil(distance / inc);
            if (frames_f < 1.0) frames_f = 1.0;
            if (!isfinite(frames_f) || frames_f >= (double)(UINT64_MAX - total))
                return 0;
            frames = (uint64_t)frames_f;
            phase += (double)frames * inc;
            if (!loop_bounded) return total + frames;

            int crossings = osc_loop_crossings(phase - loop_end, loop_length);
            int wraps = crossings < remaining ? crossings : remaining;
            phase -= (double)wraps * loop_length;
            remaining -= wraps;
            total += frames;
            if (wraps < crossings) return total;
        }
    }

    return 0;
}

void amp_envelope_schedule_one_shot_release(int v) {
    if (sv.amp_envelope_mode[v] != 1) return;
    uint64_t frames = one_shot_natural_frames(v);
    if (frames == 0) return;

    envelope_t *e = &sv.amp_envelope[v];
    uint64_t release_frames = e->release_time > 0.0f ? (uint64_t)ceilf(e->release_time) : 0;
    uint64_t start = e->sample_start;
    uint64_t release_sample = start + (frames > release_frames ? frames - release_frames : 0);
    envelope_schedule_release_e_at(e, release_sample);
}

#include "util.h"

static sben_t bench[BENLEN] = {};
static char _stats[65536] = "";

char *synth_stats(void) {
  char *ptr = _stats;
  *ptr = '\0';
  int n = 0;
  for (int i = 0; i < BENLEN; i++) {
    if (bench[i].state != BEN_B) continue;
    //double maxcb = (double)bench[i].frames / (double)MAIN_SAMPLE_RATE * (double)S_TO_MS;
    double dms = ts_diff_ns(&bench[i].a, &bench[i].b) / (double)NS_TO_MS;
    n = sprintf(ptr, "# @%d %gms\n", bench[i].order, dms);
    ptr += n;
    bench[i].state = BEN_0;
  }
  return _stats;
}

#ifdef __APPLE__
#define VOICE_CLOCK CLOCK_MONOTONIC
#else

#ifdef _WIN32
#define VOICE_CLOCK CLOCK_MONOTONIC
#else
#define VOICE_CLOCK CLOCK_MONOTONIC_COARSE
#endif

#endif




void mmf_set_params(int n, float f, float resonance);
#define FILTER_UC (16)

static float empty_capture[65536] = {0};
static float *this_capture;

synth_sample_t sampling = {0};

void synth(float *buffer, float *input, int num_frames, int num_channels, void *user) {
  (void)input;
  synth_record_bus_t *record_bus = (synth_record_bus_t *)user;
  float *record_frames = NULL;
  if (record_bus && record_bus->channels == RECORD_CHANNELS) {
    record_frames = record_bus->frames;
  }
  const int nvoices = synth_voice_count();
  for (int n = 0; n < nvoices; n++) {
    sv.record[n] = atomic_load_int(&sv.record_pending[n]);
  }
#if 0
  const int nframes = num_frames;
  (void)nframes; /* available for future vectorised frame loop */
#endif
  static uint64_t synth_random;
  static int first = 1;
  if (first) {
    synth_frames_per_callback = num_frames;
    audio_rng_init(&synth_random, 1);
    first = 0;
  }

  if (input) this_capture = input;
  else this_capture = empty_capture;


  uint64_t callback_sample = SAMPLE_COUNT_ADD(num_frames);
  for (int i = 0; i < num_frames; i++) {
    uint64_t current_sample = callback_sample + (uint64_t)i;
    float sample_left = 0.0f;
    float sample_right = 0.0f;
    float delay_input[DELAY_BUS_COUNT] = {0};
    float record_left[RECORD_TRACK_COUNT] = {0};
    float record_right[RECORD_TRACK_COUNT] = {0};

    float record_mono = 0.0f;

    float f = 0.0f;
    uint64_t noise_raw = audio_rng_next(&synth_random);
    float whiteish = 0.0f;
    int whiteish_ready = 0;
    for (int n = 0; n < nvoices; n++) {
      if (sv.finished[n]) {
        //sv.sample[n] = 0.0f; // remove to try the below
        // hold last value to modulator consumers see statle output after one-shot ends
        continue;
      }  
      if (sv.user_amp[n] <= SILENT) {
        sv.sample[n] = 0.0f;
        continue;
      }
      if (sv.glissando_enable[n]) {
        // If multiplier is effectively 1, we are already there
        if (sv.glissando_speed[n] == 1.0f) {
          sv.glissando_enable[n] = 0;
        } else {
          sv.phase_inc[n] *= sv.glissando_speed[n];

          // Check if we crossed the target (works for both gliding up and down)
          if ((sv.glissando_speed[n] > 1.0f && sv.phase_inc[n] >= sv.glissando_target[n]) ||
              (sv.glissando_speed[n] < 1.0f && sv.phase_inc[n] <= sv.glissando_target[n])) {
            sv.phase_inc[n] = sv.glissando_target[n];
            sv.glissando_enable[n] = 0;
          }
        }
      }
      int was_finished = sv.finished[n];
      char is_capture = 0;
      if (sv.wave_table_index[n] == WAVE_TABLE_NOISE_ALT) {
        if (!whiteish_ready) {
          whiteish = audio_rng_raw_float(noise_raw);
          whiteish_ready = 1;
        }
        f = whiteish;
      }
      else if (sv.wave_table_index[n] == WAVE_TABLE_CAP_LEFT) {
        f = this_capture[i * AUDIO_CHANNELS];
        is_capture = 1;
      }
      else if (sv.wave_table_index[n] == WAVE_TABLE_CAP_RIGHT) {
        f = this_capture[i * AUDIO_CHANNELS + 1];
        is_capture = 1;
      } else {
        if (sv.freq_mod_osc[n] >= 0 && sv.freq_mod_osc[n] != n) {
          int mod = sv.freq_mod_osc[n];
          float g = sv.sample[mod] * sv.freq_mod_depth[n] + sv.freq_mod_adder[n];
          float inc;
          if (sv.freq_mod_mode[n]) {
            inc = (g * sv.table_size_rate[n]);
          } else {
            inc = sv.phase_inc[n] + (sv.phase_inc[mod] * sv.freq_scale[n] * g);
          }
          f = osc_next(n, inc);
        } else {
          f = osc_next(n, sv.phase_inc[n]);
        }
      if (sv.loop_ended[n]) {
        sv.loop_ended[n] = 0;
        envelope_release_e_at(&sv.amp_envelope[n], current_sample);
        envelope_release_e_at(&sv.filter_envelope[n], current_sample);
        skred_control_voice_event(SKRED_CONTROL_EVENT_VOICE_RELEASE,
          current_sample, n);
      }
      if (!was_finished && sv.finished[n]) {
        skred_control_voice_event(SKRED_CONTROL_EVENT_VOICE_FINISHED,
          current_sample, n);
      }
      }
      if (sv.sample_hold_max[n]) {
        if (sv.sample_hold_count[n] == 0) {
          sv.sample_hold[n] = f;
        }
        sv.sample[n] = sv.sample_hold[n];
        sv.sample_hold_count[n]++;
        if (sv.sample_hold_count[n] >= sv.sample_hold_max[n]) {
          sv.sample_hold_count[n] = 0;
        }
      } else {
        sv.sample[n] = f;
      }

      // apply quantizer
      if (sv.quantize[n]) {
        sv.sample[n] = quantize_bits_int(sv.sample[n], sv.quantize[n]);
      }

      // apply multi-mode filter
      if (sv.filter_mode[n]) {
        if (sv.filter_update_counter[n] <= 0) {
          float cutoff = sv.filter_freq[n];
          if (sv.filter_envelope[n].is_active) {
            float env = envelope_step_e(&sv.filter_envelope[n], current_sample);
            env = fmaxf(0.0f, fminf(1.0f, env));
            cutoff = cutoff + (env * sv.filter_env_depth[n]);
          }
          cutoff = fmaxf(20.0f, fminf(20000.0f, cutoff));
          mmf_set_params(n, cutoff, sv.filter_res[n]);
          sv.filter_update_counter[n] = FILTER_UC;
        }
        sv.filter_update_counter[n]--;
        sv.sample[n] = mmf_process(n, sv.sample[n]);
      }

      // apply amp to sample
      float amp = sv.amp[n];
#if 1
      if (sv.smoother_enable[n]) {
        sv.smoother_gain[n] += sv.smoother_smoothing[n] * (amp - sv.smoother_gain[n]);
        amp = sv.smoother_gain[n];
      }
#endif
      float env = 1.0f;
      float mod = 1.0f;

      int amp_env_was_active = sv.amp_envelope[n].is_active;
      if (sv.use_amp_envelope[n]) env = amp_envelope_step(n, current_sample);
      if (amp_env_was_active && !sv.amp_envelope[n].is_active) {
        skred_control_voice_event(SKRED_CONTROL_EVENT_VOICE_FINISHED,
          current_sample, n);
      }

      if (sv.amp_mod_osc[n] >= 0) {
        int m = sv.amp_mod_osc[n];
        mod = sv.sample[m] * sv.amp_mod_depth[n] + sv.amp_mod_adder[n];
      }

      float final = amp * env * mod;
#if 0
      if (sv.smoother_enable[n]) {
        sv.smoother_gain[n] += sv.smoother_smoothing[n] * (final - sv.smoother_gain[n]);
        final = sv.smoother_gain[n];
      }
#endif

      sv.sample[n] *= final;

      if (sv.disconnect[n] == 0 || is_capture) {
        record_mono += sv.sample[n];
      }

      if (sv.disconnect[n] == 0) {
        if (sv.delay_send[n] > 0.0f && delay_voice_can_send(n)) {
          int bus = -1;
          int delay_track = sv.record[n];
          if (delay_track >= 1 && delay_track <= RECORD_TRACK_MAX)
            bus = delay_track - 1;
          if (bus >= 0 && bus < DELAY_BUS_COUNT)
            delay_input[bus] += sv.sample[n] * sv.delay_send[n];
        }
        float left  = sv.sample[n];
        float right = sv.sample[n];
        // accumulate samples
        if (sv.pan_mod_osc[n] >= 0) {
          // handle pan modulation
          float q = sv.sample[sv.pan_mod_osc[n]] * sv.pan_mod_depth[n] + sv.pan_mod_adder[n];
          sv.pan_left[n]  = (1.0f - q) / 2.0f;
          sv.pan_right[n] = (1.0f + q) / 2.0f;
        }
        left  *= sv.pan_left[n];
        right *= sv.pan_right[n];
        sample_left  += left;
        sample_right += right;
        int track = sv.record[n];
        if (track >= 1 && track <= RECORD_TRACK_MAX) {
          record_left[track] += left;
          record_right[track] += right;
        }
      }
    }

    for (int bus = 0; bus < DELAY_BUS_COUNT; bus++) {
      if (delay_input[bus] != 0.0f) delay_bus[bus].active = 1;
      if (!delay_bus[bus].active) continue;
      float delay_left = 0.0f;
      float delay_right = 0.0f;
      delay_process(&delay_bus[bus], delay_input[bus], &delay_left, &delay_right);
      if (delay_bus[bus].level > 0) {
        sample_left += delay_left;
        sample_right += delay_right;
        int track = bus + 1;
        if (track <= RECORD_TRACK_MAX) {
          record_left[track] += delay_left;
          record_right[track] += delay_right;
        }
      }
    }

    if (sampling.go) {
      sampling.busy = 1;
      sampling.go = 0;
      sampling.ptr = 0;
      sampling.len = 0;
      sampling.offset = 0;
      sampling.trim = 0;
    }
    if (sampling.busy) {
      if (sampling.frames >= 0) {
        sampling.where[sampling.ptr++] = record_mono;
        sampling.frames--;
      } else {
        sampling.len = sampling.ptr-1;
        sampling.busy = 0;
      }
    }

    // Adjust to main volume: smooth it otherwise is sounds crummy with realtime changes
    volume_smoother_gain += volume_smoother_smoothing * (volume_final - volume_smoother_gain);
    float volume_adjusted = volume_smoother_gain;

    sample_left  *= volume_adjusted;
    sample_right *= volume_adjusted;

    if (record_frames) {
      float *record_frame = record_frames + ((size_t)i * RECORD_CHANNELS);
      record_frame[0] = sample_left;
      record_frame[1] = sample_right;
      for (int track = 1; track <= RECORD_TRACK_MAX; track++) {
        int channel = track * AUDIO_CHANNELS;
        float track_gain = synth_track_volume_linear_get(track);
        record_frame[channel] = record_left[track] * track_gain;
        record_frame[channel + 1] = record_right[track] * track_gain;
      }
    }

    // Mirror the record/scope bus onto multichannel devices when available.
    float *output_frame = buffer + ((size_t)i * num_channels);
    for (int channel = 0; channel < num_channels; channel++) {
      output_frame[channel] = 0.0f;
    }
    if (num_channels > 0) output_frame[0] = sample_left;
    if (num_channels > 1) output_frame[1] = sample_right;
    for (int track = 1; track <= RECORD_TRACK_MAX; track++) {
      int channel = track * AUDIO_CHANNELS;
      if (channel + 1 >= num_channels) break;
      float track_gain = synth_track_volume_linear_get(track);
      output_frame[channel] = record_left[track] * track_gain;
      output_frame[channel + 1] = record_right[track] * track_gain;
    }
  }
}

int envelope_is_flat(int v) {
  if (sv.amp_envelope[v].a == 0.0f &&
    sv.amp_envelope[v].d == 0.0f &&
    sv.amp_envelope[v].s == 1.0f &&
    sv.amp_envelope[v].r == 0.0f) return 1;
  return 0;
}

int cz_set(int v, int n, float f) {
  sv.cz_mode[v] = n;
  sv.cz_distortion[v] = f;
  return 0;
}

int cmod_set(int voice, int o, float f) {
  if (voice_invalid(voice) || mod_voice_invalid(o)) return SYNTH_INVALID_VOICE;
  sv.cz_mod_osc[voice] = o;
  sv.cz_mod_depth[voice] = f;
  return 0;
}

#include <stdio.h>

// maybe these should be in wire.[ch]?

static int voice_invalid(int voice) {
  if (voice < 0 || voice >= synth_config.voice_max) return 1;
  return 0;
}


static void freq_to_note_cents(float freq, float *note, float *cents) {
  double n_float = 12.0 * log2(freq / 440.0) + 69.0;
  int n_int = (int)round(n_float);
  *note = n_int;
  *cents = 100.0 * (n_float - n_int);
}

#define D_TO_S_MAX (5)
static char _d_to_s[D_TO_S_MAX][16] = {0};
int d_to_s_idx = 0;
static char *d_to_s_or_nan(int n) {
  int r = d_to_s_idx;
  if (n >= 0) sprintf(_d_to_s[d_to_s_idx++], "%d", n); else strcpy(_d_to_s[d_to_s_idx++], "-");
  if (d_to_s_idx >= D_TO_S_MAX) d_to_s_idx = 0;
  return _d_to_s[r];
}

char *voice_format(int v, char *out, size_t out_size, int verbose) {
    if (out == NULL) return "(NULL)";
    if (out_size == 0) return out;
    if (voice_invalid(v)) {
        out[0] = '\0';
        return out;
    }

    char *ptr = out;
    size_t remaining = out_size;

#define APPEND(...) do { \
    int _n = snprintf(ptr, remaining, __VA_ARGS__); \
    if (_n > 0 && (size_t)_n < remaining) { ptr += _n; remaining -= _n; } \
    else { ptr += remaining - 1; *ptr = '\0'; remaining = 0; return out; } \
} while (0)

    /* --- always: voice number, waveform, frequency, amplitude --- */
    float note, cents;
    freq_to_note_cents(sv.freq[v], &note, &cents);
    if (note < 0.0001) note = 0;
    if (cents < 0.0001) cents = 0;
    APPEND("v%d w%d f%g n%g,%g a%g",
      v,
      sv.wave_table_index[v],
      sv.freq[v],
      note,
      cents,
      sv.user_amp[v]);
    if (sv.control_events[v]) APPEND(" vc1");

    /* --- last midi note (suppress if never set) --- */
    #if 0
    if (verbose || sv.last_midi_note[v] > 0)
        APPEND(" n%g", sv.last_midi_note[v]);
    #endif

    /* --- note detune (suppress if both zero) --- */
    if (verbose || sv.midi_transpose[v] != 0.0f || sv.midi_cents[v] != 0.0f)
        APPEND(" N%g,%g", sv.midi_transpose[v], sv.midi_cents[v]);

    /* --- midi note forward (suppress if all unset) --- */
    if (verbose
        || (int)sv.link_midi_0[v] >= 0
        || (int)sv.link_midi_1[v] >= 0
        || (int)sv.link_midi_2[v] >= 0
        || (int)sv.link_midi_3[v] >= 0) {
          APPEND(" G%s,%s,%s,%s",
            d_to_s_or_nan(sv.link_midi_0[v]),
            d_to_s_or_nan(sv.link_midi_1[v]),
            d_to_s_or_nan(sv.link_midi_2[v]),
            d_to_s_or_nan(sv.link_midi_3[v]));
        }

    if (verbose
        || (int)sv.link_velo_0[v] >= 0
        || (int)sv.link_velo_1[v] >= 0
        || (int)sv.link_velo_2[v] >= 0
        || (int)sv.link_velo_3[v] >= 0) {
          APPEND(" H%s,%s,%s,%s",
            d_to_s_or_nan(sv.link_velo_0[v]),
            d_to_s_or_nan(sv.link_velo_1[v]),
            d_to_s_or_nan(sv.link_velo_2[v]),
            d_to_s_or_nan(sv.link_velo_3[v]));
        }
    /* --- trigger link (suppress if unset) --- */
    if (verbose || (int)sv.link_trig[v] >= 0)
        APPEND(" L%g", sv.link_trig[v]);

    /* --- playback direction (suppress b0 default) --- */
    if (verbose || sv.direction[v])
        APPEND(" b%d", sv.direction[v]);

    /* --- looping (suppress B0 default) --- */
    if (verbose || sv.loop_enabled[v])
        APPEND(" B%d", sv.loop_enabled[v]);
    if (verbose || sv.loop_count[v])
        APPEND(" BC%d", sv.loop_count[v]);

    /* --- pan (suppress if centre) --- */
    if (verbose || sv.pan[v] != 0.0f)
        APPEND(" p%g", sv.pan[v]);

    if (verbose || (sv.pan_mod_osc[v] >= 0 && sv.pan_mod_depth[v] != 0.0f))
        APPEND(" P%d,%g,%g", sv.pan_mod_osc[v], sv.pan_mod_depth[v], sv.pan_mod_adder[v]);

    if (verbose || sv.delay_send[v] > 0.0f)
        APPEND(" ds%g", sv.delay_send[v]);

    if (verbose || sv.filter_mode[v])
        APPEND(" J%d K%g Q%g", sv.filter_mode[v], sv.filter_freq[v], sv.filter_res[v]);

    if (verbose || sv.use_filter_envelope[v])
        APPEND(" ft %g %g %g %g fd %g",
            sv.filter_envelope[v].a,
            sv.filter_envelope[v].d,
            sv.filter_envelope[v].s,
            sv.filter_envelope[v].r,
            sv.filter_env_depth[v]);

    /* --- phase distortion (suppress if mode 0) --- */
    if (verbose || sv.cz_mode[v])
        APPEND(" c%d,%g", sv.cz_mode[v], sv.cz_distortion[v]);

    if (verbose || (sv.cz_mod_osc[v] >= 0 && sv.cz_mod_depth[v] != 0.0f))
        APPEND(" C%d,%g", sv.cz_mod_osc[v], sv.cz_mod_depth[v]);

    if (verbose || sv.sample_hold_max[v]) APPEND(" h%d", sv.sample_hold_max[v]);

    if (verbose || sv.quantize[v]) APPEND(" q%d", sv.quantize[v]);

    if (verbose || (sv.amp_mod_osc[v] >= 0 && sv.amp_mod_depth[v] != 0.0f))
        APPEND(" A%d,%g,%g", sv.amp_mod_osc[v], sv.amp_mod_depth[v], sv.amp_mod_adder[v]);

    if (verbose || (sv.freq_mod_osc[v] >= 0 && sv.freq_mod_depth[v] != 0.0f)) {
        if (sv.freq_mod_mode[v] == 1)
            APPEND(" FF1");
        APPEND(" F%d,%g,%g", sv.freq_mod_osc[v], sv.freq_mod_depth[v], sv.freq_mod_adder[v]);
    }

    /* --- mix / record flags (suppress if default) --- */
    if (verbose || sv.disconnect[v])
        APPEND(" m%d", sv.disconnect[v]);
    int record_track = synth_record_track_get(v);
    if (verbose || record_track)
        APPEND(" r%d", record_track);

    if (verbose || (sv.smoother_enable[v] && sv.smoother_smoothing[v] != SMOOTH_DEFAULT))
        APPEND(" s%g", sv.smoother_smoothing[v]);

    // Show the time (e.g., g0.05), not the multiplier (e.g., g1.00014)
    if (verbose || sv.glissando_time[v] > 0.0f) 
        APPEND(" g%g", sv.glissando_time[v]);

    if (verbose || !envelope_is_flat(v))
        APPEND(" t%g,%g,%g,%g k%d",
            sv.amp_envelope[v].a,
            sv.amp_envelope[v].d,
            sv.amp_envelope[v].s,
            sv.amp_envelope[v].r,
            sv.amp_envelope_mode[v]);

    if (sv.text[v][0] != '\0') APPEND(" [%s] vt", sv.text[v]);

    /* ----------------------------------------------------------------
     * Verbose-only: internal engine state after a # comment marker.
     * The user can read these values but they are not skode commands.
     * ---------------------------------------------------------------- */
    if (verbose) {
        APPEND("\n#");
        APPEND(" user_amp:%g amp:%g", sv.user_amp[v], sv.amp[v]);
        APPEND(" freq_scale:%g", sv.freq_scale[v]);
        APPEND(" offset_hz:%g", sv.offset_hz[v]);
        APPEND(" phase:%g phase_inc:%g", sv.phase[v], sv.phase_inc[v]);
        APPEND(" sample:%g", sv.sample[v]);
        APPEND(" finished:%d one_shot:%d", sv.finished[v], sv.one_shot[v]);
        APPEND(" loop_active:%d loop_bounded:%d loop_remaining:%d loop_stop:%d",
            sv.loop_active[v], sv.loop_bounded[v], sv.loop_remaining[v],
            sv.loop_stop_requested[v]);
        APPEND(" smoother_gain:%g", sv.smoother_gain[v]);
        APPEND(" filter_update_counter:%d", sv.filter_update_counter[v]);
        APPEND(" filter_env_active:%d", sv.filter_envelope[v].is_active);
        APPEND(" filter_env_runtime:%g,%g,%g,%g",
            sv.filter_envelope[v].attack_time,
            sv.filter_envelope[v].decay_time,
            sv.filter_envelope[v].sustain_level,
            sv.filter_envelope[v].release_time);
        APPEND(" filter_env_release:%llu",
            (unsigned long long)sv.filter_envelope[v].sample_release);
        APPEND(" amp_env_active:%d", sv.amp_envelope[v].is_active);
        APPEND(" amp_env_runtime:%g,%g,%g,%g",
            sv.amp_envelope[v].attack_time,
            sv.amp_envelope[v].decay_time,
            sv.amp_envelope[v].sustain_level,
            sv.amp_envelope[v].release_time);
        APPEND(" amp_env_release:%llu",
            (unsigned long long)sv.amp_envelope[v].sample_release);
    }

#undef APPEND

    return out;
}


int amp_set(int voice, float f) {
  if (voice_invalid(voice) || !isfinite(f)) return SYNTH_INVALID_VOICE;
  sv.user_amp[voice] = f;
  sv.amp[voice] = DB_TO_LINEAR(f);
  return 0;
}

int pan_set(int voice, float f) {
  if (voice_invalid(voice) || !isfinite(f)) return SYNTH_INVALID_VOICE;
  if (f >= -1.0f && f <= 1.0f) {
    sv.pan[voice] = f;
    sv.pan_left[voice] = (1.0f - f) / 2.0f;
    sv.pan_right[voice] = (1.0f + f) / 2.0f;
  } else {
    return 100; // <--- LAZY! needs ERR_PAN_OUT_OF_RANGE;
  }
  return 0;
}

int wave_quant(int voice, int n) {
  sv.quantize[voice] = n;
  return 0;
}

int freq_set(int voice, float f) {
  if (voice_invalid(voice) || !isfinite(f)) return SYNTH_INVALID_VOICE;
  if (f < 0 || f >= (double)MAIN_SAMPLE_RATE) return 101;

  float target_inc = osc_get_phase_inc(voice, f);

  float glide_time = sv.glissando_time[voice];

  // Safety: Only glide if time is set and current pitch is above a 'floor' (e.g., 20Hz)
  // This prevents the 'ratio explosion' when starting from zero.
  if (glide_time > 0.0f && sv.phase_inc[voice] > 0.001f) {
    sv.glissando_target[voice] = target_inc;
    float frames = glide_time * MAIN_SAMPLE_RATE;

    // The multiplier 'm' that reaches target in N frames: start * m^N = target
    sv.glissando_speed[voice] = powf(target_inc / sv.phase_inc[voice], 1.0f / frames);
    sv.glissando_enable[voice] = 1;
  } else {
    // Snap immediately if no glide time or starting from silence
    sv.phase_inc[voice] = target_inc;
    sv.glissando_enable[voice] = 0;
  }

  sv.freq[voice] = f;
  return 0;
}

int wave_mute(int voice, int state) {
  if (voice_invalid(voice)) return SYNTH_INVALID_VOICE;
  if (state < 0) {
    if (sv.disconnect[voice] == 0) state = 1;
    else state = 0;
  }
  sv.disconnect[voice] = state;
  return 0;
}

int wave_dir(int voice, int state) {
  if (voice_invalid(voice)) return SYNTH_INVALID_VOICE;
  if (state < 0) {
    if (sv.direction[voice] == 0) state = 1;
    else state = 0;
  }
  sv.direction[voice] = state;
  return 0;
}

int pan_mod_set(int voice, int o, float f, float a) {
  if (voice_invalid(voice) || mod_voice_invalid(o)) return SYNTH_INVALID_VOICE;
  sv.pan_mod_osc[voice] = o;
  sv.pan_mod_depth[voice] = f;
  sv.pan_mod_adder[voice] = a;
  return 0;
}

int wave_set(int voice, int wave) {
  if (voice_invalid(voice)) return SYNTH_INVALID_VOICE;
  if (wave >= 0 && wave < WAVE_TABLE_MAX) {
    osc_set_wave_table_index(voice, wave);
  } else return 100; // <-- more LAZY!!! ERR_INVALID_WAVE;
  return 0;
}

int amp_mod_set(int voice, int o, float f, float a) {
  if (voice_invalid(voice) || mod_voice_invalid(o)) return SYNTH_INVALID_VOICE;
  sv.amp_mod_osc[voice] = o;
  sv.amp_mod_depth[voice] = f;
  sv.amp_mod_adder[voice] = a;
  return 0;
}

int freq_mod_set(int voice, int o, float f, float a) {
  if (voice_invalid(voice) || mod_voice_invalid(o)) return SYNTH_INVALID_VOICE;
  sv.freq_mod_osc[voice] = o;
  sv.freq_mod_depth[voice] = f;
  sv.freq_mod_adder[voice] = a;
  if (o >= 0 && sv.table_size[o] > 0)
    sv.freq_scale[voice] = (float)sv.table_size[voice] / (float)sv.table_size[o];
  else
    sv.freq_scale[voice] = 1.0f;
  return 0;
}

int wave_loop(int voice, int state) {
  if (voice_invalid(voice)) return SYNTH_INVALID_VOICE;
  if (state < 0) {
    if (sv.loop_enabled[voice] == 0) state = 1;
    else state = 0;
  }
  sv.loop_enabled[voice] = state;
  sv.loop_active[voice] = state;
  sv.loop_stop_requested[voice] = 0;
  sv.loop_ended[voice] = 0;
  if (state) {
    sv.loop_bounded[voice] = sv.loop_count[voice] > 0;
    sv.loop_remaining[voice] = sv.loop_count[voice];
  } else {
    sv.loop_bounded[voice] = 0;
    sv.loop_remaining[voice] = 0;
  }
  return 0;
}

int wave_loop_count(int voice, int count) {
  if (voice_invalid(voice) || count < 0) return SYNTH_INVALID_VOICE;
  sv.loop_count[voice] = count;
  sv.loop_enabled[voice] = 1;
  return 0;
}


int envelope_set(int voice, float a, float d, float s, float r) {
  envelope_configure_e(&sv.amp_envelope[voice], a, d, s, r);
  return 0;
}

// Set parameters - only recalculates coefficients if values changed
void mmf_set_params(int n, float f, float resonance) {
    // Only recalculate if parameters changed
    if (
      f == sv.filter[n].last_freq &&
      resonance == sv.filter[n].last_resonance &&
      sv.filter_mode[n] == sv.filter[n].last_mode) {
        return;  // No work needed!
    }

    sv.filter[n].last_freq = f;
    sv.filter[n].last_resonance = resonance;
    sv.filter[n].last_mode = sv.filter_mode[n];

    // Calculate filter coefficients (expensive operations only done here)
    float omega = 2.0f * (float)M_PI * f / (float)MAIN_SAMPLE_RATE;
    float sin_omega = sinf(omega);
    float cos_omega = cosf(omega);
    float alpha = sin_omega / (2.0f * resonance);

    float a0, b0, b1, b2, a1, a2;

    switch (sv.filter_mode[n]) {
      case 0:
        return;
      default:
      case FILTER_LOWPASS:
          b0 = (1.0f - cos_omega) / 2.0f;
          b1 = 1.0f - cos_omega;
          b2 = (1.0f - cos_omega) / 2.0f;
          a0 = 1.0f + alpha;
          a1 = -2.0f * cos_omega;
          a2 = 1.0f - alpha;
          break;
      case FILTER_HIGHPASS:
          b0 = (1.0f + cos_omega) / 2.0f;
          b1 = -(1.0f + cos_omega);
          b2 = (1.0f + cos_omega) / 2.0f;
          a0 = 1.0f + alpha;
          a1 = -2.0f * cos_omega;
          a2 = 1.0f - alpha;
          break;

      case FILTER_BANDPASS:
          b0 = alpha;
          b1 = 0.0f;
          b2 = -alpha;
          a0 = 1.0f + alpha;
          a1 = -2.0f * cos_omega;
          a2 = 1.0f - alpha;
          break;

      case FILTER_NOTCH:
          b0 = 1.0f;
          b1 = -2.0f * cos_omega;
          b2 = 1.0f;
          a0 = 1.0f + alpha;
          a1 = -2.0f * cos_omega;
          a2 = 1.0f - alpha;
          break;

      case FILTER_ALL_PASS:
          b0 = 1.0f - alpha;
          b1 = -2.0f * cos_omega;
          b2 = 1.0f + alpha;
          a0 = 1.0f + alpha;
          a1 = -2.0f * cos_omega;
          a2 = 1.0f - alpha;
          break;
    }

    // Normalize coefficients
    sv.filter[n].b0 = b0 / a0;
    sv.filter[n].b1 = b1 / a0;
    sv.filter[n].b2 = b2 / a0;
    sv.filter[n].a1 = a1 / a0;
    sv.filter[n].a2 = a2 / a0;
}


// Initialize the filter with frequency and resonance
// freq: cutoff frequency in Hz
// resonance: resonance factor (0.1 to 10.0, where 0.707 is no resonance)
// sample_rate: audio sample rate in Hz
void mmf_init(int n, float f, float resonance) {
    // Clear delay lines
    sv.filter[n].x1 = sv.filter[n].x2 = 0.0f;
    sv.filter[n].y1 = sv.filter[n].y2 = 0.0f;

    // Store parameters
    sv.filter[n].last_freq = -1.0f;  // Force coefficient calculation
    sv.filter[n].last_resonance = -1.0f;
    sv.filter[n].last_mode = -1;

    sv.filter_freq[n] = f;
    sv.filter_res[n] = resonance;

    // Calculate initial coefficients
    mmf_set_params(n, f, resonance);
}

int voice_control_events_set(int voice, int enabled);

int voice_copy(int v, int n) {
  if (voice_invalid(v) || voice_invalid(n)) return SYNTH_INVALID_VOICE;
  wave_set(n, sv.wave_table_index[v]);
  amp_set(n, sv.user_amp[v]);
  freq_set(n, sv.freq[v]);
  voice_control_events_set(n, sv.control_events[v]);
  sv.loop_count[n] = sv.loop_count[v];
  wave_loop(n, sv.loop_enabled[v]);
  wave_dir(n, sv.direction[v]);
  sv.link_midi_0[n] = sv.link_midi_0[v];
  sv.link_midi_1[n] = sv.link_midi_1[v];
  sv.link_midi_2[n] = sv.link_midi_2[v];
  sv.link_midi_3[n] = sv.link_midi_3[v];
  sv.midi_transpose[n] = sv.midi_transpose[v];
  sv.midi_cents[n] = sv.midi_cents[v];
  envelope_set(n, sv.amp_envelope[v].a, sv.amp_envelope[v].d, sv.amp_envelope[v].s, sv.amp_envelope[v].r);
  sv.link_velo_0[n] = sv.link_velo_0[v];
  sv.link_velo_1[n] = sv.link_velo_1[v];
  sv.link_velo_2[n] = sv.link_velo_2[v];
  sv.link_velo_3[n] = sv.link_velo_3[v];
  sv.link_trig[n] = sv.link_trig[v];
  sv.link_trig_samp[n] = sv.link_trig_samp[v];
  //
  pan_set(n, sv.pan[v]);
  delay_send_set(n, sv.delay_send[v]);
  amp_mod_set(n, sv.amp_mod_osc[v], sv.amp_mod_depth[v], sv.amp_mod_adder[v]);
  freq_mod_set(n, sv.freq_mod_osc[v], sv.freq_mod_depth[v], sv.freq_mod_adder[v]);
  pan_mod_set(n, sv.pan_mod_osc[v], sv.pan_mod_depth[v], sv.pan_mod_adder[v]);
  wave_quant(n, sv.quantize[v]);
  sv.sample_hold_max[n] = sv.sample_hold_max[v];
  sv.sample_hold_count[n] = sv.sample_hold_count[v];
  sv.sample_hold[n] = sv.sample_hold[v];
  cz_set(n, sv.cz_mode[v], sv.cz_distortion[v]);
  cmod_set(n, sv.cz_mod_osc[v], sv.cz_mod_depth[v]);
  sv.filter_mode[n] = sv.filter_mode[v];
  mmf_init(n, sv.filter_freq[v], sv.filter_res[v]);
  sv.phase_inc[n] = sv.phase_inc[v];
  sv.glissando_enable[n] = sv.glissando_enable[v];
  sv.glissando_speed[n] = sv.glissando_speed[v];
  sv.glissando_target[n] = sv.glissando_target[v];
  sv.glissando_time[n] = sv.glissando_time[v];
  sv.use_filter_envelope[n] = sv.use_filter_envelope[v];
  sv.filter_env_depth[n] = sv.filter_env_depth[v];
  float a = sv.filter_envelope[v].a;
  float d = sv.filter_envelope[v].d;
  float s = sv.filter_envelope[v].s;
  float r = sv.filter_envelope[v].r;
  envelope_init_e(&sv.filter_envelope[n], a, d, s, r);
  //
  // TODO stuff is missing from here...
  //
  return 0;
}

double midi2hz(float midi_note, double cents) {
    // 440Hz is the standard reference for MIDI note 69 (A4)
    const double reference_pitch = 440.0;
    const float reference_note = 69;

    // We add the cents divided by 100 to the note number 
    // to get a "fractional" MIDI note.
    double fractional_note = (double)midi_note + (cents / 100.0);

    // Calculate frequency: f = 440 * 2^((n - 69) / 12)
    return reference_pitch * pow(2.0, (fractional_note - reference_note) / 12.0);
}

int voice_set(int n, int *old_voice) {
  if (voice_invalid(n)) return SYNTH_INVALID_VOICE;
  if (old_voice) *old_voice = n;
  return 0;
}

int voice_control_events_set(int voice, int enabled) {
  if (voice_invalid(voice)) return SYNTH_INVALID_VOICE;
  sv.control_events[voice] = enabled ? 1 : 0;
  if (!sv.control_events[voice]) skred_control_voice_reset(voice);
  return 0;
}

int voice_trigger(int voice) {
  if (voice_invalid(voice)) return SYNTH_INVALID_VOICE;
  osc_trigger(voice);
  skred_control_voice_event(SKRED_CONTROL_EVENT_VOICE_TRIGGER,
    SAMPLE_COUNT_GET(), voice);
  return 0;
}

int wave_default(int voice) {
  if (voice_invalid(voice)) return SYNTH_INVALID_VOICE;
  float g = midi2hz((float)sv.midi_note[voice], 0);
  sv.freq[voice] = g;
  sv.note[voice] = (float)sv.midi_note[voice];
  osc_set_freq(voice, g);
  return 0;
}

int freq_midi(int voice, float note, float cents) {
  if (voice_invalid(voice) || !isfinite(note) || !isfinite(cents))
    return SYNTH_INVALID_VOICE;
  if (note >= 0.0 && note <= 127.0) {
    sv.last_midi_note[voice] = note;
    if (sv.midi_transpose[voice]) note += sv.midi_transpose[voice];
    float g = midi2hz(note, sv.midi_cents[voice] + cents);
    return freq_set(voice, g);
  }
  return 100; // <-- LAZY  ERR_INVALID_MIDI_NOTE;
}

int envelope_velocity(int voice, float f);

void voice_reset(int i) {
  sv.wave_table_index[i] = -1;
  sv.table_rate[i] = 0;
  sv.table_size[i] = 0;
  sv.sample[i] = 0;
  sv.amp[i] = NEG_60_DB_AS_LINEAR;
  sv.user_amp[i] = NEG_60_DB;
  sv.use_amp_envelope[i] = 1;
  voice_control_events_set(i, 0);
  sv.disconnect[i] = 0;
  sv.direction[i] = 0;
  sv.loop_enabled[i] = 0;
  sv.loop_count[i] = 0;
  sv.loop_bounded[i] = 0;
  sv.loop_remaining[i] = 0;
  sv.loop_active[i] = 0;
  sv.loop_stop_requested[i] = 0;
  sv.loop_ended[i] = 0;
  sv.amp_envelope_mode[i] = 0; // exp
  sv.amp_envelope[i].is_active = 0;
  envelope_init(i, 0.0f, 0.0f, 1.0f, 0.0f);
  sv.freq[i] = 440.0f;
  sv.midi_note[i] = 69.0f;
  sv.last_midi_note[i] = 69.0f;
  sv.midi_transpose[i] = 0;
  sv.midi_cents[i] = 0;
  sv.link_midi_0[i] = -1;
  sv.link_midi_1[i] = -1;
  sv.link_midi_2[i] = -1;
  sv.link_midi_3[i] = -1;
  sv.link_velo_0[i] = -1;
  sv.link_velo_1[i] = -1;
  sv.link_velo_2[i] = -1;
  sv.link_velo_3[i] = -1;
  sv.link_trig[i] = -1;
  sv.link_trig_samp[i] = 0;
  osc_set_wave_table_index(i, WAVE_TABLE_SINE);
  //
  sv.pan[i] = 0;
  sv.pan_left[i] = 0.5f;
  sv.pan_right[i] = 0.5f;
  sv.delay_send[i] = 0.0f;
  // pan smoothing?
  sv.amp_mod_osc[i] = -1;
  sv.amp_mod_depth[i] = 0.0f;
  sv.amp_mod_adder[i] = 0.0f;
  sv.freq_mod_osc[i] = -1;
  sv.freq_mod_depth[i] = 0.0f;
  sv.freq_mod_adder[i] = 0.0f;
  sv.freq_mod_mode[i] = 0;
  sv.freq_scale[i] = 1.0f;
  sv.pan_mod_osc[i] = -1;
  sv.pan_mod_depth[i] = 0.0f;
  sv.pan_mod_adder[i] = 0.0f;
  sv.quantize[i] = 0;
  sv.filter_mode[i] = 0;
  sv.filter_update_counter[i] = FILTER_UC;
  mmf_init(i, 8000.0f, 0.707f);
  sv.use_filter_envelope[i]   = 0;
  sv.filter_env_depth[i]      = 0.0f;
  envelope_init_e(&sv.filter_envelope[i], 0.0f, 0.0f, 1.0f, 0.0f);
  //
  sv.smoother_enable[i] = 1;
#if 0
  sv.smoother_gain[i] = 0.0f;
#else
  sv.smoother_gain[i] = sv.amp[i];
#endif
  sv.smoother_smoothing[i] = SMOOTH_DEFAULT;
  //
  sv.phase_inc[i] = 1e-9; // ??here??
  sv.glissando_enable[i] = 0;
  sv.glissando_speed[i] = 1.0f;
  sv.glissando_target[i] = 0.0f; // sv.freq[i]; // maybe 0.0f???
  sv.record[i] = 0;
  atomic_store_int(&sv.record_pending[i], 0);
  sv.text[i][0] = '\0';
}

void voice_init(void) {
  for (int i = 0; i < synth_config.voice_max; i++) {
    voice_reset(i);
  }
}


int wave_reset(int voice) {
  if (voice_invalid(voice)) voice_init();
  else voice_reset(voice);
  return 0;
}

int envelope_velocity(int voice, float f) {
    if (voice_invalid(voice)) return SYNTH_INVALID_VOICE;
    if (f == 0) {
        if (sv.one_shot[voice] && sv.loop_active[voice] && sv.loop_bounded[voice])
            sv.loop_stop_requested[voice] = 1;
        skred_control_voice_event(SKRED_CONTROL_EVENT_VOICE_RELEASE,
          SAMPLE_COUNT_GET(), voice);
        amp_envelope_release(voice);
        if (sv.filter_envelope[voice].is_active)
            envelope_release_e(&sv.filter_envelope[voice]);
    } else {
        sv.use_amp_envelope[voice] = 1;
        if (sv.one_shot[voice]) {
            osc_trigger(voice);
        }
        skred_control_voice_event(SKRED_CONTROL_EVENT_VOICE_TRIGGER,
          SAMPLE_COUNT_GET(), voice);
        amp_envelope_trigger(voice, f);
        amp_envelope_schedule_one_shot_release(voice);
        if (sv.use_filter_envelope[voice])
            envelope_trigger_e(&sv.filter_envelope[voice], f);
    }
    return 0;
}

int mmf_set_freq(int n, float f) {
  sv.filter_freq[n] = f;
  mmf_set_params(n, f, sv.filter_res[n]);
  return 0;
}

int mmf_set_res(int n, float res) {
  if (res > 0) {
    sv.filter_res[n] = res;
    mmf_set_params(n, sv.filter_freq[n], res);
  }
  return 0;
}

void normalize_preserve_zero(float *data, int length) {
  if (length == 0) return;

  // Find the maximum absolute value
  float max_abs = 0.0f;
  for (int i = 0; i < length; i++) {
    float abs_val = fabsf(data[i]);
    if (abs_val > max_abs) {
      max_abs = abs_val;
    }
  }

  // Avoid division by zero
  if (max_abs == 0.0) {
    return;  // All values are zero, nothing to normalize
  }

  // Scale all values by the same factor
  float scale_factor = 1.0f / max_abs;
  for (int i = 0; i < length; i++) {
    data[i] *= scale_factor;
  }
}

#define SIZE_SINE (4096)

#include "dwg.h"

void wave_table_init(int flag) {
  (void)flag;

  for (int i = 0 ; i < WAVE_TABLE_MAX; i++) {
    sw.data[i] = NULL;
    sw.size[i] = 0;
    sw.is_heap[i] = 0;
    sw.direction[i] = 0;
    sw.readonly[i] = 0;
    sw.refcount[i] = 0;
  }

  uint64_t white_noise;
  audio_rng_init(&white_noise, 1);
  for (int w = WAVE_TABLE_SINE; w <= WAVE_TABLE_KRG16; w++) {
    int size = SIZE_SINE;
    char *name = "?";
    switch (w) {
      case WAVE_TABLE_SINE:  name = "sine"; break;
      case WAVE_TABLE_SQR:   name = "square"; break;
      case WAVE_TABLE_SAW_DOWN: name = "saw-down"; break;
      case WAVE_TABLE_SAW_UP: name = "saw-up"; break;
      case WAVE_TABLE_TRI:   name = "triangle"; break;
      case WAVE_TABLE_NOISE: name = "noise"; break;
      case WAVE_TABLE_NOISE_ALT: name = "noise-alt"; break; // not used, here for laziness in experiment
      case WAVE_TABLE_CAP_LEFT:  name = "cap-left"; break; // not used, here for laziness in experiment
      case WAVE_TABLE_CAP_RIGHT: name = "cap-right"; break; // not used, here for laziness in experiment
      case WAVE_TABLE_CAP_BOTH:  name = "cap-both"; break; // not used, here for laziness in experiment
      case WAVE_TABLE_KRG1:  name = "dwg-strings"; break;
      case WAVE_TABLE_KRG2:  name = "dwg-clarinet"; break;
      case WAVE_TABLE_KRG3:  name = "dwg-apiano"; break;
      case WAVE_TABLE_KRG4:  name = "dwg-epiano"; break;
      case WAVE_TABLE_KRG5:  name = "dwg-epiano-hard"; break;
      case WAVE_TABLE_KRG6:  name = "dwg-clavi"; break;
      case WAVE_TABLE_KRG7:  name = "dwg-organ"; break;
      case WAVE_TABLE_KRG8:  name = "dwg-brass"; break;
      case WAVE_TABLE_KRG9:  name = "dwg-sax"; break;
      case WAVE_TABLE_KRG10: name = "dwg-violin"; break;
      case WAVE_TABLE_KRG11: name = "dwg-aguitar"; break;
      case WAVE_TABLE_KRG12: name = "dwg-dguitar"; break;
      case WAVE_TABLE_KRG13: name = "dwg-ebass"; break;
      case WAVE_TABLE_KRG14: name = "dwg-dbass"; break;
      case WAVE_TABLE_KRG15: name = "dwg-bell"; break;
      case WAVE_TABLE_KRG16: name = "dwg-whistle"; break;
      default: name = "?"; break;
    }
    strncpy(sw.name[w], name, WAVE_NAME_MAX);
    sw.data[w] = (float *)malloc(size * sizeof(float));
    if (!sw.data[w]) {
      sw.size[w] = 0;
      sw.is_heap[w] = 0;
      continue;
    }
    sw.is_heap[w] = 1;
    sw.size[w] = size;
    sw.rate[w] = MAIN_SAMPLE_RATE;
    sw.one_shot[w] = 0;
    sw.loop_start[w] = 0;
    sw.loop_end[w] = size-1;
    sw.readonly[w] = 1;
    int off = 0;
    float phase = 0;
    float delta = 1.0f / (float)size;
    while (phase < 1.0f) {
      float sine = sinf(2.0f * (float) M_PI * phase);
      float f;
      switch (w) {
        case WAVE_TABLE_SINE: f = sine; break;
        case WAVE_TABLE_SQR: f = (phase < 0.5) ? 1.0f : -1.0f; break;
        case WAVE_TABLE_SAW_DOWN: f = 2.0f * phase - 1.0f; break;
        case WAVE_TABLE_SAW_UP: f = 1.0f - 2.0f * phase; break;
        case WAVE_TABLE_TRI: f = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase); break;
        case WAVE_TABLE_NOISE: f = audio_rng_float(&white_noise); break;
        case WAVE_TABLE_NOISE_ALT: f = audio_rng_float(&white_noise); break;
        case WAVE_TABLE_KRG1:  f = W01[off]; break;
        case WAVE_TABLE_KRG2:  f = W02[off]; break;
        case WAVE_TABLE_KRG3:  f = W03[off]; break;
        case WAVE_TABLE_KRG4:  f = W04[off]; break;
        case WAVE_TABLE_KRG5:  f = W05[off]; break;
        case WAVE_TABLE_KRG6:  f = W06[off]; break;
        case WAVE_TABLE_KRG7:  f = W07[off]; break;
        case WAVE_TABLE_KRG8:  f = W08[off]; break;
        case WAVE_TABLE_KRG9:  f = W09[off]; break;
        case WAVE_TABLE_KRG10: f = W10[off]; break;
        case WAVE_TABLE_KRG11: f = W11[off]; break;
        case WAVE_TABLE_KRG12: f = W12[off]; break;
        case WAVE_TABLE_KRG13: f = W13[off]; break;
        case WAVE_TABLE_KRG14: f = W14[off]; break;
        case WAVE_TABLE_KRG15: f = W15[off]; break;
        case WAVE_TABLE_KRG16: f = W16[off]; break;
        default: f = 0; break;
      }
      sw.data[w][off++] = f;
      phase += delta;
    }
  }
}

void wave_free_one(int i) {
  if (sw.data[i]) {
    if (sw.is_heap[i]) {
      free(sw.data[i]);
    }
    sw.data[i] = NULL;
    sw.size[i] = 0;
    sw.refcount[i] = 0;
  }
}

void wave_free(void) {
  for (int i = 0; i < WAVE_TABLE_MAX; i++) wave_free_one(i);
}
