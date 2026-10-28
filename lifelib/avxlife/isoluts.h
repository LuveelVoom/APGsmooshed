
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
        uint32_t d[32];
        apg::z64_to_r32_sse2(inleaves, d);
        uint64_t k[6];
        for (int j = 0; j < 6; j++) {
            k[j] = ((uint64_t) ((d[7+3*j] >> 7) & 0x3ffff)) |
                   (((uint64_t) ((d[8+3*j] >> 7) & 0x3ffff)) << 18) |
                   (((uint64_t) ((d[9+3*j] >> 7) & 0x3ffff)) << 36);
        }
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

    bool iterate_var_leaf_compute(uint64_t *inleaves, uint64_t *outleafx) {

        if (lut24 == 0) { assemble_lut24(); }
        if (mix18 == 0) { assemble_mix18(); }

        uint32_t d[32];
        uint32_t e[32];
        uint32_t f[32];
        uint8_t  g[32];
        apg::z64_to_r32_sse2(inleaves, d);

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
