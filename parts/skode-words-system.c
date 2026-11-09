#include "skode-internal.h"
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

int skode_exec_slashmd(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s;
  int x = 0;
  if (argc == 0) x = skred_midi_debug_get() ? 0 : 1;
  else if (!skode_double_to_int(arg[0], &x)) return 0;
  skred_midi_debug_set(x);
  ctx->printf(ctx, "# midi debug %s\n", skred_midi_debug_get() ? "on" : "off");
  return 0;
}

int skode_exec_wait(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)ctx; (void)s;
  int x = 0;
  if (argc > 0 && skode_double_to_int(arg[0], &x) && x >= 0) {
    sk_sleep(x);
  }
  return 0;
}

int skode_exec_clr(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)ctx; (void)arg; (void)argc;
  ands_arg_clear(s);
  return 1;
}

int skode_exec_drop(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)ctx; (void)arg; (void)argc;
  ands_arg_drop(s);
  return 1;
}

int skode_exec_dup(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)ctx; (void)arg; (void)argc;
  ands_arg_dup(s);
  return 1;
}

int skode_exec_over(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)ctx; (void)arg; (void)argc;
  ands_arg_over(s);
  return 1;
}

int skode_exec_rot(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)ctx; (void)arg; (void)argc;
  ands_arg_rot(s);
  return 1;
}

int skode_exec_swap(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)ctx; (void)arg; (void)argc;
  ands_arg_swap(s);
  return 1;
}

int skode_exec_I(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)ctx; (void)s; (void)arg; (void)argc;
  return 0;
}

int skode_exec_udp(skode_t *ctx, ands_t *s, const double *arg, int argc) {
#ifdef UDP
  (void)s;
  if (argc) {
    ctx->printf(ctx, "# udp [%d] %d/%d\n", ctx->which, ctx->ip, ctx->port);
  }
#else
  (void)ctx; (void)s; (void)arg; (void)argc;
#endif
  return 0;
}

int skode_exec_puts(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)arg; (void)argc; (void)s;
  const char *str = ands_string(ctx->parse);
  if (str && str[0] != '\0') {
    ctx->printf(ctx, "# %s\n", str);
  } else {
    ctx->printf(ctx, "#\n");
  }
  return 0;
}

int skode_exec_log(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s;
  int x = 0;
  if (argc > 0 && skode_double_to_int(arg[0], &x)) {
    ctx->log_enable = x ? 1 : 0;
  }
  return 0;
}

int skode_exec_GS_gt(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s; (void)arg; (void)argc;
  if (!ands_string_fresh(ctx->parse) || !ands_string(ctx->parse)[0])
    ctx->printf(ctx, "# GS> requires [filename.zip]\n");
  else
    (void)skode_session_save(ctx, ands_string(ctx->parse));
  return 0;
}

int skode_exec_GS_lt(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s; (void)arg; (void)argc;
  if (!ands_string_fresh(ctx->parse) || !ands_string(ctx->parse)[0])
    ctx->printf(ctx, "# GS< requires [filename.zip]\n");
  else
    (void)skode_session_load(ctx, ands_string(ctx->parse));
  return 0;
}

int skode_exec_x(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s;
  if (argc) {
    const char *source = ands_string(ctx->parse);
    event_program_t program;
    int source_only = source[0] == '\0' || source[0] == '-';
    char hint[16] = "";
    skode_compile_result_t result = source_only ?
      SKODE_COMPILE_OK :
      skode_compile_program_describe(source, &program, hint, sizeof(hint));
    if (result == SKODE_COMPILE_OK) {
      for (int i = 0; i < argc; i++) {
        int x = 0;
        int x_valid = skode_double_to_int(arg[i], &x);
        if (isnan(arg[i]) || !x_valid || x < 0) {
          ctx->step++;
          x = ctx->step;
        } else {
          ctx->step = x;
        }
        if (x >= 0) {
          seq_step_set(ctx->pattern, ctx->step, source,
            source_only ? NULL : &program);
        }
      }
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

int skode_exec_x_data(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s;
  if (!ctx) return -1;
  int target_pattern = ctx->pattern;
  if (argc > 0) {
    int p = 0;
    if (skode_double_to_int(arg[0], &p) && p >= 0 && p < PATTERNS_MAX) {
      target_pattern = p;
    }
  }

  double *data = ands_data(ctx->parse);
  int data_len = ands_data_len(ctx->parse);
  if (!data || data_len <= 0) return 0;

  const char *source = ands_string(ctx->parse);
  event_program_t program;
  int source_only = source[0] == '\0' || source[0] == '-';
  char hint[16] = "";
  skode_compile_result_t result = source_only ?
    SKODE_COMPILE_OK :
    skode_compile_program_describe(source, &program, hint, sizeof(hint));
  if (result == SKODE_COMPILE_OK) {
    for (int i = 0; i < data_len; i++) {
      int step = 0;
      int step_valid = skode_double_to_int(data[i], &step);
      if (isnan(data[i]) || !step_valid || step < 0) {
        ctx->step++;
        step = ctx->step;
      } else {
        ctx->step = step;
      }
      if (step >= 0) {
        seq_step_set(target_pattern, step, source,
          source_only ? NULL : &program);
      }
    }
  } else if (result == SKODE_COMPILE_IMMEDIATE_ONLY) {
    if (hint[0])
      ctx->printf(ctx,
        "# pattern %d: '%s' is immediate-only and cannot be compiled into step [%s]\n"
        "# use 'ce N' in the step and bind N with '/ceb 4 N'\n", target_pattern, hint, source);
    else
      ctx->printf(ctx,
        "# pattern %d: step [%s] contains an immediate-only word\n"
        "# use 'ce N' in the step and bind N with '/ceb 4 N'\n", target_pattern, source);
    return -1;
  } else {
    ctx->printf(ctx, "# pattern %d: step could not be compiled (%d): [%s]\n",
      target_pattern, result, source);
    return -1;
  }
  return 0;
}

int skode_exec_slashm_(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s; (void)arg; (void)argc;
  synth_voice_bench(ctx->voice);
  return 0;
}

int skode_exec_slashq(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s; (void)arg; (void)argc;
  ctx->quit = -1;
  return 0;
}

int skode_exec_slashf(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s;
  int x = 0;
  if (argc > 0 && skode_double_to_int(arg[0], &x)) {
    ctx->flag = x;
  } else {
    ctx->printf(ctx, "# /f%d\n", ctx->flag);
  }
  return 0;
}

int skode_exec_slashff(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s;
  if (!argc) return 0;
  int index;
  if (!skode_double_to_int(arg[0], &index) ||
      index < 0 || index >= SKRED_FOREIGN_FUNCTION_MAX) return 0;
  (void)skode_foreign_function(ctx, index, (double *)arg + 1, argc - 1);
  return 0;
}

int skode_exec_slasht(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  int x = 0;
  if (argc == 0) x = (ctx->trace) ? 0 : 1;
  else if (!skode_double_to_int(arg[0], &x)) return 0;
  ctx->trace = x;
  ands_trace_set(s, x > 1);
  return 0;
}

int skode_exec_slashv(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s;
  int x = 0;
  if (argc == 0) x = (ctx->verbose) ? 0 : 1;
  else if (!skode_double_to_int(arg[0], &x)) return 0;
  ctx->verbose = x;
  return 0;
}

int skode_exec_slashth_q(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s; (void)arg; (void)argc;
  ctx->printf(ctx, "%s", skred_thread_status());
  return 0;
}

int skode_exec_slashth_bang(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s; (void)arg; (void)argc;
  skred_performance_reset();
  ctx->printf(ctx, "# performance counters reset\n");
  return 0;
}

int skode_exec_slashs(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s;
  int x = 0;
  if (argc == 0) {
    system_show(ctx);
  } else {
    if (!skode_double_to_int(arg[0], &x)) x = 0;
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
  return 0;
}

int skode_exec_slashh(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s;
  skode_help(ctx, (double *)arg, argc);
  return 0;
}

int skode_exec_slashl(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s;
  if (argc) {
    int x = 0;
    int verbose = 0;
    if (!skode_double_to_int(arg[0], &x)) return 0;
    if (argc > 1) skode_double_to_int(arg[1], &verbose);
    skode_load(ctx, ctx->voice, x, verbose);
  }
  return 0;
}

int skode_exec_slashls(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s;
  if (strlen(ands_string(ctx->parse))) {
    int verbose = 0;
    if (argc > 0) skode_double_to_int(arg[0], &verbose);
    skode_load_name(ctx, ands_string(ctx->parse), verbose);
  } else {
    ctx->printf(ctx, "# /ls requires [filename]\n");
  }
  return 0;
}

int skode_exec_gt_u(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s;
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
#ifdef UDP
      skred_udp_events_emit(buf);
#else
      (void)buf;
#endif
  }
  return 1;
}

int skode_exec_slashws(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s;
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
    wave_load_string(ctx, file_name, wave_slot, ch, 1);
  }
  return 0;
}

int skode_exec_slashw(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s;
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
  return 0;
}

int skode_exec_pctz(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s;
  int verbose = 0;
  if (argc > 0) skode_double_to_int(arg[0], &verbose);
  if (strlen(ands_string(ctx->parse))) {
    if (skred_vfs_mount(ands_string(ctx->parse))) {
      ctx->printf(ctx, "# vfs %s\n", skred_vfs_status());
      void *data = NULL;
      size_t size = 0;
      char resolved[1024];
      if (skode_asset_read("main.sk", SKODE_ASSET_SKODE, &data, &size, resolved, sizeof(resolved))) {
        ctx->printf(ctx, "# auto-loading %s\n", resolved[0] ? resolved : "main.sk");
        skode_load_buffer(ctx, (const char *)data, size, resolved[0] ? resolved : "main.sk", verbose);
        skred_vfs_free_file(data);
      }
    } else {
      ctx->printf(ctx, "# cannot mount %s\n", ands_string(ctx->parse));
    }
  } else {
    ctx->printf(ctx, "# %%z requires [zip-or-directory]\n");
  }
  return 0;
}

int skode_exec_pctzu(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s; (void)arg; (void)argc;
  skred_vfs_unmount();
  ctx->printf(ctx, "# vfs %s\n", skred_vfs_status());
  return 0;
}

int skode_exec_pctpwd(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s; (void)arg; (void)argc;
  ctx->printf(ctx, "# vfs %s\n", skred_vfs_status());
  return 0;
}

int skode_exec_pctcat(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s; (void)arg; (void)argc;
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

int skode_exec_pctcd(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s; (void)arg; (void)argc;
  ctx->printf(ctx, "# [%s] %%cd\n", ands_string(ctx->parse));
  if (strlen(ands_string(ctx->parse))) {
    if (!skred_chdir(ands_string(ctx->parse)))
      ctx->printf(ctx, "# cannot cd %s\n", ands_string(ctx->parse));
  }
  ctx->printf(ctx, "# vfs %s\n", skred_vfs_status());
  return 0;
}

int skode_exec_pctls(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)s;
  int p = -1;
  int x = 0;
  if (argc && skode_double_to_int(arg[0], &x)) p = x;
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
        case -1: f = 1; break;
        case 0: f = (strstr(name, ".sk") != NULL); break;
        case 1: f = (strstr(name, ".wav") != NULL); break;
        case 2: f = (strstr(name, ".mp3") != NULL); break;
        case 3: f = (strstr(name, ".ks") != NULL); break;
        case 4: f = (strstr(name, ".flac") != NULL); break;
        case 5: f = (strstr(name, ".zip") != NULL); break;
        case 6: f = (strstr(name, ".pnl") != NULL); break;
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
  return 0;
}

int skode_exec_qtime(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)ctx; (void)arg; (void)argc;
  ands_return_push(s, (double)SAMPLE_COUNT_GET());
  return 0;
}

int skode_exec_qvoice(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)arg; (void)argc;
  ands_return_push(s, (double)ctx->voice);
  return 0;
}

int skode_exec_qpattern(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)arg; (void)argc;
  ands_return_push(s, (double)ctx->pattern);
  return 0;
}

int skode_exec_qstep(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  (void)arg; (void)argc;
  ands_return_push(s, (double)ctx->step);
  return 0;
}

void skode_register_words_system(skode_vocab_t *vocab) {
  (void)vocab;
}
