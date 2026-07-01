# Skode Commands and Their Implementation

This document describes the Skode command language implemented by
`skode.c.kit`, `skode-event.c`, and the synth and sequencer modules behind
them.

Skode is compact by design. A command name is usually one to four punctuation
or letter characters followed by numeric arguments. Arguments may be
separated by commas or whitespace, and commands may be placed next to each
other:

```text
v0 w0 f220 a-8 l1
v1n60l1
```

Strings are written in square brackets and numeric data arrays in
parentheses:

```text
[v0 n45 l1] xa
[sk/drums-kick.ks] /ks
(0 0.5 1 0.5 0) /d300,44100,1
```

Skode is case-sensitive. For example, `a` controls amplitude while `A`
controls amplitude modulation.

## Execution Model

The main entry point is:

```c
int skode_consume(char *line, skode_t *ctx);
```

The execution path is:

1. `skode_consume()` passes text to the `ands` parser.
2. `skode_callback()` receives parser events.
3. Ordinary commands are dispatched by `skode_function()`.
4. Deferred, repeated, and sequence commands are compiled by
   `skode_compile_program()`.
5. Compiled commands become fixed-size `event_program_t` and `event_t`
   values.
6. `skode_execute_event()` resolves register operands and calls
   `skode_execute_voice_opcode()`.
7. The final opcode handler calls synth functions such as `amp_set()`,
   `wave_set()`, or `envelope_set()`.

This division matters: immediate commands may use strings, files, allocation,
or parser state. Scheduled commands must be representable as bounded numeric
opcodes and may run from the audio callback.

## Values, Registers, and Timing

- `vN` selects voice `N` in the current `skode_t` context.
- `$N` reads register `N`. There are `ANDS_VAR_MAX` (128) registers.
- `=N,value` writes a register. This command is schedulable.
- `-` is an explicit default value for commands that support it, notably
  `n-` and `N-`.
- `+delay command` defers by a tempo-relative amount.
- `~seconds command` defers by seconds.
- Compiled programs contain at most `SEQ_PROGRAM_OP_MAX` (32) operations.
- Each compiled opcode contains at most `SEQ_OPCODE_ARG_MAX` (4) arguments.

Register operands remain symbolic in compiled programs and are resolved when
the event executes. This permits commands such as:

```text
=0,3 v$0 n60 l1
```

## Schedulable Voice Opcodes

The commands in this section can be compiled for queues, repeats, deferred
execution, and sequence steps. Feature-gated commands are available only when
their named build option is enabled.

| Command | Arguments | Opcode | Function or state changed | Feature |
| --- | --- | --- | --- | --- |
| `v` | `voice` | `SKODE_OP_VOICE` | Changes the current voice inside the compiled program | base |
| `a` | `dB` | `SKODE_OP_AMP` | `amp_set()` | base |
| `f` | `Hz` | `SKODE_OP_FREQ` | `freq_set()` | base |
| `n` | `note [, cents]` | `SKODE_OP_MIDI_NOTE` | `skode_midi_note()` then `freq_midi()` | base |
| `p` | `pan` | `SKODE_OP_PAN` | `pan_set()` | base |
| `l` | `velocity` | `SKODE_OP_VELOCITY` | `skode_envelope_velocity()` and linked voices | base |
| `___l` | `velocity` | `SKODE_OP_ENVELOPE_VELOCITY` | `envelope_velocity()` without link propagation | internal |
| `A` | `[voice, depth [, offset]]` | `SKODE_OP_AMP_MOD` | `amp_mod_set()`; fewer than two args disable AM | `AM` |
| `b` | `[direction]` | `SKODE_OP_WAVE_DIRECTION` | `wave_dir()`; no argument toggles | base |
| `B` | `[loop]` | `SKODE_OP_WAVE_LOOP` | `wave_loop()`; no argument toggles | base |
| `BC` | `count` | `SKODE_OP_WAVE_LOOP_COUNT` | Sets bounded one-shot loop repeats; `0` means unlimited | base |
| `c` | `[mode [, depth]]` | `SKODE_OP_PHASE_DISTORTION` | `cz_set()` | `PD` |
| `C` | `[voice, depth]` | `SKODE_OP_PHASE_MOD` | `cmod_set()`; fewer than two args disable modulation | `PD` |
| `ft` | `attack, decay, sustain, release` | `SKODE_OP_FILTER_ENVELOPE` | Configures the filter envelope for its next trigger | `FILT`, `FADSR` |
| `fd` | `depth` | `SKODE_OP_FILTER_ENVELOPE_DEPTH` | Sets `sv.filter_env_depth` | `FILT`, `FADSR` |
| `F` | `[voice, depth [, offset]]` | `SKODE_OP_FREQ_MOD` | `freq_mod_set()`; zero or one arg disables FM | `FM` |
| `FF` | `mode` | `SKODE_OP_FREQ_MOD_MODE` | Sets `sv.freq_mod_mode` | `FM` |
| `g` | `time` | `SKODE_OP_GLISSANDO` | Sets `sv.glissando_enable` and `sv.glissando_time` | `GLISS` |
| `G` | `voice [, voice ...]` | `SKODE_OP_LINK_MIDI` | Sets up to four `sv.link_midi_*` voices | base |
| `h` | `phase-count` | `SKODE_OP_SAMPLE_HOLD` | Sets `sv.sample_hold_max` | `SAH` |
| `H` | `voice [, voice ...]` | `SKODE_OP_LINK_VELOCITY` | Sets up to four `sv.link_velo_*` voices | base |
| `L` | `seconds` | `SKODE_OP_TRIGGER_DELAY` | Sets `sv.link_trig` and `sv.link_trig_samp` | base |
| `J` | `mode` | `SKODE_OP_FILTER_MODE` | Sets filter mode and calls `mmf_set_params()` | `FILT` |
| `K` | `Hz` | `SKODE_OP_FILTER_FREQ` | `mmf_set_freq()` | `FILT` |
| `k` | `mode` | `SKODE_OP_ENVELOPE_MODE` | Sets `sv.amp_envelope_mode` | `ADSR` |
| `m` | `state` | `SKODE_OP_MUTE` | `wave_mute()` | base |
| `N` | `semitones [, cents]` | `SKODE_OP_MIDI_DETUNE` | Sets `sv.midi_transpose` and `sv.midi_cents` | base |
| `P` | `[voice, depth [, offset]]` | `SKODE_OP_PAN_MOD` | `pan_mod_set()`; fewer than two args disable modulation | `PANMOD` |
| `q` | `bit-depth` | `SKODE_OP_QUANTIZE` | `wave_quant()` | `CRUSH` |
| `Q` | `resonance` | `SKODE_OP_FILTER_RESONANCE` | `mmf_set_res()` | `FILT` |
| `r` | `track` | `SKODE_OP_RECORD_TRACK` | `synth_record_track_set()` | `RECORD` |
| `rt` | `[name] track` | immediate | `synth_track_name_set()` | `RECORD` or `SCOPE` |
| `rv` | `track,dB` | immediate | `synth_track_volume_set()` | `RECORD` or `SCOPE` |
| `?r` | none | immediate | show track names, volumes, and assigned voices | `RECORD` or `SCOPE` |
| `s` | `amount` | `SKODE_OP_SMOOTHER` | Enables or disables amplitude smoothing | `SMOOTHER` |
| `S` | `voice` | `SKODE_OP_VOICE_RESET` | `wave_reset()` | base |
| `t` | `attack, decay, sustain, release` | `SKODE_OP_ENVELOPE` | Configures the amplitude envelope for its next trigger | `ADSR` |
| `T` | none | `SKODE_OP_TRIGGER` | Calls `envelope_velocity(..., 1)` on the voice and velocity links | base |
| `w` | `wave [, interpolate [, one-shot]]` | `SKODE_OP_WAVE` | `wave_set()` and optional voice flags | base |
| `>` | `destination-voice` | `SKODE_OP_VOICE_COPY` | `voice_copy(current, destination)` | base |
| `/` | none | `SKODE_OP_WAVE_DEFAULT` | `wave_default()` | base |
| `=` | `register, value` | `SKODE_OP_VARIABLE_SET` | Writes `global_var[register]` | base |
| `XM` | `voice [, amount]` | `SKODE_OP_RING_MOD` | Sets `sv.ring_osc` and `sv.ring_amount` | `XM` |

### Notes on Selected Opcodes

`n` accepts a MIDI note number and optional cents offset. `n-` reuses the
voice's last MIDI note. `skode_midi_note()` also propagates the note to voices
configured by `G`.

`l` applies velocity immediately or schedules it using the voice's trigger
delay. It also propagates velocity to voices configured by `H`.

`b`, `B`, `BC`, and `l` have separate playback responsibilities. `b` selects
direction, `B` configures wrapping, and `BC` configures a bounded number of
one-shot wraps. Zero is unlimited; a positive count means repeats after the
initial loop traversal, so `BC1` permits one wrap and two traversals. Positive
`l` or `T` snapshots the `BC` bound for the new note. `B` remains an immediate
runtime switch. `l0` releases the envelopes immediately. Bounded loops also
request departure at the next loop boundary; unbounded loops keep looping under
normal ADSR release behavior.

`B0` clears active loop runtime state. `B1` immediately starts a fresh active
loop snapshot from the current `BC` configuration, so stale counted-loop
remaining state does not carry into a later unbounded loop.

Amplitude envelope mode `k1` enables timed one-shot ASR. For non-looping
one-shots and bounded `BC` one-shot loops, positive `l` schedules the release
phase to finish at the natural playback end. The `t` attack, sustain, and
release values shape the result; decay remains available but is usually not
useful for this mode. Unbounded `BC0`/`B1` loops keep normal held ADSR behavior
and release only on `l0`.

At trigger time, `osc_trigger()` initializes `loop_active`, `loop_bounded`, and
`loop_remaining`. `osc_next()` consumes wraps in either direction, including
multiple boundaries crossed by one phase increment. A requested or natural
exit raises `loop_ended`; the render loop consumes that event to release active
amplitude and filter envelopes at the exact sample while playback continues
through the one-shot tail.

`c` phase-distortion modes are:

| Mode | Shape |
| --- | --- |
| `0` | off |
| `1` | saw to pulse |
| `2` | folded sine |
| `3` | triangle |
| `4` | double sine |
| `5` | saw to triangle |
| `6` | resonant 1 |
| `7` | resonant 2 |

`w` changes the selected wavetable. The optional second and third arguments
set interpolation and one-shot playback flags.

`S` takes an explicit voice number rather than using the selected voice. In
the current implementation, an out-of-range voice invokes `voice_init()` and
resets every voice; `S100` therefore acts as reset-all with the default voice
limit. This sentinel behavior is retained for compatibility but is not a
dedicated reset-all command.

## Scheduling and Queue Commands

These commands run on the control thread but compile their string contents
into the opcode representation above.

| Command | Input | Behavior | Main functions |
| --- | --- | --- | --- |
| `+delay ...` | deferred commands | Tempo-relative defer | `skode_defer()`, `skode_queue_program()` |
| `~seconds ...` | deferred commands | Seconds-relative defer | `skode_defer()`, `skode_queue_program()` |
| `[commands] R count,seconds[,tag]` | string | Repeat using seconds | `skode_compile_program()`, `skode_queue_program()` |
| `[commands] RR count,steps[,tag]` | string | Repeat using tempo-relative step duration | `skode_compile_program()`, `skode_queue_program()` |
| `eR macro,count,seconds[,tag]` | external string | Compile a macro snapshot and repeat it using seconds | `skode_extra_copy()`, `skode_compile_program()`, `skode_queue_program()` |
| `eRR macro,count,beats[,tag]` | external string | Compile a macro snapshot and repeat it using tempo-relative beat duration | same as above |
| `[commands] DO? value[,tag]` | string | Queue once when `value > 0` | `skode_compile_program()`, `skode_queue_program()` |
| `R! tag` | numeric | Remove queued events with a tag | `seq_kill_by_tag()` |
| `R!!` | none | Remove all queued events | `seq_kill_all()` |
| `[commands] e!` | string | Compile and queue the parser string now | `skode_compile_program()`, `skode_queue_program()` |
| `e! index` | external string | Expand a literal external buffer while compiling, then queue it | same as above |
| `?o` | none | Show queued opcode events | `opcode_queue_show()` |
| `?o pattern[,step]` | numeric | Show compiled pattern programs | `opcode_pattern_show()` |

Commands containing strings, arrays, file operations, or other immediate-only
operations are rejected by `skode_compile_program()`. There is no fallback
that sends command text into the audio callback.

Literal external macro references such as `e!12` are the exception: the
compiler reads buffer 12 on the control thread and inlines its compiled
opcodes. This works inside patterns, defers, repeats, and nested macros.
Expansion uses snapshot semantics, so editing buffer 12 later does not change
an existing pattern or queued event. Undefined buffers, recursive cycles,
`e!$N`, and expansions beyond 32 operations are rejected. Argumentless `e!`
continues to mean "execute the current parser string" and is immediate-only.
`eR` and `eRR` use the same snapshot compilation but schedule the resulting
program repeatedly without expanding it once per repetition.

## Pattern and Sequence Commands

Sequence support is compiled under the `SEQ` feature.

| Command | Arguments or string | Behavior | Main function or state |
| --- | --- | --- | --- |
| `M` | `bpm` | Set tempo from 1 to 960 BPM | `tempo_set()` |
| `y` | `pattern` | Select the editing pattern | `ctx->pattern` |
| `[text] yt` | string | Name the selected pattern | `seq_text[]` |
| `ym` | `0/1` | Mute the selected pattern | `seq_mute_set()` |
| `Y` | `pattern` | Clear a pattern | `pattern_reset()` |
| `[commands] xa` | string | Append a compiled step | `seq_step_append()` |
| `[commands] x step` | string | Set a compiled step; `x-` advances the edit cursor | `seq_step_set()` |
| `<x step` | numeric | Copy a step's source text into the parser string | `seq_step_get()` |
| `xg step`, `>x step` | numeric | Jump pattern playback to a step | `seq_step_goto()` |
| `%` | `modulus` | Set the selected pattern's modulus | `seq_modulo_set()` |
| `z` | `[0..3]` | Set selected pattern state (`stop`, `start`, `pause`, `resume`), or show it | `seq_state_set()`, `pattern_show()` |
| `z?` | none | Show selected pattern | `pattern_show()` |
| `Z` | `[0..3]` | Set all pattern states, or show all patterns | `seq_state_all()`, `pattern_show()` |
| `Z?`, `z??` | none | Show all patterns with steps | `pattern_show()` |

Sequence steps retain their source text for editing and diagnostics, but
playback uses the compiled `event_program_t`. Each pattern keeps its own
persistent current voice during playback. Clearing a pattern resets that
selection to voice 0 before the pattern next runs.

Tempo is limited to 960 BPM. At four sequence steps per beat this is 64 steps
per second, which remains practical for control-rate sequencing while avoiding
unbounded catch-up work in the audio callback. If processing falls behind,
SKRED executes at most 64 missed pattern ticks in one callback and resumes from
the current timeline.

The audio callback renders adaptively around queued events and pattern clock
ticks. A block containing a timing boundary is split at that sample; a block
without one is rendered in a single call. Integer-sample events are exact, and
fractional tempo boundaries are rounded forward by less than one sample.
Device and hardware buffering latency is unaffected.

## Voice, Wave, and Synth Control

The following commands are immediate-only even when related voice opcodes are
schedulable.

| Command | Arguments or string | Behavior | Main function or state |
| --- | --- | --- | --- |
| `V` | `dB` | Set main output volume | `volume_set()` |
| `[name] vt` | string | Set selected voice label | `sv.text[]` |
| `[name] wt wave` | string, numeric | Set wavetable label | `sw.name[]` |
| `W` | `[wave [, end-or-width [, height]]]` | Show wavetable or recording data | `wavetable_show()`, waveform display helpers |
| `W@` | `wave,param[,register]` | Read wave size, rate, or duration | `sw.size[]`, `sw.rate[]`, register write |
| `v@` | `param[,register]` | Read selected voice wave, amplitude, or frequency | `sv` fields, register write |
| `w>d` | `wave` | Copy wavetable samples to parser data | `sw`, `ands_data_resize()` |
| `w>r` | `wave` | Copy wavetable samples to recording buffer | `skode_sample_alloc()` |
| `d>r` | none | Copy parser data to recording buffer | `skode_sample_alloc()` |
| `w!` | none | Apply current recording offset and trim | recording-buffer state |
| `w@` | none | Reset recording offset and trim | recording-buffer state |
| `w>` | `[samples]` | Move recording start offset | `sampling.offset` |
| `w<` | `[samples]` | Increase or decrease end trim | `sampling.trim` |
| `w<>` | `[threshold [, headroom]]` | Find recording trim points | `record_find_trim()` |
| `/r` | `[slot [, one-shot [, offset]]]` | Load recording buffer into a wave slot | `rec_load()` |
| `/d` | `[slot [, rate [, one-shot [, offset]]]]` | Load parser data into a wave slot | `data_load()` |
| `/wex` | `wave` | Expand a dynamic wave slot in the 200-999 range | `wave_table_dynamic_expand()` |

For `W@`, parameter `0` is sample count, `1` is sample rate, and `2` is
duration (`size / rate`). For `v@`, parameter `0` is wave index, `1` is user
amplitude, and `2` is frequency.

## Data and Register Commands

| Command | Arguments | Behavior | Main function |
| --- | --- | --- | --- |
| `dup` | numeric stack | Duplicate the first pending argument for the next atom | `ands_arg_dup()` |
| `drop` | numeric stack | Drop the first pending argument for the next atom | `ands_arg_drop()` |
| `swap` | numeric stack | Swap the first two pending arguments for the next atom | `ands_arg_swap()` |
| `over` | numeric stack | Copy the second pending argument to the front | `ands_arg_over()` |
| `rot` | numeric stack | Rotate the first three pending arguments left | `ands_arg_rot()` |
| `clr` | numeric stack | Clear pending arguments before the next atom | `ands_arg_clear()` |
| `D` | `[capacity]` | Show or increase parser data capacity | `ands_data_cap()`, `ands_data_resize()` |
| `/D` | `[capacity]` | Resize data and print capacity details | `ands_data_resize()` |
| `?d` | none | Print parser data | `skode_double_dump()` |
| `d@` | `index` | Print one data element | `ands_data()` |
| `=d` | `register,index` | Copy a data element to a shared register | register write |
| `=` | `[register [, value]]` | Set, inspect, or list shared registers | register read/write |
| `*=` | `register,a,b` | Store `a * b` | register write |
| `/=` | `register,a,b` | Store `a / b` when `b != 0` | register write |
| `a=` | `register,a,b` | Store `a + b` | register write |
| `s=` | `register,a,b` | Store `a - b` | register write |

The two meanings of `=` share syntax. `=N,value` is also accepted by the
scheduled opcode compiler and writes the shared register array when executed.
The immediate read/math forms `d@`, `W@`, `v@`, `=`, `*=`, `/=`, `a=`, and
`s=` also return their result as the next command's pending first argument
when they are followed by another atom in the same chunk.

### Macro and Composition Examples

ANDS macros are global shortcuts. Define them with `[name] : body ;`.
Names use the same four-character atom width as commands; longer definition
names are silently truncated to four characters. Macro arguments are written
as `$$0`..`$$7`; the older `@0`..`@7` form is still accepted for compatibility.
Plain `$0` keeps its normal meaning as a shared register reference after the
macro expands. Because macros are global, scripts loaded with `/l` can define
macros for later interactive use.

```skode
[ar] : t $$0 0 $$1 0 ;
ar 0.01 0.4
```

The example above expands to `t 0.01 0 0.4 0`, which configures an attack /
release-style amplitude envelope using the normal `t` command.

Macros can bundle small voice recipes:

```skode
[one] : w $$0 1 1 k1 t $$1 0 $$2 $$3 ;
one 12 0.002 1 0.08
```

That selects wave `12`, enables interpolation and one-shot playback, enables
timed ASR mode, and configures the envelope. Macro definitions can be inspected
and removed from the current parser context:

```skode
?m
[one] /m
/m!
```

Read commands can feed later commands in the same chunk. The return value is
left as the next command's first pending argument:

```skode
1 v@ a       # read selected voice amplitude and feed it to a
0 3 4 a= a  # store 3+4 in $0, then feed 7 to a
(2 5 8) 1 d@ f
```

The stack helpers rearrange pending numeric arguments before the next atom:

```skode
0 5 swap v  # v receives 5 instead of 0
7 3 drop v  # v receives 3
4 2 over v  # v receives 2, with the copied value at the front
1 6 7 rot v # v receives 6
```

Context-local string slots are useful scratch pads that do not use the global
external string buffers:

```skode
[lead voice] 0 s>
[temporary] vt
0 <s vt
s?
```

Runtime string formatting replaces `@0`..`@7` in the current parser string
with numeric arguments:

```skode
[take-@0.wav] 12 s% /rg 10
[voice @0 wave @1] 3 14 s% vt
```

## String Buffers and Ksynth

Skode has `SKODE_EXTRA_MAX` (128) external string buffers of 256 bytes each.

| Command | Arguments or string | Behavior | Main function |
| --- | --- | --- | --- |
| `[text] e> index` | string | Copy parser string to external buffer | `skode_copy_string()` |
| `<e index` | numeric | Copy external buffer to parser string | `ands_string_from_external()` |
| `e? [index]` | optional numeric | Show one or all non-empty buffers | direct buffer inspection |
| `[text] s> index` | string | Copy parser string to a context-local string slot | `ctx->string_slot[]` |
| `<s index` | numeric | Copy a context-local string slot to parser string | `ands_string_from_external()` |
| `s? [index]` | optional numeric | Show one or all non-empty context-local string slots | direct slot inspection |
| `[template] s% args...` | string and numeric | Replace `@0`..`@7` in parser string with numeric args | `skode_format_string_args()` |
| `e! index` | literal numeric | Compile and inline a macro buffer; schedulable | `skode_extra_copy()`, `skode_compile_program()` |
| `eR index,count,seconds[,tag]` | numeric | Repeat a compiled macro snapshot using seconds | `skode_repeat_macro()` |
| `eRR index,count,beats[,tag]` | numeric | Repeat a compiled macro snapshot using tempo | `skode_repeat_macro()` |
| `[file] /ks [verbose]` | string | Load a named Ksynth source file | `ksynth_load_name()` |
| `/k file-number[,verbose]` | numeric | Load numbered Ksynth source | `ksynth_load()` |
| `[code] ks`, `[code] k!` | string | Run Ksynth source in this Skode context | `skode_ks_eval()` |
| `kw [timeout-ms]` | optional numeric | Compatibility no-op; Ksynth now runs synchronously | parser command |
| `kw> [timeout-ms]` | optional numeric | Copy latest Ksynth result to parser data | `skode_ks_result_to_data()` |
| `k?` | none | Print latest Ksynth result | direct result inspection |
| `k>d` | none | Copy latest Ksynth result to parser data | `skode_ks_result_to_data()` |
| `d>k variable` | numeric `0..25` | Copy parser data to Ksynth `A..Z` | `ks_bind_vector()` |
| `w>k wave,variable` | numeric | Copy a wavetable directly to Ksynth `A..Z` | `ks_bind_vector()` |

Ksynth commands require the `KSYNTH` feature and are immediate-only.
Each vector is limited to one million elements. Each Skode command context
owns its own lazy Ksynth context and persistent `A..Z` variables; local input
and each UDP client therefore keep independent Ksynth state. `kw` remains
accepted for older scripts, but no longer waits for a worker. `kw>` and `k>d`
copy the latest numeric result into parser data. Wavetable metadata, including
rate, looping, and one-shot state, is not part of the transferred Ksynth vector.

## Files, Samples, and Recording

| Command | Arguments or string | Behavior | Main function |
| --- | --- | --- | --- |
| `/l file-number[,verbose]` | numeric | Load a numbered `.sk` file | `skode_load()` |
| `[filename] /ws wave[,channel]` | string | Load an audio file into a writable wave slot | `wave_load_string()` |
| `/w file-number[,wave[,channel]]` | numeric | Load a numbered audio file | `wave_load()` |
| `<r seconds`, `^r seconds` | numeric | Record audio into the sample buffer | `skode_sample_go()` |
| `>r number` | numeric | Normalize sample buffer and write `out-N.wav` | miniaudio encoder API |
| `[filename] /rg [max-seconds]` | string | Start multitrack recording | `recorder_start()` |
| `/rs` | none | Stop multitrack recording | `recorder_stop()` |
| `/r?` | none | Show multitrack recorder status | recorder status functions |
| `[name] /sg [channel-mask[,buffer-seconds]]` | string and numeric | Start shared-memory audio publication | `scope_ipc_start()` |
| `/ss` | none | Stop and unlink shared-memory publication | `scope_ipc_stop()` |
| `/s?` | none | Show shared-memory publication status | scope IPC status functions |
| `[filename] %cat` | string | Print a text file | `fopen()`, `fgets()` |
| `[directory] %cd` | string | Change working directory | `chdir()` |
| `%ls [type]` | optional numeric | List files; `0=.sk`, `1=.wav`, `2=.mp3`, `3=.ks` | `opendir()`, `readdir()` |

Multitrack file commands require `RECORD`. Shared-memory publication commands
require `SCOPE`. The `r` stem-routing command is available with either feature.

## Diagnostics and Runtime Control

| Command | Arguments | Behavior | Main function or state |
| --- | --- | --- | --- |
| `?`, `v?` | none | Show selected voice | `voice_show()` |
| `\` | none | Show selected voice verbosely | `voice_show()` |
| `??`, `v??` | none | Show active voices | `voice_show_all()` |
| `?s` | none | Show parser string | `ands_string()` |
| `?m` | none | Show global ANDS macros | `ands_macro_get()` |
| `[name] /m` | string | Remove one global ANDS macro | `ands_macro_remove()` |
| `/m!` | none | Clear all global ANDS macros | `ands_macro_clear()` |
| `/s [section]` | optional numeric | Show system, audio, synth, Skode, string, or benchmark state | `system_show()` and related helpers |
| `/t [level]` | optional numeric | Toggle or set command/parser tracing | `ctx->trace`, `ands_trace_set()` |
| `/v [level]` | optional numeric | Toggle or set verbose output | `ctx->verbose` |
| `/f [value]` | optional numeric | Show or set context flag | `ctx->flag` |
| `log state` | numeric | Enable or disable context log capture | `ctx->log_enable` |
| `udp value` | numeric | Show UDP endpoint information | context fields; `UDP` feature |
| `wait ms` | numeric | Block the control thread | `sk_sleep()` |
| `/m_` | none | Benchmark selected voice | `synth_voice_bench()`; `BENCH` feature |
| `/q` | none | Request shell exit | `ctx->quit` |

`I` currently parses a numeric argument but is a stub; it does not enable an
event logger.

## Build Features

Commands enclosed by `@if(...)` in the `.kit` sources only exist when those
features are enabled. Important command features include:

| Feature | Commands |
| --- | --- |
| `ADSR` | `k`, `t` |
| `AM` | `A` |
| `BENCH` | `/m_` and extra `/s` sections |
| `CRUSH` | `q` |
| `FADSR` with `FILT` | `ft`, `fd` |
| `FILT` | `J`, `K`, `Q` |
| `FM` | `F`, `FF` |
| `GLISS` | `g` |
| `KSYNTH` | `/ks`, `/k`, `ks`, `k!`, `kw`, `kw>`, `k?`, `k>d`, `d>k`, `w>k` |
| `PANMOD` | `P` |
| `PD` | `c`, `C` |
| `RECORD` | `r`, `rt`, `rv`, `?r`, `/rg`, `/rs`, `/r?` |
| `SCOPE` | `r`, `rt`, `rv`, `?r`, `/sg`, `/ss`, `/s?` |
| `SAH` | `h` |
| `SEQ` | timing, queue, and pattern commands |
| `SMOOTHER` | `s` |
| `UDP` | `udp` |
| `XM` | `XM` |

The normal WASM build enables the voice and sequence features listed in
`WASM_KIT_OPTS` in `Makefile`, but does not enable every optional command.

## Source Map

| Area | Source |
| --- | --- |
| Parser context and public Skode API | `skode.h` |
| Immediate command dispatch | `skode.c.kit`, `skode_function()` |
| Scheduled command compiler | `skode-event.c`, `skode_compile_program()` |
| Event and program execution | `skode-event.c`, `run_program()` |
| Voice opcode dispatch | `skode.c.kit`, `skode_execute_voice_opcode()` |
| Synth functions and state | `synth.c.kit`, `synth.h.kit` |
| Queue implementation | `skqueue.c`, `skqueue.h` |
| Sequence implementation | `seq.c.kit`, `seq.h.kit` |
| Parser implementation | `ands.c`, `ands.h` |
| Scheduled-opcode design notes | `OPCODES.md` |

When adding a new schedulable command, update all of the following:

1. `skode_opcode_t` in `skode.h`.
2. `skode_opcode_name()` in `skode-event.c`.
3. `skode_compile_callback()` in `skode-event.c`.
4. `skode_opcode_supported()` in `skode.c.kit`.
5. `skode_execute_voice_opcode()` in `skode.c.kit`.
6. The immediate command path in `skode_function()`.
7. Tests and this command reference.
