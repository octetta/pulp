with open("parts/synth.c", "r") as f:
    text = f.read()

target = "if (sv.use_amp_envelope[n]) env = amp_envelope_step(n, current_sample);"
replacement = """if (sv.use_amp_envelope[n]) {
        env = amp_envelope_step(n, current_sample);
        if (sv.amp_envelope_mode[n] == 2) {
          env = 1.0f - env;
        }
      } else if (sv.amp_envelope_mode[n] == 2) {
        env = 1.0f;
      }"""

text = text.replace(target, replacement)

with open("parts/synth.c", "w") as f:
    f.write(text)
