/*
* This file is #included into each outer-totalistic rule namespace.
*/

    uint64_t r32_centre_to_u64(uint32_t* d, int x, int y) {
        // Not written in inline assembly for a change (!)
        uint64_t z = 0;
        for (int i = 11; i >= 4; i--) {
            z = z << 8;
            z |= (d[i+y] >> (12+x)) & 255;
        }
        return z;
    }

    void displaycentre(uint32_t *d) {
        for (uint64_t i = 0; i < 16; i++) {
            for (uint64_t j = 8; j < 24; j++) {
                std::cerr << (((d[i] >> j) & 1) ? '*' : '.');
            }
            std::cerr << std::endl;
        }
        std::cerr << std::endl;
    }

    void iter4_var_leaf(uint64_t * inleaf, uint64_t * centres) {
        /*
        * Find the 8-by-8 centre after iterating a 16-by-16 leaf for a
        * further 4 iterations in the rule.
        */
        int bis = apg::best_instruction_set();
        uint32_t d[16];
        uint32_t e[16];

        if (bis >= 9) {
            apg::z64_to_r32_centre_avx(inleaf, d);
            iterate_avx_16_12(d, e, 0, 0, 0, false);
            iterate_avx_12_8(d+2, e+2, 0, 0, 0, false);
        } else {
            apg::z64_to_r32_centre_ssse3(inleaf, d);
            iterate_sse2_16_12(d, e, 0, 0, 0, false);
            iterate_sse2_12_8(d+2, e+2, 0, 0, 0, false);
        }
        centres[0] = r32_centre_to_u64(d,  0,  0);

    }

    int iterate_var_48_28(uint32_t* d, uint32_t* diffs) {
        uint32_t e[48];
            return iterate_avx512_48_28_monolith(d, e, diffs);
    }

    int iterate_var_48_28(uint32_t* d, uint32_t* h, uint32_t* diffs) {
        uint32_t e[48];
            return iterate_avx512_48_28(d, e, h, 0, diffs, false);
    }

    int iterate_var_48_28(uint32_t* d, uint32_t* h, uint32_t* j, uint32_t* diffs) {
        uint32_t e[48];
            return iterate_avx512_48_28(d, e, h, j, diffs, false);
    }

    int iterate_var_32_28(uint32_t* d, uint32_t* diffs) {
        uint32_t e[32];
            return iterate_avx512_32_28(d, e, 0, 0, diffs, false);
    }

    int iterate_var_32_28(uint32_t* d, uint32_t* h, uint32_t* diffs) {
        uint32_t e[32];
            return iterate_avx512_32_28(d, e, h, 0, diffs, false);
    }

    int iterate_var_32_28(uint32_t* d, uint32_t* h, uint32_t* j, uint32_t* diffs) {
        uint32_t e[32];
            return iterate_avx512_32_28(d, e, h, j, diffs, false);
    }

    bool iterate_var_leaf(int n, uint64_t * inleaves, uint64_t * outleaf) {

        if (n == -4) {
            /*
            * This brings a whole new meaning to 'function overloading'.
            * I'm going to attempt to justify this by claiming that MINUS n
            * corresponds to MINUScule leaf iteration. So, for instance:
            *
            * iterate_var_leaf( 4, ...)  <-- run a 32-by-32 tile 4 gens;
            * iterate_var_leaf(-4, ...)  <-- run a 16-by-16 tile 4 gens;
            */
            iter4_var_leaf(inleaves, outleaf);
            return false;
        }

        bool nochange = false;
        uint32_t d[32];
            apg::z64_to_r32_avx2(inleaves, d);
            nochange = (iterate_var_avx512(n, d) == n);
            apg::r32_centre_to_z64_avx2(d, outleaf);
        return nochange;
    }

    bool iterate_var_leaf(int n, uint64_t * inleaves, uint64_t * hleaves, uint64_t * outleaf) {
        bool nochange = false;
        uint32_t d[32];
        uint32_t h[32];
            apg::z64_to_r32_avx2(inleaves, d);
            apg::z64_to_r32_avx2(hleaves, h);
            nochange = (iterate_var_avx512(n, d, h) == n);
            apg::r32_centre_to_z64_avx2(d, outleaf);
            apg::r32_centre_to_z64_avx2(h, outleaf + 4);
        return nochange;
    }

    bool iterate_var_leaf(int n, uint64_t * inleaves, uint64_t * hleaves, uint64_t * jleaves, uint64_t * outleaf) {
        bool nochange = false;
        uint32_t d[32];
        uint32_t h[32];
        uint32_t j[32];
            apg::z64_to_r32_avx2(inleaves, d);
            apg::z64_to_r32_avx2(jleaves, j);
            apg::z64_to_r32_avx2(hleaves, h);
            nochange = (iterate_var_avx512(n, d, h, j) == n);
            apg::r32_centre_to_z64_avx2(d, outleaf);
            apg::r32_centre_to_z64_avx2(j, outleaf + 8);
            apg::r32_centre_to_z64_avx2(h, outleaf + 4);
        return nochange;
    }

