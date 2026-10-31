
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

#if defined(LIFELIB_ISO_QL)
    /*
    * QuickLife-style kernel (experimental): a 64 KiB table from a 4x4 block
    * (index: row k's 4 cells at bits 4k..4k+3, bit 0 = leftmost column) to
    * its central 2x2 (bits 0-1 the upper row, 2-3 the lower row), built
    * from lut9. Generation 2 of iterate_var_leaf2 runs straight on the
    * 18x18 generation-1 block. (APGsmooshed; see CHANGES.md)
    */
    struct QLTable {
        uint8_t t[65536];
        QLTable() {
            for (uint32_t i = 0; i < 65536; i++) {
                uint32_t v = 0;
                for (int r = 1; r <= 2; r++) {
                    for (int c = 1; c <= 2; c++) {
                        uint64_t x = ((i >> (4 * r - 4 + c - 1)) & 7) | (((i >> (4 * r + c - 1)) & 7) << 6) |
                                     (((uint64_t) ((i >> (4 * r + 4 + c - 1)) & 7)) << 12);
                        v |= ((uint32_t) xfl9(x)) << ((r - 1) * 2 + c - 1);
                    }
                }
                t[i] = v;
            }
        }
    };

    // One generation of rows r0..r0+n-1, columns c0..c0+m-1 (n, m even).
    inline void ql_gen(const uint32_t *d, uint32_t *g, int r0, int n, int c0, int m) {
        static const QLTable T;
        for (int r = r0; r < r0 + n; r += 2) {
            uint32_t a = d[r-1], b = d[r], c = d[r+1], e = d[r+2];
            uint32_t o0 = 0, o1 = 0;
            for (int x = c0; x < c0 + m; x += 2) {
                int sh = x - 1;
                uint32_t v = T.t[((a >> sh) & 15) | (((b >> sh) & 15) << 4) | (((c >> sh) & 15) << 8) | (((e >> sh) & 15) << 12)];
                o0 |= (v & 3) << x; o1 |= ((v >> 2) & 3) << x;
            }
            g[r] = o0; g[r+1] = o1;
        }
    }

    // Rows 8..23, columns 8..23 to the 16x16 Z-order output.
    inline void ql_rows_to_z(const uint32_t *g, uint64_t *outleafx) {
        uint8_t *b = (uint8_t*) outleafx;
        for (int r = 0; r < 16; r++) {
            uint32_t v = g[r + 8] >> 8;
            b[(r >> 3) * 16 + (r & 7)] = v & 0xff;
            b[(r >> 3) * 16 + 8 + (r & 7)] = (v >> 8) & 0xff;
        }
    }

    bool iterate_var_leaf_compute(uint64_t *inleaves, uint64_t *outleafx) {
        uint32_t d[32], g[32];
        apg::z64_to_r32_sse2(inleaves, d);
        ql_gen(d, g, 8, 16, 8, 16);
        ql_rows_to_z(g, outleafx);
        return false;
    }

    bool iterate_var_leaf2_compute(uint64_t *inleaves, uint64_t *outleafx) {
        uint32_t d[32], g1[32], g2[32];
        apg::z64_to_r32_sse2(inleaves, d);
        ql_gen(d, g1, 7, 18, 7, 18);
        ql_gen(g1, g2, 8, 16, 8, 16);
        ql_rows_to_z(g2, outleafx);
        return false;
    }
#elif defined(__ARM_NEON) && !defined(LIFELIB_NO_NEON_ISO)
    /*
    * NEON kernel: one cell per byte. lut9 (64 bytes) sits in four registers
    * for tbl; a cell's 9-bit neighbourhood is split as in xfl9: the rows at
    * and below it (3 + 3 bits) pick a lut9 byte, the row above (3 bits)
    * picks the bit. Grids are 32 rows of GW bytes, column c at byte c + 8.
    * (APGsmooshed; see CHANGES.md)
    */
    static const int NGW = 48;

    // Rows rlo..rhi (all 32 columns) from the Z-ordered input. Tile (tr, tc)
    // is word tc&1 | (tr&1)<<1 | (tc>>1)<<2 | (tr>>1)<<3 (byte = row, bit =
    // column), so tiles 0,1 and 2,3 of a tile row are adjacent word pairs.
    inline void neon_expand(const uint64_t *in, uint8_t *G, int rlo, int rhi) {
        const uint8x16_t bits = {1,2,4,8,16,32,64,128,1,2,4,8,16,32,64,128};
        const uint8x16_t one = vdupq_n_u8(1);
        const uint8x16_t idx0 = {0,0,0,0,0,0,0,0,8,8,8,8,8,8,8,8};
        for (int r = rlo; r <= rhi; r++) {
            int tr = r >> 3;
            const uint64_t *w = in + (((tr & 1) << 1) | ((tr >> 1) << 3));
            uint8x16_t ix = vaddq_u8(idx0, vdupq_n_u8(r & 7));
            uint8x16_t A = vld1q_u8((const uint8_t*) w), B = vld1q_u8((const uint8_t*) (w + 4));
            vst1q_u8(G + r * NGW + 8, vandq_u8(vtstq_u8(vqtbl1q_u8(A, ix), bits), one));
            vst1q_u8(G + r * NGW + 24, vandq_u8(vtstq_u8(vqtbl1q_u8(B, ix), bits), one));
        }
    }

    // Horizontal triples: bit k = column c - 1 + k, for 16 (or 8) columns from c0.
    inline uint8x16_t neon_hrow(const uint8_t *G, int r, int c0) {
        const uint8_t *p = G + r * NGW + 8 + c0;
        uint8x16_t h = vld1q_u8(p - 1);
        h = vsliq_n_u8(h, vld1q_u8(p), 1);
        return vsliq_n_u8(h, vld1q_u8(p + 1), 2);
    }
    inline uint8x8_t neon_hrow8(const uint8_t *G, int r, int c0) {
        const uint8_t *p = G + r * NGW + 8 + c0;
        uint8x8_t h = vld1_u8(p - 1);
        h = vsli_n_u8(h, vld1_u8(p), 1);
        return vsli_n_u8(h, vld1_u8(p + 1), 2);
    }

    // One generation of rows r0..r1, columns c0..c0+15 (neon_gen) or
    // c0..c0+7 (neon_gen8), from G into H.
    inline void neon_gen(const uint8x16x4_t &t, const uint8_t *G, uint8_t *H, int r0, int r1, int c0) {
        uint8x16_t ha = neon_hrow(G, r0 - 1, c0), hb = neon_hrow(G, r0, c0);
        for (int r = r0; r <= r1; r++) {
            uint8x16_t hc = neon_hrow(G, r + 1, c0);
            uint8x16_t x = vqtbl4q_u8(t, vsliq_n_u8(hb, hc, 3));
            vst1q_u8(H + r * NGW + 8 + c0, vandq_u8(vshlq_u8(x, vnegq_s8(vreinterpretq_s8_u8(ha))), vdupq_n_u8(1)));
            ha = hb; hb = hc;
        }
    }
    inline void neon_gen8(const uint8x16x4_t &t, const uint8_t *G, uint8_t *H, int r0, int r1, int c0) {
        uint8x8_t ha = neon_hrow8(G, r0 - 1, c0), hb = neon_hrow8(G, r0, c0);
        for (int r = r0; r <= r1; r++) {
            uint8x8_t hc = neon_hrow8(G, r + 1, c0);
            uint8x8_t x = vqtbl4_u8(t, vsli_n_u8(hb, hc, 3));
            vst1_u8(H + r * NGW + 8 + c0, vand_u8(vshl_u8(x, vneg_s8(vreinterpret_s8_u8(ha))), vdup_n_u8(1)));
            ha = hb; hb = hc;
        }
    }

    // Rows/columns 8..23 of G to the 16x16 Z-order output: shift each cell to
    // its bit, then a vpaddq tree sums 8 rows into (row lo, row hi) byte pairs.
    inline void neon_pack(const uint8_t *G, uint64_t *outleafx) {
        const int8x16_t sh = {0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7};
        const uint8x16_t perm = {0,2,4,6,8,10,12,14,1,3,5,7,9,11,13,15};
        for (int b = 0; b < 2; b++) {
            uint8x16_t v[8];
            for (int k = 0; k < 8; k++) { v[k] = vshlq_u8(vld1q_u8(G + (8 + 8 * b + k) * NGW + 16), sh); }
            uint8x16_t p0 = vpaddq_u8(v[0], v[1]), p1 = vpaddq_u8(v[2], v[3]);
            uint8x16_t p2 = vpaddq_u8(v[4], v[5]), p3 = vpaddq_u8(v[6], v[7]);
            uint8x16_t z = vpaddq_u8(vpaddq_u8(p0, p1), vpaddq_u8(p2, p3));
            vst1q_u8((uint8_t*) (outleafx + 2 * b), vqtbl1q_u8(z, perm));
        }
    }

    bool iterate_var_leaf_compute(uint64_t *inleaves, uint64_t *outleafx) {
        uint8x16x4_t t = vld1q_u8_x4(lut9);
        alignas(16) uint8_t G[32 * NGW], H[32 * NGW];
        neon_expand(inleaves, G, 7, 24);
        neon_gen(t, G, H, 8, 23, 8);
        neon_pack(H, outleafx);
        return false;
    }

    // Generation 1 on rows/columns 7..24 (columns 7..22 and 17..24), then
    // generation 2 on 8..23.
    bool iterate_var_leaf2_compute(uint64_t *inleaves, uint64_t *outleafx) {
        uint8x16x4_t t = vld1q_u8_x4(lut9);
        alignas(16) uint8_t G[32 * NGW], H[32 * NGW], J[32 * NGW];
        neon_expand(inleaves, G, 6, 25);
        neon_gen(t, G, H, 7, 24, 7);
        neon_gen8(t, G, H, 7, 24, 17);
        neon_gen(t, H, J, 8, 23, 8);
        neon_pack(J, outleafx);
        return false;
    }
#else
    bool iterate_var_leaf_compute(uint64_t *inleaves, uint64_t *outleafx) {
        uint32_t d[32];
        apg::z64_to_r32_sse2(inleaves, d);
        return iterate_var_leaf_rows(d, outleafx);
    }
#endif

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

#if !defined(LIFELIB_ISO_QL) && !(defined(__ARM_NEON) && !defined(LIFELIB_NO_NEON_ISO))
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

#endif

    // Per-rule choice between 1- and 2-generation tile steps (APGsmooshed;
    // see CHANGES.md): each thread counts hits of the memo below over its
    // first WARM calls; below MIN_HIT percent, upattern goes back to
    // 1-generation steps for that thread (twogen_ok()). Both paths are exact.
    // Only the lut24 kernel gains from this: its misses are memory-bound. On
    // the NEON and QuickLife kernels the default is 0, which compiles it out.
    #ifndef APG_TWOGEN_MIN_HIT
    #if defined(LIFELIB_ISO_QL) || (defined(__ARM_NEON) && !defined(LIFELIB_NO_NEON_ISO))
    #define APG_TWOGEN_MIN_HIT 0
    #else
    #define APG_TWOGEN_MIN_HIT 85
    #endif
    #endif

    // Memo for iterate_var_leaf2, keyed on the 20x20 window (400 bits, 7
    // words). Same scheme as LeafMemo. (APGsmooshed; see CHANGES.md)
    struct LeafMemo2 {
        static const uint64_t BITS = 14;
        static const uint64_t WARM = 1ull << 20;
        struct Entry { uint64_t key[7]; uint64_t val[4]; };
        Entry* t = 0;
        uint64_t calls = 0, misses = 0;
        bool enabled = true;
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
        LeafMemo2 &m = leafmemo2;
        LeafMemo2::Entry &e = m.get()[h >> (64 - LeafMemo2::BITS)];
        bool hit = (e.key[0] == k[0]) & (e.key[1] == k[1]) & (e.key[2] == k[2]) &
                   (e.key[3] == k[3]) & (e.key[4] == k[4]) & (e.key[5] == k[5]) &
                   (e.key[6] == k[6]);
    #if APG_TWOGEN_MIN_HIT > 0
        if (m.calls < LeafMemo2::WARM) {
            m.calls += 1; m.misses += (hit ? 0 : 1);
            if (m.calls == LeafMemo2::WARM) {
                m.enabled = (100 * (LeafMemo2::WARM - m.misses) >= APG_TWOGEN_MIN_HIT * LeafMemo2::WARM);
            }
        }
    #endif
        if (hit) {
            std::memcpy(outleafx, e.val, 32);
            return false;
        }
        bool r = iterate_var_leaf2_compute(inleaves, outleafx);
        std::memcpy(e.key, k, 56);
        std::memcpy(e.val, outleafx, 32);
        return r;
    }

    // Whether upattern should use 2-generation steps in this thread.
    #if APG_TWOGEN_MIN_HIT > 0
    bool twogen_ok() { return leafmemo2.enabled; }
    #else
    bool twogen_ok() { return true; }
    #endif
