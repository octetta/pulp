#include "skode-internal.h"

void skode_envelope_velocity(int voice, float x, uint64_t now) {
  if (voice < 0 || voice >= synth_config.voice_max) return;
  if (x > 0.0f && synth_config.trace_latency) {
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      sv.latency_timestamp_ns[voice] = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
  }
  uint64_t t = sv.link_trig_samp[voice];
  if (t > 0) {
    uint64_t qt = t > UINT64_MAX - now ? UINT64_MAX : t + now;
    event_t event = {0};
    event.voice = voice;
    event.opcode.code = SKODE_OP_ENVELOPE_VELOCITY;
    event.opcode.argc = 1;
    event.opcode.arg[0] = x;
    queue_event(qt, &event, 0);
  } else {
    envelope_velocity(voice, x);
  }
}

int skode_compile_scheduled(skode_t *ctx, const char *text,
    event_program_t *program) {
  skode_compile_result_t result =
    skode_compile_program_ex(text, program, ctx ? ctx->vocab : NULL);
  if (result == SKODE_COMPILE_OK) return 1;
  ctx->printf(ctx, "# command is not schedulable (%d)\n", result);
  return 0;
}

void skode_queue_repeated(const event_program_t *program, int voice,
    int count, double seconds, int tag) {
  uint64_t dt;
  if (!program || count <= 0 || count > QUEUE_SIZE ||
      !skode_seconds_to_samples(seconds, &dt)) {
    return;
  }
  uint64_t qt = SAMPLE_COUNT_GET();
  for (int i = 0; i < count; i++) {
    if (skode_queue_program(program, voice, qt, tag) != 0) break;
    qt = skode_u64_add(qt, dt);
  }
}

void skode_repeat_macro(skode_t *ctx, const double *arg, int argc,
    int tempo_relative) {
  int macro_index;
  int count;
  if (!ctx || !arg || argc < 3 ||
      !skode_double_to_int(arg[0], &macro_index) ||
      !skode_extra_valid(macro_index) ||
      !skode_double_to_int(arg[1], &count) ||
      count <= 0 || count > QUEUE_SIZE ||
      !isfinite(arg[2]) || arg[2] < 0.0) {
    return;
  }

  int tag = 0;
  if (argc > 3 && !skode_double_to_int(arg[3], &tag)) return;

  char macro[STRING_BUF_LEN];
  if (skode_extra_copy(macro_index, macro, sizeof(macro)) != 0 ||
      macro[0] == '\0') {
    return;
  }
  event_program_t program;
  if (!skode_compile_scheduled(ctx, macro, &program)) return;

  double seconds = arg[2];
  if (tempo_relative) seconds *= tempo_step_seconds_get() * 4.0f;
  skode_queue_repeated(&program, ctx->voice, count, seconds, tag);
}

int skode_opcode_supported(skode_opcode_t opcode) {
  switch (opcode) {
    case SKODE_OP_VOICE:
    case SKODE_OP_PATTERN_STATE:
    case SKODE_OP_PATTERN_LOOP:
    case SKODE_OP_STREAM_COPY:
    case SKODE_OP_STREAM_MODE:
    case SKODE_OP_STREAM_POS:
    case SKODE_OP_AMP:
    case SKODE_OP_FREQ:
    case SKODE_OP_MIDI_NOTE:
    case SKODE_OP_PAN:
    case SKODE_OP_VELOCITY:
    case SKODE_OP_ENVELOPE_VELOCITY:
    case SKODE_OP_WAVE_DIRECTION:
    case SKODE_OP_WAVE_LOOP:
    case SKODE_OP_WAVE_LOOP_COUNT:
    case SKODE_OP_LINK_MIDI:
    case SKODE_OP_LINK_VELOCITY:
    case SKODE_OP_TRIGGER_DELAY:
    case SKODE_OP_MUTE:
    case SKODE_OP_MIDI_DETUNE:
    case SKODE_OP_VOICE_RESET:
    case SKODE_OP_TRIGGER:
    case SKODE_OP_WAVE:
    case SKODE_OP_VOICE_COPY:
    case SKODE_OP_WAVE_DEFAULT:
    case SKODE_OP_VARIABLE_SET:
    case SKODE_OP_CONTROL_EVENT:
    case SKODE_OP_DELAY_PARAMS:
    case SKODE_OP_DELAY_DAMPING:
    case SKODE_OP_DELAY_FREEZE:
    case SKODE_OP_DELAY_PINGPONG:
    case SKODE_OP_DELAY_TIME:
    case SKODE_OP_DELAY_SYNC:
    case SKODE_OP_DELAY_GRIT:
      return 1;
    case SKODE_OP_AMP_MOD: return 1;
    case SKODE_OP_PHASE_DISTORTION:
    case SKODE_OP_PHASE_MOD:
    case SKODE_OP_PHASE_ENVELOPE:
    case SKODE_OP_PHASE_ENVELOPE_DEPTH:
      return 1;
    case SKODE_OP_FILTER_ENVELOPE:
    case SKODE_OP_FILTER_ENVELOPE_DEPTH:
      return 1;
    case SKODE_OP_FREQ_MOD:
    case SKODE_OP_FREQ_MOD_MODE:
    case SKODE_OP_FREQ_FEEDBACK:
      return 1;
    case SKODE_OP_GLISSANDO: return 1;
    case SKODE_OP_SAMPLE_HOLD: return 1;
    case SKODE_OP_FILTER_MODE:
    case SKODE_OP_FILTER_FREQ:
    case SKODE_OP_FILTER_RESONANCE:
      return 1;
    case SKODE_OP_ENVELOPE_MODE:
    case SKODE_OP_ENVELOPE:
      return 1;
    case SKODE_OP_PAN_MOD: return 1;
    case SKODE_OP_QUANTIZE: return 1;
    case SKODE_OP_RECORD_TRACK: return 1;
    case SKODE_OP_SMOOTHER: return 1;
    case SKODE_OP_RING_MOD: return 1;
    case SKODE_OP_WAVE_RANGE_SET:
    case SKODE_OP_WAVE_LOOP_SET:
    case SKODE_OP_POLY_NOTE:
    case SKODE_OP_POLY_RELEASE:
    case SKODE_OP_POLY_BEND:
    case SKODE_OP_FREQ_BEND:
    case SKODE_OP_FREQ_BEND_PARAM:
    case SKODE_OP_AMP_BEND:
    case SKODE_OP_AMP_BEND_PARAM:
    case SKODE_OP_PATTERN_MODULO:
    case SKODE_OP_RATCHET:
      return 1;
    case SKODE_OP_NONE:
    case SKODE_OP_DELAY:
    default:
      return 0;
  }
}

static int skode_opcode_int(const opcode_event_t *opcode, int n, int *value) {
  return opcode && n >= 0 && n < opcode->argc &&
    skode_double_to_int(opcode->arg[n], value);
}

int skode_foreign_function(skode_t *ctx, int index,
    const double *arg, int argc) {
  if (!ctx || !ctx->parse) return -1;
  skred_foreign_call_t call = {
    .index = index,
    .argc = argc,
    .arg = arg,
    .string = ands_string(ctx->parse),
    .data = ands_data(ctx->parse),
    .data_len = ands_data_len(ctx->parse),
    .voice = ctx->voice,
    .pattern = ctx->pattern,
    .step = ctx->step,
  };
  return skred_foreign_function_call(&call);
}

static void skode_opcode_links(const opcode_event_t *opcode,
    float *link0, float *link1, float *link2, float *link3, float *link4, float *link5) {
  int links[6] = {-1, -1, -1, -1, -1, -1};
  for (int i = 0; i < opcode->argc && i < 6; i++) {
    int link;
    if (skode_opcode_int(opcode, i, &link) && skode_voice_valid(link))
      links[i] = link;
  }
  *link0 = links[0];
  *link1 = links[1];
  *link2 = links[2];
  *link3 = links[3];
  *link4 = links[4];
  *link5 = links[5];
}

void skode_stream_copy(void *ctx, int dst, int src);
void skode_stream_mode(void *ctx, int n, int mode);
void skode_stream_pos(void *ctx, int n, int pos);

int skode_execute_voice_opcode(const opcode_event_t *opcode, int voice) {
  if (!opcode || !skode_voice_valid(voice) ||
      
      opcode->argc > SEQ_OPCODE_ARG_MAX || opcode->var_mask != 0) return -1;
  uint8_t default_mask =
    opcode->code == SKODE_OP_MIDI_NOTE ||
    opcode->code == SKODE_OP_MIDI_DETUNE ? 1U :
    opcode->code == SKODE_OP_DELAY_PARAMS ? 0x7eU : 0U;
  if (((uint8_t)opcode->mode & ~default_mask) != 0) return -1;
  for (int i = 0; i < opcode->argc; i++) {
    if (!isfinite(opcode->arg[i]) &&
        !(isnan(opcode->arg[i]) && (default_mask & (1U << i)))) {
      return -1;
    }
  }
  int x = 0;
  int x_valid = skode_opcode_int(opcode, 0, &x);
  switch ((skode_opcode_t)opcode->code) {
    case SKODE_OP_POLY_NOTE:
      {
        int key;
        if (opcode->argc < 4 || opcode->argc > 5 || !x_valid ||
            !skode_opcode_int(opcode, 1, &key)) return -1;
        return skred_poly_note(x, key, opcode->arg[2], opcode->arg[3],
          opcode->argc > 4 ? opcode->arg[4] : 0);
      }
    case SKODE_OP_POLY_RELEASE:
      {
        int key;
        if (opcode->argc < 2 || opcode->argc > 3 || !x_valid ||
            !skode_opcode_int(opcode, 1, &key)) return -1;
        return skred_poly_release(x, key,
          opcode->argc > 2 ? opcode->arg[2] : 0);
      }
    case SKODE_OP_POLY_BEND:
      {
        int key;
        if (opcode->argc < 3 || opcode->argc > 4 || !x_valid ||
            !skode_opcode_int(opcode, 1, &key)) return -1;
        return skred_poly_bend(x, key, opcode->arg[2],
          opcode->argc > 3 ? opcode->arg[3] : 0);
      }
    case SKODE_OP_FREQ_BEND:
      return opcode->argc == 1 ? freq_bend_set(voice, (float)opcode->arg[0]) : -1;
    case SKODE_OP_FREQ_BEND_PARAM:
      return (opcode->argc >= 1 && opcode->argc <= 2)
        ? freq_bend_param_set(voice, (float)opcode->arg[0], opcode->argc > 1 ? (float)opcode->arg[1] : 0.0f)
        : -1;
    case SKODE_OP_AMP_BEND:
      return opcode->argc == 1 ? amp_bend_set(voice, (float)opcode->arg[0]) : -1;
    case SKODE_OP_AMP_BEND_PARAM:
      return (opcode->argc >= 1 && opcode->argc <= 2)
        ? amp_bend_param_set(voice, (float)opcode->arg[0], opcode->argc > 1 ? (float)opcode->arg[1] : 0.0f)
        : -1;
    case SKODE_OP_PATTERN_STATE:
      if (opcode->argc >= 2) {
        if (seq_current_pattern >= 0) seq_state_set_locked((int)opcode->arg[0], (int)opcode->arg[1]);
        else seq_state_set((int)opcode->arg[0], (int)opcode->arg[1]);
      }
      return 0;
    case SKODE_OP_PATTERN_LOOP:
      if (opcode->argc >= 3) {
        int var_id = (int)opcode->arg[0];
        int limit = (int)opcode->arg[1];
        int dest_step = (int)opcode->arg[2];
        if (var_id >= 0 && var_id < ANDS_VAR_MAX) {
          if (global_var[var_id] < limit - 1) {
            global_var[var_id] += 1;
            if (seq_current_pattern >= 0) seq_step_goto_locked(seq_current_pattern, dest_step);
            else seq_step_goto(seq_current_pattern, dest_step); // Not used currently, but fallback
          } else {
            global_var[var_id] = 0;
          }
        }
      }
      return 0;
    case SKODE_OP_STREAM_COPY:
      if (opcode->argc >= 2) skode_stream_copy(NULL, (int)opcode->arg[0], (int)opcode->arg[1]);
      return 0;
    case SKODE_OP_STREAM_MODE:
      if (opcode->argc >= 2) skode_stream_mode(NULL, (int)opcode->arg[0], (int)opcode->arg[1]);
      return 0;
    case SKODE_OP_STREAM_POS:
      if (opcode->argc >= 2) skode_stream_pos(NULL, (int)opcode->arg[0], (int)opcode->arg[1]);
      return 0;
    case SKODE_OP_AMP:
      return opcode->argc == 1 ? amp_set(voice, opcode->arg[0]) : -1;
    case SKODE_OP_AMP_MOD:
      if (opcode->argc < 2) return amp_mod_set(voice, -1, 0, 0);
      return x_valid ? amp_mod_set(voice, x, opcode->arg[1],
        opcode->argc > 2 ? opcode->arg[2] : 0) : -1;
    case SKODE_OP_WAVE_DIRECTION:
      return opcode->argc == 0 ? wave_dir(voice, -1) :
        (x_valid ? wave_dir(voice, x) : -1);
    case SKODE_OP_WAVE_LOOP:
      return opcode->argc == 0 ? wave_loop(voice, -1) :
        (x_valid ? wave_loop(voice, x) : -1);
    case SKODE_OP_WAVE_LOOP_COUNT:
      return x_valid ? wave_loop_count(voice, x) : -1;
    case SKODE_OP_PHASE_DISTORTION:
      if (opcode->argc == 0) return cz_set(voice, 0, 0.0f);
      if (!x_valid) return -1;
      return cz_set(voice, x,
        opcode->argc > 1 ? opcode->arg[1] : 0.0f);
    case SKODE_OP_PHASE_MOD:
      if (opcode->argc < 2) return cmod_set(voice, -1, 0);
      return x_valid ? cmod_set(voice, x, opcode->arg[1]) : -1;
    case SKODE_OP_PHASE_ENVELOPE:
      if (opcode->argc != 4) return -1;
      envelope_configure_e(&sv.cz_envelope[voice], opcode->arg[0],
        opcode->arg[1], opcode->arg[2], opcode->arg[3]);
      sv.use_cz_envelope[voice] = !(opcode->arg[0] == 0 &&
        opcode->arg[1] == 0 && opcode->arg[2] == 1 &&
        opcode->arg[3] == 0);
      return 0;
    case SKODE_OP_PHASE_ENVELOPE_DEPTH:
      if (opcode->argc != 1) return -1;
      sv.cz_env_depth[voice] = opcode->arg[0];
      return 0;
    case SKODE_OP_FREQ:
      return opcode->argc == 1 ? freq_set(voice, opcode->arg[0]) : -1;
    case SKODE_OP_FILTER_ENVELOPE:
      if (opcode->argc != 4) return -1;
      envelope_configure_e(&sv.filter_envelope[voice], opcode->arg[0],
        opcode->arg[1], opcode->arg[2], opcode->arg[3]);
      sv.use_filter_envelope[voice] = !(opcode->arg[0] == 0 &&
        opcode->arg[1] == 0 && opcode->arg[2] == 1 &&
        opcode->arg[3] == 0);
      return 0;
    case SKODE_OP_FILTER_ENVELOPE_DEPTH:
      if (opcode->argc != 1) return -1;
      sv.filter_env_depth[voice] = opcode->arg[0];
      return 0;
    case SKODE_OP_FREQ_MOD:
      if (opcode->argc <= 1) return freq_mod_set(voice, -1, 0, 0);
      return x_valid ? freq_mod_set(voice, x, opcode->arg[1],
        opcode->argc > 2 ? opcode->arg[2] : 0) : -1;
    case SKODE_OP_FREQ_MOD_MODE:
      if (!x_valid) return -1;
      return freq_mod_mode_set(voice, x);
    case SKODE_OP_FREQ_FEEDBACK:
      if (opcode->argc != 1) return -1;
      return freq_feedback_set(voice, opcode->arg[0]);
    case SKODE_OP_GLISSANDO:
      if (opcode->argc != 1) return -1;
      if (opcode->arg[0] <= 0) {
        sv.glissando_enable[voice] = 0;
        sv.glissando_time[voice] = 0;
      } else {
        sv.glissando_enable[voice] = 1;
        sv.glissando_time[voice] = opcode->arg[0];
      }
      return 0;
    case SKODE_OP_LINK_MIDI:
      if (opcode->argc < 1) return -1;
      skode_opcode_links(opcode, &sv.link_midi_0[voice], &sv.link_midi_1[voice], &sv.link_midi_2[voice], &sv.link_midi_3[voice], &sv.link_midi_4[voice], &sv.link_midi_5[voice]);
      return 0;
    case SKODE_OP_SAMPLE_HOLD:
      if (!x_valid) return -1;
      sv.sample_hold_ratio[voice] = (float)x;
      if (opcode->argc > 1) {
        int m;
        if (!skode_opcode_int(opcode, 1, &m)) sv.sample_hold_mode[voice] = m;
      }
      return 0;
    case SKODE_OP_LINK_VELOCITY:
      if (opcode->argc < 1) return -1;
      skode_opcode_links(opcode, &sv.link_velo_0[voice], &sv.link_velo_1[voice], &sv.link_velo_2[voice], &sv.link_velo_3[voice], &sv.link_velo_4[voice], &sv.link_velo_5[voice]);
      return 0;
    case SKODE_OP_TRIGGER_DELAY:
      if (opcode->argc != 1) return -1;
      if (opcode->arg[0] <= 0) {
        sv.link_trig[voice] = -1;
        sv.link_trig_samp[voice] = 0;
      } else {
        long double samples =
          (long double)opcode->arg[0] * MAIN_SAMPLE_RATE;
        sv.link_trig[voice] = opcode->arg[0];
        sv.link_trig_samp[voice] = samples >= (long double)UINT64_MAX ?
          UINT64_MAX : (uint64_t)samples;
      }
      return 0;
    case SKODE_OP_FILTER_MODE:
      if (!x_valid) return -1;
      {
        int mode = (int)x;
        int character = sv.filter_mode[voice] / 10;
        if (opcode->argc > 1) {
          int c;
          if (!skode_opcode_int(opcode, 1, &c)) character = c;
        }
        sv.filter_mode[voice] = (character * 10) + (mode % 10);
        mmf_set_params(&skred_global_engine, voice, sv.filter_freq[voice], sv.filter_res[voice]);
      }
      return 0;
    case SKODE_OP_FILTER_FREQ:
      return opcode->argc == 1 ? mmf_set_freq(&skred_global_engine, voice, opcode->arg[0]) : -1;
    case SKODE_OP_ENVELOPE_MODE:
      if (!x_valid) return -1;
      sv.amp_envelope_mode[voice] = x;
      return 0;
    case SKODE_OP_ENVELOPE_VELOCITY:
      return opcode->argc == 1 ?
        envelope_velocity(voice, opcode->arg[0]) : -1;
    case SKODE_OP_VELOCITY:
    #if 1
      return opcode->argc == 1 ?
        skode_linked_velocity(voice, opcode->arg[0], SAMPLE_COUNT_GET()) : -1;
    #else
      if (opcode->argc != 1) return -1;
      {
        uint64_t now = SAMPLE_COUNT_GET();
        skode_envelope_velocity(voice, opcode->arg[0], now);
        if (sv.link_velo_0[voice] >= 0)
          skode_envelope_velocity(sv.link_velo_0[voice], opcode->arg[0], now);
        if (sv.link_velo_1[voice] >= 0)
          skode_envelope_velocity(sv.link_velo_1[voice], opcode->arg[0], now);
        if (sv.link_velo_2[voice] >= 0)
          skode_envelope_velocity(sv.link_velo_2[voice], opcode->arg[0], now);
        if (sv.link_velo_3[voice] >= 0)
          skode_envelope_velocity(sv.link_velo_3[voice], opcode->arg[0], now);
        return 0;
      }
    #endif
    case SKODE_OP_MUTE:
      return x_valid ? wave_mute(voice, x) : -1;
    case SKODE_OP_MIDI_NOTE:
      if (opcode->argc != 1 && opcode->argc != 2) return -1;
      return skode_midi_note(voice, opcode->arg[0],
        opcode->argc == 2 ? opcode->arg[1] : 0);
    case SKODE_OP_MIDI_DETUNE:
      if (opcode->argc < 1 || opcode->argc > 2) return -1;
      if (!isnan(opcode->arg[0]))
        sv.midi_transpose[voice] = opcode->arg[0];
      if (opcode->argc > 1) sv.midi_cents[voice] = opcode->arg[1];
      return 0;
    case SKODE_OP_PAN:
      return opcode->argc == 1 ? pan_set(voice, opcode->arg[0]) : -1;
    case SKODE_OP_PAN_MOD:
      if (opcode->argc < 2) return pan_mod_set(voice, -1, 0, 0);
      return x_valid ? pan_mod_set(voice, x, opcode->arg[1],
        opcode->argc > 2 ? opcode->arg[2] : 0) : -1;
    case SKODE_OP_QUANTIZE:
      if (!x_valid) return -1;
      {
        int bits = (int)x;
        int curve = sv.quantize[voice] / 100;
        if (opcode->argc > 1) {
          int c;
          if (!skode_opcode_int(opcode, 1, &c)) curve = c;
        }
        wave_quant(voice, (curve * 100) + (bits % 100));
      }
      return 0;
    case SKODE_OP_FILTER_RESONANCE:
      return opcode->argc == 1 ? mmf_set_res(&skred_global_engine, voice, opcode->arg[0]) : -1;
    case SKODE_OP_RECORD_TRACK:
      return x_valid ? synth_record_track_set(voice, x) : -1;
    case SKODE_OP_SMOOTHER:
      if (opcode->argc != 1) return -1;
      if (opcode->arg[0] <= 0) {
        sv.smoother_enable[voice] = 0;
      } else {
        sv.smoother_enable[voice] = 1;
        sv.smoother_smoothing[voice] = opcode->arg[0];
      }
      return 0;
    case SKODE_OP_VOICE_RESET:
      return x_valid ? wave_reset(x) : -1;
    case SKODE_OP_ENVELOPE:
      return opcode->argc == 4 ? envelope_set(voice, opcode->arg[0],
        opcode->arg[1], opcode->arg[2], opcode->arg[3]) : -1;
    case SKODE_OP_TRIGGER:
      if (opcode->argc != 0) return -1;
      envelope_velocity(voice, 1);
      if (sv.link_velo_0[voice] >= 0) envelope_velocity(sv.link_velo_0[voice], 1);
      if (sv.link_velo_1[voice] >= 0) envelope_velocity(sv.link_velo_1[voice], 1);
      if (sv.link_velo_2[voice] >= 0) envelope_velocity(sv.link_velo_2[voice], 1);
      if (sv.link_velo_3[voice] >= 0) envelope_velocity(sv.link_velo_3[voice], 1);
      return 0;
    case SKODE_OP_WAVE:
      if (!x_valid || wave_set(voice, x) != 0) return -1;
      if (opcode->argc > 1) {
        int value;
        if (!skode_opcode_int(opcode, 1, &value)) return -1;
        sv.interpolate[voice] = value != 0;
      }
      if (opcode->argc > 2) {
        int value;
        if (!skode_opcode_int(opcode, 2, &value)) return -1;
        sv.one_shot[voice] = value != 0;
      } else sv.one_shot[voice] = sw.one_shot[x];
      osc_reclassify(&skred_global_engine, voice);
      return 0;
    case SKODE_OP_VOICE_COPY:
      return x_valid && skode_voice_valid(x) ? voice_copy(voice, x) : -1;
    case SKODE_OP_WAVE_DEFAULT:
      return opcode->argc == 0 ? wave_default(voice) : -1;
    case SKODE_OP_VARIABLE_SET:
      if (opcode->argc != 2 || !x_valid ||
          x < 0 || x >= ANDS_VAR_MAX) return -1;
      global_var[x] = opcode->arg[1];
      return 0;
    case SKODE_OP_CONTROL_EVENT:
      return skode_emit_control_event_opcode(opcode, voice, -1, -1, -1);
    case SKODE_OP_DELAY_PARAMS:
      {
        int bus = 1;
        int coarse, fine, feedback, mod_freq, mod_depth, level;
        if (opcode->argc < 1 || opcode->argc > 7 ||
            !skode_opcode_int(opcode, 0, &bus)) return -1;
        delay_params_get(&skred_global_engine, bus, &coarse, &fine, &feedback, &mod_freq,
          &mod_depth, &level);
        if (opcode->argc > 1 && isfinite(opcode->arg[1]) &&
            !skode_opcode_int(opcode, 1, &coarse)) return -1;
        if (opcode->argc > 2 && isfinite(opcode->arg[2]) &&
            !skode_opcode_int(opcode, 2, &fine)) return -1;
        if (opcode->argc > 3 && isfinite(opcode->arg[3]) &&
            !skode_opcode_int(opcode, 3, &feedback)) return -1;
        if (opcode->argc > 4 && isfinite(opcode->arg[4]) &&
            !skode_opcode_int(opcode, 4, &mod_freq)) return -1;
        if (opcode->argc > 5 && isfinite(opcode->arg[5]) &&
            !skode_opcode_int(opcode, 5, &mod_depth)) return -1;
        if (opcode->argc > 6 && isfinite(opcode->arg[6]) &&
            !skode_opcode_int(opcode, 6, &level)) return -1;
        return delay_params_set(&skred_global_engine, bus, coarse, fine, feedback, mod_freq,
          mod_depth, level);
      }
    case SKODE_OP_DELAY_DAMPING:
      {
        int bus = 1;
        int damping, hp;
        if (opcode->argc < 1 || opcode->argc > 3 ||
            !skode_opcode_int(opcode, 0, &bus)) return -1;
        delay_damping_get(&skred_global_engine, bus, &damping, &hp);
        if (opcode->argc > 1 && isfinite(opcode->arg[1]) &&
            !skode_opcode_int(opcode, 1, &damping)) return -1;
        if (opcode->argc > 2 && isfinite(opcode->arg[2]) &&
            !skode_opcode_int(opcode, 2, &hp)) return -1;
        return delay_damping_set(&skred_global_engine, bus, damping, hp);
      }
    case SKODE_OP_DELAY_FREEZE:
      {
        int bus = 1;
        int on;
        if (opcode->argc < 1 || opcode->argc > 2 ||
            !skode_opcode_int(opcode, 0, &bus)) return -1;
        on = delay_freeze_get(&skred_global_engine, bus);
        if (opcode->argc > 1 && isfinite(opcode->arg[1]) &&
            !skode_opcode_int(opcode, 1, &on)) return -1;
        return delay_freeze_set(&skred_global_engine, bus, on);
      }
    case SKODE_OP_DELAY_PINGPONG:
      {
        int bus = 1;
        int on;
        if (opcode->argc < 1 || opcode->argc > 2 ||
            !skode_opcode_int(opcode, 0, &bus)) return -1;
        on = delay_pingpong_get(&skred_global_engine, bus);
        if (opcode->argc > 1 && isfinite(opcode->arg[1]) &&
            !skode_opcode_int(opcode, 1, &on)) return -1;
        return delay_pingpong_set(&skred_global_engine, bus, on);
      }
    case SKODE_OP_DELAY_TIME:
      {
        int bus;
        if (opcode->argc != 2 || !skode_opcode_int(opcode, 0, &bus) ||
            !isfinite(opcode->arg[1])) return -1;
        return delay_time_ms_set(&skred_global_engine, bus, (float)opcode->arg[1]);
      }
    case SKODE_OP_DELAY_SYNC:
      {
        int bus;
        if (opcode->argc != 3 || !skode_opcode_int(opcode, 0, &bus) ||
            !isfinite(opcode->arg[1]) || !isfinite(opcode->arg[2])) return -1;
        return delay_time_sync_set(&skred_global_engine, bus, (float)opcode->arg[1],
          (float)opcode->arg[2]);
      }
    case SKODE_OP_DELAY_GRIT:
      {
        int bus = 1;
        int bits, native;
        if (opcode->argc < 1 || opcode->argc > 3 ||
            !skode_opcode_int(opcode, 0, &bus)) return -1;
        delay_grit_get(&skred_global_engine, bus, &bits, &native);
        if (opcode->argc > 1 && isfinite(opcode->arg[1]) &&
            !skode_opcode_int(opcode, 1, &bits)) return -1;
        if (opcode->argc > 2 && isfinite(opcode->arg[2]) &&
            !skode_opcode_int(opcode, 2, &native)) return -1;
        return delay_grit_set(&skred_global_engine, bus, bits, native);
      }
    case SKODE_OP_RING_MOD:
      if (opcode->argc < 1 || opcode->argc > 2) return -1;
      sv.ring_osc[voice] =
        x_valid && skode_voice_valid(x) ? x : -1;
      sv.ring_amount[voice] =
        opcode->argc > 1 ? opcode->arg[1] : 0;
      return 0;
    case SKODE_OP_WAVE_RANGE_SET:
      if (opcode->argc == 2) {
        voice_wave_range_set(voice, opcode->arg[0], opcode->arg[1]);
        return 0;
      }
      if (opcode->argc == 0) {
        voice_wave_range_reset(voice);
        return 0;
      }
      return -1;
    case SKODE_OP_WAVE_LOOP_SET:
      if (opcode->argc == 2) {
        voice_loop_points_set(voice, opcode->arg[0], opcode->arg[1]);
        return 0;
      }
      if (opcode->argc == 0) {
        voice_loop_points_reset(voice);
        return 0;
      }
      return -1;
    case SKODE_OP_NONE:
    case SKODE_OP_DELAY:
    case SKODE_OP_VOICE:
    default:
      return -1;
  }
}
