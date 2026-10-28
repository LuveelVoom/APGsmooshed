#include <cpads/bdd.hpp>
#include <cpads/random/prng.hpp>
#include <gtest/gtest.h>
#include <iostream>


template<int ngens, int r>
uint64_t getprob(uint64_t hint) {

    hh::BDD bdd;

    uint32_t   a[ngens + 1][2*ngens + 1][2*ngens + 1];

    uint32_t  s0[ngens + 1][2*ngens + 1][2*ngens + 1];
    uint32_t sh2[ngens + 1][2*ngens + 1][2*ngens + 1];
    uint32_t  a0[ngens + 1][2*ngens + 1][2*ngens + 1];
    uint32_t  a1[ngens + 1][2*ngens + 1][2*ngens + 1];

    int p = 0;
    int q = 0;

    for (int i = 0; i < 2*ngens+1; i++) {
        for (int j = 0; j < 2*ngens+1; j++) {

            if ((i >= ngens-r) && (i <= ngens+r) && (j >= ngens-r) && (j <= ngens+r)) {
                a[0][i][j] = 1 & (hint >> (q++));
            } else {
                a[0][i][j] = bdd.var2node(++p);
            }
        }
    }

    // std::cout << p << " primary inputs." << std::endl;

    for (int k = 0; k < ngens; k++) {
        for (int i = k; i < 2*ngens+1-k; i++) {
            for (int j = k+1; j < 2*ngens-k; j++) {

                 s0[k][i][j] = bdd.xor_nodes(a[k][i][j-1], a[k][i][j+1]);
                sh2[k][i][j] = bdd.and_nodes(a[k][i][j-1], a[k][i][j+1]);

                 a0[k][i][j] = bdd.xor_nodes(s0[k][i][j], a[k][i][j]);
                 a1[k][i][j] = bdd.or_nodes(bdd.and_nodes(s0[k][i][j], a[k][i][j]), sh2[k][i][j]);
            }
        }

        for (int i = k+1; i < 2*ngens-k; i++) {
            for (int j = k+1; j < 2*ngens-k; j++) {

                auto y = a1[k][i-1][j];
                auto x = a1[k][i+1][j];

                auto tx = bdd.xor_nodes(a0[k][i-1][j], a0[k][i+1][j]);
                auto ty = bdd.and_nodes(a0[k][i-1][j], a0[k][i+1][j]);

                auto sll = bdd.xor_nodes(s0[k][i][j], tx);
                auto slh = bdd.or_nodes(bdd.and_nodes(s0[k][i][j], tx), ty);

                auto parity = bdd.xor_nodes(bdd.xor_nodes(x, y), bdd.xor_nodes(sh2[k][i][j], slh));
                auto exclude3 = bdd.xor_nodes(bdd.or_nodes(x, y), bdd.or_nodes(sh2[k][i][j], slh));

                a[k+1][i][j] = bdd.and_nodes(bdd.and_nodes(parity, exclude3), bdd.or_nodes(sll, a[k][i][j]));
            }
        }
    }

    return (bdd.get_prob(a[ngens][ngens][ngens]) >> (64 - p));

}


inline uint64_t transpose9(uint64_t x) {

    return (x & 0x111) | ((x & 0x22) << 2) | ((x & 0x88) >> 2) | ((x & 0x04) << 4) | ((x & 0x40) >> 4);

}

inline uint64_t vflip9(uint64_t x) {

    return (x & 0x38) | ((x & 0x07) << 6) | ((x & 0x1c0) >> 6);

}

inline uint64_t hflip9(uint64_t x) {

    return (x & 0x92) | ((x & 0x49) << 2) | ((x & 0x124) >> 2);

}


template<int ngens>
uint64_t getprob_full() {

    uint64_t acc = 0;

    uint64_t mults[512] = {0};

    for (uint64_t hint = 0; hint < 512; hint++) {

        uint64_t images[8];
        images[0] = hint;
        images[1] = transpose9(images[0]);
        images[2] = vflip9(images[0]);
        images[3] = vflip9(images[1]);
        images[4] = hflip9(images[0]);
        images[5] = hflip9(images[1]);
        images[6] = hflip9(images[2]);
        images[7] = hflip9(images[3]);

        uint64_t a = hint;
        for (int i = 0; i < 8; i++) { if (images[i] < a) { a = images[i]; } }
        mults[a] += 1;

    }

    for (uint64_t hint = 0; hint < 512; hint++) {
        if (mults[hint] > 0) {
            acc += getprob<ngens, 1>(hint) * mults[hint];
            std::cout << "." << std::flush;
        }
    }

    std::cout << "\n" << acc << std::endl;

    return acc;
}


TEST(BDD, CGoL_5x5) {

    uint64_t result = getprob_full<2>();

    // check against Oscar Cunningham's brute-force calculation of the
    // same answer:
    EXPECT_EQ(result, ((uint64_t) 8502430));

}

TEST(BDD, CGoL_7x7) {

    uint64_t result = getprob_full<3>();
    EXPECT_EQ(result, ((uint64_t) 140974273630514));

}

TEST(BDD, RandomConsistency) {

    hh::BDD bdd;
    hh::PRNG prng(3, 4, 5);

    std::vector<uint32_t> vars;

    for (int i = 0; i < 20; i++) { vars.push_back(bdd.var2node(i)); }
    
    for (int j = 0; j < 1000000; j++) {
        uint32_t v1 = vars[prng.generate() % vars.size()];
        uint32_t v2 = vars[prng.generate() % vars.size()];

        uint32_t op1 = prng.generate() & 15;
        uint32_t op2 = prng.generate() & 15;

        uint32_t w1 = bdd.apply_gate(v1, v2, op1);
        uint32_t w2 = bdd.apply_gate(v1, v2, op2);
        uint32_t w3 = bdd.apply_gate(v1, v2, op2 ^ op1);
        uint32_t w4 = bdd.apply_gate(v1, v2, op2 & op1);
        uint32_t w5 = bdd.apply_gate(v1, v2, op2 | op1);

        EXPECT_EQ(w3, bdd.xor_nodes(w1, w2));
        EXPECT_EQ(w4, bdd.and_nodes(w1, w2));
        EXPECT_EQ(w5,  bdd.or_nodes(w1, w2));

        vars.push_back(w3);
    }

    std::cout << bdd.size() << std::endl;
}

TEST(BDD, CreateVariables) {

    hh::BDD bdd;

    uint32_t a1 = bdd.var2node(37);
    uint32_t b1 = bdd.var2node(42);
    uint32_t c1 = bdd.var2node(15);

    uint32_t zero = 0;

    EXPECT_NE(a1, zero);
    EXPECT_NE(b1, zero);
    EXPECT_NE(c1, zero);
    EXPECT_NE(a1, b1);
    EXPECT_NE(b1, c1);
    EXPECT_NE(c1, a1);

    uint32_t b2 = bdd.var2node(42);
    uint32_t a2 = bdd.var2node(37);
    uint32_t c2 = bdd.var2node(15);

    EXPECT_EQ(a1, a2);
    EXPECT_EQ(b1, b2);
    EXPECT_EQ(c1, c2);

    // Test XOR:
    uint32_t a_xor_b = bdd.xor_nodes(a1, b1);
    uint32_t b_xor_a = bdd.xor_nodes(b1, a1);
    EXPECT_EQ(a_xor_b, b_xor_a);
    uint32_t a3 = bdd.xor_nodes(b1, a_xor_b);
    EXPECT_EQ(a1, a3);

    // Test AND:
    uint32_t a_and_b = bdd.and_nodes(a1, b1);
    uint32_t b_and_a = bdd.and_nodes(b1, a1);
    EXPECT_EQ(a_and_b, b_and_a);
    uint32_t b_and_a_and_a = bdd.and_nodes(a1, b_and_a);
    EXPECT_EQ(a_and_b, b_and_a_and_a);

    // TEST_OR:
    uint32_t a_or_b = bdd.or_nodes(a1, b1);
    uint32_t a_xor_b_xor_a_and_b = bdd.xor_nodes(a_and_b, a_xor_b);
    EXPECT_EQ(a_or_b, a_xor_b_xor_a_and_b);

}


TEST(BDD, ThreeInputFunctions) {

    hh::BDD bdd;

    uint32_t implicants[8];

    uint32_t a = bdd.var2node(1);
    uint32_t b = bdd.var2node(2);
    uint32_t c = bdd.var2node(3);

    for (int i = 0; i < 8; i++) {
        implicants[i] = bdd.and_nodes(bdd.and_nodes(
            (i & 1) ? (a ^ 1) : a,
            (i & 2) ? (b ^ 1) : b),
            (i & 4) ? (c ^ 1) : c);
        std::cout << implicants[i] << std::endl;
    }

    uint32_t indices[256];

    for (int i = 0; i < 256; i++) {
        uint32_t x = 0;
        for (int j = 0; j < 8; j++) {
            if ((i >> j) & 1) {
                x = bdd.or_nodes(x, implicants[j]);
            }
        }

        EXPECT_EQ(bdd.get_prob(x), ((uint64_t) hh::popc32(i)) << 61);

        indices[i] = x;
    }

    for (int i = 1; i < 256; i++) {
        for (int j = 0; j < i; j++) {
            EXPECT_NE(indices[i], indices[j]);

            {
                uint32_t expected = indices[i ^ j];
                uint32_t realised = bdd.xor_nodes(indices[i], indices[j]);
                EXPECT_EQ(expected, realised);
            }
            {
                uint32_t expected = indices[i & j];
                uint32_t realised = bdd.and_nodes(indices[i], indices[j]);
                EXPECT_EQ(expected, realised);
            }
            {
                uint32_t expected = indices[i | j];
                uint32_t realised = bdd.or_nodes(indices[i], indices[j]);
                EXPECT_EQ(expected, realised);
            }
        }
    }

    for (int i = 2; i < 256; i++) {
        auto entry = bdd.retrieve(i);
        EXPECT_GT(entry.key.variable, bdd.retrieve(entry.key.false_idx).key.variable);
        EXPECT_GT(entry.key.variable, bdd.retrieve(entry.key.true_idx).key.variable);
    }
}
