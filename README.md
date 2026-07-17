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
MIDI   # MIDI input/output, routing, and control bindings
RECORD # multitrack WAV recording
SCOPE  # shared-memory live audio publication
TRACKS # four stem routes and their track-aligned delay buses without capture
BENCH  # internal benchmark measurements
```

The standard `maxed` target enables the features listed in
`MAXED_KIT_OPTS` in `parts/Makefile`, including `XM` and `MIDI`. `ADSR` is
always appended by CMake. `SCOPE` publishes the master and
four stereo stems through a versioned shared-memory ring for external
visualizers. The transport has POSIX shared-memory and Windows `Local\\`
named-file-mapping implementations, but the current CMake configuration
enables `SCOPE` only on POSIX targets.

`TRACKS` is an internal generation option used when stem routing and delays are
needed without recording or scope capture, notably in WASM. `RECORD` and
`SCOPE` imply it automatically. It is not currently listed separately by
`skred_features()`.

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

### Windows Without NMake

On Windows, the easiest build path is CMake's Ninja generator with Zig's C
compiler. This avoids NMake and does not require Visual Studio Build Tools.
Install `zig`, `cmake`, and `ninja`, make sure they are on `PATH`, then run:

```powershell
cd parts
cmake --preset windows-zig-ninja
cmake --build --preset windows-zig-ninja
.\build_windows_zig\mini-skred.exe
```

For a feature-rich Windows build, use the Windows maxed preset:

```powershell
cmake --preset windows-zig-maxed
cmake --build --preset windows-zig-maxed
.\build_windows_zig_maxed\mini-skred.exe
```

`windows-zig-maxed` enables the portable maxed features, including Ksynth,
MIDI, recording, and track-aligned delays. It intentionally omits `SCOPE=1`;
the current CMake configuration rejects that feature on Windows even though
`scope-ipc.c` contains a named-file-mapping backend.

If Zig reports cache or filesystem permission errors, point its caches at a
writable directory before configuring:

```powershell
$env:ZIG_LOCAL_CACHE_DIR = "$env:TEMP\pulp-zig-local-cache"
$env:ZIG_GLOBAL_CACHE_DIR = "$env:TEMP\pulp-zig-global-cache"
```

If your CMake is too old for presets, the equivalent manual command is:

```powershell
cmake -G Ninja -B build_windows_zig -S . `
  -DCMAKE_TOOLCHAIN_FILE=cmake/zig-cc.cmake `
  -DCMAKE_BUILD_TYPE=Release
cmake --build build_windows_zig
```

### Cross-Build Windows From Linux

Zig can also build the Windows executable from Linux without a Windows SDK.
Because SKRED generates C sources with `kit_tool`, first build a native Linux
`kit_tool`, then run the Windows cross preset:

```sh
cd parts
cmake --preset ninja-release
cmake --build --preset ninja-release --target kit_tool

cmake --preset cross-windows-zig-ninja
cmake --build --preset cross-windows-zig-ninja --target mini-skred
file build_cross_windows_zig/mini-skred.exe
```

For the portable feature-rich cross-build:

```sh
make -C parts cross-windows-zig-maxed
```

The same `SCOPE=1` exclusion applies to the Windows cross maxed preset.

If generated `.kit` outputs look stale, use the refresh target. It removes the
generated C files inside the cross-build directory before rebuilding:

```sh
make -C parts cross-windows-zig-maxed-refresh
```

To keep the Windows executable in a stable repo-local directory, run:

```sh
make -C parts mini-skred-windows
```

This writes `parts/out/windows-x86_64/mini-skred.exe`; `parts/out/` is ignored
by Git.

To build a Windows API distribution from Linux:

```sh
make -C parts dist-api-windows
```

This installs the Windows headers, static library, DLL/import library, and
`mini-skred.exe` under `dist/windows-x86_64/skred-<version>-maxed/`, then
creates `dist/windows-x86_64/skred-<version>-maxed.zip`.

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
v0 r1 w0 f440 a0 t.01,.2,.7,.3
v1 r2 w1 f660 a0 t.01,.2,.6,.3
[take.wav]/rg
v0 l1
v1 l1
/r?
/rs
```

`take.wav` contains interleaved 32-bit float audio at the active engine/device
sample rate (44.1 kHz by default):

```text
0 master L    1 master R
2 stem 1 L    3 stem 1 R
4 stem 2 L    5 stem 2 R
6 stem 3 L    7 stem 3 R
8 stem 4 L    9 stem 4 R
```

Stems `1` through `4` also own track-aligned delay lines. `r1` through `r4`
select the stem and delay identity for a voice; `ds amount` sets how much of
that voice feeds the selected track delay. Delay returns are heard in the
master mix and included in the matching recording/scope stem.

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
