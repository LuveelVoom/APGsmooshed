#include <cpads/mxor.hpp>
#include <cpads/random/prng.hpp>
#include <gtest/gtest.h>


TEST(MXOR, FourByFour) {

    using namespace hh;

    hh::PRNG prng(1, 2, 3);

    uint16_t id = 0x8421;

    for (int i = 0; i < 1000000; i++) {

        uint16_t a = prng.generate();
        uint16_t b = prng.generate();
        uint16_t c = prng.generate();

        EXPECT_EQ(mxor4(mxor4(a, b), c), mxor4(a, mxor4(b, c)));
        EXPECT_EQ(mxor4(a, b ^ c), mxor4(a, b) ^ mxor4(a, c));
        EXPECT_EQ(mxor4(a ^ b, c), mxor4(a, c) ^ mxor4(b, c));

        EXPECT_EQ(mxor4(a, id), a);
        EXPECT_EQ(mxor4(id, a), a);
    }
}

TEST(MXOR, transpose4) {

    using namespace hh;
    hh::PRNG prng(1, 2, 3);

    for (int i = 0; i < 100000; i++) {
        uint16_t a = prng.generate();
        uint16_t b = prng.generate();

        // A^T^T == A
        EXPECT_EQ(transpose4(transpose4(a)), a);

        // (AB)^T == B^T A^T
        EXPECT_EQ(transpose4(mxor4(a, b)), mxor4(transpose4(b), transpose4(a)));
    }
}

TEST(MXOR, pinv4) {

    uint64_t total = 0;
    std::vector<uint32_t> x;

    for (uint32_t i = 0; i < 65536; i++) {
        uint16_t p = hh::pinv4(i);
        uint16_t q = hh::mxor4(p, i);
        if (q != 0x8421) { continue; }

        total += 1;

        uint16_t i0 = i & 15;
        uint16_t i1 = (i >> 4) & 15;
        uint16_t i2 = (i >> 8) & 15;
        uint16_t i3 = (i >> 12) & 15;

        if ((i0 < i1) && (i1 < i2) && (i2 < i3)) {
            x.push_back(i | (((uint32_t) p) << 16));
        }
    }

    EXPECT_EQ(total, 20160);
    ASSERT_EQ(x.size(), 840);

    /*
    for (size_t i = 0; i < 840; i++) {
        std::cout << x[i] << "," << (((i & 7) == 7) ? '\n' : ' ');
    }
    std::cout << std::endl;
    */
}
