# Making Sounds, Patterns and Songs in PULP/SKRED (Skode)

This is the definitive guide to making full tracks in Skode, built around our showcase song: **`hammer_the_signal.sk`**. 

*HAMMER THE SIGNAL* is a 134 BPM industrial techno-pop track (68 bars, ~2 minutes) written entirely as a single Skode script. It is designed to be studied, stripped down, and stolen from.

**For human readers:** This guide will hand you the mental models to build dense, evolving tracks out of simple text commands. It is packed with creative sound design recipes, pattern techniques, and structural ideas you can drop right into your own music.

**For AI Agents using this as a skill:** This document is your bridge from "knowing the syntax" to "writing a song that actually works and sounds professional." Use `hammer_the_signal.sk` as your gold-standard structural template. Where something is marked **verified**, it ran successfully in `mini-skred`. When in doubt, trust the engine's error messages.
---

## 1. Workflow that works (especially for AI agents)

Do not write a song blind. The engine will tell you what is wrong; ask it.

1. **Build a headless engine.** `make maxed` in `parts/` (needs `cmake` and ALSA dev headers).
2. **Give ALSA a null device** so there is no sound card requirement:
   ```
   # ~/.asoundrc
   pcm.!default { type null }
   ctl.!default { type null }
   ```
3. **Run scripts through stdin** with `-n` (no editor):
   ```
   printf '[song.sk] /ls\nwait 300\n/q\n' | mini-skred -n 2>&1 | grep -v ALSA
   ```
   Read every line of output. Compile failures are reported per step.
4. **Render to WAV.** `/rg <max-seconds>` records a multitrack float WAV
   (10 channels; channels 0-1 are the master). Start recording, then start the song.
   Stop with `/rs` or let it hit the max.
5. **The null device runs faster than real time** (roughly 10x here), and the
   sequencer follows the audio sample clock, so the recording is correct anyway.
   `wait N` is *wall-clock* milliseconds, so size it as `song_seconds * 110`.
6. **Start recording at song start.** Load the song with the final `y0 z1` omitted,
   start `/rg`, then send `Z0` / `y0 z1`. Otherwise you record silence while it loads.
7. **Measure.** Per-bar RMS and peak, per-bar spectral centroid, per-group solo
   levels, DC offset. Numbers catch things your reading of the script will not.

**Align your analysis to the song, not to the recording.** Rendering faster than real
time means the song starts at a variable offset after `/rg` begins (I saw 0.3 s to 5 s).
Per-bar measurements with a fixed offset lie: I briefly "found" fills one bar late
that were fine. Add a marker voice that fires on conductor step 0 (kept unmuted in
solo renders) and measure from its onset.

> Caveat: this proves the script parses, schedules and has sane levels. It does not
> prove it sounds good. Always listen.

---

## 2. Rules the engine enforced (all verified)

| Rule | Consequence |
|---|---|
| Filter commands are **lowercase**: `j` mode, `k` cutoff, `q` resonance. They are schedulable. Uppercase `K` is rejected inside a step | Older docs and the built-in `/h` text still showed `J K Q`; the docs were corrected upstream. If you see "immediate-only", you are using the old uppercase names. |
| Bitcrush is `bc [bits] [curve]`, not `q` | `q` is now filter resonance. `q5` does not crush. |
| A step compiles to at most **32 operations** | Big "stop all / reset all" steps fail to compile. Split across two conductor bars. |
| `m1` **mutes** a voice, `m0` unmutes | Easy to get backwards. Hidden modulator voices need `m1`. |
| `ds` (delay send) takes **0-1, or 0-15 which is auto-scaled** (per the delay doc) | Ambiguous near 1, so use decimals (`.2` to `.45` in the song). The send only works for **centred, unmodulated-pan** voices. |
| A pattern's length is set by its **last non-empty step** | Trailing empty steps do not extend it. Pin the length with a harmless write on the last step, e.g. `[=127,0] x15`. |
| `y`, `z`, `Z`, `x`, `ce`, `=N,v`, `DF`, `DS`, `G`, `H`, `P`, `C`, `A`, `XM`, `a`, `l`, `n`, `g`, `N` are schedulable | Patterns can start and stop other patterns. That is how the conductor works. |
| `?M 1` lists the schedulable commands | Check it before assuming a command works inside a step. |
| `nS<N>` (stream read by name) is immediate-only; `n&<N>` is the schedulable form | Use `&N` in steps. |
| `ft` / `fd` (filter envelope) | I only ever set these at voice setup and trigger them with `l`. I did not test scheduling them. **Another build rejected both at load** (`SKODE_UNKNOWN_FUNCTION`), see section 2c. If you cannot run the engine, leave them out and use a static filter, or ask for a load log. |
| `[name] vt` | Sets the voice text label (e.g. `v13 [vocal lead synth] vt`). When inspecting voices via `v<N> ?` or `v??`, Pulp prints the voice string with its label. Essential for debugging arrangements with 20+ voices. |
| `[ ... ] n>d` | Converts a space-separated string of note names into a data array. Supports note names (`C4`, `F#2`, `Bb1`), octaves, numbers, and rest symbols (`-`, `.`, `~`, `R`) which emit `NAN`. |
| Rest handling (`NAN`) | When reading a stream via `n&<S>`, a `NAN` rest automatically releases the voice and suppresses subsequent `l` (gate) triggers in that step. |
| K-synth source: `N: 264600` ran **out of memory**; `N: 131072` was fine | Keep inline samples short. Stretch them with playback frequency. |

---

### 2b. Starting, stopping and ending cleanly

- Wrap start and stop in macros so humans and scripts use one entry point:
  ```
  [use 'play' to start the song, 'stop' to stop it] puts   # prints at load time
  [play] : Z0 y0 z1 ;
  [stop] : Z0 O DQ ;
  ```
  `puts` forces text to the console when a script loads. `O` sends `l0` to **all**
  voices (kills stuck pads, risers and held notes). `DQ` clears all delay lines.
- `DF` (delay freeze) holds at unity feedback **indefinitely**, so a song that ends
  with a freeze rings forever unless something clears it. That was the noisy tail.
- `O` is **immediate-only**, so it cannot sit in a pattern step. Bind it as a
  control-event responder and trigger it from the song with a deferred `ce`:
  ```
  [O DQ] /ceb 4 99
  /cer 1
  # last conductor step:  Z0 +8 ce 99     (8 beats later: release everything, clear delays)
  ```
  Verified: the freeze holds for the 8 beats, then the output decays to silence.

### 2c. Parser quirks that bite when loading

Found while building a cover song on a real build.

| Quirk | What happens |
|---|---|
| A `;` anywhere inside a `#` comment | The comment ends there and the parser starts looking for commands again. Never write semicolons in comments. The `;` that closes a `[name] : ... ;` macro definition is fine. |

**Reading load errors.** `SKODE_UNKNOWN_FUNCTION <n>` means the engine does not accept a
command at that point in the load. One build rejected `ft` and `fd` (filter envelope) in
voice setup. Remove them, move them into a pattern step, or try the documented names
`kt` and `kd`. Check `?M` for what your build supports.

**Run it section by section.** Start each logical block of the file with a comment line that
begins `##`, so a REPL or editor can run one block at a time. A good order is: header and
`M`, delay buses, voices, events, streams, patterns, conductor, macros. Later blocks refer
to earlier ones, so run them in file order. When something fails you immediately know which
block to look at.

**Keep a voice map at the top of the file.** A short comment table (kick v0, bass v6 with
partner v7, pad pool v28-v37, boom v38 to v40) stops two sections from claiming the same
voice number.

### 2d. Labeling voices via `[name] vt`

In complex arrangements with 20 to 40 voices (like our full cover arrangements), tracking which voice does what becomes difficult if voices are anonymous numbers. Pulp supports voice text tagging via `vt`:
```
v0  w82 f440 a7 p0 [kick motorik] vt
v1  w83 f440 a8 p0 r3 ds.25 [snare 80s] vt
v6  w15 a-1 p0 t.001,.18,0,.04 [moog bass saw] vt
v13 w15 a-6 p0 t.003,.26,0,.08 [vocal lead synth] vt
```
When you inspect a voice at the REPL or in a script (`v13 ?` or dump active voices with `v??`), Pulp displays the full voice state with its label:
```
v13 w15 f440 n69,0 a-6 G14,-,-,-,-,- H14,-,-,-,-,- ds0.3 j1 2 k3000 q1.8 c4,0.2 r1 g0.015 t0.003,0.26,0,0.08 [vocal lead synth] vt
```
This makes diagnosing arrangement bugs, envelope releases, and voice routing dramatically simpler.

### 2e. Defining note streams with `n>d` and rest syntax

Instead of writing raw numeric MIDI arrays by hand, use `[ ... ] n>d`:
```
[Bb1 Bb1 Bb2 Bb1  Bb1 Bb1 Bb2 Bb1  Bb1 Bb1 Bb2 Bb1  Ab1 - Ab1 -] n>d /SS 1
```
- **Note names**: `C`, `D`, `E`, `F`, `G`, `A`, `B`.
- **Accidentals**: `#` or `+` for sharps, `b` or `-` for flats (e.g. `Bb1`, `F#2`, `Eb4`).
- **Octaves**: Direct digit after the note name (`0` through `9`).
- **Rests**: `-`, `.`, `~`, or `R` emit `NAN` into the data stream.
- **Step Sequencer Integration**: In a pattern step, read the stream with `[v6 n&1 l1] x0`. When `n&1` encounters a rest (`NAN`), the engine:
  1. Sets `voice_is_rest = 1` for that voice.
  2. Calls `voice_release(...)` to cleanly release the amplitude envelope.
  3. Suppresses any subsequent `l` (gate) in the same step, guaranteeing the rest stays completely silent without re-triggering.

### 2f. The Anti-Hanging Pattern (Preventing Stuck Notes)

A classic trap in step sequencer arrangements is voices getting "stuck" droning on their sustain level. This occurs when a conductor pattern transition stops a pattern (`y10 z0`) while a voice has an active note and an envelope with non-zero sustain (`s > 0`).

**The three rules to prevent stuck notes:**
1. **Use `s=0` decay envelopes for melodic leads, vocals, and chords**:
   Instead of `t.004,.35,.6,.25` (sustain = 0.6), write:
   ```
   v13 w15 a-6 p0 t.003,.26,0,.08 [vocal lead synth] vt
   ```
   With `s=0`, notes naturally decay to silence over their decay time, so even if a pattern halts abruptly, the voice will never drone.
2. **Explicitly release voices on conductor pattern stops**:
   Whenever the conductor silences a pattern, add explicit `l0` commands:
   ```
   # When vocal pattern y10 stops, kill any lingering release tail:
   y10 z0 v13 l0
   ```
3. **Prefer dedicated linked voices (`G`, `H`) over poly voice pools (`/pp`) for sequenced chords**:
   Poly voice pools (`pn`, `pr`) manage voices dynamically by note key. However, when a pattern is stopped with `z0`, the engine does NOT automatically release active voices in the poly pool! If the prototype envelope has `s > 0`, the allocated voice will drone forever. Furthermore, pools consume extra operations and cannot easily be sidechain-pumped without individually automating every voice in the pool. Dedicated voice trios (`v18 G19,20 H19,20`) with `s=0` decay envelopes are completely immune to hanging notes and consume a fraction of the operations.

### 2g. Stream Length Alignment (Preventing Cumulative Desync)

When reading note streams via `n&<S>` inside a looping pattern, **the number of tokens in the stream must exactly equal the pattern step length** (or a clean sub-multiple):
- If a 64-step pattern reads a stream containing 74 tokens, each loop will consume 64 tokens, leaving 10 unread. On the next loop, the melody starts 10 steps late! Across just a few bars, the song will drift completely out of sync.
- Always verify token counts: in Python generators, assert `len(notes) == pattern_steps`.
- For sustained notes (e.g. 2-beat piano chords or slow lead notes), do NOT fill non-trigger steps with rest tokens (`-`) if the step program evaluates `n&<S>` on every step, because `NAN` immediately releases the voice envelope! Instead, trigger the note on the onset step and leave intermediate steps empty or un-evaluated.
- **NEVER overwrite the final step with `[=127,0] x<last>`**:
  A common technique to extend sparse patterns to their full bar length is pinning the last step (e.g. `[=127,0] x15` or `x31`). **Only do this if the last step is actually empty!** If a pattern already writes an event on step 31 (such as `for step in range(32): emit(f"[v8 n&3 l.95] x{step}")`), writing `[=127,0] x31` immediately overwrites step 31, replacing the stream pull with register zeroing. Because token 31 is never pulled, the stream falls 1 token behind on each loop cycle, causing the entire melody to slip off-beat by one sixteenth step on every single repeat! If step $N$ already has an event, the pattern length is already pinned to $N+1$ steps; never overwrite it.



---

## 3. Waves worth knowing

Verified by `W` listing and by ear-free measurement:

- **W15** analog-style saw, **W16** analog-style square. Prefer these over the
  older `W1`/`W2`/`W3` shapes for synth parts.
- **W0** sine. **W5** noise (cycle, so it is periodic, not true noise; it has DC).
- **Built-in drum one-shots (play them at `f440`):**

| Wave | Sound | Wave | Sound |
|---|---|---|---|
| 82 | kick | 90 / 93 | hi tom |
| 83 | snare | 91 / 92 | lo / mid tom |
| 84 | closed hat | 94 | crash |
| 85 | open hat | 95 | cymbal |
| 86 | clap | 96 | cowbell |
| 87 | rim | 97 | clave |
| 88 / 89 | lo / mid tom | 98 / 99 | maracas / trigger |

You do not need to load drum sample files. Use these directly.

---

## 4. Sound design recipes (each one ran, except where marked)

Syntax shape: `v<N>` selects a voice, then commands follow. Amplitude `a` is in dB.

**Kick with pitch drop (no pitch envelope needed, use glide).** Layer a sample kick
with a sine sub and glide the sub down from a high note:
```
v0 w82 f440 a6 p0
v5 w0 a-3 p0 t.002,.30,0,.10 j1 k180 q.7
# in a step:
[v0 l1  v5 g0 n50 g.07 n28 l1]
```
`g0` snaps to the start pitch, then `g.07` plus the target note glides over 70 ms.

**Reese bass: two detuned saws, one note drives both.**
```
v6 w15 a-1 p0 t.002,.17,0,.06 j1 k650 q4 ft.001,.15,0,.05 fd2000
v6 >7          # copy settings to voice 7
v7 N0,14       # +14 cents
v6 G7 H7       # G forwards notes, H forwards triggers
```
Short decay envelope (`sustain 0`) means no release steps are needed. Put bass on the
offbeats and the kick on the beats for a natural sidechain feel.

**303 acid.** Saw, resonant low-pass, filter envelope, glide for slides, character 2 drive:
```
v8 w15 a0 t.002,.20,0,.06 j1,2 k350 q10 ft.001,.18,0,.06 fd2600 g.03
```
In the pattern, slide notes get `g.045`, normal notes `g0`. Vary velocity (`l.6` to `l1`) for accents.

**Industrial clang: ring modulation of two inharmonic oscillators.**
```
v9  w16 f311 a3 t.001,.35,0,.2 bc5 XM10 H10    # bc5 = bitcrush
v10 w15 f187 a0 t.001,.30,0,.2 m1              # modulator, muted from the mix
# step:  [v9 f311 v10 f187 v9 l1]
```
Change both `f` values per hit so successive clangs are different, inharmonic pitches.

**FM metal stab.** Phase modulation with operator feedback; the modulator tracks pitch
via links and a fixed offset:
```
v11 w0 a0 t.001,.45,0,.35 FF2 FB4 F12,2.2 G12 H12
v12 w0 a0 t.001,.30,0,.30 N21 m1
```
`N21` (21 semitones, about ratio 3.36) gives a non-harmonic, bell-metal spectrum.

**Poppy hook: detuned saw trio with phase distortion.**
```
v13 w15 a-8 p0 t.004,.25,.4,.14 j1 k5200 q1.4 c4,.35 ct.001,.2,0,.1 cd.5 r1 ds.4
v13 >14   v13 >15
v14 N0,10 p-.55 r0     v15 w16 N0,-10 p.55 r0
v13 G14,15 H14,15
```
Only the centre voice feeds the delay (`r1`); panned voices should not send to delay.
**Phase-distortion mode matters:** `c1`, `c3` and `c5` added measurable DC offset;
`c2` and `c4` did not. Prefer `c4` (or `c2`).

**Riser: inline K-synth sample, stretched by playback rate.**
```
[
N: 131072
T: !N
E: (e(T*(4.6%N)))%100
R: r T
L: 0.35 f R
H: R-L
F: 900*(e(T*(2.7%N)))*((p 2)%(p 0))
P: +\F
S: s P
W: w E*((S*.35)+(H*.6))
] k!
k>w301,44100,1
# voice: v16 w301 f187 a8 p0 t.005,.01,1,.4
```
About 3 s of audio played at `f187` lasts about 7 s (about 4 bars at 134 BPM). The
rising envelope plus the noise-minus-lowpass high-pass gave zero DC. Multi-line
`[ ... ] k!` works inline in a `.sk` file.

**Delay moves that paid off (from the delay doc, verified in the song).**
- *Freeze as a capture loop.* `DF 1 1` as the breakdown begins holds the hook's echo
  as a drone under the pad; `DF 1 0` when the hook returns. Cheap, and it sounds
  composed.
- *Rhythmic delay-time stepping.* Large `DS`/`DT` jumps glide audibly (doppler pitch
  throw). A one-shot pattern that steps `DS 2 134 .5`, `.1875`, `.125`, `.75`, `.25`
  across the last half bar of each fill gives tempo-synced throws with no extra opcodes.
- `DS` is a one-shot conversion; it does not follow later tempo changes, so re-issue
  it if you change `M`.
- Idle buses cost nothing; four buses is cheap. Delay time range is up to about 1 s.
- Other patches from the doc worth stealing: slapback (`DT 90`, feedback 0), ADT
  doubling (`DT 22` plus slight mod), tape/dub echo (`DT 320`, `DD 9 2`), dub siren
  (feedback near 15, short time, deep mod).

**Delay buses (per-track, tempo synced).**
```
DS 1 134 .75      # track 1: dotted eighth at 134 BPM
DL 1 - - 8 3 3 11 # '-' keeps coarse/fine; feedback, mod rate, mod depth, level
DD 1 7 3          # damping / highpass on the feedback path
DP 2 1            # ping-pong on track 2
DF 1 1            # freeze (used for the final tail)
```
Route a voice with `r<track>` and set its send with `ds<0..1>`.
Delay needs the voice **centred** (`p0`).

**Detuned partner voice (width, while keeping a centred voice for the delay send).** *Added later, written but not yet load-tested.*
```
v6 w15 a-1 p0 t.002,.20,0,.05 j1 k900 q2.5
v6 >7
v7 w16 a-5 N0,9 p.5 r0
v6 G7 H7
```
`>` copies the voice, `N0,9` detunes the partner 9 cents sharp, the partner is panned and
has no delay route, and `G`/`H` make it follow the main voice's notes and triggers. A
saw plus a square sounds thicker than two saws. Only the centred voice may feed a delay.

**An audible sub boom.** *Added later, not yet heard by the author. The first version with a 32 Hz target was reported as a high "dink", this is the revised version.* A sine sweep that ends near 32 Hz sounds like a high "dink" on
laptop speakers and headphones, because only the first milliseconds of the drop are
audible. End the sweep around 65 Hz (C2) or higher, give it a long decay, add a delay
tail, and layer a slowed kick sample. Playback frequency changes pitch and length, so
`f110` is two octaves down and four times longer.
```
v38 w0 a0 p0 t.002,.90,0,.40 j1 k260 q.7 r3 ds.3
v40 w82 f110 a4 p0
# in a step:  [v38 g0 n48 g.15 n36 l1 v40 l1] x16
```

**Choosing the delay time.** *Reasoning from a cover song build, the steady-pulse blur was reported by ear.* The last number of `DS` is in beats: `.25` sixteenth, `.5`
eighth, `.75` dotted eighth, `1` quarter. A dotted-eighth echo lands between constant
eighth notes and blurs a steady pulse. Use a short sixteenth slapback to thicken a
driving bass, and save dotted eighths for sparse leads. You can keep a voice routed
(`r2`) and silence it with `ds0`, then raise `ds` later without touching anything else.
There are four buses, one per track 1 to 4, and a routed voice also lands in that
track's stem.

---

## 5. Pattern techniques

**Basic step grammar.** `y<N>` select pattern, `[commands] x<step>` write a step,
`%<n>` set the step clock divider, `z1` / `z0` start / stop.
With `M 134 16`, one step is a sixteenth note.

**One generator, one source of truth.** The song was produced by a Python script that
emits the `.sk` file. That removes copy-paste errors and lets you keep every level
in one table. Strongly recommended for anything over about 100 lines.

**Polymeter.** A 12-step pattern running against 16-step drums realigns every 3 bars.
Cheap, effective industrial tension.

**Ratchets.** Add `z*N` to a step to repeat it N times within the step. Rising `z*2`,
`z*4`, `z*8` plus rising velocity makes a snare roll.

**The conductor.** Pattern 0 at `%16` is one step per bar. Each step starts and stops
other patterns (`y4 z1`, `y1 z0`) and fires one-shots (`v21 l1` for a crash). That is
the whole arrangement in about 70 short lines, and it is easy to re-order.
Start the other patterns from the conductor, not at load time.

**Automation lanes.** Patterns are not just notes. A pattern whose steps write `k`
(cutoff) and `q` (resonance) is a filter-sweep lane. Because those commands are
schedulable, it is one line per step:
```
[v8 k820 q9.5] x4
```
Generate exponential cutoff curves in code (`300 * exp(phase * ln(3400/300))`) so
sweeps feel even, and slow the lane with `%` (for example one write every 2 steps).
Measure it: the hook's per-bar spectral centroid rose from about 760 Hz to 2100 Hz over
the four-bar build sweep.
For any command that really is immediate-only, the fallback still works: write a
register in the step (`=1,820`) and fire `ce 1`, with a responder bound by
`[v8 k$1] /ceb 4 1` and enabled with `/cer 1`. Check `?M 1` to see which commands
are schedulable before reaching for it.

**Streams: note and parameter sourcing with `/SS` and `&N` (verified).**
A stream is a numbered array (0-127) that a step reads from. Fill it once at load,
then read it in any schedulable command with `&N`:
```
( 60 64 67 72 ) /SS 1        # notes
( .3 1 .6 1 )   /SS 2        # velocities
y1 %4
[v16 n&1 l&2] x0             # one step plays 60 64 67 72, with those velocities
```
- Each read **advances** that stream, and streams advance independently, so one step
  can pull a note, a velocity and a glide time from three different streams.
- `nS1` is the immediate-only form and is rejected inside a step. Use `&N`.
- `/SS` is immediate-only. To change a stream from a running song, keep **memory
  banks** and copy with `/SC dst src` (schedulable; copies contents, mode **and
  position**, so it also rewinds). `/SP stream,pos` sets the position and `/SM
  stream,mode` sets wrap / reverse / ping-pong / clamp. All three are schedulable.
- Streams do not rewind when a pattern restarts. If a pattern is stopped mid-phrase,
  reset with `/SP` when you start it again.
- What it bought in the song: the hook, bass and acid are now rhythm-only patterns
  reading notes, glide and accent from streams. Drop 2 swaps in a higher hook and a
  walking bass with one `/SC` each, so the second drop is a variation without a
  second copy of the patterns. The 128-step acid filter lane collapsed to a **single
  step at `%2`** reading cutoff and resonance from two 64-value streams.
- Remember: a step is capped at 32 operations, and every `/SP`/`/SC` counts. The drop-2
  conductor step went over the cap until I removed redundant `/SP` calls (`/SC` already
  rewinds) and moved a filter reset one bar earlier.
- A stream read inside one step that touches several voices advances once per read, so
  `k&5` on three voices consumes three values. Use one stream per voice, or keep the
  per-voice work in separate steps.

**Fake sidechain.** A 4-step pattern (one beat) that sets pad/hook amplitudes to
`base-14`, `base-8`, `base-3`, `base` on consecutive 16ths. Because it writes
*absolute* amplitudes, it overrides whatever you set at voice setup, so keep the base
levels in one table used by both the setup and the pump. On stopping the pump, also
write the base levels explicitly.

---

## 5b. Pattern flow, polyphony and events (from the PATTERN_SEQUENCING, POLYPHONY and EVENTS docs)

**One-shot patterns with the stop marker (verified).** A step containing only `-`
stops the pattern and rewinds it, so a fill can run once and clean up after itself:
```
y13
[v1 l.2] x0  ...  [v1 l.95 z*8] x15
[-] x16                     # plays steps 0-15 once, then stops
```
The conductor then only ever starts fills (`y13 z1`); it no longer needs matching
stop calls, which also saves operations in busy steps. For a two-bar fill, start it
on two consecutive bars.

**Other flow commands (from the docs; I did not test these):**
`-N` waits until pattern N reaches step 0 (locks polymetric fills back to the groove);
`-jN` jumps the playhead, and `N` can be a literal, a variable (`-j$1`) or a stream
(`-j&0`), which gives loop brackets inside a long pattern; `-sN` stops the pattern if
`N` is truthy (literal, `$1`, or `&0`), a stream-driven conditional stop;
`zg<step>` jumps; `y<N> zq1` / `zq0` start or stop on the next downbeat (good for live
jamming); `y<N> ym1` / `ym0` mute and unmute while keeping position.
Named section macros (`[sec_verse] : y0 z1, y1 zq1 ;`) plus `wait` are an alternative
to a conductor pattern for hands-off songs, but they run on wall-clock time and are
not sample-locked, so the conductor pattern is better for tight arrangement.

**Half-time without new patterns.** `y3 z%2` in a conductor step slows pattern 3's
step clock so it plays at half speed (compiled and used for the outro hats; I have
not listened critically to it). `z%` is the dynamic version of the `%` divider.

**Polyphonic pad with a voice pool (verified).** Instead of hand-assigning three pad
voices, build a prototype, make it a group, then allocate instances:
```
v26 w15 a-8  p-.5 t1.2,.4,.8,1.4 j1 k2400 q.8 c4,.2 C24,.2 G27 H27
v27 w15 a-10 p.5  N0,9 t1.2,.4,.8,1.4 j1 k2400 q.8 c4,.2 C24,.2
/pg 0,26,2,0              # group 0 = voices 26-27, root 26
/pp 0,0,28,5,0            # pool 0 = 5 instances of group 0 at voices 28-37, steal oldest releasing
# in a pattern step (schedulable):
[pn 0,101,52,.9  pn 0,102,55,.9  pn 0,103,59,.9]   # chord: pool, key, note, velocity
[pr 0,101  pr 0,102  pr 0,103]                      # release by key
```
- The **key** identifies a note's lifetime, not its pitch. Keys can repeat across loops.
- Extra instances let the long release of one chord overlap the next instead of being
  stolen. Policies: 0 release-oldest, 1 oldest, 2 round-robin, 3 quietest, 4 no-steal.
- `pn`, `pr`, `pb` (bend) are schedulable. `/pg`, `/pp`, `/pm` are immediate-only.
- References to voices outside the prototype stay absolute, so all instances share one
  LFO (`C24`). Pool copying keeps each destination's own `r`, `ds` and `vc`.
- **Pool voices still need their amplitude set directly** if something (like a pump
  pattern) automates level. Count the operations: 10 pad voices at 2 ops each is 20.
- Mono pools (`/pm pool,1,priority,legato`) give held-note priority and legato, which is
  the right tool for a legato bass or lead. Not used here.
- Inspect with `?pp` and `/vg <voice>` (dependency graph).

**Events: reacting to the engine (verified for pattern events).**
- Three producers: voice events (`vc1` on the voice; types 1-3 trigger/release/finished),
  pattern events (`yc1` on the pattern; types 5 start / 6 end, once per loop), and user
  events (`ce id`, always on). MIDI is type 7.
- Bind a response with `[commands] /ceb <type> <key>` and turn the dispatcher on with
  `/cer 1`. The song taps a cowbell on every hook phrase with
  `y9 yc1`, `[v22 l.6] /ceb 5 9`.
- **Opt-in is easy to forget.** My first attempt did nothing until I added `y9 yc1`.
- Responders run on the control thread, **not** sample-accurately. In a headless
  faster-than-real-time render I measured one tap about 90 ms late. Use events for
  accents, cleanup, and structure; keep the groove in patterns.
- A named macro must be a bounded "realtime" macro to be legal in a response; check
  with `?m`.

**Engine facts that matter for songs (ARCHITECTURE.md):**
- One sample-count timeline drives patterns and queued events; buffer size does not
  change musical time. Tempo accepts 1-960 BPM and tempo changes keep position.
- Patterns now expand dynamically (the old 128-step ceiling is gone). A step still
  compiles to a bounded program, hence the 32-operation limit.
- Pattern edits take a lock and the audio thread skips pattern work for that callback
  if it loses the race, so **load the song, do not live-edit patterns** during a render.
- The engine is a singleton and inactive voices are skipped, so a 256-voice build is
  fine; the pool and the extra voices cost little.
- Compilation is all-or-nothing: an unsupported command is rejected at load, never
  deferred to the audio thread. That is why the load log is a reliable test.

## 6. Song structure that worked

Bars are the unit; sections are multiples of 4 so loops stay aligned.

| Bars | Section | What happens |
|---|---|---|
| 0-7 | Intro | Quiet pad, polymeter clangs, hats from bar 4, riser |
| 8-15 | Build | Kick, then bass and a filtered hook sweeping open, snare roll on the last bar, drums drop out for tension |
| 16-31 | Drop 1 | Crash, full drums, bass, hook; acid from 20, stabs from 24, chops from 28 |
| 32-39 | Breakdown | Drums out; the hook's delay is frozen (`DF 1 1`) as a drone; pad, chops, clangs; riser and hook sweep back in, freeze released at bar 36 |
| 40-43 | Build 2 | Kick and hats return, snare roll |
| 44-59 | Drop 2 | Everything at once, fills on bars 51 and 59 |
| 60-67 | Outro | Strip back to kick, clangs, pad; hats drop to half time; delay freeze; final crash, `Z0`, then 8 beats later `O` and `DQ` clean up |

Musical choices: E minor, progression Em-C-G-D (bass roots E1, C1, G1, D1),
offbeat bass against four-on-the-floor kick, off-beat metal stabs on the "and" of 2 and 4.
A hook that rises to the high A on the last bar of its four-bar phrase is what makes it
"poppy"; the industrial grit comes from the crushed ring-mod clangs and screamer acid.

---

## 6b. Covering an existing song

Notes from building an instrumental cover. Make an instrumental arrangement of the
riffs and harmony. Do not transcribe lyrics.

1. **Gather two or three sources**: bass tabs, piano tutorials, chord charts, a
   production write-up. Cross-check them. Confirm key and tempo as well as notes.
2. **Count the notes.** A tutorial that lists the riff as "C x4, Eb x2, C x2, Ab x3, C,
   G x3, C" gives 16 notes, which is exactly two bars of eighth notes. A count settles
   a rhythm argument that no tab can.
3. **Do not trust ASCII tab spacing for rhythm.** It is hand aligned. Use tabs for
   pitches and counts or the audible pulse for timing.
4. **Write every note that sounds, not just the changes.** My first pass wrote only
   "C Eb C Ab G" with gaps and it sounded like stumbling. The real line repeats each
   pitch on every eighth and is steady.
5. **Look for layered riffs.** Many records pair a constant riff with an intermittent
   one, often panned apart. Build the constant one first, it carries recognition.
6. **Tab to MIDI.** Bass guitar open strings: E1 28, A1 33, D2 38, G2 43, then add the
   fret. A capo shifts chord shapes up by that many semitones, so capo 3 on Am F Dm E
   is really Cm Ab Fm G.
7. **Make it correctable by ear using `n>d` streams with rests.** Put all notes in named streams using `[ ... ] n>d` where `-` represents a rest:
```
# Authentic B-flat minor Vince Clarke 16th bassline:
[Bb1 Bb1 Bb2 Bb1  Bb1 Bb1 Bb2 Bb1  Bb1 Bb1 Bb2 Bb1  Ab1 - Ab1 -  Bb1 Bb1 Bb2 Bb1  Bb1 Bb1 Bb2 Bb1  Bb1 Bb1 Bb2 Bb1  Db2 - Eb2 -] n>d /SS 1
```
   In the pattern, read the stream sequentially:
```
y4
[vince clarke 16th bassline] yt
[v6 n&1 l1] x0
[v6 n&1 l1] x1
# ...
[v6 n&1 l1] x31
```
   When `n&1` hits `-`, the engine automatically releases the voice envelope and suppresses `l` triggers in that step. Reset streams in the conductor with `/SP 1,0` whenever looping or starting a section.
8. **Label guesses as guesses** in comments, and say which lines are placeholders.
9. **Iterate with the listener.** Ask which note is late or missing, then change one note name in the stream.
10. **Showcase Repertoire in `parts/skred-music-guide/`**:
    - **`hammer_the_signal.sk`** (134 BPM, E minor): Original industrial techno-pop demo demonstrating 32-op limits, delay freeze drone capture, and polymeter clangs.
    - **`whip_it.sk`** (158 BPM, E Mixolydian): DEVO's 1980 new wave classic. Demonstrates the whip crack via slowed snare (`w83 f175`), motorik Alan Myers drum feel, Moog bass/guitar chug trio linked via `>7 >8 G7 H7 G8 H8`, call-and-response vocal lead synth with safe `s=0` decay envelopes, and full `[name] vt` voice labeling.
    - **`dont_go.sk`** (128 BPM, B-flat minor): Yazoo's 1982 synthpop masterpiece by Vince Clarke & Alison Moyet. Demonstrates the authentic B-flat minor key, the signature opening staccato synth brass hook with octaves (`Db5 - C5 - Bb4 - F4` and leap to `F5`), relentless 16th-note bouncing bassline with rests, 16th Vince Clarke arpeggio feeding dotted-eighth echo Bus 1, and Alison's vocal laugh/cry formant pitch glides.
    - **`closer.sk`** (90 BPM, C minor): Nine Inch Nails' 1994 industrial tour-de-force by Trent Reznor. Features the "Nightclubbing" heartbeat kick (`w82` + sub glide `w0`), rubbery Minimoog bassline with octave snaps and chromatic slides, the ascending chromatic tension hook (`E -> F -> G -> Ab`), singing vocal lead synth lines, crushing chorus synth brass and sidechain pumping, delay freeze drone capture (`DF 1 1`), and the haunting Downward Spiral outro piano solo through ping-pong hall delay.
    - **`i_feel_love.sk`** (128 BPM, C modal): Donna Summer & Giorgio Moroder's 1977 electronic disco foundation. Features the iconic machine-gun Moog modular 16th-note bouncing bassline (`C -> Eb -> F -> G`), 16th-note slapback / ping-pong tape delay recreation on Bus 1, Donna Summer singing leads ("Ooh, it's so good...", "I feel love..."), cascading high sparkle arpeggios, 1970s Solina/ARP disco strings, Moroder white noise wind sweeps, and the legendary delay freeze breakdown (`DF 1 1`).
    - **`sweet_dreams_full.sk`** (125 BPM, C minor): Eurythmics' 1983 synth-pop classic by Annie Lennox & Dave Stewart. Features the iconic 2-bar Roland SH-101 / Juno bass riff (`C -> Eb -> Ab -> G`), left-panned plucky counter-riff, Dave Stewart's milk bottle crack & sub boom impact on bar 2 downbeats, expressive Annie Lennox singing vocal leads (Chorus: "Sweet dreams are made of this...", Verse: "Some of them want to use you...", Bridge: "Hold your head up..."), screaming analog portamento synth solo, and warm synth pad chords with zero-hanging envelopes.
    - **`devo_uncontrollable_urge.sk` / `uncontrollable_urge.sk`** (184 BPM, A Major / Mixolydian): DEVO's 1978 punk-funk opening blast from *Q: Are We Not Men? A: We Are Devo!*. Features Alan Myers' relentless motorik drums and snare crescendo builds, Gerald Casale's punchy bass riff and 8th-note drone (`w15 saw + w16 square`), Bob Mothersbaugh's jagged staccato power chord trio (`w15/w16`), wide stereo rhythm guitar double-tracking, Mark Mothersbaugh's quirky Minimoog bleeps and stabs, full vocal arrangements ("Got an urge, got a surge...", "It's got style, it's got class...", and the manic "YEAH! YEAH! YEAH-YEAH-YEAH-YEAH!" screams), DEVO's legendary precision start-stop dead silence breaks with metallic pick scrapes, and Bob Mothersbaugh's frantic angular guitar solo.


**Step grid cheat sheet** (with `M 125 16`, one pattern step is one sixteenth note):
4 steps per beat, 16 per bar. Eighth notes are every 2 steps, quarters every 4. A 2-bar
loop is 32 steps, so pin it with `[=127,0] x31`. A conductor on `%16` advances one
step per bar.

---

## 7. Mixing by measurement

Ears first, but numbers find what ears miss.

- **Solo every group** (mute others with `m1`, keep hidden modulators muted too) and
  record RMS and peak over a section where the group is playing. This exposed that the
  clang, stabs and pad were 15 to 20 dB below the kick.
- **Per-bar RMS and peak** across the whole song shows whether sections actually
  differ in energy, and where clipping happens. The first full render peaked above 3.0.
- **Per-bar spectral centroid** confirms sweeps really sweep.
- **DC offset** (mean of the signal). A few percent means a source is asymmetric
  (certain PD modes, the noise wave, asymmetric drive). Fix the source.
- Typical final numbers for this song: drops about -18 dB RMS, peak 0.9, DC about 0.002.
- Change one thing, re-render (seconds, not minutes), re-measure.

---

## 7b. Checking your work when you cannot run the engine

If you are an agent without the engine, or a human sharing a script with someone who has
it, you still have options.

1. **Generate the file from a script** with one table for notes, one for levels and one
   for step lists. You will edit a number, not hunt through the file.
2. **Lint before you hand it over.** `skode_lint.py` (next to this guide) checks for
   semicolons in comments, unbalanced brackets and long steps. Its long-step
   check is only a token-count guess, so the real 32-operation count still comes from
   the engine.
3. **Ask for the load log, section by section.** Because each block starts with a
   `##` line, a failing line points at a block. Fix one block, rerun, move on.
4. **Say exactly what was verified.** Written, linted, loaded, heard and measured are
   five different claims. Report only the ones that are true.

---

## 8. Mistakes to avoid (checklist)

- Using uppercase `J`/`K`/`Q` (old names). Filter commands are lowercase `j k q`, and bitcrush is `bc`.
- Using semicolons in comments. A semicolon stops the comment and the parser immediately starts looking for atoms or numbers again, which can cause confusing errors.
- Writing `m0` to hide a voice. It is `m1`.
- `ds` values above 1.
- Forgetting that a pump or automation pattern overrides setup levels.
- A pattern whose last steps are empty (it will loop short).
- Steps with more than 32 operations.
- A giant inline K-synth sample (`N` over about 131k risked OOM).
- Giving panned voices a delay send.
- Judging a mix by the script. Render and measure.
- Assuming a command is schedulable. Check `?M 1`.
- Putting `ft` / `fd` in voice setup on a build that rejects them. Remove them or ask for a load log.
- A dotted-eighth echo on a constant eighth-note part. It blurs the pulse.
- A sub boom that ends below about 60 Hz. It reads as a click on small speakers.
- Writing only the note changes of a riff and not each repeated note.
- Two sections claiming the same voice number. Keep a voice map.
- Leaving poly pools active when stopping patterns with `z0` (voices drone forever if `s > 0`; use linked voices with `s=0` decay envelopes instead).
- Token count in note streams mismatching pattern length, causing cumulative timing desync across pattern loops.
- Placing rest tokens (`-` / `NAN`) on intermediate steps of a sustained note when reading a stream on every step (causes premature choke).

---

## 9. Notes for an AI agent using this as a skill

1. Fetch and read the repo's `SKODE_USER_COMMAND_REFERENCE.md` first; this guide is
   lessons, not the reference.
2. Read `hammer_the_signal.sk` in this directory. It is the gold-standard showcase of these patterns in action. If you are asked to generate a song, copy its structural separation (Voice setup -> Conductor Pattern -> Musical Patterns).
3. Build the headless engine and render loop (section 1) **before** writing a song.
4. Generate the `.sk` from a script with a single table of per-voice base levels.
5. Write voices first, patterns second, conductor last. Test after each stage.
6. Treat every engine error as ground truth and fix the script, not the engine.
7. Report honestly: say what was measured, what was only parsed, and that the
   result has not been heard.
8. Prefer built-in resources (waves 15/16 for synth, 82-99 for drums) over loading files.
9. Keep semicolons out of comments (section 2c) and start each block with a `##` comment so the human can run it in pieces.
10. If you cannot run the engine, say so, run `skode_lint.py`, and ask for load logs by block (section 7b).
11. For covers, follow section 6b: cross-check sources, count the notes, write every repeated note, and keep pitch and rhythm in separate editable places.

## 10. Notes for humans

- **Open `hammer_the_signal.sk` right now.** Play it, tweak the tempo (`M 134`), change a synth wave (`w15` to `w16`), or solo the drums. Breaking a working song is the fastest way to learn how to make your own.
- **Start simple:** Start with a four-on-the-floor kick pattern and one voice. Get that to sound right before you add the bells and whistles.
- **Give each element its own pattern:** The conductor pattern (pattern 0) is the brain of the song. Let it trigger individual, self-contained loops.
- **Think in sections of 4 bars.** Contrast is the song: take things away as often as you add them. Drop the bass to make the hook shine, or drop the drums to create tension.
- **Automate everything:** Static filters sound like presets. Map a pattern to sweep `k` (cutoff) over 4 bars and the song will suddenly breathe.
- **Width vs. Focus:** Detune, pan and layer for width, but keep your kick, sub, and bass completely centered.
- **Run a song in `##` blocks.** Load the header, then delay, voices, events, streams, patterns, conductor and macros one at a time. When something is wrong you will know which block.
- **Fix by ear with one number.** Good scripts keep notes in a stream and the rhythm in a single step list, so "the third note is late" is a one-number change.
- **Keep a measurement habit:** If the drop is not measurably louder and denser than the build, no amount of cleverness in the notes will make it *feel* like a drop. Mix with your ears, verify with the numbers.
