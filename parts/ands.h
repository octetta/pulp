#ifndef _ANDS_H_
#define _ANDS_H_

#include <stdint.h>

enum {
  START = 0, // 0
  GET_NUMBER,
  GET_VARIABLE,
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
};

typedef struct ands_s ands_t;
typedef uint32_t atom_t;

#define ANDS_VAR_MAX (128)
#define ANDS_MACRO_NAME_LEN (5)
#define ANDS_MACRO_BODY_LEN (512)

ands_t *ands_new(int (*fn)(ands_t *s, int info), void *user);
void ands_free(ands_t *s);
int ands_consume(ands_t *s, char *line);
int ands_macro_count(ands_t *s);
int ands_macro_get(ands_t *s, int index, char *name, int name_len,
                   char *body, int body_len, int *arg_count);
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

char *ands_string_to_external(ands_t *s, char *dst, int len);
char *ands_string_from_external(ands_t *s, char *src, int len);

#define ATOM4(c) (c)

#endif
