
#pragma once
#include <stdint.h>
#include <cstring>
#include "lifeperm.h"
#include "lifelogic/iterators_all.h"

namespace apg {

    uint64_t z64_centre_to_u64(uint64_t * inleaf, int x, int y) {
        /*
        * Provided this is inlined and x, y are compile-time constants,
        * this should just involve 6 shifts, 3 ORs, and 2 ANDs:
        */
        int xs = 4 + x;
        int ys = (4 + y) << 3;
        uint64_t bitmask = (0x0101010101010101ull << xs) - 0x0101010101010101ull;
        uint64_t left  = (inleaf[0] >> ys) | (inleaf[2] << (64 - ys));
        uint64_t right = (inleaf[1] >> ys) | (inleaf[3] << (64 - ys));
        uint64_t result = ((right & bitmask) << (8 - xs)) | ((left & (~bitmask)) >> xs);
        return result;
    }

    uint64_t determine_direction(int rule, uint64_t * inleaf) {

        uint64_t centre;
        iterate_var_leaf(rule, -4, inleaf, &centre);

        uint64_t dmap = 0;
        dmap |= ((centre == z64_centre_to_u64(inleaf, -1, -1)) ?   1 : 0); // SE
        dmap |= ((centre == z64_centre_to_u64(inleaf,  0, -2)) ?   2 : 0); // S
        dmap |= ((centre == z64_centre_to_u64(inleaf,  1, -1)) ?   4 : 0); // SW
        dmap |= ((centre == z64_centre_to_u64(inleaf,  2,  0)) ?   8 : 0); // W
        dmap |= ((centre == z64_centre_to_u64(inleaf,  1,  1)) ?  16 : 0); // NW
        dmap |= ((centre == z64_centre_to_u64(inleaf,  0,  2)) ?  32 : 0); // N
        dmap |= ((centre == z64_centre_to_u64(inleaf, -1,  1)) ?  64 : 0); // NE
        dmap |= ((centre == z64_centre_to_u64(inleaf, -2,  0)) ? 128 : 0); // E

        uint64_t lmask = 0;

        if (centre) {
            if (dmap & 170) {
                lmask |= 3;
            }
            if (dmap & 85) {
                lmask |= 7;
            }
            // if (dmap) { std::cerr << centre << " " << dmap << std::endl; }
        }

        // Use a uint64 as an ordered pair of uint32s:
        return (dmap | (lmask << 32));

    }    

}
