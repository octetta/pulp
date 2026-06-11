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
KSYNTH # asynchronous k-synth evaluation
RECORD # multitrack WAV recording
SCOPE  # external shared waveform scope integration
BENCH  # internal benchmark measurements
```

The standard `maxed` target enables the features listed in
`MAXED_KIT_OPTS` in `parts/Makefile`. `XM` is available to custom `KIT_OPTS`
builds but is not enabled by that preset. `SCOPE` also requires the external
`scope-shared.h` integration, which is not included in this repository.

## Native Build

```sh
make native
./build_native/mini-skred
```

To enable features manually with CMake:

```sh
cmake -B build_native -S . -DKIT_OPTS="ADSR=1 PD=1 FILT=1 FADSR=1"
cmake --build build_native
./build_native/mini-skred
```

## Validation

```sh
make test   # build and run the default test suite
make warn   # -Wall -Wextra -Wpedantic -Werror
make warn-maxed # strict canonical maxed-preset build and tests
```

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
