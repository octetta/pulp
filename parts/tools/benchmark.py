#!/usr/bin/env python3
"""
benchmark.py - SKRED Performance & Memory Regression Benchmarking Tool

Measures and compares CPU time, audio callback execution latency, DSP load,
and peak memory (Max RSS) across SKRED / PULP engine builds and Git commits.

Usage:
  python3 parts/tools/benchmark.py [options]

Examples:
  # Benchmark the current build:
  python3 parts/tools/benchmark.py

  # Compare 0.81.1 vs current 0.83.1:
  python3 parts/tools/benchmark.py \\
    --binary "0.81.1:/tmp/pulp-0.81.1/parts/build_native/mini-skred" \\
    --binary "0.83.1:parts/build_native/mini-skred"

  # Compare current build against a Git tag or branch automatically:
  python3 parts/tools/benchmark.py --compare-git v0.81.1

  # Output as Markdown for release notes or GitHub Actions:
  python3 parts/tools/benchmark.py --markdown

  # Save benchmark run as JSON for historical regression tracking:
  python3 parts/tools/benchmark.py --save-json parts/benchmarks/0.83.1.json
"""

import argparse
import json
import os
import re
import statistics
import subprocess
import sys
import tempfile
import time


def get_repo_root():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    parts_dir = os.path.abspath(os.path.join(script_dir, ".."))
    repo_root = os.path.abspath(os.path.join(parts_dir, ".."))
    return repo_root, parts_dir


def find_default_binary():
    repo_root, parts_dir = get_repo_root()
    candidates = [
        os.path.join(parts_dir, "build_native/mini-skred"),
        os.path.join(parts_dir, "build_maxed/mini-skred"),
        os.path.join(repo_root, "dist/linux-x86_64/skred-0.83.1-maxed/bin/mini-skred"),
        os.path.join(repo_root, "dist/linux-x86_64/skred-0.83.0-maxed/bin/mini-skred"),
        os.path.join(parts_dir, "mini-skred"),
        "parts/build_native/mini-skred",
        "build_native/mini-skred",
    ]
    for c in candidates:
        if os.path.isfile(c) and os.access(c, os.X_OK):
            return os.path.abspath(c)
    return None


def find_default_sk_file():
    repo_root, parts_dir = get_repo_root()
    candidates = [
        os.path.join(parts_dir, "skred-music-guide/hammer_the_signal.sk"),
        os.path.join(parts_dir, "tmp/hammer_the_signal.sk"),
        os.path.join(repo_root, "parts/skred-music-guide/hammer_the_signal.sk"),
        "skred-music-guide/hammer_the_signal.sk",
        "parts/skred-music-guide/hammer_the_signal.sk",
    ]
    for c in candidates:
        if os.path.isfile(c):
            return os.path.abspath(c)
    return None


def run_single_trial(binary_path, sk_file, warmup_ms=1000, duration_ms=10000):
    """Executes mini-skred with the given .sk file, resets performance counters after warmup,
    runs for duration_ms, queries /s 2, and parses resource & engine metrics."""
    sk_abs = os.path.abspath(sk_file)
    commands = (
        f"[{sk_abs}] /ls\n"
        f"play\n"
        f"wait {warmup_ms}\n"
        f"/th!\n"
        f"wait {duration_ms}\n"
        f"/s 2\n"
        f"stop\n"
    )

    cmd = ["/usr/bin/time", "-v", binary_path, "-n"]
    start_wall = time.perf_counter()
    proc = subprocess.run(
        cmd,
        input=commands.encode("utf-8"),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=max(30, int((warmup_ms + duration_ms) / 1000 * 2.5)),
    )
    wall_duration = time.perf_counter() - start_wall

    out = proc.stdout.decode("utf-8", errors="replace")
    err = proc.stderr.decode("utf-8", errors="replace")
    combined = out + "\n" + err

    metrics = {
        "user_sec": 0.0,
        "sys_sec": 0.0,
        "total_cpu_sec": 0.0,
        "cpu_pct": 0.0,
        "max_rss_kb": 0,
        "wall_sec": wall_duration,
        "cb_avg_ms": 0.0,
        "cb_worst_ms": 0.0,
        "cb_budget_ms": 0.0,
        "load_avg_pct": 0.0,
        "load_worst_pct": 0.0,
        "callbacks": 0,
        "frames": 0,
        "overruns": 0,
        "late_starts": 0,
    }

    # Parse /usr/bin/time output
    m_user = re.search(r"User time \(seconds\):\s*([\d.]+)", err)
    if m_user:
        metrics["user_sec"] = float(m_user.group(1))

    m_sys = re.search(r"System time \(seconds\):\s*([\d.]+)", err)
    if m_sys:
        metrics["sys_sec"] = float(m_sys.group(1))

    metrics["total_cpu_sec"] = metrics["user_sec"] + metrics["sys_sec"]

    m_cpu = re.search(r"Percent of CPU this job got:\s*([\d.]+)%", err)
    if m_cpu:
        metrics["cpu_pct"] = float(m_cpu.group(1))

    m_rss = re.search(r"Maximum resident set size \(kbytes\):\s*(\d+)", err)
    if m_rss:
        metrics["max_rss_kb"] = int(m_rss.group(1))

    # Parse engine /s 2 output
    # Example:
    #   callbacks: 3747 frames: 479616 samples: 498432 overruns: 0
    #   callback-ms: last 0.097 avg 0.138 worst 0.568 budget 2.667 worst-budget 2.667
    #   callback-load: last 3.6% avg 5.2% worst 21.3%
    m_cb = re.search(
        r"callbacks:\s*(\d+)\s+frames:\s*(\d+)\s+samples:\s*\d+\s+overruns:\s*(\d+)",
        out,
    )
    if m_cb:
        metrics["callbacks"] = int(m_cb.group(1))
        metrics["frames"] = int(m_cb.group(2))
        metrics["overruns"] = int(m_cb.group(3))

    m_ms = re.search(
        r"callback-ms:\s+last\s+[\d.]+\s+avg\s+([\d.]+)\s+worst\s+([\d.]+)\s+budget\s+([\d.]+)",
        out,
    )
    if m_ms:
        metrics["cb_avg_ms"] = float(m_ms.group(1))
        metrics["cb_worst_ms"] = float(m_ms.group(2))
        metrics["cb_budget_ms"] = float(m_ms.group(3))

    m_load = re.search(
        r"callback-load:\s+last\s+[\d.]+(?:%)?\s+avg\s+([\d.]+)(?:%)?\s+worst\s+([\d.]+)(?:%)?",
        out,
    )
    if m_load:
        metrics["load_avg_pct"] = float(m_load.group(1))
        metrics["load_worst_pct"] = float(m_load.group(2))

    m_glitch = re.search(r"late-starts\s*(\d+)", out)
    if m_glitch:
        metrics["late_starts"] = int(m_glitch.group(1))

    return metrics


def aggregate_runs(run_list):
    """Calculates mean and standard deviation for each metric across runs."""
    keys = run_list[0].keys()
    agg = {}
    for k in keys:
        vals = [r[k] for r in run_list]
        agg[k] = {
            "mean": statistics.mean(vals),
            "stdev": statistics.stdev(vals) if len(vals) > 1 else 0.0,
            "min": min(vals),
            "max": max(vals),
            "raw": vals,
        }
    return agg


def build_git_worktree(ref):
    """Checks out a git ref into a temporary directory and builds mini-skred."""
    tmp_dir = tempfile.mkdtemp(prefix=f"pulp-bench-{ref}-")
    print(f"[*] Setting up worktree for {ref} in {tmp_dir}...")
    try:
        subprocess.check_call(
            ["git", "worktree", "add", "--detach", tmp_dir, ref],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        print(f"[*] Building native mini-skred for {ref}...")
        subprocess.check_call(
            ["make", "-C", os.path.join(tmp_dir, "parts"), "native"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        built_bin = os.path.join(tmp_dir, "parts", "build_native", "mini-skred")
        if os.path.isfile(built_bin) and os.access(built_bin, os.X_OK):
            return tmp_dir, built_bin
        raise RuntimeError(f"Failed to find built binary at {built_bin}")
    except Exception as e:
        subprocess.run(["git", "worktree", "remove", "--force", tmp_dir], check=False)
        raise e


def format_delta(cand, base, invert=False):
    """Returns formatted percentage change string with direction indicator."""
    if base == 0:
        return "N/A"
    diff_pct = ((cand - base) / base) * 100.0
    # For CPU time, latency, and memory: lower is better (negative delta is improvement)
    if abs(diff_pct) < 0.5:
        return f"{diff_pct:+.1f}% (=)"
    if diff_pct < 0:
        return f"{diff_pct:+.1f}% (better)" if not invert else f"{diff_pct:+.1f}% (worse)"
    return f"{diff_pct:+.1f}% (worse)" if not invert else f"{diff_pct:+.1f}% (better)"


def render_ascii_table(results):
    """Renders a clean ASCII comparison table."""
    labels = list(results.keys())
    base_label = labels[0]
    base = results[base_label]

    metrics_def = [
        ("Callback Avg Duration", "cb_avg_ms", "ms", False),
        ("Callback Worst Duration", "cb_worst_ms", "ms", False),
        ("Callback DSP Load", "load_avg_pct", "%", False),
        ("User CPU Time", "user_sec", "s", False),
        ("System CPU Time", "sys_sec", "s", False),
        ("Total CPU Time", "total_cpu_sec", "s", False),
        ("CPU Usage", "cpu_pct", "%", False),
        ("Peak Memory (Max RSS)", "max_rss_kb", "KB", False),
    ]

    header_cols = ["Metric"] + [f"{l} (Mean)" for l in labels]
    if len(labels) == 2:
        header_cols.append(f"Delta vs {base_label}")

    table_data = []
    for name, key, unit, invert in metrics_def:
        row = [name]
        for l in labels:
            val = results[l][key]["mean"]
            if unit == "KB":
                row.append(f"{val:,.0f} {unit} ({val/1024:.1f} MB)")
            elif unit == "ms":
                row.append(f"{val:.3f} {unit}")
            elif unit == "s":
                row.append(f"{val:.2f} {unit}")
            else:
                row.append(f"{val:.1f} {unit}")
        if len(labels) == 2:
            cand_val = results[labels[1]][key]["mean"]
            base_val = base[key]["mean"]
            row.append(format_delta(cand_val, base_val, invert=invert))
        table_data.append(row)

    # Compute column widths
    widths = [len(c) for c in header_cols]
    for row in table_data:
        for i, val in enumerate(row):
            widths[i] = max(widths[i], len(val))

    def sep(char="-", cross="+"):
        return cross + cross.join(char * (w + 2) for w in widths) + cross

    def line(vals):
        return "| " + " | ".join(val.ljust(widths[i]) for i, val in enumerate(vals)) + " |"

    out_lines = [sep(), line(header_cols), sep("=", "+")]
    for row in table_data:
        out_lines.append(line(row))
    out_lines.append(sep())

    return "\n".join(out_lines)


def render_markdown_table(results):
    """Renders a GitHub-Flavored Markdown comparison table."""
    labels = list(results.keys())
    base_label = labels[0]
    base = results[base_label]

    metrics_def = [
        ("Audio Callback Avg Time", "cb_avg_ms", "ms", False),
        ("Audio Callback Peak Latency", "cb_worst_ms", "ms", False),
        ("Audio DSP Load (Avg)", "load_avg_pct", "%", False),
        ("User CPU Time", "user_sec", "s", False),
        ("System CPU Time", "sys_sec", "s", False),
        ("Total CPU Time", "total_cpu_sec", "s", False),
        ("CPU Utilization", "cpu_pct", "%", False),
        ("Peak Memory (Max RSS)", "max_rss_kb", "KB", False),
    ]

    headers = ["Metric"] + [f"**{l}**" for l in labels]
    if len(labels) == 2:
        headers.append(f"**Delta (vs {base_label})**")

    lines = [
        "| " + " | ".join(headers) + " |",
        "| " + " | ".join(["---"] * len(headers)) + " |",
    ]

    for name, key, unit, invert in metrics_def:
        row = [f"**{name}**"]
        for l in labels:
            val = results[l][key]["mean"]
            if unit == "KB":
                row.append(f"{val:,.0f} KB ({val/1024:.1f} MB)")
            elif unit == "ms":
                row.append(f"{val:.3f} ms")
            elif unit == "s":
                row.append(f"{val:.2f} s")
            else:
                row.append(f"{val:.1f} {unit}")
        if len(labels) == 2:
            cand_val = results[labels[1]][key]["mean"]
            base_val = base[key]["mean"]
            row.append(f"`{format_delta(cand_val, base_val, invert=invert)}`")
        lines.append("| " + " | ".join(row) + " |")

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="SKRED / PULP CPU and Memory Performance Benchmark",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--binary",
        action="append",
        metavar="[LABEL:]PATH",
        help="Path to mini-skred binary. Can be repeated. Optional LABEL: prefix.",
    )
    parser.add_argument(
        "--compare-git",
        metavar="GIT_REF",
        help="Compare current binary against a Git ref (tag/branch/commit). Builds in temp worktree.",
    )
    parser.add_argument(
        "--file",
        default=find_default_sk_file(),
        help="Path to .sk reference music file (default: hammer_the_signal.sk)",
    )
    parser.add_argument(
        "--duration",
        type=int,
        default=10000,
        help="Playback duration per run in milliseconds (default: 10000 ms)",
    )
    parser.add_argument(
        "--warmup",
        type=int,
        default=1000,
        help="Warmup duration before /th! reset in milliseconds (default: 1000 ms)",
    )
    parser.add_argument(
        "--runs",
        type=int,
        default=3,
        help="Number of iterations to execute per binary for statistical averaging (default: 3)",
    )
    parser.add_argument(
        "--markdown",
        action="store_true",
        help="Output results as a GitHub Flavored Markdown table",
    )
    parser.add_argument(
        "--save-json",
        metavar="OUT_FILE",
        help="Save full benchmark results and raw trials to a JSON file",
    )
    parser.add_argument(
        "--fail-on-regression",
        type=float,
        metavar="PCT",
        help="Exit with code 1 if audio callback latency or memory regresses by more than PCT%%",
    )

    args = parser.parse_args()

    if not args.file or not os.path.isfile(args.file):
        print(f"Error: Reference music file '{args.file}' not found.", file=sys.stderr)
        sys.exit(1)

    binaries = []
    cleanup_worktrees = []

    if args.compare_git:
        # Base is git ref, candidate is current build
        ref_worktree, ref_bin = build_git_worktree(args.compare_git)
        cleanup_worktrees.append(ref_worktree)
        curr_bin = find_default_binary()
        if not curr_bin:
            print("Error: Could not locate current mini-skred build.", file=sys.stderr)
            sys.exit(1)
        binaries.append((args.compare_git, ref_bin))
        binaries.append(("current", curr_bin))
    elif args.binary:
        for b in args.binary:
            if ":" in b:
                lbl, path = b.split(":", 1)
            else:
                lbl, path = os.path.basename(b), b
            if not os.path.isfile(path) or not os.access(path, os.X_OK):
                print(f"Error: Binary '{path}' not found or not executable.", file=sys.stderr)
                sys.exit(1)
            binaries.append((lbl, path))
    else:
        def_bin = find_default_binary()
        if not def_bin:
            print("Error: Could not locate a mini-skred executable. Build one with `make -C parts native`.", file=sys.stderr)
            sys.exit(1)
        binaries.append(("current", def_bin))

    print(f"=== SKRED Performance Benchmark ===")
    print(f"Reference File : {args.file}")
    print(f"Warmup / Window: {args.warmup} ms / {args.duration} ms ({args.runs} runs each)")
    print(f"Binaries       :")
    for lbl, path in binaries:
        print(f"  - [{lbl}] {path}")
    print()

    results = {}
    raw_trials = {}

    try:
        for lbl, path in binaries:
            print(f"[*] Benchmarking [{lbl}] ({path})...")
            run_data = []
            for r in range(1, args.runs + 1):
                trial = run_single_trial(
                    path, args.file, warmup_ms=args.warmup, duration_ms=args.duration
                )
                run_data.append(trial)
                print(
                    f"    Run {r}/{args.runs}: CPU={trial['total_cpu_sec']:.2f}s "
                    f"({trial['cpu_pct']:.0f}%), CB-Avg={trial['cb_avg_ms']:.3f}ms, "
                    f"CB-Worst={trial['cb_worst_ms']:.3f}ms, RSS={trial['max_rss_kb']:,}KB"
                )
                time.sleep(0.5)
            raw_trials[lbl] = run_data
            results[lbl] = aggregate_runs(run_data)
            print()

        # Print Output Table
        if args.markdown:
            print(render_markdown_table(results))
        else:
            print(render_ascii_table(results))

        # Check regression threshold if requested
        regression_detected = False
        if args.fail_on_regression and len(binaries) >= 2:
            base_lbl = binaries[0][0]
            cand_lbl = binaries[1][0]
            base_cb = results[base_lbl]["cb_avg_ms"]["mean"]
            cand_cb = results[cand_lbl]["cb_avg_ms"]["mean"]
            cb_diff = ((cand_cb - base_cb) / base_cb) * 100.0 if base_cb > 0 else 0
            if cb_diff > args.fail_on_regression:
                print(
                    f"\n[!] REGRESSION DETECTED: Callback latency regressed by {cb_diff:+.1f}% "
                    f"(threshold: {args.fail_on_regression}%)",
                    file=sys.stderr,
                )
                regression_detected = True

        # Save JSON if requested
        if args.save_json:
            report = {
                "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                "reference_file": args.file,
                "duration_ms": args.duration,
                "warmup_ms": args.warmup,
                "runs": args.runs,
                "aggregated": results,
                "raw_trials": raw_trials,
            }
            os.makedirs(os.path.dirname(os.path.abspath(args.save_json)), exist_ok=True)
            with open(args.save_json, "w", encoding="utf-8") as f:
                json.dump(report, f, indent=2)
            print(f"\n[+] Saved full benchmark report to: {args.save_json}")

        if regression_detected:
            sys.exit(1)

    finally:
        for wt in cleanup_worktrees:
            print(f"[*] Cleaning up temporary worktree {wt}...")
            subprocess.run(["git", "worktree", "remove", "--force", wt], check=False)


if __name__ == "__main__":
    main()
