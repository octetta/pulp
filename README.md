<img src="IMG_3240.jpeg" width="100">

# `PULP`

> Pretty Unweildy Linking Parts

or

> Piled Up Lego Pieces

Nobody likes the pulp...

## Architecture

See [parts/ARCHITECTURE.md](parts/ARCHITECTURE.md) for the runtime model,
real-time boundaries, generated-source workflow, adoption options, and a guide
to navigating the codebase.

For external C/C++ hosts, see
[parts/API_INTEGRATION.md](parts/API_INTEGRATION.md) for the generated
distribution layout, compile/link examples, lifecycle, logging, feature, audio
device, recording, and scope APIs.

Skode documentation:

- [User command reference](parts/SKODE_USER_COMMAND_REFERENCE.md)
- [Command implementation reference](parts/SKODE_COMMANDS.md)
- [Scheduled opcode model](parts/OPCODES.md)

# Build

Most development happens from the `parts` directory:

```sh
cd parts
```

## Features

```
ADSR   # amplitude envelope
AM     # amplitude modulation
CRUSH  # reduce bit depth
FILT   # multimode analog-ish filter
FADSR  # filter envelope
FM     # frequency modulation
GLISS  # pitch glissando
PANMOD # stereo panning modulation
PD     # phase distortion
SAH    # sample-and-hold distortion
SEQ    # pattern sequencing
SMOOTHER # volume change smoother
XM     # ring modulation

UDP    # receive skode on UDP
KSYNTH # synchronous k-synth evaluation
RECORD # multitrack WAV recording
SCOPE  # shared-memory live audio publication
BENCH  # internal benchmark measurements
```

The standard `maxed` target enables the features listed in
`MAXED_KIT_OPTS` in `parts/Makefile`. `XM` is available to custom `KIT_OPTS`
builds but is not enabled by that preset. `SCOPE` publishes the master and
four stereo stems through a versioned POSIX shared-memory ring for external
visualizers. The current scope transport is available on POSIX systems.

## Native Build

```sh
make native
./build_native/mini-skred
```

Runtime builds default to `Release`; use `make native BUILD_TYPE=Debug` when
an unoptimized debugging build is intentional.

To enable features manually with CMake:

```sh
cmake -B build_native -S . -DCMAKE_BUILD_TYPE=Release \
  -DKIT_OPTS="ADSR=1 PD=1 FILT=1 FADSR=1"
cmake --build build_native
./build_native/mini-skred
```

## Multichannel WAV and Scope

The `maxed` build enables both `RECORD` and `SCOPE`:

```sh
cd parts
make maxed
./build_maxed/mini-skred
```

In the Mini-Skred REPL, route voices to optional stereo stems and start a
ten-channel WAV recording:

```text
v0 r1 w0 f440 a-6 t.01,.2,.7,.3
v1 r2 w1 f660 a-9 t.01,.2,.6,.3
[take.wav]/rg
v0 l1
v1 l1
/r?
/rs
```

`take.wav` contains interleaved 32-bit float audio at 44.1 kHz:

```text
0 master L    1 master R
2 stem 1 L    3 stem 1 R
4 stem 2 L    5 stem 2 R
6 stem 3 L    7 stem 3 R
8 stem 4 L    9 stem 4 R
```

For live scope publication, enter:

```text
/sg
/s?
```

Then run this in another terminal:

```sh
cd parts
./build_maxed/scope_reader skred-scope 2048
```

Stop publication with `/ss`. A named, master-only, 250 ms ring can be started
with `[my-scope]/sg3,.25` and read with
`./build_maxed/scope_reader my-scope 2048`. WAV recording and scope
publication may run at the same time.

## Validation

```sh
make test   # build and run the default test suite
make warn   # -Wall -Wextra -Wpedantic -Werror
make warn-maxed # strict canonical maxed-preset build and tests
```

To measure the synthesis portion of a 128-frame audio callback:

```sh
cmake --build build_native --target synth_callback_bench
./build_native/synth_callback_bench 32 5000
```

The report includes average and worst callback time, the callback deadline,
average deadline load, and the number of measured deadline overruns.

## Static Analysis

The project authors feature-gated C in `.c.kit` and `.h.kit` files. Generate
the canonical `MAXED_KIT_OPTS` C source tree for source-only analysis services
with:

```sh
make analysis-src
```

Commit changes under `parts/analysis-src`. CI runs `make analysis-check` and
fails when those files no longer match the templates. Configure repository
scanners such as CodeVerify to include ordinary source files under `parts/`
and use `parts/analysis-src/` for the generated modules. Build directories
under `parts/build_*` remain disposable and ignored.

## WASM Build

```sh
make wasm
```

## Cross Compile

ARM 32-bit Linux:

```sh
make pi
```

This reuses the native `kit_tool` for code generation and writes Zig cache data under `/tmp`.

If you want to invoke Zig directly with CMake, the equivalent compiler flag looks like:

```sh
cmake -B build_pi -S . \
  -DCMAKE_C_COMPILER="zig;cc;-target;arm-linux-gnueabihf" \
  -DUSE_EXTERNAL_KIT=ON \
  -DKIT_TOOL_PATH="$PWD/build_native/kit_tool"
cmake --build build_pi
```
