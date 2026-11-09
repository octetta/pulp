/*
 * tools/mkopcodes.c - Host tool to generate opcode dictionary and real-time dispatch tables.
 * Pure ANSI C (C89/C99 standard library only).
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

#define MAX_CLAUSES 2048
#define MAX_WORDS 1024
#define MAX_ARGS 8
#define STR_MAX 128
#define DOC_MAX 4096
#define CODE_MAX 8192
#define SKODE_OP_DISCRETE_BASE 128
#define SKODE_REALTIME_OPS_CAPACITY 1024

typedef enum {
    CLASS_ANY = 0,
    CLASS_NUM,
    CLASS_FINITE,
    CLASS_NAN,
    CLASS_INF_POS,
    CLASS_INF_NEG,
    CLASS_ATOM
} arg_class_t;

typedef enum {
    GRP_OSC = 0,
    GRP_FILTER,
    GRP_ENV,
    GRP_SEQ,
    GRP_VOICE,
    GRP_SYS,
    GRP_COUNT
} group_id_t;

static const char *group_names[] = {
    "osc", "filter", "env", "seq", "voice", "sys"
};

static const char *group_enum_names[] = {
    "SKODE_GRP_OSC", "SKODE_GRP_FILTER", "SKODE_GRP_ENV",
    "SKODE_GRP_SEQ", "SKODE_GRP_VOICE", "SKODE_GRP_SYS"
};

#define MAX_GUARDS 8

typedef struct {
    char name[STR_MAX];
    char alias[STR_MAX];
    group_id_t group;
    char c_name[STR_MAX];
    int is_realtime; /* 1 = realtime, 0 = immediate */
    int type_explicit;
    int needs_string;
    int needs_data;
    char guards[MAX_GUARDS][STR_MAX];
    int guard_count;
    int min_arity;
    int max_arity;
    int arity_explicit;
    int match_count;
    arg_class_t match[MAX_ARGS];
    char atom_literal[MAX_ARGS][STR_MAX];
    char summary[STR_MAX];
    char doc[DOC_MAX];
    char code[CODE_MAX];
    char ifdef[STR_MAX];
    int opcode_id;
    int defined;
} def_clause_t;

typedef struct {
    char name[STR_MAX];
    char alias[STR_MAX];
    group_id_t group;
    int clauses[64];
    int clause_count;
} def_word_t;

static def_clause_t g_clauses[MAX_CLAUSES];
static int g_clause_count = 0;

static def_word_t g_words[MAX_WORDS];
static int g_word_count = 0;

static void trim(char *s) {
    char *p = s;
    int l;
    while (isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    l = (int)strlen(s);
    while (l > 0 && isspace((unsigned char)s[l - 1])) s[--l] = '\0';
}

static void safe_strcpy(char *dst, const char *src, size_t dst_size) {
    if (!dst || dst_size == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    size_t i = 0;
    while (i + 1 < dst_size && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static group_id_t parse_group(const char *s) {
    int i;
    for (i = 0; i < GRP_COUNT; i++) {
        if (strcmp(s, group_names[i]) == 0) return (group_id_t)i;
    }
    return GRP_SYS;
}

static arg_class_t parse_arg_class(const char *s, char *atom_out) {
    atom_out[0] = '\0';
    if (strcmp(s, "num") == 0) return CLASS_NUM;
    if (strcmp(s, "finite") == 0) return CLASS_FINITE;
    if (strcmp(s, "nan") == 0) return CLASS_NAN;
    if (strcmp(s, "+inf") == 0 || strcmp(s, "inf") == 0) return CLASS_INF_POS;
    if (strcmp(s, "-inf") == 0) return CLASS_INF_NEG;
    if (strcmp(s, "any") == 0) return CLASS_ANY;
    if (s[0] == ':') {
        safe_strcpy(atom_out, s + 1, STR_MAX);
        return CLASS_ATOM;
    }
    return CLASS_ANY;
}

static void sanitize_c_name(const char *src, char *dst, size_t dst_len) {
    size_t j = 0;
    if (dst_len == 0) return;
    if (src[0] >= '0' && src[0] <= '9') {
        if (j < dst_len - 1) dst[j++] = '_';
    }
    for (size_t i = 0; src[i] && j < dst_len - 1; i++) {
        unsigned char c = (unsigned char)src[i];
        if (isalnum(c) || c == '_') {
            dst[j++] = c;
        } else {
            dst[j++] = '_';
        }
    }
    dst[j] = '\0';
}

static void finalize_clause(def_clause_t *cur, const char *filepath,
                            const char *last_name, const char *last_alias,
                            const char *last_ifdef,
                            group_id_t last_group, int last_is_realtime,
                            int last_type_explicit, int last_valid) {
    if (cur->name[0] == '\0' && last_valid && last_name[0] != '\0') {
        safe_strcpy(cur->name, last_name, sizeof(cur->name));
        if (cur->alias[0] == '\0' && last_alias[0] != '\0') {
            safe_strcpy(cur->alias, last_alias, sizeof(cur->alias));
        }
        if (cur->group == GRP_OSC && last_group != GRP_OSC) {
            cur->group = last_group;
        }
        if (!cur->type_explicit && last_type_explicit) {
            cur->is_realtime = last_is_realtime;
            cur->type_explicit = 1;
        }
    }

    if (cur->ifdef[0] == '\0' && last_valid && last_ifdef && last_ifdef[0] != '\0') {
        safe_strcpy(cur->ifdef, last_ifdef, sizeof(cur->ifdef));
    }

    if (cur->name[0] == '\0') {
        const char *base = strrchr(filepath, '/');
        if (!base) base = strrchr(filepath, '\\');
        base = base ? base + 1 : filepath;
        char stem[STR_MAX];
        safe_strcpy(stem, base, sizeof(stem));
        char *dot = strrchr(stem, '.');
        if (dot) *dot = '\0';
        safe_strcpy(cur->name, stem, sizeof(cur->name));
    }

    /* Auto-infer group from directory path if not set */
    if (cur->group == GRP_OSC) {
        if (strstr(filepath, "/filter/") || strstr(filepath, "\\filter\\")) cur->group = GRP_FILTER;
        else if (strstr(filepath, "/env/") || strstr(filepath, "\\env\\")) cur->group = GRP_ENV;
        else if (strstr(filepath, "/seq/") || strstr(filepath, "\\seq\\")) cur->group = GRP_SEQ;
        else if (strstr(filepath, "/voice/") || strstr(filepath, "\\voice\\")) cur->group = GRP_VOICE;
        else if (strstr(filepath, "/sys/") || strstr(filepath, "\\sys\\")) cur->group = GRP_SYS;
        else if (strstr(filepath, "/mod/") || strstr(filepath, "\\mod\\")) cur->group = GRP_SYS;
    }

    /* Check for parser context references in clause body */
    int code_refs_parser = (strstr(cur->code, "ands_string") != NULL) ||
                           (strstr(cur->code, "ands_data") != NULL) ||
                           (strstr(cur->code, "ctx->parse") != NULL) ||
                           (strstr(cur->code, "ctx->string_slot") != NULL) ||
                           (strstr(cur->code, "ctx") != NULL) ||
                           (strstr(cur->code, "skode_exec_") != NULL);

    if (strstr(cur->code, "ands_string") != NULL || strstr(cur->code, "ctx->string_slot") != NULL) {
        cur->needs_string = 1;
    }
    if (strstr(cur->code, "ands_data") != NULL) {
        cur->needs_data = 1;
    }

    if (cur->needs_string || cur->needs_data) {
        if (cur->type_explicit && cur->is_realtime) {
            fprintf(stderr, "mkopcodes: fatal error in %s:\n"
                            "  Clause '%s' is marked '@type realtime' but declares '@needs %s%s%s'.\n"
                            "  Real-time audio thread opcodes cannot access parser strings or data.\n",
                    filepath, cur->name,
                    cur->needs_string ? "string" : "",
                    (cur->needs_string && cur->needs_data) ? ", " : "",
                    cur->needs_data ? "data" : "");
            exit(1);
        }
        cur->is_realtime = 0;
        cur->type_explicit = 1;
    }

    if (code_refs_parser) {
        if (cur->type_explicit && cur->is_realtime) {
            fprintf(stderr, "mkopcodes: fatal error in %s:\n"
                            "  Clause '%s' is marked '@type realtime', but its code references parser string/data/context.\n"
                            "  Real-time audio thread opcodes cannot access parser state.\n"
                            "  Please mark this clause as '@type immediate' or use '@needs string' / '@needs data'.\n",
                    filepath, cur->name);
            exit(1);
        }
        cur->is_realtime = 0;
        cur->type_explicit = 1;
    }

    /* Default type to realtime if omitted */
    if (!cur->type_explicit) {
        cur->is_realtime = 1;
    }

    /* Default arity from match if omitted */
    if (!cur->arity_explicit) {
        cur->min_arity = cur->match_count;
        cur->max_arity = cur->match_count;
    }

    /* Synthesize c_name if omitted */
    if (cur->c_name[0] == '\0') {
        char safe[STR_MAX];
        sanitize_c_name(cur->name, safe, sizeof(safe));
        snprintf(cur->c_name, sizeof(cur->c_name), "skode_clause_%s_c%d", safe, g_clause_count + 1);
    }

    /* Default summary if omitted */
    if (cur->summary[0] == '\0') {
        if (cur->alias[0] && strcmp(cur->alias, cur->name) != 0) {
            snprintf(cur->summary, sizeof(cur->summary), "%.60s (%.60s)", cur->name, cur->alias);
        } else {
            safe_strcpy(cur->summary, cur->name, sizeof(cur->summary));
        }
    }

    /* Default doc if omitted */
    if (cur->doc[0] == '\0') {
        snprintf(cur->doc, sizeof(cur->doc), "%s\n", cur->summary);
    }

    cur->defined = 1;
}

static void parse_def_file(const char *filepath) {
    FILE *f = fopen(filepath, "r");
    char line[1024];
    def_clause_t cur;
    int in_doc = 0;
    int in_code = 0;
    char last_name[STR_MAX] = "";
    char last_alias[STR_MAX] = "";
    char last_ifdef[STR_MAX] = "";
    group_id_t last_group = GRP_OSC;
    int last_is_realtime = 1;
    int last_type_explicit = 0;
    int last_valid = 0;

    if (!f) {
        fprintf(stderr, "warning: cannot open %s\n", filepath);
        return;
    }

    memset(&cur, 0, sizeof(cur));

    while (fgets(line, sizeof(line), f)) {
        char trimmed[1024];
        safe_strcpy(trimmed, line, sizeof(trimmed));
        trim(trimmed);

        if (in_code) {
            if (strncmp(trimmed, "@end", 4) == 0 || strncmp(trimmed, "@name", 5) == 0 || strncmp(trimmed, "@match", 6) == 0) {
                in_code = 0;
                if (cur.defined || cur.code[0] != '\0') {
                    if (g_clause_count >= MAX_CLAUSES) {
                        fprintf(stderr, "mkopcodes: fatal error: MAX_CLAUSES (%d) exceeded\n", MAX_CLAUSES);
                        exit(1);
                    }
                    finalize_clause(&cur, filepath, last_name, last_alias, last_ifdef, last_group,
                                    last_is_realtime, last_type_explicit, last_valid);
                    safe_strcpy(last_name, cur.name, sizeof(last_name));
                    safe_strcpy(last_alias, cur.alias, sizeof(last_alias));
                    safe_strcpy(last_ifdef, cur.ifdef, sizeof(last_ifdef));
                    last_group = cur.group;
                    last_is_realtime = cur.is_realtime;
                    last_type_explicit = cur.type_explicit;
                    last_valid = 1;
                    g_clauses[g_clause_count++] = cur;
                    memset(&cur, 0, sizeof(cur));
                }
                if (strncmp(trimmed, "@name", 5) == 0 || strncmp(trimmed, "@match", 6) == 0) {
                    /* fall through to process @name or @match */
                } else {
                    continue;
                }
            } else {
                strncat(cur.code, line, sizeof(cur.code) - strlen(cur.code) - 1);
                continue;
            }
        }

        if (in_doc) {
            if (trimmed[0] == '@') {
                in_doc = 0;
                /* process tag below */
            } else {
                strncat(cur.doc, line, sizeof(cur.doc) - strlen(cur.doc) - 1);
                continue;
            }
        }

        if (trimmed[0] == '#' || trimmed[0] == '\0') continue;

        if (strncmp(trimmed, "@name", 5) == 0) {
            char *val = trimmed + 5;
            trim(val);
            safe_strcpy(cur.name, val, sizeof(cur.name));
            cur.defined = 1;
        } else if (strncmp(trimmed, "@alias", 6) == 0) {
            char *val = trimmed + 6;
            trim(val);
            safe_strcpy(cur.alias, val, sizeof(cur.alias));
            cur.defined = 1;
        } else if (strncmp(trimmed, "@group", 6) == 0) {
            char *val = trimmed + 6;
            trim(val);
            cur.group = parse_group(val);
            cur.defined = 1;
        } else if (strncmp(trimmed, "@c_name", 7) == 0) {
            char *val = trimmed + 7;
            trim(val);
            safe_strcpy(cur.c_name, val, sizeof(cur.c_name));
            cur.defined = 1;
        } else if (strncmp(trimmed, "@type", 5) == 0) {
            char *val = trimmed + 5;
            trim(val);
            cur.is_realtime = (strcmp(val, "realtime") == 0);
            cur.type_explicit = 1;
            cur.defined = 1;
        } else if (strncmp(trimmed, "@needs", 6) == 0) {
            char *val = trimmed + 6;
            trim(val);
            if (strstr(val, "string")) cur.needs_string = 1;
            if (strstr(val, "data")) cur.needs_data = 1;
            cur.defined = 1;
        } else if (strncmp(trimmed, "@guard", 6) == 0) {
            char *val = trimmed + 6;
            trim(val);
            if (val[0] && cur.guard_count < MAX_GUARDS) {
                safe_strcpy(cur.guards[cur.guard_count++], val, STR_MAX);
            }
            cur.defined = 1;
        } else if (strncmp(trimmed, "@ifdef", 6) == 0) {
            char *val = trimmed + 6;
            trim(val);
            safe_strcpy(cur.ifdef, val, sizeof(cur.ifdef));
            cur.defined = 1;
        } else if (strncmp(trimmed, "@if", 3) == 0 && (isspace((unsigned char)trimmed[3]) || trimmed[3] == '\0')) {
            char *val = trimmed + 3;
            trim(val);
            safe_strcpy(cur.ifdef, val, sizeof(cur.ifdef));
            cur.defined = 1;
        } else if (strncmp(trimmed, "@arity", 6) == 0) {
            char *val = trimmed + 6;
            trim(val);
            char *dots = strstr(val, "..");
            if (!dots) dots = strstr(val, "-");
            if (dots) {
                char min_s[32], max_s[32];
                int sep_len = (dots[0] == '.' && dots[1] == '.') ? 2 : 1;
                size_t min_len = (size_t)(dots - val);
                if (min_len >= sizeof(min_s)) min_len = sizeof(min_s) - 1;
                memcpy(min_s, val, min_len);
                min_s[min_len] = '\0';
                safe_strcpy(max_s, dots + sep_len, sizeof(max_s));
                trim(min_s);
                trim(max_s);
                cur.min_arity = atoi(min_s);
                cur.max_arity = atoi(max_s);
            } else {
                cur.min_arity = atoi(val);
                cur.max_arity = cur.min_arity;
            }
            cur.arity_explicit = 1;
            cur.defined = 1;
        } else if (strncmp(trimmed, "@match", 6) == 0) {
            char *p = strchr(trimmed, '(');
            char *end = strchr(trimmed, ')');
            cur.match_count = 0;
            if (p && end && end > p) {
                char inside[STR_MAX];
                int len = (int)(end - p - 1);
                char *token;
                int arg_idx = 0;
                if (len >= STR_MAX) len = STR_MAX - 1;
                safe_strcpy(inside, p + 1, len + 1);
                token = strtok(inside, ",");
                while (token && arg_idx < MAX_ARGS) {
                    trim(token);
                    if (token[0]) {
                        cur.match[arg_idx] = parse_arg_class(token, cur.atom_literal[arg_idx]);
                        arg_idx++;
                    }
                    token = strtok(NULL, ",");
                }
                cur.match_count = arg_idx;
                if (!cur.arity_explicit) {
                    cur.min_arity = arg_idx;
                    cur.max_arity = arg_idx;
                } else if (cur.match_count > 0 && cur.match_count < cur.max_arity) {
                    /* If fewer match patterns than max_arity were given, repeat the last match type */
                    while (cur.match_count < cur.max_arity && cur.match_count < MAX_ARGS) {
                        cur.match[cur.match_count] = cur.match[cur.match_count - 1];
                        cur.match_count++;
                    }
                }
            }
            cur.defined = 1;
        } else if (strncmp(trimmed, "@summary", 8) == 0) {
            char *val = trimmed + 8;
            trim(val);
            safe_strcpy(cur.summary, val, sizeof(cur.summary));
            cur.defined = 1;
        } else if (strncmp(trimmed, "@doc", 4) == 0) {
            in_doc = 1;
            cur.doc[0] = '\0';
            cur.defined = 1;
        } else if (strncmp(trimmed, "@code", 5) == 0) {
            in_code = 1;
            cur.code[0] = '\0';
            cur.defined = 1;
        } else if (strncmp(trimmed, "@end", 4) == 0) {
            if (cur.defined || cur.code[0] != '\0') {
                if (g_clause_count >= MAX_CLAUSES) {
                    fprintf(stderr, "mkopcodes: fatal error: MAX_CLAUSES (%d) exceeded\n", MAX_CLAUSES);
                    exit(1);
                }
                finalize_clause(&cur, filepath, last_name, last_alias, last_ifdef, last_group,
                                last_is_realtime, last_type_explicit, last_valid);
                safe_strcpy(last_name, cur.name, sizeof(last_name));
                safe_strcpy(last_alias, cur.alias, sizeof(last_alias));
                safe_strcpy(last_ifdef, cur.ifdef, sizeof(last_ifdef));
                last_group = cur.group;
                last_is_realtime = cur.is_realtime;
                last_type_explicit = cur.type_explicit;
                last_valid = 1;
                g_clauses[g_clause_count++] = cur;
                memset(&cur, 0, sizeof(cur));
            }
        }
    }

    if (cur.defined || cur.code[0] != '\0') {
        if (g_clause_count >= MAX_CLAUSES) {
            fprintf(stderr, "mkopcodes: fatal error: MAX_CLAUSES (%d) exceeded\n", MAX_CLAUSES);
            exit(1);
        }
        finalize_clause(&cur, filepath, last_name, last_alias, last_ifdef, last_group,
                        last_is_realtime, last_type_explicit, last_valid);
        g_clauses[g_clause_count++] = cur;
    }

    fclose(f);
}

static void scan_directory(const char *dirpath) {
#if defined(_WIN32)
    char pattern[MAX_PATH];
    WIN32_FIND_DATA fd;
    HANDLE hFind;
    snprintf(pattern, sizeof(pattern), "%s\\*", dirpath);
    hFind = FindFirstFile(pattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;
        char subpath[MAX_PATH];
        snprintf(subpath, sizeof(subpath), "%s\\%s", dirpath, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            scan_directory(subpath);
        } else {
            const char *ext = strrchr(fd.cFileName, '.');
            if (ext && strcmp(ext, ".def") == 0) {
                parse_def_file(subpath);
            }
        }
    } while (FindNextFile(hFind, &fd));
    FindClose(hFind);
#else
    DIR *d = opendir(dirpath);
    struct dirent *ent;
    if (!d) return;
    while ((ent = readdir(d)) != NULL) {
        char subpath[1024];
        struct stat st;
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        snprintf(subpath, sizeof(subpath), "%s/%s", dirpath, ent->d_name);
        if (stat(subpath, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                scan_directory(subpath);
            } else if (S_ISREG(st.st_mode)) {
                const char *ext = strrchr(ent->d_name, '.');
                if (ext && strcmp(ext, ".def") == 0) {
                    parse_def_file(subpath);
                }
            }
        }
    }
    closedir(d);
#endif
}

static int compare_words(const void *a, const void *b) {
    const def_word_t *wa = (const def_word_t *)a;
    const def_word_t *wb = (const def_word_t *)b;
    return strcmp(wa->name, wb->name);
}

static int clause_has_arg_guards(const def_clause_t *c) {
    for (int g = 0; g < c->guard_count; g++) {
        if (strcmp(c->guards[g], "voice") != 0 &&
            strcmp(c->guards[g], "voice_bound") != 0 &&
            strcmp(c->guards[g], "voice_valid") != 0) {
            return 1;
        }
    }
    return 0;
}

static int pattern_subsumes(arg_class_t a, arg_class_t b) {
    if (a == CLASS_ANY) return 1;
    if (a == CLASS_NUM) return 1;
    if (a == CLASS_FINITE && b == CLASS_FINITE) return 1;
    if (a == b) return 1;
    return 0;
}

static void check_clause_shadowing(void) {
    for (int w = 0; w < g_word_count; w++) {
        def_word_t *word = &g_words[w];
        for (int i = 0; i < word->clause_count; i++) {
            def_clause_t *c_i = &g_clauses[word->clauses[i]];
            if (clause_has_arg_guards(c_i)) continue;

            for (int j = i + 1; j < word->clause_count; j++) {
                def_clause_t *c_j = &g_clauses[word->clauses[j]];
                if (c_i->min_arity != c_j->min_arity || c_i->max_arity != c_j->max_arity) continue;

                int all_subsumed = 1;
                for (int a = 0; a < c_i->match_count; a++) {
                    if (!pattern_subsumes(c_i->match[a], c_j->match[a])) {
                        all_subsumed = 0;
                        break;
                    }
                }
                if (all_subsumed) {
                    fprintf(stderr, "mkopcodes: warning: in word '%s', clause %d is shadowed by clause %d and will never match\n",
                            word->name, j + 1, i + 1);
                }
            }
        }
    }
}

static void build_words(void) {
    int i, j;
    int next_opcode_id = SKODE_OP_DISCRETE_BASE;

    for (i = 0; i < g_clause_count; i++) {
        def_clause_t *c = &g_clauses[i];
        int found = -1;

        if (c->is_realtime) {
            c->opcode_id = next_opcode_id++;
        } else {
            c->opcode_id = 0;
        }

        for (j = 0; j < g_word_count; j++) {
            if (strcmp(g_words[j].name, c->name) == 0) {
                found = j;
                break;
            }
        }

        if (found < 0) {
            if (g_word_count >= MAX_WORDS) {
                fprintf(stderr, "mkopcodes: fatal error: MAX_WORDS (%d) exceeded\n", MAX_WORDS);
                exit(1);
            }
            found = g_word_count++;
            safe_strcpy(g_words[found].name, c->name, STR_MAX);
            safe_strcpy(g_words[found].alias, c->alias, STR_MAX);
            g_words[found].group = c->group;
            g_words[found].clause_count = 0;
        }

        if (g_words[found].clause_count >= 64) {
            fprintf(stderr, "mkopcodes: fatal error: max clauses per word (64) exceeded for %s\n", g_words[found].name);
            exit(1);
        }
        g_words[found].clauses[g_words[found].clause_count++] = i;
        if (g_words[found].alias[0] == '\0' && c->alias[0] != '\0') {
            safe_strcpy(g_words[found].alias, c->alias, STR_MAX);
        }
    }

    qsort(g_words, g_word_count, sizeof(def_word_t), compare_words);
    check_clause_shadowing();
}

static void emit_pp_if(FILE *f, const char *cond) {
    if (!cond || !cond[0]) return;
    if (strncmp(cond, "defined", 7) == 0 || strchr(cond, ' ') || strchr(cond, '(') || strchr(cond, '!')) {
        fprintf(f, "#if %s\n", cond);
    } else {
        fprintf(f, "#ifdef %s\n", cond);
    }
}

static void emit_header(const char *out_path, int total_rt_opcodes) {
    FILE *f = fopen(out_path, "w");
    int i;
    if (!f) {
        fprintf(stderr, "error: cannot write %s\n", out_path);
        exit(1);
    }

    fprintf(f, "/* Generated by tools/mkopcodes. Do not edit directly. */\n");
    fprintf(f, "#ifndef SKODE_DICT_GEN_H\n");
    fprintf(f, "#define SKODE_DICT_GEN_H\n\n");
    fprintf(f, "#include <stdint.h>\n");
    fprintf(f, "#include <stddef.h>\n\n");

    fprintf(f, "#define SKODE_OP_DISCRETE_BASE 128\n");
    fprintf(f, "#define SKODE_REALTIME_OPS_CAPACITY 1024\n");
    fprintf(f, "#define SKODE_TOTAL_DISCRETE_OPCODES %d\n\n", total_rt_opcodes);

    fprintf(f, "typedef enum {\n");
    for (i = 0; i < GRP_COUNT; i++) {
        fprintf(f, "    %s = %d,\n", group_enum_names[i], i);
    }
    fprintf(f, "    SKODE_GRP_COUNT = %d\n", GRP_COUNT);
    fprintf(f, "} skode_group_t;\n\n");

    fprintf(f, "typedef enum {\n");
    fprintf(f, "    MATCH_ANY = 0,\n");
    fprintf(f, "    MATCH_NUM,\n");
    fprintf(f, "    MATCH_FINITE,\n");
    fprintf(f, "    MATCH_NAN,\n");
    fprintf(f, "    MATCH_INF_POS,\n");
    fprintf(f, "    MATCH_INF_NEG,\n");
    fprintf(f, "    MATCH_ATOM\n");
    fprintf(f, "} skode_arg_class_t;\n\n");

    fprintf(f, "#define SKODE_CLAUSE_MAX_ARGS 8\n\n");

    fprintf(f, "typedef enum {\n");
    fprintf(f, "    SKODE_CLAUSE_RT_SAFE      = (1 << 0),\n");
    fprintf(f, "    SKODE_CLAUSE_IMMEDIATE    = (1 << 1),\n");
    fprintf(f, "    SKODE_CLAUSE_NEEDS_STRING = (1 << 2),\n");
    fprintf(f, "    SKODE_CLAUSE_NEEDS_DATA   = (1 << 3)\n");
    fprintf(f, "} skode_clause_flags_t;\n\n");

    fprintf(f, "typedef int (*skode_realtime_fn_t)(int voice, const float *args, int argc);\n");
    fprintf(f, "typedef int (*skode_clause_guard_fn_t)(const double *args, int argc);\n");
    fprintf(f, "struct skode_s;\nstruct ands_s;\n");
    fprintf(f, "typedef int (*skode_immediate_fn_t)(struct skode_s *ctx, struct ands_s *s, const double *args, int argc);\n\n");

    fprintf(f, "typedef struct {\n");
    fprintf(f, "    uint8_t min_arity;\n");
    fprintf(f, "    uint8_t max_arity;\n");
    fprintf(f, "    uint8_t arity;\n");
    fprintf(f, "    skode_arg_class_t match[SKODE_CLAUSE_MAX_ARGS];\n");
    fprintf(f, "    uint32_t atom_values[SKODE_CLAUSE_MAX_ARGS];\n");
    fprintf(f, "    uint16_t opcode_id;\n");
    fprintf(f, "    uint16_t flags;\n");
    fprintf(f, "    skode_clause_guard_fn_t guard;\n");
    fprintf(f, "    skode_realtime_fn_t rt_handler;\n");
    fprintf(f, "    skode_immediate_fn_t imm_handler;\n");
    fprintf(f, "    const char *c_name;\n");
    fprintf(f, "    const char *summary;\n");
    fprintf(f, "    const char *doc;\n");
    fprintf(f, "} skode_clause_t;\n\n");

    fprintf(f, "typedef struct skode_def_word {\n");
    fprintf(f, "    const char *name;\n");
    fprintf(f, "    const char *alias;\n");
    fprintf(f, "    skode_group_t group;\n");
    fprintf(f, "    uint8_t clause_count;\n");
    fprintf(f, "    const skode_clause_t *clauses;\n");
    fprintf(f, "    struct skode_def_word *next;\n");
    fprintf(f, "} skode_def_word_t;\n\n");
    fprintf(f, "#ifndef SKODE_LEGACY_WORD_T_DEFINED\n");
    fprintf(f, "typedef skode_def_word_t skode_word_t;\n");
    fprintf(f, "#endif\n\n");

    fprintf(f, "/* Discrete Opcode Enum */\n");
    fprintf(f, "typedef enum {\n");
    fprintf(f, "    SKODE_OP_DISCRETE_NONE = 0,\n");
    for (i = 0; i < g_clause_count; i++) {
        if (g_clauses[i].is_realtime) {
            fprintf(f, "    SKODE_OP_DISCRETE_%s = %d,\n", g_clauses[i].c_name, g_clauses[i].opcode_id);
        }
    }
    fprintf(f, "    SKODE_OP_DISCRETE_MAX\n");
    fprintf(f, "} skode_discrete_opcode_t;\n\n");

    fprintf(f, "/* Declarations */\n");
    fprintf(f, "extern const skode_def_word_t g_skode_words[];\n");
    fprintf(f, "extern const size_t g_skode_words_count;\n");
    fprintf(f, "extern const skode_realtime_fn_t g_skode_realtime_ops[SKODE_REALTIME_OPS_CAPACITY];\n\n");

    fprintf(f, "/* Lookup and introspection */\n");
    fprintf(f, "#include <stdio.h>\n");
    fprintf(f, "struct skred_engine_s;\n");
    fprintf(f, "const skode_def_word_t *skode_dict_find(const char *name_or_alias);\n");
    fprintf(f, "const skode_clause_t *skode_clause_match(const skode_def_word_t *word, int argc, const double *args);\n");
    fprintf(f, "const skode_clause_t *skode_clause_match_ex(const skode_def_word_t *word, int argc, const double *args, unsigned var_mask);\n");
    fprintf(f, "const char *skode_discrete_opcode_name(uint16_t opcode);\n");
    fprintf(f, "const char *skode_group_name(skode_group_t group);\n");
    fprintf(f, "skode_group_t skode_group_from_name(const char *name);\n");
    fprintf(f, "void skode_clause_explain_mismatch(const skode_def_word_t *word, int argc, const double *args, char *buf, size_t buf_size);\n");
    fprintf(f, "void skode_dict_help(FILE *out, const char *group_or_word);\n");
    fprintf(f, "void skode_engine_dump(struct skred_engine_s *e, FILE *out, const char *group_filter, int use_long_names);\n\n");

    fprintf(f, "#endif /* SKODE_DICT_GEN_H */\n");
    fclose(f);
}

static void emit_c_source(const char *out_path) {
    FILE *f = fopen(out_path, "w");
    int i, j;
    if (!f) {
        fprintf(stderr, "error: cannot write %s\n", out_path);
        exit(1);
    }

    fprintf(f, "/* Generated by tools/mkopcodes. Do not edit directly. */\n");
    fprintf(f, "#define SKODE_LEGACY_WORD_T_DEFINED 1\n");
    fprintf(f, "#include \"skode_dict_gen.h\"\n");
    fprintf(f, "#include \"synth.h\"\n");
    fprintf(f, "#include \"synth-types.h\"\n");
    fprintf(f, "#include \"synth-state.h\"\n");
    fprintf(f, "#include \"seq.h\"\n");
    fprintf(f, "#include \"skode.h\"\n");
    fprintf(f, "#include \"skode-internal.h\"\n");
    fprintf(f, "#include \"control-events.h\"\n");
    fprintf(f, "#include \"recorder.h\"\n");
    fprintf(f, "#include \"api.h\"\n");
    fprintf(f, "#include \"scope-ipc.h\"\n");
    fprintf(f, "#include \"polyphony.h\"\n");
    fprintf(f, "#include \"midi_player.h\"\n");
    fprintf(f, "#include <math.h>\n");
    fprintf(f, "#include <stdlib.h>\n");
    fprintf(f, "#include <string.h>\n");
    fprintf(f, "#include <stdio.h>\n\n");

    fprintf(f, "static const char *s_group_names[] = {\n");
    fprintf(f, "    \"osc\", \"filter\", \"env\", \"seq\", \"voice\", \"sys\"\n");
    fprintf(f, "};\n\n");

    fprintf(f, "/* Forward declarations of clause implementations */\n");
    for (i = 0; i < g_clause_count; i++) {
        def_clause_t *c = &g_clauses[i];
        if (c->is_realtime) {
            fprintf(f, "int %s(int voice, const float *args, int argc);\n", c->c_name);
        } else {
            fprintf(f, "int %s(skode_t *ctx, ands_t *s, const double *args, int argc);\n", c->c_name);
        }
    }
    fprintf(f, "\n");

    fprintf(f, "/* Pure C clause implementations */\n");
    for (i = 0; i < g_clause_count; i++) {
        def_clause_t *c = &g_clauses[i];
        if (c->is_realtime) {
            fprintf(f, "int %s(int voice, const float *args, int argc) {\n", c->c_name);
            fprintf(f, "    (void)argc;\n");
            int g;
            for (g = 0; g < c->guard_count; g++) {
                if (strcmp(c->guards[g], "voice") == 0 ||
                    strcmp(c->guards[g], "voice_bound") == 0 ||
                    strcmp(c->guards[g], "voice_valid") == 0) {
                    fprintf(f, "    if (voice < 0 || voice >= synth_config.voice_max) return -1;\n");
                } else {
                    fprintf(f, "    if (!(%s)) return -1;\n", c->guards[g]);
                }
            }
            if (c->ifdef[0]) {
                emit_pp_if(f, c->ifdef);
            }
            fprintf(f, "%s\n", c->code[0] ? c->code : "    return 0;");
            if (c->code[0] && !strstr(c->code, "return")) {
                fprintf(f, "    return 0;\n");
            }
            if (c->ifdef[0]) {
                fprintf(f, "#else\n");
                fprintf(f, "    (void)voice;\n");
                fprintf(f, "    (void)args;\n");
                fprintf(f, "    return -1;\n");
                fprintf(f, "#endif\n");
            }
            fprintf(f, "}\n\n");

            /* Generate immediate bridge */
            fprintf(f, "static int %s_imm_bridge(skode_t *ctx, ands_t *s, const double *args, int argc) {\n", c->c_name);
            fprintf(f, "    float fargs[SKODE_CLAUSE_MAX_ARGS];\n");
            fprintf(f, "    int i;\n");
            fprintf(f, "    (void)s;\n");
            fprintf(f, "    for (i = 0; i < argc && i < SKODE_CLAUSE_MAX_ARGS; i++) fargs[i] = (float)args[i];\n");
            fprintf(f, "    return %s(ctx ? ctx->voice : 0, fargs, argc);\n", c->c_name);
            fprintf(f, "}\n\n");
        } else {
            fprintf(f, "int %s(skode_t *ctx, ands_t *s, const double *args, int argc) {\n", c->c_name);
            int g;
            for (g = 0; g < c->guard_count; g++) {
                if (strcmp(c->guards[g], "voice") == 0 ||
                    strcmp(c->guards[g], "voice_bound") == 0 ||
                    strcmp(c->guards[g], "voice_valid") == 0) {
                    fprintf(f, "    if (!ctx || ctx->voice < 0 || ctx->voice >= synth_config.voice_max) return -1;\n");
                } else {
                    fprintf(f, "    if (!(%s)) return -1;\n", c->guards[g]);
                }
            }
            if (c->ifdef[0]) {
                emit_pp_if(f, c->ifdef);
            }
            fprintf(f, "%s\n", c->code[0] ? c->code : "    return 0;");
            if (c->code[0] && !strstr(c->code, "return")) {
                fprintf(f, "    return 0;\n");
            }
            if (c->ifdef[0]) {
                fprintf(f, "#else\n");
                fprintf(f, "    (void)ctx;\n");
                fprintf(f, "    (void)s;\n");
                fprintf(f, "    (void)args;\n");
                fprintf(f, "    (void)argc;\n");
                fprintf(f, "    return -1;\n");
                fprintf(f, "#endif\n");
            }
            fprintf(f, "}\n\n");
        }
    }

    /* Emit clause arrays for each word */
    for (i = 0; i < g_word_count; i++) {
        def_word_t *w = &g_words[i];
        for (j = 0; j < w->clause_count; j++) {
            def_clause_t *c = &g_clauses[w->clauses[j]];
            if (clause_has_arg_guards(c)) {
                int g;
                fprintf(f, "static int %s_guard(const double *args, int argc) {\n", c->c_name);
                fprintf(f, "    (void)argc;\n");
                for (g = 0; g < c->guard_count; g++) {
                    if (strcmp(c->guards[g], "voice") != 0 &&
                        strcmp(c->guards[g], "voice_bound") != 0 &&
                        strcmp(c->guards[g], "voice_valid") != 0) {
                        fprintf(f, "    if (!(%s)) return 0;\n", c->guards[g]);
                    }
                }
                fprintf(f, "    return 1;\n");
                fprintf(f, "}\n\n");
            }
        }
        fprintf(f, "static const skode_clause_t s_clauses_%s[] = {\n", w->name);
        for (j = 0; j < w->clause_count; j++) {
            def_clause_t *c = &g_clauses[w->clauses[j]];
            int k;
            fprintf(f, "    {\n");
            fprintf(f, "        .min_arity = %d,\n", c->min_arity);
            fprintf(f, "        .max_arity = %d,\n", c->max_arity);
            fprintf(f, "        .arity = %d,\n", c->min_arity);
            fprintf(f, "        .match = {");
            for (k = 0; k < MAX_ARGS; k++) {
                const char *cn = "MATCH_ANY";
                switch (c->match[k]) {
                    case CLASS_NUM: cn = "MATCH_NUM"; break;
                    case CLASS_FINITE: cn = "MATCH_FINITE"; break;
                    case CLASS_NAN: cn = "MATCH_NAN"; break;
                    case CLASS_INF_POS: cn = "MATCH_INF_POS"; break;
                    case CLASS_INF_NEG: cn = "MATCH_INF_NEG"; break;
                    case CLASS_ATOM: cn = "MATCH_ATOM"; break;
                    default: cn = "MATCH_ANY"; break;
                }
                fprintf(f, "%s%s", cn, (k + 1 < MAX_ARGS) ? ", " : "");
            }
            fprintf(f, "},\n");
            fprintf(f, "        .atom_values = {0},\n");
            fprintf(f, "        .opcode_id = %d,\n", c->opcode_id);
            fprintf(f, "        .flags = %s%s%s,\n",
                    c->is_realtime ? "SKODE_CLAUSE_RT_SAFE" : "SKODE_CLAUSE_IMMEDIATE",
                    c->needs_string ? " | SKODE_CLAUSE_NEEDS_STRING" : "",
                    c->needs_data ? " | SKODE_CLAUSE_NEEDS_DATA" : "");
            if (clause_has_arg_guards(c)) {
                fprintf(f, "        .guard = %s_guard,\n", c->c_name);
            } else {
                fprintf(f, "        .guard = NULL,\n");
            }
            if (c->is_realtime) {
                fprintf(f, "        .rt_handler = %s,\n", c->c_name);
                fprintf(f, "        .imm_handler = %s_imm_bridge,\n", c->c_name);
            } else {
                fprintf(f, "        .rt_handler = NULL,\n");
                fprintf(f, "        .imm_handler = %s,\n", c->c_name);
            }
            fprintf(f, "        .c_name = \"%s\",\n", c->c_name);
            fprintf(f, "        .summary = \"%s\",\n", c->summary);
            fprintf(f, "        .doc = \"%s\"\n", c->summary);
            fprintf(f, "    }%s\n", (j + 1 < w->clause_count) ? "," : "");
        }
        fprintf(f, "};\n\n");
    }

    /* Emit g_skode_words */
    fprintf(f, "const skode_def_word_t g_skode_words[] = {\n");
    for (i = 0; i < g_word_count; i++) {
        def_word_t *w = &g_words[i];
        fprintf(f, "    {\n");
        fprintf(f, "        .name = \"%s\",\n", w->name);
        if (w->alias[0]) {
            fprintf(f, "        .alias = \"%s\",\n", w->alias);
        } else {
            fprintf(f, "        .alias = NULL,\n");
        }
        fprintf(f, "        .group = %s,\n", group_enum_names[w->group]);
        fprintf(f, "        .clause_count = %d,\n", w->clause_count);
        fprintf(f, "        .clauses = s_clauses_%s,\n", w->name);
        fprintf(f, "        .next = NULL\n");
        fprintf(f, "    }%s\n", (i + 1 < g_word_count) ? "," : "");
    }
    fprintf(f, "};\n\n");
    fprintf(f, "const size_t g_skode_words_count = %d;\n\n", g_word_count);

    /* Dense Real-Time Jump Table */
    fprintf(f, "/* Dense real-time jump table */\n");
    fprintf(f, "const skode_realtime_fn_t g_skode_realtime_ops[SKODE_REALTIME_OPS_CAPACITY] = {\n");
    for (i = 0; i < SKODE_REALTIME_OPS_CAPACITY; i++) {
        for (j = 0; j < g_clause_count; j++) {
            if (g_clauses[j].is_realtime && g_clauses[j].opcode_id == i) {
                fprintf(f, "    [%d] = %s,\n", i, g_clauses[j].c_name);
                break;
            }
        }
    }
    fprintf(f, "};\n\n");

    /* Discrete opcode uppercase names */
    fprintf(f, "static const char *s_discrete_opcode_names[SKODE_REALTIME_OPS_CAPACITY] = {\n");
    for (i = 0; i < SKODE_REALTIME_OPS_CAPACITY; i++) {
        for (j = 0; j < g_clause_count; j++) {
            if (g_clauses[j].is_realtime && g_clauses[j].opcode_id == i) {
                char upper[STR_MAX];
                int k;
                safe_strcpy(upper, g_clauses[j].name, sizeof(upper));
                for (k = 0; upper[k]; k++) {
                    upper[k] = (char)toupper((unsigned char)upper[k]);
                }
                fprintf(f, "    [%d] = \"%s\",\n", i, upper);
                break;
            }
        }
    }
    fprintf(f, "};\n\n");
    fprintf(f, "const char *skode_discrete_opcode_name(uint16_t opcode) {\n");
    fprintf(f, "    if (opcode >= SKODE_REALTIME_OPS_CAPACITY) return NULL;\n");
    fprintf(f, "    return s_discrete_opcode_names[opcode];\n");
    fprintf(f, "}\n\n");

    /* Emit O(1) static hash table for skode_dict_find */
    #define DICT_HASH_SIZE 2048
    uint16_t hash_tab[DICT_HASH_SIZE];
    for (i = 0; i < DICT_HASH_SIZE; i++) hash_tab[i] = 0xFFFF;
    for (i = 0; i < g_word_count; i++) {
        /* insert canonical name */
        const char *name = g_words[i].name;
        uint32_t h = 5381;
        const unsigned char *p = (const unsigned char *)name;
        while (*p) h = ((h << 5) + h) + *p++;
        uint32_t pos = h & (DICT_HASH_SIZE - 1);
        while (hash_tab[pos] != 0xFFFF) pos = (pos + 1) & (DICT_HASH_SIZE - 1);
        hash_tab[pos] = (uint16_t)i;

        /* insert alias if present and distinct from name */
        if (g_words[i].alias[0] && strcmp(g_words[i].alias, name) != 0) {
            const char *alias = g_words[i].alias;
            h = 5381;
            p = (const unsigned char *)alias;
            while (*p) h = ((h << 5) + h) + *p++;
            pos = h & (DICT_HASH_SIZE - 1);
            while (hash_tab[pos] != 0xFFFF) pos = (pos + 1) & (DICT_HASH_SIZE - 1);
            hash_tab[pos] = (uint16_t)i;
        }
    }

    fprintf(f, "static const uint16_t s_dict_hash_table[%d] = {\n", DICT_HASH_SIZE);
    for (i = 0; i < DICT_HASH_SIZE; i++) {
        fprintf(f, " 0x%04X,", hash_tab[i]);
        if ((i + 1) % 16 == 0) fprintf(f, "\n");
    }
    fprintf(f, "};\n\n");

    /* Lookup and clause matching implementations */
    fprintf(f, "const skode_def_word_t *skode_dict_find(const char *name_or_alias) {\n");
    fprintf(f, "    if (!name_or_alias || !name_or_alias[0]) return NULL;\n");
    fprintf(f, "    uint32_t h = 5381;\n");
    fprintf(f, "    const unsigned char *p = (const unsigned char *)name_or_alias;\n");
    fprintf(f, "    while (*p) h = ((h << 5) + h) + *p++;\n");
    fprintf(f, "    uint32_t idx = h & (%d - 1);\n", DICT_HASH_SIZE);
    fprintf(f, "    for (uint32_t step = 0; step < %d; step++) {\n", DICT_HASH_SIZE);
    fprintf(f, "        uint16_t w_idx = s_dict_hash_table[(idx + step) & (%d - 1)];\n", DICT_HASH_SIZE);
    fprintf(f, "        if (w_idx == 0xFFFF) return NULL;\n");
    fprintf(f, "        const skode_def_word_t *w = &g_skode_words[w_idx];\n");
    fprintf(f, "        if (strcmp(w->name, name_or_alias) == 0) return w;\n");
    fprintf(f, "        if (w->alias && strcmp(w->alias, name_or_alias) == 0) return w;\n");
    fprintf(f, "    }\n");
    fprintf(f, "    return NULL;\n");
    fprintf(f, "}\n\n");

    fprintf(f, "static int arg_matches(double val, skode_arg_class_t cls) {\n");
    fprintf(f, "    switch (cls) {\n");
    fprintf(f, "        case MATCH_ANY:    return 1;\n");
    fprintf(f, "        case MATCH_NUM:    return 1;\n");
    fprintf(f, "        case MATCH_FINITE: return isfinite(val);\n");
    fprintf(f, "        case MATCH_NAN:    return isnan(val);\n");
    fprintf(f, "        case MATCH_INF_POS: return isinf(val) && val > 0.0;\n");
    fprintf(f, "        case MATCH_INF_NEG: return isinf(val) && val < 0.0;\n");
    fprintf(f, "        case MATCH_ATOM:   return 1;\n");
    fprintf(f, "        default:           return 0;\n");
    fprintf(f, "    }\n");
    fprintf(f, "}\n\n");

    fprintf(f, "const skode_clause_t *skode_clause_match_ex(const skode_def_word_t *word, int argc, const double *args, unsigned var_mask) {\n");
    fprintf(f, "    uint8_t i;\n");
    fprintf(f, "    if (!word) return NULL;\n");
    fprintf(f, "    for (i = 0; i < word->clause_count; i++) {\n");
    fprintf(f, "        const skode_clause_t *c = &word->clauses[i];\n");
    fprintf(f, "        int match = 1;\n");
    fprintf(f, "        int a;\n");
    fprintf(f, "        if (argc < c->min_arity || argc > c->max_arity) continue;\n");
    fprintf(f, "        for (a = 0; a < argc; a++) {\n");
    fprintf(f, "            if (var_mask & (1U << a)) {\n");
    fprintf(f, "                if (c->match[a] != MATCH_ANY && c->match[a] != MATCH_NUM && c->match[a] != MATCH_FINITE) {\n");
    fprintf(f, "                    match = 0;\n");
    fprintf(f, "                    break;\n");
    fprintf(f, "                }\n");
    fprintf(f, "                continue;\n");
    fprintf(f, "            }\n");
    fprintf(f, "            if (!arg_matches(args[a], c->match[a])) {\n");
    fprintf(f, "                match = 0;\n");
    fprintf(f, "                break;\n");
    fprintf(f, "            }\n");
    fprintf(f, "        }\n");
    fprintf(f, "        if (match && c->guard && !c->guard(args, argc)) {\n");
    fprintf(f, "            match = 0;\n");
    fprintf(f, "        }\n");
    fprintf(f, "        if (match) return c;\n");
    fprintf(f, "    }\n");
    fprintf(f, "    return NULL;\n");
    fprintf(f, "}\n\n");
    fprintf(f, "const skode_clause_t *skode_clause_match(const skode_def_word_t *word, int argc, const double *args) {\n");
    fprintf(f, "    return skode_clause_match_ex(word, argc, args, 0);\n");
    fprintf(f, "}\n\n");

    fprintf(f, "static const char *s_arg_class_name(skode_arg_class_t cls) {\n");
    fprintf(f, "    switch (cls) {\n");
    fprintf(f, "        case MATCH_ANY: return \"any\";\n");
    fprintf(f, "        case MATCH_NUM: return \"num\";\n");
    fprintf(f, "        case MATCH_FINITE: return \"finite\";\n");
    fprintf(f, "        case MATCH_NAN: return \"nan\";\n");
    fprintf(f, "        case MATCH_INF_POS: return \"+inf\";\n");
    fprintf(f, "        case MATCH_INF_NEG: return \"-inf\";\n");
    fprintf(f, "        case MATCH_ATOM: return \":atom\";\n");
    fprintf(f, "        default: return \"any\";\n");
    fprintf(f, "    }\n");
    fprintf(f, "}\n\n");

    fprintf(f, "void skode_clause_explain_mismatch(const skode_def_word_t *word, int argc, const double *args, char *buf, size_t buf_size) {\n");
    fprintf(f, "    if (!buf || buf_size == 0) return;\n");
    fprintf(f, "    buf[0] = '\\0';\n");
    fprintf(f, "    if (!word) {\n");
    fprintf(f, "        snprintf(buf, buf_size, \"unknown command\");\n");
    fprintf(f, "        return;\n");
    fprintf(f, "    }\n");
    fprintf(f, "    uint8_t arities[16];\n");
    fprintf(f, "    int arity_count = 0;\n");
    fprintf(f, "    int has_matching_arity = 0;\n");
    fprintf(f, "    uint8_t i;\n");
    fprintf(f, "    for (i = 0; i < word->clause_count; i++) {\n");
    fprintf(f, "        const skode_clause_t *c = &word->clauses[i];\n");
    fprintf(f, "        if (argc >= c->min_arity && argc <= c->max_arity) has_matching_arity = 1;\n");
    fprintf(f, "        for (uint8_t ar = c->min_arity; ar <= c->max_arity; ar++) {\n");
    fprintf(f, "            int found = 0;\n");
    fprintf(f, "            for (int j = 0; j < arity_count; j++) {\n");
    fprintf(f, "                if (arities[j] == ar) { found = 1; break; }\n");
    fprintf(f, "            }\n");
    fprintf(f, "            if (!found && arity_count < 16) arities[arity_count++] = ar;\n");
    fprintf(f, "        }\n");
    fprintf(f, "    }\n");
    fprintf(f, "    if (!has_matching_arity) {\n");
    fprintf(f, "        char ar_str[64] = \"\";\n");
    fprintf(f, "        for (int j = 0; j < arity_count; j++) {\n");
    fprintf(f, "            char num[16];\n");
    fprintf(f, "            snprintf(num, sizeof(num), \"%%s%%d\", (j > 0 ? \", \" : \"\"), arities[j]);\n");
    fprintf(f, "            strncat(ar_str, num, sizeof(ar_str) - strlen(ar_str) - 1);\n");
    fprintf(f, "        }\n");
    fprintf(f, "        snprintf(buf, buf_size, \"expected arity [%%s]\", ar_str);\n");
    fprintf(f, "        return;\n");
    fprintf(f, "    }\n");
    fprintf(f, "    for (i = 0; i < word->clause_count; i++) {\n");
    fprintf(f, "        const skode_clause_t *c = &word->clauses[i];\n");
    fprintf(f, "        if (argc < c->min_arity || argc > c->max_arity) continue;\n");
    fprintf(f, "        for (int a = 0; a < argc; a++) {\n");
    fprintf(f, "            if (!arg_matches(args[a], c->match[a])) {\n");
    fprintf(f, "                const char *val_type = \"unknown\";\n");
    fprintf(f, "                if (isnan(args[a])) val_type = \"nan\";\n");
    fprintf(f, "                else if (isinf(args[a])) val_type = (args[a] > 0 ? \"+inf\" : \"-inf\");\n");
    fprintf(f, "                else val_type = \"finite\";\n");
    fprintf(f, "                snprintf(buf, buf_size, \"clause %%d expected arg %%d as %%s, got %%s (%%g)\",\n");
    fprintf(f, "                         i + 1, a, s_arg_class_name(c->match[a]), val_type, args[a]);\n");
    fprintf(f, "                return;\n");
    fprintf(f, "            }\n");
    fprintf(f, "        }\n");
    fprintf(f, "        if (c->guard && !c->guard(args, argc)) {\n");
    fprintf(f, "            snprintf(buf, buf_size, \"clause %%d guard condition failed\", i + 1);\n");
    fprintf(f, "            return;\n");
    fprintf(f, "        }\n");
    fprintf(f, "    }\n");
    fprintf(f, "    snprintf(buf, buf_size, \"clause pattern matching failed\");\n");
    fprintf(f, "}\n\n");

    fprintf(f, "const char *skode_group_name(skode_group_t group) {\n");
    fprintf(f, "    if (group >= 0 && group < SKODE_GRP_COUNT) return s_group_names[group];\n");
    fprintf(f, "    return \"sys\";\n");
    fprintf(f, "}\n\n");

    fprintf(f, "skode_group_t skode_group_from_name(const char *name) {\n");
    fprintf(f, "    int i;\n");
    fprintf(f, "    if (!name) return SKODE_GRP_SYS;\n");
    fprintf(f, "    for (i = 0; i < SKODE_GRP_COUNT; i++) {\n");
    fprintf(f, "        if (strcmp(name, s_group_names[i]) == 0) return (skode_group_t)i;\n");
    fprintf(f, "    }\n");
    fprintf(f, "    return SKODE_GRP_SYS;\n");
    fprintf(f, "}\n\n");

    fprintf(f, "void skode_dict_help(FILE *out, const char *group_or_word) {\n");
    fprintf(f, "    size_t g, i;\n");
    fprintf(f, "    if (!out) out = stdout;\n");
    fprintf(f, "    if (!group_or_word || !group_or_word[0] || strcmp(group_or_word, \"help\") == 0) {\n");
    fprintf(f, "        fprintf(out, \"Available command groups:\\n\");\n");
    fprintf(f, "        for (g = 0; g < SKODE_GRP_COUNT; g++) {\n");
    fprintf(f, "            size_t count = 0;\n");
    fprintf(f, "            for (i = 0; i < g_skode_words_count; i++) {\n");
    fprintf(f, "                if (g_skode_words[i].group == (skode_group_t)g) count++;\n");
    fprintf(f, "            }\n");
    fprintf(f, "            fprintf(out, \"  %%-8s (%%zu words)\\n\", s_group_names[g], count);\n");
    fprintf(f, "        }\n");
    fprintf(f, "        fprintf(out, \"\\nType 'help <group>' to see commands in a group, or 'help <command>' for details.\\n\");\n");
    fprintf(f, "        return;\n");
    fprintf(f, "    }\n\n");

    fprintf(f, "    /* Check if group_or_word is a group name */\n");
    fprintf(f, "    for (g = 0; g < SKODE_GRP_COUNT; g++) {\n");
    fprintf(f, "        if (strcmp(group_or_word, s_group_names[g]) == 0) {\n");
    fprintf(f, "            fprintf(out, \"Commands in group '%%s':\\n\", s_group_names[g]);\n");
    fprintf(f, "            for (i = 0; i < g_skode_words_count; i++) {\n");
    fprintf(f, "                if (g_skode_words[i].group == (skode_group_t)g) {\n");
    fprintf(f, "                    const skode_def_word_t *w = &g_skode_words[i];\n");
    fprintf(f, "                    int c;\n");
    fprintf(f, "                    if (w->alias) {\n");
    fprintf(f, "                        fprintf(out, \"  %%-16s (alias: %%-4s) [%%d clause(s)]\\n\", w->name, w->alias, w->clause_count);\n");
    fprintf(f, "                    } else {\n");
    fprintf(f, "                        fprintf(out, \"  %%-16s [%%d clause(s)]\\n\", w->name, w->clause_count);\n");
    fprintf(f, "                    }\n");
    fprintf(f, "                    for (c = 0; c < w->clause_count; c++) {\n");
    fprintf(f, "                        if (w->clauses[c].min_arity == w->clauses[c].max_arity) {\n");
    fprintf(f, "                            fprintf(out, \"    clause %%d (arity %%d): %%s\\n\", c + 1, w->clauses[c].min_arity,\n");
    fprintf(f, "                                    w->clauses[c].summary ? w->clauses[c].summary : \"\");\n");
    fprintf(f, "                        } else {\n");
    fprintf(f, "                            fprintf(out, \"    clause %%d (arity %%d..%%d): %%s\\n\", c + 1, w->clauses[c].min_arity, w->clauses[c].max_arity,\n");
    fprintf(f, "                                    w->clauses[c].summary ? w->clauses[c].summary : \"\");\n");
    fprintf(f, "                        }\n");
    fprintf(f, "                    }\n");
    fprintf(f, "                }\n");
    fprintf(f, "            }\n");
    fprintf(f, "            return;\n");
    fprintf(f, "        }\n");
    fprintf(f, "    }\n\n");

    fprintf(f, "    /* Check if group_or_word is a command name or alias */\n");
    fprintf(f, "    {\n");
    fprintf(f, "        const skode_def_word_t *w = skode_dict_find(group_or_word);\n");
    fprintf(f, "        if (w) {\n");
    fprintf(f, "            int c;\n");
    fprintf(f, "            fprintf(out, \"Command: %%s\\n\", w->name);\n");
    fprintf(f, "            if (w->alias) fprintf(out, \"Alias:   %%s\\n\", w->alias);\n");
    fprintf(f, "            fprintf(out, \"Group:   %%s\\n\", skode_group_name(w->group));\n");
    fprintf(f, "            fprintf(out, \"Clauses (%%d):\\n\", w->clause_count);\n");
    fprintf(f, "            for (c = 0; c < w->clause_count; c++) {\n");
    fprintf(f, "                const skode_clause_t *cl = &w->clauses[c];\n");
    fprintf(f, "                if (cl->min_arity == cl->max_arity) {\n");
    fprintf(f, "                    fprintf(out, \"  [%%d] arity %%d\", c + 1, cl->min_arity);\n");
    fprintf(f, "                } else {\n");
    fprintf(f, "                    fprintf(out, \"  [%%d] arity %%d..%%d\", c + 1, cl->min_arity, cl->max_arity);\n");
    fprintf(f, "                }\n");
    fprintf(f, "                if (cl->flags & SKODE_CLAUSE_RT_SAFE) fprintf(out, \" (RT_SAFE)\");\n");
    fprintf(f, "                if (cl->flags & SKODE_CLAUSE_IMMEDIATE) fprintf(out, \" (IMMEDIATE)\");\n");
    fprintf(f, "                if (cl->flags & SKODE_CLAUSE_NEEDS_STRING) fprintf(out, \" [NEEDS_STRING]\");\n");
    fprintf(f, "                if (cl->flags & SKODE_CLAUSE_NEEDS_DATA) fprintf(out, \" [NEEDS_DATA]\");\n");
    fprintf(f, "                fprintf(out, \"\\n      Summary: %%s\\n\", cl->summary ? cl->summary : \"\");\n");
    fprintf(f, "                if (cl->doc && cl->doc[0]) {\n");
    fprintf(f, "                    fprintf(out, \"      Doc:     %%s\\n\", cl->doc);\n");
    fprintf(f, "                }\n");
    fprintf(f, "            }\n");
    fprintf(f, "            return;\n");
    fprintf(f, "        }\n");
    fprintf(f, "    }\n\n");
    fprintf(f, "    fprintf(out, \"Unknown command or group '%%s'.\\n\", group_or_word);\n");
    fprintf(f, "}\n\n");

    fprintf(f, "#ifdef sv\n#undef sv\n#endif\n");
    fprintf(f, "#ifdef sw\n#undef sw\n#endif\n\n");
    fprintf(f, "void skode_engine_dump(struct skred_engine_s *e, FILE *out, const char *group_filter, int use_long_names) {\n");
    fprintf(f, "    skode_group_t grp = SKODE_GRP_COUNT;\n");
    fprintf(f, "    int v, max_v, var;\n");
    fprintf(f, "    if (!out) out = stdout;\n");
    fprintf(f, "    if (!e) e = &skred_global_engine;\n");
    fprintf(f, "    if (group_filter && group_filter[0] && strcmp(group_filter, \"all\") != 0) {\n");
    fprintf(f, "        grp = skode_group_from_name(group_filter);\n");
    fprintf(f, "    }\n");
    fprintf(f, "    fprintf(out, \"=== SKRED ENGINE DUMP (group: %%s, names: %%s) ===\\n\",\n");
    fprintf(f, "            group_filter ? group_filter : \"all\", use_long_names ? \"canonical\" : \"alias\");\n\n");

    fprintf(f, "    max_v = e->config.voice_max > 0 ? e->config.voice_max : 64;\n\n");

    fprintf(f, "    /* OSC group */\n");
    fprintf(f, "    if (grp == SKODE_GRP_COUNT || grp == SKODE_GRP_OSC) {\n");
    fprintf(f, "        fprintf(out, \"[OSC]\\n\");\n");
    fprintf(f, "        for (v = 0; v < max_v; v++) {\n");
    fprintf(f, "            if (e->sv.freq && e->sv.freq[v] > 0.0f) {\n");
    fprintf(f, "                int wt = e->sv.wave_table_index ? e->sv.wave_table_index[v] : 0;\n");
    fprintf(f, "                if (use_long_names) {\n");
    fprintf(f, "                    fprintf(out, \"  voice %%d: wave=%%d, freq=%%g\\n\", v, wt, e->sv.freq[v]);\n");
    fprintf(f, "                } else {\n");
    fprintf(f, "                    fprintf(out, \"  v%%d: w%%d f%%g\\n\", v, wt, e->sv.freq[v]);\n");
    fprintf(f, "                }\n");
    fprintf(f, "            }\n");
    fprintf(f, "        }\n");
    fprintf(f, "    }\n\n");

    fprintf(f, "    /* FILTER group */\n");
    fprintf(f, "    if (grp == SKODE_GRP_COUNT || grp == SKODE_GRP_FILTER) {\n");
    fprintf(f, "        fprintf(out, \"[FILTER]\\n\");\n");
    fprintf(f, "        for (v = 0; v < max_v; v++) {\n");
    fprintf(f, "            if (e->sv.filter_freq && e->sv.filter_freq[v] > 0.0f) {\n");
    fprintf(f, "                float res = e->sv.filter_res ? e->sv.filter_res[v] : 0.0f;\n");
    fprintf(f, "                if (use_long_names) {\n");
    fprintf(f, "                    fprintf(out, \"  voice %%d: filter_freq=%%g, res=%%g\\n\", v, e->sv.filter_freq[v], res);\n");
    fprintf(f, "                } else {\n");
    fprintf(f, "                    fprintf(out, \"  v%%d: k%%g k_res=%%g\\n\", v, e->sv.filter_freq[v], res);\n");
    fprintf(f, "                }\n");
    fprintf(f, "            }\n");
    fprintf(f, "        }\n");
    fprintf(f, "    }\n\n");

    fprintf(f, "    /* ENV group */\n");
    fprintf(f, "    if (grp == SKODE_GRP_COUNT || grp == SKODE_GRP_ENV) {\n");
    fprintf(f, "        fprintf(out, \"[ENV]\\n\");\n");
    fprintf(f, "        for (v = 0; v < max_v; v++) {\n");
    fprintf(f, "            if (e->sv.amp && e->sv.amp[v] > 0.0f) {\n");
    fprintf(f, "                if (use_long_names) {\n");
    fprintf(f, "                    fprintf(out, \"  voice %%d: amp=%%g\\n\", v, e->sv.amp[v]);\n");
    fprintf(f, "                } else {\n");
    fprintf(f, "                    fprintf(out, \"  v%%d: a%%g\\n\", v, e->sv.amp[v]);\n");
    fprintf(f, "                }\n");
    fprintf(f, "            }\n");
    fprintf(f, "        }\n");
    fprintf(f, "    }\n\n");

    fprintf(f, "    /* SYS group */\n");
    fprintf(f, "    if (grp == SKODE_GRP_COUNT || grp == SKODE_GRP_SYS) {\n");
    fprintf(f, "        fprintf(out, \"[SYS]\\n\");\n");
    fprintf(f, "        for (var = 0; var < ANDS_VAR_MAX; var++) {\n");
    fprintf(f, "            if (global_var[var] != 0.0) {\n");
    fprintf(f, "                fprintf(out, \"  $%%-3d = %%g\\n\", var, global_var[var]);\n");
    fprintf(f, "            }\n");
    fprintf(f, "        }\n");
    fprintf(f, "    }\n\n");

    fprintf(f, "    /* SEQ group */\n");
    fprintf(f, "    if (grp == SKODE_GRP_COUNT || grp == SKODE_GRP_SEQ) {\n");
    fprintf(f, "        fprintf(out, \"[SEQ]\\n\");\n");
    fprintf(f, "        fprintf(out, \"  master_tick: %%llu, current_pattern: %%d\\n\",\n");
    fprintf(f, "                (unsigned long long)seq_master_tick(), seq_current_pattern);\n");
    fprintf(f, "    }\n");
    fprintf(f, "}\n");

    fclose(f);
}

static void emit_markdown(const char *out_path, int total_rt) {
    FILE *f = fopen(out_path, "w");
    int g, i, c, a;
    if (!f) {
        fprintf(stderr, "error: cannot write %s\n", out_path);
        exit(1);
    }

    fprintf(f, "# Pulp / Skred Command & Opcode Reference\n\n");
    fprintf(f, "Release: **0.83.0**  \n");
    fprintf(f, "Unified Opcode Dictionary and Execution Engine reference for `pulp` and `skred`.\n\n");
    fprintf(f, "Generated automatically by [`tools/mkopcodes`](file:///home/stewartj/book/pulp/tools/mkopcodes.c) from modular opcode definitions in `doc/opcodes/`.\n\n");
    fprintf(f, "---\n\n");

    fprintf(f, "## Architecture & System Overview\n\n");
    fprintf(f, "- **Total Command Words**: %d\n", g_word_count);
    fprintf(f, "- **Total Dispatch Clauses**: %d\n", g_clause_count);
    fprintf(f, "- **Real-Time Audio Opcode Handlers**: %d (audio-thread safe, zero allocation, zero string lookup)\n", total_rt);
    fprintf(f, "- **Immediate / Control Plane Clauses**: %d (REPL / parser / file system / diagnostics)\n\n", g_clause_count - total_rt);

    fprintf(f, "### Functional Command Groups\n\n");
    fprintf(f, "| Group | Identifier | Words | Description |\n");
    fprintf(f, "|:---|:---|:---:|:---|\n");
    for (g = 0; g < GRP_COUNT; g++) {
        int cnt = 0;
        for (i = 0; i < g_word_count; i++) {
            if (g_words[i].group == (group_id_t)g) cnt++;
        }
        const char *desc = "";
        switch (g) {
            case GRP_OSC:    desc = "Oscillators, waveforms, frequency, tuning, detune"; break;
            case GRP_FILTER: desc = "Filter cutoff frequency, resonance, modes, wavefolding"; break;
            case GRP_ENV:    desc = "Envelopes, ADSR parameters, curve shaping, segment times"; break;
            case GRP_SEQ:    desc = "Sequencer patterns, steps, repeats, tempo, control events"; break;
            case GRP_VOICE:  desc = "Voice allocation, polyphony, routing, amplitude, pan"; break;
            case GRP_SYS:    desc = "System configuration, MIDI, data buffers, macros, hardware"; break;
            default:         desc = "System commands"; break;
        }
        fprintf(f, "| %s | `%s` | %d | %s |\n", group_names[g], group_names[g], cnt, desc);
    }
    fprintf(f, "\n---\n\n");

    /* Group-by-group sections */
    for (g = 0; g < GRP_COUNT; g++) {
        fprintf(f, "## Group: `%s`\n\n", group_names[g]);

        /* Summary table for the group */
        fprintf(f, "| Command | Alias | Clauses | Arity | Execution | Summary |\n");
        fprintf(f, "|:---|:---|:---:|:---:|:---:|:---|\n");

        for (i = 0; i < g_word_count; i++) {
            if (g_words[i].group != (group_id_t)g) continue;
            def_word_t *w = &g_words[i];

            int min_arity = 99, max_arity = 0;
            int any_rt = 0, any_imm = 0;
            for (c = 0; c < w->clause_count; c++) {
                def_clause_t *cl = &g_clauses[w->clauses[c]];
                if (cl->min_arity < min_arity) min_arity = cl->min_arity;
                if (cl->max_arity > max_arity) max_arity = cl->max_arity;
                if (cl->is_realtime) any_rt = 1;
                else any_imm = 1;
            }
            if (min_arity == 99) min_arity = 0;

            char arity_str[32];
            if (min_arity == max_arity) snprintf(arity_str, sizeof(arity_str), "%d", min_arity);
            else snprintf(arity_str, sizeof(arity_str), "%d..%d", min_arity, max_arity);

            const char *exec_str = (any_rt && any_imm) ? "Hybrid" : (any_rt ? "Real-Time" : "Immediate");
            const char *alias_str = w->alias[0] ? w->alias : "-";
            const char *summary_str = w->clause_count > 0 ? g_clauses[w->clauses[0]].summary : "";

            fprintf(f, "| [`%s`](#%s) | `%s` | %d | %s | %s | %s |\n",
                    w->name, w->name, alias_str, w->clause_count, arity_str, exec_str, summary_str);
        }
        fprintf(f, "\n");

        /* Detailed entries for each command */
        for (i = 0; i < g_word_count; i++) {
            if (g_words[i].group != (group_id_t)g) continue;
            def_word_t *w = &g_words[i];

            fprintf(f, "### `%s`\n\n", w->name);
            if (w->alias[0]) {
                fprintf(f, "- **Alias**: `%s`\n", w->alias);
            }
            fprintf(f, "- **Group**: `%s`\n", group_names[w->group]);
            fprintf(f, "- **Clause Count**: %d\n\n", w->clause_count);

            for (c = 0; c < w->clause_count; c++) {
                def_clause_t *cl = &g_clauses[w->clauses[c]];
                fprintf(f, "#### Clause %d: `%s(", c + 1, w->name);
                for (a = 0; a < cl->match_count; a++) {
                    const char *mn = "any";
                    switch (cl->match[a]) {
                        case CLASS_NUM:     mn = "num"; break;
                        case CLASS_FINITE:  mn = "finite"; break;
                        case CLASS_NAN:     mn = "nan"; break;
                        case CLASS_INF_POS: mn = "+inf"; break;
                        case CLASS_INF_NEG: mn = "-inf"; break;
                        case CLASS_ATOM:    mn = cl->atom_literal[a][0] ? cl->atom_literal[a] : ":atom"; break;
                        default:            mn = "any"; break;
                    }
                    fprintf(f, "%s%s", mn, (a + 1 < cl->match_count) ? ", " : "");
                }
                fprintf(f, ")`\n\n");

                if (cl->min_arity == cl->max_arity) {
                    fprintf(f, "- **Arity**: `%d`\n", cl->min_arity);
                } else {
                    fprintf(f, "- **Arity**: `%d..%d`\n", cl->min_arity, cl->max_arity);
                }
                fprintf(f, "- **Execution Model**: `%s`\n", cl->is_realtime ? "realtime (audio thread safe)" : "immediate (control plane)");
                if (cl->is_realtime && cl->opcode_id > 0) {
                    fprintf(f, "- **Discrete Opcode ID**: `%d` (`%s`)\n", cl->opcode_id, cl->c_name);
                }
                if (cl->ifdef[0]) {
                    fprintf(f, "- **Feature Flag (`#ifdef`)**: `%s`\n", cl->ifdef);
                }
                if (cl->needs_string || cl->needs_data) {
                    fprintf(f, "- **Parser State Requirements**: %s%s%s\n",
                            cl->needs_string ? "`string`" : "",
                            (cl->needs_string && cl->needs_data) ? ", " : "",
                            cl->needs_data ? "`data`" : "");
                }
                if (cl->guard_count > 0) {
                    int g;
                    fprintf(f, "- **Guards**: ");
                    for (g = 0; g < cl->guard_count; g++) {
                        fprintf(f, "`%s`%s", cl->guards[g], (g + 1 < cl->guard_count ? ", " : ""));
                    }
                    fprintf(f, "\n");
                }
                if (cl->summary[0]) {
                    fprintf(f, "- **Summary**: %s\n", cl->summary);
                }
                if (cl->doc[0] && strcmp(cl->doc, cl->summary) != 0) {
                    /* Trim trailing whitespace/newlines from doc */
                    char doc_copy[DOC_MAX];
                    safe_strcpy(doc_copy, cl->doc, sizeof(doc_copy));
                    trim(doc_copy);
                    if (doc_copy[0]) {
                        fprintf(f, "\n**Description & Details**:\n\n```text\n%s\n```\n", doc_copy);
                    }
                }
                fprintf(f, "\n");
            }
            fprintf(f, "---\n\n");
        }
    }

    fclose(f);
}

static void escape_doc_string(const char *in, char *out, size_t out_size) {
    size_t o = 0;
    while (*in && o + 2 < out_size) {
        if (*in == '"' || *in == '\\') {
            out[o++] = '\\';
            out[o++] = *in++;
        } else if (*in == '\n') {
            out[o++] = '\\';
            out[o++] = 'n';
            in++;
        } else if (*in == '\r') {
            in++;
        } else {
            out[o++] = *in++;
        }
    }
    out[o] = '\0';
}

static void emit_doc_inc(const char *filepath) {
    FILE *f = fopen(filepath, "w");
    int i;
    if (!f) {
        fprintf(stderr, "error: cannot open %s for writing\n", filepath);
        exit(1);
    }

    fprintf(f, "/* Generated by tools/mkopcodes from doc/opcodes. Do not edit. */\n");

    for (i = 0; i < g_word_count; i++) {
        const def_word_t *w = &g_words[i];
        const char *grp = group_names[w->group];
        const char *sum = "";
        char sum_escaped[STR_MAX * 2];
        int c;

        for (c = 0; c < w->clause_count; c++) {
            if (g_clauses[w->clauses[c]].summary[0]) {
                sum = g_clauses[w->clauses[c]].summary;
                break;
            }
        }
        if (!sum[0]) sum = w->name;
        escape_doc_string(sum, sum_escaped, sizeof(sum_escaped));

        /* Canonical word entry */
        fprintf(f, "  { \"command.%s\", \"%s.def\", 0, \"name: %s\\ncategory: %s\\nsummary: %s\\n\" },\n",
                w->name, w->name, w->name, grp, sum_escaped);

        /* Alias entry if distinct */
        if (w->alias[0] && strcmp(w->alias, w->name) != 0) {
            fprintf(f, "  { \"command.%s\", \"%s.def\", 0, \"name: %s\\ncategory: %s\\nsummary: %s\\n\" },\n",
                    w->alias, w->name, w->alias, grp, sum_escaped);
        }
    }

    /* Engine commands */
    fprintf(f, "  { \"command.-restart\", \"api.c\", 0, \"name: -restart\\ncategory: sys\\nsummary: restart the audio engine [voices=v] [frames=f] [port=p]\\n\" },\n");
    fprintf(f, "  { \"command.-wave\", \"api.c\", 0, \"name: -wave\\ncategory: sys\\nsummary: dump wave slot as compressed base64\\n\" },\n");
    fprintf(f, "  { \"command.-upwave\", \"api.c\", 0, \"name: -upwave\\ncategory: sys\\nsummary: stream wave data via START DATA CANCEL COMMIT\\n\" },\n");

    fclose(f);
}

int main(int argc, char **argv) {
    const char *def_dir = (argc > 1) ? argv[1] : "doc/opcodes";
    const char *out_h   = (argc > 2) ? argv[2] : "src/gen/skode_dict_gen.h";
    const char *out_c   = (argc > 3) ? argv[3] : "src/gen/skred_opcodes_gen.c";
    const char *out_md  = (argc > 4) ? argv[4] : "doc/REFERENCE.md";
    const char *out_inc = (argc > 5) ? argv[5] : "src/gen/skode-doc.inc";
    int i, total_rt = 0;

    printf("mkopcodes: scanning %s\n", def_dir);
    scan_directory(def_dir);

    build_words();

    for (i = 0; i < g_clause_count; i++) {
        if (g_clauses[i].is_realtime) total_rt++;
    }

    printf("mkopcodes: parsed %d clauses across %d words (%d realtime opcodes)\n",
           g_clause_count, g_word_count, total_rt);

    emit_header(out_h, total_rt);
    emit_c_source(out_c);
    if (out_md && strcmp(out_md, "-") != 0 && strcmp(out_md, "none") != 0) {
        emit_markdown(out_md, total_rt);
    }
    if (out_inc && strcmp(out_inc, "-") != 0 && strcmp(out_inc, "none") != 0) {
        emit_doc_inc(out_inc);
        printf("mkopcodes: successfully generated %s, %s, %s, and %s\n", out_h, out_c, out_md, out_inc);
    } else {
        printf("mkopcodes: successfully generated %s, %s, and %s\n", out_h, out_c, out_md);
    }

    return 0;
}
