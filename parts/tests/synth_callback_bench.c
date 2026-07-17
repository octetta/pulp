#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

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
  const char *scenario = argc > 3 ? argv[3] : "off";
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
#ifdef SKRED_BENCH_FM
    if (strcmp(scenario, "fm-phase") == 0) {
      freq_mod_mode_set(voice, 2);
      if (voice > 0) freq_mod_set(voice, voice - 1, 4.0f, 0.0f);
    } else if (strcmp(scenario, "fm-feedback") == 0) {
      freq_mod_mode_set(voice, 2);
      freq_feedback_set(voice, 3.0f);
    } else if (strcmp(scenario, "fm-all") == 0) {
      freq_mod_mode_set(voice, 2);
      if (voice > 0) freq_mod_set(voice, voice - 1, 4.0f, 0.0f);
      freq_feedback_set(voice, 3.0f);
    } else
#endif
#ifdef SKRED_BENCH_PD
    if (strcmp(scenario, "static") == 0) {
      cz_set(voice, 6, 0.6f);
    } else if (strcmp(scenario, "envelope") == 0) {
      cz_set(voice, 6, -0.2f);
      envelope_configure_e(&sv.cz_envelope[voice], 0.01f, 0.3f, 0.5f, 0.4f);
      sv.use_cz_envelope[voice] = 1;
      sv.cz_env_depth[voice] = 0.8f;
    } else if (strcmp(scenario, "mod") == 0) {
      cz_set(voice, 6, 0.1f);
      cmod_set(voice, (voice + 1) % voices, 0.7f);
    } else if (strcmp(scenario, "all") == 0) {
      cz_set(voice, 6, -0.2f);
      envelope_configure_e(&sv.cz_envelope[voice], 0.01f, 0.3f, 0.5f, 0.4f);
      sv.use_cz_envelope[voice] = 1;
      sv.cz_env_depth[voice] = 0.8f;
      cmod_set(voice, (voice + 1) % voices, 0.7f);
    } else if (strcmp(scenario, "off") != 0) {
      fprintf(stderr, "unknown scenario: %s\n", scenario);
      synth_free();
      return 2;
    }
#else
    if (strcmp(scenario, "off") != 0) {
      fprintf(stderr, "requested benchmark feature is not enabled\n");
      synth_free();
      return 2;
    }
#endif
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
  printf("scenario=%s voices=%d callbacks=%d average_us=%.3f worst_us=%.3f "
         "deadline_us=%.3f load=%.2f%% overruns=%d\n",
         scenario, voices, callbacks, average_us, worst_us, deadline_us,
         average_us * 100.0 / deadline_us, overruns);

  synth_free();
  return 0;
}
