#!/usr/bin/env python3
"""
generate_closer.py - Generates closer.sk for PULP / SKRED.
An authentic, tour-de-force instrumental cover of Nine Inch Nails "Closer" (1994).
Written and produced by Trent Reznor.

Features:
- "Nightclubbing" heartbeat kick (sample w82 + pitch-gliding sine sub w0)
- Gated industrial snare (w83 + delay bus 3 slapback) + clap (w86)
- Gritty 16th hats with dynamic accents and open-hat chokes (w84, w85)
- Rubbery Minimoog bass (saw w15 + detuned partner square w16, drive j1,2, resonance q6, slapback delay bus 2)
  with authentic octave snaps and F#1->G1 glides
- Post-chorus ascending hook (E -> F -> G -> Ab chromatic tension, phase distortion c4, dotted-eighth echo bus 1)
- Crushing chorus synth brass & power stabs (detuned trio w15/w16, polyphonic pad pool v28-v37)
- Industrial machine ring mod clangs (w16 f311 XM19 H19 bc5) & bitcrushed noise bursts (w5 bc6)
- Stretched inline K-synth build riser (w301, zero DC offset)
- Delay freeze drone capture (DF 1 1)
- All voices clearly labeled and inspected via [name] vt
- The iconic Outro Piano ("The Downward Spiral" descending melody through hall delay bus 4)
- Safe s=0 decay envelopes and explicit conductor releases so notes never hang
- 92-bar arrangement sequenced via Pattern 0 Conductor at %16
"""

import os, sys, math

def build_closer():
    lines = []
    
    def emit(s=""):
        lines.append(s)

    # ----------------------------------------------------
    # Header & Song Metadata
    # ----------------------------------------------------
    emit("## NINE INCH NAILS - CLOSER (Instrumental Cover for PULP / SKRED)")
    emit("#  90 BPM, C minor / C blues modal tension, 92 bars (about 4:05). Trent Reznor (1994).")
    emit("#  Self-contained: drums from built-in waves 82-99, inline K-synth riser.")
    emit("#  Load:   [closer.sk] /ls")
    emit("#  Start:  play        Stop:  stop")
    emit("#")
    emit("#  VOICE MAP (all labeled via [name] vt):")
    emit("#    v0  kick (w82) + v5 sub-drop glide (w0) = the iconic heartbeat pulse")
    emit("#    v1  industrial snare (w83) + v4 gated clap (w86)")
    emit("#    v2  closed hat (w84)  v3  open hat (w85)  v15 metallic sizzle (w95)")
    emit("#    v6  Minimoog bass saw (w15, centre) + v7 partner square (w16, right)")
    emit("#    v8  chorus synth brass centre (w15) + v9 (w15 left), v10 (w16 right)")
    emit("#    v11 post-chorus ascending hook (w15) + v12 partner pulse (w16)")
    emit("#    v13 outro piano tone (w15) + v14 tape sub/wood layer (w0)")
    emit("#    v16 K-synth build riser (w301)")
    emit("#    v17 crash cymbal (w94)")
    emit("#    v18 ring mod metal clang (w16) + v19 modulator (w15)")
    emit("#    v20 bitcrushed industrial noise burst (w5)")
    emit("#    v24 pad LFO  v26,v27 pad prototype  v28-v37 poly pool (5 stereo pairs)")
    emit("#    v38 sub boom sine (w0) + v40 slowed kick sample (w82 f110)")
    emit("#")
    emit("#  DELAY BUSES:")
    emit("#    Bus 1: Hook & Lead dotted-eighth echo (DS 1 90 .75)")
    emit("#    Bus 2: Bass slapback sixteenth (DS 2 90 .25)")
    emit("#    Bus 3: Snare & Clap gated industrial tail (DS 3 90 .5)")
    emit("#    Bus 4: Outro piano & Riser deep hall wash (DS 4 90 .75, DP 4 1)")
    emit("#")
    emit("#  ARRANGEMENT (92 bars):")
    emit("#    0-3   Intro: heartbeat kick & industrial noise pulse")
    emit("#    4-7   Minimoog bass enters alone with heartbeat kick")
    emit("#    8-15  Verse 1: snare, clap, closed hats, clangs, industrial funk groove")
    emit("#    16-23 Verse 1 Pre-Chorus: 16th hats, pad swells, bass filter opens")
    emit("#    24-31 Post-Chorus Hook 1: E-F-G-Ab lead explodes with dotted echo")
    emit("#    32-39 Verse 2: stripped industrial funk groove, machine percussion")
    emit("#    40-47 Build-up: snare ratchet roll, K-synth riser, bass sweep")
    emit("#    48-63 Full Chorus: wall of sound (Ab - Bb - Cm), brass stabs, sidechain")
    emit("#    64-71 Climax & Delay Freeze: hook returns, freeze drone capture (DF 1 1)")
    emit("#    72-87 Outro: Downward Spiral piano solo over heartbeat kick & hall delay")
    emit("#    88-91 Fade to black: final piano echo decays, heartbeat stops")
    emit("#    92    End cleanup")
    emit("# ======================================================================")
    emit()
    emit("[NINE INCH NAILS - CLOSER] puts")
    emit("M 90 16")
    emit()

    # ----------------------------------------------------
    # Inline K-Synth Riser (Wave 301)
    # ----------------------------------------------------
    emit("## INLINE K-SYNTH RISER (one-shot wave 301)")
    emit("[")
    emit("/ riser: exponential sweep + highpass noise, zero DC")
    emit("N: 131072")
    emit("T: !N")
    emit("E: (e(T*(4.6%N)))%100")
    emit("R: r T")
    emit("L: 0.35 f R")
    emit("H: R-L")
    emit("F: 900*(e(T*(2.7%N)))*((p 2)%(p 0))")
    emit("P: +\\F")
    emit("S: s P")
    emit("W: w E*((S*.35)+(H*.6))")
    emit("] k!")
    emit("k>w301,44100,1")
    emit()

    # ----------------------------------------------------
    # Delay Buses Setup
    # ----------------------------------------------------
    emit("## DELAY BUSES")
    emit("# Bus 1: Lead dotted-eighth echo")
    emit("DS 1 90 .75")
    emit("DL 1 - - 8 3 3 11")
    emit("DD 1 7 3")
    emit()
    emit("# Bus 2: Bass slapback sixteenth (adds thickness without blur)")
    emit("DS 2 90 .25")
    emit("DL 2 - - 4 0 0 8")
    emit("DD 2 6 3")
    emit()
    emit("# Bus 3: Industrial snare / clap gated tail")
    emit("DS 3 90 .5")
    emit("DL 3 - - 6 0 0 9")
    emit("DD 3 5 4")
    emit()
    emit("# Bus 4: Outro piano & riser deep ping-pong hall wash")
    emit("DS 4 90 .75")
    emit("DL 4 - - 11 4 8 10")
    emit("DD 4 8 2")
    emit("DP 4 1")
    emit()

    # ----------------------------------------------------
    # Voices Setup
    # ----------------------------------------------------
    emit("## VOICES")
    emit("# Drums: heartbeat kick + sub drop layer")
    emit("v0 w82 f440 a7 p0 [kick nightclubbing] vt")
    emit("v5 w0 a-2 p0 t.002,.35,0,.10 j1 k160 q.7 [kick sub drop] vt")
    emit()
    emit("# Industrial snare & claps")
    emit("v1 w83 f440 a8 p0 r3 ds.25 [snare industrial] vt")
    emit("v2 w84 f440 a-3 p.25 [closed hat] vt")
    emit("v3 w85 f440 a-2 p-.25 [open hat] vt")
    emit("v4 w86 f440 a7 p0 [gated clap] vt")
    emit("v15 w95 f440 a-4 p-.35 [metallic sizzle] vt")
    emit("v17 w94 f440 a4 p.2 [crash cymbal] vt")
    emit()
    emit("# Minimoog Bass: saw (centre) + detuned square partner (right), Character 2 overdrive")
    emit("v6 w15 a-1 p0 t.002,.22,0,.06 j1,2 k440 q6 g.03 r2 ds.10 [minimoog bass saw] vt")
    emit("v6 >7")
    emit("v7 w16 a-5 N0,9 p.4 r0 [minimoog partner square] vt")
    emit("v6 G7 H7")
    emit()
    emit("# Chorus Synth Brass / Power Stabs (detuned trio)")
    emit("# s=0 decay envelope guarantees notes decay cleanly and never hang")
    emit("v8 w15 a-7 p0 t.008,.35,0,.25 j1,2 k3600 q1.8 c4,.25 r1 ds.3 [synth brass centre] vt")
    emit("v8 >9")
    emit("v8 >10")
    emit("v9 w15 a-8 N0,12 p-.6 r0 [synth brass left] vt")
    emit("v10 w16 a-10 N0,-12 p.6 r0 [synth brass right] vt")
    emit("v8 G9,10 H9,10")
    emit()
    emit("# Post-Chorus Ascending Hook (nasal saw + pulse partner, phase distortion c4)")
    emit("v11 w15 a-6 p0 t.004,.28,0,.20 j1 k2800 q2.2 c4,.35 r1 ds.35 [ascending hook saw] vt")
    emit("v11 >12")
    emit("v12 w16 a-9 N0,11 p.55 r0 [ascending partner pulse] vt")
    emit("v11 G12 H12")
    emit()
    emit("# Outro Piano (The Downward Spiral motif: filtered saw + tape sine sub)")
    emit("v13 w15 a-6 p0 t.002,.65,0,.45 j1 k1400 q1.4 c4,.15 r4 ds.4 [downward spiral piano] vt")
    emit("v13 >14")
    emit("v14 w0 a-4 p.3 N0,5 t.002,.70,0,.40 j1 k800 q.7 r0 [piano tape sub wood] vt")
    emit("v13 G14 H14")
    emit()
    emit("# Riser: stretched K-synth one-shot")
    emit("v16 w301 f120 a8 p0 t.005,.01,1,.4 r4 ds.3 [ksynth build riser] vt")
    emit()
    emit("# Industrial Clangs: ring modulation of two inharmonic frequencies + bitcrush")
    emit("v18 w16 f311 a2 t.001,.30,0,.15 bc5 XM19 H19 [machine ring mod clang] vt")
    emit("v19 w15 f187 a0 t.001,.25,0,.15 m1 [ring mod carrier] vt")
    emit()
    emit("# Bitcrushed industrial noise burst")
    emit("v20 w5 a-4 p-.4 t.001,.18,0,.10 j2 k1600 q1.2 bc6 [bitcrushed noise burst] vt")
    emit()
    emit("# Sub boom: audible sub drop + slowed kick")
    emit("v38 w0 a0 p0 t.002,.90,0,.40 j1 k240 q.7 r3 ds.3 [sub boom sine] vt")
    emit("v40 w82 f110 a5 p0 [slowed kick drop] vt")
    emit()
    emit("# Pad: Poly pool, 2-voice prototype (detuned stereo saw pair), 5 instances at v28-v37")
    emit("v24 w0 f.22 a0 t0,0,1,0 m1 [pad lfo] vt")
    emit("v26 w15 a-10 p-.5 t1.5,.5,0,1.6 j1 k2000 q.8 c4,.2 C24,.2 G27 H27 [pad proto left] vt")
    emit("v27 w15 a-12 p.5 N0,8 t1.5,.5,0,1.6 j1 k2000 q.8 c4,.2 C24,.2 [pad proto right] vt")
    emit("/pg 0,26,2,0")
    emit("/pp 0,0,28,5,0")
    emit("v24 l1")
    emit()

    # ----------------------------------------------------
    # Events (Cleanup Responder)
    # ----------------------------------------------------
    emit("## EVENTS")
    emit("[O DQ] /ceb 4 99")
    emit("/cer 1")
    emit()

    # ----------------------------------------------------
    # Patterns
    # ----------------------------------------------------
    emit("## PATTERNS")
    emit()

    # Pattern 1: Heartbeat Kick
    emit("y1")
    emit("[kick heartbeat] yt")
    emit("[v0 l1 v5 g0 n52 g.08 n28 l1] x0")
    emit("[v0 l.6] x7")
    emit("[v0 l.9 v5 g0 n48 g.08 n28 l.8] x8")
    emit("[v0 l.95 v5 g0 n48 g.08 n28 l.85] x10")
    emit("[v0 l.7] x14")
    emit("[=127,0] x15")
    emit()

    # Pattern 2: Snare + Clap
    emit("y2")
    emit("[snare+clap] yt")
    emit("[v1 l1 v4 l.95] x4")
    emit("[v1 l.25] x6")
    emit("[v1 l.20] x11")
    emit("[v1 l1 v4 l.95] x12")
    emit("[=127,0] x15")
    emit()

    # Pattern 3: Closed Hats 8ths (Verse)
    emit("y3")
    emit("[hats 8ths] yt")
    emit("[v2 l.5] x0")
    emit("[v2 l.3] x2")
    emit("[v2 l.5] x4")
    emit("[v2 l.3] x6")
    emit("[v2 l.5] x8")
    emit("[v2 l.3] x10")
    emit("[v2 l.5] x12")
    emit("[v2 l.3] x14")
    emit("[=127,0] x15")
    emit()

    # Pattern 18: Hats 16ths with Open Hat Offbeat Chokes
    emit("y18")
    emit("[hats 16ths + open accents] yt")
    emit("[v2 l.45] x0")
    emit("[v2 l.25] x1")
    emit("[v3 l.75 v2 l0] x2")
    emit("[v2 l.25] x3")
    emit("[v2 l.5] x4")
    emit("[v2 l.25] x5")
    emit("[v3 l.85 v2 l0] x6")
    emit("[v2 l.25] x7")
    emit("[v2 l.45] x8")
    emit("[v2 l.25] x9")
    emit("[v3 l.75 v2 l0] x10")
    emit("[v2 l.25] x11")
    emit("[v2 l.5] x12")
    emit("[v2 l.25] x13")
    emit("[v3 l.85 v2 l0] x14")
    emit("[v2 l.25] x15")
    emit("[=127,0] x15")
    emit()

    # Pattern 4: Minimoog Rubber Bassline (2-bar 32-step loop)
    # Features C1 bounces, C2 octave snaps, and F#1->G1 chromatic slides
    emit("y4")
    emit("[minimoog rubber bass 2-bar loop] yt")
    emit("[v6 g0 n36 l1] x0")
    emit("[v6 g0 n36 l.85] x2")
    emit("[v6 g0 n48 l.95] x4")
    emit("[v6 g0 n36 l.85] x6")
    emit("[v6 g0 n36 l1] x8")
    emit("[v6 g0 n39 l.9] x10")
    emit("[v6 g0 n48 l.95] x12")
    emit("[v6 g.04 n42 l.85] x14")
    emit("[v6 g0 n43 l.9] x15")
    emit("[v6 g0 n36 l1] x16")
    emit("[v6 g0 n36 l.85] x18")
    emit("[v6 g0 n48 l.95] x20")
    emit("[v6 g0 n36 l.85] x22")
    emit("[v6 g0 n34 l.95] x24")
    emit("[v6 g0 n34 l.85] x26")
    emit("[v6 g0 n48 l.95] x28")
    emit("[v6 g.04 n42 l.85] x30")
    emit("[v6 g0 n43 l.9] x31")
    emit("[=127,0] x31")
    emit()

    # Pattern 5: Industrial Clangs & Machine FX
    emit("y5")
    emit("[industrial machine clangs] yt")
    emit("[v18 f311 v19 f187 v18 l.85] x3")
    emit("[v20 l.75] x7")
    emit("[v18 f420 v19 f265 v18 l.9] x11")
    emit("[v18 f275 v19 f195 v18 l.8] x19")
    emit("[v20 l.85] x23")
    emit("[v18 f380 v19 f220 v18 l.95] x27")
    emit("[v18 f510 v19 f310 v18 l.7] x30")
    emit("[=127,0] x31")
    emit()

    # Pattern 6: Chorus Bass Roots (Ab -> Bb, 2-bar loop)
    emit("y6")
    emit("[chorus bass Ab and Bb roots] yt")
    emit("[v6 g0 n32 l1] x0")
    emit("[v6 g0 n32 l.85] x4")
    emit("[v6 g0 n44 l.95] x8")
    emit("[v6 g0 n32 l.85] x12")
    emit("[v6 g0 n34 l1] x16")
    emit("[v6 g0 n34 l.85] x20")
    emit("[v6 g0 n46 l.95] x24")
    emit("[v6 g0 n34 l.85] x28")
    emit("[=127,0] x31")
    emit()

    # Pattern 7: Post-Chorus Ascending Hook (E -> F -> G -> Ab over C minor)
    emit("y7")
    emit("[post-chorus ascending hook] yt")
    emit("[v11 g0 n64 l.9] x0")
    emit("[v11 g.03 n65 l.9] x4")
    emit("[v11 g.03 n67 l.95] x8")
    emit("[v11 g.03 n68 l1] x12")
    emit("[v11 g.03 n67 l.95] x16")
    emit("[v11 g.03 n65 l.9] x20")
    emit("[v11 g.03 n64 l.9] x24")
    emit("[v11 g.04 n60 l.85] x28")
    emit("[v11 l0] x30")
    emit("[=127,0] x31")
    emit()

    # Pattern 8: Chorus Pad Chords (Abmaj7 -> Bb9)
    emit("y8")
    emit("[chorus pad Abmaj7 and Bb9] yt")
    emit("[pn 0,101,56,.95 pn 0,102,60,.95 pn 0,103,63,.95 pn 0,104,67,.95] x0")
    emit("[pr 0,101 pr 0,102 pr 0,103 pr 0,104] x15")
    emit("[pn 0,105,58,.95 pn 0,106,62,.95 pn 0,107,65,.95 pn 0,108,70,.95] x16")
    emit("[pr 0,105 pr 0,106 pr 0,107 pr 0,108] x31")
    emit("[=127,0] x31")
    emit()

    # Pattern 9: Chorus Synth Brass Stabs (C minor power stabs)
    emit("y9")
    emit("[chorus synth brass stabs] yt")
    emit("[v8 n60 l1 v15 l.8] x0")
    emit("[v8 n63 l.9] x3")
    emit("[v8 n67 l.95] x6")
    emit("[v8 n60 l1 v15 l.8] x8")
    emit("[v8 n70 l1] x12")
    emit("[v8 n72 l1] x14")
    emit("[v8 l0] x15")
    emit("[v8 n60 l1 v15 l.8] x16")
    emit("[v8 n63 l.9] x20")
    emit("[v8 n67 l.95] x24")
    emit("[v8 n72 l1] x28")
    emit("[v8 l0] x31")
    emit("[=127,0] x31")
    emit()

    # Pattern 10: The Outro Piano (The Downward Spiral Descending Motif, 4 bars / 64 steps)
    emit("y10")
    emit("[the downward spiral outro piano] yt")
    emit("[v13 n70 l.85] x0")
    emit("[v13 n69 l.80] x8")
    emit("[v13 n68 l.85] x16")
    emit("[v13 n67 l.80] x24")
    emit("[v13 n66 l.85] x32")
    emit("[v13 n65 l.80] x40")
    emit("[v13 n63 l.85] x48")
    emit("[v13 n62 l.80] x54")
    emit("[v13 n60 l.90] x56")
    emit("[v13 l0] x63")
    emit("[=127,0] x63")
    emit()

    # Pattern 11: Dark Verse Atmosphere Pad (Cm drone)
    emit("y11")
    emit("[verse dark pad] yt")
    emit("[pn 0,109,48,.8 pn 0,110,55,.8 pn 0,111,58,.8] x0")
    emit("[pr 0,109 pr 0,110 pr 0,111] x31")
    emit("[=127,0] x31")
    emit()

    # Pattern 13: Snare Ratchet Roll (one-shot with stop marker '-')
    emit("y13")
    emit("[snare ratchet roll] yt")
    emit("[v1 l0.20] x0")
    emit("[v1 l0.25] x2")
    emit("[v1 l0.30] x4")
    emit("[v1 l0.35] x6")
    emit("[v1 l0.45 z*2] x8")
    emit("[v1 l0.55 z*2] x10")
    emit("[v1 l0.70 z*4] x12")
    emit("[v1 l0.85 z*4] x14")
    emit("[v1 l0.95 z*8] x15")
    emit("[-] x16")
    emit()

    # Pattern 14: Fake Sidechain Pump (4 steps = 1 beat)
    emit("y14")
    emit("[fake sidechain pump] yt")
    emit("[v8 a-20 v9 a-20 v10 a-22 v28 a-20 v29 a-22 v30 a-20 v31 a-22 v32 a-20 v33 a-22 v34 a-20 v35 a-22 v36 a-20 v37 a-22] x0")
    emit("[v8 a-14 v9 a-14 v10 a-16 v28 a-14 v29 a-16 v30 a-14 v31 a-16 v32 a-14 v33 a-16 v34 a-14 v35 a-16 v36 a-14 v37 a-16] x1")
    emit("[v8 a-9 v9 a-9 v10 a-12 v28 a-10 v29 a-12 v30 a-10 v31 a-12 v32 a-10 v33 a-12 v34 a-10 v35 a-12 v36 a-10 v37 a-12] x2")
    emit("[v8 a-7 v9 a-8 v10 a-10 v28 a-10 v29 a-12 v30 a-10 v31 a-12 v32 a-10 v33 a-12 v34 a-10 v35 a-12 v36 a-10 v37 a-12] x3")
    emit()

    # Pattern 15: Sub Boom Drop (one-shot with stop marker '-')
    emit("y15")
    emit("[sub boom drop] yt")
    emit("[v38 g0 n48 g.15 n36 l1 v40 l1] x0")
    emit("[-] x1")
    emit()

    # Pattern 16: Bass Cutoff Sweep (4 bars, 32 two-step increments from 440 Hz to 2800 Hz)
    emit("y16")
    emit("[bass cutoff sweep] yt")
    f_start = 440.0
    f_end = 2800.0
    num_steps = 32
    for s in range(num_steps):
        phase = s / float(num_steps - 1)
        cutoff = int(round(f_start * math.pow(f_end / f_start, phase)))
        emit(f"[v6 k{cutoff} v7 k{cutoff}] x{s*2}")
    emit("[=127,0] x63")
    emit()

    # ----------------------------------------------------
    # Conductor (Pattern 0 at %16, 92 bars)
    # ----------------------------------------------------
    emit("## CONDUCTOR (pattern 0, one step per bar, 92 bars)")
    emit("y0")
    emit("[conductor] yt")
    emit("%16")

    conductor = {}
    def c_add(bar, cmd):
        conductor.setdefault(bar, []).append(cmd)

    # Bars 0-3: Intro Heartbeat
    c_add(0, "y1 z1 v6 k440 v7 k440")
    c_add(2, "v20 l.6")

    # Bars 4-7: Minimoog Bass enters alone with heartbeat
    c_add(4, "y4 z1")

    # Bars 8-15: Verse 1 (Snare, Clap, Closed Hats, Industrial Clangs)
    c_add(8, "v17 l.8 y2 z1 y3 z1 y5 z1")
    c_add(12, "v6 k550 v7 k550")

    # Bars 16-23: Verse 1 Pre-Chorus (16th Hats, Dark Pad Swell, Sweep)
    c_add(16, "v17 l.6 y3 z0 y18 z1 y11 z1 y16 z1")
    c_add(20, "v16 l1")
    c_add(23, "y1 z0 y2 z0 y13 z1")

    # Bars 24-31: Post-Chorus Hook 1 (E -> F -> G -> Ab Ascending Lead)
    c_add(24, "v17 l1 y1 z1 y2 z1 y18 z1 y7 z1 y15 z1 y16 z0 v6 k900 v7 k900")
    c_add(31, "y7 z0 v11 l0")

    # Bars 32-39: Verse 2 (Stripped Industrial Funk Groove)
    c_add(32, "v17 l.6 y18 z0 y3 z1 y11 z0 v6 k460 v7 k460")
    c_add(36, "v6 k600 v7 k600 y5 z1")

    # Bars 40-47: Build-up to Chorus (Sweep, Riser, Ratchet Roll)
    c_add(40, "v17 l.7 y3 z0 y18 z1 y16 z1")
    c_add(44, "v16 l1")
    c_add(47, "y1 z0 y2 z0 y13 z1")

    # Bars 48-63: Full Chorus (Wall of sound: Ab -> Bb -> Cm)
    # Bar 48-49: Ab
    c_add(48, "v17 l1 y1 z1 y2 z1 y18 z1 y4 z0 y6 z1 y8 z1 y14 z1 y16 z0 v6 k3200 v7 k3000")
    # Bar 52-55: Cm (main bass + synth brass stabs)
    c_add(52, "y6 z0 y4 z1 y9 z1")
    # Bar 56-57: Ab
    c_add(56, "v17 l.8 y4 z0 y9 z0 v8 l0 y6 z1 y8 z1")
    # Bar 60-63: Cm
    c_add(60, "y6 z0 y4 z1 y9 z1")
    c_add(63, "y2 z0 y13 z1")

    # Bars 64-71: Climax & Breakdown (Hook + Stabs Clash, Delay Freeze Capture)
    c_add(64, "v17 l1 y2 z1 y7 z1")
    c_add(70, "v16 l1")
    c_add(71, "DF 1 1 y1 z0 y2 z0 y18 z0 y4 z0 y6 z0 y7 z0 y8 z0 y9 z0 y14 z0 v11 l0 v8 l0 v6 l0 v7 l0")

    # Bars 72-87: The Outro ("The Downward Spiral" Piano Solo over Heartbeat Kick)
    c_add(72, "DF 1 0 y1 z1 y10 z1 y15 z1 v11 l0 v8 l0 v6 l0 v7 l0")
    c_add(76, "v20 l.5")
    c_add(80, "y15 z1")
    c_add(84, "v20 l.4")

    # Bars 88-91: Fade to Black & Final Echo
    c_add(88, "y10 z0 v13 l0 y1 z0 v0 l1 v5 g0 n52 g.10 n28 l1")
    c_add(90, "v0 l.8 v5 g0 n52 g.10 n28 l.8")

    # Bar 92: End Cleanup
    c_add(92, "Z0 +8 ce 99")

    # Output conductor steps
    for bar in range(93):
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

    return "\n".join(lines)

if __name__ == "__main__":
    song_text = build_closer()
    out_path = os.path.join(os.path.dirname(__file__), "closer.sk")
    with open(out_path, "w") as f:
        f.write(song_text)
    print(f"Wrote {len(song_text.splitlines())} lines to {out_path}")
