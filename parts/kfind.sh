#!/bin/bash
if [ -z "$1" ]; then
  s=""
else
  s="$1"
fi
printf "# s <- %s\n" '$'
find . -name "*$s*.ks" | awk -f ktest.awk
