import re

with open("../synth-wave.c", "r") as f:
    code = f.read()

for i in range(1, 19):
    old_str = f'      case WAVE_TABLE_DRUM_{i:02d}: name = "drum-{i:02d}"; break;'
    new_str = f'      case WAVE_TABLE_DRUM_{i:02d}: name = "drum-{i:02d}"; size = WAVE_DRUM_{i:02d}_SIZE; break;'
    code = code.replace(old_str, new_str)

with open("../synth-wave.c", "w") as f:
    f.write(code)
print("patched")
