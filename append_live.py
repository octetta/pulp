with open("doc/phase_distortion.md", "a") as f:
    f.write("""
### Putting It Together: Live Neuro Sequencer
If you want to wow people with a live performance, you can hook your Dubstep growl into Skred's built-in step sequencer!
By combining a note stream with a repeating sequence pattern, you can have the bassline play itself while you dynamically tweak the Phase Distortion depth or wobble speed live.

```skred
# 1. Setup the Dubstep Growl (V1 is LFO, V0 is Carrier)
v1 w0 m1 f6 a1 t0,10,1,1 l1
v0 w15 c1,0.2 C1,0.8 a8 g0.1 t0.1,2,0.5,1 

# 2. Feed a bassline into Stream 1 (/SS 1)
(35 38 40 37) /SS 1

# 3. Bind a pattern to the sequencer (x0).
# On trigger, it grabs the next note from Stream 1 (n&1) and plays it (l1)!
[v0 n&1 l1] x0
```
With the sequence running, you will hear a tearing, gliding Neurohop bassline automatically looping and modulating itself!
""")
