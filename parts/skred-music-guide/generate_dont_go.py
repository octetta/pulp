#!/usr/bin/env python3
"""
generate_dont_go.py - Generates dont_go.sk for PULP / SKRED.
An authentic, tour-de-force instrumental cover of Yazoo's classic "Don't Go" (1982).
Written by Vince Clarke and Alison Moyet.

Features:
- Authentic key: B-flat minor (A# minor) at 128 BPM
- Relentless, snappy 16th-note analog synth bass (Pro-One / SH-1 saw w15 + detuned square w16,
  Character-2 overdrive j1,2, slapback delay Bus 2)
- Note streams defined using [ ... ] n>d with '-' for rests
- Four-on-the-floor LinnDrum/808-style electronic kick (w82) + sub punch (w0)
- Crisp gated 80s snare (w83) + electronic clap (w86) + 16th hats (w84, w85)
- Syncopated cowbell (w96) and descending Simmons electronic tom fills (w90, w91, w92)
- The iconic Vince Clarke signature synth brass hook (detuned trio w15/w16)
- The rapid 16th-note Vince Clarke arpeggio (w15/w16 detuned pair with phase distortion c4,
  feeding dotted-eighth echo Bus 1)
- Expressive Alison Moyet vocal lead synth line with safe s=0 decay envelopes
- All voices clearly tagged and inspected via [name] vt
- Alison's famous opening vocal laugh / cry FX (v21 formant glide)
- Inline K-synth build riser (w301)
- 96-bar arrangement sequenced via Pattern 0 Conductor at %16
"""

import os, sys, math

def build_dont_go():
    lines = []
    
    def emit(s=""):
        lines.append(s)

    # ----------------------------------------------------
    # Header & Song Metadata
    # ----------------------------------------------------
    emit("## YAZOO - DON'T GO (Full Arrangement Instrumental Cover for PULP / SKRED)")
    emit("#  128 BPM, B-flat minor, 96 bars (about 3:00). Vince Clarke & Alison Moyet (1982).")
    emit("#  Self-contained: drums from built-in waves 82-99, inline K-synth riser.")
    emit("#  Uses n>d to convert note name strings (with '-' rests) to MIDI note streams.")
    emit("#  Load:   [dont_go.sk] /ls")
    emit("#  Start:  play        Stop:  stop")
    emit("#")
    emit("#  VOICE MAP (all labeled via [name] vt):")
    emit("#    v0  electronic kick (w82) + v5 sub punch (w0)")
    emit("#    v1  gated 80s snare (w83) + v4 electronic clap (w86)")
    emit("#    v2  closed hat (w84)  v3  open hat (w85)  v15 cowbell (w96)")
    emit("#    v6  Vince Clarke bass saw (w15, centre) + v7 partner square (w16, right)")
    emit("#    v8  synth brass centre (w15) + v9 partner saw (w15 left), v10 (w16 right)")
    emit("#    v11 arpeggio lead centre (w15) + v12 partner pulse (w16)")
    emit("#    v13 vocal lead synth centre (w15) + v14 partner saw (w15)")
    emit("#    v16 K-synth build riser (w301)")
    emit("#    v17 crash cymbal (w94)")
    emit("#    v18 hi tom (w90)  v19 mid tom (w91)  v20 lo tom (w92) = Simmons tom roll")
    emit("#    v21 Alison Moyet vocal laugh / cry FX (w15 formant glide)")
    emit("#    v24 pad LFO  v26,v27 pad prototype  v28-v37 poly pool (5 stereo pairs)")
    emit("#    v38 sub boom sine (w0) + v40 slowed kick sample (w82 f110)")
    emit("#")
    emit("#  DELAY BUSES:")
    emit("#    Bus 1: Arp & Lead dotted-eighth echo (DS 1 128 .75, DP 1 1 ping-pong)")
    emit("#    Bus 2: Vince Clarke bass 16th slapback (DS 2 128 .25)")
    emit("#    Bus 3: Snare & Clap 80s gated room tail (DS 3 128 .5)")
    emit("#    Bus 4: Synth brass & riser stereo wash (DS 4 128 .75, DP 4 1)")
    emit("#")
    emit("#  ARRANGEMENT (96 bars):")
    emit("#    0-3   Intro: Alison's vocal laugh FX + 16th bassline enters alone")
    emit("#    4-7   Signature synth brass hook explodes in (Db5 - C5 - Bb4 - F4/F5)")
    emit("#    8-15  Full electronic drums + Vince Clarke 16th arpeggio joins")
    emit("#    16-31 Verse 1: vocal lead enters, staccato brass stabs on offbeats")
    emit("#    32-47 Chorus 1: Gb - Ab - Bbm - Db chord progression, soaring vocal lead")
    emit("#    48-55 Post-Chorus: arpeggio hook & brass hook front & centre, Simmons fills")
    emit("#    56-71 Verse 2: stripped back groove, vocal lead verse 2, filter build")
    emit("#    72-87 Chorus 2 & Climax: full wall of sound, energetic electronic funk")
    emit("#    88-95 Outro: arpeggio vamp & vocal refrain over driving bass, echo tail")
    emit("#    96    End cleanup")
    emit("# ======================================================================")
    emit()
    emit("[YAZOO - DONT GO] puts")
    emit("M 128 16")
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
    emit("# Bus 1: Arp & Lead dotted-eighth ping-pong echo")
    emit("DS 1 128 .75")
    emit("DL 1 - - 8 3 3 10")
    emit("DD 1 7 3")
    emit("DP 1 1")
    emit()
    emit("# Bus 2: Bass slapback sixteenth (tight analog bounce)")
    emit("DS 2 128 .25")
    emit("DL 2 - - 4 0 0 8")
    emit("DD 2 6 3")
    emit()
    emit("# Bus 3: Gated snare / clap 80s room tail")
    emit("DS 3 128 .5")
    emit("DL 3 - - 7 0 0 9")
    emit("DD 3 5 4")
    emit()
    emit("# Bus 4: Synth brass hall wash")
    emit("DS 4 128 .75")
    emit("DL 4 - - 10 4 8 10")
    emit("DD 4 8 2")
    emit("DP 4 1")
    emit()

    # ----------------------------------------------------
    # Voices Setup
    # ----------------------------------------------------
    emit("## VOICES")
    emit("# Drums: four-on-the-floor kick + sub punch")
    emit("v0 w82 f440 a7 p0 [kick electronic] vt")
    emit("v5 w0 a-2 p0 t.001,.25,0,.08 j1 k160 q.7 [kick sub punch] vt")
    emit()
    emit("# Crisp 80s snare, claps, hats, cowbell, crash")
    emit("v1 w83 f440 a8 p0 r3 ds.25 [snare 80s gated] vt")
    emit("v2 w84 f440 a-3 p.25 [closed hat] vt")
    emit("v3 w85 f440 a-2 p-.25 [open hat] vt")
    emit("v4 w86 f440 a7 p0 [electronic clap] vt")
    emit("v15 w96 f440 a3 p.2 [cowbell] vt")
    emit("v17 w94 f440 a4 p.2 [crash cymbal] vt")
    emit()
    emit("# Simmons Electronic Toms (hi, mid, lo)")
    emit("v18 w90 f480 a6 p-.3 [hi simmons tom] vt")
    emit("v19 w91 f380 a6 p0 [mid simmons tom] vt")
    emit("v20 w92 f280 a6 p.3 [lo simmons tom] vt")
    emit()
    emit("# Vince Clarke 16th Bass: snappy saw (centre) + detuned square (right), Character 2 drive")
    emit("v6 w15 a-1 p0 t.001,.15,0,.04 j1,2 k520 q5 g.015 r2 ds.08 [vince clarke bass saw] vt")
    emit("v6 >7")
    emit("v7 w16 a-5 N0,8 p.4 r0 [vince bass square] vt")
    emit("v6 G7 H7")
    emit()
    emit("# The Iconic Synth Brass Hook (centre saw + detuned left/right stereo pair, delay Bus 4)")
    emit("# Envelope has s=0 (decay to 0) so notes naturally articulate and never hang!")
    emit("v8 w15 a-6 p0 t.002,.20,0,.08 j1,2 k2800 q2.5 c4,.25 r4 ds.3 [synth brass centre] vt")
    emit("v8 >9")
    emit("v8 >10")
    emit("v9 w15 a-8 N0,12 p-.55 r0 [synth brass left] vt")
    emit("v10 w16 a-9 N0,-12 p.55 r0 [synth brass right] vt")
    emit("v8 G9,10 H9,10")
    emit()
    emit("# Arpeggio Lead: bright detuned saw/pulse pair with phase distortion c4, delay Bus 1")
    emit("# Uses s=0 envelope for sharp, rhythmic Vince Clarke 16th staccato")
    emit("v11 w15 a-6 p0 t.002,.18,0,.08 j1 k3800 q2.2 c4,.25 r1 ds.35 [arpeggio lead saw] vt")
    emit("v11 >12")
    emit("v12 w16 a-9 N0,9 p.55 r0 [arpeggio partner pulse] vt")
    emit("v11 G12 H12")
    emit()
    emit("# Alison Moyet Vocal Lead: rich, singing blues lead synth with subtle glide")
    emit("# Uses s=0 envelope (decay to 0) to avoid any stuck notes during arrangement transitions")
    emit("v13 w15 a-6 p0 t.003,.28,0,.10 j1,2 k3200 q1.8 g.02 c4,.20 r1 ds.3 [alison vocal lead] vt")
    emit("v13 >14")
    emit("v14 w15 a-8 N0,10 p-.5 r0 [alison vocal partner] vt")
    emit("v13 G14 H14")
    emit()
    emit("# Alison Moyet Vocal Laugh / Cry FX (resonant formant pitch sweep)")
    emit("v21 w15 a6 p0 t.01,.60,0,.25 j1,2 k1600 q4 c2,.35 g.04 [alison laugh cry fx] vt")
    emit()
    emit("# Riser: stretched K-synth one-shot")
    emit("v16 w301 f140 a8 p0 t.005,.01,1,.4 r4 ds.3 [ksynth build riser] vt")
    emit()
    emit("# Sub boom: audible sub drop + slowed kick")
    emit("v38 w0 a0 p0 t.002,.90,0,.40 j1 k240 q.7 r3 ds.3 [sub boom sine] vt")
    emit("v40 w82 f110 a5 p0 [slowed kick drop] vt")
    emit()
    emit("# Pad: Poly pool, 2-voice prototype (detuned stereo saw pair), 5 instances at v28-v37")
    emit("v24 w0 f.25 a0 t0,0,1,0 m1 [pad lfo] vt")
    emit("v26 w15 a-10 p-.5 t1.2,.4,0,1.4 j1 k2000 q.8 c4,.2 C24,.2 G27 H27 [pad proto left] vt")
    emit("v27 w15 a-12 p.5 N0,8 t1.2,.4,0,1.4 j1 k2000 q.8 c4,.2 C24,.2 [pad proto right] vt")
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
    # Streams (Defined using [ ... ] n>d with '-' for rests!)
    # ----------------------------------------------------
    emit("## STREAMS (Note arrays converted via n>d with '-' rests)")
    emit("# Stream 1: Vince Clarke 16th Bassline in Bb minor (2 bars / 32 sixteenths)")
    emit("# Notice '-' rests providing the authentic syncopated bounce!")
    emit("[Bb1 Bb1 Bb2 Bb1  Bb1 Bb1 Bb2 Bb1  Bb1 Bb1 Bb2 Bb1  Ab1 - Ab1 -  Bb1 Bb1 Bb2 Bb1  Bb1 Bb1 Bb2 Bb1  Bb1 Bb1 Bb2 Bb1  Db2 - Eb2 -] n>d /SS 1")
    emit()
    emit("# Stream 2: Chorus Bassline Roots (Gb -> Ab -> Bbm -> Db)")
    emit("[Gb1 Gb1 Gb2 Gb1  Gb1 Gb1 Gb2 Gb1  Ab1 Ab1 Ab2 Ab1  Ab1 Ab1 Ab2 Ab1  Bb1 Bb1 Bb2 Bb1  Bb1 Bb1 Bb2 Bb1  Db2 Db2 Db3 Db2  Db2 Db2 Db3 Db2] n>d /SS 2")
    emit()
    emit("# Stream 3: The Famous Brass Hook (2 bars / 32 steps in Bb minor)")
    emit("# Bar 1 has F4, Bar 2 leaps up to high F5!")
    emit("[Db5 - Db5 - C5 - C5 - Bb4 F4 - F4 - Ab4 Bb4 Ab4  Db5 - Db5 - C5 - C5 - Bb4 F5 - F5 - Ab4 Bb4 Ab4] n>d /SS 3")
    emit()
    emit("# Stream 4: Vocal Lead Chorus Hook ('Baby don't go, baby don't go...')")
    emit("[Db5 C5 Bb4 - Db5 C5 Bb4 - F5 - Eb5 Db5 C5 Bb4 - -  Db5 C5 Bb4 - Db5 C5 Bb4 - F5 - Ab5 Gb5 F5 Eb5 - -] n>d /SS 4")
    emit()
    emit("# Stream 5: Vocal Lead Verse 1 ('Came in from the city, walked into the door...')")
    emit("[Bb4 Bb4 Bb4 Bb4 Ab4 F4 - - F4 Ab4 Bb4 - - - - -  Bb4 Bb4 Bb4 Bb4 Ab4 F4 - - F4 Ab4 Bb4 - - - - -  Db5 Db5 Db5 Db5 C5 Ab4 - - Ab4 C5 Db5 - - - - -  Eb5 Eb5 Eb5 Eb5 Db5 Bb4 - - F5 - - - - - - -] n>d /SS 5")
    emit()
    emit("# Stream 6: Vince Clarke 16th Arpeggio (4 bars / 64 steps)")
    emit("[Bb3 Bb3 Bb3 - C4 Bb3 Bb3 Db4 Bb3 Eb4 Bb3 Bb3 F4 Bb3 Eb4 Db4  Bb3 Bb3 C4 Bb3 Db4 Bb3 Bb3 Eb4 Bb3 F4 Bb3 Bb3 Eb4 Bb3 Db4 Eb4  Ab3 - Ab3 Ab3 Bb3 Ab3 Ab3 C4 Ab3 Db4 Ab3 Ab3 Eb4 Ab3 Db4 C4  Gb3 - Gb3 Gb3 Ab3 Gb3 Gb3 Bb3 Gb3 C4 Gb3 Gb3 Db4 Gb3 C4 Bb3] n>d /SS 6")
    emit()

    # ----------------------------------------------------
    # Patterns
    # ----------------------------------------------------
    emit("## PATTERNS")
    emit()

    # Pattern 1: Four-on-the-floor Electronic Kick
    emit("y1")
    emit("[kick 4-on-the-floor] yt")
    emit("[v0 l1 v5 g0 n52 g.06 n30 l1] x0")
    emit("[v0 l.95 v5 g0 n50 g.06 n30 l.9] x4")
    emit("[v0 l1 v5 g0 n52 g.06 n30 l1] x8")
    emit("[v0 l.95 v5 g0 n50 g.06 n30 l.9] x12")
    emit("[v0 l.7] x14")
    emit("[=127,0] x15")
    emit()

    # Pattern 2: Gated Snare + Clap on 2 & 4
    emit("y2")
    emit("[snare+clap on 2 & 4] yt")
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

    # Pattern 18: Hats 16ths Groove with Offbeat Open Hats
    emit("y18")
    emit("[hats 16ths + open sizzle] yt")
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

    # Pattern 15: Syncopated Cowbell
    emit("y15")
    emit("[syncopated cowbell] yt")
    emit("[v15 l.85] x2")
    emit("[v15 l.65] x5")
    emit("[v15 l.80] x8")
    emit("[v15 l.65] x11")
    emit("[v15 l.90] x14")
    emit("[=127,0] x15")
    emit()

    # Pattern 4: Vince Clarke 16th Bassline (reads Stream 1 via n&1, 32 steps)
    emit("y4")
    emit("[vince clarke 16th bassline] yt")
    for step in range(32):
        emit(f"[v6 n&1 l1] x{step}")
    emit("[=127,0] x31")
    emit()

    # Pattern 8: Chorus Bass Roots (reads Stream 2 via n&2, 32 steps)
    emit("y8")
    emit("[chorus bass roots] yt")
    for step in range(32):
        emit(f"[v6 n&2 l1] x{step}")
    emit("[=127,0] x31")
    emit()

    # Pattern 5: The Iconic Synth Brass Hook (reads Stream 3 via n&3, 32 steps)
    # Rests in Stream 3 cleanly release v8 and suppress gate re-triggers
    emit("y5")
    emit("[the iconic synth brass hook] yt")
    for step in range(32):
        emit(f"[v8 n&3 l.95] x{step}")
    emit("[=127,0] x31")
    emit()

    # Pattern 6: Vince Clarke 16th Arpeggio (reads Stream 6 via n&6, 64 steps)
    emit("y6")
    emit("[vince clarke 16th arpeggio] yt")
    for step in range(64):
        emit(f"[v11 n&6 l.85] x{step}")
    emit("[=127,0] x63")
    emit()

    # Pattern 7: Vocal Lead Verse 1 (reads Stream 5 via n&5, 64 steps)
    emit("y7")
    emit("[vocal lead verse 1] yt")
    for step in range(64):
        emit(f"[v13 n&5 l.9] x{step}")
    emit("[=127,0] x63")
    emit()

    # Pattern 10: Vocal Lead Chorus Hook ('Baby don't go...', reads Stream 4, 32 steps)
    emit("y10")
    emit("[vocal lead chorus hook] yt")
    for step in range(32):
        emit(f"[v13 n&4 l.95] x{step}")
    emit("[=127,0] x31")
    emit()

    # Pattern 9: Full Chorus Polyphonic Pad Chords (Gbmaj7 -> Ab7 -> Bbm -> Db)
    emit("y9")
    emit("[chorus polyphonic pad chords] yt")
    emit("[pn 0,101,54,.95 pn 0,102,58,.95 pn 0,103,61,.95 pn 0,104,65,.95] x0")
    emit("[pr 0,101 pr 0,102 pr 0,103 pr 0,104] x7")
    emit("[pn 0,105,56,.95 pn 0,106,60,.95 pn 0,107,63,.95 pn 0,108,66,.95] x8")
    emit("[pr 0,105 pr 0,106 pr 0,107 pr 0,108] x15")
    emit("[pn 0,109,58,.95 pn 0,110,61,.95 pn 0,111,65,.95 pn 0,112,68,.95] x16")
    emit("[pr 0,109 pr 0,110 pr 0,111 pr 0,112] x23")
    emit("[pn 0,113,61,.95 pn 0,114,65,.95 pn 0,115,68,.95 pn 0,116,72,.95] x24")
    emit("[pr 0,113 pr 0,114 pr 0,115 pr 0,116] x31")
    emit("[=127,0] x31")
    emit()

    # Pattern 13: Simmons Electronic Tom Fill (one-shot with stop marker '-')
    emit("y13")
    emit("[simmons electronic tom fill] yt")
    emit("[v18 l1] x8")
    emit("[v18 l1] x10")
    emit("[v19 l1] x12")
    emit("[v20 l1] x14")
    emit("[v20 l1] x15")
    emit("[-] x16")
    emit()

    # Pattern 14: Sub Boom Drop (one-shot with stop marker '-')
    emit("y14")
    emit("[sub boom drop] yt")
    emit("[v38 g0 n48 g.15 n36 l1 v40 l1] x0")
    emit("[-] x1")
    emit()

    # Pattern 16: Bass Cutoff Sweep Lane (4 bars, 32 two-step increments from 520 Hz to 2800 Hz)
    emit("y16")
    emit("[bass cutoff sweep] yt")
    f_start = 520.0
    f_end = 2800.0
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

    # Bars 0-3: Alison Moyet vocal laugh / cry FX + 16th bassline enters alone!
    c_add(0, "v21 g0 n77 g.10 n70 l1 y4 z1 /SP 1,0 v6 k520 v7 k520")
    c_add(1, "v21 g0 n75 g.08 n68 l.9")
    c_add(2, "v21 g0 n73 g.12 n65 l.85")
    c_add(3, "v21 g0 n77 g.20 n65 l1")

    # Bars 4-7: Signature Synth Brass Hook explodes in!
    c_add(4, "v17 l.8 y5 z1 /SP 3,0")

    # Bars 8-15: Full Electronic Drums + Vince Clarke 16th Arpeggio Joins In!
    c_add(8, "v17 l1 y1 z1 y2 z1 y18 z1 y15 z1 y6 z1 /SP 6,0")
    c_add(12, "v6 k650 v7 k650")
    c_add(15, "y13 z1")

    # Bars 16-31: Verse 1 (Alison Moyet vocal lead, staccato brass stabs)
    c_add(16, "v17 l.8 y6 z0 v11 l0 y7 z1 /SP 5,0 y3 z1 y18 z0 v6 k520 v7 k520")
    c_add(20, "y18 z1 y3 z0")
    c_add(24, "v17 l.6 /SP 5,0 y7 z1")
    c_add(28, "v16 l1 y16 z1")
    c_add(31, "y7 z0 v13 l0 y13 z1")

    # Bars 32-47: Chorus 1 ('Don't go... don't go...', wall of sound!)
    c_add(32, "v17 l1 y4 z0 y8 z1 /SP 2,0 y9 z1 y10 z1 /SP 4,0 y14 z1 y16 z0 v6 k2600 v7 k2400")
    c_add(36, "v17 l.8 /SP 2,0 y8 z1 /SP 4,0 y10 z1")
    c_add(40, "v17 l.8 /SP 2,0 y8 z1 /SP 4,0 y10 z1")
    c_add(44, "y8 z0 y4 z1 /SP 1,0 y10 z0 v13 l0 v6 k800 v7 k800")
    c_add(47, "y13 z1")

    # Bars 48-55: Post-Chorus Brass Hook & Arpeggio Front & Centre
    c_add(48, "v17 l1 y9 z0 y5 z1 /SP 3,0 y6 z1 /SP 6,0")
    c_add(52, "v17 l.8")
    c_add(55, "y13 z1")

    # Bars 56-71: Verse 2 (Stripped electronic funk, vocal lead verse 2, sweep)
    c_add(56, "v17 l.7 y5 z0 v8 l0 y6 z0 v11 l0 y7 z1 /SP 5,0 y18 z0 y3 z1 v6 k520 v7 k520")
    c_add(60, "y18 z1 y3 z0")
    c_add(64, "v17 l.6 /SP 5,0 y7 z1 y16 z1")
    c_add(68, "v16 l1")
    c_add(71, "y7 z0 v13 l0 y13 z1")

    # Bars 72-87: Chorus 2 & Climax
    c_add(72, "v17 l1 y4 z0 y8 z1 /SP 2,0 y9 z1 y10 z1 /SP 4,0 y14 z1 y16 z0 v6 k2800 v7 k2600")
    c_add(76, "v17 l.8 /SP 2,0 y8 z1 /SP 4,0 y10 z1")
    c_add(80, "v17 l.8 /SP 2,0 y8 z1 /SP 4,0 y10 z1")
    c_add(84, "y8 z0 y4 z1 /SP 1,0 y5 z1 /SP 3,0 y6 z1 /SP 6,0 v6 k900 v7 k900")
    c_add(87, "y13 z1")

    # Bars 88-95: Outro / Extended Synth Vamp
    c_add(88, "v17 l1 y9 z0 y10 z1 /SP 4,0 y1 z0 y2 z0 y18 z0 y15 z0")
    c_add(90, "v17 l.6 y1 z1 y2 z1")
    c_add(92, "y10 z0 v13 l0 y1 z0 y2 z0")
    c_add(94, "y5 z0 y6 z0 y4 z0 v6 l0 v7 l0 v8 l0 v11 l0 v13 l0")

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

    return "\n".join(lines)

if __name__ == "__main__":
    song_text = build_dont_go()
    out_path = os.path.join(os.path.dirname(__file__), "dont_go.sk")
    with open(out_path, "w") as f:
        f.write(song_text)
    print(f"Wrote {len(song_text.splitlines())} lines to {out_path}")
