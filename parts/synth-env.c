#include "synth.h"
#include "synth-internal.h"
#include "util.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <stdio.h>
#include "control-events.h"

static void envelope_snapshot_config(envelope_t *e) {
    e->attack_time   = e->a * MAIN_SAMPLE_RATE;
    e->decay_time    = e->d * MAIN_SAMPLE_RATE;
    e->sustain_level = fmaxf(0, fminf(1.0f, e->s));
    e->release_time  = e->r * MAIN_SAMPLE_RATE;
}

#include <stdio.h>
#include "control-events.h"

// Configure the next trigger without disturbing an envelope in progress.
void envelope_configure_e(envelope_t *e, float a, float d, float s, float r) {
    e->a = a;
    e->d = d;
    e->s = s;
    e->r = r;
}

// Initialize both configured parameters and runtime state.
void envelope_init_e(envelope_t *e, float a, float d, float s, float r) {
    envelope_configure_e(e, a, d, s, r);
    envelope_snapshot_config(e);
    e->sample_start         = 0;
    e->sample_release       = UINT64_MAX;
    e->is_active            = 0;
    e->velocity             = 1.0f;
    e->amplitude_at_release = 0.0f;
    e->amplitude_at_trigger = 0.0f;
    e->current_amplitude    = 0.0f;
}

void envelope_init(int v, float a, float d, float s, float r) {
    envelope_init_e(&sv.amp_envelope[v], a, d, s, r);
}

void envelope_trigger_e(envelope_t *e, float f) {
    envelope_snapshot_config(e);
    if (e->is_active)
        e->amplitude_at_trigger = e->current_amplitude;
    else
        e->amplitude_at_trigger = 0.0f;
    e->sample_start   = SAMPLE_COUNT_GET();
    e->sample_release = UINT64_MAX;
    e->velocity       = f;
    e->is_active      = 1;
}

void amp_envelope_trigger(int v, float f) {
    envelope_trigger_e(&sv.amp_envelope[v], f);
}

void envelope_release_e_at(envelope_t *e, uint64_t current_sample) {
    if (e->is_active && e->sample_release == UINT64_MAX) {
        e->sample_release       = current_sample;
        e->amplitude_at_release = e->current_amplitude;
    }
}

void envelope_schedule_release_e_at(envelope_t *e, uint64_t release_sample) {
    if (e->is_active && e->sample_release == UINT64_MAX) {
        e->sample_release = release_sample;
        e->amplitude_at_release = -1.0f;
    }
}

void envelope_release_e(envelope_t *e) {
    envelope_release_e_at(e, SAMPLE_COUNT_GET());
}

void amp_envelope_release(int v) {
    envelope_release_e(&sv.amp_envelope[v]);
}

float envelope_step_e(envelope_t *e, uint64_t current_sample) {
    if (!e->is_active) return 0.0f;

    float held_out = 0.0f;
    float out = 0.0f;
    float samples_since_start = current_sample >= e->sample_start
        ? (float)(current_sample - e->sample_start) : 0.0f;

    if (samples_since_start < e->attack_time) {
        float attack_progress = samples_since_start / e->attack_time;
        float start_val = e->amplitude_at_trigger;
        float curved_progress = attack_progress * attack_progress;
        held_out = start_val + (curved_progress * (1.0f - start_val));
    }
    else if (samples_since_start < (e->attack_time + e->decay_time)) {
        float samples_in_decay = samples_since_start - e->attack_time;
        float decay_progress = samples_in_decay / e->decay_time;
        held_out = 1.0f - decay_progress * (1.0f - e->sustain_level);
    }
    else {
        held_out = e->sustain_level;
    }

    if (e->sample_release == UINT64_MAX) {
        out = held_out;
        if (e->sustain_level <= 0.0f &&
            samples_since_start >= e->attack_time + e->decay_time) {
            e->is_active = 0;
            out = 0.0f;
        }
    } else if (current_sample < e->sample_release) {
        out = e->amplitude_at_release < 0.0f ? held_out : e->amplitude_at_release;
    } else {
        if (e->amplitude_at_release < 0.0f) {
            e->amplitude_at_release = held_out;
        }
        if (e->release_time <= 0.0f) {
            e->is_active = 0;
            out = 0.0f;
        } else {
            float samples_since_release = (float)(current_sample - e->sample_release);
            if (samples_since_release < e->release_time) {
                float release_progress = samples_since_release / e->release_time;
                out = e->amplitude_at_release * (1.0f - release_progress);
            } else {
                e->is_active = 0;
                out = 0.0f;
            }
        }
    }

    e->current_amplitude = out;
    return out * e->velocity;
}

float amp_envelope_step(int v, uint64_t current_sample) {
    return envelope_step_e(&sv.amp_envelope[v], current_sample);
}

static uint64_t one_shot_natural_frames(int voice) {
    if (!sv.one_shot[voice] || (sv.loop_active[voice] && !sv.loop_bounded[voice]))
        return 0;
    double inc = fabs((double)sv.phase_inc[voice]);
    if (!isfinite(inc) || inc <= 0.0) return 0;

    bool pingpong = sv.direction[voice] == 2;
    bool reverse_step = pingpong ? (sv.pingpong_reverse[voice] != 0) :
        (sv.direction[voice] != 0);
    bool loop_active = sv.loop_active[voice] != 0;
    bool loop_bounded = loop_active && sv.loop_bounded[voice] != 0;
    int remaining = loop_bounded ? sv.loop_remaining[voice] : 0;
    double phase = sv.phase[voice];

    bool valid_loop = loop_active && sv.loop_valid[voice];
    double loop_start  = valid_loop ? (double)sv.loop_start_f[voice] : (double)sv.wave_range_start_f[voice];
    double loop_end    = valid_loop ? (double)sv.loop_end_f[voice]   : (double)sv.wave_range_end_f[voice];
    double loop_length = valid_loop ? (double)sv.loop_length[voice]  : (double)(sv.wave_range_end_f[voice] - sv.wave_range_start_f[voice]);

    if (!isfinite(phase) || !isfinite(loop_start) || !isfinite(loop_end) ||
        !isfinite(loop_length) || loop_length <= 0.0)
        return 0;

    uint64_t total = 0;
    int safety = remaining + 2;
    while (safety-- > 0) {
        double distance;
        uint64_t frames;
        if (pingpong) {
            if (reverse_step) {
                distance = phase - loop_start;
                if (!isfinite(distance) || distance < 0.0) distance = 0.0;
                double frames_f = floor(distance / inc) + 1.0;
                if (!isfinite(frames_f) || frames_f >= (double)(UINT64_MAX - total))
                    return 0;
                frames = (uint64_t)frames_f;
                phase -= (double)frames * inc;
                total += frames;
                if (!loop_bounded) return total;
                if (remaining <= 0) return total;
                remaining--;
                phase = loop_start + (loop_start - phase);
                reverse_step = 0;
            } else {
                distance = loop_end - phase;
                if (!isfinite(distance) || distance <= 0.0) distance = inc;
                double frames_f = ceil(distance / inc);
                if (frames_f < 1.0) frames_f = 1.0;
                if (!isfinite(frames_f) || frames_f >= (double)(UINT64_MAX - total))
                    return 0;
                frames = (uint64_t)frames_f;
                phase += (double)frames * inc;
                total += frames;
                if (!loop_bounded) return total;
                if (remaining <= 0) return total;
                remaining--;
                phase = loop_end - (phase - loop_end);
                reverse_step = 1;
            }
        } else if (reverse_step) {
            distance = phase - loop_start;
            if (!isfinite(distance) || distance < 0.0) distance = 0.0;
            double frames_f = floor(distance / inc) + 1.0;
            if (!isfinite(frames_f) || frames_f >= (double)(UINT64_MAX - total))
                return 0;
            frames = (uint64_t)frames_f;
            phase -= (double)frames * inc;
            if (!loop_bounded) return total + frames;

            int crossings = osc_loop_crossings(loop_start - phase, loop_length);
            int wraps = crossings < remaining ? crossings : remaining;
            phase += (double)wraps * loop_length;
            remaining -= wraps;
            total += frames;
            if (wraps < crossings) return total;
        } else {
            distance = loop_end - phase;
            if (!isfinite(distance) || distance <= 0.0) distance = inc;
            double frames_f = ceil(distance / inc);
            if (frames_f < 1.0) frames_f = 1.0;
            if (!isfinite(frames_f) || frames_f >= (double)(UINT64_MAX - total))
                return 0;
            frames = (uint64_t)frames_f;
            phase += (double)frames * inc;
            if (!loop_bounded) return total + frames;

            int crossings = osc_loop_crossings(phase - loop_end, loop_length);
            int wraps = crossings < remaining ? crossings : remaining;
            phase -= (double)wraps * loop_length;
            remaining -= wraps;
            total += frames;
            if (wraps < crossings) return total;
        }
    }

    return 0;
}

void amp_envelope_schedule_one_shot_release(int v) {
    if (sv.amp_envelope_mode[v] != 1) return;
    uint64_t frames = one_shot_natural_frames(v);
    if (frames == 0) return;

    envelope_t *e = &sv.amp_envelope[v];
    uint64_t release_frames = e->release_time > 0.0f ? (uint64_t)ceilf(e->release_time) : 0;
    uint64_t start = e->sample_start;
    uint64_t release_sample = start + (frames > release_frames ? frames - release_frames : 0);
    envelope_schedule_release_e_at(e, release_sample);
}

#include "util.h"

static sben_t bench[BENLEN] = {};
static char _stats[65536] = "";

char *synth_stats(void) {
  char *ptr = _stats;
  *ptr = '\0';
  int n = 0;
  for (int i = 0; i < BENLEN; i++) {
    if (bench[i].state != BEN_B) continue;
    //double maxcb = (double)bench[i].frames / (double)MAIN_SAMPLE_RATE * (double)S_TO_MS;
    double dms = ts_diff_ns(&bench[i].a, &bench[i].b) / (double)NS_TO_MS;
    n = sprintf(ptr, "# @%d %gms\n", bench[i].order, dms);
    ptr += n;
    bench[i].state = BEN_0;
  }
  return _stats;
}

#ifdef __APPLE__
#define VOICE_CLOCK CLOCK_MONOTONIC
#else

#ifdef _WIN32
#define VOICE_CLOCK CLOCK_MONOTONIC
#else
#define VOICE_CLOCK CLOCK_MONOTONIC_COARSE
#endif

#endif

