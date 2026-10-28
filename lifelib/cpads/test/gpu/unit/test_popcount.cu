#include <cpads/random/prng.hpp>
#include "gtest/gtest.h"

__global__ void popcount_kernel(uint32_t *a, int32_t *b) {

    allocTLSM(mem, uint32_t, 64, 3);

    mem[0] = blockIdx.x * blockDim.x + threadIdx.x;
    mem[1] = a[mem[0]];
    mem[2] = hh::popc32(mem[1]);
    b[mem[0]] = mem[2];

}

TEST(GPUIntrinsics, Popcount) {

    int n = 1000000;

    uint32_t *d_a;
    uint32_t *h_a;
    int32_t *d_b;
    int32_t *h_b;

    cudaMalloc((void**) &d_a, 4 * n);
    cudaMalloc((void**) &d_b, 4 * n);
    cudaMallocHost((void**) &h_a, 4 * n);
    cudaMallocHost((void**) &h_b, 4 * n);

    hh::PRNG pcg(1, 2, 3);

    for (int i = 0; i < n; i++) {
        h_a[i] = pcg.generate();
    }

    cudaMemcpy(d_a, h_a, 4 * n, cudaMemcpyHostToDevice);
    popcount_kernel<<<(n >> 6), 64>>>(d_a, d_b);
    cudaMemcpy(h_b, d_b, 4 * n, cudaMemcpyDeviceToHost);

    for (int i = 0; i < n; i++) {
        EXPECT_EQ(h_b[i], hh::popc32(h_a[i]));
    }

    cudaFree(d_a);
    cudaFree(d_b);
    cudaFreeHost(h_a);
    cudaFreeHost(h_b);

}
