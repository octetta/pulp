#!/bin/bash
rm -f waves/esq1/*.ks waves/drums/*.ks
for i in {0..31}; do
    slot=$((50 + i))
    printf -v num "%02d" $((i + 1))
    cat << 'INNER' > "waves/esq1/${slot}_esq1_${num}.ks"
N: 4096; X: (!N) % (N*1.0)
s(X * (p 2))
INNER
done

for i in {0..17}; do
    slot=$((82 + i))
    printf -v num "%02d" $((i + 1))
    cat << 'INNER' > "waves/drums/${slot}_drum_${num}.ks"
N: 4096; X: (!N) % (N*1.0)
s(X * (p 2)) * (1.0 - X)
INNER
done
