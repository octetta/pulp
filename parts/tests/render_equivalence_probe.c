#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "synth.h"
#include "synth-state.h"
#include "synth-types.h"

enum {
  PROBE_FRAMES = 4096,
  PROBE_BLOCK = 128
};

static uint64_t hash_bytes(uint64_t hash, const void *data, size_t size) {
  const unsigned char *bytes = data;
  for (size_t i = 0; i < size; i++) {
    hash ^= bytes[i];
    hash *= UINT64_C(1099511628211);
  }
  return hash;
}

static void configure_patch(int mode) {
  wave_table_init(0);
  voice_init();
  volume_set(0.0f);

  if (mode == 0 || mode == 3) {
    wave_set(0, WAVE_TABLE_SINE);
    freq_set(0, 440.0f);
    amp_set(0, -3.0f);
    envelope_velocity(0, 1.0f);
  }
  if (mode == 1 || mode == 3) {
    wave_set(1, WAVE_TABLE_CAP_LEFT);
    amp_set(1, -6.0f);
    envelope_velocity(1, 1.0f);
  }
  if (mode == 2 || mode == 3) {
    wave_set(2, WAVE_TABLE_NOISE_ALT);
    amp_set(2, mode == 2 ? SILENT : -12.0f);
    envelope_velocity(2, 1.0f);
  }
}

int main(int argc, char **argv) {
  int mode = argc > 1 ? atoi(argv[1]) : 0;
  int loops = argc > 2 ? atoi(argv[2]) : 1;
  uint64_t expected = 0;
  if (argc > 3) expected = strtoull(argv[3], NULL, 16);
  if (mode < 0 || mode > 3 || loops < 1) return 2;

  float output[PROBE_BLOCK * AUDIO_CHANNELS];
  float input[PROBE_BLOCK * AUDIO_CHANNELS];
  for (int i = 0; i < PROBE_BLOCK; i++) {
    input[i * 2] = (float)(i - 64) / 64.0f;
    input[i * 2 + 1] = (float)(64 - i) / 64.0f;
  }

  uint64_t hash = UINT64_C(1469598103934665603);
  struct timespec start;
  struct timespec end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  for (int loop = 0; loop < loops; loop++) {
    synth_init(8);
    configure_patch(mode);
    for (int frame = 0; frame < PROBE_FRAMES; frame += PROBE_BLOCK) {
      if (mode == 2 && frame == PROBE_FRAMES / 2) amp_set(2, -12.0f);
      memset(output, 0, sizeof(output));
      synth(output, input, PROBE_BLOCK, AUDIO_CHANNELS, NULL);
      hash = hash_bytes(hash, output, sizeof(output));
    }
    hash = hash_bytes(hash, sv.phase, 8 * sizeof(*sv.phase));
    hash = hash_bytes(hash, sv.sample, 8 * sizeof(*sv.sample));
    synth_free();
  }

  clock_gettime(CLOCK_MONOTONIC, &end);
  double milliseconds =
    (double)(end.tv_sec - start.tv_sec) * 1000.0 +
    (double)(end.tv_nsec - start.tv_nsec) / 1000000.0;
  printf("mode=%d loops=%d hash=%016llx ms=%.3f\n",
         mode, loops, (unsigned long long)hash, milliseconds);
  if (expected && hash != expected) {
    fprintf(stderr, "expected hash %016llx\n", (unsigned long long)expected);
    return 1;
  }
  return 0;
}
