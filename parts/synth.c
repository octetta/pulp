#include <float.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "synth-types.h"

#include "synth-config.h"
#include "synth-state.h"
#include "synth.h"
#include "synth-internal.h"
#include "synth-alloc.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "portable_atomic.h"
#include "control-events.h"

#define SAMPLE_COUNT_PUT(n) atomic_store_uint64(&synth_sample_count, n)
#define SAMPLE_COUNT_GET() atomic_load_uint64(&synth_sample_count)
#define SAMPLE_COUNT_ADD(n) atomic_fetch_add_uint64(&synth_sample_count, n)

#define VOLUME_DEFAULT (-20.0f)
#define DB_TO_LINEAR(v) powf(10.f, (v) / 20.0f)
#define SYNTH_INVALID_VOICE (100)

static int verbose = 0;

int volume_set(float v);
float volume_get(void);
float envelope_step_e(envelope_t *e, uint64_t current_sample);
void osc_set_freq(skred_engine_t *engine, int v, float f);
static void synth_track_defaults(void);

int synth_sample_rate_set(int sample_rate) {
  if (sample_rate < 1) sample_rate = SKRED_DEFAULT_SAMPLE_RATE;
  int old_rate = synth_sample_rate;
  synth_sample_rate = sample_rate;
  if (old_rate != sample_rate && sw.data) {
    for (int w = 0; w < synth_config.wave_table_max; w++) {
      if (sw.data[w] && (sw.rate[w] == (float)old_rate || sw.rate[w] <= 0.0f)) {
        sw.rate[w] = (float)sample_rate;
      }
    }
    for (int v = 0; v < synth_config.voice_max; v++) {
      if (sv.table_rate) {
        if (sv.table_rate[v] == (float)old_rate) sv.table_rate[v] = (float)sample_rate;
        sv.table_size_rate[v] = (float)sv.table_size[v] / (float)sample_rate;
        float f = sv.freq[v] > 0.0f ? sv.freq[v] : 440.0f;
        osc_set_freq(&skred_global_engine, v, f);
      }
    }
  }
  return synth_sample_rate;
}

int synth_sample_rate_get(void) {
  return synth_sample_rate;
}


void synth_init(int vc) {
  if (synth_sample_rate == 0) synth_sample_rate = SKRED_DEFAULT_SAMPLE_RATE;
  requested_synth_frames_per_callback = SYNTH_FRAMES_PER_CALLBACK;
  synth_frames_per_callback = 0;
  volume_user = VOLUME_DEFAULT;
  volume_final = 1.0f;
  volume_smoother_gain = 0.0f;
  volume_smoother_smoothing = 0.002f;
  volume_threshold = 0.05f;
  volume_smoother_higher_smoothing = 0.3f;

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
  for (int bus = 0; bus < DELAY_BUS_COUNT; bus++) {
    delay_cache_params(&skred_global_engine, &delay_bus[bus]);
    delay_bus[bus].base_frames = delay_bus[bus].base_frames_target;
  }
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



float clampf(float value, float min_value, float max_value) {
  if (value < min_value) return min_value;
  if (value > max_value) return max_value;
  return value;
}

int clampi(int value, int min_value, int max_value) {
  if (value < min_value) return min_value;
  if (value > max_value) return max_value;
  return value;
}

void synth_voice_bench(int voice) {
  sv.mark_b[voice].tv_sec = 0;
  sv.mark_b[voice].tv_nsec = 0;
  //clock_gettime(VOICE_CLOCK, &sv.mark_a[voice]);
  sv.mark_go[voice] = 1;
}



void mmf_set_params(skred_engine_t *engine, int n, float f, float resonance);

static float *this_capture;
static int this_capture_channels;

synth_sample_t sampling = {
  .state = SAMPLE_STATE_IDLE,
  .source = SAMPLE_SOURCE_DRY,
  .source_voice = -1,
  .channels = 1
};

#undef sv
#define sv (engine->sv)
#undef sw
#define sw (engine->sw)
#undef synth_config
#define synth_config (engine->config)
#undef synth_sample_rate
#define synth_sample_rate (engine->sample_rate)
#undef synth_sample_count
#define synth_sample_count (engine->sample_count)
#undef requested_synth_frames_per_callback
#define requested_synth_frames_per_callback (engine->requested_frames_per_callback)
#undef synth_frames_per_callback
#define synth_frames_per_callback (engine->frames_per_callback)
#undef volume_user
#define volume_user (engine->volume_user)
#undef volume_final
#define volume_final (engine->volume_final)
#undef volume_smoother_gain
#define volume_smoother_gain (engine->volume_smoother_gain)
#undef volume_smoother_smoothing
#define volume_smoother_smoothing (engine->volume_smoother_smoothing)
#undef volume_threshold
#define volume_threshold (engine->volume_threshold)
#undef volume_smoother_higher_smoothing
#define volume_smoother_higher_smoothing (engine->volume_smoother_higher_smoothing)

void synth_capture(skred_engine_t *engine, float *buffer, float *input, int num_frames,
                   int num_channels, int input_channels, void *user) {
  (void)input;
  synth_record_bus_t *record_bus = (synth_record_bus_t *)user;
  float *record_frames = NULL;
  if (record_bus && record_bus->channels == RECORD_CHANNELS) {
    record_frames = record_bus->frames;
  }
  const int nvoices = synth_voice_count();
  float track_gain[RECORD_TRACK_COUNT];
  for (int track = 1; track <= RECORD_TRACK_MAX; track++)
    track_gain[track] = synth_track_volume_linear_get(track);
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

  this_capture = input;
  this_capture_channels = input && input_channels > 0 ? input_channels : 0;

  //BEN_MARK_A(bench, benchp, num_frames, bencho);

  uint64_t callback_sample = SAMPLE_COUNT_ADD(num_frames);
  for (int i = 0; i < num_frames; i++) {
    uint64_t current_sample = callback_sample + (uint64_t)i;
    float sample_left = 0.0f;
    float sample_right = 0.0f;
    float delay_input[DELAY_BUS_COUNT] = {0};
    float record_left[RECORD_TRACK_COUNT] = {0};
    float record_right[RECORD_TRACK_COUNT] = {0};
    
    int sample_state = atomic_load_int(&sampling.state);
    if (sample_state == SAMPLE_STATE_ARMED) {
      sampling.ptr = 0;
      sampling.len = 0;
      sampling.offset = 0;
      sampling.trim = 0;
      atomic_store_int(&sampling.state, SAMPLE_STATE_RECORDING);
      sample_state = SAMPLE_STATE_RECORDING;
    }
    float record_mono = 0.0f;
    float record_voice = 0.0f;
    
    float f = 0.0f;
    uint64_t noise_raw = audio_rng_next(&synth_random);
    float whiteish = 0.0f;
    int whiteish_ready = 0;
    for (int n = 0; n < nvoices; n++) {
      if (sv.mark_go[n]) {
        //clock_gettime(VOICE_CLOCK, &sv.mark_b[n]);
        sv.mark_go[n] = 0;
      }
      if (sv.finished[n]) {
        //sv.sample[n] = 0.0f; // remove to try the below
        // hold last value to modulator consumers see statle output after one-shot ends
        continue;
      }  
      if (sv.user_amp[n] <= SILENT) {
        sv.sample[n] = 0.0f;
        sv.freq_mod_feedback_z1[n] = 0.0f;
        sv.freq_mod_feedback_z2[n] = 0.0f;
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
      int capture_channel = sv.wave_table_index[n] - WAVE_TABLE_CAPTURE_FIRST;
      if (capture_channel >= 0 &&
          capture_channel < WAVE_TABLE_CAPTURE_CHANNELS) {
        f = capture_channel < this_capture_channels
          ? this_capture[(size_t)i * this_capture_channels + capture_channel]
          : 0.0f;
        is_capture = 1;
      }
      else if (sv.wave_table_index[n] == WAVE_TABLE_NOISE_ALT) {
        if (!whiteish_ready) {
          whiteish = audio_rng_raw_float(noise_raw);
          whiteish_ready = 1;
        }
        f = whiteish;
      }
      else {
        if (sv.freq_mod_mode[n] == 2) {
          float phase_offset = 0.0f;
          if (sv.freq_mod_osc[n] >= 0 && sv.freq_mod_osc[n] != n) {
            int mod = sv.freq_mod_osc[n];
            phase_offset += sv.sample[mod] * sv.freq_mod_depth[n] +
              sv.freq_mod_adder[n];
          }
          if (sv.freq_mod_feedback[n] > 0.0f) {
            phase_offset +=
              0.5f * (sv.freq_mod_feedback_z1[n] +
                      sv.freq_mod_feedback_z2[n]) *
              sv.freq_mod_feedback[n];
          }
          f = osc_next_at(engine, n, sv.phase_inc[n], phase_offset, current_sample);
        } else if (sv.freq_mod_osc[n] >= 0 && sv.freq_mod_osc[n] != n) {
          int mod = sv.freq_mod_osc[n];
          float g = sv.sample[mod] * sv.freq_mod_depth[n] + sv.freq_mod_adder[n];
          float inc;
          if (sv.freq_mod_mode[n]) {
            inc = (g * sv.table_size_rate[n]);
          } else {
            inc = sv.phase_inc[n] + (sv.phase_inc[mod] * sv.freq_scale[n] * g);
          }
          f = osc_next_at(engine, n, inc, 0.0f, current_sample);
        } else {
          f = osc_next_at(engine, n, sv.phase_inc[n], 0.0f, current_sample);
        }
      if (sv.loop_ended[n]) {
        int release_tail = sv.one_shot[n] && sv.loop_release_tail[n];
        sv.loop_ended[n] = 0;
        if (!release_tail)
          envelope_release_e_at(&sv.amp_envelope[n], current_sample);
        if (!release_tail)
          envelope_release_e_at(&sv.filter_envelope[n], current_sample);
        if (!release_tail)
          envelope_release_e_at(&sv.cz_envelope[n], current_sample);
        sv.loop_release_tail[n] = 0;
        skred_control_voice_event(SKRED_CONTROL_EVENT_VOICE_RELEASE,
          current_sample, n);
      }
      if (!was_finished && sv.finished[n]) {
        skred_control_voice_event(SKRED_CONTROL_EVENT_VOICE_FINISHED,
          current_sample, n);
      }
        if (sv.ring_osc[n] >= 0.0) {
          f *= sv.sample[sv.ring_osc[n]];
        }
      }
      if (sv.sample_hold_ratio[n] > 0.0f) {
        float f_hz = sv.freq[n] > 0.0f ? sv.freq[n] : 440.0f;
        int sah_period = (int)(((float)MAIN_SAMPLE_RATE / f_hz) * sv.sample_hold_ratio[n]);
        if (sah_period < 1) sah_period = 1;
        int sah_mode = sv.sample_hold_mode[n];

        int sah_limit = sah_period;
        if (sah_mode == 2) {
          if (sv.sample_hold_count[n] == 0) {
            int lo = sah_period / 2;
            if (lo < 1) lo = 1;
            float jitter = audio_rng_float(&synth_random);
            sv.sample_hold_jitter_target[n] = lo + (int)(jitter * (float)sah_period);
          }
          sah_limit = sv.sample_hold_jitter_target[n];
          if (sah_limit < 1) sah_limit = 1;
        }

        if (sv.sample_hold_count[n] == 0) {
          sv.sample_hold[n] = f;
        }

        if (sah_mode == 1) {
          float coeff = 4.0f / (float)sah_period;
          if (coeff > 1.0f) coeff = 1.0f;
          sv.sample_hold_smooth[n] +=
            coeff * (sv.sample_hold[n] - sv.sample_hold_smooth[n]);
          sv.sample[n] = sv.sample_hold_smooth[n];
        } else {
          sv.sample[n] = sv.sample_hold[n];
        }

        sv.sample_hold_count[n]++;
        if (sv.sample_hold_count[n] >= sah_limit) {
          sv.sample_hold_count[n] = 0;
        }
      } else {
        sv.sample[n] = f;
      }

      // apply quantizer
      if (sv.quantize[n]) {
        int q_bits = sv.quantize[n] % 100;
        int q_curve = sv.quantize[n] / 100;
        sv.sample[n] = quantize_bits_curve(sv.sample[n], q_bits, q_curve, &synth_random);
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
          mmf_set_params(engine, n, cutoff, sv.filter_res[n]);
          sv.filter_update_counter[n] = FILTER_UC;
        }
        sv.filter_update_counter[n]--;
        sv.sample[n] = mmf_process(engine, n, sv.sample[n]);
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
      
      if (synth_config.trace_latency && env > 0.0f && sv.latency_timestamp_ns[n] > 0) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        uint64_t now_ns = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
        uint64_t delta_ns = now_ns - sv.latency_timestamp_ns[n];
        fprintf(stderr, "# command latency [v%d]: %llu ns (%.3f ms)\n", n, (unsigned long long)delta_ns, delta_ns / 1000000.0);
        sv.latency_timestamp_ns[n] = 0;
      }

#if 0
      if (sv.smoother_enable[n]) {
        sv.smoother_gain[n] += sv.smoother_smoothing[n] * (final - sv.smoother_gain[n]);
        final = sv.smoother_gain[n];
      }
#endif

      sv.sample[n] *= final;

      if (sv.freq_mod_mode[n] == 2 && sv.freq_mod_feedback[n] > 0.0f) {
        sv.freq_mod_feedback_z2[n] = sv.freq_mod_feedback_z1[n];
        sv.freq_mod_feedback_z1[n] = sv.sample[n];
      }

      if (sample_state == SAMPLE_STATE_RECORDING &&
          sampling.source == SAMPLE_SOURCE_VOICE &&
          sampling.source_voice == n) {
        record_voice = sv.sample[n];
      }
      if (sample_state == SAMPLE_STATE_RECORDING &&
          sampling.source == SAMPLE_SOURCE_DRY &&
          (sv.disconnect[n] == 0 || is_capture)) {
        record_mono += sv.sample[n];
      }

      float left  = sv.sample[n];
      float right = sv.sample[n];
      if (sv.pan_mod_osc[n] >= 0) {
        float q = sv.sample[sv.pan_mod_osc[n]] * sv.pan_mod_depth[n] +
          sv.pan_mod_adder[n];
        sv.pan_left[n]  = (1.0f - q) / 2.0f;
        sv.pan_right[n] = (1.0f + q) / 2.0f;
      }
      left  *= sv.pan_left[n];
      right *= sv.pan_right[n];

      int track = sv.record[n];
      if (track >= 1 && track <= RECORD_TRACK_MAX) {
        record_left[track] += left;
        record_right[track] += right;
      }

      if (sv.disconnect[n] == 0) {
        if (sv.delay_send[n] > 0.0f && delay_voice_can_send(engine, n)) {
          int bus = -1;
          if (track >= 1 && track <= RECORD_TRACK_MAX)
            bus = track - 1;
          if (bus >= 0 && bus < DELAY_BUS_COUNT)
            delay_input[bus] += sv.sample[n] * sv.delay_send[n];
        }
        sample_left  += left;
        sample_right += right;
      }
    }

    for (int bus = 0; bus < DELAY_BUS_COUNT; bus++) {
      if (delay_input[bus] != 0.0f) delay_bus[bus].active = 1;
      if (!delay_bus[bus].active) continue;
      float delay_left = 0.0f;
      float delay_right = 0.0f;
      delay_process(engine, &delay_bus[bus], delay_input[bus], &delay_left, &delay_right);
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

    // Adjust to main volume: smooth it otherwise is sounds crummy with realtime changes
    volume_smoother_gain += volume_smoother_smoothing * (volume_final - volume_smoother_gain);
    float volume_adjusted = volume_smoother_gain;

    sample_left  *= volume_adjusted;
    sample_right *= volume_adjusted;

    if (sample_state == SAMPLE_STATE_RECORDING &&
        atomic_load_int(&sampling.frames) > 0) {
      if (sampling.source == SAMPLE_SOURCE_MASTER) {
        size_t frame = (size_t)sampling.ptr * 2;
        sampling.where[frame] = sample_left;
        sampling.where[frame + 1] = sample_right;
      } else {
        sampling.where[sampling.ptr] =
          sampling.source == SAMPLE_SOURCE_VOICE ? record_voice : record_mono;
      }
      sampling.ptr++;
      int remaining = atomic_fetch_add_int(&sampling.frames, -1) - 1;
      if (remaining == 0) {
        sampling.len = sampling.ptr;
        atomic_store_int(&sampling.state, SAMPLE_STATE_COMPLETE);
      }
    }

    if (record_frames) {
      float *record_frame = record_frames + ((size_t)i * RECORD_CHANNELS);
      record_frame[0] = sample_left;
      record_frame[1] = sample_right;
      for (int track = 1; track <= RECORD_TRACK_MAX; track++) {
        int channel = track * AUDIO_CHANNELS;
        record_frame[channel] = record_left[track] * track_gain[track];
        record_frame[channel + 1] = record_right[track] * track_gain[track];
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
      output_frame[channel] = record_left[track] * track_gain[track];
      output_frame[channel + 1] = record_right[track] * track_gain[track];
    }
  }

  if (engine->ping_requested && num_frames > 0) {
    for (int channel = 0; channel < num_channels; channel++) {
      buffer[channel] = 1.0f;
    }
    engine->ping_requested = 0;
  }

  //BEN_MARK_B(bench, benchp, bencho);
}

void synth(skred_engine_t *engine, float *buffer, float *input, int num_frames, int num_channels,
           void *user) {
  synth_capture(engine, buffer, input, num_frames, num_channels,
                input ? AUDIO_CHANNELS : 0, user);
}

#undef sv
#define sv (skred_global_engine.sv)
#undef sw
#define sw (skred_global_engine.sw)
#undef synth_config
#define synth_config (skred_global_engine.config)
#undef synth_sample_rate
#define synth_sample_rate (skred_global_engine.sample_rate)
#undef synth_sample_count
#define synth_sample_count (skred_global_engine.sample_count)
#undef requested_synth_frames_per_callback
#define requested_synth_frames_per_callback (skred_global_engine.requested_frames_per_callback)
#undef synth_frames_per_callback
#define synth_frames_per_callback (skred_global_engine.frames_per_callback)
#undef volume_user
#define volume_user (skred_global_engine.volume_user)
#undef volume_final
#define volume_final (skred_global_engine.volume_final)
#undef volume_smoother_gain
#define volume_smoother_gain (skred_global_engine.volume_smoother_gain)
#undef volume_smoother_smoothing
#define volume_smoother_smoothing (skred_global_engine.volume_smoother_smoothing)
#undef volume_threshold
#define volume_threshold (skred_global_engine.volume_threshold)
#undef volume_smoother_higher_smoothing
#define volume_smoother_higher_smoothing (skred_global_engine.volume_smoother_higher_smoothing)

