# MOD to Skode Translation Guide

To achieve perfect MOD tracker playback in Skode, we need to mathematically align Skode's `master_tick` rate with the MOD's "Tick" rate, and then use `%` to tell Skode how many ticks make up a single tracker "Row".

Here is how the Skode settings map to standard tracker mechanics:

## 1. Setting the Tick Rate (The `M` command)
In Skode, the `M` command sets the global tempo and subdivision: `M <bpm> <subdivision>`.

In a standard MOD tracker (like Protracker), the formula for tick duration in seconds is: 
`tick_duration = 2.5 / BPM`. 
This means at 125 BPM, the tracker processes ticks at exactly 50 Hz.

Skode's formula for tick frequency is:
`step_freq_hz = (bpm * subdivision) / 240.0`

If we want Skode's tick rate to exactly match the tracker tick rate, we solve for subdivision:
`(BPM * subdivision) / 240 = BPM / 2.5`
`subdivision = 240 / 2.5`
**`subdivision = 96`**

So your translator simply needs to emit this globally at the top of the file:
```skode
M 125 96
```
This forces the sequencer's `master_tick` engine to beat exactly 50 times a second at 125 BPM! If the tracker file uses the `Fxx` command (where xx >= 32) to change the BPM mid-song, the translator just drops `M xx 96` into the orchestrator stream.

## 2. Setting the Speed/Row Length (The `%` command)
In trackers, the default "Speed" is 6, meaning each Row lasts for exactly 6 ticks. 
Now that Skode's `master_tick` is locked to tracker ticks, you literally just use the `%` command (Modulo) to define the Row length!

```skode
y 0 %6   # Pattern 0 (Channel 1) advances 1 step every 6 ticks
y 1 %6   # Pattern 1 (Channel 2) advances 1 step every 6 ticks
```
If the tracker file uses the `Fxx` command (where xx < 32) to change the speed (e.g. Speed 5), the translator just injects `%5` at that step to instantly change the row length!

## 3. Writing the Ticks (The `+-` command)
Now that everything is perfectly aligned, the Mod-to-Skode translator can write a single tracker Row as a single Skode Step, using `+-` to place effects on sub-ticks:

```skode
# Note on tick 0, Volume slide down on ticks 1, 2, 3, 4, 5
[n60 l1 +-1 v30 +-2 v20 +-3 v10 +-4 v5 +-5 v0] x0
```

## Summary of Translation Rules
- **MOD BPM** = `M <bpm> 96`
- **MOD Speed** = `%<speed>`
- **MOD Row** = `[...] x<row>`
- **MOD Effect Tick** = `+-<tick>` inside the block!

## Appendix: Transport Control Commands (`z`, `zq`, `zg`)

When building orchestrator sequences or managing patterns, you will use the `z` family of commands to control pattern playback.

### `z` (Immediate State)
**Usage:** `z1` (Play), `z0` (Stop)
This sets the pattern's running state **instantly**. If you run `z1` in the middle of a beat, the pattern will start playing immediately right at that exact millisecond. It does not wait for a downbeat.

### `zq` (Queued State)
**Usage:** `zq1` (Queue Play), `zq0` (Queue Stop)
This puts the pattern into a "pending" state. It tells the sequencer: *"Get ready to change this pattern's state, but wait for the perfect musical moment to actually do it."* 
- If a **Master Pattern** (set via `yp <pattern_id>`) is playing, the pattern waits until the master pattern wraps around to step 0, and then starts flawlessly in sync.
- If no Master Pattern is active, it waits for the next natural modulo boundary (e.g. if the pattern is `%16`, it waits until the global beat clock hits a perfect multiple of 16).

### `zg` (Goto Step)
**Usage:** `zg0`, `zg4`, `zg12`
This forces a running pattern to instantly jump to a specific step. It literally means "Go To". If pattern 0 is currently playing step 10, running `zg0` will instantly snap it back to step 0 and it will continue playing from there. 

## Appendix: Managing and Updating Streams within Patterns

If you are using streams (e.g. `nS0`) inside a pattern to sequence notes or parameter values, you cannot directly redefine the stream array contents from inside a pattern step (because `/SS` is an immediate-only REPL command). 

However, you **can** cleanly update streams inside a pattern step by using the "Memory Bank" technique with the Stream Copy (`/SC`) command!

### The Memory Bank Technique
1. **Define your sequences in the REPL (Memory Banks)**
   First, load your various musical phrases into unused streams (e.g. Streams 1, 2, and 3).
   ```skode
   1 [ 60 62 64 65 ] /SS
   2 [ 72 71 69 67 ] /SS
   3 [ 48 55 48 55 ] /SS
   ```

2. **Use an "Active" Stream in your looping pattern**
   Set up a pattern that continuously pulls from Stream 0.
   ```skode
   y 0 %1
   [ nS0 l1 ] x0
   z1
   ```

3. **Update the Active Stream from an Orchestrator Pattern**
   Now, in your orchestrator pattern (or even a deferred block), you can instantly swap the sequence that Pattern 0 is playing by compiling the `/SC` (Stream Copy) and `/SP` (Stream Position) commands!
   
   ```skode
   y 127 %16
   
   # Step 0: Copy phrase A (Stream 1) into active Stream 0, reset position to 0
   [ /SC 0 1  /SP 0 0 ] x0
   
   # Step 1: Copy phrase B (Stream 2) into active Stream 0, reset position to 0
   [ /SC 0 2  /SP 0 0 ] x1
   
   # Step 2: Copy phrase C (Stream 3) into active Stream 0, reset position to 0
   [ /SC 0 3  /SP 0 0 ] x2
   ```

### Available Compilable Stream Commands
These commands have `SKODE_OP_STREAM_*` opcodes, meaning they are fully real-time safe and can be used directly inside `[...]` pattern blocks:
- **`/SC <dst> <src>`** (Stream Copy): Replaces the contents of the destination stream with the source stream.
- **`/SP <stream> <pos>`** (Stream Position): Overrides the playhead position of a stream (useful for resetting to `0` when swapping phrases).
- **`/SM <stream> <mode>`** (Stream Mode): Changes the playback behavior of a stream (e.g., Forward, Ping-pong, Random).
