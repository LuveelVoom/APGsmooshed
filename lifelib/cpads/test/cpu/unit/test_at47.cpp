#include <cpads/core.hpp>
#include <cpads/mxor.hpp>
#include <cpads/random/prng.hpp>
#include <gtest/gtest.h>

namespace {

const static uint64_t at47_sol[48] = {
    0x000000000000ull, 0x00020d950044ull, 0x0004039b0101ull, 0x000803752002ull,
    0x001003006507ull, 0x001301000004ull, 0x001403020501ull, 0x001803206002ull,
    0x00200e004440ull, 0x00649a020400ull, 0x00a856204000ull, 0x010000053057ull,
    0x010204050054ull, 0x010500010001ull, 0x010800453002ull, 0x03200400c058ull,
    0x0400000e1110ull, 0x042094028418ull, 0x045000029508ull, 0x0602940c0010ull,
    0x0ba85400c008ull, 0x0c0830461000ull, 0x0ca874028008ull, 0x0cd830029008ull,
    0x100000900367ull, 0x100208900064ull, 0x100400980301ull, 0x100900100002ull,
    0x302008000c68ull, 0x540000080398ull, 0x706498000c08ull, 0x742090000c98ull,
    0x760290080098ull, 0x7bdf10000008ull, 0x800000e02220ull, 0x802058204828ull,
    0x809000206a08ull, 0x840030481288ull, 0x8420e0008880ull, 0x84d030009a08ull,
    0x8900004030a8ull, 0x8b205000c0a8ull, 0xa00258c00020ull, 0xa602d0480088ull,
    0xab02504000a8ull, 0xc00430a80200ull, 0xc064b8200808ull, 0xc0d430200a08ull};

// Matrix A is right-multiplied by M (column perm)
// Matrix B is left-multiplied by N (row perm)
_HD_ uint64_t transform_row(uint64_t orig, uint16_t m, uint16_t n) {

    uint32_t x0 = hh::mxor4(((uint16_t) orig), m);
    uint32_t x1 = hh::mxor4(n, ((uint16_t) (orig >> 16)));
    return (orig & 0xffff00000000ull) ^ (x1 << 16) ^ x0;

}

_HD_ uint64_t at47_perm01(uint64_t orig) {
    uint32_t x = orig;
    x ^= ((x & 0xf0000) << 4) ^ ((x & 0xf00000) >> 4) ^ ((x & 0x1111) << 1) ^ ((x & 0x2222) >> 1);
    return orig ^ (x & 0xff3333);
}

_HD_ uint64_t at47_perm12(uint64_t orig) {
    uint32_t x = orig;
    x ^= ((x & 0xf00000) << 4) ^ ((x & 0xf000000) >> 4) ^ ((x & 0x2222) << 1) ^ ((x & 0x4444) >> 1);
    return orig ^ (x & 0xff06666);
}

_HD_ uint64_t at47_perm23(uint64_t orig) {
    uint32_t x = orig;
    x ^= ((x & 0xf000000) << 4) ^ ((x & 0xf0000000) >> 4) ^ ((x & 0x4444) << 1) ^ ((x & 0x8888) >> 1);
    return orig ^ (x & 0xff00cccc);
}


} // anonymous namespace

TEST(AT47, Perms) {

    hh::PRNG prng(1, 2, 3);

    for (int i = 0; i < 10000; i++) {
        uint64_t x = prng.generate64() & 0xffffffffffffull;
        EXPECT_EQ(at47_perm01(x), transform_row(x, 0x8412, 0x8412));
        EXPECT_EQ(at47_perm12(x), transform_row(x, 0x8241, 0x8241));
        EXPECT_EQ(at47_perm23(x), transform_row(x, 0x4821, 0x4821));
    }

}

TEST(AT47, ReadTensor) {

    /*
    for (uint32_t m = 0; m < 65536; m++) {

        uint16_t n = hh::pinv4(m);
        if (hh::mxor4(m, n) != 0x8421) { continue; }

        uint64_t transformed[48];

        for (int i = 0; i < 48; i++) {
            transformed[i] = transform_row(at47_sol[i], m, n);
        }

        for (int i = 0; i < 16; i++) {
            for (int j = 16; j < 32; j++) {
                for (int k = 32; k < 48; k++) {
                    uint64_t c = 0;
                    uint64_t d = 0;
                    for (int l = 0; l < 48; l++) {
                        uint64_t x = at47_sol[l];
                        c ^= (x >> i) & (x >> j) & (x >> k);
                        uint64_t y = transformed[l];
                        d ^= (y >> i) & (y >> j) & (y >> k);
                    }
                    EXPECT_EQ(c & 1, d & 1);
                }
            }
        }
    }
    */
}
