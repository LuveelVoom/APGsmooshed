#!/bin/bash
# Build apgluxe for one rule and symmetry. See README.md for the options.
set -e

# macOS on Apple silicon: build natively even when started from a Rosetta
# shell. (Rule families without a native kernel still use x86, see below.)
if [ "$(sysctl -n sysctl.proc_translated 2>/dev/null)" = "1" ]; then
    echo "Running under Rosetta; re-running natively (arm64)..."
    exec arch -arm64 /bin/bash "$0" "$@"
fi

chmod 755 *.sh

# Ensures 'make' works properly:
rm -f ".depend" | true
rm -f "main.o" | true
make clean

# Ensures compilation will fail unless rule2asm succeeds:
rm -f "includes/params.h" | true

rulearg=`echo "$@" | grep -o "\\-\\-rule [^ ]*" | sed "s/\\-\\-rule\\ //"`
symmarg=`echo "$@" | grep -o "\\-\\-symmetry [^ ]*" | sed "s/\\-\\-symmetry\\ //"`
profilearg=`echo "$@" | grep -o "\\-\\-profile" | sed "s/\\-\\-profile/u/"`
mingwarg=`echo "$@" | grep -o "\\-\\-mingw" | sed "s/\\-\\-mingw/u/"`
gpuarg=`echo "$@" | grep -o "\\-\\-cuda" | sed "s/\\-\\-cuda/u/"`
immarg=`echo "$@" | grep -o "\\-\\-immediate" | sed "s/\\-\\-immediate/u/"`
maxpoparg=`echo "$@" | grep -o "\\-\\-maxpop [^ ]*" | sed "s/\\-\\-maxpop\\ //"`
maxgenarg=`echo "$@" | grep -o "\\-\\-maxgen [^ ]*" | sed "s/\\-\\-maxgen\\ //"`

if [ "${#symmarg}" -ne 0 ]; then
if [ "${symmarg:0:1}" = "G" ]; then
gpuarg="true"
elif [ "${symmarg:0:1}" = "H" ]; then
gpuarg="true"
fi
fi

if [ "${#mingwarg}" -ne 0 ]; then
export USE_MINGW=1
fi

if [ "${#profilearg}" -ne 0 ]; then
if [ "${#gpuarg}" -ne 0 ]; then
printf "\033[31;1mWarning: --cuda and --profile are incompatible; omitting the latter.\033[0m\n"
else
export PROFILE_APGLUXE=1
fi
fi

# Remove rule code generated for a previous rule (mkparams.py regenerates it):
rm -rf "lifelib/avxlife/lifelogic" | true

if [ "${#rulearg}" -eq 0 ]; then
rulearg="b3s23"
echo "Rule unspecified; assuming b3s23."
fi

if [ "${#symmarg}" -eq 0 ]; then
symmarg="C1"
echo "Symmetry unspecified; assuming C1."
fi

# "<sym>_spaceinvaders" (e.g. from apgluxe's automatic rebuild, which passes the
# uploaded symmetry name) means the spaceinvaders build of <sym>; lifelib only
# knows <sym>. Without explicit limits, use the defaults below.
case "$symmarg" in
*_spaceinvaders)
symmarg="${symmarg%_spaceinvaders}"
if [ "${#maxpoparg}" -eq 0 ] && [ "${#maxgenarg}" -eq 0 ]; then
maxpoparg="10000"
maxgenarg="180000"
echo "Symmetry ${symmarg}_spaceinvaders requested; using --maxpop 10000 --maxgen 180000."
fi
;;
esac

if [ "${#maxpoparg}" -eq 0 ] && [ "${#maxgenarg}" -eq 0 ]; then
echo "Maximum population and maximum generation unspecified; assuming no spaceinvaders."
fi

if [ "${#maxpoparg}" -eq 0 ] && [ "${#maxgenarg}" -ne 0 ]; then
maxpoparg="10000"
echo "Maximum population unspecified; assuming 10000."
fi

if [ "${#maxpoparg}" -ne 0 ] && [ "${#maxgenarg}" -eq 0 ]; then
maxgenarg="180000"
echo "Maximum generation unspecified; assuming 180000."
fi

gpuarg2="false"

if [ "${#gpuarg}" -ne 0 ]; then
export USE_GPU=1

gpuarg2="true"
fi

echo "Configuring rule $rulearg; symmetry $symmarg"

if command -v "python3" &>/dev/null; then
    echo "Using $(which python3) to configure lifelib..."
    python3 mkparams.py $rulearg $symmarg $gpuarg2 $maxpoparg $maxgenarg
else
    echo "Using $(which python) to configure lifelib..."
    python mkparams.py $rulearg $symmarg $gpuarg2 $maxpoparg $maxgenarg
fi

# lifelib's kernels are x86 asm, except the isotropic non-totalistic one
# (isoluts.h), which also builds for ARM (via sse2neon). On x86 every rule
# builds natively. On ARM, other rule families (totalistic, LtL, Generations,
# B0, hexagonal, ...) need x86: on macOS they are built for x86_64 and run under
# Rosetta; elsewhere the build stops, since a native binary would hit trapped asm.
makearch=""
builtrule="$( grep '#define RULESTRING ' 'includes/params.h' | grep -o '".*"' | tr -d '"' )"
case "$(uname -m)" in
x86_64|amd64|i386|i686)
    ;;
*)
    if echo "$builtrule" | grep -qE '^b[1-8ceaiknjqrytwz-]*s[0-8ceaiknjqrytwz-]*$' &&
       ! echo "$builtrule" | grep -qE '^b0?1?2?3?4?5?6?7?8?s0?1?2?3?4?5?6?7?8?$'; then
        echo "Isotropic non-totalistic rule; building natively ($(uname -m))."
    elif [ "$(uname -s)" = "Darwin" ]; then
        echo "Rule $builtrule has no native kernel; building for x86_64 (runs under Rosetta)."
        makearch="arch -x86_64"
    else
        printf "\033[31;1mRule %s has no kernel for %s: only isotropic non-totalistic rules\n" "$builtrule" "$(uname -m)"
        printf "build off x86. Use an x86-64 machine for this rule.\033[0m\n"
        exit 1
    fi
    ;;
esac

$makearch make
if [ "${#mingwarg}" -ne 0 ]; then
exit 0
fi

symmarg="$( grep 'SYMMETRY2'   'includes/params.h' | grep -o '".*"' | tr '\n' '"' | sed 's/"//g' )"
rulearg="$( grep 'RULESTRING' 'includes/params.h' | grep -o '".*"' | tr '\n' '"' | sed 's/"//g' )"

if [ "${#immarg}" -ne 0 ]; then
    ./apgluxe --rule $rulearg --symmetry $symmarg "$@"
else
    ./apgluxe --rule $rulearg --symmetry $symmarg
fi

exit 0
