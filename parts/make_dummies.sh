#!/bin/bash
for i in {1..32}; do
    printf -v num "%02d" $i
    cat << 'INNER' > "waves/esq1/50_esq1_${num}.ks"
N: 4096; X: (!N) % (N*1.0)
s(X * (p 2))
INNER
done

for i in {1..18}; do
    printf -v num "%02d" $i
    cat << 'INNER' > "waves/drums/82_drum_${num}.ks"
N: 4096; X: (!N) % (N*1.0)
s(X * (p 2)) * (1.0 - X)
INNER
done
