#include "skode-internal.h"
#include "record.h"

static int word_exec_ce(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 0 && argc <= 4) {
        opcode_event_t opcode = {
          .code = SKODE_OP_CONTROL_EVENT,
          .argc = (uint8_t)argc,
        };
        for (int i = 0; i < argc; i++) opcode.arg[i] = (float)arg[i];
        skode_emit_control_event_opcode(&opcode, voice, -1, -1, -1);
      }
      return 0;
}

static int word_exec_R_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        int tag = x;
        seq_kill_by_tag(tag);
      }
      return 0;
}

static int word_exec_R_bang_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      seq_kill_all();
      return 0;
}

static int word_exec_RR(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 1 && x_valid && x > 0 && x <= QUEUE_SIZE &&
          isfinite(arg[1]) && arg[1] >= 0.0) {
        event_program_t program;
        if (!skode_compile_scheduled(ctx, ands_string(ctx->parse), &program))
          return 0;
        int tag = 0;
        if (argc > 2) skode_double_to_int(arg[2], &tag);
        double seconds = tempo_step_seconds_get() * 4.0f * arg[1];
        skode_queue_repeated(&program, ctx->voice, x, seconds, tag);
      } return 0;
}

static int word_exec_eRR(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      skode_repeat_macro(ctx, arg, argc, 1);
      return 0;
}

static int word_exec_eR(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      skode_repeat_macro(ctx, arg, argc, 0);
      return 0;
}

static int word_exec_DO_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && x>0) {
        event_program_t program;
        if (!skode_compile_scheduled(ctx, ands_string(ctx->parse), &program))
          return 0;
        int tag = 0;
        if (argc > 1) skode_double_to_int(arg[1], &tag);
        uint64_t qt = SAMPLE_COUNT_GET();
        skode_queue_program(&program, ctx->voice, qt, tag);
      } return 0;
}

static int word_exec_R(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 1 && x_valid && x > 0 && x <= QUEUE_SIZE &&
          isfinite(arg[1]) && arg[1] >= 0.0) {
        event_program_t program;
        if (!skode_compile_scheduled(ctx, ands_string(ctx->parse), &program))
          return 0;
        int tag = 0;
        if (argc > 2) skode_double_to_int(arg[2], &tag);
        skode_queue_repeated(&program, ctx->voice, x, arg[1], tag);
      } return 0;
}

static int word_exec_xg(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

  return 0;
}

static int word_exec__gtx(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      seq_step_goto(ctx->pattern, x);
      return 0;
}

static int word_exec_xa(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        const char *source = ands_string(ctx->parse);
        event_program_t program;
        int source_only = source[0] == '\0' || strcmp(source, "-") == 0;
        skode_compile_result_t result = source_only ?
          SKODE_COMPILE_OK : skode_compile_program(source, &program);
        if (result == SKODE_COMPILE_OK) {
          seq_step_append(ctx->pattern, source, source_only ? NULL : &program);
        } else {
          ctx->printf(ctx, "# sequence command is not schedulable (%d)\n", result);
        }
      }
      return 0;
}

static int word_exec__ltx(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (arg == 0) {
      } else {
        seq_edit_lock();
        char *s = seq_step_get(ctx->pattern, x);
        ands_string_from_external(ctx->parse, s, strlen(s));
        seq_edit_unlock();
      }
      return 0;
}

static int word_exec_y(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && x >= 0 && x < PATTERNS_MAX) {
        int old_p = ctx->pattern;
        ctx->pattern = x;
        scope_pattern_pointer = x;
        if (old_p != x && old_p >= 0 && seq_control_events[x]) {
          skred_control_pattern_event(SKRED_CONTROL_EVENT_PATTERN_CHANGE, SAMPLE_COUNT_GET(), x, 0);
        }
      }
      return 0;
}

static int word_exec_ys_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int p = (argc && x >= 0 && x < PATTERNS_MAX) ? x : ctx->pattern;
        pattern_show(ctx, p, 1);
      }
      return 0;
}

static int word_exec_yt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (ctx->pattern >= 0 && ctx->pattern < PATTERNS_MAX) {
        seq_edit_lock();
        skode_copy_string(seq_text[ctx->pattern], TEXT_MAX, ands_string(ctx->parse));
        seq_edit_unlock();
      }
      return 0;
}

static int word_exec_ym(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        seq_mute_set(ctx->pattern, x);
        skred_control_pattern_event(SKRED_CONTROL_EVENT_MUTE_CHANGE, SAMPLE_COUNT_GET(), ctx->pattern, x);
      }
      return 0;
}

static int word_exec_yc(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) seq_control_events_set(ctx->pattern, x);
      return 0;
}

static int word_exec_Y(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && x >= 0 && x < PATTERNS_MAX) {
        pattern_reset(x);
      }
      return 0;
}

static int word_exec_z(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        seq_state_set(ctx->pattern, x);
      } else pattern_show(ctx, ctx->pattern, 1);
      return 0;
}

static int word_exec_zg(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && x >= 0 && x < SEQ_STEPS_MAX) {
        seq_step_goto(ctx->pattern, x);
      }
      return 0;
}

static int word_exec_zq(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        seq_state_queue(ctx->pattern, x);
        skred_control_pattern_event(SKRED_CONTROL_EVENT_PATTERN_QUEUE, SAMPLE_COUNT_GET(), ctx->pattern, x);
      }
      return 0;
}

static int word_exec_z_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      pattern_show(ctx, ctx->pattern, 1);
      return 0;
}

static int word_exec_Z(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        seq_state_all(x);
      } else {
        ctx->printf(ctx, "M%g\n", tempo_bpm_get());
        for (int p = 0; p < PATTERNS_MAX; p++) {
          if (seq_pattern_length[p] > 0 || seq_text[p][0] != '\0')
            pattern_show(ctx, p, 0);
        }
      }
      return 0;
}

static int word_exec_z_q_bs_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

  return 0;
}

static int word_exec_Z_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "M%g\n", tempo_bpm_get());
      for (int p = 0; p < PATTERNS_MAX; p++) {
        if (seq_pattern_length[p] > 0 || seq_text[p][0] != '\0')
          pattern_show(ctx, p, 1);
      }
      return 0;
}

static int word_exec__qce(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      control_event_show(ctx, 0);
      return 0;
}

static int word_exec__qce_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "# control events cleared:%d\n",
        skred_control_event_clear());
      return 0;
}

static int word_exec__qq(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      opcode_queue_show(ctx);
      return 0;
}

static int word_exec__qo(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc == 0) {
        opcode_queue_show(ctx);
      } else {
        if (!x_valid) {
          ctx->printf(ctx, "# invalid opcode pattern\n");
          return 0;
        }
        int pattern = x;
        int step = -1;
        if (pattern == -1) pattern = ctx->pattern;
        if (argc > 1 && !skode_double_to_int(arg[1], &step)) {
          ctx->printf(ctx, "# invalid opcode step\n");
          return 0;
        }
        opcode_pattern_show(ctx, pattern, step);
      }
      return 0;
}

static int word_exec__slashrg(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        const char *filename = ands_string(ctx->parse);
        double max_seconds = argc ? arg[0] : 0.0;
        if (!filename || filename[0] == '\0') {
          ctx->printf(ctx, "# /rg requires [filename]\n");
        } else if (!isfinite(max_seconds) || max_seconds < 0.0) {
          ctx->printf(ctx, "# /rg duration must be >= 0\n");
        } else if (recorder_start(filename, max_seconds) == 0) {
          if (max_seconds > 0.0) {
            ctx->printf(ctx, "# recording [%s] max=%g seconds\n",
                        filename, max_seconds);
          } else {
            ctx->printf(ctx, "# recording [%s]\n", filename);
          }
        } else {
          ctx->printf(ctx, "# recording start failed [%s]\n", filename);
        }
      }
      return 0;
}

static int word_exec__slashrs(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (recorder_stop() == 0) {
        ctx->printf(ctx, "# recording stopped\n");
      } else {
        ctx->printf(ctx, "# recording stop failed\n");
      }
      return 0;
}

static int word_exec__slashr_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        const char *state = "unknown";
        switch (recorder_state()) {
          case RECORDER_STOPPED: state = "stopped"; break;
          case RECORDER_RECORDING: state = "recording"; break;
          case RECORDER_STOPPING: state = "stopping"; break;
          case RECORDER_ERROR: state = "error"; break;
        }
        ctx->printf(ctx, "# recorder state=%s frames=%llu dropped=%llu\n",
                    state,
                    (unsigned long long)recorder_frames_written(),
                    (unsigned long long)recorder_dropped_frames());
      }
      return 0;
}

static int word_exec__slashr(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int wave_slot = EXT_SAMPLE_000;
        int one_shot = 1;
        int channel = -1;
        if (argc && !skode_double_to_int(arg[0], &wave_slot)) return 0;
        if (argc > 1 &&
            (!skode_double_to_int(arg[1], &one_shot) ||
             (one_shot != 0 && one_shot != 1))) {
          ctx->printf(ctx, "# /r mode must be 0=cycle or 1=one-shot\n");
          return 0;
        }
        if (argc > 2 && !skode_double_to_int(arg[2], &channel)) return 0;
        if (argc > 3) {
          ctx->printf(ctx, "# usage: /r slot[,mode[,channel]]\n");
          return 0;
        }
        rec_load(ctx, wave_slot, one_shot, channel);
      }
      return 0;
                        //              x/0  1     2        3
                        //              300  rate one-shot offset
}

static int word_exec__slashcer(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && x_valid) skred_control_response_set_enabled(x != 0);
      ctx->printf(ctx, "%s", skred_control_response_status());
      return 0;
}

static int word_exec__slashce_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "%s", skred_control_response_status());
      return 0;
}

static int word_exec__slashce_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc == 0) {
        skred_control_response_clear();
        ctx->printf(ctx, "# ce bindings cleared\n");
      } else if (argc > 1 && x_valid) {
        int key;
        if (skode_double_to_int(arg[1], &key)) {
          int removed = skred_control_response_remove((uint32_t)x, key);
          ctx->printf(ctx, "# ce bindings removed %d\n", removed);
        }
      }
      return 0;
}

static int word_exec__slashceb(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 1 && x_valid && ands_string_len(ctx->parse) > 0) {
        int key;
        if (skode_double_to_int(arg[1], &key) &&
            skred_control_response_bind((uint32_t)x, key,
              ands_string(ctx->parse)) == 0) {
          ctx->printf(ctx, "# ce bound %d,%d -> %s\n", x, key,
            ands_string(ctx->parse));
        } else {
          ctx->printf(ctx, "# ce binding failed\n");
        }
      } else {
        ctx->printf(ctx, "# usage: [skode-command] /ceb type key\n");
      }
      return 0;
}

static int word_exec__slashcex(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 2 && x_valid) {
        int index, type, key;
        char command[STRING_BUF_LEN];
        if (skode_double_to_int(arg[0], &index) &&
            skode_double_to_int(arg[1], &type) &&
            skode_double_to_int(arg[2], &key) &&
            skode_extra_copy(index, command, sizeof(command)) == 0 &&
            command[0] != '\0' &&
            skred_control_response_bind((uint32_t)type, key, command) == 0) {
          ctx->printf(ctx, "# ce bound %d,%d -> %s\n", type, key, command);
        } else {
          ctx->printf(ctx, "# ce binding failed\n");
        }
      } else {
        ctx->printf(ctx, "# usage: /cex external type key\n");
      }
      return 0;
}

static int word_exec__gtr(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (!ands_string_fresh(ctx->parse) ||
          !ands_string(ctx->parse)[0]) {
        ctx->printf(ctx, "# >r requires [filename]\n");
      } else {
        int state = atomic_load_int(&sampling.state);
        if (state != SAMPLE_STATE_COMPLETE) {
          ctx->printf(ctx, "# recording buffer is not complete\n");
        } else {
          skode_write_wav(ctx, ands_string(ctx->parse),
                          sampling.where, sampling.len,
                          sampling.channels == 2 ? 2 : 1,
                          MAIN_SAMPLE_RATE, 1);
        }
      }
      return 0;
}

static int word_exec__hatr(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

  return 0;
}

static int word_exec__ltr(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && isfinite(arg[0]) && arg[0] > 0.0 &&
          arg[0] <= (double)(INT_MAX / AUDIO_CHANNELS) / MAIN_SAMPLE_RATE) {
        int source = SAMPLE_SOURCE_DRY;
        int sample_voice = -1;
        if (argc > 1 && !skode_double_to_int(arg[1], &source)) {
          ctx->printf(ctx, "# <r source must be 0=dry, 1=voice, or 2=master\n");
          return 0;
        }
        if (source < SAMPLE_SOURCE_DRY || source > SAMPLE_SOURCE_MASTER) {
          ctx->printf(ctx, "# <r source must be 0=dry, 1=voice, or 2=master\n");
          return 0;
        }
        if (source == SAMPLE_SOURCE_VOICE) {
          if (argc != 3 ||
              !skode_double_to_int(arg[2], &sample_voice) ||
              !skode_voice_valid(sample_voice)) {
            ctx->printf(ctx, "# usage: <r seconds,1,voice\n");
            return 0;
          }
        } else if (argc > 2) {
          ctx->printf(ctx, "# usage: <r seconds[,source[,voice]]\n");
          return 0;
        }
        if (!skode_sample_go((int)(arg[0] * (double)MAIN_SAMPLE_RATE),
                             source, sample_voice)) {
          ctx->printf(ctx, "# recording buffer busy or allocation failed\n");
        }
      } else {
        int state = atomic_load_int(&sampling.state);
        ctx->printf(ctx, "# sample state=%d source=%d voice=%d remaining=%d frames=%d channels=%d\n",
                    state, sampling.source, sampling.source_voice,
                    atomic_load_int(&sampling.frames),
                    state == SAMPLE_STATE_COMPLETE ? sampling.len : 0,
                    sampling.channels);
      }
      return 0;
}

static int word_exec__pct(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) seq_modulo_set(ctx->pattern, x);
      return 0;
}


// Word declarations
static skode_word_t word_ce = { WID("ce"), .execute = word_exec_ce, .safety = WORD_IMMEDIATE_ONLY , .category = "events" };
static skode_word_t word_R_bang = { WID("R!"), .execute = word_exec_R_bang, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word_R_bang_bang = { WID("R!!"), .execute = word_exec_R_bang_bang, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word_RR = { WID("RR"), .execute = word_exec_RR, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word_eRR = { WID("eRR"), .execute = word_exec_eRR, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word_eR = { WID("eR"), .execute = word_exec_eR, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word_DO_q = { WID("DO?"), .execute = word_exec_DO_q, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word_R = { WID("R"), .execute = word_exec_R, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word_xg = { WID("xg"), .execute = word_exec_xg, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word__gtx = { WID(">x"), .execute = word_exec__gtx, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word_xa = { WID("xa"), .execute = word_exec_xa, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word__ltx = { WID("<x"), .execute = word_exec__ltx, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word_y = { WID("y"), .execute = word_exec_y, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word_ys_q = { WID("ys?"), .execute = word_exec_ys_q, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word_yt = { WID("yt"), .execute = word_exec_yt, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word_ym = { WID("ym"), .execute = word_exec_ym, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word_yc = { WID("yc"), .execute = word_exec_yc, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word_Y = { WID("Y"), .execute = word_exec_Y, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word_z = { WID("z"), .execute = word_exec_z, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word_zg = { WID("zg"), .execute = word_exec_zg, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word_zq = { WID("zq"), .execute = word_exec_zq, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word_z_q = { WID("z?"), .execute = word_exec_z_q, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word_Z = { WID("Z"), .execute = word_exec_Z, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word_z_q_bs_q = { WID("z?\?"), .execute = word_exec_z_q_bs_q, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word_Z_q = { WID("Z?"), .execute = word_exec_Z_q, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word__qce = { WID("?ce"), .execute = word_exec__qce, .safety = WORD_IMMEDIATE_ONLY , .category = "events" };
static skode_word_t word__qce_bang = { WID("?ce!"), .execute = word_exec__qce_bang, .safety = WORD_IMMEDIATE_ONLY , .category = "events" };
static skode_word_t word__qq = { WID("?q"), .execute = word_exec__qq, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word__qo = { WID("?o"), .execute = word_exec__qo, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };
static skode_word_t word__slashrg = { WID("/rg"), .execute = word_exec__slashrg, .safety = WORD_IMMEDIATE_ONLY , .category = "recording" };
static skode_word_t word__slashrs = { WID("/rs"), .execute = word_exec__slashrs, .safety = WORD_IMMEDIATE_ONLY , .category = "recording" };
static skode_word_t word__slashr_q = { WID("/r?"), .execute = word_exec__slashr_q, .safety = WORD_IMMEDIATE_ONLY , .category = "recording" };
static skode_word_t word__slashr = { WID("/r"), .execute = word_exec__slashr, .safety = WORD_IMMEDIATE_ONLY , .category = "recording" };
static skode_word_t word__slashcer = { WID("/cer"), .execute = word_exec__slashcer, .safety = WORD_IMMEDIATE_ONLY , .category = "events" };
static skode_word_t word__slashce_q = { WID("/ce?"), .execute = word_exec__slashce_q, .safety = WORD_IMMEDIATE_ONLY , .category = "events" };
static skode_word_t word__slashce_bang = { WID("/ce!"), .execute = word_exec__slashce_bang, .safety = WORD_IMMEDIATE_ONLY , .category = "events" };
static skode_word_t word__slashceb = { WID("/ceb"), .execute = word_exec__slashceb, .safety = WORD_IMMEDIATE_ONLY , .category = "events" };
static skode_word_t word__slashcex = { WID("/cex"), .execute = word_exec__slashcex, .safety = WORD_IMMEDIATE_ONLY , .category = "events" };
static skode_word_t word__gtr = { WID(">r"), .execute = word_exec__gtr, .safety = WORD_IMMEDIATE_ONLY , .category = "recording" };
static skode_word_t word__hatr = { WID("^r"), .execute = word_exec__hatr, .safety = WORD_IMMEDIATE_ONLY , .category = "recording" };
static skode_word_t word__ltr = { WID("<r"), .execute = word_exec__ltr, .safety = WORD_IMMEDIATE_ONLY , .category = "recording" };
static skode_word_t word__pct = { WID("%"), .execute = word_exec__pct, .safety = WORD_IMMEDIATE_ONLY , .category = "sequencer" };

static skode_word_t word_ys = { WID("ys"), .execute = word_exec_ys_q, .safety = WORD_IMMEDIATE_ONLY };

void skode_register_words_seq(skode_vocab_t *vocab) {
  skode_dict_register(vocab, &word_ys);
  skode_dict_register(vocab, &word_ce);
  skode_dict_register(vocab, &word_R_bang);
  skode_dict_register(vocab, &word_R_bang_bang);
  skode_dict_register(vocab, &word_RR);
  skode_dict_register(vocab, &word_eRR);
  skode_dict_register(vocab, &word_eR);
  skode_dict_register(vocab, &word_DO_q);
  skode_dict_register(vocab, &word_R);
  skode_dict_register(vocab, &word_xg);
  skode_dict_register(vocab, &word__gtx);
  skode_dict_register(vocab, &word_xa);
  skode_dict_register(vocab, &word__ltx);
  skode_dict_register(vocab, &word_y);
  skode_dict_register(vocab, &word_ys_q);
  skode_dict_register(vocab, &word_yt);
  skode_dict_register(vocab, &word_ym);
  skode_dict_register(vocab, &word_yc);
  skode_dict_register(vocab, &word_Y);
  skode_dict_register(vocab, &word_z);
  skode_dict_register(vocab, &word_zg);
  skode_dict_register(vocab, &word_zq);
  skode_dict_register(vocab, &word_z_q);
  skode_dict_register(vocab, &word_Z);
  skode_dict_register(vocab, &word_z_q_bs_q);
  skode_dict_register(vocab, &word_Z_q);
  skode_dict_register(vocab, &word__qce);
  skode_dict_register(vocab, &word__qce_bang);
  skode_dict_register(vocab, &word__qq);
  skode_dict_register(vocab, &word__qo);
  skode_dict_register(vocab, &word__slashrg);
  skode_dict_register(vocab, &word__slashrs);
  skode_dict_register(vocab, &word__slashr_q);
  skode_dict_register(vocab, &word__slashr);
  skode_dict_register(vocab, &word__slashcer);
  skode_dict_register(vocab, &word__slashce_q);
  skode_dict_register(vocab, &word__slashce_bang);
  skode_dict_register(vocab, &word__slashceb);
  skode_dict_register(vocab, &word__slashcex);
  skode_dict_register(vocab, &word__gtr);
  skode_dict_register(vocab, &word__hatr);
  skode_dict_register(vocab, &word__ltr);
  skode_dict_register(vocab, &word__pct);
}
