#!/usr/bin/env python3
"""build_all_ks_slots.py - Generates 32 .ks wave files (slots 50 to 81) for wavegen."""

import os
import numpy as np

# Slot mappings: (SlotNumber, InternalWaveName, ROM_Offset, Length, OneShot)
SLOT_MANIFEST = [
    (50, "saw", 0x3700, 256, 0),
    (51, "bell", 0x2900, 256, 0),
    (52, "sine", 0x3600, 256, 0),
    (53, "square", 0x7800, 1024, 0),
    (54, "pulse", 0x7400, 1024, 0),
    (55, "noise1", 0x0400, 256, 0),
    (56, "noise2", 0x0800, 256, 0),
    (57, "noise3", 0x0C00, 256, 0),
    (58, "bass", 0x1000, 512, 0),
    (59, "piano", 0x1400, 512, 0),
    (60, "elpno", 0x1800, 512, 0),
    (61, "voice1", 0x2400, 256, 0),
    (62, "voice2", 0x2600, 256, 0),
    (63, "kick", 0x0000, 512, 1),  # One-shot kick
    (64, "reed", 0x2B00, 256, 0),
    (65, "organ", 0x2E00, 256, 0),
    (66, "synth1", 0x2F00, 256, 0),
    (67, "synth2", 0x3400, 256, 0),
    (68, "synth3", 0x3500, 256, 0),
    (69, "formt1", 0x2000, 256, 0),
    (70, "formt2", 0x2100, 256, 0),
    (71, "formt3", 0x2200, 256, 0),
    (72, "formt4", 0x2300, 256, 0),
    (73, "formt5", 0x2D00, 256, 0),
    (74, "pulse2", 0x7000, 1024, 0),
    (75, "sqr2", 0x6C00, 1024, 0),
    (76, "4octs", 0x3000, 256, 0),
    (77, "prime", 0x3100, 256, 0),
    (78, "brass", 0x3200, 256, 0),
    (79, "string", 0x3300, 256, 0),
    (80, "octave", 0x2A00, 256, 0),
    (81, "oct5th", 0x2C00, 256, 0),
]

output_dir = "waves"
os.makedirs(output_dir, exist_ok=True)

with open("esq1wavlo.bin", "rb") as f:
    rom = f.read()

for slot, name, offset, length, oneshot in SLOT_MANIFEST:
    raw = np.frombuffer(rom[offset : offset + length], dtype=np.uint8)
    dc = float(np.mean(raw))
    wave = (raw.astype(np.float64) - dc) / 128.0

    if length > 256:
        stride = length // 256
        wave = wave[::stride]

    # Fourier decomposition (16 harmonics)
    fft_vals = np.fft.rfft(wave)[1:17]
    mags = np.abs(fft_vals) * (2.0 / len(wave))

    peak = np.max(mags) if np.max(mags) > 1e-6 else 1.0
    norm_a = mags / peak
    norm_a[norm_a < 0.005] = 0.0

    a_str = " ".join(f"{v:.4f}" for v in norm_a)
    filename = os.path.join(output_dir, f"{slot}_{name}.ks")

    with open(filename, "w") as out:
        out.write(f"/ ESQ-1 Slot {slot}: {name.upper()}\n")
        out.write(f"/ Source: 0x{offset:04X} ({length} bytes)\n\n")

        # 1. Engine metadata
        out.write("S: 44100.0\n")
        out.write("L: 0\n")
        out.write("E: 4095\n")
        out.write(f"O: {oneshot}\n\n")

        # 2. Phase calculation across 4096 samples
        out.write("N: 4096\n")
        out.write("X: (!N) % (N * 1.0)\n")
        out.write("P: X * (p 2)\n\n")

        # 3. Harmonics vector
        out.write(f"A: {a_str}\n\n")

        # 4. Final expression evaluating directly to the audio array
        out.write("s(P $ A)\n")

print(
    f"Successfully generated {len(SLOT_MANIFEST)} .ks files in '{output_dir}/'."
)