#ifndef _ANDS_H_
#define _ANDS_H_

#include <stdint.h>

enum {
  START = 0, // 0
  GET_NUMBER,
  GET_VARIABLE,
  GET_RETURN,
  GET_DEFER_NUMBER,
  GET_DEFER_STRING,
  GET_ATOM,
  GET_STRING,
  GET_ARRAY,
  GET_COMMENT,
  CHUNK_END,
  //
  GOT_NUMBER,
  GOT_ATOM,
  //
  FUNCTION,
  DEFER,
  GOT_STRING,
  GOT_ARRAY,
  GOT_RETURN_REF,
  MACRO_DEFINED,
  MACRO_REMOVING,
};

enum {
  ANDS_MACRO_UNCHECKED = 0,
  ANDS_MACRO_REALTIME,
  ANDS_MACRO_IMMEDIATE,
  ANDS_MACRO_INVALID,
  ANDS_MACRO_TOO_LARGE,
};

typedef struct ands_s ands_t;
typedef uint32_t atom_t;

#define ANDS_VAR_MAX (128)
#define ANDS_MACRO_NAME_LEN (5)
#define ANDS_MACRO_BODY_LEN (512)
#define ANDS_RETURN_MAX (10)  /* @0 .. @9 */

ands_t *ands_new(int (*fn)(ands_t *s, int info), void *user);
void ands_free(ands_t *s);
int ands_consume(ands_t *s, char *line);
int ands_macro_count(ands_t *s);
int ands_macro_get(ands_t *s, int index, char *name, int name_len,
                   char *body, int body_len, int *arg_count);
int ands_last_macro_index(ands_t *s);
int ands_macro_status(ands_t *s, int index);
int ands_macro_set_status(ands_t *s, int index, int status);
int ands_macro_remove(ands_t *s, const char *name);
void ands_macro_clear(ands_t *s);
uint32_t ands_atom_num(ands_t *s);
char *ands_atom_string(ands_t *s);
int ands_arg_len(ands_t *s);
double *ands_arg(ands_t *s);
int ands_arg_var(ands_t *s, int n);
void *ands_user(ands_t *s);
char *ands_string(ands_t *s);
int ands_string_len(ands_t *s);
int ands_string_fresh(ands_t *s);
double ands_defer_num(ands_t *s);
int ands_defer_var(ands_t *s);
char *ands_defer_string(ands_t *s);
char ands_defer_mode(ands_t *s);
char *ands_atom_string(ands_t *s);
void ands_trace_set(ands_t *s, int n);
char *atom_string(int i);
double *ands_data(ands_t *s);
int ands_data_len(ands_t *s);
double ands_arg_push(ands_t *s, double n);
double ands_arg_push_many(ands_t *s, double *a, int n);
void ands_arg_clear(ands_t *s);
double ands_arg_drop(ands_t *s);
double ands_arg_swap(ands_t *s);
double ands_arg_dup(ands_t *s);
double ands_arg_over(ands_t *s);
double ands_arg_rot(ands_t *s);
void ands_arg_len_set(ands_t *s, int n);
void ands_set_local(ands_t *s, int n, double x);
void ands_set_global(ands_t *s, double *p);
void ands_use_local(ands_t *s);
void ands_use_global(ands_t *s);
void ands_local_to_global(ands_t *s, int n);
void ands_global_to_local(ands_t *s, int n);
void ands_data_resize(ands_t *s, int len);
void ands_data_len_set(ands_t *s, int n);
int ands_data_cap(ands_t *s);
double ands_get_local(ands_t *s, int n);

/*
 * Return-value registers (@0..@9), read from skode text via the GET_RETURN
 * lexer state (mirrors GET_VARIABLE structurally, but is intentionally
 * NOT the same mechanism -- return values have no meaning at compile time,
 * since nothing in a compiled opcode carries "the previous command's
 * output"; ands_consume() notifies s->fn(s, GOT_RETURN_REF) whenever @N is
 * read so a compile-mode caller can reject it (SKODE_COMPILE_IMMEDIATE_ONLY),
 * the same way GOT_STRING/GOT_ARRAY already do).
 *
 * Deliberately per-parser (ands_t owns ret_value[] directly, no
 * ands_set_global()-style external swap the way variables have) -- return
 * values represent "what did THIS session's last command produce", not
 * shared process-global state the way $N variables are.
 *
 * ands_return_clear() is called automatically before every atom dispatch
 * (see action_finish_atom()/action_chunk_end() in ands.c) -- callers
 * don't need to call it themselves in the ordinary case; it's exposed for
 * anything that wants to explicitly reset return state outside of normal
 * dispatch (e.g. before a batch of internal calls that shouldn't leave
 * return values behind for the next real command to see).
 *
 * ands_return_push() is sequential append (count increments by exactly
 * one per call, silently dropped once ANDS_RETURN_MAX is reached) --
 * intended for a word reporting "here are my N results, in order".
 * ands_return_set() is direct-index write (count becomes
 * max(count, n+1), lower slots are left untouched) -- intended for
 * explicit slot assignment by C callers. These have different
 * shapes on purpose: push is how most words will report results, set is
 * for the one word whose entire job is targeted assignment.
 */
void ands_return_clear(ands_t *s);
void ands_return_push(ands_t *s, double value);
void ands_return_set(ands_t *s, int n, double value);
int ands_return_count(ands_t *s);
double ands_return_get(ands_t *s, int n);
void ands_return_set_error(ands_t *s, int code);
int ands_return_error(ands_t *s);
int ands_return_saved_count(ands_t *s);
double ands_return_saved_get(ands_t *s, int n);
int ands_return_saved_error(ands_t *s);
void ands_return_restore_saved(ands_t *s);

char *ands_string_to_external(ands_t *s, char *dst, int len);
char *ands_string_from_external(ands_t *s, char *src, int len);

#define ATOM4(c) (c)

#endif
