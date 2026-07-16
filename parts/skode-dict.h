#ifndef _SKODE_DICT_H_
#define _SKODE_DICT_H_

/*
 * Forth-style command dictionary for skode.
 *
 * BACKGROUND (why this header looks the way it does):
 *
 * skode currently has two independent ATOM4-keyed switch statements that
 * must be kept in sync by hand:
 *
 *   1. skode_function()          in skode.c.kit   -- interactive execution
 *   2. skode_compile_callback()  in skode-event.c  -- compiles a command into
 *                                                     an opcode_event_t for
 *                                                     scheduling/patterns
 *
 * Most words follow the same shape in (2): validate argc against a
 * [min,max] range, then push one opcode via the (currently static)
 * program_push() helper. This header captures that common shape directly in
 * skode_word_t (opcode_id/min_args/max_args/default_mask) so that ~80% of
 * words need no compile_fn at all -- only the handful of words with genuinely
 * custom compiled behavior (DELAY, the external-macro atom, etc.) need one.
 *
 * Macros ('[name]:body;') are pure text substitution performed in
 * ands_preprocess_macros() before any atom is ever looked up here, so they
 * are NOT registered as dictionary words. Instead, compute_macro_safety()
 * classifies a macro body by asking the real compiler (skode_compile_program)
 * whether it survives compilation -- this guarantees the classification can
 * never drift out of sync with what the scheduler actually accepts.
 *
 * Macro bodies as stored may contain unresolved `$$N` argument
 * placeholders (only substituted at invocation time), so
 * compute_macro_safety() resolves those to a neutral literal before
 * compiling -- it classifies a macro's *shape*, not any particular
 * invocation. See the implementation comment in skode-dict.c for why this
 * is safe (placeholder values can't affect OK vs IMMEDIATE_ONLY).
 */

#include <stdint.h>

/*
 * SKODE_DICT_ATOM(s) / WID(s) -- typing a word's identity once, not twice
 * ---------------------------------------------------------------------
 * ATOM4('v---') exists because GCC multichar literals are genuine integer
 * constants, cheap to write inline in a `case` label -- and that's still
 * the right tool for the two legacy switch statements in skode.c.kit/
 * skode-event.c, unchanged by anything here. It's a worse fit for
 * skode_word_t's struct initializers: every word_table[] entry needs
 * BOTH the packed atom (`'v---'`, hand-padded with dashes) AND the
 * display name (`"v"`) -- the same 1-4 characters, typed twice, in two
 * different literal syntaxes, with nothing checking they agree. A typo
 * in either one is a silent mismatch, not a compile error.
 *
 * SKODE_DICT_ATOM(s) takes an ORDINARY C STRING (no manual dash-padding) and
 * computes the same packed uint32_t ATOM4('v---') would, by leaning on
 * C's *guaranteed*, fully portable automatic string-literal concatenation
 * (unlike ATOM4 itself, this part isn't a GCC extension) to pad with
 * dashes, then indexing the padded literal at constant, always-in-bounds
 * positions 0..3 -- safe because the padded literal is always >= 5 bytes
 * when the input is <= 4 characters, which every atom name in this
 * codebase already is.
 *
 * VERIFIED, not assumed: compiled against the initial dictionary atom names
 * currently in word_table[] and confirmed bit-for-bit identical to the
 * existing ATOM4('x---') values, at both -O0 and -O2, clean under
 * -Wall -Wextra -Wno-multichar, in both a function-local and a
 * file-scope static initializer (the context word_table[] actually
 * uses).
 *
 * NOT usable as a `case` label or in `_Static_assert` -- both tried,
 * both fail with "expression is not constant": indexing a string literal
 * at a constant index is accepted by GCC for ordinary constant-folding
 * (expressions, static initializers) but is not treated as a genuine
 * integer-constant-expression, which `case`/`_Static_assert` strictly
 * require and multichar literals (being honest `int` constants) satisfy.
 * This is exactly why the legacy switches keep using ATOM4('x---')
 * unchanged -- it isn't a style preference, it's the actual boundary of
 * what each mechanism is legally allowed to do. Consequently there is no
 * compile-time way to cross-check SKODE_DICT_ATOM against ATOM4 either (both
 * `_Static_assert(SKODE_DICT_ATOM("wait") == ATOM4('wait'), ...)` attempts
 * failed the same way) -- the runtime self-check in skode_dict_init()
 * remains the verification mechanism, extended to cover this macro too
 * (see the three-way check there: ATOM4, SKODE_DICT_ATOM, and
 * pack_atom_runtime() should all agree).
 */
#define SKODE_DICT_ATOM(s) \
  ( ((uint32_t)(uint8_t)(s "----")[0] << 24) | \
    ((uint32_t)(uint8_t)(s "----")[1] << 16) | \
    ((uint32_t)(uint8_t)(s "----")[2] << 8)  | \
    ((uint32_t)(uint8_t)(s "----")[3]) )

/*
 * WID(s) -- "Word ID": expands to `.atom = ..., .name = s`, designated-
 * initializer clauses in the exact fields skode_word_t declares them as,
 * so a word_table[] entry reads as
 *   { WID("v"), .execute = word_exec_v, .opcode_id = SKODE_OP_VOICE,
 *     .min_args = 1, .max_args = 1, .safety = WORD_REAL_TIME_SAFE,
 *     .category = "voice", .summary = "voice-select voice" }
 * -- the string is typed exactly once, every other field is named rather
 * than positional (no more counting commas to know which field a bare
 * value lands in, no risk of two adjacent fields silently swapping), and
 * every field not mentioned (.compile, .default_mask, .next, .shadow,
 * .data) zero-initializes automatically -- C99 guarantees this for
 * designated initializers exactly like it already does for the trailing
 * fields of an ordinary positional one, just without requiring every
 * entry to spell out the same three trailing NULLs by hand. Verified: a
 * standalone test with this exact macro shape, mixed with designated
 * initializers for the remaining fields, produces identical values to
 * the original positional form and correctly zero-initializes every
 * omitted field, clean under -Wall -Wextra.
 */
#define WID(s) .atom = SKODE_DICT_ATOM(s), .name = s

#include "ands.h"
#include "seq.h"    /* opcode_event_t, event_program_t */
#include "skode.h"  /* skode_t, skode_opcode_t, skode_compile_result_t */

typedef enum {
  /* Compiles cleanly via skode_compile_program(); fine to schedule, queue,
     or bake into a pattern step. */
  WORD_REAL_TIME_SAFE = 0,
  /* Executes by enqueueing/deferring rather than acting synchronously.
     Also the classification given to macros whose compile result was
     ambiguous (SKODE_COMPILE_INVALID / SKODE_COMPILE_TOO_LARGE) rather than
     a clean pass/fail -- treated as "allow, but don't claim verified". */
  WORD_MAY_QUEUE,
  /* Must run synchronously on the interactive path (REPL text, printing
     help, touching the parser argument stack, file I/O, etc). Cannot be
     compiled into an event_program_t. */
  WORD_IMMEDIATE_ONLY,
} word_safety_t;

#define SKODE_DICT_BUCKETS (256)

/* Forward declaration -- skode_exec_fn/skode_compile_fn below take a
   pointer to the word being invoked, so a handler shared by multiple
   words (see skode_dict_promote_macro()) can recover its own per-word
   data; the full definition follows after these typedefs. */
struct skode_word;

/*
 * A vocabulary is a dictionary namespace: its own bucket-hashed set of
 * atom -> skode_word_t registrations, structurally identical to (and
 * reusing every mechanism of) what used to be the single implicit global
 * table. Two are relevant in practice:
 *
 *   - skode_dict_global_vocab() -- the shared, process-global vocabulary
 *     word_table[]'s builtins live in. One instance, for the whole process,
 *     matching global_var[] and the rest of skode's process-global state.
 *   - a private vocab, one per skode_t (see the `vocab` field skode.h
 *     needs to gain -- SKODE_DICT_INTEGRATION.md), created in skode_init()
 *     and destroyed in skode_free().
 *
 * Lookup checks a private vocab first, falling back to the global one
 * (skode_dict_lookup()) -- Forth's CONTEXT search order, deliberately
 * simplified to exactly two levels rather than an arbitrary vocabulary
 * stack (ONLY/ALSO/PREVIOUS and friends), since that's what this system
 * actually needs: isolate each skode_t's overrides/promoted macros from
 * every other one, nothing more. Mutation (register/override/revert/
 * remove/promote) always targets one specific, explicitly-named vocab --
 * Forth's CURRENT -- never resolved implicitly the way lookup is.
 *
 * Every mutation/lookup primitive below is deliberately vocab-agnostic:
 * they operate identically whether handed the global vocab or a private
 * one. There is no hidden global state left inside them (skode_dict_init()
 * is the one exception, bootstrapping the global vocab itself and
 * word_table[]'s builtins into it).
 */
typedef struct skode_vocab {
  struct skode_word *buckets[SKODE_DICT_BUCKETS];
} skode_vocab_t;


/*
 * Immediate/interactive execution, called from the dictionary lookup that
 * replaces (part of) skode_function()'s switch.
 *
 *   self  - the word being executed. Present so a generic handler shared
 *           by multiple words (e.g. every promoted macro uses the same
 *           execute function) can recover its own per-word data via
 *           self->data. Ordinary hand-written words ignore it.
 *   ctx   - the active skode context (voice, arg stack, log, trace, ...)
 *   s     - the live ands parser instance. Needed by words that touch the
 *           parser's own argument stack (clr/drop/dup/over/rot/swap) or that
 *           need ands_atom_string()/ands_data_len() etc.
 *   arg   - ands_arg(s): the parsed numeric argument vector
 *   argc  - ands_arg_len(s)
 *
 * Return value mirrors the existing skode_function() convention: 0 for the
 * ordinary "handled" path (equivalent to falling out of a `case` via
 * `break`), 1 for the parser-stack ops that must suppress the implicit
 * argument-stack reset the caller performs after a FUNCTION dispatch
 * (equivalent to the existing `return 1;` sites).
 */
typedef int (*skode_exec_fn)(const struct skode_word *self, skode_t *ctx,
  ands_t *s, double *arg, int argc);

/*
 * Optional custom compiler, for the minority of words whose compiled form
 * isn't "push exactly one opcode with these args" (opcode_id below covers
 * that common case generically and needs no compile_fn).
 *
 *   self    - see skode_exec_fn above.
 *   parser  - the compile-mode ands parser (for ands_arg_var(), i.e.
 *             argument-by-reference to a global variable)
 *   arg/argc- parsed numeric arguments
 *   program - the event_program_t being built; a custom compiler appends to
 *             it directly (see program_append()/program_push() in
 *             skode-event.c)
 *
 * Returns a skode_compile_result_t (typed int here to avoid a header loop;
 * skode.h defines the enum).
 */
typedef int (*skode_compile_fn)(const struct skode_word *self,
  ands_t *parser, double *arg, int argc, event_program_t *program);

typedef struct skode_word {
  uint32_t atom;              /* ATOM4-encoded command name, e.g. ATOM4('a---') */
  const char *name;           /* human-readable name, e.g. "a" */
  skode_exec_fn execute;      /* interactive execution; every registered word has one */
  skode_compile_fn compile;   /* custom compiler, or NULL to use opcode_id below */
  uint8_t opcode_id;          /* skode_opcode_t, or SKODE_OP_NONE if not opcode-backed */
  uint8_t min_args;
  uint8_t max_args;
  uint8_t default_mask;       /* bit i set => arg[i] may be NaN/omitted (mirrors
                                  program_push()'s default_mask) */
  word_safety_t safety;
  const char *category;       /* mirrors the existing @doc "category:" field */
  const char *summary;        /* mirrors the existing @doc "summary:" field */
  struct skode_word *next;    /* bucket-chain link -- distinct atoms only; see
                                  skode_dict_override()/skode_dict_revert() */
  struct skode_word *shadow;  /* previous registration for the SAME atom, or
                                  NULL. Forms a per-atom version stack, kept
                                  separate from the bucket chain so overriding
                                  an atom never grows bucket-lookup cost --
                                  see skode-dict.c for the splice mechanics. */
  void *data;                 /* opaque per-word data, unused (NULL) by every
                                  hand-written word. Promoted macros point
                                  this at their skode_macro_template_t; the
                                  shared player functions in skode-dict.c
                                  recover it via self->data. */
} skode_word_t;

/* Initializes the dictionary and registers all built-in words. Safe to call
   more than once (idempotent). Call this from skode_init() / once at
   process start -- the dictionary is process-global, matching global_var[]
   and the other skode globals it sits next to. */
void skode_dict_init(void);

/* The shared, process-global vocabulary word_table[]'s builtins are
   registered into by skode_dict_init(). Always non-NULL after
   skode_dict_init() has run. */
skode_vocab_t *skode_dict_global_vocab(void);

/* Allocates and zero-initializes a new, empty private vocabulary. Intended
   for one-per-skode_t use (see skode.h's `vocab` field and the
   skode_init()/skode_free() hooks in SKODE_DICT_INTEGRATION.md), but
   nothing here assumes that -- a vocab is just a namespace. */
skode_vocab_t *skode_dict_vocab_create(void);

/*
 * Frees `vocab` and everything skode_dict_promote_macro() allocated into
 * it -- walks every bucket's full shadow chain, freeing each promoted-
 * macro word and its template (skode_dict_word_is_promoted_macro() decides
 * which entries those are), then frees `vocab` itself. Hand-written
 * words/overrides registered into `vocab` are left untouched (the
 * dictionary never allocated them, so it doesn't free them) -- if the
 * caller registered any such words in a vocab it's about to destroy, it
 * remains responsible for them exactly as it always was.
 *
 * Does not touch skode_dict_global_vocab() -- calling this on it would be
 * a caller bug (freeing process-global state), not something this
 * function guards against.
 */
void skode_dict_vocab_destroy(skode_vocab_t *vocab);

/*
 * Two-level lookup -- Forth's CONTEXT search order, fixed at exactly two
 * levels: checks `private_vocab` first (if non-NULL), falling back to
 * skode_dict_global_vocab(). This is what skode_execute_word() and
 * skode_compile_word() use internally; call it directly only if you need
 * dictionary lookup outside of dispatch (introspection, tooling, etc).
 */
const skode_word_t *skode_dict_lookup(skode_vocab_t *private_vocab,
  uint32_t atom);

/*
 * Registers a single word. Returns 0 on success, -1 if the atom is already
 * registered (duplicate atoms are a programming error, not a runtime
 * condition, so callers should treat -1 as a hard failure during startup).
 * This is the strict path -- used only for word_table[]'s static/builtin
 * entries at skode_dict_init() time, where a collision is always a bug in
 * word_table itself. Runtime redefinition must go through
 * skode_dict_override() instead; skode_dict_register() will never silently
 * shadow anything.
 *
 * Concurrency contract (applies to this and the three functions below):
 * none of them do any internal locking, matching the "minimal runtime
 * overhead" principle this system was built around. This is safe with
 * respect to the real-time audio path unconditionally --
 * skode_execute_voice_opcode() (the actual audio/scheduler-side execution
 * of a compiled opcode_event_t) switches purely on a numeric opcode ID and
 * never dereferences a skode_word_t, so the dictionary is never touched
 * from that thread. It is NOT safe to call these from more than one
 * control-plane thread concurrently (e.g. a UDP listener thread and a
 * local REPL thread both calling skode_consume() at once) without external
 * synchronization -- skode_dict_find() walking a bucket while another
 * thread mutates it is an ordinary data race. If skode's control plane is
 * (or becomes) multi-threaded, either funnel all skode_consume() calls
 * through one thread (skqueue.h looks like the natural existing mechanism
 * for that -- confirm before relying on it) or add a mutex around these
 * four calls. This was not previously a concern because
 * skode_dict_register() was only ever called once, at startup, before any
 * concurrent access could occur; override/revert/remove change that
 * assumption and need this resolved before they're used from more than one
 * thread. Vocabularies don't change this contract -- it applies per-vocab
 * (two threads mutating *different* vocabs concurrently are fine; the race
 * is specifically about two threads touching the *same* skode_vocab_t).
 */
int skode_dict_register(skode_vocab_t *vocab, skode_word_t *word);

/*
 * Registers `word` for its atom in `vocab`, shadowing whatever was
 * previously registered for that atom in that *same* vocab (a private
 * vocab overriding an atom never touches the global vocab's version of
 * it, and vice versa) rather than rejecting the call. The previously-
 * visible word (if any) becomes reachable via `word->shadow`, and is
 * unlinked from the bucket chain -- overriding an atom N times costs O(1)
 * bucket-chain length regardless of N; lookup cost for unrelated atoms in
 * the same bucket is unaffected.
 *
 * Returns the word that was shadowed (NULL if this atom had no prior
 * registration in `vocab`). This is the hook for "enhance" rather than
 * "replace": an override's execute/compile functions can look up
 * self->shadow and call through to the previous implementation (see
 * skode-dict.c's overridable-word example), the same technique classic
 * Forth systems use to extend a word's definition without losing the
 * original.
 *
 * Caller owns `word`'s storage (static, arena, or malloc'd) -- the
 * dictionary only manages the intrusive links, never allocates or frees.
 */
skode_word_t *skode_dict_override(skode_vocab_t *vocab, skode_word_t *word);

/*
 * Returns the word immediately shadowed by the currently-visible
 * registration for `atom` in `vocab` (i.e. what skode_dict_revert() would
 * expose), or NULL if `atom` isn't registered in `vocab` or has never been
 * overridden there. O(bucket length) -- does not walk the full shadow
 * chain. Prefer self->shadow (see skode_dict_override() above) when
 * called from within a word's own execute/compile function -- it's O(1)
 * and doesn't require knowing which vocab `self` was registered in.
 */
const skode_word_t *skode_dict_shadow(skode_vocab_t *vocab, uint32_t atom);

/*
 * Reverts `atom` to its previous registration in `vocab`, undoing the
 * most recent skode_dict_override() for that atom in that vocab. Splices
 * the shadowed word back into the bucket chain at the position the
 * reverted word occupied, so lookup cost for neighboring atoms is
 * unaffected.
 *
 * Deliberately refuses to revert a word with no shadow (i.e. one that has
 * never been overridden in `vocab`, including every static word_table[]
 * entry in the global vocab) -- that would silently unregister the atom,
 * which is a materially different operation from "undo the last override"
 * and must be requested explicitly via skode_dict_remove(). Returns the
 * reverted word on success (caller owns its storage), NULL if `atom` isn't
 * registered in `vocab` or has no shadow there.
 */
skode_word_t *skode_dict_revert(skode_vocab_t *vocab, uint32_t atom);

/*
 * Fully removes `atom` from `vocab`, including every shadowed
 * registration beneath it within that vocab -- after this call
 * skode_dict_find(vocab, atom) returns NULL for that vocab specifically
 * (skode_dict_lookup() may still find a registration in the other level
 * of the search order). Useful for temporarily disabling a word, or for
 * A/B comparing dictionary vs. legacy behavior at runtime.
 *
 * Returns the head of the removed version chain (walk ->shadow to reach
 * older registrations); caller owns all of their storage -- or see
 * skode_dict_free_promoted_macro() if some of them are promoted macros.
 * NULL if `atom` wasn't registered in `vocab`.
 */
skode_word_t *skode_dict_remove(skode_vocab_t *vocab, uint32_t atom);

/*
 * Worked example of the "enhance" pattern enabled by skode_dict_override():
 * installs a wrapped version of `m` (mute) into `vocab` that logs before
 * delegating to whatever was previously registered for that atom in that
 * vocab. Not called from skode_dict_init() -- this is a demonstration/
 * test hook, not a default behavior change. Call
 * skode_dict_revert(vocab, ATOM4('m---')) to undo it. See skode-dict.c
 * for the implementation.
 */
void skode_dict_install_example_mute_logger(skode_vocab_t *vocab);

/* Raw, single-vocab lookup. Returns NULL if the atom isn't registered in
   `vocab` specifically (use skode_dict_lookup() for the two-level search
   order most callers actually want). Always returns the currently-visible
   (most recently overridden) registration within `vocab`. */
const skode_word_t *skode_dict_find(skode_vocab_t *vocab, uint32_t atom);

/*
 * Dispatch used from skode_function(): looks the atom up via
 * skode_dict_lookup(ctx->vocab, atom) (ctx->vocab is a new field skode.h
 * needs to gain -- see SKODE_DICT_INTEGRATION.md) and, on a hit, calls its
 * execute fn and writes the result to *out_result. Returns 1 if the
 * dictionary handled the atom, 0 if the caller should fall back to the
 * legacy switch statement (this is the "incremental migration" seam -- see
 * SKODE_DICT_INTEGRATION.md).
 */
int skode_execute_word(skode_t *ctx, ands_t *s, uint32_t atom, double *arg,
  int argc, int *out_result);

/*
 * Dispatch used from skode_compile_callback(): same fallback contract, but
 * builds into an in-progress event_program_t and reports a
 * skode_compile_result_t. Returns 1 on a hit (dictionary handled it,
 * including the case where handling it means reporting
 * SKODE_COMPILE_IMMEDIATE_ONLY), 0 on a miss (fall back to the legacy
 * switch).
 *
 * `private_vocab` is explicit here, unlike skode_execute_word()'s implicit
 * ctx->vocab -- compilation has no reachable skode_t at all (see the
 * design note in SKODE_DICT_INTEGRATION.md: skode_compile_t, the compile-
 * mode parser's user pointer, has never carried one). Pass NULL for
 * global-vocab-only compilation (the existing behavior, unaffected if the
 * caller doesn't care about vocabularies); pass a specific private vocab
 * to make that vocab's overrides/promoted macros visible on the compile
 * path too.
 */
int skode_compile_word(ands_t *parser, uint32_t atom, double *arg, int argc,
  event_program_t *program, skode_vocab_t *private_vocab,
  int *out_compile_result);

/*
 * Classifies a macro body by attempting to compile it with the real
 * compiler (skode_compile_program_ex -- see SKODE_DICT_INTEGRATION.md for
 * the new skode_compile_program_ex()/private_vocab plumbing this now
 * depends on). This deliberately does not re-implement a parallel lexer/
 * dictionary walk: reusing the actual compiler guarantees the
 * classification can never disagree with what the scheduler accepts.
 *
 * `vocab` makes any private-vocab words/promoted macros the body
 * references visible during the check (e.g. a macro that calls another
 * promoted macro living only in the same private vocab) -- pass NULL to
 * check against the global vocab only, which is correct for any macro
 * that doesn't depend on private-vocab-only words.
 *
 *   SKODE_COMPILE_OK              -> WORD_REAL_TIME_SAFE
 *   SKODE_COMPILE_IMMEDIATE_ONLY  -> WORD_IMMEDIATE_ONLY
 *   SKODE_COMPILE_INVALID /
 *   SKODE_COMPILE_TOO_LARGE       -> WORD_MAY_QUEUE (ambiguous: allowed, but
 *                                     not verified real-time-safe)
 */
word_safety_t compute_macro_safety(skode_vocab_t *vocab, const char *body);
skode_compile_result_t skode_dict_macro_compile_status(
  skode_vocab_t *vocab, const char *body);

/*
 * Reports a macro's computed safety to the user. Call this from
 * skode_callback()'s MACRO_DEFINED case (see SKODE_DICT_INTEGRATION.md §4
 * -- ands.c notifies via the generic s->fn(s, MACRO_DEFINED) callback it
 * already uses for FUNCTION/DEFER/etc., staying skode-agnostic itself)
 * -- macros are not dictionary words themselves (they're text
 * substitution), so this only classifies and prints feedback. Uses
 * ctx->vocab internally (see compute_macro_safety() above for why that
 * matters) -- no separate vocab parameter needed since ctx already
 * carries one.
 */
void skode_dict_report_macro_safety(skode_t *ctx, const char *name,
  const char *body);

/*
 * Maximum number of distinct $$N parameter slots a promotable macro may
 * use. Bounded to fit comfortably within SEQ_OPCODE_ARG_MAX (8) -- no
 * single opcode in this codebase takes more than 8 arguments, and a macro
 * needing more distinct parameters than that almost certainly wants to be
 * several separate commands rather than one promoted word.
 */
#define SKODE_DICT_MACRO_PARAM_MAX (8)

/*
 * Attempts to compile `body` (a macro's stored, not-yet-invoked text, same
 * input compute_macro_safety() takes) into a reusable, pre-compiled
 * dictionary word registered into `vocab`: an atom that dispatches
 * directly through the already-compiled opcode sequence, on both the
 * interactive and compile paths, without re-running
 * ands_preprocess_macros() text substitution on every invocation.
 * `vocab` serves double duty: it's both the compile-time context (any
 * private-vocab words the body references need to be visible while
 * compiling it, same as compute_macro_safety() above) and the
 * registration target (skode_dict_override(vocab, ...) is used
 * internally) -- in practice these are naturally the same vocab, since a
 * macro's body would reference whatever is visible in its own eventual
 * search order. Pass skode_dict_global_vocab() to promote into the shared
 * global vocabulary instead of a private one.
 *
 * Only promotes bodies that compile cleanly and entirely to opcodes --
 * i.e. the same condition compute_macro_safety() reports as
 * WORD_REAL_TIME_SAFE. A body containing anything WORD_IMMEDIATE_ONLY
 * (strings, arrays, other non-opcode constructs) cannot be promoted; this
 * function returns NULL rather than partially promoting it.
 *
 * `$$N` placeholders are preserved through promotion, not baked to a
 * neutral value: the promoted word takes exactly as many arguments as the
 * macro's highest-numbered placeholder requires (max_args == min_args ==
 * highest N + 1), and each invocation substitutes the caller's real
 * arguments into the pre-compiled opcode template before pushing/executing
 * it. See skode-dict.c for how parameter slots are detected and tagged
 * (magnitude sentinel during the text round-trip through the real
 * compiler; converted to a payload-carrying NaN for the template's stored
 * form -- the text lexer's START-state character-class gate makes a
 * literal NaN unreachable via text injection, see the design note in
 * skode-dict.c).
 *
 * On success, registers the promoted word via skode_dict_override(vocab,
 * ...) (so redefining an already-promoted macro, or a macro whose name
 * happens to collide with an existing dictionary atom, works via the same
 * shadow mechanism as any other override) and returns it. Returns NULL if
 * the body doesn't compile cleanly, uses more than
 * SKODE_DICT_MACRO_PARAM_MAX parameters, or the compiled template would
 * exceed SEQ_PROGRAM_OP_MAX opcodes.
 *
 * Allocates (word + template) on success -- the one deliberate exception
 * to this system's no-hidden-allocation principle, scoped to macro
 * *definition* time (a control-plane, non-real-time event that already
 * allocates elsewhere in the macro subsystem), never to invocation.
 * Freed by whichever of skode_dict_revert()/skode_dict_remove() eventually
 * un-registers it, or in bulk by skode_dict_vocab_destroy() -- callers
 * holding onto a promoted word outside of those paths remain responsible
 * for it, the same ownership contract as every other skode_dict_*
 * mutation.
 */
skode_word_t *skode_dict_promote_macro(skode_vocab_t *vocab,
  const char *name, const char *body);
int skode_dict_unpromote_macro(skode_vocab_t *vocab, const char *name);

/*
 * True if `word` was produced by skode_dict_promote_macro() (as opposed to
 * a hand-written word_table[] entry or a hand-written override). Useful
 * before calling skode_dict_revert()/skode_dict_remove() on it if the
 * caller wants to know whether it needs to free `word` and its template
 * afterward.
 */
int skode_dict_word_is_promoted_macro(const skode_word_t *word);

/*
 * Frees a promoted-macro word and its template (one allocation covers
 * both -- see skode-dict.c). No-op (returns `word` unchanged) if `word`
 * wasn't produced by skode_dict_promote_macro(), so it's safe to call on
 * anything skode_dict_revert()/skode_dict_remove() hands back without
 * checking skode_dict_word_is_promoted_macro() first. Always returns NULL
 * on an actual free, as a reminder not to touch `word` afterward.
 */
skode_word_t *skode_dict_free_promoted_macro(skode_word_t *word);

#endif
