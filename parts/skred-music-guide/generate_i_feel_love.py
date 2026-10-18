#!/usr/bin/env python3
"""
generate_i_feel_love.py - Generates i_feel_love.sk for PULP / SKRED

Full Arrangement Instrumental / Electronic Disco Cover of Donna Summer & Giorgio Moroder's
1977 masterpiece "I Feel Love".

Features:
- The iconic Moog Modular 16th-note bouncing bassline (C -> Eb -> F -> G progression)
- Real 16th slapback/ping-pong delay sync on Bus 1 recreating Moroder's galloping tape delay
- Donna Summer's singing vocal leads ("Ooh, it's so good...", "Ooh, I feel love...") with safe s=0 decay envelopes
- Cascading sparkling high arpeggio layer (w16 pulse with phase distortion)
- Warm 1970s Solina/ARP disco string ensemble chords (w15 saw trio)
- Authentic Moroder white noise wind sweeps and filter modulation
- Full 4-on-the-floor disco drums: punchy electronic kick, sub boom, claps, crisp snare, running 16th hats
- Legendary delay freeze breakdown (DF 1 1) with sweeping filter build
- 96-bar arrangement sequenced via Pattern 0 Conductor at %16
- All voices clearly tagged and inspected via [name] vt
"""

import os, sys, math

def build_i_feel_love():
    lines = []
    
    def emit(s=""):
        lines.append(s)

    # ----------------------------------------------------
    # Header & Song Metadata
    # ----------------------------------------------------
    emit("## DONNA SUMMER / GIORGIO MORODER - I FEEL LOVE (Instrumental Cover for PULP / SKRED)")
    emit("#  128 BPM, C minor / modal progression (C -> Eb -> F -> G), 96 bars (~3:00).")
    emit("#  Produced by Giorgio Moroder & Pete Bellotte (1977).")
    emit("#  Self-contained: drums from built-in waves 82-99, inline K-synth riser.")
    emit("#  Uses n>d to convert note name strings (with '-' rests) to MIDI note streams.")
    emit("#  Load:   [i_feel_love.sk] /ls")
    emit("#  Start:  play        Stop:  stop")
    emit("#")
    emit("#  VOICE MAP (all labeled via [name] vt):")
    emit("#    v0  four-on-the-floor kick (w82) + v5 sub boom (w0)")
    emit("#    v1  disco snare (w83) + v4 electronic clap (w86)")
    emit("#    v2  closed hat 16ths (w84)  v3  open hat offbeat (w85)")
    emit("#    v6  Moog bass saw centre (w15) + v7 partner square (w16, right)")
    emit("#    v8  Donna Summer vocal lead centre (w15) + v9 (left w15), v10 (right w16)")
    emit("#    v11 Donna chorus falsetto lead centre (w15) + v12 partner (w16)")
    emit("#    v13 high sparkle arpeggio centre (w16) + v14 partner saw (w15)")
    emit("#    v16 K-synth build riser (w301)")
    emit("#    v17 crash cymbal (w94)")
    emit("#    v18 hi tom (w90)  v19 mid tom (w91)  v20 lo tom (w92) = Simmons tom roll")
    emit("#    v21 Moroder white noise wind sweep (w5 resonant filter sweep)")
    emit("#    v26 disco strings root (w15) + v27 3rd (w15 left), v28 5th (w16 right)")
    emit("#")
    emit("#  DELAY BUSES:")
    emit("#    Bus 1: Moog bass 16th slapback / ping-pong gallop (DS 1 128 .25, DP 1 1)")
    emit("#    Bus 2: Donna vocal lead tape echo (DS 2 128 .75, dotted eighth)")
    emit("#    Bus 3: Snare & Clap room tail + sparkle arp slap (DS 3 128 .5)")
    emit("#    Bus 4: Disco strings & riser stereo hall wash (DS 4 128 1, DP 4 1)")
    emit("#")
    emit("#  ARRANGEMENT (96 bars):")
    emit("#    0-7   Intro: Lone Moog bass starts dark, 16th hats fade in, sparkle arp enters")
    emit("#    8-15  Groove drops: 4-on-floor kick slams in, filter sweep opens, claps & strings")
    emit("#    16-31 Verse 1: Donna Summer vocal lead ('Ooh, it's so good...'), C -> Eb -> F/G")
    emit("#    32-47 Chorus 1: 'Ooh, I feel love...' soaring hook, sparkling arps, wall of sound")
    emit("#    48-55 Breakdown: Drums drop out, delay freezes (DF 1 1), cosmic floating drone")
    emit("#    56-63 Re-build: Kick returns, white noise wind rises, filter opens, tom roll build")
    emit("#    64-79 Chorus 2 & Climax: Full wall of sound, energetic electronic funk")
    emit("#    80-87 Extended Climax Vamp: High vocal ad-libs, modulating through C-Eb-F-G")
    emit("#    88-95 Outro: Drums thin out, bass returns to C, hats half-time, final delay freeze")
    emit("#    96    End cleanup")
    emit("# ======================================================================")
    emit()
    emit("[DONNA SUMMER - I FEEL LOVE] puts")
    emit("M 128 16")
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
    emit("L: 0.32 f R")
    emit("H: R-L")
    emit("F: 750*(e(T*(2.5%N)))*((p 2)%(p 0))")
    emit("P: +\\F")
    emit("S: s P")
    emit("W: w E*((S*.35)+(H*.55))")
    emit("] k!")
    emit("k>w301,44100,1")
    emit()

    # ----------------------------------------------------
    # Delay Buses (Tempo Synced at 128 BPM)
    # ----------------------------------------------------
    emit("## DELAY BUSES")
    emit("# Bus 1: Moog bass 16th slapback / ping-pong gallop (creates the Moroder pulse!)")
    emit("DS 1 128 .25")
    emit("DL 1 - - 8 2 2 11")
    emit("DD 1 6 2")
    emit("DP 1 1")
    emit()
    emit("# Bus 2: Donna Summer vocal lead dotted-eighth space echo")
    emit("DS 2 128 .75")
    emit("DL 2 - - 8 3 4 10")
    emit("DD 2 7 2")
    emit("DP 2 1")
    emit()
    emit("# Bus 3: Snare & Clap room tail + sparkle slap")
    emit("DS 3 128 .5")
    emit("DL 3 - - 6 1 1 8")
    emit("DD 3 7 3")
    emit()
    emit("# Bus 4: Disco strings & riser stereo hall wash")
    emit("DS 4 128 1")
    emit("DL 4 - - 10 3 6 11")
    emit("DD 4 7 2")
    emit("DP 4 1")
    emit()

    # ----------------------------------------------------
    # Voices Setup
    # ----------------------------------------------------
    emit("## VOICES")
    emit("# Drums: four-on-the-floor kick + sub boom")
    emit("v0 w82 f440 a7 p0 [kick electronic] vt")
    emit("v5 w0 a-2 p0 t.001,.22,0,.06 j1 k160 q.7 [kick sub punch] vt")
    emit()
    emit("# Crisp disco snare, claps, hats, crash")
    emit("v1 w83 f440 a7 p0 r3 ds.25 [snare 80s gated] vt")
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
    emit("# The Iconic Moog Modular Bassline: punchy resonant saw (centre) + detuned square (right)")
    emit("# Sends ds.35 to Bus 1 for the famous galloping 16th tape-echo bounce!")
    emit("v6 w15 a-2 p0 t.001,.14,0,.04 j1,2 k1200 q3.8 g0 r1 ds.35 [moog bass saw] vt")
    emit("v6 >7")
    emit("v7 w16 a-6 N0,8 p.45 r0 [moog bass square] vt")
    emit("v6 G7 H7")
    emit()
    emit("# Donna Summer Vocal Lead (Verse): ethereal, floating singing synth with smooth glide")
    emit("# Soft attack (60ms) and long singing decay (1.8s) eliminate the chopped/spastic transient")
    emit("v8 w15 a-6 p0 t.06,1.8,0,.45 j1 k2300 q1.1 g.09 c4,.15 r2 ds.40 [donna vocal lead] vt")
    emit("v8 >9")
    emit("v8 >10")
    emit("v9 w15 a-8 N0,9 p-.5 r0 [donna vocal left] vt")
    emit("v10 w16 a-9 N0,-9 p.5 r0 [donna vocal right] vt")
    emit("v8 G9,10 H9,10")
    emit()
    emit("# Donna Summer Chorus Falsetto Hook ('I feel love, I feel love...')")
    emit("# Silky attack and long decay with 90ms portamento glide")
    emit("v11 w15 a-5 p0 t.06,1.6,0,.40 j1 k2600 q1.2 g.09 c4,.20 r2 ds.40 [donna chorus hook] vt")
    emit("v11 >12")
    emit("v12 w16 a-8 N0,9 p.5 r0 [donna chorus partner] vt")
    emit("v11 G12 H12")
    emit()
    emit("# High Sparkle Arpeggio: bright detuned pulse/saw pair with phase distortion c4, delay Bus 3")
    emit("v13 w16 a-9 p-.35 t.001,.16,0,.06 j1 k4500 q2.0 c4,.2 r3 ds.25 [high sparkle arp] vt")
    emit("v13 >14")
    emit("v14 w15 a-11 N0,12 p.35 r0 [sparkle partner saw] vt")
    emit("v13 G14 H14")
    emit()
    emit("# Giorgio Moroder White Noise Wind Sweep (resonant filter wash)")
    emit("v21 w5 a-5 p0 t.04,1.4,0,.4 j1,1 k600 q4 [moroder noise wind] vt")
    emit()
    emit("# Riser: stretched K-synth one-shot")
    emit("v16 w301 f220 a6 p0 t.005,.01,1,.35 r4 ds.3 [ksynth riser] vt")
    emit()
    emit("# 1970s Solina / ARP Disco Strings (Trio: Root, 3rd, 5th, delay Bus 4)")
    emit("v26 w15 a-8 p0 t.06,.50,.4,.30 j1 k2200 q1.2 c4,.2 r4 ds.3 [disco strings root] vt")
    emit("v27 w15 a-9 p-.55 t.06,.50,.4,.30 j1 k2200 q1.2 c4,.2 r4 ds.3 [disco strings 3rd] vt")
    emit("v28 w16 a-9 p.55 t.06,.50,.4,.30 j1 k2200 q1.2 c4,.2 r4 ds.3 [disco strings 5th] vt")
    emit()

    # ----------------------------------------------------
    # Events
    # ----------------------------------------------------
    emit("## EVENTS")
    emit("[O DQ] /ceb 4 99")
    emit("/cer 1")
    emit()

    # ----------------------------------------------------
    # Streams (Defined using [ ... ] n>d with '-' for rests!)
    # ----------------------------------------------------
    emit("## STREAMS (Note arrays converted via n>d with '-' rests)")
    emit("# Stream 1: Active Moog Bassline (16 steps / 1 bar, starts in C minor)")
    emit("# Note pattern: C2 C2 C3 C3  G2 G2 Bb2 Bb2  C2 C2 C3 C3  G2 G2 Bb2 Bb2")
    emit("[C2 C2 C3 C3  G2 G2 Bb2 Bb2  C2 C2 C3 C3  G2 G2 Bb2 Bb2] n>d /SS 1")
    emit()
    emit("# Stream 2: Bank - C Minor Bass Cell")
    emit("[C2 C2 C3 C3  G2 G2 Bb2 Bb2  C2 C2 C3 C3  G2 G2 Bb2 Bb2] n>d /SS 2")
    emit()
    emit("# Stream 3: Bank - Eb Major Bass Cell")
    emit("[Eb2 Eb2 Eb3 Eb3  Bb2 Bb2 Db3 Db3  Eb2 Eb2 Eb3 Eb3  Bb2 Bb2 Db3 Db3] n>d /SS 3")
    emit()
    emit("# Stream 4: Bank - F Major Bass Cell")
    emit("[F2 F2 F3 F3  C3 C3 Eb3 Eb3  F2 F2 F3 F3  C3 C3 Eb3 Eb3] n>d /SS 4")
    emit()
    emit("# Stream 5: Bank - G Minor / G7 Bass Cell")
    emit("[G2 G2 G3 G3  D3 D3 F3 F3  G2 G2 G3 G3  D3 D3 F3 F3] n>d /SS 5")
    emit()
    emit("# Stream 8: Active Sparkle Arp (16 steps / 1 bar, starts in C)")
    emit("[C5 Eb5 G5 C6  Eb5 G5 C6 Eb6  C5 Eb5 G5 C6  Eb5 G5 C6 Eb6] n>d /SS 8")
    emit()
    emit("# Stream 9: Bank - Sparkle Arp in Eb")
    emit("[Eb5 G5 Bb5 Eb6  G5 Bb5 Eb6 G6  Eb5 G5 Bb5 Eb6  G5 Bb5 Eb6 G6] n>d /SS 9")
    emit()
    emit("# Stream 10: Bank - Sparkle Arp in F")
    emit("[F5 A5 C6 F6  A5 C6 F6 A6  F5 A5 C6 F6  A5 C6 F6 A6] n>d /SS 10")
    emit()
    emit("# Stream 11: Bank - Sparkle Arp in G")
    emit("[G5 Bb5 D6 G6  Bb5 D6 G6 Bb6  G5 Bb5 D6 G6  Bb5 D6 G6 Bb6] n>d /SS 11")
    emit()

    # ----------------------------------------------------
    # Patterns
    # ----------------------------------------------------
    emit("## PATTERNS")
    emit()

    # Pattern 1: Four-on-the-floor Electronic Kick
    emit("y1")
    emit("[kick 4-on-the-floor] yt")
    emit("[v0 l1 v5 g0 n52 g.06 n30 l1] x0,8")
    emit("[v0 l.95 v5 g0 n50 g.06 n30 l.9] x4,12")
    emit("[=127,0] x15")
    emit()

    # Pattern 2: Gated Snare + Electronic Clap on 2 & 4
    emit("y2")
    emit("[snare+clap on 2 & 4] yt")
    emit("[v1 l1 v4 l.95] x4,12")
    emit("[v1 l.25] x6")
    emit("[v1 l.20] x11")
    emit("[v1 l.30] x14")
    emit("[=127,0] x15")
    emit()

    # Pattern 3: Running 16th Closed Hats with Accents & Offbeat Open Sizzle
    emit("y3")
    emit("[hats 16ths groove + offbeat sizzle] yt")
    emit("[v2 l.25] x1,3,5,7,9,11,13,15")
    emit("[v2 l.50] x0,8")
    emit("[v2 l.55] x4,12")
    emit("[v3 l.80 v2 l0] x2,10")
    emit("[v3 l.85 v2 l0] x6,14")
    emit("[=127,0] x15")
    emit()

    # Pattern 6: The Iconic Moog Bassline (16 steps, reads Stream 1 via n&1)
    # Velocity accents built directly into steps for the machine-gun pulse!
    emit("y6")
    emit("[the iconic moog bassline] yt")
    emit("[v6 n&1 l1] x0,4,8,12")
    emit("[v6 n&1 l0.9] x2,6,10,14")
    emit("[v6 n&1 l0.75] x1,3,5,7")
    emit("[v6 n&1 l0.75] x9,11,13,15")
    emit()

    # Pattern 8: Donna Summer Verse Vocal ('Ooh, it's so good...', 64 steps / 4 bars)
    # Sparsely triggered on beat onsets without intermediate rests so notes ring and float!
    emit("y8")
    emit("[donna summer verse vocal] yt")
    # Bar 1 (over C): 'Oooooh...' holds across beats 1-2, 'it's so good' on 3-4
    emit("[v8 n79 l.9] x0")     # G5 ('Oooooh...')
    emit("[v8 n75 l.85] x8")    # Eb5 ('It's so...')
    emit("[v8 n77 l.9] x12")    # F5 ('...good')
    # Bar 2 (over C): 'Oooooh...' holds across beats 1-2, 'it's so good' into root C
    emit("[v8 n79 l.9] x16")    # G5 ('Oooooh...')
    emit("[v8 n75 l.85] x24")   # Eb5 ('It's so...')
    emit("[v8 n72 l.9] x28")    # C5 ('...good')
    # Bar 3 (over Eb): 'Heaven knows...' soaring
    emit("[v8 n82 l.9] x32")    # Bb5 ('Heaven...')
    emit("[v8 n79 l.85] x40")   # G5 ('...knows...')
    emit("[v8 n75 l.9] x44")    # Eb5 ('...it's good')
    # Bar 4 (over F / G): Soaring climax phrase
    emit("[v8 n84 l.95] x48")   # C6 (High soaring falsetto peak)
    emit("[v8 n82 l.85] x54")   # Bb5
    emit("[v8 n79 l.9] x58")    # G5
    emit("[v8 n77 l.85] x62")   # F5
    emit("[=127,0] x63")
    emit()

    # Pattern 10: Donna Summer Chorus Hook ('I feel love...', 64 steps / 4 bars)
    # Sparsely triggered on melodic accents so notes sing smoothly through space delay!
    emit("y10")
    emit("[donna summer chorus hook] yt")
    # Bar 1 (over C): 'Ooh, I feel love...'
    emit("[v11 n72 l.9] x0")    # C5 ('Ooh...')
    emit("[v11 n75 l.85] x6")   # Eb5 ('...I...')
    emit("[v11 n79 l.95] x8")   # G5 ('...FEEL...')
    emit("[v11 n77 l.9] x12")   # F5 ('...love...')
    # Bar 2 (over C): 'Ooh, I feel love...'
    emit("[v11 n72 l.9] x16")   # C5
    emit("[v11 n75 l.85] x22")  # Eb5
    emit("[v11 n79 l.95] x24")  # G5
    emit("[v11 n72 l.9] x28")   # C5
    # Bar 3 (over Eb): Soaring higher
    emit("[v11 n75 l.9] x32")   # Eb5
    emit("[v11 n79 l.85] x38")  # G5
    emit("[v11 n82 l.95] x40")  # Bb5 ('...FEEL...')
    emit("[v11 n79 l.9] x44")   # G5 ('...love...')
    # Bar 4 (over F / G): Climax
    emit("[v11 n77 l.9] x48")   # F5
    emit("[v11 n84 l.95] x52")  # C6 (High C6 peak!)
    emit("[v11 n82 l.9] x56")   # Bb5
    emit("[v11 n79 l.85] x60")  # G5
    emit("[=127,0] x63")
    emit()

    # Pattern 9: High Sparkle Arpeggio (16 steps, reads Stream 8 via n&8)
    emit("y9")
    emit("[high sparkle arpeggio] yt")
    emit("[v13 n&8 l0.75] x0,2,4,6,8,10,12,14")
    emit("[v13 n&8 l0.60] x1,3,5,7,9,11,13,15")
    emit()

    # Pattern 11: Disco Strings Chords (32 steps = 2 bars, C minor colour)
    # Swelling Solina chords on bar downbeats
    emit("y11")
    emit("[disco strings chords] yt")
    emit("[v26 n48 l.85 v27 n51 l.85 v28 n55 l.85] x0")
    emit("[v26 n48 l.70 v27 n51 l.70 v28 n55 l.70] x16")
    emit("[=127,0] x31")
    emit()

    # Pattern 12: Disco Strings Modulation (Eb -> F, 32 steps)
    emit("y12")
    emit("[disco strings modulation Eb and F] yt")
    emit("[v26 n51 l.85 v27 n55 l.85 v28 n58 l.85] x0")
    emit("[v26 n53 l.85 v27 n57 l.85 v28 n60 l.85] x16")
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

    # Pattern 15: Giorgio Moroder White Noise Wind Sweep (one-shot with stop marker '-')
    emit("y15")
    emit("[moroder noise wind sweep] yt")
    emit("[v21 l1] x0")
    emit("[-] x1")
    emit()

    ## K-SYNTH PARAMETER CREATION (Bass Filter Sweep Stream)
    emit("## K-SYNTH PARAMETER CREATION (Bass Filter Sweep Stream)")
    emit("# Synthesize 32-step exponential sweep (750 Hz -> 3600 Hz) directly into /SS 12:")
    emit("[750 * ((3600%750) ^ ((!32)%31))] k! k>d /SS 12")
    emit()
    emit("y16")
    emit("[bass filter sweep open] yt")
    emit("[=0,&12 v6 k$0 v7 k$0] x0,2,4,6,8,10,12,14")
    emit("[=0,&12 v6 k$0 v7 k$0] x16,18,20,22,24,26,28,30")
    emit("[=0,&12 v6 k$0 v7 k$0] x32,34,36,38,40,42,44,46")
    emit("[=0,&12 v6 k$0 v7 k$0] x48,50,52,54,56,58,60,62")
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

    # Bars 0-7: Intro - Lone Moog bass starts dark, 16th hats fade in, sparkle arp enters
    c_add(0, "y6 z1 /SP 1,0 v6 k700 v7 k700")
    c_add(2, "y3 z1")
    c_add(4, "v6 k1100 v7 k1100 y9 z1 /SP 8,0")
    c_add(7, "y13 z1")

    # Bars 8-15: Groove drops - 4-on-the-floor kick slams in, filter sweep begins, claps & strings
    c_add(8, "v17 l1 y1 z1 y16 z1")
    c_add(12, "y2 z1 y11 z1 v6 k2200 v7 k2200")
    c_add(15, "y13 z1 y15 z1")

    # Bars 16-31: Verse 1 - Donna Summer vocal lead ('Ooh, it's so good...'), C -> Eb -> F/G
    c_add(16, "v17 l.8 y8 z1 /SC 1 2 /SP 1,0")
    c_add(20, "v6 k2400 v7 k2400")
    c_add(24, "/SC 1 3 /SP 1,0 /SC 8 9 /SP 8,0 y11 z0 y12 z1")
    c_add(28, "/SC 1 4 /SP 1,0 /SC 8 10 /SP 8,0")
    c_add(30, "/SC 1 5 /SP 1,0 /SC 8 11 /SP 8,0")
    c_add(31, "y8 z0 v8 l0 y13 z1 y15 z1")

    # Bars 32-47: Chorus 1 - 'Ooh, I feel love...', wall of sound, soaring strings & sparkle
    c_add(32, "v17 l1 /SC 1 2 /SP 1,0 /SC 8 8 /SP 8,0 y12 z0 y11 z1 y10 z1 v6 k3200 v7 k3200")
    c_add(36, "v17 l.6")
    c_add(40, "/SC 1 3 /SP 1,0 /SC 8 9 /SP 8,0 y11 z0 y12 z1")
    c_add(44, "/SC 1 4 /SP 1,0 /SC 8 10 /SP 8,0")
    c_add(46, "/SC 1 5 /SP 1,0 /SC 8 11 /SP 8,0")
    c_add(47, "y10 z0 v11 l0 y13 z1 y15 z1")

    # Bars 48-55: The Legendary Delay Freeze Breakdown!
    # Drums drop out, delay freezes (DF 1 1), cosmic floating drone
    c_add(48, "v17 l1 DF 1 1 y1 z0 y2 z0 y3 z0 y6 z0 y9 z0 y12 z0 v6 l0 v7 l0 v13 l0")
    c_add(49, "v8 l0 v11 l0")
    c_add(50, "y11 z1")
    c_add(52, "v16 l1")
    c_add(55, "y13 z1 y15 z1")

    # Bars 56-63: Re-build - Kick returns, white noise wind rises, filter opens, tom roll build
    c_add(56, "DF 1 0 v17 l1 y1 z1 /SC 1 2 /SP 1,0 y6 z1 v6 k800 v7 k800 y16 z1")
    c_add(58, "y3 z1")
    c_add(60, "y9 z1 /SC 8 8 /SP 8,0")
    c_add(63, "y13 z1 y15 z1")

    # Bars 64-79: Chorus 2 & Climax - Maximum energy, wall of sound!
    c_add(64, "v17 l1 y2 z1 y10 z1 y11 z1 v6 k3600 v7 k3600")
    c_add(68, "v17 l.7")
    c_add(72, "/SC 1 3 /SP 1,0 /SC 8 9 /SP 8,0 y11 z0 y12 z1")
    c_add(76, "/SC 1 4 /SP 1,0 /SC 8 10 /SP 8,0")
    c_add(78, "/SC 1 5 /SP 1,0 /SC 8 11 /SP 8,0")
    c_add(79, "y13 z1 y15 z1")

    # Bars 80-87: Extended Climax Vamp - Soaring vocal lead verse melody over chorus groove
    c_add(80, "v17 l1 /SC 1 2 /SP 1,0 /SC 8 8 /SP 8,0 y12 z0 y11 z1 y10 z0 v11 l0 y8 z1")
    c_add(84, "/SC 1 3 /SP 1,0 /SC 8 9 /SP 8,0 y11 z0 y12 z1")
    c_add(87, "y8 z0 v8 l0 y13 z1")

    # Bars 88-95: Outro Strip-Back
    c_add(88, "v17 l.8 /SC 1 2 /SP 1,0 /SC 8 8 /SP 8,0 y12 z0 y11 z0 y2 z0 y3 z%2 v6 k1400 v7 k1400")
    c_add(90, "v6 k900 v7 k900 y9 z0 v13 l0")
    c_add(92, "DF 1 1 y1 z0")
    c_add(94, "y6 z0 y3 z0 v6 l0 v7 l0")

    # Bar 96: End Cleanup
    c_add(96, "Z0 +8 ce 99")

    # Output conductor steps (sparse)
    for bar in sorted(conductor.keys()):
        cmds = conductor[bar]
        emit(f"[{' '.join(cmds)}] x{bar}")
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
    target_path = os.path.join(os.path.dirname(__file__), "i_feel_love.sk")
    content = build_i_feel_love()
    with open(target_path, "w") as f:
        f.write(content)
    print(f"Wrote {len(content.splitlines())} lines to {target_path}")

if __name__ == "__main__":
    main()
