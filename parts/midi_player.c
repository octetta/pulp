#include "midi_player.h"
#include "midi.h"
#include "seq.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SLOTS 4
#define MAX_EVENTS 65536

typedef struct {
    int type;
    int ch;
    int d1;
    int d2;
    uint32_t abs_time;
} smf_event_t;

typedef struct {
    int active;
    int playing;
    int sync_mode;
    int ppq;
    float natural_bpm;
    uint32_t channel_mask;
    int max_poly[16];
    int note_count;
    smf_event_t *events;
    int event_count;
    int event_capacity;
    
    int current_event_idx;
    double current_tick;
    uint64_t last_sample;
} midi_slot_t;

static midi_slot_t slots[MAX_SLOTS] = {0};

static uint32_t read_vlq(uint8_t **p) {
    uint32_t val = 0;
    while (1) {
        uint8_t b = *(*p)++;
        val = (val << 7) | (b & 0x7F);
        if (!(b & 0x80)) break;
    }
    return val;
}

void midi_player_load(int slot, const char *filename) {
    if (slot < 0 || slot >= MAX_SLOTS) return;
    FILE *f = fopen(filename, "rb");
    if (!f) return;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t *data = malloc(size);
    if (fread(data, 1, size, f) != size) { 
    // Calculate max polyphony per channel
    int active[16][128] = {0};
    int current_poly[16] = {0};
    for (int i = 0; i < slots[slot].event_count; i++) {
        smf_event_t *e = &slots[slot].events[i];
        if (e->type == 9 && e->d2 > 0) {
            if (!active[e->ch][e->d1]) {
                active[e->ch][e->d1] = 1;
                current_poly[e->ch]++;
                if (current_poly[e->ch] > slots[slot].max_poly[e->ch]) {
                    slots[slot].max_poly[e->ch] = current_poly[e->ch];
                }
            }
        } else if (e->type == 8 || (e->type == 9 && e->d2 == 0)) {
            if (active[e->ch][e->d1]) {
                active[e->ch][e->d1] = 0;
                current_poly[e->ch]--;
            }
        }
    }

    free(data); fclose(f); return; }
    fclose(f);

    if (memcmp(data, "MThd", 4) != 0) { 
    // Calculate max polyphony per channel
    int active[16][128] = {0};
    int current_poly[16] = {0};
    for (int i = 0; i < slots[slot].event_count; i++) {
        smf_event_t *e = &slots[slot].events[i];
        if (e->type == 9 && e->d2 > 0) {
            if (!active[e->ch][e->d1]) {
                active[e->ch][e->d1] = 1;
                current_poly[e->ch]++;
                if (current_poly[e->ch] > slots[slot].max_poly[e->ch]) {
                    slots[slot].max_poly[e->ch] = current_poly[e->ch];
                }
            }
        } else if (e->type == 8 || (e->type == 9 && e->d2 == 0)) {
            if (active[e->ch][e->d1]) {
                active[e->ch][e->d1] = 0;
                current_poly[e->ch]--;
            }
        }
    }

    free(data); return; }
    
    if (slots[slot].events) free(slots[slot].events);
    slots[slot].events = malloc(sizeof(smf_event_t) * MAX_EVENTS);
    slots[slot].event_capacity = MAX_EVENTS;
    slots[slot].event_count = 0;
    slots[slot].ppq = (data[12] << 8) | data[13];
    slots[slot].natural_bpm = 120.0;
    
    slots[slot].channel_mask = 0;
    for (int i=0; i<16; i++) slots[slot].max_poly[i] = 0;

    slots[slot].note_count = 0;
    
    uint8_t *p = data + 14;
    while (p < data + size) {
        if (memcmp(p, "MTrk", 4) != 0) { p++; continue; }
        uint32_t trk_len = (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];
        p += 8;
        uint8_t *end = p + trk_len;
        
        uint32_t abs_time = 0;
        uint8_t last_status = 0;
        
        while (p < end) {
            uint32_t dt = read_vlq(&p);
            abs_time += dt;
            
            uint8_t status = *p;
            if (status & 0x80) { p++; last_status = status; }
            else { status = last_status; }
            
            if (status == 0xFF) {
                uint8_t mtype = *p++;
                uint32_t mlen = read_vlq(&p);
                if (mtype == 0x51 && mlen == 3) {
                    uint32_t us_per_q = (p[0] << 16) | (p[1] << 8) | p[2];
                    if (us_per_q > 0) slots[slot].natural_bpm = 60000000.0f / us_per_q;
                }
                p += mlen;
            } else if (status == 0xF0 || status == 0xF7) {
                uint32_t mlen = read_vlq(&p);
                p += mlen;
            } else {
                uint8_t cmd = status & 0xF0;
                uint8_t ch = status & 0x0F;
                if (cmd == 0x80 || cmd == 0x90 || cmd == 0xB0 || cmd == 0xE0) {
                    uint8_t d1 = *p++;
                    uint8_t d2 = *p++;
                    if (slots[slot].event_count < MAX_EVENTS) {
                        slots[slot].channel_mask |= (1 << ch);
                        if (cmd == 0x90) slots[slot].note_count++;
                        smf_event_t *e = &slots[slot].events[slots[slot].event_count++];
                        e->abs_time = abs_time;
                        e->type = cmd >> 4;
                        e->ch = ch;
                        e->d1 = d1;
                        e->d2 = d2;
                    }
                } else if (cmd == 0xA0) p += 2;
                else if (cmd == 0xC0 || cmd == 0xD0) p += 1;
            }
        }
    }
    
    // Sort events by abs_time
    for (int i = 0; i < slots[slot].event_count - 1; i++) {
        for (int j = 0; j < slots[slot].event_count - i - 1; j++) {
            if (slots[slot].events[j].abs_time > slots[slot].events[j+1].abs_time) {
                smf_event_t temp = slots[slot].events[j];
                slots[slot].events[j] = slots[slot].events[j+1];
                slots[slot].events[j+1] = temp;
            }
        }
    }
    
    
    // Calculate max polyphony per channel
    int active[16][128] = {0};
    int current_poly[16] = {0};
    for (int i = 0; i < slots[slot].event_count; i++) {
        smf_event_t *e = &slots[slot].events[i];
        if (e->type == 9 && e->d2 > 0) {
            if (!active[e->ch][e->d1]) {
                active[e->ch][e->d1] = 1;
                current_poly[e->ch]++;
                if (current_poly[e->ch] > slots[slot].max_poly[e->ch]) {
                    slots[slot].max_poly[e->ch] = current_poly[e->ch];
                }
            }
        } else if (e->type == 8 || (e->type == 9 && e->d2 == 0)) {
            if (active[e->ch][e->d1]) {
                active[e->ch][e->d1] = 0;
                current_poly[e->ch]--;
            }
        }
    }

    free(data);
    slots[slot].active = 1;
    slots[slot].playing = 0;
    slots[slot].sync_mode = 0;
    slots[slot].current_event_idx = 0;
    slots[slot].current_tick = 0;
}

void midi_player_play(int slot) {
    if (slot >= 0 && slot < MAX_SLOTS && slots[slot].active) {
        slots[slot].playing = 1;
        slots[slot].current_event_idx = 0;
        slots[slot].current_tick = 0;
    }
}

void midi_player_stop(int slot) {
    if (slot >= 0 && slot < MAX_SLOTS) slots[slot].playing = 0;
    slots[slot].sync_mode = 0;
}


void midi_player_sync(int slot, int mode) {
    if (slot >= 0 && slot < MAX_SLOTS) {
        slots[slot].sync_mode = mode;
    }
}

void midi_player_tick(uint64_t now_samples) {
    float bpm = tempo_bpm_get();
    for (int i = 0; i < MAX_SLOTS; i++) {
        if (!slots[i].active || !slots[i].playing) continue;
        
        
        if (slots[i].sync_mode == 1) {
            uint64_t mtick = seq_master_tick();
            double sub = tempo_subdivision_get();
            if (sub <= 0.0) sub = 16.0;
            double target_tick = (double)mtick * (slots[i].ppq / (sub / 4.0));
            // If we jump backwards (e.g. sequence restart), reset event cursor
            if (target_tick < slots[i].current_tick) {
                slots[i].current_event_idx = 0;
            }
            slots[i].current_tick = target_tick;
        } else {
            if (slots[i].last_sample == 0) slots[i].last_sample = now_samples;
            uint64_t delta_samples = now_samples - slots[i].last_sample;
            
            // 48000 Hz assumed
            double ticks_per_sec = (bpm * slots[i].ppq) / 60.0;
            double ticks_per_sample = ticks_per_sec / 48000.0;
            
            slots[i].current_tick += delta_samples * ticks_per_sample;
        }
        slots[i].last_sample = now_samples;

        
        while (slots[i].current_event_idx < slots[i].event_count && 
               slots[i].events[slots[i].current_event_idx].abs_time <= slots[i].current_tick) {
            smf_event_t *e = &slots[i].events[slots[i].current_event_idx];
            skred_midi_test_inject(e->type, e->ch, e->d1, e->d2);
            slots[i].current_event_idx++;
        }
        
        if (slots[i].current_event_idx >= slots[i].event_count) {
            if (slots[i].sync_mode == 0) {
                slots[i].playing = 0; // stop when done only in free-run mode
            }
        }
    }
}


#include "skode.h"
void midi_player_dump(int slot, int start, int limit, void *vctx) {
    skode_t *ctx = (skode_t *)vctx;
    if (slot < 0 || slot >= MAX_SLOTS || !slots[slot].active) {
        if (ctx->printf) ctx->printf(ctx, "# MIDI slot %d not active\n", slot);
        return;
    }
    
    int end = start + limit;
    if (end > slots[slot].event_count) end = slots[slot].event_count;
    if (start < 0) start = 0;
    
    if (ctx->printf) ctx->printf(ctx, "# MIDI Dump Slot %d: Events %d to %d (of %d)\n", slot, start, end - 1, slots[slot].event_count);
    for (int i = start; i < end; i++) {
        smf_event_t *e = &slots[slot].events[i];
        if (ctx->printf) ctx->printf(ctx, "# [%d] tick=%u type=%d ch=%d d1=%d d2=%d\n", i, e->abs_time, e->type, e->ch, e->d1, e->d2);
    }
}


void midi_player_status(int slot, void *vctx) {
    skode_t *ctx = (skode_t *)vctx;
    if (slot < 0 || slot >= MAX_SLOTS) return;
    if (!slots[slot].active) {
        if (ctx->printf) ctx->printf(ctx, "# MIDI %d: EMPTY\n", slot);
        return;
    }
    
    if (ctx->printf) {
        ctx->printf(ctx, "# MIDI %d: %s tick=%.1f ppq=%d events=%d/%d sync=%d\n",
            slot, slots[slot].playing ? "PLAYING" : "STOPPED",
            slots[slot].current_tick, slots[slot].ppq,
            slots[slot].current_event_idx, slots[slot].event_count,
            slots[slot].sync_mode);
        
        
        ctx->printf(ctx, "#   -> bpm: %.1f | notes: %d | channels: ", 
            slots[slot].natural_bpm, slots[slot].note_count);
        int first_ch = 1;
        for (int ch = 0; ch < 16; ch++) {
            if (slots[slot].channel_mask & (1 << ch)) {
                ctx->printf(ctx, "%s%d(poly:%d)", first_ch ? "" : ", ", ch, slots[slot].max_poly[ch]);
                first_ch = 0;
            }
        }
        ctx->printf(ctx, "\n");


    }

}

void midi_player_seek(int slot, double tick) {
    if (slot >= 0 && slot < MAX_SLOTS && slots[slot].active) {
        slots[slot].current_tick = tick;
        // Binary search or simple linear rewind for event idx
        slots[slot].current_event_idx = 0;
        while (slots[slot].current_event_idx < slots[slot].event_count && 
               slots[slot].events[slots[slot].current_event_idx].abs_time < tick) {
            slots[slot].current_event_idx++;
        }
    }
}
