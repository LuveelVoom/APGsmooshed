// Leaf memo check (APGsmooshed; see CHANGES.md): iterate_var_leaf (memo) must
// equal iterate_var_leaf_compute (no memo). Inputs come from a small pool with
// cells outside the central 18x18 randomised, so the memo hits; sometimes a
// random cell anywhere is flipped, so window changes must miss.
// Then the two-generation leaf against a scalar reference.
// Prints the mismatch count; 0 is a pass.
#include "lifelib/avxlife/lifeperm.h"
#include "lifelib/avxlife/lifelogic/iterators_b3-qr4k5cek6cs23-a4er56aek.h"
#include <cstdio>
#include <cstring>
static uint64_t s = 88172645463325252ull;
static uint64_t rnd() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return s; }
static int refcell(const uint8_t* lut9, const uint32_t* d, int r, int c) {
    uint32_t A = (d[r-1] >> (c-1)) & 7, B = (d[r] >> (c-1)) & 7, C = (d[r+1] >> (c-1)) & 7;
    return (lut9[B | (C << 3)] >> A) & 1;
}
int main() {
    namespace K = b3_qr4k5cek6cs23_a4er56aek;
    uint64_t pool[64][16];
    for (int p = 0; p < 64; p++) for (int i = 0; i < 16; i++) pool[p][i] = (p & 1) ? (rnd() & rnd()) : rnd();
    long mism = 0;
    for (long it = 0; it < 2000000; it++) {
        uint64_t a[16], o1[4], o2[4];
        std::memcpy(a, pool[rnd() % 64], sizeof a);
        // Outside the window (rows/columns 7..24) only; tile layout as in isoluts.h.
        uint64_t r = rnd();
        if (r & 1) { a[0] ^= rnd() >> 1; a[5] ^= rnd() & ~(1ull << 56); a[10] ^= rnd() & ~0x80ull; a[15] ^= rnd() & ~1ull; }
        if (r & 2) { a[1] ^= rnd() >> 8; a[4] ^= rnd() >> 8; a[11] ^= rnd() << 8; a[14] ^= rnd() << 8; }
        if (r & 4) { a[2] ^= rnd() & 0x7f7f7f7f7f7f7f7full; a[8] ^= rnd() & 0x7f7f7f7f7f7f7f7full;
                     a[7] ^= rnd() & 0xfefefefefefefefeull; a[13] ^= rnd() & 0xfefefefefefefefeull; }
        if ((r & 0xf0) == 0) { a[rnd() % 16] ^= 1ull << (rnd() % 64); }
        K::iterate_var_leaf(a, o1);
        K::iterate_var_leaf_compute(a, o2);
        if (std::memcmp(o1, o2, 32)) { mism++; }
    }
    // Two-generation leaf (memo and uncached) vs two generations of a scalar
    // reference built from lut9 (cell(r,c) from rows r-1..r+1, columns c-1..c+1).
    for (long it = 0; it < 200000; it++) {
        uint64_t a[16], o1[4], o2[4]; uint32_t d[32], g1[32] = {0};
        if (it & 1) { for (int i = 0; i < 16; i++) a[i] = (it & 2) ? rnd() : (rnd() & rnd() & rnd()); }
        else { std::memcpy(a, pool[rnd() % 64], sizeof a); a[0] ^= rnd() & 0x3f3f3f3f3f3f3f3full; }
        K::iterate_var_leaf2(a, o1);
        K::iterate_var_leaf2_compute(a, o2);
        apg::z64_to_r32_sse2(a, d);
        for (int r = 7; r <= 24; r++) for (int c = 7; c <= 24; c++) g1[r] |= (uint32_t) refcell(K::lut9, d, r, c) << c;
        for (int r = 0; r < 16; r++) for (int c = 0; c < 16; c++) {
            int ref = refcell(K::lut9, g1, r + 8, c + 8);
            int w = (r >> 3) * 2 + (c >> 3), b = (r & 7) * 8 + (c & 7);
            if ((int) ((o1[w] >> b) & 1) != ref) { mism++; }
            if ((int) ((o2[w] >> b) & 1) != ref) { mism++; }
        }
    }
    std::printf("%ld\n", mism);
    return mism != 0;
}
