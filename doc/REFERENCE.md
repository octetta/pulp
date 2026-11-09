# Pulp / Skred Command & Opcode Reference

Release: **0.83.0**  
Unified Opcode Dictionary and Execution Engine reference for `pulp` and `skred`.

Generated automatically by [`tools/mkopcodes`](file:///home/stewartj/book/pulp/tools/mkopcodes.c) from modular opcode definitions in `doc/opcodes/`.

---

## Architecture & System Overview

- **Total Command Words**: 305
- **Total Dispatch Clauses**: 603
- **Real-Time Audio Opcode Handlers**: 145 (audio-thread safe, zero allocation, zero string lookup)
- **Immediate / Control Plane Clauses**: 458 (REPL / parser / file system / diagnostics)

### Functional Command Groups

| Group | Identifier | Words | Description |
|:---|:---|:---:|:---|
| osc | `osc` | 21 | Oscillators, waveforms, frequency, tuning, detune |
| filter | `filter` | 7 | Filter cutoff frequency, resonance, modes, wavefolding |
| env | `env` | 15 | Envelopes, ADSR parameters, curve shaping, segment times |
| seq | `seq` | 61 | Sequencer patterns, steps, repeats, tempo, control events |
| voice | `voice` | 60 | Voice allocation, polyphony, routing, amplitude, pan |
| sys | `sys` | 141 | System configuration, MIDI, data buffers, macros, hardware |

---

## Group: `osc`

| Command | Alias | Clauses | Arity | Execution | Summary |
|:---|:---|:---:|:---:|:---:|:---|
| [`freq`](#freq) | `f` | 2 | 1 | Real-Time | set voice oscillator frequency in Hz |
| [`freq_bend`](#freq_bend) | `fb` | 1 | 1 | Real-Time | frequency bend (-1..1) |
| [`freq_bend_param`](#freq_bend_param) | `fbp` | 2 | 1..2 | Real-Time | frequency bend range in semitones |
| [`freq_feedback`](#freq_feedback) | `FB` | 1 | 1 | Real-Time | oscillator frequency feedback amount |
| [`freq_mod`](#freq_mod) | `F` | 4 | 0..3 | Real-Time | disable frequency modulation |
| [`freq_mod_mode`](#freq_mod_mode) | `FF` | 1 | 1 | Real-Time | frequency modulation routing mode |
| [`hard_sync`](#hard_sync) | `HS` | 1 | 1..6 | Real-Time | hard-sync voice links |
| [`midi_detune`](#midi_detune) | `N` | 4 | 1..2 | Real-Time | voice MIDI transposition in semitones |
| [`midi_note`](#midi_note) | `n` | 3 | 1..2 | Real-Time | set voice frequency from MIDI note number |
| [`phase_dist`](#phase_dist) | `c` | 3 | 0..2 | Real-Time | reset phase distortion |
| [`phase_mod`](#phase_mod) | `C` | 2 | 1..2 | Real-Time | phase modulation source or disable |
| [`query_wave_spectrogram`](#query_wave_spectrogram) | `?ws` | 1 | 1 | Immediate | display wavetable spectrogram (?ws alias) |
| [`wave`](#wave) | `w` | 3 | 1..3 | Real-Time | set voice oscillator wavetable index |
| [`wave_dir`](#wave_dir) | `b` | 2 | 0..1 | Real-Time | toggle waveform playback direction |
| [`wave_loop`](#wave_loop) | `B` | 2 | 0..1 | Real-Time | toggle waveform loop mode |
| [`wave_loop_count`](#wave_loop_count) | `BC` | 1 | 1 | Real-Time | set waveform loop repeat count |
| [`wave_loop_range`](#wave_loop_range) | `VL` | 2 | 0..2 | Immediate | reset waveform loop points |
| [`wave_name`](#wave_name) | `wt` | 1 | 1 | Immediate | set wavetable name |
| [`wave_param`](#wave_param) | `W*` | 1 | 2 | Immediate | query wavetable slot metadata parameter |
| [`wave_range`](#wave_range) | `VS` | 2 | 0..2 | Immediate | reset waveform sample playback range |
| [`wave_spectrogram`](#wave_spectrogram) | `WS` | 1 | 1 | Immediate | display wavetable spectrogram |

### `freq`

- **Alias**: `f`
- **Group**: `osc`
- **Clause Count**: 2

#### Clause 1: `freq(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `131` (`skode_clause_freq_set`)
- **Summary**: set voice oscillator frequency in Hz

**Description & Details**:

```text
Sets the oscillator base frequency in Hertz for the active voice.
Usage: f hz (or freq hz)
```

#### Clause 2: `freq(nan)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `132` (`skode_clause_freq_reset`)
- **Summary**: reset voice oscillator frequency to 440 Hz

**Description & Details**:

```text
Resets the oscillator base frequency to default concert A (440 Hz).
Usage: f - (or freq -)
```

---

### `freq_bend`

- **Alias**: `fb`
- **Group**: `osc`
- **Clause Count**: 1

#### Clause 1: `freq_bend(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `136` (`skode_clause_freq_bend`)
- **Summary**: frequency bend (-1..1)

**Description & Details**:

```text
Sets oscillator frequency bend.
Usage: fb amount (or freq_bend amount)
```

---

### `freq_bend_param`

- **Alias**: `fbp`
- **Group**: `osc`
- **Clause Count**: 2

#### Clause 1: `freq_bend_param(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `137` (`skode_clause_freq_bend_param_1`)
- **Summary**: frequency bend range in semitones

**Description & Details**:

```text
Sets frequency bend range in semitones with default offset 0.
Usage: fbp range (or freq_bend_param range)
```

#### Clause 2: `freq_bend_param(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `138` (`skode_clause_freq_bend_param_2`)
- **Summary**: frequency bend range and offset

**Description & Details**:

```text
Sets frequency bend range in semitones and fine offset.
Usage: fbp range offset (or freq_bend_param range offset)
```

---

### `freq_feedback`

- **Alias**: `FB`
- **Group**: `osc`
- **Clause Count**: 1

#### Clause 1: `freq_feedback(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `145` (`skode_clause_freq_feedback`)
- **Summary**: oscillator frequency feedback amount

**Description & Details**:

```text
Sets oscillator frequency self-feedback amount.
Usage: FB amount (or freq_feedback amount)
```

---

### `freq_mod`

- **Alias**: `F`
- **Group**: `osc`
- **Clause Count**: 4

#### Clause 1: `freq_mod()`

- **Arity**: `0`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `140` (`skode_clause_freq_mod_0`)
- **Summary**: disable frequency modulation

**Description & Details**:

```text
Disables frequency modulation on active voice.
Usage: F (or freq_mod)
```

#### Clause 2: `freq_mod(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `141` (`skode_clause_freq_mod_1`)
- **Summary**: frequency modulation disable or source

**Description & Details**:

```text
Disables frequency modulation if arg < 0, or sets source with depth 0.
Usage: F source
```

#### Clause 3: `freq_mod(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `142` (`skode_clause_freq_mod_2`)
- **Summary**: frequency modulation source and depth

**Description & Details**:

```text
Sets FM source voice and depth.
Usage: F source depth
```

#### Clause 4: `freq_mod(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `143` (`skode_clause_freq_mod_3`)
- **Summary**: frequency modulation source, depth and scale

**Description & Details**:

```text
Sets FM source voice, depth and scale parameter.
Usage: F source depth scale
```

---

### `freq_mod_mode`

- **Alias**: `FF`
- **Group**: `osc`
- **Clause Count**: 1

#### Clause 1: `freq_mod_mode(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `144` (`skode_clause_freq_mod_mode`)
- **Summary**: frequency modulation routing mode

**Description & Details**:

```text
Sets FM routing mode (0:linear FM, 1:phase modulation style, etc.).
Usage: FF mode (or freq_mod_mode mode)
```

---

### `hard_sync`

- **Alias**: `HS`
- **Group**: `osc`
- **Clause Count**: 1

#### Clause 1: `hard_sync(finite, finite, finite, finite, finite, finite)`

- **Arity**: `1..6`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `139` (`skode_clause_hard_sync`)
- **Summary**: hard-sync voice links

**Description & Details**:

```text
Sets oscillator hard-sync master-slave targets (1 to 6 voices). Pass -1 to clear.
Usage: HS target [target2 [target3 [target4 [target5 [target6]]]]]
```

---

### `midi_detune`

- **Alias**: `N`
- **Group**: `osc`
- **Clause Count**: 4

#### Clause 1: `midi_detune(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `156` (`skode_clause_midi_detune_1`)
- **Summary**: voice MIDI transposition in semitones

**Description & Details**:

```text
Sets voice transposition offset in semitones.
Usage: N semitones (or midi_detune semitones)
```

#### Clause 2: `midi_detune(nan)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `157` (`skode_clause_midi_detune_nan`)
- **Summary**: reset voice MIDI transposition

**Description & Details**:

```text
Resets voice transposition offset.
Usage: N -
```

#### Clause 3: `midi_detune(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `158` (`skode_clause_midi_detune_2`)
- **Summary**: voice MIDI transposition and fine detune in cents

**Description & Details**:

```text
Sets voice transposition offset in semitones and fine detune in cents.
Usage: N semitones cents
```

#### Clause 4: `midi_detune(nan, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `159` (`skode_clause_midi_detune_nan_cents`)
- **Summary**: voice MIDI default transposition and fine detune in cents

**Description & Details**:

```text
Keeps default note transposition while setting fine detune in cents.
Usage: N - cents
```

---

### `midi_note`

- **Alias**: `n`
- **Group**: `osc`
- **Clause Count**: 3

#### Clause 1: `midi_note(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `133` (`skode_clause_midi_note_1`)
- **Summary**: set voice frequency from MIDI note number

**Description & Details**:

```text
Sets the oscillator frequency from a MIDI note number (0..127).
Usage: n note (or midi_note note)
```

#### Clause 2: `midi_note(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `134` (`skode_clause_midi_note_2`)
- **Summary**: set voice frequency from MIDI note number with microtonal detune in cents

**Description & Details**:

```text
Sets the oscillator frequency from a MIDI note number plus fine detune in cents.
Usage: n note cents (or midi_note note cents)
```

#### Clause 3: `midi_note(nan)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `135` (`skode_clause_midi_note_nan`)
- **Summary**: retrigger or hold default MIDI note

**Description & Details**:

```text
Triggers or holds default MIDI note using sentinel value.
Usage: n - (or midi_note -)
```

---

### `phase_dist`

- **Alias**: `c`
- **Group**: `osc`
- **Clause Count**: 3

#### Clause 1: `phase_dist()`

- **Arity**: `0`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `146` (`skode_clause_phase_dist_0`)
- **Summary**: reset phase distortion

**Description & Details**:

```text
Resets Casio CZ-style phase distortion on active voice.
Usage: c (or phase_dist)
```

#### Clause 2: `phase_dist(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `147` (`skode_clause_phase_dist_1`)
- **Summary**: phase distortion waveform

**Description & Details**:

```text
Sets phase distortion waveform line (1..8) with 0 depth.
Usage: c line
```

#### Clause 3: `phase_dist(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `148` (`skode_clause_phase_dist_2`)
- **Summary**: phase distortion waveform and depth

**Description & Details**:

```text
Sets phase distortion waveform line and modulation depth.
Usage: c line depth
```

---

### `phase_mod`

- **Alias**: `C`
- **Group**: `osc`
- **Clause Count**: 2

#### Clause 1: `phase_mod(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `149` (`skode_clause_phase_mod_1`)
- **Summary**: phase modulation source or disable

**Description & Details**:

```text
Disables PM if source < 0, or sets source voice with 0 depth.
Usage: C source (or phase_mod source)
```

#### Clause 2: `phase_mod(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `150` (`skode_clause_phase_mod_2`)
- **Summary**: phase modulation source and depth

**Description & Details**:

```text
Sets phase modulation source voice and depth.
Usage: C source depth
```

---

### `query_wave_spectrogram`

- **Alias**: `?ws`
- **Group**: `osc`
- **Clause Count**: 1

#### Clause 1: `query_wave_spectrogram(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: display wavetable spectrogram (?ws alias)

**Description & Details**:

```text
Displays ASCII spectrogram visualization for wavetable index (?ws alias).
Usage: ?ws wave
```

---

### `wave`

- **Alias**: `w`
- **Group**: `osc`
- **Clause Count**: 3

#### Clause 1: `wave(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `128` (`skode_clause_wave_1`)
- **Summary**: set voice oscillator wavetable index

**Description & Details**:

```text
Sets the oscillator wavetable index for the selected voice. One-shot mode
follows the wave's own default (sw.one_shot). Mirrors legacy SKODE_OP_WAVE.
Usage: w index
```

#### Clause 2: `wave(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `129` (`skode_clause_wave_2`)
- **Summary**: set voice wavetable index and interpolation

**Description & Details**:

```text
Sets the oscillator wavetable index and interpolation flag (0=off, non-zero=on).
Usage: w index, interpolate
```

#### Clause 3: `wave(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `130` (`skode_clause_wave_3`)
- **Summary**: set voice wavetable index, interpolation, and one-shot override

**Description & Details**:

```text
Sets oscillator wavetable index, interpolation, and explicit one-shot override
(0=loop, non-zero=one-shot).
Usage: w index, interpolate, one_shot
```

---

### `wave_dir`

- **Alias**: `b`
- **Group**: `osc`
- **Clause Count**: 2

#### Clause 1: `wave_dir()`

- **Arity**: `0`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `151` (`skode_clause_wave_dir_0`)
- **Summary**: toggle waveform playback direction

**Description & Details**:

```text
Toggles forward/reverse playback direction.
Usage: b (or wave_dir)
```

#### Clause 2: `wave_dir(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `152` (`skode_clause_wave_dir_1`)
- **Summary**: set waveform playback direction

**Description & Details**:

```text
Sets waveform playback direction (0:forward, 1:reverse, etc.).
Usage: b dir (or wave_dir dir)
```

---

### `wave_loop`

- **Alias**: `B`
- **Group**: `osc`
- **Clause Count**: 2

#### Clause 1: `wave_loop()`

- **Arity**: `0`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `153` (`skode_clause_wave_loop_0`)
- **Summary**: toggle waveform loop mode

**Description & Details**:

```text
Toggles looping on active voice wave.
Usage: B (or wave_loop)
```

#### Clause 2: `wave_loop(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `154` (`skode_clause_wave_loop_1`)
- **Summary**: set waveform loop mode

**Description & Details**:

```text
Sets waveform loop mode (0:one-shot, 1:loop, 2:ping-pong).
Usage: B mode (or wave_loop mode)
```

---

### `wave_loop_count`

- **Alias**: `BC`
- **Group**: `osc`
- **Clause Count**: 1

#### Clause 1: `wave_loop_count(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `155` (`skode_clause_wave_loop_count`)
- **Summary**: set waveform loop repeat count

**Description & Details**:

```text
Sets maximum loop iterations before stopping.
Usage: BC count (or wave_loop_count count)
```

---

### `wave_loop_range`

- **Alias**: `VL`
- **Group**: `osc`
- **Clause Count**: 2

#### Clause 1: `wave_loop_range()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: reset waveform loop points

**Description & Details**:

```text
Resets loop points to entire sample range.
Usage: VL (or wave_loop_range)
```

#### Clause 2: `wave_loop_range(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set waveform loop points

**Description & Details**:

```text
Sets start and end loop boundary points.
Usage: VL start end (or wave_loop_range start end)
```

---

### `wave_name`

- **Alias**: `wt`
- **Group**: `osc`
- **Clause Count**: 1

#### Clause 1: `wave_name(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `string`
- **Summary**: set wavetable name

**Description & Details**:

```text
Assigns name from bracket string to wavetable slot.
Usage: [name] wt wave
```

---

### `wave_param`

- **Alias**: `W*`
- **Group**: `osc`
- **Clause Count**: 1

#### Clause 1: `wave_param(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query wavetable slot metadata parameter

**Description & Details**:

```text
Reads wavetable slot metadata parameter (0:size, 1:rate, 2:seconds, 3:loop start, 4:loop end, 5:one-shot, 6:direction).
Usage: W* wave param (or wave_param wave param)
```

---

### `wave_range`

- **Alias**: `VS`
- **Group**: `osc`
- **Clause Count**: 2

#### Clause 1: `wave_range()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: reset waveform sample playback range

**Description & Details**:

```text
Resets sample playback window to start and end of buffer.
Usage: VS (or wave_range)
```

#### Clause 2: `wave_range(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set waveform sample playback range

**Description & Details**:

```text
Sets start and end points for sample playback window.
Usage: VS start end (or wave_range start end)
```

---

### `wave_spectrogram`

- **Alias**: `WS`
- **Group**: `osc`
- **Clause Count**: 1

#### Clause 1: `wave_spectrogram(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: display wavetable spectrogram

**Description & Details**:

```text
Displays ASCII spectrogram visualization for wavetable index.
Usage: WS wave
```

---

## Group: `filter`

| Command | Alias | Clauses | Arity | Execution | Summary |
|:---|:---|:---:|:---:|:---:|:---|
| [`filter_freq`](#filter_freq) | `k` | 2 | 1 | Real-Time | set voice filter cutoff frequency in Hz |
| [`filter_freq_alt`](#filter_freq_alt) | `K` | 2 | 0..1 | Immediate | set multimode filter cutoff hz |
| [`filter_mode`](#filter_mode) | `j` | 2 | 1..2 | Real-Time | set multimode filter type (0:off, 1:lp, 2:bp, 3:hp, 4:notch) |
| [`filter_mode_alt`](#filter_mode_alt) | `J` | 3 | 0..2 | Immediate | set multimode filter response type |
| [`filter_res`](#filter_res) | `q` | 2 | 1 | Real-Time | set multimode filter resonance (0.0 to 1.0+) |
| [`filter_res_alt`](#filter_res_alt) | `Q` | 2 | 0..1 | Immediate | set multimode filter resonance |
| [`wavefold`](#wavefold) | `wf` | 4 | 1..2 | Real-Time | set voice wavefold / filter drive amount |

### `filter_freq`

- **Alias**: `k`
- **Group**: `filter`
- **Clause Count**: 2

#### Clause 1: `filter_freq(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `164` (`skode_clause_filter_freq_set`)
- **Summary**: set voice filter cutoff frequency in Hz

**Description & Details**:

```text
Sets the multimode filter cutoff frequency in Hz for the selected voice.
Usage: k hz
```

#### Clause 2: `filter_freq(nan)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `165` (`skode_clause_filter_freq_reset`)
- **Summary**: reset voice filter cutoff frequency to open (default)

**Description & Details**:

```text
Resets filter cutoff to fully open (20 kHz) when passed a NaN sentinel.
Usage: k -
```

---

### `filter_freq_alt`

- **Alias**: `K`
- **Group**: `filter`
- **Clause Count**: 2

#### Clause 1: `filter_freq_alt(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set multimode filter cutoff hz

**Description & Details**:

```text
Sets cutoff frequency for active voice filter in Hz.
Usage: K 800
```

#### Clause 2: `filter_freq_alt()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: filter cutoff hz no-op

**Description & Details**:

```text
Filter cutoff frequency no-op.
```

---

### `filter_mode`

- **Alias**: `j`
- **Group**: `filter`
- **Clause Count**: 2

#### Clause 1: `filter_mode(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `168` (`skode_clause_filter_mode_1`)
- **Summary**: set multimode filter type (0:off, 1:lp, 2:bp, 3:hp, 4:notch)

**Description & Details**:

```text
Sets filter mode type for active voice.
Usage: j mode (or filter_mode mode)
```

#### Clause 2: `filter_mode(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `169` (`skode_clause_filter_mode_2`)
- **Summary**: set multimode filter type and character

**Description & Details**:

```text
Sets filter mode type (0..4) and character model.
Usage: j mode char (or filter_mode mode char)
```

---

### `filter_mode_alt`

- **Alias**: `J`
- **Group**: `filter`
- **Clause Count**: 3

#### Clause 1: `filter_mode_alt(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set multimode filter response type

**Description & Details**:

```text
Sets filter mode (0=off, 1=lp, 2=hp, 3=bp, 4=notch, etc.).
Usage: J 1
```

#### Clause 2: `filter_mode_alt()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: filter mode no-op

**Description & Details**:

```text
Filter mode no-op.
```

#### Clause 3: `filter_mode_alt(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set filter mode and character

**Description & Details**:

```text
Sets filter mode and character model.
Usage: J 1 2
```

---

### `filter_res`

- **Alias**: `q`
- **Group**: `filter`
- **Clause Count**: 2

#### Clause 1: `filter_res(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `166` (`skode_clause_filter_res_set`)
- **Summary**: set multimode filter resonance (0.0 to 1.0+)

**Description & Details**:

```text
Sets the filter resonance / Q parameter for the active voice.
Usage: q res (or filter_res res)
```

#### Clause 2: `filter_res(nan)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `167` (`skode_clause_filter_res_reset`)
- **Summary**: reset filter resonance to zero (flat)

**Description & Details**:

```text
Resets filter resonance to 0.0 default.
Usage: q - (or filter_res -)
```

---

### `filter_res_alt`

- **Alias**: `Q`
- **Group**: `filter`
- **Clause Count**: 2

#### Clause 1: `filter_res_alt(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set multimode filter resonance

**Description & Details**:

```text
Sets filter resonance / Q parameter for active voice.
Usage: Q 0.5
```

#### Clause 2: `filter_res_alt()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: filter resonance no-op

**Description & Details**:

```text
Filter resonance no-op.
```

---

### `wavefold`

- **Alias**: `wf`
- **Group**: `filter`
- **Clause Count**: 4

#### Clause 1: `wavefold(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `160` (`skode_clause_wavefold_drive`)
- **Summary**: set voice wavefold / filter drive amount

**Description & Details**:

```text
Sets the filter overdrive/wavefolding amount for the selected voice.
Usage: wf amount
```

#### Clause 2: `wavefold(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `161` (`skode_clause_wavefold_drive_bias`)
- **Summary**: set voice wavefold drive and bias

**Description & Details**:

```text
Sets the filter overdrive and symmetry bias for the selected voice.
Usage: wf amount, bias
```

#### Clause 3: `wavefold(nan)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `162` (`skode_clause_wavefold_reset_nan`)
- **Summary**: reset voice wavefold drive to default using sentinel NaN

**Description & Details**:

```text
Resets wavefold drive to 0 (disabled) when passed a NaN sentinel (e.g. wf -).
Usage: wf -
```

#### Clause 4: `wavefold(+inf)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `163` (`skode_clause_wavefold_max_inf`)
- **Summary**: set voice wavefold drive to maximum limit using +Inf

**Description & Details**:

```text
Clamps wavefold drive to maximum saturation ceiling (+Inf).
Usage: wf inf
```

---

## Group: `env`

| Command | Alias | Clauses | Arity | Execution | Summary |
|:---|:---|:---:|:---:|:---:|:---|
| [`amp_env_mode`](#amp_env_mode) | `tm` | 1 | 1 | Real-Time | set amplitude envelope mode |
| [`amp_envelope_multi`](#amp_envelope_multi) | `te` | 2 | 0..1 | Immediate | configure multistage amp envelope from data buffer |
| [`envelope`](#envelope) | `t` | 1 | 4 | Real-Time | configure ADSR envelope parameters (attack, decay, sustain, release) |
| [`filter_env`](#filter_env) | `kt` | 1 | 4 | Real-Time | filter cutoff envelope parameters (ADSR) |
| [`filter_env_depth`](#filter_env_depth) | `kd` | 1 | 1 | Real-Time | filter cutoff envelope depth |
| [`filter_env_depth_alt`](#filter_env_depth_alt) | `fd` | 2 | 0..1 | Immediate | set filter envelope modulation depth |
| [`filter_env_multi`](#filter_env_multi) | `fte` | 2 | 0..1 | Immediate | configure multistage filter envelope from data |
| [`filter_env_multi_alt`](#filter_env_multi_alt) | `kte` | 2 | 0..1 | Immediate | configure multistage filter envelope from data (alt) |
| [`filter_env_time`](#filter_env_time) | `ft` | 3 | 0..4 | Immediate | set filter envelope adzr times |
| [`phase_env`](#phase_env) | `ct` | 1 | 4 | Real-Time | phase distortion envelope parameters (ADSR) |
| [`phase_env_depth`](#phase_env_depth) | `cd` | 1 | 1 | Real-Time | phase distortion envelope depth |
| [`pitch_env`](#pitch_env) | `pt` | 1 | 4 | Immediate | configure pitch/frequency envelope |
| [`pitch_env_depth`](#pitch_env_depth) | `pd` | 1 | 1 | Immediate | set pitch envelope depth |
| [`pitch_env_multi`](#pitch_env_multi) | `pte` | 1 | 0 | Immediate | configure multistage pitch envelope |
| [`trigger`](#trigger) | `T` | 1 | 0 | Real-Time | trigger voice envelope at full velocity (1.0) |

### `amp_env_mode`

- **Alias**: `tm`
- **Group**: `env`
- **Clause Count**: 1

#### Clause 1: `amp_env_mode(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `176` (`skode_clause_amp_env_mode`)
- **Summary**: set amplitude envelope mode

**Description & Details**:

```text
Configures amplitude envelope generator mode for active voice.
Usage: tm mode
```

---

### `amp_envelope_multi`

- **Alias**: `te`
- **Group**: `env`
- **Clause Count**: 2

#### Clause 1: `amp_envelope_multi()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: configure multistage amp envelope from data buffer

**Description & Details**:

```text
Configures multistage amplitude envelope from data buffer.
Usage: (0 0 100 1 50 .5) te
```

#### Clause 2: `amp_envelope_multi(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: configure multistage amp envelope with argument

**Description & Details**:

```text
Configures multistage amplitude envelope with argument.
```

---

### `envelope`

- **Alias**: `t`
- **Group**: `env`
- **Clause Count**: 1

#### Clause 1: `envelope(finite, finite, finite, finite)`

- **Arity**: `4`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `170` (`skode_clause_env_set`)
- **Summary**: configure ADSR envelope parameters (attack, decay, sustain, release)

**Description & Details**:

```text
Sets the attack time, decay time, sustain level, and release time for the active voice envelope.
Usage: t a d s r (or envelope a d s r)
```

---

### `filter_env`

- **Alias**: `kt`
- **Group**: `env`
- **Clause Count**: 1

#### Clause 1: `filter_env(finite, finite, finite, finite)`

- **Arity**: `4`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `174` (`skode_clause_filter_env`)
- **Summary**: filter cutoff envelope parameters (ADSR)

**Description & Details**:

```text
Configures filter cutoff envelope (attack, decay, sustain, release).
Usage: kt a d s r (or filter_env a d s r)
```

---

### `filter_env_depth`

- **Alias**: `kd`
- **Group**: `env`
- **Clause Count**: 1

#### Clause 1: `filter_env_depth(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `175` (`skode_clause_filter_env_depth`)
- **Summary**: filter cutoff envelope depth

**Description & Details**:

```text
Sets filter cutoff envelope modulation depth in octaves.
Usage: kd depth (or filter_env_depth depth)
```

---

### `filter_env_depth_alt`

- **Alias**: `fd`
- **Group**: `env`
- **Clause Count**: 2

#### Clause 1: `filter_env_depth_alt(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set filter envelope modulation depth

**Description & Details**:

```text
Sets modulation depth of the filter envelope on cutoff.
Usage: fd 2000
```

#### Clause 2: `filter_env_depth_alt()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: filter envelope modulation depth no-op

**Description & Details**:

```text
Filter envelope depth no-op.
```

---

### `filter_env_multi`

- **Alias**: `fte`
- **Group**: `env`
- **Clause Count**: 2

#### Clause 1: `filter_env_multi()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: configure multistage filter envelope from data

**Description & Details**:

```text
Configures multistage filter envelope from data buffer.
Usage: (0 0 100 1 50 .5) fte
```

#### Clause 2: `filter_env_multi(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: configure multistage filter envelope with argument

**Description & Details**:

```text
Configures multistage filter envelope with argument.
```

---

### `filter_env_multi_alt`

- **Alias**: `kte`
- **Group**: `env`
- **Clause Count**: 2

#### Clause 1: `filter_env_multi_alt()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: configure multistage filter envelope from data (alt)

**Description & Details**:

```text
Configures multistage filter envelope from data buffer.
Usage: (0 0 100 1 50 .5) kte
```

#### Clause 2: `filter_env_multi_alt(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: configure multistage filter envelope with argument

**Description & Details**:

```text
Configures multistage filter envelope with argument.
```

---

### `filter_env_time`

- **Alias**: `ft`
- **Group**: `env`
- **Clause Count**: 3

#### Clause 1: `filter_env_time(finite, finite, finite, finite)`

- **Arity**: `4`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set filter envelope adzr times

**Description & Details**:

```text
Sets filter envelope attack, decay, sustain, release times.
Usage: ft 10 50 1 100
```

#### Clause 2: `filter_env_time()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: filter envelope time no-op

**Description & Details**:

```text
Filter envelope time no-op.
```

#### Clause 3: `filter_env_time(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: filter envelope time with single argument

**Description & Details**:

```text
Filter envelope time with single argument.
```

---

### `phase_env`

- **Alias**: `ct`
- **Group**: `env`
- **Clause Count**: 1

#### Clause 1: `phase_env(finite, finite, finite, finite)`

- **Arity**: `4`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `172` (`skode_clause_phase_env`)
- **Summary**: phase distortion envelope parameters (ADSR)

**Description & Details**:

```text
Configures Casio CZ phase envelope (attack, decay, sustain, release).
Usage: ct a d s r (or phase_env a d s r)
```

---

### `phase_env_depth`

- **Alias**: `cd`
- **Group**: `env`
- **Clause Count**: 1

#### Clause 1: `phase_env_depth(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `173` (`skode_clause_phase_env_depth`)
- **Summary**: phase distortion envelope depth

**Description & Details**:

```text
Sets phase distortion envelope modulation depth (-1.0 to 1.0).
Usage: cd depth (or phase_env_depth depth)
```

---

### `pitch_env`

- **Alias**: `pt`
- **Group**: `env`
- **Clause Count**: 1

#### Clause 1: `pitch_env(finite, finite, finite, finite)`

- **Arity**: `4`
- **Execution Model**: `immediate (control plane)`
- **Summary**: configure pitch/frequency envelope

**Description & Details**:

```text
Configures the 4-stage ADSR pitch envelope for the selected voice.
Usage: pt a,d,s,r
```

---

### `pitch_env_depth`

- **Alias**: `pd`
- **Group**: `env`
- **Clause Count**: 1

#### Clause 1: `pitch_env_depth(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set pitch envelope depth

**Description & Details**:

```text
Sets modulation depth for voice pitch/frequency envelope.
Usage: pd depth
```

---

### `pitch_env_multi`

- **Alias**: `pte`
- **Group**: `env`
- **Clause Count**: 1

#### Clause 1: `pitch_env_multi()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `data`
- **Summary**: configure multistage pitch envelope

**Description & Details**:

```text
Loads multistage pitch envelope breakpoints from active data buffer.
Usage: pte
```

---

### `trigger`

- **Alias**: `T`
- **Group**: `env`
- **Clause Count**: 1

#### Clause 1: `trigger()`

- **Arity**: `0`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `171` (`skode_clause_trigger_full`)
- **Summary**: trigger voice envelope at full velocity (1.0)

**Description & Details**:

```text
Triggers the voice amplitude envelope and linked voices at velocity 1.0.
Usage: T (or trigger)
```

---

## Group: `seq`

| Command | Alias | Clauses | Arity | Execution | Summary |
|:---|:---|:---:|:---:|:---:|:---|
| [`bpm`](#bpm) | `M` | 2 | 1..2 | Real-Time | set sequencer tempo in beats per minute |
| [`control_event`](#control_event) | `ce` | 4 | 1..4 | Real-Time | emit control event with 1 argument |
| [`control_event_bind`](#control_event_bind) | `/ceb` | 1 | 2 | Immediate | bind control event to Skode command |
| [`control_event_bind_external`](#control_event_bind_external) | `/cex` | 2 | 0..3 | Immediate | bind external string slot to control event responder |
| [`control_event_bind_fire`](#control_event_bind_fire) | `/ce!` | 2 | 0..2 | Immediate | clear all control event bindings |
| [`control_event_bind_query`](#control_event_bind_query) | `/ce?` | 1 | 0 | Immediate | query control event bindings |
| [`control_event_query`](#control_event_query) | `?ce` | 1 | 0 | Immediate | query control event ring buffer |
| [`control_event_query_detail`](#control_event_query_detail) | `?ce!` | 1 | 0 | Immediate | drain and print all control events |
| [`control_event_remove`](#control_event_remove) | `/cer` | 2 | 0..1 | Immediate | query control event responder status |
| [`defer`](#defer) | `+` | 1 | 1 | Immediate | defer execution by relative seconds |
| [`opcode_queue_query`](#opcode_queue_query) | `?o` | 3 | 0..2 | Immediate | query opcode queue entries |
| [`pattern_all`](#pattern_all) | `Z` | 2 | 0..1 | Immediate | display all pattern programs and tempo |
| [`pattern_all_data`](#pattern_all_data) | `Z>` | 2 | 0..1 | Immediate | apply playback state to patterns in data array |
| [`pattern_all_query`](#pattern_all_query) | `Z?` | 1 | 0 | Immediate | query all patterns summary |
| [`pattern_events`](#pattern_events) | `yc` | 1 | 1 | Immediate | enable or disable control events on active pattern |
| [`pattern_goto`](#pattern_goto) | `zg` | 1 | 1 | Immediate | jump active pattern playback step |
| [`pattern_goto_alt`](#pattern_goto_alt) | `-j` | 1 | 1 | Real-Time | jump active pattern playback step (-j alias) |
| [`pattern_loop`](#pattern_loop) | `/zl` | 1 | 3 | Real-Time | pattern step loop counter and branch |
| [`pattern_master`](#pattern_master) | `yp` | 2 | 0..1 | Immediate | query master pattern |
| [`pattern_modulo`](#pattern_modulo) | `%` | 1 | 1 | Immediate | set active pattern step loop modulo |
| [`pattern_modulo_alt`](#pattern_modulo_alt) | `z%` | 1 | 1 | Real-Time | set active pattern step loop modulo (z% alias) |
| [`pattern_mute`](#pattern_mute) | `ym` | 1 | 1 | Immediate | mute or unmute pattern |
| [`pattern_queue`](#pattern_queue) | `zq` | 1 | 1 | Immediate | queue pattern start or stop at boundary |
| [`pattern_ratchet`](#pattern_ratchet) | `z*` | 1 | 1 | Real-Time | set step ratchet multiplier |
| [`pattern_reset`](#pattern_reset) | `Y` | 1 | 1 | Immediate | reset pattern steps and length |
| [`pattern_rewind`](#pattern_rewind) | `Z!` | 1 | 0 | Immediate | rewind all patterns to step 0 |
| [`pattern_select`](#pattern_select) | `y` | 2 | 0..1 | Immediate | select active pattern |
| [`pattern_show`](#pattern_show) | `ys` | 2 | 0..1 | Immediate | display active pattern steps and status |
| [`pattern_state`](#pattern_state) | `z` | 2 | 0..1 | Immediate | show active pattern state |
| [`pattern_state_query`](#pattern_state_query) | `z?` | 1 | 0 | Immediate | query active pattern play state |
| [`pattern_state_query_all`](#pattern_state_query_all) | `z??` | 1 | 0 | Immediate | query play state of all patterns |
| [`pattern_state_set`](#pattern_state_set) | `/z` | 1 | 2 | Real-Time | set pattern state by explicit index |
| [`pattern_step_query`](#pattern_step_query) | `ys?` | 1 | 0 | Immediate | query pattern step details |
| [`pattern_text`](#pattern_text) | `yt` | 2 | 0..1 | Immediate | set pattern text label from active string |
| [`queue_query`](#queue_query) | `?q` | 1 | 0 | Immediate | query scheduler event queue |
| [`record_gate`](#record_gate) | `/rg` | 3 | 0..2 | Immediate | arm audio recording with gate threshold |
| [`record_pause`](#record_pause) | `^r` | 1 | 0 | Immediate | pause recorded audio playback |
| [`record_play`](#record_play) | `>r` | 1 | 0 | Immediate | play recorded audio buffer |
| [`record_query`](#record_query) | `/r?` | 1 | 0 | Immediate | query audio recorder status |
| [`record_rewind`](#record_rewind) | `<r` | 4 | 0..3 | Immediate | query audio recording state |
| [`record_stop`](#record_stop) | `/rs` | 1 | 0 | Immediate | stop audio recording |
| [`record_toggle`](#record_toggle) | `/r` | 4 | 0..3 | Immediate | load recording into default wave slot |
| [`record_wait`](#record_wait) | `/rw` | 2 | 0..1 | Immediate | record wait / arm |
| [`repeat`](#repeat) | `R` | 2 | 2..3 | Immediate | repeat scheduled program at seconds interval |
| [`repeat_kill`](#repeat_kill) | `R!` | 1 | 1 | Immediate | kill scheduled events by tag |
| [`repeat_kill_all`](#repeat_kill_all) | `R!!` | 1 | 0 | Immediate | kill all scheduled sequencer events |
| [`repeat_macro`](#repeat_macro) | `eR` | 1 | 1 | Immediate | repeat user macro at seconds interval |
| [`repeat_macro_ticks`](#repeat_macro_ticks) | `eRR` | 1 | 1 | Immediate | repeat user macro at beat subdivision interval |
| [`repeat_ticks`](#repeat_ticks) | `RR` | 2 | 2..3 | Immediate | repeat scheduled program at beat subdivision interval |
| [`seq_dequeue`](#seq_dequeue) | `DQ` | 2 | 0..1 | Immediate | clear all scheduled events |
| [`seq_do_query`](#seq_do_query) | `DO?` | 1 | 0 | Immediate | query deferred execution queue |
| [`step_all_data`](#step_all_data) | `X>` | 2 | 0..1 | Immediate | assign step program to steps in data array on active pattern |
| [`step_append`](#step_append) | `xa` | 1 | 0 | Immediate | append bracketed program step to active pattern |
| [`step_get`](#step_get) | `xg` | 1 | 1 | Immediate | get pattern step source string |
| [`step_goto`](#step_goto) | `>x` | 1 | 1 | Immediate | jump active pattern to target step |
| [`step_read`](#step_read) | `<x` | 1 | 1 | Immediate | read pattern step source into parser string buffer |
| [`stream_copy`](#stream_copy) | `/SC` | 1 | 2 | Real-Time | copy stream contents |
| [`stream_mode`](#stream_mode) | `/SM` | 1 | 2 | Real-Time | set stream playback mode |
| [`stream_pos`](#stream_pos) | `/SP` | 1 | 2 | Real-Time | set stream read position |
| [`stream_set`](#stream_set) | `/SS` | 1 | 1 | Immediate | set stream array from stack data |
| [`stream_show`](#stream_show) | `?S` | 2 | 0..1 | Immediate | show stream state and data |

### `bpm`

- **Alias**: `M`
- **Group**: `seq`
- **Clause Count**: 2

#### Clause 1: `bpm(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `177` (`skode_clause_tempo_bpm`)
- **Summary**: set sequencer tempo in beats per minute

**Description & Details**:

```text
Sets the master sequencer tempo in BPM.
Usage: M bpm (or bpm bpm)
```

#### Clause 2: `bpm(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `178` (`skode_clause_tempo_bpm_sub`)
- **Summary**: set sequencer tempo in BPM with step subdivision

**Description & Details**:

```text
Sets the sequencer tempo in BPM and step subdivision (e.g. 16 for 16th notes).
Usage: M bpm sub (or bpm bpm sub)
```

---

### `control_event`

- **Alias**: `ce`
- **Group**: `seq`
- **Clause Count**: 4

#### Clause 1: `control_event(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `179` (`skode_clause_control_event_1`)
- **Summary**: emit control event with 1 argument

**Description & Details**:

```text
Emits control event.
Usage: ce id (or control_event id)
```

#### Clause 2: `control_event(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `180` (`skode_clause_control_event_2`)
- **Summary**: emit control event with 2 arguments

**Description & Details**:

```text
Emits control event with parameter.
Usage: ce id p1
```

#### Clause 3: `control_event(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `181` (`skode_clause_control_event_3`)
- **Summary**: emit control event with 3 arguments

**Description & Details**:

```text
Emits control event with 2 parameters.
Usage: ce id p1 p2
```

#### Clause 4: `control_event(finite, finite, finite, finite)`

- **Arity**: `4`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `182` (`skode_clause_control_event_4`)
- **Summary**: emit control event with 4 arguments

**Description & Details**:

```text
Emits control event with 3 parameters.
Usage: ce id p1 p2 p3
```

---

### `control_event_bind`

- **Alias**: `/ceb`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `control_event_bind(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: bind control event to Skode command

**Description & Details**:

```text
Binds control event (type, id) to execute Skode code string.
Usage: [command] /ceb type id
```

---

### `control_event_bind_external`

- **Alias**: `/cex`
- **Group**: `seq`
- **Clause Count**: 2

#### Clause 1: `control_event_bind_external(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: bind external string slot to control event responder

**Description & Details**:

```text
Binds external string buffer slot to control event type and key.
Usage: /cex slot,type,key
```

#### Clause 2: `control_event_bind_external()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: control event bind external usage

**Description & Details**:

```text
Prints usage for /cex.
Usage: /cex
```

---

### `control_event_bind_fire`

- **Alias**: `/ce!`
- **Group**: `seq`
- **Clause Count**: 2

#### Clause 1: `control_event_bind_fire()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: clear all control event bindings

**Description & Details**:

```text
Clears all installed control event bindings.
Usage: /ce!
```

#### Clause 2: `control_event_bind_fire(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: remove control event binding

**Description & Details**:

```text
Removes control event binding matching type and id.
Usage: /ce! type id
```

---

### `control_event_bind_query`

- **Alias**: `/ce?`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `control_event_bind_query()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query control event bindings

**Description & Details**:

```text
Prints installed control event bindings table.
Usage: /ce?
```

---

### `control_event_query`

- **Alias**: `?ce`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `control_event_query()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query control event ring buffer

**Description & Details**:

```text
Prints pending control events from dispatcher ring buffer.
Usage: ?ce
```

---

### `control_event_query_detail`

- **Alias**: `?ce!`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `control_event_query_detail()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: drain and print all control events

**Description & Details**:

```text
Drains and displays all pending control events with full parameters.
Usage: ?ce!
```

---

### `control_event_remove`

- **Alias**: `/cer`
- **Group**: `seq`
- **Clause Count**: 2

#### Clause 1: `control_event_remove()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query control event responder status

**Description & Details**:

```text
Queries control event responder enabled status.
Usage: /cer
```

#### Clause 2: `control_event_remove(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set control event responder enabled

**Description & Details**:

```text
Enables or disables control event responder.
Usage: /cer 0|1
```

---

### `defer`

- **Alias**: `+`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `defer(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: defer execution by relative seconds

**Description & Details**:

```text
Defers execution of the trailing command sequence by relative seconds.
Usage: +seconds [command]
```

---

### `opcode_queue_query`

- **Alias**: `?o`
- **Group**: `seq`
- **Clause Count**: 3

#### Clause 1: `opcode_queue_query()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query opcode queue entries

**Description & Details**:

```text
Displays detailed opcode queue state and timing.
Usage: ?o
```

#### Clause 2: `opcode_queue_query(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query opcode pattern entries

**Description & Details**:

```text
Displays pattern opcode entries.
Usage: ?o pattern
```

#### Clause 3: `opcode_queue_query(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query opcode pattern and step entries

**Description & Details**:

```text
Displays pattern and step opcode entries.
Usage: ?o pattern, step
```

---

### `pattern_all`

- **Alias**: `Z`
- **Group**: `seq`
- **Clause Count**: 2

#### Clause 1: `pattern_all()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: display all pattern programs and tempo

**Description & Details**:

```text
Prints tempo and step program listings for all non-empty patterns.
Usage: Z (or pattern_all)
```

#### Clause 2: `pattern_all(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set playback state across all patterns

**Description & Details**:

```text
Sets global playback state for all patterns (0:stop, 1:play).
Usage: Z state (or pattern_all state)
```

---

### `pattern_all_data`

- **Alias**: `Z>`
- **Group**: `seq`
- **Clause Count**: 2

#### Clause 1: `pattern_all_data()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `data`
- **Summary**: apply playback state to patterns in data array

**Description & Details**:

```text
Starts playback for all patterns listed in the active data array atomically under lock.
Usage: (p1 p2 ...) Z>
```

#### Clause 2: `pattern_all_data(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `data`
- **Summary**: apply explicit playback state to patterns in data array

**Description & Details**:

```text
Sets playback state (0:stop, 1:play, 2:pause, 3:resume) for all patterns listed in the active data array atomically under lock.
Usage: (p1 p2 ...) Z> state
```

---

### `pattern_all_query`

- **Alias**: `Z?`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `pattern_all_query()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query all patterns summary

**Description & Details**:

```text
Prints comprehensive summary of all defined patterns.
Usage: Z?
```

---

### `pattern_events`

- **Alias**: `yc`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `pattern_events(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: enable or disable control events on active pattern

**Description & Details**:

```text
Sets control event generation for active pattern (0:disable, 1:enable).
Usage: yc state (or pattern_events state)
```

---

### `pattern_goto`

- **Alias**: `zg`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `pattern_goto(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: jump active pattern playback step

**Description & Details**:

```text
Sets the next playback step position for the active pattern.
Usage: zg step (or pattern_goto step)
```

---

### `pattern_goto_alt`

- **Alias**: `-j`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `pattern_goto_alt(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `183` (`skode_clause_pattern_goto_alt`)
- **Summary**: jump active pattern playback step (-j alias)

**Description & Details**:

```text
Sets the next playback step position for the active pattern (-j alias).
Usage: -j step
```

---

### `pattern_loop`

- **Alias**: `/zl`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `pattern_loop(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `189` (`skode_clause_pattern_loop_c164`)
- **Summary**: pattern step loop counter and branch

**Description & Details**:

```text
Loops pattern step execution using a variable counter until limit is reached.
Usage: /zl var_id limit dest_step
```

---

### `pattern_master`

- **Alias**: `yp`
- **Group**: `seq`
- **Clause Count**: 2

#### Clause 1: `pattern_master()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query master pattern

**Description & Details**:

```text
Prints the current master pattern index.
Usage: yp (or pattern_master)
```

#### Clause 2: `pattern_master(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set master pattern for pattern synchronization

**Description & Details**:

```text
Sets master sync pattern. Pass -1 to disable.
Usage: yp pattern (or pattern_master pattern)
```

---

### `pattern_modulo`

- **Alias**: `%`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `pattern_modulo(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set active pattern step loop modulo

**Description & Details**:

```text
Sets loop length modulo for the active pattern.
Usage: % count (or pattern_modulo count)
```

---

### `pattern_modulo_alt`

- **Alias**: `z%`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `pattern_modulo_alt(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `184` (`skode_clause_pattern_modulo_alt`)
- **Summary**: set active pattern step loop modulo (z% alias)

**Description & Details**:

```text
Sets loop length modulo for the active pattern (z% alias).
Usage: z% count
```

---

### `pattern_mute`

- **Alias**: `ym`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `pattern_mute(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: mute or unmute pattern

**Description & Details**:

```text
Sets pattern mute state (0:unmute, 1:mute).
Usage: ym state (or pattern_mute state)
```

---

### `pattern_queue`

- **Alias**: `zq`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `pattern_queue(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: queue pattern start or stop at boundary

**Description & Details**:

```text
Queues active pattern state change at next bar boundary.
Usage: zq mode (or pattern_queue mode)
```

---

### `pattern_ratchet`

- **Alias**: `z*`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `pattern_ratchet(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `190` (`skode_clause_pattern_ratchet_c165`)
- **Summary**: set step ratchet multiplier

**Description & Details**:

```text
Sets sub-step ratchet multiplier for pattern step.
Usage: z* multiplier (or * multiplier)
```

---

### `pattern_reset`

- **Alias**: `Y`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `pattern_reset(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: reset pattern steps and length

**Description & Details**:

```text
Clears all steps in target pattern.
Usage: Y pattern (or pattern_reset pattern)
```

---

### `pattern_rewind`

- **Alias**: `Z!`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `pattern_rewind()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: rewind all patterns to step 0

**Description & Details**:

```text
Resets playheads of all active patterns back to step 0.
Usage: Z! (or pattern_rewind)
```

---

### `pattern_select`

- **Alias**: `y`
- **Group**: `seq`
- **Clause Count**: 2

#### Clause 1: `pattern_select(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: select active pattern

**Description & Details**:

```text
Sets active pattern index in context.
Usage: y pattern (or pattern_select pattern)
```

#### Clause 2: `pattern_select()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: pattern select no-op with 0 args

**Description & Details**:

```text
Pattern select no-op when no args provided on stack.
```

---

### `pattern_show`

- **Alias**: `ys`
- **Group**: `seq`
- **Clause Count**: 2

#### Clause 1: `pattern_show()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: display active pattern steps and status

**Description & Details**:

```text
Prints step program listings for active pattern.
Usage: ys (or pattern_show)
```

#### Clause 2: `pattern_show(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: display target pattern steps and status

**Description & Details**:

```text
Prints step program listings for target pattern.
Usage: ys pattern (or pattern_show pattern)
```

---

### `pattern_state`

- **Alias**: `z`
- **Group**: `seq`
- **Clause Count**: 2

#### Clause 1: `pattern_state()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: show active pattern state

**Description & Details**:

```text
Prints step program listings for active pattern.
Usage: z (or pattern_state)
```

#### Clause 2: `pattern_state(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set active pattern playback state

**Description & Details**:

```text
Sets pattern playback state (0:stop, 1:play, 2:solo).
Usage: z state (or pattern_state state)
```

---

### `pattern_state_query`

- **Alias**: `z?`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `pattern_state_query()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query active pattern play state

**Description & Details**:

```text
Prints play/stop state and position for active pattern.
Usage: z?
```

---

### `pattern_state_query_all`

- **Alias**: `z??`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `pattern_state_query_all()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query play state of all patterns

**Description & Details**:

```text
Prints play state of all pattern slots.
Usage: z??
```

---

### `pattern_state_set`

- **Alias**: `/z`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `pattern_state_set(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `188` (`skode_clause_pattern_state_set_c163`)
- **Summary**: set pattern state by explicit index

**Description & Details**:

```text
Sets pattern playback state for explicit pattern index (pattern, state).
Usage: /z pattern state
```

---

### `pattern_step_query`

- **Alias**: `ys?`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `pattern_step_query()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query pattern step details

**Description & Details**:

```text
Prints step count and details for active pattern.
Usage: ys?
```

---

### `pattern_text`

- **Alias**: `yt`
- **Group**: `seq`
- **Clause Count**: 2

#### Clause 1: `pattern_text()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `string`
- **Summary**: set pattern text label from active string

**Description & Details**:

```text
Configures text label for the active pattern from the preceding parser string.
Usage: [label] yt
```

#### Clause 2: `pattern_text(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set pattern text label (optional argument)

**Description & Details**:

```text
Configures text label for the active pattern with an optional numeric parameter.
Usage: [label] yt num
```

---

### `queue_query`

- **Alias**: `?q`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `queue_query()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query scheduler event queue

**Description & Details**:

```text
Displays contents of realtime event scheduler queue.
Usage: ?q
```

---

### `record_gate`

- **Alias**: `/rg`
- **Group**: `seq`
- **Clause Count**: 3

#### Clause 1: `record_gate()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: arm audio recording with gate threshold

**Description & Details**:

```text
Arms audio recorder with default gate threshold.
Usage: /rg
```

#### Clause 2: `record_gate(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: arm audio recording with threshold

**Description & Details**:

```text
Arms audio recorder with specified dB gate threshold.
Usage: /rg threshold
```

#### Clause 3: `record_gate(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: arm audio recording with threshold and duration

**Description & Details**:

```text
Arms audio recorder with gate threshold and maximum duration.
Usage: /rg threshold duration
```

---

### `record_pause`

- **Alias**: `^r`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `record_pause()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: pause recorded audio playback

**Description & Details**:

```text
Pauses playback of recorded audio buffer.
Usage: ^r
```

---

### `record_play`

- **Alias**: `>r`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `record_play()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: play recorded audio buffer

**Description & Details**:

```text
Plays recorded audio buffer through output.
Usage: >r
```

---

### `record_query`

- **Alias**: `/r?`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `record_query()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query audio recorder status

**Description & Details**:

```text
Prints audio recording state, capacity, and captured length.
Usage: /r?
```

---

### `record_rewind`

- **Alias**: `<r`
- **Group**: `seq`
- **Clause Count**: 4

#### Clause 1: `record_rewind()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query audio recording state

**Description & Details**:

```text
Prints audio recording state, capacity, and remaining frames.
Usage: <r
```

#### Clause 2: `record_rewind(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: record audio for duration

**Description & Details**:

```text
Records dry mix audio for duration in seconds.
Usage: <r seconds
```

#### Clause 3: `record_rewind(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: record audio with source

**Description & Details**:

```text
Records audio with source selection (0=dry, 1=voice, 2=master).
Usage: <r seconds source
```

#### Clause 4: `record_rewind(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: record audio from voice

**Description & Details**:

```text
Records audio from voice (source=1).
Usage: <r seconds,1,voice
```

---

### `record_stop`

- **Alias**: `/rs`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `record_stop()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: stop audio recording

**Description & Details**:

```text
Stops active audio recording.
Usage: /rs
```

---

### `record_toggle`

- **Alias**: `/r`
- **Group**: `seq`
- **Clause Count**: 4

#### Clause 1: `record_toggle()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load recording into default wave slot

**Description & Details**:

```text
Loads recorded audio buffer into default sample slot.
Usage: /r
```

#### Clause 2: `record_toggle(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load recording into slot

**Description & Details**:

```text
Loads recorded audio buffer into wavetable slot.
Usage: /r slot
```

#### Clause 3: `record_toggle(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load recording into slot with mode

**Description & Details**:

```text
Loads recorded audio buffer into slot with mode (0=cycle, 1=one-shot).
Usage: /r slot mode
```

#### Clause 4: `record_toggle(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load recording into slot with mode and channel

**Description & Details**:

```text
Loads recorded audio buffer into slot with mode and channel (-1=stereo avg, 0=left, 1=right).
Usage: /r slot mode channel
```

---

### `record_wait`

- **Alias**: `/rw`
- **Group**: `seq`
- **Clause Count**: 2

#### Clause 1: `record_wait()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: record wait / arm

**Description & Details**:

```text
Arms recorder waiting for next audio trigger.
Usage: /rw
```

#### Clause 2: `record_wait(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: record wait with duration

**Description & Details**:

```text
Arms recorder waiting for trigger with duration limit.
Usage: /rw duration
```

---

### `repeat`

- **Alias**: `R`
- **Group**: `seq`
- **Clause Count**: 2

#### Clause 1: `repeat(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `string`
- **Summary**: repeat scheduled program at seconds interval

**Description & Details**:

```text
Repeats currently bracketed program N times at seconds interval.
Usage: [program] count interval R
```

#### Clause 2: `repeat(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `string`
- **Summary**: repeat scheduled program at seconds interval with tag

**Description & Details**:

```text
Repeats currently bracketed program N times at seconds interval with event tag.
Usage: [program] count interval tag R
```

---

### `repeat_kill`

- **Alias**: `R!`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `repeat_kill(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: kill scheduled events by tag

**Description & Details**:

```text
Removes all scheduled events from the queue matching the given tag number.
Usage: R! tag (or repeat_kill tag)
```

---

### `repeat_kill_all`

- **Alias**: `R!!`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `repeat_kill_all()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: kill all scheduled sequencer events

**Description & Details**:

```text
Clears the entire deferred and repeated event queue.
Usage: R!! (or repeat_kill_all)
```

---

### `repeat_macro`

- **Alias**: `eR`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `repeat_macro(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: repeat user macro at seconds interval

**Description & Details**:

```text
Repeats user macro at seconds interval.
Usage: macro_id count interval eR
```

---

### `repeat_macro_ticks`

- **Alias**: `eRR`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `repeat_macro_ticks(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: repeat user macro at beat subdivision interval

**Description & Details**:

```text
Repeats user macro at beat subdivision interval.
Usage: macro_id count beats eRR
```

---

### `repeat_ticks`

- **Alias**: `RR`
- **Group**: `seq`
- **Clause Count**: 2

#### Clause 1: `repeat_ticks(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `string`
- **Summary**: repeat scheduled program at beat subdivision interval

**Description & Details**:

```text
Repeats bracketed program N times at musical beat subdivision interval.
Usage: [program] count beats RR
```

#### Clause 2: `repeat_ticks(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `string`
- **Summary**: repeat scheduled program at beat subdivision interval with tag

**Description & Details**:

```text
Repeats bracketed program N times at musical beat subdivision interval with event tag.
Usage: [program] count beats tag RR
```

---

### `seq_dequeue`

- **Alias**: `DQ`
- **Group**: `seq`
- **Clause Count**: 2

#### Clause 1: `seq_dequeue()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: clear all scheduled events

**Description & Details**:

```text
Removes all scheduled events from the queue.
Usage: DQ
```

#### Clause 2: `seq_dequeue(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: dequeue scheduled event by tag

**Description & Details**:

```text
Removes scheduled events matching tag from queue.
Usage: DQ tag
```

---

### `seq_do_query`

- **Alias**: `DO?`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `seq_do_query()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query deferred execution queue

**Description & Details**:

```text
Prints status of deferred do-queue entries.
Usage: DO?
```

---

### `step_all_data`

- **Alias**: `X>`
- **Group**: `seq`
- **Clause Count**: 2

#### Clause 1: `step_all_data()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `string`, `data`
- **Summary**: assign step program to steps in data array on active pattern

**Description & Details**:

```text
Assigns the compiled step program from the string buffer to all step indices listed in the active data array for the current active pattern.
Usage: [v8 g&4 n&3 l&5] ( 0 2 3 5 6 8 10 11 ... ) X>
   or: ( 0 2 3 5 6 8 10 11 ... ) [v8 g&4 n&3 l&5] X>
```

#### Clause 2: `step_all_data(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `string`, `data`
- **Summary**: assign step program to steps in data array on explicit pattern

**Description & Details**:

```text
Assigns the compiled step program from the string buffer to all step indices listed in the active data array for the specified pattern.
Usage: [v8 g&4 n&3 l&5] ( 0 2 3 5 ... ) 5 X>
   or: 5 [v8 g&4 n&3 l&5] ( 0 2 3 5 ... ) X>
```

---

### `step_append`

- **Alias**: `xa`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `step_append()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `string`
- **Summary**: append bracketed program step to active pattern

**Description & Details**:

```text
Compiles the currently bracketed program string and appends as a step to the active pattern.
Usage: [program] xa (or [program] step_append)
```

---

### `step_get`

- **Alias**: `xg`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `step_get(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: get pattern step source string

**Description & Details**:

```text
Loads source program string from pattern step into parser string buffer.
Usage: xg step
```

---

### `step_goto`

- **Alias**: `>x`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `step_goto(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: jump active pattern to target step

**Description & Details**:

```text
Sets active pattern playhead to step position.
Usage: >x step (or step_goto step)
```

---

### `step_read`

- **Alias**: `<x`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `step_read(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `string`
- **Summary**: read pattern step source into parser string buffer

**Description & Details**:

```text
Retrieves the step source string from active pattern into the string buffer.
Usage: <x step (or step_read step)
```

---

### `stream_copy`

- **Alias**: `/SC`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `stream_copy(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `185` (`skode_clause_stream_copy_c160`)
- **Summary**: copy stream contents

**Description & Details**:

```text
Copies contents from source stream to destination stream.
Usage: /SC dst src
```

---

### `stream_mode`

- **Alias**: `/SM`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `stream_mode(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `186` (`skode_clause_stream_mode_c161`)
- **Summary**: set stream playback mode

**Description & Details**:

```text
Sets stream playback mode (0=wrap fwd, 1=wrap rev, 2=pingpong, 3=clamp).
Usage: /SM stream_idx mode
```

---

### `stream_pos`

- **Alias**: `/SP`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `stream_pos(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `187` (`skode_clause_stream_pos_c162`)
- **Summary**: set stream read position

**Description & Details**:

```text
Sets current read position in stream array.
Usage: /SP stream_idx pos
```

---

### `stream_set`

- **Alias**: `/SS`
- **Group**: `seq`
- **Clause Count**: 1

#### Clause 1: `stream_set(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `data`
- **Summary**: set stream array from stack data

**Description & Details**:

```text
Populates a stream array with values parsed from stack data.
Usage: ( 1 2 3 ) /SS stream_idx
```

---

### `stream_show`

- **Alias**: `?S`
- **Group**: `seq`
- **Clause Count**: 2

#### Clause 1: `stream_show()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: show stream state and data

**Description & Details**:

```text
Displays stream contents and modes.
Usage: ?S [stream_id]
```

#### Clause 2: `stream_show(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: show specific stream state and data

**Description & Details**:

```text
Displays specified stream contents.
Usage: stream_id ?S
```

---

## Group: `voice`

| Command | Alias | Clauses | Arity | Execution | Summary |
|:---|:---|:---:|:---:|:---:|:---|
| [`all_voices_gate_off`](#all_voices_gate_off) | `O` | 2 | 0..1 | Immediate | turn gate off for all voices |
| [`amp`](#amp) | `a` | 2 | 1 | Real-Time | set voice output amplitude / gain |
| [`amp_bend`](#amp_bend) | `ab` | 1 | 1 | Real-Time | amplitude bend (-1..1) |
| [`amp_bend_param`](#amp_bend_param) | `abp` | 2 | 1..2 | Real-Time | amplitude bend range in dB |
| [`amp_mod`](#amp_mod) | `A` | 4 | 0..3 | Real-Time | disable amplitude modulation |
| [`crush`](#crush) | `bc` | 1 | 0..2 | Real-Time | voice waveform bit-crush and quantization |
| [`delay_send`](#delay_send) | `ds` | 2 | 0..1 | Immediate | set voice delay send level |
| [`gate_off`](#gate_off) | `off` | 2 | 0..1 | Immediate | turn active voice gate off |
| [`gate_on`](#gate_on) | `on` | 2 | 0..1 | Immediate | turn active voice gate on |
| [`glissando`](#glissando) | `g` | 1 | 1..2 | Real-Time | voice glissando portamento rate and optional mode |
| [`global_status`](#global_status) | `GS` | 2 | 0..1 | Immediate | show global engine status |
| [`inst_note`](#inst_note) | `i` | 3 | 1..3 | Real-Time | trigger instrument note |
| [`master_volume`](#master_volume) | `V` | 2 | 0..1 | Immediate | set master output volume |
| [`mute`](#mute) | `m` | 2 | 0..1 | Real-Time | mute or unmute voice audio output (0 = unmute, 1 = mute) |
| [`pan`](#pan) | `p` | 2 | 1 | Real-Time | set voice stereo pan position (-1.0 left to +1.0 right) |
| [`pan_mod`](#pan_mod) | `P` | 4 | 0..3 | Real-Time | disable pan modulation |
| [`poly_bend`](#poly_bend) | `pb` | 2 | 3..4 | Real-Time | apply polyphonic pitch bend |
| [`poly_bend_gb`](#poly_bend_gb) | `gb` | 2 | 3..4 | Real-Time | apply polyphonic pitch bend (gb alias) |
| [`poly_note`](#poly_note) | `pn` | 2 | 4..5 | Real-Time | trigger polyphonic note |
| [`poly_note_gn`](#poly_note_gn) | `gn` | 2 | 4..5 | Real-Time | trigger polyphonic note (gn alias) |
| [`poly_release`](#poly_release) | `pr` | 2 | 2..3 | Real-Time | release polyphonic note |
| [`poly_release_gr`](#poly_release_gr) | `gr` | 2 | 2..3 | Real-Time | release polyphonic note (gr alias) |
| [`query_all_voices_info`](#query_all_voices_info) | `v??` | 2 | 0..1 | Immediate | query all voices information |
| [`query_voice_info`](#query_voice_info) | `v?` | 2 | 0..1 | Immediate | query voice information |
| [`rec_auto_trim`](#rec_auto_trim) | `w<>` | 4 | 0..3 | Immediate | auto-detect recording buffer trim points |
| [`rec_auto_trim_alias`](#rec_auto_trim_alias) | `ra` | 4 | 0..3 | Immediate | auto-detect recording buffer trim points (alias) |
| [`rec_to_wave_slot`](#rec_to_wave_slot) | `w!` | 2 | 0..1 | Immediate | save trimmed recording to wavetable slot |
| [`rec_to_wave_slot_alias`](#rec_to_wave_slot_alias) | `r!` | 2 | 0..1 | Immediate | save trimmed recording to wavetable slot (alias) |
| [`rec_trim_end`](#rec_trim_end) | `w<` | 2 | 0..1 | Immediate | advance recording end trim by 1 |
| [`rec_trim_end_alias`](#rec_trim_end_alias) | `re` | 2 | 0..1 | Immediate | advance recording end trim by 1 (alias) |
| [`rec_trim_reset`](#rec_trim_reset) | `w*` | 2 | 0..1 | Immediate | reset recording buffer trim points |
| [`rec_trim_reset_alias`](#rec_trim_reset_alias) | `r*` | 2 | 0..1 | Immediate | reset recording buffer trim points (alias) |
| [`rec_trim_start`](#rec_trim_start) | `w>` | 2 | 0..1 | Immediate | advance recording start offset by 1 |
| [`rec_trim_start_alias`](#rec_trim_start_alias) | `rs` | 2 | 0..1 | Immediate | advance recording start offset by 1 (alias) |
| [`record_tracks_show`](#record_tracks_show) | `?r` | 2 | 0..1 | Immediate | display record track routing |
| [`ring_mod`](#ring_mod) | `XM` | 2 | 1..2 | Real-Time | ring modulation source |
| [`sample_hold`](#sample_hold) | `h` | 3 | 0..2 | Real-Time | set sample and hold ratio |
| [`smoother`](#smoother) | `s` | 1 | 1..2 | Real-Time | parameter smoothing time and optional mode |
| [`track_route`](#track_route) | `r` | 3 | 0..2 | Real-Time | route voice to track |
| [`track_route_track`](#track_route_track) | `rt` | 2 | 0..1 | Immediate | route active voice to track |
| [`track_route_voice`](#track_route_voice) | `rv` | 3 | 0..2 | Immediate | set track volume in dB |
| [`velocity`](#velocity) | `l` | 1 | 1 | Real-Time | set voice note velocity |
| [`velocity_legato`](#velocity_legato) | `___l` | 2 | 0..1 | Immediate | trigger voice envelope velocity with legato |
| [`voice`](#voice) | `v` | 4 | 0..3 | Immediate | select active voice index for subsequent commands |
| [`voice_control_events`](#voice_control_events) | `vc` | 2 | 0..1 | Immediate | enable or disable voice control events |
| [`voice_copy`](#voice_copy) | `>` | 1 | 1 | Real-Time | copy voice state to target voice |
| [`voice_param_query`](#voice_param_query) | `v*` | 2 | 1..2 | Immediate | query voice parameter value |
| [`voice_reset`](#voice_reset) | `S` | 1 | 1 | Real-Time | reset voice state to defaults |
| [`voice_show`](#voice_show) | `?` | 2 | 0..1 | Immediate | display active voice summary |
| [`voice_show_all`](#voice_show_all) | `??` | 2 | 0..1 | Immediate | display all voices summary |
| [`voice_show_verbose`](#voice_show_verbose) | `\\` | 2 | 0..1 | Immediate | display verbose voice parameters |
| [`voice_text`](#voice_text) | `vt` | 2 | 0..1 | Immediate | set voice label text |
| [`voice_wave_display`](#voice_wave_display) | `VW` | 4 | 0..3 | Immediate | render ascii waveform for active voice |
| [`wave_default`](#wave_default) | `/` | 2 | 0..1 | Real-Time | reset voice waveform to default sine |
| [`wave_display`](#wave_display) | `W` | 4 | 0..3 | Immediate | render ascii waveform for wave slot |
| [`wave_expand`](#wave_expand) | `/wex` | 2 | 0..1 | Immediate | dynamically expand wavetable capacity |
| [`wave_export_wav`](#wave_export_wav) | `w>w` | 2 | 0..1 | Immediate | export wavetable slot to WAV file |
| [`wave_loop_points`](#wave_loop_points) | `WL` | 4 | 0..3 | Immediate | set wavetable slot loop points |
| [`wave_to_data`](#wave_to_data) | `w>d` | 2 | 0..1 | Immediate | copy wavetable slot samples to data array |
| [`wave_to_rec`](#wave_to_rec) | `w>r` | 2 | 0..1 | Immediate | copy wavetable slot into recording buffer |

### `all_voices_gate_off`

- **Alias**: `O`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `all_voices_gate_off()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: turn gate off for all voices

**Description & Details**:

```text
Turns gate off for all synthesizer voices.
Usage: O
```

#### Clause 2: `all_voices_gate_off(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: turn gate off for all voices with argument

**Description & Details**:

```text
Turns gate off for all voices.
```

---

### `amp`

- **Alias**: `a`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `amp(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `191` (`skode_clause_amp_set`)
- **Summary**: set voice output amplitude / gain

**Description & Details**:

```text
Sets the output level (0.0 .. 1.0+) for the selected voice.
Usage: a level (or amp level)
```

#### Clause 2: `amp(nan)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `192` (`skode_clause_amp_reset`)
- **Summary**: reset voice output amplitude to default 0.5

**Description & Details**:

```text
Resets the voice output level to 0.5 default.
Usage: a - (or amp -)
```

---

### `amp_bend`

- **Alias**: `ab`
- **Group**: `voice`
- **Clause Count**: 1

#### Clause 1: `amp_bend(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `197` (`skode_clause_amp_bend`)
- **Summary**: amplitude bend (-1..1)

**Description & Details**:

```text
Sets amplitude bend.
Usage: ab amount (or amp_bend amount)
```

---

### `amp_bend_param`

- **Alias**: `abp`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `amp_bend_param(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `198` (`skode_clause_amp_bend_param_1`)
- **Summary**: amplitude bend range in dB

**Description & Details**:

```text
Sets amplitude bend range in dB with default offset 0.
Usage: abp range (or amp_bend_param range)
```

#### Clause 2: `amp_bend_param(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `199` (`skode_clause_amp_bend_param_2`)
- **Summary**: amplitude bend range and offset

**Description & Details**:

```text
Sets amplitude bend range in dB and fine offset.
Usage: abp range offset (or amp_bend_param range offset)
```

---

### `amp_mod`

- **Alias**: `A`
- **Group**: `voice`
- **Clause Count**: 4

#### Clause 1: `amp_mod()`

- **Arity**: `0`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `205` (`skode_clause_amp_mod_0`)
- **Summary**: disable amplitude modulation

**Description & Details**:

```text
Disables amplitude modulation on active voice.
Usage: A (or amp_mod)
```

#### Clause 2: `amp_mod(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `206` (`skode_clause_amp_mod_1`)
- **Summary**: amplitude modulation source or disable

**Description & Details**:

```text
Disables amplitude modulation if source < 0, or sets source voice with 0 depth.
Usage: A source
```

#### Clause 3: `amp_mod(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `207` (`skode_clause_amp_mod_2`)
- **Summary**: amplitude modulation source and depth

**Description & Details**:

```text
Sets amplitude modulation source voice and depth.
Usage: A source depth
```

#### Clause 4: `amp_mod(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `208` (`skode_clause_amp_mod_3`)
- **Summary**: amplitude modulation source, depth and scale

**Description & Details**:

```text
Sets amplitude modulation source voice, depth and scale factor.
Usage: A source depth scale
```

---

### `crush`

- **Alias**: `bc`
- **Group**: `voice`
- **Clause Count**: 1

#### Clause 1: `crush(finite, finite)`

- **Arity**: `0..2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `238` (`skode_clause_crush`)
- **Summary**: voice waveform bit-crush and quantization

**Description & Details**:

```text
Sets waveform bit-crush bit depth (0..24) and optional quantization curve (0=linear, 1=mu-law/companded, 2=MOS 6581 SID DAC non-linear).
Passing 0 or no arguments disables quantization.
Usage: bc [bits[, curve]] (or crush [bits[, curve]])
```

---

### `delay_send`

- **Alias**: `ds`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `delay_send(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set voice delay send level

**Description & Details**:

```text
Sets delay send level for the active voice.
Usage: ds 15
```

#### Clause 2: `delay_send()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: delay send no-op

**Description & Details**:

```text
Delay send no-op.
```

---

### `gate_off`

- **Alias**: `off`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `gate_off()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: turn active voice gate off

**Description & Details**:

```text
Triggers voice gate off (velocity 0.0) with linked voices.
Usage: off
```

#### Clause 2: `gate_off(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: turn active voice gate off with argument

**Description & Details**:

```text
Triggers voice gate off.
```

---

### `gate_on`

- **Alias**: `on`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `gate_on()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: turn active voice gate on

**Description & Details**:

```text
Triggers voice gate on (velocity 1.0) with linked voices.
Usage: on
```

#### Clause 2: `gate_on(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: turn active voice gate on with argument

**Description & Details**:

```text
Triggers voice gate on.
```

---

### `glissando`

- **Alias**: `g`
- **Group**: `voice`
- **Clause Count**: 1

#### Clause 1: `glissando(finite, finite)`

- **Arity**: `1..2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `201` (`skode_clause_glissando`)
- **Summary**: voice glissando portamento rate and optional mode

**Description & Details**:

```text
Sets pitch slide / portamento time in seconds. Pass 0 to disable.
Optional second argument sets glide mode (0=constant time, 1=constant rate, 2=exponential).
Usage: g time[, mode] (or glissando time[, mode])
```

---

### `global_status`

- **Alias**: `GS`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `global_status()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: show global engine status

**Description & Details**:

```text
Displays global engine status overview.
Usage: GS
```

#### Clause 2: `global_status(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: show global engine status with verbosity

**Description & Details**:

```text
Displays global engine status with verbosity flag.
Usage: GS 1
```

---

### `inst_note`

- **Alias**: `i`
- **Group**: `voice`
- **Clause Count**: 3

#### Clause 1: `inst_note(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `235` (`skode_clause_inst_note_c299`)
- **Summary**: trigger instrument note

**Description & Details**:

```text
Triggers instrument note with default velocity 1.0.
Usage: i note
```

#### Clause 2: `inst_note(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `236` (`skode_clause_inst_note_c300`)
- **Summary**: trigger instrument note with velocity

**Description & Details**:

```text
Triggers instrument note with velocity.
Usage: i note velocity
```

#### Clause 3: `inst_note(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `237` (`skode_clause_inst_note_c301`)
- **Summary**: trigger instrument note with velocity and cents

**Description & Details**:

```text
Triggers instrument note with velocity and fine cents offset.
Usage: i note velocity cents
```

---

### `master_volume`

- **Alias**: `V`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `master_volume(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set master output volume

**Description & Details**:

```text
Sets master output volume in dB.
Usage: V -6
```

#### Clause 2: `master_volume()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: master volume no-op

**Description & Details**:

```text
Master volume no-op.
```

---

### `mute`

- **Alias**: `m`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `mute(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `195` (`skode_clause_mute_set`)
- **Summary**: mute or unmute voice audio output (0 = unmute, 1 = mute)

**Description & Details**:

```text
Controls audio muting for the selected voice.
Usage: m 1 (mute) or m 0 (unmute)
```

#### Clause 2: `mute()`

- **Arity**: `0`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `196` (`skode_clause_mute_toggle`)
- **Summary**: toggle voice mute state

**Description & Details**:

```text
Toggles the audio muting state of the selected voice.
Usage: m (or mute)
```

---

### `pan`

- **Alias**: `p`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `pan(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `193` (`skode_clause_pan_set`)
- **Summary**: set voice stereo pan position (-1.0 left to +1.0 right)

**Description & Details**:

```text
Sets the stereo panning position for the selected voice.
Usage: p pan (or pan pan)
```

#### Clause 2: `pan(nan)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `194` (`skode_clause_pan_center`)
- **Summary**: center voice stereo pan (0.0)

**Description & Details**:

```text
Resets voice stereo panning to dead center (0.0).
Usage: p - (or pan -)
```

---

### `pan_mod`

- **Alias**: `P`
- **Group**: `voice`
- **Clause Count**: 4

#### Clause 1: `pan_mod()`

- **Arity**: `0`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `209` (`skode_clause_pan_mod_0`)
- **Summary**: disable pan modulation

**Description & Details**:

```text
Disables stereo pan modulation on active voice.
Usage: P (or pan_mod)
```

#### Clause 2: `pan_mod(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `210` (`skode_clause_pan_mod_1`)
- **Summary**: pan modulation source or disable

**Description & Details**:

```text
Disables pan modulation if source < 0, or sets source voice with 0 depth.
Usage: P source
```

#### Clause 3: `pan_mod(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `211` (`skode_clause_pan_mod_2`)
- **Summary**: pan modulation source and depth

**Description & Details**:

```text
Sets pan modulation source voice and depth.
Usage: P source depth
```

#### Clause 4: `pan_mod(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `212` (`skode_clause_pan_mod_3`)
- **Summary**: pan modulation source, depth and scale

**Description & Details**:

```text
Sets pan modulation source voice, depth and scale.
Usage: P source depth scale
```

---

### `poly_bend`

- **Alias**: `pb`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `poly_bend(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `223` (`skode_clause_poly_bend_3`)
- **Summary**: apply polyphonic pitch bend

**Description & Details**:

```text
Bends pitch of active note in pool (pool, key, semitones).
Usage: pb pool key semitones
```

#### Clause 2: `poly_bend(finite, finite, finite, finite)`

- **Arity**: `4`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `224` (`skode_clause_poly_bend_4`)
- **Summary**: apply polyphonic pitch bend with cents

**Description & Details**:

```text
Bends pitch of active note in pool with semitones and fine cents.
Usage: pb pool key semitones cents
```

---

### `poly_bend_gb`

- **Alias**: `gb`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `poly_bend_gb(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `225` (`skode_clause_poly_bend_gb_3`)
- **Summary**: apply polyphonic pitch bend (gb alias)

**Description & Details**:

```text
Bends pitch of active note in pool (gb alias).
Usage: gb pool key semitones
```

#### Clause 2: `poly_bend_gb(finite, finite, finite, finite)`

- **Arity**: `4`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `226` (`skode_clause_poly_bend_gb_4`)
- **Summary**: apply polyphonic pitch bend with cents (gb alias)

**Description & Details**:

```text
Bends pitch of active note in pool with cents (gb alias).
Usage: gb pool key semitones cents
```

---

### `poly_note`

- **Alias**: `pn`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `poly_note(finite, finite, finite, finite)`

- **Arity**: `4`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `215` (`skode_clause_poly_note_4`)
- **Summary**: trigger polyphonic note

**Description & Details**:

```text
Triggers note on pool (pool, key, note number, velocity).
Usage: pn pool,key,note,velocity
```

#### Clause 2: `poly_note(finite, finite, finite, finite, finite)`

- **Arity**: `5`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `216` (`skode_clause_poly_note_5`)
- **Summary**: trigger polyphonic note with cents

**Description & Details**:

```text
Triggers note on pool with fine detuning in cents.
Usage: pn pool,key,note,velocity,cents
```

---

### `poly_note_gn`

- **Alias**: `gn`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `poly_note_gn(finite, finite, finite, finite)`

- **Arity**: `4`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `217` (`skode_clause_poly_note_gn_4`)
- **Summary**: trigger polyphonic note (gn alias)

**Description & Details**:

```text
Triggers note on pool (gn alias).
Usage: gn pool,key,note,velocity
```

#### Clause 2: `poly_note_gn(finite, finite, finite, finite, finite)`

- **Arity**: `5`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `218` (`skode_clause_poly_note_gn_5`)
- **Summary**: trigger polyphonic note with cents (gn alias)

**Description & Details**:

```text
Triggers note on pool with fine detuning (gn alias).
Usage: gn pool,key,note,velocity,cents
```

---

### `poly_release`

- **Alias**: `pr`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `poly_release(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `219` (`skode_clause_poly_release_2`)
- **Summary**: release polyphonic note

**Description & Details**:

```text
Releases active note on pool by key index.
Usage: pr pool key
```

#### Clause 2: `poly_release(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `220` (`skode_clause_poly_release_3`)
- **Summary**: release polyphonic note with release velocity

**Description & Details**:

```text
Releases active note on pool with custom note-off velocity.
Usage: pr pool key velocity
```

---

### `poly_release_gr`

- **Alias**: `gr`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `poly_release_gr(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `221` (`skode_clause_poly_release_gr_2`)
- **Summary**: release polyphonic note (gr alias)

**Description & Details**:

```text
Releases active note on pool (gr alias).
Usage: gr pool key
```

#### Clause 2: `poly_release_gr(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `222` (`skode_clause_poly_release_gr_3`)
- **Summary**: release polyphonic note with velocity (gr alias)

**Description & Details**:

```text
Releases active note on pool with release velocity (gr alias).
Usage: gr pool key velocity
```

---

### `query_all_voices_info`

- **Alias**: `v??`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `query_all_voices_info()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query all voices information

**Description & Details**:

```text
Queries all synth voices status.
Usage: v??
```

#### Clause 2: `query_all_voices_info(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query all voices information with argument

**Description & Details**:

```text
Queries all synth voices status with argument.
```

---

### `query_voice_info`

- **Alias**: `v?`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `query_voice_info()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query voice information

**Description & Details**:

```text
Queries active voice parameters.
Usage: v?
```

#### Clause 2: `query_voice_info(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query voice information with argument

**Description & Details**:

```text
Queries voice information with argument.
```

---

### `rec_auto_trim`

- **Alias**: `w<>`
- **Group**: `voice`
- **Clause Count**: 4

#### Clause 1: `rec_auto_trim()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: auto-detect recording buffer trim points

**Description & Details**:

```text
Automatically detects recording buffer silence threshold and sets trim points.
Usage: w<>
```

#### Clause 2: `rec_auto_trim(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: auto-detect trim with threshold

**Description & Details**:

```text
Automatically detects recording buffer trim points with threshold.
Usage: -40 w<>
```

#### Clause 3: `rec_auto_trim(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: auto-detect trim with start and end threshold

**Description & Details**:

```text
Automatically detects recording buffer trim points with start and end threshold.
Usage: -40 -50 w<>
```

#### Clause 4: `rec_auto_trim(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: auto-detect trim with margin

**Description & Details**:

```text
Automatically detects recording buffer trim points with thresholds and margin.
Usage: -40 -50 10 w<>
```

---

### `rec_auto_trim_alias`

- **Alias**: `ra`
- **Group**: `voice`
- **Clause Count**: 4

#### Clause 1: `rec_auto_trim_alias()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: auto-detect recording buffer trim points (alias)

**Description & Details**:

```text
Automatically detects recording buffer silence threshold and sets trim points.
Usage: ra
```

#### Clause 2: `rec_auto_trim_alias(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: auto-detect trim with threshold (alias)

**Description & Details**:

```text
Automatically detects recording buffer trim points with threshold.
Usage: -40 ra
```

#### Clause 3: `rec_auto_trim_alias(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: auto-detect trim with start and end threshold (alias)

**Description & Details**:

```text
Automatically detects recording buffer trim points with start and end threshold.
Usage: -40 -50 ra
```

#### Clause 4: `rec_auto_trim_alias(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: auto-detect trim with margin (alias)

**Description & Details**:

```text
Automatically detects recording buffer trim points with thresholds and margin.
Usage: -40 -50 10 ra
```

---

### `rec_to_wave_slot`

- **Alias**: `w!`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `rec_to_wave_slot(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: save trimmed recording to wavetable slot

**Description & Details**:

```text
Saves trimmed recording buffer into specified wavetable slot.
Usage: 0 w!
```

#### Clause 2: `rec_to_wave_slot()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: save trimmed recording to default wavetable slot

**Description & Details**:

```text
Saves trimmed recording buffer into default slot.
Usage: w!
```

---

### `rec_to_wave_slot_alias`

- **Alias**: `r!`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `rec_to_wave_slot_alias(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: save trimmed recording to wavetable slot (alias)

**Description & Details**:

```text
Saves trimmed recording buffer into specified wavetable slot.
Usage: 0 r!
```

#### Clause 2: `rec_to_wave_slot_alias()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: save trimmed recording to default wavetable slot (alias)

**Description & Details**:

```text
Saves trimmed recording buffer into default slot.
Usage: r!
```

---

### `rec_trim_end`

- **Alias**: `w<`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `rec_trim_end()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: advance recording end trim by 1

**Description & Details**:

```text
Advances recording buffer end trim by 1 sample.
Usage: w<
```

#### Clause 2: `rec_trim_end(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: advance recording end trim by samples

**Description & Details**:

```text
Advances recording buffer end trim by specified sample count.
Usage: 100 w<
```

---

### `rec_trim_end_alias`

- **Alias**: `re`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `rec_trim_end_alias()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: advance recording end trim by 1 (alias)

**Description & Details**:

```text
Advances recording buffer end trim by 1 sample.
Usage: re
```

#### Clause 2: `rec_trim_end_alias(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: advance recording end trim by samples (alias)

**Description & Details**:

```text
Advances recording buffer end trim by specified sample count.
Usage: 100 re
```

---

### `rec_trim_reset`

- **Alias**: `w*`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `rec_trim_reset()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: reset recording buffer trim points

**Description & Details**:

```text
Resets recording buffer start offset and end trim to 0.
Usage: w*
```

#### Clause 2: `rec_trim_reset(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: reset recording buffer trim points with argument

**Description & Details**:

```text
Resets recording buffer trim points.
```

---

### `rec_trim_reset_alias`

- **Alias**: `r*`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `rec_trim_reset_alias()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: reset recording buffer trim points (alias)

**Description & Details**:

```text
Resets recording buffer start offset and end trim to 0.
Usage: r*
```

#### Clause 2: `rec_trim_reset_alias(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: reset recording buffer trim points with argument (alias)

**Description & Details**:

```text
Resets recording buffer trim points.
```

---

### `rec_trim_start`

- **Alias**: `w>`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `rec_trim_start()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: advance recording start offset by 1

**Description & Details**:

```text
Advances recording buffer start offset by 1 sample.
Usage: w>
```

#### Clause 2: `rec_trim_start(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: advance recording start offset by samples

**Description & Details**:

```text
Advances recording buffer start offset by specified sample count.
Usage: 100 w>
```

---

### `rec_trim_start_alias`

- **Alias**: `rs`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `rec_trim_start_alias()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: advance recording start offset by 1 (alias)

**Description & Details**:

```text
Advances recording buffer start offset by 1 sample.
Usage: rs
```

#### Clause 2: `rec_trim_start_alias(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: advance recording start offset by samples (alias)

**Description & Details**:

```text
Advances recording buffer start offset by specified sample count.
Usage: 100 rs
```

---

### `record_tracks_show`

- **Alias**: `?r`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `record_tracks_show()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: display record track routing

**Description & Details**:

```text
Displays active recording stems and voice routing.
Usage: ?r
```

#### Clause 2: `record_tracks_show(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: display record track routing with argument

**Description & Details**:

```text
Displays active recording stems and voice routing.
```

---

### `ring_mod`

- **Alias**: `XM`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `ring_mod(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `203` (`skode_clause_ring_mod_1`)
- **Summary**: ring modulation source

**Description & Details**:

```text
Sets ring modulation source voice with 100% amount. Pass -1 to disable.
Usage: XM source (or ring_mod source)
```

#### Clause 2: `ring_mod(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `204` (`skode_clause_ring_mod_2`)
- **Summary**: ring modulation source and amount

**Description & Details**:

```text
Sets ring modulation source voice and mix amount (0.0 to 1.0).
Usage: XM source amount (or ring_mod source amount)
```

---

### `sample_hold`

- **Alias**: `h`
- **Group**: `voice`
- **Clause Count**: 3

#### Clause 1: `sample_hold(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `227` (`skode_clause_sample_hold_1`)
- **Summary**: set sample and hold ratio

**Description & Details**:

```text
Sets sample and hold decimation ratio for voice.
Usage: h 4
```

#### Clause 2: `sample_hold()`

- **Arity**: `0`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `228` (`skode_clause_sample_hold_0`)
- **Summary**: sample and hold no-op

**Description & Details**:

```text
Sample and hold no-op.
```

#### Clause 3: `sample_hold(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `229` (`skode_clause_sample_hold_2`)
- **Summary**: set sample and hold ratio and mode

**Description & Details**:

```text
Sets sample and hold decimation ratio and quantization mode.
Usage: h 4 1
```

---

### `smoother`

- **Alias**: `s`
- **Group**: `voice`
- **Clause Count**: 1

#### Clause 1: `smoother(finite, finite)`

- **Arity**: `1..2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `202` (`skode_clause_smoother`)
- **Summary**: parameter smoothing time and optional mode

**Description & Details**:

```text
Sets parameter smoothing filter time. Pass 0 to disable.
Optional second argument sets smoother mode (0=exponential, 1=linear).
Usage: s time[, mode] (or smoother time[, mode])
```

---

### `track_route`

- **Alias**: `r`
- **Group**: `voice`
- **Clause Count**: 3

#### Clause 1: `track_route(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `230` (`skode_clause_track_route_1`)
- **Summary**: route voice to track

**Description & Details**:

```text
Routes active voice output to specified record stem/track.
Usage: r 2
```

#### Clause 2: `track_route()`

- **Arity**: `0`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `231` (`skode_clause_track_route_0`)
- **Summary**: track route no-op

**Description & Details**:

```text
Track route no-op.
```

#### Clause 3: `track_route(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `232` (`skode_clause_track_route_2`)
- **Summary**: route specific voice to track

**Description & Details**:

```text
Routes specific voice to track.
Usage: r track voice
```

---

### `track_route_track`

- **Alias**: `rt`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `track_route_track(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: route active voice to track

**Description & Details**:

```text
Routes active voice to specified track.
Usage: rt 2
```

#### Clause 2: `track_route_track()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: track route track no-op

**Description & Details**:

```text
Track route track no-op.
```

---

### `track_route_voice`

- **Alias**: `rv`
- **Group**: `voice`
- **Clause Count**: 3

#### Clause 1: `track_route_voice(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set track volume in dB

**Description & Details**:

```text
Sets volume in dB for specified track.
Usage: rv 2, -9
```

#### Clause 2: `track_route_voice(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: route target voice to active voice track

**Description & Details**:

```text
Routes target voice to the track of the active voice.
Usage: rv 0
```

#### Clause 3: `track_route_voice()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: track route voice no-op

**Description & Details**:

```text
Track route voice no-op.
```

---

### `velocity`

- **Alias**: `l`
- **Group**: `voice`
- **Clause Count**: 1

#### Clause 1: `velocity(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `200` (`skode_clause_velocity`)
- **Summary**: set voice note velocity

**Description & Details**:

```text
Sets voice note velocity / gate duration.
Usage: l vel (or velocity vel)
```

---

### `velocity_legato`

- **Alias**: `___l`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `velocity_legato(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: trigger voice envelope velocity with legato

**Description & Details**:

```text
Triggers voice envelope velocity with legato mode.
Usage: ___l 0.8
```

#### Clause 2: `velocity_legato()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: velocity legato no-op

**Description & Details**:

```text
Velocity legato no-op.
```

---

### `voice`

- **Alias**: `v`
- **Group**: `voice`
- **Clause Count**: 4

#### Clause 1: `voice(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: select active voice index for subsequent commands

**Description & Details**:

```text
Selects which synthesizer voice is targeted by subsequent voice parameters.
Usage: v 0 (selects voice 0)
```

#### Clause 2: `voice()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: voice selection no-op with 0 args

**Description & Details**:

```text
Voice selection no-op when no args provided on stack.
```

#### Clause 3: `voice(finite, any)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: select active voice index with 2 stack args

**Description & Details**:

```text
Selects active voice index ignoring extra stack arguments.
```

#### Clause 4: `voice(finite, any, any)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: select active voice index with 3 stack args

**Description & Details**:

```text
Selects active voice index ignoring extra stack arguments.
```

---

### `voice_control_events`

- **Alias**: `vc`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `voice_control_events(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: enable or disable voice control events

**Description & Details**:

```text
Enables or disables voice event publication to the control queue (1=on, 0=off).
Usage: vc 1
```

#### Clause 2: `voice_control_events()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: voice control events no-op

**Description & Details**:

```text
Voice control events no-op.
```

---

### `voice_copy`

- **Alias**: `>`
- **Group**: `voice`
- **Clause Count**: 1

#### Clause 1: `voice_copy(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `214` (`skode_clause_voice_copy`)
- **Summary**: copy voice state to target voice

**Description & Details**:

```text
Copies all parameters from the active voice into target voice.
Usage: > dest_voice (or voice_copy dest_voice)
```

---

### `voice_param_query`

- **Alias**: `v*`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `voice_param_query(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query voice parameter value

**Description & Details**:

```text
Queries voice parameter (0=wave_index, 1=amplitude, 2=frequency) and pushes to stack.
Usage: v* param
```

#### Clause 2: `voice_param_query(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: copy voice parameter value into variable

**Description & Details**:

```text
Copies voice parameter (0=wave, 1=amp, 2=freq) into variable register $var.
Usage: v* param var
```

---

### `voice_reset`

- **Alias**: `S`
- **Group**: `voice`
- **Clause Count**: 1

#### Clause 1: `voice_reset(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `213` (`skode_clause_voice_reset`)
- **Summary**: reset voice state to defaults

**Description & Details**:

```text
Resets synthesis parameters for target voice to default state.
Usage: S voice (or voice_reset voice)
```

---

### `voice_show`

- **Alias**: `?`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `voice_show()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: display active voice summary

**Description & Details**:

```text
Displays active voice parameters and settings.
Usage: ?
```

#### Clause 2: `voice_show(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: display active voice summary with argument

**Description & Details**:

```text
Displays active voice summary with argument.
```

---

### `voice_show_all`

- **Alias**: `??`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `voice_show_all()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: display all voices summary

**Description & Details**:

```text
Displays status summary for all synth voices.
Usage: ??
```

#### Clause 2: `voice_show_all(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: display all voices summary with argument

**Description & Details**:

```text
Displays status summary for all synth voices with argument.
```

---

### `voice_show_verbose`

- **Alias**: `\\`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `voice_show_verbose()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: display verbose voice parameters

**Description & Details**:

```text
Displays active voice detailed parameters and settings.
Usage: \
```

#### Clause 2: `voice_show_verbose(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: display verbose voice parameters with argument

**Description & Details**:

```text
Displays active voice detailed parameters with argument.
```

---

### `voice_text`

- **Alias**: `vt`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `voice_text()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set voice label text

**Description & Details**:

```text
Sets label text for active voice from string buffer.
Usage: [Lead synth] vt
```

#### Clause 2: `voice_text(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set voice label text with argument

**Description & Details**:

```text
Sets label text for active voice.
```

---

### `voice_wave_display`

- **Alias**: `VW`
- **Group**: `voice`
- **Clause Count**: 4

#### Clause 1: `voice_wave_display()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: render ascii waveform for active voice

**Description & Details**:

```text
Renders ASCII waveform preview for active voice.
Usage: VW
```

#### Clause 2: `voice_wave_display(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: render ascii waveform for specified voice

**Description & Details**:

```text
Renders ASCII waveform preview for specified voice index.
Usage: VW 0
```

#### Clause 3: `voice_wave_display(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: render ascii waveform with dimensions

**Description & Details**:

```text
Renders ASCII waveform preview with specified width and height.
Usage: VW width height
```

#### Clause 4: `voice_wave_display(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: render ascii waveform for voice with dimensions

**Description & Details**:

```text
Renders ASCII waveform preview for voice with width and height.
Usage: VW voice width height
```

---

### `wave_default`

- **Alias**: `/`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `wave_default()`

- **Arity**: `0`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `233` (`skode_clause_wave_default_0`)
- **Summary**: reset voice waveform to default sine

**Description & Details**:

```text
Resets the active voice oscillator waveform to default sine wave.
Usage: /
```

#### Clause 2: `wave_default(any)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `234` (`skode_clause_wave_default_1`)
- **Summary**: reset voice waveform to default sine with argument

**Description & Details**:

```text
Resets the active voice oscillator waveform to default sine wave.
```

---

### `wave_display`

- **Alias**: `W`
- **Group**: `voice`
- **Clause Count**: 4

#### Clause 1: `wave_display(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: render ascii waveform for wave slot

**Description & Details**:

```text
Renders ASCII waveform preview for wave slot or recording buffer.
Usage: 0 W
```

#### Clause 2: `wave_display()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: render ascii waveform for default slot

**Description & Details**:

```text
Renders ASCII waveform preview.
```

#### Clause 3: `wave_display(any, any)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: render ascii waveform with dimensions

**Description & Details**:

```text
Renders ASCII waveform preview with dimensions.
Usage: 0 80 W
```

#### Clause 4: `wave_display(any, any, any)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: render ascii waveform with width and height

**Description & Details**:

```text
Renders ASCII waveform preview with width and height.
Usage: 0 80 20 W
```

---

### `wave_expand`

- **Alias**: `/wex`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `wave_expand(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: dynamically expand wavetable capacity

**Description & Details**:

```text
Expands wavetable slot capacity up to 999.
Usage: 300 /wex
```

#### Clause 2: `wave_expand()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: wave expand no-op

**Description & Details**:

```text
Wave expand no-op.
```

---

### `wave_export_wav`

- **Alias**: `w>w`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `wave_export_wav(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: export wavetable slot to WAV file

**Description & Details**:

```text
Exports samples from wavetable slot to WAV file named in string buffer.
Usage: [export.wav] 0 w>w
```

#### Clause 2: `wave_export_wav()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: wave export wav no-op

**Description & Details**:

```text
Wave export wav no-op.
```

---

### `wave_loop_points`

- **Alias**: `WL`
- **Group**: `voice`
- **Clause Count**: 4

#### Clause 1: `wave_loop_points(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set wavetable slot loop points

**Description & Details**:

```text
Sets loop start and end sample positions for specified wavetable slot.
Usage: WL slot start end
```

#### Clause 2: `wave_loop_points()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: wave loop points no-op

**Description & Details**:

```text
Wave loop points no-op.
```

#### Clause 3: `wave_loop_points(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: wave loop points with 1 arg

**Description & Details**:

```text
Wave loop points with 1 arg.
```

#### Clause 4: `wave_loop_points(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: wave loop points with 2 args

**Description & Details**:

```text
Wave loop points with 2 args.
```

---

### `wave_to_data`

- **Alias**: `w>d`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `wave_to_data(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: copy wavetable slot samples to data array

**Description & Details**:

```text
Copies samples from wavetable slot into the parser data buffer.
Usage: 0 w>d
```

#### Clause 2: `wave_to_data()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: wave to data no-op

**Description & Details**:

```text
Wave to data no-op.
```

---

### `wave_to_rec`

- **Alias**: `w>r`
- **Group**: `voice`
- **Clause Count**: 2

#### Clause 1: `wave_to_rec(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: copy wavetable slot into recording buffer

**Description & Details**:

```text
Copies samples from wavetable slot into the live recording buffer.
Usage: 0 w>r
```

#### Clause 2: `wave_to_rec()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: wave to recording buffer no-op

**Description & Details**:

```text
Wave to recording buffer no-op.
```

---

## Group: `sys`

| Command | Alias | Clauses | Arity | Execution | Summary |
|:---|:---|:---:|:---:|:---:|:---|
| [`audio_input`](#audio_input) | `/ai` | 1 | 1 | Immediate | select audio input device |
| [`audio_list`](#audio_list) | `/als` | 1 | 0 | Immediate | list audio devices |
| [`audio_output`](#audio_output) | `/ao` | 1 | 1 | Immediate | select audio output device |
| [`audio_query`](#audio_query) | `/a?` | 1 | 0 | Immediate | query audio device status |
| [`cz_env_multi`](#cz_env_multi) | `cte` | 2 | 0..1 | Immediate | configure multistage cz envelope from data buffer |
| [`data_buffer`](#data_buffer) | `D` | 2 | 0..1 | Immediate | query data buffer capacity |
| [`data_clear`](#data_clear) | `/D` | 1 | 0 | Immediate | clear or print data buffer allocation |
| [`data_fill`](#data_fill) | `d!` | 1 | 2 | Immediate | store value at data buffer index |
| [`data_query`](#data_query) | `?d` | 1 | 0 | Immediate | dump data array contents |
| [`data_scale`](#data_scale) | `d*` | 1 | 1 | Immediate | get value at data buffer index |
| [`data_to_ksynth`](#data_to_ksynth) | `d>k` | 1 | 1 | Immediate | bind data array to KSynth variable |
| [`data_to_midi_out`](#data_to_midi_out) | `d>MO` | 1 | 0 | Immediate | send data array as MIDI bytes |
| [`data_to_rec`](#data_to_rec) | `d>r` | 1 | 0 | Immediate | copy data array to record buffer |
| [`data_to_sample`](#data_to_sample) | `/d` | 5 | 0..4 | Immediate | load data array into default sample slot |
| [`delay_damping`](#delay_damping) | `DD` | 4 | 0..3 | Hybrid | get or set delay damping |
| [`delay_freeze`](#delay_freeze) | `DF` | 3 | 0..2 | Hybrid | get or toggle delay freeze |
| [`delay_grit`](#delay_grit) | `DG` | 4 | 0..3 | Hybrid | get or set delay grit |
| [`delay_params`](#delay_params) | `DL` | 8 | 0..7 | Hybrid | get delay parameters |
| [`delay_pingpong`](#delay_pingpong) | `DP` | 3 | 0..2 | Hybrid | get or toggle delay pingpong |
| [`delay_query`](#delay_query) | `DL?` | 2 | 0..1 | Immediate | query delay status |
| [`delay_time_ms`](#delay_time_ms) | `DT` | 3 | 0..2 | Hybrid | set delay time in milliseconds |
| [`delay_time_sync`](#delay_time_sync) | `DS` | 4 | 0..3 | Hybrid | set tempo-synced delay time |
| [`dictionary_show`](#dictionary_show) | `?M` | 2 | 0..1 | Immediate | show all dictionary words |
| [`engine_flag`](#engine_flag) | `/f` | 2 | 0..1 | Immediate | query engine flag |
| [`engine_quit`](#engine_quit) | `/q` | 2 | 0..1 | Immediate | quit engine |
| [`foreign_function`](#foreign_function) | `/ff` | 4 | 1..4 | Immediate | invoke foreign function |
| [`help_system`](#help_system) | `/h` | 3 | 0..2 | Immediate | display help |
| [`ksynth_eval`](#ksynth_eval) | `ks` | 1 | 0 | Immediate | evaluate inline KSynth expression |
| [`ksynth_load_file`](#ksynth_load_file) | `/ks` | 2 | 0..1 | Immediate | load KSynth script from file |
| [`ksynth_load_slot`](#ksynth_load_slot) | `/k` | 2 | 1..2 | Immediate | load built-in KSynth instrument slot |
| [`ksynth_query`](#ksynth_query) | `k?` | 1 | 0 | Immediate | dump KSynth expression result |
| [`ksynth_render`](#ksynth_render) | `k!` | 1 | 0 | Immediate | evaluate and render KSynth expression |
| [`ksynth_to_data`](#ksynth_to_data) | `k>d` | 1 | 0 | Immediate | copy KSynth result vector to data array |
| [`ksynth_to_wave`](#ksynth_to_wave) | `k>w` | 5 | 0..4 | Immediate | load KSynth result into default wave slot |
| [`ksynth_wave`](#ksynth_wave) | `kw` | 2 | 0..1 | Immediate | ksynth wave trigger |
| [`ksynth_wave_export`](#ksynth_wave_export) | `kw>` | 2 | 0..1 | Immediate | export KSynth result to data buffer |
| [`log_enable`](#log_enable) | `log` | 2 | 0..1 | Immediate | enable or disable logging |
| [`macro_clear_all`](#macro_clear_all) | `/m!` | 1 | 0 | Immediate | clear all defined user macros |
| [`macro_delete`](#macro_delete) | `/m` | 1 | 0 | Immediate | delete defined user macro |
| [`macro_exec`](#macro_exec) | `e!` | 2 | 0..1 | Immediate | execute bracketed string immediately |
| [`macro_list_query`](#macro_list_query) | `?m` | 1 | 0 | Immediate | list all defined user macros |
| [`macro_load`](#macro_load) | `<e` | 1 | 1 | Immediate | load external macro into string buffer |
| [`macro_query`](#macro_query) | `e?` | 1 | 1 | Immediate | print contents of external macro slot |
| [`macro_store`](#macro_store) | `e>` | 1 | 1 | Immediate | store bracketed string into external macro slot |
| [`map_exec`](#map_exec) | `map!` | 3 | 1..3 | Immediate | map iteration over range 0..count-1 |
| [`midi_bridge_bind`](#midi_bridge_bind) | `/mb` | 1 | 3 | Immediate | bind MIDI event to Skode command |
| [`midi_bridge_clear`](#midi_bridge_clear) | `/mbC` | 1 | 0 | Immediate | clear all MIDI Skode bindings |
| [`midi_bridge_query`](#midi_bridge_query) | `/mb?` | 1 | 0 | Immediate | query MIDI Skode bindings |
| [`midi_bridge_unbind`](#midi_bridge_unbind) | `/mbd` | 1 | 3 | Immediate | remove MIDI Skode binding |
| [`midi_debug`](#midi_debug) | `/md` | 2 | 0..1 | Immediate | toggle midi debug logging |
| [`midi_file_dump`](#midi_file_dump) | `/mfD` | 1 | 3 | Immediate | dump MIDI file events |
| [`midi_file_load`](#midi_file_load) | `/mf` | 1 | 1 | Immediate | load MIDI file into slot |
| [`midi_file_play`](#midi_file_play) | `/mf>` | 1 | 1 | Immediate | play MIDI file slot |
| [`midi_file_seek`](#midi_file_seek) | `/mfP` | 1 | 2 | Immediate | seek MIDI file playback position |
| [`midi_file_status`](#midi_file_status) | `/mf?` | 2 | 0..1 | Immediate | query MIDI player slots status |
| [`midi_file_stop`](#midi_file_stop) | `/mf<` | 1 | 1 | Immediate | stop MIDI file playback |
| [`midi_file_sync`](#midi_file_sync) | `/mfS` | 1 | 2 | Immediate | set MIDI file sync mode |
| [`midi_input`](#midi_input) | `/mi` | 1 | 1 | Immediate | open MIDI input port |
| [`midi_input_close`](#midi_input_close) | `/mic` | 1 | 0 | Immediate | close MIDI input port |
| [`midi_input_virt`](#midi_input_virt) | `/miV` | 1 | 0 | Immediate | open virtual MIDI input port |
| [`midi_list`](#midi_list) | `/mL` | 1 | 0 | Immediate | list MIDI devices |
| [`midi_list_ls`](#midi_list_ls) | `/mls` | 1 | 0 | Immediate | list MIDI devices (alias) |
| [`midi_out`](#midi_out) | `MO` | 3 | 1..3 | Immediate | send raw MIDI byte |
| [`midi_output`](#midi_output) | `/mo` | 1 | 1 | Immediate | open MIDI output port |
| [`midi_output_close`](#midi_output_close) | `/moc` | 1 | 0 | Immediate | close MIDI output port |
| [`midi_output_virt`](#midi_output_virt) | `/moV` | 1 | 0 | Immediate | open virtual MIDI output port |
| [`midi_pool_route`](#midi_pool_route) | `/mp` | 2 | 2..3 | Immediate | route MIDI channel to polyphony pool |
| [`midi_pool_unroute`](#midi_pool_unroute) | `/mpd` | 1 | 2 | Immediate | remove MIDI pool route |
| [`midi_query`](#midi_query) | `/m?` | 1 | 0 | Immediate | query MIDI device status |
| [`midi_route_clear`](#midi_route_clear) | `/mC` | 1 | 0 | Immediate | clear all MIDI routes |
| [`midi_route_query`](#midi_route_query) | `/mR` | 1 | 0 | Immediate | query active MIDI routes |
| [`midi_to_freq`](#midi_to_freq) | `m>f` | 1 | 0 | Immediate | convert MIDI note numbers in data array to Hz |
| [`midi_voice_route`](#midi_voice_route) | `/mv` | 2 | 2..3 | Immediate | route MIDI channel to voice |
| [`midi_voice_unroute`](#midi_voice_unroute) | `/mvd` | 1 | 2 | Immediate | remove MIDI voice route |
| [`notes_to_data`](#notes_to_data) | `n>d` | 1 | 0 | Immediate | parse musical note string into data array |
| [`opcode_exec`](#opcode_exec) | `EXEC` | 5 | 1..5 | Immediate | numeric opcode escape |
| [`performance_reset`](#performance_reset) | `/th!` | 2 | 0..1 | Immediate | reset performance counters |
| [`poly_group`](#poly_group) | `/pg` | 2 | 3..4 | Immediate | configure polyphony group |
| [`poly_group_query`](#poly_group_query) | `?pg` | 2 | 0..1 | Immediate | query all polyphony groups |
| [`poly_group_reset`](#poly_group_reset) | `/pg!` | 1 | 1 | Immediate | refresh polyphony group |
| [`poly_mode`](#poly_mode) | `/pm` | 3 | 2..4 | Immediate | set polyphonic voice pool mode |
| [`poly_pool`](#poly_pool) | `/pp` | 2 | 4..5 | Immediate | configure polyphonic voice pool |
| [`poly_pool_query`](#poly_pool_query) | `?pp` | 2 | 0..1 | Immediate | query all polyphonic voice pools |
| [`poly_pool_reset`](#poly_pool_reset) | `/pp!` | 1 | 1 | Immediate | refresh polyphony pool |
| [`query_pattern`](#query_pattern) | `?p` | 2 | 0..1 | Immediate | query active pattern index |
| [`query_step`](#query_step) | `?st` | 2 | 0..1 | Immediate | query active step index |
| [`query_string`](#query_string) | `?s` | 3 | 0..2 | Immediate | print current string argument |
| [`query_time`](#query_time) | `?t` | 2 | 0..1 | Immediate | query current sample time |
| [`query_voice`](#query_voice) | `?v` | 2 | 0..1 | Immediate | query active voice index |
| [`rec_to_data`](#rec_to_data) | `r>d` | 2 | 0..1 | Immediate | copy recording buffer to data array |
| [`return_push`](#return_push) | `*R` | 9 | 0..8 | Immediate | reset or clear return registers |
| [`return_query`](#return_query) | `?R` | 1 | 0 | Immediate | show return registers without consuming them |
| [`scope_event`](#scope_event) | `/sm` | 1 | 0 | Immediate | publish scope message event |
| [`scope_start`](#scope_start) | `/sg` | 3 | 0..2 | Immediate | start scope IPC streaming |
| [`scope_status`](#scope_status) | `/s?` | 1 | 0 | Immediate | query scope IPC status |
| [`scope_stop`](#scope_stop) | `/ss` | 1 | 0 | Immediate | stop scope IPC streaming |
| [`session_load`](#session_load) | `GS<` | 2 | 0..1 | Immediate | load global session from zip |
| [`session_save`](#session_save) | `GS>` | 2 | 0..1 | Immediate | save global session to zip |
| [`skode_load_file`](#skode_load_file) | `/l` | 2 | 1..2 | Immediate | load script file by index |
| [`skode_load_name`](#skode_load_name) | `/ls` | 2 | 0..1 | Immediate | load script file by name |
| [`sleep_wait`](#sleep_wait) | `wait` | 2 | 0..1 | Immediate | sleep for ms |
| [`stack_clear`](#stack_clear) | `clr` | 5 | 0..4 | Immediate | clear argument stack |
| [`stack_drop`](#stack_drop) | `drop` | 5 | 0..4 | Immediate | drop argument from stack |
| [`stack_dup`](#stack_dup) | `dup` | 5 | 0..4 | Immediate | duplicate top stack argument |
| [`stack_over`](#stack_over) | `over` | 5 | 0..4 | Immediate | duplicate second stack argument to top |
| [`stack_rot`](#stack_rot) | `rot` | 5 | 0..4 | Immediate | rotate top three stack arguments |
| [`stack_swap`](#stack_swap) | `swap` | 5 | 0..4 | Immediate | swap top two stack arguments |
| [`step_program_set`](#step_program_set) | `x` | 8 | 1..8 | Immediate | set pattern step program |
| [`stop`](#stop) | `-` | 1 | 0 | Immediate | stop audio playback and halt sequencer |
| [`string_format`](#string_format) | `s%` | 5 | 0..4 | Immediate | format string template with arguments |
| [`string_puts`](#string_puts) | `puts` | 2 | 0..1 | Immediate | print parser string buffer |
| [`string_slot_load`](#string_slot_load) | `<s` | 1 | 1 | Immediate | load string from memory slot into parser buffer |
| [`string_slot_query`](#string_slot_query) | `s?` | 2 | 0..1 | Immediate | query all string memory slots |
| [`string_slot_store`](#string_slot_store) | `s>` | 1 | 1 | Immediate | save string from parser buffer into memory slot |
| [`system_show`](#system_show) | `/s` | 2 | 0..1 | Immediate | show system status |
| [`thread_status`](#thread_status) | `/th?` | 2 | 0..1 | Immediate | query thread status |
| [`timestamp_logger`](#timestamp_logger) | `I` | 2 | 0..1 | Immediate | timestamp logger |
| [`trace_toggle`](#trace_toggle) | `/t` | 2 | 0..1 | Immediate | toggle parser trace |
| [`udp_emit`](#udp_emit) | `>u` | 5 | 0..4 | Immediate | emit formatted udp event |
| [`udp_info`](#udp_info) | `udp` | 2 | 0..1 | Immediate | show udp status |
| [`var_add`](#var_add) | `a=` | 1 | 3 | Immediate | add two values and store in variable |
| [`var_div`](#var_div) | `/=` | 1 | 3 | Immediate | divide two values and store in variable |
| [`var_from_data`](#var_from_data) | `=d` | 1 | 2 | Immediate | copy data array element to variable |
| [`var_mul`](#var_mul) | `*=` | 1 | 3 | Immediate | multiply two values and store in variable |
| [`var_set`](#var_set) | `=` | 3 | 0..2 | Immediate | print all non-zero variables |
| [`var_sub`](#var_sub) | `s=` | 1 | 3 | Immediate | subtract two values and store in variable |
| [`verbose_toggle`](#verbose_toggle) | `/v` | 2 | 0..1 | Immediate | toggle verbose output |
| [`vfs_cat`](#vfs_cat) | `%cat` | 2 | 0..1 | Immediate | print file contents from vfs |
| [`vfs_cd`](#vfs_cd) | `%cd` | 2 | 0..1 | Immediate | change vfs directory |
| [`vfs_ls`](#vfs_ls) | `%ls` | 3 | 0..2 | Immediate | list vfs directory |
| [`vfs_mount`](#vfs_mount) | `%z` | 2 | 0..1 | Immediate | mount vfs directory or zip |
| [`vfs_pwd`](#vfs_pwd) | `%pwd` | 2 | 0..1 | Immediate | print current vfs directory |
| [`vfs_unmount`](#vfs_unmount) | `%zu` | 2 | 0..1 | Immediate | unmount vfs |
| [`voice_bench`](#voice_bench) | `/m_` | 2 | 0..1 | Immediate | benchmark synth voice |
| [`voice_graph`](#voice_graph) | `/vg` | 3 | 1..3 | Immediate | inspect voice routing graph |
| [`voice_link_gate`](#voice_link_gate) | `G` | 5 | 0..4 | Real-Time | link gate to target voice |
| [`voice_link_sync`](#voice_link_sync) | `H` | 5 | 0..4 | Real-Time | link velocity to target voice |
| [`voice_link_trig`](#voice_link_trig) | `L` | 2 | 0..1 | Real-Time | set voice link trigger duration |
| [`wave_load_file`](#wave_load_file) | `/w` | 3 | 1..3 | Immediate | load wave file by index |
| [`wave_load_name`](#wave_load_name) | `/ws` | 3 | 0..2 | Immediate | load wave file by name into default slot |
| [`wave_to_ksynth`](#wave_to_ksynth) | `w>k` | 1 | 2 | Immediate | bind wavetable samples to KSynth variable |

### `audio_input`

- **Alias**: `/ai`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `audio_input(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: select audio input device

**Description & Details**:

```text
Selects audio capture device by index (-1 for default, -2 to disable).
Usage: /ai index
```

---

### `audio_list`

- **Alias**: `/als`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `audio_list()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: list audio devices

**Description & Details**:

```text
Lists available audio host devices.
Usage: /als
```

---

### `audio_output`

- **Alias**: `/ao`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `audio_output(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: select audio output device

**Description & Details**:

```text
Selects audio playback device by index (-1 for default).
Usage: /ao index
```

---

### `audio_query`

- **Alias**: `/a?`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `audio_query()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query audio device status

**Description & Details**:

```text
Prints active audio input/output configuration and status.
Usage: /a?
```

---

### `cz_env_multi`

- **Alias**: `cte`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `cz_env_multi()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: configure multistage cz envelope from data buffer

**Description & Details**:

```text
Configures Casio CZ style envelope stages from data array.
Usage: (0 0 100 1 50 .5) cte
```

#### Clause 2: `cz_env_multi(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: configure multistage cz envelope with argument

**Description & Details**:

```text
Configures CZ envelope with argument.
```

---

### `data_buffer`

- **Alias**: `D`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `data_buffer()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `data`
- **Summary**: query data buffer capacity

**Description & Details**:

```text
Prints the current capacity of the internal data array.
Usage: D (or data_buffer)
```

#### Clause 2: `data_buffer(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `data`
- **Summary**: resize data buffer capacity

**Description & Details**:

```text
Resizes internal data buffer to hold at least N elements.
Usage: D capacity
```

---

### `data_clear`

- **Alias**: `/D`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `data_clear()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `data`
- **Summary**: clear or print data buffer allocation

**Description & Details**:

```text
Prints data pointer, capacity and length.
Usage: /D (or data_clear)
```

---

### `data_fill`

- **Alias**: `d!`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `data_fill(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `data`
- **Summary**: store value at data buffer index

**Description & Details**:

```text
Sets data[index] = value in the internal data array.
Usage: d! val index (or data_fill val index)
```

---

### `data_query`

- **Alias**: `?d`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `data_query()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `data`
- **Summary**: dump data array contents

**Description & Details**:

```text
Prints values stored in the internal data array.
Usage: ?d (or data_query)
```

---

### `data_scale`

- **Alias**: `d*`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `data_scale(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `data`
- **Summary**: get value at data buffer index

**Description & Details**:

```text
Reads data[index] and pushes value onto argument stack.
Usage: d* index (or data_scale index)
```

---

### `data_to_ksynth`

- **Alias**: `d>k`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `data_to_ksynth(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `KSYNTH`
- **Summary**: bind data array to KSynth variable

**Description & Details**:

```text
Binds active data buffer array to KSynth global variable index.
Usage: d>k var
```

---

### `data_to_midi_out`

- **Alias**: `d>MO`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `data_to_midi_out()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Parser State Requirements**: `data`
- **Summary**: send data array as MIDI bytes

**Description & Details**:

```text
Transmits contents of active data buffer as raw SysEx or MIDI bytes.
Usage: d>MO
```

---

### `data_to_rec`

- **Alias**: `d>r`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `data_to_rec()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `data`
- **Summary**: copy data array to record buffer

**Description & Details**:

```text
Copies contents of active data buffer into audio recording buffer.
Usage: d>r (or data_to_rec)
```

---

### `data_to_sample`

- **Alias**: `/d`
- **Group**: `sys`
- **Clause Count**: 5

#### Clause 1: `data_to_sample()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load data array into default sample slot

**Description & Details**:

```text
Loads active data array samples into sample slot.
Usage: /d
```

#### Clause 2: `data_to_sample(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load data array into specified sample slot

**Description & Details**:

```text
Loads active data array into specified sample slot.
Usage: /d slot
```

#### Clause 3: `data_to_sample(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load data array with sample rate

**Description & Details**:

```text
Loads active data array into slot with custom sample rate.
Usage: /d slot rate
```

#### Clause 4: `data_to_sample(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load data array with one-shot flag

**Description & Details**:

```text
Loads data array with slot, rate, and one-shot playback flag.
Usage: /d slot rate one_shot
```

#### Clause 5: `data_to_sample(finite, finite, finite, finite)`

- **Arity**: `4`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load data array with offset

**Description & Details**:

```text
Loads data array with slot, rate, one-shot flag, and loop offset.
Usage: /d slot rate one_shot offset
```

---

### `delay_damping`

- **Alias**: `DD`
- **Group**: `sys`
- **Clause Count**: 4

#### Clause 1: `delay_damping()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: get or set delay damping

**Description & Details**:

```text
Sets delay damping filters.
Usage: DD
```

#### Clause 2: `delay_damping(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `249` (`skode_clause_delay_damping_1`)
- **Summary**: set delay damping bus

**Description & Details**:

```text
Sets delay damping bus.
Usage: DD bus
```

#### Clause 3: `delay_damping(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `250` (`skode_clause_delay_damping_2`)
- **Summary**: set delay damping bus and lp damping

**Description & Details**:

```text
Sets delay damping bus and lowpass damping.
Usage: DD bus damping
```

#### Clause 4: `delay_damping(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `251` (`skode_clause_delay_damping_3`)
- **Summary**: set delay damping bus, lp and hp

**Description & Details**:

```text
Sets delay damping bus, lowpass and highpass filter cutoff.
Usage: DD bus damping hp
```

---

### `delay_freeze`

- **Alias**: `DF`
- **Group**: `sys`
- **Clause Count**: 3

#### Clause 1: `delay_freeze()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: get or toggle delay freeze

**Description & Details**:

```text
Sets or toggles delay line buffer freeze.
Usage: DF
```

#### Clause 2: `delay_freeze(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `252` (`skode_clause_delay_freeze_1`)
- **Summary**: set delay freeze bus

**Description & Details**:

```text
Sets delay freeze bus.
Usage: DF bus
```

#### Clause 3: `delay_freeze(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `253` (`skode_clause_delay_freeze_2`)
- **Summary**: set delay freeze bus and state

**Description & Details**:

```text
Sets delay freeze bus and state (1=freeze, 0=unfreeze).
Usage: DF bus state
```

---

### `delay_grit`

- **Alias**: `DG`
- **Group**: `sys`
- **Clause Count**: 4

#### Clause 1: `delay_grit()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: get or set delay grit

**Description & Details**:

```text
Sets delay grit bitcrusher parameters.
Usage: DG
```

#### Clause 2: `delay_grit(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `239` (`skode_clause_delay_grit_1`)
- **Summary**: set delay grit bus

**Description & Details**:

```text
Sets delay grit bus.
Usage: DG bus
```

#### Clause 3: `delay_grit(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `240` (`skode_clause_delay_grit_2`)
- **Summary**: set delay grit bus and bits

**Description & Details**:

```text
Sets delay grit bus and bit depth.
Usage: DG bus bits
```

#### Clause 4: `delay_grit(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `241` (`skode_clause_delay_grit_3`)
- **Summary**: set delay grit bus, bits and native mode

**Description & Details**:

```text
Sets delay grit bus, bit depth and native sample rate mode.
Usage: DG bus bits native
```

---

### `delay_params`

- **Alias**: `DL`
- **Group**: `sys`
- **Clause Count**: 8

#### Clause 1: `delay_params()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: get delay parameters

**Description & Details**:

```text
Shows delay parameters.
Usage: DL
```

#### Clause 2: `delay_params(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `242` (`skode_clause_delay_params_1`)
- **Summary**: set delay bus

**Description & Details**:

```text
Sets delay bus.
Usage: DL bus
```

#### Clause 3: `delay_params(finite, any)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `243` (`skode_clause_delay_params_2`)
- **Summary**: set delay bus and coarse time

**Description & Details**:

```text
Sets delay bus and coarse time.
```

#### Clause 4: `delay_params(finite, any, any)`

- **Arity**: `3`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `244` (`skode_clause_delay_params_3`)
- **Summary**: set delay bus, coarse, fine

**Description & Details**:

```text
Sets delay bus, coarse, fine.
```

#### Clause 5: `delay_params(finite, any, any, any)`

- **Arity**: `4`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `245` (`skode_clause_delay_params_4`)
- **Summary**: set delay bus, coarse, fine, feedback

**Description & Details**:

```text
Sets delay bus, coarse, fine, feedback.
```

#### Clause 6: `delay_params(finite, any, any, any, any)`

- **Arity**: `5`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `246` (`skode_clause_delay_params_5`)
- **Summary**: set delay bus, coarse, fine, feedback, mod_freq

**Description & Details**:

```text
Sets delay parameters with mod_freq.
```

#### Clause 7: `delay_params(finite, any, any, any, any, any)`

- **Arity**: `6`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `247` (`skode_clause_delay_params_6`)
- **Summary**: set delay bus, coarse, fine, feedback, mod_freq, mod_depth

**Description & Details**:

```text
Sets delay parameters with mod_depth.
```

#### Clause 8: `delay_params(finite, any, any, any, any, any, any)`

- **Arity**: `7`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `248` (`skode_clause_delay_params_7`)
- **Summary**: set all delay parameters

**Description & Details**:

```text
Sets all delay parameters: bus, coarse, fine, feedback, mod_freq, mod_depth, level.
Usage: DL bus,coarse,fine,feedback,mod_freq,mod_depth,level
```

---

### `delay_pingpong`

- **Alias**: `DP`
- **Group**: `sys`
- **Clause Count**: 3

#### Clause 1: `delay_pingpong()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: get or toggle delay pingpong

**Description & Details**:

```text
Sets or toggles delay line pingpong panning mode.
Usage: DP
```

#### Clause 2: `delay_pingpong(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `254` (`skode_clause_delay_pingpong_1`)
- **Summary**: set delay pingpong bus

**Description & Details**:

```text
Sets delay pingpong bus.
Usage: DP bus
```

#### Clause 3: `delay_pingpong(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `255` (`skode_clause_delay_pingpong_2`)
- **Summary**: set delay pingpong bus and state

**Description & Details**:

```text
Sets delay pingpong bus and state (1=pingpong on, 0=off).
Usage: DP bus state
```

---

### `delay_query`

- **Alias**: `DL?`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `delay_query()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query delay status

**Description & Details**:

```text
Prints delay line status summary.
Usage: DL?
```

#### Clause 2: `delay_query(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query delay bus status

**Description & Details**:

```text
Prints delay bus status.
Usage: DL? 1
```

---

### `delay_time_ms`

- **Alias**: `DT`
- **Group**: `sys`
- **Clause Count**: 3

#### Clause 1: `delay_time_ms(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `256` (`skode_clause_delay_time_ms_2`)
- **Summary**: set delay time in milliseconds

**Description & Details**:

```text
Sets delay time directly in milliseconds.
Usage: DT bus ms
```

#### Clause 2: `delay_time_ms()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: delay time ms no-op

**Description & Details**:

```text
Delay time ms no-op.
```

#### Clause 3: `delay_time_ms(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `257` (`skode_clause_delay_time_ms_1`)
- **Summary**: delay time ms bus

**Description & Details**:

```text
Delay time ms bus only.
```

---

### `delay_time_sync`

- **Alias**: `DS`
- **Group**: `sys`
- **Clause Count**: 4

#### Clause 1: `delay_time_sync(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `258` (`skode_clause_delay_time_sync_3`)
- **Summary**: set tempo-synced delay time

**Description & Details**:

```text
Sets tempo-synced delay time with beats and division.
Usage: DS bus beats division
```

#### Clause 2: `delay_time_sync()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: delay time sync no-op

**Description & Details**:

```text
Delay time sync no-op.
```

#### Clause 3: `delay_time_sync(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `259` (`skode_clause_delay_time_sync_1`)
- **Summary**: delay time sync bus

**Description & Details**:

```text
Delay time sync bus only.
```

#### Clause 4: `delay_time_sync(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `260` (`skode_clause_delay_time_sync_2`)
- **Summary**: delay time sync bus and beats

**Description & Details**:

```text
Delay time sync bus and beats.
Usage: DS bus beats
```

---

### `dictionary_show`

- **Alias**: `?M`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `dictionary_show()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: show all dictionary words

**Description & Details**:

```text
Displays all dictionary words and categories.
Usage: ?M [mode]
```

#### Clause 2: `dictionary_show(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: show dictionary words by filter mode

**Description & Details**:

```text
Displays dictionary words filtered by mode (1=real-time, 2=immediate).
Usage: mode ?M
```

---

### `engine_flag`

- **Alias**: `/f`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `engine_flag()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query engine flag

**Description & Details**:

```text
Displays current engine flag value.
Usage: /f
```

#### Clause 2: `engine_flag(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set engine flag

**Description & Details**:

```text
Sets engine flag value.
Usage: /f 1
```

---

### `engine_quit`

- **Alias**: `/q`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `engine_quit()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: quit engine

**Description & Details**:

```text
Signals the engine to terminate.
Usage: /q
```

#### Clause 2: `engine_quit(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: quit engine with argument

**Description & Details**:

```text
Signals the engine to terminate.
```

---

### `foreign_function`

- **Alias**: `/ff`
- **Group**: `sys`
- **Clause Count**: 4

#### Clause 1: `foreign_function(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: invoke foreign function

**Description & Details**:

```text
Calls registered foreign function by index.
Usage: /ff index
```

#### Clause 2: `foreign_function(finite, any)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: invoke foreign function with 1 arg

**Description & Details**:

```text
Calls registered foreign function with argument.
Usage: /ff index arg1
```

#### Clause 3: `foreign_function(finite, any, any)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: invoke foreign function with 2 args

**Description & Details**:

```text
Calls registered foreign function with arguments.
Usage: /ff index arg1 arg2
```

#### Clause 4: `foreign_function(finite, any, any, any)`

- **Arity**: `4`
- **Execution Model**: `immediate (control plane)`
- **Summary**: invoke foreign function with 3 args

**Description & Details**:

```text
Calls registered foreign function with arguments.
Usage: /ff index arg1 arg2 arg3
```

---

### `help_system`

- **Alias**: `/h`
- **Group**: `sys`
- **Clause Count**: 3

#### Clause 1: `help_system()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: display help

**Description & Details**:

```text
Displays help summary or command documentation.
Usage: /h
```

#### Clause 2: `help_system(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: display help for topic

**Description & Details**:

```text
Displays help for specified command or topic.
Usage: /h topic
```

#### Clause 3: `help_system(any, any)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: display help with arguments

**Description & Details**:

```text
Displays help with arguments.
```

---

### `ksynth_eval`

- **Alias**: `ks`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `ksynth_eval()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `KSYNTH`
- **Summary**: evaluate inline KSynth expression

**Description & Details**:

```text
Evaluates inline KSynth expression string.
Usage: [expression] ks
```

---

### `ksynth_load_file`

- **Alias**: `/ks`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `ksynth_load_file()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `KSYNTH`
- **Summary**: load KSynth script from file

**Description & Details**:

```text
Loads and evaluates a KSynth script from file path in bracket string.
Usage: [file.k] /ks
```

#### Clause 2: `ksynth_load_file(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `KSYNTH`
- **Summary**: load KSynth script with verbosity

**Description & Details**:

```text
Loads KSynth script from file with verbosity level (0 or 1).
Usage: [file.k] /ks verbose
```

---

### `ksynth_load_slot`

- **Alias**: `/k`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `ksynth_load_slot(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `KSYNTH`
- **Summary**: load built-in KSynth instrument slot

**Description & Details**:

```text
Loads built-in KSynth instrument by slot number.
Usage: /k slot
```

#### Clause 2: `ksynth_load_slot(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `KSYNTH`
- **Summary**: load built-in KSynth slot with verbosity

**Description & Details**:

```text
Loads built-in KSynth instrument with verbose flag.
Usage: /k slot verbose
```

---

### `ksynth_query`

- **Alias**: `k?`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `ksynth_query()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `KSYNTH`
- **Summary**: dump KSynth expression result

**Description & Details**:

```text
Prints numeric values of the most recent KSynth result vector.
Usage: k?
```

---

### `ksynth_render`

- **Alias**: `k!`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `ksynth_render()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `KSYNTH`
- **Summary**: evaluate and render KSynth expression

**Description & Details**:

```text
Evaluates and renders KSynth expression string into audio or wavetable.
Usage: [expression] k!
```

---

### `ksynth_to_data`

- **Alias**: `k>d`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `ksynth_to_data()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `KSYNTH`
- **Summary**: copy KSynth result vector to data array

**Description & Details**:

```text
Transfers most recent KSynth vector result into the active data buffer.
Usage: k>d
```

---

### `ksynth_to_wave`

- **Alias**: `k>w`
- **Group**: `sys`
- **Clause Count**: 5

#### Clause 1: `ksynth_to_wave()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `KSYNTH`
- **Summary**: load KSynth result into default wave slot

**Description & Details**:

```text
Loads active KSynth result into default sample wavetable slot.
Usage: k>w
```

#### Clause 2: `ksynth_to_wave(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `KSYNTH`
- **Summary**: load KSynth result into specified wave slot

**Description & Details**:

```text
Loads active KSynth result into wavetable slot.
Usage: k>w slot
```

#### Clause 3: `ksynth_to_wave(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `KSYNTH`
- **Summary**: load KSynth result with sample rate

**Description & Details**:

```text
Loads active KSynth result into wavetable slot with playback sample rate.
Usage: k>w slot rate
```

#### Clause 4: `ksynth_to_wave(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `KSYNTH`
- **Summary**: load KSynth result with one-shot flag

**Description & Details**:

```text
Loads active KSynth result with slot, rate, and one-shot playback flag.
Usage: k>w slot rate one_shot
```

#### Clause 5: `ksynth_to_wave(finite, finite, finite, finite)`

- **Arity**: `4`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `KSYNTH`
- **Summary**: load KSynth result with offset

**Description & Details**:

```text
Loads active KSynth result with slot, rate, one-shot flag, and loop offset.
Usage: k>w slot rate one_shot offset
```

---

### `ksynth_wave`

- **Alias**: `kw`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `ksynth_wave()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `KSYNTH`
- **Summary**: ksynth wave trigger

**Description & Details**:

```text
Triggers KSynth wavetable generation.
Usage: kw
```

#### Clause 2: `ksynth_wave(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `KSYNTH`
- **Summary**: ksynth wave trigger with argument

**Description & Details**:

```text
Triggers KSynth wavetable generation with argument.
Usage: kw arg
```

---

### `ksynth_wave_export`

- **Alias**: `kw>`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `ksynth_wave_export()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `KSYNTH`
- **Summary**: export KSynth result to data buffer

**Description & Details**:

```text
Copies active KSynth expression result into the data buffer.
Usage: kw>
```

#### Clause 2: `ksynth_wave_export(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `KSYNTH`
- **Summary**: export KSynth result to data buffer

**Description & Details**:

```text
Copies active KSynth expression result into data buffer with argument.
Usage: kw> arg
```

---

### `log_enable`

- **Alias**: `log`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `log_enable(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: enable or disable logging

**Description & Details**:

```text
Enables or disables console/buffer logging (1=enable, 0=disable).
Usage: 1 log
```

#### Clause 2: `log_enable()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: log enable no-op

**Description & Details**:

```text
Log enable no-op when no args provided.
```

---

### `macro_clear_all`

- **Alias**: `/m!`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `macro_clear_all()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: clear all defined user macros

**Description & Details**:

```text
Deletes all registered user macros from memory.
Usage: /m!
```

---

### `macro_delete`

- **Alias**: `/m`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `macro_delete()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: delete defined user macro

**Description & Details**:

```text
Removes user macro definition specified by string bracket.
Usage: [name] /m
```

---

### `macro_exec`

- **Alias**: `e!`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `macro_exec()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `string`
- **Summary**: execute bracketed string immediately

**Description & Details**:

```text
Executes the currently bracketed program string.
Usage: [program] e! (or [program] macro_exec)
```

#### Clause 2: `macro_exec(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: execute numbered external macro slot

**Description & Details**:

```text
Executes the code stored in external macro slot N.
Usage: e! slot (or macro_exec slot)
```

---

### `macro_list_query`

- **Alias**: `?m`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `macro_list_query()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: list all defined user macros

**Description & Details**:

```text
Prints list of all defined user macros with RT capability status.
Usage: ?m
```

---

### `macro_load`

- **Alias**: `<e`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `macro_load(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `string`
- **Summary**: load external macro into string buffer

**Description & Details**:

```text
Loads external macro slot string into the active parser string buffer.
Usage: <e slot (or macro_load slot)
```

---

### `macro_query`

- **Alias**: `e?`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `macro_query(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: print contents of external macro slot

**Description & Details**:

```text
Prints the program string stored in external macro slot N.
Usage: e? slot (or macro_query slot)
```

---

### `macro_store`

- **Alias**: `e>`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `macro_store(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `string`
- **Summary**: store bracketed string into external macro slot

**Description & Details**:

```text
Stores bracketed code string into numbered macro memory slot (0..127).
Usage: [program] e>slot (or [program] macro_store slot)
```

---

### `map_exec`

- **Alias**: `map!`
- **Group**: `sys`
- **Clause Count**: 3

#### Clause 1: `map_exec(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: map iteration over range 0..count-1

**Description & Details**:

```text
Executes template command string iterating from 0 to count-1.
Usage: [template] count map!
```

#### Clause 2: `map_exec(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: map iteration over range start..end

**Description & Details**:

```text
Executes template command string iterating from start to end (step +/-1).
Usage: [template] start end map!
```

#### Clause 3: `map_exec(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: map iteration with custom step

**Description & Details**:

```text
Executes template command string iterating from start to end with step size.
Usage: [template] start end step map!
```

---

### `midi_bridge_bind`

- **Alias**: `/mb`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_bridge_bind(finite, any, any)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Parser State Requirements**: `string`
- **Summary**: bind MIDI event to Skode command

**Description & Details**:

```text
Binds MIDI message (type, channel, data1) to execute Skode code string.
Usage: [code] /mb type channel data1
```

---

### `midi_bridge_clear`

- **Alias**: `/mbC`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_bridge_clear()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: clear all MIDI Skode bindings

**Description & Details**:

```text
Clears all active MIDI-to-Skode command bindings.
Usage: /mbC
```

---

### `midi_bridge_query`

- **Alias**: `/mb?`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_bridge_query()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: query MIDI Skode bindings

**Description & Details**:

```text
Prints all active MIDI-to-Skode command bindings.
Usage: /mb?
```

---

### `midi_bridge_unbind`

- **Alias**: `/mbd`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_bridge_unbind(finite, any, any)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: remove MIDI Skode binding

**Description & Details**:

```text
Removes MIDI Skode binding matching type, channel, and data1.
Usage: /mbd type channel data1
```

---

### `midi_debug`

- **Alias**: `/md`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `midi_debug()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: toggle midi debug logging

**Description & Details**:

```text
Toggles MIDI debug logging.
Usage: /md
```

#### Clause 2: `midi_debug(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: set midi debug logging

**Description & Details**:

```text
Sets MIDI debug logging (0=off, 1=on).
Usage: /md 1
```

---

### `midi_file_dump`

- **Alias**: `/mfD`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_file_dump(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: dump MIDI file events

**Description & Details**:

```text
Dumps events from MIDI player slot starting at event index up to limit.
Usage: /mfD slot start limit
```

---

### `midi_file_load`

- **Alias**: `/mf`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_file_load(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Parser State Requirements**: `string`
- **Summary**: load MIDI file into slot

**Description & Details**:

```text
Loads a standard MIDI file (.mid) from bracket string into player slot (0..3).
Usage: [file.mid] /mf slot
```

---

### `midi_file_play`

- **Alias**: `/mf>`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_file_play(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: play MIDI file slot

**Description & Details**:

```text
Starts playback of loaded MIDI file in slot (0..3).
Usage: /mf> slot
```

---

### `midi_file_seek`

- **Alias**: `/mfP`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_file_seek(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: seek MIDI file playback position

**Description & Details**:

```text
Seeks playback of MIDI file slot to tick position.
Usage: /mfP slot tick
```

---

### `midi_file_status`

- **Alias**: `/mf?`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `midi_file_status()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: query MIDI player slots status

**Description & Details**:

```text
Prints playback status of all MIDI player slots (0..3).
Usage: /mf?
```

#### Clause 2: `midi_file_status(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: query single MIDI player slot status

**Description & Details**:

```text
Prints playback status of specified MIDI player slot.
Usage: /mf? slot
```

---

### `midi_file_stop`

- **Alias**: `/mf<`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_file_stop(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: stop MIDI file playback

**Description & Details**:

```text
Stops playback of MIDI file in slot (0..3).
Usage: /mf< slot
```

---

### `midi_file_sync`

- **Alias**: `/mfS`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_file_sync(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: set MIDI file sync mode

**Description & Details**:

```text
Sets tempo synchronization mode for player slot (0=free, 1=sync to sequencer BPM).
Usage: /mfS slot mode
```

---

### `midi_input`

- **Alias**: `/mi`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_input(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: open MIDI input port

**Description & Details**:

```text
Opens the specified MIDI input port by index.
Usage: /mi port
```

---

### `midi_input_close`

- **Alias**: `/mic`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_input_close()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: close MIDI input port

**Description & Details**:

```text
Closes active MIDI input port.
Usage: /mic
```

---

### `midi_input_virt`

- **Alias**: `/miV`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_input_virt()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Parser State Requirements**: `string`
- **Summary**: open virtual MIDI input port

**Description & Details**:

```text
Opens a virtual MIDI input port with name from string bracket.
Usage: [name] /miV
```

---

### `midi_list`

- **Alias**: `/mL`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_list()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: list MIDI devices

**Description & Details**:

```text
Enumerates available MIDI input and output ports.
Usage: /mL (or /mls)
```

---

### `midi_list_ls`

- **Alias**: `/mls`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_list_ls()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: list MIDI devices (alias)

**Description & Details**:

```text
Enumerates available MIDI input and output ports.
Usage: /mls
```

---

### `midi_out`

- **Alias**: `MO`
- **Group**: `sys`
- **Clause Count**: 3

#### Clause 1: `midi_out(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: send raw MIDI byte

**Description & Details**:

```text
Sends 1 raw MIDI status byte to open MIDI output port.
Usage: MO byte
```

#### Clause 2: `midi_out(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: send 2 raw MIDI bytes

**Description & Details**:

```text
Sends 2 raw MIDI bytes (e.g. Program Change, Channel Pressure) to open MIDI output port.
Usage: MO byte1,byte2
```

#### Clause 3: `midi_out(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: send 3 raw MIDI bytes

**Description & Details**:

```text
Sends 3 raw MIDI bytes (e.g. Note On, Note Off, CC) to open MIDI output port.
Usage: MO byte1,byte2,byte3
```

---

### `midi_output`

- **Alias**: `/mo`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_output(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: open MIDI output port

**Description & Details**:

```text
Opens the specified MIDI output port by index.
Usage: /mo port
```

---

### `midi_output_close`

- **Alias**: `/moc`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_output_close()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: close MIDI output port

**Description & Details**:

```text
Closes active MIDI output port.
Usage: /moc
```

---

### `midi_output_virt`

- **Alias**: `/moV`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_output_virt()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Parser State Requirements**: `string`
- **Summary**: open virtual MIDI output port

**Description & Details**:

```text
Opens a virtual MIDI output port with name from string bracket.
Usage: [name] /moV
```

---

### `midi_pool_route`

- **Alias**: `/mp`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `midi_pool_route(any, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: route MIDI channel to polyphony pool

**Description & Details**:

```text
Routes MIDI channel (-1 or nan for any) to a polyphony voice pool.
Usage: /mp channel pool
```

#### Clause 2: `midi_pool_route(any, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: route MIDI channel to polyphony pool with pitch bend range

**Description & Details**:

```text
Routes MIDI channel (-1 or nan for any) to a polyphony voice pool with semitone bend range.
Usage: /mp channel pool bend
```

---

### `midi_pool_unroute`

- **Alias**: `/mpd`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_pool_unroute(any, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: remove MIDI pool route

**Description & Details**:

```text
Removes MIDI pool route matching channel and target pool.
Usage: /mpd channel pool
```

---

### `midi_query`

- **Alias**: `/m?`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_query()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: query MIDI device status

**Description & Details**:

```text
Prints status of open MIDI input and output ports.
Usage: /m?
```

---

### `midi_route_clear`

- **Alias**: `/mC`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_route_clear()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: clear all MIDI routes

**Description & Details**:

```text
Removes all installed MIDI routes.
Usage: /mC
```

---

### `midi_route_query`

- **Alias**: `/mR`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_route_query()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: query active MIDI routes

**Description & Details**:

```text
Prints all currently installed MIDI channel routes.
Usage: /mR
```

---

### `midi_to_freq`

- **Alias**: `m>f`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_to_freq()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Parser State Requirements**: `data`
- **Summary**: convert MIDI note numbers in data array to Hz

**Description & Details**:

```text
Converts all values in the active data array from MIDI notes to frequencies in Hz.
Usage: m>f (or midi_to_freq)
```

---

### `midi_voice_route`

- **Alias**: `/mv`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `midi_voice_route(any, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: route MIDI channel to voice

**Description & Details**:

```text
Routes MIDI channel (-1 or nan for any) to a synth voice.
Usage: /mv channel voice
```

#### Clause 2: `midi_voice_route(any, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: route MIDI channel to voice with pitch bend range

**Description & Details**:

```text
Routes MIDI channel (-1 or nan for any) to a synth voice with semitone bend range.
Usage: /mv channel voice bend
```

---

### `midi_voice_unroute`

- **Alias**: `/mvd`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `midi_voice_unroute(any, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SKRED_MIDI_ENABLED`
- **Summary**: remove MIDI voice route

**Description & Details**:

```text
Removes MIDI voice route matching channel and target voice.
Usage: /mvd channel voice
```

---

### `notes_to_data`

- **Alias**: `n>d`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `notes_to_data()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: parse musical note string into data array

**Description & Details**:

```text
Parses note name string (e.g. [C4 E4 G4]) and writes MIDI note numbers to data buffer.
Usage: [notes] n>d
```

---

### `opcode_exec`

- **Alias**: `EXEC`
- **Group**: `sys`
- **Clause Count**: 5

#### Clause 1: `opcode_exec(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: numeric opcode escape

**Description & Details**:

```text
Directly executes a numeric opcode for CLAP host automation bridging.
Usage: EXEC opcode_id [args...]
```

#### Clause 2: `opcode_exec(finite, any)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: opcode_exec (EXEC)

**Description & Details**:

```text
opcode_exec (EXEC)
```

#### Clause 3: `opcode_exec(finite, any, any)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: opcode_exec (EXEC)

**Description & Details**:

```text
opcode_exec (EXEC)
```

#### Clause 4: `opcode_exec(finite, any, any, any)`

- **Arity**: `4`
- **Execution Model**: `immediate (control plane)`
- **Summary**: opcode_exec (EXEC)

**Description & Details**:

```text
opcode_exec (EXEC)
```

#### Clause 5: `opcode_exec(finite, any, any, any, any)`

- **Arity**: `5`
- **Execution Model**: `immediate (control plane)`
- **Summary**: opcode_exec (EXEC)

**Description & Details**:

```text
opcode_exec (EXEC)
```

---

### `performance_reset`

- **Alias**: `/th!`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `performance_reset()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: reset performance counters

**Description & Details**:

```text
Resets performance monitor counters.
Usage: /th!
```

#### Clause 2: `performance_reset(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: reset performance counters with argument

**Description & Details**:

```text
Resets performance monitor counters.
```

---

### `poly_group`

- **Alias**: `/pg`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `poly_group(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: configure polyphony group

**Description & Details**:

```text
Defines voice expansion group (group, source voice, width).
Usage: /pg group source width
```

#### Clause 2: `poly_group(finite, finite, finite, finite)`

- **Arity**: `4`
- **Execution Model**: `immediate (control plane)`
- **Summary**: configure polyphony group with root offset

**Description & Details**:

```text
Defines voice expansion group with root voice offset.
Usage: /pg group source width root
```

---

### `poly_group_query`

- **Alias**: `?pg`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `poly_group_query()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query all polyphony groups

**Description & Details**:

```text
Prints status of all configured polyphony groups.
Usage: ?pg
```

#### Clause 2: `poly_group_query(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query single polyphony group

**Description & Details**:

```text
Prints status of specified polyphony group.
Usage: ?pg group
```

---

### `poly_group_reset`

- **Alias**: `/pg!`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `poly_group_reset(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: refresh polyphony group

**Description & Details**:

```text
Refreshes polyphony group configuration and clones parameters.
Usage: /pg! group
```

---

### `poly_mode`

- **Alias**: `/pm`
- **Group**: `sys`
- **Clause Count**: 3

#### Clause 1: `poly_mode(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set polyphonic voice pool mode

**Description & Details**:

```text
Sets allocation mode for polyphonic pool (0=poly, 1=mono, 2=legato).
Usage: /pm pool mode
```

#### Clause 2: `poly_mode(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set polyphonic pool mode and note priority

**Description & Details**:

```text
Sets allocation mode and note priority (0=last, 1=low, 2=high).
Usage: /pm pool mode priority
```

#### Clause 3: `poly_mode(finite, finite, finite, finite)`

- **Arity**: `4`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set polyphonic pool mode, priority, and articulation

**Description & Details**:

```text
Sets allocation mode, priority, and articulation (0=retrigger, 1=legato).
Usage: /pm pool mode priority articulation
```

---

### `poly_pool`

- **Alias**: `/pp`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `poly_pool(finite, finite, finite, finite)`

- **Arity**: `4`
- **Execution Model**: `immediate (control plane)`
- **Summary**: configure polyphonic voice pool

**Description & Details**:

```text
Configures a dynamic polyphonic allocation pool (pool, group, base, count).
Usage: /pp pool group base count
```

#### Clause 2: `poly_pool(finite, finite, finite, finite, finite)`

- **Arity**: `5`
- **Execution Model**: `immediate (control plane)`
- **Summary**: configure polyphonic voice pool with steal policy

**Description & Details**:

```text
Configures polyphonic allocation pool with custom voice stealing policy.
Usage: /pp pool group base count policy
```

---

### `poly_pool_query`

- **Alias**: `?pp`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `poly_pool_query()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query all polyphonic voice pools

**Description & Details**:

```text
Prints status of all configured polyphonic voice pools.
Usage: ?pp
```

#### Clause 2: `poly_pool_query(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query single polyphonic voice pool

**Description & Details**:

```text
Prints status of specified polyphonic voice pool.
Usage: ?pp pool
```

---

### `poly_pool_reset`

- **Alias**: `/pp!`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `poly_pool_reset(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: refresh polyphony pool

**Description & Details**:

```text
Refreshes polyphonic voice pool members and state.
Usage: /pp! pool
```

---

### `query_pattern`

- **Alias**: `?p`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `query_pattern()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query active pattern index

**Description & Details**:

```text
Pushes active pattern index to return register.
Usage: ?p
```

#### Clause 2: `query_pattern(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query active pattern index with argument

**Description & Details**:

```text
Pushes active pattern index to return register.
```

---

### `query_step`

- **Alias**: `?st`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `query_step()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query active step index

**Description & Details**:

```text
Pushes active sequence step index to return register.
Usage: ?st
```

#### Clause 2: `query_step(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query active step index with argument

**Description & Details**:

```text
Pushes active sequence step index to return register.
```

---

### `query_string`

- **Alias**: `?s`
- **Group**: `sys`
- **Clause Count**: 3

#### Clause 1: `query_string()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `string`
- **Summary**: print current string argument

**Description & Details**:

```text
Prints the string token currently stored in the parser buffer.
Usage: ?s
```

#### Clause 2: `query_string(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `string`
- **Summary**: print current string argument

**Description & Details**:

```text
Prints current string argument, ignoring argument stack.
```

#### Clause 3: `query_string(any, any)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `string`
- **Summary**: print current string argument

**Description & Details**:

```text
Prints current string argument, ignoring argument stack.
```

---

### `query_time`

- **Alias**: `?t`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `query_time()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query current sample time

**Description & Details**:

```text
Pushes current engine sample count to return register.
Usage: ?t
```

#### Clause 2: `query_time(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query current sample time with argument

**Description & Details**:

```text
Pushes current engine sample count to return register.
```

---

### `query_voice`

- **Alias**: `?v`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `query_voice()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query active voice index

**Description & Details**:

```text
Pushes active voice index to return register.
Usage: ?v
```

#### Clause 2: `query_voice(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query active voice index with argument

**Description & Details**:

```text
Pushes active voice index to return register.
```

---

### `rec_to_data`

- **Alias**: `r>d`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `rec_to_data()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `data`
- **Summary**: copy recording buffer to data array

**Description & Details**:

```text
Copies completed audio recording into active data array.
Usage: r>d (or rec_to_data)
```

#### Clause 2: `rec_to_data(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Parser State Requirements**: `data`
- **Summary**: copy recording channel to data array

**Description & Details**:

```text
Copies target channel from audio recording into active data array.
Usage: r>d channel
```

---

### `return_push`

- **Alias**: `*R`
- **Group**: `sys`
- **Clause Count**: 9

#### Clause 1: `return_push()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: reset or clear return registers

**Description & Details**:

```text
Pushes arguments into parser return registers @0 through @7.
Usage: *R [arg0] [arg1] ...
```

#### Clause 2: `return_push(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: push 1 argument into return registers

**Description & Details**:

```text
Pushes 1 argument into return register @0.
```

#### Clause 3: `return_push(any, any)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: push 2 arguments into return registers

**Description & Details**:

```text
Pushes 2 arguments into return registers @0 and @1.
```

#### Clause 4: `return_push(any, any, any)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: push 3 arguments into return registers

**Description & Details**:

```text
Pushes 3 arguments into return registers @0 through @2.
```

#### Clause 5: `return_push(any, any, any, any)`

- **Arity**: `4`
- **Execution Model**: `immediate (control plane)`
- **Summary**: push 4 arguments into return registers

**Description & Details**:

```text
Pushes 4 arguments into return registers @0 through @3.
```

#### Clause 6: `return_push(any, any, any, any, any)`

- **Arity**: `5`
- **Execution Model**: `immediate (control plane)`
- **Summary**: push 5 arguments into return registers

**Description & Details**:

```text
Pushes 5 arguments into return registers @0 through @4.
```

#### Clause 7: `return_push(any, any, any, any, any, any)`

- **Arity**: `6`
- **Execution Model**: `immediate (control plane)`
- **Summary**: push 6 arguments into return registers

**Description & Details**:

```text
Pushes 6 arguments into return registers @0 through @5.
```

#### Clause 8: `return_push(any, any, any, any, any, any, any)`

- **Arity**: `7`
- **Execution Model**: `immediate (control plane)`
- **Summary**: push 7 arguments into return registers

**Description & Details**:

```text
Pushes 7 arguments into return registers @0 through @6.
```

#### Clause 9: `return_push(any, any, any, any, any, any, any, any)`

- **Arity**: `8`
- **Execution Model**: `immediate (control plane)`
- **Summary**: push 8 arguments into return registers

**Description & Details**:

```text
Pushes 8 arguments into return registers @0 through @7.
```

---

### `return_query`

- **Alias**: `?R`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `return_query()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: show return registers without consuming them

**Description & Details**:

```text
Displays current saved return registers (@0 through @9).
Usage: ?R
```

---

### `scope_event`

- **Alias**: `/sm`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `scope_event()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SCOPE`
- **Parser State Requirements**: `string`
- **Summary**: publish scope message event

**Description & Details**:

```text
Publishes a text event marker into the scope IPC stream.
Usage: [string] /sm
```

---

### `scope_start`

- **Alias**: `/sg`
- **Group**: `sys`
- **Clause Count**: 3

#### Clause 1: `scope_start()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SCOPE`
- **Parser State Requirements**: `string`
- **Summary**: start scope IPC streaming

**Description & Details**:

```text
Starts scope IPC streaming using default channel mask and buffer duration.
Usage: /sg
```

#### Clause 2: `scope_start(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SCOPE`
- **Parser State Requirements**: `string`
- **Summary**: start scope IPC streaming with channel mask

**Description & Details**:

```text
Starts scope IPC streaming with specified channel mask.
Usage: /sg mask
```

#### Clause 3: `scope_start(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SCOPE`
- **Parser State Requirements**: `string`
- **Summary**: start scope IPC streaming with channel mask and seconds

**Description & Details**:

```text
Starts scope IPC streaming with specified channel mask and buffer duration.
Usage: /sg mask seconds
```

---

### `scope_status`

- **Alias**: `/s?`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `scope_status()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SCOPE`
- **Summary**: query scope IPC status

**Description & Details**:

```text
Prints current status of scope IPC ring buffer.
Usage: /s?
```

---

### `scope_stop`

- **Alias**: `/ss`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `scope_stop()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `SCOPE`
- **Summary**: stop scope IPC streaming

**Description & Details**:

```text
Stops scope IPC streaming.
Usage: /ss
```

---

### `session_load`

- **Alias**: `GS<`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `session_load()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load global session from zip

**Description & Details**:

```text
Loads global session archive from the filename in parser string buffer.
Usage: [session.zip] GS<
```

#### Clause 2: `session_load(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load global session from zip with argument

**Description & Details**:

```text
Loads global session archive.
```

---

### `session_save`

- **Alias**: `GS>`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `session_save()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: save global session to zip

**Description & Details**:

```text
Saves global session archive to the filename in parser string buffer.
Usage: [session.zip] GS>
```

#### Clause 2: `session_save(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: save global session to zip with argument

**Description & Details**:

```text
Saves global session archive.
```

---

### `skode_load_file`

- **Alias**: `/l`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `skode_load_file(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load script file by index

**Description & Details**:

```text
Loads script file by numeric index into active voice.
Usage: /l 0
```

#### Clause 2: `skode_load_file(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load script file by index with verbosity

**Description & Details**:

```text
Loads script file by numeric index with verbosity flag.
Usage: /l 0 1
```

---

### `skode_load_name`

- **Alias**: `/ls`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `skode_load_name()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load script file by name

**Description & Details**:

```text
Loads script file specified in string buffer.
Usage: [script.sk] /ls
```

#### Clause 2: `skode_load_name(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load script file by name with verbosity

**Description & Details**:

```text
Loads script file specified in string buffer with verbosity flag.
Usage: [script.sk] /ls 1
```

---

### `sleep_wait`

- **Alias**: `wait`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `sleep_wait(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: sleep for ms

**Description & Details**:

```text
Sleeps execution for specified milliseconds.
Usage: wait 100
```

#### Clause 2: `sleep_wait()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: sleep wait no-op

**Description & Details**:

```text
Sleep wait no-op.
Usage: wait
```

---

### `stack_clear`

- **Alias**: `clr`
- **Group**: `sys`
- **Clause Count**: 5

#### Clause 1: `stack_clear()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: clear argument stack

**Description & Details**:

```text
Clears all arguments from the parser stack.
Usage: clr
```

#### Clause 2: `stack_clear(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: clear argument stack

**Description & Details**:

```text
Clears all arguments from the parser stack.
Usage: 1 clr
```

#### Clause 3: `stack_clear(any, any)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: clear argument stack

**Description & Details**:

```text
Clears all arguments from the parser stack.
```

#### Clause 4: `stack_clear(any, any, any)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: clear argument stack

**Description & Details**:

```text
Clears all arguments from the parser stack.
```

#### Clause 5: `stack_clear(any, any, any, any)`

- **Arity**: `4`
- **Execution Model**: `immediate (control plane)`
- **Summary**: clear argument stack

**Description & Details**:

```text
Clears all arguments from the parser stack.
```

---

### `stack_drop`

- **Alias**: `drop`
- **Group**: `sys`
- **Clause Count**: 5

#### Clause 1: `stack_drop(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: drop argument from stack

**Description & Details**:

```text
Drops top argument from the parser stack.
Usage: 1 drop
```

#### Clause 2: `stack_drop()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: drop argument from stack

**Description & Details**:

```text
Drops argument from the parser stack.
```

#### Clause 3: `stack_drop(any, any)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: drop argument from stack

**Description & Details**:

```text
Drops top argument from parser stack.
```

#### Clause 4: `stack_drop(any, any, any)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: drop argument from stack

**Description & Details**:

```text
Drops top argument from parser stack.
```

#### Clause 5: `stack_drop(any, any, any, any)`

- **Arity**: `4`
- **Execution Model**: `immediate (control plane)`
- **Summary**: drop argument from stack

**Description & Details**:

```text
Drops top argument from parser stack.
```

---

### `stack_dup`

- **Alias**: `dup`
- **Group**: `sys`
- **Clause Count**: 5

#### Clause 1: `stack_dup(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: duplicate top stack argument

**Description & Details**:

```text
Duplicates top argument on the parser stack.
Usage: 1 dup
```

#### Clause 2: `stack_dup()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: duplicate top stack argument

**Description & Details**:

```text
Duplicates top argument on the parser stack.
```

#### Clause 3: `stack_dup(any, any)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: duplicate top stack argument

**Description & Details**:

```text
Duplicates top argument on parser stack.
```

#### Clause 4: `stack_dup(any, any, any)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: duplicate top stack argument

**Description & Details**:

```text
Duplicates top argument on parser stack.
```

#### Clause 5: `stack_dup(any, any, any, any)`

- **Arity**: `4`
- **Execution Model**: `immediate (control plane)`
- **Summary**: duplicate top stack argument

**Description & Details**:

```text
Duplicates top argument on parser stack.
```

---

### `stack_over`

- **Alias**: `over`
- **Group**: `sys`
- **Clause Count**: 5

#### Clause 1: `stack_over(any, any)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: duplicate second stack argument to top

**Description & Details**:

```text
Copies second argument to top of parser stack.
Usage: 1 2 over
```

#### Clause 2: `stack_over()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: duplicate second stack argument to top

**Description & Details**:

```text
Copies second argument to top of parser stack.
```

#### Clause 3: `stack_over(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: duplicate second stack argument to top

**Description & Details**:

```text
Copies second argument to top of parser stack.
```

#### Clause 4: `stack_over(any, any, any)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: duplicate second stack argument to top

**Description & Details**:

```text
Copies second argument to top of parser stack.
```

#### Clause 5: `stack_over(any, any, any, any)`

- **Arity**: `4`
- **Execution Model**: `immediate (control plane)`
- **Summary**: duplicate second stack argument to top

**Description & Details**:

```text
Copies second argument to top of parser stack.
```

---

### `stack_rot`

- **Alias**: `rot`
- **Group**: `sys`
- **Clause Count**: 5

#### Clause 1: `stack_rot(any, any, any)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: rotate top three stack arguments

**Description & Details**:

```text
Rotates top three parser arguments left.
Usage: 1 2 3 rot
```

#### Clause 2: `stack_rot()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: rotate top three stack arguments

**Description & Details**:

```text
Rotates top three parser arguments.
```

#### Clause 3: `stack_rot(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: rotate top three stack arguments

**Description & Details**:

```text
Rotates top three parser arguments.
```

#### Clause 4: `stack_rot(any, any)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: rotate top three stack arguments

**Description & Details**:

```text
Rotates top three parser arguments.
```

#### Clause 5: `stack_rot(any, any, any, any)`

- **Arity**: `4`
- **Execution Model**: `immediate (control plane)`
- **Summary**: rotate top three stack arguments

**Description & Details**:

```text
Rotates top three parser arguments.
```

---

### `stack_swap`

- **Alias**: `swap`
- **Group**: `sys`
- **Clause Count**: 5

#### Clause 1: `stack_swap(any, any)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: swap top two stack arguments

**Description & Details**:

```text
Swaps the top two arguments on the parser stack.
Usage: 1 2 swap
```

#### Clause 2: `stack_swap()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: swap top two stack arguments

**Description & Details**:

```text
Swaps top two arguments on parser stack.
```

#### Clause 3: `stack_swap(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: swap top two stack arguments

**Description & Details**:

```text
Swaps top two arguments on parser stack.
```

#### Clause 4: `stack_swap(any, any, any)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: swap top two stack arguments

**Description & Details**:

```text
Swaps top two arguments on parser stack.
```

#### Clause 5: `stack_swap(any, any, any, any)`

- **Arity**: `4`
- **Execution Model**: `immediate (control plane)`
- **Summary**: swap top two stack arguments

**Description & Details**:

```text
Swaps top two arguments on parser stack.
```

---

### `step_program_set`

- **Alias**: `x`
- **Group**: `sys`
- **Clause Count**: 8

#### Clause 1: `step_program_set(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set pattern step program

**Description & Details**:

```text
Sets program for specified pattern step from string buffer.
Usage: [v0 n60] 0 x
```

#### Clause 2: `step_program_set(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set pattern step program for two steps

**Description & Details**:

```text
Sets program for specified pattern steps from string buffer.
Usage: [v0 n60] 0 4 x
```

#### Clause 3: `step_program_set(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set pattern step program for three steps

**Description & Details**:

```text
Sets program for specified pattern steps from string buffer.
```

#### Clause 4: `step_program_set(finite, finite, finite, finite)`

- **Arity**: `4`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set pattern step program for four steps

**Description & Details**:

```text
Sets program for specified pattern steps from string buffer.
```

#### Clause 5: `step_program_set(finite, finite, finite, finite, finite)`

- **Arity**: `5`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set pattern step program for five steps

**Description & Details**:

```text
Sets program for specified pattern steps from string buffer.
```

#### Clause 6: `step_program_set(finite, finite, finite, finite, finite, finite)`

- **Arity**: `6`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set pattern step program for six steps

**Description & Details**:

```text
Sets program for specified pattern steps from string buffer.
```

#### Clause 7: `step_program_set(finite, finite, finite, finite, finite, finite, finite)`

- **Arity**: `7`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set pattern step program for seven steps

**Description & Details**:

```text
Sets program for specified pattern steps from string buffer.
```

#### Clause 8: `step_program_set(finite, finite, finite, finite, finite, finite, finite, finite)`

- **Arity**: `8`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set pattern step program for eight steps

**Description & Details**:

```text
Sets program for specified pattern steps from string buffer.
```

---

### `stop`

- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `stop()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: stop audio playback and halt sequencer

**Description & Details**:

```text
Stops playback, halts all sequencer patterns, and mutes sounding voices.
Usage: stop
```

---

### `string_format`

- **Alias**: `s%`
- **Group**: `sys`
- **Clause Count**: 5

#### Clause 1: `string_format()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: format string template with arguments

**Description & Details**:

```text
Substitutes format placeholders (@0, @1...) in string with stack arguments.
Usage: [template] s%
```

#### Clause 2: `string_format(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: format string template with 1 argument

**Description & Details**:

```text
Substitutes @0 in template string with stack argument.
Usage: [template] arg0 s%
```

#### Clause 3: `string_format(any, any)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: format string template with 2 arguments

**Description & Details**:

```text
Substitutes @0, @1 in template string with stack arguments.
Usage: [template] arg0 arg1 s%
```

#### Clause 4: `string_format(any, any, any)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: format string template with 3 arguments

**Description & Details**:

```text
Substitutes @0, @1, @2 in template string with stack arguments.
Usage: [template] arg0 arg1 arg2 s%
```

#### Clause 5: `string_format(any, any, any, any)`

- **Arity**: `4`
- **Execution Model**: `immediate (control plane)`
- **Summary**: format string template with 4 arguments

**Description & Details**:

```text
Substitutes @0, @1, @2, @3 in template string with stack arguments.
Usage: [template] arg0 arg1 arg2 arg3 s%
```

---

### `string_puts`

- **Alias**: `puts`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `string_puts()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: print parser string buffer

**Description & Details**:

```text
Prints the current string buffer to log/stdout.
Usage: [hello] puts
```

#### Clause 2: `string_puts(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: print parser string buffer with argument

**Description & Details**:

```text
Prints the current string buffer to log/stdout.
```

---

### `string_slot_load`

- **Alias**: `<s`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `string_slot_load(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load string from memory slot into parser buffer

**Description & Details**:

```text
Loads string from slot (0..15) into the active parser string buffer.
Usage: <s slot
```

---

### `string_slot_query`

- **Alias**: `s?`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `string_slot_query()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query all string memory slots

**Description & Details**:

```text
Prints contents of all non-empty string memory slots (0..15).
Usage: s?
```

#### Clause 2: `string_slot_query(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query single string memory slot

**Description & Details**:

```text
Prints contents of specified string memory slot.
Usage: s? slot
```

---

### `string_slot_store`

- **Alias**: `s>`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `string_slot_store(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: save string from parser buffer into memory slot

**Description & Details**:

```text
Stores string from active parser buffer into slot (0..15).
Usage: [string] s> slot
```

---

### `system_show`

- **Alias**: `/s`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `system_show()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: show system status

**Description & Details**:

```text
Displays system status overview.
Usage: /s
```

#### Clause 2: `system_show(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: show subsystem status

**Description & Details**:

```text
Displays specific subsystem status (0=system, 1=threads, 2=audio, 3=synth, 4=stats, 5=skode, 6=seq, 7=strings).
Usage: /s 2
```

---

### `thread_status`

- **Alias**: `/th?`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `thread_status()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query thread status

**Description & Details**:

```text
Queries thread status and diagnostics.
Usage: /th?
```

#### Clause 2: `thread_status(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query thread status with argument

**Description & Details**:

```text
Queries thread status and diagnostics.
```

---

### `timestamp_logger`

- **Alias**: `I`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `timestamp_logger()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: timestamp logger

**Description & Details**:

```text
Timestamp logger toggle.
```

#### Clause 2: `timestamp_logger(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: timestamp logger with argument

**Description & Details**:

```text
Timestamp logger toggle with argument.
```

---

### `trace_toggle`

- **Alias**: `/t`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `trace_toggle()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: toggle parser trace

**Description & Details**:

```text
Toggles trace execution logging.
Usage: /t
```

#### Clause 2: `trace_toggle(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set parser trace level

**Description & Details**:

```text
Sets parser trace level (0=off, 1=on, 2=verbose).
Usage: /t 1
```

---

### `udp_emit`

- **Alias**: `>u`
- **Group**: `sys`
- **Clause Count**: 5

#### Clause 1: `udp_emit()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `UDP`
- **Summary**: emit formatted udp event

**Description & Details**:

```text
Emits UDP message formatted according to string buffer.
Usage: [event %g %g] >u
```

#### Clause 2: `udp_emit(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `UDP`
- **Summary**: emit formatted udp event with 1 arg

**Description & Details**:

```text
Emits UDP message formatted with 1 argument.
Usage: [event %g] 123 >u
```

#### Clause 3: `udp_emit(any, any)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `UDP`
- **Summary**: emit formatted udp event with 2 args

**Description & Details**:

```text
Emits UDP message formatted with 2 arguments.
Usage: [event %g %g] 1 2 >u
```

#### Clause 4: `udp_emit(any, any, any)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `UDP`
- **Summary**: emit formatted udp event with 3 args

**Description & Details**:

```text
Emits UDP message formatted with 3 arguments.
Usage: [event %g %g %g] 1 2 3 >u
```

#### Clause 5: `udp_emit(any, any, any, any)`

- **Arity**: `4`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `UDP`
- **Summary**: emit formatted udp event with 4 args

**Description & Details**:

```text
Emits UDP message formatted with 4 arguments.
```

---

### `udp_info`

- **Alias**: `udp`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `udp_info()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `UDP`
- **Summary**: show udp status

**Description & Details**:

```text
Shows UDP network endpoint status.
Usage: udp
```

#### Clause 2: `udp_info(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `UDP`
- **Summary**: show udp status with argument

**Description & Details**:

```text
Shows UDP network endpoint status with argument.
```

---

### `var_add`

- **Alias**: `a=`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `var_add(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: add two values and store in variable

**Description & Details**:

```text
Stores (v1 + v2) into variable register $var.
Usage: a= var v1 v2
```

---

### `var_div`

- **Alias**: `/=`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `var_div(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: divide two values and store in variable

**Description & Details**:

```text
Stores (v1 / v2) into variable register $var.
Usage: /= var v1 v2
```

---

### `var_from_data`

- **Alias**: `=d`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `var_from_data(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: copy data array element to variable

**Description & Details**:

```text
Copies value from data buffer at index into variable register $var.
Usage: =d var index
```

---

### `var_mul`

- **Alias**: `*=`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `var_mul(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: multiply two values and store in variable

**Description & Details**:

```text
Stores (v1 * v2) into variable register $var.
Usage: *= var v1 v2
```

---

### `var_set`

- **Alias**: `=`
- **Group**: `sys`
- **Clause Count**: 3

#### Clause 1: `var_set()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: print all non-zero variables

**Description & Details**:

```text
Prints all non-zero global variable registers ($0..$127).
Usage: = (or var_set)
```

#### Clause 2: `var_set(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: query variable value

**Description & Details**:

```text
Prints value of variable $N and pushes onto argument stack.
Usage: =var (or var_set var)
```

#### Clause 3: `var_set(finite, any)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set variable value

**Description & Details**:

```text
Stores value into variable register $var.
Usage: =var,val (or var_set var val)
```

---

### `var_sub`

- **Alias**: `s=`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `var_sub(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: subtract two values and store in variable

**Description & Details**:

```text
Stores (v1 - v2) into variable register $var.
Usage: s= var v1 v2
```

---

### `verbose_toggle`

- **Alias**: `/v`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `verbose_toggle()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: toggle verbose output

**Description & Details**:

```text
Toggles verbose diagnostics output.
Usage: /v
```

#### Clause 2: `verbose_toggle(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: set verbose level

**Description & Details**:

```text
Sets verbose diagnostics level (0=off, 1=on).
Usage: /v 1
```

---

### `vfs_cat`

- **Alias**: `%cat`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `vfs_cat()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `!defined(SKRED_NO_VFS)`
- **Summary**: print file contents from vfs

**Description & Details**:

```text
Prints contents of file specified in string buffer.
Usage: [file.txt] %cat
```

#### Clause 2: `vfs_cat(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `!defined(SKRED_NO_VFS)`
- **Summary**: print file contents from vfs with argument

**Description & Details**:

```text
Prints contents of file.
```

---

### `vfs_cd`

- **Alias**: `%cd`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `vfs_cd()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `!defined(SKRED_NO_VFS)`
- **Summary**: change vfs directory

**Description & Details**:

```text
Changes current working directory to path in string buffer.
Usage: [subdir] %cd
```

#### Clause 2: `vfs_cd(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `!defined(SKRED_NO_VFS)`
- **Summary**: change vfs directory with argument

**Description & Details**:

```text
Changes current working directory.
```

---

### `vfs_ls`

- **Alias**: `%ls`
- **Group**: `sys`
- **Clause Count**: 3

#### Clause 1: `vfs_ls()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `!defined(SKRED_NO_VFS)`
- **Summary**: list vfs directory

**Description & Details**:

```text
Lists files in current working directory.
Usage: %ls
```

#### Clause 2: `vfs_ls(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `!defined(SKRED_NO_VFS)`
- **Summary**: list vfs directory filtered by file type

**Description & Details**:

```text
Lists files filtered by file type code (0=.sk, 1=.wav, 2=.mp3, 3=.ks, 4=.flac, 5=.zip, 6=.pnl).
Usage: 1 %ls
```

#### Clause 3: `vfs_ls(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `!defined(SKRED_NO_VFS)`
- **Summary**: list vfs directory filtered by file type and index

**Description & Details**:

```text
Selects file by index into string buffer.
Usage: 1 0 %ls
```

---

### `vfs_mount`

- **Alias**: `%z`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `vfs_mount()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `!defined(SKRED_NO_VFS)`
- **Summary**: mount vfs directory or zip

**Description & Details**:

```text
Mounts zip archive or host directory from string buffer.
Usage: [bundle.zip] %z
```

#### Clause 2: `vfs_mount(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `!defined(SKRED_NO_VFS)`
- **Summary**: mount vfs directory or zip with argument

**Description & Details**:

```text
Mounts zip archive or host directory.
```

---

### `vfs_pwd`

- **Alias**: `%pwd`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `vfs_pwd()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `!defined(SKRED_NO_VFS)`
- **Summary**: print current vfs directory

**Description & Details**:

```text
Prints the current working directory in VFS.
Usage: %pwd
```

#### Clause 2: `vfs_pwd(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `!defined(SKRED_NO_VFS)`
- **Summary**: print current vfs directory with argument

**Description & Details**:

```text
Prints the current working directory in VFS.
```

---

### `vfs_unmount`

- **Alias**: `%zu`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `vfs_unmount()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `!defined(SKRED_NO_VFS)`
- **Summary**: unmount vfs

**Description & Details**:

```text
Unmounts active VFS bundle.
Usage: %zu
```

#### Clause 2: `vfs_unmount(any)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `!defined(SKRED_NO_VFS)`
- **Summary**: unmount vfs with argument

**Description & Details**:

```text
Unmounts active VFS bundle.
```

---

### `voice_bench`

- **Alias**: `/m_`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `voice_bench()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: benchmark synth voice

**Description & Details**:

```text
Runs benchmark on active synth voice.
Usage: /m_
```

#### Clause 2: `voice_bench(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: benchmark synth voice with argument

**Description & Details**:

```text
Runs benchmark on active synth voice.
```

---

### `voice_graph`

- **Alias**: `/vg`
- **Group**: `sys`
- **Clause Count**: 3

#### Clause 1: `voice_graph(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: inspect voice routing graph

**Description & Details**:

```text
Inspects audio and modulation routing tree for voice.
Usage: /vg voice
```

#### Clause 2: `voice_graph(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: inspect voice routing graph with format

**Description & Details**:

```text
Inspects voice graph with output format (0=text, 1=dot/mermaid).
Usage: /vg voice format
```

#### Clause 3: `voice_graph(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: inspect voice routing graph with format and depth

**Description & Details**:

```text
Inspects voice graph with format and traversal depth limit.
Usage: /vg voice format depth
```

---

### `voice_link_gate`

- **Alias**: `G`
- **Group**: `sys`
- **Clause Count**: 5

#### Clause 1: `voice_link_gate(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `261` (`skode_clause_voice_link_gate_1`)
- **Summary**: link gate to target voice

**Description & Details**:

```text
Links gate of active voice to follow target voice.
Usage: G 0
```

#### Clause 2: `voice_link_gate()`

- **Arity**: `0`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `262` (`skode_clause_voice_link_gate_0`)
- **Summary**: voice link gate clear

**Description & Details**:

```text
Clears gate links for active voice.
```

#### Clause 3: `voice_link_gate(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `263` (`skode_clause_voice_link_gate_2`)
- **Summary**: link gate to 2 target voices

**Description & Details**:

```text
Links gate to two target voices.
Usage: G v1 v2
```

#### Clause 4: `voice_link_gate(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `264` (`skode_clause_voice_link_gate_3`)
- **Summary**: link gate to 3 target voices

**Description & Details**:

```text
Links gate to 3 target voices.
```

#### Clause 5: `voice_link_gate(finite, finite, finite, finite)`

- **Arity**: `4`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `265` (`skode_clause_voice_link_gate_4`)
- **Summary**: link gate to 4 target voices

**Description & Details**:

```text
Links gate to 4 target voices.
```

---

### `voice_link_sync`

- **Alias**: `H`
- **Group**: `sys`
- **Clause Count**: 5

#### Clause 1: `voice_link_sync(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `266` (`skode_clause_voice_link_sync_1`)
- **Summary**: link velocity to target voice

**Description & Details**:

```text
Links velocity of active voice to follow target voice.
Usage: H 0
```

#### Clause 2: `voice_link_sync()`

- **Arity**: `0`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `267` (`skode_clause_voice_link_sync_0`)
- **Summary**: voice link velocity clear

**Description & Details**:

```text
Clears velocity links for active voice.
```

#### Clause 3: `voice_link_sync(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `268` (`skode_clause_voice_link_sync_2`)
- **Summary**: link velocity to 2 target voices

**Description & Details**:

```text
Links velocity to two target voices.
Usage: H v1 v2
```

#### Clause 4: `voice_link_sync(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `269` (`skode_clause_voice_link_sync_3`)
- **Summary**: link velocity to 3 target voices

**Description & Details**:

```text
Links velocity to 3 target voices.
```

#### Clause 5: `voice_link_sync(finite, finite, finite, finite)`

- **Arity**: `4`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `270` (`skode_clause_voice_link_sync_4`)
- **Summary**: link velocity to 4 target voices

**Description & Details**:

```text
Links velocity to 4 target voices.
```

---

### `voice_link_trig`

- **Alias**: `L`
- **Group**: `sys`
- **Clause Count**: 2

#### Clause 1: `voice_link_trig(finite)`

- **Arity**: `1`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `271` (`skode_clause_voice_link_trig_1`)
- **Summary**: set voice link trigger duration

**Description & Details**:

```text
Sets trigger re-arm time in seconds for linked voice events.
Usage: L 0.25
```

#### Clause 2: `voice_link_trig()`

- **Arity**: `0`
- **Execution Model**: `realtime (audio thread safe)`
- **Discrete Opcode ID**: `272` (`skode_clause_voice_link_trig_0`)
- **Summary**: voice link trigger duration reset

**Description & Details**:

```text
Resets trigger re-arm time.
```

---

### `wave_load_file`

- **Alias**: `/w`
- **Group**: `sys`
- **Clause Count**: 3

#### Clause 1: `wave_load_file(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load wave file by index

**Description & Details**:

```text
Loads wave file by index into default slot.
Usage: 0 /w
```

#### Clause 2: `wave_load_file(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load wave file by index into slot

**Description & Details**:

```text
Loads wave file by index into specified slot.
Usage: 0 1 /w
```

#### Clause 3: `wave_load_file(finite, finite, finite)`

- **Arity**: `3`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load wave file by index into slot and channel

**Description & Details**:

```text
Loads wave file by index into slot and channel.
Usage: 0 1 0 /w
```

---

### `wave_load_name`

- **Alias**: `/ws`
- **Group**: `sys`
- **Clause Count**: 3

#### Clause 1: `wave_load_name()`

- **Arity**: `0`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load wave file by name into default slot

**Description & Details**:

```text
Loads wave file named in string buffer into default slot.
Usage: [sample.wav] /ws
```

#### Clause 2: `wave_load_name(finite)`

- **Arity**: `1`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load wave file by name into slot

**Description & Details**:

```text
Loads wave file named in string buffer into specified slot.
Usage: [sample.wav] 0 /ws
```

#### Clause 3: `wave_load_name(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Summary**: load wave file by name into slot and channel

**Description & Details**:

```text
Loads wave file named in string buffer into specified slot and channel.
Usage: [sample.wav] 0 1 /ws
```

---

### `wave_to_ksynth`

- **Alias**: `w>k`
- **Group**: `sys`
- **Clause Count**: 1

#### Clause 1: `wave_to_ksynth(finite, finite)`

- **Arity**: `2`
- **Execution Model**: `immediate (control plane)`
- **Feature Flag (`#ifdef`)**: `KSYNTH`
- **Summary**: bind wavetable samples to KSynth variable

**Description & Details**:

```text
Reads wavetable slot audio samples and binds to KSynth variable index.
Usage: w>k wave var
```

---

