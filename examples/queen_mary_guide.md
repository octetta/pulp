# Skode Arrangement Guide: Music for the Funeral of Queen Mary

This guide documents the Skode script `queen_mary.sk`, which arranges Henry Purcell's
*Music for the Funeral of Queen Mary* (the March made famous by Wendy Carlos's Moog
synthesizer rendition in *A Clockwork Orange*).

> **Source material**: Notes and structure taken directly from the
> [classclef.com MIDI file](https://www.classclef.com/midi/Requiem%20Funeral%20of%20Queen%20Mary%20II%20by%20Henry%20Purcell.mid)
> and confirmed against the
> [PDF score](https://www.classclef.com/pdf/Requiem%20Funeral%20of%20Queen%20Mary%20II%20by%20Henry%20Purcell.pdf).

---

## Musical Facts

| Property | Value |
|---|---|
| Composer | Henry Purcell (1659–1695) |
| Key | E minor (guitar arrangement) |
| Time signature | 6/8 |
| Tempo | Dotted-quarter = 40 BPM (`M 60 8` in Skode) |
| Form | 20 bars, Da Capo (played twice) |
| Section A | Bars 1–8 (8 bars = 48 eighth-note steps) |
| Section B | Bars 9–20 (12 bars = 72 eighth-note steps) |

In 6/8, each bar has **two dotted-quarter beats**. The slow, heavy two-beat pulse
is what gives the march its distinctive funeral character.

---

## Form and Structure

```
Intro (2 bars): Timpani solo
Section A ×2 (16 bars): Full ensemble — E minor theme
Section B ×2 (24 bars): Full ensemble — ascending/descending theme
End: All voices released
```

The Timpani pattern (Pattern 1, 12 steps = 2 bars) loops continuously underneath
the brass ensemble throughout the full piece.

---

## Grid and Timing

**Tempo command:** `M 60 8`

`M bpm subdivision` sets the step rate as: `step_freq = (bpm * subdivision) / 240 Hz`.
With `M 60 8`: `(60 × 8) / 240 = 2 Hz` → one step = **0.5 seconds** = one eighth note at d.=40.

This is the correct way to represent 6/8 compound meter in Skode — there is no native
compound-meter mode, but choosing the right BPM and subdivision gives an exact eighth-note
grid. Notes fall on steps 0 and 3 within each 6-step bar, matching the two dotted-quarter
beats of 6/8.

| Duration | Dotted-quarter beats | Steps |
|---|---|---|
| Dotted-half (whole bar in 6/8) | 2 | 6 |
| Dotted-quarter (one beat) | 1 | 3 |
| Quarter | 2/3 | 2 |
| Eighth | 1/3 | 1 |

---

## Synthesis Design (Wendy Carlos Moog Emulation)

### Brass Ensemble (Voices 1–4)

Voices 1–4 run the same sawtooth patch with subtle inter-voice detuning via `N`
(semitones, cents). This creates the thick, beating, analog unison characteristic
of a Moog Modular.

```skred
[bras] : v $$0 w 2 a 0.4 t 0.02 0.15 0.75 0.12 J 1 K 1400 Q 1.8 ;
1 bras  2 bras  3 bras  4 bras

v 1 N 0  0   # Unison (reference)
v 2 N 0  6   # +6 cents
v 3 N 0 -6   # -6 cents
v 4 N 0 -3   # -3 cents
```

Patch parameters:
- `w 2` — Sawtooth oscillator
- `t 0.02 0.15 0.75 0.12` — Fast 20ms attack, 150ms decay, 75% sustain, 120ms release
- `J 1 K 1400 Q 1.8` — 24dB resonant low-pass filter at 1400Hz

### Delay / Reverb

All four brass voices route to Track 1's delay line for reverb space:

```skred
DL 1,6,8,15,0,31,10  # long decay, high feedback, wide stereo mod depth
v 1 p 0 r 1 ds 8     # center-pan, route to track 1, send level 8
```

`DL` parameters: `track, coarse, fine, feedback, modfreq, moddepth, level`

### Timpani (Voice 0)

Triangle wave with a pitch-sweep envelope to simulate a mallet strike. The sweep
is built using `g` (glide/portamento):

```skred
[Tlow] : v 0 g 0 n 60 g 0.08 n 48 l 1 ;
# Instantly set to C4 (no glide), then glide down to C3 over 80ms
```

---

## Chord Macros

Each macro sets a distinct pitch on all four voices and triggers them simultaneously.
Every voice gets its own `n` (pitch) and `l 1` (envelope trigger):

```skred
[SAa] : v 1 n 64 l 1  v 2 n 59 l 1  v 3 n 52 l 1  v 4 n 40 l 1 ;
# Em: E4(soprano) B3(alto) E3(tenor) E2(bass)
```

**Macro naming rules** (required by the `ands` parser):
- 1–4 **alphabetic characters only** — no digits, no symbols
- Digits in a macro name are silently interpreted as command arguments, not
  part of the name, causing the macro to be completely ignored at runtime.
- Section A macros: `SAa` – `SAm`
- Section B macros: `SBa` – `SBr`

---

## Section A — Melody and Harmony (Bars 1–8)

| Bar | Beat | Melody note | Harmony | Pattern step |
|-----|------|-------------|---------|------|
| 1 | 1 | E4 (whole bar) | E minor | 0 |
| 2 | 1 | F4 (dotted-qtr) | D7 | 6 |
| 2 | 2 | F4 (qtr) | A minor | 9 |
| 3 | 1 | E4 (whole bar) | E minor/C# | 12 |
| 4 | 1 | E4 (qtr) | A minor | 18 |
| 4 | 2 | A4 (qtr) | A major | 21 |
| 5 | 1 | C4 (dotted-qtr) | C major | 24 |
| 5 | 2 | C4 (dotted-qtr) | C major | 27 |
| 6 | 1 | A4 (dotted-qtr) | F major | 30 |
| 6 | 2 | A4 (qtr) | F major | 33 |
| 7 | 1 | Ab4 (whole bar) | E augmented | 36 |
| 8 | 1 | Ab4 (qtr) | E augmented | 42 |
| 8 | 2 | A4 (qtr) | E/F# | 45 |

---

## Section B — Melody and Harmony (Bars 9–20)

| Bar | Beat | Melody note | Harmony | Pattern step |
|-----|------|-------------|---------|------|
| 9 | 1 | B4 (whole bar) | Em/G | 0 |
| 10 | 1 | A4 (dotted-qtr) | A major | 6 |
| 10 | 2 | F#4 (dotted-qtr) | B minor | 9 |
| 11 | 1 | G4 (whole bar) | E minor (low) | 12 |
| 12 | 1 | E4 (qtr) | C major | 18 |
| 12 | 2 | B3 (qtr) | G/D | 21 |
| 13 | 1 | G4 (whole bar) | C major | 24 |
| 14 | 1 | F4 (dotted-qtr) | F major | 30 |
| 14 | 2 | D4 (dotted-qtr) | G major | 33 |
| 15 | 1 | E4 (whole bar) | C major | 36 |
| 16 | 1 | E4 (qtr) | C major | 42 |
| 16 | 2 | G4 (qtr) | G major | 45 |
| 17 | 1 | A4 (whole bar) | A minor | 48 |
| 18 | 1 | A4 (dotted-qtr) | D minor | 54 |
| 18 | 2 | Ab4 (dotted-qtr) | E7 (leading tone) | 57 |
| 19 | 1 | A4 (whole bar) | A minor | 60 |
| 20 | 1 | D4 (qtr) | D minor | 66 |
| 20 | 2 | E4 (qtr) | E minor (final) | 69 |

---

## Note-Offs and Gate Timing

The brass voices have a sustained envelope (`t 0.02 0.15 0.75 0.12`). Without
explicit note-offs, notes ring indefinitely. An `[off]` macro is placed at the
end of each note's gate duration:

```skred
[ SAa ] x 0    # Trigger E minor chord at step 0
[ off ] x 5    # Release all voices at step 5 (90% of 6-step whole bar)
[ SAb ] x 6    # Next chord has a clean attack
```

The Timpani has zero sustain and does **not** require note-offs.

---

## Master Orchestrator (Pattern 0)

Pattern 0 uses a 48-step modulo (= 8 bars) as its clock. The Timpani loops
automatically. `/z pattern state` starts (`1`) or stops (`0`) a pattern.

```skred
y0
48 %

[ /z 1 1 ] x 0              # Timpani solo intro
[ /z 2 1 ] x 1              # Section A, pass 1
[ /z 2 0   /z 2 1 ] x 2     # Section A, pass 2 (restart)
[ /z 2 0   /z 3 1 ] x 3     # Section B, pass 1
[ /z 3 0   /z 3 1 ] x 5     # Section B, pass 2 (restart)
[ /z 3 0   /z 1 0   off ] x 7  # Stop all + release voices
[ /z 0 0 ] x 8              # Stop master
```

Section B is 72 steps = 1.5 master steps, so it spans from master step 3 to
the start of step 5 naturally before the restart fires.

---

## How to Play

```skred
# Load and start the full piece:
[examples/queen_mary.sk] /ls
y0 z 1

# Loop just Section A (for development):
[examples/queen_mary.sk] /ls
y2 z 1

# Hear the Timpani alone:
[examples/queen_mary.sk] /ls
y1 z 1
```

---

## Key Skode Commands Reference

| Command | Purpose |
|---|---|
| `M 60 8` | Set tempo (dotted-quarter = 40 BPM) |
| `w 2` / `w 1` | Sawtooth / Triangle oscillator |
| `t A D S R` | ADSR envelope times |
| `J 1 K freq Q res` | 24dB low-pass filter |
| `N semitones cents` | Per-voice pitch offset (detune) |
| `g seconds` | Portamento glide rate (real-time safe via SKODE_OP_GLISSANDO opcode) |
| `n midi_note` | Set voice pitch |
| `l 1` / `l 0` | Note on / note off |
| `DL track,...` | Configure track delay (reverb) |
| `r 1` / `p 0` / `ds amount` | Route voice to track / center / delay send |
| `y pattern` | Select active pattern for editing |
| `N %` | Set pattern clock divisor (steps per master tick) |
| `/z pattern state` | Start or stop a pattern |
| `[name] : body ;` | Define a macro (1–4 alpha chars, no digits) |
| `[ cmd ] x step` | Assign a command string to a pattern step |
