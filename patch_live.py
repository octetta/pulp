with open("doc/phase_distortion.md", "r") as f:
    text = f.read()

append = """
#### Live Tweaking Examples
While the sequencer is driving the bassline, you can type commands to mutate the sound in real-time. Try pasting these commands one by one while it plays:

* **Change the wobble speed:** Make the LFO rapidly flutter or slowly sweep.
  `v1 f12` (Fast)
  `v1 f2` (Slow)
* **Push the distortion over the edge:** Increase the depth to make the synth scream.
  `v0 C1, 1.5`
* **Change the filter texture:** Switch the PD Mode from 1 (Pulse) to 2 (Folded Square) to instantly make the bass sound metallic and hollow.
  `v0 c2, 0.2`
* **Massive Glide:** Increase the glissando time so the notes slide sluggishly into each other.
  `v0 g0.5`
"""

text += append

with open("doc/phase_distortion.md", "w") as f:
    f.write(text)
