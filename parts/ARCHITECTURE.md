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

The public embedding surface is intentionally centered on the text command API:

```c
int skred_start(unsigned int frames, unsigned int voices, int udp_port);
int skred_command(char *command);
void skred_stop(void);
```

`api.h` also exposes supporting integration functions for version and feature
inspection, command logging, audio-device selection, runtime service health,
and optional recording and shared-memory scope control.

The optional voice-pool layer is implemented in `polyphony.c.kit`. It stores
fixed group layouts, pool allocators, monophonic held-note ledgers, and graph
formatting buffers. Structural commands run on the control thread. Performance
commands compile to `SKODE_OP_POLY_NOTE`, `SKODE_OP_POLY_RELEASE`, and
`SKODE_OP_POLY_BEND`, so allocation and note changes remain bounded and
allocation-free when executed from the audio event path. Physical voices and
all ordinary `v` commands remain available underneath the pool layer.

It also exposes a bounded control-plane event ring. Hosts do not register a C
callback. They use `skred_control_event_poll()` from their own control/UI loop
to receive notifications such as `SKRED_CONTROL_EVENT_VOICE_TRIGGER`,
`SKRED_CONTROL_EVENT_VOICE_RELEASE`, and `SKRED_CONTROL_EVENT_VOICE_FINISHED`,
plus explicit Skode `ce` markers and opt-in pattern boundary notifications.
Hosts that need to sleep in an event loop can multiplex on
`skred_control_event_wait_fd()` on POSIX systems or
`skred_control_event_wait_handle()` on Windows, then drain with
`skred_control_event_poll()`. Hosts can also use the built-in response
dispatcher: `/ceb` binds an event to a Skode command, and `/cer 1` starts an
API-level dispatcher thread that sleeps on the same wait object and runs
matching responses outside the audio callback.

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
                                  apply due scheduled work,
                                then render to the next
                                      timing boundary
```

Immediate commands execute while Skode text is being parsed. Scheduled
commands are first converted to fixed-size `event_program_t` and `event_t`
values. The audio callback never parses command text.

### Two Kinds of Events

The codebase uses "event" for two related but distinct mechanisms:

- **Scheduled opcode events** are pending work for the audio engine. Skode
  defers, repeats, and compiled pattern operations create `event_t` values in
  the sequencer queue. These events answer "what is waiting to happen?" Hosts
  can inspect them with `skred_scheduled_event_snapshot()` or `?q`.
- **Control-plane events** are notifications emitted after engine behavior has
  happened. Voice lifecycle code publishes `skred_control_event_t` records such
  as trigger, release, and finished into a bounded ring. Patterns can opt into
  start/end boundary notifications with `yc1`, and scheduled Skode can emit
  explicit user notifications with `ce id[,a,b,c]`. These events answer
  "what happened?" Hosts service them by polling
  `skred_control_event_poll()` or, in `mini-skred`, by issuing `?ce`. Event
  publication is opt-in per voice with `vc1`; voices default to `vc0`.

This separation is intentional. Scheduled opcode events drive sound-generation
state changes. Control-plane events let an external application mirror or react
to selected engine state without putting callbacks, I/O, or UI work on the
real-time audio path.

For host applications, the control-plane contract is:

- Voice lifecycle events are emitted only for voices enabled with `vc1`.
- Pattern boundary events are emitted only for patterns enabled with `yc1`.
- User events are emitted only by explicit `ce id[,a,b,c]` commands.
- The host owns interpretation of `ce` IDs and payload values.
- The host must drain the bounded ring with `skred_control_event_poll()` and
  watch the dropped-event counter. The fd/HANDLE APIs are only wake signals.
- Built-in response bindings may be serviced automatically with `/cer 1` or
  `skred_control_dispatch_start()`, or manually by calling
  `skred_control_dispatch_pump()` from a host service loop. Native builds use
  a waitable dispatcher thread. Emscripten uses a bounded browser timer pump;
  it does not create a worker that blocks in an emulated `select()`.
- Diagnostic readers can use `skred_control_event_snapshot()` or `?ce` to
  inspect outstanding events without consuming them; `?ce!` explicitly discards
  outstanding control-plane events.
- Event response bindings match on event type plus `voice`, `pattern`, or user
  `id`; the event `tag` is provenance/cancellation metadata from scheduled
  work, not a response key.
- Hosts may expose application code through `/ff0` through `/ff9` foreign C
  function bindings when Skode needs to call outside the built-in API.

## What Is Unusual

### Text Is the Main Control API

The primary API accepts Skode strings instead of exposing every synthesizer
operation as a C function. This gives the command line, UDP clients, browser
UI, scripts, and embedding applications the same control vocabulary.

Skode is deliberately compact:

```text
v0 w0 n60 a0 l1
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

`api.h` is the preferred integration boundary. Its core control API is
`skred_start()`, `skred_command()`, and `skred_stop()`, with additional helpers
for feature/version inspection, logging, audio-device management, and optional
recording and shared-memory scope control. It also exposes polling APIs for
control-plane notifications and scheduled-event queue snapshots. `api.c.kit`
owns the singleton engine, miniaudio context, active device, command context,
control-event ring, optional UDP service, recorder, and shared-memory scope
publisher.

The miniaudio callback performs this order:

1. Prepare an optional ten-channel capture bus for recording or scope output.
2. Run due queued events and pattern steps with `seq()`.
3. Ask the sequencer for the next event or tempo-tick sample in the block.
4. Render up to that boundary with `synth()`.
5. Repeat sequencing and rendering for each boundary in the block.
6. Submit the complete block to the recorder and/or scope publisher.

Most callbacks still call `synth()` once. A callback is split only when a
queued event or pattern clock tick falls inside it. Output, capture input, and
capture-bus pointers are advanced for each segment, while the absolute synth
sample counter remains the timing authority. Integer-sample queued events are
exact; fractional tempo boundaries are rounded forward to the next sample, so
normal sequencing error is less than one sample. Audio-device buffering and
hardware latency are unchanged.

`mini-skred.c.kit` is a thin example host. It supports an interactive editor,
line-oriented subprocess operation, audio-device selection, and UDP startup. It
also provides diagnostic service commands: `?ce` snapshots control-plane
notifications, `?ce!` clears them, and `?q` shows pending scheduled opcode
events.

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
32 operations. Each operation contains an opcode, up to eight numeric
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

#### Oscillator Playback Classes

Each voice caches a playback classification. A non-one-shot, forward,
full-range wave with no active loop is a simple cycle; `osc_next()` sends it
through a short phase-wrap path that skips the general direction, subrange,
loop-count, release-tail, and one-shot boundary machinery. All other
configurations use the general path. Wave selection and commands that change
mode, direction, range, or looping reclassify the voice, so this is an
automatic implementation detail rather than a user-selected optimization.

Memory-backed wave loaders initialize loop metadata with neutral half-open
bounds `[0, length)`. Parser data and `k>w` results default to cycle mode;
recordings and decoded audio files default to one-shot mode. Selecting a wave
without a mode override inherits that stored mode.

#### One-Shot Loop State

Wave direction, loop configuration, and note lifecycle are separate:

- `b` controls the sign of oscillator travel.
- `B` controls whether loop-boundary wrapping is configured.
- `BC` sets a configured wrap bound; zero means unlimited. A positive count is
  the number of repeats after the initial traversal, so `BC1` produces two
  traversals of the loop region.
- Positive `l` and `T` retrigger a one-shot, initialize runtime looping from
  `B`, and snapshot the configured `BC` bound. `l0` releases envelopes
  immediately and asks any active one-shot loop, bounded or unbounded, to leave
  the loop at the next boundary in the current playback direction.

For one-shots, loop points do not change the trigger origin. Forward triggers
start at physical sample `0`, play the pre-loop segment, wrap from `loop_end`
to `loop_start`, and repeat the loop region according to `B`/`BC`. Backward
triggers start at the physical last sample, play down to `loop_start`, wrap to
`loop_end`, and repeat in reverse. `l0` and exhausted bounded loops disable
runtime looping at that same directional boundary, then playback continues
toward the physical end in the current direction.

The voice arrays distinguish persistent configuration from active-note state.
`loop_enabled` and `loop_count` are configuration. `loop_active`,
`loop_bounded`, `loop_remaining`, and `loop_stop_requested` belong to the
current trigger. A `BC` edit does not retroactively change the bounded-loop
count already in progress. `B` is intentionally immediate: `wave_loop()`
updates both configured and active looping. `B0` clears active loop runtime
state; `B1` starts a fresh active loop snapshot from the current `BC`
configuration.

Loop points are layered. The wave owns default `loop_start` and `loop_end`
metadata, changed by `WL wave,start,end`. Selecting a wave with `w` copies those
defaults into the voice. `VL start,end` then marks the selected voice as
overridden and replaces its local loop points; plain `VL` clears that override
and reapplies the current wave defaults. Later `WL` changes update voices that
still follow the wave defaults and leave overridden voices alone. `loop_start`
is inclusive, `loop_end` is the exclusive boundary, and `loop_end` may equal the
physical sample count.

`osc_next()` selects the relevant boundary from the actual phase-step
direction. It accounts for more than one wrap when a large phase increment
crosses several loop lengths. A requested exit or exhausted bound disables
runtime looping and emits `loop_ended`; the render loop consumes that event at
the same sample and releases active amplitude, filter, and phase-distortion
envelopes. Oscillator playback then continues into the one-shot tail and sets
`finished` only at the physical wave boundary.

Envelope configuration follows the same snapshot principle. `t` and `ft`
update parameters for the next trigger, while an envelope already in progress
continues using the durations and sustain level captured by its last `l` or
`T`.

Amplitude envelope mode `k1` is timed one-shot ASR. On positive `l`/`T`, finite
one-shots compute a natural playback duration from table length, phase
increment, and configured `BC` repeats. The amp envelope release is scheduled
so it finishes at that natural end. Unbounded one-shot loops do not have a
natural end, so they keep normal held ADSR behavior until `l0`; that release
then exits the loop in the current direction and plays the remaining wave tail.

#### FM and Lookup-Phase Modulation

The optional `FM` feature has three render modes. `FF0` adds a relative
frequency deviation to the carrier phase increment, `FF1` treats the `F`
control value as instantaneous hertz, and `FF2` advances the carrier at its
unmodulated phase increment but adds the `F` control value to the wavetable
lookup phase in radians. Consequently, bipolar `FF2` modulation cannot
accumulate into carrier pitch drift.

`FB 0..7` is configuration for `FF2` operator feedback. Each voice owns two
previous post-envelope output samples. Their average, multiplied by `FB`, is
added to the lookup phase along with any external `F` source. The history is
runtime note state: it is zeroed on positive triggers, copies, resets, `FB0`,
and transitions out of `FF2`. It is never allocated or cleared from the audio
callback.

The ordinary voice loop remains the operator scheduler. A modulation source
with a lower voice number has already rendered its current sample; a
higher-numbered source contributes its previous sample. Stage-one
DX-inspired patches therefore arrange acyclic chains in ascending order.
Self-reference through `F` remains disabled; `FB` is the only intentional
cycle and is explicitly delayed by its two-sample history.

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

The file uses the active engine/device sample rate (44.1 kHz by default) and
interleaved 32-bit float samples. Channels `0..1` are master left/right,
followed by four stereo stem pairs in channels `2..9`. Voice command `r1`
through `r4` adds that voice to the corresponding stem while leaving it in the
master; `r0` removes the stem route. Each non-master stem also owns one
mono-send/stereo-return delay line. `ds amount` feeds the delay attached to the
voice's current `r1`..`r4` route, and that wet return is added both to the main
stereo mix and to the matching stem. `[filename]/rg` starts, `/r?` reports
progress, and `/rs` drains and finalizes the encoder.

### Shared-Memory Scope

Files:

- `scope-ipc.c`
- `scope-ipc.h`
- `scope-reader.c`

With `SCOPE=1`, the same ten-channel master/stem bus can be published to a
versioned shared-memory overwrite ring. The transport has POSIX
shared-memory and Windows named-file-mapping implementations, although the
current CMake configuration enables `SCOPE` only on POSIX targets. The
producer writes full-rate interleaved floats and atomically advances an
absolute frame counter. A seqlock-style publication counter lets readers
reject a partial or overwritten snapshot and retry.

The scope stems use the same routing as recording: dry voice contribution
comes from `r1`..`r4`, and each stem includes the return from its corresponding
track delay. The master channels include all dry voices plus all active track
delay returns.

The audio callback never waits for a reader. If scope lifecycle work briefly
owns the publisher lock, that callback simply skips scope publication. Stop
and restart operations run outside the real-time path and wait for an
in-progress publication before unmapping.

The bundled `scope_reader` is a renderer-independent consumer example:

```sh
./build_maxed/scope_reader skred-scope 2048
```

It reports peak and RMS values for enabled channels. A Raylib oscilloscope can
use the same reader API, select the newest window, trigger on a zero crossing,
and draw at display frequency.

`RECORD` and `SCOPE` share one rendered capture bus when both are active.
Rendering and stem mixing therefore happen once per callback; the completed
block is submitted independently to the WAV ring and shared-memory ring.
Either feature can also be built and used without the other.

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

### Asset VFS and ZIP Mounts

Files:

- `exp-vfs/skred_vfs.c`
- `exp-vfs/skred_vfs.h`
- `exp-vfs/miniz*.c`

The VFS is part of the main API library. It presents disk directories,
file-backed ZIP archives, and copied in-memory ZIP archives through one
stdio-like interface with a VFS-relative current directory. Skode file,
wavetable, and Ksynth loaders search the active mount before real-directory
and type-specific fallback paths. A `file:` prefix explicitly bypasses a ZIP
mount.

ZIP extraction, directory enumeration, mounting, and writing are all
control-thread operations. They do not run from scheduled opcodes or the audio
callback. Browser hosts use the memory-mount API for uploaded asset bundles;
native Skode uses `%z`, `%zu`, `%pwd`, `%cd`, `%ls`, and `%cat`.

The top-level `vfs/` directory is an older standalone prototype. The
integrated implementation is `parts/exp-vfs/`, despite the historical
directory name.

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
- the optional control-event response dispatcher thread
- optional MIDI backend callback threads
- the optional UDP receiver thread
- the optional recording writer thread

Ksynth evaluation is synchronous and runs on whichever command or UDP thread
invoked it; it does not create a separate worker.

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

On Windows, prefer the Ninja/Zig presets instead of CMake's NMake generator:

```powershell
cd parts
cmake --preset windows-zig-ninja
cmake --build --preset windows-zig-ninja
```

Use `windows-zig-maxed` for the portable maxed feature set. It includes
Ksynth, MIDI, recording, and track routing, but omits `SCOPE=1` because the
current CMake configuration rejects that feature on Windows.

Linux hosts can also cross-build a Windows executable with Zig. Build the
native generator first, then configure the Windows target:

```sh
cd parts
cmake --preset ninja-release
cmake --build --preset ninja-release --target kit_tool
cmake --preset cross-windows-zig-ninja
cmake --build --preset cross-windows-zig-ninja --target mini-skred
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
- `ksynth_bridge_tests.c` covers Ksynth data and wavetable transfer when
  `KSYNTH` is enabled.
- `recording_tests.c` covers recording when `RECORD` is enabled.
- `scope_ipc_tests.c` covers cross-process reads, wraparound, restart
  generations, and Skode scope commands when `SCOPE` is enabled.
- `midi_tests.c` covers hardware-independent MIDI event publication, routing,
  bindings, voice/pool behavior, and pitch bend when `MIDI` is enabled.
- `polyphony_tests.c` covers group cloning, allocation/stealing, monophonic
  held-note behavior, routing preservation, and dependency graphs.
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

See [API_INTEGRATION.md](API_INTEGRATION.md) for the generated distribution
layout, compile/link examples, lifecycle, logging, control-plane event
polling, `vc`/`yc`/`ce` event enablement, feature, audio-device, recording, and
scope API notes.

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

## MIDI Boundary

MIDI is an optional `MIDI=1` service beside the audio device, not part of the
audio callback. `midi.c` owns the minimidio context and the single active input
and output endpoints; the vendored header remains unmodified under
`vendor/minimidio/`. Public `skred_midi_*()` functions keep minimidio types out
of `api.h`.

Inbound callbacks translate structured MIDI 1.0 messages into
`SKRED_CONTROL_EVENT_MIDI` records in the bounded multi-producer event ring.
This preserves the existing real-time boundary: the backend thread does no
allocation, Skode parsing, response dispatch, or host callback work. The event
type is the dispatcher key, so response bindings can later map particular MIDI
message classes without changing the ring. Fixed events carry channel and two
data values; variable-length SysEx input needs a separate bounded payload
design before it can be exposed safely.

The control dispatcher also applies a fixed-capacity MIDI route table after it
consumes each event. Routes select one MIDI channel or all channels and target
either a physical voice or a poly pool. Note-on velocity is normalized to
`0..1`; note-off releases the matching voice lifetime or pool key; pitch bend
uses the route's symmetric range (±2 semitones by default). All-channel pool
routes encode channel plus note into the allocation key, so equal notes on two
channels retain independent lifetimes and bend. Groups are clone templates,
not note allocators, so their live MIDI target is the corresponding pool.
Backend callbacks remain publication-only; they never inspect or execute this
table.

A second fixed-capacity table maps other fixed-size MIDI messages to Skode
command templates. Bindings filter on message type plus optional channel and
first-data-byte selectors, then substitute channel/raw/normalized values only
after the dispatcher consumes the event. This covers realtime transport,
control change, program/pressure messages, and parameter control without
letting parsing or arbitrary command execution leak into the backend callback.
MIDI Stop can represent pause and Continue can represent resume; MIDI itself
has no distinct Pause status byte.

One practical use of generic bindings is a drum kit: MIDI note number is the
`data1` filter, normalized velocity is substituted into one command, and that
command may trigger one voice, several layered voices, or a named macro.
Note-off bindings and multi-voice commands can implement simple choke behavior.
This remains a control-plane facility: every hit expands and parses text. If
large kits or tighter latency become a priority, the natural next layer is a
fixed note-to-precompiled-program table with velocity input, choke groups,
velocity ranges, round-robin selection, and captured MIDI event timestamps.
Such a table should execute only verified real-time-safe opcodes and should not
move parsing or allocation into the backend MIDI callback.

For pitched instruments, direct voice routes provide the smallest monophonic
path, while pool routes preserve the allocator's polyphonic or monophonic mode.
In particular, mono priority, held-note fallback, and legato remain properties
of `/pm`; MIDI routing translates messages but does not duplicate that state
machine. Poly routes use channel-plus-note keys and apply channel bend to every
held allocation belonging to the matching route.

Outbound `MO` and `d>MO` are immediate control-plane operations. A future
sample-accurate MIDI output facility should compile into a bounded opcode and
enqueue timestamped bytes; it should not make backend calls from the audio
thread. Runtime endpoint commands remain at the `skred_command()` boundary,
parallel to audio-device commands, because device enumeration, permissions,
and virtual-port lifecycle are neither schedulable nor real-time work.

Platform policy is explicit: CoreMIDI and ALSA can expose virtual endpoints;
WinMM and Web MIDI cannot. Web MIDI still supports enumerated physical or
browser-authorized ports, and its permission-bearing initialization is left to
an explicit user-triggered `/mL` or API call. MIDI-enabled WASM therefore uses
Asyncify; non-MIDI builds do not acquire that cost.

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

## Promoted Skode Macros: Follow-up Work

Named macros are checked with the real Skode compiler when defined. Definitions
that compile entirely to scheduled opcodes are cached as dictionary programs;
immediate definitions retain text-expansion behavior; `?m` reports their
classification. Cached parameter locations are explicit template metadata,
not distinguished numeric values.

The following constraints should be addressed before promoted macros become a
widely relied-on public facility:

- **Invalidate transitive dependants.** A promoted macro that calls another
  promoted macro currently captures the callee's program at definition time.
  Redefining or removing the callee can therefore leave the caller's cache
  stale. With only 64 short definitions, the preferred initial fix is to
  remove all promoted entries and reclassify the whole macro table in passes
  after every definition/removal. Continue until no more definitions become
  resolvable; then report remaining cycles or unresolved dependencies. Add a
  dependency graph only if definition-time recompilation becomes measurable.
- **Serialize mutation.** Macro storage and the global dictionary are mutable
  process-global structures without internal synchronization. Funnel local,
  UDP, and other command ingestion through one control-thread owner, or guard
  macro storage plus dictionary lookup/mutation with one documented lock. A
  definition changes both structures and should be treated as one transaction.
- **Refine status taxonomy.** Keep `immediate` distinct from errors: it is a
  valid control-thread macro. During whole-table reclassification, distinguish
  temporarily `unresolved` dependencies and cycles from permanently `invalid`
  syntax, and retain the existing `too-large` result.
- **Keep ownership aligned.** Macros are process-global today, so their cached
  words belong in the global vocabulary. Do not introduce per-context macro
  promotion until macro storage itself becomes context-local.
- **Split the implementation when it grows.** Macro classification, template
  compilation, promotion, and invalidation are natural candidates for a
  future `skode-macro.c`; generic lookup/registration and built-in words should
  remain in `skode-dict.c`.

Unless deliberately documented otherwise, nested named macros should retain
their historical live-definition semantics rather than silently becoming
permanent snapshots.
