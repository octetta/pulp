# Advanced Skred UDP Events Reference

The `>u` word broadcasts formatted ASCII strings to any subscribed UDP clients (like visualizers or external engines). While you can use `>u` manually, the real power comes from coupling it with Skred's **Control Event Dispatcher** (`/ce`, `/cex`). 

This allows you to automatically broadcast UDP events exactly when samples finish, envelopes release, or sequencer patterns loop—with sample-accurate precision.

---

## 1. Voice Lifecycle: Envelope & Sample Endings
You can track when an ADSR envelope enters its release phase (e.g. from `l0` or exiting a loop region) or when a one-shot audio sample completely finishes playing.

Because the Skode parser uses `[` and `]` for string boundaries, nested brackets are not allowed. To use `>u` inside an event binding, we save the command to an **External Macro** (`e>N`) first, and bind it using `/cex`.

**Skode Input:**
```skode
( 1. Create our UDP broadcast commands in macro slots 0 and 1 )
[ [ /voice/finished %d ] 5 >u ] e>0
[ [ /voice/release %d ] 5 >u ] e>1

( 2. Select voice 5 and enable its lifecycle control events )
v5 vc1

( 3. Bind Macro 0 to Voice Finished (type 3) for Voice 5 )
/cex 0 3 5

( 4. Bind Macro 1 to Envelope Release (type 2) for Voice 5 )
/cex 1 2 5

( 5. Start the control event dispatcher thread )
/cer 1
```

Now, whenever Voice 5 is triggered and subsequently finishes playing its wave data, or its envelope is released, a UDP packet like `/voice/finished 5` will be automatically broadcast!

---

## 2. Sequencer Sync: Pattern Loop Points
You can trigger UDP broadcasts exactly when a sequencer pattern loops (starts or ends). This guarantees perfect visual synchronization with your generative patterns.

**Skode Input:**
```skode
( 1. Create a macro to broadcast when pattern 0 starts/loops )
[ [ /seq/loop %d ] 0 >u ] e>2

( 2. Enable control events for pattern 0 )
y0 yc1

( 3. Bind Macro 2 to Pattern Start (type 5) for Pattern 0 )
/cex 2 5 0

( 4. Ensure the dispatcher is running )
/cer 1
```
Whenever Pattern 0 wraps around to step 0, it emits the `SKRED_CONTROL_EVENT_PATTERN_START` event, executing Macro 2 and sending `/seq/loop 0` over UDP.

> [!TIP]
> **Pattern Control Event Types:**
> - `5`: Pattern Start (Downbeat / Loop point)
> - `6`: Pattern End
> - `9`: Pattern Step (Fires on every active step)
> - `10`: Pattern Change (Fires when a sequence jumps to a new pattern)

---

## 3. Direct Step Triggers (Inside Patterns)
Because `>u` is an immediate word (it formats strings and interacts with the UDP subsystem), it **cannot** be compiled directly into a real-time pattern sequence. 

Instead, use Skred's decoupled architecture: embed a user control event (`ce <id>`) into your pattern, and bind that ID to your `>u` macro!

**Skode Input:**
```skode
( Step 0: Play a kick on Voice 0 AND emit user event 42 )
[ v0 f60 a1 ce42 ] x0

( Step 4: Play a snare on Voice 1 AND emit user event 43 )
[ v1 f200 a1 ce43 ] x4

( Bind our UDP strings to those user events )
( Type 4 is SKRED_CONTROL_EVENT_USER )
[ [ /drum/kick %d ] 1 >u ] /ceb 4 42
[ [ /drum/snare %d ] 1 >u ] /ceb 4 43

( Start the dispatcher )
/cer 1
```

---

## 4. Including Event Metadata (Time, Voice, Pattern)
When your event macro executes, the dispatcher guarantees that the parser context knows exactly which voice, pattern, and step triggered the event. 

You can use the `?t`, `?v`, `?p`, and `?st` words to explicitly push these values onto the stack to embed them into your UDP strings! This gives your UDP clients sample-accurate timestamps.

**Skode Input:**
```skode
( Broadcast event with timestamp and source info! )
( Stack order: ?p pushes first, ?t pushes last )
[ [ /seq/event %g %d %d ] ?p ?st ?t >u ] e>3

( Bind to Pattern Change (type 10) for all patterns )
/cex 3 10 -1
/cer 1
```
**UDP Output:**
```text
/seq/event 12845920 0 4
```

---

## 5. Parameter Sweeps and Variables (`%g`)
The `>u` word acts as a string formatter (like `sprintf`). It reads arguments off the stack right-to-left. You can use it to broadcast continuous data, variables, or math results.

**Skode Input:**
```skode
( Read the 'I' iterator and divide by 100 for a dynamic sweep )
[ /sweep/val %g ] I 100 / >u

( Read from a shared variable 'v0' )
[ /status/v0 %g ] v0 >u

( Multi-argument event: Note and Velocity )
[ /voice/play %d %g %g ] 1 60.0 0.8 >u
```
**UDP Output:**
```text
/sweep/val 0.5
/status/v0 3.14
/voice/play 1 60 0.8
```

> [!IMPORTANT]
> The `>u` string parser currently supports up to 8 arguments formatted with `%g` (for floats/doubles) and `%d` (for values cast to integers). Ensure your stack values align with your format string!

---

## 6. Testing with Netcat (Quickstart)
Skred uses a **"ping-to-subscribe"** architecture for UDP events. External clients must send a packet to the events port first so the engine knows where to send broadcasts. If a client goes silent for 10 seconds, it is automatically unsubscribed to prevent spamming dead ports.

Here is a step-by-step example for testing the event system locally using `netcat` (`nc`) and `mini-skred`:

**Step 1: Start Skred with the Events Port**
Open your first terminal and start `mini-skred`, using the `-e` flag to specify the UDP events port (e.g., `60441`).
```bash
parts/build_maxed/mini-skred -e 60441
```

**Step 2: Start Netcat and Subscribe**
Open a second terminal window and run `netcat` in UDP mode pointing to that port:
```bash
nc -u 127.0.0.1 60441
```
*(Once netcat is running, type **`SUB`** and press **Enter** to send the ping. Netcat will stay open listening for broadcasts).*

**Step 3: Trigger an ADSR Release Event**
Go back to your first terminal running `mini-skred`. We will bind a UDP broadcast to an envelope release event, and then trigger it:

```skode
( 1. Save our broadcast macro )
[ [ /voice/release %d ] 5 >u ] e>0

( 2. Enable control events for Voice 5 and bind the macro to Envelope Release )
v5 vc1
/cex 0 2 5

( 3. Start the event dispatcher )
/cer 1

( 4. Trigger Voice 5's envelope, then release it! )
v5 l1
l0
```

**Step 4: See the Output**
The moment you send `l0`, the voice enters its ADSR release phase. Look at your second terminal running `netcat`. You will immediately see the broadcast arrive:
```text
/voice/release 5
```
