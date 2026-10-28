#pragma once

#include "iterators.h"
#include <iostream>

#define ZERO_SIX_TILES  multiverse[threadnum + 128] = 0; multiverse[threadnum + 192] = 0; \
                        multiverse[threadnum + 256] = 0; multiverse[threadnum + 320] = 0; \
                        multiverse[threadnum + 384] = 0; multiverse[threadnum + 448] = 0; threadnum += 384

#define CUDABITREVERSE(c, b)  c = (b >> 32) | (b << 32); \
                        c = ((c & 0xffff0000ffff0000ull) >> 16) | ((c & 0x0000ffff0000ffffull) << 16); \
                        c = ((c & 0xff00ff00ff00ff00ull) >>  8) | ((c & 0x00ff00ff00ff00ffull) <<  8); \
                        c = ((c & 0xf0f0f0f0f0f0f0f0ull) >>  4) | ((c & 0x0f0f0f0f0f0f0f0full) <<  4); \
                        c = ((c & 0xccccccccccccccccull) >>  2) | ((c & 0x3333333333333333ull) <<  2); \
                        c = ((c & 0xaaaaaaaaaaaaaaaaull) >>  1) | ((c & 0x5555555555555555ull) <<  1)

__global__ void copyhashes_C1(uint64_cu *multiverse, uint32_cu *univec, uint64_cu *hashes, uint32_cu offset, bool initial) {

    #include "cphash_header.h"

    // Copy SHA-256 hash:
    if ((threadIdx.x >= 24) && (threadIdx.x < 40)) {
        b = hashes[(hashnum << 2) + (threadIdx.x >> 2) - 6];
        b = (b >> (16 * (threadIdx.x & 3)));
        b = ((b & 0x00ff) << 32) | ((b & 0xff00) << 16);
    }

    #ifdef SKIP_18_GENS
    // Advance by 18 generations:
    __shared__ uint64_cu tmp1[64];
    __shared__ uint64_cu tmp2[64];
    uint32_cu u = (threadIdx.x +  1) & 63;
    uint32_cu d = (threadIdx.x + 63) & 63;

    ADVANCE_TILE_64(b, b, tmp1, tmp2)
    ADVANCE_TILE_64(b, b, tmp1, tmp2)
    ADVANCE_TILE_64(b, b, tmp1, tmp2)
    ADVANCE_TILE_64(b, b, tmp1, tmp2)
    ADVANCE_TILE_64(b, b, tmp1, tmp2)
    ADVANCE_TILE_64(b, b, tmp1, tmp2)

    ADVANCE_TILE_64(b, b, tmp1, tmp2)
    ADVANCE_TILE_64(b, b, tmp1, tmp2)
    ADVANCE_TILE_64(b, b, tmp1, tmp2)
    ADVANCE_TILE_64(b, b, tmp1, tmp2)
    ADVANCE_TILE_64(b, b, tmp1, tmp2)
    ADVANCE_TILE_64(b, b, tmp1, tmp2)

    ADVANCE_TILE_64(b, b, tmp1, tmp2)
    ADVANCE_TILE_64(b, b, tmp1, tmp2)
    ADVANCE_TILE_64(b, b, tmp1, tmp2)
    ADVANCE_TILE_64(b, b, tmp1, tmp2)
    ADVANCE_TILE_64(b, b, tmp1, tmp2)
    ADVANCE_TILE_64(b, b, tmp1, tmp2)
    #endif

    #include "cphash_footer.h"
}

__global__ void copyhashes_D2_p1(uint64_cu *multiverse, uint32_cu *univec, uint64_cu *hashes, uint32_cu offset, bool initial) {

    #include "cphash_header.h"

    if ((threadIdx.x >= 32) && (threadIdx.x < 48)) {
        b = hashes[(hashnum << 2) + (threadIdx.x >> 2) - 8];
        b = (b >> (16 * (threadIdx.x & 3)));
        b = ((b & 0x00ff) << 32) | ((b & 0xff00) << 16);
    } else if ((threadIdx.x > 16) && (threadIdx.x < 32)) {
        uint32_cu ti = 64 - threadIdx.x;
        b = hashes[(hashnum << 2) + (ti >> 2) - 8];
        b = (b >> (16 * (ti & 3)));
        b = ((b & 0x00ff) << 32) | ((b & 0xff00) << 16);
    }

    #include "cphash_footer.h"
}

__global__ void copyhashes_D2_p2(uint64_cu *multiverse, uint32_cu *univec, uint64_cu *hashes, uint32_cu offset, bool initial) {

    #include "cphash_header.h"

    // Copy SHA-256 hash:
    if ((threadIdx.x >= 32) && (threadIdx.x < 48)) {
        b = hashes[(hashnum << 2) + (threadIdx.x >> 2) - 8];
        b = (b >> (16 * (threadIdx.x & 3)));
        b = ((b & 0x00ff) << 32) | ((b & 0xff00) << 16);
    } else if ((threadIdx.x >= 16) && (threadIdx.x < 32)) {
        uint32_cu ti = 63 - threadIdx.x;
        b = hashes[(hashnum << 2) + (ti >> 2) - 8];
        b = (b >> (16 * (ti & 3)));
        b = ((b & 0x00ff) << 32) | ((b & 0xff00) << 16);
    }

    #include "cphash_footer.h"
}

__global__ void copyhashes_D4_p1(uint64_cu *multiverse, uint32_cu *univec, uint64_cu *hashes, uint32_cu offset, bool initial) {

    #include "cphash_header.h"

    if ((threadIdx.x >= 32) && (threadIdx.x < 48)) {
        b = hashes[(hashnum << 2) + (threadIdx.x >> 2) - 8];
        b = (b >> (16 * (threadIdx.x & 3)));
        b = ((b & 0x00ff) << 32) | ((b & 0xff00) << 16);
    } else if ((threadIdx.x > 16) && (threadIdx.x < 32)) {
        uint32_cu ti = 64 - threadIdx.x;
        b = hashes[(hashnum << 2) + (ti >> 2) - 8];
        b = (b >> (16 * (ti & 3)));
        b = ((b & 0x00ff) << 32) | ((b & 0xff00) << 16);
    }

    uint64_cu CUDABITREVERSE(c, b);
    b = (b >> 7) | (c << 8);

    #include "cphash_footer.h"
}

__global__ void copyhashes_D4_p2(uint64_cu *multiverse, uint32_cu *univec, uint64_cu *hashes, uint32_cu offset, bool initial) {

    #include "cphash_header.h"

    if ((threadIdx.x >= 32) && (threadIdx.x < 48)) {
        b = hashes[(hashnum << 2) + (threadIdx.x >> 2) - 8];
        b = (b >> (16 * (threadIdx.x & 3)));
        b = ((b & 0x00ff) << 32) | ((b & 0xff00) << 16);
    } else if ((threadIdx.x > 16) && (threadIdx.x < 32)) {
        uint32_cu ti = 64 - threadIdx.x;
        b = hashes[(hashnum << 2) + (ti >> 2) - 8];
        b = (b >> (16 * (ti & 3)));
        b = ((b & 0x00ff) << 32) | ((b & 0xff00) << 16);
    }

    uint64_cu CUDABITREVERSE(c, b);
    b = (b >> 8) | (c << 8);

    #include "cphash_footer.h"
}

__global__ void copyhashes_D4_p4(uint64_cu *multiverse, uint32_cu *univec, uint64_cu *hashes, uint32_cu offset, bool initial) {

    #include "cphash_header.h"

    // Copy SHA-256 hash:
    if ((threadIdx.x >= 32) && (threadIdx.x < 48)) {
        b = hashes[(hashnum << 2) + (threadIdx.x >> 2) - 8];
        b = (b >> (16 * (threadIdx.x & 3)));
        b = ((b & 0x00ff) << 32) | ((b & 0xff00) << 16);
    } else if ((threadIdx.x >= 16) && (threadIdx.x < 32)) {
        uint32_cu ti = 63 - threadIdx.x;
        b = hashes[(hashnum << 2) + (ti >> 2) - 8];
        b = (b >> (16 * (ti & 3)));
        b = ((b & 0x00ff) << 32) | ((b & 0xff00) << 16);
    }

    uint64_cu CUDABITREVERSE(c, b);
    b = (b >> 8) | (c << 8);

    #include "cphash_footer.h"
}

void copyhashes(std::string full_symmetry, int universes_left, uint64_cu *multiverse,
                uint32_cu *univec, uint64_cu *hashes, uint32_cu offset, bool initial) {

    if (universes_left <= 0) { return; }

    std::string symmetry = full_symmetry;

    if (symmetry[0] == 'G') {
        symmetry = "C" + symmetry.substr(1);
    } else if (symmetry[0] == 'H') {
        symmetry = "D" + symmetry.substr(1);
    }

    if (symmetry == "C1") {
        copyhashes_C1<<<universes_left, 64>>>(multiverse, univec, hashes, offset, initial);
    } else if (symmetry == "D2_+1") {
        copyhashes_D2_p1<<<universes_left, 64>>>(multiverse, univec, hashes, offset, initial);
    } else if (symmetry == "D2_+2") {
        copyhashes_D2_p2<<<universes_left, 64>>>(multiverse, univec, hashes, offset, initial);
    } else if (symmetry == "D4_+1") {
        copyhashes_D4_p1<<<universes_left, 64>>>(multiverse, univec, hashes, offset, initial);
    } else if (symmetry == "D4_+2") {
        copyhashes_D4_p2<<<universes_left, 64>>>(multiverse, univec, hashes, offset, initial);
    } else if (symmetry == "D4_+4") {
        copyhashes_D4_p4<<<universes_left, 64>>>(multiverse, univec, hashes, offset, initial);
    } else {
        std::cerr << "Fatal: symmetry " << symmetry << " unrecognised!!!" << std::endl;
        exit(1);
    }

}
