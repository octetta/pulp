#ifndef SYNTH_STATE_H
#define SYNTH_STATE_H

/*
 * synth-state.h — heap-allocated parallel arrays for voice and wave state.
 *
 * All pointers are NULL until synth_alloc_voices() / synth_alloc_waves()
 * are called from synth_init().  They are set back to NULL by
 * synth_free_voices() / synth_free_waves() called from synth_free().
 *
 * Layout is struct-of-arrays (SoA).  This is intentional:
 *   - Contiguous same-type data lets the auto-vectorizer load full SIMD
 *     registers across voices (e.g. 4 or 8 floats at once).
 *   - Array-of-structs (AoS) would interleave fields and defeat this.
 *
 * Access:  sv.phase[v],  sw.data[w]
 * This is a direct drop-in for the old globals voice_phase[v], wave_table_data[w].
 *
 * The audio callback must NEVER call malloc/free.  These pointers are
 * populated once at init time and then treated as plain C arrays for the
 * lifetime of the engine.
 *
 * Voice counts are always rounded to a multiple of VOICE_ALIGN (see
 * synth-config.h).  Extra voices beyond the user-requested count are
 * zeroed by calloc and produce silence without any special-casing here.
 */

#include "synth-config.h"
#include "synth-types.h"   /* mmf_t, envelope_t, wave_name_t */
#include "portable_atomic.h"

#include <time.h>          /* struct timespec (voice_mark_a/b) */

/* ------------------------------------------------------------------ */
/* Voice state                                                         */
/* ------------------------------------------------------------------ */

typedef struct {

    /* --- oscillator / playback core (hot path, vectorize across voices) --- */
    double * restrict phase;
    float  * restrict phase_inc;
    float ** restrict table;        /* per-voice pointer to wave data */
    int    * restrict table_size;
    float  * restrict table_rate;
    float  * restrict table_size_rate;
    int    * restrict one_shot;
    int    * restrict finished;
    int    * restrict direction;          /* 0 forward, 1 backward, 2 ping-pong */
    int    * restrict pingpong_reverse;   /* runtime leg for direction 2 */
    int    * restrict loop_enabled;
    int    * restrict loop_count;          /* configured wraps; 0 = unlimited */
    int    * restrict loop_bounded;        /* active note uses loop_remaining */
    int    * restrict loop_remaining;      /* wraps left for the active note */
    int    * restrict loop_active;         /* runtime loop state */
    int    * restrict loop_stop_requested; /* leave loop at next boundary */
    int    * restrict loop_release_tail;   /* l0 exits sample loop into tail */
    int    * restrict loop_ended;           /* boundary event for envelopes */
    int    * restrict loop_start;
    int    * restrict loop_end;
    float  * restrict loop_start_f;
    float  * restrict loop_end_f;
    int    * restrict loop_valid;
    int    * restrict loop_length;
    int    * restrict loop_override;

    int    * restrict wave_range_start;
    int    * restrict wave_range_end;
    float  * restrict wave_range_start_f;
    float  * restrict wave_range_end_f;
    int    * restrict wave_range_override;

    /* --- amplitude / pan (hot path) --- */
    float  * restrict amp;
    float  * restrict user_amp;
    float  * restrict pan;
    float  * restrict pan_left;
    float  * restrict pan_right;
    float  * restrict delay_send;

    /* --- per-voice output sample --- */
    float  * restrict sample;

    /* --- pitch / tuning --- */
    float  * restrict freq;
    float  * restrict note;
    float  * restrict midi_note;
    float  * restrict last_midi_note;
    float  * restrict midi_transpose;
    float  * restrict midi_cents;
    float  * restrict offset_hz;
    float  * restrict freq_scale;
    float  * restrict link_midi_0;
    float  * restrict link_midi_1;
    float  * restrict link_midi_2;
    float  * restrict link_midi_3;
    float  * restrict link_velo_0;
    float  * restrict link_velo_1;
    float  * restrict link_velo_2;
    float  * restrict link_velo_3;
    float  * restrict link_trig;
    uint64_t  * restrict link_trig_samp;

    /* --- config flags --- */
    int    * restrict wave_table_index;
    int    * restrict disconnect;
    int    * restrict control_events;
    /* 0 = master only; 1..RECORD_TRACK_MAX = additional stereo stem. */
    int    * restrict record;
    atomic_int_t * restrict record_pending;
    int    * restrict interpolate;
    int    * restrict phase_reset;


    int    * restrict quantize;

    int    * restrict cz_mode;
    float  * restrict cz_distortion;
    int    * restrict cz_mod_osc;
    float  * restrict cz_mod_depth;

    float  * restrict filter_freq;
    float  * restrict filter_res;
    int    * restrict filter_mode;
    mmf_t  * restrict filter;
    envelope_t * restrict filter_envelope;
    int        * restrict use_filter_envelope;
    float      * restrict filter_env_depth;
    int        * restrict filter_update_counter;

    envelope_t * restrict amp_envelope;
    int        * restrict amp_envelope_mode;
    int        * restrict use_amp_envelope;

    int    * restrict glissando_enable;
    float  * restrict glissando_speed;
    float  * restrict glissando_target;
    float  * restrict glissando_time;

    int    * restrict smoother_enable;
    float  * restrict smoother_gain;
    float  * restrict smoother_smoothing;

    float  * restrict sample_hold;
    int    * restrict sample_hold_count;
    int    * restrict sample_hold_max;

    int    * restrict freq_mod_osc;
    float  * restrict freq_mod_depth;
    float  * restrict freq_mod_adder;
    int    * restrict freq_mod_mode;
    int    * restrict pan_mod_osc;
    float  * restrict pan_mod_depth;
    float  * restrict pan_mod_adder;
    int    * restrict amp_mod_osc;
    float  * restrict amp_mod_depth;
    float  * restrict amp_mod_adder;

    int    * restrict ring_osc;
    float  * restrict ring_amount;

    text_t * restrict text;
} synth_voices_t;

/* ------------------------------------------------------------------ */
/* Wave table state                                                    */
/* ------------------------------------------------------------------ */

typedef struct {

    float      ** restrict data;
    int         * restrict size;
    float       * restrict rate;
    int         * restrict one_shot;
    int         * restrict loop_enabled;
    int         * restrict loop_start;
    int         * restrict loop_end;
    float       * restrict midi_note;
    float       * restrict offset_hz;
    float       * restrict direction;
    int         * restrict is_heap;
    int         * restrict refcount;
    int         * restrict readonly;
    wave_name_t * restrict name;

} synth_waves_t;

/* ------------------------------------------------------------------ */
/* Globals — defined in synth-alloc.c                                 */
/* ------------------------------------------------------------------ */

extern synth_voices_t sv;
extern synth_waves_t  sw;

#endif /* SYNTH_STATE_H */
