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
  (routine hashes), `t.cpp` and the generated headers it needs. Upstream's
  `test/minstrels.rle` moved here.
- Removed: `.gitlab-ci.yml` files (upstream CI), `.gitmodules`.


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
- lifetree memo maps (`std::map`) as `std::unordered_map` -10/-6/+1%, or as a
  flat table with inline slots +1/-1/-5%.
- Caching classification of multi-tile clusters: repeats were 60% of calls but
  0.58 s of 12.4 s; unique clusters dominate.
- Two-generation tile steps for isotropic rules: exact (casim and census
  checks), +12..+40% without PGO but -12..+16% with PGO on both sides, and the
  extra code slowed 1-generation builds by 4-10%. Not included.
- mimalloc: not tested (it would have to be built from downloaded source).
- Thread count: on the M4 Pro, -p 12..16 is about 6% above -p 10, and no
  further.
