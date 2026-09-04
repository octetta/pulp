with open("doc/phase_distortion.md", "r") as f:
    text = f.read()

parts = text.split("---")
# parts[0] is Intro + The new examples
# parts[1] is The Casio CZ Legacy
# parts[2] is Beyond Casio

# I will extract the new examples from parts[0] and append them to parts[1]
intro = parts[0].split("### Funky Pulse Bass")[0]
examples = "### Funky Pulse Bass" + parts[0].split("### Funky Pulse Bass")[1]

parts[0] = intro
parts[1] = parts[1] + "\n" + examples

with open("doc/phase_distortion.md", "w") as f:
    f.write("---".join(parts))
