#!/bin/bash
# Build gpuA and check it: GPU vs CPU reference (30 and 500 gens, torus wrap
# included) and CPU reference vs casim (30 gens), on B3/S23, qr4k, a pure
# shift MAP rule (pins orientation) and a random non-isotropic MAP rule.
set -e
cd "$(dirname "$0")"
T=$(mktemp -d)
clang++ -std=c++17 -O2 -fobjc-arc gpuA.mm -framework Metal -framework Foundation -o gpuA
fail=0
for rule in b3s23 b3-qr4k5cek6cs23-a4er56aek shiftNE randmap:7; do
    name=$(python3 -I mktable.py "$rule" "$T/t.txt")
    for k in lut life; do
        [ $k = life ] && [ "$rule" != b3s23 ] && continue
        for wb in 32 64; do
            ./gpuA "$T/t.txt" $k $wb 128 64 30 30 8 "$T/d.txt" | grep -q 'verify.*ok' || { echo "FAIL $k $wb $rule (30)"; fail=1; }
            python3 -I checkdump.py "$T/d.txt" "$name" 30 > /dev/null || { echo "FAIL casim $k $wb $rule"; fail=1; }
            ./gpuA "$T/t.txt" $k $wb 128 64 500 100 8 | grep -q 'verify.*ok' || { echo "FAIL $k $wb $rule (500)"; fail=1; }
        done
    done
done
rm -rf "$T"
[ $fail = 0 ] && echo "all gpuA tests passed"
exit $fail
