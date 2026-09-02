# Skode MIDI Player Feature Guide

The internal Skode `.mid` file player has been upgraded to a first-class sequencer citizen. It now features full playback management, synchronization, prefix parsing, and safe text-dumping commands.

## 1. Load and Playback Management

### `/mf` (Load MIDI File)
Loads a MIDI file into one of the 4 background sequencer slots (0-3).
```skred
[examples/Sweet_Dreams.mid] /mf 1  # Loads the file into slot 1
```

### Transport Controls
Trigger playback exactly like you trigger patterns.
```skred
/mf> 1   # Play slot 1
/mf< 1   # Stop slot 1
```

### `/mfP` (Position / Seek)
Jumps the playhead to a specific MIDI tick. Events are chronologically sorted in memory, so this is instantaneous and O(1).
```skred
/mfP 1 480   # Seeks slot 1 to tick 480
```

### `/mf?` (Status Query)
Queries the current state of a loaded MIDI slot. If no slot is provided, it prints the status of all 4 slots.
```skred
/mf? 1
# Output: 
# MIDI 1: PLAYING tick=480.0 ppq=96 events=12/7881 sync=0
#   -> bpm: 124.0 | notes: 7596 | channels: 0(poly:1), 1(poly:2), 2(poly:1), 3(poly:2), 4(poly:4), 5(poly:6), 6(poly:2), 7(poly:2), 8(poly:2), 9(poly:4), 10(poly:6), 11(poly:1), 12(poly:2)
```

## 2. Synchronization Modes (`/mfS`)

By default, the MIDI player free-runs based on its own internal clock and the global tempo. However, you can mathematically lock it to Skode's master sequencer.

```skred
/mfS 1 1  # Sets slot 1 to Master Sync mode
/mfS 1 0  # Sets slot 1 back to Free-Run mode
```

**Master Sync Mode (1)**: 
The MIDI file's internal `current_tick` is overridden by Skode's global `seq_master_tick()`. 
* If you rewind the `y` pattern sequences, the `.mid` file rewinds automatically!
* If your sequences jump ahead, the `.mid` file jumps ahead.
* It guarantees your `.mid` file never drifts out of phase with your live-coded drum patterns.

## 3. The Pagination API (`/mfD`)

Dumping a 4-minute MIDI file to the console usually results in UDP buffer overflows and dropped packets. To safely inspect the contents of a MIDI file over the network, use the paginated Dump command.

```skred
# /mfD <slot> <start_index> <limit>
/mfD 1 0 10   # Dump the first 10 events of slot 1
```

**Output format:**
```skred
# MIDI Dump Slot 1: Events 0 to 9 (of 7881)
# [0] tick=99 type=11 ch=0 d1=7 d2=105
# [1] tick=99 type=11 ch=0 d1=10 d2=20
```

* `type=9` is Note On
* `type=8` is Note Off
* `type=11` is Control Change (CC)

This allows front-end scripts or text editors to safely query the exact contents of the MIDI file in chunks, guaranteeing zero dropped network packets.
