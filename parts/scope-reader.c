#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "scope-ipc.h"

int main(int argc, char **argv) {
  const char *name = argc > 1 ? argv[1] : SKRED_SCOPE_DEFAULT_NAME;
  uint32_t requested = argc > 2 ? (uint32_t)strtoul(argv[2], NULL, 0) : 2048;
  int iterations = argc > 3 ? atoi(argv[3]) : 0;
  if (requested == 0) requested = 2048;

  skred_scope_reader_t reader;
  if (scope_ipc_reader_open(&reader, name) != 0) {
    fprintf(stderr, "cannot open scope [%s]\n", name);
    return 1;
  }

  if (requested > reader.header->capacity_frames)
    requested = reader.header->capacity_frames;
  float *frames = malloc((size_t)requested * RECORD_CHANNELS * sizeof(float));
  if (!frames) {
    scope_ipc_reader_close(&reader);
    return 1;
  }

  printf("name=%s version=%u generation=%" PRIu64
         " rate=%u channels=%u mask=%u capacity=%u\n",
         reader.name, reader.header->version, reader.header->generation,
         reader.header->sample_rate, reader.header->channel_count,
         reader.header->channel_mask, reader.header->capacity_frames);

  struct timespec delay = {0, 16000000};
  int pass = 0;
  while (iterations == 0 || pass < iterations) {
    uint64_t first_frame = 0;
    int count = scope_ipc_reader_latest(&reader, frames, requested,
                                        &first_frame);
    if (count < 0) break;
    if (count > 0) {
      printf("frame=%" PRIu64 " count=%d", first_frame, count);
      for (uint32_t channel = 0; channel < reader.header->channel_count;
           channel++) {
        if (!(reader.header->channel_mask & (UINT32_C(1) << channel)))
          continue;
        double sum_squares = 0.0;
        float peak = 0.0f;
        for (int frame = 0; frame < count; frame++) {
          float value = frames[(size_t)frame * RECORD_CHANNELS + channel];
          float absolute = fabsf(value);
          if (absolute > peak) peak = absolute;
          sum_squares += (double)value * value;
        }
        printf(" ch%u_peak=%g ch%u_rms=%g", channel, peak, channel,
               sqrt(sum_squares / count));
      }
      printf("\n");
      fflush(stdout);
    }
    pass++;
    if (iterations == 0 || pass < iterations) nanosleep(&delay, NULL);
  }

  free(frames);
  scope_ipc_reader_close(&reader);
  return 0;
}
