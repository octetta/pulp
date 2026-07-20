#include "polyphony.h"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "skode.h"
#include "synth.h"
#include "synth-config.h"
#include "synth-state.h"

typedef struct {
  int valid;
  int source;
  int width;
  int root_offset;
  uint32_t generation;
} poly_group_t;

enum {
  POLY_INSTANCE_FREE = 0,
  POLY_INSTANCE_HELD = 1,
  POLY_INSTANCE_RELEASING = 2,
};

typedef struct {
  int state;
  int key;
  float note;
  float cents;
  float velocity;
  float bend_cents;
  uint64_t started;
  uint64_t released;
  uint32_t generation;
} poly_instance_t;

typedef struct {
  int used;
  int key;
  float note;
  float cents;
  float velocity;
  float bend_cents;
  uint64_t order;
} poly_held_t;

typedef struct {
  int valid;
  int group;
  int base;
  int count;
  int policy;
  int mode;
  int priority;
  int articulation;
  int round_robin;
  int mono_active_key;
  float bend_cents;
  uint64_t order;
  poly_instance_t instance[VOICE_MAX_HARD_LIMIT];
  poly_held_t held[SKRED_POLY_HELD_MAX];
} poly_pool_t;

static poly_group_t poly_group[SKRED_POLY_GROUP_MAX];
static poly_pool_t poly_pool[SKRED_POLY_POOL_MAX];
/* Sized for a status line for every hard-limit instance and a complete
   machine graph containing every possible per-voice relationship. */
static char poly_status[131072];
static char graph_status[262144];

static void release_instance(poly_pool_t *pool, int index, uint64_t now);

static int poly_voice_valid(int voice) {
  return voice >= 0 && voice < synth_config.voice_max;
}

static int poly_int_valid(int value, int limit) {
  return value >= 0 && value < limit;
}

static int poly_key_in_range(int key) {
  return key >= -16777216 && key <= 16777216;
}

static int poly_note_key_valid(int key) {
  return poly_key_in_range(key) && key != -1;
}

static void append_text(char *out, size_t size, size_t *used,
    const char *format, ...) {
  if (!out || !used || *used >= size) return;
  va_list ap;
  va_start(ap, format);
  int written = vsnprintf(out + *used, size - *used, format, ap);
  va_end(ap);
  if (written < 0) return;
  if ((size_t)written >= size - *used) *used = size;
  else *used += (size_t)written;
}

void skred_poly_reset(void) {
  memset(poly_group, 0, sizeof(poly_group));
  memset(poly_pool, 0, sizeof(poly_pool));
  for (int p = 0; p < SKRED_POLY_POOL_MAX; p++)
    poly_pool[p].mono_active_key = INT_MIN;
}

static int remap_voice(int voice, const poly_group_t *group, int dest_base) {
  if (!group || voice < group->source ||
      voice >= group->source + group->width) return voice;
  return dest_base + (voice - group->source);
}

static float remap_link(float link, const poly_group_t *group, int dest_base) {
  int voice = (int)link;
  if (link < 0 || (float)voice != link) return link;
  return (float)remap_voice(voice, group, dest_base);
}

static void remap_dependencies(int voice, const poly_group_t *group,
    int dest_base) {
  sv.link_midi_0[voice] = remap_link(sv.link_midi_0[voice], group, dest_base);
  sv.link_midi_1[voice] = remap_link(sv.link_midi_1[voice], group, dest_base);
  sv.link_midi_2[voice] = remap_link(sv.link_midi_2[voice], group, dest_base);
  sv.link_midi_3[voice] = remap_link(sv.link_midi_3[voice], group, dest_base);
  sv.link_velo_0[voice] = remap_link(sv.link_velo_0[voice], group, dest_base);
  sv.link_velo_1[voice] = remap_link(sv.link_velo_1[voice], group, dest_base);
  sv.link_velo_2[voice] = remap_link(sv.link_velo_2[voice], group, dest_base);
  sv.link_velo_3[voice] = remap_link(sv.link_velo_3[voice], group, dest_base);
  if (sv.amp_mod_osc[voice] >= 0)
    sv.amp_mod_osc[voice] = remap_voice(sv.amp_mod_osc[voice], group, dest_base);
  if (sv.freq_mod_osc[voice] >= 0)
    sv.freq_mod_osc[voice] = remap_voice(sv.freq_mod_osc[voice], group, dest_base);
  if (sv.pan_mod_osc[voice] >= 0)
    sv.pan_mod_osc[voice] = remap_voice(sv.pan_mod_osc[voice], group, dest_base);
  if (sv.cz_mod_osc[voice] >= 0)
    sv.cz_mod_osc[voice] = remap_voice(sv.cz_mod_osc[voice], group, dest_base);
  if (sv.ring_osc[voice] >= 0)
    sv.ring_osc[voice] = remap_voice(sv.ring_osc[voice], group, dest_base);
}

static int clone_voice(const poly_group_t *group, int source, int dest,
    int dest_base) {
  if (!group || !poly_voice_valid(source) || !poly_voice_valid(dest)) return -1;
  float delay_send = sv.delay_send[dest];
  int control_events = sv.control_events[dest];
  int track = synth_record_track_get(dest);
  if (source != dest) {
    /* voice_copy() intentionally leaves a destination override in place when
       the source uses its wave defaults. Clear those two sticky settings so a
       pool instance is an exact copy of the prototype. */
    voice_wave_range_reset(dest);
    voice_loop_points_reset(dest);
    if (voice_copy(source, dest) != 0) return -1;
  }
  sv.one_shot[dest] = sv.one_shot[source];
  sv.interpolate[dest] = sv.interpolate[source];
  sv.disconnect[dest] = sv.disconnect[source];
  sv.phase_reset[dest] = sv.phase_reset[source];
  envelope_set(dest, sv.amp_envelope[source].a, sv.amp_envelope[source].d,
    sv.amp_envelope[source].s, sv.amp_envelope[source].r);
  sv.amp_envelope_mode[dest] = sv.amp_envelope_mode[source];
  memcpy(sv.text[dest], sv.text[source], sizeof(sv.text[dest]));
  sv.use_filter_envelope[dest] = sv.use_filter_envelope[source];
  sv.filter_env_depth[dest] = sv.filter_env_depth[source];
  envelope_init_e(&sv.filter_envelope[dest], sv.filter_envelope[source].a,
    sv.filter_envelope[source].d, sv.filter_envelope[source].s,
    sv.filter_envelope[source].r);
  sv.freq_mod_mode[dest] = sv.freq_mod_mode[source];
  sv.smoother_enable[dest] = sv.smoother_enable[source];
  sv.smoother_smoothing[dest] = sv.smoother_smoothing[source];
  sv.smoother_gain[dest] = sv.amp[dest];
  /* Glide enable/time are configuration; phase, target and speed belong to
     the currently sounding note and must start clean in a new instance. */
  sv.glissando_enable[dest] = sv.glissando_enable[source];
  sv.glissando_time[dest] = sv.glissando_time[source];
  sv.phase_inc[dest] = osc_get_phase_inc(dest, sv.freq[dest]);
  sv.glissando_target[dest] = sv.phase_inc[dest];
  sv.glissando_speed[dest] = 1.0f;
  sv.delay_send[dest] = delay_send;
  voice_control_events_set(dest, control_events);
  synth_record_track_set(dest, track);
  sv.pingpong_reverse[dest] = 0;
  sv.loop_bounded[dest] = 0;
  sv.loop_remaining[dest] = 0;
  sv.loop_active[dest] = 0;
  sv.loop_stop_requested[dest] = 0;
  sv.loop_release_tail[dest] = 0;
  sv.loop_ended[dest] = 0;
  sv.finished[dest] = 0;
  sv.sample[dest] = 0;
  osc_reclassify(dest);
  sv.sample_hold[dest] = 0;
  sv.sample_hold_count[dest] = 0;
  remap_dependencies(dest, group, dest_base);
  return 0;
}

static int clone_instance(const poly_group_t *group, int dest_base) {
  if (!group || !group->valid || !poly_voice_valid(dest_base) ||
      dest_base + group->width > synth_config.voice_max) return -1;
  for (int i = 0; i < group->width; i++) {
    if (clone_voice(group, group->source + i, dest_base + i, dest_base) != 0)
      return -1;
  }
  return 0;
}

int skred_poly_group_set(int group, int source, int width, int root_offset) {
  if (!poly_int_valid(group, SKRED_POLY_GROUP_MAX) ||
      !poly_voice_valid(source) || width < 1 ||
      width > SKRED_POLY_GROUP_VOICE_MAX ||
      source + width > synth_config.voice_max ||
      root_offset < 0 || root_offset >= width) return -1;
  if (poly_group[group].valid &&
      (poly_group[group].source != source || poly_group[group].width != width ||
       poly_group[group].root_offset != root_offset)) {
    for (int p = 0; p < SKRED_POLY_POOL_MAX; p++)
      if (poly_pool[p].valid && poly_pool[p].group == group) return -1;
  }
  poly_group[group].valid = 1;
  poly_group[group].source = source;
  poly_group[group].width = width;
  poly_group[group].root_offset = root_offset;
  poly_group[group].generation++;
  return 0;
}

static int pool_layout_valid(const poly_group_t *group, int base, int count) {
  if (!group || !group->valid || count < 1 || count > VOICE_MAX_HARD_LIMIT)
    return 0;
  if (!poly_voice_valid(base) || base + count * group->width > synth_config.voice_max)
    return 0;
  int source_end = group->source + group->width;
  int dest_end = base + count * group->width;
  int overlaps = base < source_end && group->source < dest_end;
  return !overlaps || base == group->source;
}

static int policy_valid(int policy) {
  return policy >= SKRED_POLY_STEAL_RELEASE_OLDEST &&
    policy <= SKRED_POLY_STEAL_NONE;
}

int skred_poly_pool_set(int pool, int group, int base, int count, int policy) {
  if (!poly_int_valid(pool, SKRED_POLY_POOL_MAX) ||
      !poly_int_valid(group, SKRED_POLY_GROUP_MAX) ||
      !policy_valid(policy) || !pool_layout_valid(&poly_group[group], base, count))
    return -1;
  int end = base + count * poly_group[group].width;
  for (int p = 0; p < SKRED_POLY_POOL_MAX; p++) {
    if (p == pool || !poly_pool[p].valid) continue;
    poly_group_t *other_group = &poly_group[poly_pool[p].group];
    int other_end = poly_pool[p].base + poly_pool[p].count * other_group->width;
    if (base < other_end && poly_pool[p].base < end) return -1;
  }
  if (poly_pool[pool].valid) {
    uint64_t now = SAMPLE_COUNT_GET();
    for (int i = 0; i < poly_pool[pool].count; i++)
      if (poly_pool[pool].instance[i].state != POLY_INSTANCE_FREE)
        release_instance(&poly_pool[pool], i, now);
  }
  poly_pool_t fresh;
  memset(&fresh, 0, sizeof(fresh));
  fresh.valid = 1;
  fresh.group = group;
  fresh.base = base;
  fresh.count = count;
  fresh.policy = policy;
  fresh.mode = SKRED_POLY_MODE_POLY;
  fresh.priority = SKRED_POLY_PRIORITY_LAST;
  fresh.articulation = SKRED_POLY_ARTICULATION_RETRIGGER;
  fresh.mono_active_key = INT_MIN;
  for (int i = 0; i < count; i++) {
    int dest = base + i * poly_group[group].width;
    if (clone_instance(&poly_group[group], dest) != 0) return -1;
  }
  poly_pool[pool] = fresh;
  return 0;
}

static int group_instance_inactive(const poly_group_t *group, int base) {
  if (!group || !group->valid) return 1;
  for (int i = 0; i < group->width; i++) {
    int voice = base + i;
    if (poly_voice_valid(voice) && sv.amp_envelope[voice].is_active) return 0;
  }
  return 1;
}

static void pool_reap(poly_pool_t *pool) {
  if (!pool || !pool->valid) return;
  poly_group_t *group = &poly_group[pool->group];
  for (int i = 0; i < pool->count; i++) {
    if (pool->instance[i].state == POLY_INSTANCE_RELEASING &&
        group_instance_inactive(group, pool->base + i * group->width)) {
      memset(&pool->instance[i], 0, sizeof(pool->instance[i]));
    }
  }
}

int skred_poly_pool_refresh(int pool) {
  if (!poly_int_valid(pool, SKRED_POLY_POOL_MAX) || !poly_pool[pool].valid)
    return -1;
  poly_pool_t *p = &poly_pool[pool];
  poly_group_t *group = &poly_group[p->group];
  pool_reap(p);
  for (int i = 0; i < p->count; i++) {
    if (p->instance[i].state == POLY_INSTANCE_FREE &&
        clone_instance(group, p->base + i * group->width) != 0) return -1;
  }
  return 0;
}

int skred_poly_group_refresh(int group) {
  if (!poly_int_valid(group, SKRED_POLY_GROUP_MAX) || !poly_group[group].valid)
    return -1;
  poly_group[group].generation++;
  for (int p = 0; p < SKRED_POLY_POOL_MAX; p++) {
    if (poly_pool[p].valid && poly_pool[p].group == group &&
        skred_poly_pool_refresh(p) != 0) return -1;
  }
  return 0;
}

static void release_instance(poly_pool_t *pool, int index, uint64_t now) {
  if (!pool || index < 0 || index >= pool->count) return;
  poly_group_t *group = &poly_group[pool->group];
  int root = pool->base + index * group->width + group->root_offset;
  skode_envelope_velocity(root, 0, now);
  if (sv.link_velo_0[root] >= 0) skode_envelope_velocity(sv.link_velo_0[root], 0, now);
  if (sv.link_velo_1[root] >= 0) skode_envelope_velocity(sv.link_velo_1[root], 0, now);
  if (sv.link_velo_2[root] >= 0) skode_envelope_velocity(sv.link_velo_2[root], 0, now);
  if (sv.link_velo_3[root] >= 0) skode_envelope_velocity(sv.link_velo_3[root], 0, now);
  pool->instance[index].state = POLY_INSTANCE_RELEASING;
  pool->instance[index].released = now;
}

static void pitch_instance(poly_pool_t *pool, int index, float note,
    float cents) {
  poly_group_t *group = &poly_group[pool->group];
  int root = pool->base + index * group->width + group->root_offset;
  skode_midi_note(root, note, cents + pool->bend_cents);
}

static void gate_instance(poly_pool_t *pool, int index, float velocity,
    uint64_t now) {
  poly_group_t *group = &poly_group[pool->group];
  int root = pool->base + index * group->width + group->root_offset;
  skode_envelope_velocity(root, velocity, now);
  if (sv.link_velo_0[root] >= 0) skode_envelope_velocity(sv.link_velo_0[root], velocity, now);
  if (sv.link_velo_1[root] >= 0) skode_envelope_velocity(sv.link_velo_1[root], velocity, now);
  if (sv.link_velo_2[root] >= 0) skode_envelope_velocity(sv.link_velo_2[root], velocity, now);
  if (sv.link_velo_3[root] >= 0) skode_envelope_velocity(sv.link_velo_3[root], velocity, now);
}

static int find_instance_key(poly_pool_t *pool, int key) {
  if (!pool) return -1;
  for (int i = 0; i < pool->count; i++)
    if (pool->instance[i].state != POLY_INSTANCE_FREE &&
        pool->instance[i].key == key) return i;
  return -1;
}

static float instance_level(poly_pool_t *pool, int index) {
  poly_group_t *group = &poly_group[pool->group];
  float level = 0;
  for (int i = 0; i < group->width; i++) {
    float value = fabsf(sv.amp_envelope[pool->base + index * group->width + i].current_amplitude);
    if (value > level) level = value;
  }
  return level;
}

static int choose_instance(poly_pool_t *pool) {
  pool_reap(pool);
  for (int i = 0; i < pool->count; i++)
    if (pool->instance[i].state == POLY_INSTANCE_FREE) return i;
  if (pool->policy == SKRED_POLY_STEAL_NONE) return -1;
  if (pool->policy == SKRED_POLY_STEAL_ROUND_ROBIN) {
    int selected = pool->round_robin++ % pool->count;
    return selected;
  }
  int selected = -1;
  uint64_t oldest = UINT64_MAX;
  float quietest = FLT_MAX;
  if (pool->policy == SKRED_POLY_STEAL_RELEASE_OLDEST) {
    for (int i = 0; i < pool->count; i++) {
      if (pool->instance[i].state == POLY_INSTANCE_RELEASING &&
          pool->instance[i].released < oldest) {
        oldest = pool->instance[i].released;
        selected = i;
      }
    }
    if (selected >= 0) return selected;
  }
  for (int i = 0; i < pool->count; i++) {
    if (pool->policy == SKRED_POLY_STEAL_QUIETEST) {
      float level = instance_level(pool, i);
      if (level < quietest) {
        quietest = level;
        selected = i;
      }
    } else if (pool->instance[i].started < oldest) {
      oldest = pool->instance[i].started;
      selected = i;
    }
  }
  return selected;
}

static int held_find(poly_pool_t *pool, int key) {
  for (int i = 0; i < SKRED_POLY_HELD_MAX; i++)
    if (pool->held[i].used && pool->held[i].key == key) return i;
  return -1;
}

static int held_select(poly_pool_t *pool) {
  int selected = -1;
  for (int i = 0; i < SKRED_POLY_HELD_MAX; i++) {
    if (!pool->held[i].used) continue;
    if (selected < 0) {
      selected = i;
      continue;
    }
    poly_held_t *a = &pool->held[i];
    poly_held_t *b = &pool->held[selected];
    if ((pool->priority == SKRED_POLY_PRIORITY_LAST && a->order > b->order) ||
        (pool->priority == SKRED_POLY_PRIORITY_FIRST && a->order < b->order) ||
        (pool->priority == SKRED_POLY_PRIORITY_HIGH && a->note > b->note) ||
        (pool->priority == SKRED_POLY_PRIORITY_LOW && a->note < b->note))
      selected = i;
  }
  return selected;
}

static int mono_note(poly_pool_t *pool, int key, float note, float velocity,
    float cents, uint64_t now) {
  int held = held_find(pool, key);
  int new_key = held < 0;
  if (held < 0) {
    for (int i = 0; i < SKRED_POLY_HELD_MAX; i++) {
      if (!pool->held[i].used) { held = i; break; }
    }
  }
  if (held < 0) return -1;
  int was_empty = held_select(pool) < 0;
  pool->held[held].used = 1;
  pool->held[held].key = key;
  pool->held[held].note = note;
  pool->held[held].cents = cents;
  pool->held[held].velocity = velocity;
  if (new_key) pool->held[held].bend_cents = 0;
  pool->held[held].order = ++pool->order;
  int active = held_select(pool);
  if (active < 0) return -1;
  poly_held_t *current = &pool->held[active];
  int changed = pool->mono_active_key != current->key;
  pool->mono_active_key = current->key;
  pitch_instance(pool, 0, current->note,
    current->cents + current->bend_cents);
  if (was_empty || (changed && pool->articulation == SKRED_POLY_ARTICULATION_RETRIGGER))
    gate_instance(pool, 0, current->velocity, now);
  poly_instance_t *instance = &pool->instance[0];
  instance->state = POLY_INSTANCE_HELD;
  instance->key = current->key;
  instance->note = current->note;
  instance->cents = current->cents;
  instance->velocity = current->velocity;
  instance->started = now;
  instance->generation++;
  return 0;
}

int skred_poly_note(int pool_index, int key, float note, float velocity,
    float cents) {
  if (!poly_int_valid(pool_index, SKRED_POLY_POOL_MAX) ||
      !poly_pool[pool_index].valid || !poly_note_key_valid(key) ||
      !isfinite(note) || note < 0 || note > 127 || !isfinite(velocity) ||
      velocity <= 0 || !isfinite(cents)) return -1;
  poly_pool_t *pool = &poly_pool[pool_index];
  uint64_t now = SAMPLE_COUNT_GET();
  if (pool->mode == SKRED_POLY_MODE_MONO)
    return mono_note(pool, key, note, velocity, cents, now);
  if (pool->mode != SKRED_POLY_MODE_POLY) return -1;
  int index = find_instance_key(pool, key);
  if (index < 0) index = choose_instance(pool);
  if (index < 0) return 1;
  if (pool->instance[index].state != POLY_INSTANCE_FREE)
    release_instance(pool, index, now);
  pitch_instance(pool, index, note, cents);
  gate_instance(pool, index, velocity, now);
  poly_instance_t *instance = &pool->instance[index];
  instance->state = POLY_INSTANCE_HELD;
  instance->key = key;
  instance->note = note;
  instance->cents = cents;
  instance->velocity = velocity;
  instance->bend_cents = 0;
  instance->started = now;
  instance->generation++;
  return 0;
}

static int mono_release(poly_pool_t *pool, int key, uint64_t now) {
  int held = held_find(pool, key);
  if (held < 0) return 0;
  int was_active = pool->mono_active_key == key;
  memset(&pool->held[held], 0, sizeof(pool->held[held]));
  if (!was_active) return 0;
  int active = held_select(pool);
  if (active < 0) {
    release_instance(pool, 0, now);
    pool->mono_active_key = INT_MIN;
    return 0;
  }
  poly_held_t *current = &pool->held[active];
  pool->mono_active_key = current->key;
  pitch_instance(pool, 0, current->note,
    current->cents + current->bend_cents);
  if (pool->articulation == SKRED_POLY_ARTICULATION_RETRIGGER)
    gate_instance(pool, 0, current->velocity, now);
  pool->instance[0].key = current->key;
  pool->instance[0].note = current->note;
  pool->instance[0].cents = current->cents;
  pool->instance[0].velocity = current->velocity;
  return 0;
}

int skred_poly_release(int pool_index, int key, float release_velocity) {
  (void)release_velocity;
  if (!poly_int_valid(pool_index, SKRED_POLY_POOL_MAX) ||
      !poly_pool[pool_index].valid || !poly_note_key_valid(key)) return -1;
  poly_pool_t *pool = &poly_pool[pool_index];
  uint64_t now = SAMPLE_COUNT_GET();
  if (pool->mode == SKRED_POLY_MODE_MONO) return mono_release(pool, key, now);
  int index = find_instance_key(pool, key);
  if (index < 0) return 0;
  release_instance(pool, index, now);
  return 0;
}

int skred_poly_bend(int pool_index, int key, float semitones, float cents) {
  if (!poly_int_valid(pool_index, SKRED_POLY_POOL_MAX) ||
      !poly_pool[pool_index].valid || !poly_key_in_range(key) ||
      !isfinite(semitones) || !isfinite(cents)) return -1;
  poly_pool_t *pool = &poly_pool[pool_index];
  float bend = semitones * 100.0f + cents;
  if (key == -1) {
    pool->bend_cents = bend;
    for (int i = 0; i < pool->count; i++) {
      if (pool->instance[i].state == POLY_INSTANCE_HELD)
        pitch_instance(pool, i, pool->instance[i].note,
          pool->instance[i].cents + pool->instance[i].bend_cents);
    }
    return 0;
  }
  if (pool->mode == SKRED_POLY_MODE_MONO) {
    int held = held_find(pool, key);
    if (held < 0) return -1;
    pool->held[held].bend_cents = bend;
    if (pool->mono_active_key == key)
      skode_midi_note(pool->base + poly_group[pool->group].root_offset,
        pool->held[held].note,
        pool->held[held].cents + bend + pool->bend_cents);
    return 0;
  }
  int index = find_instance_key(pool, key);
  if (index < 0) return -1;
  poly_group_t *group = &poly_group[pool->group];
  int root = pool->base + index * group->width + group->root_offset;
  pool->instance[index].bend_cents = bend;
  skode_midi_note(root, pool->instance[index].note,
    pool->instance[index].cents + bend + pool->bend_cents);
  return 0;
}

int skred_poly_pool_mode(int pool_index, int mode, int priority,
    int articulation) {
  if (!poly_int_valid(pool_index, SKRED_POLY_POOL_MAX) ||
      !poly_pool[pool_index].valid || mode < SKRED_POLY_MODE_POLY ||
      mode > SKRED_POLY_MODE_MONO || priority < SKRED_POLY_PRIORITY_LAST ||
      priority > SKRED_POLY_PRIORITY_FIRST ||
      articulation < SKRED_POLY_ARTICULATION_RETRIGGER ||
      articulation > SKRED_POLY_ARTICULATION_LEGATO) return -1;
  poly_pool_t *pool = &poly_pool[pool_index];
  uint64_t now = SAMPLE_COUNT_GET();
  for (int i = 0; i < pool->count; i++)
    if (pool->instance[i].state != POLY_INSTANCE_FREE)
      release_instance(pool, i, now);
  memset(pool->held, 0, sizeof(pool->held));
  pool->mono_active_key = INT_MIN;
  pool->mode = mode;
  pool->priority = priority;
  pool->articulation = articulation;
  return 0;
}

static const char *policy_name(int policy) {
  static const char *name[] = {
    "release-oldest", "oldest", "round-robin", "quietest", "no-steal"
  };
  return policy_valid(policy) ? name[policy] : "invalid";
}

const char *skred_poly_group_status(int requested) {
  size_t used = 0;
  poly_status[0] = '\0';
  for (int g = 0; g < SKRED_POLY_GROUP_MAX; g++) {
    if (requested >= 0 && g != requested) continue;
    if (!poly_group[g].valid) continue;
    append_text(poly_status, sizeof(poly_status), &used,
      "/pg %d,%d,%d,%d # generation %u root v%d\n", g,
      poly_group[g].source, poly_group[g].width, poly_group[g].root_offset,
      poly_group[g].generation,
      poly_group[g].source + poly_group[g].root_offset);
  }
  if (used == 0) append_text(poly_status, sizeof(poly_status), &used,
    "# no voice groups\n");
  return poly_status;
}

const char *skred_poly_pool_status(int requested) {
  size_t used = 0;
  poly_status[0] = '\0';
  for (int p = 0; p < SKRED_POLY_POOL_MAX; p++) {
    if (requested >= 0 && p != requested) continue;
    if (!poly_pool[p].valid) continue;
    pool_reap(&poly_pool[p]);
    append_text(poly_status, sizeof(poly_status), &used,
      "/pp %d,%d,%d,%d,%d # policy %s\n"
      "/pm %d,%d,%d,%d # mode %s\n",
      p, poly_pool[p].group, poly_pool[p].base, poly_pool[p].count,
      poly_pool[p].policy, policy_name(poly_pool[p].policy), p,
      poly_pool[p].mode, poly_pool[p].priority,
      poly_pool[p].articulation,
      poly_pool[p].mode == SKRED_POLY_MODE_MONO ? "mono" : "poly");
    for (int i = 0; i < poly_pool[p].count; i++) {
      poly_instance_t *instance = &poly_pool[p].instance[i];
      const char *state = instance->state == POLY_INSTANCE_HELD ? "held" :
        instance->state == POLY_INSTANCE_RELEASING ? "releasing" : "free";
      append_text(poly_status, sizeof(poly_status), &used,
        "#   instance %d root v%d state %s", i,
        poly_pool[p].base + i * poly_group[poly_pool[p].group].width +
          poly_group[poly_pool[p].group].root_offset, state);
      if (instance->state != POLY_INSTANCE_FREE)
        append_text(poly_status, sizeof(poly_status), &used,
          " key %d note %g velocity %g", instance->key, instance->note,
          instance->velocity);
      append_text(poly_status, sizeof(poly_status), &used, "\n");
    }
  }
  if (used == 0) append_text(poly_status, sizeof(poly_status), &used,
    "# no voice pools\n");
  return poly_status;
}

typedef struct {
  int to;
  int type;
  const char *name;
} graph_edge_t;

static int graph_add(graph_edge_t *edge, int count, int max, int to,
    int type, const char *name) {
  if (to < 0 || !poly_voice_valid(to) || count >= max) return count;
  edge[count].to = to;
  edge[count].type = type;
  edge[count].name = name;
  return count + 1;
}

static int graph_edges(int voice, graph_edge_t *edge, int max) {
  int count = 0;
  float pitch[] = {sv.link_midi_0[voice], sv.link_midi_1[voice],
    sv.link_midi_2[voice], sv.link_midi_3[voice]};
  float gate[] = {sv.link_velo_0[voice], sv.link_velo_1[voice],
    sv.link_velo_2[voice], sv.link_velo_3[voice]};
  for (int i = 0; i < 4; i++)
    count = graph_add(edge, count, max, (int)pitch[i],
      SKRED_VOICE_EDGE_PITCH, "pitch");
  for (int i = 0; i < 4; i++)
    count = graph_add(edge, count, max, (int)gate[i],
      SKRED_VOICE_EDGE_GATE, "gate");
  count = graph_add(edge, count, max, sv.amp_mod_osc[voice],
    SKRED_VOICE_EDGE_AMP_MOD, "amp-mod");
  count = graph_add(edge, count, max, sv.freq_mod_osc[voice],
    SKRED_VOICE_EDGE_FREQ_MOD, "freq-mod");
  count = graph_add(edge, count, max, sv.pan_mod_osc[voice],
    SKRED_VOICE_EDGE_PAN_MOD, "pan-mod");
  count = graph_add(edge, count, max, sv.cz_mod_osc[voice],
    SKRED_VOICE_EDGE_PHASE_MOD, "phase-mod");
  count = graph_add(edge, count, max, sv.ring_osc[voice],
    SKRED_VOICE_EDGE_RING_MOD, "ring-mod");
  return count;
}

static void graph_ascii_walk(int voice, int level, int depth,
    unsigned char *seen, size_t *used) {
  if (level >= depth) return;
  graph_edge_t edge[16];
  int count = graph_edges(voice, edge, 16);
  for (int i = 0; i < count; i++) {
    for (int n = 0; n < level; n++)
      append_text(graph_status, sizeof(graph_status), used, "|  ");
    append_text(graph_status, sizeof(graph_status), used, "%s- %s -> v%d%s\n",
      i == count - 1 ? "`" : "|", edge[i].name, edge[i].to,
      seen[edge[i].to] ? " (seen)" : "");
    if (!seen[edge[i].to]) {
      seen[edge[i].to] = 1;
      graph_ascii_walk(edge[i].to, level + 1, depth, seen, used);
    }
  }
}

static void graph_machine_walk(int voice, int level, int depth,
    unsigned char *seen, size_t *used) {
  if (level >= depth) return;
  graph_edge_t edge[16];
  int count = graph_edges(voice, edge, 16);
  for (int i = 0; i < count; i++) {
    append_text(graph_status, sizeof(graph_status), used,
      "edge %d %d %d %s\n", voice, edge[i].to, edge[i].type,
      edge[i].name);
    if (!seen[edge[i].to]) {
      seen[edge[i].to] = 1;
      append_text(graph_status, sizeof(graph_status), used,
        "node %d\n", edge[i].to);
      graph_machine_walk(edge[i].to, level + 1, depth, seen, used);
    }
  }
}

const char *skred_voice_graph(int voice, int format, int depth) {
  graph_status[0] = '\0';
  if (!poly_voice_valid(voice) || (format != 0 && format != 1)) {
    snprintf(graph_status, sizeof(graph_status), "# invalid voice graph request\n");
    return graph_status;
  }
  if (depth <= 0 || depth > synth_config.voice_max) depth = synth_config.voice_max;
  unsigned char seen[VOICE_MAX_HARD_LIMIT] = {0};
  seen[voice] = 1;
  size_t used = 0;
  if (format == 0) {
    append_text(graph_status, sizeof(graph_status), &used, "voice v%d\n", voice);
    graph_ascii_walk(voice, 0, depth, seen, &used);
  } else {
    append_text(graph_status, sizeof(graph_status), &used,
      "skred-voice-graph 1\nroot %d\nnode %d\n", voice, voice);
    graph_machine_walk(voice, 0, depth, seen, &used);
    append_text(graph_status, sizeof(graph_status), &used, "end\n");
  }
  return graph_status;
}
