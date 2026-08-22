#include "synth.h"
#include "synth-internal.h"
#include "util.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "control-events.h"

int envelope_is_flat(int v) {
  if (sv.amp_envelope[v].a == 0.0f &&
    sv.amp_envelope[v].d == 0.0f &&
    sv.amp_envelope[v].s == 1.0f &&
    sv.amp_envelope[v].r == 0.0f) return 1;
  return 0;
}

int cz_set(int v, int n, float f) {
  sv.cz_mode[v] = n;
  sv.cz_distortion[v] = fmaxf(-1.0f, fminf(1.0f, f));
  return 0;
}

int cmod_set(int voice, int o, float f) {
  if (voice_invalid(voice) || mod_voice_invalid(o)) return SYNTH_INVALID_VOICE;
  sv.cz_mod_osc[voice] = o;
  sv.cz_mod_depth[voice] = f;
  return 0;
}

#include <stdio.h>
#include "control-events.h"

// maybe these should be in wire.[ch]?

int voice_invalid(int voice) {
  if (voice < 0 || voice >= synth_config.voice_max) return 1;
  return 0;
}

int wave_invalid(int wave) {
  if (wave < 0 || wave >= synth_config.wave_table_max) return 1;
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

    /* --- frequency bend (suppress if default) --- */
    if (verbose || sv.freq_bend[v] != 0.0f)
        APPEND(" fb%g", sv.freq_bend[v]);
    if (verbose || sv.freq_bend_range[v] != 2.0f || sv.freq_bend_offset[v] != 0.0f) {
        if (sv.freq_bend_offset[v] != 0.0f)
            APPEND(" fbp%g,%g", sv.freq_bend_range[v], sv.freq_bend_offset[v]);
        else
            APPEND(" fbp%g", sv.freq_bend_range[v]);
    }

    /* --- amplitude bend (suppress if default) --- */
    if (verbose || sv.amp_bend[v] != 0.0f)
        APPEND(" ab%g", sv.amp_bend[v]);
    if (verbose || sv.amp_bend_range[v] != 12.0f || sv.amp_bend_offset[v] != 0.0f) {
        if (sv.amp_bend_offset[v] != 0.0f)
            APPEND(" abp%g,%g", sv.amp_bend_range[v], sv.amp_bend_offset[v]);
        else
            APPEND(" abp%g", sv.amp_bend_range[v]);
    }

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
        || sv.ring_osc[v] >= 0)
        APPEND(" XM%d,%g",
          sv.ring_osc[v], sv.ring_amount[v]);

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
    if (sv.wave_range_override[v])
        APPEND(" VS%d,%d", sv.wave_range_start[v], sv.wave_range_end[v]);
    if (sv.loop_override[v])
        APPEND(" VL%d,%d", sv.loop_start[v], sv.loop_end[v]);

    /* --- pan (suppress if centre) --- */
    if (verbose || sv.pan[v] != 0.0f)
        APPEND(" p%g", sv.pan[v]);

    if (verbose || (sv.pan_mod_osc[v] >= 0 && sv.pan_mod_depth[v] != 0.0f))
        APPEND(" P%d,%g,%g", sv.pan_mod_osc[v], sv.pan_mod_depth[v], sv.pan_mod_adder[v]);

    if (verbose || sv.delay_send[v] > 0.0f)
        APPEND(" ds%g", sv.delay_send[v]);

    if (verbose || sv.filter_mode[v])
        APPEND(" J%d %d K%g Q%g", sv.filter_mode[v] % 10, sv.filter_mode[v] / 10, sv.filter_freq[v], sv.filter_res[v]);

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

    if (verbose || sv.use_cz_envelope[v])
        APPEND(" ct %g %g %g %g cd %g",
            sv.cz_envelope[v].a,
            sv.cz_envelope[v].d,
            sv.cz_envelope[v].s,
            sv.cz_envelope[v].r,
            sv.cz_env_depth[v]);

    if (verbose || sv.sample_hold_ratio[v] > 0.0f) APPEND(" h%g %d", sv.sample_hold_ratio[v], sv.sample_hold_mode[v]);

    if (verbose || sv.quantize[v]) APPEND(" q%d %d", sv.quantize[v] % 100, sv.quantize[v] / 100);

    if (verbose || (sv.amp_mod_osc[v] >= 0 && sv.amp_mod_depth[v] != 0.0f))
        APPEND(" A%d,%g,%g", sv.amp_mod_osc[v], sv.amp_mod_depth[v], sv.amp_mod_adder[v]);

    if (verbose || sv.freq_mod_mode[v] != 0)
        APPEND(" FF%d", sv.freq_mod_mode[v]);
    if (verbose || (sv.freq_mod_osc[v] >= 0 && sv.freq_mod_depth[v] != 0.0f)) {
        APPEND(" F%d,%g,%g", sv.freq_mod_osc[v], sv.freq_mod_depth[v], sv.freq_mod_adder[v]);
    }
    if (verbose || sv.freq_mod_feedback[v] > 0.0f)
        APPEND(" FB%g", sv.freq_mod_feedback[v]);

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
        APPEND(" loop_points:%d..%d loop_override:%d",
            sv.loop_start[v], sv.loop_end[v], sv.loop_override[v]);
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
        APPEND(" cz_env_active:%d", sv.cz_envelope[v].is_active);
        APPEND(" cz_env_runtime:%g,%g,%g,%g",
            sv.cz_envelope[v].attack_time,
            sv.cz_envelope[v].decay_time,
            sv.cz_envelope[v].sustain_level,
            sv.cz_envelope[v].release_time);
        APPEND(" cz_env_release:%llu",
            (unsigned long long)sv.cz_envelope[v].sample_release);
        APPEND(" amp_env_active:%d", sv.amp_envelope[v].is_active);
        APPEND(" amp_env_runtime:%g,%g,%g,%g",
            sv.amp_envelope[v].attack_time,
            sv.amp_envelope[v].decay_time,
            sv.amp_envelope[v].sustain_level,
            sv.amp_envelope[v].release_time);
        APPEND(" amp_env_release:%llu",
            (unsigned long long)sv.amp_envelope[v].sample_release);
        //APPEND(" latency:%.2fms",
            //(double)ts_diff_ns(&sv.mark_a[v], &sv.mark_b[v]) / 1000000.0);
    }

#undef APPEND

    return out;
}


int amp_set(int voice, float f) {
  if (voice_invalid(voice) || !isfinite(f)) return SYNTH_INVALID_VOICE;
  sv.user_amp[voice] = f;
  float total_db = f;
  if (sv.amp_bend) {
    total_db += sv.amp_bend[voice] * sv.amp_bend_range[voice] + sv.amp_bend_offset[voice];
  }
  sv.amp[voice] = DB_TO_LINEAR(total_db);
  return 0;
}

int freq_bend_set(int voice, float val) {
  if (voice_invalid(voice) || !isfinite(val)) return SYNTH_INVALID_VOICE;
  if (val < -1.0f) val = -1.0f;
  if (val > 1.0f) val = 1.0f;
  sv.freq_bend[voice] = val;
  osc_set_freq(&skred_global_engine, voice, sv.freq[voice]);
  return 0;
}

int freq_bend_param_set(int voice, float range, float offset) {
  if (voice_invalid(voice) || !isfinite(range) || !isfinite(offset)) return SYNTH_INVALID_VOICE;
  sv.freq_bend_range[voice] = range;
  sv.freq_bend_offset[voice] = offset;
  osc_set_freq(&skred_global_engine, voice, sv.freq[voice]);
  return 0;
}

int amp_bend_set(int voice, float val) {
  if (voice_invalid(voice) || !isfinite(val)) return SYNTH_INVALID_VOICE;
  if (val < -1.0f) val = -1.0f;
  if (val > 1.0f) val = 1.0f;
  sv.amp_bend[voice] = val;
  amp_set(voice, sv.user_amp[voice]);
  return 0;
}

int amp_bend_param_set(int voice, float range, float offset) {
  if (voice_invalid(voice) || !isfinite(range) || !isfinite(offset)) return SYNTH_INVALID_VOICE;
  sv.amp_bend_range[voice] = range;
  sv.amp_bend_offset[voice] = offset;
  amp_set(voice, sv.user_amp[voice]);
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
  int curve = n / 100;
  int bits = n % 100;
  bits = clampi(bits, 0, 24);
  curve = clampi(curve, 0, 2);
  sv.quantize[voice] = curve * 100 + bits;
  return 0;
}

int freq_set(int voice, float f) {
  if (voice_invalid(voice) || !isfinite(f)) return SYNTH_INVALID_VOICE;
  if (f < 0 || f >= (double)MAIN_SAMPLE_RATE) return 101;

  float target_inc = osc_get_phase_inc(&skred_global_engine, voice, f);

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
  if (state > 2) state = 2;
  sv.direction[voice] = state;
  sv.pingpong_reverse[voice] = state == 1;
  osc_reclassify(&skred_global_engine, voice);
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
  if (!wave_invalid(wave)) {
    osc_set_wave_table_index(&skred_global_engine, voice, wave);
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

int freq_mod_mode_set(int voice, int mode) {
  if (voice_invalid(voice) || mode < 0 || mode > 2)
    return SYNTH_INVALID_VOICE;
  sv.freq_mod_mode[voice] = mode;
  if (mode != 2) {
    sv.freq_mod_feedback_z1[voice] = 0.0f;
    sv.freq_mod_feedback_z2[voice] = 0.0f;
  }
  return 0;
}

int freq_feedback_set(int voice, float amount) {
  if (voice_invalid(voice) || !isfinite(amount) ||
      amount < 0.0f || amount > 7.0f)
    return SYNTH_INVALID_VOICE;
  sv.freq_mod_feedback[voice] = amount;
  if (amount == 0.0f) {
    sv.freq_mod_feedback_z1[voice] = 0.0f;
    sv.freq_mod_feedback_z2[voice] = 0.0f;
  }
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
  sv.loop_release_tail[voice] = 0;
  sv.loop_ended[voice] = 0;
  if (state) {
    sv.loop_bounded[voice] = sv.loop_count[voice] > 0;
    sv.loop_remaining[voice] = sv.loop_count[voice];
  } else {
    sv.loop_bounded[voice] = 0;
    sv.loop_remaining[voice] = 0;
  }
  osc_reclassify(&skred_global_engine, voice);
  return 0;
}

int wave_loop_count(int voice, int count) {
  if (voice_invalid(voice) || count < 0) return SYNTH_INVALID_VOICE;
  sv.loop_count[voice] = count;
  sv.loop_enabled[voice] = 1;
  osc_reclassify(&skred_global_engine, voice);
  return 0;
}


int envelope_set(int voice, float a, float d, float s, float r) {
  envelope_configure_e(&sv.amp_envelope[voice], a, d, s, r);
  return 0;
}

// Set parameters - only recalculates coefficients if values changed
void mmf_set_params(skred_engine_t *engine, int n, float f, float resonance) {
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

    int base_mode = sv.filter_mode[n] % 10;   // 1=LP,2=HP,3=BP,4=Notch,5=Allpass
    int character = sv.filter_mode[n] / 10;    // 0=clean,1=driven,2=screamer

    // Calculate filter coefficients (expensive operations only done here)
    float omega = 2.0f * (float)M_PI * f / (float)MAIN_SAMPLE_RATE;
    float sin_omega = sinf(omega);
    float cos_omega = cosf(omega);
    float alpha = sin_omega / (2.0f * resonance);

    // Tier 1, free: reshapes the coefficient curve. Only runs here, on
    // parameter change — costs nothing extra per sample.
    if (character == 2) {
      alpha /= 1.6f;   // "screamer": pushes the resonant peak harder by narrowing bandwidth
    }

    float a0, b0, b1, b2, a1, a2;

    switch (base_mode) {
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

    // Tier 2, cheap: character sets a post-filter drive amount.
    // drive == 0 lets mmf_process skip the extra work entirely.
    switch (character) {
      case 1:  sv.filter[n].drive = 1.5f + resonance * 0.5f; break;  // "driven"/warm
      case 2:  sv.filter[n].drive = 1.2f; break;                       // a little bite on top of the alpha boost
      default: sv.filter[n].drive = 0.0f; break;                        // "clean" — zero added cost
    }
}


// Initialize the filter with frequency and resonance
// freq: cutoff frequency in Hz
// resonance: resonance factor (0.1 to 10.0, where 0.707 is no resonance)
// sample_rate: audio sample rate in Hz
void mmf_init(skred_engine_t *engine, int n, float f, float resonance) {
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
    mmf_set_params(engine, n, f, resonance);
}

int voice_control_events_set(int voice, int enabled);

int voice_copy(int v, int n) {
  if (voice_invalid(v) || voice_invalid(n)) return SYNTH_INVALID_VOICE;
  wave_set(n, sv.wave_table_index[v]);
  sv.one_shot[n] = sv.one_shot[v];
  amp_set(n, sv.user_amp[v]);
  freq_set(n, sv.freq[v]);
  voice_control_events_set(n, sv.control_events[v]);
  if (sv.wave_range_override[v])
    voice_wave_range_set(n, sv.wave_range_start[v], sv.wave_range_end[v]);
  sv.loop_count[n] = sv.loop_count[v];
  wave_loop(n, sv.loop_enabled[v]);
  wave_dir(n, sv.direction[v]);
  sv.pingpong_reverse[n] = sv.pingpong_reverse[v];
  if (sv.loop_override[v])
    voice_loop_points_set(n, sv.loop_start[v], sv.loop_end[v]);
  osc_reclassify(&skred_global_engine, n);
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
  delay_send_set(&skred_global_engine, n, sv.delay_send[v]);
  amp_mod_set(n, sv.amp_mod_osc[v], sv.amp_mod_depth[v], sv.amp_mod_adder[v]);
  freq_mod_set(n, sv.freq_mod_osc[v], sv.freq_mod_depth[v], sv.freq_mod_adder[v]);
  freq_mod_mode_set(n, sv.freq_mod_mode[v]);
  freq_feedback_set(n, sv.freq_mod_feedback[v]);
  sv.freq_mod_feedback_z1[n] = 0.0f;
  sv.freq_mod_feedback_z2[n] = 0.0f;
  pan_mod_set(n, sv.pan_mod_osc[v], sv.pan_mod_depth[v], sv.pan_mod_adder[v]);
  wave_quant(n, sv.quantize[v]);
  sv.sample_hold_ratio[n] = sv.sample_hold_ratio[v];
  sv.sample_hold_mode[n] = sv.sample_hold_mode[v];
  sv.sample_hold_count[n] = sv.sample_hold_count[v];
  sv.sample_hold[n] = sv.sample_hold[v];
  sv.sample_hold_smooth[n] = sv.sample_hold_smooth[v];
  sv.sample_hold_jitter_target[n] = sv.sample_hold_jitter_target[v];
  cz_set(n, sv.cz_mode[v], sv.cz_distortion[v]);
  cmod_set(n, sv.cz_mod_osc[v], sv.cz_mod_depth[v]);
  sv.use_cz_envelope[n] = sv.use_cz_envelope[v];
  sv.cz_env_depth[n] = sv.cz_env_depth[v];
  envelope_init_e(&sv.cz_envelope[n],
    sv.cz_envelope[v].a, sv.cz_envelope[v].d,
    sv.cz_envelope[v].s, sv.cz_envelope[v].r);
  sv.filter_mode[n] = sv.filter_mode[v];
  mmf_init(&skred_global_engine, n, sv.filter_freq[v], sv.filter_res[v]);
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
  sv.ring_osc[n] = sv.ring_osc[v];
  sv.ring_amount[n] = sv.ring_amount[v];
  freq_bend_param_set(n, sv.freq_bend_range[v], sv.freq_bend_offset[v]);
  freq_bend_set(n, sv.freq_bend[v]);
  amp_bend_param_set(n, sv.amp_bend_range[v], sv.amp_bend_offset[v]);
  amp_bend_set(n, sv.amp_bend[v]);
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
  osc_trigger(&skred_global_engine, voice);
  skred_control_voice_event(SKRED_CONTROL_EVENT_VOICE_TRIGGER,
    SAMPLE_COUNT_GET(), voice);
  return 0;
}

int wave_default(int voice) {
  if (voice_invalid(voice)) return SYNTH_INVALID_VOICE;
  float g = midi2hz((float)sv.midi_note[voice], 0);
  sv.freq[voice] = g;
  sv.note[voice] = (float)sv.midi_note[voice];
  osc_set_freq(&skred_global_engine, voice, g);
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
  sv.pingpong_reverse[i] = 0;
  sv.loop_enabled[i] = 0;
  sv.loop_count[i] = 0;
  sv.loop_bounded[i] = 0;
  sv.loop_remaining[i] = 0;
  sv.loop_active[i] = 0;
  sv.loop_stop_requested[i] = 0;
  sv.loop_release_tail[i] = 0;
  sv.loop_ended[i] = 0;
  sv.loop_override[i] = 0;
  sv.playback_class[i] = OSC_PLAYBACK_GENERAL;
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
  sv.freq_bend[i] = 0.0f;
  sv.freq_bend_range[i] = 2.0f;
  sv.freq_bend_offset[i] = 0.0f;
  sv.amp_bend[i] = 0.0f;
  sv.amp_bend_range[i] = 12.0f;
  sv.amp_bend_offset[i] = 0.0f;
  osc_set_wave_table_index(&skred_global_engine, i, WAVE_TABLE_SINE);
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
  sv.freq_mod_feedback[i] = 0.0f;
  sv.freq_mod_feedback_z1[i] = 0.0f;
  sv.freq_mod_feedback_z2[i] = 0.0f;
  sv.freq_scale[i] = 1.0f;
  sv.pan_mod_osc[i] = -1;
  sv.pan_mod_depth[i] = 0.0f;
  sv.pan_mod_adder[i] = 0.0f;
  sv.quantize[i] = 0;
  sv.filter_mode[i] = 0;
  sv.filter_update_counter[i] = FILTER_UC;
  mmf_init(&skred_global_engine, i, 8000.0f, 0.707f);
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
  sv.ring_osc[i] = -1;
  sv.ring_amount[i] = 0.0;
  sv.cz_mode[i] = 0;
  sv.cz_mod_osc[i] = -1;
  sv.cz_mod_depth[i] = 0.0f;
  sv.cz_distortion[i] = 0.0f;
  sv.use_cz_envelope[i] = 0;
  sv.cz_env_depth[i] = 0.0f;
  envelope_init_e(&sv.cz_envelope[i], 0.0f, 0.0f, 1.0f, 0.0f);
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
    if (f < 0 && sv.one_shot[voice]) {
        sv.loop_stop_requested[voice] = 0;
        sv.loop_release_tail[voice] = 0;
        sv.amp_envelope[voice].is_active = 0;
        sv.filter_envelope[voice].is_active = 0;
        sv.cz_envelope[voice].is_active = 0;
        sv.finished[voice] = 1;
        skred_control_voice_event(SKRED_CONTROL_EVENT_VOICE_RELEASE,
          SAMPLE_COUNT_GET(), voice);
        return 0;
    }
    if (f == 0) {
        int one_shot_loop_release = sv.one_shot[voice] && sv.loop_active[voice];
        if (one_shot_loop_release) {
            sv.loop_stop_requested[voice] = 1;
            sv.loop_release_tail[voice] = 1;
        }
        skred_control_voice_event(SKRED_CONTROL_EVENT_VOICE_RELEASE,
          SAMPLE_COUNT_GET(), voice);
        if (!one_shot_loop_release)
            amp_envelope_release(voice);
        if (!one_shot_loop_release && sv.filter_envelope[voice].is_active)
            envelope_release_e(&sv.filter_envelope[voice]);
        if (!one_shot_loop_release && sv.cz_envelope[voice].is_active)
            envelope_release_e(&sv.cz_envelope[voice]);
    } else {
      sv.use_amp_envelope[voice] = 1;
      sv.freq_mod_feedback_z1[voice] = 0.0f;
      sv.freq_mod_feedback_z2[voice] = 0.0f;
      if (sv.one_shot[voice]) {
          osc_trigger(&skred_global_engine, voice);
      }
      skred_control_voice_event(SKRED_CONTROL_EVENT_VOICE_TRIGGER,
        SAMPLE_COUNT_GET(), voice);
      amp_envelope_trigger(voice, f);
      amp_envelope_schedule_one_shot_release(voice);
      if (sv.use_filter_envelope[voice])
          envelope_trigger_e(&sv.filter_envelope[voice], f);
      if (sv.use_cz_envelope[voice])
          envelope_trigger_e(&sv.cz_envelope[voice], f);
    }
    return 0;
}

int mmf_set_freq(skred_engine_t *engine, int n, float f) {
  sv.filter_freq[n] = f;
  mmf_set_params(engine, n, f, sv.filter_res[n]);
  return 0;
}

int mmf_set_res(skred_engine_t *engine, int n, float res) {
  if (res > 0) {
    sv.filter_res[n] = res;
    mmf_set_params(engine, n, sv.filter_freq[n], res);
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


int mod_voice_invalid(int voice) {
  return voice < -1 || voice >= synth_config.voice_max;
}
