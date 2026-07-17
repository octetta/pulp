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
make dist-api-windows # cross-build and zip the Windows maxed package
```

Native packages are installed below `dist/<platform>/`, where the default
platform is derived from the host OS and architecture. The installed tree
looks like:

```text
dist/<platform>/skred-<version>-maxed/
  bin/mini-skred
  include/skred/api.h
  include/skred/polyphony.h
  include/skred/skred_vfs.h
  include/skred/skred-version.h
  lib*/libapi.a
  lib*/libapi.so.<version>
  lib/libapi.<version>.dylib
```

The Linux-hosted Windows cross package is written under
`dist/windows-x86_64/skred-<version>-maxed/` and archived as
`dist/windows-x86_64/skred-<version>-maxed.zip`. It includes:

```text
bin/mini-skred.exe
bin/libapi.dll
include/skred/api.h
include/skred/skred-version.h
lib/libapi.a
lib/libapi.dll.a
```

Use the `native` package for the default feature set, or the `maxed` package
when you need optional features such as sequencing, UDP, MIDI, Ksynth,
recording, and shared-memory scope publication. The Windows maxed package
includes Ksynth, MIDI, recording, and track routing but omits `SCOPE`.

## Published API Releases

When `VERSION` changes on `main`, the `Publish API distributions` GitHub
Actions workflow builds the maxed API for all supported desktop release
platforms. It creates the immutable tag and GitHub Release `v<version>` only
after every platform build and package check succeeds. The workflow can also
be run manually with `workflow_dispatch`.

Each release contains:

```text
skred-<version>-maxed-linux-x86_64.tar.gz
skred-<version>-maxed-macos-universal.tar.gz
skred-<version>-maxed-windows-x86_64.zip
SHA256SUMS
```

The platform suffix is on the release asset rather than the directory inside
the archive. Each archive still expands to `skred-<version>-maxed/`, preserving
the installed package layout used by existing consumers.

Download a fixed version from:

```text
https://github.com/octetta/pulp/releases/download/v<version>/<asset>
```

For example:

```sh
version=0.49.1
curl -fLO \
  "https://github.com/octetta/pulp/releases/download/v${version}/skred-${version}-maxed-linux-x86_64.tar.gz"
curl -fLO \
  "https://github.com/octetta/pulp/releases/download/v${version}/SHA256SUMS"
sha256sum --check --ignore-missing SHA256SUMS
```

Consumers implementing a `latest` mode should resolve the latest published
release through the GitHub Releases API, then use its tag and assets. Reading
`VERSION` directly from `main` can briefly expose a version whose release jobs
are still running.

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
    skred_command("v0 w0 f440 a0 l1");

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

From the repository root, define a Linux x86-64 package location once:

```sh
PKG="dist/linux-x86_64/skred-$(cat VERSION)-maxed"
if [ -f "$PKG/lib64/libapi.a" ]; then
  LIBDIR="$PKG/lib64"
else
  LIBDIR="$PKG/lib"
fi
```

`GNUInstallDirs` may select `lib` or `lib64` for a native Linux package, so
consumers should detect the installed directory as above rather than assuming
one spelling.

Static linking then looks like:

```sh
cc -I "$PKG/include" \
   my_host.c \
   "$LIBDIR/libapi.a" \
   -lasound -ldl -lpthread -lm \
   -o my_host
```

The maxed Linux static archive includes the ALSA MIDI backend, so static hosts
must link ALSA. Using `$(pkg-config --libs alsa)` instead of spelling
`-lasound` is preferred when integrating this into a build system. The shared
library records ALSA as a dynamic dependency.

Shared-library linking can use the installed library directory:

```sh
cc -I "$PKG/include" \
   my_host.c \
   -L "$LIBDIR" -lapi \
   -lm -lpthread \
   -o my_host
```

Run it with the package library directory on the runtime loader path:

```sh
LD_LIBRARY_PATH="$LIBDIR" ./my_host
```

On macOS-style packages, use the `lib` directory present in the package:

```sh
PKG="dist/macos-$(uname -m)/skred-$(cat VERSION)-maxed"
cc -I "$PKG/include" \
   my_host.c \
   "$PKG/lib/libapi.a" \
   -framework CoreMIDI -framework CoreFoundation \
   -lm -lpthread \
   -o my_host
```

For shared-library builds on macOS, link the installed dylib and configure the
runtime search path with your normal deployment mechanism, such as `rpath` or
`DYLD_LIBRARY_PATH` during local testing.

Windows static hosts inherit the maxed archive's system-library requirements:
`ws2_32`, `ole32`, `uuid`, `user32`, `advapi32`, and `winmm`. Import-library
users should deploy `libapi.dll` beside the host executable or through their
normal DLL search-path arrangement.

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

## Runtime Health

`skred_thread_status()` returns a human-readable snapshot of SKRED-owned
service health. It reports audio callback load and overruns, control-event
dispatcher wake/dispatch counters, UDP packet/command counters when UDP is
included, and recorder/scope state when those features are included.

```c
puts(skred_thread_status());
```

The same snapshot is available from Skode:

```text
/th?
```

The output is portable service accounting rather than OS thread CPU
accounting. It is intended for host diagnostics, status panels, and quick
health checks from mini-skred or embedded applications.

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

### Polling and Waitable Notification

`skred_control_event_poll()` remains the only API that copies event data out of
SKRED. Hosts can either call it periodically or use the waitable notification
helpers to sleep until the ring becomes non-empty.

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
        case SKRED_CONTROL_EVENT_MIDI:
            /* e->id is message type; value[] is channel, data1, data2 */
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

For diagnostics, `skred_control_event_snapshot(events, max_events)` copies
ready events without consuming them. Mini-Skred exposes that as `?ce`.
`skred_control_event_clear()` discards outstanding events without resetting the
sequence or dropped counters; Mini-Skred exposes that as `?ce!`.

For event-loop integration, SKRED exposes a coalesced wake signal:

```c
int skred_control_event_wait_fd(void);       /* POSIX select/poll fd */
void *skred_control_event_wait_handle(void); /* Windows HANDLE */
int skred_control_event_wait(int timeout_ms);
```

The wait object means "the control-event ring is probably non-empty; call
`skred_control_event_poll()` now." It does not carry event data and it does not
count events one-for-one. If multiple events arrive before the host drains the
ring, the wait object may wake only once.

On Linux and macOS, use the fd in `select()` or `poll()`:

```c
int ce_fd = skred_control_event_wait_fd();

FD_ZERO(&readfds);
FD_SET(stdin_fd, &readfds);
FD_SET(ce_fd, &readfds);
select(maxfd + 1, &readfds, NULL, NULL, NULL);

if (FD_ISSET(ce_fd, &readfds)) {
    skred_control_event_poll(events, 64);
}
```

On Windows, use the HANDLE with `WaitForSingleObject()` or
`WaitForMultipleObjects()`:

```c
HANDLE handles[2] = {
    GetStdHandle(STD_INPUT_HANDLE),
    skred_control_event_wait_handle()
};

DWORD r = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
if (r == WAIT_OBJECT_0 + 1) {
    skred_control_event_poll(events, 64);
}
```

The convenience `skred_control_event_wait(timeout_ms)` returns `1` when the
host should poll, `0` on timeout, and `-1` when notification support is
unavailable. Use `timeout_ms < 0` to wait forever.

### Built-In Response Bindings

The host may also use SKRED's built-in responder table instead of writing its
own event dispatch loop:

```c
skred_control_response_bind(SKRED_CONTROL_EVENT_USER, 42, "v0 l1");
skred_control_response_set_enabled(1);
/* ... later, before tearing down the host/engine: */
skred_control_dispatch_stop();
```

The equivalent Skode form uses the parser string and numeric event type:

```text
[v0 l1] /ceb 4 42
/cer 1
```

`skred_control_response_set_enabled(1)`, or the Skode command `/cer 1`, starts
SKRED's API-level control dispatcher thread. The dispatcher sleeps on the same
waitable control-event notification described above, drains the ring when it
wakes, and runs matching Skode response commands from a dedicated Skode
context. `/cer 0`, `skred_control_response_set_enabled(0)`, or
`skred_control_dispatch_stop()` stops that thread. `skred_stop()` also stops it
before freeing engine state.

For hosts that want to integrate the same responder table into an existing
service thread, `skred_control_dispatch_pump(max_events)` performs one
nonblocking dispatch pass and returns the number of response commands executed;
it does not require starting the dispatcher thread with `/cer 1`.
`skred_control_response_poll()` remains as a compatibility alias for
`skred_control_dispatch_pump(256)`. Both consume events from the same ring used
by `skred_control_event_poll()`, so a host should choose either custom polling
or built-in dispatch for a given subscription path.

Response bindings dispatch by event type plus a key:

| Event type | Key used by the responder |
| --- | --- |
| `SKRED_CONTROL_EVENT_VOICE_TRIGGER` | `event.voice` |
| `SKRED_CONTROL_EVENT_VOICE_RELEASE` | `event.voice` |
| `SKRED_CONTROL_EVENT_VOICE_FINISHED` | `event.voice` |
| `SKRED_CONTROL_EVENT_USER` | `event.id` |
| `SKRED_CONTROL_EVENT_PATTERN_START` | `event.pattern` |
| `SKRED_CONTROL_EVENT_PATTERN_END` | `event.pattern` |
| `SKRED_CONTROL_EVENT_MIDI` | `event.id` (`skred_midi_message_type_t`) |

Use key `-1` for a wildcard binding. Tags are not response keys. Tags are
source metadata from scheduled or repeated work, useful for correlation and for
cancelling queued work with commands such as `R! tag`.

Before running a matching response command, the dispatcher sets its Skode
context to the event source. For voice events, plain voice-relative commands
such as `VS` and `VL` therefore apply to the voice that emitted the event:

```text
[VS1,7 VL2,6 BC1 l1] /ceb 2 5
/cer 1
```

This binds voice `5` release events to retarget voice `5` to a new sample range
and loop region, then retrigger it. Include an explicit `vN` in the response
command only when the response should affect a different voice.

Longer chains can be built by rebinding the next response from the current
response. The dispatcher snapshots the matching command before running it, so a
`/ceb` executed inside the response affects the next matching event, not the
current one:

```c
skred_control_response_bind(
    SKRED_CONTROL_EVENT_VOICE_RELEASE, 5,
    "[VS22000,32000 VL24000,31000 BC1 l1] /ceb 2 5 "
    "VS12000,22000 VL14000,21000 BC1 l1");
```

On the first release from voice `5`, this command installs the second-step
response, switches voice `5` to `[12000..22000)`, sets its loop to
`[14000..21000)`, and retriggers it. On the next release, the newly installed
response advances voice `5` to `[22000..32000)`.

There is no special fixed step count for this self-rebinding style. A chain for
one voice normally occupies one responder slot because each `/ceb 2 voice`
replaces the previous release binding for that voice. The practical limits are
that the responder table stores 64 bindings total, each stored response command
is limited to 512 bytes, and each step must install the next step before it
needs to fire.

To reset a chain, re-run the initial `/ceb`/`skred_control_response_bind()`
seed command. To stop it, remove the binding with `/ce! 2 voice` or
`skred_control_response_remove(SKRED_CONTROL_EVENT_VOICE_RELEASE, voice)`.
`skred_control_event_reset()` clears queued notifications and sequence numbers,
but it does not clear responder bindings. A final chain step can either remove
its own binding to stop, or rebind the first step to make the chain loop.

In the REPL, use `/cex external,type,key` to bind a command stored in an
external string slot. This avoids nested bracket strings when authoring
multi-step chains interactively.

`skred_control_event_reset()` clears the ring and sequence counter. Use it when
starting a new host-side subscription session or intentionally discarding old
notifications. `skred_control_event_dropped()` returns a cumulative count of
events that could not be published because the ring was full. The current ring
keeps 1024 outstanding control-plane events.

Mini-Skred does not parse or dispatch control responses itself. Commands such
as `/ceb`, `/cer`, `/ce?`, and `/ce!` are ordinary Skode slash commands handled
by the Skode parser, and mini-skred simply submits text with `skred_command()`.

### Foreign C Functions

Hosts can bind up to ten C callbacks and invoke them from Skode with `/ff0`
through `/ff9`. This is intended for application-owned behavior that should
live outside the built-in control-event responder.

```c
static int app_ff(const skred_foreign_call_t *call, void *user) {
    /* call->index is 0..9, call->arg[0..argc-1] are numeric args. */
    /* call->string is the current [string], call->data is current (data). */
    /* voice/pattern/step identify the Skode context that made the call. */
    return 0;
}

skred_foreign_function_bind(0, app_ff, app_state);
skred_command("(10,20) [marker] /ff0 1,2,3");
skred_foreign_function_clear(0);
```

The digit in `/ff0` through `/ff9` selects the callback slot. It is not included
in `call->arg`; in the example above, `argc == 3` and `arg[]` is `1, 2, 3`.
The pointers inside `skred_foreign_call_t` are valid only while the callback is
running; copy anything that must outlive the call. Calling an unbound `/ffN` is
a silent no-op.

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
  cancellation commands such as `R! tag`. Tags are provenance, not dispatcher
  keys for `skred_control_response_bind()`.
- `opcode` identifies the compiled Skode opcode that produced the event when
  that is meaningful. For user events this is `SKODE_OP_CONTROL_EVENT`; for
  pattern boundary events it is `0`.
- `id`, `value_count`, and `value[0..2]` carry the payload from user events
  emitted by Skode `ce id[,a,b,c]`.

Event type meanings:

| Number | Type | Enablement | Important fields | Meaning |
| --- | --- | --- | --- | --- |
| `1` | `SKRED_CONTROL_EVENT_VOICE_TRIGGER` | Selected voice has `vc1` | `voice`, `sample`, optional `pattern`, `step`, `tag`, `opcode` | A voice began sounding. |
| `2` | `SKRED_CONTROL_EVENT_VOICE_RELEASE` | Selected voice has `vc1` | `voice`, `sample`, optional source fields | A voice entered its release lifecycle, including explicit `l0` release and bounded one-shot loop exhaustion. |
| `3` | `SKRED_CONTROL_EVENT_VOICE_FINISHED` | Selected voice has `vc1` | `voice`, `sample`, optional source fields | A voice became inactive after a trigger/release lifecycle. |
| `4` | `SKRED_CONTROL_EVENT_USER` | Explicit `ce` command | `id`, `value_count`, `value[]`, `voice`, optional source fields | Skode sent a host-defined marker. |
| `5` | `SKRED_CONTROL_EVENT_PATTERN_START` | Selected pattern has `yc1` | `pattern`, `step`, `sample` | Pattern playback landed on step `0`. |
| `6` | `SKRED_CONTROL_EVENT_PATTERN_END` | Selected pattern has `yc1` | `pattern`, `step`, `sample` | Pattern playback reached a stop marker or the last playable step before wrap. |
| `7` | `SKRED_CONTROL_EVENT_MIDI` | Open MIDI input and event mask contains the message type | `id` is `skred_midi_message_type_t`; `value[]` is channel, data1, data2 | A MIDI message arrived on the control plane. System messages use channel `-1`; song position uses `value[1]`. |

For human inspection in `mini-skred`, the matching Skode command is:

```text
?ce
```

`?ce` is non-destructive. Use `?ce!` to discard outstanding control events.

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
6. Prefer `skred_control_event_wait_fd()` on Linux/macOS or
   `skred_control_event_wait_handle()` on Windows when integrating with an
   existing event loop.
7. If drops occur, rebuild any mirrored host state from current SKRED state and
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

The canonical immediate Skode commands are `/als`, `/a?`, `/ao selection`, and
`/ai selection`. Numeric selection uses the C API values: `-1` is the default
device and capture-only `-2` is off. Because these are genuine Skode commands,
they also work in `.sk` files and immediate macros.

For host compatibility, `skred_command()` additionally recognizes the older
API-only `/aout default` and `/ain off` spellings before passing other text to
Skode. `/aout` cannot be a Skode atom because it exceeds four characters, and
its textual selector is not part of Skode's numeric grammar. The lower-level
`skred_audio_command()` helper remains available to hosts that need its
handled/error return value; `skred_audio_message()` returns its latest status.
`skred_audio_status()` and `/a?` include a cheap delay summary showing active
delay lines, configured sends, and sends currently eligible to feed a routed
track delay.

## MIDI Device Control

Build with `MIDI=1` to include the vendored minimidio backend. MIDI is not
opened by `skred_start()`: initialize it explicitly with `skred_midi_init()` or
`/mL`. That separation is important in browsers, where Web MIDI permission must
be requested from a user-triggered call. Web MIDI supports enumerated input and
output devices but not virtual ports. MIDI-enabled WASM builds use Asyncify for
the permission request.

The public API enumerates inputs and outputs independently, opens one of each,
creates virtual ports where the platform supports them, and sends raw byte
buffers. The compact runtime commands are recognized at the
`skred_command()` API router and also implemented as genuine immediate Skode
atoms, so they work in loader contexts and immediate named macros:

| Command | Meaning |
| --- | --- |
| `/mL` | Initialize MIDI and list inputs and outputs. |
| `/mi N`, `/mo N` | Open enumerated input or output `N`. |
| `/miV [name]`, `/moV [name]` | Create a virtual input or output; the name is used when this call initializes the MIDI context. |
| `/mic`, `/moc` | Close the active input or output. |
| `/m?` | Show capabilities, open endpoints, and the input event mask. |

Input callbacks only normalize and publish into the existing bounded
control-event ring. They never parse Skode or invoke host callbacks. By
default all representable messages except SysEx and active sensing are enabled.
Use `skred_midi_event_mask_set()` to select message types; bit `N` controls
`skred_midi_message_type_t` value `N`. SysEx input is intentionally omitted
from the fixed-size event payload in this first bridge.

Once an output is open, immediate Skode command `MO status[,data1[,data2]]`
sends one to three raw bytes. `d>MO` converts the current data array to bytes
and sends it as one raw buffer, including SysEx buffers. Both commands reject
fractional values and values outside `0..255`; `d>MO` is bounded at 65,536
bytes. These are immediate control-plane operations, not real-time opcodes.

Incoming note, release, and pitch-bend messages can target physical voices or
poly pools through the fixed MIDI route table. Other fixed-size messages can
expand bounded command templates on the control dispatcher:

```text
/mv 0,0,2
/mp 1,0,12
[v3 K{d2}] /mb 11,.,74
/mR
/mb?
```

The equivalent C APIs are `skred_midi_route_*()` and
`skred_midi_binding_*()`. Routes and bindings are applied after the dispatcher
drains `SKRED_CONTROL_EVENT_MIDI`; backend callbacks only publish fixed-size
events. See the MIDI section of
[SKODE_USER_COMMAND_REFERENCE.md](SKODE_USER_COMMAND_REFERENCE.md) for channel
wildcards, message type numbers, and template fields.

## Asset VFS

`skred_vfs.h` is installed with the public headers. Hosts can mount a disk
directory, a file-backed ZIP, or a copied in-memory ZIP and use the stdio-like
file/directory API:

```c
skred_vfs_mount("assets.zip");
puts(skred_vfs_status());
/* Skode /ls, /ws, /ks, %ls, and %cat now see the mount. */
skred_vfs_unmount();
```

Browser hosts use `skred_vfs_mount_zip_memory()` for uploaded asset bundles.
`skred_vfs_read_real_file()` bypasses an active mount. These calls perform
allocation and file/archive work and belong on the host control thread, never
an audio callback. The full contract is documented in
[exp-vfs/skred_vfs.md](exp-vfs/skred_vfs.md).

## Voice Groups, Pools, and Graphs

Skode can clone a multi-voice prototype into a fixed pool, allocate it with
schedulable `pn`/`pr`/`pb` commands, and use the first pool instance in
monophonic held-note mode. Configuration is available both as `/pg`, `/pp`,
and `/pm` commands and through the `skred_poly_*` functions declared by
`api.h`.

Hosts can request either a human ASCII dependency tree or stable graph text:

```c
puts(skred_voice_graph(0, 0, 0)); /* ASCII */
puts(skred_voice_graph(0, 1, 0)); /* machine line protocol */
```

The complete command/API contract, numeric enums, edge protocol, lifecycle
rules, and examples are in `POLYPHONY.md`.

## Recording and Scope

When built with `RECORD=1`, SKRED can write a 10-channel float WAV: stereo
master plus four stereo stems. Voices routed with `r1`..`r4` contribute to the
matching stem while remaining in the master mix. Each stem also owns a delay
line; `ds amount` feeds the selected voice into the delay for its current
record/scope track, and the wet return is present in both the master and that
stem.

```c
skred_command("v0 r1");                    /* route voice 0 to stem 1 */
skred_record_start("take.wav", 0.0);       /* 0 means no duration cap */
skred_command("v0 l1");
skred_record_stop();
```

When built with `SCOPE=1`, SKRED publishes the same 10-channel bus through
shared memory. `scope-ipc.c` implements POSIX shared memory and a Windows named
file mapping, but the current CMake configuration rejects `SCOPE=1` on Windows;
the supported preset is therefore POSIX-only:

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
