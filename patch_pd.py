with open("doc/phase_distortion.md", "r") as f:
    text = f.read()

text += """
### Architectural Secrets: Mutes and Glides

**Why do we use `m1` (Mute) on modulators?**
In Skred's architecture, if you want an LFO to be completely silent but still modulate another voice, you might be tempted to just set its amplitude to zero (`a0`). However, because modulation is computed *after* the envelope stage, `a0` will literally squash the LFO signal to zero, stopping it from doing any modulation! 
By using `m1` (Mute), the voice still generates a full-strength enveloped signal (`a1`) that other voices can read, but it simply disconnects its output from the master mix bus. Always use `a1 m1` for silent LFOs!

**Glissando / Portamento (`g`)**
Neuro and Dubstep basses frequently glide between pitches. You can achieve this seamlessly in Skred using the `g time` command.
For example, adding `g 0.1` to your bass voice will make it glide smoothly between frequencies over 100 milliseconds whenever a new pitch is triggered, making those Phase Distortion sweeps sound incredibly fluid and rubbery.
"""

with open("doc/phase_distortion.md", "w") as f:
    f.write(text)
