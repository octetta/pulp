# Presentation Architecture: BEEPS and PORTS
**Subtitle:**

Annoy everyone in your house while building an audio synthesizer

**Tagline:**

Follow my two-plus-year (but really life-long journey) to make exactly the sound creation tools that I always wanted and that no one else asked for.

2 年以上（でも本当には生涯にわたる）の旅に従い、私がいつも欲しかった、そして他の誰も求めていない音声作成ツールを正確に作ることです。

---

## Presentation Timeline (Total: 40 Minutes)

| Phase | Section | Time Allocation |
| :--- | :--- | :--- |
| **Phase 1** | The Obsession & The Control Plane (Slides 1–2) | 5 Minutes |
| **Phase 2** | The Architecture of Isolation (Slides 3–4) | 10 Minutes |
| **Phase 3** | Mathematical Shortcuts with k-synth (Slides 5–6) | 10 Minutes |
| **Phase 4** | The Part Where We Annoy the Room (Slides 7–9) | 15 Minutes |

---

# Slide-by-Slide Outline

## Phase 1: The Obsession & The Control Plane (0:00 - 0:05)

### Slide 1: Title & The Tagline
*   **Visual:** Big, bold, central text.
    *   *BEEPS and PORTS: annoy everyone in your house while building an audio synthesizer*
    *   *ビープ音とポート：オーディオシンセサイザーを構築しながら家中の誰もが迷惑する*
*   **Speaker Voice:** Read it out loud with a smile. "This is a two-plus-year journey—honestly a lifelong obsession—to build the exact sound creation tools that I always wanted, and that absolutely no one else asked for."

### Slide 2: The Horizon & The Friction
*   **Visual:** A simple two-tier architecture stack diagram:
    *   `Control Plane: Elixir (Future Node Orchestration & Logic)`
    *   `Synthesis Layer: SKRED in the PULP repository (Today's Focus)`
*   **Speaker Voice:** "To put this in context up front: the ultimate home for this engine is an ecosystem where Elixir acts as the high-level control plane to handle complex orchestration, sequencing, and cluster logic. But we aren't focusing on Elixir today. Today, we are zooming all the way down into the underlying engine layer itself. Before you can orchestrate a wall of sound, you need a low-friction, immediate way to make raw noise directly from a text interface."

---

## Phase 2: The Architecture of Isolation (0:05 - 0:15)

### Slide 3: Separation of Concerns (Pulp vs. Skred vs. SKODE)
*   **Visual:** A clear, linear text flow diagram:
    `SKODE (ASCII Shorthand)` -> `SKRED (CLI / WASM / C API)` -> `PULP repository components` -> `Speakers`
*   **Speaker Voice:** Break down the ecosystem:
    *   **PULP:** The repository containing the engine, parser, sequencer, hosts, experiments, and build tooling.
    *   **SKRED:** The audio engine and public C API, with native CLI and WebAssembly hosts.
    *   **SKODE:** The ultra-compact ASCII shorthand language used to talk to the engine.

### Slide 4: Safe Architecture via Pipes and Network Ports
*   **Visual:** A simple graphic showing an isolated process box with an explosive "Blast Radius" indicator contained cleanly outside the host system.
*   **Speaker Voice:** "Let's be completely candid: this C code is not defensive, bubble-wrapped, corporate enterprise software. If you feed it complete garbage, it might segfault. But instead of cluttering the high-priority audio thread with thousands of lines of bulletproof error-checking, we handle safety through architecture.
    
    For process isolation, we can run `mini-skred -n` behind **pipes** or control a separate process through **UDP**. The embedded C API and browser build run the command and rendering layers in one process, so process isolation is a host choice rather than an intrinsic property of the engine."

---

## Phase 3: Mathematical Shortcuts with k-synth (0:15 - 0:25)

### Slide 5: Array-Oriented Wave Generation
*   **Visual:** A clean, short snippet of your right-associative `k-synth` syntax code. Below it, a text link: `github.com/kparc/ksimple`.
*   **Speaker Voice:** "To populate an audio engine, you need data structures. Writing massive, nested loops in raw C just to generate lookup wavetables is tedious and high-ceremony. Instead, I integrated an array-oriented syntax directly into the data pipeline via `k-synth`.
    
    It is heavily modeled after `ksimple`—Arthur Whitney’s minimal K interpreter designed for educational clarity. Adopting that compact style allows me to mathematically define complex audio arrays on the fly on the host side or in the browser workspace with practically zero boilerplate."

### Slide 6: Vintage Tables & Homage to 1985
*   **Visual:** A bulleted list highlighting:
    *   16 digital wavetables modeled after the **Korg DW-8000** Digital Waveform Generator System (DWGS).
    *   Bespoke, array-generated digital drum definitions.
*   **Speaker Voice:** "Once you have an array language baked into your synthesis workspace, you need something iconic to generate.
    
    A massive shoutout to Korg, who is sponsoring this event—I've spent a lot of time obsessed with the 1985 **Korg DW-8000**. Its digital waveform architecture was brilliant. So, I used `k-synth` as a rapid generator to mathematically recreate those exact 16 single-cycle digital wavetables. Alongside those vintage shapes, I wrote tight definitions for digital drum sounds. This gives our runtime an immediate, rich, nostalgic sonic palette right out of the box—no massive sample folders required."

---

## Phase 4: Making Noise (0:25 - 0:40)

### Slide 7: The SKODE Cheat Sheet
*   **Visual:** High-contrast text examples for the audience to track visually during the demo:
    *   `v0 w0 f440 a0` -> Voice 0, sine wave, 440 Hz, unity amplitude (0 dB).
    *   `v1 m1 f4` -> Voice 1 becomes a control modulator running at 4 Hz.
    *   `v0 F1,50` -> Target Voice 0's frequency modulation variable using Voice 1 as the source.
*   **Speaker Voice:** "Before I pull up the live shell, here are the basic keys. The grammar is stateless in structure but stateful in execution—parameters stay exactly where you put them until a new command overrides them."

### Slide 8: The Live Demo
*   **Visual:** A single, bold word: **DEMO**. (Include the playground URL: `octetta.github.io/pulp/doc/`).
*   **Speaker Voice:** Drop the slides completely and switch to your live workspace (Terminal or WebAssembly browser shell). Execute your live sequence:


Act I (The Baseline Tone):
v0 w0 f220 a-8

Act II (The Modulation Layer):
v1 m1 f6
v0 F1,40

Act III (The Korg Homage & Drums):
v0 w8
v2 w0 n36 a-6 l1

Act IV (The Fade Out):
v0 a-60 v2 a-60


----

### Slide 9: Links, Future, & Collaborations

*  **Visual:** Clean, scannable links and future milestones:


Engine Core: github.com/octetta/pulp

"skred try-it" Shell: octetta.github.io/pulp/doc

"skred learn-it" tutorial: octetta.github.io/pulp/doc/learn.html

Array Generator: github.com/octetta/k-synth

"ksynth try-it": octetta.github.io/k-synth


Next Steps: Exploring libpd embedding, macro parsing, and cluster sequencing.

Looking For: Compelling R&D collaborations, instrument design exploration, or deep architectural discussions with people who love synthesis.

*  **Speaker Voice:**: "I built this because it’s the exact tool I wanted to see in the world. If you want to push the boundaries of low-ceremony text protocols, explore custom wavetable generation, or talk audio instrument R&D, let’s grab a beer after the sessions. Go clone the repos, play with the shorthand, crash the shell, and go annoy everyone in your own house.
Thank you."
