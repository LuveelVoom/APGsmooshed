#pragma once

#include "../core.hpp"

/**
 * This file contains functionality for manipulating elements of the finite
 * field F_p, where p == Phi_192(2) == 2^64 - 2^32 + 1. See the article:
 *
 * https://cp4space.hatsya.com/2021/09/01/an-efficient-prime-for-number-theoretic-transforms/
 *
 * for an introduction to this particular finite field and its various
 * useful properties.
 */

namespace hh {

/**
 * Reduce an arbitrary 159-bit unsigned integer modulo 2^64 - 2^32 + 1.
 *
 * 'low', 'middle', and 'high' are 64, 32, and 63 bits, respectively.
 * The upper half of 'middle' is safely ignored.
 */
_HD_ uint64_t ff64_reduce159(uint64_t low, uint64_t middle, uint64_t high) {

    constexpr uint64_t modulus = 0xffffffff00000001ull;

    uint64_t low2 = low - high;
    if (high > low) { low2 += modulus; } // correct for underflow

    // At this point, we have reduced our 159-bit input into a 96-bit
    // result. We want to further reduce into a 64-bit result.

    // x^2 == x - 1 (mod x^2 - x + 1), so we need to add (b << 32) - b
    // to the result:
    uint64_t product = (middle << 32); product -= (product >> 32);
    uint64_t result = low2 + product;

    // We now correct for a possible overflow. The 'product' in the
    // previous expression was upper-bounded by 0xfffffffe00000001,
    // and 'x' was upper-bounded by 0xffffffffffffffff, so their sum
    // is (numerically) upper-bounded by 0x1fffffffe00000000. This
    // is less than twice the modulus, so we only need to subtract
    // the modulus at most once.
    if ((result < product) || (result >= modulus)) { result -= modulus; }

    // The result is now in the range [0, modulus - 1]:
    return result;

}

/**
 * Multiply by (1 << k) where k <= 32 and reduce.
 *
 * For a method that works for arbitrary k, use operator<<(hh::ff64, int)
 * instead.
 */
_HD_ uint64_t ff64_shortshift(uint64_t x, int k) {

    constexpr uint64_t modulus = 0xffffffff00000001ull;

    uint64_t low = (x << k);
    uint64_t middle = (x >> (64 - k));
    uint64_t product = (middle << 32) - middle;
    uint64_t result = low + product;

    if ((result < product) || (result >= modulus)) { result -= modulus; }

    return result;
}

/**
 * Represents an element of the finite field F_(2^64 - 2^32 + 1).
 */
struct ff64 {

    constexpr static uint64_t modulus = 0xffffffff00000001ull;

    uint64_t x;

    // 64-bit constructor
    _HD_ ff64(const uint64_t& v) : x(v) { if (x >= modulus) { x -= modulus; } }

    // 128-bit constructor
    _HD_ ff64(const uint64_t& low, const uint64_t& high) : x(ff64_reduce159(low, high, high >> 32)) { }

    // 159-bit constructor
    _HD_ ff64(const uint64_t& low, const uint64_t& middle, const uint64_t& high) : x(ff64_reduce159(low, middle, high)) { }

    _HD_ ff64& operator+=(const ff64& other) {
        x += other.x;
        if ((x < other.x) || (x >= modulus)) { x -= modulus; }
        return (*this);
    }

    _HD_ ff64& operator-=(const ff64& other) {
        if (other.x > x) { x += modulus; }
        x -= other.x;
        return (*this);
    }

    // unary negation
    _HD_ ff64 operator-() const {
        return ff64(modulus - x);
    }

    // multiplication by primitive root 0x84000001
    _HD_ ff64& advance() {
        uint64_t y = ff64_shortshift(x, 31);
        uint64_t z = ff64_shortshift(x, 26);
        x += y; if ((x < y) || (x >= modulus)) { x -= modulus; }
        x += z; if ((x < z) || (x >= modulus)) { x -= modulus; }
        return (*this);
    }

    _HD_ ff64& operator<<=(int shiftamt) {

        // reduce into the interval [0, 95]:
        int xs = shiftamt % 192;
        if (xs < 0) { xs += 192; }
        if (xs >= 96) { xs -= 96; x = modulus - x; }

        // compute result in 159 bits:
        uint64_t low = 0;
        uint64_t med = 0;
        uint64_t high = 0;

        if (xs < 64) {
            low = x << xs;
            if (xs) { med = x >> (64 - xs); }
        } else {
            med = x << (xs - 64);
        }

        if (xs > 32) {
            high = x >> (96 - xs);
        }

        x = ff64_reduce159(low, med, high);

        return (*this);
    }

    _HD_ ff64& operator>>=(int shiftamt) {
        return ((*this) <<= (-shiftamt));
    }

    _HD_ ff64& operator*=(const ff64& rhs) {
        uint64_t low = x; uint64_t high = rhs.x;
        mul64x64(low, high);
        x = ff64_reduce159(low, high, high >> 32);
        return (*this);
    }
};

_HD_ ff64 operator<<(const ff64& lhs, int shiftamt) {
    ff64 x = lhs; x <<= shiftamt; return x;
}

_HD_ ff64 operator>>(const ff64 &lhs, int shiftamt) {
    ff64 x = lhs; x >>= shiftamt; return x;
}

_HD_ ff64 operator+(const ff64& lhs, const ff64& rhs) {
    ff64 x = lhs; x += rhs; return x;
}

_HD_ ff64 operator-(const ff64& lhs, const ff64& rhs) {
    ff64 x = lhs; x -= rhs; return x;
}

_HD_ ff64 operator*(const ff64& lhs, const ff64& rhs) {
    ff64 x = lhs; x *= rhs; return x;
}

static_assert(sizeof(ff64) == 8, "ff64 should be 64 bits");

/**
 * Modular inversion in the finite field inspired by Remco Bloemen's
 * binary GCD algorithm:
 *
 * https://xn--2-umb.com/22/goldilocks/
 *
 * We depart from that implementation by early-exiting the loop when
 * v reaches 1 (which it must do, because gcd(u, v) == 1 throughout).
 * This gives approximately a 20% speedup.
 *
 * We special-case a == 0 and return 0, so as to calculate the
 * pseudoinverse x^-1 == x^(p - 2).
 */
_HD_ ff64 ff64_inverse(const ff64 &a) {

    uint64_t u = 0xffffffff00000001ull;
    uint64_t v = a.x;
    if (v == 0) { return a; }

    uint64_t t0 = 0;
    uint64_t t1 = 1;
    int k = hh::ctz64(v);
    v >>= k;
    k += 96;
    while (v != 1) {
        u -= v;
        t0 += t1;
        int count = hh::ctz64(u);
        u >>= count;
        t1 <<= count;
        k += count;
        if (u < v) {
            { uint64_t w = u; u = v; v = w; }
            { uint64_t t2 = t1; t1 = t0; t0 = t2; }
            k += 96;
        }
    }

    t0 += t1 * (u - 1);

    ff64 res(t0);
    return (res << (191 * k));
}

/// Implement division in terms of modular inverse.
_HD_ ff64 operator/(const ff64& lhs, const ff64& rhs) {
    ff64 x = lhs; x *= ff64_inverse(rhs); return x;
}

} // namespace hh
