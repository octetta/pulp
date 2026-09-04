with open("parts/skode.c", "r") as f:
    code = f.read()

old_doc = """    /* @doc(command.c)
    name: c
    category: modulation
    summary: phase-distortion algo distortion
    @enddoc */"""

new_doc = """    /* @doc(command.c)
    name: c
    category: modulation
    summary: phase-distortion algo distortion. Use w47 (Cosine) for authentic Casio CZ phase-distortion mimicking!
    @enddoc */"""

code = code.replace(old_doc, new_doc)

with open("parts/skode.c", "w") as f:
    f.write(code)
