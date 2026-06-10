<img src="IMG_3240.jpeg" width="100">

# `PULP`

> Pretty Unweildy Linking Parts

or

> Piled Up Lego Pieces

Nobody likes the pulp...


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
PANMOD # stereo panning modulation
PD     # phase distortion 
SAH    # sample rate distortion 
SEQ    # pattern sequencing 
SMOOTHER # volume change smoother 

UDP    # receive skode on UDP
RECORD # multi track wav recording 
BENCH  # random benchmark measures
```

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
make test   # smoke test
make warn   # -Wall -Wextra -Wpedantic -Werror
```

## Static Analysis

The project authors feature-gated C in `.c.kit` and `.h.kit` files. Generate
the canonical max-feature C source tree for source-only analysis services with:

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
