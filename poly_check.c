#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

uint32_t read_vlq(uint8_t **p) {
    uint32_t val = 0;
    uint8_t b;
    do {
        b = *(*p)++;
        val = (val << 7) | (b & 0x7F);
    } while (b & 0x80);
    return val;
}

typedef struct {
    uint32_t abs_time;
    uint8_t type;
    uint8_t ch;
    uint8_t d1;
    uint8_t d2;
} event_t;

event_t events[30000];
int ev_count = 0;

int main(int argc, char **argv) {
    FILE *f = fopen(argv[1], "rb");
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);
    
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
                    if (us_per_q > 0) printf("BPM = %f\n", 60000000.0f / us_per_q);
                }
                p += mlen;
            } else if (status == 0xF0 || status == 0xF7) {
                uint32_t mlen = read_vlq(&p);
                p += mlen;
            } else {
                uint8_t cmd = status & 0xF0;
                uint8_t ch = status & 0x0F;
                if (cmd == 0x80 || cmd == 0x90) {
                    uint8_t d1 = *p++;
                    uint8_t d2 = *p++;
                    events[ev_count++] = (event_t){abs_time, cmd >> 4, ch, d1, d2};
                } else if (cmd == 0xB0 || cmd == 0xE0) { p += 2; }
                else if (cmd == 0xA0) { p += 2; }
                else if (cmd == 0xC0 || cmd == 0xD0) { p += 1; }
            }
        }
    }
    
    // Bubble sort
    for (int i=0; i<ev_count-1; i++) {
        for (int j=0; j<ev_count-i-1; j++) {
            if (events[j].abs_time > events[j+1].abs_time) {
                event_t t = events[j]; events[j] = events[j+1]; events[j+1] = t;
            }
        }
    }
    
    int active[16][128] = {0};
    int poly[16] = {0};
    int max_poly[16] = {0};
    
    for (int i=0; i<ev_count; i++) {
        int ch = events[i].ch;
        int type = events[i].type;
        int note = events[i].d1;
        int vel = events[i].d2;
        
        if (type == 9 && vel > 0) {
            if (!active[ch][note]) {
                active[ch][note] = 1;
                poly[ch]++;
                if (poly[ch] > max_poly[ch]) max_poly[ch] = poly[ch];
            }
        } else if (type == 8 || (type == 9 && vel == 0)) {
            if (active[ch][note]) {
                active[ch][note] = 0;
                poly[ch]--;
            }
        }
    }
    
    for (int i=0; i<16; i++) {
        if (max_poly[i] > 0) printf("Channel %d: Max Polyphony = %d\n", i, max_poly[i]);
    }
}
