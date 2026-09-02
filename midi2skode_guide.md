# `midi2skode`: MIDI to Skode Pattern Generator

`midi2skode` is a custom C utility designed to bridge the gap between traditional MIDI sequencing and Skode's live-coded, step-based architecture. 

Rather than attempting to force a 4-minute linear MIDI song into Skode's 16-step memory limit, `midi2skode` acts as a **pattern decomposition tool**. It analyzes the MIDI file, groups events by track and bar, and dumps out authentic, ready-to-use Skode code (`.sk`) that you can copy, paste, and live-code.

## Architecture and Strategy

The core strategy behind `midi2skode` is **mathematical chunking and metadata extraction**:

1. **Format Agnostic:** It natively parses both Format 0 (single track) and Format 1 (multi-track) MIDI files by reading the raw binary chunks (`MThd` and `MTrk`).
2. **Absolute Time to Step Time:** It calculates the ticks-per-step ratio using the MIDI file's internal Pulses Per Quarter note (PPQ). By combining this with the user's `--div` parameter (e.g., 4 for 16th notes, 2 for 8th notes), it snaps absolute time events directly onto a perfect step grid.
3. **Macro Generation:** It automatically generates Skode `ANDS` parser-compliant macros (`[ChA]`, `[ChB]`) for every active MIDI channel.
4. **General MIDI (GM) Drum Detection:** It identifies MIDI Channel 10 (`ch 9` internally) as the percussion track. It maps these notes to a dedicated `[Drm]` macro and injects comments mapping the MIDI note numbers to their GM drum names (e.g., `35: Acoustic Bass Drum`), eliminating guesswork when assigning your drum samples.
5. **Expression Harvesting:** It intercepts Control Change (CC) messages and injects them as timestamped comments above each pattern block. This provides a blueprint of the original track's filter sweeps, volume fades, and pan automation, which you can recreate manually in your Arranger tracks.
6. **Velocity Scaling:** With the `--velocity` flag, MIDI velocities (0-127) are mathematically scaled down to Skred's native amplitude range (0-20) and injected directly into the pattern atoms.

## Usage Guide

Compile the tool if you haven't already:
```bash
gcc -O2 midi2skode.c -o midi2skode
```

### Basic Usage

Run the tool on a MIDI file and output the results to a `.sk` file:
```bash
./midi2skode examples/Sweet_Dreams.mid > sweet_dreams_raw.sk
```

### Advanced Usage

You can shape the resulting patterns using command-line arguments to perfectly fit your performance goals:

```bash
./midi2skode my_song.mid --meter 4/4 --div 4 --steps 16 --velocity > output.sk
```

*   `--steps <N>`: Adjusts how many steps each Skode pattern holds (defaults to 16).
*   `--div <N>`: Sets the grid resolution. It defaults to `4` (16th notes per quarter note). If the riff you are extracting is entirely composed of 8th notes (like our Sweet Dreams loop), pass `--div 2`.
*   `--meter <N/D>`: Hint the time signature (e.g., `3/4`). The tool calculates the correct pattern size to ensure your chunks loop perfectly in that meter.
*   `--velocity`: Updates the macro definitions to accept a second parameter (`a $$1`) and automatically injects scaled amplitude values for every step.

## Workflow Example: The "Proof of Concept"

Here is how you use `midi2skode` to create a live-codeable performance:

1. **Extract the DNA:** Run `./midi2skode Sweet_Dreams.mid --velocity --div 2 --steps 16 > raw.sk`.
2. **Hunt for Loops:** Open `raw.sk`. You will see hundreds of patterns (e.g., `y 10 yt [T0P0]`, `y 11 yt [T0P1]`). Look for the section where the core groove is playing. In Sweet Dreams, Patterns 0 and 1 represent the iconic 2-bar repeating riff.
3. **Copy and Paste:** Copy those two patterns into a new `performance.sk` file.
4. **Assign Voices:** Look at the generated `# Drum Note 35: Acoustic Bass Drum` comment. Now you know you need to load a Kick sample into `v 9` (the channel associated with `[Drm]`). Look at `[ChA]`, `[ChB]`, and `[ChC]`; assign a Bass synth to `v 0`, a Lead to `v 1`, and an Arp to `v 2`.
5. **Perform:** You now have the mathematically perfect notes, rhythms, and velocities of the original track running in your Skode sequencer, ready for live-coding orchestration!
