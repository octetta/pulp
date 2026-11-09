#include "skode-internal.h"
#include "scope-ipc.h"

static int word_exec_ab(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
/* @doc

`ab val` set voice amplitude bend normalized value (-1.0 to 1.0)

@enddoc */
      if (argc) amp_bend_set(voice, (float)arg[0]);
      return 0;
}

static int word_exec_abp(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
/* @doc

`abp range[,offset]` set voice amplitude bend range in dB and optional offset

@enddoc */
      if (argc) {
        float range = (float)arg[0];
        float offset = argc > 1 ? (float)arg[1] : 0.0f;
        amp_bend_param_set(voice, range, offset);
      }
      return 0;
}

static int word_exec_A(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
/* @doc

`A x,y,z` set voice amplitude modulation (AM) using voice `x`, depth 'y', and offset 'z'

@enddoc */
      if (argc < 2) {
        amp_mod_set(voice, -1, 0, 0);
      } else if (x_valid) {
        float a = 0;
        if (argc > 2) a = arg[2];
        amp_mod_set(voice, x, arg[1], a);
      }
      return 0;
}

static int word_exec_b(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
/* @doc

`b [0|1|2]` set voice waveform playback direction:
0=forward, 1=backward, 2=ping-pong

@enddoc */
      if (argc == 0) { wave_dir(voice, -1); } else { wave_dir(voice, x); } return 0;
}

static int word_exec_B(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
/* @doc

`B [0|1]` set voice waveform looping 0=off, 1=on

@enddoc */
      if (argc == 0) { wave_loop(voice, -1); } else { wave_loop(voice, x); } return 0;
}

static int word_exec_BC(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
/* @doc

`BC count` set the number of one-shot loop repeats; 0 means unlimited

@enddoc */
      if (argc && x_valid) wave_loop_count(voice, x);
      return 0;
}

static int word_exec_c(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
/* @doc

`c mode, amount` set voice CZ / phase distortion with `mode` (below) and a
signed `amount` from -1.0 to 1.0. 0.0 is the undistorted phase for every mode.
  0 = off
  1 = saw -> pulse
  2 = folded sine
  3 = triangle
  4 = double sine
  5 = saw -> triangle
  6 = resonant 1
  7 = resonant 2

@enddoc */
      if (argc == 0) {
        cz_set(voice, 0, 0);
      } else if (argc == 1) {
        cz_set(voice, x, 0);
      } else {
        cz_set(voice, x, arg[1]);
      }
      return 0;
}

static int word_exec_C(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc < 2) {
        cmod_set(voice, -1, 0);
      } else if (x_valid) {
        cmod_set(voice, x, arg[1]);
      }
      return 0;
}

static int word_exec_ct(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
/* @doc

`ct attack,decay,sustain,release` sets the phase-distortion envelope. The
neutral `ct 0 0 1 0` disables it.

@enddoc */
      if (argc == 4) {
        float a = arg[0];
        float d = arg[1];
        float s = arg[2];
        float r = arg[3];
        envelope_configure_e(&sv.cz_envelope[voice], a, d, s, r);
        sv.use_cz_envelope[voice] = !(a == 0 && d == 0 && s == 1 && r == 0);
      }
      return 0;
}

static int word_exec_cte(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  (void)self; (void)atom; (void)voice; (void)arg; (void)argc;
  double *data = s ? ands_data(s) : NULL;
  int len = s ? ands_data_len(s) : 0;
  if (len > 0) {
      envelope_configure_multistage_e(&sv.cz_envelope[voice], data, len);
      sv.use_cz_envelope[voice] = 1;
  }
  return 0;
}

static int word_exec_cd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
/* @doc

`cd depth` sets the signed amount added by the phase-distortion envelope.

@enddoc */
      if (argc) sv.cz_env_depth[voice] = arg[0];
      return 0;
}

static int word_exec_fb(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
/* @doc

`fb val` set voice frequency bend normalized value (-1.0 to 1.0)

@enddoc */
      if (argc) freq_bend_set(voice, (float)arg[0]);
      return 0;
}

static int word_exec_fbp(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
/* @doc

`fbp range[,offset]` set voice frequency bend range in semitones and optional offset

@enddoc */
      if (argc) {
        float range = (float)arg[0];
        float offset = argc > 1 ? (float)arg[1] : 0.0f;
        freq_bend_param_set(voice, range, offset);
      }
      return 0;
}

static int word_exec_ft(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_fte(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  (void)self; (void)atom; (void)voice; (void)arg; (void)argc;
  double *data = s ? ands_data(s) : NULL;
  int len = s ? ands_data_len(s) : 0;
  if (len > 0) {
      envelope_configure_multistage_e(&sv.filter_envelope[voice], data, len);
      sv.use_filter_envelope[voice] = 1;
  }
  return 0;
}

static int word_exec_fd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) sv.filter_env_depth[voice] = arg[0];
      return 0;
}

static int word_exec_F(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_FF(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) freq_mod_mode_set(voice, x);
      return 0;
}

static int word_exec_FB(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) freq_feedback_set(voice, arg[0]);
      return 0;
}

static int word_exec_g(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        if (arg[0] <= 0) {
          sv.glissando_enable[voice] = 0;
          sv.glissando_time[voice] = 0.0;
        } else {
          sv.glissando_enable[voice] = 1;
          sv.glissando_time[voice] = arg[0];
          sv.glissando_mode[voice] = (argc > 1) ? (int)arg[1] : 0;
        }
      }
      return 0;
}

static int word_exec_G(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        int links[6] = {-1, -1, -1, -1, -1, -1};
        for (int i = 0; i < argc && i < 6; i++) {
          int link;
          if (skode_double_to_int(arg[i], &link) && skode_voice_valid(link))
            links[i] = link;
        }
        sv.link_midi_0[voice] = links[0];
        sv.link_midi_1[voice] = links[1];
        sv.link_midi_2[voice] = links[2];
        sv.link_midi_3[voice] = links[3];
        sv.link_midi_4[voice] = links[4];
        sv.link_midi_5[voice] = links[5];
      }
      return 0;
}

static int word_exec_h(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_H(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        int links[6] = {-1, -1, -1, -1, -1, -1};
        for (int i = 0; i < argc && i < 6; i++) {
          int link;
          if (skode_double_to_int(arg[i], &link) && skode_voice_valid(link))
            links[i] = link;
        }
        sv.link_velo_0[voice] = links[0];
        sv.link_velo_1[voice] = links[1];
        sv.link_velo_2[voice] = links[2];
        sv.link_velo_3[voice] = links[3];
        sv.link_velo_4[voice] = links[4];
        sv.link_velo_5[voice] = links[5];
      }
      return 0;
    // TODO re-allocate the data/array buffer with the arg
}

static int word_exec_L(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_J(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_K(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) { mmf_set_freq(&skred_global_engine, voice, arg[0]); }
      return 0;
}

static int word_exec____l(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && isfinite(arg[0])) envelope_velocity(voice, arg[0]);
      return 0;
}
static int word_exec_O(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  (void)self; (void)atom; (void)voice;
  for (int i = 0; i < synth_config.voice_max; i++) {
    envelope_velocity(i, 0.0);
  }
  return 0;
}

static int word_exec_l(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_M(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_N(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_ds(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) delay_send_set(&skred_global_engine, voice, arg[0]);
      return 0;
}

static int word_exec_DG(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_DL(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_DL_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_DD(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_DF(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_DP(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_DT(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc == 2) {
        int bus;
        if (skode_double_to_int(arg[0], &bus) && isfinite(arg[1]))
          delay_time_ms_set(&skred_global_engine, bus, (float)arg[1]);
      }
      return 0;
}

static int word_exec_DS(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc == 3) {
        int bus;
        if (skode_double_to_int(arg[0], &bus) && isfinite(arg[1]) && isfinite(arg[2]))
          delay_time_sync_set(&skred_global_engine, bus, (float)arg[1], (float)arg[2]);
      }
      return 0;
}

static int word_exec_GS(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      global_status_show(ctx, argc > 0 && arg[0] > 0.0);
      return 0;
}

static int word_exec_P(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 0) {
        int bits = (int)x;
        int curve = sv.quantize[voice] / 100;
        if (argc > 1 && isfinite(arg[1])) curve = (int)arg[1];
        wave_quant(voice, (curve * 100) + (bits % 100));
      }
      return 0;
}

static int word_exec_Q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) { mmf_set_res(&skred_global_engine, voice, arg[0]); }
      return 0;
}

static int word_exec_r(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) synth_record_track_set(voice, x);
      return 0;
}

static int word_exec_rt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && x > 0 && x <= RECORD_TRACK_MAX) {
        synth_track_name_set(x, ands_string(ctx->parse));
        #ifdef SCOPE
        scope_ipc_track_metadata_set(x, synth_track_name_get(x),
          synth_track_volume_db_get(x));
        #endif
      }
      return 0;
}

static int word_exec_rv(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 1 && x > 0 && x <= RECORD_TRACK_MAX) {
        synth_track_volume_set(x, arg[1]);
        #ifdef SCOPE
        scope_ipc_track_metadata_set(x, synth_track_name_get(x),
          synth_track_volume_db_get(x));
        #endif
      }
      return 0;
}

static int word_exec_s(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        if (arg[0] <= 0) {
          sv.smoother_enable[voice] = 0;
        } else {
          sv.smoother_enable[voice] = 1;
          sv.smoother_smoothing[voice] = arg[0];
          sv.smoother_mode[voice] = (argc > 1) ? (int)arg[1] : 0;
        }
      }
      return 0;
}

static int word_exec_S(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && x_valid) wave_reset(x);
      return 0;
}

static int word_exec_t(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc > 3) envelope_set(voice, arg[0], arg[1], arg[2], arg[3]);
      return 0;
}

static int word_exec_te(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  (void)self; (void)atom; (void)voice; (void)arg; (void)argc;
  double *data = s ? ands_data(s) : NULL;
  int len = s ? ands_data_len(s) : 0;
  if (len > 0) {
      envelope_configure_multistage_e(&sv.amp_envelope[voice], data, len);
      sv.use_amp_envelope[voice] = 1;
      sv.amp_envelope_mode[voice] = 0;
  }
  return 0;
}

static int word_exec_T(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  (void)self; (void)s; (void)arg;
  if (argc != 0) return -1;
  int voice = ctx->voice;
  envelope_velocity(voice, 1);
  if (sv.link_velo_0[voice] >= 0) envelope_velocity(sv.link_velo_0[voice], 1);
  if (sv.link_velo_1[voice] >= 0) envelope_velocity(sv.link_velo_1[voice], 1);
  if (sv.link_velo_2[voice] >= 0) envelope_velocity(sv.link_velo_2[voice], 1);
  if (sv.link_velo_3[voice] >= 0) envelope_velocity(sv.link_velo_3[voice], 1);
  if (sv.link_velo_4[voice] >= 0) envelope_velocity(sv.link_velo_4[voice], 1);
  if (sv.link_velo_5[voice] >= 0) envelope_velocity(sv.link_velo_5[voice], 1);
  return 0;
}

static int word_exec_HS(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  (void)self; (void)s;
  int voice = ctx->voice;
  if (argc < 1) return -1;
  int links[6] = {-1, -1, -1, -1, -1, -1};
  for (int i = 0; i < argc && i < 6; i++) {
    int link;
    if (skode_double_to_int(arg[i], &link) && skode_voice_valid(link))
      links[i] = link;
  }
  sv.link_sync_0[voice] = links[0];
  sv.link_sync_1[voice] = links[1];
  sv.link_sync_2[voice] = links[2];
  sv.link_sync_3[voice] = links[3];
  sv.link_sync_4[voice] = links[4];
  sv.link_sync_5[voice] = links[5];
  osc_hard_sync_sanitize(voice);
  return 0;
}

static int word_exec_vc(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) voice_control_events_set(voice, x != 0);
      return 0;
}

static int word_exec_V(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        volume_set(arg[0]);
        #ifdef SCOPE
        scope_ipc_track_metadata_set(0, synth_track_name_get(0),
          synth_track_volume_db_get(0));
        #endif
      }
      return 0;
}

static int word_exec_vt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      skode_copy_string(sv.text[voice], TEXT_MAX, ands_string(ctx->parse));
      return 0;
}

static int word_exec_WL(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_VS(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_VL(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_VW(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_w(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_w_gtd(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_w_gtr(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_w_gtw(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_w_bang(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_w_star(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (atomic_load_int(&sampling.state) == SAMPLE_STATE_COMPLETE) {
        sampling.offset = 0;
        sampling.trim = 0;
      } else {
        ctx->printf(ctx, "# recording buffer is not complete\n");
      }
      return 0;
}

static int word_exec_w_gt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_w_lt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_w_lt_gt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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

static int word_exec_W(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
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
          if (argc != 2) {
            wavetable_waveform_show(ctx, x, w, h, sw.loop_start[x],
              sw.loop_end[x], NULL);
          } else {
            ctx->printf(ctx, "# Wavetable Summary [%d..%d]\n", x, m);
            for (int i=x; i<=m; i++) {
              wavetable_summary_show(ctx, i);
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
        ctx->printf(ctx, "# Wavetable Summary (MAX %d)\n", synth_config.wave_table_max);
        for (int i=0; i<synth_config.wave_table_max; i++) {
          wavetable_summary_show(ctx, i);
          c++;
        }
      }
      return 0;
}

static int word_exec_XM(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc) {
        sv.ring_osc[voice] = x_valid && skode_voice_valid(x) ? x : -1;
        if (argc > 1) sv.ring_amount[voice] = arg[1];
        else sv.ring_amount[voice] = 0.0;
      }
      return 0;
}

static int word_exec_v_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;

  return 0;
}

static int word_exec__q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      voice_show(ctx, voice, ' ', ctx->verbose); return 0;
}

static int word_exec__bs_bs(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      voice_show(ctx, voice, ' ', 1); return 0;
}

static int word_exec_v_q_bs_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
  voice_show_all(ctx, voice, ctx->verbose); return 0;
}

static int word_exec__q_bs_q(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      voice_show_all(ctx, voice, ctx->verbose); return 0;
}

static int word_exec__qr(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      record_tracks_show(ctx); return 0;
}

static int word_exec__gt(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (x_valid && skode_voice_valid(x)) voice_copy(voice, x);
      return 0;
}

static int word_exec__slash(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      wave_default(voice);
      return 0;
}

static int word_exec__slashwex(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  uint32_t atom = ands_atom_num(s);
  int voice = ctx->voice;
  int x = 0;
  int x_valid = argc > 0 && skode_double_to_int(arg[0], &x);
  (void)x_valid;
  (void)voice;
  (void)self; (void)atom; (void)voice; (void)x; (void)x_valid;
      if (argc && x >= 200 && x <=999) wave_table_dynamic_expand(x);
      return 0;
}


/* 'on' and 'off' are gate-on / gate-off aliases for l1 / l0 */
static int word_exec_on(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  (void)self; (void)s; (void)arg; (void)argc;
  skode_linked_velocity(ctx->voice, 1.0, SAMPLE_COUNT_GET());
  return 0;
}
static int word_exec_off(const skode_word_t *self, skode_t *ctx, ands_t *s, double *arg, int argc) {
  (void)self; (void)s; (void)arg; (void)argc;
  skode_linked_velocity(ctx->voice, 0.0, SAMPLE_COUNT_GET());
  return 0;
}

int skode_exec_ds(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_ds(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_DG(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_DG(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_DL(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_DL(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_DL_q(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_DL_q(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_DD(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_DD(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_DF(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_DF(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_DP(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_DP(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_DT(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_DT(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_DS(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_DS(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_GS(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_GS(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_cte(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_cte(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_ft(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_ft(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_fte(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_fte(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_fd(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_fd(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_G(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_G(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_h(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_h(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_H(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_H(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_L(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_L(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_J(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_J(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_K(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_K(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec____l(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec____l(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_O(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_O(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_on(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_on(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_off(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_off(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_bc(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_q(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_Q(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_Q(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_r(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_r(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_rt(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_rt(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_rv(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_rv(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_te(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_te(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_vc(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_vc(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_V(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_V(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_vt(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_vt(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_WL(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_WL(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_VL(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_VL(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_VS(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_VS(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_VW(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_VW(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_w_gtd(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_w_gtd(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_w_gtr(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_w_gtr(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_w_gtw(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_w_gtw(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_w_bang(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_w_bang(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_w_star(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_w_star(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_w_gt(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_w_gt(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_w_lt(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_w_lt(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_w_lt_gt(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_w_lt_gt(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_W(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_W(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_slashwex(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__slashwex(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_v_q(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_v_q(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec__q(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__q(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec__bs_bs(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__bs_bs(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec_v_q_bs_q(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec_v_q_bs_q(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec__q_bs_q(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__q_bs_q(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec__qr(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__qr(NULL, ctx, s, (double*)arg, argc);
}
int skode_exec__slash(skode_t *ctx, ands_t *s, const double *arg, int argc) {
  return word_exec__slash(NULL, ctx, s, (double*)arg, argc);
}

void skode_register_words_dsp(skode_vocab_t *vocab) {
  (void)vocab;
}
