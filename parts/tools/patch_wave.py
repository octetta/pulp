import re

with open("../synth-wave.c", "r") as f:
    code = f.read()

# Replace loop limit
code = code.replace("w <= WAVE_TABLE_KRG2_CENTERED", "w <= WAVE_TABLE_DRUM_18")

# Generate names
esq_names = ""
for i in range(1, 33):
    esq_names += f'      case WAVE_TABLE_ESQ1_{i:02d}: name = "esq1-{i:02d}"; break;\n'
drum_names = ""
for i in range(1, 19):
    drum_names += f'      case WAVE_TABLE_DRUM_{i:02d}: name = "drum-{i:02d}"; break;\n'

code = code.replace('      case WAVE_TABLE_KRG16: name = "dwg-whistle"; break;\n', 
                    '      case WAVE_TABLE_KRG16: name = "dwg-whistle"; break;\n' + esq_names + drum_names)


# Generate floats
esq_floats = ""
for i in range(1, 33):
    esq_floats += f'        case WAVE_TABLE_ESQ1_{i:02d}: f = WAVE_ESQ1_{i:02d}[off]; break;\n'
drum_floats = ""
for i in range(1, 19):
    drum_floats += f'        case WAVE_TABLE_DRUM_{i:02d}: f = WAVE_DRUM_{i:02d}[off]; break;\n'

code = code.replace('        case WAVE_TABLE_KRG16: f = W16[off]; break;\n',
                    '        case WAVE_TABLE_KRG16: f = W16[off]; break;\n' + esq_floats + drum_floats)

with open("../synth-wave.c", "w") as f:
    f.write(code)
print("patched")
