#ifndef _SYNTH_H_
#define _SYNTH_H_

#include "synth-config.h"
#include "synth-state.h"

#include "portable_atomic.h"

//extern atomic_uint64_t synth_sample_count;
#define SAMPLE_COUNT_PUT(n) atomic_store_uint64(&synth_sample_count, n)
#define SAMPLE_COUNT_GET() atomic_load_uint64(&synth_sample_count)
#define SAMPLE_COUNT_ADD(n) atomic_fetch_add_uint64(&synth_sample_count, n)

void synth(skred_engine_t *engine, float *buffer, float *input, int num_frames, int num_channels, void *user);
void synth_capture(skred_engine_t *engine, float *buffer, float *input, int num_frames,
                   int output_channels, int input_channels, void *user);
void synth_init(int vc);
void synth_free(void);

int synth_sample_rate_set(int sample_rate);
int synth_sample_rate_get(void);

      int wave_quant(int voice, int n);
      float quantize_bits_curve(float v, int bits, int curve, uint64_t *rng);

      void mmf_init(skred_engine_t *engine, int, float, float);
      void mmf_set_params(skred_engine_t *engine, int, float, float);
      int  mmf_set_freq(skred_engine_t *engine, int, float);
      int  mmf_set_res(skred_engine_t *engine, int, float);
      float mmf_process(skred_engine_t *engine, int n, float input);

      float cz_phasor(int n, float p, float d, float active_start, float active_end);
      int   cz_set(int v, int n, float f);
      int   cmod_set(int voice, int o, float f);

      void  envelope_init(int v, float a, float d, float s, float r);
      void  amp_envelope_trigger(int v, float f);
      void  amp_envelope_release(int v);
      float amp_envelope_step(int v, uint64_t current_sample);
      int   envelope_is_flat(int v);
      int   envelope_set(int voice, float a, float d, float s, float r);
      int   envelope_velocity(int voice, float f);

void audio_rng_init(uint64_t *rng, uint64_t seed);
uint64_t audio_rng_next(uint64_t *rng);
float audio_rng_float(uint64_t *rng);
float osc_get_phase_inc(skred_engine_t *engine, int v, float f);
void osc_set_freq(skred_engine_t *engine, int v, float f);
float osc_next(skred_engine_t *engine, int voice, float phase_inc);
void osc_set_wave_table_index(skred_engine_t *engine, int voice, int wave);
void osc_trigger(skred_engine_t *engine, int voice);

int volume_set(float v);
float volume_get(void);

int synth_record_track_set(int voice, int track);
int synth_record_track_get(int voice);
int synth_track_volume_set(int track, float db);
float synth_track_volume_db_get(int track);
float synth_track_volume_linear_get(int track);
int synth_track_name_set(int track, const char *name);
const char *synth_track_name_get(int track);

int amp_set(int v, float f);
int pan_set(int voice, float f);
int delay_send_set(skred_engine_t *engine, int voice, float amount);
int delay_params_set(skred_engine_t *engine, int bus, int coarse, int fine, int feedback, int mod_freq,
                     int mod_depth, int level);
void delay_params_get(skred_engine_t *engine, int bus, int *coarse, int *fine, int *feedback, int *mod_freq,
                      int *mod_depth, int *level);
int delay_damping_set(skred_engine_t *engine, int bus, int damping, int hp);
void delay_damping_get(skred_engine_t *engine, int bus, int *damping, int *hp);
int delay_freeze_set(skred_engine_t *engine, int bus, int on);
int delay_freeze_get(skred_engine_t *engine, int bus);
int delay_pingpong_set(skred_engine_t *engine, int bus, int on);
int delay_pingpong_get(skred_engine_t *engine, int bus);
int delay_grit_set(skred_engine_t *engine, int bus, int bits, int native);
void delay_grit_get(skred_engine_t *engine, int bus, int *bits, int *native);
int delay_time_ms_set(skred_engine_t *engine, int bus, float target_ms);
int delay_time_sync_set(skred_engine_t *engine, int bus, float bpm, float division);
void delay_clear(skred_engine_t *engine);
const char *delay_bus_format(int bus);
void delay_clear(skred_engine_t *engine);
const char *delay_bus_format(int bus);
const char *delay_format(void);
const char *delay_status(void);
void synth(skred_engine_t *engine, float *buffer, float *input, int num_frames, int num_channels, void *user);
void synth_capture(skred_engine_t *engine, float *buffer, float *input, int num_frames,
                   int output_channels, int input_channels, void *user);
int freq_set(int v, float f);
int voice_set(int n, int *old_voice);
int voice_control_events_set(int voice, int enabled);
int voice_copy(int v, int n);
int wave_set(int voice, int wave);
void osc_reclassify(skred_engine_t *engine, int voice);
int wave_mute(int voice, int state);
int wave_dir(int voice, int state);
int freq_midi(int voice, float note, float cents);
int freq_bend_set(int voice, float val);
int freq_bend_param_set(int voice, float range, float offset);
int amp_bend_set(int voice, float val);
int amp_bend_param_set(int voice, float range, float offset);

int amp_mod_set(int voice, int o, float f, float a);

int wave_reset(int voice);

int freq_mod_set(int voice, int o, float f, float a);
int freq_mod_mode_set(int voice, int mode);
int freq_feedback_set(int voice, float amount);

int pan_mod_set(int voice, int o, float f, float a);

char *voice_format(int v, char *out, size_t out_size, int verbose);
int voice_trigger(int voice);
int wave_default(int voice);
int wave_loop(int voice, int state);
int wave_loop_count(int voice, int count);
int wave_loop_points_set(int wave, int start, int end);
int voice_wave_range_set(int voice, int start, int end);
int voice_wave_range_reset(int voice);
int voice_loop_points_set(int voice, int start, int end);
int voice_loop_points_reset(int voice);
int voice_copy(int v, int n);
float midi2hz(float f);
int voice_set(int n, int *old_voice);
int voice_trigger(int voice);
int wave_default(int voice);
void wave_table_init(int flag);
void wave_free(void);
void wave_free_one(int i);
void voice_init(void);

char *synth_stats(void);
void synth_voice_bench(int voice);

void normalize_preserve_zero(float *data, int length);

void envelope_init_e(envelope_t *e, float a, float d, float s, float r);
void envelope_configure_e(envelope_t *e, float a, float d, float s, float r);
void envelope_trigger_e(envelope_t *e, float f);
void envelope_release_e_at(envelope_t *e, uint64_t current_sample);
void envelope_schedule_release_e_at(envelope_t *e, uint64_t release_sample);
void envelope_release_e(envelope_t *e);
float envelope_step_e(envelope_t *e, uint64_t current_sample);

#endif
