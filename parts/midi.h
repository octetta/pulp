#ifndef SKRED_MIDI_H
#define SKRED_MIDI_H

#include <stdint.h>

/* Internal bridge from the MIDI callback into api.c's bounded event ring. */
void skred_control_midi_event(int type, int channel, int data1, int data2);

/* Runtime command router. Returns 0 when line is not a MIDI command. */
int skred_midi_command(const char *line);

/* Hardware-free test seam: applies the public event mask and publishes. */
void skred_midi_test_inject(int type, int channel, int data1, int data2);

#endif
