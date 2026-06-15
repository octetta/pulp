# Vendored Sources

This directory contains source snapshots maintained in separate upstream
repositories but compiled directly into PULP.

Each package directory should contain:

- the imported source files
- its upstream license
- an `UPSTREAM.md` recording repository, revision, local changes, and update
  procedure

PULP-specific adapters, queues, commands, and tests should remain outside
`vendor/`. Avoid adding PULP concepts to vendored APIs when a generic
embedding API or a local adapter can provide the boundary.

Current packages:

- `ksynth`: array-oriented DSP language and evaluator
- `uedit`: terminal line editor used by the native Mini-Skred host
