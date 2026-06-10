#ifndef SKRED_RECORDER_H
#define SKRED_RECORDER_H

#include <stdint.h>

#include "synth-types.h"


enum {
  RECORDER_STOPPED = 0,
  RECORDER_RECORDING,
  RECORDER_STOPPING,
  RECORDER_ERROR
};

int recorder_init(int max_block_frames, int sample_rate);
void recorder_uninit(void);
int recorder_start(const char *filename, double max_seconds);
int recorder_stop(void);
int recorder_state(void);
uint64_t recorder_frames_written(void);
uint64_t recorder_dropped_frames(void);
synth_record_bus_t *recorder_begin_block(int frame_count);
void recorder_end_block(int frame_count);


#endif
