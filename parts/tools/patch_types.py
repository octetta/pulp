with open("../synth-types.h", "r") as f:
    code = f.read()

struct_def = """
typedef struct {
    int slot;
    const char *name;
    int size;
    float sample_rate;
    int loop_start;
    int loop_end;
    int one_shot;
    const float *data;
} static_wave_meta_t;

"""

if "static_wave_meta_t" not in code:
    code = code.replace("typedef enum {", struct_def + "typedef enum {", 1)
    with open("../synth-types.h", "w") as f:
        f.write(code)
print("patched")
