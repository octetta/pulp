# Codex Session

This file is a lightweight handoff note for work that spans Codex sessions,
VS Code windows, and related repositories. Keep it short and update it at
meaningful handoff points, especially before switching between `pulp` and
`k-synth`.

Preferred target path is `.codex/SESSION.md`, but `.codex` is currently
read-only in this workspace, so this root-level file is the active session
note for now.

## Current Goal

Pause after completing the MIDI control-plane integration and signed
phase-distortion envelope work. On return, preserve native, Windows, and
WebAssembly behavior while addressing only the known unrelated test failures
if the user chooses to prioritize them.

## Related Repositories

- `/home/stewartj/book/pulp`
- `/home/stewartj/book/k-synth`
- `/home/stewartj/ro-totem`

## Current State

Pulp has the reconciled vendored k-synth API, dictionary-backed Skode commands,
promoted real-time-capable macros, optional MIDI I/O plus control-plane
voice/poly/binding routes, canonical audio-device commands, and signed
phase-distortion with a dedicated envelope. The working tree is intentionally
dirty with the current uncommitted MIDI, documentation, PD, benchmark,
generated-analysis, and WASM-artifact changes. Do not discard or overwrite
them when resuming.

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

- MIDI implementation/API/help: `parts/midi.c`, `parts/midi.h`,
  `parts/api.c.kit`, `parts/api.h`, and Skode documentation.
- Signed PD and envelope: `parts/synth.c.kit`, `parts/synth-state.h.kit`,
  `parts/synth-alloc.c.kit`, `parts/skode.c.kit`, `parts/skode-event.c`, and
  `parts/skode.h`.
- Validation support: `parts/tests/midi_tests.c`,
  `parts/tests/skode_state_tests.c`, and `parts/tests/synth_callback_bench.c`.
- Generated maxed analysis sources and `doc/skred_api.js`/`.wasm` are refreshed.

## Verified

- `make native`
- `make maxed`
- `make wasm`
- `ctest --test-dir build_native --output-on-failure` (known wave-display
  assertions remain)
- `ctest --test-dir build_maxed --output-on-failure` (known wave-display and
  track-delay assertions remain)
- `./build_native/ksynth_bridge_tests`
- `./build_maxed/ksynth_bridge_tests`
- `./build_maxed/synth_callback_bench 32 20000 <scenario>` for all PD scenarios
- 64- and 128-voice PD callback stress passes

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
  four-character atom limit, are `/mL`, `/m?`, `/mi N`, `/miV [name]`, `/mic`,
  `/mo N`, `/moV [name]`, and `/moc`.
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

### 2026-07-16 - WASM Lazy Dictionary Initialization Fix

- Fixed `skred_command()`/WASM reporting `unknown atom` for the newly
  dictionary-backed `v`, `a`, `f`, `n`, `p`, and `m` words. `skode_consume()`
  now initializes the dictionary alongside its existing lazy parser setup for
  API-owned `SKODE_EMPTY()` contexts that never pass through `skode_init()`.
- Added API-path regression coverage using the exact compact command
  `v0a0f440l1`, refreshed `parts/analysis-src/skode.c`, and rebuilt
  `doc/skred_api.wasm`.
- Verified native and WASM-feature builds, both `audio_command_tests`, and the
  rebuilt WASM artifact directly under Node (no unknown atoms). Full ctest has
  only the same three known wavetable-display assertion failures documented
  above; all other tests pass.

### 2026-07-16 - Control-Plane MIDI Note Routing

- Added a fixed 32-entry MIDI route table applied by the existing control
  dispatcher, after backend callbacks publish into the bounded event ring.
  Routes map one zero-based MIDI channel or all channels to a physical voice or
  poly pool, handling note-on, note-off, and configurable-range pitch bend.
- Commands are `/mv channel,voice[,bend]`, `/mp channel,pool[,bend]`, `/mvd`,
  `/mpd`, `/mR`, and `/mC`; `.` or `-` selects every channel and bend defaults to ±2
  semitones. Groups remain templates, so live poly MIDI targets their pool.
- All-channel pool keys combine channel and note, preserving independent note
  lifetimes and pitch bend for equal notes arriving on different channels.
  Removing, replacing, or clearing a route releases its held notes.
- Added the public `skred_midi_route_*()` API and WASM exports, refreshed
  generated analysis sources and browser artifacts, and documented the routing
  model and commands.
- Verified MIDI-enabled and MIDI-disabled builds, strict C11 compilation,
  hardware-free voice/pool/channel/bend tests, WASM-feature tests, and direct
  Node loading of the new WASM exports. Full WASM ctest retains only the three
  known wavetable-display failures; all other tests pass.
- Follow-up: replaced the non-Skode `*` wildcard with NaN-style `.`/`-` and
  added `/mb type,channel,data1 command` generic MIDI-to-Skode bindings for
  transport, CC, program/pressure, and other fixed messages. Templates support
  `{ch}`, `{d1}`, `{d2}`, `{unit}`, and `{bend}`; `/mb?`, `/mbd`, and `/mbC`
  inspect, remove, and clear bindings.
- Follow-up: `/mb` message types are numeric-only; textual aliases were removed
  to keep the command surface within Skode's numeric argument grammar. The user
  reference documents every accepted decimal type value.
- Follow-up: renamed invalid `/mb-` to the valid four-character atom `/mbd`
  (`d` for delete); the old spelling is no longer intercepted.
- Follow-up: MIDI management, route, and binding commands now also dispatch as
  genuine immediate Skode atoms, so loader contexts and immediate macros can
  invoke them. Binding commands use the parser string form `[command] /mb
  type channel data1`; status emits the same pasteable form. Invalid close and
  route-delete spellings were renamed to `/mic`, `/moc`, `/mvd`, and `/mpd`.
- Audio-device audit: `/als` and `/a?` were API-only, while `/aout` was too long
  to be an atom and `default`/`off` were textual selectors. Added genuine
  immediate Skode `/ao selection` and `/ai selection` commands (`-1` default,
  capture `-2` off), and made `/als` and `/a?` work through Skode as well.
- MIDI `{ch}`/`{d1}`/`{d2}`/`{unit}`/`{bend}` forms are explicitly documented
  as dispatcher template markers inside opaque bracket strings, not as Skode
  operators; expansion produces numeric Skode text before parsing.
- Added embedded `/h` records for all canonical MIDI and audio-device commands.
  They are appended after legacy help records so established numeric category
  indices remain stable. `[midi] /h`, `[/mb] /h`, and `[/ao] /h` now expose
  the new command surface and are covered by help regression tests.
- Documented `/mb` drum-map recipes: one-note/one-voice triggers, layered
  multi-voice drums, velocity-fed immediate macros, channel-10 isolation, and
  hi-hat choke behavior. Architecture notes distinguish this current
  control-plane approach from a possible future precompiled drum-map layer.
- Added MIDI pitched-instrument recipes covering direct single-voice mono,
  pool-backed mono with priority/legato/held-note fallback, and multi-voice
  polyphonic pools with stealing policies, bend ranges, and channel selection.

### 2026-07-16 - Signed Phase Distortion, PD Envelope, and Rest Checkpoint

- Changed the public `c mode,amount` phase-distortion amount to signed
  `-1..1`, with `0` as the exact identity for every PD mode. This is an
  intentional patch-semantic break from mode 1's former `.5` neutral point.
- The audio path now combines the signed base amount, optional `C` oscillator
  modulation, and the new PD envelope, then clamps once to `-1..1`. Fixed the
  older bug where the base `c` amount was ignored unless a `C` modulator was
  connected.
- Added schedulable `ct attack,decay,sustain,release` and `cd depth` commands,
  including opcode names, built-in help, voice formatting/copy/reset, and
  trigger/release/one-shot behavior. An envelope already in progress retains
  its captured settings if `ct 0 0 1 0` disables future triggers.
- Updated the command reference, architecture notes, CZ-style example, maxed
  generated analysis sources, and browser WASM artifacts. Added regression
  tests for zero identity, distinct positive/negative shapes, bounded output,
  compiled `ct`/`cd`, envelope lifecycle/copy, and base PD without `C`.
- Extended the opt-in `synth_callback_bench` target with `off`, `static`,
  `envelope`, `mod`, and `all` scenarios. On this host, 128-frame callbacks
  with 32 active voices averaged about 2.0-2.3% callback load with PD off,
  4.2-4.3% with static resonant PD, and 4.7-4.8% with envelope plus oscillator
  modulation. At 128 active voices the corresponding figures were 8.0%,
  17.7%, and 20.0%, with no overruns in the 10,000-callback stress pass. PD
  has an expected roughly 2x per-enabled-voice oscillator cost, but no runaway
  behavior; disabled voices skip all PD envelope/modulation math.
- Verified successful native, maxed, and WASM builds; the focused new PD tests
  pass in maxed builds. Native `synth_callback_bench` also builds with PD
  absent and accepts the `off` scenario.
- Known unrelated failures remain: three wavetable-display assertions in both
  native and maxed `skode.command-state`, plus three track-delay assertions in
  maxed. Strict `warn-maxed` stops earlier on existing VFS
  `-Wformat-truncation` diagnostics. One isolated benchmark run recorded a
  scheduler-noise overrun, while repeated runs and the 64/128-voice stress
  passes did not; use average load and repeated runs for comparisons.
- No physical MIDI hardware validation has been performed. No Git history was
  changed. Resume from the existing dirty working tree.
