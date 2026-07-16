#include "api.h"
#include "midi.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void expect_no_event(void) {
  skred_control_event_t event;
  assert(skred_control_event_poll(&event, 1) == 0);
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

  puts("midi control-event bridge: ok");
  return 0;
}
