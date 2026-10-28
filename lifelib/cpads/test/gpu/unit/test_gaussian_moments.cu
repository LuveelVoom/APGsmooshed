#include <cpads/random/gaussian.hpp>
#include <cpads/random/prng.hpp>
#include "gtest/gtest.h"

template<int n_moments>
__global__ void gaussian_moment_kernel(uint64_t epoch, int iterations, double* output) {

    CREATE_SMEM_ICDF(smem_icdf);

    double moments[n_moments] {0.0};

    hh::PRNG prng(epoch, threadIdx.x, blockIdx.x);

    for (int i = 0; i < iterations; i++) {
        uint32_t entropy = prng.generate();
        double normal = hh::warpGaussian(entropy, smem_icdf);

        double moment = 1.0;
        #pragma unroll
        for (int j = 0; j < n_moments; j++) {
            moments[j] += moment;
            moment *= normal;
        }
    }

    for (int j = 0; j < n_moments; j++) {
        output[((j * gridDim.x) + blockIdx.x) * blockDim.x + threadIdx.x] += moments[j];
    }
}

template<int n_moments>
auto computeMoments(double* moments, uint64_t n_epochs, int n_threads, int iterations) {

    double* moments_device;
    double* moments_host;

    size_t nbytes = sizeof(double) * n_threads * n_moments;

    cudaMalloc(&moments_device,   nbytes);
    cudaMallocHost(&moments_host, nbytes);
    memset(moments_host, 0, nbytes);
    cudaMemcpy(moments_device, moments_host, nbytes, cudaMemcpyHostToDevice);

    int n_blocks = n_threads >> 10;

    for (uint64_t epoch = 0; epoch < n_epochs; epoch++) {

        gaussian_moment_kernel<n_moments><<<n_blocks, 1024>>>(epoch, iterations, moments_device);

    }

    cudaMemcpy(moments_host, moments_device, nbytes, cudaMemcpyDeviceToHost);

    int k = 0;
    for (int j = 0; j < n_moments; j++) {
        moments[j] = 0.0;
        for (int i = 0; i < n_threads; i++) {
            moments[j] += moments_host[k++];
        }
    }

    cudaFreeHost(moments_host);
    cudaFree(moments_device);

    return moments;
}

TEST(Gaussian, Moments) {

    constexpr size_t n_moments = 13;
    double moments[n_moments]{0.0};
    computeMoments<n_moments>(moments, 4, 1048576, 1024);

    std::cout << ((uint64_t) moments[0]) << " trials completed." << std::endl;

    std::vector<double> idealMoments;
    idealMoments.push_back(1.0);
    idealMoments.push_back(0.0);
    while (idealMoments.size() < 2 * n_moments) {
        idealMoments.push_back((idealMoments.size() - 1) * idealMoments[idealMoments.size() - 2]);
    }

    for (int i = 1; i < ((int) n_moments); i++) {
        double m = moments[i] / moments[0];
        double d = m - idealMoments[i];
        double e = std::sqrt((idealMoments[2*i] - idealMoments[i] * idealMoments[i]) / moments[0]);
        double tstat = d/e;
        std::cout << "moment " << i << ":\n    discrepancy = " << d << ";\n    value = " << m << ";\n    tstat = " << tstat << std::endl;

        EXPECT_LE(tstat,  4.0);
        EXPECT_GE(tstat, -4.0);
    }

}
