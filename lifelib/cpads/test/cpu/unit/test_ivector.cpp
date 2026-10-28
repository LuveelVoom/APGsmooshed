#include <algorithm>
#include <cpads/random/prng.hpp>
#include <cpads/ivector.hpp>
#include "gtest/gtest.h"


TEST(Ivector, RangeLoop) {

    hh::ivector<uint32_t> w;

    hh::PRNG prng(1, 2, 3);
    for (int i = 0; i < 1000000; i++) {
        uint32_t x = prng.generate();
        w.push_back(x);
    }

    int j = 0;
    for (auto&& x : w) {
        uint32_t y = w[j]; j += 1;
        EXPECT_EQ(x, y);
    }

    EXPECT_EQ(j, 1000000);
}


TEST(Ivector, Sorting) {

    std::vector<uint32_t> v;
    hh::ivector<uint32_t> w;

    hh::PRNG prng(1, 2, 3);

    for (int i = 0; i < 1000000; i++) {

        uint32_t x = prng.generate();
        v.push_back(x);
        w.push_back(x);

    }

    std::sort(v.begin(), v.end());
    std::sort(w.begin(), w.end());

    for (int i = 0; i < 1000000; i++) {
        EXPECT_EQ(v[i], w[i]);
    }
}

