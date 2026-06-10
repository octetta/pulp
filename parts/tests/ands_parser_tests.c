#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ands.h"

#define MAX_EVENTS 64
#define MAX_ARGS 8

typedef struct {
  int info;
  char atom[5];
  int argc;
  double args[MAX_ARGS];
  int arg_var[MAX_ARGS];
  char text[256];
  char defer_mode;
  double defer_num;
  int defer_var;
} event_t;

typedef struct {
  event_t events[MAX_EVENTS];
  int len;
} recorder_t;

static int failures = 0;

static const char *event_name(int info) {
  switch (info) {
    case FUNCTION: return "FUNCTION";
    case DEFER: return "DEFER";
    case GOT_STRING: return "GOT_STRING";
    case GOT_ARRAY: return "GOT_ARRAY";
    case CHUNK_END: return "CHUNK_END";
    default: return "OTHER";
  }
}

static void fail(const char *test, const char *msg) {
  fprintf(stderr, "FAIL %s: %s\n", test, msg);
  failures++;
}

static int near_double(double a, double b) {
  if (isnan(a) && isnan(b)) return 1;
  return fabs(a - b) < 1e-9;
}

static int record_cb(ands_t *s, int info) {
  recorder_t *r = (recorder_t *)ands_user(s);
  if (r->len >= MAX_EVENTS) return 0;

  event_t *e = &r->events[r->len++];
  memset(e, 0, sizeof(*e));
  e->info = info;
  strncpy(e->atom, ands_atom_string(s), sizeof(e->atom) - 1);
  e->argc = ands_arg_len(s);
  if (e->argc > MAX_ARGS) e->argc = MAX_ARGS;
  for (int i = 0; i < e->argc; i++) {
    e->args[i] = ands_arg(s)[i];
    e->arg_var[i] = ands_arg_var(s, i);
  }

  if (info == GOT_STRING) {
    strncpy(e->text, ands_string(s), sizeof(e->text) - 1);
  } else if (info == GOT_ARRAY) {
    e->argc = ands_data_len(s);
    if (e->argc > MAX_ARGS) e->argc = MAX_ARGS;
    for (int i = 0; i < e->argc; i++) e->args[i] = ands_data(s)[i];
  } else if (info == DEFER) {
    e->defer_mode = ands_defer_mode(s);
    e->defer_num = ands_defer_num(s);
    e->defer_var = ands_defer_var(s);
    strncpy(e->text, ands_defer_string(s), sizeof(e->text) - 1);
  }

  return 0;
}

static void expect_len(const char *test, recorder_t *r, int n) {
  if (r->len != n) {
    char msg[128];
    snprintf(msg, sizeof(msg), "expected %d events, got %d", n, r->len);
    fail(test, msg);
    for (int i = 0; i < r->len; i++) {
      fprintf(stderr, "  %d: %s %s argc=%d text=[%s]\n",
              i, event_name(r->events[i].info), r->events[i].atom,
              r->events[i].argc, r->events[i].text);
    }
  }
}

static void expect_event(const char *test, recorder_t *r, int idx, int info, const char *atom) {
  if (idx >= r->len) {
    fail(test, "missing event");
    return;
  }
  event_t *e = &r->events[idx];
  if (e->info != info || strcmp(e->atom, atom) != 0) {
    char msg[160];
    snprintf(msg, sizeof(msg), "event %d expected %s %s, got %s %s",
             idx, event_name(info), atom, event_name(e->info), e->atom);
    fail(test, msg);
  }
}

static void expect_arg(const char *test, recorder_t *r, int idx, int argi, double value) {
  if (idx >= r->len || argi >= r->events[idx].argc) {
    fail(test, "missing arg");
    return;
  }
  if (!near_double(r->events[idx].args[argi], value)) {
    char msg[160];
    snprintf(msg, sizeof(msg), "event %d arg %d expected %.12g, got %.12g",
             idx, argi, value, r->events[idx].args[argi]);
    fail(test, msg);
  }
}

static void run_parse(const char *test, char *input, recorder_t *r) {
  memset(r, 0, sizeof(*r));
  ands_t *s = ands_new(record_cb, r);
  if (!s) {
    fail(test, "ands_new returned NULL");
    return;
  }
  ands_consume(s, input);
  ands_free(s);
}

static void test_compact_commands(void) {
  const char *test = "compact commands";
  recorder_t r;
  char input[] = "v0w1f440a-12";
  run_parse(test, input, &r);

  expect_len(test, &r, 5);
  expect_event(test, &r, 0, FUNCTION, "v---");
  expect_arg(test, &r, 0, 0, 0);
  expect_event(test, &r, 1, FUNCTION, "w---");
  expect_arg(test, &r, 1, 0, 1);
  expect_event(test, &r, 2, FUNCTION, "f---");
  expect_arg(test, &r, 2, 0, 440);
  expect_event(test, &r, 3, FUNCTION, "a---");
  expect_arg(test, &r, 3, 0, -12);
  expect_event(test, &r, 4, CHUNK_END, "----");
}

static void test_strings_and_arrays(void) {
  const char *test = "strings and arrays";
  recorder_t r;
  char input[] = "[kick one] vt (1, 2 -3.5 4e2) d@";
  run_parse(test, input, &r);

  expect_len(test, &r, 5);
  expect_event(test, &r, 0, GOT_STRING, "----");
  if (strcmp(r.events[0].text, "kick one") != 0) fail(test, "string text mismatch");
  expect_event(test, &r, 1, GOT_ARRAY, "vt--");
  expect_arg(test, &r, 1, 0, 1);
  expect_arg(test, &r, 1, 1, 2);
  expect_arg(test, &r, 1, 2, -3.5);
  expect_arg(test, &r, 1, 3, 400);
  expect_event(test, &r, 2, FUNCTION, "vt--");
  expect_event(test, &r, 3, FUNCTION, "d@--");
  expect_event(test, &r, 4, CHUNK_END, "----");
}

static void test_variables_and_nan_defaults(void) {
  const char *test = "variables and nan defaults";
  recorder_t r;
  memset(&r, 0, sizeof(r));
  ands_t *s = ands_new(record_cb, &r);
  ands_set_local(s, 0, 42);
  ands_set_local(s, 127, -7);
  char input[] = "$0 $127 . - f";
  ands_consume(s, input);
  ands_free(s);

  expect_len(test, &r, 2);
  expect_event(test, &r, 0, FUNCTION, "f---");
  expect_arg(test, &r, 0, 0, 42);
  expect_arg(test, &r, 0, 1, -7);
  expect_arg(test, &r, 0, 2, NAN);
  expect_arg(test, &r, 0, 3, NAN);
  expect_event(test, &r, 1, CHUNK_END, "----");
}

static void test_comments_and_chunks(void) {
  const char *test = "comments and chunks";
  recorder_t r;
  char input[] = "v0 # ignore this; f220";
  run_parse(test, input, &r);

  expect_len(test, &r, 4);
  expect_event(test, &r, 0, FUNCTION, "v---");
  expect_event(test, &r, 1, CHUNK_END, "----");
  expect_event(test, &r, 2, FUNCTION, "f---");
  expect_arg(test, &r, 2, 0, 220);
  expect_event(test, &r, 3, CHUNK_END, "----");
}

static void test_defer(void) {
  const char *test = "defer";
  recorder_t r;
  char input[] = "+100 v0 a10~50 T;";
  run_parse(test, input, &r);

  expect_len(test, &r, 4);
  expect_event(test, &r, 0, CHUNK_END, "----");
  expect_event(test, &r, 1, DEFER, "----");
  if (r.events[1].defer_mode != '~' || !near_double(r.events[1].defer_num, 100)) {
    fail(test, "first defer timing mismatch");
  }
  if (strcmp(r.events[1].text, " v0 a10") != 0) fail(test, "first defer text mismatch");
  expect_event(test, &r, 2, DEFER, "----");
  if (r.events[2].defer_mode != '~' || !near_double(r.events[2].defer_num, 50)) {
    fail(test, "second defer timing mismatch");
  }
  if (strcmp(r.events[2].text, " T") != 0) fail(test, "second defer text mismatch");
  expect_event(test, &r, 3, CHUNK_END, "----");
}

static void test_variable_provenance(void) {
  const char *test = "variable provenance";
  recorder_t r;
  char input[] = "v$3 n60,$4 +$5 l$6";
  run_parse(test, input, &r);

  expect_event(test, &r, 0, FUNCTION, "v---");
  if (r.events[0].arg_var[0] != 3)
    fail(test, "voice argument lost variable provenance");
  expect_event(test, &r, 1, FUNCTION, "n---");
  if (r.events[1].arg_var[0] != -1 || r.events[1].arg_var[1] != 4)
    fail(test, "mixed note arguments lost provenance");
  expect_event(test, &r, 3, DEFER, "----");
  if (r.events[3].defer_var != 5)
    fail(test, "defer argument lost variable provenance");
}

int main(void) {
  test_compact_commands();
  test_strings_and_arrays();
  test_variables_and_nan_defaults();
  test_comments_and_chunks();
  test_defer();
  test_variable_provenance();

  if (failures) {
    fprintf(stderr, "%d ANDS parser test failure(s)\n", failures);
    return 1;
  }
  printf("ANDS parser tests passed\n");
  return 0;
}
