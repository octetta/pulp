# Skred UDP Events Reference

The `>u` word allows Skred to broadcast custom-formatted ASCII strings to any subscribed UDP clients (like visualizers, external game engines, or OSC bridges). 

Because Skred evaluates the string exactly as you format it, **you** define the "kinds" of events. The `>u` word acts as a string formatter (similar to `sprintf` in C) that pulls arguments directly off the Skode stack.

> [!NOTE]
> **Syntax Rules:**
> - Push a string to the stack using `[ ... ]`
> - Provide the arguments (up to 8) matching your format specifiers.
> - Call `>u` to execute the string formatting and broadcast the UDP packet.
> - Supported formatters are `%g` (for floats/doubles) and `%d` (for integers).

---

## 1. Simple Triggers (No Arguments)
Useful for basic clock ticks, resets, or transport controls where the string itself is the entire message.

**Skode Input:**
```skode
[ /transport/play ] >u
[ /clock/tick ] >u
```
**UDP Output:**
```text
/transport/play
/clock/tick
```

---

## 2. Integer Parameters (`%d`)
Useful for discrete state changes like steps in a sequencer, pattern changes, or voice indexes. Skred will automatically cast the stack value to an integer.

**Skode Input:**
```skode
[ /seq/step %d ] 4 >u
[ /ui/scene/change %d ] 12 >u
```
**UDP Output:**
```text
/seq/step 4
/ui/scene/change 12
```

---

## 3. Float/Double Parameters (`%g`)
The most common type of event in Skred. Useful for continuous parameters like frequencies, velocities, or modulations.

**Skode Input:**
```skode
[ /filter/cutoff %g ] 440.5 >u
[ /lfo/rate %g ] 0.125 >u
```
**UDP Output:**
```text
/filter/cutoff 440.5
/lfo/rate 0.125
```

---

## 4. Multi-Argument Events
You can combine up to 8 arguments in a single event. The arguments are pulled from the stack in the order they appear in the string.

**Skode Input:**
```skode
[ /voice/play %d %g %g ] 1 60.0 0.8 >u
```
*(Wait, let's look at the stack order. Skode reads left to right, so `1 60.0 0.8` places `0.8` at the top of the stack. However, for `>u`, arguments are processed sequentially from the stack bottom-to-top relative to the command, matching intuitive reading order.)*

**UDP Output:**
```text
/voice/play 1 60 0.8
```

---

## 5. Dynamic & Calculated Events
Because the arguments are just standard Skode stack values, they can be the result of math, variables, or iterators.

**Skode Input:**
```skode
( Read the 'I' iterator and divide by 100 for a dynamic sweep )
[ /sweep/val %g ] I 100 / >u

( Read from variable 'v0' )
[ /status/v0 %g ] v0 >u
```
**UDP Output:** (Assuming `I` was 50 and `v0` was 3.14)
```text
/sweep/val 0.5
/status/v0 3.14
```

---

## 6. Real-World Sequencer Example
Here is how you might integrate UDP events directly into a Skode sequence pattern so that a visualizer perfectly syncs with your drum track:

```skode
( Pattern 0, Step 0: Play a kick on Voice 0, emit UDP event )
[ /drum/kick %g ] 1.0 >u v0 f60 a1

( Pattern 0, Step 4: Play a snare on Voice 1, emit UDP event )
[ /drum/snare %g ] 0.8 >u v1 f200 a1
```

> [!TIP]
> If you are building a visualizer in Python, Unity, or TouchDesigner, you can parse these incoming strings simply by splitting them on the space character: `parts = data.split(" ")` where `parts[0]` is the address (e.g. `"/drum/kick"`) and `parts[1]` is the value.
