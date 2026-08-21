#include "skode.h"

static int word_exec__slashmb_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "%s", skred_midi_binding_status());
      return 0;
}
static skode_word_t word__slashmb_q = { WID("/mb?"), .execute = word_exec__slashmb_q, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashmbC(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      skred_midi_binding_clear();
      ctx->printf(ctx, "# MIDI Skode bindings cleared\n");
      return 0;
}
static skode_word_t word__slashmbC = { WID("/mbC"), .execute = word_exec__slashmbC, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashpg(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int group, source, width, root = 0;
        if (argc < 3 || argc > 4 ||
            !skode_double_to_int(arg[0], &group) ||
            !skode_double_to_int(arg[1], &source) ||
            !skode_double_to_int(arg[2], &width) ||
            (argc > 3 && !skode_double_to_int(arg[3], &root)) ||
            skred_poly_group_set(group, source, width, root) != 0) {
          ctx->printf(ctx, "# usage: /pg group,source,width[,root-offset]\n");
        }
      }
      return 0;
}
static skode_word_t word__slashpg = { WID("/pg"), .execute = word_exec__slashpg, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashpg_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (!x_valid || argc != 1 || skred_poly_group_refresh(x) != 0)
        ctx->printf(ctx, "# usage: /pg! group\n");
      return 0;
}
static skode_word_t word__slashpg_bang = { WID("/pg!"), .execute = word_exec__slashpg_bang, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashpp(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int pool, group, base, count, policy = SKRED_POLY_STEAL_RELEASE_OLDEST;
        if (argc < 4 || argc > 5 ||
            !skode_double_to_int(arg[0], &pool) ||
            !skode_double_to_int(arg[1], &group) ||
            !skode_double_to_int(arg[2], &base) ||
            !skode_double_to_int(arg[3], &count) ||
            (argc > 4 && !skode_double_to_int(arg[4], &policy)) ||
            skred_poly_pool_set(pool, group, base, count, policy) != 0) {
          ctx->printf(ctx,
            "# usage: /pp pool,group,base,count[,steal-policy]\n");
        }
      }
      return 0;
}
static skode_word_t word__slashpp = { WID("/pp"), .execute = word_exec__slashpp, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashpp_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (!x_valid || argc != 1 || skred_poly_pool_refresh(x) != 0)
        ctx->printf(ctx, "# usage: /pp! pool\n");
      return 0;
}
static skode_word_t word__slashpp_bang = { WID("/pp!"), .execute = word_exec__slashpp_bang, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashpm(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int pool, mode, priority = SKRED_POLY_PRIORITY_LAST;
        int articulation = SKRED_POLY_ARTICULATION_RETRIGGER;
        if (argc < 2 || argc > 4 ||
            !skode_double_to_int(arg[0], &pool) ||
            !skode_double_to_int(arg[1], &mode) ||
            (argc > 2 && !skode_double_to_int(arg[2], &priority)) ||
            (argc > 3 && !skode_double_to_int(arg[3], &articulation)) ||
            skred_poly_pool_mode(pool, mode, priority, articulation) != 0) {
          ctx->printf(ctx,
            "# usage: /pm pool,mode[,priority[,articulation]]\n");
        }
      }
      return 0;
}
static skode_word_t word__slashpm = { WID("/pm"), .execute = word_exec__slashpm, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__qpg(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "%s", skred_poly_group_status(x_valid ? x : -1));
      return 0;
}
static skode_word_t word__qpg = { WID("?pg"), .execute = word_exec__qpg, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__qpp(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "%s", skred_poly_pool_status(x_valid ? x : -1));
      return 0;
}
static skode_word_t word__qpp = { WID("?pp"), .execute = word_exec__qpp, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashvg(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int graph_voice, format = 0, depth = 0;
        if (argc < 1 || argc > 3 ||
            !skode_double_to_int(arg[0], &graph_voice) ||
            (argc > 1 && !skode_double_to_int(arg[1], &format)) ||
            (argc > 2 && !skode_double_to_int(arg[2], &depth))) {
          ctx->printf(ctx, "# usage: /vg voice[,format[,depth]]\n");
        } else {
          ctx->printf(ctx, "%s", skred_voice_graph(graph_voice, format, depth));
        }
      }
      return 0;
}
static skode_word_t word__slashvg = { WID("/vg"), .execute = word_exec__slashvg, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_pn(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int pool, key;
        int result = -1;
        if (argc >= 4 && argc <= 5 &&
            skode_double_to_int(arg[0], &pool) &&
            skode_double_to_int(arg[1], &key))
          result = skred_poly_note(pool, key, arg[2], arg[3],
            argc > 4 ? arg[4] : 0);
        if (result < 0 || argc < 4 || argc > 5 ||
            !skode_double_to_int(arg[0], &pool) ||
            !skode_double_to_int(arg[1], &key))
          ctx->printf(ctx, "# usage: pn pool,key,note,velocity[,cents]\n");
        else if (result > 0)
          ctx->printf(ctx, "# poly pool %d is full (no-steal policy)\n", pool);
      }
      return 0;
}
static skode_word_t word_pn = { WID("pn"), .execute = word_exec_pn, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_pr(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int pool, key;
        if (argc < 2 || argc > 3 ||
            !skode_double_to_int(arg[0], &pool) ||
            !skode_double_to_int(arg[1], &key) ||
            skred_poly_release(pool, key, argc > 2 ? arg[2] : 0) != 0)
          ctx->printf(ctx, "# usage: pr pool,key[,release-velocity]\n");
      }
      return 0;
}
static skode_word_t word_pr = { WID("pr"), .execute = word_exec_pr, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_pb(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int pool, key;
        if (argc < 3 || argc > 4 ||
            !skode_double_to_int(arg[0], &pool) ||
            !skode_double_to_int(arg[1], &key) ||
            skred_poly_bend(pool, key, arg[2], argc > 3 ? arg[3] : 0) != 0)
          ctx->printf(ctx, "# usage: pb pool,key,semitones[,cents]\n");
      }
      return 0;
}
static skode_word_t word_pb = { WID("pb"), .execute = word_exec_pb, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_ab(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

}
static skode_word_t word_ab = { WID("ab"), .execute = word_exec_ab, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_abp(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

}
static skode_word_t word_abp = { WID("abp"), .execute = word_exec_abp, .safety = WORD_IMMEDIATE_ONLY };

#ifdef SKRED_FEATURE_AM
static int word_exec_A(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

}
static skode_word_t word_A = { WID("A"), .execute = word_exec_A, .safety = WORD_IMMEDIATE_ONLY };
#endif

static int word_exec_b(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

}
static skode_word_t word_b = { WID("b"), .execute = word_exec_b, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_B(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

}
static skode_word_t word_B = { WID("B"), .execute = word_exec_B, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_BC(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

}
static skode_word_t word_BC = { WID("BC"), .execute = word_exec_BC, .safety = WORD_IMMEDIATE_ONLY };

#ifdef SKRED_FEATURE_PD
static int word_exec_c(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

}
static skode_word_t word_c = { WID("c"), .execute = word_exec_c, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_PD
static int word_exec_C(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc < 2) {
        cmod_set(voice, -1, 0);
      } else if (x_valid) {
        cmod_set(voice, x, arg[1]);
      }
      return 0;
}
static skode_word_t word_C = { WID("C"), .execute = word_exec_C, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_PD
static int word_exec_ct(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

}
static skode_word_t word_ct = { WID("ct"), .execute = word_exec_ct, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_PD
static int word_exec_cd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

}
static skode_word_t word_cd = { WID("cd"), .execute = word_exec_cd, .safety = WORD_IMMEDIATE_ONLY };
#endif

static int word_exec_D(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        if (x > ands_data_cap(ctx->parse)) ands_data_resize(ctx->parse, x);
      } else {
        ctx->printf(ctx, "# D[%d]\n", ands_data_cap(ctx->parse));
      }
      return 0;
}
static skode_word_t word_D = { WID("D"), .execute = word_exec_D, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_MO(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        uint8_t bytes[3];
        if (argc < 1 || argc > 3) {
          ctx->printf(ctx, "# usage: MO status[,data1[,data2]]\n");
          return 0;
        }
        int valid = 1;
        for (int i = 0; i < argc; i++) {
          int byte;
          if (!skode_double_to_int(arg[i], &byte) || byte < 0 || byte > 255 ||
              arg[i] != (double)byte) {
            valid = 0;
            break;
          }
          bytes[i] = (uint8_t)byte;
        }
        int result = valid ? skred_midi_send_raw(bytes, argc) : -2;
        if (result != 0)
          ctx->printf(ctx, "# MIDI output failed (%d)\n", result);
      }
      return 0;
}
static skode_word_t word_MO = { WID("MO"), .execute = word_exec_MO, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_ce(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word_ce = { WID("ce"), .execute = word_exec_ce, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__qd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        double *data = ands_data(ctx->parse);
        int data_len = ands_data_len(ctx->parse);
        skode_double_dump(ctx, data, data_len);
      }
      return 0;
}
static skode_word_t word__qd = { WID("?d"), .execute = word_exec__qd, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_fb(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

}
static skode_word_t word_fb = { WID("fb"), .execute = word_exec_fb, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_fbp(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

}
static skode_word_t word_fbp = { WID("fbp"), .execute = word_exec_fbp, .safety = WORD_IMMEDIATE_ONLY };

#if defined(SKRED_FEATURE_FILT) && defined(SKRED_FEATURE_FADSR)
static int word_exec_ft(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc == 4) {
        float a = arg[0];
        float d = arg[1];
        float s = arg[2];
        float r = arg[3];
        envelope_configure_e(&sv.filter_envelope[voice], a, d, s, r);
        sv.use_filter_envelope[voice] = !(a==0 && d==0 && s==1 && r==0);
      }
      return 0;
}
static skode_word_t word_ft = { WID("ft"), .execute = word_exec_ft, .safety = WORD_IMMEDIATE_ONLY };
#endif

#if defined(SKRED_FEATURE_FILT) && defined(SKRED_FEATURE_FADSR)
static int word_exec_fd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) sv.filter_env_depth[voice] = arg[0];
      return 0;
}
static skode_word_t word_fd = { WID("fd"), .execute = word_exec_fd, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_FM
static int word_exec_F(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc <= 1) {
        freq_mod_set(voice, -1, 0, 0);
      } else if (x_valid) {
        float a = 0;
        if (argc > 2) a = arg[2];
        freq_mod_set(voice, x, arg[1], a);
      }
      return 0;
}
static skode_word_t word_F = { WID("F"), .execute = word_exec_F, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_FM
static int word_exec_FF(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) freq_mod_mode_set(voice, x);
      return 0;
}
static skode_word_t word_FF = { WID("FF"), .execute = word_exec_FF, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_FM
static int word_exec_FB(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) freq_feedback_set(voice, arg[0]);
      return 0;
}
static skode_word_t word_FB = { WID("FB"), .execute = word_exec_FB, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_GLISS
static int word_exec_g(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        if (arg[0] <= 0) {
          sv.glissando_enable[voice] = 0;
          sv.glissando_time[voice] = 0.0;
        } else {
          sv.glissando_enable[voice] = 1;
          sv.glissando_time[voice] = arg[0];
        }
      }
      return 0;
}
static skode_word_t word_g = { WID("g"), .execute = word_exec_g, .safety = WORD_IMMEDIATE_ONLY };
#endif

static int word_exec_G(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        int links[4] = {-1, -1, -1, -1};
        for (int i = 0; i < argc && i < 4; i++) {
          int link;
          if (skode_double_to_int(arg[i], &link) && skode_voice_valid(link))
            links[i] = link;
        }
        sv.link_midi_0[voice] = links[0];
        sv.link_midi_1[voice] = links[1];
        sv.link_midi_2[voice] = links[2];
        sv.link_midi_3[voice] = links[3];
      }
      return 0;
}
static skode_word_t word_G = { WID("G"), .execute = word_exec_G, .safety = WORD_IMMEDIATE_ONLY };

#ifdef SKRED_FEATURE_SAH
static int word_exec_h(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 0) {
        float ratio = (float)arg[0];
        int mode = sv.sample_hold_mode[voice];
        if (argc > 1 && isfinite(arg[1])) mode = (int)arg[1];
        sv.sample_hold_ratio[voice] = ratio;
        sv.sample_hold_mode[voice] = mode;
      }
      return 0;
}
static skode_word_t word_h = { WID("h"), .execute = word_exec_h, .safety = WORD_IMMEDIATE_ONLY };
#endif

static int word_exec_H(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        int links[4] = {-1, -1, -1, -1};
        for (int i = 0; i < argc && i < 4; i++) {
          int link;
          if (skode_double_to_int(arg[i], &link) && skode_voice_valid(link))
            links[i] = link;
        }
        sv.link_velo_0[voice] = links[0];
        sv.link_velo_1[voice] = links[1];
        sv.link_velo_2[voice] = links[2];
        sv.link_velo_3[voice] = links[3];
      }
      return 0;
    // TODO re-allocate the data/array buffer with the arg
}
static skode_word_t word_H = { WID("H"), .execute = word_exec_H, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashD(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__slashD = { WID("/D"), .execute = word_exec__slashD, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_I(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {} return 0; // TODO en/dis-able send timestamp wire to the event logger
}
static skode_word_t word_I = { WID("I"), .execute = word_exec_I, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_L(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        double seconds = arg[0];
        if (!isfinite(seconds) || seconds <= 0.0) {
          sv.link_trig[voice] = -1.0f;
          sv.link_trig_samp[voice] = 0;
        } else {
          long double samples = (long double)seconds * (long double)MAIN_SAMPLE_RATE;
          sv.link_trig[voice] = (float)seconds;
          sv.link_trig_samp[voice] =
            samples >= (long double)UINT64_MAX ? UINT64_MAX : (uint64_t)samples;
        }
      }
      return 0;
}
static skode_word_t word_L = { WID("L"), .execute = word_exec_L, .safety = WORD_IMMEDIATE_ONLY };

#ifdef SKRED_FEATURE_FILT
static int word_exec_J(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 0) {
        int mode = (int)x;
        int character = sv.filter_mode[voice] / 10;
        if (argc > 1 && isfinite(arg[1])) character = (int)arg[1];
        sv.filter_mode[voice] = (character * 10) + (mode % 10);
        mmf_set_params(&skred_global_engine, voice,
          sv.filter_freq[voice],
          sv.filter_res[voice]);
      }
      return 0;
}
static skode_word_t word_J = { WID("J"), .execute = word_exec_J, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_FILT
static int word_exec_K(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) { mmf_set_freq(&skred_global_engine, voice, arg[0]); }
      return 0;
}
static skode_word_t word_K = { WID("K"), .execute = word_exec_K, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_KSYNTH
static int word_exec__slashks(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__slashks = { WID("/ks"), .execute = word_exec__slashks, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_KSYNTH
static int word_exec__slashk(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        int verbose = 0;
        if (argc > 1) skode_double_to_int(arg[1], &verbose);
        ksynth_load(ctx, x, verbose);
      }
      return 0;
}
static skode_word_t word__slashk = { WID("/k"), .execute = word_exec__slashk, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_KSYNTH
static int word_exec_ks(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

}
static skode_word_t word_ks = { WID("ks"), .execute = word_exec_ks, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_KSYNTH
static int word_exec_k_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int len = 0;
        char *cmd = ands_string(ctx->parse);
        if (cmd) len = strlen(cmd);
        if (ctx->trace) {
          ctx->printf(ctx, "cmd:[%s] len:%d\n", cmd, len);
        }
        if (len) skode_ks_eval(ctx, cmd, len);
      }
      return 0;
}
static skode_word_t word_k_bang = { WID("k!"), .execute = word_exec_k_bang, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_KSYNTH
static int word_exec_kw(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        (void)x;
      }
      return 0;
}
static skode_word_t word_kw = { WID("kw"), .execute = word_exec_kw, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_KSYNTH
static int word_exec_kw_gt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        (void)x;
        skode_ks_result_to_data(ctx);
      }
      return 0;
}
static skode_word_t word_kw_gt = { WID("kw>"), .execute = word_exec_kw_gt, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_KSYNTH
static int word_exec_k_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        K result = (K)ctx->ks_result;
        if (result && !k_is_func(result))
          skode_double_dump(ctx, result->f, (size_t)result->n);
      }
      return 0;
}
static skode_word_t word_k_q = { WID("k?"), .execute = word_exec_k_q, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_KSYNTH
static int word_exec_k_gtd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        skode_ks_result_to_data(ctx);
      }
      return 0;
}
static skode_word_t word_k_gtd = { WID("k>d"), .execute = word_exec_k_gtd, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_KSYNTH
static int word_exec_k_gtw(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int wave_slot = EXT_SAMPLE_000;
        int one_shot = 0;
        float rate = (float)MAIN_SAMPLE_RATE;
        float offset = 0.0f;
        if (argc && !skode_double_to_int(arg[0], &wave_slot)) break;
        if (argc > 1) rate = arg[1];
        if (argc > 2) skode_double_to_int(arg[2], &one_shot);
        if (argc > 3) offset = arg[3];
        if (skode_ks_result_to_data(ctx))
          data_load(ctx, wave_slot, one_shot, rate, offset);
      }
      return 0;
}
static skode_word_t word_k_gtw = { WID("k>w"), .execute = word_exec_k_gtw, .safety = WORD_IMMEDIATE_ONLY };
#endif

static int word_exec_k(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) { sv.amp_envelope_mode[voice] = x; } return 0;
}
static skode_word_t word_k = { WID("k"), .execute = word_exec_k, .safety = WORD_IMMEDIATE_ONLY };

#ifdef SKRED_FEATURE_UDP
static int word_exec_udp(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        ctx->printf(ctx, "# udp [%d] %d/%d\n", ctx->which, ctx->ip, ctx->port);
      }
      return 0;
}
static skode_word_t word_udp = { WID("udp"), .execute = word_exec_udp, .safety = WORD_IMMEDIATE_ONLY };
#endif

static int word_exec_log(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        if (x) { ctx->log_enable = 1; } else { ctx->log_enable = 0; }
      }
      return 0;
}
static skode_word_t word_log = { WID("log"), .execute = word_exec_log, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec____l(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && isfinite(arg[0])) envelope_velocity(voice, arg[0]);
      return 0;
}
static skode_word_t word____l = { WID("___l"), .execute = word_exec____l, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_l(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
    #if 1
      if (argc) skode_linked_velocity(voice, arg[0], SAMPLE_COUNT_GET());
    #else
      if (argc) {
        uint64_t now = SAMPLE_COUNT_GET();
        int a = sv.link_velo_0[voice];
        int b = sv.link_velo_1[voice];
        int c = sv.link_velo_2[voice];
        int d = sv.link_velo_3[voice];
        double vel = arg[0];
        skode_envelope_velocity(voice, vel, now);
        if (a >= 0) skode_envelope_velocity(a, vel, now);
        if (b >= 0) skode_envelope_velocity(b, vel, now);
        if (c >= 0) skode_envelope_velocity(c, vel, now);
        if (d >= 0) skode_envelope_velocity(d, vel, now);
      }
    #endif
      return 0;
}
static skode_word_t word_l = { WID("l"), .execute = word_exec_l, .safety = WORD_IMMEDIATE_ONLY };

#ifdef SKRED_FEATURE_SEQ
static int word_exec_M(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        float bpm = arg[0];
        float sub = (argc >= 2 && arg[1] > 0.0f) ? arg[1] : 16.0f;
        if (tempo_set_subdivision(bpm, sub) != 0)
          ctx->printf(ctx, "# tempo must be between %g and %g BPM\n",
            (double)SEQ_TEMPO_MIN_BPM, (double)SEQ_TEMPO_MAX_BPM);
        else
          skred_control_pattern_event(SKRED_CONTROL_EVENT_TEMPO_CHANGE, SAMPLE_COUNT_GET(), -1, 0);
      }
      return 0;
}
static skode_word_t word_M = { WID("M"), .execute = word_exec_M, .safety = WORD_IMMEDIATE_ONLY };
#endif

static int word_exec_N(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        if (isnan(arg[0])) {
          // do nothing
        } else {
          sv.midi_transpose[voice] = arg[0];
        }
        if (argc > 1) sv.midi_cents[voice] = arg[1];
      }
      return 0;
}
static skode_word_t word_N = { WID("N"), .execute = word_exec_N, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_ds(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) delay_send_set(&skred_global_engine, voice, arg[0]);
      return 0;
}
static skode_word_t word_ds = { WID("ds"), .execute = word_exec_ds, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_DG(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int bus = 1;
        int bits, native;
        if (argc > 0) skode_double_to_int(arg[0], &bus);
        delay_grit_get(&skred_global_engine, bus, &bits, &native);
        if (argc > 1 && isfinite(arg[1])) skode_double_to_int(arg[1], &bits);
        if (argc > 2 && isfinite(arg[2])) skode_double_to_int(arg[2], &native);
        delay_grit_set(&skred_global_engine, bus, bits, native);
      }
      return 0;
}
static skode_word_t word_DG = { WID("DG"), .execute = word_exec_DG, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_DL(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int bus = 1;
        int coarse, fine, feedback, mod_freq, mod_depth, level;
        if (argc > 0) skode_double_to_int(arg[0], &bus);
        delay_params_get(&skred_global_engine, bus, &coarse, &fine, &feedback, &mod_freq, &mod_depth, &level);
        if (argc > 1 && isfinite(arg[1])) skode_double_to_int(arg[1], &coarse);
        if (argc > 2 && isfinite(arg[2])) skode_double_to_int(arg[2], &fine);
        if (argc > 3 && isfinite(arg[3])) skode_double_to_int(arg[3], &feedback);
        if (argc > 4 && isfinite(arg[4])) skode_double_to_int(arg[4], &mod_freq);
        if (argc > 5 && isfinite(arg[5])) skode_double_to_int(arg[5], &mod_depth);
        if (argc > 6 && isfinite(arg[6])) skode_double_to_int(arg[6], &level);
        delay_params_set(&skred_global_engine, bus, coarse, fine, feedback, mod_freq, mod_depth, level);
      }
      return 0;
}
static skode_word_t word_DL = { WID("DL"), .execute = word_exec_DL, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_DL_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        int bus = 1;
        skode_double_to_int(arg[0], &bus);
        ctx->printf(ctx, "%s", delay_bus_format(bus));
      } else {
        ctx->printf(ctx, "%s", delay_format());
      }
      return 0;
}
static skode_word_t word_DL_q = { WID("DL?"), .execute = word_exec_DL_q, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_DD(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int bus = 1;
        int damping, hp;
        if (argc > 0) skode_double_to_int(arg[0], &bus);
        delay_damping_get(&skred_global_engine, bus, &damping, &hp);
        if (argc > 1 && isfinite(arg[1])) skode_double_to_int(arg[1], &damping);
        if (argc > 2 && isfinite(arg[2])) skode_double_to_int(arg[2], &hp);
        delay_damping_set(&skred_global_engine, bus, damping, hp);
      }
      return 0;
}
static skode_word_t word_DD = { WID("DD"), .execute = word_exec_DD, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_DF(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int bus = 1;
        int on;
        if (argc > 0) skode_double_to_int(arg[0], &bus);
        on = delay_freeze_get(&skred_global_engine, bus);
        if (argc > 1 && isfinite(arg[1])) skode_double_to_int(arg[1], &on);
        delay_freeze_set(&skred_global_engine, bus, on);
      }
      return 0;
}
static skode_word_t word_DF = { WID("DF"), .execute = word_exec_DF, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_DP(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int bus = 1;
        int on;
        if (argc > 0) skode_double_to_int(arg[0], &bus);
        on = delay_pingpong_get(&skred_global_engine, bus);
        if (argc > 1 && isfinite(arg[1])) skode_double_to_int(arg[1], &on);
        delay_pingpong_set(&skred_global_engine, bus, on);
      }
      return 0;
}
static skode_word_t word_DP = { WID("DP"), .execute = word_exec_DP, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_DT(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc == 2) {
        int bus;
        if (skode_double_to_int(arg[0], &bus) && isfinite(arg[1]))
          delay_time_ms_set(&skred_global_engine, bus, (float)arg[1]);
      }
      return 0;
}
static skode_word_t word_DT = { WID("DT"), .execute = word_exec_DT, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_DS(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc == 3) {
        int bus;
        if (skode_double_to_int(arg[0], &bus) && isfinite(arg[1]) && isfinite(arg[2]))
          delay_time_sync_set(&skred_global_engine, bus, (float)arg[1], (float)arg[2]);
      }
      return 0;
}
static skode_word_t word_DS = { WID("DS"), .execute = word_exec_DS, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_GS(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      global_status_show(ctx, argc > 0 && arg[0] > 0.0);
      return 0;
}
static skode_word_t word_GS = { WID("GS"), .execute = word_exec_GS, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_GS_gt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (!ands_string_fresh(ctx->parse) || !ands_string(ctx->parse)[0])
        ctx->printf(ctx, "# GS> requires [filename.zip]\n");
      else
        (void)skode_session_save(ctx, ands_string(ctx->parse));
      return 0;
}
static skode_word_t word_GS_gt = { WID("GS>"), .execute = word_exec_GS_gt, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_GS_lt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (!ands_string_fresh(ctx->parse) || !ands_string(ctx->parse)[0])
        ctx->printf(ctx, "# GS< requires [filename.zip]\n");
      else
        (void)skode_session_load(ctx, ands_string(ctx->parse));
      return 0;
}
static skode_word_t word_GS_lt = { WID("GS<"), .execute = word_exec_GS_lt, .safety = WORD_IMMEDIATE_ONLY };

#ifdef SKRED_FEATURE_PANMOD
static int word_exec_P(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc < 2) {
        pan_mod_set(voice, -1, 0, 0);
      } else if (x_valid) {
        float a = 0;
        if (argc > 2) a = arg[2];
        pan_mod_set(voice, x, arg[1], a);
      }
      return 0;
}
static skode_word_t word_P = { WID("P"), .execute = word_exec_P, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_CRUSH
static int word_exec_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 0) {
        int bits = (int)x;
        int curve = sv.quantize[voice] / 100;
        if (argc > 1 && isfinite(arg[1])) curve = (int)arg[1];
        wave_quant(voice, (curve * 100) + (bits % 100));
      }
      return 0;
}
static skode_word_t word_q = { WID("q"), .execute = word_exec_q, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_FILT
static int word_exec_Q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) { mmf_set_res(&skred_global_engine, voice, arg[0]); }
      return 0;
}
static skode_word_t word_Q = { WID("Q"), .execute = word_exec_Q, .safety = WORD_IMMEDIATE_ONLY };
#endif

#if defined(SKRED_FEATURE_RECORD) || defined(SKRED_FEATURE_SCOPE) || defined(SKRED_FEATURE_TRACKS)
static int word_exec_r(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) synth_record_track_set(voice, x);
      return 0;
}
static skode_word_t word_r = { WID("r"), .execute = word_exec_r, .safety = WORD_IMMEDIATE_ONLY };
#endif

#if defined(SKRED_FEATURE_RECORD) || defined(SKRED_FEATURE_SCOPE) || defined(SKRED_FEATURE_TRACKS)
static int word_exec_rt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && x > 0 && x <= RECORD_TRACK_MAX) {
        synth_track_name_set(x, ands_string(ctx->parse));
        #ifdef SKRED_FEATURE_SCOPE
        scope_ipc_track_metadata_set(x, synth_track_name_get(x),
          synth_track_volume_db_get(x));
        #endif
      }
      return 0;
}
static skode_word_t word_rt = { WID("rt"), .execute = word_exec_rt, .safety = WORD_IMMEDIATE_ONLY };
#endif

#if defined(SKRED_FEATURE_RECORD) || defined(SKRED_FEATURE_SCOPE) || defined(SKRED_FEATURE_TRACKS)
static int word_exec_rv(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 1 && x > 0 && x <= RECORD_TRACK_MAX) {
        synth_track_volume_set(x, arg[1]);
        #ifdef SKRED_FEATURE_SCOPE
        scope_ipc_track_metadata_set(x, synth_track_name_get(x),
          synth_track_volume_db_get(x));
        #endif
      }
      return 0;
}
static skode_word_t word_rv = { WID("rv"), .execute = word_exec_rv, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_SEQ
static int word_exec_R_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        int tag = x;
        seq_kill_by_tag(tag);
      }
      return 0;
}
static skode_word_t word_R_bang = { WID("R!"), .execute = word_exec_R_bang, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_SEQ
static int word_exec_R_bang_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      seq_kill_all();
      return 0;
}
static skode_word_t word_R_bang_bang = { WID("R!!"), .execute = word_exec_R_bang_bang, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_SEQ
static int word_exec_RR(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 1 && x_valid && x > 0 && x <= QUEUE_SIZE &&
          isfinite(arg[1]) && arg[1] >= 0.0) {
        event_program_t program;
        if (!skode_compile_scheduled(ctx, ands_string(ctx->parse), &program))
          break;
        int tag = 0;
        if (argc > 2) skode_double_to_int(arg[2], &tag);
        double seconds = tempo_step_seconds_get() * 4.0f * arg[1];
        skode_queue_repeated(&program, ctx->voice, x, seconds, tag);
      } return 0;
}
static skode_word_t word_RR = { WID("RR"), .execute = word_exec_RR, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_SEQ
static int word_exec_eRR(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      skode_repeat_macro(ctx, arg, argc, 1);
      return 0;
}
static skode_word_t word_eRR = { WID("eRR"), .execute = word_exec_eRR, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_SEQ
static int word_exec_eR(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      skode_repeat_macro(ctx, arg, argc, 0);
      return 0;
}
static skode_word_t word_eR = { WID("eR"), .execute = word_exec_eR, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_SEQ
static int word_exec_DO_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && x>0) {
        event_program_t program;
        if (!skode_compile_scheduled(ctx, ands_string(ctx->parse), &program))
          break;
        int tag = 0;
        if (argc > 1) skode_double_to_int(arg[1], &tag);
        uint64_t qt = SAMPLE_COUNT_GET();
        skode_queue_program(&program, ctx->voice, qt, tag);
      } return 0;
}
static skode_word_t word_DO_q = { WID("DO?"), .execute = word_exec_DO_q, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_SEQ
static int word_exec_R(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 1 && x_valid && x > 0 && x <= QUEUE_SIZE &&
          isfinite(arg[1]) && arg[1] >= 0.0) {
        event_program_t program;
        if (!skode_compile_scheduled(ctx, ands_string(ctx->parse), &program))
          break;
        int tag = 0;
        if (argc > 2) skode_double_to_int(arg[2], &tag);
        skode_queue_repeated(&program, ctx->voice, x, arg[1], tag);
      } return 0;
}
static skode_word_t word_R = { WID("R"), .execute = word_exec_R, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_SMOOTHER
static int word_exec_s(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        if (arg[0] <= 0) {
          sv.smoother_enable[voice] = 0;
        } else {
          sv.smoother_enable[voice] = 1;
          sv.smoother_smoothing[voice] = arg[0];
        }
      }
      return 0;
}
static skode_word_t word_s = { WID("s"), .execute = word_exec_s, .safety = WORD_IMMEDIATE_ONLY };
#endif

static int word_exec_S(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) wave_reset(x);
      return 0;
}
static skode_word_t word_S = { WID("S"), .execute = word_exec_S, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_t(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 3) envelope_set(voice, arg[0], arg[1], arg[2], arg[3]);
      return 0;
}
static skode_word_t word_t = { WID("t"), .execute = word_exec_t, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_T(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        envelope_velocity(voice, 1);
        if (sv.link_velo_0[voice] >= 0) envelope_velocity(sv.link_velo_0[voice], 1);
        if (sv.link_velo_1[voice] >= 0) envelope_velocity(sv.link_velo_1[voice], 1);
        if (sv.link_velo_2[voice] >= 0) envelope_velocity(sv.link_velo_2[voice], 1);
        if (sv.link_velo_3[voice] >= 0) envelope_velocity(sv.link_velo_3[voice], 1);
      }
      return 0;
}
static skode_word_t word_T = { WID("T"), .execute = word_exec_T, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_vc(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) voice_control_events_set(voice, x != 0);
      return 0;
}
static skode_word_t word_vc = { WID("vc"), .execute = word_exec_vc, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_V(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        volume_set(arg[0]);
        #ifdef SKRED_FEATURE_SCOPE
        scope_ipc_track_metadata_set(0, synth_track_name_get(0),
          synth_track_volume_db_get(0));
        #endif
      }
      return 0;
}
static skode_word_t word_V = { WID("V"), .execute = word_exec_V, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_vt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      skode_copy_string(sv.text[voice], TEXT_MAX, ands_string(ctx->parse));
      return 0;
}
static skode_word_t word_vt = { WID("vt"), .execute = word_exec_vt, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_wt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && skode_wave_valid(x)) {
        skode_copy_string(sw.name[x], WAVE_NAME_MAX, ands_string(ctx->parse));
      }
      return 0;
}
static skode_word_t word_wt = { WID("wt"), .execute = word_exec_wt, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_WL(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 2 && x_valid && skode_wave_valid(x)) {
        int start, end;
        if (skode_double_to_int(arg[1], &start) &&
            skode_double_to_int(arg[2], &end)) {
          wave_loop_points_set(x, start, end);
        }
      }
      return 0;
}
static skode_word_t word_WL = { WID("WL"), .execute = word_exec_WL, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_VS(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc >= 2) {
        int start, end;
        if (skode_double_to_int(arg[0], &start) &&
            skode_double_to_int(arg[1], &end)) {
          if (voice_wave_range_set(voice, start, end) != 0) {
            ctx->printf(ctx,
              "# VS rejected for v%d: %d..%d must be within 0..%d\n",
              voice, start, end, sv.table_size[voice]);
          }
        }
      } else if (argc == 0) {
        voice_wave_range_reset(voice);
      }
      return 0;
}
static skode_word_t word_VS = { WID("VS"), .execute = word_exec_VS, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_VL(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc >= 2) {
        int start, end;
        if (skode_double_to_int(arg[0], &start) &&
            skode_double_to_int(arg[1], &end)) {
          if (voice_loop_points_set(voice, start, end) != 0) {
            ctx->printf(ctx,
              "# VL rejected for v%d: %d..%d must be within VS %d..%d\n",
              voice, start, end, sv.wave_range_start[voice],
              sv.wave_range_end[voice]);
          }
        }
      } else if (argc == 0) {
        voice_loop_points_reset(voice);
      }
      return 0;
}
static skode_word_t word_VL = { WID("VL"), .execute = word_exec_VL, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_VW(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int target_voice = voice;
        int w = WAVE_DISPLAY_DEFAULT_WIDTH;
        int h = WAVE_DISPLAY_DEFAULT_HEIGHT;
        if (argc == 1) {
          int parsed_voice;
          if (skode_double_to_int(arg[0], &parsed_voice)) target_voice = parsed_voice;
        } else if (argc == 2) {
          w = wave_display_dim(arg[0], w, WAVE_DISPLAY_MIN_WIDTH, WAVE_DISPLAY_MAX_WIDTH);
          h = wave_display_dim(arg[1], h, WAVE_DISPLAY_MIN_HEIGHT, WAVE_DISPLAY_MAX_HEIGHT);
        } else if (argc >= 3) {
          int parsed_voice;
          if (skode_double_to_int(arg[0], &parsed_voice)) target_voice = parsed_voice;
          w = wave_display_dim(arg[1], w, WAVE_DISPLAY_MIN_WIDTH, WAVE_DISPLAY_MAX_WIDTH);
          h = wave_display_dim(arg[2], h, WAVE_DISPLAY_MIN_HEIGHT, WAVE_DISPLAY_MAX_HEIGHT);
        }
        if (target_voice >= 0 && target_voice < synth_config.voice_max) {
          int wave = sv.wave_table_index[target_voice];
          if (skode_wave_valid(wave)) {
            // ctx->printf(ctx, "# wave [%d..%d)\n", sv.wave_range_start[target_voice], sv.wave_range_end[target_voice]);
            char label[96];
            snprintf(label, sizeof(label), "voice %d wave %d", target_voice, wave);
            wavetable_waveform_show(ctx, wave, w, h,
              sv.loop_start[target_voice], sv.loop_end[target_voice], label);
          }
        }
      }
      return 0;
}
static skode_word_t word_VW = { WID("VW"), .execute = word_exec_VW, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_w(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && wave_set(voice, x) == 0) {
        int n;
        if (argc > 1) {
          if (skode_double_to_int(arg[1], &n)) sv.interpolate[voice] = n != 0;
        }
        if (argc > 2) {
          if (skode_double_to_int(arg[2], &n)) sv.one_shot[voice] = n != 0;
        } else sv.one_shot[voice] = sw.one_shot[x];
        osc_reclassify(&skred_global_engine, voice);
      }
      return 0;
}
static skode_word_t word_w = { WID("w"), .execute = word_exec_w, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__eqd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__eqd = { WID("=d"), .execute = word_exec__eqd, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_d_star(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word_d_star = { WID("d*"), .execute = word_exec_d_star, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_d_gtr(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word_d_gtr = { WID("d>r"), .execute = word_exec_d_gtr, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_r_gtd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word_r_gtd = { WID("r>d"), .execute = word_exec_r_gtd, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_d_gtMO(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        double *data = ands_data(ctx->parse);
        int data_len = ands_data_len(ctx->parse);
        if (!data || data_len <= 0 || data_len > 65536) {
          ctx->printf(ctx, "# d>MO requires 1..65536 data bytes\n");
          return 0;
        }
        uint8_t *bytes = (uint8_t*)malloc((size_t)data_len);
        if (!bytes) {
          ctx->printf(ctx, "# d>MO allocation failed\n");
          return 0;
        }
        int valid = 1;
        for (int i = 0; i < data_len; i++) {
          int byte;
          if (!skode_double_to_int(data[i], &byte) || byte < 0 || byte > 255 ||
              data[i] != (double)byte) {
            valid = 0;
            break;
          }
          bytes[i] = (uint8_t)byte;
        }
        int result = valid ? skred_midi_send_raw(bytes, data_len) : -2;
        free(bytes);
        if (result != 0)
          ctx->printf(ctx, "# MIDI output failed (%d)\n", result);
      }
      return 0;
}
static skode_word_t word_d_gtMO = { WID("d>MO"), .execute = word_exec_d_gtMO, .safety = WORD_IMMEDIATE_ONLY };

#ifdef SKRED_FEATURE_KSYNTH
static int word_exec_d_gtk(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word_d_gtk = { WID("d>k"), .execute = word_exec_d_gtk, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_KSYNTH
static int word_exec_w_gtk(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 1) {
        int wave;
        int variable;
        if (!skode_double_to_int(arg[0], &wave) ||
            !skode_double_to_int(arg[1], &variable) ||
            !skode_wave_valid(wave) || !sw.data[wave] || sw.size[wave] <= 0) {
          ctx->printf(ctx, "# invalid wavetable for w>k\n");
          break;
        }
        size_t len = (size_t)sw.size[wave];
        if (len > 1000000 || len > SIZE_MAX / sizeof(double)) {
          ctx->printf(ctx, "# ksynth vector too large: %zu\n", len);
          break;
        }
        double *values = malloc(len * sizeof(double));
        if (!values) {
          ctx->printf(ctx, "# allocation failed\n");
          break;
        }
        for (size_t i = 0; i < len; i++) values[i] = sw.data[wave][i];
        skode_ks_bind_values(ctx, variable, values, len);
        free(values);
      }
      return 0;
}
static skode_word_t word_w_gtk = { WID("w>k"), .execute = word_exec_w_gtk, .safety = WORD_IMMEDIATE_ONLY };
#endif

static int word_exec_w_gtd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (x_valid && skode_wave_valid(x) && sw.data[x] && sw.size[x] > 0) {
        if (sw.size[x] > ands_data_cap(ctx->parse)) ands_data_resize(ctx->parse, sw.size[x]);
        double *data = ands_data(ctx->parse);
        if (!data || sw.size[x] > ands_data_cap(ctx->parse)) return 0;
        for (int i=0; i<sw.size[x]; i++) data[i] = sw.data[x][i];
        ands_data_len_set(ctx->parse, sw.size[x]);
      }
      return 0;
}
static skode_word_t word_w_gtd = { WID("w>d"), .execute = word_exec_w_gtd, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_w_gtr(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (x_valid && skode_wave_valid(x) && sw.data[x] && sw.size[x] > 0) {
        int valid = 1;
        int sample_state = atomic_load_int(&sampling.state);
        if (sample_state == SAMPLE_STATE_ARMED ||
            sample_state == SAMPLE_STATE_RECORDING) {
          valid = 0;
          ctx->printf(ctx, "# recording buffer busy\n");
        } else if (sw.size[x] > sampling.capacity) {
          skode_sample_alloc(sw.size[x]);
          valid = sampling.where != NULL && sampling.capacity >= sw.size[x];
        }
        if (valid) {
          sampling.offset = 0;
          sampling.trim = 0;
          for (int i=0; i<sw.size[x]; i++) sampling.where[i] = sw.data[x][i];
          sampling.len = sw.size[x];
          sampling.channels = 1;
          atomic_store_int(&sampling.state, SAMPLE_STATE_COMPLETE);
        }
      }
      return 0;
}
static skode_word_t word_w_gtr = { WID("w>r"), .execute = word_exec_w_gtr, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_w_gtw(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (!ands_string_fresh(ctx->parse) ||
          !ands_string(ctx->parse)[0]) {
        ctx->printf(ctx, "# w>w requires [filename]\n");
      } else if (!x_valid || !skode_wave_valid(x) ||
                 !sw.data[x] || sw.size[x] <= 0) {
        ctx->printf(ctx, "# invalid wavetable for w>w\n");
      } else {
        double stored_rate = sw.rate[x];
        ma_uint32 sample_rate = MAIN_SAMPLE_RATE;
        if (isfinite(stored_rate) && stored_rate >= 1.0 &&
            stored_rate <= (double)UINT32_MAX - 0.5) {
          sample_rate = (ma_uint32)(stored_rate + 0.5);
        }
        skode_write_wav(ctx, ands_string(ctx->parse), sw.data[x],
                        sw.size[x], 1, sample_rate, 0);
      }
      return 0;
}
static skode_word_t word_w_gtw = { WID("w>w"), .execute = word_exec_w_gtw, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_w_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        if (atomic_load_int(&sampling.state) != SAMPLE_STATE_COMPLETE ||
            !sampling.where || sampling.offset < 0 || sampling.trim < 0 ||
            sampling.offset > sampling.len ||
            sampling.trim > sampling.len - sampling.offset) {
          ctx->printf(ctx, "# invalid recording bounds\n");
          return 0;
        }
        int channels = sampling.channels == 2 ? 2 : 1;
        int new_len = sampling.len - sampling.offset - sampling.trim;
        memmove(sampling.where,
                sampling.where + (size_t)sampling.offset * channels,
                (size_t)new_len * channels * sizeof(float));
        sampling.len = new_len;
        sampling.trim = 0;
        sampling.offset = 0;
      }
      return 0;
}
static skode_word_t word_w_bang = { WID("w!"), .execute = word_exec_w_bang, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_w_star(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (atomic_load_int(&sampling.state) == SAMPLE_STATE_COMPLETE) {
        sampling.offset = 0;
        sampling.trim = 0;
      } else {
        ctx->printf(ctx, "# recording buffer is not complete\n");
      }
      return 0;
}
static skode_word_t word_w_star = { WID("w*"), .execute = word_exec_w_star, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_w_gt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (atomic_load_int(&sampling.state) != SAMPLE_STATE_COMPLETE) {
        ctx->printf(ctx, "# recording buffer is not complete\n");
        return 0;
      }
      if (argc == 0) x = 1;
      if (argc == 0 || x_valid) {
        long long next = (long long)sampling.offset + x;
        if (next < 0) next = 0;
        if (next > sampling.len) next = sampling.len;
        sampling.offset = (int)next;
        if (sampling.trim > sampling.len - sampling.offset)
          sampling.trim = sampling.len - sampling.offset;
      }
      return 0;
}
static skode_word_t word_w_gt = { WID("w>"), .execute = word_exec_w_gt, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_w_lt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (atomic_load_int(&sampling.state) != SAMPLE_STATE_COMPLETE) {
        ctx->printf(ctx, "# recording buffer is not complete\n");
        return 0;
      }
      if (argc == 0) x = 1;
      if (argc == 0 || x_valid) {
        long long next = (long long)sampling.trim + x;
        int max_trim = sampling.len - sampling.offset;
        if (max_trim < 0) max_trim = 0;
        if (next < 0) next = 0;
        if (next > max_trim) next = max_trim;
        sampling.trim = (int)next;
      }
      return 0;
}
static skode_word_t word_w_lt = { WID("w<"), .execute = word_exec_w_lt, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_w_lt_gt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        float arg0 = -1;
        float arg1 = -1;
        int margin = 0;
        if (argc > 0) arg0 = arg[0];
        if (argc > 1) arg1 = arg[1];
        if (argc > 2) skode_double_to_int(arg[2], &margin);
        record_find_trim(argc, arg0, arg1, margin);
      }
      return 0;
}
static skode_word_t word_w_lt_gt = { WID("w<>"), .execute = word_exec_w_lt_gt, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_WS(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && arg[0] >= 0) {
        int w = WAVE_DISPLAY_DEFAULT_WIDTH;
        int h = WAVE_DISPLAY_DEFAULT_HEIGHT / 2;
        wavetable_spectrogram_show(ctx, x, w, h, sw.loop_start[x], sw.loop_end[x], NULL);
      }
      return 0;
}
static skode_word_t word_WS = { WID("WS"), .execute = word_exec_WS, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_W(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        int w = WAVE_DISPLAY_DEFAULT_WIDTH;
        int h = WAVE_DISPLAY_DEFAULT_HEIGHT;
        int m = 0;
        int wave_max = synth_config.wave_table_max - 1;
        int show_record_buffer = (arg[0] < 0 || isnan(arg[0]));
        if (show_record_buffer) {
          if (argc > 1) {
            w = wave_display_dim(arg[1], w, WAVE_DISPLAY_MIN_WIDTH, WAVE_DISPLAY_MAX_WIDTH);
          }
          if (argc > 2) {
            h = wave_display_dim(arg[2], h, WAVE_DISPLAY_MIN_HEIGHT, WAVE_DISPLAY_MAX_HEIGHT);
          }
        } else if (argc == 2) {
          if (isnan(arg[1])) m = wave_max;
          else if (!skode_double_to_int(arg[1], &m)) m = x;
          if (m < x) m = x;
          if (m > wave_max) m = wave_max;
        } else if (argc >= 3) {
          w = wave_display_dim(arg[1], w, WAVE_DISPLAY_MIN_WIDTH, WAVE_DISPLAY_MAX_WIDTH);
          h = wave_display_dim(arg[2], h, WAVE_DISPLAY_MIN_HEIGHT, WAVE_DISPLAY_MAX_HEIGHT);
        }
        if (!show_record_buffer && skode_wave_valid(x)) {
        if (m == 0) {
            wavetable_waveform_show(ctx, x, w, h, sw.loop_start[x],
              sw.loop_end[x], NULL);
          } else {
            for (int i=x; i<=m; i++) {
              wavetable_show(ctx, i);
            }
          }
        } else {
          if (atomic_load_int(&sampling.state) != SAMPLE_STATE_COMPLETE) {
            ctx->printf(ctx, "# recording buffer is not complete\n");
          } else if (sampling.where) {
            if ((sampling.offset > sampling.len) || (sampling.len - sampling.trim <= 0)) {
              ctx->printf(ctx,"NO!\n");
              ctx->printf(ctx, "offset:%d\n", sampling.offset);
              ctx->printf(ctx, "trim:%d\n", sampling.trim);
              ctx->printf(ctx, "len:%d\n", sampling.len);
              ctx->printf(ctx, "where:%p\n", sampling.where);
              ctx->printf(ctx, "state:%d\n",
                          atomic_load_int(&sampling.state));
            } else {
              float *display = sampling.where;
              float *mono = NULL;
              if (sampling.channels == 2) {
                mono = malloc((size_t)sampling.len * sizeof(float));
                if (mono) {
                  for (int i = 0; i < sampling.len; i++)
                    mono[i] = record_frame_mono(i);
                  display = mono;
                }
              }
              print_wave_stats(ctx, "recording", display, sampling.len,
                               (float)MAIN_SAMPLE_RATE);
              print_audio_braille_labeled(ctx, display, sampling.len, w, h,
                sampling.offset, sampling.len - sampling.trim);
              free(mono);
              int len = sampling.len - sampling.offset - sampling.trim;
              ctx->printf(ctx, "# recording channels %d\n",
                          sampling.channels == 2 ? 2 : 1);
              ctx->printf(ctx,"# found start %d end %d |%d| %gms\n",
                sampling.offset, sampling.len - sampling.trim, len,
                SAMPLES_TO_MSEC(len));
              ctx->printf(ctx,"+offset %d -trim %d = |%d| %gms\n",
                sampling.offset, sampling.trim, len,
                SAMPLES_TO_MSEC(len));
            }
          }
        }
      } else if (argc == 0) {
        int c = 0;
        ctx->printf(ctx, "# MAX %d\n", synth_config.wave_table_max);
        for (int i=0; i<synth_config.wave_table_max; i++) {
          wavetable_show(ctx, i);
          c++;
        }
      }
      return 0;
}
static skode_word_t word_W = { WID("W"), .execute = word_exec_W, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_xg(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

}
static skode_word_t word_xg = { WID("xg"), .execute = word_exec_xg, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__gtx(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      seq_step_goto(ctx->pattern, x);
      return 0;
}
static skode_word_t word__gtx = { WID(">x"), .execute = word_exec__gtx, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_xa(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word_xa = { WID("xa"), .execute = word_exec_xa, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__ltx(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__ltx = { WID("<x"), .execute = word_exec__ltx, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_x(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
          skode_compile_result_t result = source_only ?
            SKODE_COMPILE_OK : skode_compile_program(source, &program);
          if (result == SKODE_COMPILE_OK) {
            seq_step_set(ctx->pattern, ctx->step, source,
              source_only ? NULL : &program);
          } else {
            ctx->printf(ctx, "# sequence command is not schedulable (%d)\n", result);
          }
        }
      }
      return 0;
}
static skode_word_t word_x = { WID("x"), .execute = word_exec_x, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_y(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word_y = { WID("y"), .execute = word_exec_y, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_ys_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int p = (argc && x >= 0 && x < PATTERNS_MAX) ? x : ctx->pattern;
        pattern_show(ctx, p, 1);
      }
      return 0;
}
static skode_word_t word_ys_q = { WID("ys?"), .execute = word_exec_ys_q, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word_ys = { WID("ys"), .execute = word_exec_ys_q, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_yt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (ctx->pattern >= 0 && ctx->pattern < PATTERNS_MAX) {
        seq_edit_lock();
        skode_copy_string(seq_text[ctx->pattern], TEXT_MAX, ands_string(ctx->parse));
        seq_edit_unlock();
      }
      return 0;
}
static skode_word_t word_yt = { WID("yt"), .execute = word_exec_yt, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_ym(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        seq_mute_set(ctx->pattern, x);
        skred_control_pattern_event(SKRED_CONTROL_EVENT_MUTE_CHANGE, SAMPLE_COUNT_GET(), ctx->pattern, x);
      }
      return 0;
}
static skode_word_t word_ym = { WID("ym"), .execute = word_exec_ym, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_yc(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) seq_control_events_set(ctx->pattern, x);
      return 0;
}
static skode_word_t word_yc = { WID("yc"), .execute = word_exec_yc, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_Y(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && x >= 0 && x < PATTERNS_MAX) {
        pattern_reset(x);
      }
      return 0;
}
static skode_word_t word_Y = { WID("Y"), .execute = word_exec_Y, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_z(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        seq_state_set(ctx->pattern, x);
      } else pattern_show(ctx, ctx->pattern, 1);
      return 0;
}
static skode_word_t word_z = { WID("z"), .execute = word_exec_z, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_zg(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && x >= 0 && x < SEQ_STEPS_MAX) {
        seq_step_goto(ctx->pattern, x);
      }
      return 0;
}
static skode_word_t word_zg = { WID("zg"), .execute = word_exec_zg, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_zq(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        seq_state_queue(ctx->pattern, x);
        skred_control_pattern_event(SKRED_CONTROL_EVENT_PATTERN_QUEUE, SAMPLE_COUNT_GET(), ctx->pattern, x);
      }
      return 0;
}
static skode_word_t word_zq = { WID("zq"), .execute = word_exec_zq, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_z_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      pattern_show(ctx, ctx->pattern, 1);
      return 0;
}
static skode_word_t word_z_q = { WID("z?"), .execute = word_exec_z_q, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_Z(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word_Z = { WID("Z"), .execute = word_exec_Z, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_z_q_bs_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

}
static skode_word_t word_z_q_bs_q = { WID("z?\?"), .execute = word_exec_z_q_bs_q, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_Z_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "M%g\n", tempo_bpm_get());
      for (int p = 0; p < PATTERNS_MAX; p++) {
        if (seq_pattern_length[p] > 0 || seq_text[p][0] != '\0')
          pattern_show(ctx, p, 1);
      }
      return 0;
}
static skode_word_t word_Z_q = { WID("Z?"), .execute = word_exec_Z_q, .safety = WORD_IMMEDIATE_ONLY };

#ifdef SKRED_FEATURE_XM
static int word_exec_XM(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        sv.ring_osc[voice] = x_valid && skode_voice_valid(x) ? x : -1;
        if (argc > 1) sv.ring_amount[voice] = arg[1];
        else sv.ring_amount[voice] = 0.0;
      }
      return 0;
}
static skode_word_t word_XM = { WID("XM"), .execute = word_exec_XM, .safety = WORD_IMMEDIATE_ONLY };
#endif

static int word_exec_v_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

}
static skode_word_t word_v_q = { WID("v?"), .execute = word_exec_v_q, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      voice_show(ctx, voice, ' ', ctx->verbose); return 0;
}
static skode_word_t word__q = { WID("?"), .execute = word_exec__q, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__bs_bs(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      voice_show(ctx, voice, ' ', 1); return 0;
}
static skode_word_t word__bs_bs = { WID("\\"), .execute = word_exec__bs_bs, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_v_q_bs_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

}
static skode_word_t word_v_q_bs_q = { WID("v?\?"), .execute = word_exec_v_q_bs_q, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__q_bs_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      voice_show_all(ctx, voice, ctx->verbose); return 0;
}
static skode_word_t word__q_bs_q = { WID("?\?"), .execute = word_exec__q_bs_q, .safety = WORD_IMMEDIATE_ONLY };

#if defined(SKRED_FEATURE_RECORD) || defined(SKRED_FEATURE_SCOPE) || defined(SKRED_FEATURE_TRACKS)
static int word_exec__qr(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      record_tracks_show(ctx); return 0;
}
static skode_word_t word__qr = { WID("?r"), .execute = word_exec__qr, .safety = WORD_IMMEDIATE_ONLY };
#endif

static int word_exec__qs(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "# [%s]\n", ands_string(ctx->parse));
      return 0;
}
static skode_word_t word__qs = { WID("?s"), .execute = word_exec__qs, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_s_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word_s_q = { WID("s?"), .execute = word_exec_s_q, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__qm(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      skode_macros_show(ctx, 0);
      return 0;
}
static skode_word_t word__qm = { WID("?m"), .execute = word_exec__qm, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__qce(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      control_event_show(ctx, 0);
      return 0;
}
static skode_word_t word__qce = { WID("?ce"), .execute = word_exec__qce, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__qce_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "# control events cleared:%d\n",
        skred_control_event_clear());
      return 0;
}
static skode_word_t word__qce_bang = { WID("?ce!"), .execute = word_exec__qce_bang, .safety = WORD_IMMEDIATE_ONLY };

#ifdef SKRED_FEATURE_SEQ
static int word_exec__qq(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      opcode_queue_show(ctx);
      return 0;
}
static skode_word_t word__qq = { WID("?q"), .execute = word_exec__qq, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_SEQ
static int word_exec__qo(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc == 0) {
        opcode_queue_show(ctx);
      } else {
        if (!x_valid) {
          ctx->printf(ctx, "# invalid opcode pattern\n");
          break;
        }
        int pattern = x;
        int step = -1;
        if (pattern == -1) pattern = ctx->pattern;
        if (argc > 1 && !skode_double_to_int(arg[1], &step)) {
          ctx->printf(ctx, "# invalid opcode step\n");
          break;
        }
        opcode_pattern_show(ctx, pattern, step);
      }
      return 0;
}
static skode_word_t word__qo = { WID("?o"), .execute = word_exec__qo, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_BENCH
static int word_exec__slashm_(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      synth_voice_bench(voice);
      return 0;
}
static skode_word_t word__slashm_ = { WID("/m_"), .execute = word_exec__slashm_, .safety = WORD_IMMEDIATE_ONLY };
#endif

static int word_exec__slashq(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->quit = -1;
      return 0;
}
static skode_word_t word__slashq = { WID("/q"), .execute = word_exec__slashq, .safety = WORD_IMMEDIATE_ONLY };

#ifdef SKRED_FEATURE_SCOPE
static int word_exec__slashsg(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        const char *name = ands_string_fresh(ctx->parse)
          ? ands_string(ctx->parse) : SKRED_SCOPE_DEFAULT_NAME;
        uint32_t channel_mask = SKRED_SCOPE_ALL_CHANNELS;
        double buffer_seconds = SKRED_SCOPE_DEFAULT_SECONDS;
        int mask = 0;
        if (!name || name[0] == '\0') name = SKRED_SCOPE_DEFAULT_NAME;
        if (argc > 0) {
          if (!skode_double_to_int(arg[0], &mask) || mask <= 0 ||
              (uint32_t)mask > SKRED_SCOPE_ALL_CHANNELS) {
            ctx->printf(ctx, "# /sg channel mask must be 1..%u\n",
                        SKRED_SCOPE_ALL_CHANNELS);
            break;
          }
          channel_mask = (uint32_t)mask;
        }
        if (argc > 1) buffer_seconds = arg[1];
        if (!isfinite(buffer_seconds) || buffer_seconds <= 0.0) {
          ctx->printf(ctx, "# /sg buffer seconds must be > 0\n");
        } else if (scope_ipc_start(name, channel_mask,
                                   buffer_seconds) == 0) {
          skred_scope_status_t status;
          scope_ipc_status(&status);
          ctx->printf(ctx,
            "# scope [%s] channels=%u mask=%u capacity=%u frames\n",
            status.name, status.channel_count, status.channel_mask,
            status.capacity_frames);
        } else {
          ctx->printf(ctx, "# scope start failed [%s]\n", name);
        }
      }
      return 0;
}
static skode_word_t word__slashsg = { WID("/sg"), .execute = word_exec__slashsg, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_SCOPE
static int word_exec__slashss(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      scope_ipc_stop();
      ctx->printf(ctx, "# scope stopped\n");
      return 0;
}
static skode_word_t word__slashss = { WID("/ss"), .execute = word_exec__slashss, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_SCOPE
static int word_exec__slashs_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        skred_scope_status_t status;
        scope_ipc_status(&status);
        if (status.active) {
          ctx->printf(ctx,
            "# scope state=publishing name=[%s] rate=%d channels=%d mask=%u capacity=%u frames=%llu\n",
            status.name, status.sample_rate, status.channel_count,
            status.channel_mask, status.capacity_frames,
            (unsigned long long)status.write_frame);
        } else {
          ctx->printf(ctx, "# scope state=stopped\n");
        }
      }
      return 0;
}
static skode_word_t word__slashs_q = { WID("/s?"), .execute = word_exec__slashs_q, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_RECORD
static int word_exec__slashrg(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__slashrg = { WID("/rg"), .execute = word_exec__slashrg, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_RECORD
static int word_exec__slashrs(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (recorder_stop() == 0) {
        ctx->printf(ctx, "# recording stopped\n");
      } else {
        ctx->printf(ctx, "# recording stop failed\n");
      }
      return 0;
}
static skode_word_t word__slashrs = { WID("/rs"), .execute = word_exec__slashrs, .safety = WORD_IMMEDIATE_ONLY };
#endif

#ifdef SKRED_FEATURE_RECORD
static int word_exec__slashr_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__slashr_q = { WID("/r?"), .execute = word_exec__slashr_q, .safety = WORD_IMMEDIATE_ONLY };
#endif

static int word_exec__slashr(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__slashr = { WID("/r"), .execute = word_exec__slashr, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__slashd = { WID("/d"), .execute = word_exec__slashd, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashf(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) { ctx->flag = x; }
      else { ctx->printf(ctx, "# /f%d\n", ctx->flag); }
      return 0;
}
static skode_word_t word__slashf = { WID("/f"), .execute = word_exec__slashf, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashff(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__slashff = { WID("/ff"), .execute = word_exec__slashff, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashm(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__slashm = { WID("/m"), .execute = word_exec__slashm, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashm_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ands_macro_clear(ctx->parse);
      ctx->printf(ctx, "# macros cleared\n");
      return 0;
}
static skode_word_t word__slashm_bang = { WID("/m!"), .execute = word_exec__slashm_bang, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slasht(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc == 0) x = (ctx->trace) ? 0 : 1;
      ctx->trace = x;
      ands_trace_set(s, x > 1);
      return 0;
}
static skode_word_t word__slasht = { WID("/t"), .execute = word_exec__slasht, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashv(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc == 0) x = (ctx->verbose) ? 0 : 1;
      ctx->verbose = x;
      return 0;
}
static skode_word_t word__slashv = { WID("/v"), .execute = word_exec__slashv, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashcer(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && x_valid) skred_control_response_set_enabled(x != 0);
      ctx->printf(ctx, "%s", skred_control_response_status());
      return 0;
}
static skode_word_t word__slashcer = { WID("/cer"), .execute = word_exec__slashcer, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashce_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "%s", skred_control_response_status());
      return 0;
}
static skode_word_t word__slashce_q = { WID("/ce?"), .execute = word_exec__slashce_q, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashth_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "%s", skred_thread_status());
      return 0;
}
static skode_word_t word__slashth_q = { WID("/th?"), .execute = word_exec__slashth_q, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashth_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      skred_performance_reset();
      ctx->printf(ctx, "# performance counters reset\n");
      return 0;
}
static skode_word_t word__slashth_bang = { WID("/th!"), .execute = word_exec__slashth_bang, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashce_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__slashce_bang = { WID("/ce!"), .execute = word_exec__slashce_bang, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashceb(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__slashceb = { WID("/ceb"), .execute = word_exec__slashceb, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashcex(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__slashcex = { WID("/cex"), .execute = word_exec__slashcex, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__lts(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && x_valid && x >= 0 && x < SKODE_STRING_SLOT_MAX) {
        ands_string_from_external(ctx->parse, ctx->string_slot[x],
                                  strlen(ctx->string_slot[x]));
      }
      return 0;
}
static skode_word_t word__lts = { WID("<s"), .execute = word_exec__lts, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_s_gt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && x_valid && x >= 0 && x < SKODE_STRING_SLOT_MAX) {
        skode_copy_string(ctx->string_slot[x], SKODE_STRING_SLOT_LEN,
                          ands_string(ctx->parse));
      }
      return 0;
}
static skode_word_t word_s_gt = { WID("s>"), .execute = word_exec_s_gt, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_s_pct(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        char formatted[SKODE_STRING_SLOT_LEN];
        skode_format_string_args(formatted, sizeof(formatted),
                                 ands_string(ctx->parse), arg, argc);
        ands_string_from_external(ctx->parse, formatted, strlen(formatted));
        return 1;
      }
}
static skode_word_t word_s_pct = { WID("s%"), .execute = word_exec_s_pct, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__lte(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && skode_extra_valid(x)) {
        char macro[STRING_BUF_LEN];
        if (skode_extra_copy(x, macro, sizeof(macro)) == 0)
          ands_string_from_external(ctx->parse, macro, strlen(macro));
      }
      return 0;
}
static skode_word_t word__lte = { WID("<e"), .execute = word_exec__lte, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_e_gt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && skode_extra_valid(x)) {
        char *s = ands_string(ctx->parse);
        simple_mutex_lock(&skode_extra_mutex);
        skode_copy_string(EXTRA_PTR(x), STRING_BUF_LEN, s);
        simple_mutex_unlock(&skode_extra_mutex);
      }
      return 0;
}
static skode_word_t word_e_gt = { WID("e>"), .execute = word_exec_e_gt, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_e_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word_e_bang = { WID("e!"), .execute = word_exec_e_bang, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_e_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word_e_q = { WID("e?"), .execute = word_exec_e_q, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashs(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
            #ifdef SKRED_FEATURE_BENCH
            case 1: show_threads(ctx); break;
            case 4: show_stats(ctx); break;
            case 6: ctx->printf(ctx, "%s", seq_stats()); break;
            #endif
          }
        }
      }
      return 0;
}
static skode_word_t word__slashs = { WID("/s"), .execute = word_exec__slashs, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashh(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      skode_help(ctx, arg, argc);
      return 0;
}
static skode_word_t word__slashh = { WID("/h"), .execute = word_exec__slashh, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashl(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        int verbose = 0;
        if (argc > 1) skode_double_to_int(arg[1], &verbose);
        skode_load(ctx, voice, x, verbose);
      }
      return 0;
}
static skode_word_t word__slashl = { WID("/l"), .execute = word_exec__slashl, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashls(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__slashls = { WID("/ls"), .execute = word_exec__slashls, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashws(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__slashws = { WID("/ws"), .execute = word_exec__slashws, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashw(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__slashw = { WID("/w"), .execute = word_exec__slashw, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__gtr(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__gtr = { WID(">r"), .execute = word_exec__gtr, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__hatr(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

}
static skode_word_t word__hatr = { WID("^r"), .execute = word_exec__hatr, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__ltr(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__ltr = { WID("<r"), .execute = word_exec__ltr, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__gt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (x_valid && skode_voice_valid(x)) voice_copy(voice, x);
      return 0;
}
static skode_word_t word__gt = { WID(">"), .execute = word_exec__gt, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slash(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      wave_default(voice);
      return 0;
}
static skode_word_t word__slash = { WID("/"), .execute = word_exec__slash, .safety = WORD_IMMEDIATE_ONLY };

#ifdef SKRED_FEATURE_SEQ
static int word_exec__pct(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) seq_modulo_set(ctx->pattern, x);
      return 0;
}
static skode_word_t word__pct = { WID("%"), .execute = word_exec__pct, .safety = WORD_IMMEDIATE_ONLY };
#endif

static int word_exec_W_star(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word_W_star = { WID("W*"), .execute = word_exec_W_star, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_v_star(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word_v_star = { WID("v*"), .execute = word_exec_v_star, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__star_eq(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__star_eq = { WID("*="), .execute = word_exec__star_eq, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slash_eq(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__slash_eq = { WID("/="), .execute = word_exec__slash_eq, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_a_eq(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word_a_eq = { WID("a="), .execute = word_exec_a_eq, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec_s_eq(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word_s_eq = { WID("s="), .execute = word_exec_s_eq, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__eq(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__eq = { WID("="), .execute = word_exec__eq, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__slashwex(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && x >= 200 && x <=999) wave_table_dynamic_expand(x);
      return 0;
}
static skode_word_t word__slashwex = { WID("/wex"), .execute = word_exec__slashwex, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__pctz(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__pctz = { WID("%z"), .execute = word_exec__pctz, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__pctzu(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      skred_vfs_unmount();
      ctx->printf(ctx, "# vfs %s\n", skred_vfs_status());
      return 0;
}
static skode_word_t word__pctzu = { WID("%zu"), .execute = word_exec__pctzu, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__pctpwd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "# vfs %s\n", skred_vfs_status());
      return 0;
}
static skode_word_t word__pctpwd = { WID("%pwd"), .execute = word_exec__pctpwd, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__pctcat(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__pctcat = { WID("%cat"), .execute = word_exec__pctcat, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__pctcd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "# [%s] %%cd\n", ands_string(ctx->parse));
      if (strlen(ands_string(ctx->parse))) {
        if (!skred_chdir(ands_string(ctx->parse)))
          ctx->printf(ctx, "# cannot cd %s\n", ands_string(ctx->parse));
      }
      ctx->printf(ctx, "# vfs %s\n", skred_vfs_status());
      return 0;
}
static skode_word_t word__pctcd = { WID("%cd"), .execute = word_exec__pctcd, .safety = WORD_IMMEDIATE_ONLY };

static int word_exec__pctls(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
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
static skode_word_t word__pctls = { WID("%ls"), .execute = word_exec__pctls, .safety = WORD_IMMEDIATE_ONLY };


void skode_register_immediate_words(skode_vocab_t *vocab) {
  skode_dict_register(vocab, &word__slashmb_q);

  skode_dict_register(vocab, &word__slashmbC);

  skode_dict_register(vocab, &word__slashpg);

  skode_dict_register(vocab, &word__slashpg_bang);

  skode_dict_register(vocab, &word__slashpp);

  skode_dict_register(vocab, &word__slashpp_bang);

  skode_dict_register(vocab, &word__slashpm);

  skode_dict_register(vocab, &word__qpg);

  skode_dict_register(vocab, &word__qpp);

  skode_dict_register(vocab, &word__slashvg);

  skode_dict_register(vocab, &word_pn);

  skode_dict_register(vocab, &word_pr);

  skode_dict_register(vocab, &word_pb);

  skode_dict_register(vocab, &word_ab);

  skode_dict_register(vocab, &word_abp);

#ifdef SKRED_FEATURE_AM
  skode_dict_register(vocab, &word_A);
#endif

  skode_dict_register(vocab, &word_b);

  skode_dict_register(vocab, &word_B);

  skode_dict_register(vocab, &word_BC);

#ifdef SKRED_FEATURE_PD
  skode_dict_register(vocab, &word_c);
#endif

#ifdef SKRED_FEATURE_PD
  skode_dict_register(vocab, &word_C);
#endif

#ifdef SKRED_FEATURE_PD
  skode_dict_register(vocab, &word_ct);
#endif

#ifdef SKRED_FEATURE_PD
  skode_dict_register(vocab, &word_cd);
#endif

  skode_dict_register(vocab, &word_D);

  skode_dict_register(vocab, &word_MO);

  skode_dict_register(vocab, &word_ce);

  skode_dict_register(vocab, &word__qd);

  skode_dict_register(vocab, &word_fb);

  skode_dict_register(vocab, &word_fbp);

#if defined(SKRED_FEATURE_FILT) && defined(SKRED_FEATURE_FADSR)
  skode_dict_register(vocab, &word_ft);
#endif

#if defined(SKRED_FEATURE_FILT) && defined(SKRED_FEATURE_FADSR)
  skode_dict_register(vocab, &word_fd);
#endif

#ifdef SKRED_FEATURE_FM
  skode_dict_register(vocab, &word_F);
#endif

#ifdef SKRED_FEATURE_FM
  skode_dict_register(vocab, &word_FF);
#endif

#ifdef SKRED_FEATURE_FM
  skode_dict_register(vocab, &word_FB);
#endif

#ifdef SKRED_FEATURE_GLISS
  skode_dict_register(vocab, &word_g);
#endif

  skode_dict_register(vocab, &word_G);

#ifdef SKRED_FEATURE_SAH
  skode_dict_register(vocab, &word_h);
#endif

  skode_dict_register(vocab, &word_H);

  skode_dict_register(vocab, &word__slashD);

  skode_dict_register(vocab, &word_I);

  skode_dict_register(vocab, &word_L);

#ifdef SKRED_FEATURE_FILT
  skode_dict_register(vocab, &word_J);
#endif

#ifdef SKRED_FEATURE_FILT
  skode_dict_register(vocab, &word_K);
#endif

#ifdef SKRED_FEATURE_KSYNTH
  skode_dict_register(vocab, &word__slashks);
#endif

#ifdef SKRED_FEATURE_KSYNTH
  skode_dict_register(vocab, &word__slashk);
#endif

#ifdef SKRED_FEATURE_KSYNTH
  skode_dict_register(vocab, &word_ks);
#endif

#ifdef SKRED_FEATURE_KSYNTH
  skode_dict_register(vocab, &word_k_bang);
#endif

#ifdef SKRED_FEATURE_KSYNTH
  skode_dict_register(vocab, &word_kw);
#endif

#ifdef SKRED_FEATURE_KSYNTH
  skode_dict_register(vocab, &word_kw_gt);
#endif

#ifdef SKRED_FEATURE_KSYNTH
  skode_dict_register(vocab, &word_k_q);
#endif

#ifdef SKRED_FEATURE_KSYNTH
  skode_dict_register(vocab, &word_k_gtd);
#endif

#ifdef SKRED_FEATURE_KSYNTH
  skode_dict_register(vocab, &word_k_gtw);
#endif

  skode_dict_register(vocab, &word_k);

#ifdef SKRED_FEATURE_UDP
  skode_dict_register(vocab, &word_udp);
#endif

  skode_dict_register(vocab, &word_log);

  skode_dict_register(vocab, &word____l);

  skode_dict_register(vocab, &word_l);

#ifdef SKRED_FEATURE_SEQ
  skode_dict_register(vocab, &word_M);
#endif

  skode_dict_register(vocab, &word_N);

  skode_dict_register(vocab, &word_ds);

  skode_dict_register(vocab, &word_DG);

  skode_dict_register(vocab, &word_DL);

  skode_dict_register(vocab, &word_DL_q);

  skode_dict_register(vocab, &word_DD);

  skode_dict_register(vocab, &word_DF);

  skode_dict_register(vocab, &word_DP);

  skode_dict_register(vocab, &word_DT);

  skode_dict_register(vocab, &word_DS);

  skode_dict_register(vocab, &word_GS);

  skode_dict_register(vocab, &word_GS_gt);

  skode_dict_register(vocab, &word_GS_lt);

#ifdef SKRED_FEATURE_PANMOD
  skode_dict_register(vocab, &word_P);
#endif

#ifdef SKRED_FEATURE_CRUSH
  skode_dict_register(vocab, &word_q);
#endif

#ifdef SKRED_FEATURE_FILT
  skode_dict_register(vocab, &word_Q);
#endif

#if defined(SKRED_FEATURE_RECORD) || defined(SKRED_FEATURE_SCOPE) || defined(SKRED_FEATURE_TRACKS)
  skode_dict_register(vocab, &word_r);
#endif

#if defined(SKRED_FEATURE_RECORD) || defined(SKRED_FEATURE_SCOPE) || defined(SKRED_FEATURE_TRACKS)
  skode_dict_register(vocab, &word_rt);
#endif

#if defined(SKRED_FEATURE_RECORD) || defined(SKRED_FEATURE_SCOPE) || defined(SKRED_FEATURE_TRACKS)
  skode_dict_register(vocab, &word_rv);
#endif

#ifdef SKRED_FEATURE_SEQ
  skode_dict_register(vocab, &word_R_bang);
#endif

#ifdef SKRED_FEATURE_SEQ
  skode_dict_register(vocab, &word_R_bang_bang);
#endif

#ifdef SKRED_FEATURE_SEQ
  skode_dict_register(vocab, &word_RR);
#endif

#ifdef SKRED_FEATURE_SEQ
  skode_dict_register(vocab, &word_eRR);
#endif

#ifdef SKRED_FEATURE_SEQ
  skode_dict_register(vocab, &word_eR);
#endif

#ifdef SKRED_FEATURE_SEQ
  skode_dict_register(vocab, &word_DO_q);
#endif

#ifdef SKRED_FEATURE_SEQ
  skode_dict_register(vocab, &word_R);
#endif

#ifdef SKRED_FEATURE_SMOOTHER
  skode_dict_register(vocab, &word_s);
#endif

  skode_dict_register(vocab, &word_S);

  skode_dict_register(vocab, &word_t);

  skode_dict_register(vocab, &word_T);

  skode_dict_register(vocab, &word_vc);

  skode_dict_register(vocab, &word_V);

  skode_dict_register(vocab, &word_vt);

  skode_dict_register(vocab, &word_wt);

  skode_dict_register(vocab, &word_WL);

  skode_dict_register(vocab, &word_VS);

  skode_dict_register(vocab, &word_VL);

  skode_dict_register(vocab, &word_VW);

  skode_dict_register(vocab, &word_w);

  skode_dict_register(vocab, &word__eqd);

  skode_dict_register(vocab, &word_d_star);

  skode_dict_register(vocab, &word_d_gtr);

  skode_dict_register(vocab, &word_r_gtd);

  skode_dict_register(vocab, &word_d_gtMO);

#ifdef SKRED_FEATURE_KSYNTH
  skode_dict_register(vocab, &word_d_gtk);
#endif

#ifdef SKRED_FEATURE_KSYNTH
  skode_dict_register(vocab, &word_w_gtk);
#endif

  skode_dict_register(vocab, &word_w_gtd);

  skode_dict_register(vocab, &word_w_gtr);

  skode_dict_register(vocab, &word_w_gtw);

  skode_dict_register(vocab, &word_w_bang);

  skode_dict_register(vocab, &word_w_star);

  skode_dict_register(vocab, &word_w_gt);

  skode_dict_register(vocab, &word_w_lt);

  skode_dict_register(vocab, &word_w_lt_gt);

  skode_dict_register(vocab, &word_WS);

  skode_dict_register(vocab, &word_W);

  skode_dict_register(vocab, &word_xg);

  skode_dict_register(vocab, &word__gtx);

  skode_dict_register(vocab, &word_xa);

  skode_dict_register(vocab, &word__ltx);

  skode_dict_register(vocab, &word_x);

  skode_dict_register(vocab, &word_y);

  skode_dict_register(vocab, &word_ys_q);
  skode_dict_register(vocab, &word_ys);

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

#ifdef SKRED_FEATURE_XM
  skode_dict_register(vocab, &word_XM);
#endif

  skode_dict_register(vocab, &word_v_q);

  skode_dict_register(vocab, &word__q);

  skode_dict_register(vocab, &word__bs_bs);

  skode_dict_register(vocab, &word_v_q_bs_q);

  skode_dict_register(vocab, &word__q_bs_q);

#if defined(SKRED_FEATURE_RECORD) || defined(SKRED_FEATURE_SCOPE) || defined(SKRED_FEATURE_TRACKS)
  skode_dict_register(vocab, &word__qr);
#endif

  skode_dict_register(vocab, &word__qs);

  skode_dict_register(vocab, &word_s_q);

  skode_dict_register(vocab, &word__qm);

  skode_dict_register(vocab, &word__qce);

  skode_dict_register(vocab, &word__qce_bang);

#ifdef SKRED_FEATURE_SEQ
  skode_dict_register(vocab, &word__qq);
#endif

#ifdef SKRED_FEATURE_SEQ
  skode_dict_register(vocab, &word__qo);
#endif

#ifdef SKRED_FEATURE_BENCH
  skode_dict_register(vocab, &word__slashm_);
#endif

  skode_dict_register(vocab, &word__slashq);

#ifdef SKRED_FEATURE_SCOPE
  skode_dict_register(vocab, &word__slashsg);
#endif

#ifdef SKRED_FEATURE_SCOPE
  skode_dict_register(vocab, &word__slashss);
#endif

#ifdef SKRED_FEATURE_SCOPE
  skode_dict_register(vocab, &word__slashs_q);
#endif

#ifdef SKRED_FEATURE_RECORD
  skode_dict_register(vocab, &word__slashrg);
#endif

#ifdef SKRED_FEATURE_RECORD
  skode_dict_register(vocab, &word__slashrs);
#endif

#ifdef SKRED_FEATURE_RECORD
  skode_dict_register(vocab, &word__slashr_q);
#endif

  skode_dict_register(vocab, &word__slashr);

  skode_dict_register(vocab, &word__slashd);

  skode_dict_register(vocab, &word__slashf);

  skode_dict_register(vocab, &word__slashff);

  skode_dict_register(vocab, &word__slashm);

  skode_dict_register(vocab, &word__slashm_bang);

  skode_dict_register(vocab, &word__slasht);

  skode_dict_register(vocab, &word__slashv);

  skode_dict_register(vocab, &word__slashcer);

  skode_dict_register(vocab, &word__slashce_q);

  skode_dict_register(vocab, &word__slashth_q);

  skode_dict_register(vocab, &word__slashth_bang);

  skode_dict_register(vocab, &word__slashce_bang);

  skode_dict_register(vocab, &word__slashceb);

  skode_dict_register(vocab, &word__slashcex);

  skode_dict_register(vocab, &word__lts);

  skode_dict_register(vocab, &word_s_gt);

  skode_dict_register(vocab, &word_s_pct);

  skode_dict_register(vocab, &word__lte);

  skode_dict_register(vocab, &word_e_gt);

  skode_dict_register(vocab, &word_e_bang);

  skode_dict_register(vocab, &word_e_q);

  skode_dict_register(vocab, &word__slashs);

  skode_dict_register(vocab, &word__slashh);

  skode_dict_register(vocab, &word__slashl);

  skode_dict_register(vocab, &word__slashls);

  skode_dict_register(vocab, &word__slashws);

  skode_dict_register(vocab, &word__slashw);

  skode_dict_register(vocab, &word__gtr);

  skode_dict_register(vocab, &word__hatr);

  skode_dict_register(vocab, &word__ltr);

  skode_dict_register(vocab, &word__gt);

  skode_dict_register(vocab, &word__slash);

#ifdef SKRED_FEATURE_SEQ
  skode_dict_register(vocab, &word__pct);
#endif

  skode_dict_register(vocab, &word_W_star);

  skode_dict_register(vocab, &word_v_star);

  skode_dict_register(vocab, &word__star_eq);

  skode_dict_register(vocab, &word__slash_eq);

  skode_dict_register(vocab, &word_a_eq);

  skode_dict_register(vocab, &word_s_eq);

  skode_dict_register(vocab, &word__eq);

  skode_dict_register(vocab, &word__slashwex);

  skode_dict_register(vocab, &word__pctz);

  skode_dict_register(vocab, &word__pctzu);

  skode_dict_register(vocab, &word__pctpwd);

  skode_dict_register(vocab, &word__pctcat);

  skode_dict_register(vocab, &word__pctcd);

  skode_dict_register(vocab, &word__pctls);

}
