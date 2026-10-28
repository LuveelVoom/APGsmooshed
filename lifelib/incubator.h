#pragma once

#include <map>
#include <utility>
#include <stdint.h>
#include <vector>

#include "bitworld.h"

namespace apg {

    template<int W, int H>
    struct Incube {
        uint64_t d[H];
        uint64_t hist[H];
        uint64_t gl[H];
        int tx; int ty;
        Incube<W, H>* neighbours[8];
    };

    template<int W, int H>
    class incubator {

        public:
        std::map<std::pair<int, int>, Incube<W, H> > tiles;

        Incube<W, H>* getNeighbour(Incube<W, H>* sqt, int i) {

            if ((sqt == 0) || (i == -1)) { return sqt; }

            if (!(sqt->neighbours[i])) {
                int x = sqt->tx;
                int y = sqt->ty;

                if ((146 >> i) & 1) { x += 1; }
                if ((73  >> i) & 1) { x -= 1; }
                if ((224 >> i) & 1) { y += 1; }
                if ((28  >> i) & 1) { y -= 1; }

                auto it = tiles.find(std::pair<int, int>(x, y));

                if (it != tiles.end()) {
                    sqt->neighbours[i] = &(it->second);
                } else {
                    sqt->neighbours[i] = sqt;
                }
            }

            Incube<W, H>* sn = sqt->neighbours[i];

            if (sn == sqt) {
                return 0;
            } else {
                return sn;
            }
        }

        int isGlider(Incube<W, H> *sqt, int px, int py) {

            if (sqt->gl[py] & (1ull << px)) { return 2; }

            if ((px < 2) || (py < 2) || (px > W - 3) || (py > H - 5)) { return 0; }

            int x = px;
            int y = py + 1;

            if ((sqt->d[y-2] | sqt->d[y+2]) & (31ull << (x - 2))) { return 0; }

            uint64_t projection = ((sqt->d[y+1] | sqt->d[y] | sqt->d[y-1]) >> (x - 2)) & 31ull;

            if (projection == 7) {
                x -= 1; // ..ooo
            } else if (projection == 14) {
                // .ooo.
            } else if (projection == 28) {
                x += 1; // ooo..
            } else {
                // The shadow does not match that of a glider.
                return 0;
            }

            // Now (x, y) should be the central cell of the putative glider.

            if ((x < 3) || (x > (W - 4))) {
                return 0;
            } else if ((sqt->d[y] | sqt->d[y-1] | sqt->d[y+1]) & (99ull << (x - 3))) {
                return 0;
            } else if ((sqt->d[y-2] | sqt->d[y+2]) & (127ull << (x - 3))) {
                return 0;
            } else if ((sqt->d[y-3] | sqt->d[y+3]) & (60ull << (x - 3))) {
                return 0;
            } else {
                // 512 bits to indicate which 16 of the 512 3-by-3 bitpatterns correspond
                // to a glider in some orientation and phase.
                unsigned long long array [] = {
                    0x0000000000000000ull,
                    0x0400000000800000ull,
                    0x0000000000000000ull,
                    0x0010044000200000ull,
                    0x0400000000800000ull,
                    0x0010004002000800ull,
                    0x0000040002200800ull,
                    0x0000000000000000ull};

                int high3 = ((sqt->d[y]) >> (x - 1)) & 7ull;
                int low6 = (((sqt->d[y-1]) >> (x - 1)) & 7ull) | ((((sqt->d[y+1]) >> (x - 1)) & 7ull) << 3);

                if (array[high3] & (1ull << low6)) {

                    sqt->gl[y-1] |= (7ull << (x - 1));
                    sqt->gl[y  ] |= (7ull << (x - 1));
                    sqt->gl[y+1] |= (7ull << (x - 1));

                    return 1;
                } else {
                    return 0;
                }
            }
        }

        int isBlinker(Incube<W, H>* sqt, int x, int y) {
            if ((x < 3) || (y < 3) || (x > W - 4) || (y > H - 4)) {
                return 0;
            } else if ((sqt->d[y  ] | sqt->hist[y  ]) & (99ull << (x - 3))) {
                return 0;
            } else if ((sqt->d[y-1] | sqt->hist[y-1] | sqt->d[y+1] | sqt->hist[y+1]) & (54ull << (x - 3))) {
                return 0;
            } else if ((sqt->d[y-2] | sqt->hist[y-2] | sqt->d[y+2] | sqt->hist[y+2]) & (62ull << (x - 3))) {
                return 0;
            } else if ((sqt->d[y-3] | sqt->hist[y-3] | sqt->d[y+3] | sqt->hist[y+3]) & ( 8ull << (x - 3))) {
                return 0;
            } else {
                // std::cout << "Blinker detected." << std::endl;
                sqt->d[y] &= (~(7ull << (x-1)));
                sqt->d[y-1] &= (~(1ull << x));
                sqt->d[y+1] &= (~(1ull << x));
                return 3;
            }
        }

        int isVerticalBeehive(Incube<W, H>* sqt, int x, int y) {
            if ((x < 3) || (y < 2) || (x > W - 4) || (y > H - 6)) {
                return 0;
            } else if ((sqt->d[y+1] | sqt->hist[y+1] | sqt->d[y+2] | sqt->hist[y+2]) & (107ull << (x - 3))) {
                return 0;
            } else if ((sqt->d[y  ] | sqt->hist[y  ] | sqt->d[y+3] | sqt->hist[y+3]) & (119ull << (x - 3))) {
                return 0;
            } else if ((sqt->d[y-1] | sqt->hist[y-1] | sqt->d[y+4] | sqt->hist[y+4]) & ( 62ull << (x - 3))) {
                return 0;
            } else if ((sqt->d[y-2] | sqt->hist[y-2] | sqt->d[y+5] | sqt->hist[y+5]) & (  8ull << (x - 3))) {
                return 0;
            } else {
                sqt->d[y  ] &= (~(1ull << x));
                sqt->d[y+1] &= (~(5ull << (x-1)));
                sqt->d[y+2] &= (~(5ull << (x-1)));
                sqt->d[y+3] &= (~(1ull << x));
                return 6;
            }
        }

        int isBlock(Incube<W, H>* sqt, int x, int y) {
            if ((sqt->d[y] | sqt->hist[y] | sqt->d[y+1] | sqt->hist[y+1]) & (51ull << (x - 2))) {
                return 0;
            } else if ((sqt->d[y-1] | sqt->hist[y-1] | sqt->d[y+2] | sqt->hist[y+2]) & (63ull << (x - 2))) {
                return 0;
            } else if ((sqt->d[y-2] | sqt->hist[y-2] | sqt->d[y+3] | sqt->hist[y+3]) & (30ull << (x - 2))) {
                return 0;
            } else {
                sqt->d[y  ] &= (~(3ull << x));
                sqt->d[y+1] &= (~(3ull << x));
                return 4;
            }
        }

        int isAnnoyance(Incube<W, H>* sqt, int x, int y) {

            if ((x < 2) || (y < 2) || (x > W - 4) || (y > H - 4)) { return 0; }

            if ((sqt->d[y] >> (x + 1)) & 1) {
                if ((sqt->d[y+1] >> x) & 1) {
                    return isBlock(sqt, x, y);
                } else {
                    return isBlinker(sqt, x+1, y);
                }
            } else {
                if ((sqt->d[y+1] >> x) & 1) {
                    return isBlinker(sqt, x, y+1);
                } else {
                    return isVerticalBeehive(sqt, x, y);
                }
            }
        }

        void purge(Incube<W, H>* sqt, uint64_t* excess) {
            for (int y = 0; y < H; y++) {
                uint64_t r = sqt->d[y];
                while (r != 0) {
                    uint64_t x = __builtin_ctzll(r);
                    int annoyance = isAnnoyance(sqt, x, y);
                    r ^= (1ull << x);
                    r &= sqt->d[y];
                    if ((annoyance > 0) && (excess != 0)) {
                        excess[annoyance] += 1;
                    }
                    if (annoyance == 0) { isGlider(sqt, x, y); }
                }
            }
        }

        void purge(uint64_t* excess) {
            for (auto it = tiles.begin(); it != tiles.end(); ++it) {
                purge(&(it->second), excess);
            }
        }

        void to_bitworld(bitworld &bw, int z) {
            for (auto it = tiles.begin(); it != tiles.end(); ++it) {
                Incube<W, H>* sqt = &(it->second);
                int64_t x = it->first.first * (W / 8);
                int64_t y = it->first.second * (H / 8);
                uint64_t* q = (z ? ((z == 2) ? sqt->gl : sqt->hist) : sqt->d);
                uint64_t f[8] = {0};
                for (uint64_t j = 0; j < (H / 8); j++) {
                    int bis = best_instruction_set();
                    if (bis >= 9) {
                        transpose_bytes_avx(q + (8*j), f);
                    } else {
                        transpose_bytes_sse2(q + (8*j), f);
                    }
                    for (uint64_t i = 0; i < (W / 8); i++) {
                        if (f[i]) { bw.world.emplace(std::pair<int32_t, int32_t>(x + i, y + j), f[i]); }
                    }
                }
            }
        }

        void setcell(Incube<W, H>* sqt, int x, int y, int state) {

            uint64_t mask = (1ull << x);

            if (state == 1) {
                sqt->d[y] |= mask;
            } else {
                sqt->d[y] &= ~mask;
                if (state == 2) {
                    sqt->hist[y] |= mask;
                } else {
                    sqt->hist[y] &= ~mask;
                }
            }

        }

        int getcell(Incube<W, H>* sqt, int x, int y) {

            uint64_t mask = (1ull << x);

            if (sqt->d[y] & mask) {
                return 1;
            } else {
                if (sqt->hist[y] & mask) {
                    return 2;
                } else {
                    return 0;
                }
            }
        }

        std::vector<int> get_component(Incube<W, H>* sqt, int x, int y) {

            int i = 0;
            std::vector<Incube<W, H>*> tileList;
            std::vector<int> intList;
            int ll = 1;

            tileList.push_back(sqt);
            intList.push_back(x);
            intList.push_back(y);
            intList.push_back(getcell(sqt, x, y));
            setcell(sqt, x, y, 0);

            int population = 1;

            while (i < ll) {

                Incube<W, H>* sqt1 = tileList[i];
                int ox = intList[3*i];
                int oy = intList[3*i + 1];

                for (int rx = -2; rx <= 2; rx++) {
                    for (int ry = -2; ry <= 2; ry++) {

                        int norm = rx*rx + ry*ry;

                        if (norm <= 5) {

                            int px = rx + ox;
                            int py = ry + oy;

                            int nj = -1;
                            if (px < 0) { px += W; nj += 1; } else if (px >= W) { px -= W; nj += 2; }
                            if (py < 0) { py += H; nj += 3; } else if (py >= H) { py -= H; nj += 6; }

                            auto sqt2 = getNeighbour(sqt1, nj);

                            int v = (sqt2 ? getcell(sqt2, px, py) : 0);

                            if (v > 0) {
                                setcell(sqt2, px, py, 0);

                                tileList.push_back(sqt2);
                                intList.push_back(px);
                                intList.push_back(py);
                                intList.push_back(v);
                                ll += 1;

                                if (v == 1) { population += 1; }
                            }
                        }
                    }
                }

                i += 1;
            }

            for (int j = 0; j < ll; j++) {
                auto sqt3 = tileList[j];
                intList[j*3] += W * sqt3->tx;
                intList[j*3 + 1] += H * sqt3->ty;
            }

            intList.push_back(population);

            return intList;

        }

    };
}
