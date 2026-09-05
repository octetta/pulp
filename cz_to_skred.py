#!/usr/bin/env python3
'''
Casio CZ Series to Skred Translation Guide

THE CASIO CZ ARCHITECTURE:
The Casio CZ series uses Phase Distortion (PD) synthesis. Instead of filters, it modifies the phase
of a sine wave reader to create harmonics. It has three main control blocks per line:
  1. DCO (Digitally Controlled Oscillator) - Controls Pitch over time.
  2. DCW (Digitally Controlled Waveform) - Controls Harmonic Brightness (Phase Distortion) over time.
  3. DCA (Digitally Controlled Amplifier) - Controls Volume over time.

MAPPING TO SKRED:
Skred natively supports Phase Distortion and arbitrary multi-stage envelopes, making it a perfect match:
  - DCO Envelope -> Skred `pte` (Pitch Time Extended) envelope.
  - DCW Envelope -> Skred `cte` (Phase Distortion Time Extended) envelope.
  - DCA Envelope -> Skred `te` (Amplitude Time Extended) envelope.
  - PD Waveform  -> Skred `w47` (Casio Cosine wave) combined with `c <mode>, <offset>` phase distortion.

CASIO ENVELOPE PARAMETERS (0-99):
Casio envelopes have up to 8 stages. Each stage has a Rate (0-99) and a Level (0-99).
You can place a Sustain point (where the envelope holds while the key is pressed) and an End point.
  - RATE (0-99): 99 is extremely fast (~1ms). 0 is extremely slow (~20 seconds).
                 The curve is non-linear. We approximate it using `0.005 * ((100 - rate) ** 1.3)`.
  - LEVEL (0-99): Maps linearly to Skred's 0.0 to 1.0 amplitude scale.

SKRED TRANSLATION:
In Skred, an envelope is a flat array of `( rate1 level1  rate2 level2 ... ) command`.
To translate a Casio envelope to Skred:
  1. Convert each Rate to seconds.
  2. Convert each Level to a 0.0-1.0 float.
  3. At the Casio Sustain stage, insert Skred's `-1` sustain flag instead of the time value.
  4. Truncate the array after the release stage.
'''
import sys

def rate_to_time(rate):
    """
    Approximation of the Casio CZ rate (0-99) to time in seconds.
    The true CZ scale is non-linear and context-dependent.
    This provides a musically sensible mapping for Skred.
    """
    if rate == 99: return 0.001
    if rate == 0: return 20.0
    # A simple exponential curve
    return 0.005 * ((100 - rate) ** 1.3)

def level_to_amp(level):
    """
    CZ levels are 0-99.
    """
    return level / 99.0

def generate_skred_env(command, rates, levels, sustain_stage=None):
    """
    Generates a Skred multistage envelope array command.
    """
    out = ["("]
    for i in range(len(rates)):
        time_sec = rate_to_time(rates[i])
        amp = level_to_amp(levels[i])
        
        if sustain_stage is not None and i == sustain_stage:
            time_sec = -time_sec  # Skred sustain flag
            
        out.append(f"{time_sec:.3f} {amp:.2f}")
    
    out.append(f") {command}")
    return " ".join(out)

def generate_factory_presets():
    # A dictionary holding some of the most famous CZ-101 factory presets
    presets = {
        "Elec Piano 1": {
            "dcw_rates": [99, 50, 40, 20, 20, 20, 20, 50],
            "dcw_levels": [99, 60, 40, 0, 0, 0, 0, 0],
            "dcw_sus": 3,
            "dca_rates": [99, 45, 30, 20, 20, 20, 20, 50],
            "dca_levels": [99, 70, 0, 0, 0, 0, 0, 0],
            "dca_sus": 2,
            "pd_mode": 1
        },
        "Fretless Bass": {
            "dcw_rates": [99, 50, 40, 0, 0, 0, 0, 60],
            "dcw_levels": [99, 50, 0, 0, 0, 0, 0, 0],
            "dcw_sus": 2,
            "dca_rates": [99, 45, 40, 0, 0, 0, 0, 60],
            "dca_levels": [99, 60, 0, 0, 0, 0, 0, 0],
            "dca_sus": 2,
            "pd_mode": 1
        },
        "Fairlight Vox": {
            "dcw_rates": [60, 50, 0, 0, 0, 0, 0, 50],
            "dcw_levels": [99, 80, 0, 0, 0, 0, 0, 0],
            "dcw_sus": 1,
            "dca_rates": [50, 50, 0, 0, 0, 0, 0, 30],
            "dca_levels": [99, 80, 0, 0, 0, 0, 0, 0],
            "dca_sus": 1,
            "pd_mode": 4
        },
        "Bells": {
            "dcw_rates": [99, 70, 0, 0, 0, 0, 0, 70],
            "dcw_levels": [99, 0, 0, 0, 0, 0, 0, 0],
            "dcw_sus": 1,
            "dca_rates": [99, 20, 0, 0, 0, 0, 0, 20],
            "dca_levels": [99, 0, 0, 0, 0, 0, 0, 0],
            "dca_sus": 1,
            "pd_mode": 4
        }
    }
    
    print("# CZ-101 Greatest Hits automatically generated for Skred")
    for name, p in presets.items():
        print(f"\n# === {name} ===")
        print(f"v0 w47 c{p['pd_mode']},0 cd1.0 a8")
        
        # We only output the stages up to the release to keep the arrays clean
        stages = p['dcw_sus'] + 2
        print(generate_skred_env("cte", p["dcw_rates"][:stages], p["dcw_levels"][:stages], p["dcw_sus"]))
        
        stages = p['dca_sus'] + 2
        print(generate_skred_env("te", p["dca_rates"][:stages], p["dca_levels"][:stages], p["dca_sus"]))

if __name__ == "__main__":
    generate_factory_presets()
