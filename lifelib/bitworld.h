#pragma once

#include "_version.py"
#include "bitbounds.h"
#include "sanirule.h"
#include "classic/bpattern.h"

#include <stdint.h>
#include <map>
#include <utility>
#include <iostream>
#include <vector>
#include <sstream>
#include <string>

namespace apg {

    struct RleWriter {

        std::ostream &outstream;
        char terminator;
        int maxlength;
        int N;
        int linelength;

        uint64_t runlength;
        uint64_t lastx;
        uint64_t lasty;
        uint64_t lastc;

        RleWriter(std::ostream &outstream, char terminator, int maxlength, int N) : outstream(outstream),
            terminator(terminator), maxlength(maxlength), N(N), linelength(0),
            runlength(0), lastx(0), lasty(0), lastc(0) { }

        void addstring(std::string xs) {
            if (linelength + ((int) xs.length()) > maxlength) {
                outstream << terminator;
                linelength = 0;
            }
            outstream << xs;
            linelength += xs.length();
        }

        void printrun(uint64_t n, std::string c) {
            if (n >= 2) {
                std::ostringstream x;
                x << n << c;
                addstring(x.str());
            } else if (n == 1) {
                addstring(c);
            }
        }

        std::string state2string(uint64_t c) {
            if (c == 0) {
                return ((N == 1) ? "b" : ".");
            } else if (c == 1) {
                return ((N == 1) ? "o" : "A");
            } else {
                uint64_t d = c - 1;
                std::string a(1, (char) ('A' + (d % 24)));
                d = d / 24;
                while (d > 0) {
                    a = std::string(1, (char) ('p' + ((d + 10) % 11))) + a;
                    d = d / 11;
                }
                return a;
            }
        }

        void flushrun() {
            printrun(runlength, state2string(lastc));
            runlength = 0;
        }

        void addcell(uint64_t x, uint64_t y, uint64_t c) {

            if ((y > lasty) || (x > lastx) || (c != lastc)) {
                flushrun();
                if (y > lasty) {
                    printrun(y - lasty, "$");
                    lasty = y;
                    lastx = 0;
                }
                if (x > lastx) {
                    printrun(x - lastx, state2string(0));
                }
            }

            runlength += 1; lastx = x + 1; lastc = c;
        }

        void finalise() {
            flushrun();
            outstream << "!";
        }
    };

    // Convert (x, y) pairs into Morton order:
    uint64_t morton32(uint64_t x, uint64_t y) {
        uint64_t z = x | (y << 32);
        z = (z & 0xffff00000000ffffull) | ((z & 0x00000000ffff0000ull) << 16) | ((z & 0x0000ffff00000000ull) >> 16);
        z = (z & 0xff0000ffff0000ffull) | ((z & 0x0000ff000000ff00ull) << 8) | ((z & 0x00ff000000ff0000ull) >> 8);
        z = (z & 0xf00ff00ff00ff00full) | ((z & 0x00f000f000f000f0ull) << 4) | ((z & 0x0f000f000f000f00ull) >> 4);
        z = (z & 0xc3c3c3c3c3c3c3c3ull) | ((z & 0x0c0c0c0c0c0c0c0cull) << 2) | ((z & 0x3030303030303030ull) >> 2);
        z = (z & 0x9999999999999999ull) | ((z & 0x2222222222222222ull) << 1) | ((z & 0x4444444444444444ull) >> 1);
        return z;
    }

    uint64_t inflcorner(uint64_t x) {
        uint64_t z = x & (0x000000000f0f0f0full);
        z = (z & 0xffff00000000ffffull) | ((z & 0x00000000ffff0000ull) << 16) | ((z & 0x0000ffff00000000ull) >> 16);
        z = (z & 0xff0000ffff0000ffull) | ((z & 0x0000ff000000ff00ull) << 8) | ((z & 0x00ff000000ff0000ull) >> 8);
        z = (z & 0xc3c3c3c3c3c3c3c3ull) | ((z & 0x0c0c0c0c0c0c0c0cull) << 2) | ((z & 0x3030303030303030ull) >> 2);
        z = (z & 0x9999999999999999ull) | ((z & 0x2222222222222222ull) << 1) | ((z & 0x4444444444444444ull) >> 1);
        z |= (z << 1);
        z |= (z << 8);
        return z;
    }

    std::pair<uint64_t, uint64_t> morton64(uint64_t x, uint64_t y) {
        uint64_t zhigh = morton32(x >> 32, y >> 32);
        uint64_t zlow = morton32(x & 0xffffffffull, y & 0xffffffffull);
        return std::make_pair(zhigh, zlow);
    }

    // Compare apgcode representations to determine which one is simpler:
    std::string comprep(std::string a, std::string b)
    {
        if (a.compare("#") == 0) {
            return b;
        } else if (b.compare("#") == 0) {
            return a;
        } else if (a.length() < b.length()) {
            return a;
        } else if (b.length() < a.length()) {
            return b;
        } else if (a.compare(b) < 0) {
            return a;
        } else {
            return b;
        }
    }

    /*
     * A container for storing two-dimensional binary data as a collection of
     * 8-by-8 tiles. Limited to universes of size (2 ** 35)-by-(2 ** 35).
     */
    struct bitworld {

        public:
        std::map<std::pair<int32_t, int32_t>, uint64_t> world;

        // Return a Morton-ordered associative array containing the bitworld,
        // suitable for converting into a compressed quadtree representation.
        void mortonmap(std::map<uint64_t, uint64_t> *mmap) const {
            std::map<std::pair<int32_t, int32_t>, uint64_t>::const_iterator it;
            for (it = world.cbegin(); it != world.cend(); ++it ) {
                uint32_t x = 0x80000000u + ((uint32_t) it->first.first);
                uint32_t y = 0x80000000u + ((uint32_t) it->first.second);
                if (it->second) { mmap->emplace(morton32(x, y), it->second); }
            }
        }

        bool getcell(uint64_t x, uint64_t y) const {
            int32_t u = (uint32_t) (x >> 3);
            int32_t v = (uint32_t) (y >> 3);
            auto it = world.find({u, v});
            if (it == world.end()) {
                return false;
            } else {
                uint64_t w = (it->second >> ((x & 7) | ((y & 7) << 3)));
                return (w & 1);
            }
        }

        void setcell(int64_t x, int64_t y, bool newval) {
            int32_t u = (uint32_t) (x >> 3);
            int32_t v = (uint32_t) (y >> 3);
            uint64_t w = (1ull << ((x & 7) | ((y & 7) << 3)));
            if (newval) {
                world[std::make_pair(u, v)] |= w;
            } else {
                world[std::make_pair(u, v)] &= (~w);
            }
        }

        std::string canonise_orientation(int length, int breadth, int ox, int oy, int a, int b, int c, int d)
        {
            std::string representation;

            char charnames[] = "0123456789abcdefghijklmnopqrstuvwxyz";

            for (int v = 0; v < ((breadth-1)/5)+1; v++) {
                int zeroes = 0;
                if (v != 0) { representation += 'z'; }
                for (int u = 0; u < length; u++) {
                    int baudot = 0;
                    for (int w = 0; w < 5; w++) {
                        int64_t x = ox + a*u + b*(5*v + w);
                        int64_t y = oy + c*u + d*(5*v + w);
                        baudot = (baudot >> 1) + 16 * getcell(x, y);
                    }
                    if (baudot == 0) {
                        zeroes += 1;
                    } else {
                        while (zeroes >= 40) {
                            representation += "yz";
                            zeroes -= 39;
                        }
                        if (zeroes > 0) {
                            if (zeroes == 1) {
                                representation += '0';
                            } else if (zeroes == 2) {
                                representation += 'w';
                            } else if (zeroes == 3) {
                                representation += 'x';
                            } else {
                                representation += 'y';
                                representation += charnames[zeroes - 4];
                            }
                        }
                        zeroes = 0;
                        representation += charnames[baudot];
                    }
                }
            }
            while ((representation.size() > 0) && (representation[representation.size() - 1] == 'z')) {
                representation = representation.substr(0, representation.size() - 1);
            }
            if (representation.size() == 0) { representation = "0"; }
            return representation;
        }

        bitworld& operator+=(const bitworld& rhs) {

            std::map<std::pair<int32_t, int32_t>, uint64_t>::const_iterator it;
            for (it = rhs.world.begin(); it != rhs.world.end(); ++it) {
                world[it->first] |= (it->second);
            }
            return *this;
        }

        bitworld& operator^=(const bitworld& rhs) {

            std::map<std::pair<int32_t, int32_t>, uint64_t>::const_iterator it;
            for (it = rhs.world.begin(); it != rhs.world.end(); ++it) {
                world[it->first] ^= (it->second);
            }
            return *this;
        }

        void intersect(const bitworld& rhs, uint64_t xx) {
            std::map<std::pair<int32_t, int32_t>, uint64_t>::iterator it;
            for (it = world.begin(); it != world.end(); /* no increment */ ) {
                std::map<std::pair<int32_t, int32_t>, uint64_t>::const_iterator it2 = rhs.world.find(it->first);
                uint64_t tile = xx;
                if (it2 != rhs.world.end()) {
                    tile ^= it2->second;
                }
                tile &= it->second;

                if (tile == 0) {
                    it = world.erase(it);
                } else {
                    it->second = tile;
                    ++it;
                }
            }
        }

        bitworld& operator-=(const bitworld& rhs) {

            if (rhs.world.size() < world.size()) {
                for (auto it = rhs.world.begin(); it != rhs.world.end(); ++it) {
                    world[it->first] &= (~(it->second));
                }
            } else {
                intersect(rhs, 0xffffffffffffffffull);
            }
            return *this;
        }

        bitworld& operator&=(const bitworld& rhs) {

            intersect(rhs, 0);
            return *this;

        }

        bpattern to_bpattern() const {

            bpattern b;
            for (int i = 0; i < 64; i++) { b.x[i] = 0; }

            for (auto it = world.begin(); it != world.end(); ++it ) {
                int64_t x = it->first.first;
                int64_t y = it->first.second;

                for (int i = 0; i < 8; i++) {
                    uint64_t subrow = ((it->second) >> (8*i)) & 255;
                    b.x[i + 8*(y & 7)] |= (subrow << (8*(x & 7)));
                }
            }

            return b;
        }

        std::vector<std::pair<int64_t, int64_t> > getcells() const {
            std::vector<std::pair<int64_t, int64_t> > celllist;
            for (auto it = world.begin(); it != world.end(); ++it ) {
                int64_t x = it->first.first;
                int64_t y = it->first.second;
                uint64_t tile = it->second;
                for (int i = 0; i < 64; i++) {
                    if ((tile >> i) & 1) {
                        celllist.push_back(std::pair<int64_t, int64_t>((x << 3) + (i & 7), (y << 3) + (i >> 3)));
                    }
                }
            }
            return celllist;
        }

        bitworld& inplace_rot2() {
            auto celllist = getcells();
            for (auto it = celllist.begin(); it != celllist.end(); ++it) {
                int x = it->first;
                int y = it->second;
                setcell(-x, -y, 1);
            }
            return *this;
        }

        bitworld& inplace_rot3() {
            auto celllist = getcells();
            for (auto it = celllist.begin(); it != celllist.end(); ++it) {
                int x = it->first;
                int y = it->second;
                setcell(-y, x-y, 1);
                setcell(y-x, -x, 1);
            }
            return *this;
        }

        void printrepr() {
            std::map<std::pair<int32_t, int32_t>, uint64_t>::iterator it;
            for (it = world.begin(); it != world.end(); ++it ) {
                int32_t x = it->first.first;
                int32_t y = it->first.second;
                std::cout << '(' << x << ',' << y << ')' << std::endl;
                uint64_t tile = it->second;
                for (int i = 0; i < 64; i++) {
                    std::cout << ((tile & 1) ? '*' : '.');
                    tile = tile >> 1;
                    if (i % 8 == 7) { std::cout << std::endl; }
                }
            }
        }

        uint64_t population() const {
            uint64_t pop = 0;
            for (const auto& kv : world) {
                pop += __builtin_popcountll(kv.second);
            }
            return pop;
        }

        int64_t get_tldiag() {

            int64_t tldiag = 20000000000000ll;

            std::map<std::pair<int32_t, int32_t>, uint64_t>::iterator it;
            for (it = world.begin(); it != world.end(); /* no increment */ ) {
                uint64_t tile = it->second;
                if (tile == 0) {
                    it = world.erase(it);
                } else {
                    int64_t diagonal = ((int64_t) it->first.first ) * 8;
                    diagonal += ((int64_t) it->first.second ) * 8;
                    if (diagonal < tldiag) {
                        int64_t dz = uint64_tl(tile);
                        if (diagonal + dz < tldiag) { tldiag = diagonal + dz; }
                    }
                    ++it;
                }
            }

            return tldiag;
        }

        bool getdbox(int64_t *dbox) const {

            // These are set outside the admissible range:
            int64_t tl =    20000000000000ll;
            int64_t br =   -20000000000000ll;
            int64_t bl =    20000000000000ll;
            int64_t tr =   -20000000000000ll;

            bool nonempty = false;

            for (const auto& kv : world) {
                const auto& coords = kv.first;
                uint64_t tile = kv.second;
                if (tile != 0) {
                    int64_t abscissa = ((int64_t) coords.first ) * 8;
                    int64_t ordinate = ((int64_t) coords.second) * 8;
                    int64_t major = abscissa + ordinate;
                    int64_t minor = abscissa - ordinate;

                    if (major < tl) {
                        int64_t dx = uint64_tl(tile);
                        if (major + dx < tl) { tl = major + dx; }
                    }

                    if (major + 14 > br) {
                        int64_t dx = uint64_br(tile);
                        if (major + dx > br) { br = major + dx; }
                    }

                    if (minor < bl) {
                        int64_t dy = uint64_bl(tile);
                        if (minor + dy < bl) { bl = minor + dy; }
                    }

                    if (minor + 14 > tr) {
                        int64_t dy = uint64_tr(tile);
                        if (minor + dy > tr) { tr = minor + dy; }
                    }

                    nonempty = true;
                }
            }

            if (nonempty) {
                dbox[0] = tl;
                dbox[1] = bl - 7;
                dbox[2] = (1 + br - tl);
                dbox[3] = (1 + tr - bl);
            }

            return nonempty;
        }

        bool getbbox(int64_t *bbox) const {

            // These are set outside the admissible range:
            int64_t left =    20000000000000ll;
            int64_t right =  -20000000000000ll;
            int64_t top =     20000000000000ll;
            int64_t bottom = -20000000000000ll;

            bool nonempty = false;

            for (const auto& kv : world) {
                const auto& coords = kv.first;
                uint64_t tile = kv.second;
                if (tile != 0) {
                    int64_t abscissa = ((int64_t) coords.first ) * 8;
                    int64_t ordinate = ((int64_t) coords.second) * 8;

                    if (abscissa < left) {
                        int64_t dx = uint64_left(tile);
                        if (abscissa + dx < left) { left = abscissa + dx; }
                    }

                    if (abscissa + 7 > right) {
                        int64_t dx = uint64_right(tile);
                        if (abscissa + dx > right) { right = abscissa + dx; }
                    }

                    if (ordinate < top) {
                        int64_t dy = uint64_top(tile);
                        if (ordinate + dy < top) { top = ordinate + dy; }
                    }

                    if (ordinate + 7 > bottom) {
                        int64_t dy = uint64_bottom(tile);
                        if (ordinate + dy > bottom) { bottom = ordinate + dy; }
                    }

                    nonempty = true;
                }
            }

            if (nonempty) {
                bbox[0] = left;
                bbox[1] = top;
                bbox[2] = (1 + right - left);
                bbox[3] = (1 + bottom - top);
            }

            return nonempty;
        }

        bool getboct(int64_t *boct) const {
            getbbox(boct);
            return getdbox(boct + 4);
        }

        void clean() {
            for (auto it = world.begin(); it != world.end(); /* no increment */ ) {
                uint64_t tile = it->second;
                if (tile == 0) {
                    // Empty tile; erase:
                    it = world.erase(it);
                } else {
                    ++it;
                }
            }
        }

        bitworld inflate() {
            bitworld nb;
            for (auto it = world.begin(); it != world.end(); ++it) {
                uint64_t tile = it->second;
                std::pair<int32_t, int32_t> coords(it->first.first * 2, it->first.second * 2);
                if (tile != 0) {
                    nb.world[std::pair<int32_t, int32_t>(coords.first, coords.second)] = inflcorner(tile);
                    nb.world[std::pair<int32_t, int32_t>(coords.first+1, coords.second)] = inflcorner(tile >> 4);
                    nb.world[std::pair<int32_t, int32_t>(coords.first, coords.second+1)] = inflcorner(tile >> 32);
                    nb.world[std::pair<int32_t, int32_t>(coords.first+1, coords.second+1)] = inflcorner(tile >> 36);
                }
            }
            return nb;
        }

        bitworld get1cell() {

            bitworld x;
            for (auto it = world.begin(); it != world.end(); /* no increment */ ) {
                uint64_t tile = it->second;
                if (tile == 0) {
                    it = world.erase(it);
                } else {
                    x.world[it->first] = tile & (~(tile - 1));
                    break;
                }
            }
            return x;

        }

        bitworld br1cell() {

            std::pair<int32_t, int32_t> record(-1000000000, -1000000000);
            uint64_t recordcell = 0;
            for (auto it = world.begin(); it != world.end(); /* no increment */ ) {
                uint64_t tile = it->second;
                if (tile == 0) {
                    it = world.erase(it);
                } else {
                    if (it->first.first + it->first.second > record.first + record.second) {
                        record = it->first;
                        recordcell = tile & (~(tile - 1));
                    }
                    ++it;
                }
            }
            bitworld x;
            x.world[record] = recordcell;
            return x;
        }

    };

    bitworld bpattern_to_bitworld(const bpattern &b) {
        bitworld res;
        for (uint64_t i = 0; i < 64; i++) {
            for (uint64_t j = 0; j < 64; j++) {
                if ((b.x[i] >> j) & 1) {
                    res.setcell(j, i, 1);
                }
            }
        }
        return res;
    }

    std::vector<bitworld> cells2vec(uint64_t n, int64_t* coords, uint64_t* states) {

        std::vector<bitworld> planes;
        for (uint64_t i = 0; i < n; i++) {
            uint64_t bp = 0;
            uint64_t colour = (states == 0) ? 1 : states[i];
            while (colour > 0) {
                if (colour & 1) {
                    if (bp >= planes.size()) { planes.resize(bp+1); }
                    planes[bp].setcell(coords[2*i], coords[2*i+1], 1);
                }
                bp += 1; colour = colour >> 1;
            }
        }
        return planes;

    }

    std::vector<bitworld> apg2vec(std::string apgcode) {
        /*
        * Taken almost verbatim from the Catagolue source code.
        */

        std::vector<bitworld> planes(1);

        int x = 0; int y = 0; bool iny = false;
        for (unsigned int i = 0; i < apgcode.size(); i++) {
            char c = apgcode[i]; int a = -1;
            if (c == '_') {
                x = 0; y = 0;
                planes.emplace_back();
            } else if ((c >= 97) && (c < 123)) {
                a = c - 87;
            } else if ((c >= 48) && (c < 58)) {
                a = c - 48;
            } else if ((c >= 65) && (c < 91)) {
                a = c - 55;
            }

            if (a >= 0) {
                if (iny) {
                    iny = false; x += a;
                } else if (a < 32) {
                    for (int j = 0; j < 5; j++) {
                        if (a & (1 << j)) {
                            planes.back().setcell(x, y+j, 1);
                        }
                    }
                    x += 1;
                } else if (a == 35) {
                    x = 0; y += 5;
                } else {
                    x += (a - 30);
                    iny = (a == 34);
                }
            }
        }

        return planes;
    }

    std::string bp2rle(const apg::bpattern &bp) {
        std::ostringstream outstream;
        RleWriter rw(outstream, '\n', 66, 1);
        for (uint64_t y = 0; y < 64; y++) {
            for (uint64_t x = 0; x < 64; x++) {
                if ((bp.x[y] >> x) & 1) {
                    rw.addcell(x, y, 1);
                }
            }
        }
        rw.finalise();
        return outstream.str();
    }

    std::string vec2rle(const std::vector<bitworld>& planes, const RuleMapper& rm) {

        std::map<std::pair<int64_t, int64_t>, uint64_t> cells;

        int N = planes.size();

        int64_t minx = 0;
        int64_t miny = 0;

        for (int i = 0; i < N; i++) {
            auto celllist = planes[i].getcells();
            for (auto&& cell : celllist) {
                cells[std::pair<int64_t, int64_t>(cell.second, cell.first)] += (1ull << i);
                minx = cell.first;
                miny = cell.second;
            }
        }

        if (cells.empty()) { return "b!"; }

        for (auto it = cells.begin(); it != cells.end(); ++it) {
            minx = std::min(minx, it->first.second);
            miny = std::min(miny, it->first.first);
        }

        std::ostringstream outstream;
        RleWriter rw(outstream, '\n', 66, N);

        for (auto it = cells.begin(); it != cells.end(); ++it) {
            uint64_t y = it->first.first - miny;
            uint64_t x = it->first.second - minx;
            uint64_t c = rm.ll_to_golly(it->second);

            rw.addcell(x, y, c);
        }

        rw.finalise();

        return outstream.str();
    }

    std::vector<bitworld> rle2vec(std::string rle, const RuleMapper& rm) {

        if (rle.find('!') == std::string::npos) {
            auto p = rle.find('_');
            if (p != std::string::npos) {
                return apg2vec(rle.substr(p+1));
            }
        }

        std::vector<bitworld> planes;
        uint64_t x = 0; uint64_t y = 0; uint64_t count = 0;
        uint64_t colour = 0;
        for (unsigned int i = 0; i < rle.size(); i++) {
            char c = rle[i];
            if ((c >= '0') && (c <= '9')) {
                count *= 10;
                count += (c - '0');
            } else if ((c == 'b') || (c == '.')) {
                if (count == 0) { count = 1; }
                x += count; count = 0;
            } else if (c == '$') {
                if (count == 0) { count = 1; }
                y += count; x = 0; count = 0;
            } else if ((c == 'o') || ((c >= 'A') && (c <= 'X'))) {
                if (count == 0) { count = 1; }
                if (c == 'o') {
                    colour = colour * 24 + 1;
                } else {
                    colour = colour * 24 + (c - 'A') + 1;
                }
                uint64_t bp = 0;
                colour = rm.golly_to_ll(colour);
                while (colour > 0) {
                    if (colour & 1) {
                        if (bp >= planes.size()) { planes.resize(bp+1); }
                        for (uint64_t j = 0; j < count; j++) {
                            planes[bp].setcell(x+j, y, 1);
                        }
                    }
                    bp += 1; colour = colour >> 1;
                }
                x += count; count = 0; colour = 0;
            } else if ((c >= 'p') && (c <= 'z')) {
                uint64_t m = c - 'o';
                colour = colour * 11 + (m % 11);
            } else if (c == '!') { break; }
        }
        return planes;
    }

    std::vector<bitworld> rle2vec(const std::string& rle) {
        return rle2vec(rle, IdentityMapper());
    }

    bitworld _shift_left(const bitworld &a, uint32_t k) {
        bitworld b;
        uint64_t bitmask = (0x0101010101010101ull << k) - 0x0101010101010101ull;
        for (const auto& kv : a.world) {
            b.world[kv.first] |= (kv.second & (~bitmask)) >> k;
            int32_t newx = kv.first.first - 1;
            int32_t newy = kv.first.second;
            b.world[std::make_pair(newx, newy)] |= (kv.second & bitmask) << (8-k);
        }
        b.clean();
        return b;
    }

    bitworld _shift_right(const bitworld &a, uint32_t k) {
        bitworld b;
        uint64_t bitmask = (0x0101010101010101ull << (8-k)) - 0x0101010101010101ull;
        for (const auto& kv : a.world) {
            b.world[kv.first] |= (kv.second & bitmask) << k;
            int32_t newx = kv.first.first + 1;
            int32_t newy = kv.first.second;
            b.world[std::make_pair(newx, newy)] |= (kv.second & (~bitmask)) >> (8-k);
        }
        b.clean();
        return b;
    }

    bitworld _shift_down(const bitworld &a, uint32_t k) {
        bitworld b;
        for (const auto& kv : a.world) {
            b.world[kv.first] |= (kv.second << (8*k));
            int32_t newx = kv.first.first;
            int32_t newy = kv.first.second + 1;
            b.world[std::make_pair(newx, newy)] |= (kv.second >> (8*(8-k)));
        }
        b.clean();
        return b;
    }

    bitworld _shift_up(const bitworld &a, uint32_t k) {
        bitworld b;
        for (const auto& kv : a.world) {
            b.world[kv.first] |= (kv.second >> (8*k));
            int32_t newx = kv.first.first;
            int32_t newy = kv.first.second - 1;
            b.world[std::make_pair(newx, newy)] |= (kv.second << (8*(8-k)));
        }
        b.clean();
        return b;
    }

    bitworld bleed(const bitworld &a, std::string s) {
        bitworld b = a;
        for (uint64_t i = 0; i < s.length(); i++) {
            char c = s[i];
            if (c == '9') {
                b += _shift_left(b, 1);
                b += _shift_right(b, 1);
                b += _shift_up(b, 1);
                b += _shift_down(b, 1);
            } else if (c == '5') {
                bitworld d = _shift_left(b, 1);
                d += _shift_right(b, 1);
                d += _shift_up(b, 1);
                d += _shift_down(b, 1);
                b += d;
            }
        }
        return b;
    }

    bitworld shift_bitworld(const bitworld &a, int64_t sx, int64_t sy) {
        uint32_t sl = ((uint32_t) (-sx)) & 7;
        uint32_t su = ((uint32_t) (-sy)) & 7;

        int32_t dx = (((int32_t) sl) + sx) >> 3;
        int32_t dy = (((int32_t) su) + sy) >> 3;

        bitworld b;
        std::map<std::pair<int32_t, int32_t>, uint64_t>::const_iterator it;
        for (it = a.world.begin(); it != a.world.end(); ++it) {
            int32_t newx = it->first.first + dx;
            int32_t newy = it->first.second + dy;
            if (it->second) { b.world[std::make_pair(newx, newy)] = it->second; }
        }
        if (sl) {b = _shift_left(b, sl); }
        if (su) {b = _shift_up(b, su); }
        return b;
    }

    bitworld fix_topleft(const bitworld &a) {
        int64_t bbox[4] = {0};
        bool nonempty = a.getbbox(bbox);
        if (nonempty == false) { return a; }
        return shift_bitworld(a, -bbox[0], -bbox[1]);
    }

    bitworld grow_cluster(const bitworld &seed, const bitworld &backdrop, std::string growth) {

        bitworld agglom = seed;
        bitworld cluster = seed;

        while (cluster.population() != 0) {
            cluster = bleed(cluster, growth);
            cluster &= backdrop;
            cluster -= agglom;
            agglom += cluster;
        }
        return agglom;
    }

    std::string canonise_orientation(std::vector<bitworld> &bwv, int length, int breadth, int ox, int oy, int a, int b, int c, int d) {
        std::string s = "";
        for (uint64_t i = 0; i < bwv.size(); i++) {
            if (i != 0) { s += "_"; }
            s += bwv[i].canonise_orientation(length, breadth, ox, oy, a, b, c, d);
        }
        return s;
    }

    std::string wechslerhex(std::vector<bitworld> &bwv) {

        std::string rep = "#";

        int64_t rect[8] = {0ll};

        bitworld bw = bwv[0];
        for (uint64_t i = 1; i < bwv.size(); i++) { bw += bwv[i]; }
        bw.getboct(rect);

        int64_t rect4 = rect[0] + rect[2] - 1;
        int64_t rect5 = rect[1] + rect[3] - 1;
        int64_t rect9 = rect[5] + rect[7] - 1;

        rep = comprep(rep, canonise_orientation(bwv, rect[2], rect[3], rect[0], rect[1],  1,  0,  0,  1));
        rep = comprep(rep, canonise_orientation(bwv, rect[3], rect[2], rect[0], rect[1],  0,  1,  1,  0));

        rep = comprep(rep, canonise_orientation(bwv, rect[3], rect[7], rect9 + rect[1], rect[1],  1, -1,  1,  0));
        rep = comprep(rep, canonise_orientation(bwv, rect[7], rect[3], rect9 + rect[1], rect[1], -1,  1,  0,  1));

        rep = comprep(rep, canonise_orientation(bwv, rect[7], rect[2], rect4, rect4 - rect9,  0, -1,  1, -1));
        rep = comprep(rep, canonise_orientation(bwv, rect[2], rect[7], rect4, rect4 - rect9, -1,  0, -1,  1));

        rep = comprep(rep, canonise_orientation(bwv, rect[2], rect[3], rect4, rect5, -1,  0,  0, -1));
        rep = comprep(rep, canonise_orientation(bwv, rect[3], rect[2], rect4, rect5,  0, -1, -1,  0));

        rep = comprep(rep, canonise_orientation(bwv, rect[3], rect[7], rect[5] + rect5, rect5, -1,  1, -1,  0));
        rep = comprep(rep, canonise_orientation(bwv, rect[7], rect[3], rect[5] + rect5, rect5,  1, -1,  0, -1));

        rep = comprep(rep, canonise_orientation(bwv, rect[7], rect[2], rect[0], rect[0] - rect[5],  0,  1, -1,  1));
        rep = comprep(rep, canonise_orientation(bwv, rect[2], rect[7], rect[0], rect[0] - rect[5],  1,  0,  1, -1));

        return rep;
    }

    std::string wechslernone(std::vector<bitworld> &bwv, int64_t *rect) {

        return canonise_orientation(bwv, rect[2], rect[3], rect[0], rect[1],  1,  0,  0,  1);

    }

    std::string wechslerise(std::vector<bitworld> &bwv, int64_t *rect) {

        std::string rep = "#";
        int64_t rect4 = rect[0] + rect[2] - 1;
        int64_t rect5 = rect[1] + rect[3] - 1;
        // if (((rect[2] + 2) * (rect[3] + 2)) <= 2000) {
            rep = comprep(rep, canonise_orientation(bwv, rect[2], rect[3], rect[0], rect[1],  1,  0,  0,  1));
            rep = comprep(rep, canonise_orientation(bwv, rect[2], rect[3], rect4, rect[1], -1,  0,  0,  1));
            rep = comprep(rep, canonise_orientation(bwv, rect[2], rect[3], rect[0], rect5,  1,  0,  0, -1));
            rep = comprep(rep, canonise_orientation(bwv, rect[2], rect[3], rect4, rect5, -1,  0,  0, -1));
            rep = comprep(rep, canonise_orientation(bwv, rect[3], rect[2], rect[0], rect[1],  0,  1,  1,  0));
            rep = comprep(rep, canonise_orientation(bwv, rect[3], rect[2], rect4, rect[1],  0, -1,  1,  0));
            rep = comprep(rep, canonise_orientation(bwv, rect[3], rect[2], rect[0], rect5,  0,  1, -1,  0));
            rep = comprep(rep, canonise_orientation(bwv, rect[3], rect[2], rect4, rect5,  0, -1, -1,  0));
        // }
        return rep;
    }

}
