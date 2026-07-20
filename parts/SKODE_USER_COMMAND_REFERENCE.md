# Skode User Command Reference

This is a practical reference for writing Skode. It describes command
parameters and what each command does to voices, sounds, patterns, or queued
events. For implementation details, see
[SKODE_COMMANDS.md](SKODE_COMMANDS.md).

Voice groups, polyphonic/monophonic pools, allocation policies, and dependency
graphs have a dedicated guide: [POLYPHONY.md](POLYPHONY.md).

## Reading Skode

Skode is case-sensitive. Commands may be separated by spaces or written
together:

```text
v0 w1 n60 a0 l1
v0w1n60a0l1
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
| `a dB` | Amplitude in decibels | Sets voice loudness. `a0` is the normal synth-patch level; negative values attenuate. Long normalized one-shots and Ksynth drums conventionally use `a10`, with final listening level controlled by `V`. | Yes |
| `V dB` | Master amplitude in decibels | Changes the final output level for the entire synth, not just the selected voice. | No |
| `p pan` | Stereo position from `-1` to `1` | Moves the voice across the stereo field. `-1` is left, `0` is center, and `1` is right. | Yes |
| `ds amount` | Send `0..1` or `0..15` | Sends the selected voice's mono signal to the delay owned by its record/scope track. The voice must be routed with `r1`..`r4`, centered with `p0`, and have no pan modulation. | No |
| `DL track,coarse,fine,feedback,modfreq,moddepth,level` | Track `1..4`, DW-style delay parameters | Sets the mono-send/stereo-return delay attached to one record/scope track. Parameter ranges are `0..7`, `0..15`, `0..15`, `0..31`, `0..31`, `0..15`. | No |
| `DL? [track]` | Optional track `1..4` | Displays one track delay, or all four track delays, as copy/pasteable `DL...` commands. | No |
| `GS [full]` | Optional boolean | Displays copy/pasteable global synth state and the build version. With a value greater than `0`, also prints a larger text snapshot for saving/reloading. | No |
| `m state` | `0` or nonzero | Removes or restores the voice in the master output without deleting its settings. A muted voice assigned with `r1`..`r4` remains available in that dry record/scope stem. | Yes |
| `l velocity` | Envelope velocity | Triggers or updates the voice envelope with the supplied velocity. It also affects voices linked with `H`. | Yes |
| `T` | None | Retriggers the selected voice at velocity `1`, including velocity-linked voices. | Yes |
| `L seconds` | Trigger delay; `0` disables | Delays velocity/envelope triggering for the selected voice. This is a per-voice trigger delay, distinct from queue defers. | Yes |
| `H voice[,voice...]` | Up to four voice indices | Sends later `l` and `T` envelope triggers to the listed voices. A new `H` replaces the old link list. | Yes |
| `s amount` | Smoothing amount; `0` disables | Smooths amplitude changes to reduce abrupt level transitions. Requires `SMOOTHER`. | Yes |
| `vc state` | `0` or nonzero | Disables or enables synth-generated control-plane events for the selected voice. Disabled by default. Voice show commands include `vc1` only when enabled. | No |

## Voice Groups and Polyphony

| Command | Parameters | Effect | Schedulable |
| --- | --- | --- | --- |
| `/pg group,source,width[,root]` | Integer group/layout values | Defines a contiguous multi-voice prototype and its root offset. | No |
| `/pg! group` | Group index | Refreshes free instances in every pool using the group. | No |
| `/pp pool,group,base,count[,policy]` | Integer pool/layout values | Clones a group into a physical voice pool. Policy defaults to `0`. | No |
| `/pp! pool` | Pool index | Refreshes the pool's free instances. | No |
| `/pm pool,mode[,priority[,articulation]]` | Numeric mode values | Selects polyphonic or monophonic behavior. | No |
| `?pg [group]` | Optional group | Shows copy/pasteable group definitions. | No |
| `?pp [pool]` | Optional pool | Shows configuration and live allocation state. | No |
| `pn pool,key,note,velocity[,cents]` | Numeric note identity and performance values | Allocates or updates a group instance and performs note-on. | Yes |
| `pr pool,key[,release-velocity]` | Numeric note identity | Releases the allocation; unknown/stolen keys are harmless. | Yes |
| `pb pool,key,semitones[,cents]` | Numeric note identity and bend | Bends one allocation; key `-1` bends the whole pool. | Yes |
| `/vg voice[,format[,depth]]` | Format `0` ASCII or `1` graph text | Shows the reachable voice dependency graph. | No |

Steal policy values are `0` release-oldest, `1` oldest, `2` round-robin, `3`
quietest, and `4` no-steal. Pool modes are `0` polyphonic and `1` monophonic;
`2` is reserved for arpeggiation. See the dedicated guide for mono priority,
articulation, refresh, cloning, graph protocol, and full examples.

## Wave Playback

| Command | Parameters | Effect | Schedulable |
| --- | --- | --- | --- |
| `w wave[,interpolate[,mode]]` | Wave index and optional flags | Selects a wavetable. Interpolation smooths movement between samples. Mode is `0` for a repeating cycle or `1` for one-shot playback; omit it to inherit the mode stored by the loader. | Yes |
| `/` | None | Restores the selected voice's default waveform settings. | Yes |
| `b [direction]` | `0` forward, `1` backward, `2` ping-pong; no argument toggles between `0` and `1` | Changes the direction mode used to read the selected wave. | Yes |
| `B [loop]` | `0` off, `1` on; no argument toggles | Controls whether wave playback wraps at its end. This is most noticeable with sampled or one-shot waves. | Yes |
| `BC count` | Nonnegative integer; `0` means unlimited | Enables looping and limits a one-shot voice to `count` wraps after its first pass through the loop. The configured count is captured on `l` or `T`; changing it does not alter a note already playing. | Yes |
| `WL wave,start,end` | Wave index and sample boundaries | Sets the wave's loop start and end boundary. `start` is inclusive, `end` is exclusive, and `end` may equal the wave sample count. | No |
| `VS start,end` | Sample boundaries on the selected voice | Overrides the selected voice's playable sample range after `w` has assigned a wave. `VS` with no arguments resets the voice to the full current wave range. | Yes |
| `VL start,end` | Sample boundaries on the selected voice | Overrides the selected voice's loop points after `w` has assigned a wave. `VL` with no arguments resets the voice to the current wave's `WL` defaults. | Yes |
| `q bits` | Integer bit-depth control | Quantizes the waveform and adds bit-crusher distortion. Requires `CRUSH`. | Yes |
| `h phases` | Integer hold length | Holds oscillator values for multiple phases, producing stepped or sample-and-hold distortion. Requires `SAH`. | Yes |
| `[name] vt` | String | Gives the selected voice a display label. It does not alter sound. | No |
| `[name] wt wave` | String and wave index | Gives a wavetable a display label. It does not alter sound. | No |

A cycle wave should contain one complete period, with its end prepared to join
smoothly back to its beginning. Full-range, forward cycle playback with no
active loop automatically uses the pared-down oscillator path; table length
does not need to be a special value. Direction changes, subranges, explicit
loops, and one-shots continue to use the general playback path.

Delay sends tap the selected voice after its oscillator, filter, envelope, and
amplitude, but before pan. The record/scope route owns the delay identity:
`r1` sends to track delay `1`, `r2` sends to track delay `2`, and so on through
`r4`. `ds` sets only the send amount for the selected voice. A send is active
only while the voice is routed to `r1`..`r4`, centered with `p0`, and has no pan
modulation. Track delay returns are added both to the main stereo mix and to
the matching record/scope stem. Track delays with no incoming send are skipped
by the audio callback until a voice feeds them, so unused delays have little to
no CPU cost. Modulation intensity spreads the left and right delay taps, and
modulation frequency animates that stereo spread.

```skode
# Track 1: short, bright slapback. Voice 0 sends at full amount.
DL1,0,8,3,0,0,12
v0 r1 p0 ds15 l1

# Track 2: longer modulated stereo delay. Voice 1 sends at half amount.
DL2,4,12,8,10,20,11
v1 r2 p0 ds.5 l1

# Track 3: darker repeat bed with no modulation.
DL3,5,4,10,0,0,8
v2 r3 p0 ds6 l1

# This voice is panned, so its track delay send is intentionally ignored.
v3 r1 p.5 ds15 l1

# This voice has no record/scope track, so ds has no track delay to feed.
v4 r0 p0 ds15 l1

# Show copy/pasteable version, master volume, tempo, and track delay commands.
GS

# Show a larger text snapshot for saving to a `.sk` file.
GS1
```

`GS1` includes pasteable global settings, macros, labels and `WL` loop metadata
for user-loaded wavetables, record/scope track routing when available, active
voice definitions, and sequencer patterns. Wavetable sample data is not
embedded; the snapshot includes a comment to make that limitation visible in
saved files.

### Direction, Looping, and Triggering

`b`, `B`, `BC`, and `l` describe different parts of one playback lifecycle:

- `b` selects reading direction. `b0` moves toward the wave or loop end;
  `b1` moves toward its start; `b2` is ping-pong mode. It does not enable
  looping or trigger playback.
- `B` selects whether playback wraps at the active loop boundary. `B1` enables
  wrapping and `B0` disables it. With no argument, `B` toggles the setting.
  This applies immediately to the current playback as well as later triggers.
- `BC` enables looping and optionally bounds it for one-shot waves. `BC0`
  means unlimited wrapping. A positive value counts additional boundary
  traversals after the first traversal of the loop region, so `BC1` plays that
  region twice: the initial traversal, then one repeat. In `b2` ping-pong mode,
  each trip from one boundary to the other counts as one traversal.
- A positive `l` value, or `T`, starts or retriggers a one-shot, initializes
  runtime looping from `B`, snapshots the current `BC` bound, resets the
  remaining wrap count, and triggers its envelopes. Changing `BC` during
  playback affects the next trigger; changing `B` affects the current one.
  Forward one-shot playback starts at physical sample `0`, reaches the loop end
  boundary, then wraps back to the loop start. Backward playback starts at the
  physical last sample, reaches the loop start boundary, then wraps back to the
  loop end. Ping-pong playback starts forward, reverses at `loop_end`, reverses
  again at `loop_start`, and repeats.
- `l0` releases active envelopes immediately. For a looping one-shot it also
  requests a clean loop exit: playback leaves the loop when it next reaches
  the boundary selected by `b`, then continues through any remaining wave tail.
  Forward playback exits at the loop end boundary and continues toward the
  physical wave end; backward playback exits at the loop start boundary and
  continues toward the physical wave beginning. Ping-pong exits at whichever
  loop boundary the current leg reaches next, then continues toward the
  physical tail in that same direction.

Sample ranges and loop points are layered. `VS start,end` narrows the selected
voice's playable wave range after `w`; plain `VS` restores the full current
wave range. `WL wave,start,end` stores loop defaults on the wavetable, `w`
copies those defaults into the selected voice, and `VL start,end` overrides the
selected voice after the wave assignment. A later `WL` update follows voices
that still use wave defaults, but leaves voices with `VL` overrides alone.
Plain `VL` clears the override and copies the current wave defaults back into
the voice when they fit inside the active `VS` range; otherwise the active `VS`
range is used as the loop region. `VS` and `VL` only change regions; use `B1`
or `BC count` to enable looping before `l1`.

When a positive `BC` count is exhausted without `l0`, the oscillator leaves the
loop at that boundary and emits one `VOICE_RELEASE` control-plane event while
continuing through the physical tail. Explicit `l0` emits its release event
immediately and does not emit a duplicate release when the loop boundary is
reached. Ordinary cyclic wavetables can still use `b` and `B`, but bounded loop
completion and one-shot retriggering apply only to one-shots. For forward
playback, loop exhaustion exits at `loop_end` and continues toward the physical
sample end; for backward playback, exhaustion exits at `loop_start` and
continues toward sample `0`; for ping-pong playback, exhaustion exits at the
next reached boundary in the current ping-pong leg.

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
| `FF2` | Adds the control value directly to the wavetable lookup phase in radians. The carrier's persistent phase continues at its base frequency, producing stable DX-style phase modulation. |

Approximately, for ordinary oscillator waves:

```text
FF0 frequency = carrier Hz + modulator Hz * control
FF1 frequency = control
FF2 lookup phase = carrier phase + control radians
```

For example:

```text
v1 f5                    # slow modulator
v0 FF1 F1,20,220         # carrier sweeps approximately 200..240 Hz
```

In `FF1`, use a positive offset large enough that `offset - abs(depth)` does
not drive the instantaneous frequency below zero. At audio-rate modulator
frequencies, increasing depth produces progressively richer FM sidebands.

In `FF2`, `depth` is the phase-modulation index in radians. A sine modulator
with `F1,1` moves the lookup phase by up to approximately one radian;
increasing the index adds progressively stronger sidebands without changing
the carrier's base phase increment. `offset` is a constant phase offset and is
normally left at zero.

`FB amount` adds operator self-feedback in `FF2`, from `0` (off) through `7`
(strong). Feedback uses the average of the operator's previous two
post-envelope samples as an additional phase offset:

```text
feedback phase = average(previous two outputs) * FB
```

Feedback history is cleared by `FB0`, by leaving `FF2`, and at each positive
trigger. `FB` is stored in other modes but affects sound only in `FF2`.
Feedback belongs on the operator whose spectrum should brighten; that operator
can then modulate another voice through `F`.

### Phase-Distortion Modulation

`c mode,amount` sets a signed phase-distortion amount in `-1..1`. Zero is the
exact undistorted phase for every mode. `C voice,depth` adds bipolar oscillator
modulation, while `ct attack,decay,sustain,release` and `cd depth` add a
triggered envelope:

```text
effective amount = clamp(c amount + modulator sample * C depth
                         + ct envelope * cd depth, -1, 1)
```

`C` has no separate offset because the base amount from `c` serves that role.
Negative `c`, `C`, or `cd` values reverse their contribution. `ct` follows
voice note-on/note-off just like the amplitude and filter envelopes, and the
neutral `ct 0 0 1 0` disables it.

### Ring Modulation

`XM voice[,amount]` multiplies the destination oscillator sample by the
modulator sample. This is bipolar multiplication, producing sum-and-difference
frequencies rather than ordinary volume tremolo. In the current implementation
the optional `amount` is stored and displayed but is not applied to the audio;
selecting the modulator voice enables full-strength ring modulation.

| Command | Parameters | Effect | Schedulable |
| --- | --- | --- | --- |
| `A voice,depth[,offset]` | Amplitude multiplier scale and bias | Multiplies amplitude by `sample * depth + offset`. Requires `AM`. | Yes |
| `F voice,depth[,offset]` | Relative scale, hertz control, or phase index | Modulates according to `FF`. In `FF2`, depth and offset are radians. Requires `FM`. | Yes |
| `FF mode` | `0` relative, `1` absolute hertz, `2` phase | Selects frequency or lookup-phase modulation. Requires `FM`. | Yes |
| `FB amount` | `0..7` | Adds two-sample operator feedback in `FF2`; zero clears its history. Requires `FM`. | Yes |
| `P voice,depth[,offset]` | Pan scale and center | Sets pan to `sample * depth + offset`. Requires `PANMOD`. | Yes |
| `c [mode[,amount]]` | Phase-distortion mode and signed base amount | Reshapes oscillator phase. No arguments disables it; an omitted amount defaults to the neutral value `0`. Requires `PD`. | Yes |
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

Queued events and pattern ticks use the synth's absolute sample counter at the
active engine/device rate (44.1 kHz by default). The audio callback renders
adaptively: when a timing boundary falls inside a device block, it renders up
to that sample, executes the due commands, and then renders the rest of the
block. Blocks without internal boundaries are rendered normally in one call.

Events scheduled at integer sample times affect that exact sample. Tempo
boundaries that fall between samples are rounded forward, giving less than one
sample of sequencing error (under 0.023 ms at the default 44.1 kHz) during normal
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
| `/cex external,type,key` | External string slot, event type, event key | Binds an external string slot as a response command. Useful for multi-step chains without nested bracket strings. |
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

Voice-event responses run with the event voice selected. That makes release
events useful for chaining regions of the same sampled wave:

```text
v5 w300 VS0,12000 VL2000,9000 BC1 l1
[VS12000,22000 VL14000,21000 BC1 l1] /ceb 2 5
/cer 1
```

Type `2` is `SKRED_CONTROL_EVENT_VOICE_RELEASE`. When voice `5` emits that
release event, the response command updates `VS` and `VL` on voice `5` and
retriggers it. Add an explicit `vN` to the response command only when you want
the event from one voice to control a different voice.

Longer chains use the same mechanism: a response can install the next response
with `/ceb` before it retriggers the voice. The API integration guide shows the
self-rebinding form for multi-step region chains.

There is no fixed chain length for one voice: each step replaces the current
`/ceb 2 voice` binding, so a same-voice chain normally uses one responder slot.
The practical limits are the 64 total responder bindings and the 512-byte
stored command limit for each step.

For example, this REPL sequence loads a large audio file into wave `300`, plays
an intro region once without looping, plays a middle region three times, then
plays an outro region once. External string slots avoid nested bracket strings,
so the response dispatcher can read the next step even though it runs in its
own Skode context:

```text
[large-take.wav] /ws 300,0
v5 w300,1,1 vc1

[ /ce! 3 5 /cex1,2,5 VS10000,20000 VL10000,20000 BC2 l1 ] e>0
[ /ce! 2 5 B0 VS20000,30000 l1 ] e>1

/cex0,3,5
/cer 1
B0 VS0,10000 l1
```

The intro is non-looping, so it hands off with type `3`,
`SKRED_CONTROL_EVENT_VOICE_FINISHED`. External string `0` removes that finished
binding, installs external string `1` as the next type `2` release response,
sets both `VS` and `VL` to `[10000..20000)`, and uses `BC2` to play that region
three times: the first traversal plus two repeats. When the bounded loop count
is exhausted, type `2`, `SKRED_CONTROL_EVENT_VOICE_RELEASE`, fires. External
string `1` removes the release binding, turns looping off with `B0`, switches
to the outro range `[20000..30000)`, and retriggers it once.

To reset the chain from the REPL, run `/cex0,3,5` again and trigger the intro
with `B0 VS0,10000 l1`. To stop the chain immediately, run:

```text
/ce! 3 5
/ce! 2 5
```

`/ce!` with no arguments clears all responder bindings. `?ce!` clears queued
control events, but does not remove response bindings.

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

## Named Macros

Named macros are global four-character commands defined directly in Skode:

```text
[name]: body;
```

Names longer than four characters are truncated. Parameters are written as
`$$0` through `$$7`; `@N` is not a parameter alias—it reads a return value.
For example:

```text
[tone]: v $$0 n $$1 a $$2;
tone 2,60,-9
```

Every definition is checked immediately using the real Skode compiler:

- `realtime`: the body compiles entirely to bounded scheduled opcodes. It is
  cached as a dictionary program and can be used interactively or anywhere
  schedulable Skode is accepted.
- `immediate`: the macro is valid but requires parser/control-thread behavior.
  It retains text-expansion semantics and cannot be placed on the scheduled
  real-time path.
- `invalid` or `too-large`: compilation failed or exceeded the bounded program
  size.

Inspect and manage definitions with:

| Command | Effect |
| --- | --- |
| `?m` | Show every named macro, parameter count, and capability status. |
| `[name] /m` | Remove one named macro and any cached dictionary program. |
| `/m!` | Clear all named macros and cached programs. |

Redefinition replaces the stored text, classification, and cached program.
Named macros loaded from Skode files are global and remain available for later
commands.

### Returning values from named macros

Use `*R` as the final command of an immediate macro to return up to ten values:

```text
[bounds]: *R .1 .9;
bounds
?R
# returns @0=0.1 @1=0.9
@0 a
```

`@0` through `@9` supply a returned value as a numeric argument. `?R` displays
the current tuple without consuming it. Return values are parser-local and
immediate-only; a macro containing `*R` or `@N` is therefore classified
`immediate`.

## External Numbered Macros

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
| `*R [values...]` | Up to ten values | Returns its arguments as `@0`, `@1`, and so on; see “Named Macros.” | No |
| `@N` | Return slot `0` through `9` | Supplies return value `N` as a numeric argument to the next command. | No |
| `?R` | None | Displays all current return values without consuming or changing them. | No |
| `(values...)` | Numeric list | Replaces the parser's data array. | No |
| `D [capacity]`, `/D [capacity]` | Optional element capacity | Displays or enlarges the parser data allocation. | No |
| `?d` | None | Displays the current data array. | No |
| `d* index` | Data index | Displays one data value. | No |
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
| `[filename] w>w wave` | File name and wave index | Writes the wavetable directly as mono floating-point WAV data, unchanged, using its stored sample rate. |
| `d>r` | None | Copies parser data into the temporary recording buffer. |
| `/r [slot[,mode[,channel]]]` | Wave destination and options | Loads the temporary recording into a mono wavetable. Mode is `0` cycle or `1` one-shot (default). Stereo recordings default to a mono downmix; channel `0` or `1` selects left or right. |
| `/d [slot[,rate[,mode[,offset]]]]` | Wave destination and options | Loads parser data into a wavetable at the requested sample rate. Mode is `0` cycle (default) or `1` one-shot. |
| `w> [frames]` | Start offset adjustment | Moves the temporary recording's start point. |
| `w< [frames]` | End trim adjustment | Changes how many frames are removed from the recording end. |
| `w<> [threshold[,end-threshold[,margin-frames]]]` | Detection thresholds and optional frame margin | Finds useful start and end trim points from signal level. For stereo, either channel can make a frame audible. Defaults to a small silence threshold of `0.001`. |
| `w!` | None | Applies current recording offsets and trims. |
| `w*` | None | Resets recording offsets and trims. |
| `/wex wave` | Dynamic wave index `200` through `999` | Expands storage for a dynamic wavetable slot. |
| `<r seconds[,source[,voice]]`, `^r ...` | Duration, source mode, optional source voice | Records source `0` dry mono, `1` one selected voice, or `2` the audible stereo master. Source defaults to `0`; source `1` requires the voice argument. |
| `[filename] >r` | File name | Normalizes the completed temporary recording and writes it as a mono or stereo WAV at the main sample rate. |
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

For example, `[periodic.wav] w>w300` exports wave 300 without changing its
sample values, while `[take.wav] >r` exports and normalizes the temporary
recording. The supplied filename is used literally, so include the `.wav`
extension when desired.

`<r 5` and `<r 5,0` capture five seconds of the current dry mono voice sum,
before panning, delay returns, and master volume. `<r 5,1,12` captures voice
12 after its voice processing but before pan and master volume. `<r 5,2`
captures the audible stereo master after panning, delay returns, and master
volume. The older `<r seconds,voice` form is intentionally replaced by the
unambiguous source-mode syntax.

When `/r` installs a one-shot recording, it automatically records the
duration-derived pitch metadata that makes MIDI note 69 play it at its natural
rate. The former, misleading `offset` argument is no longer exposed.

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
v0 r1 w0 f440 a0 t.01,.2,.7,.3
v1 r2 w1 f660 a0 t.01,.2,.6,.3
[take.wav]/rg
v0 l1
v1 l1
/r?
/rs
```

`/rs` drains the writer queue and finalizes the WAV header before returning.
The output uses the active engine/device sample rate (44.1 kHz by default),
32-bit float samples, and ten channels in this order:

| Channel | Signal |
| --- | --- |
| 0, 1 | Master left, right |
| 2, 3 | Stem 1 left, right |
| 4, 5 | Stem 2 left, right |
| 6, 7 | Stem 3 left, right |
| 8, 9 | Stem 4 left, right |

Every unmuted voice is present in the master. `r1` through `r4` additionally
route the selected voice into a stem; `r0` removes that extra route. `m1`
removes the voice from the master but preserves its dry stem route, allowing
isolated stem recording or monitoring. To stop automatically after five
seconds, use `[take.wav]/rg5`.

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

### Audio Device Selection

Audio device operations are immediate Skode commands and can be used
interactively, in `.sk` files, or from immediate macros:

| Command | Parameters | Effect |
| --- | --- | --- |
| `/als` | None | Refresh and list output and input devices. |
| `/a?` | None | Show audio state and the active/requested devices. |
| `/ao selection` | Device list index or `-1` | Select an output; `-1` means the default output. |
| `/ai selection` | Device list index, `-1`, or `-2` | Select an input; `-1` means the default input and `-2` disables capture. |

The older API-router spellings `/aout default` and `/ain off` remain host
compatibility aliases. They are not Skode syntax: `/aout` exceeds the
four-character atom limit and the textual arguments are not numeric.

### MIDI I/O

Build with `MIDI=1`, then use `/mL` to request MIDI access and list ports.
`/mi N` and `/mo N` open an input or output, `/mic` and `/moc` close them, and
`/m?` shows status. `[name] /miV` and `[name] /moV` create virtual ports on
CoreMIDI and ALSA; WinMM and Web MIDI do not provide virtual ports.

Incoming note-on, note-off, and pitch-bend messages can be routed on the
control plane. `/mv channel,voice[,bend]` routes to one voice and `/mp
channel,pool[,bend]` routes to a poly pool. `channel` is `0..15`, or `.`/`-` for
all channels; install multiple routes to listen to several selected channels.
`bend` is the symmetric full-scale range in semitones and defaults to `2`.
For example, `/mv 0,3` maps channel 1 (zero-based channel `0`) to voice 3, and
`/mp .,0,12` maps all channels to pool 0 with a ±12-semitone bend range.

`/mR` lists routes, `/mvd channel,voice` and `/mpd channel,pool` remove one,
and `/mC` clears all routes. Adding a command route starts the existing control
dispatcher. The C API can instead configure routes and have a host-owned loop
call `skred_control_dispatch_pump()`.

Poly groups are templates; MIDI targets the associated pool because the pool
owns live note allocation, releases, stealing, mono priority, and per-note
bend state.

Other MIDI input can invoke arbitrary Skode with `[command] /mb
type,channel,data1`. The bracketed command uses the normal parser string buffer,
so the binding syntax works unchanged in interactive input, `.sk` files, and
immediate macros. `type` is numeric:

| Type | MIDI message | First-data selector |
| ---: | --- | --- |
| `8` | Note Off | Note number |
| `9` | Note On | Note number |
| `10` | Polyphonic Key Pressure | Note number |
| `11` | Control Change | Controller number |
| `12` | Program Change | Program number |
| `13` | Channel Pressure | Pressure value |
| `14` | Pitch Bend | LSB |
| `17` | MTC Quarter Frame | Message byte |
| `18` | Song Position | 14-bit song position |
| `19` | Song Select | Song number |
| `20` | Tune Request | `0` |
| `24` | Timing Clock | `0` |
| `26` | Start | `0` |
| `27` | Continue | `0` |
| `28` | Stop | `0` |
| `31` | System Reset | `0` |

Use `.` or `-` for any channel or any first data value. System/realtime
messages have no channel, so their channel selector is normally `.`.

Command templates substitute `{ch}`, `{d1}`, `{d2}`, `{unit}` (`d2 / 127`),
and `{bend}` (normalized `-1..1`). The braces are not Skode operators. They
are MIDI-binding template markers stored inside an opaque bracket string; the
control dispatcher replaces them with numeric text before the resulting Skode
command is parsed. Any other braces are copied literally. Examples:

```text
[Z1] /mb 26,.,.                  # MIDI Start: play all patterns
[Z0] /mb 28,.,.                  # use Stop as pause
[Z1] /mb 27,.,.                  # Continue resumes
[v3K{d2}] /mb 11,.,74            # CC 74 controls voice 3 filter cutoff
```

MIDI has Start, Continue, and Stop but no separate Pause message; mapping Stop
to pause and Continue to resume is the usual transport interpretation. `/mb?`
lists bindings, `/mbd type,channel,data1` removes one, and `/mbC` clears them.
The equivalent C API is `skred_midi_binding_*()`.

All commands in this MIDI section are immediate Skode commands rather than
API-only aliases. They can therefore be loaded from a file or invoked by an
immediate macro. For example:

```text
[map1]:[v3K{d2}] /mb 11 . 74;
map1
```

Bindings are process-global and remain installed after the loader or macro
context returns. On Web MIDI, `/mL` still has to run from a browser user gesture
because opening a file cannot grant browser MIDI permission.

#### MIDI drum maps

The first-data selector for message type `9` is the MIDI note number, so `/mb`
can act as a drum map. Configure each voice's sample, envelope, tuning, pan, and
routing first, then bind incoming notes to its trigger. `{unit}` converts MIDI
velocity from `0..127` to the `0..1` range expected by `l`:

```text
[v0l{unit}] /mb 9 . 36          # kick
[v1l{unit}] /mb 9 . 38          # snare
[v2l{unit}] /mb 9 . 42          # closed hi-hat
```

A binding stores one command template for each `(type, channel, data1)` key,
but that template may control several voices. This makes layered samples
straightforward:

```text
[v3l{unit} v4l{unit} v5l{unit}] /mb 9 . 40
```

The three voices can independently provide the drum body, transient, and room
layer. A named macro keeps larger kits readable while still allowing the
MIDI dispatcher to supply velocity:

```text
[kik]:v0l$$0 v1l$$0;
[snr]:v2l$$0 v3l$$0 v4l$$0;
[hat]:v5l$$0;

[kik {unit}] /mb 9 . 36
[snr {unit}] /mb 9 . 38
[hat {unit}] /mb 9 . 42
```

Channel selectors can isolate a conventional drum channel. Channels are
zero-based, so selector `9` is MIDI channel 10:

```text
[v6l{unit}] /mb 9 9 46          # open hi-hat on channel 10
[v6l0] /mb 8 9 46               # matching note-off releases it
[v6l0 v5l{unit}] /mb 9 9 42     # closed hat chokes open hat, then triggers
```

This control-plane approach is suitable for modest kits and for developing the
desired mapping behavior. Each hit still expands text and parses Skode on the
control dispatcher. A future performance-oriented drum-map facility could
compile each note's real-time-safe program once, retain velocity as an input,
and add explicit choke groups, velocity layers, round-robin samples, and event
timestamp handling without parsing text for every hit.

#### MIDI mono and poly synths

For the simplest monophonic instrument, configure one voice and route a MIDI
channel directly to it. Channels are zero-based, so this listens to MIDI
channel 1 and uses a ±2-semitone pitch-bend range:

```text
v0 w0 a0 t.01,.15,.7,.35
/mv 0 0 2
```

Note-on sets the voice pitch and triggers its envelope, note-off releases the
currently active note, and pitch bend retunes it without retriggering. This
direct route tracks only the latest active key. It does not retain a held-note
stack, so use a monophonic pool when overlapping keys must return to an earlier
held note.

A pool provides proper monophonic priority and articulation. This example uses
voice 2 as a one-voice prototype, materializes one instance at voice 16, selects
last-note legato mode, and maps MIDI channel 1 with a ±12-semitone bend range:

```text
v2 w0 a0 t.01,.15,.7,.35
/pg 1 2 1 0
/pp 1 1 16 1 0
/pm 1 1 0 1
/mp 0 1 12
```

The `/pm` arguments after the pool number are `mode=1` (mono), `priority=0`
(last note), and `articulation=1` (legato). Other priorities are `1` highest,
`2` lowest, and `3` first. With overlapping keys, releasing the active note
returns to the appropriate still-held note; the envelope releases only after
the last key is released.

For polyphony, define the sound as a prototype group, clone several instances,
then route MIDI to the pool. This two-voice sound layers an octave above its
root and creates four playable instances in voices 8 through 15:

```text
v0 w0 a0 t.01,.15,.7,.35 N0 G1 H1
v1 w0 a0 t.005,.1,.5,.25 N12
/pg 0 0 2 0
/pp 0 0 8 4 0
/mp 0 0 2
```

Each MIDI note allocates one two-voice instance. Note-off releases the instance
identified by that channel and note, and channel pitch bend applies to all
notes still held through that route. The final `/pp` argument is the stealing
policy: `0` release-oldest, `1` oldest, `2` round-robin, `3` quietest, or `4`
no-steal.

Use `.` instead of a channel number to accept all channels:

```text
/mp . 0 2
```

All-channel pool routes keep equal notes from different channels independent.
Install several `/mv` or `/mp` routes when only a selected set of channels
should drive the instrument. `/mR` displays the installed routes; `/mvd` and
`/mpd` remove them.

With an output open, `MO 144,60,100` sends a three-byte note-on message.
`d>MO` sends every value in the data array as a raw byte, which is useful for
SysEx. These commands execute immediately and are not pattern-schedulable.

| Command | Parameters | Effect |
| --- | --- | --- |
| `?`, `v?` | None | Displays the selected voice. |
| `\` | None | Displays the selected voice with additional detail. |
| `??`, `v??` | None | Displays active voices. |
| `W [wave[,end-or-width[,height]]]` | Optional display parameters | Displays one wavetable, recording data, or all loaded waves. A single-wave display includes sample count, baseline duration, playback mode, loop points, loop duration, stats, and a loop marker row under the waveform. |
| `VW [voice[,width,height]]` | Optional voice and display dimensions | Displays the wavetable assigned to a voice and marks that voice's current loop points. With two arguments, they are interpreted as width and height for the selected voice. |
| `WS wave` | Wavetable index | Displays a compact spectrogram over the wave's loop region. |
| `W* wave,param[,register]` | Wave, property, optional destination | Reads wave sample count (`0`), sample rate (`1`), duration (`2`), loop start (`3`), or loop end (`4`). |
| `v* param[,register]` | Property and optional destination | Reads selected voice wave (`0`), amplitude (`1`), or frequency (`2`). |
| `DL? [track]` | Optional track `1..4` | Displays one track delay, or all four track delays, as copy/pasteable `DL...` commands. |
| `GS [full]` | Optional boolean | Displays copy/pasteable version, master volume, tempo, and track delay commands. With a value greater than `0`, also prints a larger text snapshot for saving/reloading. |
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
| `/h [category[,entry]]` | Optional help indices; current string may select by name | Displays generated command help. Use `[term] /h` to search. |
| `/q` | None | Requests exit from the interactive shell. |

`W` reports the selected renderer as `display braille` or `display ascii`.
It uses the compact Unicode Braille plotter on Linux and macOS. Windows
console hosts default to an ASCII plot because common `cmd.exe` and PowerShell
fonts render Braille cells poorly. Set `SKRED_WAVE_DISPLAY=braille` to force
the Unicode renderer, or `SKRED_WAVE_DISPLAY=ascii` to force the portable
ASCII renderer on any platform.

For a single wavetable, the marker row beneath the waveform spans
`[loop_start..loop_end)`, matching the `WL` convention: start is inclusive and
end is exclusive.

`w<>` scans the temporary recording buffer for four consecutive samples above
the start/end silence thresholds, then moves to a nearby zero crossing. The
optional third argument expands the found region by that many samples before
the zero-crossing search. With no arguments, tiny nonzero recorder residue
below `0.001` is treated as silence.

## File and Ksynth Utilities

| Command | Parameters | Effect |
| --- | --- | --- |
| `/l file[,verbose]` | Numbered Skode file | Loads and executes a `.sk` file. |
| `[filename] /ls [verbose]` | Skode filename | Loads and executes a named Skode file. The literal filename is tried first; bare names also fall back to `sk/filename`. |
| `[filename] %cat` | String | Prints a text file. |
| `[zip-or-directory] %z` | String | Mounts a ZIP archive or disk directory as the active VFS root. |
| `%zu` | None | Unmounts and returns to disk mode at the real current directory. |
| `%pwd` | None | Displays VFS mode, mounted root, and VFS working directory. |
| `[directory] %cd` | String | Changes the VFS working directory. |
| `%ls [type]` | `0` `.sk`, `1` `.wav`, `2` `.mp3`, `3` `.ks` | Lists matching files in the active VFS directory. |
| `[file] /ks [verbose]` | Ksynth filename | Loads a named Ksynth source file. Requires `KSYNTH`. |
| `/k file[,verbose]` | Numbered Ksynth file | Loads numbered Ksynth source. Requires `KSYNTH`. |
| `[code] ks`, `[code] k!` | Ksynth source string | Runs source in this Skode context's Ksynth evaluator. |
| `kw [timeout-ms]` | Optional timeout | Compatibility no-op; Ksynth now runs synchronously. |
| `kw> [timeout-ms]` | Optional timeout | Copies the latest Ksynth result into parser data. |
| `k?` | None | Displays the latest Ksynth result. |
| `k>d` | None | Copies the latest Ksynth result into parser data. |
| `k>w [slot[,rate[,mode[,offset]]]]` | Wave destination and options | Loads the latest Ksynth result directly into a wave. Defaults to slot `300`, the main sample rate, and cycle mode. |
| `d>k variable` | Variable `0` through `25` | Copies parser data into Ksynth variable `A` through `Z`. |
| `w>k wave,variable` | Wave index and variable `0` through `25` | Copies a wavetable directly into Ksynth variable `A` through `Z`. |

Loaders search the active VFS first, then the real current directory and their
type-specific `sk`, `wav`, or `ks` fallback directory. While a ZIP is mounted,
prefix a path with `file:` to force access to a real filesystem path.
`skred_vfs_mount_zip_memory()` provides the equivalent memory-backed mount for
browser/API hosts.

Each Skode command context owns its own Ksynth evaluator and persistent
`A` through `Z` variables. For example, `(1 2 3) d>k0 [A,A] ks kw>` binds
`A` before evaluating the concatenation; `kw` is accepted for compatibility
but no longer waits. Sample-rate and loop metadata are not copied with array
values; provide the desired rate and playback mode to `k>w` or `/d`. Vectors
are limited to one million elements.

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
k>w300
```

This binds the data array to `A`, reverses it with Ksynth's monadic `i`, and
loads the result into wavetable 300 as a cycle at the main sample rate.

**Transform a wavetable directly:**

```text
w>k10,0
[w A] ks
k>w300
```

This copies wave 10 into `A`, peak-normalizes it with Ksynth's monadic `w`,
and stores the result in wave 300. `w>k` avoids changing the current Skode
data array during import.

**Concatenate two built-in wavetables into a periodic waveform:**

```text
w>k10,0
w>k11,1
[A,B] ks
k>w300
v0 w300,1
```

Built-in waves 10 and 11 each contain 4096 samples. The comma concatenates
`A` and `B` into one 8192-sample cycle. `k>w300` stores it in the default cycle
mode, and `w300,1` selects it with interpolation enabled while inheriting that
mode. The oscillator wraps from the end of wave 11 back to the beginning of
wave 10.

**Concatenate two wavetables into a one-shot waveform:**

```text
w>k10,0
w>k11,1
[A,B] ks
k>w301,44100,1
v0 w301,1
```

This creates the same 8192-sample sequence but stores it in wave 301 as a
one-shot. `w301,1` selects it with interpolation enabled and inherits
one-shot mode, so a trigger reads wave 10 followed by wave 11 and then stops
at the end instead of wrapping. Use another `k>w` rate when the source material
is not 44100 Hz.

`kw` is kept for older scripts and does nothing. `k>d` and `kw>` both copy the
latest completed result.

## Example Sounds

These are starting points rather than exact emulations. The synthesizer
examples require the named feature gates, and the sample examples require
`KSYNTH`. Run each example separately, or reset its voices with `S` before
trying the next one.

Standalone example banks are available under `parts/examples/`:

- `fm-dx-inspired.sk` contains four directly copyable `FF2`/`FB` examples.
- `pd-cz-inspired.sk` installs `czbr`, `czbs`, `czbl`, and `czpd`.
- `moog-inspired.sk` installs `mgbs`, `mgld`, `mgpl`, and `mgpd`.
- `ksynth-drums-inspired.sk` renders waves `470..472`, configures voices
  `0..2`, and installs `kick`, `snar`, and `chat`.

Load a macro bank with `/ls`, then invoke the named patch with a MIDI note:

```text
[examples/pd-cz-inspired.sk] /ls
czbr 48
~1 v0 l0
```

### Synthesizer-Inspired Patches

**Moog-style resonant bass.** A saw wave, fast filter envelope, and resonant
low-pass produce the rounded attack and dark sustain associated with classic
Moog bass patches. Requires `ADSR`, `FILT`, and `FADSR`.

```text
S0
v0 w2 a0 t.005,.18,.7,.25 J1 K120 Q5 ft.002,.25,.08,.3 fd2600 n36 l1
~.75 v0 l0
```

**Casio CZ-1-style phase-distortion brass.** A resonant phase-distortion shape
gives a bright, slightly hollow digital brass tone. Requires `ADSR` and `PD`.

```text
S0
v0 w0 a0 c6,-.15 ct.01,.3,.35,.45 cd.9 t.015,.35,.55,.45 n48 l1
~1 v0 l0
```

Try `c7,-.25 ct.005,.2,.2,.3 cd.8` for a thinner resonant sweep, or
`c1,-.6 ct.002,.15,.1,.2 cd1` for a more obvious saw-to-pulse attack.

**Korg DW-8000-style brass pad.** Wave `22` is the built-in DWGS brass
wavetable. A slow filter envelope softens its digital harmonic spectrum.
Requires `ADSR`, `FILT`, and `FADSR`.

```text
S0
v0 w22 a0 t.08,.5,.7,.8 J1 K500 Q2 ft.12,.7,.3,.8 fd3200 n48 l1
~1.5 v0 l0
```

The 16 built-in DWGS waves occupy slots `15` through `30`; wave `15` is
strings, `18` is electric piano, `20` is clavinet, and `29` is bell.

**DX-inspired electric piano.** Voice `0` is a muted 2:1 sine modulator with
light feedback. Voice `1` is the audible carrier. `G0 H0` makes the carrier's
note and velocity commands also reach the modulator. Requires `ADSR` and `FM`.

```text
S0 S1
v0 w0 m1 N12,0 a0 t.001,.28,0,.18 FF2 FB.2
v1 w0 G0 H0 a0 t.002,1.2,.18,.6 FF2 F0,1.38 n52 l1
~1.5 v1 l0
```

Increase `F0,1.38` toward `F0,1.8` for a harder tine, or increase `FB.2` toward
`FB.4` for a buzzier modulator. `m1` removes voice `0` from the main mix while
leaving it available as a modulation source.

**DX-inspired struck bell.** Three sine operators form a current-sample chain:
voice `0` modulates `1`, which modulates audible carrier `2`. The transpositions
give approximate 3:1, 2:1, and 1:1 ratios. Requires `ADSR` and `FM`.

```text
S0 S1 S2
v0 w0 m1 N19,2 a0 t.001,.16,0,.25 FF2 FB.54
v1 w0 m1 N12,0 a0 t.001,.7,0,.5 FF2 F0,.89
v2 w0 G0,1 H0,1 a0 t.001,2.4,0,1.2 FF2 F1,1.05 n69 l1
~2.5 v2 l0
```

Keep modulators at lower voice numbers than the operators they feed. The
current renderer processes voices in ascending order, so `0 -> 1 -> 2` uses
current samples throughout the chain.

**Feedback bass.** A same-ratio feedback operator gives the carrier a bright,
growling attack that decays into a sine fundamental. Requires `ADSR` and `FM`.

```text
S0 S1
v0 w0 m1 N0,0 a0 t.001,.22,.08,.18 FF2 FB1.27
v1 w0 G0 H0 a0 t.002,.35,.7,.3 FF2 F0,1.51 n36 l1
~1 v1 l0
```

Raise `FB1.27` toward `FB2` for a noisier edge. Lowering `F0,1.51` changes the
brightness without changing the feedback operator's own spectrum.

**Metallic percussion.** A short, non-octave modulator envelope and strong
feedback produce an inharmonic digital strike. Requires `ADSR` and `FM`.

```text
S0 S1
v0 w0 m1 N7,0 a0 t.0005,.09,0,.08 FF2 FB2.01
v1 w0 G0 H0 a0 t.0005,.55,0,.25 FF2 F0,2.9 n57 l1
~.7 v1 l0
```

Changing `N7,0` on voice `0` changes the partial spacing; try `N12,0` for a
more harmonic strike or `N19,2` for an approximate 3:1 ratio.

### DX-Inspired FM Versus a DX7

`FF2` and `FB` provide the two most important signal-path ingredients for
DX-like patches: lookup-phase modulation and operator self-feedback. They do
not make this engine a DX7 emulator:

- A DX7 voice has six operators selected from 32 fixed algorithms. Skode uses
  general synth voices and explicit `F` connections, with one modulation input
  per destination voice.
- Skode's operator level envelope is ADSR. A DX7 operator has four rates and
  four levels, so imported envelopes cannot yet be represented exactly.
- Ratios are approximated with `N` transpose/cents. There is no dedicated
  DX ratio/fixed-frequency mode, coarse/fine mapping, or DX detune curve.
- Keyboard level/rate scaling, per-operator velocity sensitivity, pitch
  envelope, and the DX LFO/modulation-sensitivity model are not implemented.
- Voice evaluation is ascending by voice number rather than compiled from an
  operator algorithm. A higher-numbered modulator supplies its previous
  sample, while a lower-numbered modulator supplies its current sample.
- Poly groups can clone six-voice graphs, but their root note/gate propagation
  currently reaches only four linked voices in addition to the root.
- `FB 0..7` is a continuous phase-index scale using a two-sample average. Its
  numbers are musically useful but are not a bit-exact recreation of the DX7
  feedback-level table, sine ROM, arithmetic, quantization, or aliasing.

These differences make the examples starting points for the same families of
sound rather than compatible reproductions of DX7 patches or SysEx data.

### Drum Samples

The repository includes Ksynth definitions for synthesized drum one-shots.
When Skode is started from the `parts` directory, these commands generate a
kick, snare, and closed hi-hat in dynamic wave slots `300` through `302`:

```text
[sk/drums-kick.ks] /ks k>d d>r /r300
[sk/drums-snare.ks] /ks k>d d>r /r301
[sk/drums-chh.ks] /ks k>d d>r /r302
```

The standalone `examples/ksynth-drums-inspired.sk` file performs this setup,
assigns the waves to voices `0..2`, and installs `kick`, `snar`, and `chat`
trigger macros.

Assign each sample to a voice and trigger it with `l1` or `T`:

```text
v0 w300 f440 a10
v1 w301 f440 a10
v2 w302 f440 a10

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
[sk/nap-fm-alarm.ks] /ks k>d d>r /r310
[sk/nap-noise-sweep.ks] /ks k>d d>r /r311
[sk/nap-perc-zap.ks] /ks k>d d>r /r312

v3 w310 f440 a10
v4 w311 f440 a10
v5 w312 f440 a10
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
v0 w0 n69 a0 p0 l1
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

Most commands below exist only when their build feature is enabled. MIDI
management atoms remain parseable in all builds but report an unavailable
backend unless `MIDI=1`:

| Feature | Commands |
| --- | --- |
| `ADSR` | `k`, `t` |
| `AM` | `A` |
| `CRUSH` | `q` |
| `FILT` | `J`, `K`, `Q` |
| `FILT` and `FADSR` | `ft`, `fd` |
| `FM` | `F`, `FF`, `FB` |
| `GLISS` | `g` |
| `KSYNTH` | `/ks`, `/k`, `ks`, `k!`, `kw`, `kw>`, `k?`, `k>d`, `k>w`, `d>k`, `w>k` |
| `MIDI` | `/mL`, `/m?`, `/mi`, `/miV`, `/mic`, `/mo`, `/moV`, `/moc`, `MO`, `d>MO`, `/mv`, `/mvd`, `/mp`, `/mpd`, `/mR`, `/mC`, `/mb`, `/mb?`, `/mbd`, `/mbC` |
| `PANMOD` | `P` |
| `PD` | `c`, `C`, `ct`, `cd` |
| `RECORD` | `r`, `/rg`, `/rs`, `/r?` |
| `SCOPE` | `r`, `/sg`, `/ss`, `/s?` |
| `SAH` | `h` |
| `SEQ` | Timing, event, and pattern commands |
| `SMOOTHER` | `s` |
| `TRACKS` | `r`, `rt`, `rv`, `?r`, `ds`, `DL`, `DL?` |
| `UDP` | `udp` and API/host UDP startup |
| `XM` | `XM` |
