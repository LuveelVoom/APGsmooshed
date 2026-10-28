#include <cpads/random/prng.hpp>
#include <cpads/sorting/bitonic.hpp>
#include <algorithm>
#include "gtest/gtest.h"

__global__ void bitonic_kernel(uint64_t *a) {

    int n = 3 * (threadIdx.x + blockDim.x * blockIdx.x);

    __shared__ uint64_t smem[3 << 8];

    hh::vec<uint64_t, 3> x;
    x[0] = a[n];
    x[1] = a[n + 1];
    x[2] = a[n + 2];

    hh::block_bitonic_sort<3, 8>(x, smem);

    a[n] = x[0];
    a[n + 1] = x[1];
    a[n + 2] = x[2];

}

TEST(Bitonic, Sort24) {

    int n = 196608;

    uint64_t *d_a;
    uint64_t *h_a;
    uint64_t *h_b;

    cudaMalloc((void**) &d_a, 8 * n);
    cudaMallocHost((void**) &h_a, 8 * n);
    cudaMallocHost((void**) &h_b, 8 * n);

    hh::PRNG pcg(1, 2, 3);

    for (int i = 0; i < n; i++) {
        h_a[i] = pcg.generate64();
    }

    cudaMemcpy(d_a, h_a, 8 * n, cudaMemcpyHostToDevice);
    bitonic_kernel<<<(n / 768), 256>>>(d_a);
    cudaMemcpy(h_b, d_a, 8 * n, cudaMemcpyDeviceToHost);

    for (int i = 0; i < n; i += 768) {
        std::sort(&(h_a[i]), &(h_a[i+768]));
    }

    for (int i = 0; i < n; i++) {
        EXPECT_EQ(h_b[i], h_a[i]);
    }

    cudaFree(d_a);
    cudaFreeHost(h_a);
    cudaFreeHost(h_b);

}
