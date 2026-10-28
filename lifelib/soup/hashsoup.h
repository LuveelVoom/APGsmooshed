#pragma once
#include "../bitworld.h"
#include "sha256.h"

namespace apg {

    // Reverse each byte in an integer:
    uint64_t uint64_hreflect(uint64_t x) {
        uint64_t y = ((x & 0xaaaaaaaaaaaaaaaaull) >> 1) | ((x & 0x5555555555555555ull) << 1);
                 y = ((y & 0xccccccccccccccccull) >> 2) | ((y & 0x3333333333333333ull) << 2);
                 y = ((y & 0xf0f0f0f0f0f0f0f0ull) >> 4) | ((y & 0x0f0f0f0f0f0f0f0full) << 4);
        return y;
    }

    // Reverse each byte in an integer:
    uint64_t uint64_vreflect(uint64_t x) {
        uint64_t y = ((x & 0xff00ff00ff00ff00ull) >> 8)  | ((x & 0x00ff00ff00ff00ffull) << 8);
                 y = ((y & 0xffff0000ffff0000ull) >> 16) | ((y & 0x0000ffff0000ffffull) << 16);
                 y = ((y & 0xffffffff00000000ull) >> 32) | ((y & 0x00000000ffffffffull) << 32);
        return y;
    }


    // Transpose an 8-by-8 matrix (from Hacker's Delight):
    void transpose8rS32(uint8_t* A, int m, int n, uint8_t* B) {
        unsigned x, y, t;

        // Load the array and pack it into x and y.

        x = (A[0]<<24)   | (A[m]<<16)   | (A[2*m]<<8) | A[3*m];
        y = (A[4*m]<<24) | (A[5*m]<<16) | (A[6*m]<<8) | A[7*m];

        t = (x ^ (x >> 7)) & 0x00AA00AA;  x = x ^ t ^ (t << 7);
        t = (y ^ (y >> 7)) & 0x00AA00AA;  y = y ^ t ^ (t << 7);

        t = (x ^ (x >>14)) & 0x0000CCCC;  x = x ^ t ^ (t <<14);
        t = (y ^ (y >>14)) & 0x0000CCCC;  y = y ^ t ^ (t <<14);

        t = (x & 0xF0F0F0F0) | ((y >> 4) & 0x0F0F0F0F);
        y = ((x << 4) & 0xF0F0F0F0) | (y & 0x0F0F0F0F);
        x = t;

        B[0]=x>>24;    B[n]=x>>16;    B[2*n]=x>>8;  B[3*n]=x;
        B[4*n]=y>>24;  B[5*n]=y>>16;  B[6*n]=y>>8;  B[7*n]=y;
    }

    // Transpose a 16-by-16 matrix:
    void transpose16(unsigned char* A, unsigned char* B) {
        transpose8rS32(A, 2, 2, B);
        transpose8rS32(A+16, 2, 2, B+1);
        transpose8rS32(A+1, 2, 2, B+16);
        transpose8rS32(A+17, 2, 2, B+17);
    }

    bitworld hashsoup_mateon(std::string prehash, std::string symmetry, uint8_t *digest, bool verbose) {

        bitworld bw;

        if ((symmetry == "Mateon1_32x32_Test") || (symmetry == "Mateon1_64x64_Test") || (symmetry == "Mateon1_128x128_Test") ||
            (symmetry == "Mateon1_256x256_Test") || (symmetry == "Mateon1_512x512_Test") || (symmetry == "Mateon1_1k_Test") ||
            (symmetry == "Mateon1_2k_Test") || (symmetry == "Mateon1_4k_Test") || (symmetry == "Mateon1_8k_Test")) {

            std::string extendedhash(prehash);
            extendedhash += ":000";
            int counter = 0;

            int tile_size = 0;
            if (symmetry == "Mateon1_32x32_Test") tile_size = 4;
            if (symmetry == "Mateon1_64x64_Test") tile_size = 8;
            if (symmetry == "Mateon1_128x128_Test") tile_size = 16;
            if (symmetry == "Mateon1_256x256_Test") tile_size = 32;
            if (symmetry == "Mateon1_512x512_Test") tile_size = 64;
            if (symmetry == "Mateon1_1k_Test") tile_size = 128;
            if (symmetry == "Mateon1_2k_Test") tile_size = 256;
            if (symmetry == "Mateon1_4k_Test") tile_size = 512;
            if (symmetry == "Mateon1_8k_Test") tile_size = 1024;

            for (int j = 0; j < tile_size; j++) {
                for (int i = 0; i < tile_size; i++) {
                    if (i % 4 == 0 && !(j == 0 && i == 0)) { // re-seed, but if this is the first time use already computed digest
                        counter++;
                        if (counter < 1000) {
                          extendedhash[extendedhash.size() - 3] = (char)((counter % 1000 / 100) + '0');
                          extendedhash[extendedhash.size() - 2] = (char)((counter % 100 / 10) + '0');
                          extendedhash[extendedhash.size() - 1] = (char)((counter % 10 / 1) + '0');
                        } else {
                          extendedhash = prehash + ":" + std::to_string(counter);
                        }
                        SHA256 ctx = SHA256();
                        ctx.init();
                        if (verbose) std::cout << "Digesting: " << extendedhash << std::endl;
                        ctx.update( (unsigned char*)extendedhash.c_str(), extendedhash.length());
                        ctx.final(digest);
                    }
                    for (int b = 0; b < 8; b++)
                        bw.world[std::pair<int32_t, int32_t>(i, j)] |= (((uint64_t) digest[b + 8*(i%4)] << (8 * b)));
                }
            }
        }

        if (symmetry == "Mateon1_Glider8_4_5_Test" || symmetry == "Mateon1_Glider6_5_6_Test") {
            unsigned gli_target = 0;
            unsigned width = 0;
            unsigned shift = 0;

            if (symmetry == "Mateon1_Glider8_4_5_Test") { gli_target = 8; width = 4; shift = 5; }
            if (symmetry == "Mateon1_Glider6_5_6_Test") { gli_target = 6; width = 5; shift = 6; }

            if (gli_target == 0) return bw; // Ensure we know if something went wrong

            unsigned digest_counter = 0;
            unsigned digest_idx = 0;
            unsigned digest_bits = 0;

            auto sha_rand = [&](unsigned requested_bits) -> uint64_t {
                uint64_t val = 0;
                uint64_t ishift = 0;

                while (1) {
                    if (requested_bits == 0) return val;

                    if (digest_idx >= 32) {
                        SHA256 ctx = SHA256();
                        ctx.init();
                        std::string str = prehash + ":" + std::to_string(++digest_counter);
                        if (verbose) std::cout << "Digesting: " << str << std::endl;
                        ctx.update( (unsigned char*)str.c_str(), str.length());
                        ctx.final(digest);
                        digest_idx = 0;
                    }

                    if (requested_bits <= 8 - digest_bits) {
                        val |= ((uint64_t)digest[digest_idx] & ((1 << requested_bits) - 1)) << ishift;
                        digest[digest_idx] >>= requested_bits;
                        digest_bits += requested_bits;
                        if (digest_bits >= 8) {
                            digest_bits = 0;
                            digest_idx++;
                        }
                        // std::cout << "rand: " << val << std::endl;
                        // std::cout << "state: ctr/i+b: " << digest_counter << "/" << digest_idx << "+" << digest_bits << std::endl;
                        return val;
                    } else {
                        val |= (uint64_t)digest[digest_idx++] << ishift;
                        ishift += 8 - digest_bits;
                        requested_bits -= 8 - digest_bits;
                        digest_bits = 0;
                    }
                }
            };

            constexpr bool GLI_PHASE[2][3][3] = {
                {{1, 1, 1}, {1, 0, 0}, {0, 1, 0}},
                {{0, 1, 1}, {1, 1, 0}, {0, 0, 1}}};

            int minx = 999, maxx = -999;
            int miny = 999, maxy = -999;

            if (verbose) {
                std::cout << "=== === ===\n";
                std::cout << "Using Seed " << prehash << std::endl;
            }

            for (int quadrantx = -1, quadranty = -1; quadranty <= 1; (quadrantx += 2), (quadrantx > 1 ? ((quadrantx = -1),(quadranty += 2)) : 0)) {
                unsigned counter = 0;
repeat:
                while (counter < gli_target) {
                    bool ori = sha_rand(1) == 1;
                    bool flip = sha_rand(1) == 1;
                    bool phase = sha_rand(1) == 1;

                    uint64_t off = sha_rand(width);
                    uint64_t shf = sha_rand(shift) + 4;

                    int x = shf * quadrantx;
                    int y = shf * quadranty;

                    if (ori) {
                        x += quadrantx * (off + 1);
                    } else {
                        y += quadranty * off;
                    }

                    for (int dy = -2; dy < 5; dy++)
                        for (int dx = -2; dx < 5; dx++)
                            if (bw.getcell(x + dx, y + dy)) goto repeat;

                    // std::cout << "placing glider (" << x << ", " << y << "); ori/flip/phase: " << ori << "/" << flip << "/" << phase << std::endl;

                    for (int dy = 0; dy < 3; dy++) {
                        for (int dx = 0; dx < 3; dx++) {
                            int gx = quadrantx == -1 ? 2 - dx : dx;
                            int gy = quadranty == -1 ? 2 - dy : dy;
                            if (flip) std::swap(gx, gy);
                            bw.setcell(x + dx, y + dy, GLI_PHASE[phase][gy][gx]);
                            if (x+dx < minx) minx = x+dx;
                            if (x+dx > maxx) maxx = x+dx;
                            if (y+dy < miny) miny = y+dy;
                            if (y+dy > maxy) maxy = y+dy;
                        }
                    }
                    counter++;
                }
            }
            if (verbose) {
                for (int y = miny; y <= maxy; y++) {
                    for (int x = minx; x <= maxx; x++) {
                        std::cout << (bw.getcell(x, y) ? 'o' : '.');
                    }
                    std::cout << std::endl;
                }
            }
        }

        return bw;
    }

    // Produce a SHA-256 hash of a string, and use it to generate a soup:
    bitworld hashsoup_inner(std::string prehash, std::string symmetry) {
        bool verbose = false;
        if (prehash.front() == '@') {
            prehash.erase(0, 1);
            verbose = true;
        }

        uint8_t digest[32];
        memset(digest, 0, 32);

        SHA256 ctx = SHA256();
        ctx.init();
        ctx.update( (unsigned char*)prehash.c_str(), prehash.length());
        ctx.final(digest);

        if (symmetry.substr(0, 8) == "Mateon1_") {
            return hashsoup_mateon(prehash, symmetry, digest, verbose);
        }

        uint8_t tsegid[32];
        memset(tsegid, 0, 32);

        if ((symmetry == "C2_2") && (prehash.substr(0, 2) != "l_")) {
            // Correction for pre-apgluxe behaviour:
            std::memcpy(tsegid, digest, 32);
            transpose16(tsegid, digest);
        } else {
            transpose16(digest, tsegid);
        }

        if ((symmetry == "D2_xo") || (symmetry == "D2_x") || (symmetry == "D8_4") || (symmetry == "D8_1") || (symmetry == "D4_x4") || (symmetry == "D4_x1")) {
            // We make our arrays diagonally symmetric:
            uint8_t diggid[32];
            memset(diggid, 0, 32);
            for (int i = 0; i < 8; i++) {
                diggid[2*i] = (digest[2*i] & ((1 << (8 - i)) - 1)) | (tsegid[2*i] & (256 - (1 << (8 - i))));
                diggid[2*i + 17] = (digest[2*i + 17] & ((1 << (8 - i)) - 1)) | (tsegid[2*i + 17] & (256 - (1 << (8 - i))));
                diggid[2*i + 1] = digest[2*i + 1];
                diggid[2*i + 16] = tsegid[2*i + 16];
            }

            for (int i = 0; i < 32; i++) {
                tsegid[i] ^= (diggid[i] ^ digest[i]);
            }
            std::memcpy(digest, diggid, 32);
        }

        bitworld bw;

        if ((symmetry == "8x32") || (symmetry == "4x64") || (symmetry == "2x128") || (symmetry == "1x256")) {
            int height = symmetry[0] - 48;
            int eighthwidth = 32 / height;
            for (int j = 0; j < height; j++) {
                for (int i = 0; i < eighthwidth; i++) {
                    bw.world[std::pair<int32_t, int32_t>(eighthwidth - 1 - i, 0)] |= (((uint64_t) digest[eighthwidth*j+i]) << (8*j));
                }
            }
            return bw;
        }

        if (symmetry == "10x10")
        {
            // source: https://conwaylife.com/forums/viewtopic.php?p=73815#p73815
            for(int i=0; i<10; i++) digest[2*i+1] &= 192;
            for(int i=20; i<32; i++) digest[i] = 0;
        }

        uint64_t a = 0;
        uint64_t b = 0;
        uint64_t c = 0;
        uint64_t d = 0;

        for (int j = 0; j < 8; j++) {
            a |= (((uint64_t) digest[2*j]) << 8*j);
            b |= (((uint64_t) digest[2*j+1]) << 8*j);
            c |= (((uint64_t) digest[2*j+16]) << 8*j);
            d |= (((uint64_t) digest[2*j+17]) << 8*j);
        }

        if ((symmetry == "C1") || (symmetry == "G1") || (symmetry == "D2_x")) {
            #ifdef LONG_TILES
            bw.world[std::pair<int32_t, int32_t>(1, 2)] = b;
            bw.world[std::pair<int32_t, int32_t>(2, 2)] = a;
            bw.world[std::pair<int32_t, int32_t>(1, 3)] = d;
            bw.world[std::pair<int32_t, int32_t>(2, 3)] = c;
            #else
            bw.world[std::pair<int32_t, int32_t>(1, 1)] = b;
            bw.world[std::pair<int32_t, int32_t>(2, 1)] = a;
            bw.world[std::pair<int32_t, int32_t>(1, 2)] = d;
            bw.world[std::pair<int32_t, int32_t>(2, 2)] = c;
            #endif
            return bw;
        }

        bw.world[std::pair<int32_t, int32_t>(0, 0)] = b;
        bw.world[std::pair<int32_t, int32_t>(1, 0)] = a;
        bw.world[std::pair<int32_t, int32_t>(0, 1)] = d;
        bw.world[std::pair<int32_t, int32_t>(1, 1)] = c;

        bitworld dbw;
        dbw.world[std::pair<int32_t, int32_t>(0, 0)] = uint64_vreflect(uint64_hreflect(c));
        dbw.world[std::pair<int32_t, int32_t>(1, 0)] = uint64_vreflect(uint64_hreflect(d));
        dbw.world[std::pair<int32_t, int32_t>(0, 1)] = uint64_vreflect(uint64_hreflect(a));
        dbw.world[std::pair<int32_t, int32_t>(1, 1)] = uint64_vreflect(uint64_hreflect(b));

        if ((symmetry == "C4_1") || (symmetry == "C4_4") || (symmetry == "D4_x1") || (symmetry == "D4_x4")) {
            a = 0; b = 0; c = 0; d = 0;
            for (int j = 0; j < 8; j++) {
                a |= (((uint64_t) tsegid[2*j]) << 8*j);
                b |= (((uint64_t) tsegid[2*j+1]) << 8*j);
                c |= (((uint64_t) tsegid[2*j+16]) << 8*j);
                d |= (((uint64_t) tsegid[2*j+17]) << 8*j);
            }
        }

        bitworld vbw;
        vbw.world[std::pair<int32_t, int32_t>(0, 0)] = uint64_vreflect(d);
        vbw.world[std::pair<int32_t, int32_t>(1, 0)] = uint64_vreflect(c);
        vbw.world[std::pair<int32_t, int32_t>(0, 1)] = uint64_vreflect(b);
        vbw.world[std::pair<int32_t, int32_t>(1, 1)] = uint64_vreflect(a);

        if (symmetry == "D2_xo") { return shift_bitworld(vbw, 0, -15); }

        if (symmetry == "D2_+1") { bw += shift_bitworld(vbw, 0, -15); return bw; }
        if (symmetry == "D2_+2") { bw += shift_bitworld(vbw, 0, -16); return bw; }

            // To do: only allow rules without B0,2(c,i),4(c,i),6(c,i) for the following as these transitions break gutter symmetry
        if (symmetry == "D2_+1_gO1s0") { bw += shift_bitworld(vbw, 0, -17); return bw; }
            // To do: only allow rules without B0,1c,2(k,n),3(n,y),4(y,z),5r,6i as these transitions break skewgutter symmetry
        if (symmetry == "D2_+1_gO1s1") { bw += shift_bitworld(vbw, 1, -17); return bw; }
            // To do: only allow rules without B0,1,2(a,i,k,n),3(c,q,r),4(c,n,y,z),5(e,r),6i as these transitions break double-skewgutter symmetry
        if (symmetry == "D2_+1_gO1s2") { bw += shift_bitworld(vbw, 2, -17); return bw; }

        bitworld hbw;
        hbw.world[std::pair<int32_t, int32_t>(0, 0)] = uint64_hreflect(a);
        hbw.world[std::pair<int32_t, int32_t>(1, 0)] = uint64_hreflect(b);
        hbw.world[std::pair<int32_t, int32_t>(0, 1)] = uint64_hreflect(c);
        hbw.world[std::pair<int32_t, int32_t>(1, 1)] = uint64_hreflect(d);

        if (symmetry == "C2_4") {
            bw += shift_bitworld(dbw, 16, -16);
        } else if (symmetry == "C2_2") {
            bw += shift_bitworld(dbw, 16, -15);
        } else if (symmetry == "C2_1") {
            bw += shift_bitworld(dbw, 15, -15);
        } else if ((symmetry == "D8_4") || (symmetry == "D4_+4") || (symmetry == "D4_x4") || (symmetry == "C4_4")) {
            bw += shift_bitworld(vbw, 0, -16);
            bw += shift_bitworld(hbw, 16, 0);
            bw += shift_bitworld(dbw, 16, -16);
        } else if (symmetry == "D4_+2") {
            bw += shift_bitworld(vbw, 0, -15);
            bw += shift_bitworld(hbw, 16, 0);
            bw += shift_bitworld(dbw, 16, -15);
        } else if ((symmetry == "D8_1") || (symmetry == "D4_+1") || (symmetry == "D4_x1") || (symmetry == "C4_1")) {
            bw += shift_bitworld(vbw, 0, -15);
            bw += shift_bitworld(hbw, 15, 0);
            bw += shift_bitworld(dbw, 15, -15);
        }

        return bw;

    }

    std::vector<bitworld> hashsoup(std::string prehash, std::string full_symmetry) {

        if (full_symmetry.find("stdin") != std::string::npos) {
            auto pos = prehash.find('-');
            if (pos == std::string::npos) {
                return hashsoup(prehash, "C1");
            } else {
                std::string rle = prehash.substr(pos);
                return rle2vec(rle);
            }
        }

        std::string symmetry = full_symmetry;
        uint64_t inflations = 0;
        while (symmetry[0] == 'i') {
            inflations += 1;
            symmetry = symmetry.substr(1);
        }

        bool rot3 = true;
        bool rot2 = false;

        if (full_symmetry == "C6") {
            symmetry = "C2_1";
        } else if (full_symmetry == "D12") {
            symmetry = "D2_x"; rot2 = true;
        } else if (full_symmetry == "C3_1") {
            symmetry = "C1";
        } else if (full_symmetry == "D6_1") {
            symmetry = "D2_x";
        } else if (full_symmetry == "D6_1o") {
            symmetry = "D2_xo";
        } else {
            rot3 = false;
        }

        if (symmetry[0] == 'G') {
            symmetry = "C" + symmetry.substr(1);
        } else if (symmetry[0] == 'H') {
            symmetry = "D" + symmetry.substr(1);
        }

        bitworld bw = hashsoup_inner(prehash, symmetry);

        if (rot3) {
            bw = shift_bitworld(bw, -15, 0).inplace_rot3();
            if (rot2) { bw.inplace_rot2(); }
        }

        for (uint64_t i = 0; i < inflations; i++) {
            bw = bw.inflate();
        }

        std::vector<bitworld> bvec;
        bvec.push_back(bw);
        return bvec;
    }

}

