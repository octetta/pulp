# Skode User Command Reference

This is a practical reference for writing Skode. It describes command
parameters and what each command does to voices, sounds, patterns, or queued
events. For implementation details, see
[SKODE_COMMANDS.md](SKODE_COMMANDS.md).

## Reading Skode

Skode is case-sensitive. Commands may be separated by spaces or written
together:

```text
v0 w1 n60 a-8 l1
v0w1n60a-8l1
```

Numeric arguments may be separated by commas or spaces. Square brackets place
text in the parser's string buffer:

```text
[v0 n60 l1] R4,.25
```

`$N` reads shared register `N`. Compiled patterns and events read registers
when the command executes rather than when it is compiled:

```text
=0,60 v0 n$0 l1
```

The `Schedulable` column means a command can be used in pattern steps, defers,
repeats, and compiled external macros. A compiled step or event can contain at
most 32 operations.

## Voice and Pitch

| Command | Parameters | Effect | Schedulable |
| --- | --- | --- | --- |
| `v voice` | Integer voice index | Selects the voice affected by following commands. A pattern remembers its selected voice between steps. | Yes |
| `f hz` | Frequency in hertz | Sets the selected voice directly to an oscillator frequency. | Yes |
| `n note[,cents]` | MIDI note number; optional cents offset | Tunes the selected voice to a MIDI pitch. Fractional notes are accepted. `n-` reuses the last note. Notes also reach voices linked with `G`. | Yes |
| `N semitones[,cents]` | Transposition and fine detune | Offsets later `n` pitches for the selected voice. `N-` preserves the current semitone setting while allowing cents to be changed. | Yes |
| `g seconds` | Glide time; `0` disables | Glides from the current pitch to later pitch changes instead of jumping immediately. Requires `GLISS`. | Yes |
| `G voice[,voice...]` | Up to four voice indices | Sends later `n` pitch changes to the listed voices as well as the selected voice. A new `G` replaces the old link list. | Yes |
| `> destination` | Destination voice index | Copies the selected voice's synthesis settings to another voice. The destination becomes an independent copy. | Yes |
| `S voice` | Explicit voice index | Resets that voice's oscillator and synthesis state. An out-of-range index currently resets all voices; this is compatibility behavior rather than a dedicated command. | Yes |

## Level, Position, and Triggering

| Command | Parameters | Effect | Schedulable |
| --- | --- | --- | --- |
| `a dB` | Amplitude in decibels | Sets voice loudness. Values closer to `0` are louder; negative values attenuate. | Yes |
| `V dB` | Master amplitude in decibels | Changes the final output level for the entire synth, not just the selected voice. | No |
| `p pan` | Stereo position from `-1` to `1` | Moves the voice across the stereo field. `-1` is left, `0` is center, and `1` is right. | Yes |
| `m state` | `0` or nonzero | Mutes or unmutes oscillator output without deleting the voice settings. | Yes |
| `l velocity` | Envelope velocity | Triggers or updates the voice envelope with the supplied velocity. It also affects voices linked with `H`. | Yes |
| `T` | None | Retriggers the selected voice at velocity `1`, including velocity-linked voices. | Yes |
| `L seconds` | Trigger delay; `0` disables | Delays velocity/envelope triggering for the selected voice. This is a per-voice trigger delay, distinct from queue defers. | Yes |
| `H voice[,voice...]` | Up to four voice indices | Sends later `l` and `T` envelope triggers to the listed voices. A new `H` replaces the old link list. | Yes |
| `s amount` | Smoothing amount; `0` disables | Smooths amplitude changes to reduce abrupt level transitions. Requires `SMOOTHER`. | Yes |
| `vc state` | `0` or nonzero | Disables or enables synth-generated control-plane events for the selected voice. Disabled by default. Voice show commands include `vc1` only when enabled. | No |

## Wave Playback

| Command | Parameters | Effect | Schedulable |
| --- | --- | --- | --- |
| `w wave[,interpolate[,one-shot]]` | Wave index and optional boolean flags | Selects a wavetable. Interpolation smooths movement between samples. One-shot mode stops at the wave end instead of continuously cycling. | Yes |
| `/` | None | Restores the selected voice's default waveform settings. | Yes |
| `b [direction]` | `0` forward, `1` backward; no argument toggles | Changes the direction used to read the selected wave. | Yes |
| `B [loop]` | `0` off, `1` on; no argument toggles | Controls whether wave playback wraps at its end. This is most noticeable with sampled or one-shot waves. | Yes |
| `BC count` | Nonnegative integer; `0` means unlimited | Enables looping and limits a one-shot voice to `count` wraps after its first pass through the loop. The configured count is captured on `l` or `T`; changing it does not alter a note already playing. | Yes |
| `q bits` | Integer bit-depth control | Quantizes the waveform and adds bit-crusher distortion. Requires `CRUSH`. | Yes |
| `h phases` | Integer hold length | Holds oscillator values for multiple phases, producing stepped or sample-and-hold distortion. Requires `SAH`. | Yes |
| `[name] vt` | String | Gives the selected voice a display label. It does not alter sound. | No |
| `[name] wt wave` | String and wave index | Gives a wavetable a display label. It does not alter sound. | No |

### Direction, Looping, and Triggering

`b`, `B`, `BC`, and `l` describe different parts of one playback lifecycle:

- `b` selects reading direction. `b0` moves toward the wave or loop end;
  `b1` moves toward its start. It does not enable looping or trigger playback.
- `B` selects whether playback wraps at the active loop boundary. `B1` enables
  wrapping and `B0` disables it. With no argument, `B` toggles the setting.
  This applies immediately to the current playback as well as later triggers.
- `BC` enables looping and optionally bounds it for one-shot waves. `BC0`
  means unlimited wrapping. A positive value counts additional wraps after the
  first traversal of the loop region, so `BC1` plays that region twice: the
  initial traversal, then one repeat.
- A positive `l` value, or `T`, starts or retriggers a one-shot, initializes
  runtime looping from `B`, snapshots the current `BC` bound, resets the
  remaining wrap count, and triggers its envelopes. Changing `BC` during
  playback affects the next trigger; changing `B` affects the current one.
- `l0` releases active envelopes immediately. For a looping one-shot it also
  requests a clean loop exit: playback leaves the loop when it next reaches
  the boundary selected by `b`, then continues through any remaining wave tail.

When a positive `BC` count is exhausted without `l0`, the oscillator leaves the
loop at that boundary and automatically releases the active amplitude and
filter envelopes. Ordinary cyclic wavetables can still use `b` and `B`, but
bounded loop completion and one-shot retriggering apply only to one-shots.

```text
v0 w300 b0 BC1 t.01,.1,.8,.3 l1
```

This reads forward, traverses the loop region once, wraps once for a second
traversal, releases the envelope at the next loop boundary, and plays the wave
tail. Sending `l0` before that boundary releases the envelope early and uses
the same boundary-and-tail exit.

## Envelopes and Filters

### Amplitude ADSR

The `t` command defines an amplitude envelope as
`attack,decay,sustain,release`:

- **Attack** is the time in seconds taken to rise from the current envelope
  level to full level (`1`). Retriggering during a note begins from the
  envelope's current level.
- **Decay** is the time in seconds taken to fall from full level to the
  sustain level.
- **Sustain** is the level held after decay. It is a level, not a duration,
  and is clamped to the range `0` through `1`.
- **Release** is the time in seconds taken to fall from the current level to
  silence after a release command.

`l velocity` starts or retriggers both the amplitude and filter envelopes.
The envelope output is multiplied by the supplied velocity. `l0` releases the
envelopes; a nonzero sustain continues indefinitely until that release occurs.
`T` retriggers at velocity `1`.

Changing `t` or `ft` while an envelope is active does not reshape or restart
that envelope. The new settings are stored and take effect on the next `l` or
`T` trigger.

The Wave Playback section describes how `l`, `B`, and `BC` coordinate envelope
release with a one-shot loop boundary and waveform tail.

For example:

```text
v0 t.01,.2,.6,.5 n60 l1
```

This attacks in 10 ms, decays over 200 ms to 60% level, holds there, then
takes 500 ms to fade after `l0`.

### Filter Modes, Cutoff, and Resonance

`J`, `K`, and `Q` describe one filter:

- `J` selects the filter response or bypasses it.
- `K` sets its cutoff or center frequency in hertz.
- `Q` sets its quality factor. `Q0.707` is a useful neutral starting point.
  Higher positive values make the response narrower and more resonant around
  `K`; very low values make it broader and more damped.

Practical cutoff values are generally between `20` and `20000` Hz, subject to
the output sample rate. `Q` must be positive; values around `0.1` through `10`
cover most conventional uses, though the implementation does not impose that
range.

| `J` mode | Filter | How `K` and `Q` affect the sound |
| --- | --- | --- |
| `0` | Bypass | The filter is not processed. Stored `K` and `Q` values remain available when filtering is enabled again. |
| `1` | Low-pass | `K` is the cutoff: frequencies above it are reduced. Raising `Q` emphasizes the cutoff, adding bite or ringing. |
| `2` | High-pass | `K` is the cutoff: frequencies below it are reduced. Raising `Q` emphasizes the cutoff. |
| `3` | Band-pass | `K` is the center frequency that remains prominent. Raising `Q` narrows the audible band. |
| `4` | Notch | `K` is the center of the rejected frequency band. Raising `Q` narrows the notch. |
| `5` | All-pass | The magnitude is broadly retained while phase changes around `K`; `Q` controls the width of that phase transition. This is most audible through interaction, modulation, or mixing. |

Examples:

```text
J1 K800 Q.707   # smooth low-pass
J1 K800 Q5      # resonant low-pass
J2 K200 Q.707   # remove low-frequency content
J3 K1200 Q4     # narrow band centered near 1.2 kHz
J4 K1000 Q2     # notch around 1 kHz
```

### Filter ADSR

`ft attack,decay,sustain,release` defines a second ADSR with the same time and
level meanings as `t`. `fd depth` adds the envelope to the base `K` frequency:

```text
effective cutoff = K + (filter envelope * fd)
```

`fd` is therefore measured in hertz. A positive depth sweeps upward from `K`;
a negative depth sweeps downward. While the filter envelope is active, the
effective cutoff is clamped between `20` and `20000` Hz.

```text
v0 J1 K300 Q2 ft.01,.3,.1,.4 fd2500 n48 l1
```

This starts a low-pass sweep above the 300 Hz base cutoff, decays toward a
smaller sustained offset, and returns toward the base cutoff after `l0`.
`ft0,0,1,0` disables filter-envelope processing on the next trigger. An
envelope already in progress continues with the settings captured when it was
triggered.

| Command | Parameters | Effect | Schedulable |
| --- | --- | --- | --- |
| `t attack,decay,sustain,release` | Times in seconds; sustain level `0..1` | Sets the amplitude envelope triggered by `l` and `T` and released by `l0`. Requires `ADSR`. | Yes |
| `k mode` | Integer stored mode | Stores and reports the amplitude-envelope mode. The current envelope calculation does not branch on this value, so it presently has no audible effect. Requires `ADSR`. | Yes |
| `J mode` | `0` bypass, `1` low-pass, `2` high-pass, `3` band-pass, `4` notch, `5` all-pass | Selects the filter response. Requires `FILT`. | Yes |
| `K hz` | Cutoff or center frequency in hertz | Sets the frequency interpreted according to `J`. Requires `FILT`. | Yes |
| `Q quality` | Positive quality factor | Sets resonance or bandwidth around `K`; `0.707` is a useful neutral value. Requires `FILT`. | Yes |
| `ft attack,decay,sustain,release` | Times in seconds; sustain level `0..1` | Sets the filter envelope triggered and released with the amplitude envelope. Requires `FILT` and `FADSR`. | Yes |
| `fd depth` | Cutoff movement in hertz | Adds a positive or negative envelope-controlled offset to `K`. Requires `FILT` and `FADSR`. | Yes |

## Modulation and Timbre

For `A`, `F`, and `P`, `voice` selects the modulator voice and the control
signal is:

```text
control = modulator sample * depth + offset
```

The modulator sample is the actual output of that voice, so its waveform,
amplitude, envelope, and other processing affect the modulation. A simple
bipolar oscillator commonly moves around `-1` through `1`, but louder or
otherwise processed modulators may exceed that range.

- **Depth** scales how strongly the modulator moves the destination
  parameter. A negative depth reverses the direction of movement.
- **Offset** is the control value around which modulation occurs. It shifts
  the whole modulation range without changing its width.
- Omitting offset uses `0`.
- Supplying too few arguments to `A`, `F`, or `P` disables that modulation.

### Amplitude Modulation

For `A voice,depth,offset`, the control value directly multiplies the
destination voice's amplitude:

```text
output amplitude = normal amplitude * control
```

With a nominal `-1..1` modulator:

| Settings | Approximate multiplier | Result |
| --- | --- | --- |
| `A1,.5,.5` | `0..1` | Tremolo from silence to normal level |
| `A1,.25,.75` | `.5..1` | Gentler tremolo without reaching silence |
| `A1,1,0` | `-1..1` | Bipolar AM with phase inversion and stronger sidebands |
| `A1,0,1` | Constant `1` | No audible modulation, though the route remains enabled |

An offset of `0` is true bipolar amplitude modulation rather than conventional
volume-only tremolo. Negative control values invert waveform polarity.

### Pan Modulation

For `P voice,depth,offset`, the control value becomes the pan position:

```text
-1 = left, 0 = center, 1 = right
```

With a nominal `-1..1` modulator, `P1,1,0` moves across the full stereo field,
`P1,.5,0` moves between half-left and half-right, and `P1,.25,.5` moves on the
right side around pan `0.5`. Pan modulation is not clamped internally; keeping
the resulting control near `-1..1` avoids channel gain or polarity behavior
outside the normal pan range.

### Frequency Modulation

`F voice,depth,offset` first calculates the same control value, but `FF`
changes its meaning:

| Mode | Interpretation |
| --- | --- |
| `FF0` | Adds frequency movement to the carrier. Depth and offset scale the modulator oscillator's frequency, so the result depends on both the modulator pitch and sample value. This is the original relative-FM mode. |
| `FF1` | Treats the control value as the carrier's instantaneous frequency in hertz. Offset is the center frequency and depth is approximately the frequency deviation for a nominal `-1..1` modulator. |

Approximately, for ordinary oscillator waves:

```text
FF0 frequency = carrier Hz + modulator Hz * control
FF1 frequency = control
```

For example:

```text
v1 f5                    # slow modulator
v0 FF1 F1,20,220         # carrier sweeps approximately 200..240 Hz
```

In `FF1`, use a positive offset large enough that `offset - abs(depth)` does
not drive the instantaneous frequency below zero. At audio-rate modulator
frequencies, increasing depth produces progressively richer FM sidebands.

### Phase-Distortion Modulation

`c mode,depth` sets the base phase-distortion amount. `C voice,depth` then adds
a bipolar modulation term:

```text
effective distortion = c depth + modulator sample * C depth
```

`C` has no separate offset because the base depth from `c` serves that role.
A negative `C` depth reverses the sweep. The useful range depends on the
selected phase-distortion mode, and the effective value is not clamped.

### Ring Modulation

`XM voice[,amount]` multiplies the destination oscillator sample by the
modulator sample. This is bipolar multiplication, producing sum-and-difference
frequencies rather than ordinary volume tremolo. In the current implementation
the optional `amount` is stored and displayed but is not applied to the audio;
selecting the modulator voice enables full-strength ring modulation.

| Command | Parameters | Effect | Schedulable |
| --- | --- | --- | --- |
| `A voice,depth[,offset]` | Amplitude multiplier scale and bias | Multiplies amplitude by `sample * depth + offset`. Requires `AM`. | Yes |
| `F voice,depth[,offset]` | Relative scale or hertz deviation and center | Modulates frequency according to `FF`. Requires `FM`. | Yes |
| `FF mode` | `0` relative, `1` absolute hertz | Selects how the `F` control value becomes oscillator frequency. Requires `FM`. | Yes |
| `P voice,depth[,offset]` | Pan scale and center | Sets pan to `sample * depth + offset`. Requires `PANMOD`. | Yes |
| `c [mode[,depth]]` | Phase-distortion mode and base amount | Reshapes oscillator phase. No arguments disables it; omitted depth defaults to `0.5`. Requires `PD`. | Yes |
| `C voice,depth` | Phase-distortion modulation scale | Adds `sample * depth` to the amount set by `c`. Fewer than two arguments disables it. Requires `PD`. | Yes |
| `XM voice[,amount]` | Ring-modulator voice; currently unused amount | Multiplies the selected oscillator by another oscillator at full strength. Requires `XM`. | Yes |

Phase-distortion modes:

| Mode | Shape |
| --- | --- |
| `0` | Off |
| `1` | Saw to pulse |
| `2` | Folded sine |
| `3` | Triangle |
| `4` | Double sine |
| `5` | Saw to triangle |
| `6` | Resonant shape 1 |
| `7` | Resonant shape 2 |

## Timing and Events

`+` uses musical timing and follows the current tempo. A value of `1` is one
beat. `~` always uses seconds. Pattern steps are based on quarter-beat master
ticks and their `%` modulus, so their duration is not necessarily one beat.

### Timing Accuracy

Queued events and pattern ticks use the synth's absolute 44.1 kHz sample
counter. The audio callback renders adaptively: when a timing boundary falls
inside a device block, it renders up to that sample, executes the due commands,
and then renders the rest of the block. Blocks without internal boundaries are
rendered normally in one call.

Events scheduled at integer sample times affect that exact sample. Tempo
boundaries that fall between samples are rounded forward, giving less than one
sample of sequencing error (under 0.023 ms at 44.1 kHz) during normal
operation. This improves event placement inside the audio stream; it does not
remove output-device or hardware buffering latency.

| Command | Parameters | Effect |
| --- | --- | --- |
| `M bpm` | `1` through `960` BPM | Sets pattern and tempo-relative event speed. It does not retime events already placed at absolute sample times. |
| `+beats commands` | Beat delay followed by commands | Compiles the remaining commands and schedules them after a tempo-relative delay. |
| `~seconds commands` | Second delay followed by commands | Compiles the remaining commands and schedules them after a real-time delay. |
| `[commands] R count,seconds[,tag]` | Repeat count, interval, optional tag | Queues the command program `count` times, starting immediately and spacing starts by seconds. |
| `[commands] RR count,beats[,tag]` | Repeat count, beat interval, optional tag | Queues the program repeatedly using tempo-relative spacing. |
| `eR macro,count,seconds[,tag]` | Macro index, repeat count, interval, optional tag | Compiles an external macro once and queues `count` invocations, starting immediately and spacing starts by seconds. |
| `eRR macro,count,beats[,tag]` | Macro index, repeat count, beat interval, optional tag | Compiles an external macro once and queues repeated invocations using tempo-relative spacing. |
| `[commands] DO? value[,tag]` | Condition and optional tag | Queues the program once when `value` is greater than zero. |
| `R! tag` | Integer event tag | Removes queued events carrying that tag. Already executed events are unaffected. |
| `R!!` | None | Clears all queued events. It does not erase patterns or reset voices. |
| `ce id[,a,b,c]` | Integer event id and up to three numeric values | Emits a `SKRED_CONTROL_EVENT_USER` control-plane event. This command is schedulable, so defers, repeats, patterns, and macros can send host-visible markers. |
| `?q` | None | Displays queued compiled events waiting in the sequencer queue. |
| `?ce` | None | Displays outstanding control-plane events without consuming them. |
| `?ce!` | None | Clears outstanding control-plane events without resetting voices or patterns. |
| `?o` | None | Compatibility alias for queued compiled events. |
| `?o pattern[,step]` | Pattern and optional step | Displays the opcodes compiled for pattern steps. |
| `wait ms` | Nonnegative milliseconds | Blocks the command/control thread. It does not create a musical event and should not be used for audio-rate scheduling. |

Deferred and repeated programs can contain only schedulable commands.

`?q` and `?ce` show different views. `?q` reports pending scheduled opcode
events that have not executed yet. `?ce` peeks at the control-plane
notification ring and reports things the synth has already observed without
clearing them. Use `?ce!` when you explicitly want to discard outstanding
control-plane notifications. `ce id[,a,b,c]` writes to that control-plane stream
immediately or when its scheduled opcode executes; pollers see the event id
plus up to three numeric values.

### Control-Event Response Dispatcher

Skode can bind control-plane events to Skode commands. These are parser slash
commands handled by Skode itself, not ad-hoc mini-skred syntax:

| Command | Parameters | Effect |
| --- | --- | --- |
| `/cer state` | `0` or nonzero | Stops or starts the API control-event dispatcher thread. |
| `[command] /ceb type key` | Skode response command, numeric event type, event key | Runs `command` whenever a matching control event is received. |
| `/ce! type key` | Numeric event type and key | Removes matching bindings. |
| `/ce!` | None | Removes all responder bindings. |
| `/ce?` | None | Displays responder state and bindings. |

Event type numbers are the public `SKRED_CONTROL_EVENT_*` enum values:

| Number | Event |
| --- | --- |
| `1` | `SKRED_CONTROL_EVENT_VOICE_TRIGGER` |
| `2` | `SKRED_CONTROL_EVENT_VOICE_RELEASE` |
| `3` | `SKRED_CONTROL_EVENT_VOICE_FINISHED` |
| `4` | `SKRED_CONTROL_EVENT_USER` |
| `5` | `SKRED_CONTROL_EVENT_PATTERN_START` |
| `6` | `SKRED_CONTROL_EVENT_PATTERN_END` |

`key` is the event id for `SKRED_CONTROL_EVENT_USER`, the pattern number for
pattern events, and the voice number for voice events. Use `-1` as a wildcard
key. Event tags do not participate in responder matching; tags are source
metadata from queued/repeated programs and are still useful for `R! tag`
cancellation and host-side correlation.

User-event example:

```text
[v4 l1] /ceb 4 42
[v5 l1] /ceb 4 43
/cer 1
ce 42
ce 43
```

Here both bindings listen for type `4`, `SKRED_CONTROL_EVENT_USER`. The key is
the `ce` id. `ce 42` runs `v4 l1`; `ce 43` runs `v5 l1`.

Voice-event example:

```text
v0 vc1
v1 vc1
[v2 l1] /ceb 1 0
[v3 l1] /ceb 1 1
/cer 1
v0 l1
v1 l1
```

Type `1` is `SKRED_CONTROL_EVENT_VOICE_TRIGGER`. The key is the triggering
voice number. Because voice `0` and voice `1` both have `vc1`, `v0 l1` emits a
trigger event with key `0` and runs `v2 l1`; `v1 l1` emits key `1` and runs
`v3 l1`. Voices without `vc1` do not emit lifecycle events.

Pattern-event example:

```text
y0 yc1
[ce 100] x0
[-] x1
y1 yc1
[ce 200] x0
[-] x1
[v6 l1] /ceb 5 0
[v7 l1] /ceb 5 1
[v8 l1] /ceb 6 0
[v9 l1] /ceb 6 1
/cer 1
```

Type `5` is `SKRED_CONTROL_EVENT_PATTERN_START`; type `6` is
`SKRED_CONTROL_EVENT_PATTERN_END`. The key is the pattern number. Pattern `0`
start runs `v6 l1`, pattern `1` start runs `v7 l1`, pattern `0` end runs
`v8 l1`, and pattern `1` end runs `v9 l1`. The `ce 100` and `ce 200` commands
inside the patterns are separate user events whose responder key would be
`100` or `200` if you bind type `4`.

`/cer 1` starts SKRED's API-level dispatcher thread. That thread sleeps on the
public control-event wait object, drains events when woken, and runs matching
Skode commands from a dedicated Skode context. `/cer 0` stops it, and
`skred_stop()` stops it during engine teardown. Mini-Skred does not implement a
separate parser or responder loop; it submits these commands through
`skred_command()` like any other Skode text. Dispatcher service consumes events
from the same ring inspected by `?ce`, so use `?ce` before enabling the
dispatcher when you want to see pending events non-destructively.

### Foreign C Function Calls

Applications using `api.h` can bind C callbacks to `/ff0` through `/ff9`.
Unbound slots do nothing and do not report an error.

```text
(10,20,30) [marker] /ff0 1,2,3
```

The callback receives the slot number, numeric arguments `1,2,3`, parser string
`marker`, parser data `10,20,30`, and the current voice/pattern/step context.
The slot digit is not part of the numeric argument list; `/ff3 10,20` calls slot
`3` with two callback arguments, `10` and `20`. The string and data are borrowed
from the parser for the duration of the call.

## Patterns

Skode provides 128 patterns, numbered `0` through `127`. Each pattern holds
up to 128 steps, also numbered `0` through `127`.

| Command | Parameters | Effect |
| --- | --- | --- |
| `y pattern` | Pattern index | Selects the pattern edited by `x`, `xa`, `yt`, `ym`, `%`, and `z`. |
| `[name] yt` | String | Sets the selected pattern's display name. |
| `[commands] x step` | Step index | Compiles and stores a specific step. Replacing a step replaces both its source text and compiled program. |
| `[commands] x-` | No explicit step | Advances the edit cursor and stores the step there. |
| `[commands] xa` | String | Appends a compiled step after the pattern's current end. |
| `[] x step` | Empty string and step | Clears a step. An empty step inside the existing pattern length consumes time but performs no operation; empty trailing steps do not extend the pattern. |
| `[-] x step` | Stop marker and step | Stores the pattern stop marker. Playback stops when it reaches that step. |
| `<x step` | Step index | Copies a step's source text into the parser string buffer for inspection or editing. |
| `xg step`, `>x step` | Step index | Moves the selected pattern's playback pointer to a step. |
| `% modulus` | Positive integer; minimum `1` | Makes the selected pattern advance only on every Nth quarter-beat master tick. The default is `4`, or one step per beat. Larger values run more slowly. |
| `ym state` | `0` or nonzero | Mutes pattern execution while retaining its contents and playback state. |
| `yc state` | `0` or nonzero | Disables or enables pattern boundary control-plane events for the selected pattern. Disabled by default. Pattern show commands include `yc1` only when enabled. |
| `Y pattern` | Pattern index | Clears the pattern, stops it, and resets its persistent playback voice to voice `0`. |
| `z state` | `0` stop, `1` start, `2` pause, `3` resume | Changes the selected pattern's playback state. With no argument, displays the pattern. |
| `z?` | None | Displays the selected pattern and its steps. |
| `Z state` | `0` stop, `1` start, `2` pause, `3` resume | Applies a playback-state command to every pattern. With no argument, displays pattern summaries. |
| `Z?`, `z??` | None | Displays all patterns with their steps. |

Pattern source text is retained for display, but playback uses the compiled
snapshot. Editing an external macro later does not alter a pattern step that
was already compiled from it.

With `yc1`, a pattern emits `SKRED_CONTROL_EVENT_PATTERN_START` when playback
lands on step `0`, and `SKRED_CONTROL_EVENT_PATTERN_END` when it reaches a stop
marker or the last playable step before wrap.

## External Macros

There are 128 external string buffers, numbered `0` through `127`. Each stores
up to 255 text characters plus its terminator.

| Command | Parameters | Effect | Schedulable |
| --- | --- | --- | --- |
| `[commands] e>N` | Literal buffer index | Stores the current string as external macro `N`. | No |
| `<e N` | Buffer index | Copies external macro `N` into the parser string buffer without executing it. | No |
| `e? [N]` | Optional buffer index | Displays one macro, or every nonempty macro when no index is given. | No |
| `e!N` | Literal buffer index | Compiles and executes macro `N`. When used inside another compiled program, its opcodes are inlined as a snapshot. | Yes |
| `eR N,count,seconds[,tag]` | Literal buffer index, count, interval, optional tag | Compiles macro `N` once and repeats that snapshot at real-time intervals. | No |
| `eRR N,count,beats[,tag]` | Literal buffer index, count, beat interval, optional tag | Compiles macro `N` once and repeats that snapshot at tempo-relative intervals. | No |
| `[commands] e!` | Current parser string | Compiles and executes the current string immediately. Argumentless `e!` itself cannot be nested in a compiled program. | No |

Literal `e!N` calls may be used inside patterns, defers, repeats, and other
external macros. Recursive macro cycles, undefined macros, runtime-selected
`e!$N`, and expansions beyond 32 operations are rejected.

`eR` and `eRR` are control-thread scheduling commands. The macro is copied and
compiled once when the command is issued, so later edits do not affect queued
invocations. The 32-operation limit applies to one compiled invocation, not
the repeat count.

## Registers and Data

| Command | Parameters | Effect | Schedulable |
| --- | --- | --- | --- |
| `=N,value` | Register index and value | Writes shared register `N`. In a compiled program, the write happens when the opcode executes. | Yes |
| `=N` | Register index | Displays a register in the immediate command interface. | No |
| `=` | None | Displays registers in the immediate command interface. | No |
| `*=N,a,b` | Register and operands | Stores `a * b` in a shared register. | No |
| `/=N,a,b` | Register and operands | Stores `a / b` when `b` is nonzero. | No |
| `a=N,a,b` | Register and operands | Stores `a + b`. | No |
| `s=N,a,b` | Register and operands | Stores `a - b`. | No |
| `(values...)` | Numeric list | Replaces the parser's data array. | No |
| `D [capacity]`, `/D [capacity]` | Optional element capacity | Displays or enlarges the parser data allocation. | No |
| `?d` | None | Displays the current data array. | No |
| `d@ index` | Data index | Displays one data value. | No |
| `=d register,index` | Register and data index | Copies a data value into a shared register. | No |

## Samples, Waves, and Recording

These commands operate on files or sample memory and therefore run
immediately on the control thread.

| Command | Parameters | Effect |
| --- | --- | --- |
| `[filename] /ws wave[,channel]` | File name, writable destination wave, channel | Loads an audio file into the requested wavetable slot. Slots occupied by read-only built-in waves or active voices are rejected. |
| `/w file[,wave[,channel]]` | Numbered file and optional destination | Loads a numbered audio file. |
| `w>d wave` | Wave index | Copies wavetable samples into the parser data array. |
| `w>r wave` | Wave index | Copies wavetable samples into the temporary recording buffer. |
| `d>r` | None | Copies parser data into the temporary recording buffer. |
| `/r [slot[,one-shot[,offset]]]` | Wave destination and options | Loads the temporary recording buffer into a wavetable. |
| `/d [slot[,rate[,one-shot[,offset]]]]` | Wave destination and options | Loads parser data into a wavetable at the requested sample rate. |
| `w> [samples]` | Start offset adjustment | Moves the temporary recording's start point. |
| `w< [samples]` | End trim adjustment | Changes how many samples are removed from the recording end. |
| `w<> [threshold[,headroom]]` | Detection threshold and margin | Finds useful start and end trim points from signal level. |
| `w!` | None | Applies current recording offsets and trims. |
| `w@` | None | Resets recording offsets and trims. |
| `/wex wave` | Dynamic wave index `200` through `999` | Expands storage for a dynamic wavetable slot. |
| `<r seconds`, `^r seconds` | Duration | Records output into the temporary sample buffer. |
| `>r number` | Output number | Normalizes the temporary sample and writes `out-N.wav`. |
| `[filename] /rg [max-seconds]` | Output filename and optional limit | Starts multitrack WAV recording. Requires `RECORD`. |
| `/rs` | None | Stops multitrack recording. Requires `RECORD`. |
| `/r?` | None | Displays multitrack recorder status. Requires `RECORD`. |
| `r track` | `0` none, `1` through `4` stem | Routes the selected voice into a multitrack stem. Requires `RECORD` or `SCOPE`. |
| `[name] rt track` | String plus track number | Names record/scope stem tracks 1 through 4. |
| `rv track,dB` | Track number plus dB | Sets the final stem level for record/scope tracks 1 through 4. Defaults to `-20 dB`. |
| `?r` | None | Shows stem names and `#` followed by the voices assigned to each track. |
| `[name] /sg [channel-mask[,buffer-seconds]]` | Shared-memory name, channel bit mask, ring duration | Starts live publication of the ten-channel master/stem bus. Defaults to `skred-scope`, all channels, and one second. Requires `SCOPE`. |
| `/ss` | None | Stops publication and unlinks the shared-memory name. Existing mappings remain readable and report inactive. Requires `SCOPE`. |
| `/s?` | None | Displays scope name, format, mask, capacity, and absolute frame counter. Requires `SCOPE`. |

Scope channel-mask bits follow the file layout: bits `0` and `1` are master
left/right, bits `2` and `3` are stem 1, through bits `8` and `9` for stem 4.
For example, mask `3` publishes master L/R and mask `15` advertises master plus
stem 1. The shared ring remains ten-channel interleaved float data; the mask
tells consumers which channels were requested for display.
The shared-memory header also includes per-track names and dB levels for the
master slot and stems 1 through 4.

### Multichannel WAV Walkthrough

Build and launch Mini-Skred with recording support:

```sh
cd parts
make maxed
./build_maxed/mini-skred
```

Configure two voices, assign them to stereo stems, and start recording:

```text
v0 r1 w0 f440 a-6 t.01,.2,.7,.3
v1 r2 w1 f660 a-9 t.01,.2,.6,.3
[take.wav]/rg
v0 l1
v1 l1
/r?
/rs
```

`/rs` drains the writer queue and finalizes the WAV header before returning.
The output is a 44.1 kHz, 32-bit float, ten-channel WAV in this order:

| Channel | Signal |
| --- | --- |
| 0, 1 | Master left, right |
| 2, 3 | Stem 1 left, right |
| 4, 5 | Stem 2 left, right |
| 6, 7 | Stem 3 left, right |
| 8, 9 | Stem 4 left, right |

Every audible voice is present in the master. `r1` through `r4` additionally
route the selected voice into a stem; `r0` removes that extra route. To stop
automatically after five seconds, use `[take.wav]/rg5`.

### Shared-Memory Scope Walkthrough

Start the default one-second, all-channel publisher in Mini-Skred:

```text
/sg
/s?
```

In another terminal, run the bundled consumer:

```sh
cd parts
./build_maxed/scope_reader skred-scope 2048
```

The final argument is the number of newest frames copied for each display
update. The example reader prints peak and RMS measurements; a graphical
consumer can use `scope_ipc_reader_latest()` from `scope-ipc.h` and draw the
same frames.

To publish only master L/R with a 250 ms ring:

```text
[my-scope]/sg3,.25
```

Then connect with:

```sh
./build_maxed/scope_reader my-scope 2048
```

Use `/ss` to mark the mapping inactive and unlink its name. An already-mapped
reader can observe the inactive flag and close cleanly. Scope publication and
multichannel WAV recording can be active simultaneously.

## Inspection and Runtime Control

| Command | Parameters | Effect |
| --- | --- | --- |
| `?`, `v?` | None | Displays the selected voice. |
| `\` | None | Displays the selected voice with additional detail. |
| `??`, `v??` | None | Displays active voices. |
| `W [wave[,end-or-width[,height]]]` | Optional display parameters | Displays one wavetable, recording data, or all loaded waves. |
| `W@ wave,param[,register]` | Wave, property, optional destination | Reads wave sample count (`0`), sample rate (`1`), or duration (`2`). |
| `v@ param[,register]` | Property and optional destination | Reads selected voice wave (`0`), amplitude (`1`), or frequency (`2`). |
| `?s` | None | Displays the current parser string. |
| `/s [section]` | Optional section number | Displays runtime, audio, synth, Skode, string, or benchmark state. |
| `/th?` | None | Displays SKRED service/thread health: audio callback load, control-event dispatcher counters, UDP activity, recorder state, and scope publication state. |
| `/t [level]` | Optional trace level | Toggles or sets command and parser tracing. |
| `/v [level]` | Optional verbosity level | Toggles or sets verbose output. |
| `/f [value]` | Optional value | Displays or sets the current context flag. |
| `log state` | `0` or nonzero | Disables or enables command-context log capture. |
| `udp value` | Any numeric argument | Displays the current UDP context endpoint. Requires `UDP`. |
| `/m_` | None | Benchmarks the selected voice. Requires `BENCH`. |
| `I value` | Numeric value | Reserved event-logging stub. It currently has no effect. |
| `/q` | None | Requests exit from the interactive shell. |

`W` reports the selected renderer as `display braille` or `display ascii`.
It uses the compact Unicode Braille plotter on Linux and macOS. Windows
console hosts default to an ASCII plot because common `cmd.exe` and PowerShell
fonts render Braille cells poorly. Set `SKRED_WAVE_DISPLAY=braille` to force
the Unicode renderer, or `SKRED_WAVE_DISPLAY=ascii` to force the portable
ASCII renderer on any platform.

## File and Ksynth Utilities

| Command | Parameters | Effect |
| --- | --- | --- |
| `/l file[,verbose]` | Numbered Skode file | Loads and executes a `.sk` file. |
| `[filename] /cat` | String | Prints a text file. |
| `[directory] /cd` | String | Changes the process working directory. |
| `/ls [type]` | `0` `.sk`, `1` `.wav`, `2` `.mp3`, `3` `.ks` | Lists matching files in the working directory. |
| `[file] /ks [verbose]` | Ksynth filename | Loads a named Ksynth source file. Requires `KSYNTH`. |
| `/k file[,verbose]` | Numbered Ksynth file | Loads numbered Ksynth source. Requires `KSYNTH`. |
| `[code] ks`, `[code] k!` | Ksynth source string | Runs source in this Skode context's Ksynth evaluator. |
| `kw [timeout-ms]` | Optional timeout | Compatibility no-op; Ksynth now runs synchronously. |
| `kw> [timeout-ms]` | Optional timeout | Copies the latest Ksynth result into parser data. |
| `k?` | None | Displays the latest Ksynth result. |
| `k>d` | None | Copies the latest Ksynth result into parser data. |
| `d>k variable` | Variable `0` through `25` | Copies parser data into Ksynth variable `A` through `Z`. |
| `w>k wave,variable` | Wave index and variable `0` through `25` | Copies a wavetable directly into Ksynth variable `A` through `Z`. |

Each Skode command context owns its own Ksynth evaluator and persistent
`A` through `Z` variables. For example, `(1 2 3) d>k0 [A,A] ks kw>` binds
`A` before evaluating the concatenation; `kw` is accepted for compatibility
but no longer waits. Sample-rate and loop metadata are not copied with array
values; provide the desired rate when loading returned parser data with `/d`.
Vectors are limited to one million elements.

### Processing Wavetables with Ksynth

Variables are numbered in Skode and named in Ksynth:

```text
0 = A
1 = B
...
25 = Z
```

Bindings and evaluations run immediately in the current Skode command context.
`kw>` copies the latest result into the current Skode data array. Ksynth
variables `A` through `Z` belong to that context, so local input and separate
UDP clients keep independent variables.

**Transform parser data and load the result into a wavetable:**

```text
(0 .5 1 .5 0) d>k0
[i A] ks
kw>1000
/d300,44100
```

This binds the data array to `A`, reverses it with Ksynth's monadic `i`, waits
up to 1000 milliseconds, and loads the result into wavetable 300 at 44100 Hz.

**Transform a wavetable directly:**

```text
w>k10,0
[w A] ks
kw>1000
/d300,44100
```

This copies wave 10 into `A`, peak-normalizes it with Ksynth's monadic `w`,
and stores the result in wave 300. `w>k` avoids changing the current Skode
data array during import.

**Concatenate two built-in wavetables into a periodic waveform:**

```text
w>k10,0
w>k11,1
[A,B] ks
kw>1000
/d300,44100,0
v0 w300,1,0
```

Built-in waves 10 and 11 each contain 4096 samples. The comma concatenates
`A` and `B` into one 8192-sample cycle. `/d300,44100,0` stores it as a
periodic wavetable, and `w300,1,0` selects it with interpolation enabled and
one-shot playback disabled. The oscillator wraps from the end of wave 11 back
to the beginning of wave 10.

**Concatenate two wavetables into a one-shot waveform:**

```text
w>k10,0
w>k11,1
[A,B] ks
kw>1000
/d301,44100,1
v0 w301,1,1
```

This creates the same 8192-sample sequence but stores it in wave 301 as a
one-shot. `w301,1,1` selects it with interpolation enabled and one-shot
playback enabled, so a trigger reads wave 10 followed by wave 11 and then
stops at the end instead of wrapping. Use another `/d` rate when the source
material is not 44100 Hz.

`kw` is kept for older scripts and does nothing. `k>d` and `kw>` both copy the
latest completed result.

## Example Sounds

These are starting points rather than exact emulations. The synthesizer
examples require the named feature gates, and the sample examples require
`KSYNTH`. Run each example separately, or reset its voices with `S` before
trying the next one.

### Synthesizer-Inspired Patches

**Moog-style resonant bass.** A saw wave, fast filter envelope, and resonant
low-pass produce the rounded attack and dark sustain associated with classic
Moog bass patches. Requires `ADSR`, `FILT`, and `FADSR`.

```text
S0
v0 w2 a-8 t.005,.18,.7,.25 J1 K120 Q5 ft.002,.25,.08,.3 fd2600 n36 l1
~.75 v0 l0
```

**Casio CZ-1-style phase-distortion brass.** A resonant phase-distortion shape
gives a bright, slightly hollow digital brass tone. Requires `ADSR` and `PD`.

```text
S0
v0 w0 a-10 c6,.72 t.015,.35,.55,.45 n48 l1
~1 v0 l0
```

Try `c7,.55` for a thinner resonant shape, or `c1,.8` for a more obvious
saw-to-pulse character.

**Korg DW-8000-style brass pad.** Wave `17` is the built-in DWGS brass
wavetable. A slow filter envelope softens its digital harmonic spectrum.
Requires `ADSR`, `FILT`, and `FADSR`.

```text
S0
v0 w17 a-12 t.08,.5,.7,.8 J1 K500 Q2 ft.12,.7,.3,.8 fd3200 n48 l1
~1.5 v0 l0
```

Other built-in DWGS waves occupy slots `10` through `25`; wave `10` is
strings, `13` is electric piano, `15` is clavinet, and `24` is bell.

**Yamaha DX7-style electric piano.** Voice `1` is a decaying sine modulator
for the sine carrier on voice `0`. Its envelope makes the bright FM attack
settle into a softer body. Requires `ADSR` and `FM`.

```text
S0 S1
v1 w0 n76 a-18 t.001,.35,0,.15 l1
v0 w0 n52 a-8 t.002,1.2,.18,.6 FF0 F1,8 l1
~1.5 v0 l0 v1 l0
```

Increase `F1,8` toward `F1,14` for a harder tine attack. The modulator is also
present in the output, so its amplitude controls both FM strength and how much
of the upper sine tone is directly audible.

### Drum Samples

The repository includes Ksynth definitions for synthesized drum one-shots.
When Skode is started from the `parts` directory, these commands generate a
kick, snare, and closed hi-hat in dynamic wave slots `300` through `302`:

```text
[sk/drums-kick.ks] /ks kw k>d d>r /r300
[sk/drums-snare.ks] /ks kw k>d d>r /r301
[sk/drums-chh.ks] /ks kw k>d d>r /r302
```

Assign each sample to a voice and trigger it with `l1` or `T`:

```text
v0 w300 f440 a-6
v1 w301 f440 a-8
v2 w302 f440 a-12

v0 T
~.25 v2 T
~.5 v1 T
~.75 v2 T
```

The same loading pattern works with files such as `drums-clap.ks`,
`drums-cowbell.ks`, `drums-tomhi.ks`, and `drums-crash.ks`.

### Sound Effects

These bundled one-shots provide useful raw material for alarms, transitions,
and game-like effects:

```text
[sk/nap-fm-alarm.ks] /ks kw k>d d>r /r310
[sk/nap-noise-sweep.ks] /ks kw k>d d>r /r311
[sk/nap-perc-zap.ks] /ks kw k>d d>r /r312

v3 w310 f440 a-12
v4 w311 f440 a-10
v5 w312 f440 a-8
```

Trigger the alarm, a rising noise transition, or an electric zap:

```text
v3 T
~1 v4 T
~2.25 v5 T
```

Changing sample playback frequency changes both pitch and duration:

```text
v5 f220 T
~.5 v5 f880 T
```

## Short Examples

Play a centered A4 on voice 0:

```text
v0 w0 n69 a-10 p0 l1
```

Create a filtered, enveloped note:

```text
v0 w1 t.01,.2,.6,.4 k1 J0 K1200 Q.3 n48 l1
```

Store and reuse a melodic macro:

```text
[v0 n60l1 ~.2 n64l1 ~.2 n67l1] e>0
e!0
```

Put that macro in a pattern:

```text
M120
y0
[e!0] xa
[~.25 n72l1] xa
z1
```

Queue four notes a quarter-second apart and cancel them by tag if needed:

```text
[v1 n60 l1] R4,.25,7
R!7
```

Use registers to change a compiled pattern without rebuilding it:

```text
=0,60
y1
[v0 n$0 l1] xa
z1
=0,67
```

## Feature-Gated Commands

Some commands exist only when their build feature is enabled:

| Feature | Commands |
| --- | --- |
| `ADSR` | `k`, `t` |
| `AM` | `A` |
| `CRUSH` | `q` |
| `FILT` | `J`, `K`, `Q` |
| `FILT` and `FADSR` | `ft`, `fd` |
| `FM` | `F`, `FF` |
| `GLISS` | `g` |
| `KSYNTH` | `/ks`, `/k`, `ks`, `k!`, `kw`, `kw>`, `k?`, `k>d`, `d>k`, `w>k` |
| `PANMOD` | `P` |
| `PD` | `c`, `C` |
| `RECORD` | `r`, `/rg`, `/rs`, `/r?` |
| `SCOPE` | `r`, `/sg`, `/ss`, `/s?` |
| `SAH` | `h` |
| `SEQ` | Timing, event, and pattern commands |
| `SMOOTHER` | `s` |
| `XM` | `XM` |
