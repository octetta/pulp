#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "ands.h"

/**
 * ANDS GRAMMAR (EBNF-ish)
 * 
 * chunk      = (element)* (';' | EOT)
 * element    = atom | number | string | array | variable | defer | comment
 * atom       = [a-zA-Z!@%^&*_=:"'<>?/]+
 * number     = [-]?[0-9]+('.'[0-9]+)?([eE][-+]?[0-9]+)? | '.' | '-' | 'e'
 * string     = '{' [^}]* '}' ... migrating to [,] soon, but either work now
 * array      = '(' (number (',' | ' ')*)* ')'
 * variable   = '$' [0-9]{1,3}
 * defer      = ('+' | '~') number chunk
 * comment    = '#' [^\n;]* ('\n' | ';')
 * separator  = ' ' | ',' | '\t' | '\n' | '\r'
 * 
 * EXECUTION MODEL:
 * - Numbers accumulate in arg[] array
 * - When atom completes, FUNCTION callback fires with accumulated arg[]
 * - arg[] is cleared after callback returns 0
 * - ';' or EOT triggers CHUNK_END event
 * - Atom names are packed into 32-bit integers for fast switching
 * 
 * SPECIAL FEATURES:
 * - Whitespace optional between atoms and numbers: "v0w1f440" works
 * - Commas and spaces are interchangeable separators
 * - Bare '.', '-', or 'e' produce NaN (for future "default value" feature)
 * - Arguments can precede atoms (side effect): "440 f" works like "f440"
 * - Variables $0-$127 for value storage and recall
 */

#define IS_NUMBER(c) (isdigit(c) || strchr("-.", c))
#define IS_SEPARATOR(c) (isspace(c) || c == ',')
#define IS_STRING(c) (c == '[')
#define IS_STRING_END(c) (c == ']')
#define IS_ARRAY(c) (c == '(')
#define IS_ARRAY_END(c) (c == ')')
#define IS_VARIABLE(c) (c == '$')
#define IS_COMMENT(c) (c == '#')
#define IS_CHUNK_END(c) (c == ';' || c == 0x04) // 0x04 ASCII EOT / end of xmit
#define IS_DEFER(c) (c == '+' || c == '~')
#define IS_ATOM(c) (isalpha(c) || strchr("!@%^&*_=:\"'<>?/", c))
#define IS_NUMBER_EX(c) (isxdigit(c) || strchr("-.eExX", c))

static double ands_strtod(char *s) {
  double d = NAN;
  if (s[1] == '\0' && (s[0] == '-' || s[0] == 'e' || s[0] == '.')) return d;
  d = strtod(s, NULL);
  return d;
}

static double ands_defer_time_clamp(double d) {
  return (d < 0.0) ? 0.0 : d;
}

static int ands_var_parse(char **ptr, char *end) {
  int value = 0;
  int digits = 0;

  while (*ptr < end && digits < 3 && isdigit(**ptr)) {
    value = (value * 10) + (**ptr - '0');
    (*ptr)++;
    digits++;
  }

  return digits ? value : -1;
}

#define ARG_MAX (8)
#define ATOM_MAX (4)
#define ATOM_NIL (0x2d2d2d2d)
#define VAR_MAX (ANDS_VAR_MAX)

// ============================================================================
// UNIFIED BUFFER MANAGEMENT
// ============================================================================

typedef struct {
    char *data;
    int len;
    int cap;
} buffer_t;

static void buffer_init(buffer_t *b, int cap) {
    b->data = (char*)malloc(cap * sizeof(char));
    b->len = 0;
    b->cap = cap;
    if (b->data) b->data[0] = '\0';
}

static void buffer_free(buffer_t *b) {
    if (b->data) free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static void buffer_clear(buffer_t *b) {
    b->len = 0;
    if (b->data) b->data[0] = '\0';
}

static void buffer_push(buffer_t *b, char c) {
    if (b->data && b->len < b->cap - 1) {
        b->data[b->len++] = c;
        b->data[b->len] = '\0';
    }
}

static char* buffer_str(buffer_t *b) {
    return b->data;
}

#define NUM_BUF_LEN (128)
#define STRING_BUF_LEN (256)
#define DATA_BUF_LEN (1024)

// ============================================================================
// MAIN SKODE STRUCTURE
// ============================================================================

#define STRING_BUF_MOD (2)

typedef struct ands_s {
    // Unified buffers
    buffer_t num;
    buffer_t string[STRING_BUF_MOD];
    int string_idx;
    int string_read_idx;
    buffer_t atom;
    buffer_t defer;
    
    // Data array
    double *data;
    int data_len;
    int data_cap;
    
    // Defer state
    double defer_num;
    char defer_mode;
    int defer_var;
    
    // Argument stack
    double arg[ARG_MAX];
    int arg_var[ARG_MAX];
    int arg_len;
    int arg_cap;
    
    // Atom state
    atom_t atom_num;
    
    // Parser state
    int state;
    
    // Variables
    double local_var[VAR_MAX];
    double *global_var;
    double *global_save;
    
    // Callback
    int (*fn)(struct ands_s *s, int info);
    void *user;
    
    // Mode
    int mode;
    
    // Trace
    int trace;
} ands_t;

// ============================================================================
// ATOM ENCODING - Simpler implementation with documentation
// ============================================================================

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

/**
 * Pack atom string into 32-bit integer for fast switching.
 * Example: "f" becomes 'f---' (0x662d2d2d)
 * Uses network byte order for consistency.
 */
static void atom_finish(ands_t *s) {
    uint32_t i = ATOM_NIL;  // Start with '----' (0x2d2d2d2d)
    char *p = (char *)&i;
    int len = s->atom.len < ATOM_MAX ? s->atom.len : ATOM_MAX;
    for (int n = 0; n < len; n++) {
        p[n] = s->atom.data[n];
    }
    s->atom_num = ntohl(i);
}

static void atom_reset(ands_t *s) {
    s->atom_num = ATOM_NIL;
}

char* atom_string(int i) {
    static char str[5] = "****";
    char *p = (char *)&i;
    for (int j = 0; j < 4; j++) {
        str[3-j] = p[j];
    }
    return str;
}

// ============================================================================
// ACTION FUNCTIONS - Extracted for clarity
// ============================================================================

static void action_finish_number(ands_t *s) {
    double val = ands_strtod(buffer_str(&s->num));
    if (s->trace) printf("# ARG_PUSH %g\n", val);
    if (s->arg_len < s->arg_cap) {
        s->arg[s->arg_len] = val;
        s->arg_var[s->arg_len] = -1;
        s->arg_len++;
    }
    buffer_clear(&s->num);
}

static void action_finish_atom(ands_t *s) {
    if (s->trace) printf("# ATOM %s\n", buffer_str(&s->atom));

    if (s->atom_num != ATOM_NIL) {
        if (s->fn(s, FUNCTION) == 0) {
            s->arg_len = 0;  // Clear args
        }
        atom_reset(s);
    }

    atom_finish(s);
    buffer_clear(&s->atom);
}

static void action_finish_defer(ands_t *s) {
    if (s->trace) printf("# DEFER\n");
    s->fn(s, DEFER);
    buffer_clear(&s->defer);
}

static void action_finish_array(ands_t *s) {
    // Push final number if any
    if (s->num.len > 0) {
        if (s->data && s->data_len < s->data_cap)
            s->data[s->data_len++] = ands_strtod(buffer_str(&s->num));
        buffer_clear(&s->num);
    }
}

static void action_chunk_end(ands_t *s) {
    // Handle leftover atom
    if (s->atom_num != ATOM_NIL) {
        if (s->trace) printf("# left-over ATOM\n");
        s->fn(s, FUNCTION);
        atom_reset(s);
    }

    // Handle leftover defer
    if (s->defer.len > 0) {
        if (s->trace) printf("# left-over DEFER\n");
        s->fn(s, DEFER);
        buffer_clear(&s->defer);
    }

    if (s->trace) printf("# CHUNK_END\n");
    s->fn(s, CHUNK_END);
    s->arg_len = 0;  // Clear args
}

// ============================================================================
// STATE MACHINE
// ============================================================================

int ands_consume(ands_t *s, char *line) {
    char *ptr = line;
    char *end = ptr + strlen(ptr);

    while (1) {
        if (ptr >= end) {
            switch (s->state) {
                case GET_ATOM:
                    action_finish_atom(s);
                    s->state = START;
                    break;
                case GET_NUMBER:
                    action_finish_number(s);
                    s->state = START;
                    break;
                default:
                    break;
            }
            break;
        }

    reprocess:
        switch (s->state) {
            case START:
                if (IS_NUMBER(*ptr)) {
                    buffer_clear(&s->num);
                    buffer_push(&s->num, *ptr);
                    s->state = GET_NUMBER;
                }
                else if (IS_SEPARATOR(*ptr)) { /* skip */ }
                else if (IS_STRING(*ptr))    {
                  action_finish_atom(s);
                  buffer_clear(&s->string[s->string_idx]);
                  s->state = GET_STRING;
                }
                else if (IS_ARRAY(*ptr))     {
                  buffer_clear(&s->num);
                  s->data_len = 0;
                  s->state = GET_ARRAY;
                }
                else if (IS_VARIABLE(*ptr))  { s->state = GET_VARIABLE; }
                else if (IS_COMMENT(*ptr))   { s->state = GET_COMMENT; }
                else if (IS_CHUNK_END(*ptr)) { action_chunk_end(s); s->state = START; }
                else if (IS_DEFER(*ptr))     { action_chunk_end(s); s->defer_mode = *ptr; s->state = GET_DEFER_NUMBER; }
                else if (iscntrl(*ptr))      { /* skip control chars */ }
                else {
                    buffer_clear(&s->atom);
                    buffer_push(&s->atom, *ptr);
                    s->state = GET_ATOM;
                }
                break;

            case GET_NUMBER:
                if (IS_NUMBER(*ptr)) {
                    buffer_push(&s->num, *ptr);
                } else if (*ptr == '$') {
                    printf("# VAR after number?\n");
                } else {
                    action_finish_number(s);
                    s->state = START;
                    goto reprocess;
                }
                break;

            case GET_STRING:
                if (IS_STRING_END(*ptr)) {
                    // Flip buffers: reading from current, writing to next
                    s->string_read_idx = s->string_idx;
                    s->fn(s, GOT_STRING);
                    s->string_idx = (s->string_idx + 1) % STRING_BUF_MOD;
                    s->state = START;
                } else {
                    buffer_push(&s->string[s->string_idx], *ptr);
                }
                break;

            case GET_ARRAY:
                if (IS_ARRAY_END(*ptr)) {
                    action_finish_array(s);
                    s->fn(s, GOT_ARRAY);
                    s->state = START;
                } else if (IS_NUMBER_EX(*ptr)) {
                    buffer_push(&s->num, *ptr);
                } else if (IS_SEPARATOR(*ptr)) {
                    if (s->num.len > 0) {
                        if (s->data_len < (s->data_cap-1)) s->data[s->data_len++] = ands_strtod(buffer_str(&s->num));
                        buffer_clear(&s->num);
                    }
                }
                break;

            case GET_COMMENT:
                if (IS_CHUNK_END(*ptr)) {
                    action_chunk_end(s);
                    s->state = START;
                } else if (*ptr == '\n') {
                    s->state = START;
                }
                break;

            case GET_VARIABLE:
                if (isdigit(*ptr)) {
                    int variable = ands_var_parse(&ptr, end);
                    double d = ands_get_local(s, variable);
                    if (s->arg_len < s->arg_cap) {
                        s->arg[s->arg_len] = d;
                        s->arg_var[s->arg_len] = variable;
                        s->arg_len++;
                    }
                    if (s->trace) printf("# GET_VARIABLE %d (%g)\n", variable, d);
                    s->state = START;
                    ptr--;
                } else {
                    if (s->trace) puts("# not a var");
                    s->state = START;
                    goto reprocess;
                }
                break;

            case GET_DEFER_NUMBER:
                if (IS_NUMBER(*ptr)) {
                    buffer_push(&s->num, *ptr);
                } else if (IS_VARIABLE(*ptr)) {
                    ptr++;
                    if (ptr < end && isdigit(*ptr)) {
                        int variable = ands_var_parse(&ptr, end);
                        s->defer_num = ands_defer_time_clamp(ands_get_local(s, variable));
                        s->defer_var = variable;
                        ptr--;
                    } else {
                        s->defer_num = 0.0;
                        s->defer_var = -1;
                    }
                    buffer_clear(&s->num);
                    s->state = GET_DEFER_STRING;
                } else {
                    s->defer_num = ands_defer_time_clamp(ands_strtod(buffer_str(&s->num)));
                    s->defer_var = -1;
                    buffer_clear(&s->num);
                    s->state = GET_DEFER_STRING;
                    goto reprocess;
                }
                break;

            case GET_DEFER_STRING:
                if (IS_DEFER(*ptr)) {
                    s->defer_mode = *ptr;
                    action_finish_defer(s);
                    s->state = GET_DEFER_NUMBER;
                } else if (IS_CHUNK_END(*ptr)) {
                    action_finish_defer(s);
                    s->state = START;
                } else {
                    buffer_push(&s->defer, *ptr);
                }
                break;

            case GET_ATOM:
                if (IS_ATOM(*ptr)) {
                    buffer_push(&s->atom, *ptr);
                } else {
                    action_finish_atom(s);
                    s->state = START;
                    goto reprocess;
                }
                break;

            default:
                if (s->trace) puts("# default -> START");
                s->state = START;
                break;
        }
        ptr++;
    }

    if (s->mode == 0) {
        action_chunk_end(s);
        s->state = START;
    }
    return 0;
}

// ============================================================================
// PUBLIC API
// ============================================================================

ands_t *ands_new(int (*fn)(ands_t *s, int info), void *user) {
    if (!fn) return NULL;
    ands_t *s = (ands_t*)calloc(1, sizeof(ands_t));
    if (!s) return NULL;

    s->global_var = s->local_var;
    s->global_save = s->local_var;
    for (int i = 0; i < VAR_MAX; i++) {
        s->local_var[i] = 0;
    }

    buffer_init(&s->num, NUM_BUF_LEN);
    buffer_init(&s->string[0], STRING_BUF_LEN);
    buffer_init(&s->string[1], STRING_BUF_LEN);
    s->string_idx = 0;
    s->string_read_idx = 0;
    buffer_init(&s->atom, ATOM_MAX + 1);
    buffer_init(&s->defer, STRING_BUF_LEN);

    s->data_cap = DATA_BUF_LEN;
    s->data_len = 0;
    s->data = (double*)malloc(s->data_cap * sizeof(double));
    if (!s->num.data || !s->string[0].data || !s->string[1].data ||
        !s->atom.data || !s->defer.data || !s->data) {
        ands_free(s);
        return NULL;
    }

    s->defer_num = 0;
    s->defer_mode = '?';
    s->defer_var = -1;

    s->arg_cap = ARG_MAX;
    s->arg_len = 0;
    for (int i = 0; i < ARG_MAX; i++) s->arg_var[i] = -1;

    s->atom_num = ATOM_NIL;

    s->fn = fn;
    s->user = user;

    s->state = START;
    s->mode = 0;
    s->trace = 0;

    return s;
}

void ands_free(ands_t *s) {
    if (!s) return;
    buffer_free(&s->num);
    buffer_free(&s->string[0]);
    buffer_free(&s->string[1]);
    buffer_free(&s->atom);
    buffer_free(&s->defer);

    if (s->data) free(s->data);
    s->data = NULL;
    s->data_cap = 0;
    s->data_len = 0;
    free(s);
}

// Accessor functions
uint32_t ands_atom_num(ands_t *s) { return s->atom_num; }
int ands_arg_len(ands_t *s) { return s->arg_len; }
double *ands_arg(ands_t *s) { return s->arg; }
int ands_arg_var(ands_t *s, int n) {
  return n >= 0 && n < s->arg_len ? s->arg_var[n] : -1;
}
void *ands_user(ands_t *s) { return s->user; }
char *ands_string(ands_t *s) { return buffer_str(&s->string[s->string_read_idx]); }
int ands_string_len(ands_t *s) { return s->string[s->string_read_idx].len; }
void ands_chunk_mode(ands_t *s, int mode) { s->mode = mode; }
int ands_chunk_mode_get(ands_t *s) { return s->mode; }
void ands_trace_set(ands_t *s, int n) { s->trace = n; }
double ands_defer_num(ands_t *s) { return s->defer_num; }
int ands_defer_var(ands_t *s) { return s->defer_var; }
char *ands_defer_string(ands_t *s) { return buffer_str(&s->defer); }
char ands_defer_mode(ands_t *s) { return s->defer_mode; }
char *ands_atom_string(ands_t *s) { return atom_string(s->atom_num); }
double *ands_data(ands_t *s) { return s->data; }
void ands_data_len_set(ands_t *s, int n) {
  if (n >= 0 && n <= s->data_cap) s->data_len = n;
}
int ands_data_len(ands_t *s) { return s->data_len; }
void ands_data_resize(ands_t *s, int len) {
  if (!s || len <= 0) return;
  double *data = (double *)calloc((size_t)len, sizeof(double));
  if (!data) return;
  if (s->data) {
    free(s->data);
  }
  s->data = data;
  s->data_len = 0;
  s->data_cap = len;
}
int ands_data_cap(ands_t *s) { return s->data_cap; }

void ands_arg_clear(ands_t *s) {
    if (!s) return;
    s->arg_len = 0;
    for (int i = 0; i < ARG_MAX; i++) s->arg_var[i] = -1;
}

double ands_arg_push(ands_t *s, double n) {
    if (s->arg_len < s->arg_cap) {
        s->arg[s->arg_len] = n;
        s->arg_var[s->arg_len] = -1;
        s->arg_len++;
    }
    return n;
}

void ands_arg_len_set(ands_t *s, int n) {
    if (!s || n < 0 || n > s->arg_cap) return;
    for (int i = s->arg_len; i < n; i++) s->arg_var[i] = -1;
    s->arg_len = n;
}

double ands_arg_drop(ands_t *s) {
    int n = s->arg_len;
    double x = 0;
    if (n > 0) {
        x = s->arg[0];
        for (int i = 1; i < ARG_MAX; i++) {
            s->arg[i-1] = s->arg[i];
            s->arg_var[i-1] = s->arg_var[i];
        }
        s->arg_len--;
    }
    return x;
}

double ands_arg_swap(ands_t *s) {
    if (s->arg_len > 1) {
        double t = s->arg[0];
        int tv = s->arg_var[0];
        s->arg[0] = s->arg[1];
        s->arg_var[0] = s->arg_var[1];
        s->arg[1] = t;
        s->arg_var[1] = tv;
    }
    return 0;
}

double ands_arg_push_many(ands_t *s, double *a, int n) {
    for (int i = 0; i < n && s->arg_len < s->arg_cap; i++) {
        s->arg[s->arg_len] = a[i];
        s->arg_var[s->arg_len] = -1;
        s->arg_len++;
    }
    return 0;
}

double ands_get_local(ands_t *s, int n) {
  if (n >= 0 && n < VAR_MAX) {
    return s->global_var[n];
  }
  return NAN;
}

void ands_set_local(ands_t *s, int n, double x) {
    if (n >= 0 && n < VAR_MAX) {
        s->global_var[n] = x;
    }
}

void ands_set_global(ands_t *s, double *p) { s->global_var = p; s->global_save = p; }
void ands_use_local(ands_t *s) { s->global_var = s->local_var; }
void ands_use_global(ands_t *s) { s->global_var = s->global_save; }

void ands_local_to_global(ands_t *s, int n) {
    if (n >= 0 && n < VAR_MAX) {
        s->global_var[n] = s->local_var[n];
    }
}

void ands_global_to_local(ands_t *s, int n) {
    if (n >= 0 && n < VAR_MAX) {
        s->local_var[n] = s->global_var[n];
    }
}

#define STRING_PTR(s) s->string[s->string_idx % STRING_BUF_MOD].data

char *ands_string_from_external(ands_t *s, char *src, int len) {
  if (!s || !src) return NULL;
  if (len < 0) len = 0;
  if (len >= STRING_BUF_LEN) len = STRING_BUF_LEN - 1;
  memcpy(STRING_PTR(s), src, (size_t)len);
  STRING_PTR(s)[len] = '\0';
  return STRING_PTR(s);
}

char *ands_string_to_external(ands_t *s, char *dst, int len) {
  if (!s || !dst || len <= 0) return dst;
  snprintf(dst, (size_t)len, "%s", STRING_PTR(s));
  return dst;
}
