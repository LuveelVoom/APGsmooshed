#!/bin/bash
# Known-answer census test for the currently built ./apgluxe.
# Runs a no-upload test haul (-t 1) with a fixed seed in a temp directory and
# compares the sorted census with tests/kat/<stdlib>/<rule>_<symmetry>_<soups>.txt.
#
# <stdlib> is the C++ standard library apgluxe is linked against (libc++ for
# clang on macOS, libstdc++ for GCC, and for clang on Linux). It matters because
# object separation in lifelib's classifier visits a std::unordered_map in
# iteration order, so a few pseudo-objects can be split differently (see
# CHANGES.md). The libc++ references came from the original program.
#
# Usage: tests/kat.sh [threads]
set -e
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
apg="$here/../apgluxe"
params="$here/../includes/params.h"
[ -x "$apg" ] || { echo "kat: no ./apgluxe; build one with ./recompile.sh first"; exit 2; }
rule=$(grep '#define RULESTRING ' "$params" | grep -o '".*"' | tr -d '"')
sym=$(grep '#define SYMMETRY ' "$params" | grep -o '".*"' | tr -d '"')
sym2=$(grep '#define SYMMETRY2 ' "$params" | grep -o '".*"' | tr -d '"')

stdlib=libc++
if command -v otool >/dev/null 2>&1; then
    otool -L "$apg" 2>/dev/null | grep -q 'libstdc++' && stdlib=libstdc++
elif command -v ldd >/dev/null 2>&1; then
    ldd "$apg" 2>/dev/null | grep -q 'libstdc++' && stdlib=libstdc++
fi

# Reference for the uploaded symmetry name first (e.g. C1_smooshed_stabiliser), else
# the plain one; _smooshed_stabiliser builds have no plain fallback (their census
# differs by design). The soup count must follow the symmetry directly.
ref=$(ls "$here/kat/$stdlib/${rule}_${sym2}_"[0-9]*.txt 2>/dev/null | head -1)
if [ -z "$ref" ] && ! grep -q '#define LEARNED_PERIOD' "$params"; then
    ref=$(ls "$here/kat/$stdlib/${rule}_${sym}_"[0-9]*.txt 2>/dev/null | head -1)
fi
if [ -z "$ref" ]; then
    echo "kat: no $stdlib reference for $rule/$sym2 (have: $(ls "$here/kat/$stdlib" | tr '\n' ' '))"
    exit 2
fi
n=${ref##*_}; n=${n%.txt}

tmp="$(mktemp -d)"; trap 'rm -rf "$tmp"' EXIT
# Incremental totalHash vs full sweep (needs the rule's generated headers).
if [ -n "$(grep '#define UPATTERN apg::upattern<apg::UTile' "$params")" ]; then
    (cd "$here/.." && ${CXX:-g++} -std=c++11 -O2 -march=native -w -pthread -I . tests/hashtest.cpp includes/md5.cpp includes/happyhttp.cpp -o "$tmp/hashtest") &&
        "$tmp/hashtest" | sed 's/^/kat: /' || { echo "kat: FAIL hashtest"; exit 1; }
fi
cp "$apg" "$tmp/"; cd "$tmp"
start=$(date +%s)
./apgluxe -t 1 -v 0 -L 1 -s benchseed -n "$n" -p "${1:-10}" > run.out 2>&1
sed -n '/@CENSUS TABLE/,/@SAMPLE_SOUPIDS/p' log.*.txt | grep -v '^@' | grep . | sort > census.txt
arch=$(file -b apgluxe | grep -oE 'x86[-_]64|arm64|aarch64' | head -1)
desc="$rule/$sym2, $n soups in $(( $(date +%s) - start ))s (${arch:-unknown arch}, $stdlib)"
if cmp -s census.txt "$ref"; then
    echo "kat: PASS $desc"
else
    diff census.txt "$ref" | head; echo "kat: FAIL $desc"; exit 1
fi
