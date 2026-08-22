#include "synth.h"
#include "synth-internal.h"
#include "util.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "control-events.h"

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

float audio_rng_raw_float(uint64_t raw) {
    uint32_t val = (uint32_t)(raw >> 32);
    return (float)((int32_t)val) / 2147483648.0f;
}

float osc_get_phase_inc(skred_engine_t *engine, int v, float f) {
  float g = f;
  if (sv.freq_bend) {
    float semitones = sv.freq_bend[v] * sv.freq_bend_range[v] + sv.freq_bend_offset[v];
    if (semitones != 0.0f) {
      g *= powf(2.0f, semitones / 12.0f);
    }
  }
  if (sv.one_shot[v]) {
    if (sv.offset_hz[v] > 0.0f) {
      g /= sv.offset_hz[v];
      return (g * (float)sv.table_size[v]) / (float)MAIN_SAMPLE_RATE;
    }
    float rate = sv.table_rate[v] > 0.0f ? sv.table_rate[v] : (float)MAIN_SAMPLE_RATE;
    return (g / 440.0f) * (rate / (float)MAIN_SAMPLE_RATE);
  }
  return (g * (float)sv.table_size[v]) / (float)MAIN_SAMPLE_RATE;
}

void osc_set_freq(skred_engine_t *engine, int v, float f) {
  sv.phase_inc[v] = osc_get_phase_inc(engine, v, f);
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

float cz_phasor(int n, float p, float d, float active_start, float active_end) {
    const float length = active_end - active_start;
    if (length <= 0.0f) return p;
    float phase = (p - active_start) / length;
    d = fmaxf(-1.0f, fminf(1.0f, d));
    const float amount = fabsf(d);
    
    switch (n) {
        case 1: { // saw -> pulse
            const float breakpoint = 0.5f + 0.499f * d;
            const float inv_d = 0.5f / breakpoint;
            const float inv_1_minus_d = 0.5f / (1.0f - breakpoint);
            if (phase < breakpoint) {
                phase *= inv_d;
            } else {
                phase = 0.5f + (phase - breakpoint) * inv_1_minus_d;
            }
            break;
        }
        case 2: { // square (folded sine)
            const float half_d = d * 0.499f;
            const float scale = 0.5f / (0.5f - half_d);
            if (phase < 0.5f) {
                phase *= scale;
            } else {
                phase = 1.0f - (1.0f - phase) * scale;
            }
            break;
        }
        case 3: { // triangle
            const float half_d = d * 0.499f;
            const float scale = 0.5f / (0.5f - half_d);
            if (phase < 0.5f) {
                phase *= scale;
            } else {
                phase = 0.5f + (phase - 0.5f) * scale;
            }
            break;
        }
        case 4: { // double sine
            const float doubled = fmodf(phase * 2.0f, 1.0f);
            const float target = d < 0.0f ? 1.0f - doubled : doubled;
            phase += amount * (target - phase);
            break;
        }
        case 5: { // saw -> triangle
            const float half_d = d * 0.499f;
            const float scale1 = 0.5f / (0.5f - half_d);
            const float scale2 = 0.5f / (0.5f + half_d);
            if (phase < 0.5f) {
                phase *= scale1;
            } else {
                phase = 0.5f + (phase - 0.5f) * scale2;
            }
            break;
        }
        case 6: { // resonant 1
            float exponent = 1.0f + 4.0f * amount;
            phase = fast_pow(phase, d < 0.0f ? 1.0f / exponent : exponent);
            break;
        }
        case 7: { // resonant 2
            float exponent = 1.0f + 8.0f * amount;
            phase = fast_pow(phase, d < 0.0f ? 1.0f / exponent : exponent);
            break;
        }
        default:
            return p;
    }

    phase = fmaxf(0.0f, fminf(1.0f, phase));
    return active_start + phase * length;
}

int osc_loop_crossings(double distance, double loop_length) {
    double crossings = distance / loop_length;
    return crossings >= (double)(INT_MAX - 1) ?
        INT_MAX : (int)crossings + 1;
}

void osc_reclassify(skred_engine_t *engine, int voice) {
    if (voice_invalid(voice)) return;
    sv.playback_class[voice] =
        sv.table[voice] != NULL &&
        sv.table_size[voice] > 0 &&
        !sv.one_shot[voice] &&
        sv.direction[voice] == 0 &&
        !sv.loop_active[voice] &&
        sv.wave_range_start[voice] == 0 &&
        sv.wave_range_end[voice] == sv.table_size[voice]
        ? OSC_PLAYBACK_CYCLE_SIMPLE
        : OSC_PLAYBACK_GENERAL;
}

static inline double osc_cycle_phase_next(int voice, float phase_inc) {
    const double end = (double)sv.table_size[voice];
    double phase = sv.phase[voice] + (double)phase_inc;

    if (phase >= end) {
        phase -= end;
        if (phase >= end) phase = fmod(phase, end);
    } else if (phase < 0.0) {
        phase += end;
        if (phase < 0.0) {
            phase = fmod(phase, end);
            if (phase < 0.0) phase += end;
        }
    }
    return phase;
}

static inline float osc_sample_at_phase(skred_engine_t *engine, int voice, double phase, float phase_offset,
    uint64_t current_sample, int table_size, float wave_range_start,
    float wave_range_end, bool one_shot, float active_start, float active_end) {
    double final_phase;
    (void)current_sample;

    if (sv.cz_mode[voice]) {
      float amount = sv.cz_distortion[voice];
      int dv = sv.cz_mod_osc[voice];
      if (dv >= 0)
        amount += sv.sample[dv] * sv.cz_mod_depth[voice];
      if (sv.cz_envelope[voice].is_active)
        amount += envelope_step_e(&sv.cz_envelope[voice], current_sample) *
          sv.cz_env_depth[voice];
      amount = fmaxf(-1.0f, fminf(1.0f, amount));
      final_phase = cz_phasor(sv.cz_mode[voice], (float)phase, amount,
        active_start, active_end);
    } else {
      final_phase = phase;
    }

    /*
     * FF2 phase modulation changes only the wavetable lookup position. The
     * persistent oscillator phase continues at phase_inc, so bipolar
     * modulation and feedback do not pull the operator off pitch.
     * phase_offset is expressed in radians (the conventional FM index unit).
     */
    if (phase_offset != 0.0f) {
      const double span = (double)wave_range_end - (double)wave_range_start;
      double offset = (double)phase_offset * span / (double)FM_TWO_PI;
      final_phase = (double)wave_range_start +
        fmod(final_phase - (double)wave_range_start + offset, span);
      if (final_phase < (double)wave_range_start) final_phase += span;
    }

    int idx = (int)final_phase;
    if (idx >= (int)wave_range_end) idx = (int)wave_range_end - 1;
    if (idx < (int)wave_range_start) idx = (int)wave_range_start;

    if (sv.interpolate[voice]) {
        float frac = (float)(final_phase - (double)idx);
        int next_idx = idx + 1;
        if (next_idx >= (int)wave_range_end)
            next_idx = one_shot ? (int)wave_range_end - 1 :
                (int)wave_range_start;
        float sample1 = sv.table[voice][idx];
        float sample2 = sv.table[voice][next_idx];
        return sample1 + frac * (sample2 - sample1);
    }
    return sv.table[voice][idx];
}

float osc_next_at(skred_engine_t *engine, int voice, float phase_inc, float phase_offset,
    uint64_t current_sample) {
    if (sv.finished[voice]) return 0.0f;

    if (sv.playback_class[voice] == OSC_PLAYBACK_CYCLE_SIMPLE) {
        double phase = osc_cycle_phase_next(voice, phase_inc);
        if (!isfinite(phase)) {
            sv.phase[voice] = 0.0;
            return 0.0f;
        }
        sv.phase[voice] = phase;
        return osc_sample_at_phase(engine, voice, phase, phase_offset, current_sample,
            sv.table_size[voice], 0.0f, (float)sv.table_size[voice], false,
            0.0f, (float)sv.table_size[voice]);
    }

    const int table_size = sv.table_size[voice];
    const bool one_shot = sv.one_shot[voice];
    const bool loop_active = sv.loop_active[voice];
    const float wave_range_start = sv.wave_range_start_f[voice];
    const float wave_range_end = sv.wave_range_end_f[voice];
    
    const bool pingpong = sv.direction[voice] == 2;
    bool pingpong_reverse = pingpong && sv.pingpong_reverse[voice];
    double phase_step = phase_inc;
    if (pingpong) phase_step = pingpong_reverse ? -fabs(phase_step) : fabs(phase_step);
    else if (sv.direction[voice]) phase_step = -phase_step;
    bool reverse_step = phase_step < 0.0;

    double phase = sv.phase[voice] + phase_step;
    
    if (!isfinite(phase)) {
        sv.phase[voice] = wave_range_start;
        sv.finished[voice] = one_shot;
        return 0.0f;
    }
    
    // Get loop boundaries (precomputed if available)
    const bool valid_loop = loop_active && sv.loop_valid[voice];
    const double loop_start = valid_loop ? (double)sv.loop_start_f[voice] : (double)wave_range_start;
    const double loop_end = valid_loop ? (double)sv.loop_end_f[voice] : (double)wave_range_end;
    const double loop_length = valid_loop ? (double)sv.loop_length[voice] : (double)(wave_range_end - wave_range_start);

    // Wrap phase
    if (!one_shot) {
        if (pingpong && loop_length > 0.0 && (phase >= loop_end || phase < loop_start)) {
            double offset = fmod(phase - loop_start, loop_length * 2.0);
            if (offset < 0.0) offset += loop_length * 2.0;
            if (offset >= loop_length) {
                phase = loop_end - (offset - loop_length);
                sv.pingpong_reverse[voice] = 1;
            } else {
                phase = loop_start + offset;
                sv.pingpong_reverse[voice] = 0;
            }
        } else if (phase >= loop_end) {
            phase = loop_start + fmod(phase - loop_start, loop_length);
        } else if (phase < loop_start) {
            phase = loop_end - fmod(loop_start - phase, loop_length);
        }
    } else if (pingpong && loop_active &&
               ((!reverse_step && phase >= loop_end) ||
                (reverse_step && phase < loop_start))) {
        int guard = 32;
        while (guard-- > 0 &&
               ((!reverse_step && phase >= loop_end) ||
                (reverse_step && phase < loop_start))) {
            if (sv.loop_stop_requested[voice]) {
                sv.loop_active[voice] = 0;
                sv.loop_stop_requested[voice] = 0;
                sv.loop_release_tail[voice] = 0;
                if (!reverse_step && phase >= (double)wave_range_end) {
                    phase = (double)wave_range_end - 1e-6;
                    sv.finished[voice] = 1;
                } else if (reverse_step && phase < (double)wave_range_start) {
                    phase = (double)wave_range_start;
                    sv.finished[voice] = 1;
                }
                break;
            }
            if (sv.loop_bounded[voice] && sv.loop_remaining[voice] <= 0) {
                sv.loop_active[voice] = 0;
                sv.loop_stop_requested[voice] = 0;
                sv.loop_release_tail[voice] = 1;
                sv.loop_ended[voice] = 1;
                if (!reverse_step && phase >= (double)wave_range_end) {
                    phase = (double)wave_range_end - 1e-6;
                    sv.finished[voice] = 1;
                } else if (reverse_step && phase < (double)wave_range_start) {
                    phase = (double)wave_range_start;
                    sv.finished[voice] = 1;
                }
                break;
            }
            if (sv.loop_bounded[voice]) sv.loop_remaining[voice]--;
            if (!reverse_step) {
                phase = loop_end - (phase - loop_end);
                sv.pingpong_reverse[voice] = 1;
            } else {
                phase = loop_start + (loop_start - phase);
                sv.pingpong_reverse[voice] = 0;
            }
            pingpong_reverse = sv.pingpong_reverse[voice] != 0;
            phase_step = pingpong_reverse ? -fabs(phase_step) : fabs(phase_step);
            reverse_step = phase_step < 0.0;
        }
    } else if (!reverse_step && phase >= loop_end) {
        if (!loop_active) {
            phase = loop_end - 1e-6;
            sv.finished[voice] = 1;
        } else if (sv.loop_stop_requested[voice]) {
            sv.loop_active[voice] = 0;
            sv.loop_stop_requested[voice] = 0;
            sv.loop_release_tail[voice] = 0;
            if (phase >= (double)wave_range_end) {
                phase = (double)wave_range_end - 1e-6;
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
                sv.loop_release_tail[voice] = 1;
                sv.loop_ended[voice] = 1;
                if (phase >= (double)wave_range_end) {
                    phase = (double)wave_range_end - 1e-6;
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
            sv.loop_release_tail[voice] = 0;
            if (phase < (double)wave_range_start) {
                phase = (double)wave_range_start;
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
                sv.loop_release_tail[voice] = 1;
                sv.loop_ended[voice] = 1;
                if (phase < (double)wave_range_start) {
                    phase = (double)wave_range_start;
                    sv.finished[voice] = 1;
                }
            }
        } else {
            phase = loop_end - fmod(loop_start - phase, loop_length);
        }
    }
    
    sv.phase[voice] = phase;
    return osc_sample_at_phase(engine, voice, phase, phase_offset, current_sample,
        table_size, wave_range_start, wave_range_end, one_shot,
        (float)loop_start, (float)loop_end);
}

float osc_next(skred_engine_t *engine, int voice, float phase_inc) {
    return osc_next_at(engine, voice, phase_inc, 0.0f, SAMPLE_COUNT_GET());
}

void osc_set_wave_table_index(skred_engine_t *engine, int voice, int wave) {
  // if we were using a r/w wave table, adjust ref count
  int old = sv.wave_table_index[voice];
  // old == -1 means voice was never assigned a wave table yet (e.g. first
  // call from voice_reset).  Guard before indexing to avoid sw.readonly[-1].
  if (old != wave) {
    if (old >= 0) sw.refcount[old]--;
    if (wave >= 0) sw.refcount[wave]++;
  }
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
    sv.loop_override[voice] = 0;
    sv.wave_range_override[voice] = 0;
    sv.wave_range_start[voice] = 0;
    sv.wave_range_end[voice]   = sv.table_size[voice];
    sv.wave_range_start_f[voice] = 0.0f;
    sv.wave_range_end_f[voice]   = (float)sv.table_size[voice];
    sv.loop_start[voice] = sw.loop_start[wave];
    sv.loop_enabled[voice] = sw.loop_enabled[wave];
    sv.loop_active[voice] = sv.loop_enabled[voice];
    sv.loop_bounded[voice] = sv.loop_count[voice] > 0;
    sv.loop_remaining[voice] = sv.loop_count[voice];
    sv.loop_stop_requested[voice] = 0;
    sv.loop_release_tail[voice] = 0;
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
    if (update_freq) {
      osc_set_freq(engine, voice, sv.freq[voice]);
    }
    osc_reclassify(engine, voice);
  }
}

static void voice_loop_points_apply(int voice, int start, int end);

static void voice_wave_range_apply(int voice, int start, int end) {
  sv.wave_range_start[voice] = start;
  sv.wave_range_end[voice]   = end;
  sv.wave_range_start_f[voice] = (float)start;
  sv.wave_range_end_f[voice]   = (float)end;
}

int voice_wave_range_set(int voice, int start, int end) {
    if (voice_invalid(voice) || sv.table_size[voice] < 2 ||
        start < 0 || end <= start || end > sv.table_size[voice]) {
        return SYNTH_INVALID_VOICE;
    }
    voice_wave_range_apply(voice, start, end);
    if (sv.loop_start[voice] < start || sv.loop_end[voice] > end ||
        sv.loop_end[voice] <= sv.loop_start[voice]) {
      voice_loop_points_apply(voice, start, end);
    }
    sv.wave_range_override[voice] = 1;
    osc_reclassify(&skred_global_engine, voice);
    return 0;
}

int voice_wave_range_reset(int voice) {
    if (voice_invalid(voice)) return SYNTH_INVALID_VOICE;
    voice_wave_range_apply(voice, 0, sv.table_size[voice]);
    sv.wave_range_override[voice] = 0;
    osc_reclassify(&skred_global_engine, voice);
    return 0;
}

static void voice_loop_points_apply(int voice, int start, int end) {
  sv.loop_start[voice] = start;
  sv.loop_end[voice] = end;
  sv.loop_start_f[voice] = (float)start;
  sv.loop_end_f[voice] = (float)end;
  if (end > start) {
    sv.loop_valid[voice] = 1;
    sv.loop_length[voice] = end - start;
  } else {
    sv.loop_valid[voice] = 0;
    sv.loop_length[voice] = sv.wave_range_end[voice] - sv.wave_range_start[voice];
  }
}

static void voice_loop_points_apply_default(int voice, int start, int end) {
  if (start < sv.wave_range_start[voice] || end <= start ||
      end > sv.wave_range_end[voice]) {
    start = sv.wave_range_start[voice];
    end = sv.wave_range_end[voice];
  }
  voice_loop_points_apply(voice, start, end);
}

int voice_loop_points_set(int voice, int start, int end) {
  if (voice_invalid(voice) || sv.table_size[voice] < 2 ||
      start < sv.wave_range_start[voice] || end <= start ||
      end > sv.wave_range_end[voice]) {
    return SYNTH_INVALID_VOICE;
  }
  voice_loop_points_apply(voice, start, end);
  sv.loop_override[voice] = 1;
  osc_reclassify(&skred_global_engine, voice);
  return 0;
}

int voice_loop_points_reset(int voice) {
  if (voice_invalid(voice)) return SYNTH_INVALID_VOICE;
  int wave = sv.wave_table_index[voice];
  if (wave_invalid(wave) || !sw.data[wave])
    return SYNTH_INVALID_VOICE;
  voice_loop_points_apply_default(voice, sw.loop_start[wave], sw.loop_end[wave]);
  sv.loop_override[voice] = 0;
  osc_reclassify(&skred_global_engine, voice);
  return 0;
}

int wave_loop_points_set(int wave, int start, int end) {
  if (wave_invalid(wave) || !sw.data[wave] ||
      sw.size[wave] < 2 || start < 0 || end <= start ||
      end > sw.size[wave]) {
    return -1;
  }
  sw.loop_start[wave] = start;
  sw.loop_end[wave] = end;
  for (int voice = 0; voice < synth_config.voice_max; voice++) {
    if (sv.wave_table_index[voice] == wave && !sv.loop_override[voice])
      voice_loop_points_apply_default(voice, start, end);
  }
  return 0;
}

void osc_trigger(skred_engine_t *engine, int voice) {
    sv.finished[voice] = 0;
    sv.loop_active[voice] = sv.loop_enabled[voice];
    sv.loop_bounded[voice] = sv.loop_count[voice] > 0;
    sv.loop_remaining[voice] = sv.loop_count[voice];
    sv.loop_stop_requested[voice] = 0;
    sv.loop_release_tail[voice] = 0;
    sv.loop_ended[voice] = 0;
    sv.pingpong_reverse[voice] = sv.direction[voice] == 1;
    osc_reclassify(engine, voice);
    
    if (sv.one_shot[voice]) {
        if (sv.direction[voice] == 1) {
            sv.phase[voice] = (double)sv.wave_range_end_f[voice] - 1e-6;
        } else {
            sv.phase[voice] = (double)sv.wave_range_start_f[voice];
        }
    } else {
        // Preserve direction, but start at appropriate boundary
        if (sv.direction[voice] == 1) {
            // Backward playback: start at loop end
            sv.phase[voice] = sv.loop_active[voice]
                ? (double)sv.loop_end[voice] - 1e-6  // or sv.loop_end_f[voice]
                : (double)sv.wave_range_end_f[voice] - 1e-6;
        } else {
            // Forward playback: start at loop start
            sv.phase[voice] = sv.loop_active[voice]
                ? (double)sv.loop_start[voice]  // or sv.loop_start_f[voice]
                : (double)sv.wave_range_start_f[voice];
        }
    }
}

float quantize_bits_curve(float v, int bits, int curve, uint64_t *rng) {
  int levels = (1 << bits) - 1;
  if (levels < 1) levels = 1;

  if (curve == 1) {
    // Cheap rational compander — no log/exp. Boosts low-level detail
    // before quantizing, un-boosts after: turns the linear staircase
    // into something closer to tape/cassette grit at the same bit depth.
    const float k = 4.0f;
    float sign = v < 0.0f ? -1.0f : 1.0f;
    float av = fabsf(v);
    if (av > 1.0f) av = 1.0f; // Prevent compander wrap-around and div by zero
    float companded = av * (1.0f + k) / (1.0f + k * av);
    int iv = (int)roundf(companded * (float)levels);
    float q = (float)iv * (1.0f / (float)levels);
    float expanded = q / (1.0f + k * (1.0f - q));
    return sign * expanded;
  }

  if (curve == 2) {
    // Triangular dither before rounding — quantization distortion
    // becomes broadband noise instead of a tonal buzz.
    float t1 = audio_rng_float(rng);
    float t2 = audio_rng_float(rng);
    float dither = (t1 - t2) * (0.5f / (float)levels);
    int iv = (int)roundf((v + dither) * (float)levels);
    return (float)iv * (1.0f / (float)levels);
  }

  // curve 0 — same math as before, just correctly rounded (was
  // truncating-toward-zero on negative samples: (int)(x+0.5) only
  // rounds correctly for x >= 0).
  int iv = (int)roundf(v * (float)levels);
  return (float)iv * (1.0f / (float)levels);
}

