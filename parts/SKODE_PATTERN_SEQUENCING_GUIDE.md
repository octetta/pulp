# Master Guide to Pattern Sequencing & Song Building in SKRED

Welcome! If you've ever used a classic hardware drum machine (like a Roland TR-808, TR-909, LinnDrum, Akai MPC, or Elektron Digitakt), you'll feel right at home with SKRED's pattern sequencer.

This guide breaks down how SKRED handles pattern creation, live performance jamming, and full song automation in a clear, conversational way with copy-pasteable examples.

---

## 1. The Core Mental Model

In SKRED, your music is structured into three layers:

```
┌─────────────────────────────────────────────────────────┐
│                     1. Master Clock                     │
│               M <BPM> [subdivision] (e.g. M 120)        │
└────────────────────────────┬────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────┐
│                  2. Dedicated Voices                    │
│   v0: Kick  |  v1: Snare  |  v2: Hat  |  v3: Acid Bass  │
└────────────────────────────┬────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────┐
│                 3. Pattern Slots (y0..y127)             │
│   y0: Main Beat  |  y1: Verse  |  y2: Chorus  | y3: Fill│
└────────────────────────────┬────────────────────────────┘
```

- **Master Clock (`M <BPM>`)**: Defines the global tempo and step clock. `M 120` runs at 120 BPM with 16th-note steps ($125\text{ ms}$ per step, $2.0\text{s}$ per 16-step bar).
- **Voices (`v0`..`v4`)**: Individual instruments. Drum voices (`v0`..`v3`) run at natural unpitched sample rate (`f440` / `n69`), while synth voices (`v4`) accept pitch note numbers (`n36`, `n48`).
- **Pattern Bank (`y0`..`y127`)**: 128 pattern slots, each holding up to 128 steps (`x0`..`x127`).

---

## 2. Building 16-Step Drum Machine Patterns

Let's write a classic TR-909 peak-time techno beat in Pattern 0 (`y0`):

```skode
# 1. Set Master Tempo (130 BPM, 16th-note steps)
M 130

# 2. Load Drum & Synth Wave Slots:
[sk/drums-kick.ks]/ks 1 k>d d>r /r100
[sk/drums-snare.ks]/ks 1 k>d d>r /r101
[sk/drums-chh.ks]/ks 1 k>d d>r /r102
[sk/drums-ohh.ks]/ks 1 k>d d>r /r103
[sk/nap-bass-acid.ks]/ks 1 k>d d>r /r104

# 3. Assign Dedicated Voices:
v0 w100 f440 a10          # Kick (v0 auto-chokes sub-bass tails on re-trigger)
v1 w101 f440 a8           # Snare
v2 w102 f440 a3           # Closed Hi-Hat
v3 w103 f440 a5           # Open Hi-Hat
v4 w104 f440 a6           # 303 Acid Synth

# 4. Program Pattern 0 (16-Step Bar)
y0
x0 [v0l1 v4n36l1]         # Kick + Acid Bass C2 (Beat 1)
x1 [v2l0.4]               # Closed Hat
x2 [v3l0.8]               # Offbeat Open Hat Accent
x3 [v2l0.4 v4n48l0.6]     # Closed Hat + Acid Bass C3
x4 [v0l1 v1l1]            # Kick + Snare Backbeat (Beat 2)
x5 [v2l0.4]
x6 [v3l0.8 v4n41l0.6]     # Offbeat Open Hat + Acid Bass F2
x7 [v2l0.4]
x8 [v0l1 v4n36l1]         # Kick + Acid Bass C2 (Beat 3)
x9 [v2l0.4]
x10 [v3l0.8 v4n43l0.6]    # Offbeat Open Hat + Acid Bass G2
x11 [v2l0.4]
x12 [v0l1 v1l1]           # Kick + Snare Backbeat (Beat 4)
x13 [v2l0.4]
x14 [v1l0.35 v3l0.8]      # Ghost Snare + Open Hat
x15 [v1l0.8 z*4]          # 32nd-Note Snare Ratchet Fill into Beat 1!

# 5. Start Pattern 0
Z1
```

> [!TIP]
> **Why `v0` Kick stays punchy**: Re-triggering `v0l1` automatically resets voice 0's envelope and sample playback position to offset 0, naturally choking previous sub-bass tails so kicks never get muddy!

---

## 3. Live Jamming & Quantized Downbeat Queuing (`zq`)

When jamming live, you don't want patterns to switch mid-beat (which sounds glitchy). You want new patterns to enter cleanly on the **downbeat** (beat 1 of the next bar).

SKRED handles this with **Downbeat Quantized Queuing (`zq`)**:

```skode
# Scenario: Pattern 0 is playing (y0 z1).
# You want to transition to Pattern 1 on the next bar:

y1 zq1                     # Queue Pattern 1 to start on the next downbeat
```

### What happens under the hood:
1. Pattern 0 finishes playing steps `x0` through `x15`.
2. On step 0 of the next bar (`seq_pointer[0] == 0`), Pattern 1 automatically launches in perfect phase sync!
3. To queue a pattern to stop on the next downbeat: `y0 zq0`.

### Muting & Unmuting Live (`ym`):
To drop instruments in and out without switching patterns:

```skode
y1 ym1                     # Mute Pattern 1 (retains playback position)
y1 ym0                     # Unmute Pattern 1 (instantly drops back in)
```

---

## 4. Advanced Drum Machine Tricks

### A. Step Ratchets & Micro-Rolls (`z*`)
Subdivide any step into rapid micro-triggers for trap hats, triplet fills, or IDM rolls:

- `z*2`: Double-tap fill
- `z*3`: Triplet roll (3 triggers per step)
- `z*4`: 32nd-note ratchet burst (4 triggers per step)
- `z*8`: 64th-note ultra-fast roll (8 triggers per step)

```skode
x3 [v2l0.4 z*3]            # Triplet hi-hat roll on step 3
x15 [v1l0.9 z*8]           # 64th-note snare roll turnaround on step 15
```

### B. Dynamic Speed Changes (`z%`)
Change step clock division mid-pattern for halftime breakdowns or double-time builds:

- `z%16`: Standard 16th-note steps
- `z%8`: Halftime 8th-note steps (twice as long per step)
- `z%32`: Double-time 32nd-note steps (twice as fast per step)

```skode
x0 [v0l1 z%16]             # Standard 16th notes
x4 [v1l1 z%8]              # Halftime breakdown on step 4
x6 [v0l1 z%32]             # Double-time build on step 6
```

### C. Lockstep Polymetric Fills (`-0`)
Create a 3-step drum fill in Pattern 1 that plays once and then pauses until Pattern 0 rolls back to step 0:

```skode
# Pattern 1 (Polymetric Fill):
y1
x0 [v1l1]                  # Snare hit
x1 [v1l0.8 z*4]            # Snare ratchet fill
x2 -0                      # Hold here until Pattern 0 reaches Step 0!
```

---

## 5. Full Song Construction (Macros + Automation)

To build a complete, hands-off automated song in SKRED:

1. **Define Patterns**: Set up `y0` (Main Beat), `y1` (Verse), `y2` (Chorus), `y3` (Fill).
2. **Create Section Macros**: Bundle pattern state switches into four-letter macros (`[...]`).
3. **Sequence Timeline**: Use `wait <ms>` (at 120 BPM, 1 bar = $2000\text{ ms}$).

### Complete Example Song Script

```skode
# =============================================================
# Complete Automated Song Script in SKRED
# Tempo: 120 BPM (1 Bar = 2000ms)
# =============================================================
M 120

# 1. Define Song Section Macros:
[sec_intro]:   y0 z1,  y1 z0,  y2 z0;         # Intro: Main Beat (y0)
[sec_verse]:   y0 z1,  y1 zq1, y2 z0;         # Verse: Add Verse Variation (y1)
[sec_chorus]:  y0 z0,  y1 z0,  y2 zq1;        # Chorus: Peak Chorus Beat (y2)
[sec_outro]:   y0 zq1, y1 z0,  y2 z0;         # Outro: Return to Main Beat

# 2. Execute Automated Song Sequence:

[sec_intro]                                   # Launch Intro
wait 8000                                     # Play Intro for 4 bars (8.0s)

[sec_verse]                                   # Queue Verse on downbeat
wait 8000                                     # Play Verse for 4 bars

[sec_chorus]                                  # Queue Chorus on downbeat
wait 16000                                    # Play Chorus for 8 bars (16.0s)

[sec_outro]                                   # Queue Outro on downbeat
wait 8000                                     # Play Outro for 4 bars

Z0                                            # Stop all patterns at end of song
```

---

## 6. Quick Command Reference

| Goal | Command | Example |
| --- | --- | --- |
| Set Master Tempo | `M <BPM> [subdivision]` | `M 120` (16th notes) or `M 120 8` (8th notes) |
| Select Pattern | `y <pattern>` | `y0`, `y1`, `y2` |
| Set Step Content | `x <step> [<commands>]` | `x0 [v0l1 v4n36l1]` |
| Step Ratchet | `z* <count>` | `z*3` (triplet), `z*4` (32nd), `z*8` (64th) |
| Speed Division | `z% <modulo>` | `z%16` (16th notes), `z%8` (8th notes) |
| Start Pattern | `y<N> z1` or `Z1` | `y0 z1` (start Pattern 0), `Z1` (start all) |
| Quantized Downbeat Queue | `y<N> zq1` / `y<N> zq0` | `y1 zq1` (queue Pattern 1 to start on next bar) |
| Jump Playhead | `zg <step>` | `zg4` (jump to step 4) |
| Pattern Mute | `y<N> ym1` / `y<N> ym0` | `y1 ym1` (mute), `y1 ym0` (unmute) |
| Cross-Pattern Wait Step | `[-N]` | `x2 -0` (wait at step 2 until Pattern 0 hits step 0) |

Happy sequencing! 🎧🥁
