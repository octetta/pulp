# Pulp/Skode MIDI Quick Reference

This is a quick guide to connecting MIDI devices, routing them to the synthesizer, and dynamically binding MIDI events to Skode commands.

## 1. Connecting MIDI Devices

Pulp handles MIDI via internal endpoint commands. You can either connect Pulp to an existing device, or have Pulp create a virtual port for other software (like VMPK or your DAW) to connect to.

### Listing Devices
```skode
/mL
```
*Lists all available MIDI inputs and outputs along with their ID numbers.*

### Opening a Device
Once you know the ID from `/mL` (e.g., `3`), you can open it:
```skode
/mi 3   # Open MIDI Input 3
/mo 3   # Open MIDI Output 3
```

### Creating Virtual Ports (Recommended for VMPK/DAWs)
Instead of relying on hardware IDs, Pulp can create its own virtual ports:
```skode
[] /miV       # Creates a virtual MIDI input named "pulp"
[vmpk] /miV   # Creates a virtual MIDI input named "vmpk"
```
*(Note: Since the name is a string, it must precede the command.)*
*After running this, open your MIDI software and connect its output to the newly created "pulp" port.*

---

## 2. Basic Voice & Pool Routing

Once MIDI is flowing in, you need to tell Pulp where the notes should go.

### Routing to a Monophonic Voice
To route a specific MIDI channel directly to a single synthesizer voice:
```skode
/mv <channel>,<voice>
```
*Example: `/mv 0,0` (Routes MIDI Channel 0 to Voice 0)*

### Routing to a Polyphonic Pool
To play chords, route a MIDI channel to a polyphony pool instead:
```skode
/mp <channel>,<pool>
```
*Example: `/mp 0,0` (Routes MIDI Channel 0 to Poly Pool 0)*

---

## 3. Dynamic MIDI Bindings (`/mb`)

If you want MIDI to trigger Skode commands (like macros) instead of just playing synth notes, use the `/mb` (MIDI Bind) command.

**Syntax:**
```skode
[skode-command] /mb <type>,<channel>,<data1>
```

### Event Types
- **9**: Note On
- **8**: Note Off
- **11**: Control Change (CC)
- **14**: Pitch Bend

### Wildcards
If you use `-1` for the `channel` or `data1` (Note/CC number), Pulp treats it as a wildcard ("match any").

### Placeholders
Inside your `[skode-command]` bracket, you can use these dynamic placeholders to inject incoming MIDI data directly into your command:
- `{ch}`: The MIDI Channel (0-15)
- `{d1}`: Data Byte 1 (The Note Number, CC Number, etc.)
- `{d2}`: Data Byte 2 (Velocity, CC Value, etc.)
- `{unit}`: Data Byte 2 scaled to a `0.0` to `1.0` float (Great for parameters!)
- `{bend}`: Pitch bend mapped to a clean float.

### Binding Examples

**Trigger specific macros based on the exact key pressed:**
```skode
[e!{d1}] /mb 9,0,-1
```
*(When Note On (9) arrives on Channel 0, for any note (-1), execute `e!` appended with the Note Number `{d1}`. Pressing MIDI key 69 executes `e!69`.)*

**Map a CC Knob to a Global Variable:**
```skode
[{unit} >v0] /mb 11,-1,74
```
*(When a CC (11) arrives on any channel (-1), for CC number 74, take the `{unit}` float value and store it in variable `v0`.)*

**Map Note Off to stop a macro:**
```skode
[e!0] /mb 8,0,69
```
*(When Note Off (8) arrives on Channel 0 for Note 69, execute `e!0`.)*

---

## 4. Managing Bindings

**View Active Bindings:**
```skode
/mb?
```

**Remove a Specific Binding:**
```skode
/mbd <type>,<channel>,<data1>
```
*Example: `/mbd 9,0,-1`*

**Clear All Bindings:**
```skode
/mbC
```
