#!/usr/bin/env python3
'''
Korg DW-8000 to Skred Translation Guide

THE KORG DW-8000 ARCHITECTURE:
The DW-8000 is a hybrid synth that plays digital waveforms (DWGS) through analog filters and VCAs,
topped off with a legendary digital delay line. It features two oscillators per voice.

MAPPING TO SKRED:
  - Digital Waveforms -> Skred `w15` through `w30` (Korg DWGS wavetables).
  - VCF (Analog Filter) -> Skred `J24` (24dB Analog Lowpass Filter) modulated by `fte` (Filter Envelope).
  - VCA (Amplifier) -> Skred `te` (Amplitude Envelope).
  - 2-Oscillator Mix -> Two Skred voices (e.g. `v0` and `v1`) linked via `G 1` (note) and `H 1` (trigger), with `v1` detuned (`N 0, 7`).
  - Digital Delay/Chorus -> Skred `DL` (Delay Line) command on a recording track (`r1`).

ADBSSR ENVELOPES (0-31):
The DW-8000 uses a unique 6-parameter ADBSSR envelope: Attack, Decay, Break Point, Slope, Sustain, Release.
All parameters are integers from 0 to 31.
  - RATES (0-31): 0 is the fastest (~1ms), 31 is the slowest (~20+ seconds).
                  We approximate this exponential curve via `0.005 * (val ** 1.8)`.
  - LEVELS (0-31): Break Point and Sustain are levels. Mapped linearly (val / 31.0) to Skred's 0.0-1.0 scale.

SKRED TRANSLATION:
Skred's flat array format easily mimics an ADBSSR curve in 5 stages:
  1. Attack Stage: Time = Attack Rate, Target Level = 1.0
  2. Decay Stage: Time = Decay Rate, Target Level = Break Point Level
  3. Slope Stage: Time = Slope Rate, Target Level = Sustain Level
  4. Sustain Stage: Time = -1 (Skred Sustain Flag), Target Level = Sustain Level
  5. Release Stage: Time = Release Rate, Target Level = 0.0
'''

def dw8000_rate_to_time(val):
    """
    DW-8000 parameter values are 0-31.
    0 is fastest (approx 1ms), 31 is very slow (approx 20+ seconds).
    This is an approximation of the exponential curve.
    """
    if val == 0: return 0.001
    return 0.005 * (val ** 1.8)

def dw8000_level_to_amp(val):
    """
    DW-8000 level parameters are 0-31.
    """
    return val / 31.0

def generate_adbssr_array(command, a_r, d_r, b_l, s_r, s_l, r_r):
    # Stage 1: Attack (to 1.0)
    t1 = dw8000_rate_to_time(a_r)
    l1 = 1.0
    
    # Stage 2: Decay (to Break Point)
    t2 = dw8000_rate_to_time(d_r)
    l2 = dw8000_level_to_amp(b_l)
    
    # Stage 3: Slope (to Sustain)
    t3 = dw8000_rate_to_time(s_r)
    l3 = dw8000_level_to_amp(s_l)
    
    # Stage 4: Sustain Hold (-1 flag in Skred)
    t4 = -1
    l4 = l3
    
    # Stage 5: Release (to 0.0)
    t5 = dw8000_rate_to_time(r_r)
    l5 = 0.0
    
    return f"( {t1:.3f} {l1:.2f}  {t2:.3f} {l2:.2f}  {t3:.3f} {l3:.2f}  {t4} {l4:.2f}  {t5:.3f} {l5:.2f} ) {command}"

def generate_dw8000_presets():
    print("# Korg DW-8000 Factory Patches automatically generated for Skred")
    
    # A dictionary of some classic DW-8000 style patches
    # VCF/VCA envelopes use the ADBSSR format: (Attack, Decay, Break Point, Slope, Sustain, Release)
    presets = {
        "Classic Analog Brass": {
            "wave": 2,          # Sawtooth-like DWGS waveform
            "detune": 7,        # OSC2 detune in cents
            "filter_k": 3000,   # Base filter cutoff
            "filter_q": 0.3,    # Filter resonance
            "filter_env_depth": 0.8,
            "vcf_env": [12, 15, 20, 18, 10, 16], # Filter ADBSSR
            "vca_env": [8, 12, 25, 10, 20, 14],  # Amp ADBSSR
            "delay": "1, 0, 15, 0, 6, 18, 10"    # Short lush digital chorus
        },
        "Slap Bass": {
            "wave": 3,
            "detune": 4,
            "filter_k": 800,
            "filter_q": 0.6,
            "filter_env_depth": 0.9,
            "vcf_env": [2, 8, 10, 4, 0, 8],
            "vca_env": [2, 10, 15, 6, 0, 8],
            "delay": "1, 0, 10, 0, 0, 0, 8"      # Slapback echo
        },
        "Lush Strings": {
            "wave": 6,          # Rich ensemble wave
            "detune": 12,       # Heavy detune for thick chorusing
            "filter_k": 4000,
            "filter_q": 0.2,
            "filter_env_depth": 0.4,
            "vcf_env": [25, 15, 25, 20, 25, 24], # Slow attack, slow release
            "vca_env": [22, 15, 31, 25, 31, 26],
            "delay": "1, 0, 20, 15, 4, 25, 15"   # Long delay with heavy modulation
        },
        "Digital Bell": {
            "wave": 8,          # Glassy digital wave
            "detune": 3,
            "filter_k": 6000,
            "filter_q": 0.1,
            "filter_env_depth": 0.5,
            "vcf_env": [0, 20, 0, 31, 0, 25],
            "vca_env": [0, 18, 0, 31, 0, 22],
            "delay": "1, 0, 25, 20, 5, 20, 12"   # Spacious echoing delay
        }
    }
    
    for name, p in presets.items():
        print(f"\n# === {name} ===")
        print(f"# Setup the DW-style Delay on Track 1")
        print(f"DL {p['delay']}\n")
        
        print("# --- OSC 2 (Detuned) ---")
        print(f"v1 w{p['wave']} a 8")
        print(f"N 0, {p['detune']}            # Detune OSC 2 by +{p['detune']} cents for analog thickness")
        print("r1 ds1.0          # Route voice to Track 1 and send full signal to Delay")
        print(f"J24 0 K{p['filter_k']} Q{p['filter_q']}  # Analog Lowpass Filter")
        print(generate_adbssr_array("fte", *p["vcf_env"]))
        print(f"fd {p['filter_env_depth']}")
        print(generate_adbssr_array("te", *p["vca_env"]))

        print("\n# --- OSC 1 (Master) ---")
        print(f"v0 w{p['wave']} a 8")
        print("G 1 H 1           # Link Note and Trigger to OSC 2 (v1)")
        print("r1 ds1.0          # Route voice to Track 1 and send full signal to Delay")
        print(f"J24 0 K{p['filter_k']} Q{p['filter_q']}  # Analog Lowpass Filter")
        print(generate_adbssr_array("fte", *p["vcf_env"]))
        print(f"fd {p['filter_env_depth']}")
        print(generate_adbssr_array("te", *p["vca_env"]))

if __name__ == "__main__":
    generate_dw8000_presets()