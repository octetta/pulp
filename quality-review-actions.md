# Quality Review Actions

Recorded June 6, 2026.

## Status Update

Reviewed against the implementation on July 17, 2026.

- **Resolved:** `ks_eval()` result ownership.
- **Resolved:** event queue publication ordering; covered by
  `skqueue_tests.c`.
- **Still open:** deterministic UDP worker shutdown. The worker remains
  detached, and `udp_stop()` closes its socket without joining it.
- **Resolved:** `skode_free()` now owns parser, dictionary-vocabulary, and
  optional Ksynth cleanup. UDP client contexts call it before reuse and at
  worker exit.
- **Partially resolved:** audio device initialization and startup failures now
  propagate from `skred_start()`, but complete unwind coverage for every
  partial-start failure remains follow-up work.
- **Known test debt:** the state suite currently has three wavetable-display
  assertions and, in track-enabled builds, delay assertions whose synthetic
  voice fixture does not initialize the newer wave-range fields. Normal
  `w`-selected voices initialize those fields through
  `osc_set_wave_table_index()`.

Line references below describe the June 6 snapshot and may have moved.

## Critical

### Fix `ks_eval` result ownership - Resolved

- `ks_eval()` now promotes its result to an owned allocation before resetting
  the evaluation arena.
- Callers release owned results with `k_free()`.
- Bridge tests cover retained array results across subsequent evaluations.

Relevant code:

- `parts/vendor/ksynth/ksynth.c`
- `parts/tests/ksynth_bridge_tests.c`

## High

### Correct event queue publication ordering - Resolved

The June 6 implementation published `write_idx` before completing the selected
slot. The current queue uses per-slot readiness state and generation checks, so
the consumer only copies fully published events. A sustained
multi-producer/single-consumer test now covers this behavior.

Relevant code:

- `parts/skqueue.c`
- `parts/tests/skqueue_tests.c`

### Make worker shutdown deterministic - Open for UDP

- Ksynth evaluation is synchronous in the current integration and its context
  is released by `skode_free()`.
- The UDP worker remains detached and still needs synchronized stop state,
  wakeup, joining, and repeated start/stop coverage. `udp_running` should also
  use the portable atomic contract rather than a plain cross-thread integer.

Relevant code:

- `parts/skode.c.kit`
- `parts/udp.c`

## Medium

### Complete parser and context cleanup - Resolved

- `ands_free()` releases internal allocations and the `ands_t` object.
- `skode_free()` releases parser state, a context-local dictionary vocabulary,
  retained Ksynth results, and the Ksynth context when present.
- API, loader, control-dispatcher, and UDP client contexts use this destructor.

Relevant code:

- `parts/ands.c`
- `parts/skode.c.kit`
- `parts/udp.c`

### Handle initialization and allocation failures - Partially Resolved

- `ands_new()` now checks object and buffer allocations.
- Audio device initialization and startup failures now propagate.
- `skred_start()` now returns failure when audio startup fails.
- Complete unwind of every resource created before a later startup failure
  remains to be audited and tested.

Relevant code:

- `parts/ands.c`
- `parts/api.c.kit`

## Verification Work

- Keep `make test` and `make warn` passing.
- Add direct tests for k-synth result lifetime and repeated evaluations.
- Add queue concurrency and cancellation tests.
- Add worker lifecycle and restart tests.
- Add allocation and audio startup failure tests.
- Run AddressSanitizer and UndefinedBehaviorSanitizer when their runtime
  libraries are available in the development environment.

## June 6 Review Snapshot

- `make test`: passed all 3 existing tests.
- `make warn`: passed with `-Wall -Wextra -Wpedantic -Werror`.
- Sanitizer build: unavailable because the installed ASan/UBSan runtime
  libraries were missing.
- Overall assessment: good experimental/research quality, with moderate-to-high
  runtime risk until the ownership and concurrency issues above are resolved.

The queue, Ksynth result-lifetime, and parser/context ownership risks described
in that assessment have since been addressed. The main remaining item from
this review is deterministic UDP shutdown, followed by complete unwind tests
for partial `skred_start()` failures.
