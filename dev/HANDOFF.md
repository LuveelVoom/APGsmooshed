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

### Per-tile periods vs one global L (2026-11-21)

Idea: instead of one global L for the whole universe, each tile may settle with
its own period from a set {lcm(p, 6) : p common, <= cap} (plus the global L).
Implementation needs only one snapshot per tile: snapshot at step T, compare at
every 6-generation boundary T+6k, accept a match only if 6k is in the set,
re-snapshot after T+cap. So memory is the same as now; only the wait grows.

Fresh logs (old ones lost): instrumented copy of the tree (`APG_SOUPLOG` in
`censusSoup`, scratchpad only), single thread per rule, 4 rules in parallel
next to a 10-thread search, so per-soup times are noisy. Script:
`tools/gpufilter_multiL.py` (warm-up 60%, test 40%, common = 0.1% of warm-up,
ships deleted, global L starts at 6). "Per-tile" scores per object (each
period divides some member), which is optimistic: it ignores tiles holding
two oscillators that only fit different members.

| rule (C1 unless noted) | soups | global L, cap 240 | per-tile, cap 240 | global, cap 1200 | per-tile, cap 1200 |
|---|---|---|---|---|---|
| qr4k | 63.5k | 160x (L=42) | 4281x | 4281x (L=378) | 4281x |
| b2-as12 | 100k | 5x (L=84) | 8x | 8x (L=1092) | 893x (708 for p236) |
| b2e... | 40k | 35x (L=24) | 35x | 35x | 35x |
| b3s23-q4i / D4_+4 | 100k | 698x (L=6) | same | same | same |
| b35ys23 | 20k | 132x (L=6) | same | same | same |
| b2n3-q5y6cs23-k | 15k | 7x (L=204) | 9x | 9x | 9x |
| b2ci3ai4c8s02ae3eijkq4iz5ar6i7e | 15k | 215x (L=6) | same | same | same |

- Per-tile helps exactly where common periods are coprime-ish and large:
  as12 (p14, p26, p236), qr4k (p27 with p42). Elsewhere it changes nothing.
- b2n3: still 9x. Remaining blockers are a ship below the threshold
  (xq4_26psk) and yl puffers, which must be forwarded anyway.
- b2e: blocked by rare p7/p28 oscillators (xp7_64hah4c, xp28_5i8g4a, ...),
  not by L.
- b2ci 215x is misleading: soup 1359 (0.60 s of 1.31 s total) is in the
  warm-up part of this log. With it in the test part the bound is ~2x again.
- Values above ~100x rest on 2-10 forwarded soups.
- "Lost" (discarded soups containing an object below the common threshold)
  is large at 0.1%: as12 ~3000 of 40000 test soups, b2e ~1100 of 16000,
  q4i 111 of 40000. Mostly rare still lifes and low-period oscillators; the
  same property as the Life GPU search. Per-tile adds almost nothing to it.

### On-the-fly learning (2026-11-21)

Simulation on the same logs: `tools/gpufilter_online.py`. Warm-up 2000 soups
(0.1% threshold), then the filter is updated every 1000 soups from forwarded
soups only: an object seen in M forwarded soups is admitted (period lcm(p, 6)
<= 1200 into the tile set, or ship into the ship table); yl/zz never.
"missed" = distinct objects in the log never censused (seen >= 2 times in a
full census).

| rule | no learning: last 40% / missed | M=5: last 40% / missed | M=2: last 40% |
|---|---|---|---|
| qr4k | 160x / 54 | 6053x / 54 | 6053x |
| b2-as12 | 893x / 599 | same | same |
| b2e | 35x / 246 | 374x / 279 | 460x |
| b3s23-q4i D4_+4 | 698x / 26 | 3601x / 26 | 5919x |
| b35ys23 | 79x / 4 | 218x / 4 | 1068x |
| b2n3-q5y6cs23-k | 9x / 9 | 9x / 9 | 9x |
| b2ci... | 215x / 9 | same | same |

- Learning helps most where ships/periods sit just under the warm-up
  threshold (b2e, q4i, qr4k). It cannot help b2n3 (remaining blockers are yl
  puffers) or the b2ci tail soup.
- Extra loss from learning is small (b2e +33 objects, others +0..+3).
- The big loss is structural and exists without learning: still lifes and
  oscillators of admitted periods are never censused, rare or not (as12:
  599 objects seen >= 2 times are never censused, e.g. xs12_221q20r4,
  xp26_gglglggzwa0a). Same property as the Life GPU search, so the census is
  a different thing from a C1 census.
- Warm-up 20000 instead of 2000 (8000 for the 15-20k logs): with learning
  on (M=5 or 2) the last-40% bounds are the same within noise (qr4k 6053x,
  q4i 3601x/5919x, b2e 374x/449x vs 460x, b2n3 9x, b2ci 215x); learning
  converges to the same settings. Without learning the bigger warm-up helps
  (qr4k 160x -> 4281x, b35y 79x -> 132x). Lower "missed" with the bigger
  warm-up only reflects the extra fully censused warm-up soups.
- Admitting a period discards every oscillator of that period; GPU ship
  deletion matches (p, dx, dy) plus a small population, so admitting a ship
  also discards other small ships of that velocity. Learn ships more freely
  than periods.

### Metal Phase A: raw throughput (2026-11-21)

`tools/metal/gpuA.mm` (+ `mktable.py`, `checkdump.py`, `test.sh`). Standalone:
many N x N torus universes, one threadgroup per universe, one thread per row,
all generations in threadgroup memory. Kernels: generic 512-entry LUT (any
non-B0 rule, table baked into the source) and hand-written bitsliced B3/S23.
Shaders compile at runtime (`newLibraryWithSource`), so no Xcode is needed.
Soups: 16x16 at density 1/2 from a xorshift PRNG (not apgluxe's SHA-256 soups).
`test.sh`: GPU vs CPU reference bit-exact (30 and 500 gens, torus wrap) and
CPU reference vs casim (30 gens) on B3/S23, qr4k, a shift-NE MAP rule (a
mirrored rule is caught) and a random MAP rule; all pass.

Measured (M4 Pro, 20 GPU cores, 16384 universes, 3 runs, noise ~1-2%,
search suspended, Firefox video decoding in the background), cell-gens/s:

| kernel | WB=32, N=64 | WB=64, N=64 | WB=32, N=128 | WB=64, N=128 |
|---|---|---|---|---|
| LUT (qr4k) | 1.67e11 | 1.61e11 | 3.4e10 | 1.10e11 |
| bitsliced B3/S23 | 2.78e12 | 2.64e12 | 2.95e12 | 2.65e12 |

- 64-bit words cost only ~5-10% against 32-bit in the bitsliced kernel.
- LUT is ~17-25x slower than bitsliced Life. LUT WB=32 at N=128 is 3x slower
  than WB=64 (register spill from the full 128-cell unroll, inferred; not
  investigated).
- CPU baseline: clean qr4k C1 build, 10 threads: 6600 soups/s (300k soups).
- Generations to stabilisation in apgluxe (`stabilise3`, counter in a
  scratchpad copy, 5000 qr4k soups): mean 4931, median 620, but 985 of the
  986 soups with xp42_2his take > 10000 (apgluxe's periodicity test is slow
  on p42; a GPU compare at t+42 would settle them early). Without them:
  mean 836, median 472, p90 1300.
- At ~1000 gens/soup the GPU screens: LUT 64x64 ~40k soups/s (6x CPU), LUT
  128x128 ~6.7k (1x), bitsliced Life speed 128x128 ~160k (24x), 64x64 ~650k.
  A generated INT circuit is likely 2-4x Life's op count (inferred), so
  ~40-80k soups/s at 128x128.
- B3/S23-a5 and B3/S23-q4i (C1): LUT throughput is rule-independent (same
  1.6e11 / 1.1e11 cell-gens/s). CPU (clean build, 10 threads, 500k soups):
  a5 88.7k soups/s, q4i 174k. Stabilisation gens (20k soups): a5 mean 398,
  median 320, p99 1324; q4i mean 157, median 124, p99 512. GPU screening at
  the mean: LUT 64x64 a5 ~100k (1.1x CPU), q4i ~254k (1.5x); LUT 128x128
  0.2-0.25x; bitsliced Life speed 64x64 18-24x, 128x128 4.6-5.9x. For
  Life-like-ash rules the CPU is fast, so only a bitsliced kernel with small
  boxes helps.
- So the LUT kernel is only worth it with small boxes; a bitsliced INT kernel
  (Phase C) and an active-tile layout (cost by active area, not box) are
  what make the GPU clearly faster.

### CPU: learned period in stabilisation (2026-11-21)

Implemented as the opt-in `--symmetry <sym>_smooshed_stabiliser` build (README,
CHANGES.md section 9). L is learned at runtime from the census (count-based
threshold, cap 1000), so it can differ from the soup-based values in the
tables below (qr4k D8_1 learns 756, not 108; both ~1.5x). Experiments:

qr4k: the 20% of soups with xp42_2his take 81% of CPU time (6.1 ms vs
0.34 ms). `naivestab_awesome` tests population periodicity at 12/8, later
18/24/30, for up to 1000 iterations; none is a multiple of 42, so these
soups run ~20000 gens before phase II (hashes every 30 gens) catches them.
Experiment in a scratchpad copy (not in the repo): `APG_LEARNED_PERIOD=L`
pre-pass (40 steps of L, stable after 8 equal populations) and
`APG_NAIVE_CAP=K` (phase I iterations). Per-soup census vs original, single
thread, seeds gpubench and stab2 (5000 qr4k soups, 20000 others):

| variant | qr4k speed | soups with a different census |
|---|---|---|
| cap 100 | 0.87x / 0.94x | 7 / 4 |
| cap 200 | 1.22x / 1.29x | 0 / 1 |
| L=42 | 1.01x / 1.02x | 6 / 3 |
| L=42 + cap 100 | 1.34x / 1.36x | 6 / 4 |
| L=84 | 2.23x / 2.27x | 8 / 4 |
| L=84 + cap 200 | 2.39x / 2.44x | 8 / 4 |
| cap 100, b3s23-q4i | 0.93x / 1.00x | 0 / 0 |
| cap 100, b3s23-a5 | 0.58x / 0.57x | 1 / 0 |

- L must be a multiple of the ships' periods too (population of a c/4 ship
  changes with period 4): L=42 does nothing, L=84 works. p42 soups go from
  median 20446 to 1176 stabilisation gens, 6.1 ms to 1.6 ms.
- A low cap alone is rejected: phase II is slower than phase I for slowly
  settling soups (a5 0.58x).
- Census differs in ~0.1% of soups (fewer xq2_4ra/xq4_27, more xs4_252,
  xp2_*): stopping earlier misses late reactions, e.g. ships crashing into
  far debris. Not identical to the original, so it would need an option,
  off by default.

Thorough run (same day). The learned test now runs after APG_LEARNED_AFTER
ordinary phase I iterations and needs APG_LEARNED_CONFIRM equal populations.
L = lcm of periods of oscillators and ships in >= 1% of soups (b2e capped at
112). Single thread, 8 jobs in parallel; qr4k 3 seeds x 30000, others 2 x 20000.

| rule (L) | confirm/after | speed | differing soups |
|---|---|---|---|
| qr4k (84) | 8/0 | 2.19-2.21x | 63 of 90000 |
| qr4k (84) | 4/0 | 2.37-2.41x | 66 |
| qr4k (84) | 8/40 | 2.30-2.33x | 62 |
| qr4k (84) | 4/40 | 2.50-2.53x | 65 |
| qr4k (84) | 8/80 | 2.48-2.51x | 55 |
| b2-as12 (42) | 4/40, 8/40 | 1.76-1.83x | 0 of 40000 |
| b2e (112) | 4/40, 8/40 | 1.02x | 0 |
| b2n3-q5y6cs23-k (204) | 4/40 | 1.12x | 1 |
| b3s23-q4i (12, control) | 4/40, 8/40 | 0.97-1.04x | 0 |

- No object found by the original is missing from any variant's census.
- The differences are stop-time artefacts in both directions: the variant
  sometimes runs longer than the original and still differs (an xq2_4ra
  counted as a ship before it crashes, xp2_f after). Neither census is
  "the" correct one.
- Clean builds, 10 threads, 300k soups, 2 runs: qr4k 6400 -> 15400 soups/s
  (2.4x), b2-as12 28.2k -> 50.2k (1.78x).

Other symmetries (confirm 4, after 40; 2 seeds x 15000; single thread, up to
8 jobs in parallel plus some other load, so ~10% seed-to-seed noise). L from
C1 vs L learned from that symmetry's own census (periods in >= 1% of soups):

| rule, symmetry | C1 L | speed | own L | speed | differing soups |
|---|---|---|---|---|---|
| qr4k C2_4 | 84 | 2.02-2.06x | 84 | same | 13 of 30000 |
| qr4k C4_4 | 84 | 1.78-1.84x | 84 | same | 19 |
| qr4k D2_+1 | 84 | 1.85-1.88x | 84 | same | 9 |
| qr4k D4_x1 | 84 | 1.56-1.57x | 756 | not tested | 5 |
| qr4k D8_1 | 84 | 1.08-1.09x | 108 | 1.56-1.64x | 0 |
| b2-as12 C2_4 | 42 | 1.65-1.84x | 42 | same | 0 |
| b2-as12 C4_4 | 42 | 1.88-1.91x | 84 | 2.21-2.29x | 0 |
| b2-as12 D2_+1 | 42 | 1.32-1.33x | 546 | 1.51x | 0 |
| b2-as12 D4_x1 | 42 | 1.79-1.83x | 42 | same | 0 |
| b2-as12 D8_1 | 42 | 1.14-1.16x | 42 | same | 0 |

No object found by the original is missing anywhere. L must be learned per
symmetry (symmetric soups make different common ash).

### GPU: tile skipping estimate (2026-11-21)

`tools/metal/activetiles.py` (NumPy, not GPU): 400 soups per rule in a
128x128 box with an absorbing edge; a tile is stepped only if a cell changed
in or next to it in the previous generation; the box stops when its state
repeats with period L (checked every L gens).

| rule (L) | box settle gens mean / median / p90 | 32x32 tiles | 16x16 tiles |
|---|---|---|---|
| B3/S23-q4i (12) | 174 / 132 / 324 | 5.2x less stepping | 15.6x |
| B3/S23-a5 (60) | 462 / 420 / 720 | 5.1x | not run |
| qr4k (84) | 690 / 588 / 1176 | 3.7x | 8.3x |

- Correction to an earlier claim: the bitsliced Life kernel is ALU-bound
  (~55 ops/word-gen at 8.7e10 word-gens/s ~ 4.8e12 ops/s, about the GPU's
  integer peak [unsure: peak from memory]). Fewer barriers would give
  10-20% at most. The levers are ops per cell (circuit) and cells stepped.
- 32x32 tiles map to one SIMD group (uint32 rows, shuffles); skipping is a
  uniform branch. 16x16 needs packing active tiles from many universes into
  SIMD groups (the CUDA `compact_tiles` approach): more gain, much more code.
- The absorbing edge deletes everything that reaches it (ash too), so these
  settle times are a little optimistic for a real filter that forwards
  edge-touching soups.

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
