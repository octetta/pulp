#include "synth.h"
#include "synth-internal.h"
#include "util.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "control-events.h"
#define SIZE_SINE (4096)
#include "dwg.h"
#include "vendor/ksynth/ksynth.h"
#include "dwg_sources.h"
#include "esq1_sources.h"
#include "drums_sources.h"

static void synth_eval_and_register_ks_wave(int w, const char *name, const char *ks_code, int is_drum) {
  if (wave_invalid(w)) return;

  if (is_drum) {
    // Reset PRNG seed to 1 so noise generation in ksynth ('r') is 100% deterministic
    srand(1);
  }

  size_t mem_limit = is_drum ? (16 * 1024 * 1024) : (4 * 1024 * 1024);
  ks_ctx *ctx = ks_create(mem_limit, 1000000000, 44100.0);
  if (!ctx) return;

  K result = NULL;
  const char *text = ks_code;
  size_t text_len = strlen(text);
  size_t pos = 0;

  while (pos < text_len) {
    char line[1024];
    size_t start = pos;
    while (pos < text_len && text[pos] != '\n' && text[pos] != '\r') pos++;
    size_t len = pos - start;
    while (pos < text_len && (text[pos] == '\n' || text[pos] == '\r')) pos++;
    if (len == 0) continue;
    if (len >= sizeof(line)) len = sizeof(line) - 1;
    memcpy(line, text + start, len);
    line[len] = '\0';
    result = ks_eval(ctx, line, len);
    if (ctx->last_status != KS_OK) {
      result = NULL;
      break;
    }
  }

  if (result && result->n > 0) {
    int size = result->n;
    float *buf = (float *)malloc(size * sizeof(float));
    if (buf) {
      for (int s = 0; s < size; s++) {
        buf[s] = (float)result->f[s];
      }

      strncpy(sw.name[w], name, WAVE_NAME_MAX);
      sw.data[w] = buf;
      sw.size[w] = size;
      sw.rate[w] = 44100.0f;
      sw.is_heap[w] = 1;
      sw.readonly[w] = 1;

      K k_o = k_get(ctx, 'O');
      int one_shot = (k_o && k_o->n == 1 && k_o->f[0] > 0) ? 1 : 0;
      sw.one_shot[w] = one_shot;

      K k_s = k_get(ctx, 'S');
      if (k_s && k_s->n == 1) sw.rate[w] = (float)k_s->f[0];

      K k_l = k_get(ctx, 'L');
      sw.loop_start[w] = (k_l && k_l->n == 1) ? (int)k_l->f[0] : 0;

      K k_e = k_get(ctx, 'E');
      sw.loop_end[w] = (k_e && k_e->n == 1) ? (int)k_e->f[0] : (one_shot ? 0 : size - 1);
      if (one_shot) {
        sw.loop_start[w] = 0;
        sw.loop_end[w] = 0;
      }
    }
  }

  ks_destroy(ctx);
}

static void synth_init_ksynth_dwg(void) {
  for (int idx = 0; DWG_PATCH_SOURCES[idx].slot != 0; idx++) {
    const dwg_patch_source_t *p = &DWG_PATCH_SOURCES[idx];
    synth_eval_and_register_ks_wave(p->slot, p->name, p->ks_code, 0);
  }
}

static void synth_init_centered_krg2(void) {
  int src = WAVE_TABLE_KRG2;
  int dst = WAVE_TABLE_KRG2_CENTERED;
  if (sw.data[src] && sw.size[src] > 0) {
    int sz = sw.size[src];
    float *buf = (float *)malloc(sz * sizeof(float));
    if (buf) {
      for (int off = 0; off < sz; off++) {
        buf[off] = sw.data[src][(off + sz * 3 / 4) % sz];
      }
      strncpy(sw.name[dst], "centered krg2", WAVE_NAME_MAX);
      sw.data[dst] = buf;
      sw.size[dst] = sz;
      sw.rate[dst] = sw.rate[src];
      sw.is_heap[dst] = 1;
      sw.readonly[dst] = 1;
      sw.one_shot[dst] = 0;
      sw.loop_start[dst] = 0;
      sw.loop_end[dst] = sz - 1;
    }
  }
}

static void synth_init_ksynth_esq1(void) {
  for (int idx = 0; ESQ1_PATCH_SOURCES[idx].slot != 0; idx++) {
    const esq1_patch_source_t *p = &ESQ1_PATCH_SOURCES[idx];
    synth_eval_and_register_ks_wave(p->slot, p->name, p->ks_code, 0);
  }
}

static void synth_init_ksynth_drums(void) {
  for (int idx = 0; DRUM_PATCH_SOURCES[idx].slot != 0; idx++) {
    const drum_patch_source_t *p = &DRUM_PATCH_SOURCES[idx];
    synth_eval_and_register_ks_wave(p->slot, p->name, p->ks_code, 1);
  }
}

void wave_table_init(int flag) {
  (void)flag;

  for (int i = 0 ; i < synth_config.wave_table_max; i++) {
    sw.data[i] = NULL;
    sw.size[i] = 0;
    sw.is_heap[i] = 0;
    sw.direction[i] = 0;
    sw.readonly[i] = 0;
    sw.refcount[i] = 0;
  }

  uint64_t white_noise;
  audio_rng_init(&white_noise, 1);
  for (int w = WAVE_TABLE_SINE; w <= WAVE_TABLE_KRG2_CENTERED; w++) {
    if (wave_invalid(w)) continue;
    if (w >= WAVE_TABLE_KRG1 && w <= WAVE_TABLE_KRG16) continue;
    if (w == WAVE_TABLE_KRG2_CENTERED) continue;
    int size = SIZE_SINE;
    char *name = "?";
    switch (w) {
      case WAVE_TABLE_SINE:  name = "sine"; break;
      case WAVE_TABLE_COSINE: name = "cosine"; break;
      case WAVE_TABLE_CENTERED_PULSE: name = "centered pulse"; break;
      case WAVE_TABLE_SQR:   name = "square"; break;
      case WAVE_TABLE_SAW_DOWN: name = "saw-down"; break;
      case WAVE_TABLE_SAW_UP: name = "saw-up"; break;
      case WAVE_TABLE_TRI:   name = "triangle"; break;
      case WAVE_TABLE_NOISE: name = "noise"; break;
      case WAVE_TABLE_NOISE_ALT: name = "noise-alt"; break; // not used, here for laziness in experiment
      case WAVE_TABLE_CAP_1: name = "input-1-left"; break;
      case WAVE_TABLE_CAP_2: name = "input-1-right"; break;
      case WAVE_TABLE_CAP_3: name = "input-2-left"; break;
      case WAVE_TABLE_CAP_4: name = "input-2-right"; break;
      case WAVE_TABLE_CAP_5: name = "input-3-left"; break;
      case WAVE_TABLE_CAP_6: name = "input-3-right"; break;
      case WAVE_TABLE_CAP_7: name = "input-4-left"; break;
      case WAVE_TABLE_CAP_8: name = "input-4-right"; break;
      case WAVE_TABLE_KRG17: name = "krg-17"; size = 2048; break;
      case WAVE_TABLE_KRG18: name = "krg-18"; size = 2048; break;
      case WAVE_TABLE_KRG19: name = "krg-19"; size = 2048; break;
      case WAVE_TABLE_KRG20: name = "krg-20"; size = 2048; break;
      case WAVE_TABLE_KRG21: name = "krg-21"; size = 2048; break;
      case WAVE_TABLE_KRG22: name = "krg-22"; size = 2048; break;
      case WAVE_TABLE_KRG23: name = "krg-23"; size = 2048; break;
      case WAVE_TABLE_KRG24: name = "krg-24"; size = 2048; break;
      case WAVE_TABLE_KRG25: name = "krg-25"; size = 2048; break;
      case WAVE_TABLE_KRG26: name = "krg-26"; size = 2048; break;
      case WAVE_TABLE_KRG27: name = "krg-27"; size = 2048; break;
      case WAVE_TABLE_KRG28: name = "krg-28"; size = 2048; break;
      case WAVE_TABLE_KRG29: name = "krg-29"; size = 2048; break;
      case WAVE_TABLE_KRG30: name = "krg-30"; size = 2048; break;
      case WAVE_TABLE_KRG31: name = "krg-31"; size = 2048; break;
      case WAVE_TABLE_KRG32: name = "krg-32"; size = 2048; break;
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
        case WAVE_TABLE_COSINE: f = -cosf(2.0f * (float) M_PI * phase); break;
        case WAVE_TABLE_CENTERED_PULSE: f = (phase > 0.25f && phase < 0.75f) ? 1.0f : -1.0f; break;
        case WAVE_TABLE_SQR: f = (phase < 0.5) ? 1.0f : -1.0f; break;
        case WAVE_TABLE_SAW_DOWN: f = 2.0f * phase - 1.0f; break;
        case WAVE_TABLE_SAW_UP: f = 1.0f - 2.0f * phase; break;
        case WAVE_TABLE_TRI: f = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase); break;
        case WAVE_TABLE_NOISE: f = audio_rng_float(&white_noise); break;
        case WAVE_TABLE_NOISE_ALT: f = audio_rng_float(&white_noise); break;
        case WAVE_TABLE_KRG17: f = W17[off]; break;
        case WAVE_TABLE_KRG18: f = W18[off]; break;
        case WAVE_TABLE_KRG19: f = W19[off]; break;
        case WAVE_TABLE_KRG20: f = W20[off]; break;
        case WAVE_TABLE_KRG21: f = W21[off]; break;
        case WAVE_TABLE_KRG22: f = W22[off]; break;
        case WAVE_TABLE_KRG23: f = W23[off]; break;
        case WAVE_TABLE_KRG24: f = W24[off]; break;
        case WAVE_TABLE_KRG25: f = W25[off]; break;
        case WAVE_TABLE_KRG26: f = W26[off]; break;
        case WAVE_TABLE_KRG27: f = W27[off]; break;
        case WAVE_TABLE_KRG28: f = W28[off]; break;
        case WAVE_TABLE_KRG29: f = W29[off]; break;
        case WAVE_TABLE_KRG30: f = W30[off]; break;
        case WAVE_TABLE_KRG31: f = W31[off]; break;
        case WAVE_TABLE_KRG32: f = W32[off]; break;
        default: f = 0; break;
      }
      sw.data[w][off++] = f;
      phase += delta;
    }
  }

  synth_init_ksynth_dwg();
  synth_init_centered_krg2();
  synth_init_ksynth_esq1();
  synth_init_ksynth_drums();
}


void wave_free_one(int i) {
  if (wave_invalid(i)) return;
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
  for (int i = 0; i < synth_config.wave_table_max; i++) wave_free_one(i);
}
