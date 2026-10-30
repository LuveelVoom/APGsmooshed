#!/bin/bash
# Known-answer test of the SIMD routines and the isotropic leaf kernel: builds
# tests/t.cpp and compares its hashes (7 routines x 200000 random inputs) with
# tests/unit_expected.txt, which came from the original x86 asm.
#   - native build (x86: original asm; ARM: intrinsics via sse2neon)
#   - on x86 also the portable intrinsics (-DLIFELIB_PORTABLE)
#   - on an Apple-silicon Mac also the original asm under Rosetta
set -e
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cxx="${CXX:-c++}"
tmp="$(mktemp -d)"; trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/lifelib"; cp -R "$here/../lifelib/avxlife" "$tmp/lifelib/"
rm -rf "$tmp/lifelib/avxlife/lifelogic"; mkdir "$tmp/lifelib/avxlife/lifelogic"
cp "$here"/ma_*.h "$here"/iterators_*.h "$tmp/lifelib/avxlife/lifelogic/"

fail=0
run() {  # name, arch prefix, flags[, compiler]
    if $2 "${4:-$cxx}" -std=c++11 -O2 $3 -w -I "$tmp" "$here/t.cpp" -o "$tmp/$1" && $2 "$tmp/$1" > "$tmp/$1.txt"; then
        if cmp -s "$tmp/$1.txt" "$here/unit_expected.txt"; then echo "unit: PASS $1"; else echo "unit: FAIL $1"; paste "$tmp/$1.txt" "$here/unit_expected.txt"; fail=1; fi
    else
        echo "unit: could not build/run $1"; fail=1
    fi
    # Leaf memo vs uncached kernel, with inputs that hit the memo.
    if $2 "${4:-$cxx}" -std=c++11 -O2 $3 -w -I "$tmp" "$here/memo.cpp" -o "$tmp/$1-memo" && $2 "$tmp/$1-memo" > "$tmp/$1-memo.txt"; then
        echo "unit: PASS $1 memo"
    else
        echo "unit: FAIL $1 memo ($(cat "$tmp/$1-memo.txt" 2>/dev/null) mismatches)"; fail=1
    fi
}
case "$(uname -m)" in
x86_64|amd64)
    run native "" "-march=native"
    run portable "" "-march=native -DLIFELIB_PORTABLE" ;;
*)
    run native "" ""
    if [ "$(uname -s)" = "Darwin" ] && arch -x86_64 /usr/bin/true 2>/dev/null; then
        run x86-asm "arch -x86_64" "-arch x86_64 -march=native" /usr/bin/c++  # system clang targets x86
    fi ;;
esac
exit $fail
