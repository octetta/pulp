#include <math.h>
#include <stdio.h>
#include <string.h>

#include "api.h"
#include "polyphony.h"
#include "seq.h"
#include "skode.h"
#include "synth.h"
#include "synth-state.h"

static int failures;

static void expect(int condition, const char *message) {
  if (condition) return;
  fprintf(stderr, "FAIL polyphony: %s\n", message);
  failures++;
}

static int near(float a, float b) {
  return fabsf(a - b) < 0.01f;
}

static float note_hz(float note) {
  return 440.0f * powf(2.0f, (note - 69.0f) / 12.0f);
}

int main(void) {
  synth_init(32);
  wave_table_init(0);
  voice_init();
  skred_poly_reset();

  sv.link_midi_0[0] = 1;
  sv.link_velo_0[0] = 1;
  sv.midi_transpose[1] = 12;
  sv.link_midi_1[0] = 5;
  sv.one_shot[0] = 1;
  sv.interpolate[0] = 0;
  sv.disconnect[0] = 1;
  sv.amp_envelope_mode[0] = 1;
  snprintf(sv.text[0], sizeof(sv.text[0]), "prototype");
  sv.wave_range_override[8] = 1;
  sv.loop_override[8] = 1;
  delay_send_set(8, 0.75f);
  voice_control_events_set(8, 1);
#ifdef SKRED_TEST_TRACKS
  synth_record_track_set(8, 3);
#endif

  expect(skred_poly_group_set(0, 0, 2, 0) == 0,
         "group definition failed");
  expect(skred_poly_pool_set(0, 0, 8, 2,
           SKRED_POLY_STEAL_RELEASE_OLDEST) == 0,
         "pool definition failed");
  expect((int)sv.link_midi_0[8] == 9,
         "internal pitch dependency was not remapped");
  expect((int)sv.link_velo_0[8] == 9,
         "internal gate dependency was not remapped");
  expect((int)sv.link_midi_0[10] == 11,
         "second instance dependency was not remapped");
  expect((int)sv.link_midi_1[8] == 5,
         "external pitch dependency was incorrectly remapped");
  expect(near(sv.delay_send[8], 0.75f),
         "group clone overwrote destination delay send");
  expect(sv.control_events[8] == 1,
         "group clone overwrote destination control-event setting");
#ifdef SKRED_TEST_TRACKS
  expect(synth_record_track_get(8) == 3,
         "group clone overwrote destination track assignment");
#endif
  expect(sv.wave_range_override[8] == 0 && sv.loop_override[8] == 0,
         "group clone retained stale destination overrides");
  expect(sv.one_shot[8] == 1 && sv.interpolate[8] == 0 &&
         sv.disconnect[8] == 1 && sv.amp_envelope_mode[8] == 1 &&
         strcmp(sv.text[8], "prototype") == 0,
         "group clone omitted voice configuration");

  expect(skred_poly_note(0, 1001, 60, 0.8f, 0) == 0,
         "first poly note failed");
  expect(skred_poly_note(0, 1002, 64, 0.7f, 0) == 0,
         "second poly note failed");
  expect(near(sv.last_midi_note[8], 60) && near(sv.last_midi_note[10], 64),
         "poly notes did not use separate instances");
  expect(near(sv.last_midi_note[9], 60) && near(sv.last_midi_note[11], 64),
         "pitch links did not reach cloned members");
  expect(skred_poly_bend(0, 1001, 2, 0) == 0,
         "per-note bend failed");
  expect(near(sv.freq[8], note_hz(62)),
         "per-note bend produced the wrong pitch");
  expect(skred_poly_bend(0, -1, 1, 0) == 0 &&
         near(sv.freq[8], note_hz(63)) &&
         near(sv.freq[10], note_hz(65)),
         "pool bend did not compose with per-note bend");
  expect(skred_poly_note(0, -1, 60, .5f, 0) < 0 &&
         skred_poly_release(0, -1, 0) < 0,
         "reserved bend key was accepted as a note key");
  expect(skred_poly_release(0, 1001, 0) == 0,
         "poly release failed");
  expect(strstr(skred_poly_pool_status(0), "releasing") != NULL,
         "pool status did not show releasing instance");
  expect(skred_poly_note(0, 1003, 67, .6f, 0) == 0 &&
         strstr(skred_poly_pool_status(0),
           "instance 0 root v8 state held key 1003") != NULL,
         "release-oldest policy did not prefer a releasing instance");
  expect(skred_poly_release(0, 1001, 0) == 0 &&
         strstr(skred_poly_pool_status(0), "state held key 1003") != NULL,
         "stale release affected the replacement allocation");

  expect(skred_poly_pool_set(1, 0, 16, 1,
           SKRED_POLY_STEAL_RELEASE_OLDEST) == 0,
         "mono pool definition failed");
  expect(skred_poly_pool_mode(1, SKRED_POLY_MODE_MONO,
           SKRED_POLY_PRIORITY_LAST, SKRED_POLY_ARTICULATION_LEGATO) == 0,
         "mono mode failed");
  expect(skred_poly_note(1, 2001, 60, 0.8f, 0) == 0 &&
         skred_poly_note(1, 2002, 67, 0.6f, 0) == 0,
         "mono note stack failed");
  expect(near(sv.last_midi_note[16], 67),
         "last-note priority did not select newest note");
  expect(skred_poly_release(1, 2002, 0) == 0,
         "mono active release failed");
  expect(near(sv.last_midi_note[16], 60),
         "mono release did not return to held note");
  expect(skred_poly_release(1, 2001, 0) == 0,
         "mono final release failed");

  expect(skred_poly_pool_set(2, 0, 20, 1, SKRED_POLY_STEAL_NONE) == 0,
         "no-steal pool definition failed");
  expect(skred_poly_note(2, 3001, 48, .5f, 0) == 0 &&
         skred_poly_note(2, 3002, 50, .5f, 0) == 1,
         "no-steal policy did not report a full pool");
  expect(skred_poly_release(2, 9999, 0) == 0,
         "unknown release was not a successful no-op");
  expect(skred_poly_pool_set(5, 0, 10, 1, SKRED_POLY_STEAL_OLDEST) < 0,
         "overlapping pool layout was accepted");
  expect(skred_poly_group_set(0, 1, 2, 0) < 0,
         "live group geometry was changed underneath a pool");
  expect(skred_poly_pool_set(5, 0, 28, 1, 99) < 0,
         "unknown steal policy was accepted");

  expect(skred_poly_pool_set(3, 0, 24, 2,
           SKRED_POLY_STEAL_RELEASE_OLDEST) == 0 &&
         skred_poly_pool_mode(3, SKRED_POLY_MODE_MONO,
           SKRED_POLY_PRIORITY_HIGH, SKRED_POLY_ARTICULATION_LEGATO) == 0,
         "multi-instance pool could not switch to mono");
  expect(skred_poly_note(3, 4001, 60, .5f, 0) == 0 &&
         skred_poly_note(3, 4002, 55, .5f, 0) == 0 &&
         near(sv.last_midi_note[24], 60) &&
         skred_poly_note(3, 4003, 67, .5f, 0) == 0 &&
         near(sv.last_midi_note[24], 67),
         "high-note mono priority failed");
  expect(skred_poly_release(3, 4003, 0) == 0 &&
         near(sv.last_midi_note[24], 60),
         "high-note priority fallback failed");

  expect(skred_poly_pool_set(4, 0, 28, 2,
           SKRED_POLY_STEAL_ROUND_ROBIN) == 0 &&
         skred_poly_note(4, 5001, 48, .5f, 0) == 0 &&
         skred_poly_note(4, 5002, 50, .5f, 0) == 0 &&
         skred_poly_note(4, 5003, 52, .5f, 0) == 0 &&
         strstr(skred_poly_pool_status(4), "instance 0 root v28 state held key 5003"),
         "round-robin stealing failed");
  expect(skred_poly_pool_set(4, 0, 28, 2,
           SKRED_POLY_STEAL_QUIETEST) == 0 &&
         skred_poly_note(4, 5101, 48, .5f, 0) == 0 &&
         skred_poly_note(4, 5102, 50, .5f, 0) == 0,
         "quietest pool setup failed");
  sv.amp_envelope[28].current_amplitude = .8f;
  sv.amp_envelope[29].current_amplitude = .8f;
  sv.amp_envelope[30].current_amplitude = .1f;
  sv.amp_envelope[31].current_amplitude = .1f;
  expect(skred_poly_note(4, 5103, 52, .5f, 0) == 0 &&
         strstr(skred_poly_pool_status(4), "instance 1 root v30 state held key 5103"),
         "quietest stealing failed");

  sv.link_midi_0[9] = 8;

  const char *ascii = skred_voice_graph(8, 0, 8);
  expect(strstr(ascii, "voice v8") && strstr(ascii, "pitch -> v9") &&
         strstr(ascii, "gate -> v9"),
         "ASCII dependency graph was incomplete");
  const char *graph = skred_voice_graph(8, 1, 8);
  expect(strstr(graph, "skred-voice-graph 1") &&
         strstr(graph, "edge 8 9 0 pitch") &&
         strstr(graph, "edge 9 8 0 pitch") &&
         strstr(graph, "edge 8 9 1 gate") && strstr(graph, "end\n"),
         "machine dependency graph was incomplete");

  event_program_t program;
  expect(skode_compile_program("pn0,3001,72,.5", &program) ==
           SKODE_COMPILE_OK && program.count == 1 &&
           program.op[0].opcode.code == SKODE_OP_POLY_NOTE,
         "pn was not schedulable");
  expect(skode_compile_program("pr0,3001", &program) ==
           SKODE_COMPILE_OK &&
           program.op[0].opcode.code == SKODE_OP_POLY_RELEASE,
         "pr was not schedulable");
  expect(skode_compile_program("pb0,-1,2", &program) ==
           SKODE_COMPILE_OK &&
           program.op[0].opcode.code == SKODE_OP_POLY_BEND,
         "pb was not schedulable");

  skode_t ctx = SKODE_EMPTY();
  skode_init(&ctx);
  ctx.log_enable = 1;
  char command[] = "/vg8,1,8";
  expect(skode_consume(command, &ctx) == 0 &&
         strstr(ctx.log, "skred-voice-graph 1") != NULL,
         "/vg did not expose graph text through Skode");
  skode_free(&ctx);

  synth_free();
  if (failures) {
    fprintf(stderr, "%d polyphony failure(s)\n", failures);
    return 1;
  }
  printf("Polyphony tests passed\n");
  return 0;
}
