#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "recorder.h"
#include "skode.h"
#include "synth.h"
#include "synth-state.h"
#include "synth-types.h"

void seq_init(void);

static int failures;

static void fail(const char *test, const char *message) {
  fprintf(stderr, "FAIL %s: %s\n", test, message);
  failures++;
}

static void test_track_routing(void) {
  const char *test = "track routing";
  enum { FRAMES = 128 };
  float output[FRAMES * AUDIO_CHANNELS] = {0};
  float recorded[FRAMES * RECORD_CHANNELS] = {0};
  synth_record_bus_t bus = {recorded, RECORD_CHANNELS};

  synth_init(4);
  wave_table_init(0);
  voice_init();

  wave_set(0, WAVE_TABLE_SINE);
  freq_set(0, 440.0f);
  amp_set(0, 0.0f);
  pan_set(0, -1.0f);
  synth_record_track_set(0, 1);
  envelope_velocity(0, 1.0f);

  wave_set(1, WAVE_TABLE_SINE);
  freq_set(1, 660.0f);
  amp_set(1, 0.0f);
  pan_set(1, 1.0f);
  synth_record_track_set(1, 2);
  envelope_velocity(1, 1.0f);

  synth(output, NULL, FRAMES, AUDIO_CHANNELS, &bus);

  int track1_nonzero = 0;
  int track2_nonzero = 0;
  for (int frame = 0; frame < FRAMES; frame++) {
    int output_index = frame * AUDIO_CHANNELS;
    int record_index = frame * RECORD_CHANNELS;
    if (recorded[record_index] != output[output_index] ||
        recorded[record_index + 1] != output[output_index + 1]) {
      fail(test, "master track does not match device output");
      break;
    }
    if (fabsf(recorded[record_index + 2]) > 1e-7f ||
        fabsf(recorded[record_index + 3]) > 1e-7f) {
      track1_nonzero = 1;
    }
    if (fabsf(recorded[record_index + 4]) > 1e-7f ||
        fabsf(recorded[record_index + 5]) > 1e-7f) {
      track2_nonzero = 1;
    }
    for (int channel = 6; channel < RECORD_CHANNELS; channel++) {
      if (recorded[record_index + channel] != 0.0f) {
        fail(test, "unassigned track contains audio");
        frame = FRAMES;
        break;
      }
    }
  }

  if (!track1_nonzero) fail(test, "track 1 contains no audio");
  if (!track2_nonzero) fail(test, "track 2 contains no audio");

  memset(output, 0, sizeof(output));
  memset(recorded, 0, sizeof(recorded));
  synth_record_track_set(0, 3);
  synth(output, NULL, FRAMES, AUDIO_CHANNELS, &bus);

  int old_track_nonzero = 0;
  int new_track_nonzero = 0;
  for (int frame = 0; frame < FRAMES; frame++) {
    int record_index = frame * RECORD_CHANNELS;
    if (fabsf(recorded[record_index + 2]) > 1e-7f ||
        fabsf(recorded[record_index + 3]) > 1e-7f) {
      old_track_nonzero = 1;
    }
    if (fabsf(recorded[record_index + 6]) > 1e-7f ||
        fabsf(recorded[record_index + 7]) > 1e-7f) {
      new_track_nonzero = 1;
    }
  }
  if (old_track_nonzero) fail(test, "old track remained active after callback boundary");
  if (!new_track_nonzero) fail(test, "new track was not active on next callback");

  synth_free();
}

static void test_sub_block_record_offsets(void) {
  const char *test = "sub-block record offsets";
  enum { FRAMES = 128, SPLIT = 37 };
  float output[FRAMES * AUDIO_CHANNELS] = {0};
  float recorded[FRAMES * RECORD_CHANNELS] = {0};

  synth_init(4);
  wave_table_init(0);
  voice_init();
  wave_set(0, WAVE_TABLE_SINE);
  freq_set(0, 440.0f);
  amp_set(0, 0.0f);
  envelope_velocity(0, 1.0f);

  synth_record_bus_t first = {recorded, RECORD_CHANNELS};
  synth(output, NULL, SPLIT, AUDIO_CHANNELS, &first);

  synth_record_bus_t second = {
    recorded + ((size_t)SPLIT * RECORD_CHANNELS),
    RECORD_CHANNELS
  };
  synth(output + (SPLIT * AUDIO_CHANNELS), NULL, FRAMES - SPLIT,
        AUDIO_CHANNELS, &second);

  for (int frame = 0; frame < FRAMES; frame++) {
    int output_index = frame * AUDIO_CHANNELS;
    int record_index = frame * RECORD_CHANNELS;
    if (recorded[record_index] != output[output_index] ||
        recorded[record_index + 1] != output[output_index + 1]) {
      fail(test, "segmented master track does not match device output");
      break;
    }
  }

  synth_free();
}

static uint16_t read_u16_le(const unsigned char *p) {
  return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t read_u32_le(const unsigned char *p) {
  return (uint32_t)p[0] |
         ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

static void test_ring_writer(void) {
  const char *test = "ring writer";
  char filename[128];
  snprintf(filename, sizeof(filename), "/tmp/pulp-recording-%ld.wav",
           (long)getpid());

  if (recorder_init(64, MAIN_SAMPLE_RATE) != 0) {
    fail(test, "recorder_init failed");
    return;
  }
  if (recorder_start(filename, 0.0) != 0) {
    fail(test, "recorder_start failed");
    recorder_uninit();
    return;
  }

  const int blocks = 4;
  const int frames_per_block = 32;
  for (int block = 0; block < blocks; block++) {
    synth_record_bus_t *bus = recorder_begin_block(frames_per_block);
    if (!bus) {
      fail(test, "recorder_begin_block returned NULL");
      break;
    }
    for (int frame = 0; frame < frames_per_block; frame++) {
      for (int channel = 0; channel < RECORD_CHANNELS; channel++) {
        bus->frames[frame * RECORD_CHANNELS + channel] =
          (float)(block + channel) / 32.0f;
      }
    }
    recorder_end_block(frames_per_block);
  }

  if (recorder_stop() != 0) fail(test, "recorder_stop failed");
  if (recorder_frames_written() != (uint64_t)(blocks * frames_per_block)) {
    fail(test, "unexpected written frame count");
  }
  if (recorder_dropped_frames() != 0) {
    fail(test, "frames were dropped");
  }
  recorder_uninit();

  FILE *file = fopen(filename, "rb");
  if (!file) {
    fail(test, "recorded WAV was not created");
    return;
  }
  unsigned char header[44];
  size_t size = fread(header, 1, sizeof(header), file);
  fclose(file);
  unlink(filename);

  if (size != sizeof(header) ||
      memcmp(header, "RIFF", 4) != 0 ||
      memcmp(header + 8, "WAVE", 4) != 0) {
    fail(test, "invalid WAV header");
    return;
  }
  if (read_u16_le(header + 22) != RECORD_CHANNELS) {
    fail(test, "WAV channel count is not 10");
  }
  if (read_u32_le(header + 24) != MAIN_SAMPLE_RATE) {
    fail(test, "WAV sample rate mismatch");
  }
}

static void test_duration_limit_and_restart(void) {
  const char *test = "duration limit and restart";
  char filename[128];
  snprintf(filename, sizeof(filename), "/tmp/pulp-recording-limit-%ld.wav",
           (long)getpid());

  if (recorder_init(64, MAIN_SAMPLE_RATE) != 0 ||
      recorder_start(filename, 50.0 / MAIN_SAMPLE_RATE) != 0) {
    fail(test, "recorder setup failed");
    recorder_uninit();
    return;
  }

  for (int block = 0; block < 2; block++) {
    synth_record_bus_t *bus = recorder_begin_block(32);
    if (!bus) {
      fail(test, "recorder stopped before duration limit");
      break;
    }
    memset(bus->frames, 0, 32 * RECORD_CHANNELS * sizeof(float));
    recorder_end_block(32);
  }

  if (recorder_stop() != 0) fail(test, "limited recorder stop failed");
  if (recorder_frames_written() != 50) {
    fail(test, "duration limit did not stop at the exact frame");
  }

  if (recorder_start(filename, 0.0) != 0) {
    fail(test, "recorder restart failed");
  } else {
    synth_record_bus_t *bus = recorder_begin_block(8);
    if (!bus) {
      fail(test, "restart did not accept audio");
    } else {
      memset(bus->frames, 0, 8 * RECORD_CHANNELS * sizeof(float));
      recorder_end_block(8);
    }
    if (recorder_stop() != 0 || recorder_frames_written() != 8) {
      fail(test, "restart frame count mismatch");
    }
  }
  recorder_uninit();
  unlink(filename);
}

static void test_skode_recording_commands(void) {
  const char *test = "skode recording commands";
  char filename[128];
  char command[160];
  snprintf(filename, sizeof(filename), "/tmp/pulp-skode-recording-%ld.wav",
           (long)getpid());

  if (recorder_init(64, MAIN_SAMPLE_RATE) != 0) {
    fail(test, "recorder_init failed");
    return;
  }

  skode_t ctx = SKODE_EMPTY();
  skode_init(&ctx);
  ctx.log_enable = 1;

  snprintf(command, sizeof(command), "[%s]/rg", filename);
  if (skode_consume(command, &ctx) != 0 ||
      recorder_state() != RECORDER_RECORDING) {
    fail(test, "[filename]/rg did not start recording");
  }

  char status[] = "/r?";
  if (skode_consume(status, &ctx) != 0 ||
      strstr(ctx.log, "# recorder state=recording") == NULL) {
    fail(test, "/r? did not report recording status");
  }

  char stop[] = "/rs";
  if (skode_consume(stop, &ctx) != 0 ||
      recorder_state() != RECORDER_STOPPED) {
    fail(test, "/rs did not stop recording");
  }

  snprintf(command, sizeof(command), "[%s]/rg0.001", filename);
  if (skode_consume(command, &ctx) != 0 ||
      recorder_state() != RECORDER_RECORDING) {
    fail(test, "[filename]/rg0.001 did not start limited recording");
  } else {
    synth_record_bus_t *bus = recorder_begin_block(64);
    if (!bus) {
      fail(test, "limited recording did not accept audio");
    } else {
      memset(bus->frames, 0, 64 * RECORD_CHANNELS * sizeof(float));
      recorder_end_block(64);
    }
    recorder_stop();
    if (recorder_frames_written() !=
        (uint64_t)(0.001 * MAIN_SAMPLE_RATE)) {
      fail(test, "/rg duration was not applied");
    }
  }

  recorder_uninit();
  unlink(filename);
}

int main(void) {
  test_track_routing();
  test_sub_block_record_offsets();
  test_ring_writer();
  test_duration_limit_and_restart();
  test_skode_recording_commands();

  if (failures) {
    fprintf(stderr, "%d recording test failure(s)\n", failures);
    return 1;
  }
  printf("Recording tests passed\n");
  return 0;
}
