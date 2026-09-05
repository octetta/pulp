#!/usr/bin/env python3
'''
Yamaha DX-7 to Skred Translation Guide

THE DX-7 ARCHITECTURE:
The Yamaha DX-7 is the grandfather of 6-Operator Phase Modulation synthesis.
It features 6 sine wave operators arranged in 32 predefined "Algorithms".
Each operator has its own Frequency Ratio (Coarse/Fine) and a 4-Rate/4-Level Envelope.

MAPPING TO SKRED:
- DX-7 Operator -> Skred voice playing a sine wave (`w 0`).
- Carrier Operators -> Standard Skred voices.
- Modulator Operators -> Muted voices (`m 1`) that use `F <voice> <depth>` Phase Modulation.
- FM Mode -> `FF 2` explicitly sets Skred to Phase Modulation mode (true DX-7 math).
- Routing -> Operators are linked using `G` (MIDI Note) and `H` (Velocity Trigger).

DX-7 ENVELOPES (0-99):
Each DX-7 Envelope has 4 Rates (R1, R2, R3, R4) and 4 Levels (L1, L2, L3, L4).
- R1/L1: Initial Attack to peak level.
- R2/L2: First Decay.
- R3/L3: Second Decay. **L3 is the Sustain Level.**
- R4/L4: Release.
In Skred, this translates cleanly to a 4-stage flat array, using `-1` at L3 for sustain.
'''

def dx7_rate_to_time(rate):
    """
    Approximation of DX-7 Rates (0-99) to seconds.
    The true DX-7 rate is highly complex and depends on key scaling,
    but this provides a musically functional exponential curve.
    """
    if rate == 99: return 0.001
    if rate == 0: return 20.0
    return 0.005 * ((100 - rate) ** 1.35)

def dx7_level_to_amp(level):
    """
    DX-7 Output Levels (0-99) mapped to Skred 0.0-1.0.
    """
    return level / 99.0

def generate_dx7_env(r1, r2, r3, r4, l1, l2, l3, l4):
    """Generates a Skred `te` envelope array from DX-7 EG parameters."""
    t1 = dx7_rate_to_time(r1)
    a1 = dx7_level_to_amp(l1)
    
    t2 = dx7_rate_to_time(r2)
    a2 = dx7_level_to_amp(l2)
    
    t3 = dx7_rate_to_time(r3)
    a3 = dx7_level_to_amp(l3)
    
    t_sus = -1
    a_sus = a3
    
    t4 = dx7_rate_to_time(r4)
    a4 = dx7_level_to_amp(l4)
    
    return f"( {t1:.3f} {a1:.2f}  {t2:.3f} {a2:.2f}  {t3:.3f} {a3:.2f}  {t_sus} {a_sus:.2f}  {t4:.3f} {a4:.2f} ) te"

def generate_dx7_presets():
    print("# Yamaha DX-7 Patches automatically generated for Skred")
    
    # We define simplified 2-Operator patches representing Algorithm 16
    presets = {
        "Solid Bass": {
            # Operator 2 (Modulator)
            "op2_ratio": 2.0,  # Coarse 2
            "op2_out": 85,     # Output Level
            "op2_env": [80, 50, 30, 40,  # Rates 1-4
                        99, 70, 0,  0],  # Levels 1-4
            
            # Operator 1 (Carrier)
            "op1_ratio": 1.0,  # Coarse 1
            "op1_out": 99,
            "op1_env": [80, 40, 20, 30,
                        99, 85, 0,  0],
        },
        "E. Piano 1": {
            # Plucky FM Tine
            "op2_ratio": 14.0, # High frequency bell/tine harmonic
            "op2_out": 70,
            "op2_env": [99, 70, 20, 40,
                        99, 0,  0,  0],
            
            "op1_ratio": 1.0,  # Fundamental electric piano body
            "op1_out": 99,
            "op1_env": [90, 60, 40, 40,
                        99, 80, 60, 0],
        },
        "Tubular Bells": {
            "op2_ratio": 3.5,  # Inharmonic bell ratio
            "op2_out": 80,
            "op2_env": [99, 30, 10, 20,
                        99, 70, 0,  0],
            
            "op1_ratio": 1.0,
            "op1_out": 99,
            "op1_env": [99, 40, 15, 20,
                        99, 85, 0,  0],
        }
    }
    
    for name, p in presets.items():
        print(f"\n# === DX-7 {name} (Algorithm 16 style) ===")
        
        # Calculate Modulator Transposition (N)
        import math
        # DX-7 Ratio to semitones: 12 * log2(Ratio)
        op2_semi = round(12 * math.log2(p["op2_ratio"])) if p["op2_ratio"] > 0 else 0
        
        # Calculate Skred FM Depth based on Modulator output level (0-99)
        # DX-7 mod depth is highly exponential. 
        mod_depth = 0.05 * (1.08 ** (p["op2_out"] / 2.0))
        
        print("# --- Operator 2 (Modulator) ---")
        print("v1 w0 a 0")
        print(f"N 0, {op2_semi}00         # Ratio {p['op2_ratio']} transposed to {op2_semi} semitones")
        print("m 1               # Modulator audio is muted")
        print(generate_dx7_env(*p["op2_env"]))
        
        print("\n# --- Operator 1 (Carrier) ---")
        print("v0 w0 a 10        # Carrier output volume")
        print("G 1 H 1           # Route Note and Velocity triggers to Modulator (v1)")
        print(f"F 1 {mod_depth:.2f}          # Phase Modulate with Modulator (v1)")
        print("FF 2              # Enable Phase Modulation (True DX-7 math)")
        print(generate_dx7_env(*p["op1_env"]))

if __name__ == "__main__":
    generate_dx7_presets()
