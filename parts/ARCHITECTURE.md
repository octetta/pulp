# SKRED Architecture and Adoption Guide

SKRED is a small, embeddable audio engine controlled by a compact text
language called Skode. The repository is named PULP, while the runnable engine
and C API are named SKRED.

The project combines several ideas that are normally separate:

- a wavetable synthesizer with runtime-sized, struct-of-arrays voice state
- a terse command language suitable for live use and machine generation
- a compiler from selected Skode commands to bounded numeric opcodes
- a pattern sequencer and timestamped event queue
- build-time feature selection through generated C
- native, UDP, subprocess, and WebAssembly entry points

This document explains how those pieces fit together and how to approach the
codebase when evaluating, embedding, or extending it.

## Architectural Overview

The public embedding surface is intentionally small:

```c
int skred_start(unsigned int frames, unsigned int voices, int udp_port);
int skred_command(char *command);
void skred_stop(void);
```

At runtime, commands follow one of two paths:

```text
                           control thread
                                 |
                         ASCII Skode command
                                 |
                           ANDS parser
                                 |
                    +------------+-------------+
                    |                          |
              immediate command          compiled command
                    |                          |
          files, strings, status,       bounded numeric
          editing, synth control          opcode program
                    |                          |
                    |                 +--------+--------+
                    |                 |                 |
                    |             pattern step      event queue
                    |                 |                 |
                    +-----------------+--------+--------+
                                               |
                                         audio callback
                                               |
                                  render current block,
                                then apply due scheduled work
                                  for the following block
```

Immediate commands execute while Skode text is being parsed. Scheduled
commands are first converted to fixed-size `event_program_t` and `event_t`
values. The audio callback never parses command text.

## What Is Unusual

### Text Is the Main Control API

The primary API accepts Skode strings instead of exposing every synthesizer
operation as a C function. This gives the command line, UDP clients, browser
UI, scripts, and embedding applications the same control vocabulary.

Skode is deliberately compact:

```text
v0 w0 n60 a-8 l1
[v0 n36 l1] xa
~0.25 v0 l0
```

Whitespace is often optional, numeric arguments may precede or follow an atom,
and command names are packed into 32-bit values for switch-based dispatch.
See [SKODE_USER_COMMAND_REFERENCE.md](SKODE_USER_COMMAND_REFERENCE.md) for
the user-facing language reference and [SKODE_COMMANDS.md](SKODE_COMMANDS.md)
for implementation details.

### Skode Has Immediate and Compiled Semantics

Skode is not interpreted uniformly. Commands involving files, strings, data
arrays, diagnostics, or parser-owned state remain on the control thread.
Commands suitable for real-time execution can be compiled into numeric
opcodes.

This is a central design boundary:

- Immediate execution is expressive and may allocate, format text, or perform
  I/O.
- Compiled execution is bounded and suitable for patterns, defers, repeats,
  and the event queue.

Compilation is all-or-nothing. Unsupported commands do not fall back to
running parser text in the audio callback. The opcode model is documented in
[OPCODES.md](OPCODES.md).

Literal external macros such as `e!12` are expanded while compiling. Existing
patterns and queued events therefore keep snapshot semantics when the macro is
edited later.

### Features Produce Different C Programs

Files ending in `.c.kit` and `.h.kit` are source templates processed by
`kit.c`. Directives such as:

```c
@if(FILT)
/* filter implementation */
@endif
```

include or remove code before the C compiler runs. `KIT_OPTS` therefore
selects features structurally rather than merely disabling them at runtime.

This keeps small builds genuinely small, but it means contributors must test
more than one feature configuration. Generated files in build directories are
disposable. The `.kit` templates are authoritative.

`analysis-src/` is the checked-in generated view for the canonical
`MAXED_KIT_OPTS` preset used by source analysis services. Do not edit it
directly; regenerate it with:

```sh
make analysis-src
```

### Synth State Uses Parallel Arrays

Voice state is stored in `synth_voices_t sv` as a struct of pointers to
parallel arrays:

```c
sv.phase[voice]
sv.freq[voice]
sv.pan[voice]
sv.amp_envelope[voice]
```

The layout favors contiguous access across voices and gives the compiler a
better opportunity to vectorize the audio loop. Voice counts are rounded to
`VOICE_ALIGN`, and all voice and wave storage is allocated during
`synth_init()`.

The sample loop avoids source work that cannot affect the current frame.
Reset voices at or below the `-60 dB` silence floor exit before oscillator
processing. Capture input is read only when an active capture voice requests
it. The audio RNG still advances exactly once per frame to preserve the noise
stream across silent intervals, but conversion to a floating-point noise sample
is deferred until an active noise voice requests it.

The allocator has a strict ownership rule: `synth-alloc.c.kit` is the only
module that allocates and frees the main synth state. The audio callback must
not call it.

### One Timeline Drives Events and Patterns

The atomic synth sample counter is the engine clock. Scheduled events carry
absolute sample timestamps. Pattern ticks are derived from that same sample
timeline and the current tempo rather than from callback count.

Consequences:

- changing the requested audio buffer size does not redefine musical time
- deferred commands and patterns share a common timebase
- tempo changes preserve the current timeline position
- catch-up work is capped at 64 pattern ticks per callback

Tempo is accepted from 1 through 960 BPM. At four sequence steps per beat,
960 BPM represents 64 sequence ticks per second.

## Major Components

### Public API and Audio Device Layer

Files:

- `api.h`
- `api.c.kit`
- `mini-skred.c.kit`

`api.h` is the preferred integration boundary. `api.c.kit` owns the singleton
engine, miniaudio context, active device, command context, optional UDP
service, and optional recorder.

The miniaudio callback performs this order:

1. Prepare an optional recording bus.
2. Run due queued events and pattern steps with `seq()`.
3. Ask the sequencer for the next event or tempo-tick sample in the block.
4. Render up to that boundary with `synth()`.
5. Repeat sequencing and rendering for each boundary in the block.
6. Submit the complete recording block without writing files on the audio thread.

Most callbacks still call `synth()` once. A callback is split only when a
queued event or pattern clock tick falls inside it. Output, capture input, and
recording pointers are advanced for each segment, while the absolute synth
sample counter remains the timing authority. Integer-sample queued events are
exact; fractional tempo boundaries are rounded forward to the next sample, so
normal sequencing error is less than one sample. Audio-device buffering and
hardware latency are unchanged.

`mini-skred.c.kit` is a thin example host. It supports an interactive editor,
line-oriented subprocess operation, audio-device selection, and UDP startup.

### ANDS Parser

Files:

- `ands.h`
- `ands.c`

ANDS is the streaming parser beneath Skode. It recognizes numbers, variables,
atoms, bracketed strings, numeric arrays, comments, chunk separators, and the
`+`/`~` defer prefixes.

The parser emits callback events such as `FUNCTION`, `DEFER`, `GOT_STRING`,
and `CHUNK_END`. It does not know synthesizer semantics. That separation makes
ANDS useful as a small syntax layer while `skode.c.kit` owns the command
vocabulary.

Each `skode_t` has its own parser and editing state. Local API control and UDP
clients can therefore retain independent selected voices, patterns, strings,
and parser state while sharing the engine.

### Skode Dispatcher

Files:

- `skode.h`
- `skode.c.kit`

`skode_consume()` feeds text to ANDS. Parser callbacks arrive at
`skode_callback()`, and ordinary command atoms are dispatched by
`skode_function()`.

This module contains the broad control-plane behavior:

- immediate synth commands
- pattern editing and diagnostics
- wavetable and sample manipulation
- external string macros
- Ksynth and file operations
- queue administration
- status and logging

When adding a command, first decide whether it is immediate-only or can be
represented as a real-time opcode. That decision determines whether the work
belongs only here or also in `skode-event.c`.

### Opcode Compiler and Executor

Files:

- `skode-event.c`
- `skode.h`
- `seq.h.kit`

`skode_compile_program()` invokes a separate ANDS parser configured with a
compiler callback. The result is an `event_program_t` containing no more than
32 operations. Each operation contains an opcode, up to four numeric
arguments, variable-reference bits, and optional delay mode.

Program execution tracks a current voice. It executes due operations directly
and places future operations into the event queue as `event_t` values.
Register references remain symbolic until execution, so a queued `$N`
argument reads the register value when the event becomes due.

The executor eventually calls `skode_execute_voice_opcode()`, which is the
bridge from the generic opcode representation to synth functions and state.

### Sequencer

Files:

- `seq.h.kit`
- `seq.c.kit`

The sequencer owns:

- 128 patterns
- up to 128 textual steps per pattern
- one compiled program beside each step
- pattern length, position, state, mute, and modulus
- tempo and master tick state
- the timestamped event queue

Source text is retained for editing and display, but playback uses the
compiled program. Each pattern also retains its current voice between steps.
The 128-step limit balances the larger 32-operation compiled programs while
keeping the fixed pattern table bounded.
Clearing a pattern advances its generation so the API layer resets that
persistent voice.

Pattern edits use a mutex. The audio callback uses `trylock`; if editing is in
progress, it skips pattern processing for that callback instead of blocking
the real-time thread.

### Event Queue

Files:

- `skqueue.h`
- `skqueue.c`

The queue has two stages:

1. A bounded multi-producer ring receives events from control threads.
2. The audio consumer transfers complete entries into a timestamp-ordered
   min-heap.

Each ring slot has a publication flag, preventing the consumer from observing
a producer's partially written payload. Queue clearing increments a
generation, which invalidates in-flight old events without moving the read
index past an active producer.

Administrative operations lock the heap. The audio consumer uses `trylock`
and returns immediately if an administrator is inspecting, cancelling, or
clearing events.

### Synth Engine

Files:

- `synth.c.kit`
- `synth.h.kit`
- `synth-state.h.kit`
- `synth-alloc.c.kit`
- `synth-config.h`
- `synth-types.h`

The synth is a wavetable engine with optional envelopes, modulation, filters,
distortion, smoothing, panning, glissando, and recording routes. Feature
directives remove unused state and processing code from generated builds.

The main lifecycle is:

```text
synth_init()
wave_table_init()
voice_init()
        |
     synth() for every audio block
        |
wave_free()
synth_free()
```

Most command handlers mutate the shared voice arrays directly. This is a
deliberately compact engine rather than a transactional parameter system.
Adopters adding high-volume automation should preserve the established
real-time rules and consider adding bounded command transport instead of
performing arbitrary cross-thread writes.

#### One-Shot Loop State

Wave direction, loop configuration, and note lifecycle are separate:

- `b` controls the sign of oscillator travel.
- `B` controls whether loop-boundary wrapping is configured.
- `BC` sets a configured wrap bound; zero means unlimited. A positive count is
  the number of repeats after the initial traversal, so `BC1` produces two
  traversals of the loop region.
- Positive `l` and `T` retrigger a one-shot, initialize runtime looping from
  `B`, and snapshot the configured `BC` bound. `l0` releases envelopes
  immediately and requests departure at the next boundary.

The voice arrays distinguish persistent configuration from active-note state.
`loop_enabled` and `loop_count` are configuration. `loop_active`,
`loop_bounded`, `loop_remaining`, and `loop_stop_requested` belong to the
current trigger. A `BC` edit does not retroactively change the bounded-loop
count already in progress. `B` is intentionally immediate: `wave_loop()`
updates both configured and active looping.

`osc_next()` selects the relevant boundary from the actual phase-step
direction. It accounts for more than one wrap when a large phase increment
crosses several loop lengths. A requested exit or exhausted bound disables
runtime looping and emits `loop_ended`; the render loop consumes that event at
the same sample and releases active amplitude and filter envelopes. Oscillator
playback then continues into the one-shot tail and sets `finished` only at the
physical wave boundary.

Envelope configuration follows the same snapshot principle. `t` and `ft`
update parameters for the next trigger, while an envelope already in progress
continues using the durations and sustain level captured by its last `l` or
`T`.

### Recording

Files:

- `recorder.c.kit`
- `recorder.h.kit`

With `RECORD=1`, the synth can produce a ten-channel bus: stereo master plus
four stereo stems. The audio callback writes blocks into a miniaudio PCM ring.
A writer thread drains the ring and performs WAV encoding.

This separation is important: disk I/O and encoder work do not occur in the
audio callback. If the ring cannot accept data, recording reports dropped
frames rather than waiting.

### UDP Control

Files:

- `udp.c`
- `udp.h`

The optional UDP server accepts Skode text and keeps a pool of `skode_t`
contexts indexed by client address and port. This gives clients independent
parser selections while they control the same synth and sequencer.

UDP is a convenience transport, not an authentication or reliability layer.
An adopter exposing it beyond a trusted network should add validation,
authorization, rate limiting, and protocol-level acknowledgement as needed.

### WebAssembly

Files:

- `wasm-assets/mkwasm`
- `wasm-assets/test-wasm.html`
- `../doc/skred_api.js`
- `../doc/skred_api.wasm`

The WASM build uses the same generated engine sources and public C API. The
build embeds example Skode, Ksynth, and audio assets into Emscripten's virtual
filesystem and exports the API through `ccall`/`cwrap`.

The generated JavaScript and WASM under `doc/` are distribution artifacts.
Run `make wasm` after changes affecting generated code or the public runtime.

## Threading and Real-Time Rules

The project has several possible threads:

- the miniaudio callback thread
- the application or command thread
- the optional UDP receiver thread
- the optional recording writer thread
- optional Ksynth workers

The most important rule is that the audio callback must remain bounded:

- do not parse Skode text there
- do not allocate or free memory there
- do not perform file or network I/O there
- do not wait on a control-thread mutex
- do not format diagnostic strings there

The event queue and pattern editor use publication flags, atomics, mutexes, and
audio-side `trylock` behavior to enforce this boundary. New shared subsystems
should follow the same pattern: prepare or compile work on a control thread,
then send fixed-size data or stable resource identifiers to audio execution.

## Build and Source Workflow

Work from the `parts` directory:

```sh
cd parts
make native
make test
```

Useful validation targets:

```sh
make warn         # strict default-feature build
make warn-maxed   # strict canonical maxed-preset build
make analysis-src # regenerate checked-in generated C
make wasm         # rebuild browser artifacts
```

Feature selection is passed to CMake through `KIT_OPTS`:

```sh
cmake -B build_native -S . \
  -DKIT_OPTS="SEQ=1 ADSR=1 FILT=1 FADSR=1 UDP=1"
cmake --build build_native
```

When changing feature-gated code, test both a minimal build and a build where
the feature is enabled. A symbol that is valid in the maximum build may be
unused or absent in the default build.

## Tests

Tests live in `tests/` and are registered in `CMakeLists.txt`:

- `ands_parser_tests.c` covers parser contracts.
- `skode_state_tests.c` covers commands, compiled programs, macros,
  sequencing, tempo limits, and state behavior.
- `skqueue_tests.c` stresses concurrent event publication.
- `audio_command_tests.c` covers audio-device command routing.
- `recording_tests.c` covers recording when `RECORD` is enabled.
- the CMake smoke test checks generated build artifacts and feature output.
- `synth_callback_bench.c` is an opt-in timing diagnostic for average and
  worst synthesis callback cost and measured deadline overruns.

For a new command, add tests at the lowest useful layer. Parser syntax belongs
in the ANDS tests; command behavior and opcode compilation belong in Skode
state tests; queue concurrency belongs in queue tests.

## How to Approach the Codebase

A practical reading order is:

1. Read `api.h` to understand the supported host boundary.
2. Read the execution-model section of `SKODE_COMMANDS.md`.
3. Follow `skred_command()` to `skode_consume()` and `skode_function()`.
4. Read `OPCODES.md`, then follow `skode_compile_program()` through
   `run_program()`.
5. Read `seq.h.kit` and the central `seq()` function.
6. Read `synth-state.h.kit` before entering the larger `synth.c.kit`.
7. Read `CMakeLists.txt` and `kit.c` before changing feature generation.

The generated files under `build_*` can be easier to navigate with a debugger,
but changes belong in the corresponding `.kit` template or plain source file.
Use `analysis-src/` as a readable maxed-preset snapshot, not as an editing
target.

## Adoption Strategies

### Embed the Existing API

This is the lowest-risk path. Link the `api` static library and drive it with
`skred_start()`, `skred_command()`, and `skred_stop()`. Use
`skred_audio_*()` for device management.

This approach preserves the native command language and keeps your host
isolated from internal state layout.

### Use SKRED as a Subprocess

Run `mini-skred -n` and exchange line-oriented Skode over standard input and
output. This gives process isolation and avoids C ABI integration at the cost
of transport latency and lifecycle management.

### Use UDP for Live Control

Build with `UDP=1` and send Skode datagrams. This is convenient for trusted
local-network tools, controllers, and rapid prototyping. Treat it as an
unreliable control transport.

### Build a Browser Host

Use `make wasm` and wrap exported functions with Emscripten `cwrap`. Browser
code can use the same textual command protocol as native hosts.

### Fork the Engine Internals

Direct internal adoption is reasonable when you need a custom audio backend,
different scheduling semantics, or a substantially different state model.
Start by keeping the Skode compiler and opcode boundary intact; it is the
cleanest separation between rich control behavior and real-time execution.

## Current Constraints Adopters Should Know

- The engine is a singleton. Major synth, parser-register, sequencer, device,
  and API state is global. Multiple independent engines in one process would
  require a substantial context refactor.
- The public API is C-friendly but does not currently advertise a formal
  versioned ABI guarantee.
- Pattern, queue, program, argument, macro, and parser limits are fixed
  constants. Review them before accepting untrusted or machine-generated
  workloads.
- The textual API is compact and case-sensitive. Hosts should preserve command
  strings exactly and surface parser diagnostics to users.
- Some synth parameters are shared directly between control and audio code.
  Extensions should not assume that every field has transactional or
  sample-accurate update semantics.
- UDP has no built-in security or delivery guarantees.
- Feature combinations create different compiled interfaces internally.
  Avoid reaching into feature-specific state from an embedding host.
- Audio-device ownership lives inside the API layer. A host that already owns
  an audio callback may prefer to extract or expose the block renderer instead
  of starting SKRED's miniaudio device.

## Where to Make Changes

| Goal | Primary files |
| --- | --- |
| Add or change Skode syntax | `ands.c`, `ands.h` |
| Add an immediate command | `skode.c.kit` |
| Add a schedulable command | `skode.h`, `skode-event.c`, `skode.c.kit` |
| Change patterns or tempo | `seq.c.kit`, `seq.h.kit` |
| Change queue behavior | `skqueue.c`, `skqueue.h` |
| Add a synth feature | `synth.c.kit`, `synth-state.h.kit`, `synth-alloc.c.kit` |
| Change embedding or devices | `api.c.kit`, `api.h` |
| Change the native host | `mini-skred.c.kit` |
| Change WASM packaging | `wasm-assets/mkwasm` |
| Change feature generation | `kit.c`, `CMakeLists.txt` |
| Add recording behavior | `recorder.c.kit`, `recorder.h.kit` |

For a new schedulable synth command, the typical sequence is:

1. Add an opcode identifier to `skode_opcode_t`.
2. Teach the compiler to encode the command.
3. Teach the executor to apply the opcode.
4. Keep immediate command behavior consistent with compiled behavior.
5. Document it in `SKODE_COMMANDS.md` and `OPCODES.md`.
6. Add compilation and execution tests.
7. Run strict default and maxed-preset builds.
8. Regenerate `analysis-src/` and WASM artifacts when applicable.

## Design Direction

The project's strongest architectural idea is the boundary between expressive
text control and bounded real-time execution. An adopter can replace the host,
transport, UI, or even large portions of the synth while retaining that model:

```text
rich control input -> validate and compile -> fixed real-time data
```

Keeping that boundary explicit will make extensions easier to reason about
and will preserve the qualities that make SKRED useful as a small live audio
engine.
