#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "scope-ipc.h"
#include "skode.h"

static int failures;

static void fail(const char *message) {
  fprintf(stderr, "FAIL scope IPC: %s\n", message);
  failures++;
}

static void fill_frames(float *frames, int first, int count) {
  for (int frame = 0; frame < count; frame++) {
    for (int channel = 0; channel < RECORD_CHANNELS; channel++) {
      frames[(size_t)frame * RECORD_CHANNELS + channel] =
        (float)((first + frame) * 100 + channel);
    }
  }
}

static int child_verify(const char *name) {
  skred_scope_reader_t reader;
  if (scope_ipc_reader_open(&reader, name) != 0) return 10;
  if (reader.header->sample_rate != 100 ||
      reader.header->channel_count != RECORD_CHANNELS ||
      reader.header->channel_mask != 3 ||
      reader.header->capacity_frames != 8) {
    scope_ipc_reader_close(&reader);
    return 11;
  }
  float latest[8 * RECORD_CHANNELS];
  uint64_t first = 0;
  int count = scope_ipc_reader_latest(&reader, latest, 8, &first);
  if (count != 8 || first != 4) {
    scope_ipc_reader_close(&reader);
    return 12;
  }
  for (int frame = 0; frame < count; frame++) {
    for (int channel = 0; channel < RECORD_CHANNELS; channel++) {
      float expected = (float)((frame + 4) * 100 + channel);
      if (latest[(size_t)frame * RECORD_CHANNELS + channel] != expected) {
        scope_ipc_reader_close(&reader);
        return 13;
      }
    }
  }
  scope_ipc_reader_close(&reader);
  return 0;
}

static void test_cross_process_ring(void) {
  char name[80];
  snprintf(name, sizeof(name), "pulp-scope-%ld", (long)getpid());
  if (scope_ipc_init(8, 100) != 0 ||
      scope_ipc_start(name, 3, 0.08) != 0) {
    fail("initialization failed");
    return;
  }

  synth_record_bus_t *bus = scope_ipc_begin_block(6);
  if (!bus) {
    fail("first capture bus unavailable");
  } else {
    fill_frames(bus->frames, 0, 6);
    scope_ipc_publish(bus->frames, 6);
  }
  bus = scope_ipc_begin_block(6);
  if (!bus) {
    fail("second capture bus unavailable");
  } else {
    fill_frames(bus->frames, 6, 6);
    scope_ipc_publish(bus->frames, 6);
  }

  pid_t child = fork();
  if (child < 0) {
    fail("fork failed");
  } else if (child == 0) {
    _exit(child_verify(name));
  } else {
    int status = 0;
    if (waitpid(child, &status, 0) != child ||
        !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
      fail("external reader validation failed");
    }
  }

  skred_scope_reader_t old_reader;
  if (scope_ipc_reader_open(&old_reader, name) != 0) {
    fail("could not retain old generation");
  } else {
    uint64_t old_generation = old_reader.header->generation;
    scope_ipc_stop();
    if (old_reader.header->active != 0) fail("old generation stayed active");
    if (scope_ipc_start(name, 3, 0.08) != 0) {
      fail("restart failed");
    } else {
      skred_scope_reader_t new_reader;
      if (scope_ipc_reader_open(&new_reader, name) != 0 ||
          new_reader.header->generation <= old_generation) {
        fail("restart generation did not advance");
      }
      scope_ipc_reader_close(&new_reader);
    }
    scope_ipc_reader_close(&old_reader);
  }
  scope_ipc_uninit();
}

static void test_skode_commands(void) {
  char name[80];
  char command[128];
  snprintf(name, sizeof(name), "pulp-scope-command-%ld", (long)getpid());
  if (scope_ipc_init(8, 100) != 0) {
    fail("command test initialization failed");
    return;
  }
  skode_t ctx = SKODE_EMPTY();
  skode_init(&ctx);
  ctx.log_enable = 1;

  snprintf(command, sizeof(command), "[%s]/sg3,.08", name);
  if (skode_consume(command, &ctx) != 0 || !scope_ipc_active() ||
      strstr(ctx.log, "# scope [") == NULL) {
    fail("/sg did not start publication");
  }
  char status[] = "/s?";
  if (skode_consume(status, &ctx) != 0 ||
      strstr(ctx.log, "scope state=publishing") == NULL) {
    fail("/s? did not report publication");
  }
  char stop[] = "/ss";
  if (skode_consume(stop, &ctx) != 0 || scope_ipc_active()) {
    fail("/ss did not stop publication");
  }
  char start_default[] = "/sg";
  if (skode_consume(start_default, &ctx) != 0 || !scope_ipc_active() ||
      strcmp(scope_ipc_name(), "/skred-scope") != 0 ||
      scope_ipc_capacity_frames() != 100) {
    fail("default /sg settings were not applied");
  }
  scope_ipc_stop();
  scope_ipc_uninit();
}

int main(void) {
  test_cross_process_ring();
  test_skode_commands();
  if (failures) return 1;
  printf("Scope IPC tests passed\n");
  return 0;
}
