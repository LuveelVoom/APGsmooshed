#include <cpads/core.hpp>
#include "gtest/gtest.h"


template<typename Fn>
uint64_t run_gcd_test(Fn lambda, uint64_t limit) {

    uint64_t z = 0;

    for (uint64_t x = 0; x < limit; x++) {
        for (uint64_t y = 0; y < limit; y++) {
            z += lambda(x, y);
        }
    }

    return z;
}


TEST(GCD, Binary) {
    auto z = run_gcd_test(hh::binary_gcd, 1000);
    EXPECT_EQ(z, 5432880);
}


TEST(Core, FloorSqrt) {

    for (uint64_t y = 0; y < 100; y++) {
        for (uint64_t x = y*y; x < (y+1)*(y+1); x++) {
            EXPECT_EQ(hh::floor_sqrt(x), y);
        }
    }

    EXPECT_EQ(hh::floor_sqrt((uint64_t) -1), 0xffffffffull);

}
