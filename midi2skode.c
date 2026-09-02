#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// ---------------------------------------------------------
// DATA STRUCTURES
// ---------------------------------------------------------
typedef struct {
    int track;
    int abs_time;
    int ch;
    int type; // 0=Note, 1=CC, 2=PitchBend
    int d1;   // Note or CC#
    int d2;   // Velocity or Value
} MidiEvent;

MidiEvent *events = NULL;
int event_count = 0;
int event_capacity = 0;

void add_event(int trk, int time, int ch, int type, int d1, int d2) {
    if (event_count >= event_capacity) {
        event_capacity = event_capacity == 0 ? 1024 : event_capacity * 2;
        events = realloc(events, event_capacity * sizeof(MidiEvent));
    }
    events[event_count++] = (MidiEvent){trk, time, ch, type, d1, d2};
}

uint32_t read_vlq(uint8_t **p) {
    uint32_t val = 0;
    while (1) {
        uint8_t b = *(*p)++;
        val = (val << 7) | (b & 0x7F);
        if (!(b & 0x80)) break;
    }
    return val;
}

// ---------------------------------------------------------
// DRUM MAPPING (MIDI CH 10)
// ---------------------------------------------------------
const char* get_drum_name(int note) {
    switch(note) {
        case 35: return "Acoustic Bass Drum";
        case 36: return "Bass Drum 1";
        case 37: return "Side Stick";
        case 38: return "Acoustic Snare";
        case 39: return "Hand Clap";
        case 40: return "Electric Snare";
        case 41: return "Low Floor Tom";
        case 42: return "Closed Hi-Hat";
        case 43: return "High Floor Tom";
        case 44: return "Pedal Hi-Hat";
        case 45: return "Low Tom";
        case 46: return "Open Hi-Hat";
        case 47: return "Low-Mid Tom";
        case 48: return "Hi-Mid Tom";
        case 49: return "Crash Cymbal 1";
        case 50: return "High Tom";
        case 51: return "Ride Cymbal 1";
        case 52: return "Chinese Cymbal";
        case 53: return "Ride Bell";
        case 54: return "Tambourine";
        case 55: return "Splash Cymbal";
        case 56: return "Cowbell";
        case 57: return "Crash Cymbal 2";
        default: return "Perc";
    }
}

const char* get_cc_name(int cc) {
    switch(cc) {
        case 1: return "ModWheel";
        case 7: return "Volume";
        case 10: return "Pan";
        case 11: return "Expression";
        case 64: return "Sustain";
        case 74: return "FilterCutoff";
        default: return "CC";
    }
}

// ---------------------------------------------------------
// MAIN
// ---------------------------------------------------------
int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <file.mid> [options]\n", argv[0]);
        printf("Options:\n");
        printf("  --steps <N>       Number of steps per pattern (default: 16)\n");
        printf("  --div <N>         Steps per quarter note (default: 4 for 16th notes)\n");
        printf("  --meter <N/D>     Hint for bar chunking (e.g., 4/4 or 6/8)\n");
        printf("  --velocity        Include velocity parameters in output macros\n");
        return 1;
    }
    
    int steps_per_pattern = 16;
    int steps_per_quarter = 4; // 16th notes
    int use_velocity = 0;
    
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--steps") == 0 && i+1 < argc) {
            steps_per_pattern = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--div") == 0 && i+1 < argc) {
            steps_per_quarter = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--velocity") == 0) {
            use_velocity = 1;
        } else if (strcmp(argv[i], "--meter") == 0 && i+1 < argc) {
            int num, den;
            if (sscanf(argv[++i], "%d/%d", &num, &den) == 2) {
                steps_per_pattern = num * (4.0 / den) * steps_per_quarter;
            }
        }
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t *data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);

    if (memcmp(data, "MThd", 4) != 0) {
        printf("Error: Not a MIDI file\n");
        return 1;
    }

    uint16_t fmt = (data[8] << 8) | data[9];
    uint16_t tracks = (data[10] << 8) | data[11];
    uint16_t ppq = (data[12] << 8) | data[13];
    
    printf("# ==========================================\n");
    printf("# SKODE GENERATOR: MIDI TO .SK\n");
    printf("# File: %s\n", argv[1]);
    printf("# Format: %d | Tracks: %d | PPQ: %d\n", fmt, tracks, ppq);
    printf("# Config: %d steps/pattern | %d steps/quarter\n", steps_per_pattern, steps_per_quarter);
    printf("# ==========================================\n\n");

    uint8_t *p = data + 14;
    int track_idx = 0;
    int tempo_bpm = 120;
    
    while (p < data + size) {
        if (memcmp(p, "MTrk", 4) != 0) { p++; continue; }
        uint32_t trk_len = (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];
        p += 8;
        uint8_t *end = p + trk_len;
        
        int abs_time = 0;
        uint8_t last_status = 0;
        
        while (p < end) {
            uint32_t dt = read_vlq(&p);
            abs_time += dt;
            
            uint8_t status = *p;
            if (status & 0x80) {
                p++;
                last_status = status;
            } else {
                status = last_status;
            }
            
            if (status == 0xFF) {
                uint8_t mtype = *p++;
                uint32_t mlen = read_vlq(&p);
                if (mtype == 0x51 && mlen == 3) {
                    uint32_t mpqn = (p[0] << 16) | (p[1] << 8) | p[2];
                    if (mpqn > 0) tempo_bpm = 60000000 / mpqn;
                }
                p += mlen;
            } else if (status == 0xF0 || status == 0xF7) {
                uint32_t mlen = read_vlq(&p);
                p += mlen;
            } else {
                uint8_t cmd = status & 0xF0;
                uint8_t ch = status & 0x0F;
                
                if (cmd == 0x80 || cmd == 0x90) {
                    uint8_t n = *p++;
                    uint8_t v = *p++;
                    if (cmd == 0x90 && v > 0) {
                        add_event(track_idx, abs_time, ch, 0, n, v);
                    }
                } else if (cmd == 0xB0) { // CC
                    uint8_t cc = *p++;
                    uint8_t val = *p++;
                    add_event(track_idx, abs_time, ch, 1, cc, val);
                } else if (cmd == 0xE0) { // Pitch Bend
                    uint8_t lsb = *p++;
                    uint8_t msb = *p++;
                    add_event(track_idx, abs_time, ch, 2, lsb, msb);
                } else if (cmd == 0xA0) {
                    p += 2;
                } else if (cmd == 0xC0 || cmd == 0xD0) {
                    p += 1;
                }
            }
        }
        track_idx++;
    }
    
    printf("M%d\n\n", tempo_bpm);
    
    int max_pat = 0;
    int ticks_per_step = ppq / steps_per_quarter;
    if (ticks_per_step == 0) ticks_per_step = 1;

    for (int i = 0; i < event_count; i++) {
        // Round to nearest step instead of flooring
        int step = (events[i].abs_time + ticks_per_step / 2) / ticks_per_step;
        int pat = step / steps_per_pattern;
        if (pat > max_pat) max_pat = pat;
    }
    
    printf("# --- MACRO DEFINITIONS ---\n");
    for (int c = 0; c < 16; c++) {
        int has_ch = 0;
        int is_drum = (c == 9);
        for (int i=0; i<event_count; i++) if (events[i].ch == c && events[i].type == 0) has_ch = 1;
        
        if (has_ch) {
            if (is_drum) {
                printf("# Drum Channel (CH 10)\n");
                printf("[Drm] : v %d n $$0 ", c);
                if (use_velocity) printf("a $$1 ");
                printf("l 1 ;\n");
            } else {
                printf("[Ch%c] : v %d n $$0 ", 'A' + c, c);
                if (use_velocity) printf("a $$1 ");
                printf("l 1 ;\n");
            }
        }
    }
    printf("\n");
    
    int pat_id = 10; 
    
    for (int trk = 0; trk < tracks; trk++) {
        for (int ch = 0; ch < 16; ch++) {
            int trk_has_events = 0;
            for (int i=0; i<event_count; i++) if (events[i].track == trk && events[i].ch == ch) trk_has_events = 1;
            if (!trk_has_events) continue;
            
            printf("# ==========================================\n");
            printf("# TRACK %d - CHANNEL %d\n", trk, ch);
            printf("# ==========================================\n");
            
            for (int p = 0; p <= max_pat; p++) {
            int steps[256][16] = {0}; 
            int vels[256][16]  = {0};
            int chs[256][16]   = {0};
            int step_counts[256] = {0};
            
            char cc_comments[65536] = {0};
            int has_cc = 0;
            
            int pat_has_events = 0;
            for (int i = 0; i < event_count; i++) {
                if (events[i].track != trk || events[i].ch != ch) continue;
                int step = (events[i].abs_time + ticks_per_step / 2) / ticks_per_step;
                int pat = step / steps_per_pattern;
                
                if (pat == p) {
                    pat_has_events = 1;
                    int s = step % steps_per_pattern;
                    
                    if (events[i].type == 0) { // Note
                        if (s < 256 && step_counts[s] < 16) {
                            steps[s][step_counts[s]] = events[i].d1;
                            vels[s][step_counts[s]]  = events[i].d2;
                            chs[s][step_counts[s]]   = events[i].ch;
                            step_counts[s]++;
                        }
                    } else if (events[i].type == 1) { // CC
                        has_cc = 1;
                        char buf[128];
                        snprintf(buf, sizeof(buf), "#   Step %d: CC %d (%s) -> %d\n", s, events[i].d1, get_cc_name(events[i].d1), events[i].d2);
                        size_t cur_len = strlen(cc_comments);
                        if (cur_len + strlen(buf) < sizeof(cc_comments) - 1) {
                            strcat(cc_comments, buf);
                        }
                    }
                }
            }
            
            if (pat_has_events) {
                printf("y %d yt [T%dC%dP%d]\n", pat_id++, trk, ch, p);
                
                int drum_notes[128] = {0};
                for (int s = 0; s < steps_per_pattern; s++) {
                    for (int n = 0; n < step_counts[s]; n++) {
                        if (chs[s][n] == 9) drum_notes[steps[s][n]] = 1;
                    }
                }
                for (int dn = 0; dn < 128; dn++) {
                    if (drum_notes[dn]) printf("#   Drum Note %d: %s\n", dn, get_drum_name(dn));
                }
                
                if (has_cc) {
                    printf("#   --- Control Changes ---\n");
                    printf("%s", cc_comments);
                }
                
                printf("%% 1\n");
                
                for (int s = 0; s < steps_per_pattern; s++) {
                    if (step_counts[s] > 0) {
                        printf("[");
                        for (int n = 0; n < step_counts[s]; n++) {
                            int c = chs[s][n];
                            if (c == 9) {
                                printf("Drm %d", steps[s][n]);
                            } else {
                                printf("Ch%c %d", 'A' + c, steps[s][n]);
                            }
                            if (use_velocity) {
                                int amp = (vels[s][n] * 20) / 127;
                                printf(" %d", amp);
                            }
                            if (n < step_counts[s]-1) printf(" ");
                        }
                        printf("] x %d   ", s);
                    }
                }
                printf("\n\n");
            }
        }
    }
    
    }
    free(data);
    free(events);
    return 0;
}
