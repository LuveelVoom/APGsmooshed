#include <cpads/algebra/ff64.hpp>
#include <cpads/random/prng.hpp>
#include <gtest/gtest.h>


TEST(FF64, AddSub) {

    hh::PRNG prng(1, 2, 3);

    for (int i = 0; i < 10000; i++) {

        uint64_t a = prng.generate64();
        uint64_t b = prng.generate64();

        hh::ff64 x = a;
        hh::ff64 y = b;

        hh::ff64 z = x; z += y;

        EXPECT_EQ(z.x, (uint64_t) ((((__uint128_t) a) + b) % hh::ff64::modulus));
        z -= x;
        EXPECT_EQ(z.x, b);
        z -= x;
        EXPECT_EQ(z.x, (uint64_t) ((((__uint128_t) b) + hh::ff64::modulus - a) % hh::ff64::modulus));

    }

    {
        // check modular wraparound:
        hh::ff64 x = 0;
        x += 0x8000000000000000ull;
        x += 0x8000000000000000ull;
        EXPECT_EQ(x.x, 0xffffffffull);
    }
}


TEST(FF64, Reduce128) {

    hh::PRNG prng(1, 2, 3);

    for (int i = 0; i < 10000; i++) {

        __uint128_t x = 0;
        for (int j = 0; j < 4; j++) {
            x = (x << 32) + prng.generate();
        }

        hh::ff64 y((uint64_t) x, (uint64_t) (x >> 64));
        EXPECT_EQ(y.x, (uint64_t) (x % hh::ff64::modulus));

    }
}


TEST(FF64, LeftShift) {

    hh::PRNG prng(1, 2, 3);

    for (int i = 0; i < 500; i++) {
        for (int j = 0; j < 500; j++) {

            hh::ff64 x = prng.generate64();
            hh::ff64 y = x << i;
            hh::ff64 z = y << j;
            hh::ff64 w = x << (i + j);

            EXPECT_EQ(z.x, w.x);
        }
    }

    for (int i = 0; i < 100000; i++) {

        hh::ff64 x = prng.generate64();
        hh::ff64 y = x + x;
        hh::ff64 z = x << 1;

        EXPECT_EQ(y.x, z.x);
    }
}

TEST(FF64, PrimitiveRoot) {

    hh::PRNG prng(1, 2, 3);

    hh::ff64 r = 0x84000001;

    for (int i = 0; i < 10000; i++) {
        hh::ff64 x = prng.generate64();
        hh::ff64 y = x * r;
        hh::ff64 z = x + (x << 31) + (x << 26);

        EXPECT_EQ(z.x, y.x); // compare shift with multiplication

        x.advance();
        EXPECT_EQ(x.x, y.x); // compare with specialised advance() instruction
    }
}

TEST(FF64, RingOps) {

    hh::PRNG prng(1, 2, 3);

    for (int i = 0; i < 10000; i++) {

        hh::ff64 a = prng.generate64();
        hh::ff64 b = prng.generate64();
        hh::ff64 c = prng.generate64();

        EXPECT_EQ((a * b).x, (b * a).x);
        EXPECT_EQ(((a * b) * c).x, (a * (b * c)).x);

        EXPECT_EQ((a + b).x, (b + a).x);
        EXPECT_EQ(((a + b) + c).x, (a + (b + c)).x);

        EXPECT_EQ((a + (b - a)).x, b.x);

        // distributive law:
        EXPECT_EQ((a * (b + c)).x, ((a * b) + (a * c)).x);

        // powers of two:
        for (int j = 0; j < 64; j++) {
            hh::ff64 d = (1ull << j);
            EXPECT_EQ((a * d).x, (a << j).x);
        }
    }

}

/**
 * Alternative implementation of inversion using an addition chain.
 */
_HD_ hh::ff64 ff64_inverse_ac(const hh::ff64& a) {

    using namespace hh;

    ff64 a2 = a * a;
    ff64 a3 = a2 * a;
    ff64 a6 = a3 * a3;
    ff64 a7 = a6 * a;
    ff64 a14 = a7 * a7;
    ff64 a28 = a14 * a14;
    ff64 a56 = a28 * a28;
    ff64 a63 = a56 * a7;
    ff64 a126 = a63 * a63;
    ff64 a252 = a126 * a126;
    ff64 a504 = a252 * a252;
    ff64 a1008 = a504 * a504;
    ff64 a2016 = a1008 * a1008;
    ff64 a4032 = a2016 * a2016;
    ff64 a4095 = a4032 * a63;
    ff64 a8190 = a4095 * a4095;
    ff64 a16380 = a8190 * a8190;
    ff64 a32760 = a16380 * a16380;

    ff64 b = a32760 * a7;
    ff64 c = b;

    for (int i = 0; i < 15; i++) { c = c * c; }
    c = c * b; // 2^30 - 1
    c = c * c; // 2^31 - 2
    c = c * a; // 2^31 - 1

    ff64 d = c;
    for (int i = 0; i < 32; i++) { d = d * d; }
    d = d * c; // (2^31 - 1)(2^32 + 1)
    d = d * d; // (2^32 - 2)(2^32 + 1)
    d = d * a;

    return d;
}

template<typename Fn>
void run_inverse_test(int trials, Fn lambda) {

    hh::PRNG prng(1, 2, 3);
    for (int i = 0; i < trials; i++) {
        // generate a positive number
        hh::ff64 a = prng.generate64();
        if (a.x == 0) { a.x += 1; }
        lambda(a);
    }
}

template<typename Fn>
void run_timing_test(int trials, Fn lambda) {
    uint64_t res = 0;
    run_inverse_test(trials, [&](const hh::ff64 &a) { res += lambda(a); });
    std::cout << res << std::endl;
}

TEST(FF64, Inverse) {
    run_inverse_test(100000, [](const hh::ff64 &a) {
        hh::ff64 b = ff64_inverse_ac(a);
        hh::ff64 c = hh::ff64_inverse(b);
        EXPECT_EQ(a.x, c.x);
        EXPECT_EQ((a * b).x, 1);
        EXPECT_EQ((b * a).x, 1);
    });

    hh::ff64 zero = 0;

    // check that 0 maps to 0:
    EXPECT_EQ(ff64_inverse_ac(zero).x, 0);
    EXPECT_EQ(hh::ff64_inverse(zero).x, 0);
}

TEST(FF64, InverseNone) {
    run_timing_test(1000000, [](const hh::ff64 &a) { return a.x; });
}

TEST(FF64, InverseAC) {
    run_timing_test(1000000, [](const hh::ff64 &a) {
        hh::ff64 b = ff64_inverse_ac(a);
        return b.x;
    });
}

TEST(FF64, InverseGCD) {
    run_timing_test(1000000, [](const hh::ff64 &a) {
        hh::ff64 b = hh::ff64_inverse(a);
        return b.x;
    });
}

