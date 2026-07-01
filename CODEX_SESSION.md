# Codex Session

This file is a lightweight handoff note for work that spans Codex sessions,
VS Code windows, and related repositories. Keep it short and update it at
meaningful handoff points, especially before switching between `pulp` and
`k-synth`.

Preferred target path is `.codex/SESSION.md`, but `.codex` is currently
read-only in this workspace, so this root-level file is the active session
note for now.

## Current Goal

Maintain Pulp's integration with the updated k-synth code and keep the wasm
Skred control-plane workflow stable.

## Related Repositories

- `/home/stewartj/book/pulp`
- `/home/stewartj/book/k-synth`
- `/home/stewartj/ro-totem`

## Current State

Pulp has the reconciled vendored k-synth API and a wasm control-plane responder
fix for `/cer 0`.

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
