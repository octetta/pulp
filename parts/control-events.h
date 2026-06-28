#ifndef SKRED_CONTROL_EVENTS_H
#define SKRED_CONTROL_EVENTS_H

#include <stdint.h>

#include "api.h"

void skred_control_voice_source(int voice, int pattern, int step, int tag,
                                uint32_t opcode);
void skred_control_voice_event(uint32_t type, uint64_t sample, int voice);

#endif
