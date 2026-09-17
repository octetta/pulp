#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../vendor/ksynth/ksynth.h"

char* read_file(const char* path, size_t *out_sz) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(sz + 1);
    fread(buf, 1, sz, f);
    buf[sz] = 0;
    fclose(f);
    if (out_sz) *out_sz = sz;
    return buf;
}

int main(int argc, char **argv) {
    if (argc != 6) {
        fprintf(stderr, "Usage: wavegen <slot> <name> <array_name> <ks_file> <expected_size>\n");
        return 1;
    }
    
    int slot = atoi(argv[1]);
    const char *name = argv[2];
    const char *array_name = argv[3];
    const char *path = argv[4];
    int expected_size = atoi(argv[5]);
    
    size_t text_len = 0;
    char *text = read_file(path, &text_len);
    if (!text) {
        fprintf(stderr, "Error reading %s\n", path);
        return 1;
    }
    
    ks_ctx *ctx = ks_create(64 * 1024 * 1024, 1000000000, 44100.0);
    K result = NULL;
    
    size_t pos = 0;
    while (pos < text_len) {
        char line[4096];
        size_t start = pos;
        size_t len;
        while (pos < text_len && text[pos] != '\n' && text[pos] != '\r') pos++;
        len = pos - start;
        while (pos < text_len && (text[pos] == '\n' || text[pos] == '\r')) pos++;
        
        if (len == 0) continue;
        if (len >= sizeof(line)) len = sizeof(line) - 1;
        memcpy(line, text + start, len);
        line[len] = '\0';
        
        result = ks_eval(ctx, line, len);
        
        if (ctx->last_status != KS_OK) {
            fprintf(stderr, "Ksynth error evaluating %s on line '%s': %s\n", path, line, ctx->last_err_msg);
            return 1;
        }
    }
    
    if (!result) {
        fprintf(stderr, "No result from %s\n", path);
        return 1;
    }
    
    if (expected_size > 0 && result->n != expected_size) {
        fprintf(stderr, "Expected size %d but got %d from %s\n", expected_size, result->n, path);
        return 1;
    }
    
    int actual_size = result->n;
    
    float sample_rate = 44100.0f;
    int loop_start = 0;
    int loop_end = actual_size - 1;
    int one_shot = 0;
    
    K k_s = k_get(ctx, 'S');
    if (k_s && k_s->n == 1) sample_rate = k_s->f[0];
    
    K k_l = k_get(ctx, 'L');
    if (k_l && k_l->n == 1) loop_start = (int)k_l->f[0];
    
    K k_e = k_get(ctx, 'E');
    if (k_e && k_e->n == 1) loop_end = (int)k_e->f[0];
    
    K k_o = k_get(ctx, 'O');
    if (k_o && k_o->n == 1 && k_o->f[0] > 0) {
        one_shot = 1;
        loop_start = 0;
        loop_end = 0;
    }
    
    printf("static const float %s_DATA[%d] = {\n", array_name, actual_size);
    for (int i = 0; i < actual_size; i++) {
        printf("%ff, ", (float)result->f[i]);
        if (i % 8 == 7) printf("\n");
    }
    printf("\n};\n\n");
    
    printf("static const static_wave_meta_t %s_META = { %d, \"%s\", %d, %ff, %d, %d, %d, %s_DATA };\n\n",
           array_name, slot, name, actual_size, sample_rate, loop_start, loop_end, one_shot, array_name);
    
    free(text);
    ks_destroy(ctx);
    return 0;
}
