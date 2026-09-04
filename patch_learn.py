with open("doc/learn.html", "r") as f:
    text = f.read()

addition = """<template data-type="markdown">
**`w47`** is a Cosine wave. It sounds identical to a Sine wave, but it is technically shifted. It is the best choice to use as the base waveform when applying Phase Distortion (`c` command) to mimic a classic Casio CZ synthesizer, as the math creates a kink at the peak of the wave rather than at the zero-crossing.
</template>
<template data-type="code"> v0 w47 a0 c1,0.5 f440 t0 2 0 0 l1</template>

"""

if "**`w47`**" not in text:
    text = text.replace("<template data-type=\"markdown\">\n**`w4`**", addition + "<template data-type=\"markdown\">\n**`w4`**")
    
    with open("doc/learn.html", "w") as f:
        f.write(text)
