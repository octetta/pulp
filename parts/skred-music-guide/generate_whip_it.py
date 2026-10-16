#!/usr/bin/env python3
"""
generate_whip_it.py - Generates whip_it.sk for PULP / SKRED.
An authentic, kick-ass instrumental cover of DEVO's 1980 classic "Whip It".
Written by Mark Mothersbaugh and Gerald Casale.

Timing & Rest Architecture:
- 158 BPM (M 158 16), where 1 bar = 16 sixteenth steps.
- Main Riff: Authentic 2-bar call-and-response groove:
  Bar A (16 steps): E2 on 0, E2 on 2, G#2 on 4, B2 on 6, D3 on 8, with 7 steps of rest on the back half!
    Stream 1: [E2 - E2 - G#2 - B2 - D3 - - - - - - -] n>d
  Bar B1 (16 steps): D3 on 0 (stabs), WHIP CRACK on 4, A2 on 8 (stabs), WHIP CRACK on 12!
    Stream 2: [D3 - - - - - - - A2 - - - - - - -] n>d
  Bar B2 (16 steps): Turnaround walking bass into E: D3 on 0, A2 on 8, A2 on 10, G2 on 12, F#2 on 14!
    Stream 3: [D3 - - - - - - - A2 - A2 - G2 - F#2 -] n>d
- Verse 1 Vocals ("Crack that whip!"): 8 bars / 128 steps with spacious rests between calls.
  Whip cracks land on the beat right after each call!
- Pre-chorus ("When a problem comes along..."): 8 bars / 128 steps with whip crack on "whip it!".
- Chorus ("Now whip it! Into shape!"): C -> G -> D -> C (4 bars / 64 steps), driving 8th roots,
  polyphonic synth chords, and vocal lead.
- Bob Mothersbaugh's octave guitar/synth solo: 8 bars / 128 steps with authentic rests.
- Slowed snare whip crack (w83 at f175) layered with sharp rim transient (w87) and lash tail (w5).
"""

import os, sys

def build_whip_it():
    lines = []
    def emit(s=""):
        lines.append(s)

    # ----------------------------------------------------
    # Header & Metadata
    # ----------------------------------------------------
    emit("## DEVO - WHIP IT (Full Instrumental Cover for PULP / SKRED)")
    emit("#  158 BPM, E Mixolydian / E Major, 92 bars (~2:20).")
    emit("#  Self-contained: drums from built-in waves 82-99, inline K-synth riser.")
    emit("#  Uses slowed snare sample (w83 at f175) for the authentic whip crack sound!")
    emit("#  Uses n>d to convert note name strings (with '-' rests) to MIDI streams.")
    emit("#  Load:   [whip_it.sk] /ls")
    emit("#  Start:  play        Stop:  stop")
    emit("#")
    emit("#  VOICE MAP:")
    emit("#    v0  motorik electronic kick (w82)")
    emit("#    v1  crisp 80s snare (w83) + v4 electronic clap (w86)")
    emit("#    v2  closed hat (w84)  v3  open hat (w85)  v5  crash cymbal (w94)")
    emit("#    v6  Moog bass saw (w15, centre)")
    emit("#    v7  Moog bass partner square (w16, right, linked to v6)")
    emit("#    v8  guitar-style chug (w16, left, linked to v6)")
    emit("#    v11 guitar / synth solo voice (w15) + v12 partner pulse (w16, linked to v11)")
    emit("#    v13 vocal lead synth (w15) + v14 partner saw (w15, linked to v13)")
    emit("#    v16 K-synth build riser (w301)")
    emit("#    v21 slowed snare whip crack (w83 at f175, delay Bus 1)")
    emit("#    v22 whip initial transient crack (w87)")
    emit("#    v23 whip leather lash tail (w5 noise)")
    emit("#    v24 pad LFO  v26,v27 pad prototype  v28-v37 poly pool (chorus synth chords)")
    emit("#")
    emit("#  DELAY BUSES:")
    emit("#    Bus 1: Whip crack & lead slapback (DS 1 158 .25, DL 1 - - 5 0 0 9)")
    emit("#    Bus 2: Bass slapback sixteenth (DS 2 158 .25, DL 2 - - 4 0 0 8)")
    emit("#    Bus 3: Gated snare room (DS 3 158 .5, DL 3 - - 6 0 0 9)")
    emit("#    Bus 4: Synth chorus wash (DS 4 158 .75, DL 4 - - 10 4 8 10, DP 4 1)")
    emit("#")
    emit("#  ARRANGEMENT (92 bars):")
    emit("#    0-3   Intro drums (Alan Myers motorik beat enters)")
    emit("#    4-11  Iconic 2-bar riff enters (A -> B1 -> A -> B2) with whip cracks on 2 & 4")
    emit("#    12-19 Verse 1: 'Crack that whip!' + whip cracks after vocal calls")
    emit("#    20-27 Pre-chorus 1: 'When a problem comes along, you must whip it!'")
    emit("#    28-35 Chorus 1: C - G - D - C progression, synth chords + whip cracks")
    emit("#    36-43 Return to main riff + whip accents")
    emit("#    44-51 Verse 2: 'When something's going wrong...'")
    emit("#    52-59 Pre-chorus 2 + riser sweep")
    emit("#    60-67 Chorus 2")
    emit("#    68-75 Bob Mothersbaugh's angular synth/guitar solo")
    emit("#    76-83 Final Chorus: full explosive wall of sound, continuous whip snaps")
    emit("#    84-91 Outro vamp & final whip crack flurry")
    emit("#    92    End cleanup")
    emit("# ======================================================================")
    emit()
    emit("[DEVO - WHIP IT] puts")
    emit("M 158 16")
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
    emit("# Bus 1: Whip crack & lead slapback")
    emit("DS 1 158 .25")
    emit("DL 1 - - 5 0 0 9")
    emit("DD 1 8 3")
    emit()
    emit("# Bus 2: Bass slapback sixteenth")
    emit("DS 2 158 .25")
    emit("DL 2 - - 4 0 0 8")
    emit("DD 2 6 3")
    emit()
    emit("# Bus 3: Snare room tail")
    emit("DS 3 158 .5")
    emit("DL 3 - - 6 0 0 9")
    emit("DD 3 5 4")
    emit()
    emit("# Bus 4: Synth chorus wash")
    emit("DS 4 158 .75")
    emit("DL 4 - - 10 4 8 10")
    emit("DD 4 8 2")
    emit("DP 4 1")
    emit()

    # ----------------------------------------------------
    # Voices Setup
    # ----------------------------------------------------
    emit("## VOICES")
    emit("# Motorik drums (Alan Myers style)")
    emit("v0 w82 f440 a7 p0 [kick motorik] vt")
    emit("v1 w83 f440 a8 p0 r3 ds.25 [snare 80s] vt")
    emit("v2 w84 f440 a-3 p.25 [closed hat] vt")
    emit("v3 w85 f440 a-2 p-.25 [open hat] vt")
    emit("v4 w86 f440 a6 p0 [electronic clap] vt")
    emit("v5 w94 f440 a4 p.2 [crash cymbal] vt")
    emit()
    emit("# Moog bass + guitar trio: v6 (centre saw), v7 (right square), v8 (left guitar chug)")
    emit("# Linked via G and H so v7 and v8 track v6 note and gate automatically!")
    emit("v6 w15 a-1 p0 t.001,.18,0,.04 j1,2 k1800 q3.5 r2 ds.08 [moog bass saw] vt")
    emit("v6 >7")
    emit("v7 w16 a-5 N0,7 p.4 r0 [moog bass square] vt")
    emit("v6 >8")
    emit("v8 w16 a-6 N0,0 p-.35 t.001,.15,0,.03 j1 k3200 q1 c4,.15 [guitar chug] vt")
    emit("v6 G7 H7 G8 H8")
    emit()
    emit("# DEVO Solo Lead: bright saw/pulse pair with phase distortion c4, delay Bus 1")
    emit("v11 w15 a-6 p0 t.002,.20,0,.10 j1 k3800 q2.2 c4,.25 r1 ds.35 [solo lead saw] vt")
    emit("v11 >12")
    emit("v12 w16 a-9 N12,9 p.55 r0 [solo partner pulse] vt")
    emit("v11 G12 H12")
    emit()
    emit("# Vocal Lead Synth: Mark Mothersbaugh / Gerald Casale vocal line")
    emit("# Uses s=0 envelope (decay to 0) so vocal notes never hang or get stuck!")
    emit("v13 w15 a-6 p0 t.003,.26,0,.08 j1,2 k3000 q1.8 g.015 c4,.20 r1 ds.3 [vocal lead synth] vt")
    emit("v13 >14")
    emit("v14 w15 a-8 N0,10 p-.5 r0 [vocal partner saw] vt")
    emit("v13 G14 H14")
    emit()
    emit("# ====================================================================")
    emit("# THE WHIP CRACK: Snare played at a slower rate (w83 at f175)")
    emit("# Layered with sharp rim crack (w87) and noise lash tail (w5)")
    emit("# ====================================================================")
    emit("v21 w83 f175 a9 p0 r1 ds.35 [whip slowed snare] vt")
    emit("v22 w87 f350 a7 p-.1 [whip rim crack] vt")
    emit("v23 w5 f1400 a-3 p.2 t.001,.09,0,.03 j2 k2800 q2 [whip noise lash] vt")
    emit()
    emit("# Riser: stretched K-synth one-shot")
    emit("v16 w301 f160 a8 p0 t.005,.01,1,.4 r4 ds.3 [ksynth riser] vt")
    emit()
    emit("# Pad: Poly pool, 2-voice prototype (detuned stereo saw pair), 5 instances at v28-v37")
    emit("v24 w0 f.30 a0 t0,0,1,0 m1 [pad lfo] vt")
    emit("v26 w15 a-10 p-.5 t1.0,.4,.8,1.2 j1 k2200 q.8 c4,.2 C24,.2 G27 H27 [pad proto left] vt")
    emit("v27 w15 a-12 p.5 N0,8 t1.0,.4,.8,1.2 j1 k2200 q.8 c4,.2 C24,.2 [pad proto right] vt")
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
    emit("# Stream 1: Bar A (16 sixteenth steps) - DUN-DUN da-da-DUN!")
    emit("# Notes on 0, 2, 4, 6, 8, with 7 steps of rest on the back half!")
    emit("[E2 - E2 - G#2 - B2 - D3 - - - - - - -] n>d /SS 1")
    emit()
    emit("# Stream 2: Bar B1 (16 sixteenth steps) - D stab on 1, A stab on 3")
    emit("[D3 - - - - - - - A2 - - - - - - -] n>d /SS 2")
    emit()
    emit("# Stream 3: Bar B2 (16 sixteenth steps) - Walking bass fill into E")
    emit("[D3 - - - - - - - A2 - A2 - G2 - F#2 -] n>d /SS 3")
    emit()
    emit("# Stream 5: Verse 1 Vocal Line ('Crack that whip! ... Give the past a slip!')")
    emit("# 8 bars / 128 sixteenth steps with natural rests between calls")
    emit("[")
    emit("- - - - - - - - - - - - - - E3 -")
    emit("E5 - - - E5 - - - - - - - - - - -")
    emit("- - - - - - - - - - - - - - D5 -")
    emit("- - D5 - D5 - D5 - E5 - - - - - - -")
    emit("- - - - - - - - - - - - - - D5 -")
    emit("- - D5 - D5 - E5 - - - - - - - - -")
    emit("- - - - - - - - - - - - - - D5 -")
    emit("- - D5 - E5 - D5 - E5 - - - - - - -")
    emit("] n>d /SS 5")
    emit()
    emit("# Stream 6: Pre-Chorus Vocal Line ('When a problem comes along... you must whip it!')")
    emit("# 8 bars / 128 sixteenth steps")
    emit("[")
    emit("- - - - - - - - - - - - B4 - A#4 -")
    emit("B4 - A#4 - B4 - A#4 - B4 - - - - - D5 -")
    emit("- - D5 - C#5 - D5 - - - - - B4 - B4 -")
    emit("A#4 - B4 - A#4 - B4 - A#4 - B4 - - - F#5 -")
    emit("- - F#5 - E5 - D5 - - - - - B4 - A#4 -")
    emit("B4 - A#4 - B4 - A#4 - B4 - - - - - D5 -")
    emit("- - D5 - C#5 - D5 - - - - - B4 - B4 -")
    emit("A#4 - B4 - A#4 - B4 - - - - - - - - -")
    emit("] n>d /SS 6")
    emit()
    emit("# Stream 7: Chorus Bass roots (C -> G -> D -> C, 4 bars / 64 sixteenth steps)")
    emit("[C2 - C2 - C2 - C2 - C2 - C2 - C2 - C2 -  G1 - G1 - G1 - G1 - G1 - G1 - G1 - G1 -  D2 - D2 - D2 - D2 - D2 - D2 - D2 - D2 -  C2 - C2 - C2 - C2 - C2 - C2 - C2 - C2 -] n>d /SS 7")
    emit()
    emit("# Stream 8: Chorus Vocal Hook ('Now whip it! Into shape! Shape it up, get straight!...')")
    emit("# 4 bars / 64 sixteenth steps")
    emit("[")
    emit("- - - - - - C4 - C5 - C4 - - - - -")
    emit("- - - - - - - - D5 - D4 - D5 - - -")
    emit("- - - - - - D4 - D5 - D4 - D5 - D5 -")
    emit("- - - - - - - - - C5 - - C5 - - -")
    emit("] n>d /SS 8")
    emit()
    emit("# Stream 9: Bob Mothersbaugh's Angular Synth/Guitar Solo (8 bars / 128 steps)")
    emit("[")
    emit("- - - - - - - - - - - - - - - -")
    emit("- - - - A5 - G5 - B5 - G5 - - - - -")
    emit("- - - - - - - - - - - - - - - -")
    emit("- - - - B5 - B5 - G5 - - - - - - -")
    emit("- - - - - - - - - - - - - - - -")
    emit("- - - - A5 - G5 - B5 - G5 - - - - -")
    emit("- - - - - - - - - - - - - - - -")
    emit("- - - - - - G5 - B5 - B5 - G5 - - -")
    emit("] n>d /SS 9")
    emit()

    # ----------------------------------------------------
    # Patterns
    # ----------------------------------------------------
    emit("## PATTERNS")
    emit()

    # Pattern 1: Alan Myers Motorik Four-on-the-Floor Kick
    emit("y1")
    emit("[kick motorik] yt")
    emit("[v0 l1] x0")
    emit("[v0 l.9] x4")
    emit("[v0 l1] x8")
    emit("[v0 l.95] x12")
    emit("[=127,0] x15")
    emit()

    # Pattern 2: Crisp Gated Snare + Clap on 2 & 4
    emit("y2")
    emit("[snare+clap on 2 & 4] yt")
    emit("[v1 l1 v4 l.9] x4")
    emit("[v1 l1 v4 l.9] x12")
    emit("[v1 l.25] x14")
    emit("[=127,0] x15")
    emit()

    # Pattern 3: Closed Hats 8ths with Offbeat Open Hats
    emit("y3")
    emit("[hats 8ths + open sizzle] yt")
    emit("[v2 l.5] x0")
    emit("[v2 l.3] x2")
    emit("[v2 l.5] x4")
    emit("[v3 l.75 v2 l0] x6")
    emit("[v2 l.5] x8")
    emit("[v2 l.3] x10")
    emit("[v2 l.5] x12")
    emit("[v3 l.85 v2 l0] x14")
    emit("[=127,0] x15")
    emit()

    # Pattern 4: Relentless 16th Hats (High energy sections)
    emit("y4")
    emit("[hats 16ths groove] yt")
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

    # Pattern 6: Main Riff Bar A (16 steps, reads Stream 1)
    # DUN-DUN da-da-DUN on 0, 2, 4, 6, 8, rests on 9-15!
    emit("y6")
    emit("[main riff bar A] yt")
    for step in range(16):
        emit(f"[v6 n&1 l1] x{step}")
    emit("[=127,0] x15")
    emit()

    # Pattern 7: Main Riff Bar B1 (16 steps, reads Stream 2)
    # D stab on 0, WHIP CRACK on 4, A stab on 8, WHIP CRACK on 12!
    emit("y7")
    emit("[main riff bar B1 with whip cracks] yt")
    for step in range(16):
        if step == 4 or step == 12:
            emit(f"[v6 n&2 l1 v21 l1 v22 l1 v23 l1] x{step}")
        else:
            emit(f"[v6 n&2 l1] x{step}")
    emit("[=127,0] x15")
    emit()

    # Pattern 8: Main Riff Bar B2 (16 steps, turnaround walking bass into E)
    # D stab on 0, WHIP CRACK on 4, A on 8 & 10, G on 12, F# on 14!
    emit("y8")
    emit("[main riff bar B2 turnaround] yt")
    for step in range(16):
        if step == 4:
            emit(f"[v6 n&3 l1 v21 l1 v22 l1 v23 l1] x{step}")
        else:
            emit(f"[v6 n&3 l1] x{step}")
    emit("[=127,0] x15")
    emit()

    # Pattern 9: Verse 1 Vocal Line ('Crack that whip! ...')
    # 8 bars / 128 sixteenth steps, reads Stream 5 with rests!
    # Whip cracks placed on beat 4 right after the calls!
    emit("y9")
    emit("[verse 1 vocal lead + whip cracks] yt")
    whip_verse_steps = {20, 60, 84, 124}
    for step in range(128):
        if step in whip_verse_steps:
            emit(f"[v13 n&5 l1 v21 l1 v22 l1 v23 l1] x{step}")
        else:
            emit(f"[v13 n&5 l1] x{step}")
    emit("[=127,0] x127")
    emit()

    # Pattern 10: Pre-Chorus Vocal Line ('When a problem comes along... you must whip it!')
    # 8 bars / 128 sixteenth steps, reads Stream 6
    # Whip cracks on "whip it!" at steps 28, 60, 92, 124!
    emit("y10")
    emit("[pre-chorus vocal lead + whip cracks] yt")
    whip_pre_steps = {28, 60, 92, 124}
    for step in range(128):
        if step in whip_pre_steps:
            emit(f"[v13 n&6 l1 v21 l1 v22 l1 v23 l1] x{step}")
        else:
            emit(f"[v13 n&6 l1] x{step}")
    emit("[=127,0] x127")
    emit()

    # Pattern 11: Chorus Bass Roots (64 sixteenth steps across 4 bars, reads Stream 7)
    emit("y11")
    emit("[chorus bass roots 4 bars] yt")
    for step in range(64):
        emit(f"[v6 n&7 l1] x{step}")
    emit("[=127,0] x63")
    emit()

    # Pattern 12: Bob Mothersbaugh's Angular Synth Solo (reads Stream 9)
    # 8 bars / 128 sixteenth steps
    emit("y12")
    emit("[devo synth solo 8 bars] yt")
    for step in range(128):
        emit(f"[v11 n&9 l.95] x{step}")
    emit("[=127,0] x127")
    emit()

    # Pattern 13: Chorus Synth Chords (Cmaj -> Gmaj -> Dmaj -> Cmaj, 4 bars / 64 steps)
    emit("y13")
    emit("[chorus polyphonic synth chords] yt")
    # C major (bar 1: steps 0-15)
    emit("[pn 0,101,48,.95 pn 0,102,52,.95 pn 0,103,55,.95] x0")
    emit("[pr 0,101 pr 0,102 pr 0,103] x15")
    # G major (bar 2: steps 16-31)
    emit("[pn 0,104,43,.95 pn 0,105,47,.95 pn 0,106,50,.95] x16")
    emit("[pr 0,104 pr 0,105 pr 0,106] x31")
    # D major (bar 3: steps 32-47)
    emit("[pn 0,107,45,.95 pn 0,108,50,.95 pn 0,109,54,.95] x32")
    emit("[pr 0,107 pr 0,108 pr 0,109] x47")
    # C major (bar 4: steps 48-63)
    emit("[pn 0,101,48,.95 pn 0,102,52,.95 pn 0,103,55,.95] x48")
    emit("[pr 0,101 pr 0,102 pr 0,103] x63")
    emit("[=127,0] x63")
    emit()

    # Pattern 14: Chorus Vocal Hook ('Now whip it! Into shape!...')
    # 4 bars / 64 sixteenth steps, reads Stream 8
    emit("y14")
    emit("[chorus vocal hook line] yt")
    for step in range(64):
        if step == 60:
            emit(f"[v13 n&8 l1 v21 l1 v22 l1 v23 l1] x{step}")
        else:
            emit(f"[v13 n&8 l1] x{step}")
    emit("[=127,0] x63")
    emit()

    # Pattern 15: Double Whip Crack for Section Turnarounds!
    emit("y15")
    emit("[double whip crack on 3 & 4] yt")
    emit("[v21 l1 v22 l1 v23 l1] x8")
    emit("[v21 l1 v22 l1 v23 l1] x12")
    emit("[=127,0] x15")
    emit()

    # Pattern 16: Snare Ratchet Roll (one-shot build)
    emit("y16")
    emit("[snare ratchet roll build] yt")
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

    # Pattern 17: Sidechain Ducking Pump (4 steps = 1 beat)
    emit("y17")
    emit("[sidechain ducking pump] yt")
    emit("[v28 a-18 v29 a-20 v30 a-18 v31 a-20 v8 a-16] x0")
    emit("[v28 a-12 v29 a-14 v30 a-12 v31 a-14 v8 a-12] x1")
    emit("[v28 a-8 v29 a-10 v30 a-8 v31 a-10 v8 a-8] x2")
    emit("[v28 a-5 v29 a-7 v30 a-5 v31 a-7 v8 a-5] x3")
    emit()

    # ----------------------------------------------------
    # Conductor (Pattern 0 at %16, 92 bars total)
    # ----------------------------------------------------
    emit("## CONDUCTOR (pattern 0, one step per bar, 92 bars)")
    emit("y0")
    emit("[conductor] yt")
    emit("%16")

    conductor = {}
    def c_add(bar, cmd):
        conductor.setdefault(bar, []).append(cmd)

    # Bars 0-3: Intro Drums (Alan Myers motorik beat enters)
    c_add(0, "y1 z1 y3 z1 v6 k1800 v7 k1800")
    c_add(2, "y2 z1")

    # Bars 4-11: Iconic Riff enters! (Bar A -> Bar B1 -> Bar A -> Bar B2)
    c_add(4, "v5 l1 y6 z1 /SP 1,0")
    c_add(5, "y6 z0 y7 z1 /SP 2,0")
    c_add(6, "y7 z0 y6 z1 /SP 1,0")
    c_add(7, "y6 z0 y8 z1 /SP 3,0")
    c_add(8, "v5 l.8 y8 z0 y6 z1 /SP 1,0 y4 z1 y3 z0")
    c_add(9, "y6 z0 y7 z1 /SP 2,0")
    c_add(10, "y7 z0 y6 z1 /SP 1,0")
    c_add(11, "y6 z0 y8 z1 /SP 3,0 y16 z1") # Snare roll build into Verse 1

    # Bars 12-19: Verse 1 ('Crack that whip!')
    # Underneath: Riff cycles A -> B1 -> A -> B2 -> A -> B1 -> A -> B2
    c_add(12, "v5 l1 y8 z0 y6 z1 /SP 1,0 y9 z1 /SP 5,0")
    c_add(13, "y6 z0 y7 z1 /SP 2,0")
    c_add(14, "y7 z0 y6 z1 /SP 1,0")
    c_add(15, "y6 z0 y8 z1 /SP 3,0")
    c_add(16, "v5 l.8 y8 z0 y6 z1 /SP 1,0")
    c_add(17, "y6 z0 y7 z1 /SP 2,0")
    c_add(18, "y7 z0 y6 z1 /SP 1,0")
    c_add(19, "y6 z0 y8 z1 /SP 3,0")

    # Bars 20-27: Pre-chorus 1 ('When a problem comes along, you must whip it!')
    c_add(20, "v5 l1 y8 z0 y6 z1 /SP 1,0 y9 z0 v13 l0 y10 z1 /SP 6,0")
    c_add(21, "y6 z0 y7 z1 /SP 2,0")
    c_add(22, "y7 z0 y6 z1 /SP 1,0")
    c_add(23, "y6 z0 y8 z1 /SP 3,0")
    c_add(24, "v5 l.8 y8 z0 y6 z1 /SP 1,0")
    c_add(25, "y6 z0 y7 z1 /SP 2,0")
    c_add(26, "y7 z0 y6 z1 /SP 1,0 v16 l1") # Riser sweep enters
    c_add(27, "y6 z0 y8 z1 /SP 3,0 y2 z0 y16 z1") # Snare roll build

    # Bars 28-35: Chorus 1 ('Now whip it! Into shape!')
    # C -> G -> D -> C progression (repeated twice)
    c_add(28, "v5 l1 y8 z0 y10 z0 v13 l0 y11 z1 /SP 7,0 y13 z1 y14 z1 /SP 8,0 y17 z1 y2 z1 v6 k2600 v7 k2400")
    c_add(32, "v5 l.8 /SP 7,0 y11 z1 y13 z1 /SP 8,0 y14 z1 y15 z1")
    c_add(35, "y2 z0 y16 z1")

    # Bars 36-43: Post-Chorus Return to Main Riff + Whip Accents
    c_add(36, "v5 l1 y11 z0 y13 z0 y14 z0 v13 l0 y17 z0 y6 z1 /SP 1,0 y2 z1 v6 k1800 v7 k1800")
    c_add(37, "y6 z0 y7 z1 /SP 2,0")
    c_add(38, "y7 z0 y6 z1 /SP 1,0")
    c_add(39, "y6 z0 y8 z1 /SP 3,0")
    c_add(40, "v5 l.8 y8 z0 y6 z1 /SP 1,0")
    c_add(41, "y6 z0 y7 z1 /SP 2,0")
    c_add(42, "y7 z0 y6 z1 /SP 1,0")
    c_add(43, "y6 z0 y8 z1 /SP 3,0 y16 z1")

    # Bars 44-51: Verse 2 ('When something's going wrong...')
    c_add(44, "v5 l1 y8 z0 y6 z1 /SP 1,0 y9 z1 /SP 5,0")
    c_add(45, "y6 z0 y7 z1 /SP 2,0")
    c_add(46, "y7 z0 y6 z1 /SP 1,0")
    c_add(47, "y6 z0 y8 z1 /SP 3,0")
    c_add(48, "v5 l.8 y8 z0 y6 z1 /SP 1,0")
    c_add(49, "y6 z0 y7 z1 /SP 2,0")
    c_add(50, "y7 z0 y6 z1 /SP 1,0")
    c_add(51, "y6 z0 y8 z1 /SP 3,0")

    # Bars 52-59: Pre-Chorus 2
    c_add(52, "v5 l1 y8 z0 y6 z1 /SP 1,0 y9 z0 v13 l0 y10 z1 /SP 6,0")
    c_add(53, "y6 z0 y7 z1 /SP 2,0")
    c_add(54, "y7 z0 y6 z1 /SP 1,0")
    c_add(55, "y6 z0 y8 z1 /SP 3,0")
    c_add(56, "v5 l.8 y8 z0 y6 z1 /SP 1,0")
    c_add(57, "y6 z0 y7 z1 /SP 2,0")
    c_add(58, "y7 z0 y6 z1 /SP 1,0 v16 l1") # Riser sweep
    c_add(59, "y6 z0 y8 z1 /SP 3,0 y2 z0 y16 z1")

    # Bars 60-67: Chorus 2
    c_add(60, "v5 l1 y8 z0 y10 z0 v13 l0 y11 z1 /SP 7,0 y13 z1 y14 z1 /SP 8,0 y17 z1 y2 z1 v6 k2600 v7 k2400")
    c_add(64, "v5 l.8 /SP 7,0 y11 z1 y13 z1 /SP 8,0 y14 z1 y15 z1")
    c_add(67, "y2 z0 y16 z1")

    # Bars 68-75: Bob Mothersbaugh's Angular Synth / Guitar Solo!
    # Main riff and motorik drums drive underneath!
    c_add(68, "v5 l1 y11 z0 y13 z0 y14 z0 v13 l0 y17 z0 y6 z1 /SP 1,0 y12 z1 /SP 9,0 y2 z1 v6 k1800 v7 k1800")
    c_add(69, "y6 z0 y7 z1 /SP 2,0")
    c_add(70, "y7 z0 y6 z1 /SP 1,0")
    c_add(71, "y6 z0 y8 z1 /SP 3,0")
    c_add(72, "v5 l.8 y8 z0 y6 z1 /SP 1,0")
    c_add(73, "y6 z0 y7 z1 /SP 2,0")
    c_add(74, "y7 z0 y6 z1 /SP 1,0")
    c_add(75, "y6 z0 y8 z1 /SP 3,0 y12 z0 y2 z0 y16 z1")

    # Bars 76-83: Final Chorus (Maximum Energy Wall of Sound + Continuous Whip Snaps!)
    c_add(76, "v5 l1 y8 z0 y11 z1 /SP 7,0 y13 z1 y14 z1 /SP 8,0 y17 z1 y2 z1 v6 k2800 v7 k2600")
    c_add(80, "v5 l.8 /SP 7,0 y11 z1 y13 z1 /SP 8,0 y14 z1 y15 z1")
    c_add(83, "y16 z1")

    # Bars 84-91: Outro Vamp & Final Whip Flurry
    c_add(84, "v5 l1 y11 z0 y13 z0 y14 z0 v13 l0 y17 z0 y6 z1 /SP 1,0 y15 z1")
    c_add(85, "y6 z0 y7 z1 /SP 2,0")
    c_add(86, "y7 z0 y6 z1 /SP 1,0 y15 z1")
    c_add(87, "y6 z0 y8 z1 /SP 3,0")
    c_add(88, "v5 l.8 y8 z0 y6 z1 /SP 1,0 y15 z1")
    c_add(89, "y6 z0 y7 z1 /SP 2,0")
    c_add(90, "y7 z0 y6 z1 /SP 1,0 y15 z1")
    c_add(91, "y1 z0 y2 z0 y4 z0 y6 z0 y7 z0 y8 z0 v13 l0 v11 l0 v21 l1 v22 l1 v23 l1")

    # Bar 92: End Cleanup
    c_add(92, "Z0 +4 ce 99")

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
    song_text = build_whip_it()
    guide_dir = os.path.dirname(__file__)
    for filename in ["whip_it.sk", "whip-it.sk"]:
        out_path = os.path.join(guide_dir, filename)
        with open(out_path, "w") as f:
            f.write(song_text)
        print(f"Wrote {len(song_text.splitlines())} lines to {out_path}")
