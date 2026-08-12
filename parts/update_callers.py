import re

def main():
    funcs = [
        "osc_get_phase_inc", "osc_set_freq", "osc_reclassify", "osc_sample_at_phase", 
        "osc_next_at", "osc_next", "osc_set_wave_table_index", "osc_trigger",
        "mmf_process", "mmf_set_params", "mmf_init", "mmf_set_freq", "mmf_set_res",
        "delay_voice_can_send", "delay_send_set", "delay_params_set", "delay_params_get",
        "delay_damping_set", "delay_freeze_set", "delay_pingpong_set", "delay_damping_get",
        "delay_freeze_get", "delay_pingpong_get", "delay_grit_set", "delay_grit_get",
        "delay_time_ms_set", "delay_time_sync_set", "delay_clear", "delay_process",
        "delay_filter_feedback", "delay_cache_params"
    ]
    
    with open("synth.c.kit", "r") as f:
        content = f.read()

    # Split into functions / blocks heuristically
    blocks = re.split(r'^(?=[a-zA-Z_])', content, flags=re.MULTILINE)
    
    out_blocks = []
    for block in blocks:
        has_engine = 'skred_engine_t *engine' in block.split('{')[0]
        arg_str = 'engine' if has_engine else '&skred_global_engine'
        
        for func in funcs:
            # We want to replace `func(` with `func(arg_str, `
            # But only if it's a function call, not the definition itself!
            # The definition has `skred_engine_t *engine` already.
            # So we can match `func(` not preceded by `void ` or `float ` or `int `
            
            pattern = r'(?<!void\s)(?<!float\s)(?<!int\s)(?<!static\s)(?<!inline\s)\b' + func + r'\s*\('
            # Also avoid matching if we already inserted it
            # e.g., delay_cache_params(engine, bus)
            
            def replacer(m):
                return m.group(0) + arg_str + ', '
                
            # We need to make sure we don't do it if the first argument is already `engine` or `&skred_global_engine`
            # This requires a slightly more complex regex or simply checking after replacement
            pass
            
            # Better regex:
            pattern2 = r'(?<!void\s)(?<!float\s)(?<!int\s)(?<!static\s)(?<!inline\s)\b(' + func + r')\s*\(\s*(?!engine|&skred_global_engine)'
            block = re.sub(pattern2, r'\1(' + arg_str + r', ', block)
            
            # Special case for delay_clear() which has no other arguments
            # If it was delay_clear(), it becomes delay_clear(engine, ) -> delay_clear(engine)
            block = block.replace(arg_str + ', )', arg_str + ')')
            
        out_blocks.append(block)
        
    with open("synth.c.kit", "w") as f:
        f.write("".join(out_blocks))
        
    # Also do skode.c.kit and polyphony.c.kit
    for filename in ["skode.c.kit", "polyphony.c.kit"]:
        try:
            with open(filename, "r") as f:
                content = f.read()
            for func in funcs:
                pattern = r'\b(' + func + r')\s*\(\s*(?!engine|&skred_global_engine)'
                content = re.sub(pattern, r'\1(&skred_global_engine, ', content)
                content = content.replace('&skred_global_engine, )', '&skred_global_engine)')
            with open(filename, "w") as f:
                f.write(content)
        except FileNotFoundError:
            pass
            

if __name__ == "__main__":
    main()
    import glob
    for filename in glob.glob("tests/*.c"):
        try:
            with open(filename, "r") as f:
                content = f.read()
            for func in funcs:
                pattern = r'\b(' + func + r')\s*\(\s*(?!engine|&skred_global_engine)'
                content = re.sub(pattern, r'\1(&skred_global_engine, ', content)
                content = content.replace('&skred_global_engine, )', '&skred_global_engine)')
            with open(filename, "w") as f:
                f.write(content)
        except Exception:
            pass
