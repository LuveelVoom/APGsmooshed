HANDOFF: open ideas for APGsmooshed
=================================

State as of the commit that added this file. What is already done, with
measurements and checks, is in CHANGES.md; read it first, including "Tried and
rejected". This file lists work that was investigated but not done, with the
numbers behind it, so it can be picked up without redoing the analysis.

Priorities of the owner: isotropic non-totalistic (INT) rules first; B3/S23 and
hexagonal rules don't matter. Exhaustive results must stay exact: every soup
must be censused exactly as before, because Catagolue peer verification
re-censuses soup IDs from other people's hauls. Anything that can change a
census goes behind an option labelled unsafe, off by default.


How changes were verified (keep doing this)
--------------------------------------------

- `tests/kat.sh [threads]`: fixed-seed census vs stored references (libstdc++
  references for the three INT rules; libc++ references for those plus b3s23
  and b3678s34678, from the original program). Run at 1 thread as well as more;
  the original program is the oracle, single-threaded.
- `tests/unit.sh`: SIMD routines and INT kernels vs hashes from the original
  asm; leaf memos (1- and 2-generation) vs uncached kernels and a scalar lut9
  reference; native and `-DLIFELIB_PORTABLE` builds.
- `kat.sh` also builds `tests/hashtest.cpp` (incremental totalHash vs full
  sweep).
- Speed: alternate A and B runs of the same fixed-seed haul
  (`apgluxe -t 1 -v 0 -s benchseed -n N -p P`), at least 3 rounds each; report
  ranges, thread count and machine. Confirm identical census first.
- Profiling: callgrind works if the build uses `-march=x86-64-v3`
  (valgrind can't decode the AVX-512 that `-march=native` emits on AVX-512
  hosts). perf wasn't available in the cloud sandbox.
- Mutation-test new checks: break the code on purpose once and confirm the
  check fails.
- The adaptive step size was written in a Linux sandbox, then checked on macOS
  against the libc++ references (CHANGES.md section 2). When changing it, force
  its switch path by defining `APG_TWOGEN_MIN_HIT` as 101.


1. Native ARM build of the Life-like (totalistic) path
------------------------------------------------------

Why: on Apple silicon, totalistic rules (`b36s23`, `b36s245`, ...) are built for
x86_64 and run under Rosetta with lifelib's plain SSE2 kernel. Going native for
the INT path gave +65% on an M4 Pro (CHANGES.md section 1: 2941 -> 4843 soups/s,
qr4k D8_1, before later changes). A similar gain is plausible here
[inferred; not measured].

What it takes:
- `lifelib/genera/_iwriter.py` (class `iwriter`, used by `genera/lifelike.py`)
  generates the kernels as inline-asm text (`ll_<iset>_<rule>.asm` included
  into `asm(...)` blocks in `iterators_<rule>.h`). Add a variant that emits the
  SSE2 sequence as intrinsics (`_mm_xor_si128`, `_mm_srli_epi32`, ...), one
  intrinsic per instruction, like `lifelib/avxlife/portable_sse.h` did for the
  INT kernel. On ARM, `sse2neon.h` (already vendored) maps them to NEON.
  Select it with `LIFELIB_PORTABLE`, as for the INT path.
- `lifelib/classic/vtile28.h`: 4 asm blocks, currently `__builtin_trap()` under
  `LIFELIB_PORTABLE`. Port them to intrinsics.
- Check `lifelib/avxlife/lifeperm.h`, `leaf_iterators.h` and the incubator for
  other asm reached on this path (`grep -n 'asm *(' lifelib/**/*.h`).
- `recompile.sh`: build totalistic rules natively on ARM (today only the INT
  genus is native, everything else is `arch -x86_64`).
- `best_instruction_set()` reports level 8 (SSE4.2) on ARM, so dispatch takes
  the SSE paths; keep it that way.

Verification without a Mac: on x86, build with `-DLIFELIB_PORTABLE` (the
portable intrinsics, compiled for x86) and compare with the asm build:
unit-level hashes of the generated kernel functions on random inputs, then
census on b3s23/C1 and b3678s34678/C1 (kat has libc++ references; on Linux use
the libstdc++ census of the asm build as reference). Then on the Mac: kat
with libc++.

Effort: a few hours [inferred].


2. AVX-512 kernel for all Life-like rules (x86 with AVX-512 only)
-----------------------------------------------------------------

Today only b3s23 gets AVX-512 (`genera/b3s23life.py`, class `b3s23writer`,
uses `vpternlog` and `vpermi2d`). Other Life-like rules use the AVX2 kernel
from `genera/_iwriter.py` even on AVX-512 machines.

Measured on b3s23, 100k soups, Xeon @ 2.8 GHz, -p 2, one run each:
- AVX2 kernel, VTile28: 13005 soups/s
- AVX-512 kernel, VTile28 (LONG_TILES disabled): 15062 (+16%), census
  identical to the AVX2 build
- AVX-512 kernel + VTile44 + incubator<56,88> (upstream's b3s23 setup): 18601
  (+43%), but census differs from the VTile28 builds; the original program
  behaves the same, so it comes from the tile size. Don't use VTile44 for other
  rules unless census changes are acceptable.

What it takes: an AVX-512 variant of `iwriter` (load/shift/adders/logic) for
general Life-like rules, emitted under `#ifdef __AVX512F__` together with an
`iterate_var_48_28`-style entry point or just the 32_28 functions; the
generic rule logic (`rule2gates`, `genlogic`) can map to `vpternlog`. Wire it
for VTile28 only. Verify: census vs the AVX2 build on b36s23, b36s245,
b3678s34678 (C1 and D8_1), single-threaded.

Gain only on AVX-512 x86 (not Apple silicon, not Intel laptops before Ice
Lake). Effort: a few hours [inferred].


3. GPU search for INT rules, with a per-rule filter generated on the fly
------------------------------------------------------------------------

How the existing GPU path works (lifelib/cuda, read from code, not run):
- It never makes a census. Per epoch it runs 1M soups in 64x64 tiles (one
  thread per row, a row is a uint64), discards "boring" soups and returns the
  "interesting" ones; the CPU censuses only those (`cuda/gs_impl.h`,
  `includes/searching.h`). Hauls are uploaded under GPU symmetries (`G1`,
  `H2_+1`, ...; `mkparams.py` renames C->G, D->H). Only C1, D2_*, D4_* are
  supported (`cuda/copyhashes.h`); not D8_1.
- Boring: every tile's 52x52 region at generation t+6 equals generation t,
  for the whole universe (up to 127 hexagonally arranged tiles, roughly +-300
  cells), within 24000 generations (`schedule`, `combine` in
  `cuda/gpupattern.h`). Interesting: activity reaches the universe edge, or
  time runs out.
- Escaping Life gliders and *WSS are recognised by fixed shifts and population
  (<= 18) in boundary tiles and deleted (`gpupattern.h` ~395-440).
- The rule enters only through the macro `ADVANCE_TILE_64` (one generation of a
  64x64 torus), written by `genera/lifelike.py` and `b3s23life.py`;
  `genera/isotropic.py` doesn't write it, so `--cuda` with an INT rule fails.
  `cuda2/` is b3s23-only.
- `README.md` lists the CUDA path as untested in this fork.

Measured: what fraction of CPU time a filter would remove (upper bound for the
speedup, GPU treated as free). Method: per-soup census and CPU time logged by an
instrumented `censusSoup` (single thread; 20000 soups qr4k, 20000 b2-as12, 8000
b2e, all C1). A soup is discarded if every object in its census is allowed. Rule
settings learned from the first 2000 soups ("common" = in >= 0.5% of soups);
scores on the rest. Script: `tools/gpufilter.py` (see below).

| filter | qr4k C1 | b2-as12 C1 | b2e C1 |
|---|---|---|---|
| as is: still lifes + periods dividing 6, no ship removal | 1.0x | 1.2x | 1.0x |
| learned period L (lcm of common periods), no ship removal | 1.1x (L=42) | 1.4x (L=42) | 1.0x (L=4) |
| learned L + delete the rule's common ships | 47x | 5.8x | 16x |
| strict: discard only if every object is common | 12.5x | 3.1x | 7.3x |

Common ships: qr4k xq2_4ra, xq31_2mhe, xq4_153, xq4_27; b2-as12 xq6_15,
xq6_30d; b2e xq14_jucz1, xq16_25d68c, xq20_29d2, xq2_57, xq34_1f9c, xq4_147,
xq4_1b8c. Most frequent blockers after that: qr4k xq15_6jis, xp27_8sanas8zw131;
b2-as12 xp236_22xb1b, xp26_*; b2e xp12_*, xq48_35529f.

Conclusions:
- Ship deletion is the essential part; a period setting alone is worthless.
- The learned-L+ships filter discards soups whose only "rare" objects are
  still lifes or oscillators with period dividing L, so those never reach the
  census (the Life GPU search has the same property). The strict row is the
  bound if that loss is unacceptable.
- Bounds ignore GPU cost. An INT kernel on the GPU is slower than Life's
  adder network: a 512-entry LUT per cell (simple; maybe 10-20x slower
  [inferred]) or a generated bitsliced circuit per rule (fast; needs a
  generator).

Design for the per-rule filter (generated at startup or by `recompile.sh`):
1. Warm-up: census ~2000 soups on the CPU (as above).
2. Period: L = lcm of common periods; tiles compare generation t+L with t
   instead of t+6. Cap L (qr4k gives 42; if lcm gets large, drop rare periods).
3. Ships: decode each common xq apgcode with lifelib, run one period to get
   (period p, displacement dx, dy) and its phases. Generalise the Life
   deletion: in boundary tiles, compare generation t+p with t shifted by
   (dx, dy) for each (p, dx, dy) class, delete matching small components moving
   outward. Alternative without ship tables: an absorbing boundary at radius R
   that deletes everything crossing it, with a mass limit (deleting more than a
   few ship masses -> interesting), so puffers/rakes/guns still get forwarded.
   Check that idea against yl (linear growth) soups: they must be forwarded.
4. Emit the settings into the generated GPU header next to `ADVANCE_TILE_64`.

Oracle for testing without a GPU: a CPU emulation of the filter (per-soup
discard decision) built from the same settings; the GPU build must make the
same decisions on the same soups. A census can't be the oracle (GPU hauls
are a different symmetry).

Apple silicon (Metal): the CUDA kernels (~1200 lines: gpupattern.h,
copyhashes.h, cumsum.h, cusha256.h) use shared memory, barriers, popcount and
64-bit integers, all available in Metal; the ~90 host-side CUDA API calls
(cudaMalloc/memcpy/launches) need rewriting with Metal (metal-cpp or
Objective-C++). Unified memory removes most copies. Apple GPUs have 32-bit
ALUs, so 64-bit ops may cost more [unsure; not measured]. Do the INT filter
first (on CUDA or as a CPU emulation): without it, a Metal port only helps
Life-like rules. Intel Iris Plus iGPUs (2019 MacBook Pro) are much weaker and
not a target. Effort: days [inferred].

Filter scoring script (as used for the table): `tools/gpufilter.py LOG...`.
- Instrument `SoupSearcher::censusSoup(seedroot, suffix, cfier)` in
  `includes/searcher.h`: copy `census` before, time the call, then print
  `SOUP <suffix> <microseconds> <code>:<count> ...` for codes whose count rose.
- Run `apgluxe -t 1 -v 0 -s gpufilter -n N -p 1 2> log`.
- Classify: `xs*` = period 1, `xp<p>_*` = period p, `xq<p>_*` = ship,
  anything else (yl, zz, ...) forwards.


### More GPU measurements (larger logs, more rules)

Settings learned from warm-up soups; L capped (periods added by frequency
while lcm <= cap); ships = those in >= 0.1% of warm-up soups; test = last 40%
of each log; bound = CPU time of all test soups / CPU time of forwarded ones
(GPU treated as free). Script used: like `tools/gpufilter.py`, with a cap.

| rule (C1 unless noted) | soups | common ships | L (cap 60) | bound cap 12 | cap 60 | cap 240 |
|---|---|---|---|---|---|---|
| b3-qr4k5cek6cs23-a4er56aek | 100k | 5 | 42 | 2x | 234x | 234x |
| b2-as12 | 100k | 2 | 42 | 2x | 6x | 6x |
| b2e3aeinr4ce5c6n7c8s1c2-i3-a4acen5cen6c78 | 40k | 9 | 24 | 31x | 38x | 438x |
| b3s23-q4i / D4_+4 | 100k | 1 | 12 | 1188x | 1188x | 1188x |
| b35ys23 | 20k | 3 | 30 | 238x | 721x | 721x |
| b2n3-q5y6cs23-k | 15k | 4 | 60 | 4x | 5x | 13x (L=204) |
| b2ci3ai4c8s02ae3eijkq4iz5ar6i7e | 15k | 2 | 2 | 2x | 2x | 2x |
| same, without soup 1359 | 15k | 2 | 2 | 50x | 50x | 50x |

Findings:
- More warm-up soups barely matter: 500 soups already find the common ships.
  The threshold and the cap on L matter. Without a cap L explodes (b2-as12:
  L=64428 at a 0.1% threshold), past the GPU's 24000-generation limit, so cap
  L (about 60-240).
- Bounds above ~100x rest on a handful of forwarded soups (0.01% of 40000 =
  4 soups); treat them as "the CPU stops being the bottleneck", not as
  numbers. Then GPU throughput decides.
- Rules with Life-like ash (b3s23-q4i, b35ys23) are the best case. Rules with
  many high-period oscillators (b2n3-q5y6cs23-k: p10, p68, p204) are the worst.
- Rare expensive soups can dominate: in b2ci3ai4c8s02ae3eijkq4iz5ar6i7e one
  soup (suffix 1359 of seed `gpufilter2`) took 1.8 s, 40% of the CPU time of
  15000 soups; it ends PATHOLOGICAL with xq4_157. The GPU would forward it
  (time limit), so the CPU still pays. Need more soups per rule to estimate this
  tail reliably.

### Spaceinvaders

- CPU speedups apply to spaceinvaders builds: same kernels, memo and
  stabilisation code; the MAXPOP/MAXGEN checks run at the same points with the
  same generation counts (not separately measured).
- CPU spaceinvaders with default limits (maxpop 10000, maxgen 180000) does not
  skip soup 1359 above (population stays low; each phase stays under
  maxgen); it still took 1.63 s. A lower maxgen would skip it.
- GPU spaceinvaders (idea): when a universe hits the GPU time limit, or its
  population grows past a limit, count the soup as skipped instead of
  forwarding it. Keep forwarding when only a small object reaches the
  universe edge, or rare (interesting) ships would be skipped: this needs a
  population/area test on the GPU (popcount reductions already exist in
  `gpupattern.h`). The criterion differs from CPU spaceinvaders (GPU time and
  space limits), so upload under its own symmetry name. For the rule above it
  would turn a 2x bound into ~50x.
- The GPU path with spaceinvaders (`--cuda` plus `--maxpop`) is untested;
  check how `mkparams.py` names the symmetry (G1 plus `_spaceinvaders`).


4. Smaller leads (INT path)
---------------------------

Profile of the current INT build (qr4k D8_1, callgrind): ~40% two-generation
memo lookups and misses, ~20% upattern scheduling (updateTile, updateNeighbour,
jumptable), ~20% classification. Left over:
- Faster memo key/hash (SIMD gather of the 20x20 window; cheaper hash). Few %.
- `universal_leaf_iterator` copies 4x32 bytes into `inleaves` per call;
  the 2-generation path could read the tile buffers directly. Few %.
- Steps above 2 generations: tried twice, not included. With the
  `naivestab_awesome` fix the census is exact, but there's no gain with
  --profile. See "Tried and rejected" in CHANGES.md.

That profile is x86. On the M4 Pro with the NEON kernel (`sample`):
- TLS: `tlv_get_addr` is 1.4-3% of samples (thread_local memo per leaf call).
- Is the leaf memo still worth it now that a miss costs 45 ns? The hit path
  is 13-17% of samples.
- The scheduler (advance, universal_leaf_iterator, updateNeighbour) is now the
  largest cost, especially on b2e.


5. Open investigation: no speedup on a 2019 13" MacBook Pro
-----------------------------------------------------------

Report: a user with a 2019 13" MacBook Pro (2.4 GHz quad-core Intel Core i5,
likely i5-8279U Coffee Lake [inferred from Apple's spec], AVX2, no AVX-512,
256 KB L2 per core, 6 MB L3; macOS Sequoia 15.7.7, Apple clang/libc++) sees no
speedup.

Measured in the sandbox, closest emulation (clang 18 + libstdc++,
`-march=skylake`, no AVX-512 instructions in the binaries, Xeon, -p 2, 2
alternating rounds):

| rule | upstream apgmera v5.43/ll2.5.9 | APGsmooshed 8a290e2 | current |
|---|---|---|---|
| b3-qr4k5cek6cs23-a4er56aek / D8_1 | 144 soups/s | 720-745 | 909 |
| b2-as12 / C1 | 206 | 1312-1381 | 1862-1864 |

Upstream census on b2-as12 equals the libstdc++ kat reference. Upstream was
assembled from GitLab zips (apgmera-master + lifelib-master; cpads taken from
APGsmooshed's vendored copy, unmodified upstream). A cache-starved test
(memo tables 2^18 entries, ~86 MB total, far beyond cache) left speed
unchanged (989-998 vs 917-1001 soups/s), so small caches don't explain it.
Conclusion so far: nothing in the code explains "no speedup" on that CPU;
likely causes are on the user's side (totalistic rule, old or wrong binary,
different comparisons). Not tested: Apple clang, libc++, macOS, the real
machine.


6. Two-generation steps were slower on some rules (now adaptive)
------------------------------------------------------------------

Measured (clang 18, `-march=skylake`, Xeon, -p 2, 2 alternating rounds,
soups/s; census identical across all three builds, checked single-threaded on
each row except qr4k/b2-as12, which have kat):

| rule / symmetry | upstream | 8a290e2 | current | current vs 8a290e2 |
|---|---|---|---|---|
| b3s23-q4i / D4_+4 | 1457-1461 | 3140-3192 | 2476-2606 | -20% |
| b35ys23 / C1 | 1137-1164 | 3097-3221 | 2680-2726 | -14% |
| b35ys23 / D8_1 | 1365-1443 | 3102-3105 | 2742-2880 | -9% |
| b2n3-q5y6cs23-k / D4_+1 | 610-624 | 1676-1714 | 1456-1548 | -10% |
| b2n3-q5y6cs23-k / C1 | 475-481 | 2050-2141 | 2238-2333 | +9% |
| b3-qr4k5cek6cs23-a4er56aek / D8_1 | 144 | 720-745 | 909 | +22..26% |
| b2-as12 / C1 | 206 | 1312-1381 | 1862-1864 | +35..42% |
| b2e3air4ijwzs12-in3k4r5i / C1 | 3.3 | 20.2-20.5 | 38.9-39.4 | +92% |

Bisected on b3s23-q4i/D4_+4: memo-key commit +3..8%, two-generation commit
-22..27%, incremental hash neutral. Cause (callgrind): the two-generation
memo hits 46% there (qr4k: 92%); each miss is 3 kernel calls, each with 32
random reads of the 16 MB lut24, so it is memory-bound even though
instructions fall 17%. Fast rules with short-lived soups and Life-like ash
lose; slow chaotic rules gain a lot.

Implemented: per-thread choice from the two-generation memo hit rate
(threshold 85% over the first 2^20 calls); measurements in CHANGES.md section
2. On ARM the premise doesn't hold: the NEON kernel doesn't use lut24, always-2
won on every rule and adaptive lost up to 35%, so it is compiled out there
(numbers in CHANGES.md section 2). The tables above are x86 and lut24 only.
Still open, x86 only: calibrate the threshold on more rules
(b2n3-q5y6cs23-k/C1 sits at 87%, where both modes are within 5%); b2e may be a
few % behind always-2 (within noise); the slow rules below aren't measured.
Original notes: choose 1- or 2-generation steps per rule. Options: (a) at runtime per thread, from the
two-generation memo's hit rate over the first ~1e6 calls (threshold to
calibrate on the table above; switching 2->1 makes upattern decache once);
(b) at build time, a short timed probe of both modes like `--profile`. Check
with this table plus the kat rules.

Not tested: slow rules b2e3air4ijwzs12-in3k4r5i/D2_+2 and
b3-ckq4zs2-c3-a4iq5k6k (C1, D8_4), skipped for time.

Rule strings: several in a list of rules from the owner have Hensel letters in
non-canonical order (e.g. b3-cnyq4j5e6n7cs2-c3-ay4einyrz6c); lifelib rejects
them (NonCanonicalError, which names the canonical spelling), in the original
program too.
