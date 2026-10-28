#pragma once
#include "core.hpp"

namespace hh {

/**
 * Multiply a pair of 4x4 matrices over F_2.
 */
_HD_ uint16_t mxor4(uint16_t a, uint16_t b) {

    // create a 64-bit register containing all row-sums of b:
    uint32_t b0 = b & 0x000f; b0 *= 0x10101010u;
    uint32_t b1 = b & 0x00f0; b1 *= 0x1100110u;
    uint32_t b2 = b & 0x0f00; b2 *= 0x111100u;
    uint32_t b3 = b >> 12; b3 *= 0x11111111u;
    uint64_t bx = b0 ^ b1 ^ b2;
    bx ^= ((bx ^ b3) << 32);

    // extract the relevant rows from bx:
    uint16_t a0 = (bx >> ((a & 0x000f) << 2)) & 15;
    uint16_t a1 = (bx >> ((a & 0x00f0) >> 2)) & 15;
    uint16_t a2 = (bx >> ((a & 0x0f00) >> 6)) & 15;
    uint16_t a3 = (bx >> ((a & 0xf000) >> 10)) & 15;
    return (a0 ^ (a1 << 4) ^ (a2 << 8) ^ (a3 << 12));
}

/**
 * Invert a 4x4 matrix over F_2.
 */
_HD_ uint16_t pinv4(uint16_t x) {

    // optimal addition chain inversion:
    uint16_t x2 = mxor4(x, x);
    uint16_t x3 = mxor4(x2, x);
    uint16_t x6 = mxor4(x3, x3);
    uint16_t x12 = mxor4(x6, x6);
    uint16_t x13 = mxor4(x12, x);
    uint16_t x26 = mxor4(x13, x13);
    uint16_t x52 = mxor4(x26, x26);
    uint16_t x104 = mxor4(x52, x52);
    uint16_t x208 = mxor4(x104, x104);
    uint16_t x416 = mxor4(x208, x208);
    return mxor4(x416, x3);
}

/**
 * Transpose a 4x4 matrix over F_2.
 */
_HD_ uint16_t transpose4(uint16_t x) {
    uint16_t d = (x ^ (x >> 3)) & 0x0a0a;
    uint16_t y = x ^ d ^ (d << 3);
    uint16_t e = (y ^ (y >> 6)) & 0x00cc;
    return (y ^ e ^ (e << 6));
}

_HD_ uint64_t transpose4batch(uint64_t x) {
    uint64_t d = (x ^ (x >> 3)) & 0x0a0a0a0a0a0a0a0aull;
    uint64_t y = x ^ d ^ (d << 3);
    uint64_t e = (y ^ (y >> 6)) & 0x00cc00cc00cc00ccull;
    return (y ^ e ^ (e << 6));
}

}

