#include <cpads/primality.hpp>
#include <gtest/gtest.h>
#include <iostream>

TEST(Primality, Pi1e8) {

    int total = 0;
    for (int n = 0; n < 100000000; n++) {
        total += hh::isprime(n);
    }

    EXPECT_EQ(total, 5761455);
}

TEST(Primality, Factor) {

    for (int n = 2; n < 100000000; n++) {
        if ((n % 10000000) == 0) { std::cout << n << std::endl; }
        int p = hh::find_prime_factor(n);
        EXPECT_EQ((n % p), 0);
        EXPECT_TRUE(hh::isprime(p));
    }
}

TEST(Primality, EulerPhi) {

    uint64_t total = 0;
    uint64_t expected = 303963552392ull;

    for (uint64_t n = 1; n <= 1000000; n++) {
        total += hh::euler_phi(n);
    }

    EXPECT_EQ(total, expected);
}
