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

## Control-Plane Events

SKRED does not install host callbacks for control-plane events. An embedding
application "subscribes" by polling the event ring exposed in `api.h`.

The control-plane event stream reports things the engine observed while running,
such as voices triggering, releasing, finishing, pattern boundaries, or explicit
user events emitted by scheduled Skode.

This is the stream a host such as ro-totem should use to mirror musical state,
drive visuals, react to pattern markers, or service host-side macros. It is a
notification stream, not the scheduler itself.

### Enabling Event Sources

Voice lifecycle notifications are per-voice opt-in. Voices default to
publication off. Enable a voice with Skode before expecting trigger, release,
or finished notifications from it:

```c
skred_command("v3 vc1");  /* publish control-plane events for voice 3 */
skred_command("v4 vc0");  /* stop publishing for voice 4 */
```

Voice show commands include `vc1` when publication is enabled. Disabled
publication is the default and is omitted from formatted voice output.

Pattern boundary notifications are per-pattern opt-in. Select the pattern and
then enable or disable boundary events:

```c
skred_command("y0 yc1");  /* publish start/end events for pattern 0 */
skred_command("y0 yc0");  /* stop publishing pattern boundary events */
```

Pattern show commands include `yc1` when publication is enabled. Disabled
publication is the default and is omitted from formatted pattern output.

Explicit user events do not require `vc1` or `yc1`. They are emitted only when
Skode executes a `ce id[,a,b,c]` command:

```c
skred_command("ce 100,1,2,3");      /* immediate user event */
skred_command("[ce 200,64] x0");    /* pattern step emits a user event */
skred_command("[ce 300] R 4,.25,9"); /* repeated user events tagged 9 */
```

Because `ce` is schedulable, defers, repeats, patterns, and external macros can
emit host-visible markers without triggering a synth voice.

### Polling

```c
skred_control_event_t events[64];
uint64_t last_dropped = skred_control_event_dropped();

for (;;) {
    int n = skred_control_event_poll(events, 64);
    for (int i = 0; i < n; i++) {
        const skred_control_event_t *e = &events[i];
        switch (e->type) {
        case SKRED_CONTROL_EVENT_VOICE_TRIGGER:
            /* voice e->voice began sounding at absolute sample e->sample */
            break;
        case SKRED_CONTROL_EVENT_VOICE_RELEASE:
            /* voice e->voice entered release */
            break;
        case SKRED_CONTROL_EVENT_VOICE_FINISHED:
            /* voice e->voice crossed back to inactive */
            break;
        case SKRED_CONTROL_EVENT_USER:
            /* e->id and e->value[] came from a Skode "ce" command */
            break;
        case SKRED_CONTROL_EVENT_PATTERN_START:
        case SKRED_CONTROL_EVENT_PATTERN_END:
            /* pattern e->pattern reached a boundary at step e->step */
            break;
        }
    }

    uint64_t dropped = skred_control_event_dropped();
    if (dropped != last_dropped) {
        /* The host did not poll fast enough; rebuild any mirrored state. */
        last_dropped = dropped;
    }

    /* Sleep, wait on your UI loop, or poll once per frame/tick. */
}
```

`skred_control_event_poll()` is nonblocking. It copies up to `max_events` ready
events into the caller's buffer and consumes them. The host should call it from
its application/control loop, UI loop, or another non-audio service thread. Do
not call it from a real-time audio callback you own, and do not do blocking UI,
I/O, allocation-heavy work, or network sends from inside SKRED's audio callback.

`skred_control_event_reset()` clears the ring and sequence counter. Use it when
starting a new host-side subscription session or intentionally discarding old
notifications. `skred_control_event_dropped()` returns a cumulative count of
events that could not be published because the ring was full. The current ring
keeps 1024 outstanding control-plane events.

### Event Contract

The fields in `skred_control_event_t` are intended for host-side correlation:

- `type` is one of the `SKRED_CONTROL_EVENT_*` values.
- `sample` is the absolute SKRED sample counter when the event was emitted.
- `sequence` is a monotonically increasing control-event sequence number.
- `voice` is the affected voice, or `-1` for events that are not voice-specific.
- `pattern` is the pattern source, or `-1` when there is no pattern source.
- `step` is the pattern step source, or `-1` when there is no pattern step
  source.
- `tag` is the optional integer tag attached when the Skode program was queued,
  or `-1` when there is no queued source tag. Tags are also used by Skode
  cancellation commands such as `R! tag`.
- `opcode` identifies the compiled Skode opcode that produced the event when
  that is meaningful. For user events this is `SKODE_OP_CONTROL_EVENT`; for
  pattern boundary events it is `0`.
- `id`, `value_count`, and `value[0..2]` carry the payload from user events
  emitted by Skode `ce id[,a,b,c]`.

Event type meanings:

| Type | Enablement | Important fields | Meaning |
| --- | --- | --- | --- |
| `SKRED_CONTROL_EVENT_VOICE_TRIGGER` | Selected voice has `vc1` | `voice`, `sample`, optional `pattern`, `step`, `tag`, `opcode` | A voice began sounding. |
| `SKRED_CONTROL_EVENT_VOICE_RELEASE` | Selected voice has `vc1` | `voice`, `sample`, optional source fields | A voice entered envelope release. |
| `SKRED_CONTROL_EVENT_VOICE_FINISHED` | Selected voice has `vc1` | `voice`, `sample`, optional source fields | A voice became inactive after a trigger/release lifecycle. |
| `SKRED_CONTROL_EVENT_USER` | Explicit `ce` command | `id`, `value_count`, `value[]`, `voice`, optional source fields | Skode sent a host-defined marker. |
| `SKRED_CONTROL_EVENT_PATTERN_START` | Selected pattern has `yc1` | `pattern`, `step`, `sample` | Pattern playback landed on step `0`. |
| `SKRED_CONTROL_EVENT_PATTERN_END` | Selected pattern has `yc1` | `pattern`, `step`, `sample` | Pattern playback reached a stop marker or the last playable step before wrap. |

For human inspection in `mini-skred`, the matching Skode command is:

```text
?ce
```

Only voices with `vc1` publish lifecycle notifications to `?ce`. Only patterns
with `yc1` publish boundary events to `?ce`. User events are explicit: a command
such as `ce 99,1,2,3` publishes `SKRED_CONTROL_EVENT_USER` with `id == 99` and
three numeric values.

This is separate from scheduled-event queue inspection. The `?q` command and
`skred_scheduled_event_snapshot()` show pending compiled commands that have not
run yet. The `skred_control_event_*` API shows notifications produced by work
that did run.

### ro-totem Integration Checklist

For a host that wants to react to SKRED state:

1. Start SKRED with `skred_start()`.
2. Send `vc1` for each voice whose trigger/release/finished lifecycle matters.
3. Send `yc1` for each pattern whose start/end boundaries matter.
4. Put `ce id[,a,b,c]` in patterns, defers, repeats, or macros for app-specific
   markers. Treat `id` as the host-defined event selector and `value[]` as its
   small numeric payload.
5. Poll `skred_control_event_poll()` from the host control/UI loop. Preserve
   `sequence` ordering, and check `skred_control_event_dropped()` after each
   poll.
6. If drops occur, rebuild any mirrored host state from current SKRED state and
   continue polling.

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

Control-plane events are published by engine code into a bounded ring and
serviced by host polling. Poll often enough for your event rate. If
`skred_control_event_dropped()` increases, treat the host's mirrored state as
lossy and refresh it from commands/status appropriate to your application.

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
