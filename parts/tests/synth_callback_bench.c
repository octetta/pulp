#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "synth.h"
#include "synth-state.h"
#include "synth-types.h"

enum {
  BENCH_FRAMES = 128,
  BENCH_CHANNELS = 2,
  BENCH_WARMUP_CALLBACKS = 100
};

static uint64_t elapsed_ns(const struct timespec *start,
                           const struct timespec *end) {
  return (uint64_t)(end->tv_sec - start->tv_sec) * UINT64_C(1000000000) +
         (uint64_t)(end->tv_nsec - start->tv_nsec);
}

int main(int argc, char **argv) {
  int voices = argc > 1 ? atoi(argv[1]) : 32;
  int callbacks = argc > 2 ? atoi(argv[2]) : 5000;
  if (voices < 1 || callbacks < 1) return 2;

  float output[BENCH_FRAMES * BENCH_CHANNELS];
  const double deadline_ns =
    (double)BENCH_FRAMES * 1000000000.0 / MAIN_SAMPLE_RATE;
  uint64_t total_ns = 0;
  uint64_t worst_ns = 0;
  int overruns = 0;

  synth_init(voices);
  wave_table_init(0);
  voice_init();
  voices = synth_voice_count();
  for (int voice = 0; voice < voices; voice++) {
    amp_set(voice, -12.0f);
    freq_set(voice, 110.0f + (float)voice);
    envelope_velocity(voice, 1.0f);
  }

  for (int i = 0; i < BENCH_WARMUP_CALLBACKS; i++)
    synth(output, NULL, BENCH_FRAMES, BENCH_CHANNELS, NULL);

  for (int i = 0; i < callbacks; i++) {
    struct timespec start;
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    synth(output, NULL, BENCH_FRAMES, BENCH_CHANNELS, NULL);
    clock_gettime(CLOCK_MONOTONIC, &end);

    uint64_t callback_ns = elapsed_ns(&start, &end);
    total_ns += callback_ns;
    if (callback_ns > worst_ns) worst_ns = callback_ns;
    if ((double)callback_ns > deadline_ns) overruns++;
  }

  double average_us = (double)total_ns / callbacks / 1000.0;
  double worst_us = (double)worst_ns / 1000.0;
  double deadline_us = deadline_ns / 1000.0;
  printf("voices=%d callbacks=%d average_us=%.3f worst_us=%.3f "
         "deadline_us=%.3f load=%.2f%% overruns=%d\n",
         voices, callbacks, average_us, worst_us, deadline_us,
         average_us * 100.0 / deadline_us, overruns);

  synth_free();
  return 0;
}
