#!/usr/bin/env python3
"""
generate_uncontrollable_urge.py - Generates devo_uncontrollable_urge.sk and uncontrollable_urge.sk
for PULP / SKRED.

An authentic, explosive post-punk / new wave cover of DEVO's 1978 opening masterpiece:
"Uncontrollable Urge" by Mark Mothersbaugh and Gerald Casale
(from their landmark debut album 'Q: Are We Not Men? A: We Are Devo!').

Features:
- Fast, frenetic 184 BPM motorik punk drive (M 184 16)
- Gerald Casale's punchy Moog/Fender bass riff & driving tonic drone (w15 saw + w16 square)
- Bob Mothersbaugh's jagged staccato guitar chops & power chords (root w15, fifth w15, octave w16)
- Bob Casale ("Bob 2") rhythm guitar for wide stereo double-tracking
- Mark Mothersbaugh's iconic Minimoog synth bleeps, squeals, and stabs
- Full vocal arrangements:
    * The manic opening "Yeah, yeah, yeah, yeah, y-y-y-yeah-yeah!"
    * Mark Mothersbaugh's jerky staccato verse lyrics:
      "Got an urge, got a surge, and it's out of control..."
      "Got an urge, I wanna purge, 'cause I'm losing control..."
      "Uncontrollable urge, I wanna tell you all about it..."
      "Uncontrollable urge, makes me scream and shout it..."
    * Chorus anthemic hook: "It's got style, it's got class... And I say: YEAH!"
    * Outro frantic chanting: "Uncontrollable urge! Out of control!"
- Alan Myers ("the human metronome") machine-like precision drumming:
  punchy kick, crack 70s snare + tight clap, 8th & 16th hats, Simmons toms, and explosive snare crescendo rolls
- DEVO's legendary start-stop dead silence breaks: stopping on a dime with metallic scrape pickups
- Bob Mothersbaugh's frantic, angular staccato guitar solo
- All voices tagged and inspectable via [name] vt
- All pitch streams converted via [ ... ] n>d with '-' rests
- Safe zero-sustain envelopes (s=0); zero hanging notes or droning poly pools
- Pinning all loop lengths inside the final step bracket [ ... =127,0] without step clobbering
- 128-bar arrangement sequenced via Pattern 0 Conductor at %16 (~2:47 duration)
"""

import os, sys

def build_uncontrollable_urge():
    lines = []
    def emit(s=""):
        lines.append(s)

    # ----------------------------------------------------
    # Header & Song Metadata
    # ----------------------------------------------------
    emit("## DEVO - UNCONTROLLABLE URGE (Full Post-Punk Showcase Cover for PULP / SKRED)")
    emit("#  184 BPM, Key of A Major / A Mixolydian, 128 bars (~2:47).")
    emit("#  Written by Mark Mothersbaugh & Gerald Casale (1978, Q: Are We Not Men? A: We Are Devo!).")
    emit("#  Self-contained: drums from built-in waves 82-99, inline K-synth riser.")
    emit("#  Uses n>d to convert note name strings (with '-' rests) to MIDI streams.")
    emit("#  Load:   [devo_uncontrollable_urge.sk] /ls")
    emit("#  Start:  play        Stop:  stop")
    emit("#")
    emit("#  VOICE MAP (all labeled via [name] vt):")
    emit("#    v0  motorik electronic kick (w82) + v5 sub punch (w0)")
    emit("#    v1  crack 70s snare (w83) + v4 tight clap (w86)")
    emit("#    v2  closed hat 8ths (w84)  v3  open hat offbeat (w85)")
    emit("#    v17 crash cymbal (w94)")
    emit("#    v18 hi tom (w90)  v19 mid tom (w91)  v20 lo tom (w92) = Simmons toms")
    emit("#    v6  Gerald Casale bass saw centre (w15) + v7 partner square right (w16)")
    emit("#    v8  Bob Mothersbaugh guitar root left (w15) + v9 fifth (w15), v10 octave (w16)")
    emit("#    v14 Bob Casale rhythm guitar root right (w16) + v15 fifth left (w15)")
    emit("#    v11 Mark Mothersbaugh Minimoog lead saw (w15) + v12 pulse partner (w16)")
    emit("#    v16 K-synth build riser (w301)")
    emit("#    v21 DEVO vocal lead centre (w15) + v22 left (w16), v23 right (w15)")
    emit("#    v24 DEVO FM metal stab / pick scrape (w0) + v25 modulator (w0)")
    emit("#")
    emit("#  DELAY BUSES:")
    emit("#    Bus 1: Vocal slapback sixteenth (DS 1 184 .25, DL 1 - - 5 0 0 9)")
    emit("#    Bus 2: Bass slapback sixteenth (DS 2 184 .25, DL 2 - - 4 0 0 8)")
    emit("#    Bus 3: Snare room reverb tail (DS 3 184 .5, DL 3 - - 6 0 0 9)")
    emit("#    Bus 4: Minimoog stereo chorus wash (DS 4 184 .75, DL 4 - - 8 3 6 10, DP 4 1)")
    emit("#")
    emit("#  ARRANGEMENT (128 bars):")
    emit("#    0-1   Alan Myers 4-count hi-hat clicks")
    emit("#    2-5   Intro F -> G build riff + machine-gun snare roll & sudden dead stop")
    emit("#    6-13  Main Unison Riff explosion & manic 'YEAH!' shouts")
    emit("#    14-29 Verse 1: 'Got an urge, got a surge, and it's out of control...'")
    emit("#    30-37 Chorus 1: 'It's got style, it's got class... And I say: YEAH!'")
    emit("#    38-41 Main Riff & 'YEAH!' reprise")
    emit("#    42-49 Start-Stop Breaks 1: sudden dead stops with metallic scrapes")
    emit("#    50-65 Verse 2: 'Got an urge, I wanna purge, 'cause I'm losing control...'")
    emit("#    66-73 Chorus 2: 'It's got style, it's got class...'")
    emit("#    74-89 Bridge & Bob Mothersbaugh Angular Guitar Solo + Minimoog madness")
    emit("#    90-97 Start-Stop Breaks 2: precision unison cuts")
    emit("#    98-105 Final Climax Chorus: maximum power, 16th hats, crashing cymbals")
    emit("#    106-121 Outro Vamp: frantic 'Uncontrollable Urge!' chanting")
    emit("#    122-125 Final 'Yeah Yeah' sprint & sudden precision cutoff")
    emit("#    126   Final unison crash hit")
    emit("#    127   Cleanup and stop")
    emit("# ======================================================================")
    emit()
    emit("[DEVO - UNCONTROLLABLE URGE] puts")
    emit("M 184 16")
    emit()

    # ----------------------------------------------------
    # Inline K-Synth Riser (Wave 301)
    # ----------------------------------------------------
    emit("## INLINE K-SYNTH RISER (one-shot wave 301)")
    emit("[")
    emit("/ riser: exponential sweep + highpass noise, zero DC")
    emit("N: 65536")
    emit("T: !N")
    emit("E: (e(T*(4.2%N)))%100")
    emit("R: r T")
    emit("L: 0.35 f R")
    emit("H: R-L")
    emit("F: 950*(e(T*(2.6%N)))*((p 2)%(p 0))")
    emit("P: +\\F")
    emit("S: s P")
    emit("W: w E*((S*.35)+(H*.55))")
    emit("] k!")
    emit("k>w301,44100,1")
    emit()

    # ----------------------------------------------------
    # Delay Buses Setup
    # ----------------------------------------------------
    emit("## DELAY BUSES")
    emit("# Bus 1: Vocal & guitar slapback sixteenth")
    emit("DS 1 184 .25")
    emit("DL 1 - - 5 0 0 9")
    emit("DD 1 7 3")
    emit()
    emit("# Bus 2: Bass slapback sixteenth (tight low-end punch)")
    emit("DS 2 184 .25")
    emit("DL 2 - - 4 0 0 8")
    emit("DD 2 6 3")
    emit()
    emit("# Bus 3: Snare room tail")
    emit("DS 3 184 .5")
    emit("DL 3 - - 6 0 0 9")
    emit("DD 3 5 4")
    emit()
    emit("# Bus 4: Minimoog stereo chorus wash")
    emit("DS 4 184 .75")
    emit("DL 4 - - 8 3 6 10")
    emit("DD 4 7 2")
    emit("DP 4 1")
    emit()

    # ----------------------------------------------------
    # Voices Setup
    # ----------------------------------------------------
    emit("## VOICES")
    emit("# Alan Myers motorik drums: kick, sub punch, crack snare, clap, hats, crash, toms")
    emit("v0 w82 f440 a7 p0 [kick motorik] vt")
    emit("v5 w0 a-2 p0 t.001,.18,0,.05 j1 k160 q.7 [kick sub punch] vt")
    emit("v1 w83 f440 a8 p0 r3 ds.18 [snare 70s crack] vt")
    emit("v4 w86 f440 a6 p0 [clap tight] vt")
    emit("v2 w84 f440 a-3 p.25 [closed hat] vt")
    emit("v3 w85 f440 a-2 p-.25 [open hat] vt")
    emit("v17 w94 f440 a5 p.2 [crash cymbal] vt")
    emit("v18 w90 f440 a6 p-.35 [hi tom] vt")
    emit("v19 w91 f360 a6 p0 [mid tom] vt")
    emit("v20 w92 f280 a6 p.35 [lo tom] vt")
    emit()
    emit("# Gerald Casale Bass: gritty, driving Moog/Rickenbacker saw + square pair")
    emit("v6 w15 a-1 p0 t.001,.14,0,.04 j1,2 k1400 q2.8 r2 ds.08 [casale bass saw] vt")
    emit("v6 >7")
    emit("v7 w16 a-5 N0,7 p.35 r0 [casale bass square] vt")
    emit("v6 G7 H7")
    emit()
    emit("# Bob Mothersbaugh ('Bob 1') Punk Guitar Power Chord Trio: root, fifth, octave")
    emit("# Crisp staccato attack, character drive j1,2, slight bitcrush, and delay slapback")
    emit("v8 w15 a-5 p-.30 t.001,.11,0,.03 j1,2 k3400 q1.2 c4,.20 r1 ds.12 [bob1 guitar root] vt")
    emit("v8 >9")
    emit("v8 >10")
    emit("v9 w15 a-7 N7,5 p.25 r1 [bob1 guitar fifth] vt")
    emit("v10 w16 a-9 N12,0 p-.45 r1 [bob1 guitar octave] vt")
    emit("v8 G9,10 H9,10")
    emit()
    emit("# Bob Casale ('Bob 2') Rhythm Guitar: stereo spread double-tracking")
    emit("v14 w16 a-7 p.40 t.001,.10,0,.03 j1 k3000 q1.0 c4,.15 [bob2 guitar root] vt")
    emit("v14 >15")
    emit("v15 w15 a-9 N7,7 p-.20 [bob2 guitar fifth] vt")
    emit("v14 G15 H15")
    emit()
    emit("# Mark Mothersbaugh Minimoog Lead & Quirky Stabs (centre-left saw + pulse partner)")
    emit("v11 w15 a-3 p-.20 t.002,.20,0,.05 j1 k2600 q4.0 g.03 r1 ds.25 [minimoog lead saw] vt")
    emit("v11 >12")
    emit("v12 w16 a-6 N0,12 p.20 [minimoog lead pulse] vt")
    emit("v11 G12 H12")
    emit()
    emit("# Inline K-Synth Riser Voice")
    emit("v16 w301 f440 a5 p0 [ksynth riser] vt")
    emit()
    emit("# DEVO Vocal Lead: Searing punk vocal synth trio with safe s=0 envelope")
    emit("v21 w15 a-2 p0 t.003,.16,0,.04 j1,2 k3600 q2.2 r1 ds.20 [devo vocal lead] vt")
    emit("v21 >22")
    emit("v21 >23")
    emit("v22 w16 a-6 N0,5 p-.40 [devo vocal left] vt")
    emit("v23 w15 a-6 N0,-5 p.40 [devo vocal right] vt")
    emit("v21 G22,23 H22,23")
    emit()
    emit("# DEVO FM Metal Stab / Pick Scrape (signature start-stop accent)")
    emit("v24 w0 a-3 p-.35 t.001,.30,0,.20 FF2 FB4 F25,2.4 G25 H25 [devo metal stab] vt")
    emit("v25 w0 a0 t.001,.20,0,.20 N21 m1 [stab modulator] vt")
    emit()

    # ----------------------------------------------------
    # Events Cleanup Responder
    # ----------------------------------------------------
    emit("## EVENTS (end-of-song cleanup responder)")
    emit("[O DQ] /ceb 4 99")
    emit("/cer 1")
    emit()

    # ----------------------------------------------------
    # Streams (Converted via n>d with '-' rests)
    # ----------------------------------------------------
    emit("## STREAMS (Note arrays converted via n>d with '-' rests)")
    emit()
    emit("# Stream 1: Intro F -> G Bass Riff (16 sixteenth steps)")
    emit("[F2 - F2 - F2 - G2 - - - - - - - - -] n>d /SS 1")
    emit()
    emit("# Stream 2: Intro F -> G Guitar Riff (16 sixteenth steps)")
    emit("[F3 - F3 - F3 - G3 - - - - - - - - -] n>d /SS 2")
    emit()
    emit("# Stream 3: Intro G Chug Bass (16 sixteenth steps)")
    emit("[G2 - G2 - G2 - G2 - G2 - G2 - G2 - - -] n>d /SS 3")
    emit()
    emit("# Stream 4: Intro G Chug Guitar (16 sixteenth steps)")
    emit("[G3 - G3 - G3 - G3 - G3 - G3 - G3 - - -] n>d /SS 4")
    emit()
    emit("# Stream 5: Main Unison Riff Bass (16 sixteenth steps: A2 -> G2 -> E2)")
    emit("[A2 - A2 - G2 - E2 - A2 - A2 - G2 - E2 -] n>d /SS 5")
    emit()
    emit("# Stream 6: Main Unison Riff Guitar (16 sixteenth steps: A3 -> G3 -> E3)")
    emit("[A3 - A3 - G3 - E3 - A3 - A3 - G3 - E3 -] n>d /SS 6")
    emit()
    emit("# Stream 7: Verse Bass Drone (16 sixteenth steps: 8 steady eighths on A2)")
    emit("[A2 - A2 - A2 - A2 - A2 - A2 - A2 - A2 -] n>d /SS 7")
    emit()
    emit("# Stream 8: Verse Guitar Chug (16 sixteenth steps: 8 steady eighths on A3)")
    emit("[A3 - A3 - A3 - A3 - A3 - A3 - A3 - A3 -] n>d /SS 8")
    emit()
    emit("# Stream 9: 'YEAH!' Manic Vocal Shouts (2 bars / 32 steps)")
    emit("[")
    emit("E5 - - - E5 - - - E5 - E5 - E5 - E5 -")
    emit("G5 - - - G5 - - - G5 - G5 - G5 - A5 -")
    emit("] n>d /SS 9")
    emit()
    emit("# Stream 10: Verse 1 Vocal Part 1 ('Got an urge, got a surge, and it's out of control...')")
    emit("# 4 bars / 64 sixteenth steps")
    emit("[")
    emit("A4 - A4 - C#5 - - - A4 - A4 - C#5 - - -")
    emit("D5 - D5 - C#5 - B4 - A4 - - - - - - -")
    emit("- - - - - - - - - - - - - - - -")
    emit("- - - - - - - - - - - - - - - -")
    emit("] n>d /SS 10")
    emit()
    emit("# Stream 11: Verse 1 Vocal Part 2 ('Got an urge, I wanna purge, cause I'm losing control...')")
    emit("# 4 bars / 64 sixteenth steps")
    emit("[")
    emit("A4 - A4 - C#5 - - - A4 - A4 - C#5 - - -")
    emit("D5 - D5 - C#5 - B4 - A4 - - - - - - -")
    emit("- - - - - - - - - - - - - - - -")
    emit("- - - - - - - - - - - - - - - -")
    emit("] n>d /SS 11")
    emit()
    emit("# Stream 12: Verse 1 Vocal Part 3 ('Uncontrollable urge, I wanna tell you all about it...')")
    emit("# 4 bars / 64 sixteenth steps")
    emit("[")
    emit("E5 - E5 - E5 - E5 - C#5 - - - - - - -")
    emit("A4 - A4 - B4 - C#5 - B4 - A4 - - - - -")
    emit("- - - - - - - - - - - - - - - -")
    emit("- - - - - - - - - - - - - - - -")
    emit("] n>d /SS 12")
    emit()
    emit("# Stream 13: Verse 1 Vocal Part 4 ('Uncontrollable urge, makes me scream and shout it...')")
    emit("# 4 bars / 64 sixteenth steps")
    emit("[")
    emit("E5 - E5 - E5 - E5 - C#5 - - - - - - -")
    emit("A4 - A4 - B4 - C#5 - B4 - A4 - - - - -")
    emit("- - - - - - - - - - - - - - - -")
    emit("- - - - - - - - - - - - - - - -")
    emit("] n>d /SS 13")
    emit()
    emit("# Stream 14: Chorus Bass Progression (D -> C -> F -> G -> D -> G -> A -> A)")
    emit("# 8 bars / 128 sixteenth steps")
    emit("[")
    emit("D2 - D2 - D2 - D2 - D2 - D2 - D2 - D2 -")
    emit("C2 - C2 - C2 - C2 - C2 - C2 - C2 - C2 -")
    emit("F2 - F2 - F2 - F2 - F2 - F2 - F2 - F2 -")
    emit("G2 - G2 - G2 - G2 - G2 - G2 - G2 - G2 -")
    emit("D2 - D2 - D2 - D2 - D2 - D2 - D2 - D2 -")
    emit("G2 - G2 - G2 - G2 - G2 - G2 - G2 - G2 -")
    emit("A2 - A2 - A2 - A2 - A2 - A2 - A2 - A2 -")
    emit("A2 - A2 - A2 - A2 - A2 - A2 - A2 - A2 -")
    emit("] n>d /SS 14")
    emit()
    emit("# Stream 15: Chorus Guitar Power Chords (8 bars / 128 sixteenth steps)")
    emit("[")
    emit("D3 - D3 - D3 - D3 - D3 - D3 - D3 - D3 -")
    emit("C3 - C3 - C3 - C3 - C3 - C3 - C3 - C3 -")
    emit("F3 - F3 - F3 - F3 - F3 - F3 - F3 - F3 -")
    emit("G3 - G3 - G3 - G3 - G3 - G3 - G3 - G3 -")
    emit("D3 - D3 - D3 - D3 - D3 - D3 - D3 - D3 -")
    emit("G3 - G3 - G3 - G3 - G3 - G3 - G3 - G3 -")
    emit("A3 - A3 - A3 - A3 - A3 - A3 - A3 - A3 -")
    emit("A3 - A3 - A3 - A3 - A3 - A3 - A3 - A3 -")
    emit("] n>d /SS 15")
    emit()
    emit("# Stream 16: Chorus Vocal Lead ('It's got style, it's got class... And I say: YEAH!')")
    emit("# 8 bars / 128 sixteenth steps")
    emit("[")
    emit("D5 - D5 - F#5 - - - - - - - - - - - -")
    emit("C5 - C5 - E5 - - - - - - - - - - - -")
    emit("F5 - F5 - A5 - - - - - - - - - - - -")
    emit("G5 - G5 - B5 - B5 - - - - - - - - - -")
    emit("D5 - D5 - D5 - D5 - E5 - F#5 - - - - -")
    emit("G5 - G5 - G5 - G5 - A5 - B5 - - - - -")
    emit("- - - - - - - - B5 - - - C#6 - - -")
    emit("C#6 - - - - - - - - - - - - - - -")
    emit("] n>d /SS 16")
    emit()
    emit("# Stream 17: Bob Mothersbaugh's Angular Guitar Solo (4 bars / 64 sixteenth steps)")
    emit("[")
    emit("A4 - C#5 - E5 - A5 - E5 - C#5 - A4 - C#5 -")
    emit("G5 - G5 - E5 - E5 - D5 - D5 - C5 - A4 -")
    emit("E5 - G5 - A5 - G5 - E5 - D5 - C#5 - A4 -")
    emit("A5 - A5 - A5 - A5 - G5 - E5 - G5 - A5 -")
    emit("] n>d /SS 17")
    emit()
    emit("# Stream 18: Outro Frantic Vocal Chants ('Uncontrollable urge! Out of control!')")
    emit("# 4 bars / 64 sixteenth steps")
    emit("[")
    emit("E5 - E5 - E5 - E5 - C#5 - - - A4 - - -")
    emit("D5 - D5 - C#5 - B4 - A4 - - - - - - -")
    emit("E5 - E5 - E5 - E5 - C#5 - - - A4 - - -")
    emit("G5 - G5 - G5 - G5 - A5 - - - - - - -")
    emit("] n>d /SS 18")
    emit()

    # ----------------------------------------------------
    # Patterns
    # ----------------------------------------------------
    emit("## PATTERNS")
    emit()

    # Pattern 1: Kick Motorik (on 1 & 3)
    emit("y1")
    emit("[kick motorik on 1 & 3] yt")
    emit("[v0 l1 v5 l.9] x0")
    emit("[v0 l1 v5 l.9] x8")
    emit("[=127,0] x15")
    emit()

    # Pattern 2: Crack Snare + Clap (on 2 & 4)
    emit("y2")
    emit("[snare+clap on 2 & 4] yt")
    emit("[v1 l1 v4 l.9] x4")
    emit("[v1 l1 v4 l.9] x12")
    emit("[=127,0] x15")
    emit()

    # Pattern 3: Closed Hats 8ths + Offbeat Open Sizzle
    emit("y3")
    emit("[hats 8ths + open sizzle] yt")
    emit("[v2 l.5] x0")
    emit("[v2 l.35] x2")
    emit("[v2 l.5] x4")
    emit("[v2 l.35] x6")
    emit("[v2 l.5] x8")
    emit("[v2 l.35] x10")
    emit("[v2 l.5] x12")
    emit("[v3 l.75 v2 l0] x14")
    emit("[=127,0] x15")
    emit()

    # Pattern 12: Relentless 16th Hats (Choruses & Climax)
    emit("y12")
    emit("[hats 16ths high energy] yt")
    for step in range(16):
        if step == 14:
            emit(f"[v3 l.8 v2 l0] x{step}")
        elif step == 15:
            emit(f"[v2 l.35 =127,0] x{step}")
        elif step % 2 == 0:
            emit(f"[v2 l.55] x{step}")
        else:
            emit(f"[v2 l.35] x{step}")
    emit()

    # Pattern 13: Machine-gun Snare Roll Crescendo
    emit("y13")
    emit("[snare roll crescendo] yt")
    emit("[v1 l0.25] x0")
    emit("[v1 l0.30] x1")
    emit("[v1 l0.35] x2")
    emit("[v1 l0.40] x3")
    emit("[v1 l0.45] x4")
    emit("[v1 l0.50] x5")
    emit("[v1 l0.55] x6")
    emit("[v1 l0.60] x7")
    emit("[v1 l0.65 z*2] x8")
    emit("[v1 l0.70 z*2] x9")
    emit("[v1 l0.75 z*2] x10")
    emit("[v1 l0.80 z*2] x11")
    emit("[v1 l0.85 z*4] x12")
    emit("[v1 l0.90 z*4] x13")
    emit("[v1 l1.00 z*4] x14")
    emit("[- =127,0] x15")
    emit()

    # Pattern 14: Intro Hi-Hat 4-Count Clicks
    emit("y14")
    emit("[intro hi-hat 4-count clicks] yt")
    emit("[v2 l.8] x0")
    emit("[v2 l.8] x4")
    emit("[v2 l.8] x8")
    emit("[v2 l.8 =127,0] x12")
    emit()

    # Pattern 15: Simmons Tom Fill
    emit("y15")
    emit("[simmons tom fill] yt")
    emit("[v18 l.9] x8")
    emit("[v18 l.9] x10")
    emit("[v19 l.9] x12")
    emit("[v20 l1 =127,0] x14")
    emit()

    # Pattern 16: Start-Stop Break Hit (Crash + Choked Power Chord + Pick Scrape)
    emit("y16")
    emit("[start-stop break hit and dead stop] yt")
    emit("[v0 l1 v5 l1 v17 l1 v6 n45 l.3 v8 n57 l.3 v14 n57 l.3] x0")
    emit("[v6 l0 v8 l0 v14 l0] x2")
    emit("[v24 n69 l.9] x12")
    emit("[v1 l.7] x14")
    emit("[=127,0] x15")
    emit()

    # Pattern 4: Intro F -> G Riff (1 bar / 16 steps, reads Stream 1 and Stream 2)
    emit("y4")
    emit("[intro F to G riff 1 bar] yt")
    for step in range(16):
        if step == 15:
            emit(f"[v6 n&1 l.9 v8 n&2 l.85 v14 n&2 l.75 =127,0] x{step}")
        else:
            emit(f"[v6 n&1 l.9 v8 n&2 l.85 v14 n&2 l.75] x{step}")
    emit()

    # Pattern 17: Intro G Chug & Build (1 bar / 16 steps, reads Stream 3 and Stream 4)
    emit("y17")
    emit("[intro G chug build 1 bar] yt")
    for step in range(16):
        if step == 15:
            emit(f"[v6 l0 v8 l0 v14 l0 =127,0] x{step}")
        else:
            emit(f"[v6 n&3 l.9 v8 n&4 l.85 v14 n&4 l.75] x{step}")
    emit()

    # Pattern 5: Verse Bass Drone (1 bar / 16 steps, reads Stream 7 with dynamic groove)
    emit("y5")
    emit("[verse bass drone on A2] yt")
    for step in range(16):
        if step == 15:
            emit(f"[v6 n&7 l.50 =127,0] x{step}")
        elif step % 4 == 0:
            emit(f"[v6 n&7 l.95] x{step}")
        elif step % 2 == 0:
            emit(f"[v6 n&7 l.75] x{step}")
        else:
            emit(f"[v6 n&7 l.50] x{step}")
    emit()

    # Pattern 6: Main Unison Bass Riff (1 bar / 16 steps, reads Stream 5)
    emit("y6")
    emit("[main unison bass riff A-G-E] yt")
    for step in range(16):
        if step == 15:
            emit(f"[v6 n&5 l.9 =127,0] x{step}")
        else:
            emit(f"[v6 n&5 l.9] x{step}")
    emit()

    # Pattern 7: Chorus Bass Progression (8 bars / 128 steps, reads Stream 14)
    emit("y7")
    emit("[chorus bass progression 8 bars] yt")
    for step in range(128):
        if step == 127:
            emit(f"[v6 n&14 l.9 =127,0] x{step}")
        else:
            emit(f"[v6 n&14 l.9] x{step}")
    emit()

    # Pattern 9: Verse Guitar Chug (1 bar / 16 steps, reads Stream 8 on v8 and v14)
    emit("y9")
    emit("[verse guitar staccato chug on A3] yt")
    for step in range(16):
        if step == 15:
            emit(f"[v8 n&8 l.40 v14 n&8 l.40 =127,0] x{step}")
        elif step % 4 == 0:
            emit(f"[v8 n&8 l.85 v14 n&8 l.75] x{step}")
        elif step % 2 == 0:
            emit(f"[v8 n&8 l.60 v14 n&8 l.55] x{step}")
        else:
            emit(f"[v8 n&8 l.40 v14 n&8 l.40] x{step}")
    emit()

    # Pattern 10: Main Unison Guitar Riff (1 bar / 16 steps, reads Stream 6)
    emit("y10")
    emit("[main unison guitar riff A-G-E] yt")
    for step in range(16):
        if step == 15:
            emit(f"[v8 n&6 l.9 v14 n&6 l.8 =127,0] x{step}")
        else:
            emit(f"[v8 n&6 l.9 v14 n&6 l.8] x{step}")
    emit()

    # Pattern 11: Chorus Guitar Power Chords (8 bars / 128 steps, reads Stream 15)
    emit("y11")
    emit("[chorus guitar power chords 8 bars] yt")
    for step in range(128):
        if step == 127:
            emit(f"[v8 n&15 l.85 v14 n&15 l.75 =127,0] x{step}")
        else:
            emit(f"[v8 n&15 l.85 v14 n&15 l.75] x{step}")
    emit()

    # Pattern 18: Bob Mothersbaugh Angular Guitar Solo (4 bars / 64 steps, reads Stream 17)
    emit("y18")
    emit("[bob mothersbaugh guitar solo 4 bars] yt")
    for step in range(64):
        if step == 63:
            emit(f"[v8 n&17 l.9 =127,0] x{step}")
        else:
            emit(f"[v8 n&17 l.9] x{step}")
    emit()

    # Pattern 21: 'YEAH!' Manic Vocal Shouts (2 bars / 32 steps, reads Stream 9)
    emit("y21")
    emit("[yeah manic vocal shouts 2 bars] yt")
    for step in range(32):
        if step == 31:
            emit(f"[v21 n&9 l1 =127,0] x{step}")
        else:
            emit(f"[v21 n&9 l1] x{step}")
    emit()

    # Pattern 22: Verse 1 Vocal Part 1 ('Got an urge, got a surge...') (4 bars / 64 steps, reads Stream 10)
    emit("y22")
    emit("[verse vocal part 1 got an urge] yt")
    for step in range(64):
        if step == 63:
            emit(f"[v21 n&10 l1 =127,0] x{step}")
        else:
            emit(f"[v21 n&10 l1] x{step}")
    emit()

    # Pattern 23: Verse 1 Vocal Part 2 ('Got an urge, I wanna purge...') (4 bars / 64 steps, reads Stream 11)
    emit("y23")
    emit("[verse vocal part 2 wanna purge] yt")
    for step in range(64):
        if step == 63:
            emit(f"[v21 n&11 l1 =127,0] x{step}")
        else:
            emit(f"[v21 n&11 l1] x{step}")
    emit()

    # Pattern 24: Verse 1 Vocal Part 3 ('Uncontrollable urge, tell you all about it...') (4 bars / 64 steps, reads Stream 12)
    emit("y24")
    emit("[verse vocal part 3 uncontrollable urge] yt")
    for step in range(64):
        if step == 63:
            emit(f"[v21 n&12 l1 =127,0] x{step}")
        else:
            emit(f"[v21 n&12 l1] x{step}")
    emit()

    # Pattern 25: Verse 1 Vocal Part 4 ('Uncontrollable urge, scream and shout it...') (4 bars / 64 steps, reads Stream 13)
    emit("y25")
    emit("[verse vocal part 4 scream and shout it] yt")
    for step in range(64):
        if step == 63:
            emit(f"[v21 n&13 l1 =127,0] x{step}")
        else:
            emit(f"[v21 n&13 l1] x{step}")
    emit()

    # Pattern 26: Chorus Vocal Lead ('It's got style, it's got class... And I say: YEAH!') (8 bars / 128 steps, reads Stream 16)
    emit("y26")
    emit("[chorus vocal lead 8 bars] yt")
    for step in range(128):
        if step == 127:
            emit(f"[v21 n&16 l1 =127,0] x{step}")
        else:
            emit(f"[v21 n&16 l1] x{step}")
    emit()

    # Pattern 27: Outro Frantic Vocal Chants ('Uncontrollable urge! Out of control!') (4 bars / 64 steps, reads Stream 18)
    emit("y27")
    emit("[outro vocal chants uncontrollable urge] yt")
    for step in range(64):
        if step == 63:
            emit(f"[v21 n&18 l1 =127,0] x{step}")
        else:
            emit(f"[v21 n&18 l1] x{step}")
    emit()

    # Pattern 28: Mark Mothersbaugh Quirky Minimoog Synth Bleeps
    emit("y28")
    emit("[minimoog quirky synth bleeps] yt")
    emit("[v11 n81 l.75] x6")
    emit("[v11 n88 l.85] x14")
    emit("[=127,0] x15")
    emit()

    # Pattern 29: Minimoog Chorus Arpeggio / Squeals
    emit("y29")
    emit("[minimoog chorus squeals] yt")
    emit("[v11 n76 l.8] x2")
    emit("[v11 n79 l.8] x6")
    emit("[v11 n81 l.85] x10")
    emit("[v11 n88 l.9 =127,0] x14")
    emit()

    # ----------------------------------------------------
    # Conductor (Pattern 0 at %16, 128 bars)
    # ----------------------------------------------------
    emit("## CONDUCTOR (Pattern 0, %16: 1 step = 1 bar / 16 sixteenths, 128 bars)")
    emit("y0")
    emit("[devo conductor 128 bars] yt")
    emit("%16")
    emit()

    # Section 1: Intro (Bars 0-5)
    # Bars 0-1: 4-count hi-hat clicks
    emit("# Bars 0-1: Alan Myers hi-hat count-in")
    emit("[y14 z1] x0")
    emit("[=126,1] x1")
    emit()

    # Bars 2-5: F -> G build riff
    emit("# Bars 2-4: Intro F -> G build riff (3 bars) + Bar 5: G chug & snare crescendo")
    emit("[y14 z0 y4 z1 /SP 1,0 /SP 2,0] x2")
    emit("[/SP 1,0 /SP 2,0] x3")
    emit("[/SP 1,0 /SP 2,0] x4")
    emit("[y4 z0 y17 z1 y13 z1 /SP 3,0 /SP 4,0] x5")
    emit()

    # Section 2: Main Unison Riff & "YEAH!" Explosion (Bars 6-13, 8 bars)
    emit("# Bars 6-13: Main Unison Riff explosion & 'YEAH!' manic shouts")
    emit("[v17 l1 y17 z0 y13 z0 y1 z1 y2 z1 y3 z1 y6 z1 y10 z1 y21 z1 /SP 5,0 /SP 6,0 /SP 9,0] x6")
    emit("[/SP 5,0 /SP 6,0] x7")
    emit("[/SP 5,0 /SP 6,0 /SP 9,0] x8")
    emit("[/SP 5,0 /SP 6,0] x9")
    emit("[/SP 5,0 /SP 6,0 /SP 9,0] x10")
    emit("[/SP 5,0 /SP 6,0] x11")
    emit("[/SP 5,0 /SP 6,0 /SP 9,0] x12")
    emit("[/SP 5,0 /SP 6,0 y15 z1] x13")
    emit()

    # Section 3: Verse 1 (Bars 14-29, 16 bars)
    # Driving A drone, staccato guitar chug, Minimoog bleeps, Mark's vocals
    emit("# Bars 14-17: Verse 1 Part 1 ('Got an urge, got a surge, and it's out of control...')")
    emit("[v17 l1 y6 z0 y10 z0 y21 z0 y15 z0 y5 z1 y9 z1 y28 z1 y22 z1 /SP 7,0 /SP 8,0 /SP 10,0] x14")
    emit("[=126,15] x15")
    emit("[=126,16] x16")
    emit("[=126,17] x17")
    emit()

    emit("# Bars 18-21: Verse 1 Part 2 ('Got an urge, I wanna purge, cause I'm losing control...')")
    emit("[y22 z0 y23 z1 /SP 11,0] x18")
    emit("[=126,19] x19")
    emit("[=126,20] x20")
    emit("[=126,21] x21")
    emit()

    emit("# Bars 22-25: Verse 1 Part 3 ('Uncontrollable urge, I wanna tell you all about it...')")
    emit("[y23 z0 y24 z1 /SP 12,0] x22")
    emit("[=126,23] x23")
    emit("[=126,24] x24")
    emit("[=126,25] x25")
    emit()

    emit("# Bars 26-29: Verse 1 Part 4 ('Uncontrollable urge, makes me scream and shout it...')")
    emit("[y24 z0 y25 z1 /SP 13,0] x26")
    emit("[=126,27] x27")
    emit("[=126,28] x28")
    emit("[y25 z0 y28 z0 y13 z1] x29")
    emit()

    # Section 4: Chorus 1 (Bars 30-37, 8 bars)
    # D -> C -> F -> G progression, 16th hats, power chords, screaming vocals
    emit("# Bars 30-37: Chorus 1 ('It's got style, it's got class... And I say: YEAH!')")
    emit("[v17 l1 y5 z0 y9 z0 y13 z0 y3 z0 y12 z1 y7 z1 y11 z1 y29 z1 y26 z1 /SP 14,0 /SP 15,0 /SP 16,0] x30")
    emit("[=126,31] x31")
    emit("[=126,32] x32")
    emit("[=126,33] x33")
    emit("[=126,34] x34")
    emit("[=126,35] x35")
    emit("[=126,36] x36")
    emit("[y15 z1] x37")
    emit()

    # Section 5: Re-Intro / Main Riff & 'YEAH!' Reprise (Bars 38-41, 4 bars)
    emit("# Bars 38-41: Re-Intro Main Riff & 'YEAH!' Reprise")
    emit("[v17 l1 y12 z0 y7 z0 y11 z0 y29 z0 y26 z0 y15 z0 y3 z1 y6 z1 y10 z1 y21 z1 /SP 5,0 /SP 6,0 /SP 9,0] x38")
    emit("[/SP 5,0 /SP 6,0] x39")
    emit("[/SP 5,0 /SP 6,0 /SP 9,0] x40")
    emit("[/SP 5,0 /SP 6,0 y15 z1] x41")
    emit()

    # Section 6: Start-Stop Breaks 1 (Bars 42-49, 8 bars)
    # Sudden precision stops on a dime with metallic scrapes!
    emit("# Bars 42-49: Start-Stop Breaks 1 (sudden dead stops & explosive pickups)")
    emit("[v17 l1 y6 z0 y10 z0 y21 z0 y15 z0 y5 z1 y9 z1 /SP 7,0 /SP 8,0] x42")
    emit("[y1 z0 y2 z0 y3 z0 y5 z0 y9 z0 y16 z1] x43")
    emit("[y16 z0 y1 z1 y2 z1 y3 z1 y5 z1 y9 z1 /SP 7,0 /SP 8,0] x44")
    emit("[y1 z0 y2 z0 y3 z0 y5 z0 y9 z0 y16 z1] x45")
    emit("[y16 z0 y1 z1 y2 z1 y3 z1 y5 z1 y9 z1 /SP 7,0 /SP 8,0] x46")
    emit("[y1 z0 y2 z0 y3 z0 y5 z0 y9 z0 y16 z1] x47")
    emit("[y16 z0 y1 z1 y2 z1 y3 z1 y5 z1 y9 z1 /SP 7,0 /SP 8,0] x48")
    emit("[y13 z1 y15 z1] x49")
    emit()

    # Section 7: Verse 2 (Bars 50-65, 16 bars)
    emit("# Bars 50-53: Verse 2 Part 1 ('Got an urge, got a surge, and it's out of control...')")
    emit("[v17 l1 y13 z0 y15 z0 y28 z1 y22 z1 /SP 7,0 /SP 8,0 /SP 10,0] x50")
    emit("[=126,51] x51")
    emit("[=126,52] x52")
    emit("[=126,53] x53")
    emit()

    emit("# Bars 54-57: Verse 2 Part 2 ('Got an urge, I wanna purge, cause I'm losing control...')")
    emit("[y22 z0 y23 z1 /SP 11,0] x54")
    emit("[=126,55] x55")
    emit("[=126,56] x56")
    emit("[=126,57] x57")
    emit()

    emit("# Bars 58-61: Verse 2 Part 3 ('Uncontrollable urge, I wanna tell you all about it...')")
    emit("[y23 z0 y24 z1 /SP 12,0] x58")
    emit("[=126,59] x59")
    emit("[=126,60] x60")
    emit("[=126,61] x61")
    emit()

    emit("# Bars 62-65: Verse 2 Part 4 ('Uncontrollable urge, makes me scream and shout it...')")
    emit("[y24 z0 y25 z1 /SP 13,0] x62")
    emit("[=126,63] x63")
    emit("[=126,64] x64")
    emit("[y25 z0 y28 z0 y13 z1] x65")
    emit()

    # Section 8: Chorus 2 (Bars 66-73, 8 bars)
    emit("# Bars 66-73: Chorus 2 ('It's got style, it's got class... And I say: YEAH!')")
    emit("[v17 l1 y5 z0 y9 z0 y13 z0 y3 z0 y12 z1 y7 z1 y11 z1 y29 z1 y26 z1 /SP 14,0 /SP 15,0 /SP 16,0] x66")
    emit("[=126,67] x67")
    emit("[=126,68] x68")
    emit("[=126,69] x69")
    emit("[=126,70] x70")
    emit("[=126,71] x71")
    emit("[=126,72] x72")
    emit("[y15 z1] x73")
    emit()

    # Section 9: Bridge & Bob Mothersbaugh Guitar Solo (Bars 74-89, 16 bars)
    emit("# Bars 74-89: Bridge & Bob Mothersbaugh Angular Guitar Solo + Minimoog sweeps")
    emit("[v17 l1 y12 z0 y7 z0 y11 z0 y29 z0 y26 z0 y15 z0 y3 z1 y5 z1 y18 z1 y28 z1 /SP 7,0 /SP 17,0] x74")
    emit("[=126,75] x75")
    emit("[=126,76] x76")
    emit("[=126,77] x77")
    emit("[/SP 17,0] x78")
    emit("[=126,79] x79")
    emit("[=126,80] x80")
    emit("[=126,81] x81")
    emit("[/SP 17,0] x82")
    emit("[=126,83] x83")
    emit("[=126,84] x84")
    emit("[=126,85] x85")
    emit("[/SP 17,0] x86")
    emit("[=126,87] x87")
    emit("[=126,88] x88")
    emit("[y18 z0 y28 z0 y13 z1 y15 z1] x89")
    emit()

    # Section 10: Start-Stop Breaks 2 (Bars 90-97, 8 bars)
    emit("# Bars 90-97: Start-Stop Breaks 2 (precision unison cuts)")
    emit("[v17 l1 y13 z0 y15 z0 y9 z1 /SP 7,0 /SP 8,0] x90")
    emit("[y1 z0 y2 z0 y3 z0 y5 z0 y9 z0 y16 z1] x91")
    emit("[y16 z0 y1 z1 y2 z1 y3 z1 y5 z1 y9 z1 /SP 7,0 /SP 8,0] x92")
    emit("[y1 z0 y2 z0 y3 z0 y5 z0 y9 z0 y16 z1] x93")
    emit("[y16 z0 y1 z1 y2 z1 y3 z1 y5 z1 y9 z1 /SP 7,0 /SP 8,0] x94")
    emit("[y1 z0 y2 z0 y3 z0 y5 z0 y9 z0 y16 z1] x95")
    emit("[y16 z0 y1 z1 y2 z1 y3 z1 y5 z1 y9 z1 /SP 7,0 /SP 8,0] x96")
    emit("[y13 z1 y15 z1] x97")
    emit()

    # Section 11: Final Climax Chorus (Bars 98-105, 8 bars)
    # Maximum volume, continuous crash accents, full wall of sound
    emit("# Bars 98-105: Final Climax Chorus (Maximum power wall of sound)")
    emit("[v17 l1 y5 z0 y9 z0 y13 z0 y15 z0 y3 z0 y12 z1 y7 z1 y11 z1 y29 z1 y26 z1 /SP 14,0 /SP 15,0 /SP 16,0] x98")
    emit("[=126,99] x99")
    emit("[=126,100] x100")
    emit("[=126,101] x101")
    emit("[v17 l1] x102")
    emit("[=126,103] x103")
    emit("[=126,104] x104")
    emit("[y15 z1] x105")
    emit()

    # Section 12: Outro Vamp (Bars 106-121, 16 bars)
    # Frantic 'Uncontrollable Urge!' chanting with rising tension
    emit("# Bars 106-121: Outro Vamp: Frantic 'Uncontrollable Urge!' chanting")
    emit("[v17 l1 y12 z0 y7 z0 y11 z0 y29 z0 y26 z0 y15 z0 y3 z1 y5 z1 y9 z1 y27 z1 /SP 7,0 /SP 8,0 /SP 18,0] x106")
    emit("[=126,107] x107")
    emit("[=126,108] x108")
    emit("[=126,109] x109")
    emit("[/SP 18,0] x110")
    emit("[=126,111] x111")
    emit("[=126,112] x112")
    emit("[=126,113] x113")
    emit("[v17 l1 /SP 18,0 y28 z1] x114")
    emit("[=126,115] x115")
    emit("[=126,116] x116")
    emit("[=126,117] x117")
    emit("[/SP 18,0] x118")
    emit("[=126,119] x119")
    emit("[=126,120] x120")
    emit("[y15 z1] x121")
    emit()

    # Section 13: Final 'Yeah Yeah' Sprint & Abrupt Stop (Bars 122-127, 6 bars)
    emit("# Bars 122-125: Final 'Yeah Yeah' sprint & sudden precision cutoff")
    emit("[v17 l1 y27 z0 y28 z0 y15 z0 y6 z1 y10 z1 y21 z1 /SP 5,0 /SP 6,0 /SP 9,0] x122")
    emit("[/SP 5,0 /SP 6,0] x123")
    emit("[/SP 5,0 /SP 6,0 /SP 9,0] x124")
    emit("[/SP 5,0 /SP 6,0 y13 z1] x125")
    emit()
    emit("# Bars 126-127: Final unison crash hit & cleanup")
    emit("[v0 l1 v5 l1 v17 l1 v6 n45 l1 v8 n57 l1 v14 n57 l1 y6 z0 y10 z0 y21 z0 y13 z0] x126")
    emit("[Z0 +4 ce 99] x127")
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
