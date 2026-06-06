# Quality Review Actions

Recorded June 6, 2026.

## Critical

### Fix `ks_eval` result ownership

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

### Correct event queue publication ordering

- `queue_put()` advances `write_idx` before the selected slot is fully written.
- A concurrent consumer can observe and copy an incomplete event.
- Introduce per-slot readiness/sequence state or use another correct bounded
  MPSC queue design.
- Add a sustained multi-producer/single-consumer concurrency test.

Relevant code:

- `parts/skqueue.c:87`
- `parts/skqueue.c:130`

### Make worker shutdown deterministic

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

### Complete parser and context cleanup

- `ands_free()` releases internal allocations but does not free the `ands_t`
  object itself.
- There is no matching destructor for parser state owned by a `skode_t`.
- UDP context cleanup is explicitly unfinished.
- Add clear ownership APIs and test cleanup of partially and fully initialized
  contexts.

Relevant code:

- `parts/ands.c:500`
- `parts/skode.c.kit:2548`
- `parts/udp.c:156`

### Handle initialization and allocation failures

- `ands_new()` assumes its object and buffer allocations succeed.
- Audio device initialization and startup return values are ignored.
- `skred_start()` reports success even when initialization fails.
- Add failure checks, unwind partially initialized state, and return meaningful
  errors to callers.

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

## Review Snapshot

- `make test`: passed all 3 existing tests.
- `make warn`: passed with `-Wall -Wextra -Wpedantic -Werror`.
- Sanitizer build: unavailable because the installed ASan/UBSan runtime
  libraries were missing.
- Overall assessment: good experimental/research quality, with moderate-to-high
  runtime risk until the ownership and concurrency issues above are resolved.
