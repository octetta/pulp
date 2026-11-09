# Skred / SKODE Opcode Specification DSL (`.def`)

Release: **0.83.0**  
Engine: **pulp / skred**

This directory contains the modular opcode definitions that power SKODE's unified command dictionary and real-time execution engine.

---

## Architecture Overview

All SKODE commands, aliases, real-time audio voice parameters, and sequencer controls are defined as declarative `.def` files. During the build phase, [`tools/mkopcodes.c`](file:///home/stewartj/book/pulp/tools/mkopcodes.c) parses all `.def` files and generates:

1. [`src/gen/skode_dict_gen.h`](file:///home/stewartj/book/pulp/src/gen/skode_dict_gen.h): Deterministic static dictionary lookup tables, clause signatures, argument predicate matchers, and opcode ID constants.
2. [`src/gen/skred_opcodes_gen.c`](file:///home/stewartj/book/pulp/src/gen/skred_opcodes_gen.c): Real-time function dispatch table (`g_skode_realtime_ops[]`) and immediate clause dispatch wrappers.

```
+------------------+         +--------------------+
| doc/opcodes/**/*.def | ----> | tools/mkopcodes.c  |
+------------------+         +--------------------+
                                      |
                                      v
                 +----------------------------------------+
                 | src/gen/skode_dict_gen.h               |
                 | src/gen/skred_opcodes_gen.c            |
                 +----------------------------------------+
                                      |
             +------------------------+-----------------------+
             v                                                v
+---------------------------+                    +---------------------------+
| Real-Time Execution Path  |                    | Immediate / Parser Path   |
| (zero malloc, zero strcmp)|                    | (REPL / File IO / System) |
| g_skode_realtime_ops[id]  |                    | skode_exec_*()            |
+---------------------------+                    +---------------------------+
```

---

## Directory Organization

Opcodes are organized by functional group:

| Group Folder | Identifier | Description |
|:---|:---|:---|
| `doc/opcodes/osc/` | `osc` | Oscillators, wave selection, phase, tune, detune |
| `doc/opcodes/filter/` | `filter` | Cutoff frequency, resonance, filter modes, wavefolding |
| `doc/opcodes/env/` | `env` | Envelopes (attack, decay, sustain, release, ADSR) |
| `doc/opcodes/mod/` | `mod` | Modulators (LFO, delay, FM, AM, ring mod, hard sync) |
| `doc/opcodes/voice/` | `voice` | Voice assignment, routing, amp, pan, voice state |
| `doc/opcodes/seq/` | `seq` | Sequencer patterns, steps, repeats, tempo, control events |
| `doc/opcodes/sys/` | `sys` | Diagnostics, file system, MIDI setup, audio hardware, help |

---

## DSL Specification (`.def` Format)

A single `.def` file can define one or more **clauses** for a word. A clause begins with `@name` and terminates with `@end`.

### Directive Reference

| Directive | Required | Description | Default If Omitted |
|:---|:---:|:---|:---|
| `@name` | No* | Canonical command name | Inherited from prior clause or filename basename |
| `@alias` | No | Short alias identifier (up to 31 chars) | Inherited from prior clause, or omitted |
| `@group` | No* | Group identifier (`osc`, `filter`, `env`, `mod`, `seq`, `voice`, `sys`) | Inherited from prior clause or directory path |
| `@c_name` | No | Unique C identifier for the generated clause handler | Auto-synthesized: `skode_clause_<name>_c<N>` |
| `@type` | No | Execution model: `realtime` or `immediate` | Defaults to `realtime` |
| `@needs` | No | Declares parser state dependencies (`string`, `data`) | Auto-inferred if code accesses `ands_string` or `ands_data` |
| `@guard` | No | Pre-condition guard expression (e.g. `voice`, `args[0] >= 0`) | None (supports multiple guards per clause) |
| `@ifdef` | No | Conditionally wraps `@code` with `#ifdef XYZ` ... `#endif` | None (unconditional) |
| `@arity` | No | Number of arguments accepted by this clause (0 to 8) | Auto-inferred by counting `@match` parameters |
| `@match` | Yes | Tuple of argument pattern matchers | `()` for arity 0, `(finite, ...)` |
| `@summary` | No | One-line summary for interactive help | Auto-generated: `"<name> (<alias>)"` |
| `@doc` | No | Multi-line documentation for the command reference | Defaults to `@summary` |
| `@code` | Yes | C code block executed when the clause matches | *(auto-appends `return 0;` if omitted)* |
| `@end` | Yes | Marks the end of a clause block | `@end` |

---

## Minimal Concise Syntax (Rapid Authoring)

Thanks to auto-inference, you do **not** need to fill out 10+ boilerplate fields. A new real-time voice parameter can be defined in **5 lines**:

```def
@name   wavefold
@alias  wf
@group  filter
@match  (finite)
@code
    voice_wavefold_drive_set(voice, args[0]);
@end
```

### Multi-Clause Inheritance in a Single File

In a `.def` file with multiple clauses, `@name`, `@alias`, and `@group` are specified **once** at the top. Subsequent clauses only specify `@match` and `@code`:

```def
@name   wavefold
@alias  wf
@group  filter

@match  (finite)
@code
    voice_wavefold_drive_set(voice, args[0]);
@end

@match  (finite, finite)
@code
    voice_wavefold_drive_set(voice, args[0]);
    voice_wavefold_bias_set(voice, args[1]);
@end

@match  (nan)
@code
    voice_wavefold_reset(voice);
@end
```

---

## Execution Types: `realtime` vs `immediate`

### 1. `@type realtime`
- **Audio-Thread Contract**: Strictly audio-thread safe.
- **Zero Allocations**: Zero calls to `malloc()`, `free()`, `realloc()`, or any heap operations.
- **Zero String Lookups**: No string parsing, `strcmp()`, or dictionary searches in the render tick.
- **Dispatch**: Compiled directly to an integer `opcode_id` in `event_program_t`. Executed inside the audio render tick via direct table lookup:
  ```c
  g_skode_realtime_ops[opcode_id](voice, args);
  ```
- **Schedulable**: Can be stored in pattern steps (`y0`, `y1`, ...), queued in the event ring buffer (`~0.5 wf 0.8`), and used inside real-time macros.

### 2. `@type immediate`
- **Parser/Control Plane Contract**: Runs on the control/REPL thread with access to full parser state (`ctx`, `s`).
- **Use Cases**: System configuration, audio hardware reinit (`/a*`), file loading (`w<path`), macro definition, REPL diagnostics (`?v`, `?m`, `?o`).
- **Dispatch**: Executed immediately during line consumption via `skode_exec_*(ctx, s, args, argc)`.
- **Real-Time Rejection**: If an immediate word is encountered while compiling a sequence pattern or deferred macro, `skode_compile_program()` rejects it with `SKODE_COMPILE_IMMEDIATE_ONLY` (# `SKODE_NOT_REALTIME_SAFE`).

---

## Static Safety & Parser Dependencies (`@needs`)

Real-time audio thread opcodes (`synth()`) execute on dedicated high-priority audio callback threads. In this execution context, parser state (`ands_t *s`, `skode_t *ctx`, input tokens, string buffers, and data vectors) does **not** exist; realtime handlers only receive `int voice` and `const float *args`.

To protect audio thread stability and prevent accidental dereferences of parser structures, [`tools/mkopcodes.c`](file:///home/stewartj/book/pulp/tools/mkopcodes.c) enforces compile-time static safety:

1. **Parser State Access Detection**: `mkopcodes` inspects clause code for references to `ands_string`, `ands_data`, `ctx->parse`, and `ctx->string_slot`.
2. **Compile-Time Rejection of Invalid Realtime Handlers**: If a clause is declared `@type realtime` but accesses parser buffers or declares `@needs string` / `@needs data`, the build fails immediately with a fatal diagnostic:
   ```
   mkopcodes: fatal error in doc/opcodes/...:
     Clause 'foo' is marked '@type realtime', but its code references parser string/data/context.
     Real-time audio thread opcodes cannot access parser state.
     Please mark this clause as '@type immediate' or use '@needs string' / '@needs data'.
   ```
3. **Auto-Inference of `@type immediate` and Clause Flags**: If `@type` is not explicitly declared as `realtime`, any clause using parser strings or data is automatically classified as `immediate`, and tagged with `SKODE_CLAUSE_NEEDS_STRING` and/or `SKODE_CLAUSE_NEEDS_DATA` in its `skode_clause_t.flags`.
4. **Introspection Visibility**: `skode_dict_help()` displays `[NEEDS_STRING]` and `[NEEDS_DATA]` alongside `(IMMEDIATE)` and `(RT_SAFE)` flags when inspecting commands.

---

## Clause Guards (`@guard`) & Shadowing Detection

### 1. Pre-Condition Guards (`@guard`)
Clauses can declare one or more `@guard` directives to validate prerequisites before execution:

* **Voice Bounds Guard (`@guard voice`)**:
  Automatically synthesizes voice range validation at the top of the function:
  ```c
  if (voice < 0 || voice >= synth_config.voice_max) return -1;
  ```
  Eliminates repetitive bounds checks across voice-oriented `.def` files.
* **Argument Range Guards (`@guard <expression>`)**:
  Evaluates custom predicates during clause matching and early in the function body:
  ```def
  @match (finite)
  @guard voice
  @guard args[0] >= 0.0f && args[0] <= 1.0f
  @code
      voice_set_amount(voice, args[0]);
  @end
  ```

### 2. Compile-Time Clause Shadowing Warnings
When multiple clauses of the same arity exist on a word, `mkopcodes` validates guard and pattern ordering. If an earlier clause (such as `@match (any)` or `@match (num)`) completely subsumes a subsequent clause (such as `@match (finite)` or `@match (nan)`), `mkopcodes` emits a compile-time warning alerting the author that the latter clause is unreachable dead code.

---

## Pattern Matcher Signatures (`@match`)

Argument patterns enable multi-clause dispatch on the same word based on argument count and numeric value characteristics:

| Matcher Predicate | Matches |
|:---|:---|
| `()` | Empty argument list (arity 0) |
| `finite` | Any valid finite floating point number (`isfinite(x)`) |
| `nan` | IEEE 754 NaN (used as reset sentinel, e.g. `k -`) |
| `inf` | Either positive or negative infinity (`isinf(x)`) |
| `+inf` | Positive infinity |
| `-inf` | Negative infinity |
| `number` | Any floating point value |
| `any` | Wildcard match |

### Multi-Clause Example

```def
@name     wavefold
@alias    wf
@group    filter
@c_name   skode_clause_wavefold_drive
@type     realtime
@arity    1
@match    (finite)
@summary  set voice wavefold drive
@doc
Applies wavefolding drive to the current voice.
Usage: wf <drive>
@code
    voice_wavefold_drive_set(voice, args[0]);
    return 0;
@end

@name     wavefold
@alias    wf
@group    filter
@c_name   skode_clause_wavefold_drive_bias
@type     realtime
@arity    2
@match    (finite, finite)
@summary  set voice wavefold drive and DC bias
@doc
Applies wavefolding drive and asymmetry bias.
Usage: wf <drive>, <bias>
@code
    voice_wavefold_drive_set(voice, args[0]);
    voice_wavefold_bias_set(voice, args[1]);
    return 0;
@end

@name     wavefold
@alias    wf
@group    filter
@c_name   skode_clause_wavefold_reset_nan
@type     realtime
@arity    1
@match    (nan)
@summary  reset wavefold drive to default
@doc
Resets wavefolder drive using NaN sentinel.
Usage: wf -
@code
    voice_wavefold_reset(voice);
    return 0;
@end
```

---

## Generator Pipeline & Verification

To re-generate dispatch structures and run verification tests:

```bash
# 1. Regenerate dictionary & opcode dispatch tables
make gen

# 2. Build library and test executables
make -C parts/build_wasm -j$(nproc)

# 3. Run full automated test suite
ctest --test-dir parts/build_wasm --output-on-failure

# 4. Run dedicated dictionary and scheduler contract suite
./parts/build_wasm/test_dict_scheduler
```
