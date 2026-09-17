with open("../synth-wave.c", "r") as f:
    lines = f.readlines()

new_lines = []
skip = False
for line in lines:
    if "case WAVE_TABLE_ESQ1_" in line or "case WAVE_TABLE_DRUM_" in line:
        continue
    
    if "for (int w = WAVE_TABLE_SINE; w <= WAVE_TABLE_DRUM_18; w++) {" in line:
        new_lines.append("  for (int w = WAVE_TABLE_SINE; w <= WAVE_TABLE_KRG2_CENTERED; w++) {\n")
        continue

    new_lines.append(line)

# Now add the meta-list loop at the end of wave_table_init()
# Find the end of wave_table_init
out_str = "".join(new_lines)
end_idx = out_str.find("void wave_free_one(int i)")
if end_idx != -1:
    meta_loop = """
  const static_wave_meta_t **lists[] = { ESQ1_WAVES, DRUM_WAVES, NULL };
  for (int lst = 0; lists[lst] != NULL; lst++) {
    const static_wave_meta_t **list = lists[lst];
    for (int i = 0; list[i] != NULL; i++) {
      const static_wave_meta_t *m = list[i];
      int w = m->slot;
      if (wave_invalid(w)) continue;
      strncpy(sw.name[w], m->name, WAVE_NAME_MAX);
      sw.data[w] = (float *)malloc(m->size * sizeof(float));
      if (!sw.data[w]) continue;
      memcpy(sw.data[w], m->data, m->size * sizeof(float));
      sw.is_heap[w] = 1;
      sw.size[w] = m->size;
      sw.rate[w] = m->sample_rate;
      sw.one_shot[w] = m->one_shot;
      sw.loop_start[w] = m->loop_start;
      sw.loop_end[w] = m->loop_end;
      sw.readonly[w] = 1;
    }
  }
}
"""
    # Replace the last `}` of wave_table_init with the meta loop
    # We find the `}` right before `void wave_free_one(int i)`
    idx = out_str.rfind("}", 0, end_idx)
    out_str = out_str[:idx] + meta_loop + out_str[idx+1:]

with open("../synth-wave.c", "w") as f:
    f.write(out_str)
print("patched")
