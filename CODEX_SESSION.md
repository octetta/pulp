# Codex Session

This file is a lightweight handoff note for work that spans Codex sessions,
VS Code windows, and related repositories. Keep it short and update it at
meaningful handoff points, especially before switching between `pulp` and
`k-synth`.

Preferred target path is `.codex/SESSION.md`, but `.codex` is currently
read-only in this workspace, so this root-level file is the active session
note for now.

## Current Goal

Continue the new Skode dictionary/macro architecture and MIDI integration while
keeping native, Windows, and WebAssembly builds stable.

## Related Repositories

- `/home/stewartj/book/pulp`
- `/home/stewartj/book/k-synth`
- `/home/stewartj/ro-totem`

## Current State

Pulp has the reconciled vendored k-synth API, the Skode dictionary foundation,
promoted real-time-capable macros, and the first complete optional MIDI I/O
slice. The working tree is clean on `main` at `eb92677` and matches
`origin/main` as of 2026-07-16, apart from this handoff-note update.

k-synth has the owned `ks_eval()` result contract restored, `ks_bind_vector()`
restored, and `main.c` frees direct eval results after optional display.

`ro-totem` is also part of the working set. Specific dependencies between
`ro-totem`, Pulp, and k-synth are not documented yet.

## Important Contracts

- `ks_eval()` returns an owned `K` that callers release with `k_free()`.
- `ks_bind_vector()` copies host-owned doubles into persistent `A`-`Z` vars.
- Pulp's `parts/api.c.kit` is the source for generated API code.
- `parts/analysis-src/api.c` should stay aligned with generated maxed output.
- In wasm, `/cer 0` must not block the browser thread on `pthread_join()`.
- MIDI backend callbacks only publish bounded control events; they do not run
  Skode, allocate event payloads, or call host code.
- Do not create branches, commit, merge, push, or otherwise change Git history
  unless the user explicitly asks for that operation.

## Recent Pulp Changes

- `parts/vendor/ksynth/ksynth.c`
- `parts/vendor/ksynth/ksynth.h`
- `parts/CMakeLists.txt`
- `parts/api.c.kit`
- `parts/analysis-src/api.c`
- `doc/skred_api.wasm`

## Verified

- `make native`
- `make maxed`
- `make wasm`
- `ctest --test-dir build_native --output-on-failure`
- `ctest --test-dir build_maxed --output-on-failure`
- `./build_native/ksynth_bridge_tests`
- `./build_maxed/ksynth_bridge_tests`

## Handoff Practice

Before switching repos or ending a work session, append a short checkpoint with:

- changed files
- tests or builds run
- unresolved issues
- cross-repo dependencies

Use dated headings under `Checkpoints`.

## Checkpoints

### 2026-07-01 - Initial Session Note

Created this handoff note after reconciling k-synth/Pulp API contracts and
fixing the wasm control-plane `/cer 0` hang path.

### 2026-07-01 - Skode Loader, Macros, Registers

- `/l` now uses an explicit per-load `skode_t` instead of a static loader
  context, reports line-specific load errors, and copies loader logs back to
  the caller.
- ANDS macros are now global, so `[name] : body ;` definitions loaded from a
  `.sk` file are available afterward in the interactive/browser context.
- Skode register docs now describe `$N`/`=` as shared registers; `l>g` and
  `g>l` were removed from the Skode command surface.
- Added regression coverage for `/l` installing a global macro and shared
  register value.
- Verified: `make`, `make native`, `make wasm`, `make maxed`, plus native,
  wasm, and maxed parser/state/ksynth/audio tests.
- Note: `make` and `make native` both use `parts/build_native`; do not run
  them concurrently.

### 2026-07-02 - Global DW-Style Delay

- Added a global fixed-size main L/R delay bus with DW-8000-like controls:
  coarse, fine, feedback, mod frequency, mod depth, and level.
- Added per-voice `ds` delay send. Sends use the voice's post-amp mono signal
  before pan, but only feed the delay when the voice is centered (`p0`) and
  pan modulation is disabled.
- The delay path is mono-send/stereo-return: modulation intensity spreads the
  L/R taps, and modulation frequency animates that stereo spread.
- Internally the code is shaped as `delay_bus[0]` so more independent delay
  buses can be added later without changing existing `DL` patches.
- Added Skode commands: `DL coarse,fine,feedback,modfreq,moddepth,level`,
  `DL?`, and `ds amount`.
- Added synth API helpers: `delay_send_set()`, `delay_params_set()`,
  `delay_params_get()`, `delay_clear()`, and `delay_format()`.
- Verified: `make`, `make native`, `make wasm`, `make maxed`; native, wasm,
  and maxed parser/state/ksynth/audio tests.

### 2026-07-02 - Four Delay Buses

- Expanded the DW-style delay from one bus to four independent buses labeled
  `1` through `4`.
- Updated commands to `DL bus,coarse,fine,feedback,modfreq,moddepth,level`,
  `ds bus,amount`, `DL? [bus]`, and `GS`.
- Kept compatibility for one-argument `ds amount`, which now targets bus `1`.
- Each voice can select one delay bus; delay sends still require centered
  pan and no pan modulation.
- Delay buses skip audio processing while idle, then remain active long enough
  for delay memory/tails to drain after receiving input.
- Updated Skode command docs and examples, plus regression tests for bus
  selection, centered-pan gating, stereo return, and `GS` output.
- Verified: `make`, `make native`, `make maxed`, `make wasm`; native, maxed,
  and wasm build-directory `ctest --output-on-failure`.

### 2026-07-03 - Track-Aligned Delay Routing Design

- Decided the four delay lines should align with record/scope tracks rather
  than remain separately selected per voice.
- Intended model: `r1`..`r4` selects the stem and its associated delay,
  `ds amount` controls only the voice's send amount, and `DL track,...`
  configures the delay attached to that track.
- Delay returns should remain in the main mix and also be included in the
  matching record/scope stem.
- Implemented the track-aligned model in `synth.c.kit`, `skode.c.kit`, and
  generated analysis sources: removed per-voice delay-bus state, made `ds`
  set only amount, derive the active delay from `r1`..`r4`, and add wet returns
  to the matching stem.
- Added a cheap `/a?` delay status line via `delay_status()`: active delay
  lines, configured sends, and currently eligible routed sends.
- Verified: `make native`, `ctest --test-dir build_native --output-on-failure`,
  `make maxed`, `ctest --test-dir build_maxed --output-on-failure`.

### 2026-07-03 - One-Shot Wave Loop Points

- Added wave-level loop point metadata command `WL wave,start,end`.
  `start` is inclusive, `end` is the exclusive loop boundary, and `end` may
  equal the wave sample count.
- Extended `W@` parameters: `3` reads loop start and `4` reads loop end.
- Updated one-shot release semantics: `l0` now asks any active one-shot loop,
  bounded or unbounded, to leave the loop at the next boundary in the current
  playback direction, then continue toward the physical wave end or beginning.
- Updated Skode docs and architecture notes; added regression coverage for
  `WL`, `W@` loop metadata, forward release exit, and backward release exit.
- Verified: `make native`, `ctest --test-dir build_native --output-on-failure`,
  `make maxed`, `ctest --test-dir build_maxed --output-on-failure`.

### 2026-07-03 - Voice Loop Overrides

- Added `VL start,end` for per-voice loop point overrides after a wavetable is
  assigned. Plain `VL` resets the selected voice back to its current wave's
  `WL` defaults.
- `w` clears voice loop overrides by copying wave defaults, while later `WL`
  edits update only voices that are still following those defaults.
- Voice display emits pasteable `VLstart,end` only when an override is active.
  `GS1` now emits `WL` metadata for user-loaded waves so wave defaults and
  voice overrides can be reconstructed together.
- Confirmed with regression coverage that `VL` affects `l1` playback once
  looping is enabled with `B1`/`BC`; documented that `VL` does not enable
  looping by itself.
- Clarified the one-shot lifecycle: triggers begin at the physical wave edge,
  enter the loop at the first directional loop boundary, and release or
  bounded-loop exhaustion exits toward the physical tail.
- Verified: `make native`, `ctest --test-dir build_native --output-on-failure`,
  `make maxed`, `ctest --test-dir build_maxed --output-on-failure` from
  `parts/`.

### 2026-07-03 - Wave Display Loop Markers

- Improved single-wavetable `W` display headers to show sample count, baseline
  duration at the wave rate, one-shot state, loop points, and loop duration.
- Single-wave waveform output now uses the labeled display path so loop points
  are drawn as a marker row under the waveform in both ASCII and Braille modes.
- Updated command docs for the `[loop_start..loop_end)` marker convention.
- Verified: `make native`, `ctest --test-dir build_native --output-on-failure`,
  `make maxed`, `ctest --test-dir build_maxed --output-on-failure` from
  `parts/`.

### 2026-07-03 - Recording Auto-Trim

- Improved `w<>` recording auto-trim: default silence threshold is now `0.001`
  instead of exact zero, detection requires a short run of consecutive audible
  samples, and an optional third argument adds sample margin before the nearest
  zero-crossing search.
- Updated docs to describe `w<> [threshold[,end-threshold[,margin-samples]]]`.
- Verified: `make native`, `ctest --test-dir build_native --output-on-failure`,
  `make maxed`, `ctest --test-dir build_maxed --output-on-failure` from
  `parts/`.

### 2026-07-05 - Skred VFS Zip Loading and ro-totem Notes

- Added Skred VFS/ZIP support under `parts/exp-vfs/` and exposed it through
  `skred/api.h`: mount disk/zip, mount in-memory zip, unmount, cwd/list/read
  helpers, and whole-file read helpers.
- Skode file-loading commands now route through the VFS/search layer. The search
  order is mounted VFS, current real directory, then type-specific `sk`, `wav`,
  and `ks` directories as appropriate. The `file:` prefix forces real filesystem
  access while a ZIP is mounted.
- Added compact Skode filesystem commands: `[bundle.zip] %z`, `%zu`, `%pwd`,
  with `%cd`, `%ls`, and `%cat` using the VFS view.
- Updated wasm support so `doc/index.html` can upload and mount a ZIP asset
  bundle with `zip`, then unmount with `unzip`; `parts/wasm-assets/mkwasm`
  exports `_malloc`, `_free`, and the VFS entry points.
- Refreshed `doc/skred_api.js` and `doc/skred_api.wasm` from the wasm build.
- Reviewed `/home/stewartj/book/ro-totem` for adoption. Best first use there is
  to mount accepted project ZIPs and keep `waves/name.wav` and `files/name`
  paths instead of extracting to a temp directory. Real user-selected files
  should use `file:/absolute/path` when a project ZIP is mounted.
- Added `/home/stewartj/book/ro-totem/SKRED_VFS_NOTES.md` with future adoption
  notes. That repo has the note as an untracked file unless it is committed
  separately.
- Important cross-repo caution: updated Skred static libraries include miniz/zip
  symbols, while ro-totem currently also compiles `vendor/miniz/miniz.c`. Check
  for duplicate or ambiguous `mz_*`/`tinfl_*`/`tdefl_*` symbols when updating
  ro-totem's vendored Skred.
- Verified during this stretch: maxed and wasm `skode_state_tests`, plus wasm
  API relink via `parts/wasm-assets/mkwasm`. The wasm link still emits expected
  Emscripten pthread/memory-growth warnings.

### 2026-07-16 - Dictionary, Macro Returns, and MIDI Foundation

- The Skode dispatch refactor now has dictionary-backed implementations for
  `v`, `a`, `f`, `n`, `p`, and `m`; architecture follow-up notes live in
  `parts/ARCHITECTURE.md`.
- `@` is no longer an atom symbol. Earlier `d@`, `w@`, `W@`, and `v@` reader
  commands were renamed to `d*`, `w*`, `W*`, and `v*`.
- Macro return values use push-style `*R value`, and the non-intrusive query
  command is `?R`. Named macros are compiled when defined: definitions made
  entirely from real-time-capable operations are promoted to cached dictionary
  programs, while immediate macros retain text-expansion behavior. `?m` reports
  each macro's classification.
- Vendored `github.com/octetta/minimidio` exactly at revision
  `0d1f33a24ca1aa9b8d22727e42cdda057570992a`, with MIT license and provenance
  under `parts/vendor/minimidio/`.
- Added optional `MIDI=1` support in `parts/midi.c`: public context,
  capabilities, input/output enumeration, device open/close, virtual ports,
  event masks, and raw output APIs. The public API does not expose minimidio
  types.
- Incoming structured MIDI 1.0 messages publish
  `SKRED_CONTROL_EVENT_MIDI` into the existing bounded multi-producer control
  ring. `id` is the MIDI message type and `value[]` contains channel, data1,
  and data2. System messages use channel `-1`; song position uses data1. SysEx
  input and active sense are excluded by default; variable-length SysEx input
  still needs a separate bounded payload design.
- Runtime device commands, routed before Skode parsing and kept within the
  four-character atom limit, are `/mL`, `/m?`, `/mi N`, `/miV [name]`, `/mi-`,
  `/mo N`, `/moV [name]`, and `/mo-`.
- Immediate output commands are `MO status[,data1[,data2]]` and `d>MO` for a
  raw data-array buffer (including SysEx). They are control-plane operations,
  not schedulable real-time opcodes.
- Web MIDI uses explicit user-triggered initialization (`/mL` or API) and an
  Asyncify-enabled WASM build. Web MIDI supports enumerated ports but not
  virtual ports. CoreMIDI and ALSA support virtual ports; WinMM does not.
- Added `parts/tests/midi_tests.c`, updated API/user/architecture documentation,
  refreshed generated analysis sources, and rebuilt `doc/skred_api.js` and
  `doc/skred_api.wasm`.
- Verified: MIDI-enabled and MIDI-disabled native builds, strict C11 compilation
  of `midi.c`, maxed native build, `make wasm`, Windows Zig cross-build with
  `MIDI=1`, and the hardware-independent MIDI control-event tests.
- Known unrelated test failure: `skode.command-state` still fails the same three
  pre-existing wavetable-display assertions (`loop 3..8 |5|`, `baseline`, and
  `voice 6 wave 300`). The failure reproduces in an older existing maxed build;
  all other maxed tests pass. No physical MIDI hardware test has been run yet.
- Git checkpoint: `main` is at `eb92677` (`be brave with midi`), with MIDI work
  also represented by preceding commit `bbd5506`. The tree was clean and
  synchronized with `origin/main` when this note was updated.
