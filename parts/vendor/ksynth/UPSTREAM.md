# Ksynth Upstream

Repository: https://github.com/octetta/k-synth

This directory contains a vendored source snapshot of Ksynth. Keep generic
array-language, evaluator, memory-management, and embedding API changes here
suitable for propagation to the upstream repository.

PULP-specific Skode integration remains outside this directory in:

- `skode.c.kit`

## Updating

1. Record the imported upstream commit below.
2. Replace or merge the upstream `ksynth.c` and `ksynth.h`.
3. Reapply any generic changes that have not yet reached upstream.
4. Run the native, maxed, and `ksynth_bridge_tests` test suites.

Imported commit: not yet recorded
