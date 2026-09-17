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
#include "esq1_waves.h"
#include "drums_waves.h"

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
  for (int w = WAVE_TABLE_SINE; w <= WAVE_TABLE_DRUM_18; w++) {
    if (wave_invalid(w)) continue;
    int size = SIZE_SINE;
    char *name = "?";
    switch (w) {
      case WAVE_TABLE_SINE:  name = "sine"; break;
      case WAVE_TABLE_COSINE: name = "cosine"; break;
      case WAVE_TABLE_CENTERED_PULSE: name = "centered pulse"; break;
      case WAVE_TABLE_KRG2_CENTERED: name = "centered krg2"; break;
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
      case WAVE_TABLE_ESQ1_01: name = "esq1-01"; break;
      case WAVE_TABLE_ESQ1_02: name = "esq1-02"; break;
      case WAVE_TABLE_ESQ1_03: name = "esq1-03"; break;
      case WAVE_TABLE_ESQ1_04: name = "esq1-04"; break;
      case WAVE_TABLE_ESQ1_05: name = "esq1-05"; break;
      case WAVE_TABLE_ESQ1_06: name = "esq1-06"; break;
      case WAVE_TABLE_ESQ1_07: name = "esq1-07"; break;
      case WAVE_TABLE_ESQ1_08: name = "esq1-08"; break;
      case WAVE_TABLE_ESQ1_09: name = "esq1-09"; break;
      case WAVE_TABLE_ESQ1_10: name = "esq1-10"; break;
      case WAVE_TABLE_ESQ1_11: name = "esq1-11"; break;
      case WAVE_TABLE_ESQ1_12: name = "esq1-12"; break;
      case WAVE_TABLE_ESQ1_13: name = "esq1-13"; break;
      case WAVE_TABLE_ESQ1_14: name = "esq1-14"; break;
      case WAVE_TABLE_ESQ1_15: name = "esq1-15"; break;
      case WAVE_TABLE_ESQ1_16: name = "esq1-16"; break;
      case WAVE_TABLE_ESQ1_17: name = "esq1-17"; break;
      case WAVE_TABLE_ESQ1_18: name = "esq1-18"; break;
      case WAVE_TABLE_ESQ1_19: name = "esq1-19"; break;
      case WAVE_TABLE_ESQ1_20: name = "esq1-20"; break;
      case WAVE_TABLE_ESQ1_21: name = "esq1-21"; break;
      case WAVE_TABLE_ESQ1_22: name = "esq1-22"; break;
      case WAVE_TABLE_ESQ1_23: name = "esq1-23"; break;
      case WAVE_TABLE_ESQ1_24: name = "esq1-24"; break;
      case WAVE_TABLE_ESQ1_25: name = "esq1-25"; break;
      case WAVE_TABLE_ESQ1_26: name = "esq1-26"; break;
      case WAVE_TABLE_ESQ1_27: name = "esq1-27"; break;
      case WAVE_TABLE_ESQ1_28: name = "esq1-28"; break;
      case WAVE_TABLE_ESQ1_29: name = "esq1-29"; break;
      case WAVE_TABLE_ESQ1_30: name = "esq1-30"; break;
      case WAVE_TABLE_ESQ1_31: name = "esq1-31"; break;
      case WAVE_TABLE_ESQ1_32: name = "esq1-32"; break;
      case WAVE_TABLE_DRUM_01: name = "drum-01"; size = WAVE_DRUM_01_SIZE; break;
      case WAVE_TABLE_DRUM_02: name = "drum-02"; size = WAVE_DRUM_02_SIZE; break;
      case WAVE_TABLE_DRUM_03: name = "drum-03"; size = WAVE_DRUM_03_SIZE; break;
      case WAVE_TABLE_DRUM_04: name = "drum-04"; size = WAVE_DRUM_04_SIZE; break;
      case WAVE_TABLE_DRUM_05: name = "drum-05"; size = WAVE_DRUM_05_SIZE; break;
      case WAVE_TABLE_DRUM_06: name = "drum-06"; size = WAVE_DRUM_06_SIZE; break;
      case WAVE_TABLE_DRUM_07: name = "drum-07"; size = WAVE_DRUM_07_SIZE; break;
      case WAVE_TABLE_DRUM_08: name = "drum-08"; size = WAVE_DRUM_08_SIZE; break;
      case WAVE_TABLE_DRUM_09: name = "drum-09"; size = WAVE_DRUM_09_SIZE; break;
      case WAVE_TABLE_DRUM_10: name = "drum-10"; size = WAVE_DRUM_10_SIZE; break;
      case WAVE_TABLE_DRUM_11: name = "drum-11"; size = WAVE_DRUM_11_SIZE; break;
      case WAVE_TABLE_DRUM_12: name = "drum-12"; size = WAVE_DRUM_12_SIZE; break;
      case WAVE_TABLE_DRUM_13: name = "drum-13"; size = WAVE_DRUM_13_SIZE; break;
      case WAVE_TABLE_DRUM_14: name = "drum-14"; size = WAVE_DRUM_14_SIZE; break;
      case WAVE_TABLE_DRUM_15: name = "drum-15"; size = WAVE_DRUM_15_SIZE; break;
      case WAVE_TABLE_DRUM_16: name = "drum-16"; size = WAVE_DRUM_16_SIZE; break;
      case WAVE_TABLE_DRUM_17: name = "drum-17"; size = WAVE_DRUM_17_SIZE; break;
      case WAVE_TABLE_DRUM_18: name = "drum-18"; size = WAVE_DRUM_18_SIZE; break;
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
        case WAVE_TABLE_KRG2_CENTERED: f = W02[(off + size * 3 / 4) % size]; break;
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
        case WAVE_TABLE_ESQ1_01: f = WAVE_ESQ1_01[off]; break;
        case WAVE_TABLE_ESQ1_02: f = WAVE_ESQ1_02[off]; break;
        case WAVE_TABLE_ESQ1_03: f = WAVE_ESQ1_03[off]; break;
        case WAVE_TABLE_ESQ1_04: f = WAVE_ESQ1_04[off]; break;
        case WAVE_TABLE_ESQ1_05: f = WAVE_ESQ1_05[off]; break;
        case WAVE_TABLE_ESQ1_06: f = WAVE_ESQ1_06[off]; break;
        case WAVE_TABLE_ESQ1_07: f = WAVE_ESQ1_07[off]; break;
        case WAVE_TABLE_ESQ1_08: f = WAVE_ESQ1_08[off]; break;
        case WAVE_TABLE_ESQ1_09: f = WAVE_ESQ1_09[off]; break;
        case WAVE_TABLE_ESQ1_10: f = WAVE_ESQ1_10[off]; break;
        case WAVE_TABLE_ESQ1_11: f = WAVE_ESQ1_11[off]; break;
        case WAVE_TABLE_ESQ1_12: f = WAVE_ESQ1_12[off]; break;
        case WAVE_TABLE_ESQ1_13: f = WAVE_ESQ1_13[off]; break;
        case WAVE_TABLE_ESQ1_14: f = WAVE_ESQ1_14[off]; break;
        case WAVE_TABLE_ESQ1_15: f = WAVE_ESQ1_15[off]; break;
        case WAVE_TABLE_ESQ1_16: f = WAVE_ESQ1_16[off]; break;
        case WAVE_TABLE_ESQ1_17: f = WAVE_ESQ1_17[off]; break;
        case WAVE_TABLE_ESQ1_18: f = WAVE_ESQ1_18[off]; break;
        case WAVE_TABLE_ESQ1_19: f = WAVE_ESQ1_19[off]; break;
        case WAVE_TABLE_ESQ1_20: f = WAVE_ESQ1_20[off]; break;
        case WAVE_TABLE_ESQ1_21: f = WAVE_ESQ1_21[off]; break;
        case WAVE_TABLE_ESQ1_22: f = WAVE_ESQ1_22[off]; break;
        case WAVE_TABLE_ESQ1_23: f = WAVE_ESQ1_23[off]; break;
        case WAVE_TABLE_ESQ1_24: f = WAVE_ESQ1_24[off]; break;
        case WAVE_TABLE_ESQ1_25: f = WAVE_ESQ1_25[off]; break;
        case WAVE_TABLE_ESQ1_26: f = WAVE_ESQ1_26[off]; break;
        case WAVE_TABLE_ESQ1_27: f = WAVE_ESQ1_27[off]; break;
        case WAVE_TABLE_ESQ1_28: f = WAVE_ESQ1_28[off]; break;
        case WAVE_TABLE_ESQ1_29: f = WAVE_ESQ1_29[off]; break;
        case WAVE_TABLE_ESQ1_30: f = WAVE_ESQ1_30[off]; break;
        case WAVE_TABLE_ESQ1_31: f = WAVE_ESQ1_31[off]; break;
        case WAVE_TABLE_ESQ1_32: f = WAVE_ESQ1_32[off]; break;
        case WAVE_TABLE_DRUM_01: f = WAVE_DRUM_01[off]; break;
        case WAVE_TABLE_DRUM_02: f = WAVE_DRUM_02[off]; break;
        case WAVE_TABLE_DRUM_03: f = WAVE_DRUM_03[off]; break;
        case WAVE_TABLE_DRUM_04: f = WAVE_DRUM_04[off]; break;
        case WAVE_TABLE_DRUM_05: f = WAVE_DRUM_05[off]; break;
        case WAVE_TABLE_DRUM_06: f = WAVE_DRUM_06[off]; break;
        case WAVE_TABLE_DRUM_07: f = WAVE_DRUM_07[off]; break;
        case WAVE_TABLE_DRUM_08: f = WAVE_DRUM_08[off]; break;
        case WAVE_TABLE_DRUM_09: f = WAVE_DRUM_09[off]; break;
        case WAVE_TABLE_DRUM_10: f = WAVE_DRUM_10[off]; break;
        case WAVE_TABLE_DRUM_11: f = WAVE_DRUM_11[off]; break;
        case WAVE_TABLE_DRUM_12: f = WAVE_DRUM_12[off]; break;
        case WAVE_TABLE_DRUM_13: f = WAVE_DRUM_13[off]; break;
        case WAVE_TABLE_DRUM_14: f = WAVE_DRUM_14[off]; break;
        case WAVE_TABLE_DRUM_15: f = WAVE_DRUM_15[off]; break;
        case WAVE_TABLE_DRUM_16: f = WAVE_DRUM_16[off]; break;
        case WAVE_TABLE_DRUM_17: f = WAVE_DRUM_17[off]; break;
        case WAVE_TABLE_DRUM_18: f = WAVE_DRUM_18[off]; break;
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
