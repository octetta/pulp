with open("doc/phase_distortion.md", "r") as f:
    text = f.read()

text = text.replace("v1 w0 f6 a1 t0,10,1,1 l1", "v1 w0 m1 f6 a1 t0,10,1,1 l1")
text = text.replace("v3 w0 f4 a1 t0,10,1,1 l1", "v3 w0 m1 f4 a1 t0,10,1,1 l1")
text = text.replace("v0 w15 c1,0.2 C1,0.8 a8 f55 t0.1,2,0.5,1 l1", "v0 w15 c1,0.2 C1,0.8 a8 f55 g0.1 t0.1,2,0.5,1 l1")
text = text.replace("v2 w21 c2,0.5 C3,0.5 a8 f55 t0.05,1,0.5,1 l1", "v2 w21 c2,0.5 C3,0.5 a8 f55 g0.15 t0.05,1,0.5,1 l1")

with open("doc/phase_distortion.md", "w") as f:
    f.write(text)
