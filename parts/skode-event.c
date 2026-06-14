#include "skode.h"

#include <math.h>
#include <string.h>

#include "seq.h"
#include "synth-config.h"
#include "synth-state.h"
#include "synth.h"

#define SKODE_ATOM(a, b, c, d) \
  (((uint32_t)(uint8_t)(a) << 24) | ((uint32_t)(uint8_t)(b) << 16) | \
   ((uint32_t)(uint8_t)(c) << 8) | (uint32_t)(uint8_t)(d))

_Static_assert(SKODE_OP_WAVE_LOOP_COUNT <= UINT8_MAX,
  "Skode opcodes must fit in opcode_event_t.code");
_Static_assert(SEQ_PROGRAM_OP_MAX <= UINT8_MAX,
  "Compiled program length must fit in event_program_t.count");

const char *skode_opcode_name(uint8_t opcode) {
  static const char *const names[] = {
    "NONE", "DELAY", "VOICE", "AMP", "FREQ", "MIDI_NOTE", "PAN",
    "VELOCITY", "ENVELOPE_VELOCITY", "AMP_MOD", "WAVE_DIRECTION",
    "WAVE_LOOP", "PHASE_DISTORTION", "PHASE_MOD", "FILTER_ENVELOPE",
    "FILTER_ENVELOPE_DEPTH", "FREQ_MOD", "FREQ_MOD_MODE", "GLISSANDO",
    "LINK_MIDI", "SAMPLE_HOLD", "LINK_VELOCITY", "TRIGGER_DELAY",
    "FILTER_MODE", "FILTER_FREQ", "ENVELOPE_MODE", "MUTE",
    "MIDI_DETUNE", "PAN_MOD", "QUANTIZE", "FILTER_RESONANCE",
    "RECORD_TRACK", "SMOOTHER", "VOICE_RESET", "ENVELOPE", "TRIGGER",
    "WAVE", "VOICE_COPY", "WAVE_DEFAULT", "VARIABLE_SET", "RING_MOD",
    "WAVE_LOOP_COUNT",
  };
  return opcode < sizeof(names) / sizeof(names[0]) ?
    names[opcode] : "UNKNOWN";
}

typedef struct {
  event_program_t *program;
  skode_compile_result_t result;
  int depth;
  uint64_t *macro_active;
  int accepts_empty;
} skode_compile_t;

static int event_voice_valid(int voice) {
  return voice >= 0 && voice < synth_config.voice_max;
}

static int program_push(event_program_t *program, skode_opcode_t code,
    ands_t *parser, const double *arg, int argc, char mode,
    uint8_t default_mask) {
  if (program->count >= SEQ_PROGRAM_OP_MAX) return -1;
  program_op_t *op = &program->op[program->count];
  memset(op, 0, sizeof(*op));
  op->opcode.code = (uint8_t)code;
  op->opcode.argc = (uint8_t)argc;
  op->opcode.mode = mode;
  for (int i = 0; i < argc; i++) {
    int variable = parser ? ands_arg_var(parser, i) : -1;
    if (variable >= 0) {
      op->opcode.var_mask |= (uint8_t)(1U << i);
      op->opcode.arg[i] = (float)variable;
    } else {
      op->opcode.arg[i] = (float)arg[i];
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

static skode_compile_result_t compile_program_inner(const char *text,
  event_program_t *program, int depth, uint64_t *macro_active);

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

  if (info == GOT_STRING || info == GOT_ARRAY) {
    compile->result = SKODE_COMPILE_IMMEDIATE_ONLY;
    return 0;
  }
  if (info == DEFER) {
    double delay = ands_defer_num(s);
    if (!isfinite(delay) || compile->depth >= SKODE_COMPILE_DEPTH_MAX) {
      compile->result = SKODE_COMPILE_INVALID;
      return 0;
    }
    double delay_arg[] = {delay};
    if (program_push(compile->program, SKODE_OP_DELAY, NULL, delay_arg, 1,
        ands_defer_mode(s), 0) != 0) {
      compile->result = SKODE_COMPILE_TOO_LARGE;
      return 0;
    }
    int delay_var = ands_defer_var(s);
    if (delay_var >= 0) {
      program_op_t *delay_op =
        &compile->program->op[compile->program->count - 1];
      delay_op->opcode.var_mask = 1;
      delay_op->opcode.arg[0] = (float)delay_var;
    }
    event_program_t nested = {0};
    compile->result = compile_program_inner(ands_defer_string(s), &nested,
      compile->depth + 1, compile->macro_active);
    if (compile->result != SKODE_COMPILE_OK) return 0;
    if (program_append(compile->program, &nested) != 0)
      compile->result = SKODE_COMPILE_TOO_LARGE;
    return 0;
  }
  if (info != FUNCTION) return 0;

  uint32_t atom = ands_atom_num(s);
  int argc = ands_arg_len(s);
  double *arg = ands_arg(s);
  skode_opcode_t opcode = SKODE_OP_NONE;
  int min_argc = -1;
  int max_argc = -1;
  uint8_t default_mask = 0;

  if (atom == SKODE_ATOM('e', '!', '-', '-')) {
    if (argc != 1 || ands_arg_var(s, 0) >= 0) {
      compile->result = SKODE_COMPILE_IMMEDIATE_ONLY;
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
      compile->depth + 1, compile->macro_active);
    *active &= ~bit;
    if (compile->result == SKODE_COMPILE_OK &&
        program_append(compile->program, &nested) != 0) {
      compile->result = SKODE_COMPILE_TOO_LARGE;
    }
    if (compile->result == SKODE_COMPILE_OK && nested.count == 0)
      compile->accepts_empty = 1;
    return 0;
  }

  switch (atom) {
    case SKODE_ATOM('v', '-', '-', '-'):
      opcode = SKODE_OP_VOICE;
      min_argc = max_argc = 1;
      break;
    case SKODE_ATOM('a', '-', '-', '-'):
      opcode = SKODE_OP_AMP;
      min_argc = max_argc = 1;
      break;
    case SKODE_ATOM('f', '-', '-', '-'):
      opcode = SKODE_OP_FREQ;
      min_argc = max_argc = 1;
      break;
    case SKODE_ATOM('n', '-', '-', '-'):
      opcode = SKODE_OP_MIDI_NOTE;
      min_argc = 1;
      max_argc = 2;
      default_mask = 1;
      break;
    case SKODE_ATOM('p', '-', '-', '-'):
      opcode = SKODE_OP_PAN;
      min_argc = max_argc = 1;
      break;
    case SKODE_ATOM('l', '-', '-', '-'):
      opcode = SKODE_OP_VELOCITY;
      min_argc = max_argc = 1;
      break;
    case SKODE_ATOM('_', '_', '_', 'l'):
      opcode = SKODE_OP_ENVELOPE_VELOCITY;
      min_argc = max_argc = 1;
      break;
    case SKODE_ATOM('A', '-', '-', '-'):
      opcode = SKODE_OP_AMP_MOD; min_argc = 0; max_argc = 3; break;
    case SKODE_ATOM('b', '-', '-', '-'):
      opcode = SKODE_OP_WAVE_DIRECTION; min_argc = 0; max_argc = 1; break;
    case SKODE_ATOM('B', '-', '-', '-'):
      opcode = SKODE_OP_WAVE_LOOP; min_argc = 0; max_argc = 1; break;
    case SKODE_ATOM('B', 'C', '-', '-'):
      opcode = SKODE_OP_WAVE_LOOP_COUNT; min_argc = max_argc = 1; break;
    case SKODE_ATOM('c', '-', '-', '-'):
      opcode = SKODE_OP_PHASE_DISTORTION; min_argc = 0; max_argc = 2; break;
    case SKODE_ATOM('C', '-', '-', '-'):
      opcode = SKODE_OP_PHASE_MOD; min_argc = 0; max_argc = 2; break;
    case SKODE_ATOM('f', 't', '-', '-'):
      opcode = SKODE_OP_FILTER_ENVELOPE; min_argc = max_argc = 4; break;
    case SKODE_ATOM('f', 'd', '-', '-'):
      opcode = SKODE_OP_FILTER_ENVELOPE_DEPTH; min_argc = max_argc = 1; break;
    case SKODE_ATOM('F', '-', '-', '-'):
      opcode = SKODE_OP_FREQ_MOD; min_argc = 0; max_argc = 3; break;
    case SKODE_ATOM('F', 'F', '-', '-'):
      opcode = SKODE_OP_FREQ_MOD_MODE; min_argc = max_argc = 1; break;
    case SKODE_ATOM('g', '-', '-', '-'):
      opcode = SKODE_OP_GLISSANDO; min_argc = max_argc = 1; break;
    case SKODE_ATOM('G', '-', '-', '-'):
      opcode = SKODE_OP_LINK_MIDI; min_argc = 1; max_argc = 4; break;
    case SKODE_ATOM('h', '-', '-', '-'):
      opcode = SKODE_OP_SAMPLE_HOLD; min_argc = max_argc = 1; break;
    case SKODE_ATOM('H', '-', '-', '-'):
      opcode = SKODE_OP_LINK_VELOCITY; min_argc = 1; max_argc = 4; break;
    case SKODE_ATOM('L', '-', '-', '-'):
      opcode = SKODE_OP_TRIGGER_DELAY; min_argc = max_argc = 1; break;
    case SKODE_ATOM('J', '-', '-', '-'):
      opcode = SKODE_OP_FILTER_MODE; min_argc = max_argc = 1; break;
    case SKODE_ATOM('K', '-', '-', '-'):
      opcode = SKODE_OP_FILTER_FREQ; min_argc = max_argc = 1; break;
    case SKODE_ATOM('k', '-', '-', '-'):
      opcode = SKODE_OP_ENVELOPE_MODE; min_argc = max_argc = 1; break;
    case SKODE_ATOM('m', '-', '-', '-'):
      opcode = SKODE_OP_MUTE; min_argc = max_argc = 1; break;
    case SKODE_ATOM('N', '-', '-', '-'):
      opcode = SKODE_OP_MIDI_DETUNE; min_argc = 1; max_argc = 2;
      default_mask = 1; break;
    case SKODE_ATOM('P', '-', '-', '-'):
      opcode = SKODE_OP_PAN_MOD; min_argc = 0; max_argc = 3; break;
    case SKODE_ATOM('q', '-', '-', '-'):
      opcode = SKODE_OP_QUANTIZE; min_argc = max_argc = 1; break;
    case SKODE_ATOM('Q', '-', '-', '-'):
      opcode = SKODE_OP_FILTER_RESONANCE; min_argc = max_argc = 1; break;
    case SKODE_ATOM('r', '-', '-', '-'):
      opcode = SKODE_OP_RECORD_TRACK; min_argc = max_argc = 1; break;
    case SKODE_ATOM('s', '-', '-', '-'):
      opcode = SKODE_OP_SMOOTHER; min_argc = max_argc = 1; break;
    case SKODE_ATOM('S', '-', '-', '-'):
      opcode = SKODE_OP_VOICE_RESET; min_argc = max_argc = 1; break;
    case SKODE_ATOM('t', '-', '-', '-'):
      opcode = SKODE_OP_ENVELOPE; min_argc = max_argc = 4; break;
    case SKODE_ATOM('T', '-', '-', '-'):
      opcode = SKODE_OP_TRIGGER; min_argc = max_argc = 0; break;
    case SKODE_ATOM('w', '-', '-', '-'):
      opcode = SKODE_OP_WAVE; min_argc = 1; max_argc = 3; break;
    case SKODE_ATOM('>', '-', '-', '-'):
      opcode = SKODE_OP_VOICE_COPY; min_argc = max_argc = 1; break;
    case SKODE_ATOM('/', '-', '-', '-'):
      opcode = SKODE_OP_WAVE_DEFAULT; min_argc = max_argc = 0; break;
    case SKODE_ATOM('=', '-', '-', '-'):
      opcode = SKODE_OP_VARIABLE_SET; min_argc = max_argc = 2;
      break;
    case SKODE_ATOM('X', 'M', '-', '-'):
      opcode = SKODE_OP_RING_MOD; min_argc = 1; max_argc = 2;
      break;
    default:
      compile->result = SKODE_COMPILE_IMMEDIATE_ONLY;
      return 0;
  }

  if (!skode_opcode_supported(opcode)) {
    compile->result = SKODE_COMPILE_IMMEDIATE_ONLY;
    return 0;
  }
  if (argc < min_argc || argc > max_argc || argc > SEQ_OPCODE_ARG_MAX) {
    compile->result = SKODE_COMPILE_INVALID;
    return 0;
  }
  for (int i = 0; i < argc; i++) {
    if (ands_arg_var(s, i) < 0 && !isfinite(arg[i]) &&
        !(isnan(arg[i]) && (default_mask & (1U << i)))) {
      compile->result = SKODE_COMPILE_INVALID;
      return 0;
    }
  }
  if (program_push(compile->program, opcode, s, arg, argc, 0,
      default_mask) != 0)
    compile->result = SKODE_COMPILE_TOO_LARGE;
  return 0;
}

static skode_compile_result_t compile_program_inner(const char *text,
    event_program_t *program, int depth, uint64_t *macro_active) {
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
  };
  ands_t *parser = ands_new(skode_compile_callback, &compile);
  if (!parser) return SKODE_COMPILE_INVALID;
  int result = ands_consume(parser, input);
  ands_free(parser);
  if (result != 0) return SKODE_COMPILE_INVALID;
  if (compile.result == SKODE_COMPILE_OK && program->count == 0 &&
      strchr(text, '#') == NULL && !compile.accepts_empty)
    return SKODE_COMPILE_INVALID;
  return compile.result;
}

skode_compile_result_t skode_compile_program(const char *text,
    event_program_t *program) {
  if (!program) return SKODE_COMPILE_INVALID;
  memset(program, 0, sizeof(*program));
  uint64_t macro_active[(SKODE_EXTRA_MAX + 63) / 64] = {0};
  return compile_program_inner(text, program, 0, macro_active);
}

int skode_midi_note(int voice, float note, float cents) {
  if (!event_voice_valid(voice)) return -1;
  if (isnan(note)) note = sv.last_midi_note[voice];
  int result = freq_midi(voice, note, cents);
  if (sv.link_midi_0[voice] >= 0) freq_midi(sv.link_midi_0[voice], note, cents);
  if (sv.link_midi_1[voice] >= 0) freq_midi(sv.link_midi_1[voice], note, cents);
  if (sv.link_midi_2[voice] >= 0) freq_midi(sv.link_midi_2[voice], note, cents);
  if (sv.link_midi_3[voice] >= 0) freq_midi(sv.link_midi_3[voice], note, cents);
  return result;
}

static int execute_opcode(const opcode_event_t *opcode, int voice) {
  if (!opcode || !event_voice_valid(voice)) return -1;
  if (opcode->argc > SEQ_OPCODE_ARG_MAX) return -1;
  opcode_event_t resolved = *opcode;
  for (int i = 0; i < opcode->argc; i++) {
    if (opcode->var_mask & (1U << i)) {
      int variable = (int)opcode->arg[i];
      if (variable < 0 || variable >= ANDS_VAR_MAX) return -1;
      resolved.arg[i] = (float)global_var[variable];
    }
    if (!isfinite(resolved.arg[i]) &&
        !(isnan(resolved.arg[i]) &&
          ((uint8_t)opcode->mode & (1U << i)))) {
      return -1;
    }
  }
  resolved.var_mask = 0;
  return skode_execute_voice_opcode(&resolved, voice);
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
  return execute_opcode(&event->opcode, voice);
}

static int delay_to_samples(char mode, double delay, uint64_t *samples) {
  if (!samples || !isfinite(delay) || delay < 0.0) return -1;
  if (mode != '+' && mode != '~') return -1;
  if (mode == '+') delay *= tempo_step_seconds_get() * 4.0;
  long double value = (long double)delay * (long double)MAIN_SAMPLE_RATE;
  if (value > (long double)UINT64_MAX) return -1;
  *samples = (uint64_t)value;
  return 0;
}

static int resolve_program_arg(const opcode_event_t *opcode, int n,
    double *value) {
  if (!opcode || !value || n < 0 || n >= opcode->argc) return -1;
  if (opcode->var_mask & (1U << n)) {
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
    int *final_voice) {
  if (!program || !event_voice_valid(voice)) return -1;
  if (program->count > SEQ_PROGRAM_OP_MAX) return -1;
  uint64_t when = base;
  int current_voice = voice;
  uint8_t current_voice_var = 0;

  for (int i = 0; i < program->count; i++) {
    const program_op_t *op = &program->op[i];
    if (op->opcode.code == SKODE_OP_DELAY) {
      uint64_t relative;
      double delay;
      if (op->opcode.argc != 1 ||
          resolve_program_arg(&op->opcode, 0, &delay) != 0)
        return -1;
      if (delay < 0.0) delay = 0.0;
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

    event_t event = {
      .voice = current_voice,
      .voice_var = current_voice_var,
      .opcode = op->opcode,
    };
    if (execute_due && when <= now) {
      if (skode_execute_event(&event, NULL) != 0) return -1;
    } else if (queue_event(when, &event, tag) != 0) {
      return -1;
    }
  }
  if (final_voice) {
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
  return 0;
}

int skode_queue_program(const event_program_t *program, int voice,
    uint64_t when, int tag) {
  return run_program(program, voice, when, SAMPLE_COUNT_GET(), tag, 0, NULL);
}

int skode_execute_program(const event_program_t *program, int voice,
    uint64_t now, int tag) {
  return run_program(program, voice, now, now, tag, 1, NULL);
}

int skode_execute_program_state(const event_program_t *program, int *voice,
    uint64_t now, int tag) {
  if (!voice) return -1;
  return run_program(program, *voice, now, now, tag, 1, voice);
}

int skode_queue_program_deferred(const event_program_t *program, int voice,
    uint64_t base, char mode, double delay, int tag) {
  uint64_t relative;
  if (delay_to_samples(mode, delay, &relative) != 0) return -1;
  uint64_t when = relative > UINT64_MAX - base ? UINT64_MAX : base + relative;
  return run_program(program, voice, when, SAMPLE_COUNT_GET(), tag, 0, NULL);
}
