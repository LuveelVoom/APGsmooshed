#pragma once

#include "basics.h"
#include "../classic/bpattern.h"


_DI_ uint4 load_bpattern(const apg::bpattern *ptr) {
    const uint4* u4ptr = ((const uint4*) ptr);
    uint4 result = u4ptr[threadIdx.x & 31];
    return result;
}

_DI_ void save_bpattern(apg::bpattern *ptr, uint4 slice) {
    uint4* u4ptr = ((uint4*) ptr);
    u4ptr[threadIdx.x & 31] = slice;
}

_DI_ uint4 load_hash(const unsigned int *hash_ptr) {

    unsigned int h = 0;

    int laneId = threadIdx.x & 31;

    if ((laneId >= 12) && (laneId < 20)) {
        h = hash_ptr[laneId - 12];
    }

    // elegantly perform necessary endianness change:
    uint4 result;
    result.x = (h & 0x0000ff00u) << 16;
    result.y = (h & 0x000000ffu);
    result.z = (h & 0xff000000u);
    result.w = (h & 0x00ff0000u) >> 16;

    return result;
}

_DI_ void hreflect_even(uint4 &b) {

    b.x = (b.x >> 8) | (b.y << 24);
    b.z = (b.z >> 8) | (b.w << 24);
    b.y = __brev(b.x);
    b.w = __brev(b.z);

}

_DI_ void hreflect_odd(uint4 &b) {

    hreflect_even(b);
    b.x <<= 1;
    b.z <<= 1;

}

_DI_ void vreflect_even(uint4 &b) {

    int laneId = threadIdx.x & 31;
    int src = (laneId < 16) ? (27 - laneId) : (laneId - 4);

    b.x = hh::shuffle_32(b.x, src);
    b.y = hh::shuffle_32(b.y, src);
    b.z = hh::shuffle_32(b.z, src);
    b.w = hh::shuffle_32(b.w, src);

    if (laneId < 16) {
        uint32_t c = b.x; b.x = b.z; b.z = c;
        c = b.y; b.y = b.w; b.w = c;
    }
}

_DI_ void vreflect_odd(uint4 &b) {

    int laneId = threadIdx.x & 31;
    int src1 = (laneId < 16) ? (28 - laneId) : (laneId - 4);
    int src2 = (laneId < 16) ? (27 - laneId) : (laneId - 4);

    b.x = hh::shuffle_32(b.x, src1);
    b.y = hh::shuffle_32(b.y, src1);
    b.z = hh::shuffle_32(b.z, src2);
    b.w = hh::shuffle_32(b.w, src2);

}

_DI_ void restrict_C2_4(uint4 &b) {

    int laneId = threadIdx.x & 31;
    if (laneId < 16) {
        b.x = 0; b.z = 0;
    } else {
        b.y = 0; b.w = 0;
    }
}

_DI_ void restrict_C2_2(uint4 &b) {

    int laneId = threadIdx.x & 31;
    if (laneId < 16) {
        b.x = 0; b.z = 0;
    } else {
        b.y &= 1;
        b.w &= 1;
    }
}

_DI_ void restrict_C2_1(uint4 &b) {

    int laneId = threadIdx.x & 31;
    if (laneId < 16) {
        b.x = 0; b.z = 0;
    } else {
        if (laneId > 16) { b.y &= 1; }
        b.w &= 1;
    }
}

_DI_ uint32_t ternary_xor(uint32_t x, uint32_t y, uint32_t z) {
    uint32_t res;
    asm("lop3.b32 %0, %1, %2, %3, 0b10010110;" : "=r"(res) : "r"(x), "r"(y), "r"(z));
    return res;
}

_DI_ uint32_t ternary_maj(uint32_t x, uint32_t y, uint32_t z) {
    uint32_t res;
    asm("lop3.b32 %0, %1, %2, %3, 0b11101000;" : "=r"(res) : "r"(x), "r"(y), "r"(z));
    return res;
}

_DI_ uint32_t calculate_f(uint32_t a, uint32_t ar, uint32_t e) {
    uint32_t res;
    asm("lop3.b32 %0, %1, %2, %3, 0b01110110;" : "=r"(res) : "r"(a), "r"(ar), "r"(e));
    return res;
}

_DI_ uint32_t calculate_g(uint32_t d, uint32_t f, uint32_t a) {
    uint32_t res;
    asm("lop3.b32 %0, %1, %2, %3, 0b11000010;" : "=r"(res) : "r"(d), "r"(f), "r"(a));
    return res;
}

_DI_ uint32_t calculate_h(uint32_t g, uint32_t bl, uint32_t br) {
    uint32_t res;
    asm("lop3.b32 %0, %1, %2, %3, 0b00010110;" : "=r"(res) : "r"(g), "r"(bl), "r"(br));
    return res;
}

_DI_ uint32_t calculate_i(uint32_t h, uint32_t f, uint32_t g) {
    uint32_t res;
    asm("lop3.b32 %0, %1, %2, %3, 0b11100000;" : "=r"(res) : "r"(h), "r"(f), "r"(g));
    return res;
}

/**
 * Advance a 64x64 torus by one generation using a single warp.
 *
 * Each thread holds a 64x2 rectangle of the torus in a uint4
 * datatype (representing a 2x2 array of 32x1 rectangles).
 */
_DI_ uint4 advance_torus(uint4 old) {

    // Precompute the lane indices of the threads representing
    // the 64x2 rectangles immediately above and below this one.
    int upperthread = (threadIdx.x + 31) & 31;
    int lowerthread = (threadIdx.x +  1) & 31;

    // 8 funnel shifts:
    uint4 al; uint4 ar;
    al.x = (old.x << 1) | (old.y >> 31);
    al.y = (old.y << 1) | (old.x >> 31);
    al.z = (old.z << 1) | (old.w >> 31);
    al.w = (old.w << 1) | (old.z >> 31);
    ar.x = (old.x >> 1) | (old.y << 31);
    ar.y = (old.y >> 1) | (old.x << 31);
    ar.z = (old.z >> 1) | (old.w << 31);
    ar.w = (old.w >> 1) | (old.z << 31);

    // 8 ternary ops:
    uint4 xor3; uint4 maj3;
    xor3.x = ternary_xor(old.x, al.x, ar.x);
    xor3.y = ternary_xor(old.y, al.y, ar.y);
    xor3.z = ternary_xor(old.z, al.z, ar.z);
    xor3.w = ternary_xor(old.w, al.w, ar.w);
    maj3.x = ternary_maj(old.x, al.x, ar.x);
    maj3.y = ternary_maj(old.y, al.y, ar.y);
    maj3.z = ternary_maj(old.z, al.z, ar.z);
    maj3.w = ternary_maj(old.w, al.w, ar.w);

    // 8 shuffles:
    uint4 xor3prime; uint4 maj3prime;
    xor3prime.x = hh::shuffle_32(xor3.x, lowerthread);
    xor3prime.y = hh::shuffle_32(xor3.y, lowerthread);
    xor3prime.z = hh::shuffle_32(xor3.z, upperthread);
    xor3prime.w = hh::shuffle_32(xor3.w, upperthread);
    maj3prime.x = hh::shuffle_32(maj3.x, lowerthread);
    maj3prime.y = hh::shuffle_32(maj3.y, lowerthread);
    maj3prime.z = hh::shuffle_32(maj3.z, upperthread);
    maj3prime.w = hh::shuffle_32(maj3.w, upperthread);

    // 24 ternary ops:
    uint4 newstate;
    {
        uint4 d; uint4 e; uint4 f; uint4 g; uint4 h;
        d.x = ternary_maj(xor3.z, xor3prime.z, al.x);
        d.y = ternary_maj(xor3.w, xor3prime.w, al.y);
        d.z = ternary_maj(xor3.x, xor3prime.x, al.z);
        d.w = ternary_maj(xor3.y, xor3prime.y, al.w);
        e.x = ternary_xor(xor3.z, xor3prime.z, al.x);
        e.y = ternary_xor(xor3.w, xor3prime.w, al.y);
        e.z = ternary_xor(xor3.x, xor3prime.x, al.z);
        e.w = ternary_xor(xor3.y, xor3prime.y, al.w);
        f.x = calculate_f(old.x, ar.x, e.x);
        f.y = calculate_f(old.y, ar.y, e.y);
        f.z = calculate_f(old.z, ar.z, e.z);
        f.w = calculate_f(old.w, ar.w, e.w);
        g.x = calculate_g(d.x, f.x, old.x);
        g.y = calculate_g(d.y, f.y, old.y);
        g.z = calculate_g(d.z, f.z, old.z);
        g.w = calculate_g(d.w, f.w, old.w);
        h.x = calculate_h(g.x, maj3.z, maj3prime.z);
        h.y = calculate_h(g.y, maj3.w, maj3prime.w);
        h.z = calculate_h(g.z, maj3.x, maj3prime.x);
        h.w = calculate_h(g.w, maj3.y, maj3prime.y);
        newstate.x = calculate_i(h.x, f.x, g.x);
        newstate.y = calculate_i(h.y, f.y, g.y);
        newstate.z = calculate_i(h.z, f.z, g.z);
        newstate.w = calculate_i(h.w, f.w, g.w);
    }

    #ifdef PEDESTRIAN_LIFE
    // correction for eighth birth transition
    uint32_t maj3_xz = maj3.x & maj3.z;
    uint32_t maj3_yw = maj3.y & maj3.w;
    newstate.x |= (maj3prime.z & maj3_xz & xor3.z & xor3prime.z &~ old.x);
    newstate.y |= (maj3prime.w & maj3_yw & xor3.w & xor3prime.w &~ old.y);
    newstate.z |= (maj3prime.x & maj3_xz & xor3.x & xor3prime.x &~ old.z);
    newstate.w |= (maj3prime.y & maj3_yw & xor3.y & xor3prime.y &~ old.w);
    #endif

    return newstate;

}


_DI_ void rotate_torus_inplace(uint4 &t, int rh, int rv) {

    if (rv & 63) {
        // translate vertically:
        uint4 d;
        d.x = (rv & 1) ? t.z : t.x;
        d.y = (rv & 1) ? t.w : t.y;
        d.z = (rv & 1) ? t.x : t.z;
        d.w = (rv & 1) ? t.y : t.w;
        int upperthread = (((rv)     >> 1) + threadIdx.x) & 31;
        int lowerthread = (((rv + 1) >> 1) + threadIdx.x) & 31;
        t.x = hh::shuffle_32(d.x, upperthread);
        t.y = hh::shuffle_32(d.y, upperthread);
        t.z = hh::shuffle_32(d.z, lowerthread);
        t.w = hh::shuffle_32(d.w, lowerthread);
    }

    if (rh & 63) {
        // translate horizontally:
        uint4 d;
        d.x = (rh & 32) ? t.y : t.x;
        d.y = (rh & 32) ? t.x : t.y;
        d.z = (rh & 32) ? t.w : t.z;
        d.w = (rh & 32) ? t.z : t.w;
        int sa = rh & 31;
        t.x = (d.x >> sa) | (d.y << (32 - sa));
        t.y = (d.y >> sa) | (d.x << (32 - sa));
        t.z = (d.z >> sa) | (d.w << (32 - sa));
        t.w = (d.w >> sa) | (d.z << (32 - sa));
    }
}


/**
 * Determine whether the pattern is too large to fit in a single tile.
 */
_DI_ bool escaped_bounding_box(uint4 &b) {

    unsigned int xz = b.x | b.z;
    unsigned int yw = b.y | b.w;

    int laneId = threadIdx.x & 31;
    bool hasMask = (laneId >= 3) && (laneId < 29);
    unsigned int xzmask = (hasMask) ? 0x0000003fu : 0xffffffffu;
    unsigned int ywmask = (hasMask) ? 0xfc000000u : 0xffffffffu;

    if (hh::ballot_32((xzmask & xz) | (ywmask & yw)) == 0) {
        // currently inside bounding box:
        return false;
    }

    // We're outside the 52x52 region, but it might be possible to
    // recentre the pattern such that it fits again. We examine the
    // horizontal and vertical extent:

    unsigned int active_threads = hh::ballot_32(xz | yw);

    int vt;
    if ((active_threads & 0xe0000007u) == 0) {
        vt = 0;
    } else if ((active_threads & 0xf0000003u) == 0) {
        vt = -2;
    } else if ((active_threads & 0xc000000fu) == 0) {
        vt = 2;
    } else if ((active_threads & 0xf8000001u) == 0) {
        vt = -4;
    } else if ((active_threads & 0x8000001fu) == 0) {
        vt = 4;
    } else if ((active_threads & 0xfc000000u) == 0) {
        vt = -6;
    } else if ((active_threads & 0x0000003fu) == 0) {
        vt = 6;
    } else {
        // pattern is too tall:
        return true;
    }

    xz |= hh::shuffle_xor_32(xz, 1);
    yw |= hh::shuffle_xor_32(yw, 1);
    xz |= hh::shuffle_xor_32(xz, 2);
    yw |= hh::shuffle_xor_32(yw, 2);
    xz |= hh::shuffle_xor_32(xz, 4);
    yw |= hh::shuffle_xor_32(yw, 4);
    xz |= hh::shuffle_xor_32(xz, 8);
    yw |= hh::shuffle_xor_32(yw, 8);
    xz |= hh::shuffle_xor_32(xz, 16);
    yw |= hh::shuffle_xor_32(yw, 16);

    int ht;
    if (xz == 0) {
        ht = 16;
    } else if (yw == 0) {
        ht = -16;
    } else {
        int rightborder = __clz(yw);
        int  leftborder = __clz(__brev(xz));

        if (leftborder + rightborder < 12) {
            // pattern is too wide:
            return true;
        }

        ht = (leftborder - rightborder) >> 1;
    }

    // recentre pattern:
    rotate_torus_inplace(b, ht, vt);
    return false;
}


/**
 * Advances a single tile and checks whether it has settled down.
 * If it settles down, return 0; otherwise, return the current
 * generation count.
 */
_DI_ int advance_initial(uint4 &a) {

    uint4 b = a;

    // Run for 30 generations without any checks. It is impossible for
    // the pattern to escape the 52x52 bounding box in this time period,
    // and only 1.6% of soups stabilise this quickly so we do not give
    // up much by skipping the stabilisation checks:
    #ifdef C1_SYMMETRY
    #define initial_gens 30
    #else
    #define initial_gens 10
    #endif

    #pragma unroll 2
    for (int i = 0; i < initial_gens; i++) {
        b = advance_torus(b);
    }

    int age = initial_gens - 6;

    // Run until generation 600, checking for bounding box escapement
    // and 2-periodicity. This loop should fit comfortably in the GPU
    // icache. About 36% of soups stabilise in this loop.
    do {

        age += 6;
        a = b;

        b = advance_torus(b);
        b = advance_torus(b);

        if (hh::ballot_32((a.x ^ b.x) | (a.y ^ b.y) | (a.z ^ b.z) | (a.w ^ b.w)) == 0) {
            // periodic with period 2:
            return 0;
        }

        b = advance_torus(b);
        b = advance_torus(b);
        b = advance_torus(b);
        b = advance_torus(b);

        if (escaped_bounding_box(b)) {
            break;
        }
    } while (age < 600);

    // Conveniently, a and b differ by 6 generations, so we can check
    // for soups which stabilise with the production of a pulsar.
    if (hh::ballot_32((a.x ^ b.x) | (a.y ^ b.y) | (a.z ^ b.z) | (a.w ^ b.w)) == 0) {
        // periodic with period 6:
        return 0;
    }

    // ================================================================
    // |                  CHECK FOR ESCAPING GLIDERS                  |
    // ================================================================

    // Take intersection of two snapshots separated by 8 generations:
    uint4 c;
    b = advance_torus(b);
    b = advance_torus(b);
    c.x = a.x & b.x;
    c.y = a.y & b.y;
    c.z = a.z & b.z;
    c.w = a.w & b.w;

    {
        uint4 d;
        d = advance_torus(c);
        d = advance_torus(d);

        if (hh::ballot_32((c.x ^ d.x) | (c.y ^ d.y) | (c.z ^ d.z) | (c.w ^ d.w)) != 0) {
            // core does not have period 2:
            return age;
        }
    }

    {
        int totpop = __popc(c.x ^ a.x) + __popc(c.y ^ a.y) + __popc(c.z ^ a.z) + __popc(c.w ^ a.w);
        totpop     = totpop << 16;
        totpop    += __popc(c.x ^ b.x) + __popc(c.y ^ b.y) + __popc(c.z ^ b.z) + __popc(c.w ^ b.w);

        totpop += hh::shuffle_xor_32(totpop, 1);
        totpop += hh::shuffle_xor_32(totpop, 2);
        totpop += hh::shuffle_xor_32(totpop, 4);
        totpop += hh::shuffle_xor_32(totpop, 8);
        totpop += hh::shuffle_xor_32(totpop, 16);

        if (totpop != 0x00050005u) {
            // diff has wrong population to be a glider:
            return age;
        }
    }

    {
        // Advance by another 6 generations to ensure glider has fully escaped:
        for (int i = 0; i < 4; i++) {
            b = advance_torus(b);
        }

        int laneId = threadIdx.x & 31;
        bool hasMask = (laneId >= 3) && (laneId < 29);
        unsigned int xzmask = (hasMask) ? 0xffffffc0u : 0;
        unsigned int ywmask = (hasMask) ? 0x03ffffffu : 0;

        int totpop = 0;

        for (int i = 0; i < 4; i++) {
            b = advance_torus(b);
            b = advance_torus(b);
            if (hh::ballot_32(((b.x & xzmask) ^ c.x) | ((b.y & ywmask) ^ c.y) | ((b.z & xzmask) ^ c.z) | ((b.w & ywmask) ^ c.w))) { return false; }
            totpop     = totpop << 8;
            totpop    += __popc(c.x ^ b.x) + __popc(c.y ^ b.y) + __popc(c.z ^ b.z) + __popc(c.w ^ b.w);
        }

        totpop += hh::shuffle_xor_32(totpop, 1);
        totpop += hh::shuffle_xor_32(totpop, 2);
        totpop += hh::shuffle_xor_32(totpop, 4);
        totpop += hh::shuffle_xor_32(totpop, 8);
        totpop += hh::shuffle_xor_32(totpop, 16);

        return ((totpop == 0x05050505u) ? 0 : age);
    }
}


_DI_ int advance_tile_inplace(uint4 &a, int rh, int rv) {

    uint4 b = advance_torus(a);
    uint4 c = advance_torus(b);

    b = advance_torus(c);
    b = advance_torus(b);
    b = advance_torus(b);
    b = advance_torus(b);

    unsigned int leftdiff;
    unsigned int rightdiff;

    {
        int laneId = threadIdx.x & 31;
        bool hasMask = (laneId >= 3) && (laneId < 29);
        unsigned int xzmask = (hasMask) ? 0xffffffc0u : 0;
        unsigned int ywmask = (hasMask) ? 0x03ffffffu : 0;

        uint4 xordiff;
        xordiff.x = (a.x ^ b.x) & xzmask;
        xordiff.y = (a.y ^ b.y) & ywmask;
        xordiff.z = (a.z ^ b.z) & xzmask;
        xordiff.w = (a.w ^ b.w) & ywmask;

        // update central 52-by-52 tile:
        a.x ^= xordiff.x;
        a.y ^= xordiff.y;
        a.z ^= xordiff.z;
        a.w ^= xordiff.w;

        xordiff.x |= xordiff.z;
        xordiff.y |= xordiff.w;
        leftdiff  = (xordiff.x >> 6) | (xordiff.y << 26);
        rightdiff = (xordiff.y << 6) | (xordiff.x >> 26);
    }

    int flags = 64;
    flags |= (hh::ballot_32(rightdiff & 0xfc000000u) ? 1 : 0);
    flags |= (hh::ballot_32( leftdiff & 0x0000003fu) ? 8 : 0);
    leftdiff  = hh::ballot_32(leftdiff);
    rightdiff = hh::ballot_32(rightdiff);

    if ((leftdiff | rightdiff) == 0) { return 0; }

    flags |= ((rightdiff & 0x00000038u) ? 2  : 0);
    flags |= (( leftdiff & 0x00000038u) ? 4  : 0);
    flags |= (( leftdiff & 0x1c000000u) ? 16 : 0);
    flags |= ((rightdiff & 0x1c000000u) ? 32 : 0);

    // Escaping glider/*WSS detection:
    if ((rh & 63) || (rv & 63)) {

        unsigned int xz = b.x | b.z;
        unsigned int yw = b.y | b.w;

        if (hh::ballot_32(xz | yw)) {

            int laneId = threadIdx.x & 31;
            bool hasMask = (laneId >= 3) && (laneId < 29);
            unsigned int xzmask = (hasMask) ? 0x0000003fu : 0xffffffffu;
            unsigned int ywmask = (hasMask) ? 0xfc000000u : 0xffffffffu;

            if (hh::ballot_32((xz & xzmask) | (yw & ywmask)) == 0) {

                rotate_torus_inplace(c, rh, rv);

                if (hh::ballot_32((c.x ^ b.x) | (c.y ^ b.y) | (c.z ^ b.z) | (c.w ^ b.w)) == 0) {

                    int totpop = __popc(b.x) + __popc(b.y) + __popc(b.z) + __popc(b.w);
                    totpop += hh::shuffle_xor_32(totpop, 1);
                    totpop += hh::shuffle_xor_32(totpop, 2);
                    totpop += hh::shuffle_xor_32(totpop, 4);
                    totpop += hh::shuffle_xor_32(totpop, 8);
                    totpop += hh::shuffle_xor_32(totpop, 16);

                    if (totpop <= 17) {
                        // either a single LWSS, MWSS, or HWSS, or between
                        // 1 and 3 gliders. Remove them from the universe:
                        a.x &= xzmask;
                        a.y &= ywmask;
                        a.z &= xzmask;
                        a.w &= ywmask;
                    }
                }
            }
        }
    }

    return flags;

}


__global__ void exhaustFirstTile(uint32_cu *hashes, uint32_cu *interesting, uint4 *output) {

    int pos = (threadIdx.x + blockIdx.x * blockDim.x) >> 5;

    uint32_t *thishash = hashes + (pos << 3);

    uint4 a = load_hash(thishash);

    #ifdef HREFLECT_EVEN
    hreflect_even(a);
    #endif
    #ifdef HREFLECT_ODD
    hreflect_odd(a);
    #endif
    #ifdef VREFLECT_EVEN
    vreflect_even(a);
    #endif
    #ifdef VREFLECT_ODD
    vreflect_odd(a);
    #endif
    #ifdef RESTRICT_C2_1
    restrict_C2_1(a);
    #endif
    #ifdef RESTRICT_C2_2
    restrict_C2_2(a);
    #endif
    #ifdef RESTRICT_C2_4
    restrict_C2_4(a);
    #endif

    int gencount = advance_initial(a);

    interesting[pos] = gencount;

    if (gencount && output) {
        int laneId = threadIdx.x & 31;
        uint4 *uverse = output + (((uint64_t) pos) << 12);
        uverse[laneId + 32] = a;

        uint4 b; b.x = 0; b.y = 0; b.z = 0; b.w = 0;

        if (laneId == 0) { b.y = 127; }

        uverse[laneId] = b;
    }
}

#include "emt.h"
#define ULTIMATE_EMT 1
#include "emt.h"

