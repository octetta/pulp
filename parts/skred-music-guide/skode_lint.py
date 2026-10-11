#!/usr/bin/env python3
"""Cheap pre-flight checks for a .sk file. Usage: python3 skode_lint.py song.sk"""
import re, sys

problems = 0
def warn(n, msg):
    global problems
    problems += 1
    print(f"line {n}: {msg}")

text = open(sys.argv[1]).read()
if text.count("[") != text.count("]"):
    warn(0, "unbalanced square brackets")

for n, raw in enumerate(text.splitlines(), 1):
    ln = raw.rstrip()
    code, _, comment = ln.partition("#")
    if ";" in comment:
        warn(n, "semicolon inside a comment (the parser leaves comment mode)")
    stripped = code.strip()
    if not stripped or stripped.startswith("("):
        continue
    m = re.match(r"\[(.*)\] x\d+$", stripped)
    if m and len(m.group(1).split()) > 30:
        warn(n, "long step (over 30 tokens), may exceed the 32-operation compile limit")

print("OK" if not problems else f"{problems} problem(s)")
