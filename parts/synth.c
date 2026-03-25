#include <float.h>
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

atomic_uint64_t synth_sample_count;

#define SAMPLE_COUNT_PUT(n) atomic_store_uint64(&synth_sample_count, n)
#define SAMPLE_COUNT_GET() atomic_load_uint64(&synth_sample_count)
#define SAMPLE_COUNT_ADD(n) atomic_fetch_add_uint64(&synth_sample_count, n)

#define VOLUME_DEFAULT (-20.0f)
#define DB_TO_LINEAR(v) powf(10.f, (v) / 20.0f)

int volume_set(float v);

void synth_init(int vc) {
  SAMPLE_COUNT_PUT(0);

  synth_config_set_voices(vc);
  printf("vc = %d\n", vc);

  printf("synth_config.voice_max = %d\n", synth_config.voice_max);
  printf("synth_config.wave_table_max = %d\n", synth_config.wave_table_max);

  /* If nobody called synth_config_set_voices() before us, fill in defaults.
   * voice_max == 0 means aligned_alloc(64, 0) which is UB / NULL on most
   * platforms — the root cause of the silent segfault on startup. */
  if (synth_config.voice_max == 0 || synth_config.wave_table_max == 0)
    synth_config_defaults();

  //synth_alloc_voices(synth_config.voice_max);
  synth_alloc_voices(vc);
  synth_config.voice_max = vc;
  synth_alloc_waves(synth_config.wave_table_max);

  printf("synth_config.voice_max = %d\n", synth_config.voice_max);
  printf("synth_config.wave_table_max = %d\n", synth_config.wave_table_max);

  volume_set(VOLUME_DEFAULT);
}


void synth_free(void) {
  synth_free_voices();
  synth_free_waves();
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

int volume_set(float v) {
  volume_user = v;
  volume_final = DB_TO_LINEAR(v);
  return 0;
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


float osc_next(int voice, float phase_inc) {
    if (sv.finished[voice]) return 0.0f;

    const int table_size = sv.table_size[voice];
    const bool one_shot = sv.one_shot[voice];
    const bool loop_enabled = sv.loop_enabled[voice];

    if (sv.direction[voice]) phase_inc = -phase_inc;

    float phase = sv.phase[voice] + phase_inc;

    if (!isfinite(phase)) {
        sv.phase[voice] = 0.0f;
        sv.finished[voice] = one_shot;
        return 0.0f;
    }

    // Get loop boundaries (precomputed if available)
    const float loop_start = loop_enabled && sv.loop_valid[voice] 
        ? sv.loop_start_f[voice] : 0.0f;
    const float loop_end = loop_enabled && sv.loop_valid[voice]
        ? sv.loop_end_f[voice] : (float)table_size;
    const float loop_length = loop_end - loop_start;

    // Wrap phase
    if (phase >= loop_end) {
        if (one_shot && !loop_enabled) {
            phase = loop_end - 1e-6f;
            sv.finished[voice] = 1;
        } else {
            phase = loop_start + fmodf(phase - loop_start, loop_length);
        }
    } else if (phase < loop_start) {
        if (one_shot && !loop_enabled) {
            phase = loop_start;
            sv.finished[voice] = 1;
        } else {
            phase = loop_end - fmodf(loop_start - phase, loop_length);
        }
    }

    sv.phase[voice] = phase;

    // Get sample

    float final_phase;

    final_phase = phase;

    int idx = (int)final_phase;

    if (idx >= table_size) idx = table_size - 1;
    if (idx < 0) idx = 0;

    // Check if interpolation is enabled for this voice
    if (sv.interpolate[voice]) {
        // Linear interpolation
        float frac = final_phase - (float)idx;

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
  if (old >= 0 && sw.readonly[old] == 0) sw.refcount[old]--;
  if (wave >= 0 && sw.readonly[wave] == 0) sw.refcount[wave]++;
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

    if (sv.one_shot[voice]) {
        if (sv.direction[voice]) {
            sv.phase[voice] = (float)(sv.table_size[voice] - 1);
        } else {
            sv.phase[voice] = 0.0f;
        }
    } else {
        // Preserve direction, but start at appropriate boundary
        if (sv.direction[voice]) {
            // Backward playback: start at loop end
            sv.phase[voice] = sv.loop_enabled[voice] 
                ? (float)sv.loop_end[voice] - 1e-6f  // or sv.loop_end_f[voice]
                : (float)(sv.table_size[voice] - 1);
        } else {
            // Forward playback: start at loop start
            sv.phase[voice] = sv.loop_enabled[voice] 
                ? (float)sv.loop_start[voice]  // or sv.loop_start_f[voice]
                : 0.0f;
        }
    }
}

float quantize_bits_int(float v, int bits) {
  int levels = (1 << bits) - 1;
  int iv = (int)(v * (float)levels + 0.5);
  return (float)iv * (1.0f / (float)levels);
}


// Initialize the envelope

void envelope_init_e(envelope_t *e, float a, float d, float s, float r) {
    e->a = a;
    e->d = d;
    e->s = s;
    e->r = r;
    e->attack_time          = a * MAIN_SAMPLE_RATE;
    e->decay_time           = d * MAIN_SAMPLE_RATE;
    e->sustain_level        = fmaxf(0, fminf(1.0f, s));
    e->release_time         = r * MAIN_SAMPLE_RATE;
    e->sample_start         = 0;
    e->sample_release       = 0;
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
    if (e->is_active)
        e->amplitude_at_trigger = e->current_amplitude;
    else
        e->amplitude_at_trigger = 0.0f;
    e->sample_start   = SAMPLE_COUNT_GET();
    e->sample_release = 0;
    e->velocity       = f;
    e->is_active      = 1;
}

void amp_envelope_trigger(int v, float f) {
    envelope_trigger_e(&sv.amp_envelope[v], f);
}

void envelope_release_e(envelope_t *e) {
    if (e->is_active && e->sample_release == 0) {
        e->sample_release       = SAMPLE_COUNT_GET();
        e->amplitude_at_release = e->current_amplitude;
    }
}

void amp_envelope_release(int v) {
    envelope_release_e(&sv.amp_envelope[v]);
}

float envelope_step_e(envelope_t *e) {
    if (!e->is_active) return 0.0f;

    float out = 0.0f;
    float samples_since_start = (float)(SAMPLE_COUNT_GET() - e->sample_start);

    if (samples_since_start < e->attack_time) {
        float attack_progress = samples_since_start / e->attack_time;
        float start_val = e->amplitude_at_trigger;
        float curved_progress = attack_progress * attack_progress;
        out = start_val + (curved_progress * (1.0f - start_val));
    }
    else if (samples_since_start < (e->attack_time + e->decay_time)) {
        float samples_in_decay = samples_since_start - e->attack_time;
        float decay_progress = samples_in_decay / e->decay_time;
        out = 1.0f - decay_progress * (1.0f - e->sustain_level);
    }
    else {
        if (e->sample_release == 0) {
            out = e->sustain_level;
        } else {
            float samples_since_release = (float)(SAMPLE_COUNT_GET() - e->sample_release);
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

float amp_envelope_step(int v) {
    return envelope_step_e(&sv.amp_envelope[v]);
}

#include "util.h"

static sben_t bench[BENLEN] = {};
static int benchp = 0;
static int64_t bencho = 0;
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

void synth_voice_bench(int voice) {
  sv.mark_b[voice].tv_sec = 0;
  sv.mark_b[voice].tv_nsec = 0;
  clock_gettime(VOICE_CLOCK, &sv.mark_a[voice]);
  sv.mark_go[voice] = 1;
}

void mmf_set_params(int n, float f, float resonance);


void synth(float *buffer, float *input, int num_frames, int num_channels, void *user) {
  const int nvoices = synth_voice_count();
#if 0
  const int nframes = num_frames;
  (void)nframes; /* available for future vectorised frame loop */
#endif
  static float *one_skred_frame;
  static uint64_t synth_random;
  static int first = 1;
  if (first) {
    printf("nvoices = %d\n", nvoices);
    synth_frames_per_callback = num_frames;
    audio_rng_init(&synth_random, 1);
    one_skred_frame = (float *)user;
    first = 0;
  }


  int skred_ptr = 0;
  SAMPLE_COUNT_ADD(num_frames); // should this be outside the loop? and add num_frames??
  for (int i = 0; i < num_frames; i++) {
    //SAMPLE_COUNT_ADD(1); // should this be outside the loop? and add num_frames??
    float sample_left = 0.0f;
    float sample_right = 0.0f;
    float f = 0.0f;
    float whiteish = audio_rng_float(&synth_random);
    for (int n = 0; n < nvoices; n++) {
      if (sv.finished[n]) {
        //sv.sample[n] = 0.0f; // remove to try the below
        // hold last value to modulator consumers see statle output after one-shot ends
        one_skred_frame[skred_ptr++] = 0.0f;
        one_skred_frame[skred_ptr++] = 0.0f;
        continue;
      }  
      if (sv.amp[n] < NEG_60_DB_AS_LINEAR) {
        sv.sample[n] = 0.0f;
        one_skred_frame[skred_ptr++] = 0.0f;
        one_skred_frame[skred_ptr++] = 0.0f;
        continue;
      }
      if (sv.wave_table_index[n] == WAVE_TABLE_NOISE_ALT) {
        // bypass lots of stuff if this voice uses random source...
        // reuse the one white noise source for each sample
        f = whiteish;
      } else {
        f = osc_next(n, sv.phase_inc[n]);
      }
      sv.sample[n] = f;



      // apply amp to sample
      float amp = sv.amp[n];
      float env = 1.0f;
      float mod = 1.0f;



      float final = amp * env * mod;


      sv.sample[n] *= final;

      if (sv.disconnect[n] == 0) {
        float left  = sv.sample[n];
        float right = sv.sample[n];
        // accumulate samples
        left  *= sv.pan_left[n];
        right *= sv.pan_right[n];
        sample_left  += left;
        sample_right += right;
        one_skred_frame[skred_ptr++] = 0.0f;
        one_skred_frame[skred_ptr++] = 0.0f;
      }
    }

    // Adjust to main volume: smooth it otherwise is sounds crummy with realtime changes
    volume_smoother_gain += volume_smoother_smoothing * (volume_final - volume_smoother_gain);
    float volume_adjusted = volume_smoother_gain;

    sample_left  *= volume_adjusted;
    sample_right *= volume_adjusted;

    // Write to all channels
    buffer[i * num_channels + 0] = sample_left;
    buffer[i * num_channels + 1] = sample_right;
  }
}



#include <stdio.h>

// maybe these should be in wire.[ch]?

static int voice_invalid(int voice) {
  if (voice < 0 || voice >= synth_config.voice_max) return 1;
  return 0;
}


#define SYNTH_INVALID_VOICE (100)

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
    APPEND("v%d w%d f%g a%g", v, sv.wave_table_index[v], sv.freq[v], sv.user_amp[v]);

    /* --- last midi note (suppress if never set) --- */
    if (verbose || sv.last_midi_note[v] > 0)
        APPEND(" n%g", sv.last_midi_note[v]);

    /* --- note detune (suppress if both zero) --- */
    if (verbose || sv.midi_transpose[v] != 0.0f || sv.midi_cents[v] != 0.0f)
        APPEND(" N%g,%g", sv.midi_transpose[v], sv.midi_cents[v]);

    /* --- midi note forward (suppress if all unset) --- */
    if (verbose
        || (int)sv.link_midi_a[v] >= 0
        || (int)sv.link_midi_b[v] >= 0
        || (int)sv.link_midi_c[v] >= 0
        || (int)sv.link_midi_d[v] >= 0)
        APPEND(" G%d,%d,%d,%d",
            (int)sv.link_midi_a[v], (int)sv.link_midi_b[v],
            (int)sv.link_midi_c[v], (int)sv.link_midi_d[v]);

    /* --- velocity trigger chain (suppress if all unset) --- */
    if (verbose
        || (int)sv.link_velo_a[v] >= 0
        || (int)sv.link_velo_b[v] >= 0
        || (int)sv.link_velo_c[v] >= 0
        || (int)sv.link_velo_d[v] >= 0)
        APPEND(" H%d,%d,%d,%d",
            (int)sv.link_velo_a[v], (int)sv.link_velo_b[v],
            (int)sv.link_velo_c[v], (int)sv.link_velo_d[v]);

    /* --- trigger link (suppress if unset) --- */
    if (verbose || (int)sv.link_trig[v] >= 0)
        APPEND(" L%d", (int)sv.link_trig[v]);

    /* --- playback direction (suppress b0 default) --- */
    if (verbose || sv.direction[v])
        APPEND(" b%d", sv.direction[v]);

    /* --- looping (suppress B0 default) --- */
    if (verbose || sv.loop_enabled[v])
        APPEND(" B%d", sv.loop_enabled[v]);

    /* --- pan (suppress if centre) --- */
    if (verbose || sv.pan[v] != 0.0f)
        APPEND(" p%g", sv.pan[v]);




    /* --- phase distortion (suppress if mode 0) --- */





    /* --- mix / record flags (suppress if default) --- */
    if (verbose || sv.disconnect[v])
        APPEND(" m%d", sv.disconnect[v]);
    if (verbose || sv.record[v])
        APPEND(" r%d", sv.record[v]);




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
        APPEND(" smoother_gain:%g", sv.smoother_gain[v]);
        APPEND(" amp_env_active:%d", sv.amp_envelope[v].is_active);
        APPEND(" latency:%.2fms",
            (double)ts_diff_ns(&sv.mark_a[v], &sv.mark_b[v]) / 1000000.0);
    }

#undef APPEND

    return out;
}


int amp_set(int voice, float f) {
  sv.user_amp[voice] = f;
  sv.amp[voice] = DB_TO_LINEAR(f);
  return 0;
}

int pan_set(int voice, float f) {
  if (f >= -1.0f && f <= 1.0f) {
    sv.pan[voice] = f;
    sv.pan_left[voice] = (1.0f - f) / 2.0f;
    sv.pan_right[voice] = (1.0f + f) / 2.0f;
  } else {
    return 100; // <--- LAZY! needs ERR_PAN_OUT_OF_RANGE;
  }
  return 0;
}


int freq_set(int voice, float f) {
  if (f >= 0 && f < (double)MAIN_SAMPLE_RATE) {
    sv.freq[voice] = f;
    osc_set_freq(voice, f);
    return 0;
  }
  return 101; // <--- LAZY needs ERR_FREQUENCY_OUT_OF_RANGE;
}


int wave_mute(int voice, int state) {
  if (state < 0) {
    if (sv.disconnect[voice] == 0) state = 1;
    else state = 0;
  }
  sv.disconnect[voice] = state;
  return 0;
}

int wave_dir(int voice, int state) {
  if (state < 0) {
    if (sv.direction[voice] == 0) state = 1;
    else state = 0;
  }
  sv.direction[voice] = state;
  return 0;
}


int wave_set(int voice, int wave) {
  if (wave >= 0 && wave < WAVE_TABLE_MAX) {
    osc_set_wave_table_index(voice, wave);
    // AUGGGHHHH... i love the scope, but this needs fixing in a better way...
    // if (scope_enable) scope_wave_update(sv.table[voice], sv.table_size[voice]);
  } else return 100; // <-- more LAZY!!! ERR_INVALID_WAVE;
  return 0;
}



int wave_loop(int voice, int state) {
  if (state < 0) {
    if (sv.loop_enabled[voice] == 0) state = 1;
    else state = 0;
  }
  sv.loop_enabled[voice] = state;
  return 0;
}


int envelope_set(int voice, float a, float d, float s, float r) {
  envelope_init(voice, a, d, s, r);
  return 0;
}


int voice_copy(int v, int n) {
  wave_set(n, sv.wave_table_index[v]);
  amp_set(n, sv.user_amp[v]);
  freq_set(n, sv.freq[v]);
  wave_loop(n, sv.loop_enabled[v]);
  wave_dir(n, sv.direction[v]);
  envelope_set(n, sv.amp_envelope[v].a, sv.amp_envelope[v].d, sv.amp_envelope[v].s, sv.amp_envelope[v].r);
  //
  pan_set(n, sv.pan[v]);
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

int voice_trigger(int voice) {
  osc_trigger(voice);
  return 0;
}

int wave_default(int voice) {
  float g = midi2hz((float)sv.midi_note[voice], 0);
  sv.freq[voice] = g;
  sv.note[voice] = (float)sv.midi_note[voice];
  osc_set_freq(voice, g);
  // FIX FIX FIX scope_wave_update(sv.table[voice], sv.table_size[voice]);
  return 0;
}

int freq_midi(int voice, float note) {
  if (note >= 0.0 && note <= 127.0) {
    sv.last_midi_note[voice] = note;
    if (sv.midi_transpose[voice]) note += sv.midi_transpose[voice];
    float g = midi2hz(note, sv.midi_cents[voice]);
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
  sv.disconnect[i] = 0;
  sv.direction[i] = 0;
  sv.freq[i] = 440.0f;
  sv.midi_note[i] = 69.0f;
  sv.last_midi_note[i] = 69.0f;
  sv.midi_transpose[i] = 0;
  sv.midi_cents[i] = 0;
  sv.link_midi_a[i] = -1;
  sv.link_midi_b[i] = -1;
  sv.link_midi_c[i] = -1;
  sv.link_midi_d[i] = -1;
  sv.link_velo_a[i] = -1;
  sv.link_velo_b[i] = -1;
  sv.link_velo_c[i] = -1;
  sv.link_velo_d[i] = -1;
  sv.link_trig[i] = -1;
  osc_set_wave_table_index(i, WAVE_TABLE_SINE);
  //
  sv.pan[i] = 0;
  sv.pan_left[i] = 0.5f;
  sv.pan_right[i] = 0.5f;
  // pan smoothing?
  //
  //

  sv.record[i] = 0;
}

void voice_init(void) {
  for (int i = 0; i < synth_config.voice_max; i++) {
    voice_reset(i);
  }
}


int wave_reset(int voice, int n) {
  if (voice_invalid(n)) voice_init();
  else voice_reset(n);
  return 0;
}

int envelope_velocity(int voice, float f) {
    if (voice_invalid(voice)) return SYNTH_INVALID_VOICE;
    if (f == 0) {
    } else {
        if (sv.one_shot[voice]) {
            osc_trigger(voice);
        }
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

void wave_table_init(int flag) {
  float *table;

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
  for (int w = WAVE_TABLE_SINE; w <= WAVE_TABLE_NOISE_ALT; w++) {
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
      default: name = "?"; break;
    }
    strncpy(sw.name[w], name, WAVE_NAME_MAX);
    sw.data[w] = (float *)malloc(size * sizeof(float));
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

