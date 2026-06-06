# SKODE Command Review

Recorded June 6, 2026.

This review evaluates the command set for internal consistency by inferring
intent from the implementation, canonical state output, documentation, and
existing patch files.

## High Priority

### `/r` reads the wrong offset argument

`/r slot one_shot offset` checks for three arguments but reads `arg[3]`, the
fourth argument, instead of `arg[2]`.

Relevant code:

- `parts/skode.c.kit:2092`

### `w>r` often performs no transfer

Wave-to-record only copies when the recording buffer must first be enlarged.
If the buffer already has sufficient capacity, its `valid` flag remains false
and nothing is copied. This conflicts with the more reliable `d>r` behavior.

Relevant code:

- `parts/skode.c.kit:1874`

### `x` cannot recognize its NaN/default argument

The `x` command compares `arg[0] == NAN`, which is always false. The apparent
intent is for a NaN placeholder such as `x-` or `x.` to advance to the next
step. This requires `isnan(arg[0])`.

Relevant code:

- `parts/skode.c.kit:2004`

## Medium Priority

### `S100` is an accidental reset-all sentinel

`S` appears to mean "reset the specified voice," but an invalid voice causes
all voices to reset. Documentation and patches use `S100` as "quiet/reset all,"
which depends on voice 100 remaining invalid.

A dedicated all-voices reset command would communicate this intention without
relying on an out-of-range value.

Relevant code:

- `parts/skode.c.kit:1783`
- `parts/synth.c.kit:1486`
- `doc/learn.html:314`

### `G` and `H` do not replace their link lists

`G5` changes only MIDI-link slot zero and leaves slots 1 through 3 untouched.
`H` behaves the same way for velocity links. These commands look like complete
list setters, and canonical voice output emits complete lists, so omitted
positions should probably reset to `-1`.

Relevant code:

- `parts/skode.c.kit:1525`
- `parts/skode.c.kit:1537`
- `parts/synth.c.kit:872`

### Voice serialization is not a complete round trip

`voice_format()` emits most selected-voice state, but it does not emit the
interpolation and one-shot arguments supported by `w`. Replaying `?` output can
therefore produce different playback behavior.

Relevant code:

- `parts/skode.c.kit:1814`
- `parts/synth.c.kit:849`

### Documented string syntax disagrees with the parser

The web quick reference recommends `{v0l1}e>0`, while the current ANDS parser
recognizes square-bracket strings only.

Relevant code:

- `doc/index.html:322`
- `parts/ands.c:310`

### Bare `wait` reads an argument that was not supplied

`wait` checks the `arg` pointer, which is always valid, instead of checking
`argc`. A bare `wait` can therefore read stale or uninitialized argument data.

Relevant code:

- `parts/skode.c.kit:1394`

## Low Priority

### Inspection syntax is inconsistent

Several commands follow a useful `?` convention:

- `?` and `v?`: show the selected voice
- `??` and `v??`: show active voices
- `?d`: show data
- `?s`: show the current string
- `k?`: show the last k-synth result
- `e?`: show stored executable strings

Other commands inspect state through unrelated forms:

- Bare `z` and `Z` show pattern state.
- Bare `D`, `/f`, and `/c` show their current values.
- `W` is both a wave query and waveform display command.
- `udp` only displays information when given an otherwise unused argument.

A more consistent `noun?` convention would make unfamiliar commands easier to
infer.

### Aliases and numeric sub-protocols increase cognitive load

Examples include:

- `D` and `/D` both resize data with different reporting behavior.
- `ks` and `k!` both execute k-synth source.
- `xg` and `>x` both jump to a pattern step.
- `^r` and `<r` both start recording into the sample buffer.
- `/s 0..7`, `W@` parameter numbers, and `v@` parameter numbers expose
  undocumented numeric command families.

Aliases may be useful for compatibility, but one form should be identified as
canonical and the numeric selectors should be named or documented.

### Amplitude documentation is inaccurate

The presentation describes `a1` as full amplitude. SKODE amplitude is expressed
in dB, making `a0` the unity setting.

Relevant code and documentation:

- `parts/skode.c.kit:1397`
- `doc/adc-tokyo-2026-beeps-and-ports.md:83`

## Inferred Design Conventions

The central musical vocabulary is reasonably coherent:

- `v`, `w`, `f`, `a`, `p`, `t`, and `l` form a compact selected-voice
  language.
- Lowercase commands mostly modify the selected voice.
- `A`, `F`, `P`, and `C` consistently describe modulation using source voice,
  depth, and optional offset.
- `n` and `N` form a MIDI-note and detuning pair.
- `G`, `H`, and `L` link MIDI note, velocity, and trigger behavior.
- `y`, `x`, `z`, and `Z` form a recognizable pattern-selection, step, and
  playback family.
- Arrow commands generally communicate movement between string, data, wave,
  recording, and variable stores.
- Slash-prefixed commands generally perform administrative, loading, or
  runtime-control operations.

The language appears to have evolved through live use rather than from a formal
command schema. That gives it a compact and expressive character, but aliases,
sentinel values, silent argument failures, and uneven query forms now make its
intent harder to infer.

## Recommended Design Work

Create a machine-readable command table with these fields:

- Command atom and canonical spelling
- Feature gate
- Argument names, types, units, and valid ranges
- Optional arguments and defaults
- Selected or explicit target
- State changes and other side effects
- Query or inspection form
- Canonical serialization form
- Compatibility aliases
- Error behavior

Use that table to drive documentation and contract tests. At minimum, add tests
for argument counts, link-list replacement, reset-all behavior, data movement,
pattern-step defaults, and `voice_format()` round trips.
