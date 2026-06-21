# SKRED C API Integration Guide

This guide explains how to use the generated SKRED native distribution from an
external C or C++ application.

SKRED's public host boundary is `api.h`. The main control model is textual:
your application starts the engine, sends Skode command strings, and stops the
engine when finished.

## Distribution Layout

Build or unpack a distribution under `dist/`:

```sh
cd parts
make native-libs     # default feature set
make maxed-libs      # MAXED_KIT_OPTS feature set
make dist-api        # archive the maxed package
```

The installed tree looks like:

```text
dist/skred-0.23.1-maxed/
  bin/mini-skred
  include/skred/api.h
  include/skred/skred-version.h
  lib*/libapi.a
  lib*/libapi.so.0.23.1
  lib/libapi.0.23.1.dylib
```

Use the `native` package for the default feature set, or the `maxed` package
when you need optional features such as sequencing, UDP, Ksynth, recording, and
shared-memory scope publication.

## Minimal Host

```c
#include <stdio.h>
#include <skred/api.h>

int main(void) {
    if (skred_start(128, 16, 0) != 0) {
        fprintf(stderr, "skred_start failed\n");
        return 1;
    }

    skred_logger(1);
    skred_command("v0 w0 f440 a-9 l1");

    puts("Press Enter to stop...");
    (void)getchar();

    skred_command("v0 l0");
    skred_stop();
    return 0;
}
```

`skred_start(frames, voices, udp_port)` initializes the singleton engine. Pass
`0` for `udp_port` to leave the optional UDP server disabled. `frames` is the
requested audio callback size, and `voices` is the requested voice count.

`skred_command()` accepts a mutable `char *` for historical reasons. If your
host stores commands as string literals in C++, pass a writable buffer or cast
only when you know the command string will not be modified by your toolchain
policy.

## Compiling an External Program

The dist targets install both the static `api` library and the shared
`api_shared` library. Use the static archive when you want one self-contained
host binary. Use the shared library when you want to update or distribute SKRED
separately from your host.

For the checked-in Linux-style `dist` layout, static linking looks like:

```sh
cc -I dist/skred-0.23.1-maxed/include \
   my_host.c \
   dist/skred-0.23.1-maxed/lib64/libapi.a \
   -lm -lpthread \
   -o my_host
```

Shared-library linking can use the installed versioned shared object directly:

```sh
cc -I dist/skred-0.23.1-maxed/include \
   my_host.c \
   dist/skred-0.23.1-maxed/lib64/libapi.so.0.23.1 \
   -lm -lpthread \
   -o my_host
```

Run it with the package library directory on the runtime loader path:

```sh
LD_LIBRARY_PATH=dist/skred-0.23.1-maxed/lib64 ./my_host
```

On macOS-style packages, use the `lib` directory present in the package:

```sh
cc -I dist/skred-0.23.1-maxed/include \
   my_host.c \
   dist/skred-0.23.1-maxed/lib/libapi.a \
   -lm -lpthread \
   -o my_host
```

For shared-library builds on macOS, link the installed dylib and configure the
runtime search path with your normal deployment mechanism, such as `rpath` or
`DYLD_LIBRARY_PATH` during local testing.

## Lifecycle

Call the API in this order:

```text
skred_start()
skred_command() zero or more times
skred_stop()
```

The engine is currently a singleton. Multiple independent engines in one
process are not supported without refactoring the internal global state.

`skred_stop()` tears down the audio device, optional UDP server, optional
recorder/scope state, Skode command context, wave tables, and synth state. Do
not call `skred_command()` after `skred_stop()` unless you start the engine
again.

For automated tests or non-audio command validation, set `SKRED_NO_AUDIO=1`
before `skred_start()` to initialize the engine without opening an audio
device.

## Command Logging

Skode commands often report diagnostics through the API log:

```c
skred_logger(1);
skred_command("/s");
const char *log = skred_log();
if (log && log[0]) puts(log);
```

`skred_log()` returns an internal buffer owned by SKRED. Read or copy it before
sending more commands.

## Version and Features

The generated package installs `skred-version.h` beside `api.h` and exposes
runtime helpers:

```c
printf("SKRED %s\n", skred_version());
printf("features: %s\n", skred_features());
```

Feature-gated APIs are always declared in `api.h`, but return failure values
when the feature is not present. For example, recording functions return `-1`
in builds without `RECORD`.

## Audio Device Control

By default, `skred_start()` opens the default playback device and leaves capture
disabled. Hosts can inspect and change device selection through:

```c
skred_audio_refresh();
skred_audio_select(0, -1);  /* playback default */
skred_audio_select(1, -2);  /* capture off */
skred_audio_reconnect();
puts(skred_audio_status());
```

Selection values are slots from the latest refresh. `-1` selects the default
device and `-2` disables capture.

The helper `skred_audio_command()` accepts the same audio-device commands used
by `mini-skred`, and `skred_audio_message()` returns the latest status string.

## Recording and Scope

When built with `RECORD=1`, SKRED can write a 10-channel float WAV: stereo
master plus four stereo stems.

```c
skred_command("v0 r1");                    /* route voice 0 to stem 1 */
skred_record_start("take.wav", 0.0);       /* 0 means no duration cap */
skred_command("v0 l1");
skred_record_stop();
```

When built with `SCOPE=1`, SKRED can publish the same 10-channel bus through
POSIX shared memory:

```c
skred_scope_start("skred-scope", 0xffffffffu, 1.0);
/* render/control... */
skred_scope_stop();
```

The `scope_reader` program built by the maxed configuration is an example
consumer.

## Threading Notes

`skred_command()` runs Skode parsing and immediate command work on the calling
thread. Scheduled commands are compiled on the control thread before they reach
the audio callback.

The audio callback is owned by SKRED's miniaudio device in the current public
API. A host that already owns its audio callback should treat `api.h` as the
high-level integration boundary or fork/extract the lower-level block renderer
from the engine internals.

Avoid calling `skred_stop()` concurrently with `skred_command()` from another
thread. If your host sends commands from multiple threads, serialize command
submission at the host boundary unless you have audited the specific command
paths you need.

## Related References

- `api.h` is the authoritative public C header.
- `SKODE_USER_COMMAND_REFERENCE.md` documents the command strings hosts send.
- `ARCHITECTURE.md` explains the runtime model and real-time boundary.
- `mini-skred.c.kit` is the bundled native host example.
