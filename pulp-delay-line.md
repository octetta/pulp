# PULP Delay Line — Design, Tutorial & Patch Book

*Covers the 4-bus track delay as of v0.55.1, including the freeze rework
(unity-feedback hold, not whole-buffer loop). Commands: `DL`, `DD`, `DF`,
`DP`, `DT`, `DS`, plus the routing commands `r` and `ds`.*

---

## 1. Design overview

PULP's delay is **4 independent buses**, tied 1:1 to tracks 1–4. Each bus
is a self-contained mono-in/stereo-out digital delay loosely modeled on the
Korg DW-8000's 12-bit delay chip: linear amplitude quantization on write,
no dither, 2–512ms native range (extendable to 1024ms — see §7). The goal
was never a museum-accurate recreation; it's "useful knobs that happen to
lean lo-fi," built to stay cheap enough that having all 4 buses active
costs nothing next to your voice count.

### Signal flow, per bus, per sample

```
        base_frames (smoothed toward target, avoids zipper clicks)
              │
   input ──►[write]                    ┌─────────────┐
              │                        │  buffer[N]   │  N = 65536 frames
              │                        │ (+ buffer_r  │  (~1.486s @ 44.1kHz)
              │                        │  if pingpong)│
              ▼                        └──────┬───────┘
   feedback path                              │ read (interpolated,
   (damping LP → HP filter)                   │  offset by base_frames
              ▲                               │  ± LFO modulation)
              │                               ▼
   [× feedback_gain] ◄───────────────────  delayed_left / delayed_right
              │
   [+ input] → [12-bit quantize] → write back into buffer
              │
              └──► wet output (× level/15) ──► summed into master mix
```

Everything in that diagram is **O(1) per bus per sample** — no allocation,
no unbounded loops, one buffer read/write per channel. Four buses of this
is a rounding error next to synthesizing even a handful of voices.

### Why it's cheap even with 4 buses running

- **Idle buses skip processing entirely.** If a bus gets no input for
  about 1.5 seconds, it's marked inactive and the per-sample loop `continue`s
  past it — zero cost until something feeds it again.
- **Every added feature (damping, hp, ping-pong, freeze) is 1–2 extra
  multiply-adds per bus per sample.** At 4 buses that's negligible next to
  a voice loop doing oscillators, envelopes, and filters per-voice.
- **Parameter changes (coarse/fine/feedback/damping/etc.) are cached**,
  not recomputed per sample — `delay_cache_params()` runs once when you
  send a command, not 44,100 times a second.

---

## 2. Routing — getting sound into a bus

Two things have to be true for a voice to reach a delay bus:

1. **The voice is routed to a track** with `r voice track` — `track` is
   1–4 (0 = master only, no track/delay routing).
2. **The voice has a delay-send amount** set with `ds voice amount` —
   `amount` is 0–1 (or 0–15, auto-scaled). Delay send only works for
   voices that are centered in the stereo field (no pan, no pan
   modulation) — panned voices don't feed the delay bus.

Track *N* always feeds delay bus *N*. There's no separate "which bus does
this track go to" mapping — they're the same index.

```
r 1 2        # voice 1 → track 2
ds 1 0.6     # voice 1 sends 60% of its signal into bus 2
DL 2 4 8 6 0 0 12   # configure bus 2's delay
```

---

## 3. Command reference

| Command | Args | Range | What it does |
|---|---|---|---|
| `DL` | bus coarse fine feedback mod-freq mod-depth level | bus 1–4, coarse 0–7, fine 0–15, feedback 0–15, mod-freq 0–31, mod-depth 0–31, level 0–15 | Core delay params. Omitted trailing args keep their current value. |
| `DL?` | [bus] | — | Show status for one bus, or all four if no bus given. |
| `DD` | bus [damping] [hp] | 0–15 each | Feedback-path tone shaping. `damping` darkens repeats (lowpass), `hp` thins low end (highpass) on long feedback chains. |
| `DF` | bus [0\|1] | — | Freeze. `1` holds the current echo at unity feedback (loops indefinitely at a stable level); `0` releases back to normal decay. |
| `DP` | bus [0\|1] | — | Ping-pong. Cross-feeds L/R feedback instead of summing to mono. |
| `DT` | bus ms | ~4–1024ms | Set delay time directly in milliseconds (converts to coarse/fine internally). |
| `DS` | bus bpm division | division: 1.0=quarter, 0.5=eighth, 0.75=dotted-eighth, etc. | Tempo-synced delay time. One-shot conversion — does **not** track tempo changes after the fact. |
| `r` | voice track | track 0–4 | Route a voice to a track/delay bus (0 = no delay routing). |
| `ds` | voice amount | 0–1 (or 0–15) | Delay send amount for a voice. Requires centered, unmodulated pan. |

### What the numbers actually mean under the hood

- **`feedback` (0–15)** → gain = `(feedback/15) × 0.82`. Max feedback gain
  is 0.82, not 1.0 — this is why normal (non-frozen) delays always decay
  eventually, even at max feedback.
- **`mod-freq` (0–31)** → LFO rate, 0–10 Hz.
- **`mod-depth` (0–31)** → time modulation of up to ±25% of the base delay
  time, plus (in non-ping-pong mode) a 2–10ms L/R offset for width.
- **`damping` (0–15)** → feedback lowpass cutoff sweeps 20kHz (off) down
  to 500Hz (dark) as you raise it.
- **`hp` (0–15)** → feedback highpass cutoff sweeps 20Hz (off) up to 2kHz
  (thin) as you raise it.
- **`coarse`/`fine`** → `base_ms = 8 × 2^coarse`, then scaled by
  `0.5 + (fine/15) × 0.5`. You'll rarely touch these directly now that
  `DT`/`DS` exist — they're the underlying encoding those commands write to.

---

## 4. Tutorial: your first delay, step by step

```
r 1 1              # voice 1 → track 1
ds 1 0.7            # send 70% of voice 1 into bus 1
DL 1 4 8 6 0 0 12   # bus 1: ~64ms base time, feedback 8/15, no mod, level 12/15
DL? 1               # check what landed
```

`DL? 1` will print something like:

```
DL1,4,8,6,0,0,12 DD0,0 DF0 DP0
```

That's coarse=4, fine=8, feedback=6, mod-freq=0, mod-depth=0, level=12,
followed by damping/hp/freeze/pingpong state.

Now make it musical instead of guessing coarse/fine by hand:

```
DT 1 250          # bus 1 delay time = 250ms, directly
DS 1 120 0.5      # or: eighth note at 120 BPM (also 250ms — same result)
```

Add some character and width:

```
DD 1 8 3          # moderate damping, light hp trim
DP 1 1            # ping-pong on
```

And when you want a moment to hang in the air:

```
DF 1 1            # freeze — current echo holds indefinitely
...
DF 1 0            # release, decay resumes at the configured feedback
```

---

## 5. Patch book — the delays people expect

### Slapback
Short, single, no-nonsense repeat. No feedback buildup, no mod.
```
DT 1 90
DL 1 - - 0 0 0 10   # feedback 0 = single repeat, no tail
DD 1 0 0            # bright, clean
DP 1 0
```

### Doubling / ADT
Very short delay, just enough to thicken without being heard as an
echo. Slight mod adds subtle chorus-like movement.
```
DT 1 22
DL 1 - - 2 4 3 8
DD 1 0 0
```

### Tape/dub echo
Moderate feedback, real damping so repeats get progressively darker —
the classic "echoes fading into the tape hiss" character.
```
DT 1 320
DL 1 - - 10 2 4 11
DD 1 9 2            # dark repeats, slight low-end trim so it doesn't get boomy
```

### Ambient wash
Long time, high feedback (near the 0.82 ceiling), gentle slow mod so
the tail shimmers instead of just repeating identically.
```
DT 1 650
DL 1 - - 14 3 10 9
DD 1 6 0
```

### Ping-pong stereo delay
```
DT 1 300
DL 1 - - 9 0 0 12
DP 1 1
```

### Tempo-synced dotted-eighth (classic "U2/dub" delay-throw timing)
```
DS 1 128 0.75
DL 1 - - 9 0 0 10
```

### Freeze pad
Play a chord, catch it, hold it as a drone under a lead line.
```
DL 1 4 8 10 3 6 12
DP 1 1
... (play the chord) ...
DF 1 1              # freeze holds it, stable, indefinitely
```

---

## 6. Way-out patches

These lean on side effects of how the engine is built, not on any single
"do the weird thing" knob — which is exactly why they're cheap: no new
code, just using the existing parameters against their grain.

### Dub siren (feedback near max)
Push `feedback` close to 15 (gain ≈ 0.82) with short time and heavy mod
depth. It won't fully self-oscillate (0.82 max gain prevents runaway),
but it gets close enough to scream and swirl in a very dub-siren way.
```
DT 1 60
DL 1 - - 15 20 28 14
DD 1 3 8       # a little hp keeps it from turning into a bass rumble
```

### Pitch-bend / doppler via fast time jumps
`base_frames` ramps toward its target with a one-pole smoother rather
than snapping instantly — that's there to kill zipper clicks on normal
parameter changes. But if you jump the time by a *large* amount with
`DT`, the ramp itself becomes audible as a brief pitch glide, tape-warp
style, while it catches up.
```
DT 1 60
... (let it settle, play something) ...
DT 1 500     # big jump — repeats pitch-bend downward for a moment
DT 1 60      # jump back — pitch-bend upward
```
Sequencing rapid alternating `DT` calls in a pattern turns this into a
rhythmic pitch-wobble effect for free — no extra state, no extra CPU,
just using the smoothing that's already there for a different purpose.

### Freeze as a live capture loop
Since `DF` holds whatever's circulating in the current delay-time window
at the moment you engage it, freezing right after a phrase effectively
captures that phrase as a loop — not a purpose-built looper, but usable
as one in a pinch, especially on a longer time setting (500ms+) where
there's enough room to catch a short musical gesture.

### Rhythmic delay-time stepping (classic dub-siren-adjacent trick)
Step `DT` (or `DS` with changing `division`) once per sequencer step
instead of setting it once. Each jump triggers the doppler glide above,
so a pattern like `DS 1 128 0.25`, `DS 1 128 0.5`, `DS 1 128 0.1875` fired
on successive steps gives you rhythmic pitch throws synced to your
sequencer, entirely from parameter automation — no new opcode needed.

### Four-bus diffusion network
Route the same voice to all 4 tracks with different `ds` amounts, and
give each bus a different time, damping, and ping-pong state. You get a
denser, more diffuse tail than any single bus can produce, still at
O(1)-per-bus cost — 4× the per-sample work of one bus, still trivial in
absolute terms.
```
r 1 1  r 1 2  r 1 3  r 1 4
ds 1 0.5
DT 1 180   DL 1 - - 8 0 0 8    DD 1 2 0   DP 1 0
DT 2 233   DL 2 - - 9 0 0 7    DD 2 5 0   DP 2 1
DT 3 340   DL 3 - - 10 0 0 6   DD 3 8 2   DP 3 0
DT 4 410   DL 4 - - 11 0 0 5   DD 4 11 3  DP 4 1
```

---

## 7. Performance notes

- Memory: each bus is two 65536-float buffers (`buffer` + `buffer_r` for
  ping-pong) — 512KB per bus, 2MB total across all 4. Fixed at compile
  time, no runtime allocation.
- Max delay time is currently 1024ms (widened from the DW-8000-accurate
  512ms ceiling to use headroom the buffer already had). If strict
  hardware-accurate range matters for a specific patch, cap `coarse` at
  6 manually via `DL` rather than using `DT`/`DS`, which will use the
  full 1024ms range.
- `DS` is a one-shot ms conversion, not a live tempo follow — changing
  your global tempo afterward does not retune buses that were set with
  `DS`. Re-issue `DS` if tempo changes.
- Nothing described here changes the per-sample cost class of the delay
  bus — every addition so far has been 1–2 extra flops per bus per
  sample, dwarfed by idle-bus skipping and by voice synthesis cost
  elsewhere in the engine.

---

## 8. Not yet implemented (ideas on the table)

Mentioned in earlier design discussion but not committed to code — listed
here so this doc doesn't imply they exist yet:

- **Variable bit-depth / "grit" knob** — parameterizing the 12-bit
  quantizer (currently fixed at 2047 levels) so you can dial between
  clean and lo-fi per bus. Would be a `DD`-style command, similarly cheap.
- **Cubic (Hermite) interpolation** on the delay read, replacing the
  current linear interpolation — cleaner modulated/chorus-y delay at a
  small additional per-read cost, still trivial at 4 buses.
