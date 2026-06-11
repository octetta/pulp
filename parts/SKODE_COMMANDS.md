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
| `c` | `[mode [, depth]]` | `SKODE_OP_PHASE_DISTORTION` | `cz_set()` | `PD` |
| `C` | `[voice, depth]` | `SKODE_OP_PHASE_MOD` | `cmod_set()`; fewer than two args disable modulation | `PD` |
| `ft` | `attack, decay, sustain, release` | `SKODE_OP_FILTER_ENVELOPE` | `envelope_init_e()` on the filter envelope | `FILT`, `FADSR` |
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
| `s` | `amount` | `SKODE_OP_SMOOTHER` | Enables or disables amplitude smoothing | `SMOOTHER` |
| `S` | `voice` | `SKODE_OP_VOICE_RESET` | `wave_reset()` | base |
| `t` | `attack, decay, sustain, release` | `SKODE_OP_ENVELOPE` | `envelope_set()` | `ADSR` |
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

## Voice, Wave, and Synth Control

The following commands are immediate-only even when related voice opcodes are
schedulable.

| Command | Arguments or string | Behavior | Main function or state |
| --- | --- | --- | --- |
| `V` | `dB` | Set main output volume | `volume_set()` |
| `[name] vt` | string | Set selected voice label | `sv.text[]` |
| `[name] wt wave` | string, numeric | Set wavetable label | `sw.name[]` |
| `W` | `[wave [, end-or-width [, height]]]` | Show wavetable or recording data | `wavetable_show()`, waveform display helpers |
| `W@` | `wave,param[,register]` | Read wave size, rate, or duration | `sw.size[]`, `sw.rate[]`, `ands_set_local()` |
| `v@` | `param[,register]` | Read selected voice wave, amplitude, or frequency | `sv` fields, `ands_set_local()` |
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
| `D` | `[capacity]` | Show or increase parser data capacity | `ands_data_cap()`, `ands_data_resize()` |
| `/D` | `[capacity]` | Resize data and print capacity details | `ands_data_resize()` |
| `?d` | none | Print parser data | `skode_double_dump()` |
| `d@` | `index` | Print one data element | `ands_data()` |
| `=d` | `register,index` | Copy a data element to a local register | `ands_set_local()` |
| `=` | `[register [, value]]` | Set, inspect, or list registers | `ands_set_local()`, `ands_get_local()` |
| `*=` | `register,a,b` | Store `a * b` | `ands_set_local()` |
| `/=` | `register,a,b` | Store `a / b` when `b != 0` | `ands_set_local()` |
| `a=` | `register,a,b` | Store `a + b` | `ands_set_local()` |
| `s=` | `register,a,b` | Store `a - b` | `ands_set_local()` |
| `l>g` | `register` | Copy local register to global register | `ands_local_to_global()` |
| `g>l` | `register` | Copy global register to local register | `ands_global_to_local()` |

The two meanings of `=` share syntax. `=N,value` is also accepted by the
scheduled opcode compiler and writes the global register array when executed.

## String Buffers and Ksynth

Skode has `SKODE_EXTRA_MAX` (128) external string buffers of 256 bytes each.

| Command | Arguments or string | Behavior | Main function |
| --- | --- | --- | --- |
| `[text] e> index` | string | Copy parser string to external buffer | `skode_copy_string()` |
| `<e index` | numeric | Copy external buffer to parser string | `ands_string_from_external()` |
| `e? [index]` | optional numeric | Show one or all non-empty buffers | direct buffer inspection |
| `e! index` | literal numeric | Compile and inline a macro buffer; schedulable | `skode_extra_copy()`, `skode_compile_program()` |
| `eR index,count,seconds[,tag]` | numeric | Repeat a compiled macro snapshot using seconds | `skode_repeat_macro()` |
| `eRR index,count,beats[,tag]` | numeric | Repeat a compiled macro snapshot using tempo | `skode_repeat_macro()` |
| `[file] /ks [verbose]` | string | Load a named Ksynth source file | `ksynth_load_name()` |
| `/k file-number[,verbose]` | numeric | Load numbered Ksynth source | `ksynth_load()` |
| `[code] ks`, `[code] k!` | string | Submit Ksynth source | `skode_ks_submit()` |
| `kw [timeout-ms]` | optional numeric | Wait for the latest Ksynth request | `skode_ks_wait()` |
| `kw> [timeout-ms]` | optional numeric | Wait and copy result to parser data | `skode_ks_result_to_data()` |
| `k?` | none | Print latest Ksynth result | `kse_result_copy()` |
| `k>d` | none | Copy latest Ksynth result to parser data | `skode_ks_result_to_data()` |

Ksynth commands require the `KSYNTH` feature and are immediate-only.

## Files, Samples, and Recording

| Command | Arguments or string | Behavior | Main function |
| --- | --- | --- | --- |
| `/l file-number[,verbose]` | numeric | Load a numbered `.sk` file | `skode_load()` |
| `[filename] /ws wave[,channel]` | string | Load an audio file by name | `wave_load_string()` |
| `/w file-number[,wave[,channel]]` | numeric | Load a numbered audio file | `wave_load()` |
| `<r seconds`, `^r seconds` | numeric | Record audio into the sample buffer | `skode_sample_go()` |
| `>r number` | numeric | Normalize sample buffer and write `out-N.wav` | miniaudio encoder API |
| `[filename] /rg [max-seconds]` | string | Start multitrack recording | `recorder_start()` |
| `/rs` | none | Stop multitrack recording | `recorder_stop()` |
| `/r?` | none | Show multitrack recorder status | recorder status functions |
| `[filename] /cat` | string | Print a text file | `fopen()`, `fgets()` |
| `[directory] /cd` | string | Change working directory | `chdir()` |
| `/ls [type]` | optional numeric | List files; `0=.sk`, `1=.wav`, `2=.mp3`, `3=.ks` | `opendir()`, `readdir()` |

Multitrack commands require the `RECORD` feature.

## Diagnostics and Runtime Control

| Command | Arguments | Behavior | Main function or state |
| --- | --- | --- | --- |
| `?`, `v?` | none | Show selected voice | `voice_show()` |
| `\` | none | Show selected voice verbosely | `voice_show()` |
| `??`, `v??` | none | Show active voices | `voice_show_all()` |
| `?s` | none | Show parser string | `ands_string()` |
| `/s [section]` | optional numeric | Show system, audio, synth, Skode, string, or benchmark state | `system_show()` and related helpers |
| `/t [level]` | optional numeric | Toggle or set command/parser tracing | `ctx->trace`, `ands_trace_set()` |
| `/v [level]` | optional numeric | Toggle or set verbose output | `ctx->verbose` |
| `/c [state]` | optional numeric | Show or set parser chunk mode | `ands_chunk_mode()` |
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
| `KSYNTH` | `/ks`, `/k`, `ks`, `k!`, `kw`, `kw>`, `k?`, `k>d` |
| `PANMOD` | `P` |
| `PD` | `c`, `C` |
| `RECORD` | `r`, `/rg`, `/rs`, `/r?` |
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
