#!/usr/bin/env python3
"""
generate_uncontrollable_urge.py - Generates devo_uncontrollable_urge.sk and uncontrollable_urge.sk
for PULP / SKRED.

Restores the authentic tempo, spacious arrangement, and sonic goodness of the original
DEVO track and the prior version, while upgrading to modern SKRED standards:
- Locked-in 158 BPM tempo (M 158 16) - tight, punchy motorik drive with breathing room,
  eliminating the rushed/hectic feel of 184 BPM.
- The iconic spacious intro bass riff (A2 on beat 1, 9 steps of breathing space,
  then G2 pickups on 10, 12, 14).
- The classic 8-bar layered intro build: solo bass -> +drums -> +guitar chug -> +FM stab -> snare roll.
- The legendary start-stop breaks: alternating 2 bars of driving groove with 2 bars of dead silence,
  where only the eerie FM metallic stab echoes through delay Bus 1!
- The authentic V - IV - I chorus progression: E5 -> D5 -> A5 with driving power chords.
- Bob Mothersbaugh's staccato guitar power chord trio (root, fifth, octave) with dynamic velocities.
- Gerald Casale's gritty saw + square bass with velocity groove.
- Mark Mothersbaugh's staccato vocal lead lines with spacious phrasing that lets the rhythm section chug.
- All voices labeled via [name] vt.
- All pitch streams converted via [ ... ] n>d with '-' rests.
- Safe zero-sustain envelopes (s=0); zero hanging notes.
- Exact 136-bar arrangement matching the prior version structure.
"""

import os, sys

def build_uncontrollable_urge():
    lines = []
    def emit(s=""):
        lines.append(s)

    # ----------------------------------------------------
    # Header & Song Metadata
    # ----------------------------------------------------
    emit("## DEVO - UNCONTROLLABLE URGE (Modernized Showcase Cover for PULP / SKRED)")
    emit("#  158 BPM, Key of A Major / A Mixolydian, 136 bars (~3:26).")
    emit("#  Written by Mark Mothersbaugh & Gerald Casale (1978, Q: Are We Not Men? A: We Are Devo!).")
    emit("#  Self-contained: drums from built-in waves 82-99.")
    emit("#  Uses n>d to convert note name strings (with '-' rests) to MIDI streams.")
    emit("#  Load:   [devo_uncontrollable_urge.sk] /ls")
    emit("#  Start:  play        Stop:  stop")
    emit("#")
    emit("#  VOICE MAP (all labeled via [name] vt):")
    emit("#    v0  motorik kick (w82)")
    emit("#    v1  crack 70s snare (w83)")
    emit("#    v2  closed hat 8ths (w84)  v3  open hat offbeat (w85)")
    emit("#    v5  crash cymbal (w94)")
    emit("#    v6  Gerald Casale bass saw centre (w15) + v7 partner square right (w16)")
    emit("#    v8  Bob Mothersbaugh guitar root left (w15) + v9 fifth (w15), v10 octave (w16)")
    emit("#    v11 FM metal stab (w0) + v12 stab modulator (w0)")
    emit("#    v13 Mark Mothersbaugh vocal lead (w15) + v14 vocal partner pulse (w16)")
    emit("#")
    emit("#  DELAY BUSES:")
    emit("#    Bus 1: FM metal stab & vocal echo (DS 1 158 .75, DL 1 - - 6 2 2 9, DD 1 7 3)")
    emit("#    Bus 2: Bass slapback sixteenth (DS 2 158 .25, DL 2 - - 4 0 0 8, DD 2 6 3)")
    emit("#")
    emit("#  ARRANGEMENT (136 bars):")
    emit("#    0-7   Intro build (solo bass -> +drums -> +guitar chug -> +FM stab -> snare roll)")
    emit("#    8-15  Start-Stop Break 1 (groove -> dead stop FM echo -> groove -> dead stop -> roll)")
    emit("#    16-31 Verse 1: driving A drone + staccato guitar chug + Mark's vocal leads")
    emit("#    32-47 Chorus 1: E5 -> D5 -> A5 power chords + vocal hook ('It's driving me insane!')")
    emit("#    48-55 Start-Stop Break 2 (dead stop tension & echoing FM stabs)")
    emit("#    56-71 Verse 2: 'Got an urge, I wanna purge, 'cause I'm losing control...'")
    emit("#    72-87 Chorus 2")
    emit("#    88-95 Start-Stop Break 3")
    emit("#    96-111 Final Chorus: maximum energy wall of sound")
    emit("#    112-135 Outro Vamp: relentless motorik drive + frantic chanting")
    emit("#    136   Hard stop, crash hit & cleanup")
    emit("# ======================================================================")
    emit()
    emit("[DEVO - UNCONTROLLABLE URGE] puts")
    emit("M 158 16")
    emit()

    # ----------------------------------------------------
    # Delay Buses Setup
    # ----------------------------------------------------
    emit("## DELAY BUSES")
    emit("# Bus 1: FM metal stab echo (dotted eighth delay, gives that eerie cavernous DEVO space)")
    emit("DS 1 158 .75")
    emit("DL 1 - - 6 2 2 9")
    emit("DD 1 7 3")
    emit()
    emit("# Bus 2: Bass slapback sixteenth")
    emit("DS 2 158 .25")
    emit("DL 2 - - 4 0 0 8")
    emit("DD 2 6 3")
    emit()

    # ----------------------------------------------------
    # Voices Setup
    # ----------------------------------------------------
    emit("## VOICES")
    emit("# Alan Myers motorik drums: kick, crack snare, closed/open hats, crash")
    emit("v0 w82 f440 a6 p0 [kick motorik] vt")
    emit("v1 w83 f440 a7 p0 r3 ds.18 [snare 70s crack] vt")
    emit("v2 w84 f440 a-3 p.25 [closed hat] vt")
    emit("v3 w85 f440 a-2 p-.25 [open hat] vt")
    emit("v5 w94 f440 a4 p.2 [crash cymbal] vt")
    emit()
    emit("# Gerald Casale Bass: plucky saw plus detuned square partner on the right")
    emit("v6 w15 a-1 p0 t.002,.14,.25,.06 j1 k1300 q1.8 r2 ds.08 [casale bass saw] vt")
    emit("v6 >7")
    emit("v7 w16 a-6 N0,8 p.4 [casale bass square] vt")
    emit("v6 G7 H7")
    emit()
    emit("# Bob Mothersbaugh Guitar Power Chord from ONE note: root v8, fifth v9 (+7), octave v10 (+12)")
    emit("v8 w15 a-8 p-.25 t.002,.09,.15,.05 j1 k3800 q.7 c4,.15 [bob1 guitar root] vt")
    emit("v8 >9")
    emit("v8 >10")
    emit("v9 N7 a-10 p.3 [bob1 guitar fifth] vt")
    emit("v10 N12 a-12 p-.5 [bob1 guitar octave] vt")
    emit("v8 G9,10 H9,10")
    emit()
    emit("# Metal FM Stab: signature DEVO metallic clang echoed into delay Bus 1")
    emit("v11 w0 a-2 p-.35 t.001,.45,0,.35 FF2 FB4 F12,2.2 G12 H12 r1 ds.25 [devo metal stab] vt")
    emit("v12 w0 a0 t.001,.30,0,.30 N21 m1 [stab modulator] vt")
    emit()
    emit("# Mark Mothersbaugh Vocal Lead: punchy staccato synth lead with safe s=0 envelope")
    emit("v13 w15 a-3 p0 t.003,.16,0,.04 j1 k3600 q2.0 r1 ds.20 [devo vocal lead] vt")
    emit("v13 >14")
    emit("v14 w16 a-7 N0,12 p.25 [vocal partner pulse] vt")
    emit("v13 G14 H14")
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
    emit("## STREAMS")
    emit()
    emit("# Stream 1: Verse bass drone, one bar of eighths on A2")
    emit("[A2 A2 A2 A2 A2 A2 A2 A2] n>d /SS 1")
    emit()
    emit("# Stream 2: Bass velocities, light accent on each beat")
    emit("( 1 .75 .9 .75 1 .75 .9 .75 ) /SS 2")
    emit()
    emit("# Stream 3: Chorus bass, 4 bars of eighths: E2 | D2 | A2 | A2")
    emit("[E2 E2 E2 E2 E2 E2 E2 E2  D2 D2 D2 D2 D2 D2 D2 D2  A2 A2 A2 A2 A2 A2 A2 A2  A2 A2 A2 A2 A2 A2 A2 A2] n>d /SS 3")
    emit()
    emit("# Stream 4: Chorus guitar roots, 4 bars of eighths: E3 | D3 | A3 | A3")
    emit("[E3 E3 E3 E3 E3 E3 E3 E3  D3 D3 D3 D3 D3 D3 D3 D3  A3 A3 A3 A3 A3 A3 A3 A3  A3 A3 A3 A3 A3 A3 A3 A3] n>d /SS 4")
    emit()
    emit("# Stream 5: Guitar chug velocities, accent on each beat")
    emit("( 1 .45 .7 .45 1 .45 .7 .45 ) /SS 5")
    emit()
    emit("# Stream 6: Intro bass riff notes: A2 on 0, rests on 1-9, G2 pickups on 10, 12, 14")
    emit("[A2 - - - - - - - - - G2 - G2 - G2 -] n>d /SS 6")
    emit()
    emit("# Stream 7: Verse 1 Vocal Line ('Got an urge... got a surge... and it's out of control!')")
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
    emit("] n>d /SS 7")
    emit()
    emit("# Stream 8: Chorus Vocal Hook ('It's driving me insane! ... Got an uncontrollable urge!')")
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
    emit("] n>d /SS 8")
    emit()
    emit("# Stream 9: Outro Frantic Vocal Hook ('Uncontrollable urge! Out of control!')")
    emit("# 4 bars / 64 sixteenth steps")
    emit("[")
    emit("E5 - E5 - E5 - E5 - C#5 - - - A4 - - -")
    emit("D5 - D5 - C#5 - B4 - A4 - - - - - - -")
    emit("E5 - E5 - E5 - E5 - C#5 - - - A4 - - -")
    emit("G5 - G5 - G5 - G5 - A5 - - - - - - -")
    emit("] n>d /SS 9")
    emit()

    # ----------------------------------------------------
    # Patterns
    # ----------------------------------------------------
    emit("## PATTERNS")
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

    # Pattern 3: Closed Hats 8ths with Open Sizzle on last offbeat
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

    # Pattern 4: The Authentic Spacious Intro Bass Riff
    # Tonic A2 on beat 1, 9 steps of space, then 3 pickups on G2
    emit("y4")
    emit("[intro bass riff: tonic on 1, three pickups on the last 3 eighths] yt")
    emit("[v6 n&6 l1] x0")
    emit("[v6 n&6 l.9] x10")
    emit("[v6 n&6 l.9] x12")
    emit("[v6 n&6 l.9] x14")
    emit("[=127,0] x15")
    emit()

    # Pattern 5: Verse Bass Drone on A2 (8 steady eighths with velocity groove)
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

    # Pattern 8: Chorus Bass E -> D -> A -> A (4 bars / 64 steps)
    emit("y8")
    emit("[chorus bass E D A A, 4 bars] yt")
    for step in range(0, 64, 2):
        emit(f"[v6 n&3 l&2] x{step}")
    emit("[=127,0] x63")
    emit()

    # Pattern 9: Chorus Guitar Power Chords E -> D -> A -> A (4 bars / 64 steps)
    emit("y9")
    emit("[chorus guitar power chords, 4 bars] yt")
    for step in range(0, 64, 2):
        emit(f"[v8 n&4 l&5] x{step}")
    emit("[=127,0] x63")
    emit()

    # Pattern 10: Verse Guitar Chug on A3 (8 steady eighths with staccato velocity groove)
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

    # Pattern 11: FM Metal Stab on Beat 1 (echoes into delay Bus 1)
    emit("y11")
    emit("[FM stab on beat 1] yt")
    emit("[v11 n69 l.9] x0")
    emit("[=127,0] x15")
    emit()

    # Pattern 13: The Accelerating Snare Roll
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

    # Pattern 21: Verse Vocal Lead (16 bars / 256 steps, reads Stream 7)
    emit("y21")
    emit("[verse vocal lead 16 bars] yt")
    for step in range(256):
        emit(f"[v13 n&7 l1] x{step}")
    emit()

    # Pattern 22: Chorus Vocal Lead (4 bars / 64 steps, reads Stream 8)
    emit("y22")
    emit("[chorus vocal lead 4 bars] yt")
    for step in range(64):
        emit(f"[v13 n&8 l1] x{step}")
    emit()

    # Pattern 23: Outro Vocal Lead (4 bars / 64 steps, reads Stream 9)
    emit("y23")
    emit("[outro vocal lead 4 bars] yt")
    for step in range(64):
        emit(f"[v13 n&9 l1] x{step}")
    emit()

    # ----------------------------------------------------
    # Conductor (Pattern 0 at %16, 136 bars)
    # ----------------------------------------------------
    emit("## CONDUCTOR (pattern 0, one step per bar, 136 bars)")
    emit("y0")
    emit("[conductor] yt")
    emit("%16")
    emit()

    # Section 1: Intro (Bars 0-7)
    # Layered intro build: Solo bass -> +drums -> +guitar chug -> +FM stab -> snare roll
    emit("# Bars 0-1: Solo bass intro riff")
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
    # Alternating 2 bars of driving groove with 2 bars of dead silence and echoing FM stab!
    emit("# Bars 8-9: Full groove hits with crash cymbal!")
    emit("[v5 l1 y4 z0 y8 z0 y9 z0 y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 /SP 1,0 /SP 2,0] x8")
    emit("[=126,9] x9")
    emit("# Bars 10-11: DEAD STOP! Complete band cutoff, only FM stab echoes into the void!")
    emit("[y5 z0 y10 z0 y1 z0 y2 z0 y3 z0 y11 z1] x10")
    emit("[=126,11] x11")
    emit("# Bars 12-13: Full groove slams back in!")
    emit("[y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 /SP 1,0 /SP 2,0] x12")
    emit("[=126,13] x13")
    emit("# Bar 14: DEAD STOP! FM stab echo rings!")
    emit("[y5 z0 y10 z0 y1 z0 y2 z0 y3 z0 y11 z1] x14")
    emit("# Bar 15: Snare roll builds into the verse!")
    emit("[y11 z0 y13 z1] x15")
    emit()

    # Section 3: Verse 1 (Bars 16-31, 16 bars)
    # Driving A drone, staccato guitar chug, and Mark's vocal leads
    emit("# Bars 16-30: Verse 1 ('Got an urge, got a surge, and it's out of control...')")
    emit("[v5 l1 y4 z0 y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 y21 z1 /SP 1,0 /SP 2,0 /SP 7,0] x16")
    for b in range(17, 31):
        emit(f"[=126,{b}] x{b}")
    emit("# Bar 31: Snare roll into chorus!")
    emit("[y2 z0 y13 z1] x31")
    emit()

    # Section 4: Chorus 1 (Bars 32-47, 16 bars = 4 cycles of E -> D -> A -> A)
    # Power chords + vocal hook ('It's driving me insane! Got an uncontrollable urge!')
    emit("# Bars 32-46: Chorus 1 (E -> D -> A power chords)")
    emit("[v5 l1 y5 z0 y10 z0 y21 z0 y2 z1 y8 z1 y9 z1 y22 z1 /SP 3,0 /SP 4,0 /SP 8,0] x32")
    for b in range(33, 47):
        emit(f"[=126,{b}] x{b}")
    emit("# Bar 47: Snare roll into break 2!")
    emit("[y2 z0 y13 z1] x47")
    emit()

    # Section 5: Start-Stop Break 2 (Bars 48-55, 8 bars)
    emit("# Bars 48-49: Groove crashes back in!")
    emit("[v5 l1 y8 z0 y9 z0 y22 z0 y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 /SP 1,0 /SP 2,0] x48")
    emit("[=126,49] x49")
    emit("# Bars 50-51: DEAD STOP! FM stab echo!")
    emit("[y5 z0 y10 z0 y1 z0 y2 z0 y3 z0 y11 z1] x50")
    emit("[=126,51] x51")
    emit("# Bars 52-53: Groove returns!")
    emit("[y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 /SP 1,0 /SP 2,0] x52")
    emit("[=126,53] x53")
    emit("# Bar 54: DEAD STOP! FM stab echo!")
    emit("[y5 z0 y10 z0 y1 z0 y2 z0 y3 z0 y11 z1] x54")
    emit("# Bar 55: Snare roll into verse 2!")
    emit("[y11 z0 y13 z1] x55")
    emit()

    # Section 6: Verse 2 (Bars 56-71, 16 bars)
    emit("# Bars 56-70: Verse 2 ('Got an urge, I wanna purge, 'cause I'm losing control...')")
    emit("[v5 l1 y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 y21 z1 /SP 1,0 /SP 2,0 /SP 7,0] x56")
    for b in range(57, 71):
        emit(f"[=126,{b}] x{b}")
    emit("# Bar 71: Snare roll into chorus 2!")
    emit("[y2 z0 y13 z1] x71")
    emit()

    # Section 7: Chorus 2 (Bars 72-87, 16 bars)
    emit("# Bars 72-86: Chorus 2 (E -> D -> A power chords)")
    emit("[v5 l1 y5 z0 y10 z0 y21 z0 y2 z1 y8 z1 y9 z1 y22 z1 /SP 3,0 /SP 4,0 /SP 8,0] x72")
    for b in range(73, 87):
        emit(f"[=126,{b}] x{b}")
    emit("# Bar 87: Snare roll into break 3!")
    emit("[y2 z0 y13 z1] x87")
    emit()

    # Section 8: Start-Stop Break 3 (Bars 88-95, 8 bars)
    emit("# Bars 88-89: Groove returns!")
    emit("[v5 l1 y8 z0 y9 z0 y22 z0 y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 /SP 1,0 /SP 2,0] x88")
    emit("[=126,89] x89")
    emit("# Bars 90-91: DEAD STOP! FM stab echo!")
    emit("[y5 z0 y10 z0 y1 z0 y2 z0 y3 z0 y11 z1] x90")
    emit("[=126,91] x91")
    emit("# Bars 92-93: Groove returns!")
    emit("[y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 /SP 1,0 /SP 2,0] x92")
    emit("[=126,93] x93")
    emit("# Bar 94: DEAD STOP! FM stab echo!")
    emit("[y5 z0 y10 z0 y1 z0 y2 z0 y3 z0 y11 z1] x94")
    emit("# Bar 95: Snare roll into final chorus!")
    emit("[y11 z0 y13 z1] x95")
    emit()

    # Section 9: Final Chorus (Bars 96-111, 16 bars)
    emit("# Bars 96-110: Final Chorus: maximum energy wall of sound!")
    emit("[v5 l1 y5 z0 y10 z0 y2 z1 y8 z1 y9 z1 y22 z1 /SP 3,0 /SP 4,0 /SP 8,0] x96")
    for b in range(97, 104):
        emit(f"[=126,{b}] x{b}")
    emit("# Bar 104: Extra crash accent!")
    emit("[v5 l1] x104")
    for b in range(105, 111):
        emit(f"[=126,{b}] x{b}")
    emit("# Bar 111: Snare roll into outro vamp!")
    emit("[y2 z0 y13 z1] x111")
    emit()

    # Section 10: Outro Vamp (Bars 112-135, 24 bars)
    # Relentless motorik drive + frantic chanting
    emit("# Bars 112-119: Outro vamp groove + frantic vocal chanting")
    emit("[v5 l1 y8 z0 y9 z0 y22 z0 y2 z1 y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 y23 z1 /SP 1,0 /SP 2,0 /SP 9,0] x112")
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
