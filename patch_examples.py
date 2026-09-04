with open("doc/phase_distortion.md", "r") as f:
    text = f.read()

new_examples = """
### Funky Pulse Bass
The CZ is legendary for its punchy, synthetic basses. This patch uses Mode 1 (Sawtooth to Pulse) with a negative distortion depth to drive a strong transient pulse that decays into a dark, funky body.
```skred
v0 w47 a8 c1,-0.65 ct0.001,0.16,0.08,0.22 cd1.0 t0.002,0.24,0.62,0.28 f55 l1
```

### Glassy Bell
Bells and chimes are a staple of Phase Distortion. This uses Mode 4 (Double Sine). A very short PD envelope creates a sharp metallic transient, while the amplitude envelope rings out smoothly over almost two seconds.
```skred
v0 w47 a8 c4,0.08 ct0.001,0.55,0,0.7 cd0.8 t0.001,1.8,0,1 f440 l1
```

### Syn-Tom (Percussion)
You can create synthetic percussion (like classic 80s toms and kicks) by pairing a very fast, steep amplitude decay with an equally fast PD decay. As the note rings out, the harmonics collapse rapidly, giving it a percussive "thwack."
```skred
v0 w47 a8 c1,0.8 ct0.001,0.08,0,0 cd1.0 t0.001,0.3,0,0 f110 l1
```

"""

# Insert before "Beyond Casio"
text = text.replace("---", new_examples + "\n---", 1) # only the first ---

with open("doc/phase_distortion.md", "w") as f:
    f.write(text)
