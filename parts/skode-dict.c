#include "skode-dict.h"
#include "skode_dict_gen.h"

#include "midi_player.h"
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
 * The words below call a few helpers that were previously `static` inside
 * skode.c, or not declared in any shared header:
 *
 *   skode.c       : skode_double_to_int  -> non-static, declared in skode.h
 *   skode.c       : sk_sleep             -> non-static, declared in skode.h
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
  if (skode_vocab_find(vocab, word->atom)) return -1;
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
  const skode_word_t *w = skode_vocab_find(vocab, atom);
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

const skode_word_t *skode_vocab_find(skode_vocab_t *vocab, uint32_t atom) {
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
    const skode_word_t *w = skode_vocab_find(private_vocab, atom);
    if (w) return w;
  }
  const skode_word_t *w = skode_vocab_find(&global_vocab, atom);
  if (w) return w;

  /* Check static .def dictionary if not found in dynamic vocabularies */
  static skode_word_t s_def_word_shim;
  char name[5];
  int len = 0;
  for (int i = 0; i < 4; i++) {
    char ch = (char)((atom >> ((3 - i) * 8)) & 0xFF);
    if (ch == '-' || ch == '\0') break;
    name[len++] = ch;
  }
  name[len] = '\0';
  if (len > 0) {
    const skode_def_word_t *gw = skode_dict_find(name);
    if (gw) {
      memset(&s_def_word_shim, 0, sizeof(s_def_word_shim));
      s_def_word_shim.atom = atom;
      s_def_word_shim.name = gw->name;
      s_def_word_shim.summary = (gw->clauses && gw->clause_count > 0) ? gw->clauses[0].summary : NULL;
      s_def_word_shim.safety = (gw->clauses && (gw->clauses[0].flags & SKODE_CLAUSE_RT_SAFE))
                               ? WORD_REAL_TIME_SAFE : WORD_IMMEDIATE_ONLY;
      return &s_def_word_shim;
    }
  }
  return NULL;
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
  if (w && w->execute) {
    if (ctx->trace) {
      ctx->printf(ctx, "# SKODE_DICT %s", w->name);
      for (int i = 0; i < argc; i++) ctx->printf(ctx, " %g", arg[i]);
      ctx->puts(ctx, "");
    }
    int r = w->execute(w, ctx, s, arg, argc);
    if (out_result) *out_result = r;
    return 1;
  }
  const char *name = (w && w->name) ? w->name : ands_atom_name(s);
  char clean_name[32];
  if (name && name[0]) {
    size_t nl = strlen(name);
    while (nl > 0 && name[nl - 1] == '-') nl--;
    if (nl < sizeof(clean_name)) {
      memcpy(clean_name, name, nl);
      clean_name[nl] = '\0';
      name = clean_name;
    }
  }
  if (strcmp(name, "EXEC") == 0) {
    if (argc == 0) {
      if (out_result) *out_result = -1;
      return 1;
    }
    int opcode_num = (int)arg[0];
    if (opcode_num < 0 || opcode_num > 255 ||
        !skode_opcode_is_realtime((skode_opcode_t)opcode_num) ||
        !skode_opcode_supported((skode_opcode_t)opcode_num)) {
      if (out_result) *out_result = -1;
      return 1;
    }
    int inner_argc = argc - 1;
    opcode_event_t op;
    memset(&op, 0, sizeof(op));
    op.code = (uint16_t)opcode_num;
    op.argc = (uint8_t)inner_argc;
    for (int i = 0; i < inner_argc && i < SEQ_OPCODE_ARG_MAX; i++) op.arg[i] = (float)arg[i+1];
    int r = skode_execute_voice_opcode(&op, ctx ? ctx->voice : 0);
    if (out_result) *out_result = r;
    return 1;
  }
  const skode_def_word_t *gw = skode_dict_find(name);
  if (gw) {
    unsigned var_mask = 0;
    if (s) {
      for (int i = 0; i < argc && i < SEQ_OPCODE_ARG_MAX; i++) {
        if (ands_arg_var(s, i) >= 0) var_mask |= (1U << i);
      }
    }
    const skode_clause_t *c = skode_clause_match_ex(gw, argc, arg, var_mask);
    if (c) {
      if (ctx && ctx->trace) {
        ctx->printf(ctx, "# SKODE_DEF %s", gw->name);
        for (int i = 0; i < argc; i++) ctx->printf(ctx, " %g", arg[i]);
        ctx->puts(ctx, "");
      }
      if (c->imm_handler) {
        int r = c->imm_handler(ctx, s, arg, argc);
        if (out_result) *out_result = r;
        return 1;
      } else if (c->opcode_id != 0) {
        opcode_event_t op;
        memset(&op, 0, sizeof(op));
        op.code = c->opcode_id;
        op.argc = argc;
        for (int i = 0; i < argc && i < SEQ_OPCODE_ARG_MAX; i++) {
          op.arg[i] = (float)arg[i];
        }
        int r = skode_execute_voice_opcode(&op, ctx ? ctx->voice : 0);
        if (out_result) *out_result = r;
        return 1;
      }
    } else {
      char reason[128];
      skode_clause_explain_mismatch(gw, argc, arg, reason, sizeof(reason));
      if (ctx) ctx->printf(ctx, "# SKODE_NO_MATCHING_CLAUSE %s (argc=%d: %s)\n", name, argc, reason);
      if (out_result) *out_result = -1;
      return 1;
    }
  }
  return 0;
}

int skode_compile_word(ands_t *parser, uint32_t atom, double *arg, int argc,
    event_program_t *program, skode_vocab_t *private_vocab,
    int *out_compile_result) {
  const skode_word_t *w = NULL;
  if (private_vocab) {
    w = skode_vocab_find(private_vocab, atom);
  }
  if (!w) {
    w = skode_vocab_find(&global_vocab, atom);
  }
  if (w) {
    if (w->safety == WORD_IMMEDIATE_ONLY) {
      return 0;
    }

    if (w->compile) {
      int rc = w->compile(w, parser, arg, argc, program);
      if (out_compile_result) *out_compile_result = rc;
      return 1;
    }

    if (w->opcode_id == SKODE_OP_NONE) {
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

  char clean_name[32];
  const char *name = ands_atom_name(parser);
  if (name && name[0]) {
    size_t nl = strlen(name);
    while (nl > 0 && name[nl - 1] == '-') nl--;
    if (nl < sizeof(clean_name)) {
      memcpy(clean_name, name, nl);
      clean_name[nl] = '\0';
      name = clean_name;
    }
  }

  if (strcmp(name, "EXEC") == 0) {
    if (argc == 0) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int opcode_num = (int)arg[0];
    if (opcode_num < 0 || opcode_num > 255 ||
        !skode_opcode_is_realtime((skode_opcode_t)opcode_num) ||
        !skode_opcode_supported((skode_opcode_t)opcode_num)) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int inner_argc = argc - 1;
    double inner_args[SEQ_OPCODE_ARG_MAX] = {0};
    for (int i = 0; i < inner_argc; i++) inner_args[i] = arg[i+1];
    int rc = skode_program_push(program, (skode_opcode_t)opcode_num, parser,
      inner_args, inner_argc, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }

  if (strcmp(name, "v") == 0 || strcmp(name, "voice") == 0) {
    if (argc != 1) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_VOICE, parser, arg, 1, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "a") == 0 || strcmp(name, "amp") == 0) {
    if (argc != 1) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_AMP, parser, arg, 1, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "f") == 0 || strcmp(name, "freq") == 0) {
    if (argc != 1) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_FREQ, parser, arg, 1, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "m") == 0 || strcmp(name, "mute") == 0) {
    if (argc != 1) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_MUTE, parser, arg, 1, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "n") == 0) {
    if (argc < 1 || argc > 2) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_MIDI_NOTE, parser, arg, argc, 0, 1);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "i") == 0) {
    if (argc < 1 || argc > 3) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_INST_NOTE, parser, arg, argc, 0, 1);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "p") == 0 || strcmp(name, "pan") == 0) {
    if (argc != 1) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_PAN, parser, arg, 1, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "l") == 0 || strcmp(name, "velocity") == 0) {
    if (argc != 1) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_VELOCITY, parser, arg, 1, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "on") == 0) {
    if (argc > 1) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    double val = (argc >= 1) ? arg[0] : 1.0;
    int rc = skode_program_push(program, SKODE_OP_VELOCITY, parser, &val, 1, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "off") == 0) {
    if (argc > 1) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    double val = 0.0;
    int rc = skode_program_push(program, SKODE_OP_VELOCITY, parser, &val, 1, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "___l") == 0) {
    if (argc != 1) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_ENVELOPE_VELOCITY, parser, arg, 1, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "=") == 0) {
    if (argc != 2) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_VARIABLE_SET, parser, arg, 2, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "ce") == 0) {
    if (argc < 1 || argc > 4) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_CONTROL_EVENT, parser, arg, argc, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "VS") == 0) {
    if (argc < 0 || argc > 2) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_WAVE_RANGE_SET, parser, arg, argc, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "VL") == 0) {
    if (argc < 0 || argc > 2) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_WAVE_LOOP_SET, parser, arg, argc, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "pn") == 0) {
    if (argc < 4 || argc > 5) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_POLY_NOTE, parser, arg, argc, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "pr") == 0) {
    if (argc < 2 || argc > 3) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_POLY_RELEASE, parser, arg, argc, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "pb") == 0) {
    if (argc < 3 || argc > 4) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_POLY_BEND, parser, arg, argc, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "y") == 0 || strcmp(name, "pattern_select") == 0) {
    if (argc != 1) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_PATTERN_SET, parser, arg, 1, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "z") == 0 || strcmp(name, "pattern_state") == 0) {
    if (argc != 1) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_PATTERN_STATE, parser, arg, 1, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "zm") == 0 || strcmp(name, "ym") == 0 || strcmp(name, "pattern_mute") == 0) {
    if (argc != 1) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_PATTERN_MUTE, parser, arg, 1, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "zq") == 0 || strcmp(name, "pattern_queue") == 0) {
    if (argc != 1) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_PATTERN_QUEUE, parser, arg, 1, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "zg") == 0 || strcmp(name, "-j") == 0 || strcmp(name, "pattern_goto") == 0) {
    if (argc != 1) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_PATTERN_GOTO, parser, arg, 1, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "Z") == 0 || strcmp(name, "pattern_state_all") == 0) {
    if (argc != 1) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_PATTERN_STATE_ALL, parser, arg, 1, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "Z!") == 0 || strcmp(name, "pattern_rewind") == 0) {
    if (argc != 0) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_PATTERN_REWIND, parser, NULL, 0, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "z%") == 0 || strcmp(name, "%") == 0 || strcmp(name, "pattern_modulo") == 0) {
    if (argc != 1) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_PATTERN_MODULO, parser, arg, 1, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "z*") == 0 || strcmp(name, "*") == 0 || strcmp(name, "pattern_ratchet") == 0) {
    if (argc != 1) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_RATCHET, parser, arg, 1, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "/SP") == 0 || strcmp(name, "SP") == 0 || strcmp(name, "stream_pos") == 0) {
    if (argc != 2) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_STREAM_POS, parser, arg, 2, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "/SC") == 0 || strcmp(name, "SC") == 0 || strcmp(name, "stream_copy") == 0) {
    if (argc != 2) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_STREAM_COPY, parser, arg, 2, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  if (strcmp(name, "/SM") == 0 || strcmp(name, "SM") == 0 || strcmp(name, "stream_mode") == 0) {
    if (argc != 2) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    int rc = skode_program_push(program, SKODE_OP_STREAM_MODE, parser, arg, 2, 0, 0);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }

  const skode_def_word_t *gw = skode_dict_find(name);
  if (gw) {
    unsigned var_mask = 0;
    if (parser) {
      for (int i = 0; i < argc && i < SEQ_OPCODE_ARG_MAX; i++) {
        if (ands_arg_var(parser, i) >= 0) var_mask |= (1U << i);
      }
    }
    const skode_clause_t *c = skode_clause_match_ex(gw, argc, arg, var_mask);
    if (!c) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_INVALID;
      return 1;
    }
    if (!(c->flags & SKODE_CLAUSE_RT_SAFE)) {
      if (out_compile_result) *out_compile_result = SKODE_COMPILE_IMMEDIATE_ONLY;
      return 1;
    }
    uint8_t default_mask = 0;
    for (int i = 0; i < argc && i < SEQ_OPCODE_ARG_MAX; i++) {
      if (isnan(arg[i])) {
        default_mask |= (uint8_t)(1U << i);
      }
    }
    int rc = skode_program_push(program, (skode_opcode_t)c->opcode_id, parser,
      arg, argc, (char)default_mask, default_mask);
    if (out_compile_result)
      *out_compile_result = (rc == 0) ? SKODE_COMPILE_OK : SKODE_COMPILE_TOO_LARGE;
    return 1;
  }
  return 0;
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


extern void skode_stream_set(void *ctx, int n, const double *data, int len);
extern void skode_stream_copy(void *ctx, int dst, int src);
extern void seq_state_set(int p, int state);
extern void seq_step_goto(int pattern, int step);
extern void skode_stream_mode(void *ctx, int n, int mode);
extern void skode_stream_pos(void *ctx, int n, int pos);

int skode_dict_show_words(skode_t *ctx, int mode) {
  if (mode != 0) {
    char categories[32][96];
    int cat_count = skode_help_categories(categories, 32);
    
    // Second pass: print words by category
    for (int c = 0; c < cat_count; c++) {
      int has_words = 0;
      for (size_t i = 0; i < g_skode_words_count; i++) {
        const skode_def_word_t *w = &g_skode_words[i];
        const char *cat = skode_group_name(w->group);
        if (!cat) cat = "uncategorized";
        if (strcmp(cat, categories[c]) != 0) continue;
        
        int show = 1;
        int is_realtime = (w->clauses && (w->clauses[0].flags & SKODE_CLAUSE_RT_SAFE));
        if (mode == 1 && !is_realtime) show = 0;
        if (mode == 2 && is_realtime) show = 0;
        if (show) { has_words = 1; break; }
      }
      
      if (!has_words) continue;
      
      ctx->printf(ctx, "# %d %s\n", c + 1, categories[c]);
      for (size_t i = 0; i < g_skode_words_count; i++) {
        const skode_def_word_t *w = &g_skode_words[i];
        const char *cat = skode_group_name(w->group);
        if (!cat) cat = "uncategorized";
        
        int show = 1;
        int is_realtime = (w->clauses && (w->clauses[0].flags & SKODE_CLAUSE_RT_SAFE));
        if (mode == 1 && !is_realtime) show = 0;
        if (mode == 2 && is_realtime) show = 0;
        if (show && w->name && strcmp(cat, categories[c]) == 0) {
          ctx->printf(ctx, "%s ", w->name);
        }
      }
      ctx->printf(ctx, "\n");
    }
  } else {
    // Flat print
    for (size_t i = 0; i < g_skode_words_count; i++) {
      const skode_def_word_t *w = &g_skode_words[i];
      if (w->name) {
        ctx->printf(ctx, "%s ", w->name);
      }
    }
    
    if (ctx->parse) {
      int mcount = ands_macro_count(ctx->parse);
      for (int i = 0; i < mcount; i++) {
        char mname[16];
        if (ands_macro_get(ctx->parse, i, mname, sizeof(mname), NULL, 0, NULL)) {
          ctx->printf(ctx, "[%s] ", mname);
        }
      }
    }
    ctx->printf(ctx, "\n");
  }
  return 0;
}

int skode_stream_show_helper(skode_t *ctx, int stream_idx) {
  int start_idx = 0;
  int end_idx = 127;
  
  if (stream_idx >= 0) {
    start_idx = stream_idx;
    end_idx = stream_idx;
  }
  
  for (int i = start_idx; i <= end_idx; i++) {
    if (i >= 0 && i < 128) {
      skode_stream_t *stream = &global_stream[i];
      if (stream->len > 0) {
        ctx->printf(ctx, "( ");
        for (int j = 0; j < stream->len; j++) {
          if (ands_is_tie(stream->data[j])) ctx->printf(ctx, ". "); else if (isnan(stream->data[j])) ctx->printf(ctx, "- "); else ctx->printf(ctx, "%g ", stream->data[j]);
        }
        ctx->printf(ctx, ") /SS %d ", i);
        if (stream->mode != 0) {
          ctx->printf(ctx, "/SM %d %d ", i, stream->mode);
        }
        if (stream->pos != 0) {
          ctx->printf(ctx, "/SP %d %d ", i, stream->pos);
        }
        ctx->printf(ctx, "# len=%d\n", stream->len);
      } else if (stream_idx >= 0) {
         ctx->printf(ctx, "# stream %d is empty\n", i);
      }
    }
  }
  return 0;
}

/* Forward declaration -- defined in the macro-promotion section below;
   skode_dict_init()'s startup self-check needs it. */
static uint32_t pack_atom_runtime(const char *name, int len);

void skode_dict_init(void) {
  if (dict_initialized) return;
  memset(&global_vocab, 0, sizeof(global_vocab));
  dict_initialized = 1;
  void skode_register_immediate_words(skode_vocab_t*);
  skode_register_immediate_words(&global_vocab);

  /* pack_atom_runtime() (used by skode_dict_promote_macro() to turn a
     runtime macro-name string into the same 32-bit encoding ATOM4()
     produces for compile-time literals) and SKODE_DICT_ATOM() both depend on
     multichar-literal byte order, which is implementation-defined.
     Verified empirically on the reference toolchain (MSB-first) rather
     than assumed. */
  if (pack_atom_runtime("wait", 4) != UINT32_C(0x77616974) ||
      SKODE_DICT_ATOM("wait") != UINT32_C(0x77616974)) {
    fprintf(stderr, "skode-dict: atom-packing mismatch on this build -- "
      "pack_atom_runtime()=%08x SKODE_DICT_ATOM()=%08x ATOM4()=%08x (all three "
      "should be equal).\n",
      (unsigned)pack_atom_runtime("wait", 4), (unsigned)SKODE_DICT_ATOM("wait"),
      (unsigned)UINT32_C(0x77616974));
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
  .atom = SKODE_DICT_ATOM("m"), .name = "m", .execute = word_exec_m_logging, .opcode_id = SKODE_OP_MUTE,
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

const skode_word_t *skode_dict_find_by_name(skode_vocab_t *vocab, const char *name) {
  return skode_dict_lookup(vocab, pack_atom_runtime(name, (int)strlen(name)));
}

int skode_dict_unpromote_macro(skode_vocab_t *vocab, const char *name) {
  if (!vocab || !name || !name[0]) return 0;
  uint32_t atom = pack_atom_runtime(name, (int)strlen(name));
  skode_word_t *current = (skode_word_t *)skode_vocab_find(vocab, atom);
  if (!skode_dict_word_is_promoted_macro(current)) return 0;
  skode_word_t *removed = current->shadow ?
    skode_dict_revert(vocab, atom) : skode_dict_remove(vocab, atom);
  skode_dict_free_promoted_macro(removed);
  return 1;
}

skode_word_t *skode_dict_promote_macro(skode_vocab_t *vocab,
    const char *name, const char *body) {
  if (!vocab) { printf("v\n"); return NULL; } if (!name) { printf("n\n"); return NULL; } if (!name[0]) { printf("n0\n"); return NULL; } if (!body) { printf("b\n"); return NULL; } if (!body[0]) { printf("b0\n"); return NULL; }

  skode_dict_unpromote_macro(vocab, name);

  char tagged[ANDS_MACRO_BODY_LEN];
  int max_param = -1;
  if (!resolve_macro_placeholders_tagged(body, tagged, sizeof(tagged),
      &max_param)) {
    printf("Failed tags\n");
    return NULL; /* uses more than SKODE_DICT_MACRO_PARAM_MAX parameters */
  }

  event_program_t compiled;
  skode_compile_result_t result =
    skode_compile_program_ex(tagged, &compiled, vocab);
  if (result != SKODE_COMPILE_OK) {
    printf("Failed compile! result=%d\n", result);
    /* Not fully, cleanly compilable -- exactly the condition
       compute_macro_safety() would report as something other than
       WORD_REAL_TIME_SAFE. Refuse rather than partially promote. */
    return NULL;
  }

  skode_macro_template_t *tmpl = malloc(sizeof(*tmpl));
  if (!tmpl) { printf("Failed alloc\n"); return NULL; }
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
