# Native MIDI File Discovery Workflow

This guide demonstrates how to load a `.mid` file natively into the Skode engine and orchestrate it live. By using the built-in background sequencer and the global routing tables, you can progressively discover, map, and synchronize a full MIDI performance without breaking your live-coding flow.

## 1. Load and Inspect
You are in a blank Skode session. First, load the MIDI file into background slot 1.
```skred
[examples/Sweet_Dreams.mid] /mf 1
```

Next, query the status to see what you are working with:
```skred
/mf? 1
```
*Expected Output:*
```text
# MIDI 1: STOPPED tick=0.0 ppq=96 events=0/7881 sync=0
#   -> bpm: 124.0 | notes: 7596 | channels: 0(poly:1), 1(poly:2), 2(poly:1), 3(poly:2), 4(poly:4), 5(poly:6), 6(poly:2), 7(poly:2), 8(poly:2), 9(poly:4), 10(poly:6), 11(poly:1), 12(poly:2)
```

From this output, you discover the natural tempo is **124.0 BPM**, and it uses Channels 0 through 12 .

## 2. Lock the Master Sync
Set Skode's global tempo to match the MIDI file, and lock the MIDI slot to `seq_master_tick()` so it stays perfectly synced when you play or pause the main sequencer.
```skred
124 M
/mfS 1 1  # Set Slot 1 to Sync Mode (1)
```

## 3. Experimenting with Synths
You don't know what instruments are on what channels yet, so you start fishing. Create a basic synth on Voice 4:
```skred
v 4 [TestSynth] vt w 15 f 440 a 5 t 0.01 0.1 0.5 0.2
```

Now, map Channel 1 to it and play the sequencer to hear what it is:
```skred
/mv 1 4  # Route MIDI Ch 1 to Voice 4
/mf> 1   # Arm the MIDI file for playback
/z       # Start the master clock
```
*(You should hear the iconic Lead riff playing!)*

You want to find the Bass, so you create a new synth and map Voice 3 to Channel 0:
```skred
v 3 [BassSynth] vt w 16 f 440 a 6 
/mv 0 3  # Route MIDI Ch 0 to Voice 3
```
*(The Bassline instantly joins the Lead!)*

## 4. Isolating the Drums
You know Channel 9 (MIDI Channel 10) is typically used for drums, but you don't know which specific MIDI notes the file uses. You use the pagination dump command (`/mfD`) to look at the first 50 events and find the Note Ons (`type=9`) on `ch=9`:
```skred
/mfD 1 0 50
```
You will see events like `d1=35` (Kick) and `d1=38` (Snare) in the dump.


Now you map those specific drum notes to your wave samples using `/mb` (MIDI Binding):
```skred
# Load a Kick wave to slot 100
[examples/sk/drums-kick.ks] /ks k>d d>r /r 100

# Initialize drum voice amplitude
v 9 a 10

# Format: [skode-command] /mb type channel note
[v 9 w 100 l 1] /mb 9 9 35
```

By working this way, you can progressively build up your synths, route channels, and inspect the file live while it loops perfectly in sync with the rest of your Skode session!

## 5. Restarting and Looping

Because MIDI files are a linear sequence of events, the file will eventually reach its end and stop playing. 

How you restart it depends on your **Sync Mode**:
* **Free Run Mode (`/mfS 1 0`)**: Simply run `/mf> 1` again. This instantly rewinds the MIDI file to the beginning and resumes playback.
* **Master Sync Mode (`/mfS 1 1`)**: Because the file is tethered to the master clock in this tutorial, you restart it by restarting the entire session. Simply run `/z`. This resets the global `master_tick` to 0, which seamlessly rewinds both your live-coded patterns and the MIDI file simultaneously so they never fall out of phase.

## 6. Advanced: Handling Polyphony (Chords)

If you blindly route a MIDI channel containing chords into a single Skode voice using `/mv`, that single voice will rapidly re-trigger and steal its own notes, flattening the chord.

Because the MIDI player pushes events directly into the live hardware queue, it can utilize Skred's native polyphony engine. Instead of mapping a channel to a single Voice (`/mv`), you map it to a **Polyphonic Pool** (`/mp`).

**Step 1: Build a Template Voice**
Design your synth on a "template" voice (e.g., Voice 10) that you will never play directly.
```skred
v 10 [PolySynth] vt w 15 f 440 a 5 t 0.1 0.1 0.5 0.5
```

**Step 2: Define a Voice Group (`/pg`)**
Create Voice Group 0, using Voice 10 as the source template (with a width of 1 voice).
```skred
# Format: /pg <group_id> <source_voice> <width>
/pg 0 10 1
```

**Step 3: Allocate a Pool (`/pp`)**
Create Pool 0, using Group 0 as its blueprint. Reserve 8 voices of polyphony starting at Voice 20 (it will automatically claim voices 20 through 27).
```skred
# Format: /pp <pool_id> <group_id> <base_voice> <count>
/pp 0 0 20 8
```

**Step 4: Route the MIDI Channel (`/mp`)**
Instead of `/mv`, use `/mp` to route a MIDI channel (e.g., Channel 1) directly into Pool 0.
```skred
# Format: /mp <channel> <pool_id>
/mp 1 0
```

Now, when a 4-note chord is played on Channel 1, the engine dynamically allocates those notes across Voices 20, 21, 22, and 23 simultaneously. You can type `?pp` at any time to monitor the live allocation of your pools!
