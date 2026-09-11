#include "skode-internal.h"

static int word_exec_D(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        if (x > ands_data_cap(ctx->parse)) ands_data_resize(ctx->parse, x);
      } else {
        ctx->printf(ctx, "# D[%d]\n", ands_data_cap(ctx->parse));
      }
      return 0;
}

static int word_exec__qd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        double *data = ands_data(ctx->parse);
        int data_len = ands_data_len(ctx->parse);
        skode_double_dump(ctx, data, data_len);
      }
      return 0;
}

static int word_exec__slashD(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        // free and re-allocate...
        if (x > 0) ands_data_resize(ctx->parse, x);
      }
      ctx->printf(ctx, "# /D data %p cap %d |%d|\n",
        ands_data(ctx->parse),
        ands_data_cap(ctx->parse),
        ands_data_len(ctx->parse));
      return 0;
}

static int word_exec__slashks(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        char *file = ands_string(ctx->parse);
        int verbose = 0;
        if (argc) skode_double_to_int(arg[0], &verbose);
        if (strlen(file)) {
          ksynth_load_name(ctx, file, verbose);
        }
      }
      return 0;
}

static int word_exec__slashk(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
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
        ksynth_load(ctx, x, verbose);
      }
      return 0;
}

static int word_exec_ks(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

      {
        int len = 0;
        char *cmd = ands_string(ctx->parse);
        if (cmd) len = strlen(cmd);
        if (ctx->trace) {
          ctx->printf(ctx, "cmd:[%s] len:%d\n", cmd, len);
        }
        // if (len) skode_ks_eval(ctx, cmd, len);
        if (len) ksynth_loader(ctx, cmd, (size_t)len, "[inline ks]", ctx->trace);
      }
      return 0;
}

static int word_exec_k_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int len = 0;
        char *cmd = ands_string(ctx->parse);
        if (cmd) len = strlen(cmd);
        if (ctx->trace) {
          ctx->printf(ctx, "cmd:[%s] len:%d\n", cmd, len);
        }
        //if (len) skode_ks_eval(ctx, cmd, len);
        if (len) ksynth_loader(ctx, cmd, (size_t)len, "[inline k!]", ctx->trace);
      }
      return 0;
}

static int word_exec_kw(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        (void)x;
      }
      return 0;
}

static int word_exec_kw_gt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        (void)x;
        skode_ks_result_to_data(ctx);
      }
      return 0;
}

static int word_exec_k_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        K result = (K)ctx->ks_result;
        if (result && !k_is_func(result))
          skode_double_dump(ctx, result->f, (size_t)result->n);
      }
      return 0;
}

static int word_exec_k_gtd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        skode_ks_result_to_data(ctx);
      }
      return 0;
}

static int word_exec_k_gtw(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int wave_slot = EXT_SAMPLE_000;
        int one_shot = 0;
        float rate = (float)MAIN_SAMPLE_RATE;
        float offset = 0.0f;
        if (argc && !skode_double_to_int(arg[0], &wave_slot)) return 0;
        if (argc > 1) rate = arg[1];
        if (argc > 2) skode_double_to_int(arg[2], &one_shot);
        if (argc > 3) offset = arg[3];
        if (skode_ks_result_to_data(ctx))
          data_load(ctx, wave_slot, one_shot, rate, offset);
      }
      return 0;
}

static int word_exec__eqd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 1 && x_valid) {
        int y;
        double *data = ands_data(ctx->parse);
        int data_len = ands_data_len(ctx->parse);
        if (skode_double_to_int(arg[1], &y) &&
            x >= 0 && x < 128 && y >= 0 && y < data_len) {
          // x is the dest var y is the d index
          ands_set_local(ctx->parse, x, data[y]);
        }
      }
      return 0;
}

static int word_exec_d_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  (void)self; (void)atom; (void)voice;
  if (argc > 1) {
    double val = arg[0];
    int index = -1;
    if (skode_double_to_int(arg[1], &index)) {
      double *data = ands_data(ctx->parse);
      int data_len = ands_data_len(ctx->parse);
      int data_cap = ands_data_cap(ctx->parse);
      if (index >= 0 && index < data_cap) {
        if (index >= data_len) {
          for (int i = data_len; i <= index; i++) {
            data[i] = 0.0;
          }
          ands_data_len_set(ctx->parse, index + 1);
        }
        data[index] = val;
      }
    }
  }
  return 0;
}

static int word_exec_d_star(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        double *data = ands_data(ctx->parse);
        int data_len = ands_data_len(ctx->parse);
        if (x>=0 && x < data_len) {
          double val = data[x];
          ctx->printf(ctx, "# %g\n", val);
          ands_arg_clear(s);
          ands_arg_push(s, val);
          return 1;
        }
      }
      return 0;
}

static int word_exec_d_gtr(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        double *data = ands_data(ctx->parse);
        int data_len = ands_data_len(ctx->parse);
        if (!data || data_len <= 0) return 0;
        int sample_state = atomic_load_int(&sampling.state);
        if (sample_state == SAMPLE_STATE_ARMED ||
            sample_state == SAMPLE_STATE_RECORDING) {
          ctx->printf(ctx, "# recording buffer busy\n");
          return 0;
        }
        if (data_len > sampling.capacity) skode_sample_alloc(data_len);
        if (!sampling.where || data_len > sampling.capacity) {
          ctx->printf(ctx, "# recording buffer allocation failed\n");
          return 0;
        }
        for (int i=0; i<data_len; i++) sampling.where[i] = (float)data[i];
        sampling.len = data_len;
        sampling.channels = 1;
        sampling.offset = 0;
        sampling.trim = 0;
        atomic_store_int(&sampling.state, SAMPLE_STATE_COMPLETE);
      }
      return 0;
}

static int word_exec_r_gtd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int channel = -1;
        if (argc > 1) {
          ctx->printf(ctx, "# usage: r>d [channel]\n");
          return 0;
        }
        if (argc == 1 && !skode_double_to_int(arg[0], &channel)) return 0;
        if (atomic_load_int(&sampling.state) != SAMPLE_STATE_COMPLETE) {
          ctx->printf(ctx, "# recording buffer is not complete\n");
          return 0;
        }
        if (!sampling.where || sampling.len > sampling.capacity ||
            sampling.offset < 0 || sampling.trim < 0 ||
            sampling.offset > sampling.len ||
            sampling.trim > sampling.len - sampling.offset) {
          ctx->printf(ctx, "# invalid recording bounds\n");
          return 0;
        }
        int channels = sampling.channels == 2 ? 2 : 1;
        if (channel < -1 || channel >= channels) {
          ctx->printf(ctx, "# recording channel must be -1..%d\n",
                      channels - 1);
          return 0;
        }
        int data_len = sampling.len - sampling.offset - sampling.trim;
        if (data_len <= 0) {
          ctx->printf(ctx, "# recording buffer is empty\n");
          return 0;
        }
        if (data_len > ands_data_cap(ctx->parse))
          ands_data_resize(ctx->parse, data_len);
        double *data = ands_data(ctx->parse);
        if (!data || data_len > ands_data_cap(ctx->parse)) {
          ctx->printf(ctx, "# data array allocation failed\n");
          return 0;
        }
        for (int i = 0; i < data_len; i++) {
          size_t frame =
            (size_t)(sampling.offset + i) * (size_t)channels;
          if (channels == 1) {
            data[i] = sampling.where[frame];
          } else if (channel >= 0) {
            data[i] = sampling.where[frame + (size_t)channel];
          } else {
            data[i] = 0.5 * (sampling.where[frame] +
                             sampling.where[frame + 1]);
          }
        }
        ands_data_len_set(ctx->parse, data_len);
      }
      return 0;
}

static int word_exec_d_gtk(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        int variable;
        if (skode_double_to_int(arg[0], &variable)) {
          skode_ks_bind_values(ctx, variable, ands_data(ctx->parse),
                               (size_t)ands_data_len(ctx->parse));
        }
      }
      return 0;
}

static int word_exec_w_gtk(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 1) {
        int wave;
        int variable;
        if (!skode_double_to_int(arg[0], &wave) ||
            !skode_double_to_int(arg[1], &variable) ||
            !skode_wave_valid(wave) || !sw.data[wave] || sw.size[wave] <= 0) {
          ctx->printf(ctx, "# invalid wavetable for w>k\n");
          return 0;
        }
        size_t len = (size_t)sw.size[wave];
        if (len > 1000000 || len > SIZE_MAX / sizeof(double)) {
          ctx->printf(ctx, "# ksynth vector too large: %zu\n", len);
          return 0;
        }
        double *values = malloc(len * sizeof(double));
        if (!values) {
          ctx->printf(ctx, "# allocation failed\n");
          return 0;
        }
        for (size_t i = 0; i < len; i++) values[i] = sw.data[wave][i];
        skode_ks_bind_values(ctx, variable, values, len);
        free(values);
      }
      return 0;
}

static int word_exec_s_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && x_valid) {
        if (x >= 0 && x < SKODE_STRING_SLOT_MAX)
          ctx->printf(ctx, "# s%d [%s]\n", x, ctx->string_slot[x]);
      } else {
        for (int i = 0; i < SKODE_STRING_SLOT_MAX; i++) {
          if (ctx->string_slot[i][0])
            ctx->printf(ctx, "# s%d [%s]\n", i, ctx->string_slot[i]);
        }
      }
      return 0;
}

static int word_exec__qm(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      skode_macros_show(ctx, 0);
      return 0;
}

static int word_exec__slashd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int wave_slot = EXT_SAMPLE_000;
        int one_shot = 0;
        float rate = (float)MAIN_SAMPLE_RATE;
        float offset = 0.0;
        if (argc && !skode_double_to_int(arg[0], &wave_slot)) return 0;
        if (argc > 1) rate = arg[1];
        if (argc > 2) skode_double_to_int(arg[2], &one_shot);
        if (argc > 3) offset = arg[3];
        data_load(ctx, wave_slot, one_shot, rate, offset);
      }
      return 0;
}

static int word_exec__slashm(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        const char *name = ands_string_fresh(ctx->parse) ? ands_string(ctx->parse) : "";
        if (name && name[0]) {
          int removed = ands_macro_remove(ctx->parse, name);
          ctx->printf(ctx, "# macro [%s] %s\n", name, removed ? "removed" : "not found");
        } else {
          ctx->printf(ctx, "# /m requires [name]\n");
        }
      }
      return 0;
}

static int word_exec__slashm_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ands_macro_clear(ctx->parse);
      ctx->printf(ctx, "# macros cleared\n");
      return 0;
}

static int word_exec__lts(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && x_valid && x >= 0 && x < SKODE_STRING_SLOT_MAX) {
        ands_string_from_external(ctx->parse, ctx->string_slot[x],
                                  strlen(ctx->string_slot[x]));
      }
      return 0;
}

static int word_exec_s_gt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && x_valid && x >= 0 && x < SKODE_STRING_SLOT_MAX) {
        skode_copy_string(ctx->string_slot[x], SKODE_STRING_SLOT_LEN,
                          ands_string(ctx->parse));
      }
      return 0;
}

static int word_exec_s_pct(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        char formatted[SKODE_STRING_SLOT_LEN];
        skode_format_string_args(formatted, sizeof(formatted),
                                 ands_string(ctx->parse), arg, argc);
        ands_string_from_external(ctx->parse, formatted, strlen(formatted));
        return 1;
      }
}

static int word_exec__lte(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && skode_extra_valid(x)) {
        char macro[STRING_BUF_LEN];
        if (skode_extra_copy(x, macro, sizeof(macro)) == 0)
          ands_string_from_external(ctx->parse, macro, strlen(macro));
      }
      return 0;
}

static int word_exec_e_gt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && skode_extra_valid(x)) {
        char *s = ands_string(ctx->parse);
        simple_mutex_lock(&skode_extra_mutex);
        skode_copy_string(EXTRA_PTR(x), STRING_BUF_LEN, s);
        simple_mutex_unlock(&skode_extra_mutex);
      }
      return 0;
}

static int word_exec_e_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        char macro[STRING_BUF_LEN] = "";
        const char *s = "";
        if (argc == 0) {
          s = ands_string(ctx->parse);
        } else if (skode_extra_copy(x, macro, sizeof(macro)) == 0) {
          s = macro;
        }
        if (s[0] != '\0') {
          event_program_t program;
          if (!skode_compile_scheduled(ctx, s, &program)) return 0;
          uint64_t now = SAMPLE_COUNT_GET();
          int tag = 0;
          skode_queue_program(&program, voice, now, tag);
        }
      }
      return 0;
}

static int word_exec_e_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      simple_mutex_lock(&skode_extra_mutex);
      if (argc) {
        if (skode_extra_valid(x)) ctx->printf(ctx, "# [%s] e>%d\n", EXTRA_PTR(x), x);
      } else {
        for (int i=0; i<STRING_BUF_IDX_MAX; i++) {
          if (strlen(EXTRA_PTR(i)))
            ctx->printf(ctx, "# [%s] e>%d\n", EXTRA_PTR(i), i);
        }
      }
      simple_mutex_unlock(&skode_extra_mutex);
      return 0;
}

static int word_exec_W_star(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 1 && x_valid && skode_wave_valid(x)) {
        int wave = x;
        int param;
        if (!skode_double_to_int(arg[1], &param)) return 0;
        double val = 0.0;
        switch (param) {
          case 0: // wavetable size
            val = sw.size[wave];
            break;
          case 1: // wavetable rate
            val = sw.rate[wave];
            break;
          case 2: // wavetable size / rate
            val = (float)sw.size[wave] / sw.rate[wave];
            break;
          case 3: // loop start boundary
            val = sw.loop_start[wave];
            break;
          case 4: // loop end boundary
            val = sw.loop_end[wave];
            break;
          default:
            argc = 0; // hack to do-nothing on unknown parameter
            break;
        }
        if (argc > 2) {
          int variable;
          if (skode_double_to_int(arg[2], &variable))
            ands_set_local(ctx->parse, variable, val);
        } else if (argc) {
          ctx->printf(ctx, "# W* %d %d -> %g\n", wave, param, val);
          ands_arg_clear(s);
          ands_arg_push(s, val);
          return 1;
        }
      }
      return 0;
}

static int word_exec_v_star(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        double val = 0.0;
        switch (x) {
          case 0: // wavetable index
            val = sv.wave_table_index[voice];
            break;
          case 1: // amplitide
            val = sv.user_amp[voice];
            break;
          case 2: // freq
            val = sv.freq[voice];
            break;
          default:
            argc = 0; // hack to do-nothing on unknown parameter
            break;
        }
        if (argc > 1) {
          int y;
          if (skode_double_to_int(arg[1], &y))
            ands_set_local(ctx->parse, y, val);
        } else if (argc) {
          ctx->printf(ctx, "# v* %d -> %g\n", x, val);
          ands_arg_clear(s);
          ands_arg_push(s, val);
          return 1;
        }
      }
      return 0;
}

static int word_exec__star_eq(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 2) {
        double val = arg[1] * arg[2];
        ands_set_local(ctx->parse, x, val);
        ands_arg_clear(s);
        ands_arg_push(s, val);
        return 1;
      }
      return 0;
}

static int word_exec__slash_eq(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 2 && arg[2] != 0.0) {
        double val = arg[1] / arg[2];
        ands_set_local(ctx->parse, x, val);
        ands_arg_clear(s);
        ands_arg_push(s, val);
        return 1;
      }
      return 0;
}

static int word_exec_a_eq(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 2) {
        double val = arg[1] + arg[2];
        ands_set_local(ctx->parse, x, val);
        ands_arg_clear(s);
        ands_arg_push(s, val);
        return 1;
      }
      return 0;
}

static int word_exec_s_eq(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 2) {
        double val = arg[1] - arg[2];
        ands_set_local(ctx->parse, x, val);
        ands_arg_clear(s);
        ands_arg_push(s, val);
        return 1;
      }
      return 0;
}

static int word_exec__eq(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 1) {
        ands_set_local(ctx->parse, x, arg[1]);
        ands_arg_clear(s);
        ands_arg_push(s, arg[1]);
        return 1;
      }
      else if (argc == 1) {
        double f = ands_get_local(ctx->parse, x);
        ctx->printf(ctx, "# $%d %g\n", x, f);
        ands_arg_clear(s);
        ands_arg_push(s, f);
        return 1;
      }
      else {
        for (int i=0; i<ANDS_VAR_MAX; i++) {
          double f = ands_get_local(ctx->parse, i);
          if (f != 0.0) ctx->printf(ctx, "# $%d %g\n", i, f);
        }
      }
      return 0;
}


// Word declarations
static skode_word_t word_D = { WID("D"), .execute = word_exec_D, .safety = WORD_IMMEDIATE_ONLY , .category = "data" };
static skode_word_t word__qd = { WID("?d"), .execute = word_exec__qd, .safety = WORD_IMMEDIATE_ONLY , .category = "data" };
static skode_word_t word__slashD = { WID("/D"), .execute = word_exec__slashD, .safety = WORD_IMMEDIATE_ONLY , .category = "data" };
static skode_word_t word__slashks = { WID("/ks"), .execute = word_exec__slashks, .safety = WORD_IMMEDIATE_ONLY , .category = "ksynth" };
static skode_word_t word__slashk = { WID("/k"), .execute = word_exec__slashk, .safety = WORD_IMMEDIATE_ONLY , .category = "ksynth" };
static skode_word_t word_ks = { WID("ks"), .execute = word_exec_ks, .safety = WORD_IMMEDIATE_ONLY , .category = "ksynth" };
static skode_word_t word_k_bang = { WID("k!"), .execute = word_exec_k_bang, .safety = WORD_IMMEDIATE_ONLY , .category = "ksynth" };
static skode_word_t word_kw = { WID("kw"), .execute = word_exec_kw, .safety = WORD_IMMEDIATE_ONLY , .category = "ksynth" };
static skode_word_t word_kw_gt = { WID("kw>"), .execute = word_exec_kw_gt, .safety = WORD_IMMEDIATE_ONLY , .category = "ksynth" };
static skode_word_t word_k_q = { WID("k?"), .execute = word_exec_k_q, .safety = WORD_IMMEDIATE_ONLY , .category = "ksynth" };
static skode_word_t word_k_gtd = { WID("k>d"), .execute = word_exec_k_gtd, .safety = WORD_IMMEDIATE_ONLY , .category = "ksynth" };
static skode_word_t word_k_gtw = { WID("k>w"), .execute = word_exec_k_gtw, .safety = WORD_IMMEDIATE_ONLY , .category = "ksynth" };
static skode_word_t word__eqd = { WID("=d"), .execute = word_exec__eqd, .safety = WORD_IMMEDIATE_ONLY , .category = "data" };
static skode_word_t word_d_bang = { WID("d!"), .execute = word_exec_d_bang, .safety = WORD_IMMEDIATE_ONLY , .category = "data" };
static skode_word_t word_d_star = { WID("d*"), .execute = word_exec_d_star, .safety = WORD_IMMEDIATE_ONLY , .category = "data" };
static skode_word_t word_d_gtr = { WID("d>r"), .execute = word_exec_d_gtr, .safety = WORD_IMMEDIATE_ONLY , .category = "data" };
static skode_word_t word_r_gtd = { WID("r>d"), .execute = word_exec_r_gtd, .safety = WORD_IMMEDIATE_ONLY , .category = "data" };
static skode_word_t word_d_gtk = { WID("d>k"), .execute = word_exec_d_gtk, .safety = WORD_IMMEDIATE_ONLY , .category = "ksynth" };
static skode_word_t word_w_gtk = { WID("w>k"), .execute = word_exec_w_gtk, .safety = WORD_IMMEDIATE_ONLY , .category = "ksynth" };
static skode_word_t word_s_q = { WID("s?"), .execute = word_exec_s_q, .safety = WORD_IMMEDIATE_ONLY , .category = "macros" };
static skode_word_t word__qm = { WID("?m"), .execute = word_exec__qm, .safety = WORD_IMMEDIATE_ONLY , .category = "macros" };
static skode_word_t word__slashd = { WID("/d"), .execute = word_exec__slashd, .safety = WORD_IMMEDIATE_ONLY , .category = "data" };
static skode_word_t word__slashm = { WID("/m"), .execute = word_exec__slashm, .safety = WORD_IMMEDIATE_ONLY , .category = "macros" };
static skode_word_t word__slashm_bang = { WID("/m!"), .execute = word_exec__slashm_bang, .safety = WORD_IMMEDIATE_ONLY , .category = "macros" };
static skode_word_t word__lts = { WID("<s"), .execute = word_exec__lts, .safety = WORD_IMMEDIATE_ONLY , .category = "macros" };
static skode_word_t word_s_gt = { WID("s>"), .execute = word_exec_s_gt, .safety = WORD_IMMEDIATE_ONLY , .category = "macros" };
static skode_word_t word_s_pct = { WID("s%"), .execute = word_exec_s_pct, .safety = WORD_IMMEDIATE_ONLY , .category = "macros" };
static skode_word_t word__lte = { WID("<e"), .execute = word_exec__lte, .safety = WORD_IMMEDIATE_ONLY , .category = "macros" };
static skode_word_t word_e_gt = { WID("e>"), .execute = word_exec_e_gt, .safety = WORD_IMMEDIATE_ONLY , .category = "macros" };
static skode_word_t word_e_bang = { WID("e!"), .execute = word_exec_e_bang, .safety = WORD_IMMEDIATE_ONLY , .category = "macros" };
static skode_word_t word_e_q = { WID("e?"), .execute = word_exec_e_q, .safety = WORD_IMMEDIATE_ONLY , .category = "macros" };
static skode_word_t word_W_star = { WID("W*"), .execute = word_exec_W_star, .safety = WORD_IMMEDIATE_ONLY , .category = "data" };
static skode_word_t word_v_star = { WID("v*"), .execute = word_exec_v_star, .safety = WORD_IMMEDIATE_ONLY , .category = "data" };
static skode_word_t word__star_eq = { WID("*="), .execute = word_exec__star_eq, .safety = WORD_IMMEDIATE_ONLY , .category = "data" };
static skode_word_t word__slash_eq = { WID("/="), .execute = word_exec__slash_eq, .safety = WORD_IMMEDIATE_ONLY , .category = "data" };
static skode_word_t word_a_eq = { WID("a="), .execute = word_exec_a_eq, .safety = WORD_IMMEDIATE_ONLY , .category = "data" };
static skode_word_t word_s_eq = { WID("s="), .execute = word_exec_s_eq, .safety = WORD_IMMEDIATE_ONLY , .category = "data" };
static skode_word_t word__eq = { WID("="), .execute = word_exec__eq, .safety = WORD_IMMEDIATE_ONLY , .category = "data" };

void skode_register_words_data(skode_vocab_t *vocab) {
  skode_dict_register(vocab, &word_D);
  skode_dict_register(vocab, &word__qd);
  skode_dict_register(vocab, &word__slashD);
  skode_dict_register(vocab, &word__slashks);
  skode_dict_register(vocab, &word__slashk);
  skode_dict_register(vocab, &word_ks);
  skode_dict_register(vocab, &word_k_bang);
  skode_dict_register(vocab, &word_kw);
  skode_dict_register(vocab, &word_kw_gt);
  skode_dict_register(vocab, &word_k_q);
  skode_dict_register(vocab, &word_k_gtd);
  skode_dict_register(vocab, &word_k_gtw);
  skode_dict_register(vocab, &word__eqd);
  skode_dict_register(vocab, &word_d_bang);
  skode_dict_register(vocab, &word_d_star);
  skode_dict_register(vocab, &word_d_gtr);
  skode_dict_register(vocab, &word_r_gtd);
  skode_dict_register(vocab, &word_d_gtk);
  skode_dict_register(vocab, &word_w_gtk);
  skode_dict_register(vocab, &word_s_q);
  skode_dict_register(vocab, &word__qm);
  skode_dict_register(vocab, &word__slashd);
  skode_dict_register(vocab, &word__slashm);
  skode_dict_register(vocab, &word__slashm_bang);
  skode_dict_register(vocab, &word__lts);
  skode_dict_register(vocab, &word_s_gt);
  skode_dict_register(vocab, &word_s_pct);
  skode_dict_register(vocab, &word__lte);
  skode_dict_register(vocab, &word_e_gt);
  skode_dict_register(vocab, &word_e_bang);
  skode_dict_register(vocab, &word_e_q);
  skode_dict_register(vocab, &word_W_star);
  skode_dict_register(vocab, &word_v_star);
  skode_dict_register(vocab, &word__star_eq);
  skode_dict_register(vocab, &word__slash_eq);
  skode_dict_register(vocab, &word_a_eq);
  skode_dict_register(vocab, &word_s_eq);
  skode_dict_register(vocab, &word__eq);
}
