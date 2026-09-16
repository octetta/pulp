# Advanced Skred UDP Events Reference

The `>u` word broadcasts formatted ASCII strings to any subscribed UDP clients (like visualizers or external engines). While you can use `>u` manually, the real power comes from coupling it with Skred's **Control Event Dispatcher** (`/ce`, `/ceb`). 

This allows you to automatically broadcast UDP events exactly when samples finish, envelopes release, or sequencer patterns loop—with sample-accurate precision.

---

## 1. Voice Lifecycle: Envelope & Sample Endings
You can track when an ADSR envelope enters its release phase (e.g. from `l0` or exiting a loop region) or when a one-shot audio sample completely finishes playing.

Because `>u` is an immediate word, we first save our format string to an **External Macro** (`e>N`). The `>u` word can then intelligently read that macro index off the stack and use it as its format string!

**Skode Input:**
```skode
( 1. Store our format strings in macro slots 0 and 1 )
[ /voice/finished %d ] e>0
[ /voice/release %d ] e>1

( 2. Select voice 5 and enable its lifecycle control events )
v5 vc1

( 3. Bind Voice Finished (type 3) to execute '5 0 >u' )
[ 5 0 >u ] /ceb 3 5

( 4. Bind Envelope Release (type 2) to execute '5 1 >u' )
[ 5 1 >u ] /ceb 2 5

( 5. Start the control event dispatcher thread )
/cer 1
```

Now, whenever Voice 5 is triggered and subsequently finishes playing its wave data, or its envelope is released, a UDP packet like `/voice/finished 5` will be automatically broadcast!

---

## 2. Sequencer Sync: Pattern Loop Points
You can trigger UDP broadcasts exactly when a sequencer pattern loops (starts or ends). This guarantees perfect visual synchronization with your generative patterns.

**Skode Input:**
```skode
( 1. Store the format string )
[ /seq/loop %d ] e>0

( 2. Enable control events for pattern 0 )
y0 yc1

( 3. Bind Pattern Start (type 5) to use macro 0 for >u )
[ 0 0 >u ] /ceb 5 0

( 4. Ensure the dispatcher is running )
/cer 1
```
Whenever Pattern 0 wraps around to step 0, it emits the `SKRED_CONTROL_EVENT_PATTERN_START` event, executing `0 0 >u` and sending `/seq/loop 0` over UDP.

> [!TIP]
> **Pattern Control Event Types:**
> - `5`: Pattern Start (Downbeat / Loop point)
> - `6`: Pattern End
> - `9`: Pattern Step (Fires on every active step)
> - `10`: Pattern Change (Fires when a sequence jumps to a new pattern)

---

## 3. Direct Step Triggers (Inside Patterns)
Because `>u` is an immediate word (it interacts with the UDP subsystem), it **cannot** be compiled directly into a real-time pattern sequence. 

Instead, use Skred's decoupled architecture: embed a user control event (`ce <id>`) into your pattern, and bind that ID to a string using `/ceb`!

**Skode Input:**
```skode
( Step 0: Play a kick on Voice 0 AND emit user event 42 )
[ v0 f60 a1 ce42 ] x0

( Step 4: Play a snare on Voice 1 AND emit user event 43 )
[ v1 f200 a1 ce43 ] x4

( Store our UDP format strings )
[ /drum/kick %d ] e>0
[ /drum/snare %d ] e>1

( Bind our execution strings to those user events )
( Type 4 is SKRED_CONTROL_EVENT_USER )
[ 1 0 >u ] /ceb 4 42
[ 1 1 >u ] /ceb 4 43

( Start the dispatcher )
/cer 1
```

---

## 4. Including Event Metadata (Time, Voice, Pattern)
When your event executes, the dispatcher guarantees that the parser context knows exactly which voice, pattern, and step triggered the event. 

You can use the `?t`, `?v`, `?p`, and `?st` words to explicitly push these values onto the stack to embed them into your UDP strings! This gives your UDP clients sample-accurate timestamps.

**Skode Input:**
```skode
( Broadcast event with timestamp and source info! )
( Stack order: ?p pushes first, ?t pushes last, 0 is the macro ID )
[ /seq/event %g %d %d ] e>0
[ ?p ?st ?t 0 >u ] /ceb 10 -1

( Start the dispatcher )
/cer 1
```
**UDP Output:**
```text
/seq/event 12845920 0 4
```

---

## 5. Dynamic Variables and Multi-Argument Formats
The `>u` word acts as a string formatter (like `sprintf`). It processes numerical arguments left-to-right before consuming the final argument as the macro ID. You can use it to broadcast Skode variables (like `$0`) or emit multiple data points at once.

**Skode Input:**
```skode
( Broadcast a dynamic value stored in register 0 )
=0,42.5
[ /sensor/temp %g ] e>0
$0 0 >u

( Multi-argument event: Voice, Note, and Velocity )
[ /voice/play %d %g %g ] e>1
1 60.0 0.8 1 >u
```
**UDP Output:**
```text
/sensor/temp 42.5
/voice/play 1 60 0.8
```

> [!IMPORTANT]
> The `>u` string parser currently supports up to 8 arguments formatted with `%g` (for floats/doubles) and `%d` (for values cast to integers). Ensure your stack values align with your format string!

---

## 6. Testing with Netcat (Quickstart)
Skred uses a **"ping-to-subscribe"** architecture for UDP events. External clients must send a packet to the events port first so the engine knows where to send broadcasts. If a client goes silent for 10 seconds, it is automatically unsubscribed to prevent spamming dead ports.

Here is a step-by-step example for testing the event system locally using `netcat` (`nc`) and `mini-skred`:

**Step 1: Start Skred with the Events Port**
Open your first terminal and start `mini-skred`, using the `-e` flag to specify the UDP events port (e.g., `60441`). Note that `mini-skred` requires the flag to have no space:
```bash
parts/build_maxed/mini-skred -e60441
```

**Step 2: Start Netcat and Subscribe**
Open a second terminal window and run `netcat` in UDP mode pointing to that port:
```bash
nc -u 127.0.0.1 60441
```
*(Once netcat is running, type **`SUB`** and press **Enter** to send the ping. Netcat will stay open listening for broadcasts).*

**Step 3: Trigger a User Event**
Go back to your first terminal running `mini-skred`. Because `mini-skred` does not run the audio thread (meaning envelopes won't advance and trigger audio lifecycle events), we will test the system using a User Control Event (type 4):

```skode
( 1. Save our format string )
[ /test/user %d ] e>0

( 2. Bind >u to User Event 42 )
[ 42 0 >u ] /ceb 4 42

( 3. Start the event dispatcher )
/cer 1

( 4. Trigger the user event directly! )
ce 42
```

**Step 4: See the Output**
The moment you send `ce 42`, the control event dispatcher processes it. Look at your second terminal running `netcat`. You will immediately see the broadcast arrive:
```text
/test/user 42
```
