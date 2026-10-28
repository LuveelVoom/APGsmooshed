#pragma once

#include "../core.hpp"

namespace hh { namespace random {

/**
 * Chacha20 quarter-round mixing function.
 */
_HD_ void quarterRound(uint32_t &a, uint32_t &b, uint32_t &c, uint32_t &d) {
    a += b; d ^= a; d = (d << 16) | (d >> 16);
    c += d; b ^= c; b = (b << 12) | (b >> 20);
    a += b; d ^= a; d = (d <<  8) | (d >>  8);
    c += d; b ^= c; b = (b <<  7) | (b >>  7);
}

struct Initialiser {

    uint32_t l0, h0, l1, h1, l2, h2;

    _HD_ Initialiser(uint64_t w0, uint64_t w1, uint64_t w2) :
        l0(w0), h0(w0 >> 32), l1(w1), h1(w1 >> 32), l2(w2), h2(w2 >> 32) {

        // add constants to prevent 0 from being a fixed point:
        h0 += 3605551297u; // NextPrime[10^9 Sqrt[13]]
        h1 += 3316624793u; // NextPrime[10^9 Sqrt[11]]
        h2 += 2645751323u; // NextPrime[10^9 Sqrt[7]]

        for (int i = 0; i < 4; i++) {
            quarterRound(l0, l1, h0, h1);
            l0 *= 1414213573u; // NextPrime[10^9 Sqrt[2]]
            quarterRound(l1, l2, h1, h2);
            l1 *= 1732050821u; // NextPrime[10^9 Sqrt[3]]
            quarterRound(l2, l0, h2, h0);
            l2 *= 2236067989u; // NextPrime[10^9 Sqrt[5]]
        }
    }
};


/**
 * CRTP Base class for all pseudo-random number generators. Derived
 * classes are required to implement:
 *
 * void initialise_inner() -- populates the state from a random input;
 * uint64_t output_function() -- where the upper 32 bits are high-quality;
 * void update_state() -- increments the internal state;
 *
 * and exposes a function generate() for outputting a uniform random
 * uint32_t and updating the internal state.
 */
template<typename MostDerivedClass>
struct PRNG_base {

    _HD_ void initialise(uint64_t w0, uint64_t w1, uint64_t w2) {

        // apply the ChaCha-based mixing function:
        Initialiser i(w0, w1, w2);

        // populate the state with the output of the mixing function:
        static_cast<MostDerivedClass&>(*this).initialise_inner(i);
    }

    _HD_ uint32_t generate() {

        auto x = static_cast<const MostDerivedClass&>(*this).output_function();
        static_cast<MostDerivedClass&>(*this).update_state();

        // truncate output to upper 32 bits:
        constexpr int shiftamt = 8 * sizeof(x) - 32;
        return (uint32_t) (x >> shiftamt);
    }

    _HD_ uint64_t generate64() {

        uint32_t upper = generate();
        uint32_t lower = generate();

        return (((uint64_t) upper) << 32) | lower;

    }
};


/**
 * Variant of Melissa O'Neill's permuted congruential generator.
 * The output function is a variable xorshift followed by a
 * quadratic permutation implemented with a single multiplication.
 */
struct PCG64 : PRNG_base<PCG64> {

    uint64_t state;

    _HD_ uint64_t output_function(uint64_t odd_constant = 12605985483714917081ull) const {

        // variable xorshift permutation:
        uint64_t xorshifted = ((state >> ((state >> 59u) + 5u)) ^ state);

        // algebraic permutation x --> 2x^2 + kx:
        uint64_t multiplier = xorshifted + xorshifted + odd_constant;
        uint64_t output = xorshifted * multiplier;

        return output;
    }

    _HD_ void update_state() {

        state = state * 6364136223846793005ull + 3511ull;

    }

    _HD_ void initialise_inner(const Initialiser &i) {

        state = (((uint64_t) i.h2) << 32) | i.l2;

    }
};


/**
 * Sebastiano Vigna's XorShift128+ generator, adapted from an LFSR
 * by George Marsaglia. We truncate the usual 64-bit output to only
 * its upper 32 bits, because the lowest bit is just an LFSR.
 */
struct XorShift128 : PRNG_base<XorShift128> {

    uint64_t state0;
    uint64_t state1;

    _HD_ uint64_t output_function() const {

        return state0 + state1;

    }

    _HD_ void update_state() {

        uint64_t s1 = state0;
        uint64_t s0 = state1;
        state0 = s0;
        s1 ^= s1 << 23;
        s1 ^= s1 >> 17;
        s1 ^= s0;
        s1 ^= s0 >> 26;
        state1 = s1;

    }

    _HD_ void initialise_inner(const Initialiser &i) {

        // prevent all-zeroes initialisation
        uint32_t nonzero_word = (i.l0 == 0) ? 4123105643u : i.l0;
        state0 = (((uint64_t) i.h0) << 32) | nonzero_word;
        state1 = (((uint64_t) i.h1) << 32) | i.l1;

    }
};


/**
 * Hybrid of Melissa O'Neill's PCG64 and Sebastiano Vigna's XorShift128+
 * This produces high-quality random uint32s with a period of 2**192 - 2**64
 */
struct Hybrid192 : PRNG_base<Hybrid192> {

    _HD_ Hybrid192(uint64_t w0, uint64_t w1, uint64_t w2) { initialise(w0, w1, w2); }

    PCG64 pcg;
    XorShift128 lfsr;

    _HD_ uint64_t output_function() const {
        uint64_t odd_constant = lfsr.output_function() | 1;
        uint64_t output = pcg.output_function(odd_constant);
        return output;
    }

    _HD_ void update_state() {
        pcg.update_state();
        lfsr.update_state();
    }

    _HD_ void initialise_inner(const Initialiser &i) {
        pcg.initialise_inner(i);
        lfsr.initialise_inner(i);
    }

};

static_assert(sizeof(PCG64) == 8, "PCG64 should be 8 bytes");
static_assert(sizeof(XorShift128) == 16, "XorShift128 should be 16 bytes");
static_assert(sizeof(Hybrid192) == 24, "Hybrid192 should be 24 bytes");

} // namespace random

// default statistical sequence generator:
using PRNG = hh::random::Hybrid192;

} // namespace hh
