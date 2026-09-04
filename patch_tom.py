with open("doc/phase_distortion.md", "r") as f:
    text = f.read()

bad_tom = """# Voice 1: Fast Pitch Envelope (muted DC square wave)
v1 w1 m1 f0 a1 t0.001,0.15,0,0 l1

# Voice 0: Syn-Tom (FF 1 triggers exponential FM from Voice 1)
v0 w47 FF1 F1,200,110 c1,0.8 ct0.001,0.08,0,0 cd1.0 a8 t0.001,0.3,0,0 l1"""

good_tom = """The Casio CZ famously lacked a pitch envelope! To create percussion like Toms or Kicks, programmers used a "fake" pitch envelope by applying a massive, lightning-fast Phase Distortion decay over a very low fundamental frequency. 
As the wave rapidly collapses from a bright Pulse (Mode 1) down to a pure Sine wave, the human ear perceives the loss of high-harmonics as a pitch-drop "thwack"!
```skred
v0 w47 a8 c1,0.9 ct0.001,0.05,0,0 cd1.0 t0.001,0.4,0,0 f65 l1
```"""

text = text.replace(bad_tom, good_tom)

with open("doc/phase_distortion.md", "w") as f:
    f.write(text)
