#include "skode.h"

#include <math.h>
#include <string.h>

#include "seq.h"
#include "synth-config.h"
#include "synth-state.h"
#include "synth.h"
#include "control-events.h"
#include "skode-dict.h"

_Static_assert(SKODE_OP_FREQ_FEEDBACK <= UINT16_MAX,
  "Skode opcodes must fit in opcode_event_t.code");
_Static_assert(SEQ_PROGRAM_OP_MAX <= UINT8_MAX,
  "Compiled program length must fit in event_program_t.count");

const char *skode_opcode_name(uint16_t opcode) {
  static const char *const names[] = {
    "NONE", "DELAY", "VOICE", "AMP", "FREQ", "MIDI_NOTE", "PAN",
    "VELOCITY", "ENVELOPE_VELOCITY", "AMP_MOD", "WAVE_DIRECTION",
    "WAVE_LOOP", "PHASE_DISTORTION", "PHASE_MOD", "FILTER_ENVELOPE",
    "FILTER_ENVELOPE_DEPTH", "FREQ_MOD", "FREQ_MOD_MODE", "GLISSANDO",
    "LINK_MIDI", "SAMPLE_HOLD", "LINK_VELOCITY", "TRIGGER_DELAY",
    "FILTER_MODE", "FILTER_FREQ", "ENVELOPE_MODE", "MUTE",
    "MIDI_DETUNE", "PAN_MOD", "QUANTIZE", "FILTER_RESONANCE",
    "RECORD_TRACK", "SMOOTHER", "VOICE_RESET", "ENVELOPE", "TRIGGER",
    "LINK_SYNC", "WAVE", "VOICE_COPY", "WAVE_DEFAULT", "VARIABLE_SET",
    "RING_MOD", "WAVE_LOOP_COUNT", "CONTROL_EVENT", "DELAY_PARAMS",
    "WAVE_RANGE_SET", "WAVE_LOOP_SET", "POLY_NOTE", "POLY_RELEASE",
    "POLY_BEND", "PHASE_ENVELOPE", "PHASE_ENVELOPE_DEPTH",
    "FREQ_FEEDBACK", "FREQ_BEND", "FREQ_BEND_PARAM",
    "AMP_BEND", "AMP_BEND_PARAM", "PATTERN_MODULO",
    "PATTERN_STATE", "PATTERN_LOOP", "PATTERN_SET",
    "PATTERN_MUTE", "PATTERN_QUEUE", "PATTERN_GOTO",
    "PATTERN_STATE_ALL", "PATTERN_REWIND",
    "STREAM_COPY", "STREAM_MODE", "STREAM_POS", "RATCHET",
    "DELAY_DAMPING", "DELAY_FREEZE", "DELAY_PINGPONG", "DELAY_TIME",
    "DELAY_SYNC", "DELAY_GRIT", "TEMPO", "INST_NOTE",
  };
  _Static_assert(sizeof(names) / sizeof(names[0]) == SKODE_OP_INST_NOTE + 1,
    "skode_opcode_name array size must match opcode enum count");
  if (opcode < sizeof(names) / sizeof(names[0])) return names[opcode];
  return skode_discrete_opcode_name(opcode);
}

typedef struct {
  event_program_t *program;
  skode_compile_result_t result;
  int depth;
  uint64_t *macro_active;
  int accepts_empty;
  skode_vocab_t *private_vocab;
  char offending_name[32];
} skode_compile_t;

static int event_voice_valid(int voice) {
  return voice >= 0 && voice < synth_config.voice_max;
}

int skode_program_push_ex(event_program_t *program, skode_opcode_t code,
    ands_t *parser, const double *arg, int argc, int arg_offset, char mode,
    uint8_t default_mask) {
  if (program->count >= SEQ_PROGRAM_OP_MAX) return -1;
  if (!skode_opcode_is_realtime(code)) return -1;
  program_op_t *op = &program->op[program->count];
  memset(op, 0, sizeof(*op));
  op->opcode.code = code;
  op->opcode.argc = (uint8_t)argc;
  op->opcode.mode = mode;
  for (int i = 0; i < argc; i++) {
    int variable = parser ? ands_arg_var(parser, i + arg_offset) : -1;
    if (variable >= 0) {
      if (variable & ANDS_STREAM_FLAG) {
        op->opcode.stream_mask |= (uint8_t)(1U << i);
        op->opcode.arg[i] = (float)(variable & ~ANDS_STREAM_FLAG);
      } else {
        op->opcode.var_mask |= (uint8_t)(1U << i);
        op->opcode.arg[i] = (float)variable;
      }
    } else {
      op->opcode.arg[i] = skode_double_to_float_note(arg[i]);
      if (!isfinite(op->opcode.arg[i]) &&
          !(isnan(op->opcode.arg[i]) && (default_mask & (1U << i)))) {
        return -1;
      }
    }
  }
  if (code != SKODE_OP_DELAY) op->opcode.mode = (char)default_mask;
  program->count++;
  return 0;
}

int skode_program_push(event_program_t *program, skode_opcode_t code,
    ands_t *parser, const double *arg, int argc, char mode,
    uint8_t default_mask) {
  return skode_program_push_ex(program, code, parser, arg, argc, 0, mode, default_mask);
}

static skode_compile_result_t compile_program_inner(const char *text,
  event_program_t *program, int depth, uint64_t *macro_active,
  skode_vocab_t *vocab, char *offending_name_out, size_t offending_name_size);

static int program_append(event_program_t *program,
    const event_program_t *nested) {
  if (!program || !nested ||
      nested->count > SEQ_PROGRAM_OP_MAX - program->count) {
    return -1;
  }
  for (int i = 0; i < nested->count; i++)
    program->op[program->count++] = nested->op[i];
  return 0;
}

static int skode_compile_callback(ands_t *s, int info) {
  skode_compile_t *compile = ands_user(s);
  if (compile->result != SKODE_COMPILE_OK) return 0;

  if (info == GOT_STRING || info == GOT_ARRAY || info == GOT_RETURN_REF) {
    compile->result = SKODE_COMPILE_IMMEDIATE_ONLY;
    const char *name = ands_atom_name(s);
    if (name) snprintf(compile->offending_name, sizeof(compile->offending_name), "%s", name);
    return 0;
  }
  if (info == DEFER) {
    double delay = ands_defer_num(s);
    if (!isfinite(delay) || compile->depth >= SKODE_COMPILE_DEPTH_MAX) {
      compile->result = SKODE_COMPILE_INVALID;
      return 0;
    }
    double delay_arg[] = {delay};
    if (skode_program_push(compile->program, SKODE_OP_DELAY, NULL, delay_arg, 1,
        ands_defer_mode(s), 0) != 0) {
      compile->result = SKODE_COMPILE_TOO_LARGE;
      return 0;
    }
    int delay_var = ands_defer_var(s);
    if (delay_var >= 0) {
      program_op_t *delay_op =
        &compile->program->op[compile->program->count - 1];
      if (delay_var & ANDS_STREAM_FLAG) {
        delay_op->opcode.stream_mask = 1;
        delay_op->opcode.arg[0] = (float)(delay_var & ~ANDS_STREAM_FLAG);
      } else {
        delay_op->opcode.var_mask = 1;
        delay_op->opcode.arg[0] = (float)delay_var;
      }
    }
    event_program_t nested = {0};
    compile->result = compile_program_inner(ands_defer_string(s), &nested,
      compile->depth + 1, compile->macro_active, compile->private_vocab,
      compile->offending_name, sizeof(compile->offending_name));
    if (compile->result != SKODE_COMPILE_OK) return 0;
    if (program_append(compile->program, &nested) != 0)
      compile->result = SKODE_COMPILE_TOO_LARGE;
    return 0;
  }
  if (info != FUNCTION) return 0;

  uint32_t atom = ands_atom_num(s);
  const char *name = ands_atom_name(s);
  int argc = ands_arg_len(s);
  double *arg = ands_arg(s);

  if (name && strcmp(name, "e!") == 0) {
    if (argc != 1 || ands_arg_var(s, 0) >= 0) {
      compile->result = SKODE_COMPILE_IMMEDIATE_ONLY;
      snprintf(compile->offending_name, sizeof(compile->offending_name), "e!");
      return 0;
    }
    if (!isfinite(arg[0]) || floor(arg[0]) != arg[0] || arg[0] < 0 ||
        arg[0] >= SKODE_EXTRA_MAX) {
      compile->result = SKODE_COMPILE_INVALID;
      return 0;
    }
    int index = (int)arg[0];
    char macro[STEP_MAX];
    if (skode_extra_copy(index, macro, sizeof(macro)) != 0 ||
        macro[0] == '\0') {
      compile->result = SKODE_COMPILE_INVALID;
      return 0;
    }
    uint64_t bit = UINT64_C(1) << (index % 64);
    uint64_t *active = &compile->macro_active[index / 64];
    if (*active & bit) {
      compile->result = SKODE_COMPILE_INVALID;
      return 0;
    }
    *active |= bit;
    event_program_t nested = {0};
    compile->result = compile_program_inner(macro, &nested,
      compile->depth + 1, compile->macro_active, compile->private_vocab,
      compile->offending_name, sizeof(compile->offending_name));
    *active &= ~bit;
    if (compile->result == SKODE_COMPILE_OK &&
        program_append(compile->program, &nested) != 0) {
      compile->result = SKODE_COMPILE_TOO_LARGE;
    }
    if (compile->result == SKODE_COMPILE_OK && nested.count == 0)
      compile->accepts_empty = 1;
    return 0;
  }

  int dict_compile_result = 0;
  if (skode_compile_word(s, atom, arg, argc, compile->program,
      compile->private_vocab, &dict_compile_result)) {
    compile->result = (skode_compile_result_t)dict_compile_result;
    if (compile->result == SKODE_COMPILE_IMMEDIATE_ONLY) {
      if (name) snprintf(compile->offending_name, sizeof(compile->offending_name), "%s", name);
    }
    return 0;
  }

  compile->result = SKODE_COMPILE_IMMEDIATE_ONLY;
  if (name) snprintf(compile->offending_name, sizeof(compile->offending_name), "%s", name);
  return 0;
}

static skode_compile_result_t compile_program_inner(const char *text,
    event_program_t *program, int depth, uint64_t *macro_active,
    skode_vocab_t *vocab, char *offending_name_out, size_t offending_name_size) {
  if (!text || !program || depth > SKODE_COMPILE_DEPTH_MAX)
    return SKODE_COMPILE_INVALID;
  size_t len = strnlen(text, STEP_MAX);
  if (len == 0 || len >= STEP_MAX)
    return SKODE_COMPILE_INVALID;

  char input[STEP_MAX];
  memcpy(input, text, len + 1);
  skode_compile_t compile = {
    .program = program,
    .result = SKODE_COMPILE_OK,
    .depth = depth,
    .macro_active = macro_active,
    .private_vocab = vocab,
  };
  ands_t *parser = ands_new(skode_compile_callback, &compile);
  if (!parser) return SKODE_COMPILE_INVALID;
  ands_set_global(parser, global_var);
  ands_set_stream_pull(parser, skode_stream_pull);
  int result = ands_consume(parser, input);
  ands_free(parser);
  if (result != 0) return SKODE_COMPILE_INVALID;
  if (compile.result == SKODE_COMPILE_OK && program->count == 0 &&
      strchr(text, '#') == NULL && !compile.accepts_empty)
    return SKODE_COMPILE_INVALID;
  if (offending_name_out && offending_name_size > 0) {
    offending_name_out[0] = '\0';
    if (compile.offending_name[0]) {
      snprintf(offending_name_out, offending_name_size, "%s", compile.offending_name);
    }
  }
  return compile.result;
}

skode_compile_result_t skode_compile_program(const char *text,
    event_program_t *program) {
  return skode_compile_program_ex(text, program, NULL);
}

skode_compile_result_t skode_compile_program_ex(const char *text,
    event_program_t *program, skode_vocab_t *vocab) {
  if (!program) return SKODE_COMPILE_INVALID;
  memset(program, 0, sizeof(*program));
  uint64_t macro_active[(SKODE_EXTRA_MAX + 63) / 64] = {0};
  return compile_program_inner(text, program, 0, macro_active, vocab, NULL, 0);
}

/* Like skode_compile_program but, on SKODE_COMPILE_IMMEDIATE_ONLY, writes the
 * offending atom as a printable string into hint_buf (up to hint_size bytes).
 * Returns the same result code as skode_compile_program. */
skode_compile_result_t skode_compile_program_describe(const char *text,
    event_program_t *program, char *hint_buf, size_t hint_size) {
  if (!program) return SKODE_COMPILE_INVALID;
  memset(program, 0, sizeof(*program));
  if (hint_buf && hint_size > 0) hint_buf[0] = '\0';
  uint64_t macro_active[(SKODE_EXTRA_MAX + 63) / 64] = {0};
  return compile_program_inner(text, program, 0, macro_active, NULL, hint_buf, hint_size);
}

static uint8_t voice_is_rest[VOICE_MAX];

int skode_midi_note(int voice, float note, float cents) {
  if (!event_voice_valid(voice)) return -1;
  if (skode_float_is_tie(note)) {
    return 0;
  }
  if (isnan(note) || (note < 0.0f && note < -127.0f)) {
    voice_is_rest[voice] = 1;
    skode_linked_velocity(voice, 0.0f, SAMPLE_COUNT_GET());
    return 0;
  }
  voice_is_rest[voice] = 0;
  float pitch = note < 0.0f ? -note : note;
  int result = freq_midi(voice, pitch, cents);
  if (sv.link_midi_0[voice] >= 0) freq_midi(sv.link_midi_0[voice], pitch, cents);
  if (sv.link_midi_1[voice] >= 0) freq_midi(sv.link_midi_1[voice], pitch, cents);
  if (sv.link_midi_2[voice] >= 0) freq_midi(sv.link_midi_2[voice], pitch, cents);
  if (sv.link_midi_3[voice] >= 0) freq_midi(sv.link_midi_3[voice], pitch, cents);
  if (sv.link_midi_4[voice] >= 0) freq_midi(sv.link_midi_4[voice], pitch, cents);
  if (sv.link_midi_5[voice] >= 0) freq_midi(sv.link_midi_5[voice], pitch, cents);
  return result;
}

int skode_inst_note(int voice, float note, float velocity, float cents) {
  if (!event_voice_valid(voice)) return -1;
  if (skode_float_is_tie(note)) {
    return 0;
  }
  if (isnan(note) || note < -127.0f) {
    voice_is_rest[voice] = 1;
    skode_linked_velocity(voice, 0.0f, SAMPLE_COUNT_GET());
    return 0;
  }
  voice_is_rest[voice] = 0;
  float pitch = note < 0.0f ? -note : note;
  freq_midi(voice, pitch, cents);
  if (sv.link_midi_0[voice] >= 0) freq_midi(sv.link_midi_0[voice], pitch, cents);
  if (sv.link_midi_1[voice] >= 0) freq_midi(sv.link_midi_1[voice], pitch, cents);
  if (sv.link_midi_2[voice] >= 0) freq_midi(sv.link_midi_2[voice], pitch, cents);
  if (sv.link_midi_3[voice] >= 0) freq_midi(sv.link_midi_3[voice], pitch, cents);
  if (sv.link_midi_4[voice] >= 0) freq_midi(sv.link_midi_4[voice], pitch, cents);
  if (sv.link_midi_5[voice] >= 0) freq_midi(sv.link_midi_5[voice], pitch, cents);

  if (note >= 0.0f) {
    float vel = velocity > 0.0f ? velocity : 1.0f;
    skode_linked_velocity(voice, vel, SAMPLE_COUNT_GET());
  }
  return 0;
}

int skode_linked_velocity(int voice, float velocity, uint64_t sample) {
  if (!event_voice_valid(voice)) return -1;
  if (voice_is_rest[voice]) {
    velocity = 0.0f;
  }
  skode_envelope_velocity(voice, velocity, sample);
  if (sv.link_velo_0[voice] >= 0) skode_envelope_velocity(sv.link_velo_0[voice], velocity, sample);
  if (sv.link_velo_1[voice] >= 0) skode_envelope_velocity(sv.link_velo_1[voice], velocity, sample);
  if (sv.link_velo_2[voice] >= 0) skode_envelope_velocity(sv.link_velo_2[voice], velocity, sample);
  if (sv.link_velo_3[voice] >= 0) skode_envelope_velocity(sv.link_velo_3[voice], velocity, sample);
  if (sv.link_velo_4[voice] >= 0) skode_envelope_velocity(sv.link_velo_4[voice], velocity, sample);
  if (sv.link_velo_5[voice] >= 0) skode_envelope_velocity(sv.link_velo_5[voice], velocity, sample);
  return 0;
}

extern double skode_stream_pull(void *ctx, int n);

static int execute_opcode(const event_t *event, int voice) {
  const opcode_event_t *opcode = &event->opcode;
  if (!event || !opcode || !event_voice_valid(voice)) return -1;
  
  if (opcode->argc > SEQ_OPCODE_ARG_MAX) return -1;
  opcode_event_t resolved = *opcode;
  for (int i = 0; i < opcode->argc; i++) {
    if (opcode->stream_mask & (1U << i)) {
      int stream_idx = (int)opcode->arg[i];
      if (stream_idx >= 0 && stream_idx < ANDS_VAR_MAX) {
        resolved.arg[i] = skode_double_to_float_note(skode_stream_pull(NULL, stream_idx));
      }
    } else if (opcode->var_mask & (1U << i)) {
      int variable = (int)opcode->arg[i];
      if (variable < 0 || variable >= ANDS_VAR_MAX) return -1;
      resolved.arg[i] = skode_double_to_float_note(global_var[variable]);
    }
    if (!isfinite(resolved.arg[i]) &&
        !(isnan(resolved.arg[i]) &&
          ((uint8_t)opcode->mode & (1U << i)))) {
      return -1;
    }
  }
  resolved.var_mask = 0;
  resolved.stream_mask = 0;
  
  int p = event->source_valid ? event->pattern : seq_current_pattern;
  
  switch (resolved.code) {
    case SKODE_OP_PATTERN_SET:
      if (resolved.argc >= 1 && resolved.arg[0] >= 0 && resolved.arg[0] < PATTERNS_MAX) {
        seq_current_pattern = (int)resolved.arg[0];
        skred_control_pattern_event(SKRED_CONTROL_EVENT_PATTERN_CHANGE, SAMPLE_COUNT_GET(), seq_current_pattern, 0);
      }
      return 0;
    case SKODE_OP_PATTERN_STATE:
      if (resolved.argc >= 1 && p >= 0 && p < PATTERNS_MAX) {
        seq_state_set_locked(p, (int)resolved.arg[0]);
      }
      return 0;
    case SKODE_OP_PATTERN_MUTE:
      if (resolved.argc >= 1 && p >= 0 && p < PATTERNS_MAX) {
        seq_mute[p] = (int)resolved.arg[0];
        skred_control_pattern_event(SKRED_CONTROL_EVENT_MUTE_CHANGE, SAMPLE_COUNT_GET(), p, (int)resolved.arg[0]);
      }
      return 0;
    case SKODE_OP_PATTERN_QUEUE:
      if (resolved.argc >= 1 && p >= 0 && p < PATTERNS_MAX) {
        int state = (int)resolved.arg[0];
        seq_pending_state[p] = state == 1 ? 1 : (state == 0 ? 2 : 0);
        skred_control_pattern_event(SKRED_CONTROL_EVENT_PATTERN_QUEUE, SAMPLE_COUNT_GET(), p, state);
      }
      return 0;
    case SKODE_OP_PATTERN_GOTO:
      if (resolved.argc >= 1 && resolved.arg[0] >= 0 && 1 && p >= 0 && p < PATTERNS_MAX) {
        seq_step_goto_locked(p, (int)resolved.arg[0]);
      }
      return 0;
    case SKODE_OP_PATTERN_REWIND:
      seq_rewind();
      return 0;
    case SKODE_OP_PATTERN_STATE_ALL:
      if (resolved.argc >= 1) {
        for (int i = 0; i < PATTERNS_MAX; i++) {
          seq_state_set_locked(i, (int)resolved.arg[0]);
        }
      }
      return 0;
    default:
      return skode_execute_voice_opcode(&resolved, voice);
  }
}

int skode_emit_control_event_opcode(const opcode_event_t *opcode, int voice,
    int pattern, int step, int tag) {
  if (!opcode || !event_voice_valid(voice) ||
      opcode->code != SKODE_OP_CONTROL_EVENT ||
      opcode->argc < 1 || opcode->argc > 4) return -1;
  double arg[4] = {0};
  for (int i = 0; i < opcode->argc; i++) {
    if (opcode->stream_mask & (1U << i)) {
      int stream_idx = (int)opcode->arg[i];
      if (stream_idx >= 0 && stream_idx < ANDS_VAR_MAX) {
        arg[i] = skode_stream_pull(NULL, stream_idx);
      }
    } else if (opcode->var_mask & (1U << i)) {
      int variable = (int)opcode->arg[i];
      if (variable < 0 || variable >= ANDS_VAR_MAX) return -1;
      arg[i] = global_var[variable];
    } else {
      arg[i] = opcode->arg[i];
    }
    if (!isfinite(arg[i])) return -1;
  }
  if (floor(arg[0]) != arg[0] || arg[0] < INT32_MIN || arg[0] > INT32_MAX)
    return -1;
  skred_control_user_event(SAMPLE_COUNT_GET(), voice, pattern, step, tag,
    SKODE_OP_CONTROL_EVENT, (int)arg[0], opcode->argc - 1, &arg[1]);
  return 0;
}

int skode_execute_event(const event_t *event, skode_t *ctx) {
  (void)ctx;
  if (!event) return -1;
  int voice = event->voice;
  if (event->voice_var) {
    int variable = event->voice_var - 1;
    if (variable < 0 || variable >= ANDS_VAR_MAX) return -1;
    double value = global_var[variable];
    if (!isfinite(value) || value < 0.0 ||
        value >= synth_config.voice_max || floor(value) != value) {
      return -1;
    }
    voice = (int)value;
  }
  if (event->source_valid)
    skred_control_voice_source(voice, event->pattern, event->step, event->tag,
      event->opcode.code);
  else
    skred_control_voice_source(voice, -1, -1, -1, event->opcode.code);
  if (event->opcode.code == SKODE_OP_CONTROL_EVENT) {
    return skode_emit_control_event_opcode(&event->opcode, voice,
      event->source_valid ? event->pattern : -1,
      event->source_valid ? event->step : -1,
      event->source_valid ? event->tag : -1);
  }
  return execute_opcode(event, voice);
}

static int delay_to_samples(char mode, double delay, uint64_t *samples) {
  if (!samples || !isfinite(delay)) return -1;
  if (mode != '+' && mode != '~') return -1;
  if (delay < 0.0) {
    delay = -delay * tempo_step_seconds_get();
  } else if (mode == '+') {
    delay *= tempo_step_seconds_get() * 4.0;
  }
  long double value = (long double)delay * (long double)MAIN_SAMPLE_RATE;
  if (value > (long double)UINT64_MAX) return -1;
  *samples = (uint64_t)value;
  return 0;
}

static int resolve_program_arg(const opcode_event_t *opcode, int n,
    double *value) {
  if (!opcode || !value || n < 0 || n >= opcode->argc) return -1;
  if (opcode->stream_mask & (1U << n)) {
    int stream_idx = (int)opcode->arg[n];
    if (stream_idx >= 0 && stream_idx < ANDS_VAR_MAX) {
      *value = skode_stream_pull(NULL, stream_idx);
    }
  } else if (opcode->var_mask & (1U << n)) {
    int variable = (int)opcode->arg[n];
    if (variable < 0 || variable >= ANDS_VAR_MAX) return -1;
    *value = global_var[variable];
  } else {
    *value = opcode->arg[n];
  }
  return isfinite(*value) ? 0 : -1;
}

static int run_program(const event_program_t *program, int voice,
    uint64_t base, uint64_t now, int tag, int execute_due,
    int *final_voice, int pattern, int step) {
  if (!program || !event_voice_valid(voice)) return -1;
  if (program->count > SEQ_PROGRAM_OP_MAX) return -1;

  int ratchet_count = 1;
  for (int i = 0; i < program->count; i++) {
    if (program->op[i].opcode.code == SKODE_OP_RATCHET) {
      double r_val;
      if (resolve_program_arg(&program->op[i].opcode, 0, &r_val) == 0 && r_val > 1.0) {
        ratchet_count = (int)r_val;
        if (ratchet_count > 16) ratchet_count = 16;
      }
    }
  }

  uint64_t step_samples = 5512;
  double samples_per_step = (double)tempo_step_seconds_get() * (double)MAIN_SAMPLE_RATE;
  if (samples_per_step > 0.0) {
    step_samples = (uint64_t)(samples_per_step + 0.5);
  }
  uint64_t sub_samples = step_samples / (uint64_t)ratchet_count;
  if (sub_samples == 0) sub_samples = 1;

  for (int r = 0; r < ratchet_count; r++) {
    uint64_t when = base + (uint64_t)r * sub_samples;
    int current_voice = voice;
    uint8_t current_voice_var = 0;
    int r_exec_due = (r == 0) ? execute_due : 0;
    memset(voice_is_rest, 0, sizeof(voice_is_rest));

    for (int i = 0; i < program->count; i++) {
      const program_op_t *op = &program->op[i];
      if (op->opcode.code == SKODE_OP_DELAY) {
        uint64_t relative;
        double delay;
        if (op->opcode.argc != 1 ||
            resolve_program_arg(&op->opcode, 0, &delay) != 0)
          return -1;
        if (delay_to_samples(op->opcode.mode, delay, &relative) != 0)
          return -1;
        when = relative > UINT64_MAX - when ? UINT64_MAX : when + relative;
        continue;
      }
      if (op->opcode.code == SKODE_OP_VOICE) {
        if (op->opcode.argc != 1) return -1;
        if (op->opcode.var_mask & 1U) {
          int variable = (int)op->opcode.arg[0];
          if (variable < 0 || variable >= ANDS_VAR_MAX) return -1;
          current_voice_var = (uint8_t)(variable + 1);
          continue;
        }
        if (op->opcode.arg[0] < 0.0 ||
            op->opcode.arg[0] >= synth_config.voice_max ||
            floor(op->opcode.arg[0]) != op->opcode.arg[0]) {
          return -1;
        }
        current_voice = (int)op->opcode.arg[0];
        current_voice_var = 0;
        continue;
      }
      if (op->opcode.code == SKODE_OP_PATTERN_MODULO) {
        if (r == 0) {
          double mod;
          if (resolve_program_arg(&op->opcode, 0, &mod) == 0 && mod > 0.0) {
            seq_modulo_set_locked(pattern >= 0 ? pattern : 0, (int)mod);
          }
        }
        continue;
      }
      if (op->opcode.code == SKODE_OP_RATCHET) {
        continue;
      }
      if (op->opcode.code == SKODE_OP_PATTERN_SET) {
        double new_p;
        if (resolve_program_arg(&op->opcode, 0, &new_p) == 0 && new_p >= 0.0) {
          pattern = (int)new_p;
        }
      }

      event_t event = {
        .voice = current_voice,
        .voice_var = current_voice_var,
        .source_valid = (pattern >= 0 || step >= 0 || tag >= 0),
        .pattern = pattern,
        .step = step,
        .tag = tag,
        .opcode = op->opcode,
      };
      if (r_exec_due && when <= now) {
        if (skode_execute_event(&event, NULL) != 0) return -1;
      } else if (queue_event(when, &event, tag) != 0) {
        return -1;
      }
    }
    if (r == 0 && final_voice) {
      if (current_voice_var) {
        int variable = current_voice_var - 1;
        double value = global_var[variable];
        if (!isfinite(value) || value < 0.0 ||
            value >= synth_config.voice_max || floor(value) != value) {
          return -1;
        }
        *final_voice = (int)value;
      } else {
        *final_voice = current_voice;
      }
    }
  }
  return 0;
}

int skode_queue_program(const event_program_t *program, int voice,
    uint64_t when, int tag) {
  return run_program(program, voice, when, SAMPLE_COUNT_GET(), tag, 0, NULL,
    -1, -1);
}

int skode_execute_program(const event_program_t *program, int voice,
    uint64_t now, int tag) {
  return run_program(program, voice, now, now, tag, 1, NULL, -1, -1);
}

int skode_execute_program_state(const event_program_t *program, int *voice,
    uint64_t now, int tag, int pattern, int step) {
  if (!voice) return -1;
  return run_program(program, *voice, now, now, tag, 1, voice, pattern, step);
}

int skode_queue_program_deferred(const event_program_t *program, int voice,
    uint64_t base, char mode, double delay, int tag, int pattern, int step) {
  uint64_t relative;
  if (delay_to_samples(mode, delay, &relative) != 0) return -1;
  uint64_t when = relative > UINT64_MAX - base ? UINT64_MAX : base + relative;
  return run_program(program, voice, when, SAMPLE_COUNT_GET(), tag, 0, NULL,
    pattern, step);
}
