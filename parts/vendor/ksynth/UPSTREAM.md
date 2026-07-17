# Ksynth Upstream

Repository: https://github.com/octetta/k-synth
Imported commit: `307a651038b9ca115340e697352134475e77345b`
Imported: 2026-07-17
Local changes: none; vendored `ksynth.c` and `ksynth.h` match that checkout.

This directory contains a vendored source snapshot of Ksynth. Keep generic
array-language, evaluator, memory-management, and embedding API changes here
suitable for propagation to the upstream repository.

PULP-specific Skode integration remains outside this directory in:

- `skode.c.kit`

## Updating

1. Select and review the upstream commit to import.
2. Replace or merge the upstream `ksynth.c` and `ksynth.h`.
3. Reapply any generic changes that have not yet reached upstream.
4. Run the native, maxed, and `ksynth_bridge_tests` test suites.
