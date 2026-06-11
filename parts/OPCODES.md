# Skode Scheduled Opcodes

Scheduled work executed by the audio callback consists only of fixed-size
numeric opcode events. Skode text is parsed and compiled on a command or UDP
thread before it enters a queue, defer, repeat, or sequence.

Commands that require a string, data array, parser-owned memory, file access,
or other control-thread state are immediate-only. Compilation rejects the
entire scheduled program instead of retaining text as a fallback.

Literal external macro calls such as `e!12` are expanded on the control thread
while the containing pattern, defer, repeat, or execute-string is compiled.
The resulting program is a snapshot: later changes to external buffer 12 do
not alter already compiled or queued events. Nested macros are supported, but
undefined buffers, recursive cycles, runtime-selected `e!$n` calls, and
expansions beyond `SEQ_PROGRAM_OP_MAX` are rejected.

`eR macro,count,seconds[,tag]` and `eRR macro,count,beats[,tag]` copy and
compile the selected external macro once on the control thread, then queue
repeated invocations of that snapshot. Repetition does not duplicate the
program's opcodes, so `SEQ_PROGRAM_OP_MAX` applies to each invocation rather
than to the total number of repeats.

## Compiled Commands

The base voice command set includes:

- voice selection, copying, reset, trigger, default wave, and wave selection
- amplitude, frequency, MIDI note/detune, pan, mute, direction, and looping
- velocity, linked MIDI/velocity voices, and `L` trigger delay
- ADSR mode/envelope, filtering, smoothing, glissando, and sample-and-hold
- AM, FM, pan, phase-distortion, and ring modulation
- quantization and per-voice recording-track selection

Feature-dependent commands compile only when their synth feature is enabled.
Ksynth commands, status/printing commands, and commands using strings or data
tables remain immediate-only.

`=slot,value` is also supported so sequence programs can update registers used
by later voice opcodes. Other administrative variable commands remain
immediate-only.

## Programs

An `event_program_t` contains at most `SEQ_PROGRAM_OP_MAX` (32) operations. It
can include `+` tempo-relative and `~` seconds-relative delay markers.
Executing a program runs due operations directly and queues future operations
as typed events.

`$n` operands remain register references in the compiled representation. They
are resolved when each opcode executes, including deferred events and
variable-selected voices. The explicit `-` default sentinel is retained for
commands such as `n-` and `N-`.

Sequence steps retain their source text for display and editing, but playback
uses the compiled program stored alongside it. Voice selection is persistent
per pattern and resets to voice 0 when the pattern is cleared. Empty steps and
the `-` stop marker do not require a compiled program. Comment-only steps such
as `#` compile to a zero-operation program, preserving their sequence position
without invoking the parser during playback.

Repeat, conditional execute, explicit execute-string, defer, and sequence-edit
commands all compile before scheduling. A compile failure leaves the queue or
sequence step unchanged and reports that the command is not schedulable.

## Real-Time Boundary

The queue contains `event_t` values only:

- voice index
- opcode identifier
- bounded argument count
- fixed numeric argument array

There are no command strings, data arrays, borrowed parser pointers, parser
calls, allocations, or string formatting in queued event execution. The
compiler itself allocates an `ands` parser and therefore must remain on the
control thread.

Future scheduled resource commands should carry a stable engine resource ID,
not a string or pointer to parser-owned storage.

## Diagnostics

- `?o` shows queued opcode events with IDs, tags, relative times, voices, and
  symbolic opcode arguments.
- `?o-1` shows the currently selected pattern.
- `?oN` shows compiled programs for pattern `N`.
- `?oN,S` shows one step `S` in pattern `N`.

Pattern diagnostics include the original source and render register operands
as `$n`, default sentinels as `-`, delays with `+` or `~`, and comment-only
steps as `(no-op)`.
