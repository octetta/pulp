# Quality Review Actions

Recorded June 6, 2026.

## Status Update

Reviewed against the implementation on June 15, 2026.

- **Resolved:** `ks_eval()` result ownership.
- **Resolved:** event queue publication ordering; covered by
  `skqueue_tests.c`.
- **Still open:** deterministic UDP worker shutdown; Ksynth shutdown is
  resolved.
- **Partially resolved:** `ands_free()` now frees the parser object and
  `ands_new()` checks allocation failures, but `skode_t` still has no explicit
  destructor and UDP context cleanup remains unfinished.
- **Partially resolved:** audio device initialization and startup failures now
  propagate from `skred_start()`, but complete unwind coverage for every
  partial-start failure remains follow-up work.

Line references below describe the June 6 snapshot and may have moved.

## Critical

### Fix `ks_eval` result ownership - Resolved

- `ks_eval()` now promotes its result to an owned allocation before resetting
  the evaluation arena.
- Callers release owned results with `k_free()`.
- Bridge tests cover retained array results across subsequent evaluations.

Relevant code:

- `parts/vendor/ksynth/ksynth.c`
- `parts/kse.c:82`

## High

### Correct event queue publication ordering - Resolved

The June 6 implementation published `write_idx` before completing the selected
slot. The current queue uses per-slot readiness state and generation checks, so
the consumer only copies fully published events. A sustained
multi-producer/single-consumer test now covers this behavior.

Relevant code:

- `parts/skqueue.c:87`
- `parts/skqueue.c:130`

### Make worker shutdown deterministic - Partially Resolved

- The Ksynth worker now drains its owned job queue, joins during shutdown,
  and destroys its synchronization resources. Repeated start/stop is covered.
- The UDP worker remains detached and still needs synchronized stop state,
  wakeup, joining, and repeated start/stop coverage.

Relevant code:

- `parts/kse.c:172`
- `parts/udp.c:166`

## Medium

### Complete parser and context cleanup - Partially Resolved

- `ands_free()` now releases internal allocations and the `ands_t` object.
- There is still no matching destructor for parser state owned by a `skode_t`.
- UDP context cleanup is explicitly unfinished.
- Add clear ownership APIs and test cleanup of partially and fully initialized
  contexts.

Relevant code:

- `parts/ands.c:500`
- `parts/skode.c.kit:2548`
- `parts/udp.c:156`

### Handle initialization and allocation failures - Partially Resolved

- `ands_new()` now checks object and buffer allocations.
- Audio device initialization and startup failures now propagate.
- `skred_start()` now returns failure when audio startup fails.
- Complete unwind of every resource created before a later startup failure
  remains to be audited and tested.

Relevant code:

- `parts/ands.c:461`
- `parts/api.c.kit:210`
- `parts/api.c.kit:213`

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

The queue risk described in that assessment has since been addressed. The
remaining highest-risk items are Ksynth result lifetime and detached worker
shutdown.
