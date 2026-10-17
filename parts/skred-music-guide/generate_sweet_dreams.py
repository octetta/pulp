#!/usr/bin/env python3
"""
generate_sweet_dreams.py - Generates sweet_dreams_full.sk and sweet_dreams.sk for PULP / SKRED

Full Arrangement Instrumental / Synth-Pop Cover of Eurythmics' 1983 classic:
"Sweet Dreams (Are Made of This)" by Annie Lennox & Dave Stewart.

Features:
- The iconic 2-bar C-minor bass riff on SH-101 / Juno (w15 saw + w16 detuned square)
- The left-panned plucky counter-riff (w16 pulse with phase distortion c4)
- Dave Stewart's famous "milk bottle crack" percussion + sub boom impact (w87 crack, w5 glass shatter, w0 sub drop, w82 thud)
- Annie Lennox's singing vocal leads:
    * Chorus Hook: "Sweet dreams are made of this..." (w15/w16 stereo trio)
    * Verse Hook: "Some of them want to use you..."
    * Gospel Bridge: "Hold your head up, movin' on..."
- Screaming analog portamento synth solo (w15 with 70ms pitch glide)
- Warm 1980s analog synth pad chords (Cm -> Ab -> G) replacing sluggish poly pools
- Tight TR-808 electronic drums: 4-on-the-floor kick, sub punch, gated snare, claps, running 16th hats
- Delay freeze breakdown (DF 1 1) with K-synth riser (w301) and cutoff sweeps
- 96-bar arrangement sequenced via Pattern 0 Conductor at %16
- All voices clearly tagged and inspected via [name] vt
"""

import os, sys, math

def build_sweet_dreams():
    lines = []
    
    def emit(s=""):
        lines.append(s)

    # ----------------------------------------------------
    # Header & Song Metadata
    # ----------------------------------------------------
    emit("## EURYTHMICS - SWEET DREAMS (ARE MADE OF THIS)")
    emit("#  Full Arrangement Cover for PULP / SKRED. 125 BPM, C minor, 96 bars (~3:04).")
    emit("#  Written & Produced by Annie Lennox & Dave Stewart (1983).")
    emit("#  Self-contained: drums from built-in waves 82-99, inline K-synth riser.")
    emit("#  Uses n>d to convert note name strings to MIDI note streams.")
    emit("#  Load:   [sweet_dreams_full.sk] /ls")
    emit("#  Start:  play        Stop:  stop")
    emit("#")
    emit("#  VOICE MAP (all labeled via [name] vt):")
    emit("#    v0  four-on-the-floor kick (w82) + v5 sub punch (w0)")
    emit("#    v1  gated 80s snare (w83) + v4 electronic clap (w86)")
    emit("#    v2  closed hat 16ths (w84)  v3  open hat offbeat (w85)")
    emit("#    v6  main bass riff saw centre (w15) + v7 partner square (w16, right)")
    emit("#    v8  Annie Lennox vocal lead centre (w15) + v9 (left w15), v10 (right w16)")
    emit("#    v11 analog synth solo centre (w15) + v12 partner (w16)")
    emit("#    v13 counter-riff plucky square left (w16)")
    emit("#    v16 K-synth build riser (w301)")
    emit("#    v17 crash cymbal (w94)")
    emit("#    v18 hi tom (w90)  v19 mid tom (w91)  v20 lo tom (w92) = Simmons toms")
    emit("#    v21 bottle crack (w87) + v22 glass shatter (w5)")
    emit("#    v38 sub boom sine (w0) + v40 slowed kick thud (w82 f110)")
    emit("#    v26 synth pad root (w15) + v27 3rd (w15 left), v28 5th (w16 right)")
    emit("#")
    emit("#  DELAY BUSES:")
    emit("#    Bus 1: Annie vocal lead space echo (DS 1 125 .75, dotted eighth)")
    emit("#    Bus 2: Main bass riff 16th slapback (DS 2 125 .25)")
    emit("#    Bus 3: Bottle crack & sub boom reverb tail (DS 3 125 .75)")
    emit("#    Bus 4: Synth pad & riser stereo wash (DS 4 125 1, DP 4 1)")
    emit("#")
    emit("#  ARRANGEMENT (96 bars):")
    emit("#    0-3   Intro: Lone bass riff + milk bottle crack & sub drop")
    emit("#    4-7   16th hats enter, filter opens, counter-riff in left channel")
    emit("#    8-15  Groove drops: 4-on-floor kick, claps, snare, synth pad swells")
    emit("#    16-23 Verse 1: Annie Lennox vocal ('Some of them want to use you...')")
    emit("#    24-39 Chorus 1: 'Sweet dreams are made of this...', full energy wall of sound")
    emit("#    40-47 Bridge: 'Hold your head up, movin' on...' with swelling gospel chords")
    emit("#    48-55 Analog Synth Solo: screaming pitch bends and portamento slides")
    emit("#    56-63 Breakdown: Drums cut, delay freezes (DF 1 1), K-synth riser build")
    emit("#    64-79 Chorus 2 & Climax: Maximum power, high vocal ad-libs, bottle cracks")
    emit("#    80-87 Verse 2 / Bridge reprise")
    emit("#    88-95 Outro: Riff vamp, stereo counter-riff, hats half-time, final delay freeze")
    emit("#    96    End cleanup")
    emit("# ======================================================================")
    emit()
    emit("[SWEET DREAMS - EURYTHMICS] puts")
    emit("M 125 16")
    emit()

    # ----------------------------------------------------
    # Inline K-Synth Riser (w301)
    # ----------------------------------------------------
    emit("## INLINE K-SYNTH RISER (one-shot wave 301)")
    emit("[")
    emit("N: 65536")
    emit("T: !N")
    emit("E: (e(T*(4.2%N)))%100")
    emit("R: r T")
    emit("L: 0.35 f R")
    emit("H: R-L")
    emit("F: 800*(e(T*(2.6%N)))*((p 2)%(p 0))")
    emit("P: +\\F")
    emit("S: s P")
    emit("W: w E*((S*.35)+(H*.55))")
    emit("] k!")
    emit("k>w301,44100,1")
    emit()

    # ----------------------------------------------------
    # Delay Buses (Tempo Synced at 125 BPM)
    # ----------------------------------------------------
    emit("## DELAY BUSES")
    emit("# Bus 1: Annie Lennox vocal lead & solo space echo (dotted eighth)")
    emit("DS 1 125 .75")
    emit("DL 1 - - 8 2 3 11")
    emit("DD 1 7 2")
    emit("DP 1 1")
    emit()
    emit("# Bus 2: Main bass riff 16th slapback (adds thickness & punch)")
    emit("DS 2 125 .25")
    emit("DL 2 - - 4 0 0 8")
    emit("DD 2 7 3")
    emit()
    emit("# Bus 3: Bottle crack & sub boom reverb tail")
    emit("DS 3 125 .75")
    emit("DL 3 - - 8 0 0 10")
    emit("DD 3 5 4")
    emit()
    emit("# Bus 4: Synth pad & riser stereo hall wash")
    emit("DS 4 125 1")
    emit("DL 4 - - 10 3 6 11")
    emit("DD 4 8 2")
    emit("DP 4 1")
    emit()

    # ----------------------------------------------------
    # Voices Setup
    # ----------------------------------------------------
    emit("## VOICES")
    emit("# Drums: punchy four-on-the-floor kick + sub punch")
    emit("v0 w82 f440 a7 p0 [kick electronic] vt")
    emit("v5 w0 a-2 p0 t.001,.22,0,.06 j1 k160 q.7 [kick sub punch] vt")
    emit()
    emit("# Crisp 80s gated snare, claps, hats, crash")
    emit("v1 w83 f440 a8 p0 r3 ds.20 [snare 80s gated] vt")
    emit("v2 w84 f440 a-3 p.25 [closed hat] vt")
    emit("v3 w85 f440 a-2 p-.25 [open hat] vt")
    emit("v4 w86 f440 a7 p0 [electronic clap] vt")
    emit("v17 w94 f440 a4 p.2 [crash cymbal] vt")
    emit()
    emit("# Simmons Electronic Toms (hi, mid, lo)")
    emit("v18 w90 f480 a6 p-.3 [hi simmons tom] vt")
    emit("v19 w91 f380 a6 p0 [mid simmons tom] vt")
    emit("v20 w92 f280 a6 p.3 [lo simmons tom] vt")
    emit()
    emit("# The Iconic Sweet Dreams Bass Riff: Roland SH-101 / Juno saw + square")
    emit("# Sends ds.10 to Bus 2 for subtle slapback width")
    emit("v6 w15 a-1 p0 t.001,.16,0,.04 j1,2 k1100 q3.2 g0 r2 ds.10 [sweet dreams bass saw] vt")
    emit("v6 >7")
    emit("v7 w16 a-5 N0,9 p.45 r0 [sweet dreams bass square] vt")
    emit("v6 G7 H7")
    emit()
    emit("# Left-Panned Counter-Riff (octave up, plucky square with phase distortion c4)")
    emit("v13 w16 a-7 p-.55 t.002,.15,0,.04 j1,2 k2400 q1.8 c4,.2 r1 ds.25 [counter riff left] vt")
    emit()
    emit("# Annie Lennox Vocal Lead: rich, soulful, expressive singing synth with portamento glide")
    emit("# Soft 50ms attack and long 1.6s decay with safe s=0 prevents any stuck notes")
    emit("v8 w15 a-5 p0 t.05,1.5,0,.35 j1 k2500 q1.2 g.08 c4,.20 r1 ds.35 [annie vocal lead] vt")
    emit("v8 >9")
    emit("v8 >10")
    emit("v9 w15 a-7 N0,9 p-.5 r0 [annie vocal left] vt")
    emit("v10 w16 a-8 N0,-9 p.5 r0 [annie vocal right] vt")
    emit("v8 G9,10 H9,10")
    emit()
    emit("# Analog Pitch-Bend Synth Solo: resonant singing saw with 70ms portamento glide")
    emit("v11 w15 a-5 p0 t.003,.35,0,.10 j1,2 k3400 q3.2 g.07 c4,.25 r1 ds.35 [analog synth solo] vt")
    emit("v11 >12")
    emit("v12 w16 a-8 N0,12 p.45 r0 [solo partner] vt")
    emit("v11 G12 H12")
    emit()
    emit("# Dave Stewart's Famous Milk Bottle Crack & Sub Boom Impact")
    emit("v21 w87 f520 a8 p-.2 [bottle crack] vt")
    emit("v22 w5 f1800 a-4 p.2 j2 k1800 q2 t.001,.08,0,.03 [bottle glass shatter] vt")
    emit("v38 w0 a2 p0 t.002,.80,0,.30 j1 k220 q.7 r3 ds.3 [sub boom drop] vt")
    emit("v40 w82 f110 a5 p0 [sub thud layer] vt")
    emit()
    emit("# Riser: stretched K-synth one-shot")
    emit("v16 w301 f187 a8 p0 t.005,.01,1,.4 r4 ds.3 [ksynth riser] vt")
    emit()
    emit("# Warm Analog Synth Pad Chords (Cm -> Ab -> G, delay Bus 4)")
    emit("v26 w15 a-8 p0 t.08,.60,.3,.35 j1 k2000 q1.0 c4,.15 r4 ds.3 [synth pad root] vt")
    emit("v27 w15 a-9 N0,8 p-.5 r0 [synth pad 3rd] vt")
    emit("v28 w16 a-9 N0,-8 p.5 r0 [synth pad 5th] vt")
    emit("v26 G27,28 H27,28")
    emit()

    # ----------------------------------------------------
    # Events
    # ----------------------------------------------------
    emit("## EVENTS")
    emit("[O DQ] /ceb 4 99")
    emit("/cer 1")
    emit()

    # ----------------------------------------------------
    # Streams (Defined using [ ... ] n>d)
    # ----------------------------------------------------
    emit("## STREAMS (Note arrays converted via n>d)")
    emit("# Stream 1: The Iconic 2-Bar Sweet Dreams Bass Riff (16 eighth notes / 32 sixteenth steps)")
    emit("# Bar 1 (Cm): C2 C2 C2 C2  Eb2 Eb2 C2 C2")
    emit("# Bar 2 (Ab/G): Ab1 Ab1 Ab1 C2  G1 G1 G1 C2")
    emit("[C2 C2 C2 C2  Eb2 Eb2 C2 C2  Ab1 Ab1 Ab1 C2  G1 G1 G1 C2] n>d /SS 1")
    emit()
    emit("# Stream 2: Left-Panned Counter-Riff (12 notes / 32 steps)")
    emit("[C3 C3 C3 C3  Eb3 Eb3 C3 C3  Ab2 Ab2 Ab2 C3  G2 G2 G2 C3] n>d /SS 2")
    emit()

    # ----------------------------------------------------
    # Patterns
    # ----------------------------------------------------
    emit("## PATTERNS")
    emit()

    # Pattern 1: Four-on-the-floor Electronic Kick
    emit("y1")
    emit("[kick 4-on-the-floor] yt")
    emit("[v0 l1 v5 g0 n50 g.06 n28 l1] x0")
    emit("[v0 l.95 v5 g0 n48 g.06 n28 l.9] x4")
    emit("[v0 l1 v5 g0 n50 g.06 n28 l1] x8")
    emit("[v0 l.95 v5 g0 n48 g.06 n28 l.9] x12")
    emit("[=127,0] x15")
    emit()

    # Pattern 2: Gated Snare + Electronic Clap on 2 & 4
    emit("y2")
    emit("[snare+clap on 2 & 4] yt")
    emit("[v1 l1 v4 l.95] x4")
    emit("[v1 l.25] x6")
    emit("[v1 l.20] x11")
    emit("[v1 l1 v4 l.95] x12")
    emit("[v1 l.30] x14")
    emit("[=127,0] x15")
    emit()

    # Pattern 3: Closed Hats 8ths (Verse)
    emit("y3")
    emit("[hats 8ths] yt")
    emit("[v2 l.50] x0")
    emit("[v2 l.30] x2")
    emit("[v2 l.50] x4")
    emit("[v2 l.30] x6")
    emit("[v2 l.50] x8")
    emit("[v2 l.30] x10")
    emit("[v2 l.50] x12")
    emit("[v2 l.30] x14")
    emit("[=127,0] x15")
    emit()

    # Pattern 18: Running 16th Closed Hats with Accents & Offbeat Open Sizzle
    emit("y18")
    emit("[hats 16ths groove + offbeat sizzle] yt")
    emit("[v2 l.50] x0")
    emit("[v2 l.25] x1")
    emit("[v3 l.80 v2 l0] x2")
    emit("[v2 l.25] x3")
    emit("[v2 l.55] x4")
    emit("[v2 l.25] x5")
    emit("[v3 l.85 v2 l0] x6")
    emit("[v2 l.25] x7")
    emit("[v2 l.50] x8")
    emit("[v2 l.25] x9")
    emit("[v3 l.80 v2 l0] x10")
    emit("[v2 l.25] x11")
    emit("[v2 l.55] x12")
    emit("[v2 l.25] x13")
    emit("[v3 l.85 v2 l0] x14")
    emit("[v2 l.25] x15")
    emit()

    # Pattern 6: The Iconic Sweet Dreams Riff (32 steps = 2 bars, 8th notes on evens)
    emit("y6")
    emit("[the iconic sweet dreams bass riff] yt")
    for step in range(0, 32, 2):
        velo = "1" if (step % 8 == 0) else "0.85"
        emit(f"[v6 n&1 l{velo}] x{step}")
    emit("[=127,0] x31")
    emit()

    # Pattern 7: Left-Panned Counter-Riff (32 steps = 2 bars, octave up)
    emit("y7")
    emit("[left-panned counter riff] yt")
    for step in range(0, 32, 2):
        velo = "0.9" if (step % 8 == 0) else "0.75"
        emit(f"[v13 n&2 l{velo}] x{step}")
    emit("[=127,0] x31")
    emit()

    # Pattern 8: Chorus Vocal Melody ('Sweet dreams are made of this...', 64 steps / 4 bars)
    # Sparsely triggered with smooth portamento, zero intermediate rest choking!
    emit("y8")
    emit("[chorus vocal melody - sweet dreams] yt")
    # Bar 1: 'Sweet dreams are made of this'
    emit("[v8 n60 l.9] x0")     # C4 ('Sweet...')
    emit("[v8 n63 l.9] x4")     # Eb4 ('...dreams...')
    emit("[v8 n67 l1] x8")      # G4 ('...are made of this')
    emit("[v8 n67 l.9] x16")    # G4
    emit("[v8 n65 l.85] x20")   # F4
    emit("[v8 n63 l.85] x24")   # Eb4
    emit("[v8 n62 l.85] x28")   # D4
    # Bar 2: 'Who am I to disagree?'
    emit("[v8 n60 l.9] x32")    # C4 ('Who...')
    emit("[v8 n63 l.9] x36")    # Eb4 ('...am I...')
    emit("[v8 n67 l1] x40")     # G4 ('...to disagree?')
    emit("[v8 n68 l.95] x48")   # Ab4
    emit("[v8 n67 l.85] x52")   # G4
    emit("[v8 n65 l.85] x56")   # F4
    emit("[v8 n63 l.85] x60")   # Eb4
    emit("[=127,0] x63")
    emit()

    # Pattern 10: Verse Vocal Melody ('Some of them want to use you...', 64 steps / 4 bars)
    emit("y10")
    emit("[verse vocal melody - some of them] yt")
    # Bar 1: 'Some of them want to use you'
    emit("[v8 n60 l.9] x0")
    emit("[v8 n60 l.85] x2")
    emit("[v8 n60 l.9] x4")
    emit("[v8 n60 l.85] x6")
    emit("[v8 n63 l.95] x8")
    emit("[v8 n62 l.85] x12")
    emit("[v8 n60 l.9] x14")
    # Bar 2: 'Some of them want to get used by you'
    emit("[v8 n60 l.9] x16")
    emit("[v8 n60 l.85] x18")
    emit("[v8 n60 l.9] x20")
    emit("[v8 n60 l.85] x22")
    emit("[v8 n63 l.95] x24")
    emit("[v8 n62 l.85] x28")
    emit("[v8 n60 l.9] x30")
    # Bar 3: 'Some of them want to abuse you'
    emit("[v8 n68 l.9] x32")
    emit("[v8 n68 l.85] x34")
    emit("[v8 n68 l.9] x36")
    emit("[v8 n68 l.85] x38")
    emit("[v8 n67 l.95] x40")
    emit("[v8 n65 l.85] x44")
    emit("[v8 n63 l.9] x46")
    # Bar 4: 'Some of them want to be abused'
    emit("[v8 n67 l.9] x48")
    emit("[v8 n67 l.85] x50")
    emit("[v8 n67 l.9] x52")
    emit("[v8 n67 l.85] x54")
    emit("[v8 n65 l.95] x56")
    emit("[v8 n63 l.85] x60")
    emit("[v8 n62 l.9] x62")
    emit("[=127,0] x63")
    emit()

    # Pattern 12: The Gospel Bridge ('Hold your head up, movin' on...', 64 steps / 4 bars)
    emit("y12")
    emit("[the gospel bridge - hold your head up] yt")
    # Bar 1: 'Hold your head up'
    emit("[v8 n67 l1] x0")      # G4 ('Hold...')
    emit("[v8 n70 l1] x8")      # Bb4 ('...your head...')
    emit("[v8 n72 l1.05] x16")  # C5 ('...up!')
    # Bar 2: 'Keep your head up'
    emit("[v8 n67 l1] x32")     # G4 ('Keep...')
    emit("[v8 n70 l1] x40")     # Bb4 ('...your head...')
    emit("[v8 n72 l1.05] x48")  # C5 ('...up!')
    # Bar 3: 'Movin' on!'
    emit("[v8 n75 l1] x56")     # Eb5 ('Movin'...')
    emit("[v8 n74 l.9] x60")    # D5 ('...on!')
    emit("[=127,0] x63")
    emit()

    # Pattern 9: Analog Synth Solo (64 steps / 4 bars)
    # Screaming pitch bends with 70ms portamento glide!
    emit("y9")
    emit("[screaming analog synth solo] yt")
    emit("[v11 n60 l1] x0")     # C4
    emit("[v11 n63 l.95] x8")   # Eb4 (glide)
    emit("[v11 n67 l1] x16")    # G4
    emit("[v11 n70 l1.05] x24") # Bb4
    emit("[v11 n72 l1.1] x32")  # C5 (soaring peak)
    emit("[v11 n75 l1] x40")    # Eb5
    emit("[v11 n72 l.95] x48")  # C5
    emit("[v11 n70 l.9] x52")   # Bb4
    emit("[v11 n67 l.9] x56")   # G4
    emit("[v11 n63 l.85] x60")  # Eb4
    emit("[=127,0] x63")
    emit()

    # Pattern 11: Synth Pad Chords (Cm -> Ab -> G, 32 steps = 2 bars)
    emit("y11")
    emit("[warm synth pad chords] yt")
    # Bar 1: C minor (C4, Eb4, G4)
    emit("[v26 n48 l.85] x0")
    # Bar 2: Ab major (Ab3, C4, Eb4) to G major (G3, B3, D4)
    emit("[v26 n44 l.85] x16")
    emit("[v26 n43 l.85] x24")
    emit("[=127,0] x31")
    emit()

    # Pattern 15: Dave Stewart Milk Bottle Crack & Sub Boom Drop (Fires on Bar 2 of the 2-bar cycle)
    emit("y15")
    emit("[milk bottle crack + sub boom drop] yt")
    emit("[v21 l1 v22 l.9 v38 g0 n48 g.15 n36 l1 v40 l1] x16")
    emit("[=127,0] x31")
    emit()

    # Pattern 13: Simmons Tom Roll Fill (one-shot with stop marker '-')
    emit("y13")
    emit("[simmons tom roll fill] yt")
    emit("[v18 l.8] x0")
    emit("[v18 l.85] x2")
    emit("[v18 l.9] x4")
    emit("[v18 l.95] x6")
    emit("[v19 l.85] x8")
    emit("[v19 l.9] x10")
    emit("[v20 l.95] x12")
    emit("[v20 l1] x14")
    emit("[-] x16")
    emit()

    # Pattern 16: Bass Cutoff Sweep Lane (4 bars, 32 two-step increments from 600 Hz to 3600 Hz)
    emit("y16")
    emit("[bass cutoff sweep open] yt")
    f_start = 600.0
    f_end = 3600.0
    num_steps = 32
    for s in range(num_steps):
        phase = s / float(num_steps - 1)
        cutoff = int(round(f_start * math.pow(f_end / f_start, phase)))
        emit(f"[v6 k{cutoff} v7 k{cutoff}] x{s*2}")
    emit("[=127,0] x63")
    emit()

    # ----------------------------------------------------
    # Conductor (Pattern 0 at %16, 96 bars total)
    # ----------------------------------------------------
    emit("## CONDUCTOR (pattern 0, one step per bar, 96 bars)")
    emit("y0")
    emit("[conductor] yt")
    emit("%16")

    conductor = {}
    def c_add(bar, cmd):
        conductor.setdefault(bar, []).append(cmd)

    # Bars 0-3: Intro - Lone bass riff enters with milk bottle crack on bar 1 and 3 downbeats
    c_add(0, "y6 z1 /SP 1,0 y15 z1 v6 k750 v7 k750")
    c_add(2, "v6 k900 v7 k900")

    # Bars 4-7: 16th hats enter, counter-riff in left channel, filter opens
    c_add(4, "y3 z1 v6 k1100 v7 k1100")
    c_add(6, "y7 z1 /SP 2,0")
    c_add(7, "y13 z1")

    # Bars 8-15: Groove drops - Kick slams in, claps & snare on 2 & 4, pad chords swell
    c_add(8, "v17 l1 y1 z1 y2 z1 y11 z1 y16 z1")
    c_add(12, "y3 z0 y18 z1 v6 k2200 v7 k2200")
    c_add(15, "y13 z1")

    # Bars 16-23: Verse 1 - Annie Lennox vocal ('Some of them want to use you...')
    c_add(16, "v17 l.8 y10 z1 v6 k1400 v7 k1400")
    c_add(20, "v6 k1800 v7 k1800")
    c_add(23, "y10 z0 v8 l0 y13 z1")

    # Bars 24-39: Chorus 1 - 'Sweet dreams are made of this...', full energy wall of sound
    c_add(24, "v17 l1 y8 z1 v6 k3200 v7 k3200")
    c_add(28, "v17 l.6")
    c_add(32, "v17 l.7")
    c_add(36, "v17 l.6")
    c_add(39, "y8 z0 v8 l0 y13 z1")

    # Bars 40-47: The Gospel Bridge ('Hold your head up, movin' on...')
    c_add(40, "v17 l1 y12 z1 v6 k1200 v7 k1200")
    c_add(44, "v17 l.7 v6 k1800 v7 k1800")
    c_add(47, "y12 z0 v8 l0 y13 z1")

    # Bars 48-55: Screaming Analog Synth Solo!
    c_add(48, "v17 l1 y9 z1 v6 k3200 v7 k3200")
    c_add(52, "v17 l.8")
    c_add(55, "y9 z0 v11 l0 y13 z1")

    # Bars 56-63: The Legendary Breakdown!
    # Drums drop out, delay freezes (DF 1 1), cosmic floating drone
    c_add(56, "v17 l1 DF 1 1 y1 z0 y2 z0 y18 z0 y6 z0 y7 z0 y15 z0 v6 l0 v7 l0 v13 l0")
    c_add(57, "v8 l0 v11 l0")
    c_add(58, "y11 z1")
    c_add(60, "v16 l1")
    c_add(63, "y13 z1")

    # Bars 64-79: Chorus 2 & Climax - Maximum energy, wall of sound!
    c_add(64, "DF 1 0 v17 l1 y1 z1 y2 z1 y18 z1 y6 z1 /SP 1,0 y7 z1 /SP 2,0 y15 z1 y8 z1 v6 k3600 v7 k3600")
    c_add(68, "v17 l.7")
    c_add(72, "v17 l.8 y9 z1")
    c_add(76, "v17 l.7")
    c_add(79, "y8 z0 y9 z0 v8 l0 v11 l0 y13 z1")

    # Bars 80-87: Verse 2 / Bridge Reprise
    c_add(80, "v17 l1 y10 z1 v6 k2200 v7 k2200")
    c_add(84, "v17 l.7")
    c_add(87, "y10 z0 v8 l0 y13 z1")

    # Bars 88-95: Outro Strip-Back
    c_add(88, "v17 l.8 y2 z0 y18 z0 y3 z%2 v6 k1400 v7 k1400")
    c_add(90, "v6 k900 v7 k900 y7 z0 v13 l0")
    c_add(92, "DF 1 1 y1 z0 y3 z0")
    c_add(94, "y6 z0 y15 z0 v6 l0 v7 l0")

    # Bar 96: End Cleanup
    c_add(96, "Z0 +8 ce 99")

    # Output conductor steps
    for bar in range(97):
        cmds = conductor.get(bar)
        if cmds:
            line_str = " ".join(cmds)
            emit(f"[{line_str}] x{bar}")
        else:
            emit(f"[=126,{bar}] x{bar}")
    emit()

    # ----------------------------------------------------
    # Macros
    # ----------------------------------------------------
    emit("## MACROS (play / stop)")
    emit("[use 'play' to start the song, 'stop' to stop it] puts")
    emit("[play] : Z0 Z! y0 z1 ;")
    emit("[stop] : Z0 O DQ ;")
    emit()

    return "\n".join(lines) + "\n"

def main():
    target_path = os.path.join(os.path.dirname(__file__), "sweet_dreams_full.sk")
    target_path_short = os.path.join(os.path.dirname(__file__), "sweet_dreams.sk")
    content = build_sweet_dreams()
    with open(target_path, "w") as f:
        f.write(content)
    with open(target_path_short, "w") as f:
        f.write(content)
    print(f"Wrote {len(content.splitlines())} lines to {target_path} and {target_path_short}")

if __name__ == "__main__":
    main()
