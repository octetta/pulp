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
  for (int w = WAVE_TABLE_SINE; w <= WAVE_TABLE_KRG16; w++) {
    if (wave_invalid(w)) continue;
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
      case WAVE_TABLE_CAP_1: name = "input-1-left"; break;
      case WAVE_TABLE_CAP_2: name = "input-1-right"; break;
      case WAVE_TABLE_CAP_3: name = "input-2-left"; break;
      case WAVE_TABLE_CAP_4: name = "input-2-right"; break;
      case WAVE_TABLE_CAP_5: name = "input-3-left"; break;
      case WAVE_TABLE_CAP_6: name = "input-3-right"; break;
      case WAVE_TABLE_CAP_7: name = "input-4-left"; break;
      case WAVE_TABLE_CAP_8: name = "input-4-right"; break;
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
