# Release 0.63.0: Phase Distortion, Sidechain Envelopes & Orchestration Polish

This release brings massive enhancements to Skred's synthesis capabilities—specifically focusing on Phase Distortion, modulation, and envelope routing—alongside critical fixes to polyphony and the WASM build pipeline.

## New Features & Synthesis Enhancements
* **Inverted / Ducking Envelopes (`k 2`)**: Added a new amplitude envelope mode (`k 2`) to the engine. When active, the envelope mathematically inverts (`1.0 - env`), allowing you to easily create sidechain-compression "ducking" drones that pump in volume when triggered.
* **Authentic Casio CZ Phase Distortion (`w 47`)**: Added a dedicated Cosine wave at slot `w 47`. Because standard sine waves hit a zero-crossing at phase 0.5, applying Phase Distortion to them creates jagged, digital artifacts. The new Cosine wave correctly centers the PD kink at the absolute peak, faithfully recreating the smooth, brassy sweeps of authentic 80s Casio CZ hardware.
* **Phase Distortion Guide (`doc/phase_distortion.md`)**: Authored a comprehensive deep-dive on Phase Distortion synthesis. Includes classic Casio patches (Brass, Pulse Bass, Syn-Tom) and advanced, modern Neurohop/Dubstep techniques (Skrillex-style "Yoy" growls, Wobble basses, and live sequence modulations).

## Fixes & Improvements
* **WASM Build Fix**: Fixed a critical build failure in the WASM pipeline (`make very-all`) by injecting `midi_player.c` into the Emscripten compile list, resolving missing symbol errors.
* **Drum Polyphony Patch**: Fixed note-stealing during dense drum sequences. Drum samples are now correctly routed to a dedicated, initialized polyphony pool (Voices 6-9) with a base amplitude (`a 10`), preventing silent strikes.
* **Waveform Validation**: Audited and fixed tutorials (`midi_native_tutorial.md`) and orchestration scripts to explicitly use valid core Korg DWGS waveforms (`w 15`, `w 16`, `w 18`).

## Content & Examples
* **New Orchestrations**: Added fully-voiced, multi-track MIDI master scripts for classic tracks:
  * `blue_monday_master.sk` (New Order)
  * `i_feel_love_master.sk` (Donna Summer - 130 BPM)
  * `seven_nation_army_master.sk` (The White Stripes - 120 BPM)
* **CZ Patch Bank Update**: Updated `parts/examples/pd-cz-inspired.sk` to utilize the new `w 47` Cosine wave for more authentic vintage emulation.
* **Workspace Cleanup**: Purged leftover intermediate C and Python diagnostic scripts from the project root.
