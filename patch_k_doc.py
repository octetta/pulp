with open("parts/skode.c", "r") as f:
    text = f.read()

target = """    /* @doc(command.k)
    name: k
    category: misc
    summary: envelope mode
    @enddoc */"""

replacement = """    /* @doc(command.k)
    name: k
    category: misc
    summary: envelope mode (0=Standard, 1=One-Shot Auto-Release, 2=Inverted/Ducking)
    @enddoc */"""

text = text.replace(target, replacement)

with open("parts/skode.c", "w") as f:
    f.write(text)
