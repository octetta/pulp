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
    if (argc != 4) {
        fprintf(stderr, "Usage: wavegen <size> <array_name> <ks_file>\n");
        return 1;
    }
    
    int expected_size = atoi(argv[1]);
    const char *array_name = argv[2];
    const char *path = argv[3];
    
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
    
    if (result->n != expected_size) {
        fprintf(stderr, "Expected size %d but got %d from %s\n", expected_size, result->n, path);
        return 1;
    }
    
    printf("float %s[%d] = {\n", array_name, expected_size);
    for (int i = 0; i < result->n; i++) {
        printf("%ff, ", (float)result->f[i]);
        if (i % 8 == 7) printf("\n");
    }
    printf("\n};\n\n");
    
    free(text);
    ks_destroy(ctx);
    return 0;
}
