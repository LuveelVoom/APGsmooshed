#include <cpads/bitswap.hpp>
#include <cpads/random/prng.hpp>
#include "gtest/gtest.h"


TEST(Bitswap, Conjugation) {

    hh::PRNG prng(3, 4, 5);

    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < i; j++) {

            hh::vec<uint64_t, 16> v;
            for (int a = 0; a < 16; a++) { v[a] = prng.generate64(); }

            auto actual = hh::bitswap(hh::bitflip(v, i), i, j);
            auto expected = hh::bitflip(hh::bitswap(v, i, j), j);

            EXPECT_EQ(actual, expected);
        }
    }
}


TEST(Bitswap, SelfInverse) {

    hh::PRNG prng(3, 4, 5);

    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < i; j++) {

            hh::vec<uint64_t, 16> v;
            for (int a = 0; a < 16; a++) { v[a] = prng.generate64(); }

            auto w = hh::bitswap(v, i, j);

            hh::vec<uint32_t, 32> w32;
            memcpy(&w32, &w, 128);
            hh::bitswap_inplace(w32, i, j);
            memcpy(&w, &w32, 128);

            EXPECT_EQ(v, w);

        }
    }
}


TEST(Bitswap, Transitive) {

    hh::PRNG prng(3, 4, 5);

    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < i; j++) {
            for (int k = 0; k < j; k++) {

                hh::vec<uint64_t, 16> v;
                for (int a = 0; a < 16; a++) { v[a] = prng.generate64(); }
                auto actual = hh::bitswap(hh::bitswap(hh::bitswap(v, i, j), j, k), i, j);
                auto expected = hh::bitswap(v, i, k);
                EXPECT_EQ(actual, expected);

                if (actual != expected) {
                    std::cerr << "Error at " << i << " " << j << " " << k << std::endl;
                }

            }
        }
    }
}
