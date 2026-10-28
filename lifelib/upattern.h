#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>
#include <type_traits>

#include "avxlife/uli.h"
#include "avxlife/lifeconsts.h"
#include "hashtrees/kivtable.h"

#include "bitworld.h"
#include "incubator.h"

namespace apg {

    const static uint32_t __16x3fffffff[] __attribute__((aligned(64))) = MAKE_SIXTEEN(0x3fffffffu);
    const static uint32_t __16xfffffffc[] __attribute__((aligned(64))) = MAKE_SIXTEEN(0xfffffffcu);

    const uint64_t udirections[] = {0x0000000000000001ull,
                                    0x0000000100000001ull,
                                    0x0000000100000000ull,
                                    0xffffffffffffffffull,
                                    0xfffffffeffffffffull,
                                    0xffffffff00000000ull};


    // Tile typename, width and height:
    template<typename T, int W, int H = W>
    class upattern {
        /*
        * A container capable of running patterns in either (unhashed) ulife
        * or vlife. Appropriate for highly chaotic patterns such as random
        * soups, where there is no regularity other than period-2 detritus.
        * Otherwise, apg::pattern is a more appropriate choice of container.
        *
        * The upattern can support either an unbounded plane or a rectangular
        * torus whose width and height are divisible, respectively, by W and
        * 2H. In the former case, new tiles are allocated as the pattern
        * expands; in the latter case, the entire universe is preallocated in
        * memory at creation time.
        */

        private:
        uint64_t torus_width;
        uint64_t torus_height;
        uint64_t lastmant;

        public:
        indirected_map<uint64_t, T> tiles;
        std::vector<T*> modified;
        std::vector<T*> temp_modified;
        std::vector<T*> popchanged;

        uint64_t tilesProcessed;
        uint64_t gensElapsed;
        int population;
        int extremal_mask;
        int glider_count;

        int64_t max_xpy;
        int64_t min_xpy;
        int64_t max_xmy;
        int64_t min_xmy;

        T* coords2ptr(int64_t x, int64_t w) {
            // Returns a pointer to tile x + omega*w.
            int64_t ix = x; int64_t iw = w;
            if (torus_width > 0) {
                // Rectangular torus implemented on a hexagonal grid:
                iw = ((iw % torus_height) + torus_height) % torus_height;
                ix -= (w - iw) / 2;
                ix = ((ix % torus_width) + torus_width) % torus_width;
            }
            uint64_t p = ((uint64_t) (ix + 0x80000000ull));
            p += (((uint64_t) (iw + 0x80000000ull)) << 32);
            T* sqt = &(tiles[p]);
            sqt->coords = p;
            return sqt;
        }

        upattern() {
            // Construct an unbounded plane universe:
            tilesProcessed = 0;
            gensElapsed = 0;
            torus_width = 0;
            torus_height = 0;
            lastmant = 1;
            population = 0;
            extremal_mask = 0;
            glider_count = 0;
            max_xpy = 0; min_xpy = 0; max_xmy = 0; min_xmy = 0;
        }

        upattern(int width, int height) {
            // Construct a rectangular toroidal universe:
            tilesProcessed = 0;
            gensElapsed = 0;
            torus_width = width / W;
            torus_height = height / H;
            lastmant = 1;
            population = 0;
            extremal_mask = 0;
            glider_count = 0;
            max_xpy = 0; min_xpy = 0; max_xmy = 0; min_xmy = 0;

            if ((width == 0) || (height == 0)) { return; }

            // Create the full connectivity digraph for the torus:
            for (int i = 0; i < torus_width; i++) {
                for (int j = 0; j < torus_height; j++) {
                    T* sqt = coords2ptr(i, j);
                    sqt->neighbours[0] = coords2ptr(i+1, j);
                    sqt->neighbours[1] = coords2ptr(i+1, j+1);
                    sqt->neighbours[2] = coords2ptr(i, j+1);
                    sqt->neighbours[3] = coords2ptr(i-1, j);
                    sqt->neighbours[4] = coords2ptr(i-1, j-1);
                    sqt->neighbours[5] = coords2ptr(i, j-1);
                }
            } 
        }

        int is_extremal(uint64_t x) {
            #ifdef DISABLE_GLIDERS
            (void) x;
            return 0;
            #else
            int ext = 0;
            int64_t tx = (x & 0xffffffffu) - 0x80000000u;
            int64_t tw = (x >> 32) - 0x80000000u;
            int64_t xpy = tx * W - (tw * (H + W/2));
            int64_t xmy = tx * W + (tw * (H - W/2));
            ext |= ((xpy >= max_xpy) ? 1 : 0);
            ext |= ((xpy <= min_xpy) ? 2 : 0);
            ext |= ((xmy >= max_xmy) ? 4 : 0);
            ext |= ((xmy <= min_xmy) ? 8 : 0);
            return (ext & extremal_mask);
            #endif
        }

        T* getNeighbour(T* sqt, int i) {
            T* y = sqt->neighbours[i];
            if (!y) {
                uint64_t x = sqt->coords + udirections[i];
                T** pointer_to_pointer = &(tiles.hashtable[x]);
                if (*pointer_to_pointer == 0) {
                    *pointer_to_pointer = &(tiles.elements[tiles.elements.newnode()]);
                    (*pointer_to_pointer)->coords = x;
                    #ifndef DISABLE_GLIDERS
                    int64_t tx = (x & 0xffffffffu) - 0x80000000u;
                    int64_t tw = (x >> 32) - 0x80000000u;
                    int64_t xpy = tx * W - (tw * (H + W/2));
                    int64_t xmy = tx * W + (tw * (H - W/2));
                    max_xpy = (xpy > max_xpy) ? xpy : max_xpy;
                    min_xpy = (xpy < min_xpy) ? xpy : min_xpy;
                    max_xmy = (xmy > max_xmy) ? xmy : max_xmy;
                    min_xmy = (xmy < min_xmy) ? xmy : min_xmy;
                    #endif
                }
                y = *pointer_to_pointer;
                y->neighbours[(i + 3) % 6] = sqt;
                sqt->neighbours[i] = y;
            }
            return y;
        }

        void decache() {
            uint32_t totalnodes = tiles.size();
            for (uint32_t it = 0; it != totalnodes; ++it) {
                T* sqt = &(tiles.elements[it]);
                if (sqt->updateflags == 0) {
                    modified.push_back(sqt);
                    sqt->updateflags |= 64;
                }
            }
            lastmant = 1;
        }

        void updateNeighbour(T* sqt, int i) {
            T* n = getNeighbour(sqt, i);
            if (n->updateflags == 0) {
                modified.push_back(n);
            }
            n->updateflags |= (1 << ((i + 3) % 6));
        }

        #include "classic/jumptable.h"

        void updateBoundary(T* sqt) {
            uint8_t offset = sqt->updateflags & 63;
            sqt->updateflags = 0;
            if (offset) { jumptable[offset](sqt); }
        }

        void runkgens(int rule, int family, uint64_t mantissa) {

            if (mantissa == 0) { return; }
            if (mantissa != lastmant) {
                if (mantissa % lastmant) { decache(); }
            }

            for (auto it = modified.begin(); it != modified.end(); ++it) {
                updateBoundary(*it);
            }

            modified.swap(temp_modified);

            for (auto it = temp_modified.begin(); it != temp_modified.end(); ++it) {
                (*it)->updateTile(this, rule, family, mantissa);
            }

            tilesProcessed += temp_modified.size();
            temp_modified.clear();

            gensElapsed += mantissa;
            lastmant = mantissa;
        }

        uint64_t valid_mantissa(int rule) {
            // W is known at compile-time so this is branchless:
            if (W == 16) {
                return uli_valid_mantissa(rule);
            } else if (W == 28) {
                return 5; // We run 2 generations at a time
            } else if (W == 24) {
                return 17; // We run 4 generations at a time
            }
        }

        void advance(int rule, int history, uint64_t generations) {
            uint64_t grem = generations;
            int family = uli_get_family(rule) + history;
            uint64_t vmb = valid_mantissa(rule);
            while (grem > 0) {
                uint64_t m = (grem >= 8) ? 8 : grem;
                while (((vmb >> m) & 1) == 0) { m -= 1; }
                if (m == 0) { break; }
                runkgens(rule, family, m);
                grem -= m;
            }
            if (grem > 0) {
                std::cerr << "Serious problem: was only able to run pattern for ";
                std::cerr << (generations - grem) << " generations instead of the ";
                std::cerr << (generations) << " generations that were requested." << std::endl;
            }
        }

        int totalPopulation() {

            for (auto it = popchanged.begin(); it != popchanged.end(); ++it) {
                (*it)->countPopulation(this);
            }

            popchanged.clear();

            return population;
        }

        bool nonempty() { return (totalPopulation() != 0); }

        void emplace_uint64(int z, int64_t x, int64_t y, uint64_t v) {
            int64_t ay = y;
            uint8_t dy = ((ay % H) + H) % H;
            ay -= dy;

            int64_t ax = ((int64_t) x) - ((ay / H) * (W / 2));
            uint8_t dx = ((ax % W) + W) % W;
            ax -= dx;

            T* sqt = coords2ptr((ax / W), -(ay / H));

            sqt->eu64(this, z, dx, dy, v);
        }

        void insertPattern(std::vector<bitworld> &planes) {
            for (uint64_t i = 0; i < planes.size(); i++) {
                // if (i == N) { break; }
                for (auto it = planes[i].world.begin(); it != planes[i].world.end(); ++it) {
                    if (it->second != 0) {
                        emplace_uint64(i, 8 * it->first.first, 8 * it->first.second, it->second);
                    }
                }
            }
        }

        void clearHistory() {
            uint32_t totalnodes = tiles.size();
            for (uint32_t it = 0; it != totalnodes; ++it) {
                T* sqt = &(tiles.elements[it]);
                sqt->clearHistory();
            }
        }

        void extractPattern(std::vector<bitworld> &planes) {
            uint32_t totalnodes = tiles.size();
            for (uint32_t it = 0; it != totalnodes; ++it) {
                T* sqt = &(tiles.elements[it]);
                int64_t tx = (sqt->coords & 0xffffffffu) - 0x80000000u;
                int64_t tw = (sqt->coords >> 32) - 0x80000000u;
                int64_t x = tx * W - tw * (W/2);
                int64_t y = -tw * H;
                for (uint64_t i = 0; i < planes.size(); i++) {
                    if (sqt->nonempty(i)) {
                        planes[i] += shift_bitworld(sqt->to_bitworld(i), x, y);
                    }
                }
            }
        }

        void insertPattern(std::string s) {
            std::vector<bitworld> planes = rle2vec(s);
            insertPattern(planes);
        }

        uint64_t totalHash(int radius) {

            uint64_t globalhash = 0;

            uint32_t totalnodes = tiles.size();
            for (uint32_t it = 0; it != totalnodes; ++it) {
                T* sqt = &(tiles.elements[it]);
                int64_t tx = (sqt->coords & 0xffffffffu) - 0x80000000u;
                int64_t tw = (sqt->coords >> 32) - 0x80000000u;
                if (tx * tx + tw * tw - tx * tw < radius * radius) {
                    globalhash += sqt->hashTile() * (sqt->coords ^ 3141592653589793ull);
                }
            }

            return globalhash;
        }
    };

    #include "classic/vtile.h"
    #include "classic/vtile28.h"
    #ifdef __AVX512F__
    #include "classic/vtile44.h"
    #endif
    #include "classic/utile.h"

    template<int H>
    void copycells(upattern<VTile<H>, 28, H>* curralgo, incubator<56, H*2>* destalgo) {

        uint32_t totalnodes = curralgo->tiles.size();
        for (uint32_t it = 0; it != totalnodes; ++it) {
            VTile<H> *sqt = &(curralgo->tiles.elements[it]);

            if (sqt->countPopulation(curralgo) == 0) { continue; }

            int64_t tx = (sqt->coords & 0xffffffffu) - 0x80000000u;
            int64_t tw = (sqt->coords >> 32) - 0x80000000u;

            bool split = (((2 * tx - tw) & 3) == 3);

            for (int half = 0; half < 1 + split; half++) {

                int mx = 2 * tx - tw + half;
                int my = -tw;

                int lx = mx % 4;
                int ly = my % 2;

                if (lx < 0) {lx += 4;}
                if (ly < 0) {ly += 2;}

                int tx = (mx - lx) / 4;
                int ty = (my - ly) / 2;

                Incube<56, H*2>* sqt2 = &(destalgo->tiles[std::pair<int, int>(tx, ty)]);
                sqt2->tx = tx; sqt2->ty = ty;

                for (int i = 0; i < H; i++) {
                    uint64_t insert;
                    if (half == 1) {
                        insert = (sqt->d[i+2] >> 16) & 0x3fff;
                    } else {
                        insert = (sqt->d[i+2] >> 2) & (split ? 0x3fff : 0xfffffff);
                    }
                    sqt2->d[i + H * ly] |= (insert << (14 * lx));
                    if (half == 1) {
                        insert = (sqt->hist[i+2] >> 16) & 0x3fff;
                    } else {
                        insert = (sqt->hist[i+2] >> 2) & (split ? 0x3fff : 0xfffffff);
                    }
                    sqt2->hist[i + H * ly] |= (insert << (14 * lx));
                }
            }
        }
    }

} // namespace apg

