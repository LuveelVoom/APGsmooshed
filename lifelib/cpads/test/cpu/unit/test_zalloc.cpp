#include <iostream>
#include <cpads/memory.hpp>
#include <gtest/gtest.h>

TEST(Memory, Zalloc) {

    uint8_t* x[5000];

    int y[257];
    for (int i = 0; i < 257; i++) {
        y[i] = 0;
    }

    // allocate memory
    for (int i = 0; i < 5000; i++) {

        x[i] = (uint8_t*) hh::zalloc(i);
        EXPECT_NE(x[i], nullptr);
        EXPECT_EQ((((uint64_t) x[i]) & 127), (uint64_t) 0);

        // check zero-initialised
        for (int j = 0; j < i; j++) {
            EXPECT_EQ(x[i][j], (uint8_t) 0);
        }

        y[x[i][-1]] += 1;
    }

    // deallocate memory
    for (int i = 0; i < 5000; i++) {
        hh::zfree(x[i]);
    }

    for (int i = 0; i < 257; i++) {
        if (y[i] > 0) {
            std::cout << i << ": " << y[i] << std::endl;
        }
    }
}

