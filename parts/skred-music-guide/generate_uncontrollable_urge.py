#!/usr/bin/env python3
"""
generate_uncontrollable_urge.py - Generates devo_uncontrollable_urge.sk and uncontrollable_urge.sk
for PULP / SKRED.

Restores the authentic, original arrangement and feel of devo_uncontrollable_urge.sk
exactly as it was, with:
- The iconic initial bass riff (y4): tonic A2 on 1, three pickups on G2 at 10, 12, 14.
- 184 BPM tempo (M 184 16) and 136-bar arrangement.
- All voices labeled with [name] vt tags.
- Note streams using [ ... ] n>d notation.
- Exact original drum, bass, guitar chug, and FM stab patterns.
"""

import os, sys

def build_uncontrollable_urge():
    lines = []
    def emit(s=""):
        lines.append(s)

    # ----------------------------------------------------
    # Header & Metadata
    # ----------------------------------------------------
    emit("## UNCONTROLLABLE URGE (instrumental cover) for PULP/SKRED")
    emit("#  184 BPM, key of A, 136 bars (about 2:57). Instrumental cover.")
    emit("#  Self-contained: drums are built-in waves 82-99.")
    emit("#     [devo_uncontrollable_urge.sk] /ls      then:   play     (stop:  stop)")
    emit("#")
    emit("#  VOICE MAP (all labeled via [name] vt):")
    emit("#    v0 kick  v1 snare  v2 closed hat  v3 open hat  v5 crash")
    emit("#    v6 bass saw (centre) + v7 detuned square partner (right)")
    emit("#    v8 guitar power chord: root, v9 fifth (+7), v10 octave (+12)")
    emit("#    v11 FM metal stab + v12 its modulator")
    emit("#  PATTERNS  0 conductor  1 kick  2 snare  3 hats  4 intro bass riff")
    emit("#            5 verse bass drone  8 chorus bass  9 chorus guitar")
    emit("#            10 verse guitar chug  11 stab  13 snare roll")
    emit("#  SECTIONS (bar numbers)")
    emit("#     0-7 intro   8-15 start-stop   16-31 verse   32-47 chorus")
    emit("#     48-55 start-stop   56-71 verse   72-87 chorus   88-95 start-stop")
    emit("#     96-111 final chorus   112-135 outro vamp   136 hard stop and cleanup")
    emit("# ======================================================================")
    emit()
    emit("[UNCONTROLLABLE URGE] puts")
    emit("M 184 16")
    emit()

    # ----------------------------------------------------
    # Delay Bus (stab echo only, the record is mostly dry)
    # ----------------------------------------------------
    emit("## DELAY BUS (stab echo only, the record is mostly dry)")
    emit("DS 1 184 .75")
    emit("DL 1 - - 6 2 2 9")
    emit("DD 1 7 3")
    emit()

    # ----------------------------------------------------
    # Voices Setup
    # ----------------------------------------------------
    emit("## VOICES")
    emit("v0 w82 f440 a6 p0 [kick motorik] vt")
    emit("v1 w83 f440 a7 p0 [snare 70s crack] vt")
    emit("v2 w84 f440 a-3 p.25 [closed hat] vt")
    emit("v3 w85 f440 a-2 p-.25 [open hat] vt")
    emit("v5 w94 f440 a4 p.2 [crash cymbal] vt")
    emit()
    emit("# bass: plucky saw plus detuned square partner on the right")
    emit("v6 w15 a-1 p0 t.002,.14,.25,.06 j1 k1300 q1.8 [bass saw] vt")
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
    emit("v11 w0 a-2 p-.35 t.001,.45,0,.35 FF2 FB4 F12,2.2 G12 H12 [fm metal stab] vt")
    emit("v12 w0 a0 t.001,.30,0,.30 N21 m1 [stab modulator] vt")
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

    # ----------------------------------------------------
    # Patterns
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
    emit()

    # Pattern 4: The authentic intro bass riff (tonic on 1, three pickups on last 3 eighths)
    emit("y4")
    emit("[intro bass riff: tonic on 1, three pickups on the last 3 eighths] yt")
    emit("[v6 n45 l1] x0")
    emit("[v6 n43 l.9] x10")
    emit("[v6 n43 l.9] x12")
    emit("[v6 n43 l.9] x14")
    emit("[=127,0] x15")
    emit()

    # Pattern 5: Verse bass drone
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

    # Pattern 9: Chorus guitar power chords, 4 bars
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

    # Pattern 13: Snare roll
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

    # ----------------------------------------------------
    # Conductor (Pattern 0, one step per bar, 136 bars)
    # ----------------------------------------------------
    emit("## CONDUCTOR (pattern 0, one step per bar, 136 bars)")
    emit("y0")
    emit("[conductor] yt")
    emit("%16")
    emit("[y4 z1] x0")
    emit("[=126,1] x1")
    emit("[y1 z1 y2 z1 y3 z1] x2")
    emit("[=126,3] x3")
    emit("[y10 z1] x4")
    emit("[=126,5] x5")
    emit("[y11 z1] x6")
    emit("[y11 z0 y1 z0 y2 z0 y13 z1] x7")
    emit("[v5 l1 y4 z0 y8 z0 y9 z0 y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 /SP 1,0 /SP 2,0] x8")
    emit("[=126,9] x9")
    emit("[y5 z0 y10 z0 y1 z0 y2 z0 y3 z0 y11 z1] x10")
    emit("[=126,11] x11")
    emit("[y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 /SP 1,0 /SP 2,0] x12")
    emit("[=126,13] x13")
    emit("[y5 z0 y10 z0 y1 z0 y2 z0 y3 z0 y11 z1] x14")
    emit("[y11 z0 y13 z1] x15")
    emit("[v5 l1 y4 z0 y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 /SP 1,0 /SP 2,0] x16")
    for b in range(17, 31):
        emit(f"[=126,{b}] x{b}")
    emit("[y2 z0 y13 z1] x31")
    emit("[v5 l1 y5 z0 y10 z0 y2 z1 y8 z1 y9 z1 /SP 3,0 /SP 4,0 /SP 1,0 /SP 2,0] x32")
    for b in range(33, 47):
        emit(f"[=126,{b}] x{b}")
    emit("[y2 z0 y13 z1] x47")
    emit("[v5 l1 y8 z0 y9 z0 y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 /SP 1,0 /SP 2,0] x48")
    emit("[=126,49] x49")
    emit("[y5 z0 y10 z0 y1 z0 y2 z0 y3 z0 y11 z1] x50")
    emit("[=126,51] x51")
    emit("[y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 /SP 1,0 /SP 2,0] x52")
    emit("[=126,53] x53")
    emit("[y5 z0 y10 z0 y1 z0 y2 z0 y3 z0 y11 z1] x54")
    emit("[y11 z0 y13 z1] x55")
    emit("[v5 l1 y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 /SP 1,0 /SP 2,0] x56")
    for b in range(57, 71):
        emit(f"[=126,{b}] x{b}")
    emit("[y2 z0 y13 z1] x71")
    emit("[v5 l1 y5 z0 y10 z0 y2 z1 y8 z1 y9 z1 /SP 3,0 /SP 4,0 /SP 1,0 /SP 2,0] x72")
    for b in range(73, 87):
        emit(f"[=126,{b}] x{b}")
    emit("[y2 z0 y13 z1] x87")
    emit("[v5 l1 y8 z0 y9 z0 y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 /SP 1,0 /SP 2,0] x88")
    emit("[=126,89] x89")
    emit("[y5 z0 y10 z0 y1 z0 y2 z0 y3 z0 y11 z1] x90")
    emit("[=126,91] x91")
    emit("[y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 /SP 1,0 /SP 2,0] x92")
    emit("[=126,93] x93")
    emit("[y5 z0 y10 z0 y1 z0 y2 z0 y3 z0 y11 z1] x94")
    emit("[y11 z0 y13 z1] x95")
    emit("[v5 l1 y5 z0 y10 z0 y2 z1 y8 z1 y9 z1 /SP 3,0 /SP 4,0 /SP 1,0 /SP 2,0] x96")
    for b in range(97, 104):
        emit(f"[=126,{b}] x{b}")
    emit("[v5 l1] x104")
    for b in range(105, 111):
        emit(f"[=126,{b}] x{b}")
    emit("[y2 z0 y13 z1] x111")
    emit("[v5 l1 y8 z0 y9 z0 y2 z1 y5 z1 y10 z1 y1 z1 y2 z1 y3 z1 y11 z0 /SP 1,0 /SP 2,0] x112")
    for b in range(113, 120):
        emit(f"[=126,{b}] x{b}")
    emit("[v5 l1 y11 z1] x120")
    for b in range(121, 127):
        emit(f"[=126,{b}] x{b}")
    emit("[y11 z0] x127")
    emit("[v5 l1] x128")
    for b in range(129, 135):
        emit(f"[=126,{b}] x{b}")
    emit("[y2 z0 y13 z1] x135")
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
