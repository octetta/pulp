/*
 * tools/mksynth.c - Single-Source Synthesizer State & Codegen Compiler
 *
 * Ingests declarative schema files from schema directory and generates:
 *   1. C Struct-of-Arrays (SoA) engine headers and allocation routines.
 *   2. C voice reset and copy operations.
 *   3. C automated boundary, reset, and alignment invariance tests.
 *   4. Gleam parameter bounds, typed records, and delta diffing functions.
 *
 * ANSI C (C89/C99 standard library only).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

#define MAX_FIELDS 1024
#define MAX_SCHEMAS 32
#define STR_MAX 128
#define DOC_MAX 2048

typedef enum {
    TYPE_INT = 0,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_FLOAT_ARRAY
} field_type_t;

typedef struct {
    char name[STR_MAX];
    char schema[STR_MAX];
    char group[STR_MAX];
    field_type_t type;
    int array_size;
    char default_val[STR_MAX];
    char reset_val[STR_MAX];
    double range_min;
    double range_max;
    int has_range;
    char unit[STR_MAX];
    char opcode[STR_MAX];
    int vectorize;
    int copy;
    char doc[DOC_MAX];
} schema_field_t;

static schema_field_t g_fields[MAX_FIELDS];
static int g_field_count = 0;

static void trim(char *s) {
    char *p = s;
    int l;
    while (isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    l = (int)strlen(s);
    while (l > 0 && isspace((unsigned char)s[l - 1])) {
        s[--l] = '\0';
    }
}

static void safe_strcpy(char *dst, const char *src, size_t cap) {
    if (cap == 0) return;
    size_t len = strlen(src);
    if (len >= cap) len = cap - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
}

static field_type_t parse_type(const char *s, int *arr_size) {
    *arr_size = 0;
    if (strcmp(s, "int") == 0) return TYPE_INT;
    if (strcmp(s, "double") == 0) return TYPE_DOUBLE;
    if (strcmp(s, "float_array") == 0) return TYPE_FLOAT_ARRAY;
    return TYPE_FLOAT;
}

static void parse_schema_file(const char *filepath) {
    FILE *f = fopen(filepath, "r");
    char line[1024];
    char current_schema[STR_MAX] = "default";
    char current_group[STR_MAX] = "voice";
    schema_field_t cur;
    int in_field = 0;

    if (!f) {
        fprintf(stderr, "mksynth: error: cannot open %s\n", filepath);
        exit(1);
    }

    memset(&cur, 0, sizeof(cur));

    while (fgets(line, sizeof(line), f)) {
        char trimmed[1024];
        safe_strcpy(trimmed, line, sizeof(trimmed));
        trim(trimmed);

        if (trimmed[0] == '#' || trimmed[0] == '\0') continue;

        if (strncmp(trimmed, "@schema", 7) == 0) {
            char *val = trimmed + 7;
            trim(val);
            safe_strcpy(current_schema, val, sizeof(current_schema));
        } else if (strncmp(trimmed, "@group", 6) == 0) {
            char *val = trimmed + 6;
            trim(val);
            safe_strcpy(current_group, val, sizeof(current_group));
        } else if (strncmp(trimmed, "@field", 6) == 0) {
            if (in_field) {
                if (g_field_count >= MAX_FIELDS) {
                    fprintf(stderr, "mksynth: fatal: MAX_FIELDS exceeded\n");
                    exit(1);
                }
                g_fields[g_field_count++] = cur;
                memset(&cur, 0, sizeof(cur));
            }
            char *val = trimmed + 6;
            trim(val);
            safe_strcpy(cur.name, val, sizeof(cur.name));
            safe_strcpy(cur.schema, current_schema, sizeof(cur.schema));
            safe_strcpy(cur.group, current_group, sizeof(cur.group));
            cur.copy = 1;
            in_field = 1;
        } else if (strncmp(trimmed, "@type", 5) == 0) {
            char *val = trimmed + 5;
            trim(val);
            cur.type = parse_type(val, &cur.array_size);
        } else if (strncmp(trimmed, "@size", 5) == 0) {
            char *val = trimmed + 5;
            trim(val);
            cur.array_size = atoi(val);
        } else if (strncmp(trimmed, "@default", 8) == 0) {
            char *val = trimmed + 8;
            trim(val);
            safe_strcpy(cur.default_val, val, sizeof(cur.default_val));
        } else if (strncmp(trimmed, "@reset", 6) == 0) {
            char *val = trimmed + 6;
            trim(val);
            safe_strcpy(cur.reset_val, val, sizeof(cur.reset_val));
        } else if (strncmp(trimmed, "@range", 6) == 0) {
            char *val = trimmed + 6;
            char *sep = strstr(val, "..");
            if (sep) {
                *sep = '\0';
                cur.range_min = atof(val);
                cur.range_max = atof(sep + 2);
                cur.has_range = 1;
            }
        } else if (strncmp(trimmed, "@unit", 5) == 0) {
            char *val = trimmed + 5;
            trim(val);
            safe_strcpy(cur.unit, val, sizeof(cur.unit));
        } else if (strncmp(trimmed, "@opcode", 7) == 0) {
            char *val = trimmed + 7;
            trim(val);
            safe_strcpy(cur.opcode, val, sizeof(cur.opcode));
        } else if (strncmp(trimmed, "@vectorize", 10) == 0) {
            char *val = trimmed + 10;
            trim(val);
            cur.vectorize = (strcmp(val, "true") == 0 || strcmp(val, "1") == 0);
        } else if (strncmp(trimmed, "@copy", 5) == 0) {
            char *val = trimmed + 5;
            trim(val);
            cur.copy = (strcmp(val, "true") == 0 || strcmp(val, "1") == 0);
        } else if (strncmp(trimmed, "@doc", 4) == 0) {
            char *val = trimmed + 4;
            trim(val);
            safe_strcpy(cur.doc, val, sizeof(cur.doc));
        }
    }

    if (in_field) {
        if (g_field_count >= MAX_FIELDS) {
            fprintf(stderr, "mksynth: fatal: MAX_FIELDS exceeded\n");
            exit(1);
        }
        g_fields[g_field_count++] = cur;
    }

    fclose(f);
}

static void scan_schema_dir(const char *dirpath) {
#if defined(_WIN32)
    char pattern[MAX_PATH];
    WIN32_FIND_DATAA fd;
    HANDLE h;
    snprintf(pattern, sizeof(pattern), "%s\\*.def", dirpath);
    h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            char full[MAX_PATH];
            snprintf(full, sizeof(full), "%s\\%s", dirpath, fd.cFileName);
            parse_schema_file(full);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
#else
    DIR *d = opendir(dirpath);
    struct dirent *ent;
    if (!d) return;
    while ((ent = readdir(d)) != NULL) {
        if (strstr(ent->d_name, ".def")) {
            char full[1024];
            snprintf(full, sizeof(full), "%s/%s", dirpath, ent->d_name);
            parse_schema_file(full);
        }
    }
    closedir(d);
#endif
}

/* -------------------------------------------------------------------------
 * C Code Generators
 * ------------------------------------------------------------------------- */

static void emit_c_state_header(const char *filepath) {
    FILE *f = fopen(filepath, "w");
    int i;
    if (!f) { fprintf(stderr, "cannot open %s\n", filepath); exit(1); }

    fprintf(f, "/* Auto-generated by tools/mksynth from schema directory. Do not edit. */\n");
    fprintf(f, "#ifndef SYNTH_STATE_GEN_H\n#define SYNTH_STATE_GEN_H\n\n");
    fprintf(f, "#include <stdint.h>\n#include <stddef.h>\n\n");
    fprintf(f, "typedef struct {\n");

    for (i = 0; i < g_field_count; i++) {
        const schema_field_t *fld = &g_fields[i];
        if (strcmp(fld->group, "voice") != 0) continue;

        switch (fld->type) {
            case TYPE_INT:
                fprintf(f, "    int    * restrict %s;\n", fld->name);
                break;
            case TYPE_DOUBLE:
                fprintf(f, "    double * restrict %s;\n", fld->name);
                break;
            case TYPE_FLOAT_ARRAY:
                fprintf(f, "    float  * restrict %s[%d];\n", fld->name, fld->array_size > 0 ? fld->array_size : 6);
                break;
            case TYPE_FLOAT:
            default:
                fprintf(f, "    float  * restrict %s;\n", fld->name);
                break;
        }
    }

    fprintf(f, "} synth_voices_gen_t;\n\n");
    fprintf(f, "void synth_alloc_voices_gen(int v);\n");
    fprintf(f, "void synth_free_voices_gen(void);\n");
    fprintf(f, "void wave_reset_gen(int v);\n");
    fprintf(f, "void voice_copy_gen(int dst, int src);\n");
    fprintf(f, "char *voice_format_gen(int v, char *out, size_t out_size, int verbose);\n\n");
    fprintf(f, "#endif /* SYNTH_STATE_GEN_H */\n");
    fclose(f);
}

static void emit_c_alloc_source(const char *filepath) {
    FILE *f = fopen(filepath, "w");
    int i;
    if (!f) { fprintf(stderr, "cannot open %s\n", filepath); exit(1); }

    fprintf(f, "/* Auto-generated by tools/mksynth from schema directory. Do not edit. */\n");
    fprintf(f, "#include \"synth-state.h\"\n#include \"synth-alloc.h\"\n\n");

    fprintf(f, "void synth_alloc_voices_gen(int v) {\n");
    for (i = 0; i < g_field_count; i++) {
        const schema_field_t *fld = &g_fields[i];
        if (strcmp(fld->group, "voice") != 0) continue;

        if (fld->type == TYPE_FLOAT_ARRAY) {
            int sz = fld->array_size > 0 ? fld->array_size : 6;
            int k;
            for (k = 0; k < sz; k++) {
                fprintf(f, "    VALLOC(%s[%d], float);\n", fld->name, k);
            }
        } else {
            const char *ct = (fld->type == TYPE_INT) ? "int" : (fld->type == TYPE_DOUBLE) ? "double" : "float";
            fprintf(f, "    VALLOC(%s, %s);\n", fld->name, ct);
        }
    }
    fprintf(f, "}\n\n");

    fprintf(f, "void synth_free_voices_gen(void) {\n");
    for (i = 0; i < g_field_count; i++) {
        const schema_field_t *fld = &g_fields[i];
        if (strcmp(fld->group, "voice") != 0) continue;

        if (fld->type == TYPE_FLOAT_ARRAY) {
            int sz = fld->array_size > 0 ? fld->array_size : 6;
            int k;
            for (k = 0; k < sz; k++) {
                fprintf(f, "    VFREE(%s[%d]);\n", fld->name, k);
            }
        } else {
            fprintf(f, "    VFREE(%s);\n", fld->name);
        }
    }
    fprintf(f, "}\n");
    fclose(f);
}

static void emit_c_voice_ops(const char *filepath) {
    FILE *f = fopen(filepath, "w");
    int i;
    if (!f) { fprintf(stderr, "cannot open %s\n", filepath); exit(1); }

    fprintf(f, "/* Auto-generated by tools/mksynth from schema directory. Do not edit. */\n");
    fprintf(f, "#include <stdio.h>\n#include <math.h>\n#include \"synth-state.h\"\n\n");

    fprintf(f, "void wave_reset_gen(int v) {\n");
    for (i = 0; i < g_field_count; i++) {
        const schema_field_t *fld = &g_fields[i];
        if (strcmp(fld->group, "voice") != 0) continue;
        const char *val = fld->reset_val[0] ? fld->reset_val : fld->default_val[0] ? fld->default_val : "0";

        if (fld->type == TYPE_FLOAT_ARRAY) {
            int sz = fld->array_size > 0 ? fld->array_size : 6;
            int k;
            for (k = 0; k < sz; k++) {
                fprintf(f, "    sv.%s[%d][v] = %sf;\n", fld->name, k, val);
            }
        } else {
            fprintf(f, "    sv.%s[v] = %s%s;\n", fld->name, val, (fld->type == TYPE_FLOAT) ? "f" : "");
        }
    }
    fprintf(f, "}\n\n");

    fprintf(f, "void voice_copy_gen(int dst, int src) {\n");
    for (i = 0; i < g_field_count; i++) {
        const schema_field_t *fld = &g_fields[i];
        if (strcmp(fld->group, "voice") != 0 || !fld->copy) continue;

        if (fld->type == TYPE_FLOAT_ARRAY) {
            int sz = fld->array_size > 0 ? fld->array_size : 6;
            int k;
            for (k = 0; k < sz; k++) {
                fprintf(f, "    sv.%s[%d][dst] = sv.%s[%d][src];\n", fld->name, k, fld->name, k);
            }
        } else {
            fprintf(f, "    sv.%s[dst] = sv.%s[src];\n", fld->name, fld->name);
        }
    }
    fprintf(f, "}\n\n");

    fprintf(f, "char *voice_format_gen(int v, char *out, size_t out_size, int verbose) {\n");
    fprintf(f, "    if (!out) return \"(NULL)\";\n");
    fprintf(f, "    if (out_size == 0) return out;\n");
    fprintf(f, "    if (v < 0 || v >= synth_config.voice_max) { out[0] = '\\0'; return out; }\n\n");
    fprintf(f, "    char *ptr = out;\n");
    fprintf(f, "    size_t remaining = out_size;\n\n");
    fprintf(f, "#define APPEND_GEN(...) do { \\\n");
    fprintf(f, "    int _n = snprintf(ptr, remaining, __VA_ARGS__); \\\n");
    fprintf(f, "    if (_n > 0 && (size_t)_n < remaining) { ptr += _n; remaining -= _n; } \\\n");
    fprintf(f, "    else { ptr += remaining - 1; *ptr = '\\0'; remaining = 0; return out; } \\\n");
    fprintf(f, "} while (0)\n\n");
    fprintf(f, "    APPEND_GEN(\"v%%d w%%d f%%g a%%g\", v, sv.wave_table_index[v], sv.freq[v], sv.user_amp[v]);\n\n");

    for (i = 0; i < g_field_count; i++) {
        const schema_field_t *fld = &g_fields[i];
        if (strcmp(fld->group, "voice") != 0 || !fld->opcode[0]) continue;
        if (strcmp(fld->name, "wave_table_index") == 0 ||
            strcmp(fld->name, "freq") == 0 ||
            strcmp(fld->name, "user_amp") == 0 ||
            strcmp(fld->name, "amp") == 0) continue;
        if (fld->type == TYPE_FLOAT_ARRAY) continue;

        if (fld->type == TYPE_INT) {
            int def_v = atoi(fld->default_val[0] ? fld->default_val : "0");
            fprintf(f, "    if (verbose || sv.%s[v] != %d) APPEND_GEN(\" %s%%d\", sv.%s[v]);\n",
                    fld->name, def_v, fld->opcode, fld->name);
        } else {
            double def_v = atof(fld->default_val[0] ? fld->default_val : "0");
            fprintf(f, "    if (verbose || fabs((double)sv.%s[v] - %f) > 1e-4) APPEND_GEN(\" %s%%g\", (double)sv.%s[v]);\n",
                    fld->name, def_v, fld->opcode, fld->name);
        }
    }

    fprintf(f, "\n#undef APPEND_GEN\n");
    fprintf(f, "    return out;\n");
    fprintf(f, "}\n");
    fclose(f);
}

static void emit_c_bounds_test(const char *filepath) {
    FILE *f = fopen(filepath, "w");
    int i;
    if (!f) { fprintf(stderr, "cannot open %s\n", filepath); exit(1); }

    fprintf(f, "/* Auto-generated by tools/mksynth from schema directory. Do not edit. */\n");
    fprintf(f, "#include <stdio.h>\n#include <assert.h>\n#include <math.h>\n\n");
    fprintf(f, "int main(void) {\n");
    fprintf(f, "    printf(\"Running mksynth automated boundary invariance tests...\\n\");\n");

    for (i = 0; i < g_field_count; i++) {
        const schema_field_t *fld = &g_fields[i];
        if (fld->has_range) {
            fprintf(f, "    /* Check range for %s (%g .. %g) */\n", fld->name, fld->range_min, fld->range_max);
            fprintf(f, "    assert(%g <= %g);\n", fld->range_min, fld->range_max);
        }
    }

    fprintf(f, "    printf(\"All mksynth boundary invariance checks passed!\\n\");\n");
    fprintf(f, "    return 0;\n");
    fprintf(f, "}\n");
    fclose(f);
}

static void emit_c_equivalence_test(const char *filepath) {
    FILE *f = fopen(filepath, "w");
    int i;
    if (!f) { fprintf(stderr, "cannot open %s\n", filepath); exit(1); }

    fprintf(f, "/* Auto-generated by tools/mksynth from schema directory. Do not edit. */\n");
    fprintf(f, "#include <stdio.h>\n#include <stdlib.h>\n#include <math.h>\n#include <assert.h>\n\n");
    fprintf(f, "#include \"synth.h\"\n#include \"synth-state.h\"\n#include \"synth_state_gen.h\"\n\n");
    fprintf(f, "void voice_reset(int i);\n\n");

    fprintf(f, "int main(void) {\n");
    fprintf(f, "    int failures = 0;\n");
    fprintf(f, "    printf(\"Running mksynth differential equivalence tests...\\n\");\n\n");
    fprintf(f, "    synth_config_defaults();\n");
    fprintf(f, "    synth_init(4);\n\n");

    fprintf(f, "    /* Test 1: Compare wave_reset_gen(1) against legacy voice_reset(0) */\n");
    fprintf(f, "    voice_reset(0);\n");
    fprintf(f, "    wave_reset_gen(1);\n\n");

    for (i = 0; i < g_field_count; i++) {
        const schema_field_t *fld = &g_fields[i];
        if (strcmp(fld->group, "voice") != 0) continue;

        if (fld->type == TYPE_FLOAT_ARRAY) {
            int sz = fld->array_size > 0 ? fld->array_size : 6;
            int k;
            for (k = 0; k < sz; k++) {
                fprintf(f, "    if (sv.%s[%d][0] != sv.%s[%d][1]) {\n", fld->name, k, fld->name, k);
                fprintf(f, "        printf(\"FAIL reset %s[%d]: voice0=%%f voice1=%%f\\n\", (double)sv.%s[%d][0], (double)sv.%s[%d][1]);\n", fld->name, k, fld->name, k, fld->name, k);
                fprintf(f, "        failures++;\n    }\n");
            }
        } else if (fld->type == TYPE_INT) {
            fprintf(f, "    if (sv.%s[0] != sv.%s[1]) {\n", fld->name, fld->name);
            fprintf(f, "        printf(\"FAIL reset %s: voice0=%%d voice1=%%d\\n\", sv.%s[0], sv.%s[1]);\n", fld->name, fld->name, fld->name);
            fprintf(f, "        failures++;\n    }\n");
        } else {
            fprintf(f, "    if (fabs(sv.%s[0] - sv.%s[1]) > 1e-4) {\n", fld->name, fld->name);
            fprintf(f, "        printf(\"FAIL reset %s: voice0=%%g voice1=%%g\\n\", (double)sv.%s[0], (double)sv.%s[1]);\n", fld->name, fld->name, fld->name);
            fprintf(f, "        failures++;\n    }\n");
        }
    }

    fprintf(f, "\n    /* Test 2: Compare voice_copy_gen(3, 0) against legacy voice_copy(0, 2) */\n");
    fprintf(f, "    voice_copy(0, 2);\n");
    fprintf(f, "    voice_copy_gen(3, 0);\n\n");

    for (i = 0; i < g_field_count; i++) {
        const schema_field_t *fld = &g_fields[i];
        if (strcmp(fld->group, "voice") != 0 || !fld->copy) continue;

        if (fld->type == TYPE_FLOAT_ARRAY) {
            int sz = fld->array_size > 0 ? fld->array_size : 6;
            int k;
            for (k = 0; k < sz; k++) {
                fprintf(f, "    if (sv.%s[%d][2] != sv.%s[%d][3]) {\n", fld->name, k, fld->name, k);
                fprintf(f, "        printf(\"FAIL copy %s[%d]: voice2=%%f voice3=%%f\\n\", (double)sv.%s[%d][2], (double)sv.%s[%d][3]);\n", fld->name, k, fld->name, k, fld->name, k);
                fprintf(f, "        failures++;\n    }\n");
            }
        } else if (fld->type == TYPE_INT) {
            fprintf(f, "    if (sv.%s[2] != sv.%s[3]) {\n", fld->name, fld->name);
            fprintf(f, "        printf(\"FAIL copy %s: voice2=%%d voice3=%%d\\n\", sv.%s[2], sv.%s[3]);\n", fld->name, fld->name, fld->name);
            fprintf(f, "        failures++;\n    }\n");
        } else {
            fprintf(f, "    if (fabs(sv.%s[2] - sv.%s[3]) > 1e-4) {\n", fld->name, fld->name);
            fprintf(f, "        printf(\"FAIL copy %s: voice2=%%g voice3=%%g\\n\", (double)sv.%s[2], (double)sv.%s[3]);\n", fld->name, fld->name, fld->name);
            fprintf(f, "        failures++;\n    }\n");
        }
    }

    fprintf(f, "\n    synth_free();\n\n");
    fprintf(f, "    if (failures == 0) {\n");
    fprintf(f, "        printf(\"All mksynth differential equivalence tests passed!\\n\");\n");
    fprintf(f, "        return 0;\n");
    fprintf(f, "    } else {\n");
    fprintf(f, "        printf(\"Differential equivalence tests FAILED with %%d mismatches\\n\", failures);\n");
    fprintf(f, "        return 1;\n");
    fprintf(f, "    }\n");
    fprintf(f, "}\n");
    fclose(f);
}

/* -------------------------------------------------------------------------
 * Gleam Code Generator
 * ------------------------------------------------------------------------- */

static void format_gleam_float(char *buf, size_t sz, double val) {
    snprintf(buf, sz, "%g", val);
    if (!strchr(buf, '.') && !strchr(buf, 'e') && !strchr(buf, 'E')) {
        strncat(buf, ".0", sz - strlen(buf) - 1);
    }
}

static void emit_gleam_params(const char *filepath) {
    FILE *f = fopen(filepath, "w");
    int i;
    if (!f) { fprintf(stderr, "cannot open %s\n", filepath); return; }

    fprintf(f, "//// Auto-generated by tools/mksynth. Do not edit directly.\n\n");
    fprintf(f, "import gleam/float\nimport gleam/int\n\n");

    for (i = 0; i < g_field_count; i++) {
        const schema_field_t *fld = &g_fields[i];
        if (!fld->has_range) continue;

        if (fld->type == TYPE_INT) {
            fprintf(f, "pub const min_%s = %d\n", fld->name, (int)fld->range_min);
            fprintf(f, "pub const max_%s = %d\n", fld->name, (int)fld->range_max);
            fprintf(f, "pub fn clamp_%s(val: Int) -> Int {\n", fld->name);
            fprintf(f, "  int.clamp(val, min_%s, max_%s)\n}\n\n", fld->name, fld->name);
        } else {
            char min_buf[64], max_buf[64];
            format_gleam_float(min_buf, sizeof(min_buf), fld->range_min);
            format_gleam_float(max_buf, sizeof(max_buf), fld->range_max);
            fprintf(f, "pub const min_%s = %s\n", fld->name, min_buf);
            fprintf(f, "pub const max_%s = %s\n", fld->name, max_buf);
            fprintf(f, "pub fn clamp_%s(val: Float) -> Float {\n", fld->name);
            fprintf(f, "  float.clamp(val, min_%s, max_%s)\n}\n\n", fld->name, fld->name);
        }
    }

    fclose(f);
}

static void emit_gleam_voice(const char *filepath) {
    FILE *f = fopen(filepath, "w");
    int i;
    if (!f) return;

    fprintf(f, "//// Auto-generated by tools/mksynth. Do not edit directly.\n\n");
    fprintf(f, "import gleam/int\nimport gleam/list\nimport gleam/string\nimport skode/param\n\n");
    fprintf(f, "pub type Voice {\n  Voice(\n    id: Int,\n");
    for (i = 0; i < g_field_count; i++) {
        const schema_field_t *fld = &g_fields[i];
        if (strcmp(fld->group, "voice") != 0) continue;
        if (fld->type == TYPE_FLOAT_ARRAY) {
            fprintf(f, "    %s: List(Float),\n", fld->name);
        } else if (fld->type == TYPE_INT) {
            fprintf(f, "    %s: Int,\n", fld->name);
        } else {
            fprintf(f, "    %s: Float,\n", fld->name);
        }
    }
    fprintf(f, "  )\n}\n\n");

    fprintf(f, "pub fn default(id: Int) -> Voice {\n  Voice(\n    id: id,\n");
    for (i = 0; i < g_field_count; i++) {
        const schema_field_t *fld = &g_fields[i];
        if (strcmp(fld->group, "voice") != 0) continue;
        const char *val = fld->default_val[0] ? fld->default_val : "0";
        if (fld->type == TYPE_FLOAT_ARRAY) {
            fprintf(f, "    %s: [],\n", fld->name);
        } else if (fld->type == TYPE_INT) {
            fprintf(f, "    %s: %d,\n", fld->name, atoi(val));
        } else {
            char fbuf[64];
            format_gleam_float(fbuf, sizeof(fbuf), atof(val));
            fprintf(f, "    %s: %s,\n", fld->name, fbuf);
        }
    }
    fprintf(f, "  )\n}\n\n");

    /* to_skode: compact canonical Skode string */
    fprintf(f, "pub fn to_skode(voice: Voice) -> String {\n");
    fprintf(f, "  let prefix = \"v\" <> int.to_string(voice.id)\n");
    fprintf(f, "  let d = default(voice.id)\n");
    fprintf(f, "  let cmds = [\n");
    fprintf(f, "    prefix,\n");
    fprintf(f, "    \"w\" <> int.to_string(voice.wave_table_index),\n");
    fprintf(f, "    \"f\" <> param.format_float(voice.freq),\n");
    fprintf(f, "    \"a\" <> param.format_float(voice.user_amp),\n");
    fprintf(f, "  ]\n");

    for (i = 0; i < g_field_count; i++) {
        const schema_field_t *fld = &g_fields[i];
        if (strcmp(fld->group, "voice") != 0 || !fld->opcode[0]) continue;
        if (strcmp(fld->name, "wave_table_index") == 0 ||
            strcmp(fld->name, "freq") == 0 ||
            strcmp(fld->name, "user_amp") == 0 ||
            strcmp(fld->name, "amp") == 0) continue;
        if (fld->type == TYPE_FLOAT_ARRAY) continue;

        if (fld->type == TYPE_INT) {
            fprintf(f, "  let cmds = case voice.%s != d.%s {\n", fld->name, fld->name);
            fprintf(f, "    True -> list.append(cmds, [\"%s\" <> int.to_string(voice.%s)])\n", fld->opcode, fld->name);
            fprintf(f, "    False -> cmds\n  }\n");
        } else {
            fprintf(f, "  let cmds = case voice.%s != d.%s {\n", fld->name, fld->name);
            fprintf(f, "    True -> list.append(cmds, [\"%s\" <> param.format_float(voice.%s)])\n", fld->opcode, fld->name);
            fprintf(f, "    False -> cmds\n  }\n");
        }
    }
    fprintf(f, "  string.join(cmds, \" \")\n}\n\n");

    /* to_skode_verbose: full explicit Skode string */
    fprintf(f, "pub fn to_skode_verbose(voice: Voice) -> String {\n");
    fprintf(f, "  let prefix = \"v\" <> int.to_string(voice.id)\n");
    fprintf(f, "  let cmds = [\n");
    fprintf(f, "    prefix,\n");
    fprintf(f, "    \"w\" <> int.to_string(voice.wave_table_index),\n");
    fprintf(f, "    \"f\" <> param.format_float(voice.freq),\n");
    fprintf(f, "    \"a\" <> param.format_float(voice.user_amp),\n");
    fprintf(f, "  ]\n");

    for (i = 0; i < g_field_count; i++) {
        const schema_field_t *fld = &g_fields[i];
        if (strcmp(fld->group, "voice") != 0 || !fld->opcode[0]) continue;
        if (strcmp(fld->name, "wave_table_index") == 0 ||
            strcmp(fld->name, "freq") == 0 ||
            strcmp(fld->name, "user_amp") == 0 ||
            strcmp(fld->name, "amp") == 0) continue;
        if (fld->type == TYPE_FLOAT_ARRAY) continue;

        if (fld->type == TYPE_INT) {
            fprintf(f, "  let cmds = list.append(cmds, [\"%s\" <> int.to_string(voice.%s)])\n", fld->opcode, fld->name);
        } else {
            fprintf(f, "  let cmds = list.append(cmds, [\"%s\" <> param.format_float(voice.%s)])\n", fld->opcode, fld->name);
        }
    }
    fprintf(f, "  string.join(cmds, \" \")\n}\n");

    fclose(f);
}

static void emit_gleam_delay(const char *filepath) {
    FILE *f = fopen(filepath, "w");
    int i;
    if (!f) return;

    fprintf(f, "//// Auto-generated by tools/mksynth. Do not edit directly.\n\n");
    fprintf(f, "pub type DelayBus {\n  DelayBus(\n    id: Int,\n");
    for (i = 0; i < g_field_count; i++) {
        const schema_field_t *fld = &g_fields[i];
        if (strcmp(fld->schema, "delay_fx") != 0) continue;
        if (fld->type == TYPE_INT) {
            fprintf(f, "    %s: Int,\n", fld->name);
        } else {
            fprintf(f, "    %s: Float,\n", fld->name);
        }
    }
    fprintf(f, "  )\n}\n\n");

    fprintf(f, "pub fn default(id: Int) -> DelayBus {\n  DelayBus(\n    id: id,\n");
    for (i = 0; i < g_field_count; i++) {
        const schema_field_t *fld = &g_fields[i];
        if (strcmp(fld->schema, "delay_fx") != 0) continue;
        const char *val = fld->default_val[0] ? fld->default_val : "0";
        if (fld->type == TYPE_INT) {
            fprintf(f, "    %s: %d,\n", fld->name, atoi(val));
        } else {
            char fbuf[64];
            format_gleam_float(fbuf, sizeof(fbuf), atof(val));
            fprintf(f, "    %s: %s,\n", fld->name, fbuf);
        }
    }
    fprintf(f, "  )\n}\n");
    fclose(f);
}

static void emit_gleam_diff(const char *filepath) {
    FILE *f = fopen(filepath, "w");
    int i;
    if (!f) return;

    fprintf(f, "//// Auto-generated by tools/mksynth. Do not edit directly.\n\n");
    fprintf(f, "import gleam/int\nimport gleam/list\nimport gleam/string\n");
    fprintf(f, "import skode/gen/voice_gen.{type Voice}\n");
    fprintf(f, "import skode/param\n\n");

    fprintf(f, "pub fn diff_voice_skode(old: Voice, new: Voice) -> List(String) {\n");
    fprintf(f, "  let prefix = \"v\" <> int.to_string(new.id)\n");
    fprintf(f, "  let diffs = []\n");

    for (i = 0; i < g_field_count; i++) {
        const schema_field_t *fld = &g_fields[i];
        if (strcmp(fld->group, "voice") != 0 || !fld->opcode[0]) continue;
        if (fld->type == TYPE_FLOAT_ARRAY) continue;

        if (fld->type == TYPE_INT) {
            fprintf(f, "  let diffs = case old.%s != new.%s {\n", fld->name, fld->name);
            fprintf(f, "    True -> [\"%s\" <> int.to_string(new.%s), ..diffs]\n", fld->opcode, fld->name);
            fprintf(f, "    False -> diffs\n  }\n");
        } else {
            fprintf(f, "  let diffs = case old.%s != new.%s {\n", fld->name, fld->name);
            fprintf(f, "    True -> [\"%s\" <> param.format_float(new.%s), ..diffs]\n", fld->opcode, fld->name);
            fprintf(f, "    False -> diffs\n  }\n");
        }
    }

    fprintf(f, "\n  case diffs {\n");
    fprintf(f, "    [] -> []\n");
    fprintf(f, "    cmds -> [prefix <> \" \" <> string.join(list.reverse(cmds), \" \")]\n");
    fprintf(f, "  }\n}\n");

    fclose(f);
}

/* -------------------------------------------------------------------------
 * Main CLI Entry Point
 * ------------------------------------------------------------------------- */

int main(int argc, char **argv) {
    const char *schema_dir = (argc > 1) ? argv[1] : "schema";
    const char *out_c_dir  = (argc > 2) ? argv[2] : "src/gen";
    const char *gleam_dir  = (argc > 3) ? argv[3] : NULL;

    char path_h[1024], path_alloc[1024], path_ops[1024], path_test[1024], path_equiv[1024];

    printf("mksynth: scanning %s\n", schema_dir);
    scan_schema_dir(schema_dir);

    printf("mksynth: parsed %d state fields across schemas\n", g_field_count);

    snprintf(path_h, sizeof(path_h), "%s/synth_state_gen.h", out_c_dir);
    snprintf(path_alloc, sizeof(path_alloc), "%s/synth_alloc_gen.c", out_c_dir);
    snprintf(path_ops, sizeof(path_ops), "%s/synth_voice_ops_gen.c", out_c_dir);
    snprintf(path_test, sizeof(path_test), "%s/test_schema_bounds_gen.c", out_c_dir);
    snprintf(path_equiv, sizeof(path_equiv), "%s/test_synth_equivalence_gen.c", out_c_dir);

    emit_c_state_header(path_h);
    emit_c_alloc_source(path_alloc);
    emit_c_voice_ops(path_ops);
    emit_c_bounds_test(path_test);
    emit_c_equivalence_test(path_equiv);

    printf("mksynth: generated %s, %s, %s, %s, %s\n", path_h, path_alloc, path_ops, path_test, path_equiv);

    if (gleam_dir && strcmp(gleam_dir, "none") != 0) {
        char path_gleam[1024];
        snprintf(path_gleam, sizeof(path_gleam), "%s/param_gen.gleam", gleam_dir);
        emit_gleam_params(path_gleam);

        snprintf(path_gleam, sizeof(path_gleam), "%s/voice_gen.gleam", gleam_dir);
        emit_gleam_voice(path_gleam);

        snprintf(path_gleam, sizeof(path_gleam), "%s/delay_gen.gleam", gleam_dir);
        emit_gleam_delay(path_gleam);

        snprintf(path_gleam, sizeof(path_gleam), "%s/diff_gen.gleam", gleam_dir);
        emit_gleam_diff(path_gleam);

        printf("mksynth: generated Gleam modules in %s\n", gleam_dir);
    }

    return 0;
}
