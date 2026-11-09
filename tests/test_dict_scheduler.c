#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dlfcn.h>
#include <assert.h>

#include "api.h"
#include "skode.h"
#include "synth.h"
#include "synth-types.h"
#include "synth-state.h"
#include "seq.h"
#include "ands.h"
#include "skode_dict_gen.h"
#include "skode-internal.h"

static int g_failures = 0;
static int g_tests_run = 0;

#define TEST_ASSERT(cond, msg) do { \
    g_tests_run++; \
    if (!(cond)) { \
        fprintf(stderr, "FAIL [%s:%d]: %s\n", __FILE__, __LINE__, msg); \
        g_failures++; \
    } else { \
        printf("  PASS: %s\n", msg); \
    } \
} while (0)

/* ========================================================================= */
/* Interceptor Harness for Suite E                                           */
/* ========================================================================= */
static int s_rt_zone = 0;
static int s_alloc_violations = 0;
static int s_string_violations = 0;

static void *(*real_malloc)(size_t) = NULL;
static void *(*real_calloc)(size_t, size_t) = NULL;
static void *(*real_realloc)(void *, size_t) = NULL;
static void (*real_free)(void *) = NULL;
static int (*real_strcmp)(const char *, const char *) = NULL;
static size_t (*real_strlen)(const char *) = NULL;

static char s_emergency_buf[8192];
static size_t s_emergency_pos = 0;

void *malloc(size_t size) {
    if (s_rt_zone) s_alloc_violations++;
    if (!real_malloc) real_malloc = (void *(*)(size_t))dlsym(RTLD_NEXT, "malloc");
    return real_malloc ? real_malloc(size) : NULL;
}

void *calloc(size_t nmemb, size_t size) {
    if (s_rt_zone) s_alloc_violations++;
    if (!real_calloc) {
        size_t total = nmemb * size;
        if (s_emergency_pos + total <= sizeof(s_emergency_buf)) {
            void *p = &s_emergency_buf[s_emergency_pos];
            s_emergency_pos += (total + 15) & ~15;
            return p;
        }
        return NULL;
    }
    return real_calloc(nmemb, size);
}

void *realloc(void *ptr, size_t size) {
    if (s_rt_zone) s_alloc_violations++;
    if (!real_realloc) real_realloc = (void *(*)(void *, size_t))dlsym(RTLD_NEXT, "realloc");
    return real_realloc ? real_realloc(ptr, size) : NULL;
}

void free(void *ptr) {
    if (ptr >= (void *)s_emergency_buf && ptr < (void *)(s_emergency_buf + sizeof(s_emergency_buf))) {
        return;
    }
    if (s_rt_zone) s_alloc_violations++;
    if (!real_free) real_free = (void (*)(void *))dlsym(RTLD_NEXT, "free");
    if (real_free) real_free(ptr);
}

int strcmp(const char *s1, const char *s2) {
    if (s_rt_zone) s_string_violations++;
    if (!real_strcmp) real_strcmp = (int (*)(const char *, const char *))dlsym(RTLD_NEXT, "strcmp");
    return real_strcmp ? real_strcmp(s1, s2) : 0;
}

size_t strlen(const char *s) {
    if (s_rt_zone) s_string_violations++;
    if (!real_strlen) real_strlen = (size_t (*)(const char *))dlsym(RTLD_NEXT, "strlen");
    return real_strlen ? real_strlen(s) : 0;
}

static void init_interceptors(void) {
    real_malloc = (void *(*)(size_t))dlsym(RTLD_NEXT, "malloc");
    real_calloc = (void *(*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
    real_realloc = (void *(*)(void *, size_t))dlsym(RTLD_NEXT, "realloc");
    real_free = (void (*)(void *))dlsym(RTLD_NEXT, "free");
    real_strcmp = (int (*)(const char *, const char *))dlsym(RTLD_NEXT, "strcmp");
    real_strlen = (size_t (*)(const char *))dlsym(RTLD_NEXT, "strlen");
}

/* ========================================================================= */
/* Test Callback Context for ANDS Scanner                                    */
/* ========================================================================= */
typedef struct {
    char captured_atom[64];
    int captured_argc;
    double captured_args[8];
    int call_count;
} parse_capture_t;

static int capture_callback(ands_t *s, int info) {
    parse_capture_t *cap = (parse_capture_t *)ands_user(s);
    if (info == FUNCTION) {
        cap->call_count++;
        strncpy(cap->captured_atom, ands_atom_name(s), sizeof(cap->captured_atom) - 1);
        cap->captured_atom[sizeof(cap->captured_atom) - 1] = '\0';
        cap->captured_argc = ands_arg_len(s);
        for (int i = 0; i < cap->captured_argc && i < 8; i++) {
            cap->captured_args[i] = ands_arg(s)[i];
        }
    }
    return 0;
}

/* ========================================================================= */
/* SUITE A: Lexical, Grouping & 31-Char Aliasing                             */
/* ========================================================================= */
static void test_suite_a(void) {
    printf("\n=== SUITE A: Lexical, Grouping & 31-Char Aliasing ===\n");

    /* 1. Verify 31-character token parsing in ANDS */
    parse_capture_t cap = {0};
    ands_t *p = ands_new(capture_callback, &cap);
    TEST_ASSERT(p != NULL, "Created ANDS parser");

    /* Exactly 31 characters (digits terminate atoms in ANDS by design,
       so the identifier uses letters/underscores only). */
    char input_31[] = "abcdefghijklmnop_qrstuvwxyz_abc 42;";
    ands_consume(p, input_31);
    TEST_ASSERT(cap.call_count == 1, "Dispatched 31-character atom");
    TEST_ASSERT(strcmp(cap.captured_atom, "abcdefghijklmnop_qrstuvwxyz_abc") == 0,
                "ANDS accurately preserved 31-character token string");
    TEST_ASSERT(strlen(cap.captured_atom) == 31, "Token length is exactly 31 characters");
    TEST_ASSERT(cap.captured_argc == 1 && cap.captured_args[0] == 42.0, "Arg 42 parsed accurately");

    /* Over-long identifiers are truncated to 31 chars, never overflow */
    memset(&cap, 0, sizeof(cap));
    char input_40[] = "abcdefghijklmnopqrstuvwxyzabcdefghijklmn 7;";
    ands_consume(p, input_40);
    TEST_ASSERT(cap.call_count == 1 && strlen(cap.captured_atom) == 31,
                "40-char identifier safely truncated to 31 chars");

    /* Legacy 4-char packed atom still produced for short names */
    memset(&cap, 0, sizeof(cap));
    char input_short[] = "v0 w1 f440;";
    ands_consume(p, input_short);
    TEST_ASSERT(cap.call_count == 3 && strcmp(cap.captured_atom, "f") == 0 &&
                cap.captured_args[0] == 440.0,
                "Streaming tokenizer unchanged: 'v0 w1 f440' -> 3 atoms, last f 440");
    ands_free(p);

    /* 2. Canonical long name vs short alias resolution */
    const skode_word_t *w_wf = skode_dict_find("wf");
    const skode_word_t *w_wavefold = skode_dict_find("wavefold");
    TEST_ASSERT(w_wf != NULL && w_wavefold != NULL, "Found both 'wf' and 'wavefold'");
    TEST_ASSERT(w_wf == w_wavefold, "Short alias 'wf' and canonical 'wavefold' resolve to identical word pointer");
    TEST_ASSERT(strcmp(w_wf->name, "wavefold") == 0, "Canonical word name is 'wavefold'");
    TEST_ASSERT(strcmp(w_wf->alias, "wf") == 0, "Word alias is 'wf'");
    TEST_ASSERT(w_wf->group == SKODE_GRP_FILTER, "Word 'wavefold' belongs to SKODE_GRP_FILTER");

    const skode_word_t *w_w = skode_dict_find("w");
    const skode_word_t *w_wave = skode_dict_find("wave");
    TEST_ASSERT(w_w == w_wave, "Alias 'w' and canonical 'wave' resolve to identical word pointer");
    TEST_ASSERT(w_w->group == SKODE_GRP_OSC, "Word 'wave' belongs to SKODE_GRP_OSC");

    const skode_word_t *w_k = skode_dict_find("k");
    const skode_word_t *w_ff = skode_dict_find("filter_freq");
    TEST_ASSERT(w_k == w_ff, "Alias 'k' and canonical 'filter_freq' resolve to identical word pointer");
    TEST_ASSERT(w_k->group == SKODE_GRP_FILTER, "Word 'filter_freq' belongs to SKODE_GRP_FILTER");

    /* 3. Nonexistent words return NULL */
    TEST_ASSERT(skode_dict_find("nonexistent_word_xyz") == NULL, "Nonexistent word resolves to NULL");

    /* 4. Introspection facilities */
    char dump_buf[1024];
    FILE *mem = fmemopen(dump_buf, sizeof(dump_buf), "w");
    if (mem) {
        skode_dict_help(mem, "filter");
        skode_dict_help(mem, "wavefold");
        skode_engine_dump(&skred_global_engine, mem, "filter", 1);
        skode_engine_dump(&skred_global_engine, mem, "osc", 0);
        fclose(mem);
        TEST_ASSERT(strlen(dump_buf) > 0, "skode_dict_help and skode_engine_dump emitted introspection text");
    }
}

/* ========================================================================= */
/* SUITE B: Multi-Clause Pattern Matching & Sentinels                        */
/* ========================================================================= */
static void test_suite_b(void) {
    printf("\n=== SUITE B: Multi-Clause Pattern Matching & Sentinels ===\n");

    const skode_word_t *w_wf = skode_dict_find("wavefold");
    TEST_ASSERT(w_wf != NULL, "Found 'wavefold' for pattern matching tests");
    TEST_ASSERT(w_wf->clause_count == 4, "'wavefold' has 4 distinct clauses");

    /* Clause 1: Arity 1, finite float -> skode_clause_wavefold_drive */
    double arg_drive[] = {0.75};
    const skode_clause_t *c1 = skode_clause_match(w_wf, 1, arg_drive);
    TEST_ASSERT(c1 != NULL, "Matched clause 1 (finite drive)");
    TEST_ASSERT(c1->opcode_id == SKODE_OP_DISCRETE_skode_clause_wavefold_drive,
                "Clause 1 resolved to SKODE_OP_DISCRETE_skode_clause_wavefold_drive (131)");
    TEST_ASSERT(strcmp(c1->c_name, "skode_clause_wavefold_drive") == 0, "c_name matches clause 1");

    /* Clause 2: Arity 2, finite floats -> skode_clause_wavefold_drive_bias */
    double arg_bias[] = {0.75, 0.2};
    const skode_clause_t *c2 = skode_clause_match(w_wf, 2, arg_bias);
    TEST_ASSERT(c2 != NULL, "Matched clause 2 (drive + bias)");
    TEST_ASSERT(c2->opcode_id == SKODE_OP_DISCRETE_skode_clause_wavefold_drive_bias,
                "Clause 2 resolved to SKODE_OP_DISCRETE_skode_clause_wavefold_drive_bias (132)");

    /* Clause 3: Arity 1, NaN sentinel -> skode_clause_wavefold_reset_nan */
    double arg_nan[] = {NAN};
    const skode_clause_t *c3 = skode_clause_match(w_wf, 1, arg_nan);
    TEST_ASSERT(c3 != NULL, "Matched clause 3 (NaN sentinel reset)");
    TEST_ASSERT(c3->opcode_id == SKODE_OP_DISCRETE_skode_clause_wavefold_reset_nan,
                "Clause 3 resolved to SKODE_OP_DISCRETE_skode_clause_wavefold_reset_nan (133)");

    /* Clause 4: Arity 1, +Inf sentinel -> skode_clause_wavefold_max_inf */
    double arg_pinf[] = {INFINITY};
    const skode_clause_t *c4 = skode_clause_match(w_wf, 1, arg_pinf);
    TEST_ASSERT(c4 != NULL, "Matched clause 4 (+Inf limit)");
    TEST_ASSERT(c4->opcode_id == SKODE_OP_DISCRETE_skode_clause_wavefold_max_inf,
                "Clause 4 resolved to SKODE_OP_DISCRETE_skode_clause_wavefold_max_inf (134)");

    /* Invalid signature: -Inf sentinel should reject (no -inf clause) */
    double arg_ninf[] = {-INFINITY};
    const skode_clause_t *c_bad1 = skode_clause_match(w_wf, 1, arg_ninf);
    TEST_ASSERT(c_bad1 == NULL, "-Inf rejected when no matching clause exists");

    /* Invalid arity: 3 arguments should reject */
    double arg_3[] = {1.0, 2.0, 3.0};
    const skode_clause_t *c_bad2 = skode_clause_match(w_wf, 3, arg_3);
    TEST_ASSERT(c_bad2 == NULL, "Arity 3 rejected (exceeds max arity 2)");

    /* Invalid arity: 0 arguments should reject */
    const skode_clause_t *c_bad3 = skode_clause_match(w_wf, 0, NULL);
    TEST_ASSERT(c_bad3 == NULL, "Arity 0 rejected");

    /* Filter Freq word (k / filter_freq) */
    const skode_word_t *w_k = skode_dict_find("k");
    TEST_ASSERT(w_k != NULL && w_k->clause_count == 2, "'k' has 2 clauses");

    double arg_k_finite[] = {800.0};
    const skode_clause_t *ck1 = skode_clause_match(w_k, 1, arg_k_finite);
    TEST_ASSERT(ck1 != NULL && ck1->opcode_id == SKODE_OP_DISCRETE_skode_clause_filter_freq_set,
                "'k 800' matched filter_freq_set clause");

    double arg_k_nan[] = {NAN};
    const skode_clause_t *ck2 = skode_clause_match(w_k, 1, arg_k_nan);
    TEST_ASSERT(ck2 != NULL && ck2->opcode_id == SKODE_OP_DISCRETE_skode_clause_filter_freq_reset,
                "'k -' (NaN) matched filter_freq_reset clause");
}

static skode_t new_ctx(void) {
    skode_t ctx = SKODE_EMPTY();
    skode_init(&ctx);
    return ctx;
}

/* ========================================================================= */
/* SUITE C: Macro Compilation & RT Safety Analysis                          */
/* ========================================================================= */
static void test_suite_c(void) {
    printf("\n=== SUITE C: Macro Compilation & RT Safety Analysis ===\n");

    /* Initialize skode context */
    skode_t ctx = new_ctx();

    /* 1. Define an RT-safe user macro containing discrete opcodes: wf 0.5 */
    synth_init(8);
    wave_table_init(0);
    voice_init();
    synth_sample_rate_set(48000);
    seq_init();
    char macro_rt[] = "[mywf]: wf 0.5 ;";
    int rc1 = skode_consume(macro_rt, &ctx);
    TEST_ASSERT(rc1 == 0, "Consumed RT-safe macro definition '[mywf]: wf 0.5 ;'");

    int idx_rt = ands_last_macro_index(ctx.parse);
    TEST_ASSERT(idx_rt >= 0, "Macro 'mywf' found in macro registry");
    int status_rt = ands_macro_status(ctx.parse, idx_rt);
    TEST_ASSERT(status_rt == ANDS_MACRO_REALTIME,
                "Macro 'mywf' tagged as ANDS_MACRO_REALTIME (all clauses carry RT_SAFE)");

    /* Verify compilation into flat opcode array */
    event_program_t prog_rt = {0};
    int sched_ok1 = skode_compile_scheduled(&ctx, "mywf", &prog_rt);
    TEST_ASSERT(sched_ok1 == 1, "Macro 'mywf' is schedulable in RT scheduler");
    TEST_ASSERT(prog_rt.count == 1, "Pre-compiled program contains exactly 1 opcode event");
    TEST_ASSERT(prog_rt.op[0].opcode.code == SKODE_OP_DISCRETE_skode_clause_wavefold_drive,
                "Compiled opcode ID is discrete ID SKODE_OP_DISCRETE_skode_clause_wavefold_drive");
    TEST_ASSERT(prog_rt.op[0].opcode.arg[0] == 0.5f, "Compiled argument is 0.5f (flat float, no strings)");

    /* 2. Define a non-RT-safe user macro containing immediate-only command (/q = stop) */
    char macro_imm[] = "[mstp]: /q ;";
    int rc2 = skode_consume(macro_imm, &ctx);
    TEST_ASSERT(rc2 == 0, "Consumed non-RT-safe macro definition '[mstp]: /q ;'");

    int idx_imm = ands_last_macro_index(ctx.parse);
    TEST_ASSERT(idx_imm >= 0, "Macro 'mstp' found in macro registry");
    int status_imm = ands_macro_status(ctx.parse, idx_imm);
    TEST_ASSERT(status_imm == ANDS_MACRO_IMMEDIATE,
                "Macro 'mstp' tagged as ANDS_MACRO_IMMEDIATE (contains non-RT-safe '/q')");

    /* Attempt to schedule the non-RT-safe macro */
    event_program_t prog_imm = {0};
    int sched_ok2 = skode_compile_scheduled(&ctx, "mstp", &prog_imm);
    TEST_ASSERT(sched_ok2 == 0, "Scheduling non-RT-safe macro was strictly rejected (# SKODE_NOT_REALTIME_SAFE)");

    /* Direct attempt to schedule non-RT-safe command '/q' */
    event_program_t prog_q = {0};
    int sched_ok3 = skode_compile_scheduled(&ctx, "/q", &prog_q);
    TEST_ASSERT(sched_ok3 == 0, "Direct scheduling of '/q' was strictly rejected (# SKODE_NOT_REALTIME_SAFE)");

    /* 3. Define a long (25-character) RT-safe macro name */
    char macro_long[] = "[long_macro_filter_sweep]: wf 0.75 ;";
    int rc_long = skode_consume(macro_long, &ctx);
    TEST_ASSERT(rc_long == 0, "Consumed 25-character macro definition '[long_macro_filter_sweep]'");

    int idx_long = ands_macro_find(ctx.parse, "long_macro_filter_sweep");
    TEST_ASSERT(idx_long >= 0, "Macro 'long_macro_filter_sweep' found with full 25-char name");
    int status_long = ands_macro_status(ctx.parse, idx_long);
    TEST_ASSERT(status_long == ANDS_MACRO_REALTIME,
                "Macro 'long_macro_filter_sweep' tagged as ANDS_MACRO_REALTIME");

    event_program_t prog_long = {0};
    int sched_ok_long = skode_compile_scheduled(&ctx, "long_macro_filter_sweep", &prog_long);
    TEST_ASSERT(sched_ok_long == 1, "Macro 'long_macro_filter_sweep' is schedulable in RT scheduler");
    TEST_ASSERT(prog_long.count == 1, "Pre-compiled long macro program contains 1 opcode event");
    TEST_ASSERT(prog_long.op[0].opcode.arg[0] == 0.75f, "Compiled argument is 0.75f");

    wave_free();
    synth_free();
}

/* ========================================================================= */
/* SUITE D: Scheduler Queue & Timing                                         */
/* ========================================================================= */
static void test_event_cb(const event_t *event) {
    skode_execute_event(event, NULL);
}

static void test_suite_d(void) {
    printf("\n=== SUITE D: Scheduler Queue & Timing ===\n");

    synth_init(8);
    wave_table_init(0);
    voice_init();
    synth_sample_rate_set(48000);
    seq_init();

    skode_t ctx = new_ctx();

    /* 1. Schedule a deferred discrete opcode event: +0.01 k 1200 */
    sv.filter_freq[0] = 500.0f;
    SAMPLE_COUNT_PUT(1000);
    uint64_t t_start = SAMPLE_COUNT_GET();

    event_program_t prog = {0};
    int ok = skode_compile_scheduled(&ctx, "k 1200", &prog);
    TEST_ASSERT(ok == 1, "Compiled 'k 1200' for queueing");
    TEST_ASSERT(prog.op[0].opcode.code == SKODE_OP_DISCRETE_skode_clause_filter_freq_set,
                "Opcode is discrete filter_freq_set");

    /* Queue program to fire at t_start + 480 samples */
    uint64_t target_tick = t_start + 480;
    int q_rc = skode_queue_program(&prog, 0, target_tick, 10);
    TEST_ASSERT(q_rc == 0, "Queued program into sequencer event queue");

    /* Run sequence before boundary: tick = target_tick - 100 */
    SAMPLE_COUNT_PUT(target_tick - 100);
    seq(target_tick - 100, test_event_cb, NULL);
    TEST_ASSERT(sv.filter_freq[0] == 500.0f, "Parameter unchanged before scheduled timestamp");

    /* Run sequence past boundary: tick = target_tick + 10 */
    SAMPLE_COUNT_PUT(target_tick + 10);
    seq(target_tick + 10, test_event_cb, NULL);
    TEST_ASSERT(sv.filter_freq[0] == 1200.0f, "Parameter updated to 1200.0f at exact scheduled timestamp");

    /* 2. Schedule repeated macro */
    event_program_t repeat_prog = {0};
    skode_compile_scheduled(&ctx, "k 750", &repeat_prog);
    skode_queue_repeated(&repeat_prog, 0, 3, 0.005, 20);
    SAMPLE_COUNT_PUT(target_tick + 2000);
    seq(target_tick + 2000, test_event_cb, NULL);
    TEST_ASSERT(sv.filter_freq[0] == 750.0f, "Repeated macro executed cleanly in queue");

    /* 3. NaN-sentinel clause survives compile + queue + audio-path dispatch */
    event_program_t nan_prog = {0};
    int nan_ok = skode_compile_scheduled(&ctx, "k -", &nan_prog);
    TEST_ASSERT(nan_ok == 1 && nan_prog.op[0].opcode.code ==
                SKODE_OP_DISCRETE_skode_clause_filter_freq_reset,
                "'k -' compiled to discrete filter_freq_reset (NaN matched at parse time)");
    uint64_t nan_when = target_tick + 3000;
    skode_queue_program(&nan_prog, 0, nan_when, 30);
    SAMPLE_COUNT_PUT(nan_when + 1);
    seq(nan_when + 1, test_event_cb, NULL);
    TEST_ASSERT(sv.filter_freq[0] == 20000.0f, "Scheduled NaN-sentinel clause executed (cutoff reset to 20 kHz)");

    wave_free();
    synth_free();
}

/* ========================================================================= */
/* SUITE E: Real-time Audio Thread Zero-Allocation Interceptor               */
/* ========================================================================= */
static void test_suite_e(void) {
    printf("\n=== SUITE E: Real-Time Audio Thread Zero-Allocation Interceptor ===\n");

    synth_init(8);
    wave_table_init(0);
    voice_init();
    synth_sample_rate_set(48000);

    float buffer[128 * 2];
    memset(buffer, 0, sizeof(buffer));

    /* Pre-warm and clear violation counters */
    s_alloc_violations = 0;
    s_string_violations = 0;

    /* Enter real-time audio thread simulation zone */
    s_rt_zone = 1;

    /* 1. Direct real-time jump table dispatch for discrete opcodes */
    float wavefold_args[1] = {0.8f};
    int rc1 = g_skode_realtime_ops[SKODE_OP_DISCRETE_skode_clause_wavefold_drive](0, wavefold_args);
    (void)rc1;

    float filter_args[1] = {1500.0f};
    int rc2 = g_skode_realtime_ops[SKODE_OP_DISCRETE_skode_clause_filter_freq_set](0, filter_args);
    (void)rc2;

    /* 2. Render 128-sample audio tick */
    synth(&skred_global_engine, buffer, NULL, 128, 2, NULL);

    /* Exit real-time zone */
    s_rt_zone = 0;

    TEST_ASSERT(s_alloc_violations == 0,
                "Zero dynamic allocations (0 malloc/calloc/realloc/free) during audio thread execution");
    TEST_ASSERT(s_string_violations == 0,
                "Zero string functions (0 strcmp/strlen) during audio thread execution");

    wave_free();
    synth_free();
}

/* ========================================================================= */
/* SUITE F: DSP Output Bit-Exact Verification                                */
/* ========================================================================= */
static void test_suite_f(void) {
    printf("\n=== SUITE F: DSP Output Bit-Exact Verification ===\n");

    synth_init(8);
    wave_table_init(0);
    voice_init();
    synth_sample_rate_set(48000);

    float buf_baseline[128 * 2];
    float buf_refactored[128 * 2];
    memset(buf_baseline, 0, sizeof(buf_baseline));
    memset(buf_refactored, 0, sizeof(buf_refactored));

    /* --- Run 1: Direct unrefactored DSP baseline --- */
    sv.freq[0] = 440.0f;
    wave_set(0, 1);
    sv.interpolate[0] = 0;
    sv.amp[0] = 1.0f;
    mmf_set_freq(&skred_global_engine, 0, 1000.0f);
    synth(&skred_global_engine, buf_baseline, NULL, 128, 2, NULL);

    /* --- Run 2: Discrete opcode dispatch path --- */
    voice_init();
    sv.freq[0] = 440.0f;
    sv.amp[0] = 1.0f;

    /* Dispatch wave clause via jump table */
    float w_args[2] = {1.0f, 0.0f};
    g_skode_realtime_ops[SKODE_OP_DISCRETE_skode_clause_wave_2](0, w_args);

    /* Dispatch filter clause via jump table */
    float k_args[1] = {1000.0f};
    g_skode_realtime_ops[SKODE_OP_DISCRETE_skode_clause_filter_freq_set](0, k_args);

    synth(&skred_global_engine, buf_refactored, NULL, 128, 2, NULL);

    /* Bit-exact comparison */
    int diff = memcmp(buf_baseline, buf_refactored, sizeof(buf_baseline));
    TEST_ASSERT(diff == 0, "DSP audio output is 100% bit-exact between direct DSP and discrete opcode dispatch");

    wave_free();
    synth_free();
}

/* ========================================================================= */
/* Main Test Runner                                                          */
/* ========================================================================= */
int main(void) {
    init_interceptors();

    printf("================================================================\n");
    printf("pulp/skred v0.82.0 - Opcode Dictionary & Scheduler Test Suite\n");
    printf("================================================================\n");

    test_suite_a();
    test_suite_b();
    test_suite_c();
    test_suite_d();
    test_suite_e();
    test_suite_f();

    printf("\n================================================================\n");
    printf("Test Results: %d tests executed, %d failure(s)\n", g_tests_run, g_failures);
    printf("================================================================\n");

    return g_failures == 0 ? 0 : 1;
}
