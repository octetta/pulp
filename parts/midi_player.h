#ifndef MIDI_PLAYER_H
#define MIDI_PLAYER_H

#include <stdint.h>

void midi_player_load(int slot, const char *filename);
void midi_player_play(int slot);
void midi_player_stop(int slot);
void midi_player_sync(int slot, int mode);
void midi_player_dump(int slot, int start, int limit, void *ctx);
void midi_player_tick(uint64_t now_samples);

#endif

void midi_player_status(int slot, void *ctx);
void midi_player_seek(int slot, double tick);
