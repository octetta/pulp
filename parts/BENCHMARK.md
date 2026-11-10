# SKRED / PULP Performance & Memory Benchmark Guide

This document describes the SKRED performance benchmarking harness, details the benchmark comparison between **v0.81.1** and **v0.83.1** using `hammer_the_signal.sk`, and establishes a roadmap for tracking CPU, latency, and memory metrics across future releases.

---

## 1. Executive Summary: v0.81.1 vs. v0.83.1

Using [`parts/skred-music-guide/hammer_the_signal.sk`](file:///home/stewartj/book/pulp/parts/skred-music-guide/hammer_the_signal.sk) as the reference composition (~2 minutes, 256 voice allocation, multi-bus delays, polyphony pools, polymeter sequencer, filter envelopes, FM synthesis, and K-Synth drum/riser samples), SKRED achieved major CPU efficiency and latency improvements in **0.83.1** while maintaining a stable memory footprint:

| Metric | **0.81.1** (Baseline) | **0.83.1** (Current) | **Delta vs 0.81.1** | Impact |
| :--- | :--- | :--- | :--- | :--- |
| **Audio Callback Avg Duration** | **0.255 ms** | **0.157 ms** | **-38.4%** | Faster block rendering |
| **Audio Callback Peak Latency** | **0.824 ms** | **0.457 ms** | **-44.5%** | Far greater headroom against underruns |
| **Audio DSP Callback Load** | **8.4 %** | **5.2 %** | **-38.1%** | Substantial drop in DSP budget usage |
| **User CPU Time (15s run)** | **1.71 s** | **1.24 s** | **-27.5%** | ~28% fewer CPU cycles burned |
| **Overall CPU Utilization** | **12.3 %** | **9.0 %** | **-26.8%** | Lower fan noise and power consumption |
| **Peak Memory (Max RSS)** | **94.5 MB** (94,497 KB) | **94.9 MB** (94,888 KB) | **+0.4%** | Constant memory footprint |

> [!TIP]
> **Why did CPU usage drop ~38%?**
> 1. **Voice-Level Block Sparsity (`synth.c`)**: In earlier builds, active SIMD chunks computed synthesis parameters across all voices in a block. The fast path introduced in 0.83.0 checks `if (!voice_active[n]) continue;`, cleanly skipping inactive voices within active blocks.
> 2. **Dense Opcode Dispatch Tables (`src/gen/skred_opcodes_gen.c`)**: Control events and sequencer dispatch now resolve through direct numerical opcode handlers rather than dynamic lookups.

---

## 2. Using the Benchmark Tool

The benchmark harness is located at [`parts/tools/benchmark.py`](file:///home/stewartj/book/pulp/parts/tools/benchmark.py). It runs `mini-skred` headless (`-n`), executes `/ls` on the reference `.sk` file, initiates `play`, allows a warm-up phase, resets the hardware performance counters (`/th!`), samples real-time audio callback statistics via `/s 2`, and collects process-level CPU/memory statistics via `/usr/bin/time -v`.

### Quick Commands

```bash
# Benchmark current build against hammer_the_signal.sk:
make -C parts bench

# Compare current build directly against a Git tag/branch:
make -C parts bench-compare REF=v0.81.1

# Direct invocation with custom duration and output:
python3 parts/tools/benchmark.py \
  --binary "0.81.1:/tmp/pulp-0.81.1/parts/build_native/mini-skred" \
  --binary "0.83.1:parts/build_native/mini-skred" \
  --duration 15000 \
  --runs 3 \
  --markdown \
  --save-json parts/benchmarks/run.json
```

### Options Reference

- `--binary [LABEL:]PATH`: Path to a `mini-skred` executable (can be passed multiple times to compare multiple builds).
- `--compare-git REF`: Checks out `REF` into an isolated temporary worktree, builds it, and compares it side-by-side against the current build.
- `--file PATH`: Path to a `.sk` song/patch file (defaults to `parts/skred-music-guide/hammer_the_signal.sk`).
- `--duration MS`: Duration of the measurement window in milliseconds (default: `10000`).
- `--warmup MS`: Warm-up time before clearing performance counters (default: `1000`).
- `--runs N`: Number of trials per binary to average out system scheduling jitter (default: `3`).
- `--markdown`: Formats summary tables as GitHub Flavored Markdown (ideal for PR comments and release notes).
- `--save-json PATH`: Exports full aggregated metrics and per-run raw trials as JSON.
- `--fail-on-regression PCT`: Exits with code 1 if audio callback latency or memory regresses by more than `PCT%` (ideal for CI gates).

---

## 3. Future Development: Tracking & Preventing Regressions

To ensure SKRED remains ultra-lightweight and performant as new features are added:

### A. CI Regression Gating (GitHub Actions)
Add a workflow step in `.github/workflows/ci.yml` on every pull request:
```yaml
- name: Performance Regression Gate
  run: |
    make -C parts native
    python3 parts/tools/benchmark.py \
      --compare-git origin/main \
      --duration 8000 \
      --runs 2 \
      --fail-on-regression 10.0
```
This fails any PR that increases average callback latency or memory by >10%.

### B. Historical Metric Tracking (`parts/benchmarks/`)
On release tags, save JSON outputs in `parts/benchmarks/<version>.json`. This provides an auditable history of:
- `cb_avg_ms` (audio callback time)
- `load_avg_pct` (engine load)
- `max_rss_kb` (resident memory)
- `user_sec` (CPU time)

### C. Reference Songs Suite
While `hammer_the_signal.sk` is the primary benchmark for heavy multi-voice electronic production, additional reference patches can be added for specialized engine paths:
- `bench_fm_stress.sk`: Extreme FM modulation matrix (operators + feedback).
- `bench_poly_stress.sk`: Maximum polyphony voice pool allocation and voice stealing.
- `bench_vfs_ksynth.sk`: Fast inline K-Synth compilation and WAV streaming.
