#ifndef SKRED_CONTROL_EVENTS_H
#define SKRED_CONTROL_EVENTS_H

#include <stdint.h>

#include "api.h"

void skred_control_voice_source(int voice, int pattern, int step, int tag,
  uint32_t opcode);
void skred_control_voice_reset(int voice);
void skred_control_voice_event(uint32_t type, uint64_t sample, int voice);
void skred_control_user_event(uint64_t sample, int voice, int pattern,
  int step, int tag, uint32_t opcode, int id, int value_count,
  const double *value);
void skred_control_pattern_event(uint32_t type, uint64_t sample, int pattern,
  int step);
void skred_control_midi_event(int type, int channel, int data1, int data2);

#endif
