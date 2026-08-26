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

| Property        | Value                                      |
|-----------------|--------------------------------------------|
| Composer        | Henry Purcell (1659–1695)                  |
| Key             | E minor                                    |
| Time signature  | 6/8                                        |
| Tempo           | Dotted-quarter = 40 BPM (`M 60 8`)         |
| Form            | 20 bars, Da Capo (played twice)            |
| Section A       | Bars 1–8 (8 bars = 48 eighth-note steps)   |
| Section B       | Bars 9–20 (12 bars = 72 eighth-note steps) |

In 6/8 each bar has **two dotted-quarter beats**. The slow, heavy two-beat pulse
is what gives the march its distinctive funeral character.

---

## Form and Structure

```
Intro (2 bars):   Timpani solo
Section A ×2      (16 bars): Full ensemble — E minor theme
Section B ×2      (24 bars): Full ensemble — ascending/descending theme
End:              All voices released
```

The timpani pattern (Pattern 1, **6 steps = 1 bar**) loops continuously underneath
the brass throughout the piece.

---

## Grid and Timing

**Tempo command:** `M 60 8`

`M bpm subdivision` sets the step rate as:  
`step_freq = (bpm * subdivision) / 240 Hz`.

With `M 60 8`: `(60 × 8) / 240 = 2 Hz` → one step = **0.5 s** = one eighth note at dotted-quarter = 40.

This is the correct way to represent 6/8 compound meter in Skode. Notes fall on steps 0 and 3 within each 6-step bar, matching the two dotted-quarter beats.

| Duration                  | Dotted-quarter beats | Steps |
|---------------------------|----------------------|-------|
| Dotted-half (whole bar)   | 2                    | 6     |
| Dotted-quarter (one beat) | 1                    | 3     |
| Quarter                   | 2/3                  | 2     |
| Eighth                    | 1/3                  | 1     |

**Important engine note:** A trailing blank step is trimmed by the pattern engine. To force a pattern to a specific length you must put an explicit no-op comment on the final step (e.g. `[#] x5`).

---

## Synthesis Design

### Brass Ensemble (Voices 1–4)

DW-8000 digital wave through a resonant analog-style low-pass, light inter-voice detune, gentle portamento, and small attack delays so the chords bloom instead of slamming.

```skred
[bras] : v $$0 w 18 a 0.36 t 0.05 0.22 0.70 0.50 J 1 K 1050 Q 1.6 ;
1 bras
2 bras
3 bras
4 bras

v 1 N 0  0
v 2 N 0  6
v 3 N 0 -5
v 4 N 0 -2

v 1 g 0.045
v 2 g 0.05
v 3 g 0.04
v 4 g 0.055

# Small attack delays (L = seconds)
v 1 L 0.00
v 2 L 0.012
v 3 L 0.025
v 4 L 0.008
```

- `w 18` — one of the DW-8000 waves (try 15–30 for different colours)
- Light detune creates the classic thick analog-unison beating
- `L` values stagger the four voices for a more human attack

### Delay / Space

```skred
DL 1 6 8 7 0 4 7
DD 1 11 5
```

- Coarse 6 / fine 8 → moderate delay time
- Feedback 7, very light modulation (mod-freq 0, mod-depth 4)
- Damping keeps the tail dark and controlled so it supports rather than detunes the brass

All brass voices are routed centre-panned into the delay bus with a modest send:

```skred
v 1 p 0 r 1 ds 5
v 2 p 0 r 1 ds 5
v 3 p 0 r 1 ds 5
v 4 p 0 r 1 ds 5
```

### Timpani (Voice 0)

Triangle wave with a fast pitch drop and filter thump to simulate a mallet strike on a membrane.

```skred
[Tx]  : v0 g0 n $$0 l1  v0 g.04 n $$1 ;
[Tlow]: Tx 41 36 ;     # E2 → C2
```

---

## Chord Macros

Each macro sets pitch + trigger on all four voices simultaneously.

**Macro naming rules** (required by the ands parser):
- 1–4 **alphabetic characters only** — no digits, no symbols
- Digits inside a macro name are treated as command arguments and the macro is silently ignored
- Section A: `SAa`–`SAm`
- Section B: `SBa`–`SBr`

---

## Section A — Melody and Harmony (Bars 1–8)

| Bar | Beat | Melody | Harmony     | Step |
|-----|------|--------|-------------|------|
| 1   | 1    | E4     | Em          | 0    |
| 2   | 1    | F4     | D7          | 6    |
| 2   | 2    | F4     | Am          | 9    |
| 3   | 1    | E4     | Em/C#       | 12   |
| 4   | 1    | E4     | Am          | 18   |
| 4   | 2    | A4     | A           | 21   |
| 5   | 1    | C4     | C           | 24   |
| 5   | 2    | C4     | C           | 27   |
| 6   | 1    | A4     | F           | 30   |
| 6   | 2    | A4     | F/F3        | 33   |
| 7   | 1    | Ab4    | Eaug        | 36   |
| 8   | 1    | Ab4    | Eaug        | 42   |
| 8   | 2    | A4     | E/F#        | 45   |

---

## Section B — Melody and Harmony (Bars 9–20)

| Bar | Beat | Melody | Harmony     | Step |
|-----|------|--------|-------------|------|
| 9   | 1    | B4     | Em/G        | 0    |
| 10  | 1    | A4     | A           | 6    |
| 10  | 2    | F#4    | Bm          | 9    |
| 11  | 1    | G4     | Em low      | 12   |
| 12  | 1    | E4     | C           | 18   |
| 12  | 2    | B3     | G/D         | 21   |
| 13  | 1    | G4     | C           | 24   |
| 14  | 1    | F4     | F           | 30   |
| 14  | 2    | D4     | G           | 33   |
| 15  | 1    | E4     | C           | 36   |
| 16  | 1    | E4     | C           | 42   |
| 16  | 2    | G4     | G           | 45   |
| 17  | 1    | A4     | Am          | 48   |
| 18  | 1    | A4     | Dm          | 54   |
| 18  | 2    | Ab4    | E7          | 57   |
| 19  | 1    | A4     | Am          | 60   |
| 20  | 1    | D4     | Dm          | 66   |
| 20  | 2    | E4     | Em (final)  | 69   |

---

## Note-Offs and Gate Timing

Brass voices use a sustained envelope. Explicit `[off]` macros are placed near the end of each note’s duration so the next chord has a clean attack.

```skred
[SAa] x0
[off] x5
[SAb] x6
...
```

Timpani has zero sustain and needs no note-offs.

---

## Patterns

### Pattern 1 – Timpani (6 steps = 1 bar)

```skred
y1
% 1
[Tlow] x0
[] x1
[] x2
[Tlow] x3
[] x4
[#] x5          # force length = 6 (trailing blank would be trimmed)
```

### Pattern 2 – Section A (48 steps)

```skred
y2
% 1
[SAa] x0
[off] x5
...
[SAm] x45
[off] x47
```

### Pattern 3 – Section B (72 steps)

```skred
y3
% 1
[SBa] x0
[off] x5
...
[SBr] x69
[off] x71
```

### Master (Pattern 0)

```skred
y0
% 12                # each master step = 2 bars
[ /z 1 1 ] x0       # timpani solo intro
[ /z 2 1 ] x1       # Section A pass 1
[ /z 2 0 /z 2 1 ] x5
[ /z 2 0 /z 3 1 ] x9
[ /z 3 0 /z 3 1 ] x15
[ /z 3 0 /z 1 0 off ] x21
[ /z 0 0 ] x22
```

---

## How to Play

```skred
# Full piece
[examples/queen_mary.sk] /ls
y0 z 1

# Section A only
y2 z 1

# Timpani alone
y1 z 1
```

---

## Key Skode Commands Used

| Command              | Purpose                                      |
|----------------------|----------------------------------------------|
| `M 60 8`             | Tempo (dotted-quarter = 40)                  |
| `w 18` / `w 1`       | DW-8000 wave / Triangle                      |
| `t A D S R`          | ADSR                                         |
| `J 1 K freq Q res`   | 24 dB low-pass                               |
| `N semitones cents`  | Detune                                       |
| `g rate`             | Portamento                                   |
| `L seconds`          | Attack delay                                 |
| `n midi_note`        | Pitch                                        |
| `l 1` / `l 0`        | Note on / off                                |
| `DL …` / `DD …`      | Delay + damping                              |
| `r` / `p` / `ds`     | Routing / pan / delay send                   |
| `y N`                | Select pattern                               |
| `% N`                | Pattern clock divisor                        |
| `/z pattern state`   | Start / stop pattern                         |
| `[name] : body ;`    | Macro (1–4 alpha chars only)                 |
| `[cmd] x step`       | Assign step                                  |
| `[#] x step`         | Force pattern length (trailing no-op)        |