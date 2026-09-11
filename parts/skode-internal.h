#include "miniaudio.h"
#pragma once

#include "skred.h"
#include "api.h"
#include "skode.h"
#include "midi_player.h"
#include "seq.h"

typedef struct {
    char      RIFFChunkID[4];
    uint32_t  RIFFChunkSize;
    char      Format[4];
    char      FormatSubchunkID[4];
    uint32_t  FormatSubchunkSize;
    uint16_t  AudioFormat;
    uint16_t  Channels;
    uint32_t  SamplesRate;
    uint32_t  ByteRate;
    uint16_t  BlockAlign;
    uint16_t  BitsPerSample;
    char      DataSubchunkID[4];
    uint32_t  DataSubchunkSize;
    // double* Data; 
} wav_t;


// interesting chunk that i might look for one day
typedef struct {
    char      RIFFChunkID[4];
    uint32_t  RIFFChunkSize;
    uint32_t  Manufacturer;
    uint32_t  Product;
    uint32_t  SamplePeriod;
    uint32_t  MIDIUnityNote;
    uint32_t  MIDIPitchFraction;
    uint32_t  SMPTEFormat;
    uint32_t  SMPTEOffset;
    uint32_t  SampleLoops;
    uint32_t  SamplerData;
    // hardcoded for one loop but SampleLoops by
    // the standard supports more than one
    // typedef struct {
    uint32_t Identifier;
    uint32_t Type;
    uint32_t Start;
    uint32_t End;
    uint32_t Fraction;
    uint32_t PlayCount;
    // } SampleLoop;
} sampler_t;

typedef struct {
    int found;
    int start;
    int end;
    int type;
    int play_count;
} mw_smpl_loop_t;



#include "synth-types.h"
#include "synth.h"
#include "synth-state.h"
#include "synth-config.h"
#include "control-events.h"
#include "polyphony.h"
#include "skode-dict.h"
#include "midi.h"

#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include "portable_win.h"
#else
#include <unistd.h>
#endif
#include <dirent.h>
#include "exp-vfs/skred_vfs.h"
#include "exp-vfs/miniz_zip.h"

#if defined(_WIN32) || defined(WIN32) || defined(__WIN32__) || defined(__WIN32) || defined(__WINDOWS__)
#ifndef SKODE_WINDOWS_BUILD
#define SKODE_WINDOWS_BUILD 1
#endif
#else
#ifndef SKODE_WINDOWS_BUILD
#define SKODE_WINDOWS_BUILD 0
#endif
#endif

#define SAMPLES_TO_MSEC(n) ((double)(n) * (double)1000.0 / (double)MAIN_SAMPLE_RATE)
#define WAVE_PEAK_ACCENT_MIN_DELTA 2
#define WTWFS(ms) if ((ms) >= 1000.0f) { \
    ctx->printf(ctx, " (%0.2fs)", (ms) / 1000.0f); \
  } else { \
    ctx->printf(ctx, " (%0.1fms)", (ms)); \
  }


typedef enum {
  SKODE_ASSET_ANY = 0,
  SKODE_ASSET_SKODE,
  SKODE_ASSET_WAVE,
  SKODE_ASSET_KSYNTH
} skode_asset_kind_t;

typedef struct {
  const char *key;
  const char *file;
  int line;
  const char *text;
} skode_doc_entry_t;

typedef struct {
    float min;
    float max;
    float peak;
    float dc;
    float rms;
    int zero_crossings;
    int clipped;
} wave_stats_t;

typedef struct { float re, im; } spectro_cplx_t;

typedef struct {
  unsigned char *data;
  size_t size;
  size_t capacity;
  int failed;
} skode_session_buffer_t;

typedef struct {
  uint32_t magic;
  uint32_t format;
  int32_t slot;
  int32_t length;
  float rate;
  int32_t one_shot;
} skode_session_wave_header_t;

typedef struct {
  uint32_t magic;
  uint32_t format;
  int32_t slot;
  int32_t length;
} skode_session_ks_header_t;

typedef struct {
  uint32_t magic;
  uint32_t format;
  int32_t num_vars;
  int32_t reserved;
} skode_session_env_header_t;

typedef struct {
  uint32_t id;
  uint32_t length;
  double value;
} skode_session_env_var_t;

// Extern functions and globals that will be shared between units.
// We will populate these as we extract modules.


int skode_session_save(skode_t *ctx, const char *filename);
int skode_session_load(skode_t *ctx, const char *filename);
#include "vendor/ksynth/ksynth.h"

#define STRING_BUF_IDX_MAX SKODE_EXTRA_MAX
#define STRING_BUF_LEN (256)

extern simple_mutex_t skode_ks_eval_mutex;
extern simple_mutex_t skode_extra_mutex;
extern char _skode_extra[STRING_BUF_IDX_MAX][STRING_BUF_LEN];

void global_status_show(skode_t *ctx, int full);
int skode_asset_read(const char *path, skode_asset_kind_t kind,
    void **data, size_t *size, char *resolved, size_t resolved_size);
void wave_install_memory(int wave_slot, float *table, int len,
    float rate, int one_shot, const char *name, float midi_note, float offset_hz);
extern synth_sample_t sampling;
int skode_sample_alloc(int frames);
int skode_load_buffer(skode_t *ctx, const char *data, size_t size,
    const char *resolved, int verbose);
void skode_ks_result_clear(skode_t *ctx);
ks_ctx *skode_ks_ctx(skode_t *ctx);

int skode_ks_eval(skode_t *ctx, char *cmd, int len);
int skode_ks_result_to_data(skode_t *ctx);
int skode_ks_bind_values(skode_t *ctx, int variable, const double *values, size_t len);
void ksynth_loader(skode_t *ctx, const char *text, size_t text_len, const char *label, int verbose);
int ksynth_load_name(skode_t *ctx, char *file, int verbose);
int ksynth_load(skode_t *ctx, int n, int verbose);
int skode_load_name(skode_t *ctx, const char *name, int verbose);
int skode_load(skode_t *ctx, int voice, int n, int verbose);
int wave_load_string(skode_t *ctx, char *name, int wave_index, int ch, int normalize);
int wave_load(skode_t *ctx, int file_num, int wave_index, int ch, int normalize);
int rec_load(skode_t *ctx, int wave_slot, int one_shot, int channel);
int data_load(skode_t *ctx, int wave_slot, int one_shot, float rate, float offset);
int skode_wave_valid(int wave);
void skode_copy_string(char *dst, size_t dst_size, const char *src);

void skode_envelope_velocity(int voice, float x, uint64_t now);
int skode_compile_scheduled(skode_t *ctx, const char *text, event_program_t *program);
void skode_queue_repeated(const event_program_t *program, int voice, int count, double seconds, int tag);
void skode_repeat_macro(skode_t *ctx, const double *arg, int argc, int delay_only);
int skode_foreign_function(skode_t *ctx, int index, const double *arg, int argc);
int skode_opcode_supported(skode_opcode_t opcode);
int skode_execute_voice_opcode(const opcode_event_t *opcode, int voice);
int skode_extra_valid(int n);
int skode_voice_valid(int voice);
int skode_seconds_to_samples(double seconds, uint64_t *out);
uint64_t skode_u64_add(uint64_t a, uint64_t b);

// Display and diagnostics prototypes
int wave_display_dim(double value, int fallback, int min, int max);
wave_stats_t wave_stats(float *data, int n);
void print_wave_stats(skode_t *ctx, const char *label, float *data, int n, float rate);
float wave_samples_to_ms(int samples, float rate);
int wave_boundary_col(int boundary, int n, int width);
void print_braille_cell(skode_t *ctx, unsigned int pattern);
void print_wave_marker_row(skode_t *ctx, int n, int width, int start, int end, int braille);
int skode_env_eq(const char *a, const char *b);
int skode_wave_display_use_braille(void);
int wave_y_from_value(float value, float max_abs, int rows);
int wave_ascii_points(float *data, int n, int width, int rows, int *y_coords, int *y_peak_min, int *y_peak_max);
void print_audio_ascii_wave(skode_t *ctx, float *data, int n, int width_chars, int height_chars, int offset, int trim, int labeled);
void print_audio_braille_connected(skode_t *ctx, float *data, int n, int width_chars, int height_chars);
void print_audio_braille_labeled(skode_t *ctx, float *data, int n, int width_chars, int height_chars, int offset, int trim);
int wavetable_show(skode_t *ctx, int n);
void wavetable_waveform_show(skode_t *ctx, int wave, int width, int height, int loop_start, int loop_end, const char *label);
void spectro_fft(spectro_cplx_t *buf, int n);
int spectro_next_pow2(int x);
int skode_spectrogram_color_mode(void);
void spectro_heat_rgb(float t, int *r, int *g, int *b);
void spectro_reset_color(skode_t *ctx, int mode);
int skode_spectrogram_line_budget(void);
int spectro_row_worst_case_bytes(int mode, int width, int use_braille);
int spectro_fit_color_mode(skode_t *ctx, int color_mode, int width, int use_braille);
void wavetable_spectrogram_show(skode_t *ctx, int wave, int width, int height, int loop_start, int loop_end, const char *label);
void voice_show(skode_t *ctx, int v, char c, int verbose);
int voice_show_all(skode_t *ctx, int voice, int verbose);
void record_tracks_show(skode_t *ctx);
void system_show(skode_t *ctx);
void skode_macros_show(skode_t *ctx, int pasteable);
void wave_labels_show(skode_t *ctx);
void global_status_show(skode_t *ctx, int full);
int show_stats_cb(int n, uint64_t timestamp, uint64_t id, int tag, const event_t *e, void *user);
void show_stats(skode_t *ctx);
void control_event_show(skode_t *ctx, int consume);
void opcode_arg_show(skode_t *ctx, const opcode_event_t *opcode, int n);
void opcode_show(skode_t *ctx, int index, const opcode_event_t *opcode);
void opcode_queue_show(skode_t *ctx);
void opcode_pattern_step_show(skode_t *ctx, int pattern, int step);
void opcode_pattern_show(skode_t *ctx, int pattern, int step);
void show_threads(skode_t *ctx);
void pattern_show(skode_t *ctx, int pattern_pointer, int verbose);

// Audio export prototypes
void normalize_buffer(float* pSamples, ma_uint32 frameCount, ma_uint32 channels);
int skode_write_wav(skode_t *ctx, const char *filename, const float *samples, int frames, uint32_t channels, uint32_t sample_rate, int normalize);
void record_find_trim(int argc, float arg0, float arg1, int margin);

#define RECORD_TRIM_DEFAULT_THRESHOLD 0.001f
#define RECORD_TRIM_CONSECUTIVE_SAMPLES 4
float record_frame_level(int frame);

// Word Registration Prototypes
void skode_register_words_dsp(skode_vocab_t *vocab);
void skode_register_words_seq(skode_vocab_t *vocab);
void skode_register_words_data(skode_vocab_t *vocab);
void skode_register_words_system(skode_vocab_t *vocab);
void skode_register_words_misc(skode_vocab_t *vocab);

#define WAVE_DISPLAY_DEFAULT_WIDTH 60
#define WAVE_DISPLAY_DEFAULT_HEIGHT 12
#define WAVE_DISPLAY_MIN_WIDTH 8
#define WAVE_DISPLAY_MAX_WIDTH 160
#define WAVE_DISPLAY_MIN_HEIGHT 2
#define WAVE_DISPLAY_MAX_HEIGHT 40

void wave_table_dynamic_expand(int n);
int skode_sample_go(int frames, int source, int voice);

#define EXTRA_PTR(n) skode_extra_ptr(n)
#define EXTRA_INIT() { _skode_extra_invalid[0] = '\0'; for (int i=0; i<STRING_BUF_IDX_MAX; i++) EXTRA_PTR(i)[0] = '\0';}
void skode_show(skode_t *ctx);
void skode_help(skode_t *ctx, double *arg, int argc);
float record_frame_mono(int frame);

void skode_double_dump(skode_t *ctx, double *data, int data_len);
void skode_format_string_args(char *dst, size_t dst_size, const char *src, double *arg, int argc);
char *skode_extra_ptr(int index);

float *mw_get(char *name, int *frames_out, wav_t *w, int ch);
float *mw_get_mem(const void *data, size_t data_size, const char *label,
                  int *frames_out, wav_t *w, int ch, char *out, int len);
int mw_get_smpl_loop_mem(const void *data, size_t data_size, int frames,
                         mw_smpl_loop_t *loop);
float *mw_free(float *f);
