# Quality Review Actions

Recorded June 6, 2026.

## Status Update

Reviewed against the implementation on June 11, 2026.

- **Still open:** `ks_eval()` result ownership.
- **Resolved:** event queue publication ordering; covered by
  `skqueue_tests.c`.
- **Still open:** deterministic Ksynth and UDP worker shutdown.
- **Partially resolved:** `ands_free()` now frees the parser object and
  `ands_new()` checks allocation failures, but `skode_t` still has no explicit
  destructor and UDP context cleanup remains unfinished.
- **Partially resolved:** audio device initialization and startup failures now
  propagate from `skred_start()`, but complete unwind coverage for every
  partial-start failure remains follow-up work.

Line references below describe the June 6 snapshot and may have moved.

## Critical

### Fix `ks_eval` result ownership - Open

- `ks_eval()` returns a pointer into its arena after resetting that arena.
- `kse.c` retains and later reads this invalid pointer.
- Define an explicit ownership contract for evaluation results, then copy or
  promote the result before resetting the arena.
- Add tests covering scalar and array results across multiple evaluations.

Relevant code:

- `parts/ksynth.c:893`
- `parts/ksynth.c:899`
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

### Make worker shutdown deterministic - Open

- The k-synth and UDP workers are detached and are not joined during shutdown.
- Their running flags are shared between threads without atomic access or a
  mutex.
- Use synchronized stop state, wake blocked workers, join them, and destroy
  thread resources before returning.
- Test repeated start/stop cycles and shutdown while work is pending.

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
