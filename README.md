APGsmooshed
===========

apgsearch (`apgluxe`) for [Catagolue](https://catagolue.hatsya.com/), with
lifelib bundled and a set of changes on top of upstream
[apgmera](https://gitlab.com/apgoucher/apgmera):

- **Faster**, with identical results: a memo in front of the isotropic
  non-totalistic rule kernel and a cheaper object-separation step in the
  classifier. Both are plain C++ and help on any computer.
- **Native on ARM** (Apple silicon, 64-bit ARM Linux) for isotropic
  non-totalistic rules. Upstream lifelib only builds for x86.
- **Spaceinvaders**: optional skipping of explosive soups (from
  jedlimlx/apgspaceinvaders).
- **Tooling**: an auto-restart wrapper for occasional crashes, fixed `--profile`
  builds with clang, and known-answer tests.

What changed and how it was checked is in [CHANGES.md](CHANGES.md). Upstream's
original README is kept as [README.upstream.md](README.upstream.md).

Speed on an Apple M4 Pro, `-p 10`, against the original program (x86 build
under Rosetta):

| rule | speedup |
|---|---|
| `b3-qr4k5cek6cs23-a4er56aek` / D8_1 | 4.3-4.5x |
| `b2-as12` / C1 | 4.3-4.6x |
| `b2e3aeinr4ce5c6n7c8s1c2-i3-a4acen5cen6c78` / C1 | 3.6x |
| `b3678s34678` / C1 (x86) | 2.2-3.3x |
| `b3s23` / C1 (x86) | unchanged |

The x86 code path alone (built for x86_64, measured under Rosetta, no
`--profile`) is 1.6-1.7x, 1.7-1.8x and 1.4-1.5x on the three non-totalistic
rules above, with identical census. Real x86 hardware wasn't available for
testing.


Requirements and platforms
--------------------------

You need a C++11 compiler (clang or GCC), `make`, `python3` and `bash`.

| platform | status |
|---|---|
| macOS, Apple silicon | **tested** (Apple clang 16, GCC 16). Isotropic non-totalistic rules native; other rule families built for x86 and run under Rosetta |
| macOS, Intel | x86 build, which is what upstream does; the x86 code path was tested under Rosetta |
| Linux x86-64 | expected to work like upstream; **untested** |
| Linux 64-bit ARM | isotropic non-totalistic rules only (other families need x86); **untested** |
| Windows | upstream's Cygwin / MSYS2 / WSL routes (see README.upstream.md); **untested** |
| NVIDIA GPU (`--cuda`) | upstream code, unchanged; **untested** |

"Isotropic non-totalistic" means rule strings with Hensel letters, such as
`b3-qr4k5cek6cs23-a4er56aek` or `b2-as12`, without B0. Totalistic rules
(`b3s23`, `b36s23`, ...), Larger than Life, Generations, B0 and hexagonal rules
use lifelib's x86 assembly kernels.

On macOS, install the command-line tools with `xcode-select --install`. On
Debian/Ubuntu: `sudo apt install build-essential python3`.


Quick start
-----------

    # build for a rule and symmetry (once per rule/symmetry)
    ./recompile.sh --rule b3-qr4k5cek6cs23-a4er56aek --symmetry D8_1 --profile

    # check the build (optional; only for rules with stored references)
    tests/kat.sh

    # search, restarting automatically if apgluxe ever crashes
    ./apgluxe-autorestart.sh -k YOURKEY -n 5000000 -p 12

Stop with Ctrl-C: apgluxe finishes and uploads the current haul, then exits, and
the wrapper doesn't restart it.


Building: `./recompile.sh [options]`
------------------------------------

| option | meaning |
|---|---|
| `--rule RULE` | rule to build for, e.g. `b3s23`, `B36/S23`, `b3-qr4k5cek6cs23-a4er56aek` (default `b3s23`) |
| `--symmetry SYM` | `C1`, `D8_1`, `D2_+1`, ... (default `C1`). `SYM_spaceinvaders` turns on spaceinvaders (below) |
| `--profile` | profile-guided build: a short training haul, then a rebuild using its profile. +5..+38% on isotropic non-totalistic rules; leave it off for B3/S23 (about 8% slower) |
| `--maxpop N` | spaceinvaders: skip soups whose population exceeds N |
| `--maxgen N` | spaceinvaders: skip soups that haven't stabilised within N generations |
| `--immediate ...` | start searching when the build finishes; the rest of the line goes to apgluxe, e.g. `--immediate -k KEY -n 5000000 -p 12` |
| `--cuda`, `--mingw` | upstream GPU / Windows cross-compile options (untested here) |

Environment variables:
- `PROFILE_SOUPS=N` sets the `--profile` training haul size (default 20000
  soups); use 2000-5000 for slow or explosive rules.
- `CPP_COMPILER=...` / `C_COMPILER=...` choose the compiler (default `g++` /
  `gcc`, which on macOS are Apple clang).

What it does automatically:
- On x86, every rule builds natively.
- On ARM, isotropic non-totalistic rules build natively. Other rule families are
  built for x86_64 on macOS (and run under Rosetta); elsewhere the build stops
  with a message, because those kernels only exist as x86 assembly.
- On a Mac, it re-runs itself natively when the terminal runs under Rosetta.
- Without `--immediate` it only builds. The last line just checks the
  configuration ("Rule ... is correctly configured") and exits.


Searching: `./apgluxe [options]` or `./apgluxe-autorestart.sh [options]`
----------------------------------------------------------------------

| option | meaning | default |
|---|---|---|
| `-k KEY` | your Catagolue payosha256 key | `#anon` (anonymous) |
| `-n N` | soups per haul (results upload after each haul) | 10000000 |
| `-p N` | worker threads. Use about the number of performance cores plus a few (12-14 on an M4 Pro); `-p 0` runs single-threaded in interactive mode | 0 |
| `-i N` | stop after N hauls | 0 = run forever |
| `-v N` | peer-verify N other people's hauls per haul | 5 if `-p` <= 4, else 0 |
| `-L 1` | also save each haul to `log.<time>.<seed>.txt` | 0 |
| `-t 1` | test mode: one haul, nothing uploaded | off |
| `-s SEED` | fixed seed | random |
| `--rule R --symmetry S` | check that the build matches; if it doesn't, rebuild for R/S and start searching with the other options (add `--profile` to profile that build) | |
| `-u N` | GPU only | |

Running `./apgluxe` with no options asks for soups per haul and a key.

Progress lines appear every 2 seconds (upstream: every 10 seconds, and only
between batches of soups, which for slow rules was much less often).


Auto-restart: `./apgluxe-autorestart.sh [apgluxe options]`
----------------------------------------------------------

Runs apgluxe and restarts it if it crashes (segfault, bus error, illegal
instruction, abort). Only the haul in progress at the crash is lost, and each
start uses a new random seed. Crashes are logged to `apgluxe-crashes.log`. A
normal exit, Ctrl-C, or an error exit (e.g. bad options) is not restarted. Five
crashes in a row, each within 2 minutes of starting, make it give up.
Environment settings: `DELAY` (seconds before a restart, default 5),
`FAST_SECS` (120), `MAX_FAST` (5), `APGLUXE` (binary to run).


Spaceinvaders (explosive rules)
-------------------------------

    ./recompile.sh --rule b2s --symmetry C1 --maxpop 10000 --maxgen 180000

or `--symmetry C1_spaceinvaders`, which uses those default limits. Giving only
one of `--maxpop` / `--maxgen` uses the default for the other.

A soup that exceeds the population limit, or runs past the generation limit, is
skipped: it counts as searched, but none of its objects are recorded. Skipped
soups are counted per haul and shown in the progress lines and the haul summary:

    b2s/C1_spaceinvaders: 20000 soups completed (40567.951 soups/second current, 40567.951 overall), 20000 aperiodic.
    20000 soups completed (20000 aperiodic, skipped).
 Hauls upload as
`<symmetry>_spaceinvaders` (e.g. `C1_spaceinvaders`), separately from normal
hauls. Different limits upload under the same name, so keep them consistent.
The generation limit applies separately to each of the two stabilisation phases
(the fork's behaviour, kept as is).

"Failed to detect periodic behaviour!" (British spelling) without spaceinvaders
is upstream's message for a soup still unsettled after 120000 generations; it is
common in rules with replicators, e.g. B36/S23 with D8_1.


Tests
-----

    tests/kat.sh [threads]   # census of a fixed no-upload haul vs stored results
    tests/unit.sh            # SIMD routines and rule kernel vs stored hashes

`kat.sh` has references for `b3s23`/C1, `b3678s34678`/C1, `b2-as12`/C1,
`b2e3aeinr4ce5c6n7c8s1c2-i3-a4acen5cen6c78`/C1 and
`b3-qr4k5cek6cs23-a4er56aek`/D8_1 when linked against libc++ (clang on
macOS), and for the three non-totalistic ones with libstdc++ (GCC, clang on
Linux). Why that matters is explained under "Known issues". `unit.sh` builds
the routines for the host (on x86 also the portable version, on Apple silicon
also the original asm under Rosetta) and compares them with hashes from the
original asm. `CXX=g++-16 tests/unit.sh` tests another compiler.


Known issues
------------

- **The census depends on the C++ standard library.** lifelib's object
  separation (`full_3dsep` / `get_patterns` in `lifelib/classifier.h`) walks a
  `std::unordered_map` in iteration order and merges islands greedily, so libc++
  and libstdc++ can split a few pseudo-objects differently. With 15000 soups of
  `b2-as12` it changed a handful of p3 objects; `b3-qr4k5cek6cs23-a4er56aek` and
  `b2e3...` were unaffected. This comes from upstream: changing only that map's
  hash under clang reproduces the same difference. It is left as is, because a
  fix would change results everywhere.
- apgluxe occasionally segfaulted (seen once in about 5 hours with the original
  x86 build under Rosetta); the cause wasn't found. Use the restart wrapper.
- The uploaded version string is upstream's (`v5.43-ll2.5.9`).


Credits and licences
--------------------

apgluxe and lifelib (including the bundled cpads) are by Adam P. Goucher, MIT
licence (`LICENCE.txt`, `lifelib/LICENCE.txt`, `lifelib/cpads/LICENCE.txt`).
See README.upstream.md for upstream's credits. The spaceinvaders option is from
jedlimlx/apgspaceinvaders. `lifelib/avxlife/sse2neon.h` is sse2neon v1.8.0
(MIT, see the header).
