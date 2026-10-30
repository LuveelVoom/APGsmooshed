// Incremental totalHash vs the full sweep (APGsmooshed; see CHANGES.md), with
// cells inserted into fresh tiles and patterns advanced between calls. Built
// for the current rule by kat.sh. Prints mismatches; 0 is a pass.
#ifdef __AVX512F__
#define LONG_TILES 1
#endif
#include "includes/base.h"
#include <cstdio>
static uint64_t s = 88172645463325252ull;
static uint64_t rnd() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return s; }
int main() {
    long bad = 0, calls = 0, newnz = 0;
    for (int trial = 0; trial < 300; trial++) {
        UPATTERN pat;
        for (int k = 0; k < 4; k++) pat.emplace_uint64(0, 8 * (k & 1), 8 * (k >> 1), rnd() & rnd());
        for (int step = 0; step < 60; step++) {
            pat.advance(0, 0, 1 + (rnd() % 7));
            uint32_t before = pat.tiles.size();
            int ins = rnd() % 3;
            for (int j = 0; j < ins; j++) {
                // up to ~ +-1600 cells away: inside and outside radius 120 tiles
                int64_t x = (int64_t) (rnd() % 3200) - 1600, y = (int64_t) (rnd() % 3200) - 1600;
                pat.emplace_uint64(0, x, y, rnd() & rnd());
            }
            uint32_t after = pat.tiles.size();
            uint64_t a = pat.totalHash(120);
            uint64_t b = pat.totalHash(120, std::false_type());
            calls++; if (a != b) bad++;
            if (after > before) newnz++;
        }
    }
    printf("hashtest: %ld mismatches in %ld calls (%ld calls after inserting into new tiles)\n", bad, calls, newnz);
    return bad != 0;
}
