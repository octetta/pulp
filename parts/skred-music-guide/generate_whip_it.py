#!/usr/bin/env python3
"""
generate_whip_it.py - Generates whip_it.sk for PULP / SKRED.
An authentic, kick-ass instrumental cover of DEVO's 1980 classic "Whip It".
Written by Mark Mothersbaugh and Gerald Casale.

Features:
- Slowed snare sample (w83 at f175) layered with sharp transient (w87) and lash tail (w5)
  producing the authentic DEVO whip crack!
- Motorik 4/4 drum groove (Alan Myers style: punchy kick w82, crisp snare w83, clap w86, 16th hats w84/w85)
- Iconic E Mixolydian main riff (E2 E2 E2 E2 A2 B2 D3 -) in guitar + Moog bass unison
- Chorus chord progression (C - G - D - C) with driving 8th roots and polyphonic synth chords
- Full vocal synth lead lines for verses ("Crack that whip!") and chorus ("When a problem comes along...")
- Bob Mothersbaugh's angular guitar/synth solo
- All melodic material defined using [ ... ] n>d with '-' for rests!
- 92-bar arrangement sequenced on Pattern 0 at %16
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
    emit("#    v6  Moog bass saw (w15, centre) + v7 partner square (w16, right)")
    emit("#    v8  guitar-style chug (w16, left)")
    emit("#    v11 guitar / synth solo voice (w15) + v12 partner pulse (w16)")
    emit("#    v13 vocal lead synth (w15) + v14 partner saw (w15)")
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
    emit("#    4-7   Iconic E riff enters in guitar + Moog bass unison")
    emit("#    8-23  Verse 1: 'Crack that whip!' + whip cracks on offbeats")
    emit("#    24-39 Chorus 1: C - G - D - C progression, vocal lead + whip cracks")
    emit("#    40-47 Return to main riff + whip accents")
    emit("#    48-63 Verse 2: 'When something's going wrong...' + builds")
    emit("#    64-71 Bob Mothersbaugh's angular synth/guitar solo")
    emit("#    72-87 Final Chorus: full explosive wall of sound, continuous whip snaps")
    emit("#    88-91 Outro vamp & final whip crack flurry")
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
    emit("# Motorik drums")
    emit("v0 w82 f440 a7 p0")
    emit("v1 w83 f440 a8 p0 r3 ds.25")
    emit("v2 w84 f440 a-3 p.25")
    emit("v3 w85 f440 a-2 p-.25")
    emit("v4 w86 f440 a6 p0")
    emit("v5 w94 f440 a4 p.2")
    emit()
    emit("# Moog bass: snappy plucky saw + detuned square partner")
    emit("v6 w15 a-1 p0 t.001,.11,0,.04 j1,2 k1800 q3.5 r2 ds.08")
    emit("v6 >7")
    emit("v7 w16 a-5 N0,7 p.4 r0")
    emit("v6 G7 H7")
    emit()
    emit("# Guitar-style chug (tight, palm-muted square tone)")
    emit("v8 w16 a-6 p-.25 t.001,.09,0,.03 j1 k3400 q1 c4,.15")
    emit()
    emit("# DEVO Solo Lead: bright saw/pulse pair with phase distortion c4, delay Bus 1")
    emit("v11 w15 a-6 p0 t.002,.20,.5,.10 j1 k3800 q2.2 c4,.25 r1 ds.35")
    emit("v11 >12")
    emit("v12 w16 a-9 N0,9 p.55 r0")
    emit("v11 G12 H12")
    emit()
    emit("# Vocal Lead Synth: Mark Mothersbaugh / Gerald Casale vocal line")
    emit("v13 w15 a-6 p0 t.003,.28,.6,.18 j1,2 k3000 q1.8 g.015 c4,.20 r1 ds.3")
    emit("v13 >14")
    emit("v14 w15 a-8 N0,10 p-.5 r0")
    emit("v13 G14 H14")
    emit()
    emit("# ====================================================================")
    emit("# THE WHIP CRACK: Snare played at a slower rate (w83 at f175)")
    emit("# Layered with sharp rim crack (w87) and noise lash tail (w5)")
    emit("# ====================================================================")
    emit("v21 w83 f175 a9 p0 r1 ds.35")
    emit("v22 w87 f350 a7 p-.1")
    emit("v23 w5 f1400 a-3 p.2 t.001,.09,0,.03 j2 k2800 q2")
    emit()
    emit("# Riser: stretched K-synth one-shot")
    emit("v16 w301 f160 a8 p0 t.005,.01,1,.4 r4 ds.3")
    emit()
    emit("# Pad: Poly pool, 2-voice prototype (detuned stereo saw pair), 5 instances at v28-v37")
    emit("v24 w0 f.30 a0 t0,0,1,0 m1")
    emit("v26 w15 a-10 p-.5 t1.0,.4,.8,1.2 j1 k2200 q.8 c4,.2 C24,.2 G27 H27")
    emit("v27 w15 a-12 p.5 N0,8 t1.0,.4,.8,1.2 j1 k2200 q.8 c4,.2 C24,.2")
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
    emit("# Stream 1: DEVO iconic main riff (E E E E A B D -) – 1 bar of eighths")
    emit("[E2 E2 E2 E2 A2 B2 D3 -] n>d /SS 1")
    emit()
    emit("# Stream 2: Main riff velocities / accents")
    emit("[1 .75 .9 .75 1 .85 1 -] n>d /SS 2")
    emit()
    emit("# Stream 3: Verse ending turnaround (A G F# E)")
    emit("[A2 A2 G2 G2 F#2 F#2 E2 -] n>d /SS 3")
    emit()
    emit("# Stream 4: Chorus bass roots (C G D C, 2 bars of eighths)")
    emit("[C2 C2 C2 C2 C2 C2 C2 C2  G1 G1 G1 G1 G1 G1 G1 G1  D2 D2 D2 D2 D2 D2 D2 D2  C2 C2 C2 C2 C2 C2 C2 C2] n>d /SS 4")
    emit()
    emit("# Stream 5: Verse 1 Vocal Line ('Crack that whip! ... Give the past a slip!')")
    emit("# 2 bars / 32 sixteenth steps")
    emit("[B3 - B3 - B3 - - - - - - - - - - -  B3 - B3 - B3 - B3 - B3 - - - - - - -] n>d /SS 5")
    emit()
    emit("# Stream 6: Chorus Vocal Line ('When a problem comes along... you must whip it!')")
    emit("# 4 bars / 64 sixteenth steps")
    emit("[G4 - E4 - G4 - E4 - G4 - E4 - G4 - - -  - - E4 - G4 - E4 - E4 - - - - - - -  A4 - F#4 - A4 - F#4 - A4 - F#4 - A4 - F#4 -  - - E4 - G4 - E4 - E4 - - - - - - -] n>d /SS 6")
    emit()
    emit("# Stream 7: Bob Mothersbaugh's Angular Synth Solo (2 bars / 32 eighth notes)")
    emit("[E4 G4 A4 B4  D5 B4 A4 G4  E4 G4 A4 B4  D5 E5 D5 B4  E4 G4 A4 B4  D5 B4 A4 G4  A4 B4 D5 E5  G5 E5 D5 B4] n>d /SS 7")
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

    # Pattern 18: Relentless 16th Hats (High energy sections)
    emit("y18")
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

    # Pattern 6: The Iconic Main Riff (E E E E A B D -)
    # Plays at eighth notes (%2 clock), reads Stream 1 and 2
    emit("y6")
    emit("[iconic main riff] yt")
    emit("%2")
    for step in range(8):
        emit(f"[v6 n&1 l&2] x{step}")
    emit("[=127,0] x7")
    emit()

    # Pattern 8: Guitar Chug in Unison with Main Riff
    emit("y8")
    emit("[guitar chug in unison] yt")
    emit("%2")
    for step in range(8):
        emit(f"[v8 n&1 l.8] x{step}")
    emit("[=127,0] x7")
    emit()

    # Pattern 7: Chorus Bass Roots (C G D C, reads Stream 4)
    emit("y7")
    emit("[chorus bass roots] yt")
    emit("%2")
    for step in range(16):
        emit(f"[v6 n&4 l1 v8 n&4 l.75] x{step}")
    emit("[=127,0] x15")
    emit()

    # Pattern 9: Chorus Synth Chords (Cmaj -> Gmaj -> Dmaj -> Cmaj)
    emit("y9")
    emit("[chorus polyphonic synth chords] yt")
    # C major (bars 1 & 4)
    emit("[pn 0,101,48,.95 pn 0,102,52,.95 pn 0,103,55,.95] x0")
    emit("[pr 0,101 pr 0,102 pr 0,103] x15")
    # G major (bar 2)
    emit("[pn 0,104,43,.95 pn 0,105,47,.95 pn 0,106,50,.95] x16")
    emit("[pr 0,104 pr 0,105 pr 0,106] x31")
    # D major (bar 3)
    emit("[pn 0,107,45,.95 pn 0,108,50,.95 pn 0,109,54,.95] x32")
    emit("[pr 0,107 pr 0,108 pr 0,109] x47")
    # C major (bar 4)
    emit("[pn 0,101,48,.95 pn 0,102,52,.95 pn 0,103,55,.95] x48")
    emit("[pr 0,101 pr 0,102 pr 0,103] x63")
    emit("[=127,0] x63")
    emit()

    # Pattern 5: Verse 1 Vocal Line ('Crack that whip! ... Give the past a slip!')
    # 2 bars / 32 sixteenth steps
    emit("y5")
    emit("[vocal lead verse 1] yt")
    for step in range(32):
        emit(f"[v13 n&5 l.95] x{step}")
    emit("[=127,0] x31")
    emit()

    # Pattern 12: Chorus Vocal Line ('When a problem comes along... you must whip it!')
    # 4 bars / 64 sixteenth steps
    emit("y12")
    emit("[vocal lead chorus line] yt")
    for step in range(64):
        emit(f"[v13 n&6 l1] x{step}")
    emit("[=127,0] x63")
    emit()

    # Pattern 11: Bob Mothersbaugh's Angular Synth Solo (reads Stream 7)
    # Plays at eighth notes (%2 clock) over 2 bars (16 eighths)
    emit("y11")
    emit("[devo synth solo] yt")
    emit("%2")
    for step in range(16):
        emit(f"[v11 n&7 l.95] x{step}")
    emit("[=127,0] x15")
    emit()

    # Pattern 10: THE ICONIC WHIP CRACK!
    # Fired on beat 2-and and 4-and right after the vocal calls
    emit("y10")
    emit("[the whip crack!] yt")
    emit("[v21 l1 v22 l1 v23 l1] x6")
    emit("[v21 l1 v22 l1 v23 l1] x14")
    emit("[=127,0] x15")
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

    # Bars 0-3: Intro Drums (Alan Myers motorik kick + 16th hats enter alone!)
    c_add(0, "y1 z1 y3 z1 v6 k1800 v7 k1800")
    c_add(2, "y2 z1")

    # Bars 4-7: Main Riff enters in guitar + Moog bass unison!
    c_add(4, "v5 l1 y6 z1 y8 z1 /SP 1,0 /SP 2,0")
    c_add(6, "y10 z1") # First whip crack!
    c_add(7, "y13 z1") # Snare roll into Verse 1

    # Bars 8-23: Verse 1 ('Crack that whip!')
    c_add(8, "v5 l1 y5 z1 /SP 5,0 y18 z1 y3 z0")
    c_add(10, "/SP 5,0 y5 z1")
    c_add(12, "/SP 5,0 y5 z1")
    c_add(14, "/SP 5,0 y5 z1")
    c_add(16, "v5 l.8 /SP 5,0 y5 z1")
    c_add(18, "/SP 5,0 y5 z1")
    c_add(20, "/SP 5,0 y5 z1")
    c_add(22, "y16 z1 v16 l1") # Build riser
    c_add(23, "y2 z0 y13 z1")

    # Bars 24-39: Chorus 1 ('When a problem comes along, you must whip it!')
    # C -> G -> D -> C (4 bars repeated 4 times)
    c_add(24, "v5 l1 y6 z0 y8 z0 y7 z1 /SP 4,0 y9 z1 y12 z1 /SP 6,0 y14 z1 y2 z1 v6 k2600 v7 k2400")
    c_add(28, "/SP 4,0 y7 z1 /SP 6,0 y12 z1")
    c_add(32, "v5 l.8 /SP 4,0 y7 z1 /SP 6,0 y12 z1")
    c_add(36, "/SP 4,0 y7 z1 /SP 6,0 y12 z1")
    c_add(39, "y2 z0 y13 z1")

    # Bars 40-47: Return to Main Riff + Whip Accents
    c_add(40, "v5 l1 y7 z0 y9 z0 y12 z0 y14 z0 y6 z1 y8 z1 /SP 1,0 /SP 2,0 y2 z1 v6 k1800 v7 k1800")
    c_add(44, "y10 z1")
    c_add(47, "y13 z1")

    # Bars 48-63: Verse 2 ('When something's going wrong...')
    c_add(48, "v5 l1 y5 z1 /SP 5,0 y18 z0 y3 z1")
    c_add(52, "/SP 5,0 y5 z1 y18 z1 y3 z0")
    c_add(56, "v5 l.8 /SP 5,0 y5 z1")
    c_add(60, "v16 l1 y10 z1")
    c_add(63, "y2 z0 y13 z1")

    # Bars 64-71: Bob Mothersbaugh's Angular Synth / Guitar Solo!
    c_add(64, "v5 l1 y11 z1 /SP 7,0 y2 z1")
    c_add(68, "/SP 7,0 y11 z1 y10 z1")
    c_add(71, "y11 z0 y2 z0 y13 z1")

    # Bars 72-87: Final Chorus (Maximum Energy Wall of Sound + Continuous Whip Snaps!)
    c_add(72, "v5 l1 y6 z0 y8 z0 y7 z1 /SP 4,0 y9 z1 y12 z1 /SP 6,0 y14 z1 y2 z1 v6 k2800 v7 k2600")
    c_add(76, "v5 l.8 /SP 4,0 y7 z1 /SP 6,0 y12 z1")
    c_add(80, "v5 l.8 /SP 4,0 y7 z1 /SP 6,0 y12 z1")
    c_add(84, "/SP 4,0 y7 z1 /SP 6,0 y12 z1")
    c_add(87, "y13 z1")

    # Bars 88-91: Outro Vamp & Final Whip Flurry
    c_add(88, "v5 l1 y7 z0 y9 z0 y12 z0 y14 z0 y6 z1 y8 z1 /SP 1,0 /SP 2,0 y10 z1")
    c_add(90, "v5 l.8 y10 z1")
    c_add(91, "y1 z0 y2 z0 y18 z0 y6 z0 y8 z0 v21 l1 v22 l1 v23 l1")

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
    # Write to whip_it.sk and also update whip-it.sk so both names work
    for filename in ["whip_it.sk", "whip-it.sk"]:
        out_path = os.path.join(guide_dir, filename)
        with open(out_path, "w") as f:
            f.write(song_text)
        print(f"Wrote {len(song_text.splitlines())} lines to {out_path}")
