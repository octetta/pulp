/*
 * Hard-sync (T v1,v2,...) tests.
 *
 * The render loop is driven one frame at a time so we can watch the slave's
 * phase right after the master wraps.
 */
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "skode.h"
#include "synth.h"
#include "synth-types.h"

static int failures;

static void expect(int condition, const char *message) {
  if (condition) return;
  fprintf(stderr, "FAIL hard sync: %s\n", message);
  failures++;
}

static void setup(void) {
  synth_init(8);
  wave_table_init(0);
  voice_init();
  synth_sample_rate_set(48000);
}

static void teardown(void) {
  wave_free();
  synth_free();
}

static void cmd(const char *s) {
  char copy[256];
  snprintf(copy, sizeof(copy), "%s", s);
  if (skred_command(copy) != 0) fprintf(stderr, "command failed: %s\n", s);
}

static void render1(void) {
  float buf[2] = {0, 0};
  synth(&skred_global_engine, buf, NULL, 1, 2, NULL);
}

static int sync_slot(int v, int slot) {
  float *p[6] = {sv.link_sync_0, sv.link_sync_1, sv.link_sync_2,
                 sv.link_sync_3, sv.link_sync_4, sv.link_sync_5};
  return (int)p[slot][v];
}

static void test_configuration(void) {
  setup();
  for (int i = 0; i < 6; i++)
    expect(sync_slot(0, i) == -1, "sync slots start unset");

  cmd("v0 T1,3,5");
  expect(sync_slot(0, 0) == 1 && sync_slot(0, 1) == 3 && sync_slot(0, 2) == 5,
         "T1,3,5 installs three targets");
  expect(sync_slot(0, 3) == -1 && sync_slot(0, 5) == -1, "unused slots stay -1");

  cmd("v0 T2");
  expect(sync_slot(0, 0) == 2 && sync_slot(0, 1) == -1 && sync_slot(0, 2) == -1,
         "a new T replaces the list instead of accumulating");

  cmd("v0 T0,1");
  expect(sync_slot(0, 0) == -1 && sync_slot(0, 1) == 1,
         "self-link is dropped, others kept");

  cmd("v0 T1,2,3,4,5,6");
  expect(sync_slot(0, 5) == 6, "six targets accepted");

  cmd("v0 T99");
  expect(sync_slot(0, 0) == -1, "out-of-range target is ignored");

  cmd("v0 T1,2");
  cmd("v0 T-1");
  expect(sync_slot(0, 0) == -1 && sync_slot(0, 1) == -1, "T-1 clears the list");

  /* voice copy and reset carry/clear the list */
  cmd("v0 T1,2");
  cmd("v0 >4");
  expect(sync_slot(4, 0) == 1 && sync_slot(4, 1) == 2, "voice copy copies sync targets");
  voice_init();
  expect(sync_slot(0, 0) == -1, "voice reset clears sync targets");

  /* dump re-emits T so a dumped voice can be replayed */
  cmd("v0 T1,2");
  char out[2048];
  voice_format(0, out, sizeof(out), 0);
  expect(strstr(out, " T1,2,") != NULL, "voice dump contains T targets");
  teardown();
}

static void test_bare_T_still_retriggers(void) {
  setup();
  cmd("v1 f440 a0 t0.001,0.1,1,0.5");
  expect(!sv.amp_envelope[1].is_active, "voice idle before T");
  cmd("v1 T");
  render1();
  expect(sv.amp_envelope[1].is_active, "bare T still retriggers (backward compatible)");
  for (int i = 0; i < 6; i++)
    expect(sync_slot(1, i) == -1, "bare T does not touch sync targets");
  teardown();
}

/* Run n frames; return how many master wraps were seen and how many of them
   left the slave's phase within `tol` of its restart point on the same or the
   next frame. */
static void watch(int master, int slave, int frames, int *wraps, int *synced,
                  int *slave_wraps_alone) {
  *wraps = *synced = *slave_wraps_alone = 0;
  double prev_m = sv.phase[master], prev_s = sv.phase[slave];
  int pending = 0;
  for (int i = 0; i < frames; i++) {
    render1();
    double m = sv.phase[master], s = sv.phase[slave];
    int master_wrapped = m < prev_m;      /* forward playback */
    if (master_wrapped) { (*wraps)++; pending = 2; }
    if (pending) {
      /* slave should be sitting near the start of its table (<= 2 incs) */
      if (s <= 2.5 * sv.phase_inc[slave] + 1e-6) { (*synced)++; pending = 0; }
      else pending--;
    }
    if (!master_wrapped && s < prev_s && !pending) (*slave_wraps_alone)++;
    prev_m = m; prev_s = s;
  }
}

static void test_simple_cycle_sync(void) {
  /* master 110 Hz, slave 330 Hz (non-integer relationship is not required,
     use 337 so the slave would normally drift against the master) */
  setup();
  cmd("v0 f110 a0 l1");
  cmd("v1 f337 a0 l1");
  cmd("v0 T1");
  for (int i = 0; i < 200; i++) render1();
  int wraps, synced, free_wraps;
  watch(0, 1, 48000 / 4, &wraps, &synced, &free_wraps);
  expect(wraps > 20, "master completes many cycles");
  expect(synced >= wraps - 1, "slave phase restarts on every master wrap (simple cycle)");
  teardown();

  /* control: without T the slave is not reset */
  setup();
  cmd("v0 f110 a0 l1");
  cmd("v1 f337 a0 l1");
  for (int i = 0; i < 200; i++) render1();
  watch(0, 1, 48000 / 4, &wraps, &synced, &free_wraps);
  expect(synced < wraps / 4, "control: unsynced slave is not restarted by master");
  teardown();
}

static void test_cleared_stops_syncing(void) {
  setup();
  cmd("v0 f110 a0 l1");
  cmd("v1 f337 a0 l1");
  cmd("v0 T1");
  cmd("v0 T-1");
  for (int i = 0; i < 200; i++) render1();
  int wraps, synced, free_wraps;
  watch(0, 1, 48000 / 4, &wraps, &synced, &free_wraps);
  expect(synced < wraps / 4, "cleared list no longer syncs");
  teardown();
}

static void test_multiple_targets(void) {
  setup();
  cmd("v0 f110 a0 l1");
  cmd("v1 f337 a0 l1");
  cmd("v2 f521 a0 l1");
  cmd("v3 f760 a0 l1");
  cmd("v0 T1,2,3");
  for (int i = 0; i < 200; i++) render1();
  int w1, s1, f1, w2, s2, f2;
  /* watch v1 then check v2/v3 phases sit near start right after a wrap */
  double prev = sv.phase[0];
  int checks = 0, good = 0;
  for (int i = 0; i < 48000 / 4; i++) {
    render1();
    double m = sv.phase[0];
    if (m < prev) {
      render1();
      checks++;
      int ok = 1;
      for (int v = 1; v <= 3; v++)
        if (sv.phase[v] > 3.0 * sv.phase_inc[v] + 1e-6) ok = 0;
      good += ok;
    }
    prev = sv.phase[0];
  }
  (void)w1; (void)s1; (void)f1; (void)w2; (void)s2; (void)f2;
  expect(checks > 20 && good >= checks - 1, "all three targets restart on each master wrap");
  teardown();
}

static void test_reverse_master_general_path(void) {
  /* reverse playback uses the general (non simple-cycle) oscillator path */
  setup();
  cmd("v0 f110 a0 b1 l1");
  cmd("v1 f337 a0 l1");
  cmd("v0 T1");
  expect(sv.playback_class[0] != sv.playback_class[1],
         "reversed master takes the general oscillator path");
  for (int i = 0; i < 400; i++) render1();
  double prev = sv.phase[0];
  int wraps = 0, good = 0;
  for (int i = 0; i < 48000 / 4; i++) {
    render1();
    double m = sv.phase[0];
    if (m > prev) {               /* reverse wrap: jumps back up to the end */
      render1();
      wraps++;
      if (sv.phase[1] <= 3.0 * sv.phase_inc[1] + 1e-6) good++;
    }
    prev = sv.phase[0];
  }
  expect(wraps > 20, "reverse master wraps");
  expect(good >= wraps - 1, "general-path wrap also syncs the slave");
  teardown();
}

static void test_master_stays_alive_for_sound_target(void) {
  /* The mixer skips whole blocks of VOICE_ALIGN voices that have nothing
     active, so master (v0, block 0) and slave (v4, block 1) are placed in
     different blocks.  The master is never triggered itself; only the slave
     is.  The sparsity pass must still run the master or sync silently stops. */
  setup();
  cmd("v0 f110 a0");
  cmd("v4 f337 a0 l1");
  double before = sv.phase[0];
  for (int i = 0; i < 300; i++) render1();
  expect(sv.phase[0] == before, "control: idle master in a skipped block does not advance");

  cmd("v0 T4");
  for (int i = 0; i < 300; i++) render1();
  expect(sv.phase[0] != before, "sync master keeps running while its target sounds");

  /* and once the target is silent again the master goes back to sleep */
  cmd("v4 l0");
  for (int i = 0; i < 48000; i++) render1();   /* let the release finish */
  double parked = sv.phase[0];
  for (int i = 0; i < 300; i++) render1();
  expect(sv.phase[0] == parked, "master sleeps again once the target is inactive");
  teardown();
}

/* The defining property of hard sync: the slave's waveform repeats at the
   MASTER's period even though the slave's own frequency is unrelated. */
static double repeat_error(int use_sync) {
  setup();
  cmd("v0 f100 a0 l1");     /* 100 Hz at 48 kHz = 480 samples per cycle */
  cmd("v1 f337 a0 l1");
  if (use_sync) cmd("v0 T1");
  enum { N = 4000, LAG = 480 };
  static float out[N];
  for (int i = 0; i < 2000; i++) render1();
  for (int i = 0; i < N; i++) { render1(); out[i] = sv.sample[1]; }
  double err = 0, energy = 0;
  for (int i = 0; i + LAG < N; i++) {
    double d = out[i] - out[i + LAG];
    err += d * d; energy += (double)out[i] * out[i];
  }
  teardown();
  return energy > 0 ? err / energy : 1e9;
}

static void test_output_repeats_at_master_period(void) {
  double synced = repeat_error(1), free_run = repeat_error(0);
  expect(synced < 0.02, "synced slave output repeats at the master period");
  expect(free_run > 0.5, "control: free-running slave does not repeat at that period");
}

int main(void) {
  test_configuration();
  test_bare_T_still_retriggers();
  test_simple_cycle_sync();
  test_cleared_stops_syncing();
  test_multiple_targets();
  test_reverse_master_general_path();
  test_master_stays_alive_for_sound_target();
  test_output_repeats_at_master_period();
  if (failures) {
    fprintf(stderr, "%d hard sync test failure(s)\n", failures);
    return 1;
  }
  printf("Hard sync tests passed\n");
  return 0;
}
