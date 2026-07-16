#include "skode-dict.h"
#include "synth.h"

#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * ---------------------------------------------------------------------
 * REQUIRED PRE-REQUISITE EXPORTS
 * ---------------------------------------------------------------------
 * The words below call a few helpers that are currently `static` inside
 * skode.c.kit, or not declared in any shared header. See
 * SKODE_DICT_INTEGRATION.md for the exact one-line diffs. Summary:
 *
 *   skode.c.kit   : skode_double_to_int  -> drop `static`, declare in skode.h
 *   skode.c.kit   : sk_sleep             -> already non-static, just declare
 *                                            in skode.h
 *   skode-event.c : program_push         -> rename to skode_program_push,
 *                                            drop `static`, declare here
 *
 * Everything else called below (voice_set, amp_set, freq_set, pan_set,
 * wave_mute, skode_midi_note, skode_envelope_velocity, skode_compile_program)
 * is already externally linked today.
 * ---------------------------------------------------------------------
 */
int skode_double_to_int(double value, int *out);
void sk_sleep(int milliseconds);
int skode_program_push(event_program_t *program, skode_opcode_t code,
  ands_t *parser, const double *arg, int argc, char mode,
  uint8_t default_mask);

/*
 * NEW prerequisite, beyond the three above: skode_compile_program() itself
 * needs a vocab-aware sibling for compute_macro_safety()/
 * skode_dict_promote_macro() to check a macro body against a private
 * vocab's words, not just the global one. See SKODE_DICT_INTEGRATION.md
 * for the skode-event.c diff (skode_compile_t gains a private_vocab field,
 * threaded through compile_program_inner() and skode_compile_callback()).
 * skode_compile_program(text, program) keeps its existing behavior
 * unchanged -- it's a thin wrapper calling this with vocab=NULL, so every
 * existing call site is unaffected. */
skode_compile_result_t skode_compile_program_ex(const char *text,
  event_program_t *program, skode_vocab_t *vocab);

/* ===================================================================
 * Dictionary storage
 * =================================================================== */

static skode_vocab_t global_vocab;
static int dict_initialized = 0;

skode_vocab_t *skode_dict_global_vocab(void) {
  return &global_vocab;
}

/* Forward declarations -- defined in the macro-promotion section below;
   skode_dict_vocab_destroy() needs them to free promoted-macro entries
   when tearing down a vocab. */
int skode_dict_word_is_promoted_macro(const skode_word_t *word);
skode_word_t *skode_dict_free_promoted_macro(skode_word_t *word);

skode_vocab_t *skode_dict_vocab_create(void) {
  skode_vocab_t *vocab = malloc(sizeof(*vocab));
  if (!vocab) return NULL;
  memset(vocab, 0, sizeof(*vocab));
  return vocab;
}

static unsigned dict_hash(uint32_t atom) {
  uint32_t h = atom;
  h ^= h >> 16;
  h *= 0x7feb352dU;
  h ^= h >> 15;
  return h % SKODE_DICT_BUCKETS;
}

int skode_dict_register(skode_vocab_t *vocab, skode_word_t *word) {
  if (!vocab || !word) return -1;
  if (skode_dict_find(vocab, word->atom)) return -1;
  unsigned b = dict_hash(word->atom);
  word->next = vocab->buckets[b];
  word->shadow = NULL;
  vocab->buckets[b] = word;
  return 0;
}

/* Shared by override/revert/remove: walks vocab's bucket b looking for
   `atom`, returning the matching node and (via *out_prev) its predecessor
   in the bucket chain (NULL if it's the bucket head). Returns NULL if not
   found, in which case *out_prev is left unset. */
static skode_word_t *dict_find_in_bucket(skode_vocab_t *vocab, unsigned b,
    uint32_t atom, skode_word_t **out_prev) {
  skode_word_t *prev = NULL;
  for (skode_word_t *w = vocab->buckets[b]; w; w = w->next) {
    if (w->atom == atom) {
      if (out_prev) *out_prev = prev;
      return w;
    }
    prev = w;
  }
  return NULL;
}

skode_word_t *skode_dict_override(skode_vocab_t *vocab, skode_word_t *word) {
  if (!vocab || !word) return NULL;
  unsigned b = dict_hash(word->atom);
  skode_word_t *prev_link;
  skode_word_t *existing = dict_find_in_bucket(vocab, b, word->atom, &prev_link);

  if (!existing) {
    /* No prior registration in this vocab -- ordinary insert, same as
       skode_dict_register but without the duplicate check. */
    word->next = vocab->buckets[b];
    word->shadow = NULL;
    vocab->buckets[b] = word;
    return NULL;
  }

  /* `word` takes `existing`'s exact bucket-chain position; `existing`
     becomes purely a shadow (unlinked from the bucket, reachable only via
     word->shadow). This keeps bucket-chain length equal to the number of
     distinct atoms in the bucket, regardless of override count. */
  word->shadow = existing;
  word->next = existing->next;
  if (prev_link) prev_link->next = word;
  else vocab->buckets[b] = word;
  existing->next = NULL;
  return existing;
}

const skode_word_t *skode_dict_shadow(skode_vocab_t *vocab, uint32_t atom) {
  const skode_word_t *w = skode_dict_find(vocab, atom);
  return w ? w->shadow : NULL;
}

skode_word_t *skode_dict_revert(skode_vocab_t *vocab, uint32_t atom) {
  if (!vocab) return NULL;
  unsigned b = dict_hash(atom);
  skode_word_t *prev_link;
  skode_word_t *current = dict_find_in_bucket(vocab, b, atom, &prev_link);
  if (!current || !current->shadow) return NULL;

  skode_word_t *shadow = current->shadow;
  shadow->next = current->next;
  if (prev_link) prev_link->next = shadow;
  else vocab->buckets[b] = shadow;

  current->next = NULL;
  current->shadow = NULL;
  return current;
}

skode_word_t *skode_dict_remove(skode_vocab_t *vocab, uint32_t atom) {
  if (!vocab) return NULL;
  unsigned b = dict_hash(atom);
  skode_word_t *prev_link;
  skode_word_t *current = dict_find_in_bucket(vocab, b, atom, &prev_link);
  if (!current) return NULL;

  if (prev_link) prev_link->next = current->next;
  else vocab->buckets[b] = current->next;

  current->next = NULL;
  /* current->shadow is left intact -- caller walks it to reach every
     older registration for this atom. */
  return current;
}

const skode_word_t *skode_dict_find(skode_vocab_t *vocab, uint32_t atom) {
  if (!vocab) return NULL;
  unsigned b = dict_hash(atom);
  for (skode_word_t *w = vocab->buckets[b]; w; w = w->next) {
    if (w->atom == atom) return w;
  }
  return NULL;
}

const skode_word_t *skode_dict_lookup(skode_vocab_t *private_vocab,
    uint32_t atom) {
  if (private_vocab) {
    const skode_word_t *w = skode_dict_find(private_vocab, atom);
    if (w) return w;
  }
  return skode_dict_find(&global_vocab, atom);
}

void skode_dict_vocab_destroy(skode_vocab_t *vocab) {
  if (!vocab) return;
  for (int b = 0; b < SKODE_DICT_BUCKETS; b++) {
    for (skode_word_t *w = vocab->buckets[b]; w; ) {
      /* Walk the whole shadow chain for this bucket slot, freeing every
         promoted-macro entry found; hand-written words (statics, arena,
         whatever the caller registered) are left alone -- the dictionary
         never allocated them, so it doesn't free them. */
      skode_word_t *chain = w;
      w = w->next; /* advance the bucket walk before we start touching links */
      while (chain) {
        skode_word_t *next_in_chain = chain->shadow;
        if (skode_dict_word_is_promoted_macro(chain)) {
          skode_dict_free_promoted_macro(chain);
        }
        chain = next_in_chain;
      }
    }
  }
  free(vocab);
}

/* ===================================================================
 * Dispatch
 * =================================================================== */

int skode_execute_word(skode_t *ctx, ands_t *s, uint32_t atom, double *arg,
    int argc, int *out_result) {
  const skode_word_t *w = skode_dict_lookup(ctx->vocab, atom);
  if (!w || !w->execute) return 0;
  if (ctx->trace) {
    ctx->printf(ctx, "# SKODE_DICT %s", w->name);
    for (int i = 0; i < argc; i++) ctx->printf(ctx, " %g", arg[i]);
    ctx->puts(ctx, "");
  }
  int r = w->execute(w, ctx, s, arg, argc);
  if (out_result) *out_result = r;
  return 1;
}

int skode_compile_word(ands_t *parser, uint32_t atom, double *arg, int argc,
    event_program_t *program, skode_vocab_t *private_vocab,
    int *out_compile_result) {
  const skode_word_t *w = skode_dict_lookup(private_vocab, atom);
  if (!w) return 0;

  if (w->safety == WORD_IMMEDIATE_ONLY) {
    if (out_compile_result) *out_compile_result = SKODE_COMPILE_IMMEDIATE_ONLY;
    return 1;
  }

  if (w->compile) {
    int rc = w->compile(w, parser, arg, argc, program);
    if (out_compile_result) *out_compile_result = rc;
    return 1;
  }

  if (w->opcode_id == SKODE_OP_NONE) {
    /* Declared safe but supplies no compile path -- treat as
       immediate-only rather than silently dropping the command. */
    if (out_compile_result) *out_compile_result = SKODE_COMPILE_IMMEDIATE_ONLY;
    return 1;
  }

  if (argc < w->min_args || argc > w->max_args) {
    if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
    return 1;
  }

  int rc = skode_program_push(program, (skode_opcode_t)w->opcode_id, parser,
    arg, argc, 0, w->default_mask);
  if (out_compile_result)
    *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
  return 1;
}

/* ===================================================================
 * Macro safety
 * =================================================================== */

/*
 * Macro bodies as stored are not, in general, independently compilable:
 * parameterized macros contain `$$N` placeholders that are
 * only resolved to real values at invocation time (see ands.c's `$$`/`@`
 * scanning, lines ~308/436/706 — governed by IS_VARIABLE, not IS_ATOM, and
 * entirely separate from the name/body grammar). A body like
 * `t $$0 0 $$1 0` is exactly what test_macros_basic() and friends store,
 * and it is not valid input to skode_compile_program() as-is.
 *
 * To classify such a macro without requiring real invocation arguments,
 * substitute each placeholder with a neutral literal ("0") before
 * compiling. This mirrors what happens at a real invocation closely enough
 * for classification purposes: it answers "does this body's *shape*
 * compile", not "does it compile for these specific arguments" (argument
 * *values* can't affect SKODE_COMPILE_OK vs SKODE_COMPILE_IMMEDIATE_ONLY —
 * that distinction is about which opcodes/constructs appear, not what
 * numbers are plugged into them).
 */
static void resolve_macro_placeholders(const char *body, char *out,
    size_t out_size) {
  size_t oi = 0;
  for (size_t i = 0; body[i] && oi + 1 < out_size; ) {
    if (body[i] == '$' && body[i + 1] == '$' &&
        isdigit((unsigned char)body[i + 2])) {
      out[oi++] = '0';
      i += 2;
      while (isdigit((unsigned char)body[i])) i++;
    } else {
      out[oi++] = body[i++];
    }
  }
  out[oi] = '\0';
}

word_safety_t compute_macro_safety(skode_vocab_t *vocab, const char *body) {
  skode_compile_result_t result =
    skode_dict_macro_compile_status(vocab, body);
  switch (result) {
    case SKODE_COMPILE_OK:
      return WORD_REAL_TIME_SAFE;
    case SKODE_COMPILE_IMMEDIATE_ONLY:
      return WORD_IMMEDIATE_ONLY;
    case SKODE_COMPILE_INVALID:
    case SKODE_COMPILE_TOO_LARGE:
    default:
      return WORD_MAY_QUEUE;
  }
}

skode_compile_result_t skode_dict_macro_compile_status(
    skode_vocab_t *vocab, const char *body) {
  event_program_t program;
  if (!body || !body[0]) return SKODE_COMPILE_IMMEDIATE_ONLY;
  char resolved[ANDS_MACRO_BODY_LEN];
  resolve_macro_placeholders(body, resolved, sizeof(resolved));
  return skode_compile_program_ex(resolved, &program, vocab);
}

void skode_dict_report_macro_safety(skode_t *ctx, const char *name,
    const char *body) {
  if (!ctx) return;
  word_safety_t safety = compute_macro_safety(ctx->vocab, body);
  const char *label =
    safety == WORD_REAL_TIME_SAFE ? "real-time-safe (compiles cleanly)" :
    safety == WORD_MAY_QUEUE ? "may-queue (unverified: compile was ambiguous)" :
    "immediate-only (contains strings/arrays or other non-schedulable content)";
  ctx->printf(ctx, "# macro [%s] classified %s\n", name, label);
}

/* ===================================================================
 * Migrated words
 * ===================================================================
 * This is a deliberately small first slice: v/a/f/n/p/m use the generic
 * opcode-backed compile path, while *R is an immediate-only new word for
 * the return-register feature. All other existing commands remain in the
 * legacy switches until they have parity coverage.
 *
 * See SKODE_DICT_STATUS.md for what's converted and what remains in the
 * legacy switch.
 */

static int word_exec_v(const skode_word_t *self, skode_t *ctx, ands_t *s,
    double *arg, int argc) {
  (void)self; (void)s;
  int x;
  if (argc && skode_double_to_int(arg[0], &x)) voice_set(x, &ctx->voice);
  return 0;
}
static int word_exec_a(const skode_word_t *self, skode_t *ctx, ands_t *s,
    double *arg, int argc) {
  (void)self; (void)s;
  if (argc) amp_set(ctx->voice, arg[0]);
  return 0;
}
static int word_exec_f(const skode_word_t *self, skode_t *ctx, ands_t *s,
    double *arg, int argc) {
  (void)self; (void)s;
  if (argc) freq_set(ctx->voice, arg[0]);
  return 0;
}
static int word_exec_n(const skode_word_t *self, skode_t *ctx, ands_t *s,
    double *arg, int argc) {
  (void)self; (void)s;
  if (argc) {
    float note = (float)arg[0];
    float cents = argc > 1 ? (float)arg[1] : 0.0f;
    skode_midi_note(ctx->voice, note, cents);
  }
  return 0;
}
static int word_exec_p(const skode_word_t *self, skode_t *ctx, ands_t *s,
    double *arg, int argc) {
  (void)self; (void)s;
  if (argc) pan_set(ctx->voice, arg[0]);
  return 0;
}
static int word_exec_m(const skode_word_t *self, skode_t *ctx, ands_t *s,
    double *arg, int argc) {
  (void)self; (void)s;
  int x;
  if (argc && skode_double_to_int(arg[0], &x)) wave_mute(ctx->voice, x);
  return 0;
}

/*
 * *R : push each argument into the return registers in order. Normal atom
 * dispatch clears the previous return set immediately before this function,
 * so one invocation defines the complete result: `*R .1 .2` produces
 * @0=.1 and @1=.2. This is intended primarily as the final command in a
 * macro body. It is deliberately IMMEDIATE_ONLY because return registers
 * are parser-local control-flow state with no scheduled-opcode equivalent.
 */
static int word_exec_R_push(const skode_word_t *self, skode_t *ctx, ands_t *s,
    double *arg, int argc) {
  (void)self; (void)ctx;
  for (int i = 0; i < argc; i++) ands_return_push(s, arg[i]);
  return 0;
}

static int word_exec_R_show(const skode_word_t *self, skode_t *ctx, ands_t *s,
    double *arg, int argc) {
  (void)self; (void)arg; (void)argc;
  int count = ands_return_saved_count(s);
  int error = ands_return_saved_error(s);
  if (count == 0 && error == 0) {
    ctx->printf(ctx, "# returns empty\n");
  } else {
    ctx->printf(ctx, "# returns");
    for (int i = 0; i < count; i++)
      ctx->printf(ctx, " @%d=%g", i, ands_return_saved_get(s, i));
    if (error != 0) ctx->printf(ctx, " error=%d", error);
    ctx->puts(ctx, "");
  }
  ands_return_restore_saved(s);
  return 0;
}

/* Auto-sized: no manually-maintained count to drift out of sync with the
   actual entry count (see WORD_COUNT below, derived from this array's
   real size rather than hand-tracked). Designated initializers throughout
   -- every field is named, not positional, so adding/removing/reordering
   a field in skode_word_t can't silently shift what a bare value in some
   other entry means, and every field not mentioned here (.compile,
   .default_mask, .next, .shadow, .data) zero-initializes automatically. */
static skode_word_t word_table[] = {
  { WID("*R"), .execute = word_exec_R_push,
    .min_args = 0, .max_args = ANDS_RETURN_MAX,
    .safety = WORD_IMMEDIATE_ONLY, .category = "parser",
    .summary = "return arguments as @0 through @9" },
  { WID("?R"), .execute = word_exec_R_show,
    .safety = WORD_IMMEDIATE_ONLY, .category = "parser",
    .summary = "show return registers without consuming them" },

  { WID("v"), .execute = word_exec_v, .opcode_id = SKODE_OP_VOICE,
    .min_args = 1, .max_args = 1,
    .safety = WORD_REAL_TIME_SAFE, .category = "voice",
    .summary = "voice-select voice" },
  { WID("a"), .execute = word_exec_a, .opcode_id = SKODE_OP_AMP,
    .min_args = 1, .max_args = 1,
    .safety = WORD_REAL_TIME_SAFE, .category = "voice",
    .summary = "amp loudness" },
  { WID("f"), .execute = word_exec_f, .opcode_id = SKODE_OP_FREQ,
    .min_args = 1, .max_args = 1,
    .safety = WORD_REAL_TIME_SAFE, .category = "voice",
    .summary = "freq hz" },
  { WID("n"), .execute = word_exec_n, .opcode_id = SKODE_OP_MIDI_NOTE,
    .min_args = 1, .max_args = 2, .default_mask = 1,
    .safety = WORD_REAL_TIME_SAFE, .category = "voice",
    .summary = "midi-freq note-number (cents)" },
  { WID("p"), .execute = word_exec_p, .opcode_id = SKODE_OP_PAN,
    .min_args = 1, .max_args = 1,
    .safety = WORD_REAL_TIME_SAFE, .category = "voice",
    .summary = "pan value" },
  { WID("m"), .execute = word_exec_m, .opcode_id = SKODE_OP_MUTE,
    .min_args = 1, .max_args = 1,
    .safety = WORD_REAL_TIME_SAFE, .category = "voice",
    .summary = "mute-audio bool" },
};
#define WORD_COUNT ((int)(sizeof(word_table) / sizeof(word_table[0])))

/* Forward declaration -- defined in the macro-promotion section below;
   skode_dict_init()'s startup self-check needs it. */
static uint32_t pack_atom_runtime(const char *name, int len);

void skode_dict_init(void) {
  if (dict_initialized) return;
  memset(&global_vocab, 0, sizeof(global_vocab));
  dict_initialized = 1;

  /* pack_atom_runtime() (used by skode_dict_promote_macro() to turn a
     runtime macro-name string into the same 32-bit encoding ATOM4()
     produces for compile-time literals) and SKODE_DICT_ATOM() (used by every
     word_table[] entry via WID(), see skode-dict.h) both depend on
     multichar-literal byte order, which is implementation-defined.
     Verified empirically on the reference toolchain (MSB-first) rather
     than assumed -- this three-way check catches a different compiler/
     platform disagreeing, loudly, at startup, instead of silently
     mis-packing every word_table[] entry AND every promoted macro's atom.
     No compile-time equivalent is available: both SKODE_DICT_ATOM("wait") ==
     ATOM4('wait') as a _Static_assert and as a case-label constant were
     tried and both failed ("expression is not constant") -- string-
     literal indexing constant-folds fine for ordinary expressions and
     static initializers, but doesn't qualify as a genuine integer-
     constant-expression the way a multichar literal does, so a runtime
     check is the strongest verification available here. */
  if (pack_atom_runtime("wait", 4) != UINT32_C(0x77616974) ||
      SKODE_DICT_ATOM("wait") != UINT32_C(0x77616974)) {
    fprintf(stderr, "skode-dict: atom-packing mismatch on this build -- "
      "pack_atom_runtime()=%08x SKODE_DICT_ATOM()=%08x ATOM4()=%08x (all three "
      "should be equal). word_table[] entries and/or promoted macros "
      "would silently register wrong atoms. Fix pack_atom_runtime()/"
      "SKODE_DICT_ATOM() before relying on this build.\n",
      (unsigned)pack_atom_runtime("wait", 4), (unsigned)SKODE_DICT_ATOM("wait"),
      (unsigned)UINT32_C(0x77616974));
  }

  for (int i = 0; i < WORD_COUNT; i++) {
    if (skode_dict_register(&global_vocab, &word_table[i]) != 0) {
      /* Duplicate atom -- a programming error in word_table, not a runtime
         condition. Fail loudly during development rather than silently
         dropping a word. */
      fprintf(stderr, "skode-dict: duplicate atom registering '%s'\n",
        word_table[i].name);
    }
  }
}

/* ===================================================================
 * Worked example: overriding a word to enhance rather than replace it
 * ===================================================================
 * Demonstrates skode_dict_override()/self->shadow together: this wraps
 * `m` (mute) to log before delegating to whatever was previously
 * registered in `vocab`, rather than reimplementing mute behavior. NOT
 * called from skode_dict_init() -- installing it is opt-in, via
 * skode_dict_install_example_mute_logger(vocab).
 * skode_dict_revert(vocab, ATOM4('m---')) undoes it, restoring whatever
 * `vocab` had for `m` before (the plain word_table[] entry above, if
 * `vocab` is the global vocab and nothing else has overridden `m` there).
 */
static int word_exec_m_logging(const skode_word_t *self, skode_t *ctx,
    ands_t *s, double *arg, int argc) {
  /* self->shadow is what skode_dict_override() set when this word was
     installed -- no need for a separate skode_dict_shadow() lookup,
     `self` already IS the currently-registered instance. */
  const skode_word_t *shadowed = self->shadow;
  if (ctx->trace) ctx->printf(ctx, "# m override: logging before delegating\n");
  return (shadowed && shadowed->execute)
    ? shadowed->execute(shadowed, ctx, s, arg, argc)
    : -1;
}

static skode_word_t word_m_logging_override = {
  WID("m"), .execute = word_exec_m_logging, .opcode_id = SKODE_OP_MUTE,
  .min_args = 1, .max_args = 1,
  .safety = WORD_REAL_TIME_SAFE, .category = "voice",
  .summary = "mute-audio bool (logging override example)"
};

void skode_dict_install_example_mute_logger(skode_vocab_t *vocab) {
  /* NOTE: word_m_logging_override is a single static instance -- calling
     this more than once (even against different vocabs) reuses the same
     struct, which only makes sense if it's only ever installed into one
     vocab at a time. A version meant for concurrent multi-vocab use would
     need its own instance per call (or per vocab), the same allocate-per-
     use pattern skode_dict_promote_macro() follows. Fine for a worked
     example; flagging so it isn't copied as-is into something that needs
     real multi-vocab concurrency. */
  skode_dict_override(vocab, &word_m_logging_override);
}

/* ===================================================================
 * Macro promotion: turning a verified real-time-safe macro into a
 * pre-compiled dictionary word
 * ===================================================================
 *
 * DESIGN NOTE -- preserving parameter identity:
 *
 * $$N placeholders are resolved by TEXT substitution before
 * skode_compile_program() ever runs (see resolve_macro_placeholders()
 * above); once compiled, an opcode_event_t's arg[] is just plain floats --
 * there's no way to tell "this came from $$0" apart from "the macro author
 * wrote this literal" after the fact. Promotion substitutes each placeholder
 * with an out-of-range variable reference ($128..$135). The normal compiler
 * preserves those references in var_mask; extract_program_params() then
 * moves them into explicit per-operand parameter metadata and clears the
 * temporary variable reference. Valid user variables stop at $127, so no
 * legal source value or variable can collide with this representation.
 */

#define MACRO_PARAM_VAR_BASE (ANDS_VAR_MAX)
#define MACRO_PARAM_NONE UINT8_MAX

/* Same substitution shape as resolve_macro_placeholders() above, but tags
   each placeholder with a reserved out-of-range variable reference.
   Returns 1 on success, 0 if a placeholder index >= SKODE_DICT_MACRO_PARAM_MAX
   was found (promotion must refuse such a macro). *max_param_seen is set
   to the highest placeholder index found, or -1 if none were found. */
static int resolve_macro_placeholders_tagged(const char *body, char *out,
    size_t out_size, int *max_param_seen) {
  size_t oi = 0;
  int max_seen = -1;
  int overflow = 0;
  for (size_t i = 0; body[i] && oi + 1 < out_size; ) {
    int is_param = 0, n = -1, skip = 0;
    if (body[i] == '$' && body[i + 1] == '$' &&
        isdigit((unsigned char)body[i + 2])) {
      size_t j = i + 2;
      n = 0;
      while (isdigit((unsigned char)body[j])) { n = n * 10 + (body[j] - '0'); j++; }
      is_param = 1;
      skip = (int)(j - i);
    }
    if (is_param) {
      if (n >= SKODE_DICT_MACRO_PARAM_MAX) overflow = 1;
      if (n > max_seen) max_seen = n;
      int written = snprintf(out + oi, out_size - oi, "$%d",
        MACRO_PARAM_VAR_BASE + n);
      if (written < 0 || (size_t)written >= out_size - oi) break;
      oi += (size_t)written;
      i += (size_t)skip;
    } else {
      out[oi++] = body[i++];
    }
  }
  out[oi] = '\0';
  if (max_param_seen) *max_param_seen = max_seen;
  return overflow ? 0 : 1;
}

/* Moves reserved variable references into explicit template metadata. */
static void extract_program_params(event_program_t *program,
    uint8_t param[SEQ_PROGRAM_OP_MAX][SEQ_OPCODE_ARG_MAX]) {
  for (int i = 0; i < program->count; i++) {
    opcode_event_t *op = &program->op[i].opcode;
    for (int j = 0; j < op->argc; j++) {
      param[i][j] = MACRO_PARAM_NONE;
      if (op->var_mask & (1U << j)) {
        int variable = (int)op->arg[j];
        if (variable >= MACRO_PARAM_VAR_BASE &&
            variable < MACRO_PARAM_VAR_BASE + SKODE_DICT_MACRO_PARAM_MAX) {
          param[i][j] = (uint8_t)(variable - MACRO_PARAM_VAR_BASE);
          op->var_mask &= (uint8_t)~(1U << j);
          op->arg[j] = 0;
        }
      }
    }
  }
}

/* Returns a copy of `tmpl_op` with every parameter slot replaced by the
   corresponding invocation argument or variable reference. */
static opcode_event_t resolve_template_op(const opcode_event_t *tmpl_op,
    const uint8_t *param, ands_t *parser, const double *arg, int argc) {
  opcode_event_t resolved = *tmpl_op;
  for (int j = 0; j < resolved.argc; j++) {
    int param_index = param[j];
    if (param_index != MACRO_PARAM_NONE && param_index < argc) {
      int variable = parser ? ands_arg_var(parser, param_index) : -1;
      if (variable >= 0) {
        resolved.var_mask |= (uint8_t)(1U << j);
        resolved.arg[j] = (float)variable;
      } else {
        resolved.arg[j] = (float)arg[param_index];
      }
    }
  }
  return resolved;
}

typedef struct skode_macro_template {
  event_program_t program;
  uint8_t param[SEQ_PROGRAM_OP_MAX][SEQ_OPCODE_ARG_MAX];
  char name[ANDS_MACRO_NAME_LEN];  /* owns word.name's storage too */
  skode_word_t word;               /* the registered word; word.data == this */
} skode_macro_template_t;

/* Recovers the enclosing template from a promoted-macro word pointer.
   Only valid when skode_dict_word_is_promoted_macro(word) is true. */
static skode_macro_template_t *template_of(const skode_word_t *word) {
  return (skode_macro_template_t *)((const char *)word -
    offsetof(skode_macro_template_t, word));
}

static int word_exec_promoted_macro(const skode_word_t *self, skode_t *ctx,
    ands_t *s, double *arg, int argc) {
  (void)s;
  const skode_macro_template_t *tmpl = template_of(self);
  if (argc < self->min_args || argc > self->max_args) {
    if (ctx->printf) ctx->printf(ctx,
      "# promoted macro %s: expected %d args, got %d\n",
      self->name, self->min_args, argc);
    return 0;
  }
  event_program_t resolved = tmpl->program;
  for (int i = 0; i < resolved.count; i++)
    resolved.op[i].opcode = resolve_template_op(
      &tmpl->program.op[i].opcode, tmpl->param[i], NULL, arg, argc);
  if (skode_execute_program_state(&resolved, &ctx->voice,
      SAMPLE_COUNT_GET(), -1, ctx->pattern, ctx->step) != 0 && ctx->printf)
    ctx->printf(ctx, "# promoted macro %s failed\n", self->name);
  return 0;
}

static int word_compile_promoted_macro(const skode_word_t *self,
    ands_t *parser, double *arg, int argc, event_program_t *program) {
  const skode_macro_template_t *tmpl = template_of(self);
  if (argc < self->min_args || argc > self->max_args)
    return SKODE_COMPILE_INVALID;
  for (int i = 0; i < tmpl->program.count; i++) {
    if (program->count >= SEQ_PROGRAM_OP_MAX) return SKODE_COMPILE_TOO_LARGE;
    opcode_event_t resolved =
      resolve_template_op(&tmpl->program.op[i].opcode, tmpl->param[i],
        parser, arg, argc);
    program->op[program->count].opcode = resolved;
    program->count++;
  }
  return SKODE_COMPILE_OK;
}

/* Packs an up-to-4-character name into the same 32-bit encoding ATOM4()
   produces for compile-time literals. Multichar-literal byte order is
   implementation-defined; this has been verified empirically (MSB-first,
   matching gcc on this toolchain -- see the startup self-check in
   skode_dict_init()) rather than assumed. Names shorter than 4 characters
   are right-padded with '-', matching how word_table[]'s literals are
   written (e.g. ATOM4('v---')). */
static uint32_t pack_atom_runtime(const char *name, int len) {
  uint32_t v = 0;
  for (int i = 0; i < 4; i++) {
    char c = (i < len) ? name[i] : '-';
    v = (v << 8) | (uint8_t)c;
  }
  return v;
}

int skode_dict_unpromote_macro(skode_vocab_t *vocab, const char *name) {
  if (!vocab || !name || !name[0]) return 0;
  uint32_t atom = pack_atom_runtime(name, (int)strlen(name));
  skode_word_t *current = (skode_word_t *)skode_dict_find(vocab, atom);
  if (!skode_dict_word_is_promoted_macro(current)) return 0;
  skode_word_t *removed = current->shadow ?
    skode_dict_revert(vocab, atom) : skode_dict_remove(vocab, atom);
  skode_dict_free_promoted_macro(removed);
  return 1;
}

skode_word_t *skode_dict_promote_macro(skode_vocab_t *vocab,
    const char *name, const char *body) {
  if (!vocab || !name || !name[0] || !body || !body[0]) return NULL;

  skode_dict_unpromote_macro(vocab, name);

  char tagged[ANDS_MACRO_BODY_LEN];
  int max_param = -1;
  if (!resolve_macro_placeholders_tagged(body, tagged, sizeof(tagged),
      &max_param)) {
    return NULL; /* uses more than SKODE_DICT_MACRO_PARAM_MAX parameters */
  }

  event_program_t compiled;
  skode_compile_result_t result =
    skode_compile_program_ex(tagged, &compiled, vocab);
  if (result != SKODE_COMPILE_OK) {
    /* Not fully, cleanly compilable -- exactly the condition
       compute_macro_safety() would report as something other than
       WORD_REAL_TIME_SAFE. Refuse rather than partially promote. */
    return NULL;
  }

  skode_macro_template_t *tmpl = malloc(sizeof(*tmpl));
  if (!tmpl) return NULL;
  memset(tmpl, 0, sizeof(*tmpl));
  tmpl->program = compiled;
  memset(tmpl->param, MACRO_PARAM_NONE, sizeof(tmpl->param));
  extract_program_params(&tmpl->program, tmpl->param);
  strncpy(tmpl->name, name, ANDS_MACRO_NAME_LEN - 1);
  tmpl->name[ANDS_MACRO_NAME_LEN - 1] = '\0';

  int param_count = (max_param >= 0) ? (max_param + 1) : 0;
  tmpl->word.atom = pack_atom_runtime(tmpl->name, (int)strlen(tmpl->name));
  tmpl->word.name = tmpl->name;
  tmpl->word.execute = word_exec_promoted_macro;
  tmpl->word.compile = word_compile_promoted_macro;
  tmpl->word.opcode_id = SKODE_OP_NONE; /* compile is always via compile_fn */
  tmpl->word.min_args = (uint8_t)param_count;
  tmpl->word.max_args = (uint8_t)param_count;
  tmpl->word.default_mask = 0;
  tmpl->word.safety = WORD_REAL_TIME_SAFE;
  tmpl->word.category = "macro";
  tmpl->word.summary = "promoted macro (auto-compiled)";
  tmpl->word.next = NULL;
  tmpl->word.shadow = NULL;
  tmpl->word.data = tmpl;

  skode_dict_override(vocab, &tmpl->word);
  return &tmpl->word;
}

int skode_dict_word_is_promoted_macro(const skode_word_t *word) {
  return word && word->execute == word_exec_promoted_macro;
}

skode_word_t *skode_dict_free_promoted_macro(skode_word_t *word) {
  if (!skode_dict_word_is_promoted_macro(word)) return word;
  skode_macro_template_t *tmpl = template_of(word);
  free(tmpl);
  return NULL;
}
