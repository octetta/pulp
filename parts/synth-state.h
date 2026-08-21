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

/* To aggregate polyphony state into the engine */
#include "polyphony.h"

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
    int    * restrict playback_class;

    /* Diagnostics */
    uint64_t * restrict latency_timestamp_ns;

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
    float  * restrict freq_bend;
    float  * restrict freq_bend_range;
    float  * restrict freq_bend_offset;
    float  * restrict amp_bend;
    float  * restrict amp_bend_range;
    float  * restrict amp_bend_offset;

    /* --- config flags --- */
    int    * restrict wave_table_index;
    int    * restrict disconnect;
    int    * restrict control_events;
    /* 0 = master only; 1..RECORD_TRACK_MAX = additional stereo stem. */
    int    * restrict record;
    atomic_int_t * restrict record_pending;
    int    * restrict interpolate;
    int    * restrict phase_reset;

    int             * restrict mark_go;
    struct timespec * restrict mark_a;
    struct timespec * restrict mark_b;

    int    * restrict quantize;

    int    * restrict cz_mode;
    float  * restrict cz_distortion;
    int    * restrict cz_mod_osc;
    float  * restrict cz_mod_depth;
    envelope_t * restrict cz_envelope;
    int        * restrict use_cz_envelope;
    float      * restrict cz_env_depth;

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
    float  * restrict sample_hold_ratio;
    int    * restrict sample_hold_mode;
    float  * restrict sample_hold_smooth;
    int    * restrict sample_hold_jitter_target;

    int    * restrict freq_mod_osc;
    float  * restrict freq_mod_depth;
    float  * restrict freq_mod_adder;
    int    * restrict freq_mod_mode;
    float  * restrict freq_mod_feedback;
    float  * restrict freq_mod_feedback_z1;
    float  * restrict freq_mod_feedback_z2;
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
/* Engine Context                                                      */
/* ------------------------------------------------------------------ */

typedef struct skred_engine_s {
    /* Audio configuration */
    synth_config_t config;
    int sample_rate;
    atomic_uint64_t sample_count;

    /* Allocations */
    synth_voices_t sv;
    synth_waves_t sw;

    /* Volumes and globals from synth.h */
    float volume_user;
    float volume_final;
    float volume_smoother_gain;
    float volume_smoother_smoothing;
    
    /* Diagnostics / Latency */
    int ping_requested;
    float volume_threshold;
    float volume_smoother_higher_smoothing;

    /* Polyphony state */
    poly_group_t poly_group[SKRED_POLY_GROUP_MAX];
    poly_pool_t poly_pool[SKRED_POLY_POOL_MAX];

    /* Buffer sizes */
    int requested_frames_per_callback;
    int frames_per_callback;
} skred_engine_t;

extern skred_engine_t skred_global_engine;

// These macros transparently map the old globals into the new engine context.
#define sv (skred_global_engine.sv)
#define sw (skred_global_engine.sw)

#define synth_config (skred_global_engine.config)
#define synth_sample_rate (skred_global_engine.sample_rate)
#define synth_sample_count (skred_global_engine.sample_count)
#define requested_synth_frames_per_callback (skred_global_engine.requested_frames_per_callback)
#define synth_frames_per_callback (skred_global_engine.frames_per_callback)

#define volume_user (skred_global_engine.volume_user)
#define volume_final (skred_global_engine.volume_final)
#define volume_smoother_gain (skred_global_engine.volume_smoother_gain)
#define volume_smoother_smoothing (skred_global_engine.volume_smoother_smoothing)
#define volume_threshold (skred_global_engine.volume_threshold)
#define volume_smoother_higher_smoothing (skred_global_engine.volume_smoother_higher_smoothing)

/*
 * Use this everywhere you need the voice count inside the audio callback.
 * Loads from a local, applies an alignment hint so the compiler can emit
 * unconditional vector code without a scalar remainder loop.
 */
static inline int synth_voice_count(void) {
    int n = synth_config.voice_max;
    /* Alignment hint — cost: zero at runtime on already-aligned data. */
#if defined(__clang__)
    __builtin_assume(n % VOICE_ALIGN == 0);
#elif defined(__GNUC__) && __GNUC__ >= 13
    __attribute__((assume(n % VOICE_ALIGN == 0)));
#else
    /* Mask forces low bits to zero; compiler infers no remainder loop needed. */
    n = n & ~(VOICE_ALIGN - 1);
#endif
    return n;
}

#endif /* SYNTH_STATE_H */
