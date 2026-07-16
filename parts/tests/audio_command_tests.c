#include <stdio.h>
#include <string.h>

#include "api.h"
#include "synth.h"
#include "synth-types.h"

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
  expect(strstr(skred_audio_message(), "audio: stopped") != NULL,
         "/a? did not report stopped state");
  expect(strstr(skred_audio_message(), "channels:") != NULL,
         "/a? did not report channel metrics");
  expect(strstr(skred_audio_message(), "requested-callback-frames:") != NULL,
         "/a? did not report callback frame metrics");
  expect(strstr(skred_audio_message(), "device-buffer:") != NULL,
         "/a? did not report effective device buffer metrics");
  expect(strstr(skred_audio_message(), "capture-waves: w7..w14") != NULL,
         "/a? did not report capture wave mapping");
  expect(strstr(skred_audio_message(), "delay:") != NULL,
         "/a? did not report delay status");
  expect(strstr(skred_audio_message(), "perf:") != NULL,
         "/a? did not report performance metrics");
  expect(strstr(skred_audio_message(), "suspected-glitches:") != NULL,
         "/a? did not report suspected glitch metrics");
  expect(strstr(skred_audio_message(), "clipped-samples") != NULL,
         "/a? did not report output integrity metrics");

  skred_performance_metrics_t metrics;
  expect(skred_performance_metrics(&metrics) == 0,
         "performance metrics API rejected output pointer");
  expect(metrics.callbacks == 0,
         "performance metrics should start with zero callbacks");
  expect(skred_performance_metrics(NULL) < 0,
         "performance metrics API accepted null output pointer");
  skred_performance_reset();

  synth_sample_rate_set(48000);
  expect(strstr(skred_audio_status(), "rate: 48000") != NULL,
         "status did not report runtime sample rate");
  synth_sample_rate_set(SKRED_DEFAULT_SAMPLE_RATE);

  expect(skred_audio_command("/ain off") == 1,
         "/ain off was not accepted");
  expect(strstr(skred_audio_message(), "requested: [off]") != NULL,
         "/ain off did not update requested state");

  expect(skred_audio_command("/aout default") == 1,
         "/aout default was not accepted");
  expect(strstr(skred_audio_message(), "requested: [default]") != NULL,
         "/aout default did not update requested state");

  expect(skred_audio_command("/ain") < 0,
         "missing /ain argument was accepted");
  expect(strstr(skred_audio_message(), "usage: /ain") != NULL,
         "missing /ain argument did not produce usage");

  expect(skred_audio_command("/aout off") < 0,
         "/aout off was accepted");
  expect(skred_audio_command("/aout -3") < 0,
         "invalid negative output selection was accepted");

  skred_logger(1);
  expect(skred_command("/a?") == 0,
         "skred_command did not preserve successful command semantics");
  expect(strstr(skred_log(), "audio: stopped") != NULL,
         "skred_command did not mirror audio status into the Skode log");
  size_t status_log_length = strlen(skred_log());
  expect(status_log_length > 0 && skred_log()[status_log_length - 1] == '\n',
         "mirrored audio status did not end with a newline");

  expect(skred_command("/ain") == 0,
         "invalid audio command escaped the unified command boundary");
  expect(strstr(skred_log(), "usage: /ain") != NULL,
         "audio command error was not mirrored into the Skode log");

  expect(skred_command("log 1") == 0,
         "non-audio command did not fall through to Skode");

  /* skred_command() owns a SKODE_EMPTY() context and depends on lazy parser
     initialization. Exercise dictionary-backed atoms through that exact API
     path so browser/WASM builds cannot silently lose the core voice words. */
  synth_init(8);
  wave_table_init(0);
  voice_init();
  expect(skred_command("v0a0f440l1") == 0,
         "compact core command failed through unified command boundary");
  expect(strstr(skred_log(), "unknown atom") == NULL,
         "compact core command lost dictionary-backed atoms");
  expect(sv.user_amp[0] == 0.0f && sv.freq[0] == 440.0f,
         "compact core command did not update voice state");
  synth_free();

  if (failures) {
    fprintf(stderr, "%d audio command test failure(s)\n", failures);
    return 1;
  }
  printf("Audio command tests passed\n");
  return 0;
}
