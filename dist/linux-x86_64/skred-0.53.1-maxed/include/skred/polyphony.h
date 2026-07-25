#ifndef SKRED_POLYPHONY_H
#define SKRED_POLYPHONY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SKRED_POLY_GROUP_MAX 16
#define SKRED_POLY_POOL_MAX 16
#define SKRED_POLY_GROUP_VOICE_MAX 16
#define SKRED_POLY_HELD_MAX 64

typedef enum {
  SKRED_POLY_STEAL_RELEASE_OLDEST = 0,
  SKRED_POLY_STEAL_OLDEST = 1,
  SKRED_POLY_STEAL_ROUND_ROBIN = 2,
  SKRED_POLY_STEAL_QUIETEST = 3,
  SKRED_POLY_STEAL_NONE = 4,
} skred_poly_steal_policy_t;

typedef enum {
  SKRED_POLY_MODE_POLY = 0,
  SKRED_POLY_MODE_MONO = 1,
  SKRED_POLY_MODE_ARP_RESERVED = 2,
} skred_poly_mode_t;

typedef enum {
  SKRED_POLY_PRIORITY_LAST = 0,
  SKRED_POLY_PRIORITY_HIGH = 1,
  SKRED_POLY_PRIORITY_LOW = 2,
  SKRED_POLY_PRIORITY_FIRST = 3,
} skred_poly_priority_t;

typedef enum {
  SKRED_POLY_ARTICULATION_RETRIGGER = 0,
  SKRED_POLY_ARTICULATION_LEGATO = 1,
} skred_poly_articulation_t;

typedef enum {
  SKRED_VOICE_EDGE_PITCH = 0,
  SKRED_VOICE_EDGE_GATE = 1,
  SKRED_VOICE_EDGE_AMP_MOD = 2,
  SKRED_VOICE_EDGE_FREQ_MOD = 3,
  SKRED_VOICE_EDGE_PAN_MOD = 4,
  SKRED_VOICE_EDGE_PHASE_MOD = 5,
  SKRED_VOICE_EDGE_RING_MOD = 6,
} skred_voice_edge_type_t;

void skred_poly_reset(void);

/* Define a live prototype made from consecutive physical voices. The root
   offset selects the member that receives pool note, release and bend input. */
int skred_poly_group_set(int group, int source, int width, int root_offset);
/* Re-copy changed prototype settings into currently free instances only. */
int skred_poly_group_refresh(int group);

/* Materialize count copies of a group at base. Policy values are the stable
   numeric skred_poly_steal_policy_t values above. */
int skred_poly_pool_set(int pool, int group, int base, int count, int policy);
int skred_poly_pool_refresh(int pool);
/* Monophonic mode uses the pool's first instance. Mode 2 is reserved and is
   not currently accepted. Changing mode releases allocations and held notes. */
int skred_poly_pool_mode(int pool, int mode, int priority, int articulation);

/* key identifies a note lifetime and must be an exactly representable integer
   in [-16777216,16777216]. -1 is reserved for pool-wide bend and is rejected
   by note/release. Return 1 means a no-steal pool was full; negative is error. */
int skred_poly_note(int pool, int key, float note, float velocity, float cents);
int skred_poly_release(int pool, int key, float release_velocity);
int skred_poly_bend(int pool, int key, float semitones, float cents);

/* Returned strings use module-owned storage and are replaced by the next
   status call. A negative selector lists all defined groups or pools. */
const char *skred_poly_group_status(int group);
const char *skred_poly_pool_status(int pool);

/* format 0 is an ASCII tree. Format 1 is the versioned line protocol described
   in POLYPHONY.md. depth <= 0 walks the complete reachable graph. The returned
   module-owned string is replaced by the next graph call. */
const char *skred_voice_graph(int voice, int format, int depth);

#ifdef __cplusplus
}
#endif

#endif
