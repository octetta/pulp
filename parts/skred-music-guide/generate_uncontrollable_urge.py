#!/usr/bin/env python3
"""
generate_uncontrollable_urge.py - Generates devo_uncontrollable_urge.sk and uncontrollable_urge.sk
for PULP / SKRED.

Restores the authentic DEVO arrangement and the iconic initial bass riff (y4) 100% untouched,
while integrating the new elements:
- The iconic initial bass riff (y4): tonic A2 on 1, 9 steps of space, then three G2 pickups on 10, 12, 14.
- 184 BPM tempo (M 184 16) and 136-bar arrangement.
- Mark Mothersbaugh's quirky Minimoog synth bleeps (y28) on offbeats and chorus squeals (y29).
- The manic "YEAH! YEAH! YEAH-YEAH-YEAH-YEAH!" vocal shouts (y21) over the driving breaks.
- Mark Mothersbaugh's verse vocal lead melody (y22) ("Got an urge, got a surge, and it's out of control...").
- Chorus vocal hook (y23) ("It's driving me insane! Got an uncontrollable urge!").
- Outro frantic vocal chanting (y24) ("Uncontrollable urge! Out of control!").
- All voices labeled via [name] vt tags.
- Safe zero-sustain envelopes (s=0); zero hanging notes.
"""

import os, sys

def build_uncontrollable_urge():
    lines = []
    def emit(s=""):
        lines.append(s)

    # ----------------------------------------------------
    # Header & Metadata
    # ----------------------------------------------------
    emit("## UNCONTROLLABLE URGE (DEVO) - Showcase Cover for PULP/SKRED")
    emit("#  184 BPM, key of A, 136 bars (about 2:57).")
    emit("#  Written by Mark Mothersbaugh & Gerald Casale (1978, Q: Are We Not Men? A: We Are Devo!).")
    emit("#  Self-contained: drums from built-in waves 82-99.")
    emit("#  Uses n>d to convert note name strings to MIDI streams.")
    emit("#     [devo_uncontrollable_urge.sk] /ls      then:   play     (stop:  stop)")
    emit("#")
    emit("#  VOICE MAP (all labeled via [name] vt):")
    emit("#    v0  kick motorik (w82)  v1  snare 70s crack (w83)")
    emit("#    v2  closed hat (w84)    v3  open hat (w85)    v5  crash cymbal (w94)")
    emit("#    v6  Gerald Casale bass saw (w15) + v7 partner square (w16)")
    emit("#    v8  Bob Mothersbaugh guitar root (w15) + v9 fifth (+7), v10 octave (+12)")
    emit("#    v11 FM metal stab (w0) + v12 stab modulator (w0)")
    emit("#    v13 Mark Mothersbaugh vocal lead (w15) + v14 partner pulse (w16)")
    emit("#    v15 Minimoog quirky bleep saw (w15) + v16 pulse partner (w16)")
    emit("#")
    emit("#  DELAY BUSES:")
    emit("#    Bus 1: FM metal stab & vocal echo (DS 1 184 .75, DL 1 - - 6 2 2 9, DD 1 7 3)")
    emit("#    Bus 2: Bass slapback sixteenth (DS 2 184 .25, DL 2 - - 4 0 0 8, DD 2 6 3)")
    emit("#")
    emit("#  SECTIONS (bar numbers):")
    emit("#     0-7   intro (y4 solo bass -> +drums -> +guitar chug -> +FM stab -> snare roll)")
    emit("#     8-15  start-stop break 1 (groove + 'YEAH!' shouts -> dead stop FM stab echo)")
    emit("#     16-31 verse 1: driving A drone + guitar chug + Minimoog bleeps + vocal lead")
    emit("#     32-47 chorus 1: E -> D -> A power chords + Minimoog squeals + vocal hook")
    emit("#     48-55 start-stop break 2")
    emit("#     56-71 verse 2: 'Got an urge, I wanna purge, cause I'm losing control...'")
    emit("#     72-87 chorus 2")
    emit("#     88-95 start-stop break 3")
    emit("#     96-111 final chorus: maximum energy wall of sound")
    emit("#     112-135 outro vamp: relentless motorik drive + frantic vocal chanting")
    emit("#     136   hard stop, crash hit & cleanup")
    emit("# ======================================================================")
    emit()
    emit("[UNCONTROLLABLE URGE] puts")
    emit("M 184 16")
    emit()

    # ----------------------------------------------------
    # Delay Buses
    # ----------------------------------------------------
    emit("## DELAY BUSES")
    emit("# Bus 1: FM metal stab echo (dotted eighth delay, gives that eerie cavernous DEVO space)")
    emit("DS 1 184 .75")
    emit("DL 1 - - 6 2 2 9")
    emit("DD 1 7 3")
    emit()
    emit("# Bus 2: Bass slapback sixteenth")
    emit("DS 2 184 .25")
    emit("DL 2 - - 4 0 0 8")
    emit("DD 2 6 3")
    emit()

    # ----------------------------------------------------
    # Voices Setup
    # ----------------------------------------------------
    emit("## VOICES")
    emit("v0 w82 f440 a6 p0 [kick motorik] vt")
    emit("v1 w83 f440 a7 p0 r3 ds.18 [snare 70s crack] vt")
    emit("v2 w84 f440 a-3 p.25 [closed hat] vt")
    emit("v3 w85 f440 a-2 p-.25 [open hat] vt")
    emit("v5 w94 f440 a4 p.2 [crash cymbal] vt")
    emit()
    emit("# bass: plucky saw plus detuned square partner on the right")
    emit("v6 w15 a-1 p0 t.002,.14,.25,.06 j1 k1300 q1.8 r2 ds.08 [bass saw] vt")
    emit("v6 >7")
    emit("v7 w16 a-6 N0,8 p.4 [bass square] vt")
    emit("v6 G7 H7")
    emit()
    emit("# guitar power chord from ONE note: v8 root, v9 fifth, v10 octave, bright and spread")
    emit("v8 w15 a-8 p-.25 t.002,.09,.15,.05 j1 k3800 q.7 c4,.15 [guitar root] vt")
    emit("v8 >9")
    emit("v8 >10")
    emit("v9 N7 a-10 p.3 [guitar fifth] vt")
    emit("v10 N12 a-12 p-.5 [guitar octave] vt")
    emit("v8 G9,10 H9,10")
    emit()
    emit("# metal FM stab (copied from hammer_the_signal.sk), modulator tracks pitch")
    emit("v11 w0 a-2 p-.35 t.001,.45,0,.35 FF2 FB4 F12,2.2 G12 H12 r1 ds.25 [fm metal stab] vt")
    emit("v12 w0 a0 t.001,.30,0,.30 N21 m1 [stab modulator] vt")
    emit()
    emit("# Mark Mothersbaugh vocal lead: punchy staccato synth lead with safe s=0 envelope")
    emit("v13 w15 a-3 p0 t.003,.16,0,.04 j1 k3600 q2.0 r1 ds.20 [devo vocal lead] vt")
    emit("v13 >14")
    emit("v14 w16 a-7 N0,12 p.25 [vocal partner pulse] vt")
    emit("v13 G14 H14")
    emit()
    emit("# Minimoog quirky synth bleeps & squeals")
    emit("v15 w15 a-3 p-.20 t.001,.08,0,.03 j1 k2800 q4.0 r1 ds.20 [minimoog bleep saw] vt")
    emit("v15 >16")
    emit("v16 w16 a-7 N0,12 p.20 [minimoog bleep pulse] vt")
    emit("v15 G16 H16")
    emit()

    # ----------------------------------------------------
    # Events Cleanup Responder
    # ----------------------------------------------------
    emit("## EVENTS (end-of-song cleanup responder)")
    emit("[O DQ] /ceb 4 99")
    emit("/cer 1")
    emit()

    # ----------------------------------------------------
    # Streams
    # ----------------------------------------------------
    emit("## STREAMS (all pitch material lives here, edit to correct notes)")
    emit("# stream 1: verse bass drone, one bar of eighths on A2")
    emit("[A2 A2 A2 A2 A2 A2 A2 A2] n>d /SS 1")
    emit("# stream 2: bass velocities, light accent on each beat")
    emit("( 1 .75 .9 .75 1 .75 .9 .75 ) /SS 2")
    emit("# stream 3: chorus bass, 4 bars of eighths: E2 | D2 | A2 | A2")
    emit("[E2 E2 E2 E2 E2 E2 E2 E2  D2 D2 D2 D2 D2 D2 D2 D2  A2 A2 A2 A2 A2 A2 A2 A2  A2 A2 A2 A2 A2 A2 A2 A2] n>d /SS 3")
    emit("# stream 4: chorus guitar roots, 4 bars of eighths: E3 | D3 | A3 | A3")
    emit("[E3 E3 E3 E3 E3 E3 E3 E3  D3 D3 D3 D3 D3 D3 D3 D3  A3 A3 A3 A3 A3 A3 A3 A3  A3 A3 A3 A3 A3 A3 A3 A3] n>d /SS 4")
    emit("# stream 5: guitar chug velocities, accent on each beat")
    emit("( 1 .45 .7 .45 1 .45 .7 .45 ) /SS 5")
    emit()
    emit("# stream 7: 'YEAH!' manic vocal shouts (2 bars / 32 steps)")
    emit("[")
    emit("E5 - - - E5 - - - E5 - E5 - E5 - E5 -")
    emit("G5 - - - G5 - - - G5 - G5 - G5 - A5 -")
    emit("] n>d /SS 7")
    emit()
    emit("# stream 8: verse vocal lead ('Got an urge... got a surge... and it's out of control!')")
    emit("# 16 bars / 256 sixteenth steps with spacious breathing room between calls")
    emit("[")
    # Bars 1-4: "Got an urge! ... Got a surge! ... And it's out of control!"
    emit("A4 - A4 - C#5 - - - - - - - - - - -")
    emit("A4 - A4 - C#5 - - - - - - - - - - -")
    emit("D5 - D5 - C#5 - B4 - A4 - - - - - - -")
    emit("- - - - - - - - - - - - - - - -")
    # Bars 5-8: "Got an urge! ... I wanna purge! ... 'Cause I'm losing control!"
    emit("A4 - A4 - C#5 - - - - - - - - - - -")
    emit("A4 - A4 - C#5 - - - - - - - - - - -")
    emit("D5 - D5 - C#5 - B4 - A4 - - - - - - -")
    emit("- - - - - - - - - - - - - - - -")
    # Bars 9-12: "Uncontrollable urge! ... I wanna tell you all about it!"
    emit("E5 - E5 - E5 - E5 - C#5 - - - - - - -")
    emit("A4 - A4 - B4 - C#5 - B4 - A4 - - - - -")
    emit("- - - - - - - - - - - - - - - -")
    emit("- - - - - - - - - - - - - - - -")
    # Bars 13-16: "Uncontrollable urge! ... Makes me scream and shout it!"
    emit("E5 - E5 - E5 - E5 - C#5 - - - - - - -")
    emit("A4 - A4 - B4 - C#5 - B4 - A4 - - - - -")
    emit("- - - - - - - - - - - - - - - -")
    emit("- - - - - - - - - - - - - - - -")
    emit("] n>d /SS 8")
    emit()
    emit("# stream 9: chorus vocal hook ('It's driving me insane! ... Got an uncontrollable urge!')")
    emit("# 4 bars / 64 sixteenth steps, loops across the chorus")
    emit("[")
    # Bar 1 (E): "I've got an uncontrollable urge!"
    emit("E5 - E5 - E5 - E5 - C#5 - - - - - - -")
    # Bar 2 (D): "It's driving me insane!"
    emit("D5 - D5 - D5 - C#5 - A4 - - - - - - -")
    # Bar 3 (A): "YEAH! YEAH!"
    emit("A4 - - - A4 - - - - - - - - - - -")
    # Bar 4 (A): "YEAH-YEAH-YEAH-YEAH!"
    emit("A4 - A4 - A4 - A4 - - - - - - - - -")
    emit("] n>d /SS 9")
    emit()
    emit("# stream 10: outro frantic vocal hook ('Uncontrollable urge! Out of control!')")
    emit("# 4 bars / 64 sixteenth steps")
    emit("[")
    emit("E5 - E5 - E5 - E5 - C#5 - - - A4 - - -")
    emit("D5 - D5 - C#5 - B4 - A4 - - - - - - -")
    emit("E5 - E5 - E5 - E5 - C#5 - - - A4 - - -")
    emit("G5 - G5 - G5 - G5 - A5 - - - - - - -")
    emit("] n>d /SS 10")
    emit()

    # ----------------------------------------------------
    # Patterns (kick, snare, hats, bass, guitar, stab, roll, vocals, minimoog)
    # ----------------------------------------------------
    emit("## PATTERNS (kick, snare, hats, bass, guitar, stab, roll)")
    emit()

    # Pattern 1: Kick on 1 and 3
    emit("y1")
    emit("[kick on 1 and 3] yt")
    emit("[v0 l1] x0")
    emit("[v0 l1] x8")
    emit("[=127,0] x15")
    emit()

    # Pattern 2: Snare on 2 and 4
    emit("y2")
    emit("[snare on 2 and 4] yt")
    emit("[v1 l1] x4")
    emit("[v1 l1] x12")
    emit("[=127,0] x15")
    emit()

    # Pattern 3: Hats 8ths, open hat on the last offbeat
    emit("y3")
    emit("[hats 8ths, open hat on the last offbeat] yt")
    emit("[v2 l.4] x0")
    emit("[v2 l.4] x2")
    emit("[v2 l.4] x4")
    emit("[v2 l.4] x6")
    emit("[v2 l.4] x8")
    emit("[v2 l.4] x10")
    emit("[v2 l.4] x12")
    emit("[v3 l.7 v2 l0] x14")
    emit("[=127,0] x15")
    emit()

    # Pattern 4: The authentic intro bass riff (100% UNTOUCHED ORIGINAL)
    # Tonic A2 on 1, then three pickups on G2 at 10, 12, 14
    emit("y4")
    emit("[intro bass riff: tonic on 1, three pickups on the last 3 eighths] yt")
    emit("[v6 n45 l1] x0")
    emit("[v6 n43 l.9] x10")
    emit("[v6 n43 l.9] x12")
    emit("[v6 n43 l.9] x14")
    emit("[=127,0] x15")
    emit()

    # Pattern 5: Verse bass drone on A2
    emit("y5")
    emit("[verse bass drone] yt")
    emit("[v6 n&1 l&2] x0")
    emit("[v6 n&1 l&2] x2")
    emit("[v6 n&1 l&2] x4")
    emit("[v6 n&1 l&2] x6")
    emit("[v6 n&1 l&2] x8")
    emit("[v6 n&1 l&2] x10")
    emit("[v6 n&1 l&2] x12")
    emit("[v6 n&1 l&2] x14")
    emit("[=127,0] x15")
    emit()

    # Pattern 8: Chorus bass E D A A, 4 bars
    emit("y8")
    emit("[chorus bass E D A A, 4 bars] yt")
    for step in range(0, 64, 2):
        emit(f"[v6 n&3 l&2] x{step}")
    emit("[=127,0] x63")
    emit()

    # Pattern 9: Chorus guitar power chords E D A A, 4 bars
    emit("y9")
    emit("[chorus guitar power chords, 4 bars] yt")
    for step in range(0, 64, 2):
        emit(f"[v8 n&4 l&5] x{step}")
    emit("[=127,0] x63")
    emit()

    # Pattern 10: Verse guitar chug on A
    emit("y10")
    emit("[verse guitar chug on A] yt")
    emit("[v8 n57 l&5] x0")
    emit("[v8 n57 l&5] x2")
    emit("[v8 n57 l&5] x4")
    emit("[v8 n57 l&5] x6")
    emit("[v8 n57 l&5] x8")
    emit("[v8 n57 l&5] x10")
    emit("[v8 n57 l&5] x12")
    emit("[v8 n57 l&5] x14")
    emit("[=127,0] x15")
    emit()

    # Pattern 11: FM stab on beat 1
    emit("y11")
    emit("[FM stab on beat 1] yt")
    emit("[v11 n69 l.9] x0")
    emit("[=127,0] x15")
    emit()

    # Pattern 13: Snare roll crescendo
    emit("y13")
    emit("[snare roll] yt")
    emit("[v1 l0.2] x0")
    emit("[v1 l0.25] x1")
    emit("[v1 l0.3] x2")
    emit("[v1 l0.35] x3")
    emit("[v1 l0.4] x4")
    emit("[v1 l0.45] x5")
    emit("[v1 l0.5] x6")
    emit("[v1 l0.55] x7")
    emit("[v1 l0.6 z*2] x8")
    emit("[v1 l0.65 z*2] x9")
    emit("[v1 l0.7 z*2] x10")
    emit("[v1 l0.75 z*2] x11")
    emit("[v1 l0.8 z*4] x12")
    emit("[v1 l0.85 z*4] x13")
    emit("[v1 l0.9 z*4] x14")
    emit("[v1 l0.95 z*8] x15")
    emit("[-] x16")
    emit()

    # Pattern 21: 'YEAH!' Manic Vocal Shouts (2 bars / 32 steps)
    emit("y21")
    emit("[yeah manic vocal shouts 2 bars] yt")
    for step in range(32):
        if step == 31:
            emit(f"[v13 n&7 l1 =127,0] x{step}")
        else:
            emit(f"[v13 n&7 l1] x{step}")
    emit()

    # Pattern 22: Verse Vocal Lead (16 bars / 256 steps)
    emit("y22")
    emit("[verse vocal lead 16 bars] yt")
    for step in range(256):
        if step == 255:
            emit(f"[v13 n&8 l1 =127,0] x{step}")
        else:
            emit(f"[v13 n&8 l1] x{step}")
    emit()

    # Pattern 23: Chorus Vocal Lead (4 bars / 64 steps)
    emit("y23")
    emit("[chorus vocal lead 4 bars] yt")
    for step in range(64):
        if step == 63:
            emit(f"[v13 n&9 l1 =127,0] x{step}")
        else:
            emit(f"[v13 n&9 l1] x{step}")
    emit()

    # Pattern 24: Outro Vocal Lead (4 bars / 64 steps)
    emit("y24")
    emit("[outro vocal lead 4 bars] yt")
    for step in range(64):
        if step == 63:
            emit(f"[v13 n&10 l1 =127,0] x{step}")
        else:
            emit(f"[v13 n&10 l1] x{step}")
    emit()

    # Pattern 28: Minimoog Quirky Synth Bleeps
    emit("y28")
    emit("[minimoog quirky synth bleeps] yt")
    emit("[v15 n81 l.8] x6")
    emit("[v15 n88 l.9] x14")
    emit("[=127,0] x15")
    emit()

    # Pattern 29: Minimoog Chorus Squeals
    emit("y29")
    emit("[minimoog chorus squeals] yt")
    emit("[v15 n76 l.8] x2")
    emit("[v15 n79 l.8] x6")
    emit("[v15 n81 l.85] x10")
    emit("[v15 n88 l.9 =127,0] x14")
    emit()

    # ----------------------------------------------------
    # Conductor (Pattern 0, one step per bar, 136 bars)
    # ----------------------------------------------------
    emit("## CONDUCTOR (pattern 0, one step per bar, 136 bars)")
    emit("y0")
    emit("[conductor] yt")
    emit("%16")
    emit()

    # Section 1: Intro (Bars 0-7) - 100% ORIGINAL Y4 PRESERVED
    emit("# Bars 0-1: Solo bass intro riff (y4)")
    emit("[y4 z1] x0")
    emit("[=126,1] x1")
    emit("# Bars 2-3: Drums enter with bass riff")
    emit("[y1 z1 y2 z1 y3 z1] x2")
    emit("[=126,3] x3")
    emit("# Bars 4-5: Guitar chug enters")
    emit("[y10 z1] x4")
    emit("[=126,5] x5")
    emit("# Bar 6: FM stab enters on beat 1")
    emit("[y11 z1] x6")
    emit("# Bar 7: Sudden break - all cut, snare roll builds!")
    emit("[y11 z0 y1 z0 y2 z0 y13 z1] x7")
    emit()

    # Section 2: Start-Stop Break 1 (Bars 8-15)
    # Full groove hits + 'YEAH!' shouts -> dead stop FM stab echo
    emit("# Bars 8-9: Full groove hits with crash cymbal + 'YEAH!' shouts!")
    emit("[v5 l1 y4 z0 y8 z0 y9 z0 y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 y21 z1 /SP 1,0 /SP 2,0 /SP 7,0] x8")
    emit("[=126,9] x9")
    emit("# Bars 10-11: DEAD STOP! Complete band cutoff, only FM stab echoes into the void!")
    emit("[y5 z0 y10 z0 y1 z0 y2 z0 y3 z0 y21 z0 y11 z1] x10")
    emit("[=126,11] x11")
    emit("# Bars 12-13: Full groove slams back in with 'YEAH!' shouts!")
    emit("[y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y21 z1 y11 z0 /SP 1,0 /SP 2,0 /SP 7,0] x12")
    emit("[=126,13] x13")
    emit("# Bar 14: DEAD STOP! FM stab echo rings!")
    emit("[y5 z0 y10 z0 y1 z0 y2 z0 y3 z0 y21 z0 y11 z1] x14")
    emit("# Bar 15: Snare roll builds into the verse!")
    emit("[y11 z0 y13 z1] x15")
    emit()

    # Section 3: Verse 1 (Bars 16-31, 16 bars)
    # Driving A drone, staccato guitar chug, Minimoog bleeps, and Mark's vocal leads
    emit("# Bars 16-30: Verse 1 ('Got an urge, got a surge, and it's out of control...')")
    emit("[v5 l1 y4 z0 y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 y28 z1 y22 z1 /SP 1,0 /SP 2,0 /SP 8,0] x16")
    for b in range(17, 31):
        emit(f"[=126,{b}] x{b}")
    emit("# Bar 31: Snare roll into chorus!")
    emit("[y2 z0 y28 z0 y22 z0 y13 z1] x31")
    emit()

    # Section 4: Chorus 1 (Bars 32-47, 16 bars = 4 cycles of E -> D -> A -> A)
    # Power chords + Minimoog squeals + vocal hook ('It's driving me insane! Got an uncontrollable urge!')
    emit("# Bars 32-46: Chorus 1 (E -> D -> A power chords)")
    emit("[v5 l1 y5 z0 y10 z0 y2 z1 y8 z1 y9 z1 y29 z1 y23 z1 /SP 3,0 /SP 4,0 /SP 9,0] x32")
    for b in range(33, 47):
        emit(f"[=126,{b}] x{b}")
    emit("# Bar 47: Snare roll into break 2!")
    emit("[y2 z0 y29 z0 y23 z0 y13 z1] x47")
    emit()

    # Section 5: Start-Stop Break 2 (Bars 48-55, 8 bars)
    emit("# Bars 48-49: Groove crashes back in with 'YEAH!' shouts!")
    emit("[v5 l1 y8 z0 y9 z0 y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 y21 z1 /SP 1,0 /SP 2,0 /SP 7,0] x48")
    emit("[=126,49] x49")
    emit("# Bars 50-51: DEAD STOP! FM stab echo!")
    emit("[y5 z0 y10 z0 y1 z0 y2 z0 y3 z0 y21 z0 y11 z1] x50")
    emit("[=126,51] x51")
    emit("# Bars 52-53: Groove returns!")
    emit("[y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y21 z1 y11 z0 /SP 1,0 /SP 2,0 /SP 7,0] x52")
    emit("[=126,53] x53")
    emit("# Bar 54: DEAD STOP! FM stab echo!")
    emit("[y5 z0 y10 z0 y1 z0 y2 z0 y3 z0 y21 z0 y11 z1] x54")
    emit("# Bar 55: Snare roll into verse 2!")
    emit("[y11 z0 y13 z1] x55")
    emit()

    # Section 6: Verse 2 (Bars 56-71, 16 bars)
    emit("# Bars 56-70: Verse 2 ('Got an urge, I wanna purge, 'cause I'm losing control...')")
    emit("[v5 l1 y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 y28 z1 y22 z1 /SP 1,0 /SP 2,0 /SP 8,0] x56")
    for b in range(57, 71):
        emit(f"[=126,{b}] x{b}")
    emit("# Bar 71: Snare roll into chorus 2!")
    emit("[y2 z0 y28 z0 y22 z0 y13 z1] x71")
    emit()

    # Section 7: Chorus 2 (Bars 72-87, 16 bars)
    emit("# Bars 72-86: Chorus 2 (E -> D -> A power chords)")
    emit("[v5 l1 y5 z0 y10 z0 y2 z1 y8 z1 y9 z1 y29 z1 y23 z1 /SP 3,0 /SP 4,0 /SP 9,0] x72")
    for b in range(73, 87):
        emit(f"[=126,{b}] x{b}")
    emit("# Bar 87: Snare roll into break 3!")
    emit("[y2 z0 y29 z0 y23 z0 y13 z1] x87")
    emit()

    # Section 8: Start-Stop Break 3 (Bars 88-95, 8 bars)
    emit("# Bars 88-89: Groove returns with 'YEAH!' shouts!")
    emit("[v5 l1 y8 z0 y9 z0 y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 y21 z1 /SP 1,0 /SP 2,0 /SP 7,0] x88")
    emit("[=126,89] x89")
    emit("# Bars 90-91: DEAD STOP! FM stab echo!")
    emit("[y5 z0 y10 z0 y1 z0 y2 z0 y3 z0 y21 z0 y11 z1] x90")
    emit("[=126,91] x91")
    emit("# Bars 92-93: Groove returns!")
    emit("[y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y21 z1 y11 z0 /SP 1,0 /SP 2,0 /SP 7,0] x92")
    emit("[=126,93] x93")
    emit("# Bar 94: DEAD STOP! FM stab echo!")
    emit("[y5 z0 y10 z0 y1 z0 y2 z0 y3 z0 y21 z0 y11 z1] x94")
    emit("# Bar 95: Snare roll into final chorus!")
    emit("[y11 z0 y13 z1] x95")
    emit()

    # Section 9: Final Chorus (Bars 96-111, 16 bars)
    emit("# Bars 96-110: Final Chorus: maximum energy wall of sound!")
    emit("[v5 l1 y5 z0 y10 z0 y2 z1 y8 z1 y9 z1 y29 z1 y23 z1 /SP 3,0 /SP 4,0 /SP 9,0] x96")
    for b in range(97, 104):
        emit(f"[=126,{b}] x{b}")
    emit("# Bar 104: Extra crash accent!")
    emit("[v5 l1] x104")
    for b in range(105, 111):
        emit(f"[=126,{b}] x{b}")
    emit("# Bar 111: Snare roll into outro vamp!")
    emit("[y2 z0 y29 z0 y23 z0 y13 z1] x111")
    emit()

    # Section 10: Outro Vamp (Bars 112-135, 24 bars)
    emit("# Bars 112-119: Outro vamp groove + frantic vocal chanting")
    emit("[v5 l1 y8 z0 y9 z0 y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 y24 z1 /SP 1,0 /SP 2,0 /SP 10,0] x112")
    for b in range(113, 120):
        emit(f"[=126,{b}] x{b}")
    emit("# Bar 120: FM stab re-enters as an accent layer!")
    emit("[v5 l1 y11 z1] x120")
    for b in range(121, 127):
        emit(f"[=126,{b}] x{b}")
    emit("# Bar 127: FM stab cuts out")
    emit("[y11 z0] x127")
    emit("# Bar 128: Crash hit!")
    emit("[v5 l1] x128")
    for b in range(129, 135):
        emit(f"[=126,{b}] x{b}")
    emit("# Bar 135: Final accelerating snare roll build!")
    emit("[y2 z0 y13 z1] x135")
    emit()

    # Section 11: End Hard Stop & Cleanup (Bar 136)
    emit("# Bar 136: Final unison crash hit & hard stop cleanup")
    emit("[v5 l1 Z0 +4 ce 99] x136")
    emit()

    # ----------------------------------------------------
    # Macros (play / stop)
    # ----------------------------------------------------
    emit("## MACROS (play / stop)")
    emit("[use 'play' to start the song, 'stop' to stop it] puts")
    emit("[play] : Z0 Z! y0 z1 ;")
    emit("[stop] : Z0 O DQ ;")

    return "\n".join(lines) + "\n"

if __name__ == "__main__":
    song_text = build_uncontrollable_urge()
    base_dir = os.path.dirname(os.path.abspath(__file__))
    out_path1 = os.path.join(base_dir, "devo_uncontrollable_urge.sk")
    out_path2 = os.path.join(base_dir, "uncontrollable_urge.sk")
    
    with open(out_path1, "w") as f:
        f.write(song_text)
    print(f"Generated {out_path1} ({len(song_text)} bytes)")
    
    with open(out_path2, "w") as f:
        f.write(song_text)
    print(f"Generated {out_path2} ({len(song_text)} bytes)")
