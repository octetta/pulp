#include "skode-internal.h"
#include "recorder.h"

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

static int word_exec_DQ(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  int x = 0;
  if (argc > 0 && skode_double_to_int(arg[0], &x)) {
    seq_kill_by_tag(x);
  } else {
    seq_kill_all();
  }
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
        char hint[16] = "";
        skode_compile_result_t result = source_only ?
          SKODE_COMPILE_OK :
          skode_compile_program_describe(source, &program, hint, sizeof(hint));
        if (result == SKODE_COMPILE_OK) {
          seq_step_append(ctx->pattern, source, source_only ? NULL : &program);
        } else if (result == SKODE_COMPILE_IMMEDIATE_ONLY) {
          if (hint[0])
            ctx->printf(ctx,
              "# pattern %d step %d: '%s' is immediate-only and cannot be compiled into step [%s]\n"
              "# use 'ce N' in the step and bind N with '/ceb 4 N'\n", ctx->pattern, ctx->step, hint, source);
          else
            ctx->printf(ctx,
              "# pattern %d step %d: step [%s] contains an immediate-only word\n"
              "# use 'ce N' in the step and bind N with '/ceb 4 N'\n", ctx->pattern, ctx->step, source);
          return -1;
        } else {
          ctx->printf(ctx, "# pattern %d step %d: step could not be compiled (%d): [%s]\n",
            ctx->pattern, ctx->step, result, source);
          return -1;
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

static int word_exec_yp(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        /* yp N  — set pattern N as master (use -1 to disable sync) */
        seq_master_pattern_set(x);
        // ctx->printf(ctx, "# master pattern: %d\n", seq_master_pattern_get());
      } else {
        /* yp    — query current master pattern */
        int mp = seq_master_pattern_get();
        if (mp < 0)
          ctx->printf(ctx, "# master pattern: disabled\n");
        else
          ctx->printf(ctx, "# master pattern: %d\n", mp);
      }
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
      if (argc && x_valid && (x < 0 || x < PATTERNS_MAX)) {
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
      if (argc && x >= 0 && 1) {
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
        ctx->printf(ctx, "M%g %g\n", tempo_bpm_get(), tempo_subdivision_get());
        for (int p = 0; p < PATTERNS_MAX; p++) {
          if (seq_pattern_length[p] > 0 || seq_text[p][0] != '\0')
            pattern_show(ctx, p, 0);
        }
      }
      return 0;
}

static int word_exec_Z_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  (void)self; (void)ctx; (void)s; (void)arg; (void)argc;
  seq_rewind();
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
      ctx->printf(ctx, "M%g %g\n", tempo_bpm_get(), tempo_subdivision_get());
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
        int channels = 0;
        if (argc > 1) {
          skode_double_to_int(arg[1], &channels);
        }
        if (!filename || filename[0] == '\0') {
          ctx->printf(ctx, "# /rg requires [filename]\n");
        } else if (!isfinite(max_seconds) || max_seconds < 0.0) {
          ctx->printf(ctx, "# /rg duration must be >= 0\n");
        } else if (channels != 0 && channels != 2 && channels != RECORD_CHANNELS) {
          ctx->printf(ctx, "# /rg channels must be 2 (stereo) or %d (multitrack)\n", RECORD_CHANNELS);
        } else if (recorder_start_channels(filename, max_seconds, channels) == 0) {
          const char *mode = (channels == 2) ? "stereo" : "multitrack";
          if (max_seconds > 0.0) {
            ctx->printf(ctx, "# recording [%s] (%s) max=%g seconds\n",
                        filename, mode, max_seconds);
          } else {
            ctx->printf(ctx, "# recording [%s] (%s)\n", filename, mode);
          }
        } else {
          ctx->printf(ctx, "# recording start failed [%s]\n", filename);
        }
      }
      return 0;
}

static int word_exec__slashrw(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
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
          ctx->printf(ctx, "# /rw requires [filename]\n");
        } else if (!isfinite(max_seconds) || max_seconds < 0.0) {
          ctx->printf(ctx, "# /rw duration must be >= 0\n");
        } else if (recorder_start_channels(filename, max_seconds, 2) == 0) {
          if (max_seconds > 0.0) {
            ctx->printf(ctx, "# recording [%s] (stereo) max=%g seconds\n",
                        filename, max_seconds);
          } else {
            ctx->printf(ctx, "# recording [%s] (stereo)\n", filename);
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
        ctx->printf(ctx, "# recorder state=%s channels=%d frames=%llu dropped=%llu\n",
                    state,
                    recorder_channels(),
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
        rec_load(ctx, wave_slot, one_shot, channel, 0);
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
          // ctx->printf(ctx, "# ce bound %d,%d -> %s\n", x, key, ands_string(ctx->parse));
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
          // ctx->printf(ctx, "# ce bound %d,%d -> %s\n", type, key, command);
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

int skode_exec_DO_q(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_DO_q(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_DQ(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_DQ(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_xg(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_xg(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_ys_q(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_ys_q(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_yt(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_yt(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_z_q(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_z_q(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_z_q_bs_q(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_z_q_bs_q(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_Z_q(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_Z_q(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_qce(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__qce(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_qce_bang(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__qce_bang(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_qq(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__qq(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_qo(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__qo(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_slashrg(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__slashrg(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_slashrw(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__slashrw(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_slashrs(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__slashrs(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_slashr_q(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__slashr_q(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_slashr(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__slashr(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_slashcer(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__slashcer(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_slashce_q(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__slashce_q(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_slashce_bang(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__slashce_bang(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_slashceb(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__slashceb(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_slashcex(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__slashcex(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_gtr(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__gtr(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_hatr(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__hatr(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_ltr(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__ltr(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_y(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_y(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_ym(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_ym(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_zg(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_zg(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_zq(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_zq(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_Z_bang(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_Z_bang(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_pct(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__pct(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_z(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_z(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_Z(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_Z(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_yp(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_yp(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_Y(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_Y(NULL, ctx, s, (double*)arg, argc);
}

void skode_register_words_seq(skode_vocab_t *vocab) {
  (void)vocab;
  /* All sequencer/recording words migrated to .def files in doc/opcodes/ */
}
