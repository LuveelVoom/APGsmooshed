#!/bin/bash
# Run apgluxe and restart it whenever it dies from a crash signal
# (SIGSEGV, SIGBUS, SIGILL, SIGABRT, SIGFPE).
#
# Usage: ./apgluxe-autorestart.sh <normal apgluxe arguments>
#   e.g. ./apgluxe-autorestart.sh -k KEY -n 5000000 -p 13
#
# - A clean exit (including Ctrl-C / SIGTERM, which apgluxe handles itself by
#   finishing up and exiting 0) or an ordinary error exit stops the loop.
# - Only the haul in progress at the moment of a crash is lost; apgluxe picks a
#   fresh random seed on each start, so no soups are repeated.
# - Crashes are logged to apgluxe-crashes.log next to this script.
# - If it crashes MAX_FAST times in a row within FAST_SECS of starting, the
#   script gives up (something is broken, not just the occasional segfault).
#
# Set APGLUXE=/path/to/binary to run a different binary (default: ./apgluxe
# next to this script).

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
bin="${APGLUXE:-$here/apgluxe}"
log="$here/apgluxe-crashes.log"
FAST_SECS=${FAST_SECS:-120}
MAX_FAST=${MAX_FAST:-5}
DELAY=${DELAY:-5}

stop=0
trap 'stop=1' INT TERM HUP

fast=0
runs=0
while :; do
    runs=$((runs + 1))
    start=$(date +%s)
    "$bin" "$@"
    code=$?
    elapsed=$(( $(date +%s) - start ))

    if [ $stop -eq 1 ]; then
        echo "[autorestart] stopped by user (exit code $code)."
        exit $code
    fi

    case $code in
        132|134|135|136|138|139)   # ILL, ABRT, TRAP, FPE, BUS, SEGV
            sig=$((code - 128))
            msg="$(date '+%Y-%m-%d %H:%M:%S') run $runs died with signal $sig ($(kill -l $sig)) after ${elapsed}s"
            echo "$msg" >> "$log"
            echo
            echo "[autorestart] $msg"
            ;;
        *)
            echo "[autorestart] apgluxe exited with code $code; not restarting."
            exit $code
            ;;
    esac

    if [ $elapsed -lt $FAST_SECS ]; then fast=$((fast + 1)); else fast=0; fi
    if [ $fast -ge $MAX_FAST ]; then
        echo "[autorestart] $fast crashes in a row within ${FAST_SECS}s of starting; giving up." | tee -a "$log"
        exit $code
    fi

    echo "[autorestart] restarting in ${DELAY}s (Ctrl-C to cancel)..."
    sleep "$DELAY" || true
    [ $stop -eq 1 ] && { echo "[autorestart] cancelled."; exit $code; }
done
