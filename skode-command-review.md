# SKODE Command Review

Recorded June 6, 2026.

This review evaluates the command set for internal consistency by inferring
intent from the implementation, canonical state output, documentation, and
existing patch files.

## Status Update

Reviewed against the implementation on June 11, 2026.

- **Resolved:** `/r` offset indexing, `w>r` transfer behavior, `x-` handling,
  `G`/`H` link replacement, square-bracket documentation, bare `wait`, and the
  presentation's amplitude examples.
- **Still open by design or pending decision:** the `S100` reset-all sentinel,
  incomplete serialization of `w` interpolation/one-shot flags, query syntax
  consistency, aliases, and undocumented numeric selector families.

Line references below describe the June 6 snapshot and may have moved.

## High Priority

### Former `/r` offset indexing bug - Resolved

The June 6 implementation read the fourth argument. It now reads `arg[2]`, so
`/r slot,one_shot,offset` matches its documented three-argument form.

Relevant code:

- `parts/skode.c.kit:2092`

### Former conditional `w>r` transfer bug - Resolved

Wave-to-record now copies whenever the recording buffer is available and large
enough, allocating it first only when necessary.

Relevant code:

- `parts/skode.c.kit:1874`

### Former `x-` default-argument bug - Resolved

The `x` command now uses `isnan(arg[0])`, so placeholders such as `x-` and
`x.` advance the edit cursor as intended.

Relevant code:

- `parts/skode.c.kit:2004`

## Medium Priority

### `S100` is an accidental reset-all sentinel - Open

`S` appears to mean "reset the specified voice," but an invalid voice causes
all voices to reset. Documentation and patches use `S100` as "quiet/reset all,"
which depends on voice 100 remaining invalid.

A dedicated all-voices reset command would communicate this intention without
relying on an out-of-range value.

Relevant code:

- `parts/skode.c.kit:1783`
- `parts/synth.c.kit:1486`
- `doc/learn.html:314`

### Former partial `G` and `H` link replacement - Resolved

`G` and `H` now build complete four-slot lists initialized to `-1`; omitted
positions clear old MIDI and velocity links.

Relevant code:

- `parts/skode.c.kit:1525`
- `parts/skode.c.kit:1537`
- `parts/synth.c.kit:872`

### Voice serialization is not a complete round trip - Open

`voice_format()` emits most selected-voice state, but it does not emit the
interpolation and one-shot arguments supported by `w`. Replaying `?` output can
therefore produce different playback behavior.

Relevant code:

- `parts/skode.c.kit:1814`
- `parts/synth.c.kit:849`

### Former string-syntax documentation mismatch - Resolved

Current documentation uses square-bracket strings, matching the ANDS parser.

Relevant code:

- `doc/index.html:322`
- `parts/ands.c:310`

### Former bare-`wait` argument bug - Resolved

`wait` now validates the first numeric argument before sleeping. A bare
`wait` is a no-op.

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

### Former amplitude documentation error - Resolved

The presentation now uses `a0` for unity amplitude and negative dB values for
quieter examples.

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

Use that table to drive documentation and contract tests. Remaining useful
coverage includes argument-count boundaries, reset-all behavior, numeric
selector contracts, and `voice_format()` round trips.
