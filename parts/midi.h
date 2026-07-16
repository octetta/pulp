#ifndef SKRED_MIDI_H
#define SKRED_MIDI_H

#include <stdint.h>
#include "api.h"

/* Internal bridge from the MIDI callback into api.c's bounded event ring. */
void skred_control_midi_event(int type, int channel, int data1, int data2);

/* Runtime command router. Returns 0 when line is not a MIDI command. */
int skred_midi_command(const char *line);

/* Hardware-free test seam: applies the public event mask and publishes. */
void skred_midi_test_inject(int type, int channel, int data1, int data2);

/* Called only by the control dispatcher after it consumes a MIDI event. */
int skred_midi_route_event(const skred_control_event_t *event);

/* Execute expanded MIDI bindings in api.c's dedicated dispatcher context. */
int skred_control_midi_command(const char *command);

#endif
