#include "skode-internal.h"
#include "scope-ipc.h"

static int word_exec__slashals(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc == 0) {
        (void)skred_audio_command("/als");
        ctx->printf(ctx, "%s\n", skred_audio_message());
      }
      return 0;
}

static int word_exec__slasha_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc == 0) ctx->printf(ctx, "# %s\n", skred_audio_status());
      return 0;
}

static int word_exec__slashai(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc == 1 && x_valid) {
        int is_capture = atom == ATOM4('/ai-');
        int result = 0;
        if (x >= 0) result = skred_audio_refresh();
        if (result == 0) result = skred_audio_select(is_capture, x);
        if (result == 0) ctx->printf(ctx, "# %s\n", skred_audio_status());
        else ctx->printf(ctx, "# audio selection failed: /a%c %d\n",
          is_capture ? 'i' : 'o', x);
      } else {
        ctx->printf(ctx, "# usage: /a%c selection (-1 default%s)\n",
          atom == ATOM4('/ai-') ? 'i' : 'o',
          atom == ATOM4('/ai-') ? ", -2 off" : "");
      }
      return 0;
}

static int word_exec__slashmL(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc == 0) {
        int result = skred_midi_init("pulp");
        if (result == 0) {
          ctx->printf(ctx, "# MIDI inputs\n");
          int count = skred_midi_input_count();
          for (int i = 0; i < count; i++) {
            char name[128] = {0};
            if (skred_midi_input_name(i, name, sizeof(name)) == 0)
              ctx->printf(ctx, "#   %d %s\n", i, name);
          }
          ctx->printf(ctx, "# MIDI outputs\n");
          count = skred_midi_output_count();
          for (int i = 0; i < count; i++) {
            char name[128] = {0};
            if (skred_midi_output_name(i, name, sizeof(name)) == 0)
              ctx->printf(ctx, "#   %d %s\n", i, name);
          }
        } else ctx->printf(ctx, "# MIDI init failed (%d)\n", result);
      }
      return 0;
}

static int word_exec__slashm_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "# %s\n", skred_midi_status());
      return 0;
}

static int word_exec__slashmi(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc == 1 && x_valid) {
        int result = skred_midi_init("pulp");
        if (result == 0) result = atom == ATOM4('/mi-') ?
          skred_midi_input_open(x) : skred_midi_output_open(x);
        if (result != 0) ctx->printf(ctx, "# MIDI open failed (%d)\n", result);
      }
      return 0;
}

static int word_exec__slashmiV(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc == 0) {
        const char *name = ands_string(ctx->parse);
        int result = skred_midi_init(name[0] ? name : "pulp");
        if (result == 0) result = atom == ATOM4('/miV') ?
          skred_midi_input_open_virtual(name) :
          skred_midi_output_open_virtual(name);
        if (result != 0)
          ctx->printf(ctx, "# MIDI virtual open failed (%d)\n", result);
      }
      return 0;
}

static int word_exec__slashmic(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc == 0 && skred_midi_input_close() != 0)
        ctx->printf(ctx, "# MIDI input close failed\n");
      return 0;
}

static int word_exec__slashmoc(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc == 0 && skred_midi_output_close() != 0)
        ctx->printf(ctx, "# MIDI output close failed\n");
      return 0;
}

static int word_exec__slashmv(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int channel = -1, target;
        float bend = 2.0f;
        if (argc >= 2 && argc <= 3 &&
            (isnan(arg[0]) || skode_double_to_int(arg[0], &channel)) &&
            skode_double_to_int(arg[1], &target) &&
            (argc < 3 || (isfinite(arg[2]) && arg[2] >= 0.0))) {
          if (argc == 3) bend = (float)arg[2];
          int kind = atom == ATOM4('/mv-') ? SKRED_MIDI_ROUTE_VOICE :
            SKRED_MIDI_ROUTE_POOL;
          if (skred_midi_route_set(channel, kind, target, bend) == 0 &&
              skred_control_dispatch_start() == 0)
            ctx->printf(ctx, "# MIDI route installed\n");
          else ctx->printf(ctx, "# MIDI route failed\n");
        } else ctx->printf(ctx, "# usage: /m%c channel target [bend]\n",
          atom == ATOM4('/mv-') ? 'v' : 'p');
      }
      return 0;
}

static int word_exec__slashmvd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int channel = -1, target;
        if (argc == 2 &&
            (isnan(arg[0]) || skode_double_to_int(arg[0], &channel)) &&
            skode_double_to_int(arg[1], &target)) {
          int kind = atom == ATOM4('/mvd') ? SKRED_MIDI_ROUTE_VOICE :
            SKRED_MIDI_ROUTE_POOL;
          ctx->printf(ctx, "# MIDI routes removed: %d\n",
            skred_midi_route_remove(channel, kind, target));
        }
      }
      return 0;
}

static int word_exec__slashmR(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "%s", skred_midi_route_status());
      return 0;
}

static int word_exec__slashmC(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      skred_midi_route_clear();
      ctx->printf(ctx, "# MIDI routes cleared\n");
      return 0;
}

static int word_exec__mf(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  if (argc == 1 && ands_string_len(ctx->parse) > 0) {
    int slot = arg[0];
    midi_player_load(slot, ands_string(ctx->parse));
    if (ctx->printf) ctx->printf(ctx, "# Loaded MIDI file into slot %d\n", slot);
  } else {
    if (ctx->printf) ctx->printf(ctx, "# usage: [filename.mid] /mf slot\n");
  }
  return 0;
}

static int word_exec__mf_play(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  if (argc == 1) midi_player_play((int)arg[0]);
  return 0;
}

static int word_exec__mf_sync(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  if (argc == 2) midi_player_sync((int)arg[0], (int)arg[1]);
  else if (ctx->printf) ctx->printf(ctx, "# usage: /mfS slot mode(0=free,1=sync)\n");
  return 0;
}

static int word_exec__mf_dump(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  if (argc == 3) midi_player_dump((int)arg[0], (int)arg[1], (int)arg[2], ctx);
  else if (ctx->printf) ctx->printf(ctx, "# usage: /mfD slot start limit\n");
  return 0;
}

static int word_exec__mf_status(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  if (argc == 1) midi_player_status((int)arg[0], ctx);
  else {
    for (int i=0; i<4; i++) midi_player_status(i, ctx);
  }
  return 0;
}

static int word_exec__mf_seek(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  if (argc == 2) midi_player_seek((int)arg[0], arg[1]);
  else if (ctx->printf) ctx->printf(ctx, "# usage: /mfP slot tick\n");
  return 0;
}

static int word_exec__mf_stop(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  if (argc == 1) midi_player_stop((int)arg[0]);
  return 0;
}

static int word_exec__slashmb(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int type, channel = -1, data1 = -1;
        if (argc == 3 && skode_double_to_int(arg[0], &type) &&
            (isnan(arg[1]) || skode_double_to_int(arg[1], &channel)) &&
            (isnan(arg[2]) || skode_double_to_int(arg[2], &data1)) &&
            ands_string_len(ctx->parse) > 0 &&
            skred_midi_binding_set(type, channel, data1,
              ands_string(ctx->parse)) == 0 &&
            skred_control_dispatch_start() == 0)
          ctx->printf(ctx, "# MIDI Skode binding installed\n");
        else ctx->printf(ctx,
          "# usage: [skode-command] /mb type channel data1\n");
      }
      return 0;
}

static int word_exec__slashmbd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      {
        int type, channel = -1, data1 = -1;
        if (argc == 3 && skode_double_to_int(arg[0], &type) &&
            (isnan(arg[1]) || skode_double_to_int(arg[1], &channel)) &&
            (isnan(arg[2]) || skode_double_to_int(arg[2], &data1)))
          ctx->printf(ctx, "# MIDI Skode bindings removed: %d\n",
            skred_midi_binding_remove(type, channel, data1));
      }
      return 0;
}

static int word_exec__slashmb_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "%s", skred_midi_binding_status());
      return 0;
}

static int word_exec__slashmbC(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      skred_midi_binding_clear();
      ctx->printf(ctx, "# MIDI Skode bindings cleared\n");
      return 0;
}

static int word_exec__slashpg(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec__slashpg_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (!x_valid || argc != 1 || skred_poly_group_refresh(x) != 0)
        ctx->printf(ctx, "# usage: /pg! group\n");
      return 0;
}

static int word_exec__slashpp(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec__slashpp_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (!x_valid || argc != 1 || skred_poly_pool_refresh(x) != 0)
        ctx->printf(ctx, "# usage: /pp! pool\n");
      return 0;
}

static int word_exec__slashpm(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec__qpg(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "%s", skred_poly_group_status(x_valid ? x : -1));
      return 0;
}

static int word_exec__qpp(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "%s", skred_poly_pool_status(x_valid ? x : -1));
      return 0;
}

static int word_exec__slashvg(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_pn(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_pr(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_pb(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_pt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  (void)self; (void)atom; (void)voice; (void)arg;
      if (argc > 3) {
          envelope_configure_e(&sv.freq_envelope[voice], arg[0], arg[1], arg[2], arg[3]);
          sv.use_freq_envelope[voice] = !(arg[0] == 0 && arg[1] == 0 && arg[2] == 1 && arg[3] == 0);
      }
      return 0;
}

static int word_exec_pd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  (void)self; (void)atom; (void)voice; (void)arg;
      if (argc > 0) sv.freq_env_depth[voice] = arg[0];
      return 0;
}

static int word_exec_pte(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  (void)self; (void)atom; (void)voice; (void)arg; (void)argc;
  double *data = s ? ands_data(s) : NULL;
  int len = s ? ands_data_len(s) : 0;
  if (len > 0) {
      envelope_configure_multistage_e(&sv.freq_envelope[voice], data, len);
      sv.use_freq_envelope[voice] = 1;
  }
  return 0;
}

static int word_exec_MO(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_k(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) { sv.amp_envelope_mode[voice] = x; } return 0;
}

static int word_exec_wt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && skode_wave_valid(x)) {
        skode_copy_string(sw.name[x], WAVE_NAME_MAX, ands_string(ctx->parse));
      }
      return 0;
}

static int word_exec_d_gtMO(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
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

static int word_exec_WS(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && arg[0] >= 0) {
        int w = WAVE_DISPLAY_DEFAULT_WIDTH;
        int h = WAVE_DISPLAY_DEFAULT_HEIGHT / 2;
        wavetable_spectrogram_show(ctx, x, w, h, sw.loop_start[x], sw.loop_end[x], NULL);
      }
      return 0;
}

static int word_exec__qs(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      ctx->printf(ctx, "# [%s]\n", ands_string(ctx->parse));
      return 0;
}

static int word_exec__slashsg(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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
            return 0;
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

static int word_exec__slashss(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      scope_ipc_stop();
      ctx->printf(ctx, "# scope stopped\n");
      return 0;
}

static int word_exec__slashs_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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


// Word declarations
static skode_word_t word__slashals = { WID("/als"), .execute = word_exec__slashals, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slasha_q = { WID("/a?"), .execute = word_exec__slasha_q, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashai = { WID("/ai"), .execute = word_exec__slashai, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashao = { WID("/ao"), .execute = word_exec__slashai, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmL = { WID("/mL"), .execute = word_exec__slashmL, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmls = { WID("/mls"), .execute = word_exec__slashmL, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashm_q = { WID("/m?"), .execute = word_exec__slashm_q, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmi = { WID("/mi"), .execute = word_exec__slashmi, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmo = { WID("/mo"), .execute = word_exec__slashmi, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmiV = { WID("/miV"), .execute = word_exec__slashmiV, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmoV = { WID("/moV"), .execute = word_exec__slashmiV, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmic = { WID("/mic"), .execute = word_exec__slashmic, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmoc = { WID("/moc"), .execute = word_exec__slashmoc, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmv = { WID("/mv"), .execute = word_exec__slashmv, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmp = { WID("/mp"), .execute = word_exec__slashmv, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmvd = { WID("/mvd"), .execute = word_exec__slashmvd, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmpd = { WID("/mpd"), .execute = word_exec__slashmvd, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmR = { WID("/mR"), .execute = word_exec__slashmR, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmC = { WID("/mC"), .execute = word_exec__slashmC, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmf = { .min_args = 1, .max_args = 1, WID("/mf"), .execute = word_exec__mf, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmf_play = { .min_args = 1, .max_args = 1, WID("/mf>"), .execute = word_exec__mf_play, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmf_sync = { .min_args = 2, .max_args = 2, WID("/mfS"), .execute = word_exec__mf_sync, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmf_dump = { .min_args = 3, .max_args = 3, WID("/mfD"), .execute = word_exec__mf_dump, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmf_status = { .min_args = 0, .max_args = 1, WID("/mf?"), .execute = word_exec__mf_status, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmf_seek = { .min_args = 2, .max_args = 2, WID("/mfP"), .execute = word_exec__mf_seek, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmf_stop = { .min_args = 1, .max_args = 1, WID("/mf<"), .execute = word_exec__mf_stop, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmb = { WID("/mb"), .execute = word_exec__slashmb, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmbd = { WID("/mbd"), .execute = word_exec__slashmbd, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmb_q = { WID("/mb?"), .execute = word_exec__slashmb_q, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashmbC = { WID("/mbC"), .execute = word_exec__slashmbC, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashpg = { WID("/pg"), .execute = word_exec__slashpg, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashpg_bang = { WID("/pg!"), .execute = word_exec__slashpg_bang, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashpp = { WID("/pp"), .execute = word_exec__slashpp, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashpp_bang = { WID("/pp!"), .execute = word_exec__slashpp_bang, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashpm = { WID("/pm"), .execute = word_exec__slashpm, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__qpg = { WID("?pg"), .execute = word_exec__qpg, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__qpp = { WID("?pp"), .execute = word_exec__qpp, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word__slashvg = { WID("/vg"), .execute = word_exec__slashvg, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word_pn = { WID("pn"), .execute = word_exec_pn, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word_pr = { WID("pr"), .execute = word_exec_pr, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word_pb = { WID("pb"), .execute = word_exec_pb, .safety = WORD_IMMEDIATE_ONLY };
static skode_word_t word_pt = { WID("pt"), .execute = word_exec_pt, .safety = WORD_IMMEDIATE_ONLY , .category = "pitch" };
static skode_word_t word_pd = { WID("pd"), .execute = word_exec_pd, .safety = WORD_IMMEDIATE_ONLY , .category = "pitch" };
static skode_word_t word_pte = { WID("pte"), .execute = word_exec_pte, .safety = WORD_IMMEDIATE_ONLY , .category = "pitch" };
static skode_word_t word_MO = { WID("MO"), .execute = word_exec_MO, .safety = WORD_IMMEDIATE_ONLY , .category = "midi" };
static skode_word_t word_k = { WID("k"), .execute = word_exec_k, .safety = WORD_IMMEDIATE_ONLY , .category = "misc" };
static skode_word_t word_wt = { WID("wt"), .execute = word_exec_wt, .safety = WORD_IMMEDIATE_ONLY , .category = "misc" };
static skode_word_t word_d_gtMO = { WID("d>MO"), .execute = word_exec_d_gtMO, .safety = WORD_IMMEDIATE_ONLY , .category = "midi" };
static skode_word_t word_WS = { WID("WS"), .execute = word_exec_WS, .safety = WORD_IMMEDIATE_ONLY , .category = "wave-specto" };
static skode_word_t word__qs = { WID("?s"), .execute = word_exec__qs, .safety = WORD_IMMEDIATE_ONLY , .category = "misc" };
static skode_word_t word__slashsg = { WID("/sg"), .execute = word_exec__slashsg, .safety = WORD_IMMEDIATE_ONLY , .category = "scope" };
static skode_word_t word__slashss = { WID("/ss"), .execute = word_exec__slashss, .safety = WORD_IMMEDIATE_ONLY , .category = "scope" };
static skode_word_t word__slashs_q = { WID("/s?"), .execute = word_exec__slashs_q, .safety = WORD_IMMEDIATE_ONLY , .category = "scope" };

void skode_register_words_misc(skode_vocab_t *vocab) {
  skode_dict_register(vocab, &word__slashals);
  skode_dict_register(vocab, &word__slasha_q);
  skode_dict_register(vocab, &word__slashai);
  skode_dict_register(vocab, &word__slashao);
  skode_dict_register(vocab, &word__slashmL);
  skode_dict_register(vocab, &word__slashmls);
  skode_dict_register(vocab, &word__slashm_q);
  skode_dict_register(vocab, &word__slashmi);
  skode_dict_register(vocab, &word__slashmo);
  skode_dict_register(vocab, &word__slashmiV);
  skode_dict_register(vocab, &word__slashmoV);
  skode_dict_register(vocab, &word__slashmic);
  skode_dict_register(vocab, &word__slashmoc);
  skode_dict_register(vocab, &word__slashmv);
  skode_dict_register(vocab, &word__slashmp);
  skode_dict_register(vocab, &word__slashmvd);
  skode_dict_register(vocab, &word__slashmpd);
  skode_dict_register(vocab, &word__slashmR);
  skode_dict_register(vocab, &word__slashmC);
  skode_dict_register(vocab, &word__slashmf);
  skode_dict_register(vocab, &word__slashmf_play);
  skode_dict_register(vocab, &word__slashmf_sync);
  skode_dict_register(vocab, &word__slashmf_dump);
  skode_dict_register(vocab, &word__slashmf_status);
  skode_dict_register(vocab, &word__slashmf_seek);
  skode_dict_register(vocab, &word__slashmf_stop);
  skode_dict_register(vocab, &word__slashmb);
  skode_dict_register(vocab, &word__slashmbd);
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
  skode_dict_register(vocab, &word_pt);
  skode_dict_register(vocab, &word_pd);
  skode_dict_register(vocab, &word_pte);
  skode_dict_register(vocab, &word_MO);
  skode_dict_register(vocab, &word_k);
  skode_dict_register(vocab, &word_wt);
  skode_dict_register(vocab, &word_d_gtMO);
  skode_dict_register(vocab, &word_WS);
  skode_dict_register(vocab, &word__qs);
  skode_dict_register(vocab, &word__slashsg);
  skode_dict_register(vocab, &word__slashss);
  skode_dict_register(vocab, &word__slashs_q);
}
