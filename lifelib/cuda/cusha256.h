#pragma once
#include "basics.h"
#include "hexgrid.h"
#include "../soup/sha256_k.h"
#include <cstring>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>

#define SAVE_BLOCK(u, i) t = ((u & 0xff00ff00ull) >> 8) | ((u & 0x00ff00ffull) << 8); \
        out[i + (threadIdx.x << 3) + (blockIdx.x << 9)] = (t >> 16) | (t << 16)

#define LOAD_BLOCK(i) t = block[i + (threadIdx.x << 4) + (blockIdx.x << 10)]; \
        t = ((t & 0xff00ff00ull) >> 8) | ((t & 0x00ff00ffull) << 8); \
        W[i][threadIdx.x] = (t >> 16) | (t << 16)

#define UPDATE_W(i) s = W[(i +  9) & 15][threadIdx.x]; \
                    t = W[(i +  1) & 15][threadIdx.x]; \
                    s += ROTR32(t,  7) ^ ROTR32(t, 18) ^ (t >>  3); \
                    t = W[(i + 14) & 15][threadIdx.x]; \
                    s += ROTR32(t, 17) ^ ROTR32(t, 19) ^ (t >> 10); \
                    W[i][threadIdx.x] += s

#define SHA256_ROUND_X(a, b, c, d, e, f, g, h, k) s = h + k + (ROTR32(e, 6) ^ ROTR32(e, 11) ^ ROTR32(e, 25)) + ((e & (f ^ g)) ^ g); \
                                                  t = ((a & (b | c)) | (b & c)) + (ROTR32(a, 2) ^ ROTR32(a, 13) ^ ROTR32(a, 22)); \
                                                  d += s; h = s + t

#define SHA256_ROUND_0(i) SHA256_ROUND_X(S0, S1, S2, S3, S4, S5, S6, S7, W[(i) & 15][threadIdx.x] + k[i])
#define SHA256_ROUND_1(i) SHA256_ROUND_X(S7, S0, S1, S2, S3, S4, S5, S6, W[(i) & 15][threadIdx.x] + k[i])
#define SHA256_ROUND_2(i) SHA256_ROUND_X(S6, S7, S0, S1, S2, S3, S4, S5, W[(i) & 15][threadIdx.x] + k[i])
#define SHA256_ROUND_3(i) SHA256_ROUND_X(S5, S6, S7, S0, S1, S2, S3, S4, W[(i) & 15][threadIdx.x] + k[i])
#define SHA256_ROUND_4(i) SHA256_ROUND_X(S4, S5, S6, S7, S0, S1, S2, S3, W[(i) & 15][threadIdx.x] + k[i])
#define SHA256_ROUND_5(i) SHA256_ROUND_X(S3, S4, S5, S6, S7, S0, S1, S2, W[(i) & 15][threadIdx.x] + k[i])
#define SHA256_ROUND_6(i) SHA256_ROUND_X(S2, S3, S4, S5, S6, S7, S0, S1, W[(i) & 15][threadIdx.x] + k[i])
#define SHA256_ROUND_7(i) SHA256_ROUND_X(S1, S2, S3, S4, S5, S6, S7, S0, W[(i) & 15][threadIdx.x] + k[i])

#define SHA256_16ROUND(i)    SHA256_ROUND_0(i); \
        SHA256_ROUND_1(i+1);  SHA256_ROUND_2(i+2);  SHA256_ROUND_3(i+3);  SHA256_ROUND_4(i+4);  SHA256_ROUND_5(i+5); \
        SHA256_ROUND_6(i+6);  SHA256_ROUND_7(i+7);  SHA256_ROUND_0(i+8);  SHA256_ROUND_1(i+9);  SHA256_ROUND_2(i+10); \
        SHA256_ROUND_3(i+11); SHA256_ROUND_4(i+12); SHA256_ROUND_5(i+13); SHA256_ROUND_6(i+14); SHA256_ROUND_7(i+15)

#define UPDATE_WBLOCK UPDATE_W(0);  UPDATE_W(1);  UPDATE_W(2);  UPDATE_W(3); \
        UPDATE_W(4);  UPDATE_W(5);  UPDATE_W(6);  UPDATE_W(7); \
        UPDATE_W(8);  UPDATE_W(9);  UPDATE_W(10); UPDATE_W(11); \
        UPDATE_W(12); UPDATE_W(13); UPDATE_W(14); UPDATE_W(15)

__global__ void cuda_sha256_1block(uint32_cu *block, uint32_cu *orig_k, uint32_cu *out) {

    uint32_cu S0 = 0x6a09e667u;
    uint32_cu S1 = 0xbb67ae85u;
    uint32_cu S2 = 0x3c6ef372u;
    uint32_cu S3 = 0xa54ff53au;
    uint32_cu S4 = 0x510e527fu;
    uint32_cu S5 = 0x9b05688cu;
    uint32_cu S6 = 0x1f83d9abu;
    uint32_cu S7 = 0x5be0cd19u;

    __shared__ uint32_cu k[64];
    k[threadIdx.x] = orig_k[threadIdx.x];
    __syncthreads();

    __shared__ uint32_cu W[16][64];

    uint32_cu t = 0;
    uint32_cu s = 0;

    LOAD_BLOCK(0);  LOAD_BLOCK(1);  LOAD_BLOCK(2);  LOAD_BLOCK(3);
    LOAD_BLOCK(4);  LOAD_BLOCK(5);  LOAD_BLOCK(6);  LOAD_BLOCK(7);
    LOAD_BLOCK(8);  LOAD_BLOCK(9);  LOAD_BLOCK(10); LOAD_BLOCK(11);
    LOAD_BLOCK(12); LOAD_BLOCK(13); LOAD_BLOCK(14); LOAD_BLOCK(15);

    SHA256_16ROUND(0);
    UPDATE_WBLOCK;
    SHA256_16ROUND(16);
    UPDATE_WBLOCK;
    SHA256_16ROUND(32);
    UPDATE_WBLOCK;
    SHA256_16ROUND(48);

    S0 += 0x6a09e667u;
    S1 += 0xbb67ae85u;
    S2 += 0x3c6ef372u;
    S3 += 0xa54ff53au;
    S4 += 0x510e527fu;
    S5 += 0x9b05688cu;
    S6 += 0x1f83d9abu;
    S7 += 0x5be0cd19u;

    SAVE_BLOCK(S0, 0); SAVE_BLOCK(S1, 1); SAVE_BLOCK(S2, 2); SAVE_BLOCK(S3, 3);
    SAVE_BLOCK(S4, 4); SAVE_BLOCK(S5, 5); SAVE_BLOCK(S6, 6); SAVE_BLOCK(S7, 7);

}

void init_sha256_1block(std::string x, uint8_t *buf) {

    std::memset(buf, 0, 64);
    std::memcpy(buf, x.c_str(), x.length());

    uint16_t len = x.length() * 8; // message length in bits
    buf[63] = len & 255;
    buf[62] = len >> 8;

    buf[x.length()] = 0x80;

}

__global__ void xor_combine(uint32_cu *fixed_part, uint32_cu *variable_part, uint32_cu *output) {

    uint32_cu b = fixed_part[(threadIdx.x & 15) + (blockIdx.y << 4)];

    uint32_cu idx1 = threadIdx.x + blockIdx.x * 640;
    uint32_cu idx2 = idx1 + blockIdx.y * gridDim.x * 640;

    uint32_cu a = variable_part[idx1]; output[idx2] = a ^ b;

    a = variable_part[idx1 +  64]; output[idx2 +  64] = a ^ b;
    a = variable_part[idx1 + 128]; output[idx2 + 128] = a ^ b;
    a = variable_part[idx1 + 192]; output[idx2 + 192] = a ^ b;

    a = variable_part[idx1 + 256]; output[idx2 + 256] = a ^ b;
    a = variable_part[idx1 + 320]; output[idx2 + 320] = a ^ b;
    a = variable_part[idx1 + 384]; output[idx2 + 384] = a ^ b;

    a = variable_part[idx1 + 448]; output[idx2 + 448] = a ^ b;
    a = variable_part[idx1 + 512]; output[idx2 + 512] = a ^ b;
    a = variable_part[idx1 + 576]; output[idx2 + 576] = a ^ b;

}

void init_sha256_plural(std::string root, int offset, int myriads, int &lastlen,
                        uint8_t *h_F, uint32_cu *d_F, uint8_t *h_V, uint32_cu *d_V, uint32_cu *d_M) {

    std::memset(h_F, 0, 64 * myriads);

    int lengths[myriads];
    uint8_t buf[64];

    for (int i = 0; i < myriads; i++) {
        uint64_t j = offset + i;
        std::ostringstream ss;
        ss << root;
        if (j > 0) { ss << j; }
        init_sha256_1block(ss.str(), buf);
        int l = ss.str().length();
        std::memcpy(h_F + 64 * i, buf, l);
        lengths[i] = l;
    }

    cudaMemcpy(d_F, h_F, 64 * myriads, cudaMemcpyHostToDevice);

    int lastxor = 0;

    for (int i = 0; i < myriads; i++) {
        uint64_t j = offset + i;
        int l = lengths[i];
        if (l != lastlen) {
            int batches = i - lastxor;
            if (batches > 0) {
                dim3 numBlocks(250, batches);
                xor_combine<<<numBlocks, 64>>>(d_F + 16 * lastxor, d_V, d_M + 160000 * lastxor);
                lastxor = i;
            }

            std::memset(h_V, 0, 640000);
            for (uint64_t k = 0; k < 10000; k++) {
                std::ostringstream ss;
                ss << root;
                ss << (j * 10000 + k);
                init_sha256_1block(ss.str(), buf);
                std::memcpy(h_V + 64 * k + l, buf + l, 64 - l);
            }
            cudaMemcpy(d_V, h_V, 640000, cudaMemcpyHostToDevice);

        }
        lastlen = l;

    }

    int batches = myriads - lastxor;
    if (batches > 0) {
        dim3 numBlocks(250, batches);
        xor_combine<<<numBlocks, 64>>>(d_F + 16 * lastxor, d_V, d_M + 160000 * lastxor);
    }
}

__global__ void compress32(uint32_cu *interesting, uint32_cu *icompressed) {
    /*
    * When we write to the 'interesting' array, we use an entire uint32
    * (four bytes) to represent a single Boolean value. This is good for
    * atomicity, but poor for space efficiency, so we compress this by
    * a factor of 32 before memcpying it from the device to the host.
    */

    uint32_cu a = interesting[(blockIdx.x << 5) + threadIdx.x];
    uint32_cu b = FULL_BALLOT(a != 0);

    if (threadIdx.x == 0) { icompressed[blockIdx.x] = b; }

}

struct hash_container {

    uint32_cu *d_A;
    uint32_cu *d_B;
    uint8_t   *h_B;
    uint32_cu *d_F;
    uint8_t   *h_F;
    uint32_cu *d_V;
    uint8_t   *h_V;
    uint32_cu *d_K;
    uint32_t  *h_seq;
    uint32_cu *d_seq;
    uint32_cu *interesting;
    uint32_cu *icompressed;
    uint64_t  *h_res;
    uint64_cu *topology;

    int lastlen;
    std::string laststring;
    uint64_t myriads_per_loop;
    uint64_t hashcount;

    void spin_up(bool verbose, int unicount) {

        laststring = "";
        lastlen = -1;
        myriads_per_loop = 100;
        hashcount = 10000 * myriads_per_loop;

        uint64_t seqlength = unicount;

        cudaMalloc((void**)&d_A, hashcount * 64);
        cudaMalloc((void**)&d_F, 64 * myriads_per_loop);
        cudaMallocHost((void**)&h_F, 64 * myriads_per_loop);
        cudaMalloc((void**)&d_V, 640000);
        cudaMallocHost((void**)&h_V, 640000);
        cudaMalloc((void**)&d_B, hashcount * 32);
        cudaMallocHost((void**)&h_B, hashcount * 32);
        cudaMalloc((void**)&d_K, 256);
        cudaMemcpy(d_K, sha256_k, 256, cudaMemcpyHostToDevice);
        cudaMalloc((void**)&d_seq, seqlength * 4);
        cudaMallocHost((void**)&h_seq, seqlength * 4);
        for (int i = 0; i < seqlength; i++) { h_seq[i] = i; }
        cudaMemcpy(d_seq, h_seq, seqlength * 4, cudaMemcpyHostToDevice);
        cudaMalloc((void**)&interesting, hashcount * 4);
        cudaMalloc((void**)&icompressed, hashcount / 8);
        cudaMallocHost((void**)&h_res, hashcount / 8);

        cudaMalloc((void**)&topology, 1024);
        uint64_t h_topology[128];

        if (verbose) { std::cout << "CUDART VERSION = " << CUDART_VERSION << std::endl; }

        prepare_topology(h_topology, verbose);

        // We include topology in both global and constant memory to optimise
        // memory access patterns:
        cudaMemcpy(topology, h_topology, 1024, cudaMemcpyHostToDevice);
        cudaMemcpyToSymbol(c_topology, h_topology, 1024);
    }

    void tear_down() {
        cudaFree((void*)d_A);
        cudaFreeHost((void*)h_B);
        cudaFree((void*)d_B);
        cudaFreeHost((void*)h_F);
        cudaFree((void*)d_F);
        cudaFreeHost((void*)h_V);
        cudaFree((void*)d_V);
        cudaFree((void*)d_K);
        cudaFreeHost((void*)h_seq);
        cudaFree((void*)interesting);
        cudaFree((void*)icompressed);
        cudaFreeHost((void*)h_res);
        cudaFree((void*)topology);
    }

    void create_hashes(std::string seed, uint64_t epoch) {
        if (seed != laststring) { laststring = seed; lastlen = -1; }
        init_sha256_plural(seed, epoch * myriads_per_loop, myriads_per_loop, lastlen, h_F, d_F, h_V, d_V, d_A);
        cuda_sha256_1block<<<(hashcount / 64), 64>>>(d_A, d_K, d_B);
    }

    void extract_gems(uint64_t epoch, uint64_t num_soups, std::vector<uint64_t> &resvec) {
        compress32<<<(num_soups / 32), 32>>>(interesting, icompressed);
        cudaMemcpy(h_res, icompressed, num_soups / 8, cudaMemcpyHostToDevice);

        for (uint64_t i = 0; i < num_soups; i += 64) {
            uint64_t v = h_res[i >> 6];
            while (v != 0) {
                uint64_t w = v & (-v);
                v ^= w;
                resvec.push_back(i + epoch * hashcount + __builtin_ctzll(w));
            }
        }
    }

};
