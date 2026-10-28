#include <cpads/montgomery.hpp>
#include <cpads/random/prng.hpp>
#include <gtest/gtest.h>

TEST(Montgomery, ModMult) {

    hh::PRNG pcg(1, 2, 3);

    for (int i = 0; i < 1000000; i++) {

        uint32_t p = pcg.generate() | 1u;
        if (p < 1000) { continue; }

        uint32_t a = pcg.generate() % p;
        uint32_t b = pcg.generate() % p;

        uint32_t expected = (((uint64_t) a) * b) % p;

        hh::Montgomery<uint32_t, uint64_t> mont(p);

        uint32_t ma = mont.transform(a);
        uint32_t mb = mont.transform(b);
        uint32_t mc = mont.multiply(ma, mb);
        uint32_t  c = mont.detransform(mc);

        EXPECT_EQ(c, expected);
    }
}

TEST(Montgomery, ModExp) {

    hh::PRNG pcg(1, 2, 3);

    for (int i = 0; i < 1000000; i++) {
        uint32_t p = pcg.generate() | 1u;
        if (p < 1000) { continue; }

        uint32_t a = pcg.generate() % p;
        uint32_t power = pcg.generate() & 15;

        uint64_t expected = 1;
        for (uint32_t i = 0; i < power; i++) {
            expected = (expected * a) % p;
        }

        hh::Montgomery<uint32_t, uint64_t> mont(p);
        uint32_t ma = mont.transform(a);
        uint32_t mc = mont.modexp(ma, power);
        uint32_t  c = mont.detransform(mc);

        EXPECT_EQ(c, expected);
    }
}

TEST(Montgomery, SPRP) {

    int n = 0;
    for (int p = 3; p < 10000000; p += 2) {
        hh::Montgomery<uint32_t, uint64_t> mont(p);
        n += mont.strong_probable_prime(2);
    }

    // there are 664578 odd primes and 162 strong pseudoprimes:
    EXPECT_EQ(n, 664740);
}
