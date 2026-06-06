#include <stdio.h>
#include <string.h>

#include "api.h"

static int failures;

static void expect(int condition, const char *message) {
  if (condition) return;
  fprintf(stderr, "FAIL audio commands: %s\n", message);
  failures++;
}

int main(void) {
  expect(skred_audio_command("v0 f440") == 0,
         "non-audio command was intercepted");

  expect(skred_audio_command("/a?") == 1,
         "/a? was not handled");
  expect(strstr(skred_audio_message(), "audio=stopped") != NULL,
         "/a? did not report stopped state");

  expect(skred_audio_command("/ain off") == 1,
         "/ain off was not accepted");
  expect(strstr(skred_audio_message(), "requested=[off]") != NULL,
         "/ain off did not update requested state");

  expect(skred_audio_command("/aout default") == 1,
         "/aout default was not accepted");
  expect(strstr(skred_audio_message(), "requested=[default]") != NULL,
         "/aout default did not update requested state");

  expect(skred_audio_command("/ain") < 0,
         "missing /ain argument was accepted");
  expect(strstr(skred_audio_message(), "usage: /ain") != NULL,
         "missing /ain argument did not produce usage");

  expect(skred_audio_command("/aout off") < 0,
         "/aout off was accepted");
  expect(skred_audio_command("/aout -3") < 0,
         "invalid negative output selection was accepted");

  if (failures) {
    fprintf(stderr, "%d audio command test failure(s)\n", failures);
    return 1;
  }
  printf("Audio command tests passed\n");
  return 0;
}
