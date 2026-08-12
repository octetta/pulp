import re
import sys

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
    
    with open("synth.h.kit", "r") as f:
        content = f.read()
        
    for func in funcs:
        # Match function definition: starts with return type, func_name(
        # e.g. static float osc_next_at(int voice, float phase_inc, ...
        # Can be multiline if arguments wrap.
        pattern = r'((?:static\s+)?(?:inline\s+)?(?:float|void|int)\s+' + func + r'\s*\()(?!\s*skred_engine_t\s*\*engine)'
        
        # Replace the opening parenthesis with (skred_engine_t *engine, 
        def replacer(m):
            if m.group(1).endswith('()'):
                return m.group(1)[:-1] + 'skred_engine_t *engine)'
            else:
                return m.group(1) + 'skred_engine_t *engine, '
                
        content = re.sub(pattern, replacer, content)
        
    with open("synth.h.kit", "w") as f:
        f.write(content)

if __name__ == "__main__":
    main()
