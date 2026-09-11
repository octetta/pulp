#pragma once

#include "skred.h"
#include "api.h"
#include "skode.h"
#include "midi_player.h"
#include "seq.h"
#include "miniwav.h"

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

