# Latency Telemetry and Loopback Testing

Skred includes built-in diagnostic tools to measure and troubleshoot audio latency. These tools are completely disabled by default to ensure zero performance overhead, but can be enabled at runtime.

## Enabling Telemetry
To turn on the latency tracking tools, set the `SKRED_TRACE_LATENCY` environment variable to `1` when launching the application:

```bash
SKRED_TRACE_LATENCY=1 ./mini-skred
```

When this environment variable is active, two forms of latency measurement become available: **Command-to-Audio Latency Tracking** and **Hardware Loopback Ping**.

---

## 1. Command-to-Audio Latency Tracking
This measures the internal software latency—the exact amount of time it takes from when a Skode command is parsed to when the synthesizer generates the very first non-zero audio sample for that note.

**How to use it:**
With the environment variable enabled, simply trigger any note using standard Skode commands (e.g. `v0 n69 l1`). 

**How to interpret the results:**
The engine will automatically print a log to `stderr` the moment the envelope opens and audio is generated:
```
# command latency [v0]: 345000 ns (0.345 ms)
```
- **v0**: Indicates which synthesizer voice was triggered.
- **ns**: The time elapsed in nanoseconds.
- **ms**: The time elapsed in milliseconds.

If this number is unusually high (e.g. > 5ms), it indicates that the system's internal command queues, thread synchronization, or audio callback periods are introducing software-side delay.

---

## 2. Hardware Loopback Ping (`/png`)
This tests the full physical round-trip latency of your hardware setup (Microphone Input to Speaker Output). 

**How to use it:**
1. Connect a physical loopback cable from your audio interface's Output to its Input (or use a software loopback like BlackHole/Loopback).
2. Start recording the input using a DAW or audio recording software.
3. Send the `/png` command to the synthesizer.

**What happens:**
The `/png` command entirely bypasses the Skode parsing engine and injects a raw, immediate impulse (`1.0f` maximum amplitude) directly into the next available output audio frame.

**How to interpret the results:**
By analyzing your recorded audio file, you can measure the exact distance (in milliseconds or samples) between when you issued the `/png` command (or an external trigger) and when the spike appeared in the recording. 

This helps you measure the "invisible" latency introduced by:
- USB/Hardware buffers
- OS Audio Drivers (CoreAudio, ALSA, WASAPI)
- `ma_engine` node graph processing (if enabled)
