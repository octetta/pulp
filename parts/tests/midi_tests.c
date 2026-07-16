#include "api.h"
#include "midi.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "skode.h"
#include "synth.h"
#include "synth-state.h"

static void expect_no_event(void) {
  skred_control_event_t event;
  assert(skred_control_event_poll(&event, 1) == 0);
}

static void consume_skode(skode_t *ctx, const char *command) {
  char copy[512];
  snprintf(copy, sizeof(copy), "%s", command);
  assert(skode_consume(copy, ctx) == 0);
}

int main(void) {
  skred_control_event_t event;
  skred_control_event_reset();

  skred_midi_event_mask_set(1u << SKRED_MIDI_NOTE_ON);
  skred_midi_test_inject(SKRED_MIDI_NOTE_ON, 2, 60, 101);
  assert(skred_control_event_poll(&event, 1) == 1);
  assert(event.type == SKRED_CONTROL_EVENT_MIDI);
  assert(event.id == SKRED_MIDI_NOTE_ON);
  assert(event.value_count == 3);
  assert(event.value[0] == 2.0);
  assert(event.value[1] == 60.0);
  assert(event.value[2] == 101.0);

  skred_midi_test_inject(SKRED_MIDI_CONTROL_CHANGE, 2, 1, 64);
  expect_no_event();

  skred_midi_event_mask_set(1u << SKRED_MIDI_CLOCK);
  skred_midi_test_inject(SKRED_MIDI_CLOCK, -1, 0, 0);
  assert(skred_control_event_poll(&event, 1) == 1);
  assert(event.id == SKRED_MIDI_CLOCK);
  assert(event.value[0] == -1.0);

  skred_midi_event_mask_set(0);
  skred_midi_test_inject(SKRED_MIDI_NOTE_ON, 0, 64, 127);
  expect_no_event();

  assert(skred_midi_command("v0 n60") == 0);
  assert(skred_midi_command("/m?") == 1);
#if defined(SKRED_MIDI_ENABLED)
  assert(strstr(skred_midi_message(), "midi: stopped") != NULL);
#else
  assert(strstr(skred_midi_message(), "midi: not built") != NULL);
#endif
  assert(skred_midi_command("/mi") < 0);
  assert(strstr(skred_midi_message(), "MIDI usage") != NULL);

  skred_logger(1);
  assert(skred_command("MO 144,60,100") == 0);
  assert(strstr(skred_log(), "MIDI output failed") != NULL);

  synth_init(8);
  wave_table_init(0);
  voice_init();
  skred_poly_reset();
  skred_midi_route_clear();
  skred_midi_event_mask_set((1u << SKRED_MIDI_NOTE_ON) |
    (1u << SKRED_MIDI_NOTE_OFF) | (1u << SKRED_MIDI_PITCH_BEND) |
    (1u << SKRED_MIDI_CONTROL_CHANGE) | (1u << SKRED_MIDI_START));

  assert(skred_midi_route_set(2, SKRED_MIDI_ROUTE_VOICE, 1, 2.0f) == 0);
  float untouched_note = sv.last_midi_note[1];
  skred_midi_test_inject(SKRED_MIDI_NOTE_ON, 3, 72, 127);
  assert(skred_control_dispatch_pump(16) == 0);
  assert(sv.last_midi_note[1] == untouched_note);
  skred_midi_test_inject(SKRED_MIDI_NOTE_ON, 2, 60, 127);
  assert(skred_control_dispatch_pump(16) == 0);
  assert(sv.last_midi_note[1] == 60.0f);
  float unbent = sv.freq[1];
  skred_midi_test_inject(SKRED_MIDI_PITCH_BEND, 2, 127, 127);
  assert(skred_control_dispatch_pump(16) == 0);
  assert(sv.freq[1] > unbent);
  skred_midi_test_inject(SKRED_MIDI_NOTE_OFF, 2, 60, 64);
  assert(skred_control_dispatch_pump(16) == 0);

  assert(skred_poly_group_set(0, 0, 1, 0) == 0);
  assert(skred_poly_pool_set(0, 0, 2, 2,
    SKRED_POLY_STEAL_RELEASE_OLDEST) == 0);
  assert(skred_midi_route_set(-1, SKRED_MIDI_ROUTE_POOL, 0, 2.0f) == 0);
  skred_midi_test_inject(SKRED_MIDI_NOTE_ON, 1, 64, 100);
  skred_midi_test_inject(SKRED_MIDI_NOTE_ON, 2, 67, 110);
  assert(skred_control_dispatch_pump(32) == 0);
  assert(strstr(skred_poly_pool_status(0), "key 192") != NULL);
  assert(strstr(skred_poly_pool_status(0), "key 323") != NULL);
  float channel_1_freq = sv.freq[2];
  float channel_2_freq = sv.freq[3];
  skred_midi_test_inject(SKRED_MIDI_PITCH_BEND, 1, 127, 127);
  assert(skred_control_dispatch_pump(16) == 0);
  assert(sv.freq[2] > channel_1_freq);
  assert(fabsf(sv.freq[3] - channel_2_freq) < 0.001f);
  assert(skred_midi_route_count() == 2);
  assert(strstr(skred_midi_route_status(), "/mv 2,1,2") != NULL);
  assert(strstr(skred_midi_route_status(), "/mp .,0,2") != NULL);
  assert(skred_midi_route_remove(2, SKRED_MIDI_ROUTE_VOICE, 1) == 1);
  skred_midi_route_clear();
  assert(skred_midi_route_count() == 0);
  assert(skred_midi_command("/mR") == 1);
  assert(strstr(skred_midi_message(), "none") != NULL);
  assert(skred_midi_command("/mv .,1,12") == 1);
  assert(skred_midi_route_count() == 1);
  assert(skred_control_dispatch_running());
  assert(skred_midi_command("/mvd -,1") == 1);
  assert(skred_midi_route_count() == 0);

  skred_midi_binding_clear();
  skode_t midi_skode = SKODE_EMPTY();
  skode_init(&midi_skode);
  consume_skode(&midi_skode, "/mv . 1 12");
  assert(skred_midi_route_count() == 1);
  consume_skode(&midi_skode, "/mvd - 1");
  assert(skred_midi_route_count() == 0);
  consume_skode(&midi_skode, "[v4a{d2}] /mb 11 . 74");
  assert(skred_midi_binding_count() == 1);
  assert(skred_midi_command("/mb cc,.,74 v4a{d2}") < 0);
  skred_midi_test_inject(SKRED_MIDI_CONTROL_CHANGE, 6, 73, 99);
  assert(skred_control_dispatch_pump(16) == 0);
  assert(sv.user_amp[4] != 99.0f);
  skred_midi_test_inject(SKRED_MIDI_CONTROL_CHANGE, 6, 74, 99);
  assert(skred_control_dispatch_pump(16) == 0);
  assert(sv.user_amp[4] == 99.0f);
  consume_skode(&midi_skode, "[mt]:[v5a-7] /mb 26 - .; mt");
  skred_midi_test_inject(SKRED_MIDI_START, -1, 0, 0);
  assert(skred_control_dispatch_pump(16) == 0);
  assert(sv.user_amp[5] == -7.0f);
  assert(skred_midi_binding_count() == 2);
  assert(strstr(skred_midi_binding_status(),
    "[v4a{d2}] /mb 11,.,74") != NULL);
  assert(strstr(skred_midi_binding_status(),
    "[v5a-7] /mb 26,.,.") != NULL);
  assert(skred_midi_command("/mb- 11,.,74") == 0);
  assert(skred_midi_binding_count() == 2);
  assert(skred_midi_command("/mbd 11,.,74") == 1);
  assert(skred_midi_binding_count() == 1);
  assert(skred_midi_command("/mbC") == 1);
  assert(skred_midi_binding_count() == 0);

  const char *binding_file = "/tmp/pulp-midi-bindings.sk";
  FILE *file = fopen(binding_file, "wb");
  assert(file != NULL);
  assert(fputs("[v6a{d2}] /mb 11 . 76\n", file) >= 0);
  assert(fclose(file) == 0);
  assert(skode_load_name(&midi_skode, binding_file, 0) == 0);
  assert(strstr(skred_midi_binding_status(),
    "[v6a{d2}] /mb 11,.,76") != NULL);
  assert(unlink(binding_file) == 0);
  skred_midi_binding_clear();
  skode_free(&midi_skode);
  skred_control_dispatch_stop();
  synth_free();

  puts("midi control-event bridge: ok");
  return 0;
}
