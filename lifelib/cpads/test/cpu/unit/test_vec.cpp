#include <cpads/vec.hpp>
#include <gtest/gtest.h>

namespace {

struct ComparisonCountingInt {

    int x;

};

int cc = 0;

bool operator<(const ComparisonCountingInt &lhs, const ComparisonCountingInt &rhs) {

    cc += 1;
    return (lhs.x < rhs.x);

}

template<typename T>
void sortAndCount(T &v) {

    cc = 0;
    v.sort();

}

template<size_t N>
void checkSorted(int expected) {

    hh::vec<ComparisonCountingInt, N> v;

    for (uint64_t i = 0; i < (1ull << N); i++) {

        for (size_t j = 0; j < N; j++) {
            v[j].x = (i >> j) & 1;
        }

        sortAndCount(v);

        for (size_t j = 0; j < N-1; j++) {
            int lhs = v[j].x;
            int rhs = v[j+1].x;
            EXPECT_LE(lhs, rhs);
        }

    }

    EXPECT_EQ(cc, expected);

}

} // anonymous namespace

TEST(Vector, Sorting) {

    checkSorted<1>(0);
    checkSorted<2>(1);
    checkSorted<3>(3);
    checkSorted<4>(5);
    checkSorted<5>(9);
    checkSorted<6>(12);
    checkSorted<7>(16);
    checkSorted<8>(19);
    checkSorted<9>(26);  // 25
    checkSorted<10>(31); // 29
    checkSorted<11>(37); // 35
    checkSorted<12>(41); // 39
    checkSorted<13>(48); // 45
    checkSorted<14>(53); // 51
    checkSorted<15>(59); // 56
    checkSorted<16>(63); // 60
    checkSorted<17>(74); // 71
    checkSorted<18>(82); // 77
    checkSorted<19>(91); // 85
    checkSorted<20>(97); // 91

}


TEST(Vector, SegmentedSorting) {

    hh::vec<int, 8> a{42, 98, -13, 7, 3511, 65, 0, 0};

    // sort descending:
    a.sort(true);
    EXPECT_EQ(a[7], -13);
    EXPECT_EQ(a[6], 0);
    EXPECT_EQ(a[5], 0);
    EXPECT_EQ(a[4], 7);
    EXPECT_EQ(a[3], 42);
    EXPECT_EQ(a[2], 65);
    EXPECT_EQ(a[1], 98);
    EXPECT_EQ(a[0], 3511);

    // sort ascending:
    a.sort();
    EXPECT_EQ(a[0], -13);
    EXPECT_EQ(a[1], 0);
    EXPECT_EQ(a[2], 0);
    EXPECT_EQ(a[3], 7);
    EXPECT_EQ(a[4], 42);
    EXPECT_EQ(a[5], 65);
    EXPECT_EQ(a[6], 98);
    EXPECT_EQ(a[7], 3511);

    hh::vec<int, 8> b{42, 0, -14, 55, 8, 69, 2, 1};

    b.segmentedSortReplace(a, std::index_sequence<3, 3, 2>{});

    EXPECT_EQ(a[0], -14);
    EXPECT_EQ(a[1], 0);
    EXPECT_EQ(a[2], 42);
    EXPECT_EQ(a[3], 8);
    EXPECT_EQ(a[4], 55);
    EXPECT_EQ(a[5], 69);
    EXPECT_EQ(a[6], 1);
    EXPECT_EQ(a[7], 2);

}


TEST(Vector, ElementwiseOps) {

    hh::vec<int, 3> a{3, 5, 7};
    hh::vec<int, 3> b{42, -6, 8};

    auto plus  = a + b;
    auto minus = a - b;
    auto times = a * b;

    EXPECT_EQ(plus[0], 45);
    EXPECT_EQ(plus[1], -1);
    EXPECT_EQ(plus[2], 15);

    EXPECT_EQ(minus[0], -39);
    EXPECT_EQ(minus[1], 11);
    EXPECT_EQ(minus[2], -1);

    EXPECT_EQ(times[0], 126);
    EXPECT_EQ(times[1], -30);
    EXPECT_EQ(times[2], 56);

}


TEST(Vector, Concatenation) {

    hh::vec<int, 3> a{3, 5, 7};
    hh::vec<int, 3> b{42, -6, 8};
    auto c = a.concat(b).concat(b);

    EXPECT_EQ(c[0], 3);
    EXPECT_EQ(c[1], 5);
    EXPECT_EQ(c[2], 7);
    EXPECT_EQ(c[3], 42);
    EXPECT_EQ(c[4], -6);
    EXPECT_EQ(c[5], 8);
    EXPECT_EQ(c[6], 42);
    EXPECT_EQ(c[7], -6);
    EXPECT_EQ(c[8], 8);

}

