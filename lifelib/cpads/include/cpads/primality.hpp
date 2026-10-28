#pragma once
#include "montgomery.hpp"

namespace hh {

// Miller-Rabin bases for the Forisek-Jancina primality tests:

const static uint16_t __fj32_bases[] __attribute__((aligned(64))) = {
#include "internal/fj32.inc"
};

const static uint16_t __fj64_bases[] __attribute__((aligned(64))) = {
#include "internal/fj64.inc"
};


/**
 * This attempts to find the smallest prime factor of n and
 * returns it. If n is composite and the smallest prime factor
 * is >= 59, then it will return 0 instead.
 */
uint64_t trialdiv_mrabin(uint64_t n) {

    // We start by performing trial division with the first 16
    // primes; this should be fast as the compiler will optimise
    // these known-modulus divisibility checks:

    #define EEXIT(p) if (n < p * p) { return n; }
    #define TRIALDIV(p) if (n % (p) == 0) { return p; }

    EEXIT(2)
    TRIALDIV(2) TRIALDIV(3) TRIALDIV(5) TRIALDIV(7)
    EEXIT(11)
    TRIALDIV(11) TRIALDIV(13) TRIALDIV(17) TRIALDIV(19)
    EEXIT(23)
    TRIALDIV(23) TRIALDIV(29) TRIALDIV(31) TRIALDIV(37)
    EEXIT(41)
    TRIALDIV(41) TRIALDIV(43) TRIALDIV(47) TRIALDIV(53)
    EEXIT(59)

    // At this point we know that our number has no prime factors
    // strictly less than 59. We now perform one or two Miller-Rabin
    // primality tests, depending on the size of n, and ensure that
    // the base is smaller than n (thereby avoiding a modulo before
    // the Montgomery modular exponentation).

    if (n <= 96640) {
        // for numbers in the range [3481, 96640] with no prime factors
        // p <= 53, a single Miller-Rabin test to base 1361 suffices:
        hh::Montgomery<uint32_t, uint64_t> mont(n);
        if (!(mont.strong_probable_prime(1361))) { return 0; }
        return n;
    } else if (n <= 0xffffffffull) {
        // 32-bit Forisek-Jancina primality test (one Miller-Rabin):
        uint64_t h = n;
        h = ((h >> 16) ^ h) * 0x45d9f3b;  
        h = ((h >> 16) ^ h) * 0x45d9f3b;
        h = ((h >> 16) ^ h) & 255;
        hh::Montgomery<uint32_t, uint64_t> mont(n);
        if (!(mont.strong_probable_prime(__fj32_bases[h]))) { return 0; }
        return n;
    } else {
        // 64-bit Forisek-Jancina primality test (two Miller-Rabins):
        hh::Montgomery<uint64_t, u128> mont(n);
        if (!(mont.strong_probable_prime(2))) { return 0; }
        uint64_t h = n;
        h = ((h >> 32) ^ h) * 0x45d9f3b3335b369ull;
        h = ((h >> 32) ^ h) * 0x3335b36945d9f3bull;
        h = ((h >> 32) ^ h) & 262143;
        if (!(mont.strong_probable_prime(__fj64_bases[h]))) { return 0; }
        return n;
    }
}

/**
 * Check the primality of an arbitrary 64-bit unsigned integer.
 */
bool isprime(uint64_t n) {

    if (n <= 63) {
        // lookup table to deal with small cases:
        return (1 & (0x28208a20a08a28acull >> n));
    } else {
        uint64_t p = trialdiv_mrabin(n);
        return (n == p);
    }
}

uint64_t find_prime_factor(uint64_t n) {

    // Perform trial division and quick primality tests:
    {
        if (n < 4) { return n; }
        uint64_t p = trialdiv_mrabin(n);
        if (p > 0) { return p; }
    }

    // Check that n is not a perfect power:
    {
        uint64_t x = std::sqrt((double) n) + 0.5;
        if ((x * x) == n) { return find_prime_factor(x); }
        if (n >= 205379) {
            float fn = ((float) n);
            x = std::cbrtf(fn) + 0.5f;
            if ((x * x * x) == n) { return find_prime_factor(x); }
            if (n >= 714924299) {
                x = std::pow(fn, 0.200000000000f) + 0.5f;
                if ((x * x * x * x * x) == n) { return find_prime_factor(x); }
            }
            if (n >= 2488651484819ull) {
                x = std::pow(fn, 0.142857142857f) + 0.5f;
                if ((x * x * x * x * x * x * x) == n) { return find_prime_factor(x); }
            }
        }
    }

    // Do Pollard-rho:
    hh::Montgomery<uint64_t, u128> mont(n);
    uint64_t g = 0;

    int bailout = 1024;

    for (uint64_t k = 59; k <= 4294967279ull; k += 30) {
        // we do a handful of trial-divisions just to ensure that
        // this algorithm terminates:
        TRIALDIV(k)
        TRIALDIV(k+2)
        TRIALDIV(k+8)
        TRIALDIV(k+12)
        TRIALDIV(k+14)
        TRIALDIV(k+18)
        TRIALDIV(k+20)
        TRIALDIV(k+24)

        uint64_t x = k;

        for (int l = 8; l < bailout; l *= 2) {
            uint64_t y = x;
            uint64_t z = 1;

            for (int i = 0; i < l; i += 8) {
                for (int j = 0; j < 8; j++) {
                    x = mont.add(mont.multiply(x, x), 1);
                    z = mont.multiply(z, mont.sub(x, y));
                }
                g = hh::binary_gcd(z, n);
                if (g != 1) { break; }
            }
            if (g != 1) { break; }
        }
        if ((g > 1) && (g < n)) { break; }
        if (bailout < 1048576) { bailout *= 2; }
    }

    return find_prime_factor(g);

    #undef TRIALDIV
    #undef EEXIT
}

uint64_t euler_phi(uint64_t n) {

    if (n < 2) { return n; }

    uint64_t p = find_prime_factor(n);

    uint64_t m = n / p;
    uint64_t res = p - 1;
    while ((m % p) == 0) {
        m = m / p;
        res *= p;
    }

    res *= euler_phi(m);
    return res;
}

} // namespace hh
