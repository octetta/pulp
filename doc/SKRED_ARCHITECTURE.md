# Skred Architecture: Three-Layer Execution Model

This document explains how Skred works internally so you can predict and reason
about what happens when you type commands, build patterns, and receive events.
Understanding these three layers resolves most confusion about why certain words
"don't work" in patterns, how timing is guaranteed, and how to connect engine
events to external programs.

---

## The Three Layers at a Glance

```
┌─────────────────────────────────────────────────────────┐
│ LAYER 1 — Audio Thread                                  │
│  seq() called once per audio buffer (~128 frames)       │
│  • Advances master clock tick                           │
│  • Fires compiled pattern steps (event_program_t)       │
│  • Drains timed event queue (skqueue)                   │
│  • Writes voice lifecycle notifications → control ring  │
└───────────────────────┬─────────────────────────────────┘
                        │ lock-free ring buffer (1024 slots)
                        ▼
┌─────────────────────────────────────────────────────────┐
│ LAYER 2 — Control Event Ring                            │
│  skred_control_event_t: type, sample, voice, pattern…   │
│  • Not parsed or executed here                          │
│  • Read by: host poll, wait-fd select, or Layer 3       │
└───────────────────────┬─────────────────────────────────┘
                        │ thread wakeup via pipe / HANDLE
                        ▼
┌─────────────────────────────────────────────────────────┐
│ LAYER 3 — Control Dispatcher (separate pthread)         │
│  Enabled with /cer 1; configured with /ceb              │
│  • Reads ring, looks up binding table                   │
│  • Runs matched Skode strings via skode_consume()       │
│  • Full parser: immediate-only words like >u work here  │
└─────────────────────────────────────────────────────────┘
```

---

## Layer 1 — The Audio Thread

The audio callback (miniaudio) calls `seq()` once per buffer. Inside `seq()`:

1. **Queued events are drained.** Any `event_t` whose sample timestamp ≤ `now`
   is executed immediately against the synth state. This is how `~0.5 l0`
   inside a pattern step schedules a note-off 500 ms in the future — the delay
   is resolved in samples and put in the skqueue; the audio thread fires it
   when the clock reaches it.

2. **The master tick advances.** The master tick is derived from wall-clock
   sample count, not callback count, so large audio buffers do not smear beat
   timing. All 128 patterns derive their step position from the same
   ever-incrementing master tick.

3. **Pattern steps fire.** For each running pattern, when `master_tick` reaches
   the next step boundary, the compiled `event_program_t` for that step is
   executed. This is pure bytecode dispatch — no string parsing, no allocation.

4. **Voice lifecycle events are written to the control ring.** Trigger, release,
   and envelope-finished events are emitted as `skred_control_event_t` structs.
   The audio thread never blocks on this — if the ring is full, the event is
   dropped and the drop counter increments.

### What can run in Layer 1?

Only **schedulable (realtime) opcodes** — a fixed set compiled from Skode text
into `opcode_event_t` bytecode. The complete list is in `skode-event.c`
(`skode_compile_callback`'s switch statement). Common examples:

| Schedulable ✓ | Immediate-only ✗ |
|---|---|
| `v`, `f`, `n`, `l`, `l0`, `l1` | `>u`, `e>N`, `<e` |
| `t`, `w`, `a`, `g`, `j`, `k`, `q` | `/ceb`, `/cer`, `udp` |
| `~N` delays, `z*N` ratchets | `y`, `z`, `ys?` |
| `ce N` (fires a user control event) | `?t`, `?v`, `?p`, `?st` |
| `=N,val` (register set) | `[…] e>N` (string/macro storage) |

If you try to put an immediate-only word into a pattern step with `xa`, you
will get a clear error naming the offending word:
```
# '>u' is immediate-only and cannot be compiled into a pattern step
# use 'ce N' in the step and bind N with '/ceb 4 N'
```

---

## Layer 2 — The Control Event Ring

The audio thread writes small `skred_control_event_t` structs into a bounded
lock-free ring (capacity 1024). Each event carries:

- `type` — what happened (see table below)
- `sample` — the exact audio sample count when it happened
- `voice`, `pattern`, `step`, `tag` — source context
- `id`, `value[3]` — for user events (`ce N`)

| Event type | When fired | Enable with |
|---|---|---|
| `VOICE_TRIGGER` | Voice envelope triggered | `vc1` on that voice |
| `VOICE_RELEASE` | Voice envelope released | `vc1` on that voice |
| `VOICE_FINISHED` | Envelope reached silence | `vc1` on that voice |
| `PATTERN_START` | Pattern loops to step 0 | `yc1` on that pattern |
| `PATTERN_STEP` | Any step fires | `yc1` on that pattern |
| `PATTERN_END` | Pattern hits a `-` stop step | `yc1` on that pattern |
| `PATTERN_WAIT` | Pattern blocked on `-N` sync | `yc1` on that pattern |
| `PATTERN_QUEUE` | `zq1` queued, waiting to start | `yc1` on that pattern |
| `PATTERN_DOWNBEAT_SWITCH` | Queued pattern actually starts | `yc1` on that pattern |
| `USER` | `ce N[,a,b,c]` executed | always |
| `MIDI` | MIDI message received | depends on binding |

These events are **notifications only** — no Skode commands are run at this
layer. External hosts (embedding apps, language bindings) can read the ring
directly with `skred_control_event_poll()`, wait on the file descriptor from
`skred_control_event_wait_fd()`, or let Layer 3 handle them automatically.

---

## Layer 3 — The Control Dispatcher

The dispatcher is an optional pthread that runs when you type `/cer 1`. It
sleeps on the notification pipe, wakes when Layer 2 events arrive, matches
them against a binding table (set up with `/ceb`), and executes the matched
Skode string with a full parser call (`skode_consume()`).

Because the dispatcher uses the **full front-end parser** (not the realtime
compiler), it can execute **any** Skode word — including immediate-only ones
like `>u`, `/ceb`, or `e>N`.

### Setting up bindings

```skode
( Bind event type 2 (VOICE_RELEASE), key = voice number 5 → command )
[ 5 0 >u ] /ceb 2 5

( Bind a user event id 42 to run a command )
[ 42 0 >u ] /ceb 4 42

( Enable the dispatcher )
/cer 1
```

The format string for `>u` is stored in macro slot 0 with `e>0`, then
referenced by index: `5 0 >u` means "take argument 5, format it using
the string in slot 0, and broadcast via UDP."

### Binding table keys

`/ceb TYPE KEY` — `KEY` is interpreted as:

| Type | Key |
|---|---|
| `VOICE_TRIGGER` (1) | voice number |
| `VOICE_RELEASE` (2) | voice number |
| `VOICE_FINISHED` (3) | voice number |
| `USER` (4) | event id (`ce N`) |
| `PATTERN_*` (5–15) | pattern number |
| `MIDI` (7) | raw MIDI message type |

---

## The Master Pattern and Downbeat Sync

When you queue a pattern with `zq1`, it doesn't start immediately. It waits
for the **master pattern's step 0** before starting, so patterns always launch
on a musical downbeat rather than mid-phrase.

By default, **pattern 0 is the master pattern**. You can change this:

```skode
yp        ( query: prints "# master pattern: 0" )
yp 3      ( set pattern 3 as master )
yp -1     ( disable sync: patterns start immediately on their own step 0 )
```

The master pattern indicator appears in `ys?` output, making it visible:
```
y0 %1 z1 ym0 yp0     ← pattern 0 is master
y1 %2 z0 ym0 zq1     ← pattern 1 is queued, waiting for master step 0
y3 %1 z1 ym0         ← pattern 3 is running normally
```

If the master pattern is stopped (`z0`), queued patterns fall back to waiting
for their own step 0 (they still align to a bar boundary, just their own).

---

## Timing in One Picture

```
sample timeline:
0────────128───────256────────────────────────────────────▶

Audio callback 1 (frames 0..127):
  seq(now=0):
    master_tick: 0 → 1 (if one step fits in 128 frames at current BPM)
    pattern 0, step 0 fires → event_program_t executes instantly
    queued events for t≤128 fired

Audio callback 2 (frames 128..255):
  seq(now=128):
    master_tick: 1 → 2
    control ring event for VOICE_TRIGGER written (from callback 1's trigger)
    ← dispatcher thread wakes, reads binding, runs />u/ command, broadcasts UDP
```

The dispatcher runs **asynchronously** relative to the audio thread. The UDP
broadcast for a trigger that happened at sample 128 will arrive at your client
a few milliseconds later, not at exactly sample 128. The `sample` field in the
control event tells you the precise sample-accurate time of the original event
if you need it for logging or UI sync.

---

## Common Patterns and Recipes

### React to a voice release, broadcast via UDP

```skode
( 1. Store format string )
[ /synth/release %d ] e>0

( 2. Enable voice lifecycle events on voice 5 )
v5 vc1

( 3. Bind voice-release event for voice 5 to format and send )
[ 5 0 >u ] /ceb 2 5

( 4. Start dispatcher )
/cer 1
```

### Fire a user event from a pattern step, handle it in the dispatcher

```skode
( Pattern step: fires user event 42 on every step )
[ ce 42 ] xa

( Out-of-pattern: bind event 42 to a UDP message )
[ /beat %d ] e>0
[ 42 0 >u ] /ceb 4 42
/cer 1
```

### Query what's running

```skode
Z?      ( show all active patterns with state, modulo, master flag )
ys?     ( show current pattern steps )
?ce     ( show control event ring contents )
??      ( show opcode event queue )
yp      ( show current master pattern )
```
