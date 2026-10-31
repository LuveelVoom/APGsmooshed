Changes from upstream
=====================

Based on:
- apgmera `2237ebe` (2025-01-05, "bump lifelib"), https://gitlab.com/apgoucher/apgmera
- lifelib `05632e3` (2025-01-05, "bugfix for catagolue"), https://gitlab.com/apgoucher/lifelib
- cpads `bfb3d49` (2024-10-23), lifelib's submodule, https://gitlab.com/hatsya/open-source/cpads
- spaceinvaders: jedlimlx/apgspaceinvaders commits `68e32ac` and `4d18dc1` (May 2024)
- sse2neon v1.8.0, https://github.com/DLTcollab/sse2neon

lifelib and cpads are vendored (plain copies, not submodules), so an upstream
update can't silently drop the changes below. Code changes in lifelib are marked
`(APGsmooshed; see CHANGES.md)` or guarded by `LIFELIB_PORTABLE`. To move to a
newer upstream: export the old upstream commits above, diff them against this
tree, and re-apply that diff to the new upstream.

"Census identical" below means: sorted census of a fixed-seed, no-upload haul
equal to the original program's, on b3s23/C1 (200k soups),
b3678s34678/C1 (300k), b2-as12/C1 (15k), b2e3aeinr4ce5c6n7c8s1c2-i3-a4acen5cen6c78/C1
(15k) and b3-qr4k5cek6cs23-a4er56aek/D8_1 (30k) where the change applies.
These are the references in tests/kat/.


1. Native ARM build for isotropic non-totalistic rules
-------------------------------------------------------

lifelib's kernels are x86 inline asm. The SSE2/SSSE3/SSE4.1 blocks reached at
instruction-set level 8 (SSE4.2) are transcribed one intrinsic per instruction in
`lifelib/avxlife/portable_sse.h`. On non-x86 targets `LIFELIB_PORTABLE` is
defined and the intrinsics come from `sse2neon.h`. Every other asm block is
guarded and traps if reached. `best_instruction_set()` reports level 8 on ARM, so
dispatch takes the same paths as an x86 SSE4.2 machine (and as x86 under Rosetta).

Files: `lifelib/avxlife/{portable_sse.h, sse2neon.h, lifeperm.h, isoluts.h, eors.h,
eightbit.h, ltl.h}`, `lifelib/classic/{vtile28.h, vtile44.h}`,
`includes/hashsoup2.h` (stack-pointer asm used only as seed entropy:
`__builtin_frame_address` off x86).

Checked: the 7 routines and the leaf kernel on 200000 random inputs give
bit-identical hashes as original asm, x86 intrinsics and arm64 (tests/unit.sh);
census identical on the three non-totalistic rules. The 1-generation kernel and
an experimental 2-generation composition also matched casim on 9000 random 32x32
inputs (0 mismatches).

`recompile.sh` builds natively on ARM only for the isotropic genus (rule strings
matching `^b[1-8ceaiknjqrytwz-]*s[0-8ceaiknjqrytwz-]*$` that aren't totalistic).
Everything else is built for x86_64 on macOS, or rejected on other ARM systems.
An earlier version keyed this on "generated rule code contains asm", which let
LtL rules (also lifelib family 4, with their asm in the static ltl.h) build
natively and trap immediately.

Measured (M4 Pro, -p 10, soups/s, census identical):

| build | qr4k D8_1 | b2-as12 | b2e |
|---|---|---|---|
| original, x86 under Rosetta | 2941 | 4202 | 1290 |
| arm64 | 4843 | 6755 | 2063 |
| arm64 + --profile | 5585 | 7574 | 2361 |


2. Leaf memo (isotropic non-totalistic rules, any architecture)
----------------------------------------------------------------

`iterate_var_leaf` in `lifelib/avxlife/isoluts.h` gets a per-thread,
direct-mapped cache of 2^14 entries (80 bytes each, 1.25 MiB per thread). It is
keyed on the central 18x18 cells of the 32x32 input, which is all the kernel
reads: rows 7..24, bits 7..24 of the row form. That was read off the code and
also checked: flipping any of the other 700 bits changed the output in 0 of
300000 trials. It is exact: a full-key compare, and empty slots hold the real
result for the all-zero input (so B0 is safe, although the isotropic genus
excludes B0).

Leaf inputs repeat a lot (oscillators, still lifes): 88% hit rate with 2^14
entries on real soups (2^12: 78%, 2^16: 92%, 2^18: 93%).

Measured, back to back: first version (128-byte key) +14..+26% over arm64 +
PGO; 18x18 key a further +27..34% (qr4k), +22..35% (b2-as12), +38% (b2e) with
PGO.

Key without the transpose: the key used to be built by transposing the whole
32x32 input to rows (`z64_to_r32_sse2`) and cutting out 18x18. It is now read
straight from the Z-ordered input words. Each word is an 8x8 tile (byte = row,
bit = column), so the window is 4 whole tiles (words 3, 6, 9, 12), 8 one-row or
one-column slivers and 4 corner bits, packed by AND/shift/OR into the same 6
words. The transpose now runs only on a miss (inside the kernel). Exact: by unit
vectors, the 324 window bits map to 324 distinct key bits and the other 700 to
none; the map is linear; 0 disagreements between "keys equal" and "windows
equal" in 2M perturbation trials; memo vs uncached kernel 0 mismatches in 4M
calls (asm and portable builds). `tests/memo.cpp` (run by `unit.sh`) repeats
the last check with inputs that hit the memo; it fails (65 mismatches) if one
corner bit is dropped from the key. Census identical (kat, 1 and 2 threads) on
the three non-totalistic rules.

Measured on Linux x86-64 (Xeon @ 2.8 GHz, AVX-512, 2 cores, GCC 13.3, -p 2, no
PGO, 3 alternating runs each, soups/s): qr4k D8_1 736-770 -> 960-998 (+28%),
b2-as12 1114-1171 -> 1416-1436 (+21..29%), b2e 292-304 -> 372-382 (+22..31%).
ARM: see section 7.


Two-generation steps (isotropic genus): isotropic rules had valid mantissa
{0, 1}, so the tile scheduler advanced one generation per step. Every tile with
p2 debris was re-run every generation (the source of the 95% memo hit rate),
and each step paid the scheduler's per-tile cost. Totalistic rules step 2
generations, so p2 tiles go idle. Now `genera/isotropic.py` has mantissa
{0, 1, 2}, `autocompile.py` dispatches n == 2 to `iterate_var_leaf2` for
family-4 rules whose mantissa contains 2 (LtL, also family 4, is unchanged), and
`isoluts.h` has:
- `iterate_var_leaf_rows`: the old kernel body, taking rows instead of Z-order
  (`iterate_var_leaf_compute` = transpose + this).
- `iterate_var_leaf2_compute`: generation 1 on rows/columns 7..24 from two
  kernel calls on diagonally shifted rows (covering 7..22 and 9..24) plus the 8
  cells they miss from `xfl9` (the lookup the kernel's tables are built from),
  then generation 2 with one more kernel call.
- `iterate_var_leaf2`: memo as above, keyed on the 20x20 window (400 bits, 7
  words; same unit-vector check: 400 distinct bits, 0 from outside, 0
  disagreements in 2M trials).

`upattern::advance` keeps one-generation steps when `history != 0` on 16-wide
tiles: the leaf iterator ORs only a step's last generation into the history
layer, so a 2-generation step would drop odd generations from the envelope
`separate()` uses. Switching from 2 back to 1 already makes upattern decache
every tile (upstream logic, also used by the totalistic path).

Checked: the scalar `lut9` reference matches the 1-generation kernel on 25.6M
cells for qr4k and on 4 random 512-bit tables (so it doesn't rely on isotropy);
2-generation leaf (memo and uncached) vs two reference generations: 0
mismatches in 102M cells (qr4k) and 0 on the 4 random tables, asm and portable
builds; `tests/memo.cpp` checks this and fails (108778 mismatches) if one of the
8 scalar cells is dropped. Census identical (kat, 1 and 2 threads) on the three
non-totalistic rules. Hexagonal rules use the same genus and get the change too;
not tested end to end.

Measured (same Xeon, -p 2, no PGO, 3 alternating runs, soups/s, against the
transpose-free key above): qr4k D8_1 969-979 -> 1286-1326 (+32..37%), b2-as12
1413-1468 -> 1847-1899 (+26..34%), b2e 379-390 -> 530-566 (+36..49%). Against
the build before both changes: qr4k +67..80%, b2-as12 +58..70%, b2e +74..94%.
Under callgrind (qr4k, 1500 soups) instructions fell from 22.6G to 14.9G with
the first version (4 kernel calls per miss); leaf steps 50.6M -> 17.5M two-gen
(92% memo hits) + 1.4M one-gen. A first version computing generation 1 with 4
shifted kernel calls was 4..11% slower than the final one.
ARM and --profile: see section 7.

Per-rule choice of step size: two-generation steps are slower on rules whose
two-generation memo rarely hits (each miss is 3 kernel calls with random reads
of the 16 MB lut24, so memory-bound). Each thread now counts that memo's hit
rate over its first 2^20 calls; below 85% (`APG_TWOGEN_MIN_HIT`), upattern
uses 1-generation steps in that thread from then on (`twogen_ok()` in
isoluts.h, `uli_twogen_ok` generated by autocompile.py). Hashlife (used in
classification) keeps two-generation leaves. Both paths are exact, so the
per-thread decision can't change results.

Hit rate (2-generation mode, 1 thread) and soups/s (Xeon, GCC 13, -p 2, 2
alternating rounds) for always-2 / adaptive / always-1:

| rule / symmetry | hit rate | always-2 | adaptive | always-1 |
|---|---|---|---|---|
| b3s23-q4i / D4_+4 | 47% | 3125-3143 | 3491-3565 | 3647-3650 |
| b35ys23 / C1 | 56% | 3417-3572 | 3629-3919 | 3827-3922 |
| b2n3-q5y6cs23-k / D4_+1 | 80% | 1758-1889 | 2047 | 2090-2182 |
| b2n3-q5y6cs23-k / C1 | 87% | 2675-2677 | 2502-2675 | 2508-2619 |
| b3-qr4k5cek6cs23-a4er56aek / D8_1 | 94% | 1053-1082 | 1066-1082 | 941-965 |
| b2-as12 / C1 | 98% | 1777-1797 | 1818-1819 | 1467-1509 |
| b2e3aeinr4ce5c6n7c8s1c2-i3-a4acen5cen6c78 / C1 | | 632-675 | 619-626 | 461-463 |

The remaining gap to the better mode is mostly the warm-up (these runs are
short). Census identical to always-2 (single-threaded) on the four
non-kat rows; kat passes on the three kat rules; unit.sh passes.

This applies to the lut24 kernel only. On the M4 Pro with the NEON kernel
(section 8) a miss reads no tables and two-generation steps win on every rule,
so `APG_TWOGEN_MIN_HIT` defaults to 0 there (and with `LIFELIB_ISO_QL`, where
adaptive wasn't measured), which compiles the choice out; define it to
override. x86 and `LIFELIB_NO_NEON_ISO` default to 85. M4 Pro, NEON kernel,
-p 10, 3 alternating runs, soups/s:

| rule / symmetry | always-2 | adaptive at 85 | always-1 |
|---|---|---|---|
| b3s23-q4i / D4_+4 | 94384-99527 | 73611-76908 | 71813-72137 |
| b35ys23 / C1 | 112170-113766 | 73964-76002 | 71187-72294 |
| b2n3-q5y6cs23-k / D4_+1 | 55659-56647 | 39904-41655 | 37500-39453 |
| b2n3-q5y6cs23-k / C1 | 68989-71633 | 53319-63311 | 41017-42346 |
| b3-qr4k5cek6cs23-a4er56aek / D8_1 | 20531-20756 | 20053-20565 | 10763-10908 |
| b2-as12 / C1 | 27640-28022 | 27760-28235 | 18797-19236 |
| b2e3aeinr4ce5c6n7c8s1c2-i3-a4acen5cen6c78 / C1 | 7398-7592 | 7199-7515 | 4451-4613 |

With --profile: b3s23-q4i 103681-104548 vs 85929-87854 adaptive, b2n3 C1
84926-85215 vs 73126-76453. Final ARM build vs the previous commit: b3s23-q4i
96061-97943 vs 95306-97253, qr4k 20891-21586 vs 21038-21322, b2-as12
28345-28827 vs 28552-28841, i.e. noise.

Checked: census identical across always-2, adaptive and always-1 on all seven
rows (1 thread, 5000-20000 soups). kat passes on arm64 by default and with the
threshold forced to 101, so that every thread switches mid-haul (qr4k, b2-as12,
b2e at 1 and 4 threads; the slower times show the switch happened); on x86_64
under Rosetta with the adaptive code active (qr4k, b2-as12) and on b3s23 (empty
switch). unit.sh passes. x86 speed wasn't measured here.


3. Object separation builds patterns in one go (all rules, any architecture)
---------------------------------------------------------------------------

`get_patterns` in `lifelib/classifier.h` added every cell of an island to its
pattern one at a time (a hashlife `subrect` plus a union per cell). For
one-plane rules it now collects the cells in a bitworld and builds each pattern
once. Checked: 119003 islands over 3 rules gave the same hypernode and depth as
the old code; census identical on all five rules. Multi-plane rules keep the old
loop.

Measured: arm64 qr4k +26%, b2-as12 +20%, b2e +1%; x86 b3678s34678 2.2-3x;
b3s23 unchanged.

**Optional since 2026-10-29:** on by default (makefile adds
`-DAPG_FAST_SEPARATION`); `./recompile.sh --no-fast-separation` builds
upstream's per-cell loop instead. Census identical either way (kat, off with 1
thread, on with 2 threads, all three non-totalistic rules). Cost of turning it
off, measured on the M4 Pro (-p 10, no PGO, 3 runs, soups/s, builds that also
had the experimental 4-generation steps): qr4k 13153-14494 vs 19152-22095,
b2-as12 18455-19235 vs 28625-29650, b2e 6304-6360 vs 7802-7909. apgluxe's
automatic rebuild runs `recompile.sh` without the option, so it gets the
default.


4. Spaceinvaders
----------------

The diff of jedlimlx/apgspaceinvaders (`--maxpop`, `--maxgen`, `SYMMETRY2`,
`<sym>_spaceinvaders` upload names) applied unchanged to
`includes/{apgluxe,searcher,searching,stabilise}.h`, `mkparams.py`,
`recompile.sh`. Local addition: `recompile.sh --symmetry C1_spaceinvaders` is
accepted (stripped to C1, default limits). apgluxe's automatic rebuild passes
the uploaded name, which lifelib rejected, so that rebuild failed in the fork.
Checked: off by default (census identical); Seeds (b2s, x86) skipped all 50
test soups at maxpop and the haul completed.

Skipped soups are counted (`aperiodicSoups` in `includes/stabilise.h`, once per
soup in `censusSoup`, reset per haul) and reported in the progress lines and the
haul summary, instead of the fork's one message per skipped soup.

Kept as in the fork, but worth knowing:
- `--maxgen` is checked separately in the two stabilisation phases (phase I
  counts its own steps, phase II its own generations).
- The `HASHLIFE_ONLY` branch of `censusSoup` has no population check.


5. Build and tooling
--------------------

- makefile: upstream's clang test `grep -o clang` matched twice for Apple clang
  ("Apple clang version ... (clang-...)"), so `--profile` never merged its
  profile and added gcc-only flags. Now `grep -q`.
- makefile: the `--profile` training haul is `PROFILE_SOUPS` (default 20000;
  upstream hardcoded 100000), with threads = CPU count (upstream 4). Compilers
  can be chosen with `CPP_COMPILER` / `C_COMPILER`.
- `recompile.sh`: re-runs natively when started under Rosetta; the native/x86
  choice described in section 1. lifelib is vendored, so the
  `update-lifelib.sh` step (git submodule update) is gone.
- Progress lines (`includes/searching.h`) every 2 s instead of 10 s, and also
  while a batch of soups is running (upstream printed only between batches; a
  100000-soup batch takes about 10 s on b3-qr4k5cek6cs23-a4er56aek). The
  soup-count trigger is 200000 instead of 1000000.
- Version string `v5.43-ll2.5.9-smooshed` (`APG_VERSION` in `includes/base.h`):
  upstream's string with a suffix, shown at startup and sent as `@VERSION` with
  each haul.
- `apgluxe-autorestart.sh`: restarts apgluxe after crash signals; logs them;
  gives up after 5 fast crashes in a row.
- tests/: `kat.sh` (census known answers, per C++ standard library), `unit.sh`
  (routine hashes), `t.cpp` and the generated headers it needs.
  `unit.sh` also builds the opt-in QuickLife kernel (section 8). Upstream's
  `test/minstrels.rle` moved here.
- Removed: `.gitlab-ci.yml` files (upstream CI), `.gitmodules`.


6. Incremental totalHash (rules on UTile: isotropic and other non-Life genera)
------------------------------------------------------------------------------

Phase II of `stabilise3` calls `upattern::totalHash(120)` every 30 generations;
it summed a term for every tile ever allocated. For tile types with a cached
hash (`UTile`; detected by a `hashContrib` field, so `VTile`/Life keep the old
sweep) the sum is now kept (mod 2^64, so the value is identical) and on each
call only these tiles are recomputed: those whose cached hash went stale
(`updateTile` pushes a tile on `hashstale` when it clears `hashCurrent`) and
those allocated since the last call (tile indices only grow). Every other tile
would contribute its unchanged cached hash, so its stored term is still right;
this includes upstream's `eu64`, which changes cells without clearing
`hashCurrent`. Only tiles inside the radius get `hashTile` called, as before.

Checked: a build comparing every incremental result with the full sweep: 0
differences over the kat hauls of all three non-totalistic rules; it aborts if
stale tiles aren't pushed. New tiles are always still empty when first counted in
real soups (140k checked), so `tests/hashtest.cpp` (run by `kat.sh`) inserts
cells into fresh tiles between calls: 0 differences in 18000 calls, 11666
if new tiles are skipped. Census identical (kat) on the three rules.

Measured (Xeon, -p 2, 3 alternating runs, soups/s): b2e 555-566 -> 673-697
(+19..26%); qr4k and b2-as12 within noise (qr4k 1240-1329 vs 1192-1313,
b2-as12 +0..6%). Combined with section 2, b2e is 292-304 -> 673-697 soups/s
(+121..139%) over the version before these changes.


7. ARM measurements of sections 2 (memo key, two-generation steps) and 6
------------------------------------------------------------------------

Apple M4 Pro, Apple clang, -p 10, soups/s from wall time of a fixed-seed
no-upload haul (100k soups; b2e 40k), 3 runs alternating old/new, old = commit
8a290e2. These runs are short, so the noise is large (single runs up to -40%).
Census identical (kat, 1 thread) for all three rules with and without
--profile; unit.sh passes natively and under Rosetta (asm), including the
memo test; kat also passes for b3s23 and b3678s34678 (x86 builds, which don't
use any of this code).

| rule | old | new | old --profile | new --profile |
|---|---|---|---|---|
| qr4k D8_1 | 6389, 7271, 8922 | 14054, 16741, 15705 | 11382, 13971, 12264 | 17396, 16851, 20060 |
| b2-as12 | 15339, 16098, 16025 | 24116, 24217, 24719 | 21146, 13401, 21882 | 29408, 30890, 30292 |
| b2e | 3360, 3335, 3344 | 7086, 7098, 7128 | 4420, 4333, 3528 | 8845, 8554, 9274 |

Pairwise, without --profile: qr4k +75..130% (noisiest), b2-as12 +50..58%,
b2e +111..114%. With --profile on both sides: qr4k +21..64%, b2-as12 +38..39%
(one old run at 13401 left out), b2e +97..163%.
The earlier two-generation attempt (see "Tried and rejected") lost its gain
under --profile; this version keeps it.

Profile of the new build (`sample`, qr4k, -p 1): the leaf kernel on memo misses
(`iterate_var_leaf_rows`) is now the largest single function, then the tile
scheduler (`upattern::advance` with `updateTile` inlined), the memo hit path
and `universal_leaf_iterator`. On b2e the scheduler dominates.


8. NEON leaf kernel (isotropic non-totalistic rules, ARM)
---------------------------------------------------------

On ARM, `iterate_var_leaf_compute` and `iterate_var_leaf2_compute`
(`lifelib/avxlife/isoluts.h`, the memo-miss paths) are a NEON kernel instead of
the x86 code transcribed through sse2neon. One cell per byte; `lut9` (the
rule's 512-bit table, 64 bytes) sits in four registers, and `tbl` looks up 16
cells at once: the 6 bits of the rows at and below a cell pick a lut9 byte, the
3 bits of the row above pick the bit (the same split as `xfl9`). No memory
tables, so `lut24` (16 MiB) and `mix18` (1 MiB) are no longer touched on ARM.
The input is expanded straight from the Z-ordered words with `tbl`/`tst`, and
the output is packed with a `vpaddq` tree that yields the Z-order words
directly. The two-generation leaf computes generation 1 on the full 18x18
(16-lane plus 8-lane block) instead of two overlapping 16x16 kernel calls and
8 scalar cells. x86 keeps the original asm. `-DLIFELIB_NO_NEON_ISO` turns the
NEON kernel off.

Also added, opt-in with `-DLIFELIB_ISO_QL`: a QuickLife-style kernel (64 KiB
table from a 4x4 block to its central 2x2, built from lut9; plain C++, any
architecture), in response to a suggestion to use QuickLife tables. It is
slower than the NEON kernel on ARM and isn't the default anywhere.

Checked: unit.sh passes with both kernels (routine hashes equal to the original
x86 asm on 200000 random inputs; memo test, which includes the two-generation
leaf against a scalar lut9 reference); microbenchmark on 3.8M real memo-miss
inputs recorded from qr4k and b2e soups: 0 mismatches against the scalar
reference for every variant; census identical (kat) on all three
non-totalistic rules, with and without --profile, and with and without
`--no-fast-separation`; b3s23 (x86) kat passes.

Measured, microbenchmark (M4 Pro, ns per call on recorded miss inputs, 1
thread, qr4k; b2e similar): two-generation 113 -> 45 (old: 3 kernel calls +
scalar cells; QuickLife table on the same direct 18x18 structure 77; the
same structure with a 16 MiB 4x6->2x4 table 61); one-generation 29.5 -> 18.5
(QuickLife 35). Most of the old cost was the 3-call structure, not the table.

End to end (M4 Pro, -p 10, 3 alternating runs, soups/s, 150k soups; b2e 40k),
against HEAD 3d48f0b:

| | base | NEON | QuickLife (opt-in) |
|---|---|---|---|
| qr4k D8_1, no PGO | 12823-16801 | 17995-20689 | 17277-19118 |
| b2-as12, no PGO | 25206-26135 | 26810-27377 | 25641-26400 |
| b2e, no PGO | 6902-7109 | 7399-7503 | 7153-7234 |
| qr4k D8_1, --profile | 18344-20436 | 21885-25257 | |
| b2-as12, --profile | 32152-32874 | 32610-32997 | |
| b2e, --profile | 8640-9141 | 8948-9305 | |

So with --profile: qr4k +19..24%, b2-as12 and b2e within noise (0..+4%).
Before this change the kernel was 26% of busy samples on qr4k and 7% on b2e
(`sample`, -p 1), which bounds what a kernel change can give.


Found in upstream, not changed
------------------------------

- **Census depends on the C++ standard library.** `full_3dsep` /
  `get_patterns` iterate a `std::unordered_map<uint64_t, uint64_t>` and merge
  islands greedily (`return subpats; // improvement made`), so the split of some
  pseudo-objects follows hash-table order. GCC/libstdc++ and clang/libc++ gave
  different censuses on b2-as12 (a few p3 objects in 15k soups). Under clang,
  changing only that map's hash function reproduced exactly the same difference.
  Not fixed, since any fix changes results on every platform; tests/kat/ keeps
  references per library.
- An occasional segfault of the original x86 build under Rosetta (one crash
  report: SIGSEGV at address 0x16a in `bitworld::intersect` from
  `base_classifier::census`, after about 5 hours). Cause not found; it looked
  like earlier heap corruption. Hence the restart wrapper.


Tried and rejected (soups/s, back to back unless noted, census identical)
---------------------------------------------------------------------------

- `ROSETTA_ADVERTISE_AVX=1` (AVX2 under Rosetta): b3s23 -66%, b3678s34678 -25%.
- lut24 (16 MiB) replaced by two lut18 (256 KiB) lookups: x86 -5..-13%, arm64
  -3..-9%. A microbenchmark on random leaves showed +11..+28%; random leaves miss
  cache far more than real soups.
- mix18 table replaced by arithmetic: -4..+3% (noise).
- `MallocNanoZone=0`: -4..-9%.
- Apple clang without LTO -5%; Homebrew clang 23 `-mcpu=apple-m4` (no LTO) +3%,
  and -2..+4.5% with PGO; GCC 16 -8%.
- `--profile` for b3s23 (x86): -8%.
- Memo with 2^12 entries (smaller gains); inline 16-word compare instead of
  memcmp (-1..-6%).
- Four-generation steps for isotropic rules (mantissa {0,1,2,4},
  `iterate_var_leaf4` from four shifted two-generation blocks, 24x24-keyed memo;
  the leaf itself was exact against a scalar reference): much slower (kat time
  qr4k 35 s vs 22 s, b2e 111 s vs 21 s) and **census-changing** on b2e (a few p4
  objects). Cause: `naivestab_awesome` returns period 2 when `pat.modified` is
  empty, which assumes steps of at most 2 generations; with 4-generation steps
  p4 soups go idle and get that period. Any step size above 2 needs that check
  changed.
  Tried again 2026-10-29 on top of the NEON kernel (section 8): leaf4 computed
  directly (generations on 22, 20, 18, 16 columns), 9-word key for the 24x24
  window (unit-vector check: 576 distinct bits, 0 from outside; leaf vs 4
  scalar generations: 0 mismatches in 300k inputs), `naivestab_awesome`
  returning 4 instead of 2. Census identical, but b2e kat took 92 s instead of
  16 s: phase II advances 30 generations (7 x 4 + 2), and every switch from 4-
  to 2-generation steps decaches all tiles (`runkgens`), so large patterns were
  recomputed in full every 30 generations. Using 4-generation steps only when a
  whole `advance` call is a multiple of 4 fixed that. Then (-p 10, soups/s):
  without PGO +5..10% over NEON alone (qr4k 19104-21638, b2-as12 28316-29981,
  b2e 7654-7858), but with --profile no gain or a loss (qr4k 21023-24380 vs
  21885-25257, b2-as12 32218-33366 vs 32610-32997, b2e 8405-8679 vs
  8948-9305). Not included.
- 2^16 entries for the two-generation memo: qr4k +0.6..3.5%, b2e -1..+3%, for
  4x the memory per thread.
- A second 1-generation memo (row-keyed) for the kernel calls on the
  two-generation miss path: qr4k -8..-15%, b2e -3..-10%.
- Memo with 2^16 entries on x86 (Xeon, 2 MiB L2 per core, transpose-free key):
  954-998 vs 972-1024 soups/s for 2^14, i.e. noise. qr4k D8_1 already hits 95%
  at 2^14 (50.6M calls, 2.6M misses, 1500 soups under callgrind).
- lifetree memo maps (`std::map`) as `std::unordered_map` -10/-6/+1%, or as a
  flat table with inline slots +1/-1/-5%.
- Caching classification of multi-tile clusters: repeats were 60% of calls but
  0.58 s of 12.4 s; unique clusters dominate.
- Two-generation tile steps for isotropic rules: exact (casim and census
  checks), +12..+40% without PGO but -12..+16% with PGO on both sides, and the
  extra code slowed 1-generation builds by 4-10%. Not included.
  (Later included in a different form, see section 2: on Linux x86 without
  PGO it gave +26..49% over the 1-generation build. The PGO result above was
  on the M4 with the earlier version. The new version keeps its gain with
  --profile on the M4, see section 7.)
- Two-generation memo entries padded to 128 bytes and 128-byte aligned (the
  M4's cache line; 88-byte entries straddle two lines most of the time): no
  gain on the M4 (no PGO, -p 10, soups/s: qr4k 14282/11486/16487 ->
  13915/11872/13266, b2-as12 24156/24007/23851 -> 13986/23876/23439, b2e
  6666/6928/7021 -> 4574/6895/4720; the low values are outliers but there's
  no gain either way).
- mimalloc: not tested (it would have to be built from downloaded source).
- Thread count: on the M4 Pro, -p 12..16 is about 6% above -p 10, and no
  further.
