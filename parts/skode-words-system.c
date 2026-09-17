#include "skode-internal.h"

static int word_exec__slashmd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc == 0) x = skred_midi_debug_get() ? 0 : 1;
      skred_midi_debug_set(x);
      ctx->printf(ctx, "# midi debug %s\n", skred_midi_debug_get() ? "on" : "off");
      return 0;
}

static int word_exec_wait(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (x_valid && x >= 0) sk_sleep(x);
      return 0;
}

static int word_exec_clr(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ands_arg_clear(s);
      return 1;
}

static int word_exec_drop(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ands_arg_drop(s);
      return 1;
}

static int word_exec_dup(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ands_arg_dup(s);
      return 1;
}

static int word_exec_over(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ands_arg_over(s);
      return 1;
}

static int word_exec_rot(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ands_arg_rot(s);
      return 1;
}

static int word_exec_swap(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ands_arg_swap(s);
      return 1;
}

static int word_exec_I(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {} return 0; // TODO en/dis-able send timestamp wire to the event logger
}

#ifdef UDP
static int word_exec_udp(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        ctx->printf(ctx, "# udp [%d] %d/%d\n", ctx->which, ctx->ip, ctx->port);
      }
      return 0;
}
#endif

static int word_exec_log(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        if (x) { ctx->log_enable = 1; } else { ctx->log_enable = 0; }
      }
      return 0;
}

static int word_exec_GS_gt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (!ands_string_fresh(ctx->parse) || !ands_string(ctx->parse)[0])
        ctx->printf(ctx, "# GS> requires [filename.zip]\n");
      else
        (void)skode_session_save(ctx, ands_string(ctx->parse));
      return 0;
}

static int word_exec_GS_lt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (!ands_string_fresh(ctx->parse) || !ands_string(ctx->parse)[0])
        ctx->printf(ctx, "# GS< requires [filename.zip]\n");
      else
        (void)skode_session_load(ctx, ands_string(ctx->parse));
      return 0;
}

static int word_exec_x(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        if (isnan(arg[0]) || !x_valid || x < 0) {
          ctx->step++;
          x = ctx->step;
        } else {
          ctx->step = x;
        }
        if (x >= 0 && x < SEQ_STEPS_MAX) {
          const char *source = ands_string(ctx->parse);
          event_program_t program;
          int source_only = source[0] == '\0' || source[0] == '-';
          char hint[16] = "";
          skode_compile_result_t result = source_only ?
            SKODE_COMPILE_OK :
            skode_compile_program_describe(source, &program, hint, sizeof(hint));
          if (result == SKODE_COMPILE_OK) {
            seq_step_set(ctx->pattern, ctx->step, source,
              source_only ? NULL : &program);
          } else if (result == SKODE_COMPILE_IMMEDIATE_ONLY) {
            if (hint[0])
              ctx->printf(ctx,
                "# '%s' is immediate-only and cannot be compiled into a pattern step\n"
                "# use 'ce N' in the step and bind N with '/ceb 4 N'\n", hint);
            else
              ctx->printf(ctx,
                "# step contains an immediate-only word (e.g. a string bracket or "
                "word like >u, e>, /ceb)\n"
                "# use 'ce N' in the step and bind N with '/ceb 4 N'\n");
          } else {
            ctx->printf(ctx, "# step could not be compiled (%d): [%s]\n",
              result, source);
          }
        }
      }
      return 0;
}

static int word_exec__slashm_(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      synth_voice_bench(voice);
      return 0;
}

static int word_exec__slashq(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->quit = -1;
      return 0;
}

static int word_exec__slashf(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) { ctx->flag = x; }
      else { ctx->printf(ctx, "# /f%d\n", ctx->flag); }
      return 0;
}

static int word_exec__slashff(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        if (!argc) return 0;
        int index;
        if (!skode_double_to_int(arg[0], &index) ||
            index < 0 || index >= SKRED_FOREIGN_FUNCTION_MAX) return 0;
        (void)skode_foreign_function(ctx, index, arg + 1, argc - 1);
      }
      return 0;
}

static int word_exec__slasht(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc == 0) x = (ctx->trace) ? 0 : 1;
      ctx->trace = x;
      ands_trace_set(s, x > 1);
      return 0;
}

static int word_exec__slashv(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc == 0) x = (ctx->verbose) ? 0 : 1;
      ctx->verbose = x;
      return 0;
}

static int word_exec__slashth_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "%s", skred_thread_status());
      return 0;
}

static int word_exec__slashth_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      skred_performance_reset();
      ctx->printf(ctx, "# performance counters reset\n");
      return 0;
}

static int word_exec__slashs(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        if (argc == 0) {
          system_show(ctx);
        } else {
          switch (x) {
            default:
            case 0: system_show(ctx); break;
            case 2: audio_show(ctx); break;
            case 3: ctx->printf(ctx, "%s", synth_stats()); break;
            case 5: skode_show(ctx); break;
            case 7:
              simple_mutex_lock(&skode_extra_mutex);
              for (int i=0; i<STRING_BUF_IDX_MAX; i++) {
                if (strlen(EXTRA_PTR(i)))
                  ctx->printf(ctx, "# [%s] e>%d\n", EXTRA_PTR(i), i);
              }
              simple_mutex_unlock(&skode_extra_mutex);
              break;
            case 1: show_threads(ctx); break;
            case 4: show_stats(ctx); break;
            case 6: ctx->printf(ctx, "%s", seq_stats()); break;
          }
        }
      }
      return 0;
}

static int word_exec__slashh(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      skode_help(ctx, arg, argc);
      return 0;
}

static int word_exec__slashl(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        int verbose = 0;
        if (argc > 1) skode_double_to_int(arg[1], &verbose);
        skode_load(ctx, voice, x, verbose);
      }
      return 0;
}

static int word_exec__slashls(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (strlen(ands_string(ctx->parse))) {
        int verbose = 0;
        if (argc > 0) skode_double_to_int(arg[0], &verbose);
        skode_load_name(ctx, ands_string(ctx->parse), verbose);
      } else {
        ctx->printf(ctx, "# /ls requires [filename]\n");
      }
      return 0;
}

static int word_exec_gt_u(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  (void)self; (void)s;
  const char *fmt = ands_string(ctx->parse);
  int a_idx = 0;
  char fallback_fmt[256] = "";
  
  if (strlen(fmt) == 0 && argc > 0) {
      int macro_idx = (int)arg[argc - 1];
      if (macro_idx >= 0 && macro_idx < 128 && skode_extra_copy(macro_idx, fallback_fmt, sizeof(fallback_fmt)) == 0) {
          fmt = fallback_fmt;
          argc--; // Consume the last argument so it isn't formatted
      }
  }

  if (fmt && strlen(fmt) > 0) {
      char buf[256];
      buf[0] = '\0';
      char *out = buf;
      int remaining = sizeof(buf) - 1;
      
      while (*fmt && remaining > 0) {
          if (*fmt == '%' && *(fmt+1) == 'g') {
              int n = snprintf(out, remaining, "%g", a_idx < argc ? arg[a_idx] : 0.0);
              if (n > 0) { out += n; remaining -= n; }
              a_idx++;
              fmt += 2;
          } else if (*fmt == '%' && *(fmt+1) == 'd') {
              int n = snprintf(out, remaining, "%d", a_idx < argc ? (int)arg[a_idx] : 0);
              if (n > 0) { out += n; remaining -= n; }
              a_idx++;
              fmt += 2;
          } else {
              *out++ = *fmt++;
              remaining--;
          }
      }
      *out = '\0';
      skred_udp_events_emit(buf);
  }
  return 1;
}

static int word_exec__slashws(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "# [%s] /ws\n", ands_string(ctx->parse));
      if (strlen(ands_string(ctx->parse))) {
        char *file_name = ands_string(ctx->parse);
        int wave_slot = EXT_SAMPLE_000;
        int ch = -1;
        if (argc >= 1) {
          if (!skode_double_to_int(arg[0], &wave_slot)) return 0;
          if (argc > 1) {
            if (!skode_double_to_int(arg[1], &ch)) ch = -1;
          }
        }
        ctx->printf(ctx, "# [%s] /ws %d %d\n", ands_string(ctx->parse), wave_slot, ch);
        wave_load_string(ctx, file_name, wave_slot, ch, 1);
      }
      return 0;
}

static int word_exec__slashw(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int file_num = 0;
        int wave_slot = EXT_SAMPLE_000;
        int ch = -1;
        if (argc >= 2) {
          if (!skode_double_to_int(arg[0], &file_num) ||
              !skode_double_to_int(arg[1], &wave_slot)) return 0;
          if (argc > 2 && !skode_double_to_int(arg[2], &ch)) ch = -1;
        } else if (argc == 1) {
          if (!skode_double_to_int(arg[0], &file_num)) return 0;
          wave_slot = EXT_SAMPLE_000;
        }
        if (argc) wave_load(ctx, file_num, wave_slot, ch, 1);
      }
      return 0;
}

static int word_exec__pctz(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (strlen(ands_string(ctx->parse))) {
        if (skred_vfs_mount(ands_string(ctx->parse)))
          ctx->printf(ctx, "# vfs %s\n", skred_vfs_status());
        else
          ctx->printf(ctx, "# cannot mount %s\n", ands_string(ctx->parse));
      } else {
        ctx->printf(ctx, "# %%z requires [zip-or-directory]\n");
      }
      return 0;
}

static int word_exec__pctzu(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      skred_vfs_unmount();
      ctx->printf(ctx, "# vfs %s\n", skred_vfs_status());
      return 0;
}

static int word_exec__pctpwd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "# vfs %s\n", skred_vfs_status());
      return 0;
}

static int word_exec__pctcat(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (strlen(ands_string(ctx->parse))) {
        void *data = NULL;
        size_t size = 0;
        char resolved[1024];
        if (skode_asset_read(ands_string(ctx->parse), SKODE_ASSET_ANY,
            &data, &size, resolved, sizeof(resolved))) {
          const char *text = (const char *)data;
          size_t pos = 0;
          while (pos < size) {
            char line[1024];
            size_t start = pos;
            size_t len;
            while (pos < size && text[pos] != '\n' && text[pos] != '\r') pos++;
            len = pos - start;
            while (pos < size && (text[pos] == '\n' || text[pos] == '\r')) pos++;
            if (len >= sizeof(line)) len = sizeof(line) - 1;
            memcpy(line, text + start, len);
            line[len] = '\0';
            for (size_t i = 0; i < len; i++) {
              if (!isprint((unsigned char)line[i]) && line[i] != '\t') {
                line[i] = '\0';
                break;
              }
            }
            ctx->printf(ctx, "%s\n", line);
          }
          skred_vfs_free_file(data);
        }
      }
      return 0;
}

static int word_exec__pctcd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "# [%s] %%cd\n", ands_string(ctx->parse));
      if (strlen(ands_string(ctx->parse))) {
        if (!skred_chdir(ands_string(ctx->parse)))
          ctx->printf(ctx, "# cannot cd %s\n", ands_string(ctx->parse));
      }
      ctx->printf(ctx, "# vfs %s\n", skred_vfs_status());
      return 0;
}

static int word_exec__pctls(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
      /*
          types
          0 = .sk
          1 = .wav
          2 = .mp3
          3 = .ks
          4 = .flac
          5 = .zip
          6 = .pnl
      */
      int p = -1;
      if (argc) p = x;
      SkredDirent *entry;
      SkredDir *dp = skred_opendir(".");
        if (dp) {
          int index = -100;
          if (argc > 1) {
            index = (int)arg[1];
          }
          int count = 0;
          while ((entry = skred_readdir(dp))) {
            char *name = entry->d_name;
            int f = 0;
            switch (p) {
              default:
              case -1:
                f = 1;
                break;
              case 0:
                f = (strstr(name, ".sk") != NULL);
                break;
              case 1:
                f = (strstr(name, ".wav") != NULL);
                break;
              case 2:
                f = (strstr(name, ".mp3") != NULL);
                break;
              case 3:
                f = (strstr(name, ".ks") != NULL);
                break;
              case 4:
                f = (strstr(name, ".flac") != NULL);
                break;
              case 5:
                f = (strstr(name, ".zip") != NULL);
                break;
              case 6:
                f = (strstr(name, ".pnl") != NULL);
                break;
            }
            if (f) {
              if (index == -100) {
                ctx->printf(ctx,
                  "# [%s%s] # %d\n",
                  name, entry->is_directory ? "/" : "", count);
              }
              if (index == count) {
                ctx->printf(ctx, "# [%s%s] # %d\n",
                  name, entry->is_directory ? "/" : "", index);
                ands_string_from_external(ctx->parse, name, strlen(name));
              }
              count++;
              if (index == -1) {
                if (rand() % count == 0) {
                  ands_string_from_external(ctx->parse, name, strlen(name));
                }
              }
            }
          }
          skred_closedir(dp);
        }
      }
      return 0;
}


// Word declarations
static skode_word_t word__slashmd = { WID("/md"), .execute = word_exec__slashmd, .safety = WORD_IMMEDIATE_ONLY , .category = "runtime" };
static skode_word_t word_wait = { WID("wait"), .execute = word_exec_wait, .safety = WORD_IMMEDIATE_ONLY , .category = "parser" };
static skode_word_t word_clr = { WID("clr"), .execute = word_exec_clr, .safety = WORD_IMMEDIATE_ONLY , .category = "parser" };
static skode_word_t word_drop = { WID("drop"), .execute = word_exec_drop, .safety = WORD_IMMEDIATE_ONLY , .category = "parser" };
static skode_word_t word_dup = { WID("dup"), .execute = word_exec_dup, .safety = WORD_IMMEDIATE_ONLY , .category = "parser" };
static skode_word_t word_over = { WID("over"), .execute = word_exec_over, .safety = WORD_IMMEDIATE_ONLY , .category = "parser" };
static skode_word_t word_rot = { WID("rot"), .execute = word_exec_rot, .safety = WORD_IMMEDIATE_ONLY , .category = "parser" };
static skode_word_t word_swap = { WID("swap"), .execute = word_exec_swap, .safety = WORD_IMMEDIATE_ONLY , .category = "parser" };
static skode_word_t word_I = { WID("I"), .execute = word_exec_I, .safety = WORD_IMMEDIATE_ONLY , .category = "runtime" };
#ifdef UDP
static skode_word_t word_udp = { WID("udp"), .execute = word_exec_udp, .safety = WORD_IMMEDIATE_ONLY , .category = "runtime" };
#endif
static skode_word_t word_log = { WID("log"), .execute = word_exec_log, .safety = WORD_IMMEDIATE_ONLY , .category = "runtime" };
static skode_word_t word_GS_gt = { WID("GS>"), .execute = word_exec_GS_gt, .safety = WORD_IMMEDIATE_ONLY , .category = "files" };
static skode_word_t word_GS_lt = { WID("GS<"), .execute = word_exec_GS_lt, .safety = WORD_IMMEDIATE_ONLY , .category = "files" };
static skode_word_t word_x = { WID("x"), .execute = word_exec_x, .safety = WORD_IMMEDIATE_ONLY , .category = "parser" };
static skode_word_t word__slashm_ = { WID("/m_"), .execute = word_exec__slashm_, .safety = WORD_IMMEDIATE_ONLY , .category = "runtime" };
static skode_word_t word__slashq = { WID("/q"), .execute = word_exec__slashq, .safety = WORD_IMMEDIATE_ONLY , .category = "runtime" };
static skode_word_t word__slashf = { WID("/f"), .execute = word_exec__slashf, .safety = WORD_IMMEDIATE_ONLY , .category = "runtime" };
static skode_word_t word__slashff = { WID("/ff"), .execute = word_exec__slashff, .safety = WORD_IMMEDIATE_ONLY , .category = "runtime" };
static skode_word_t word__slasht = { WID("/t"), .execute = word_exec__slasht, .safety = WORD_IMMEDIATE_ONLY , .category = "runtime" };
static skode_word_t word__slashv = { WID("/v"), .execute = word_exec__slashv, .safety = WORD_IMMEDIATE_ONLY , .category = "runtime" };
static skode_word_t word__slashth_q = { WID("/th?"), .execute = word_exec__slashth_q, .safety = WORD_IMMEDIATE_ONLY , .category = "runtime" };
static skode_word_t word__slashth_bang = { WID("/th!"), .execute = word_exec__slashth_bang, .safety = WORD_IMMEDIATE_ONLY , .category = "runtime" };
static skode_word_t word__slashs = { WID("/s"), .execute = word_exec__slashs, .safety = WORD_IMMEDIATE_ONLY , .category = "runtime" };
static skode_word_t word__slashh = { WID("/h"), .execute = word_exec__slashh, .safety = WORD_IMMEDIATE_ONLY , .category = "runtime" };
static skode_word_t word__slashl = { WID("/l"), .execute = word_exec__slashl, .safety = WORD_IMMEDIATE_ONLY , .category = "files" };
static skode_word_t word__slashls = { WID("/ls"), .execute = word_exec__slashls, .safety = WORD_IMMEDIATE_ONLY , .category = "files" };
static skode_word_t word__slashws = { WID("/ws"), .execute = word_exec__slashws, .safety = WORD_IMMEDIATE_ONLY , .category = "files" };
static skode_word_t word__slashw = { WID("/w"), .execute = word_exec__slashw, .safety = WORD_IMMEDIATE_ONLY , .category = "files" };
static skode_word_t word__pctz = { WID("%z"), .execute = word_exec__pctz, .safety = WORD_IMMEDIATE_ONLY , .category = "files" };
static skode_word_t word__pctzu = { WID("%zu"), .execute = word_exec__pctzu, .safety = WORD_IMMEDIATE_ONLY , .category = "files" };
static skode_word_t word__pctpwd = { WID("%pwd"), .execute = word_exec__pctpwd, .safety = WORD_IMMEDIATE_ONLY , .category = "files" };
static skode_word_t word__pctcat = { WID("%cat"), .execute = word_exec__pctcat, .safety = WORD_IMMEDIATE_ONLY , .category = "files" };
static skode_word_t word__pctcd = { WID("%cd"), .execute = word_exec__pctcd, .safety = WORD_IMMEDIATE_ONLY , .category = "files" };
static skode_word_t word__pctls = { WID("%ls"), .execute = word_exec__pctls, .safety = WORD_IMMEDIATE_ONLY , .category = "files" };
static int word_exec__qtime(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  (void)self; (void)ctx; (void)arg; (void)argc;
  ands_return_push(s, (double)SAMPLE_COUNT_GET());
  return 0;
}

static int word_exec__qvoice(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  (void)self; (void)arg; (void)argc;
  ands_return_push(s, (double)ctx->voice);
  return 0;
}

static int word_exec__qpattern(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  (void)self; (void)arg; (void)argc;
  ands_return_push(s, (double)ctx->pattern);
  return 0;
}

static int word_exec__qstep(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  (void)self; (void)arg; (void)argc;
  ands_return_push(s, (double)ctx->step);
  return 0;
}

static skode_word_t word_gt_u = { WID(">u"), .execute = word_exec_gt_u, .safety = WORD_IMMEDIATE_ONLY, .category = "events" };
static skode_word_t word__qt = { WID("?t"), .execute = word_exec__qtime, .safety = WORD_IMMEDIATE_ONLY, .category = "events" };
static skode_word_t word__qv = { WID("?v"), .execute = word_exec__qvoice, .safety = WORD_IMMEDIATE_ONLY, .category = "events" };
static skode_word_t word__qp = { WID("?p"), .execute = word_exec__qpattern, .safety = WORD_IMMEDIATE_ONLY, .category = "events" };
static skode_word_t word__qs = { WID("?st"), .execute = word_exec__qstep, .safety = WORD_IMMEDIATE_ONLY, .category = "events" };

void skode_register_words_system(skode_vocab_t *vocab) {
  skode_dict_register(vocab, &word__slashmd);
  skode_dict_register(vocab, &word_wait);
  skode_dict_register(vocab, &word_clr);
  skode_dict_register(vocab, &word_drop);
  skode_dict_register(vocab, &word_dup);
  skode_dict_register(vocab, &word_over);
  skode_dict_register(vocab, &word_rot);
  skode_dict_register(vocab, &word_swap);
  skode_dict_register(vocab, &word_I);
#ifdef UDP
  skode_dict_register(vocab, &word_udp);
  skode_dict_register(vocab, &word_gt_u);
  skode_dict_register(vocab, &word__qt);
  skode_dict_register(vocab, &word__qv);
  skode_dict_register(vocab, &word__qp);
  skode_dict_register(vocab, &word__qs);
#endif
  skode_dict_register(vocab, &word_log);
  skode_dict_register(vocab, &word_GS_gt);
  skode_dict_register(vocab, &word_GS_lt);
  skode_dict_register(vocab, &word_x);
  skode_dict_register(vocab, &word__slashm_);
  skode_dict_register(vocab, &word__slashq);
  skode_dict_register(vocab, &word__slashf);
  skode_dict_register(vocab, &word__slashff);
  skode_dict_register(vocab, &word__slasht);
  skode_dict_register(vocab, &word__slashv);
  skode_dict_register(vocab, &word__slashth_q);
  skode_dict_register(vocab, &word__slashth_bang);
  skode_dict_register(vocab, &word__slashs);
  skode_dict_register(vocab, &word__slashh);
  skode_dict_register(vocab, &word__slashl);
  skode_dict_register(vocab, &word__slashls);
  skode_dict_register(vocab, &word__slashws);
  skode_dict_register(vocab, &word__slashw);
  skode_dict_register(vocab, &word__pctz);
  skode_dict_register(vocab, &word__pctzu);
  skode_dict_register(vocab, &word__pctpwd);
  skode_dict_register(vocab, &word__pctcat);
  skode_dict_register(vocab, &word__pctcd);
  skode_dict_register(vocab, &word__pctls);
}
