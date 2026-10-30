
    uint8_t* lut18 = 0;
    uint8_t* lut24 = 0;
    uint32_t* mix18 = 0;

    uint8_t xfl9(uint64_t i) {
        return ((lut9[((i >> 6) & 7) | ((i >> 9) & 56)] >> (i & 7)) & 1);
    }

    void assemble_lut18() {
        lut18 = (uint8_t*) std::malloc(0x40000); // don't bother freeing; the OS will do that.
        for (uint64_t i = 0; i < 0x40000; i++) {
            lut18[i] = xfl9(i) | (xfl9(i >> 1) << 1) | (xfl9(i >> 2) << 2) | (xfl9(i >> 3) << 3);
        }
    }

    void assemble_lut24() {
        if (lut18 == 0) { assemble_lut18(); }
        lut24 = (uint8_t*) std::malloc(0x1000000); // don't bother freeing; the OS will do that.
        for (uint64_t i = 0; i < 0x1000000; i++) {
            lut24[i] = lut18[i & 0x3ffff] | (lut18[i >> 6] << 4);
        }
    }

    void assemble_mix18() {
        mix18 = (uint32_t*) std::malloc(0x100000); // see above.
        for (uint64_t i = 0; i < 0x40000; i++) {
            mix18[i] = (i & 0x3f) | (((i >> 4) & 0x3f) << 8) | (((i >> 8) & 0x3f) << 16) | (((i >> 12) & 0x3f) << 24);
        }
    }

    bool iterate_var_leaf_compute(uint64_t *inleaves, uint64_t *outleafx);

    /*
    * Per-thread memo of iterate_var_leaf: direct-mapped, keyed on the full
    * central 18x18 input it reads, so results are exact. Empty slots hold the
    * real result for the all-zero input (correct for B0 too).
    * (APGsmooshed; see CHANGES.md)
    */
    struct LeafMemo {
        // Key: the central 18x18 cells (rows 7..24, bits 7..24 of the row
        // form), the only input iterate_var_leaf_compute reads.
        static const uint64_t BITS = 14;
        struct Entry { uint64_t key[6]; uint64_t val[4]; };
        Entry* t = 0;
        Entry* get() {
            if (t == 0) {
                t = (Entry*) std::malloc(sizeof(Entry) << BITS);
                uint64_t zin[16] = {0}; Entry z; std::memset(&z, 0, sizeof z);
                iterate_var_leaf_compute(zin, z.val);
                for (uint64_t i = 0; i < (1ull << BITS); i++) { t[i] = z; }
            }
            return t;
        }
        ~LeafMemo() { std::free(t); }
    };
    thread_local LeafMemo leafmemo;

    bool iterate_var_leaf(uint64_t *inleaves, uint64_t *outleafx) {
        // Key read straight from the Z-ordered input, without transposing.
        // inleaves[w] is the 8x8 tile at tile row/column (w>>1&1 | w>>2&2,
        // w&1 | w>>1&2); byte i = row i, bit j = column j. The 18x18 window
        // (rows/columns 7..24) is tiles 3, 6, 9, 12 whole, row 7 of 1 and 4,
        // row 0 of 11 and 14, column 7 of 2 and 8, column 0 of 7 and 13 and
        // one corner bit of 0, 5, 10, 15. All 324 bits land on distinct key
        // bits, so the key is exactly the 18x18 window, as before.
        // (APGsmooshed; see CHANGES.md)
        const uint64_t *w = inleaves;
        const uint64_t c7 = 0x8080808080808080ull, c0 = 0x0101010101010101ull;
        uint64_t k[6];
        k[0] = w[3]; k[1] = w[6]; k[2] = w[9]; k[3] = w[12];
        k[4] = (w[2] & c7) | ((w[8] & c7) >> 1) | (w[7] & c0) | ((w[13] & c0) << 1);
        k[5] = (w[1] >> 56) | ((w[4] >> 56) << 8) | ((w[11] & 0xff) << 16) |
               ((w[14] & 0xff) << 24) | ((w[0] >> 63) << 32) |
               (((w[5] >> 56) & 1) << 33) | (((w[10] >> 7) & 1) << 34) |
               ((w[15] & 1) << 35);
        uint64_t h = 0;
        for (int j = 0; j < 6; j++) { h += k[j] * (0x9E3779B97F4A7C15ull + 2 * j); }
        h ^= h >> 31; h *= 0xff51afd7ed558ccdull; h ^= h >> 29;
        LeafMemo::Entry &e = leafmemo.get()[h >> (64 - LeafMemo::BITS)];
        if ((e.key[0] == k[0]) & (e.key[1] == k[1]) & (e.key[2] == k[2]) &
            (e.key[3] == k[3]) & (e.key[4] == k[4]) & (e.key[5] == k[5])) {
            std::memcpy(outleafx, e.val, 32);
            return false;
        }
        bool r = iterate_var_leaf_compute(inleaves, outleafx);
        std::memcpy(e.key, k, 48);
        std::memcpy(e.val, outleafx, 32);
        return r;
    }

    bool iterate_var_leaf_rows(const uint32_t *d, uint64_t *outleafx);

    bool iterate_var_leaf_compute(uint64_t *inleaves, uint64_t *outleafx) {
        uint32_t d[32];
        apg::z64_to_r32_sse2(inleaves, d);
        return iterate_var_leaf_rows(d, outleafx);
    }

    /*
    * One generation from the 32 rows d (only rows 7..24, bits 7..24 are
    * read) to the central 16x16 in Z order. Split out of
    * iterate_var_leaf_compute so iterate_var_leaf2 can feed shifted rows.
    * (APGsmooshed; see CHANGES.md)
    */
    bool iterate_var_leaf_rows(const uint32_t *d, uint64_t *outleafx) {

        if (lut24 == 0) { assemble_lut24(); }
        if (mix18 == 0) { assemble_mix18(); }

        uint32_t e[32];
        uint32_t f[32];
        uint8_t  g[32];

        for (uint64_t i = 7; i < 25; i++) { f[i] = mix18[(d[i] >> 7) & 0x3ffff]; }

#ifndef LIFELIB_PORTABLE
        asm (
        // Load data from memory into registers:
        "movups (%1), %%xmm1 \n\t"
        "movups 16(%1), %%xmm2 \n\t"
        "movups 32(%1), %%xmm3 \n\t"

        "movups 28(%0), %%xmm0 \n\t"
        "pshufb %%xmm1, %%xmm0 \n\t"
        "movdqa %%xmm2, %%xmm4 \n\t"
        "pandn %%xmm0, %%xmm4 \n\t"
        "psrld $2, %%xmm4 \n\t"
        "pand %%xmm2, %%xmm0 \n\t"
        "por %%xmm4, %%xmm0 \n\t"
        "movdqa %%xmm3, %%xmm4 \n\t"
        "pandn %%xmm0, %%xmm4 \n\t"
        "psrld $4, %%xmm4 \n\t"
        "pand %%xmm3, %%xmm0 \n\t"
        "por %%xmm4, %%xmm0 \n\t"
        "movups %%xmm0, (%2) \n\t"

        "movups 36(%0), %%xmm0 \n\t"
        "pshufb %%xmm1, %%xmm0 \n\t"
        "movdqa %%xmm2, %%xmm4 \n\t"
        "pandn %%xmm0, %%xmm4 \n\t"
        "psrld $2, %%xmm4 \n\t"
        "pand %%xmm2, %%xmm0 \n\t"
        "por %%xmm4, %%xmm0 \n\t"
        "movdqa %%xmm3, %%xmm4 \n\t"
        "pandn %%xmm0, %%xmm4 \n\t"
        "psrld $4, %%xmm4 \n\t"
        "pand %%xmm3, %%xmm0 \n\t"
        "por %%xmm4, %%xmm0 \n\t"
        "movups %%xmm0, 16(%2) \n\t"

        "movups 44(%0), %%xmm0 \n\t"
        "pshufb %%xmm1, %%xmm0 \n\t"
        "movdqa %%xmm2, %%xmm4 \n\t"
        "pandn %%xmm0, %%xmm4 \n\t"
        "psrld $2, %%xmm4 \n\t"
        "pand %%xmm2, %%xmm0 \n\t"
        "por %%xmm4, %%xmm0 \n\t"
        "movdqa %%xmm3, %%xmm4 \n\t"
        "pandn %%xmm0, %%xmm4 \n\t"
        "psrld $4, %%xmm4 \n\t"
        "pand %%xmm3, %%xmm0 \n\t"
        "por %%xmm4, %%xmm0 \n\t"
        "movups %%xmm0, 32(%2) \n\t"

        "movups 52(%0), %%xmm0 \n\t"
        "pshufb %%xmm1, %%xmm0 \n\t"
        "movdqa %%xmm2, %%xmm4 \n\t"
        "pandn %%xmm0, %%xmm4 \n\t"
        "psrld $2, %%xmm4 \n\t"
        "pand %%xmm2, %%xmm0 \n\t"
        "por %%xmm4, %%xmm0 \n\t"
        "movdqa %%xmm3, %%xmm4 \n\t"
        "pandn %%xmm0, %%xmm4 \n\t"
        "psrld $4, %%xmm4 \n\t"
        "pand %%xmm3, %%xmm0 \n\t"
        "por %%xmm4, %%xmm0 \n\t"
        "movups %%xmm0, 48(%2) \n\t"

        "movups 60(%0), %%xmm0 \n\t"
        "pshufb %%xmm1, %%xmm0 \n\t"
        "movdqa %%xmm2, %%xmm4 \n\t"
        "pandn %%xmm0, %%xmm4 \n\t"
        "psrld $2, %%xmm4 \n\t"
        "pand %%xmm2, %%xmm0 \n\t"
        "por %%xmm4, %%xmm0 \n\t"
        "movdqa %%xmm3, %%xmm4 \n\t"
        "pandn %%xmm0, %%xmm4 \n\t"
        "psrld $4, %%xmm4 \n\t"
        "pand %%xmm3, %%xmm0 \n\t"
        "por %%xmm4, %%xmm0 \n\t"
        "movups %%xmm0, 64(%2) \n\t"

        "movups 68(%0), %%xmm0 \n\t"
        "pshufb %%xmm1, %%xmm0 \n\t"
        "movdqa %%xmm2, %%xmm4 \n\t"
        "pandn %%xmm0, %%xmm4 \n\t"
        "psrld $2, %%xmm4 \n\t"
        "pand %%xmm2, %%xmm0 \n\t"
        "por %%xmm4, %%xmm0 \n\t"
        "movdqa %%xmm3, %%xmm4 \n\t"
        "pandn %%xmm0, %%xmm4 \n\t"
        "psrld $4, %%xmm4 \n\t"
        "pand %%xmm3, %%xmm0 \n\t"
        "por %%xmm4, %%xmm0 \n\t"
        "movups %%xmm0, 80(%2) \n\t"

        "movups 76(%0), %%xmm0 \n\t"
        "pshufb %%xmm1, %%xmm0 \n\t"
        "movdqa %%xmm2, %%xmm4 \n\t"
        "pandn %%xmm0, %%xmm4 \n\t"
        "psrld $2, %%xmm4 \n\t"
        "pand %%xmm2, %%xmm0 \n\t"
        "por %%xmm4, %%xmm0 \n\t"
        "movdqa %%xmm3, %%xmm4 \n\t"
        "pandn %%xmm0, %%xmm4 \n\t"
        "psrld $4, %%xmm4 \n\t"
        "pand %%xmm3, %%xmm0 \n\t"
        "por %%xmm4, %%xmm0 \n\t"
        "movups %%xmm0, 96(%2) \n\t"

        "movups 84(%0), %%xmm0 \n\t"
        "pshufb %%xmm1, %%xmm0 \n\t"
        "movdqa %%xmm2, %%xmm4 \n\t"
        "pandn %%xmm0, %%xmm4 \n\t"
        "psrld $2, %%xmm4 \n\t"
        "pand %%xmm2, %%xmm0 \n\t"
        "por %%xmm4, %%xmm0 \n\t"
        "movdqa %%xmm3, %%xmm4 \n\t"
        "pandn %%xmm0, %%xmm4 \n\t"
        "psrld $4, %%xmm4 \n\t"
        "pand %%xmm3, %%xmm0 \n\t"
        "por %%xmm4, %%xmm0 \n\t"
        "movups %%xmm0, 112(%2) \n\t"

        : /* no output operands -- implicitly volatile */
        : "r" (f), "r" (lut9 + 64), "r" (e)
        : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "memory" );
#else
        apg::portable::isolut_mix(f, lut9 + 64, e);
#endif

        for (uint64_t i = 0; i < 32; i++) { g[i] = lut24[e[i]]; }
        std::memcpy(outleafx, g, 32);

        for (uint64_t i = 0; i < 4; i++) {
            outleafx[i] = (outleafx[i] & 0xf00ff00ff00ff00full) |
                         ((outleafx[i] & 0x00f000f000f000f0ull) << 4) |
                         ((outleafx[i] & 0x0f000f000f000f00ull) >> 4);
            outleafx[i] = (outleafx[i] & 0xffff00000000ffffull) |
                         ((outleafx[i] & 0x00000000ffff0000ull) << 16) |
                         ((outleafx[i] & 0x0000ffff00000000ull) >> 16);
        }

#ifndef LIFELIB_PORTABLE
        asm (
        "movups (%0), %%xmm0 \n\t"
        "shufps $216, %%xmm0, %%xmm0 \n\t"
        "movups %%xmm0, (%0) \n\t"
        "movups 16(%0), %%xmm0 \n\t"
        "shufps $216, %%xmm0, %%xmm0 \n\t"
        "movups %%xmm0, 16(%0) \n\t"
        : /* no output operands -- implicitly volatile */
        : "r" (outleafx)
        : "xmm0", "memory" );
#else
        apg::portable::isolut_shuf216(outleafx);
#endif

        return false;
    }

    /*
    * Two generations at once (mantissa 2): 20x20 input window (rows/columns
    * 6..25) to the central 16x16. Generation 1 is needed on rows/columns
    * 7..24. Two one-generation blocks on inputs shifted diagonally give rows
    * and columns 7..22 and 9..24 (they overlap and agree); the 8 cells they
    * miss (rows 7-8 x columns 23-24, rows 23-24 x columns 7-8) come from xfl9,
    * the same lut9 lookup the kernel's tables are built from. Generation 2 is
    * one more call on those rows. (APGsmooshed; see CHANGES.md)
    */
    bool iterate_var_leaf2_compute(uint64_t *inleaves, uint64_t *outleafx) {
        uint32_t d[32];
        apg::z64_to_r32_sse2(inleaves, d);
        uint32_t g1[32] = {0};
        for (int k = -1; k <= 1; k += 2) {
            // block shifted by (dy, dx) = (k, k)
            uint32_t s[32];
            for (int i = 7; i < 25; i++) {
                s[i] = (k > 0) ? (d[i + k] >> 1) : (d[i + k] << 1);
            }
            uint64_t o[4];
            iterate_var_leaf_rows(s, o);
            for (int r = 0; r < 16; r++) {
                uint32_t lo = (o[(r >> 3) * 2] >> ((r & 7) * 8)) & 0xff;
                uint32_t hi = (o[(r >> 3) * 2 + 1] >> ((r & 7) * 8)) & 0xff;
                g1[r + 8 + k] |= (lo | (hi << 8)) << (8 + k);
            }
        }
        static const int cr[8] = {7, 7, 8, 8, 23, 23, 24, 24};
        static const int cc[8] = {23, 24, 23, 24, 7, 8, 7, 8};
        for (int j = 0; j < 8; j++) {
            int r = cr[j], c = cc[j];
            uint64_t i = ((d[r - 1] >> (c - 1)) & 7) | (((uint64_t) ((d[r] >> (c - 1)) & 7)) << 6) |
                         (((uint64_t) ((d[r + 1] >> (c - 1)) & 7)) << 12);
            g1[r] |= ((uint32_t) xfl9(i)) << c;
        }
        return iterate_var_leaf_rows(g1, outleafx);
    }

    // Memo for iterate_var_leaf2, keyed on the 20x20 window (400 bits, 7
    // words). Same scheme as LeafMemo. (APGsmooshed; see CHANGES.md)
    struct LeafMemo2 {
        static const uint64_t BITS = 14;
        struct Entry { uint64_t key[7]; uint64_t val[4]; };
        Entry* t = 0;
        Entry* get() {
            if (t == 0) {
                t = (Entry*) std::malloc(sizeof(Entry) << BITS);
                uint64_t zin[16] = {0}; Entry z; std::memset(&z, 0, sizeof z);
                iterate_var_leaf2_compute(zin, z.val);
                for (uint64_t i = 0; i < (1ull << BITS); i++) { t[i] = z; }
            }
            return t;
        }
        ~LeafMemo2() { std::free(t); }
    };
    thread_local LeafMemo2 leafmemo2;

    bool iterate_var_leaf2(uint64_t *inleaves, uint64_t *outleafx) {
        // Window rows/columns 6..25: tiles 3, 6, 9, 12 whole; rows 6-7 of 1
        // and 4; rows 0-1 of 11 and 14; columns 6-7 of 2 and 8; columns 0-1 of
        // 7 and 13; a 2x2 corner of 0, 5, 10, 15 (tile layout as in
        // iterate_var_leaf). All 400 bits land on distinct key bits.
        const uint64_t *w = inleaves;
        const uint64_t c67 = 0xc0c0c0c0c0c0c0c0ull, c01 = 0x0303030303030303ull;
        uint64_t k[7];
        k[0] = w[3]; k[1] = w[6]; k[2] = w[9]; k[3] = w[12];
        k[4] = (w[2] & c67) | ((w[8] & c67) >> 2) | (w[7] & c01) | ((w[13] & c01) << 2);
        k[5] = (w[1] >> 48) | ((w[4] >> 48) << 16) | ((w[11] & 0xffff) << 32) |
               ((w[14] & 0xffff) << 48);
        k[6] = ((w[0] >> 48) & 0xc0c0) | ((w[5] >> 48) & 0x0303) |
               ((w[10] & 0xc0c0) << 16) | ((w[15] & 0x0303) << 16);
        uint64_t h = 0;
        for (int j = 0; j < 7; j++) { h += k[j] * (0x9E3779B97F4A7C15ull + 2 * j); }
        h ^= h >> 31; h *= 0xff51afd7ed558ccdull; h ^= h >> 29;
        LeafMemo2::Entry &e = leafmemo2.get()[h >> (64 - LeafMemo2::BITS)];
        if ((e.key[0] == k[0]) & (e.key[1] == k[1]) & (e.key[2] == k[2]) &
            (e.key[3] == k[3]) & (e.key[4] == k[4]) & (e.key[5] == k[5]) &
            (e.key[6] == k[6])) {
            std::memcpy(outleafx, e.val, 32);
            return false;
        }
        bool r = iterate_var_leaf2_compute(inleaves, outleafx);
        std::memcpy(e.key, k, 56);
        std::memcpy(e.val, outleafx, 32);
        return r;
    }
