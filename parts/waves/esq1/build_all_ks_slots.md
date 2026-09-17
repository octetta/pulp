### Required ROM Assets

This script extracts raw single-cycle and multisample PCM tables directly from 
the original Ensoniq ESQ-1 factory ROM binaries. Due to copyright restrictions, 
these proprietary ROM images cannot be redistributed with this repository. 
Users must provide their own legally acquired or dumped binary images.

| Filename | Description | Expected Size | Expected CRC32 / MD5 |
| :--- | :--- | :--- | :--- |
| `esq1wavlo.bin` | ES5503 DOC Waveform ROM (Bank 0 / Low) | **65,536 bytes** (64 KB) | `26ec7e3d` / `e9df95b364817a3a8a3ee2688081a28a` |
| `esq1wavhi.bin` *(optional)* | ES5503 DOC Waveform ROM (Bank 1 / High) | **65,536 bytes** (64 KB) | `75a74e53` / `152069ce542bfbe8b2e5975dbd572dfc` |
| `3p5lo.bin` *(optional)* | 6809 Firmware / OS EPROM v3.5 (Low) | **32,768 bytes** (32 KB) | `b6e9a6df` / `d59cb879b2914104278486fc7b65313a` |

*Note: For all sustained single-cycle geometric and formant oscillators (Slots 50–81), only `esq1wavlo.bin` is required.*

#### Where to Obtain the Files
These files are standard dumps of the Ensoniq ESQ-1 hardware firmware and wave generation EPROMs. They can be found in the following public archives:

1. **MAME ROM Distribution Sets:** Search for the MAME driver package `esq1.zip`. The archive contains `esq1wavlo.bin` and `esq1wavhi.bin` as standard ROM definitions.
2. **Rainer Buchty's Ensoniq Hardware Archive:** Available under the firmware downloads and hardware reverse-engineering section at `buchty.net/ensoniq`.
3. **Physical EPROM Dumps:** DUMP 27C512 (or equivalent 64 KB EPROM) sockets labeled `WAV-LO` from an ESQ-1 / ESQ-M synthesizer mainboard.

Place `esq1wavlo.bin` into your working directory prior to running `build_all_ks_slots.py`.