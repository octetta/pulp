with open("parts/synth-wave.c", "r") as f:
    lines = f.readlines()

for i in range(len(lines)):
    if "case WAVE_TABLE_SINE:  name = \"sine\"; break;" in lines[i]:
        lines.insert(i+1, "      case WAVE_TABLE_COSINE: name = \"cosine\"; break;\n")
        break

for i in range(len(lines)):
    if "case WAVE_TABLE_SINE: f = sine; break;" in lines[i]:
        lines.insert(i+1, "        case WAVE_TABLE_COSINE: f = -cosf(2.0f * (float) M_PI * phase); break;\n")
        break

with open("parts/synth-wave.c", "w") as f:
    f.writelines(lines)
