#include <cmath>
#include <stdio.h>
#include <vector>
#include <cpads/random/gaussian.hpp>
#include <cpads/random/prng.hpp>

__global__ void gaussian_kernel(uint64_t epoch, int iterations, double* output_gauss, uint32_t* output_uniform) {

    CREATE_SMEM_ICDF(smem_icdf);

    hh::PRNG prng(epoch, threadIdx.x, blockIdx.x);

    for (int i = 0; i < iterations; i++) {
        int index = ((i * gridDim.x) + blockIdx.x) * blockDim.x + threadIdx.x;
        uint32_t entropy = prng.generate();
        output_uniform[index] = entropy;
        double normal = hh::warpGaussian(entropy, smem_icdf);
        output_gauss[index] = normal;
    }
}


class UniformRandomSource {

    double* output_device_gauss;
    double* output_host_gauss;
    uint32_t* output_device_uniform;
    uint32_t* output_host_uniform;

    int iterations;
    uint64_t epoch;

    public:

    void run_kernel() {

        gaussian_kernel<<<1024, 1024>>>(epoch, iterations, output_device_gauss, output_device_uniform);
        epoch += 1;

    }

    void pcie() {
        {
            size_t nbytes = ((size_t) iterations) << 23;
            cudaMemcpy(output_host_gauss, output_device_gauss, nbytes, cudaMemcpyDeviceToHost);
        }
        {
            size_t nbytes = ((size_t) iterations) << 22;
            cudaMemcpy(output_host_uniform, output_device_uniform, nbytes, cudaMemcpyDeviceToHost);
        }
    }

    UniformRandomSource(int iterations) : iterations(iterations), epoch(0) {
        {
            size_t nbytes = ((size_t) iterations) << 23;
            cudaMalloc(&output_device_gauss,   nbytes);
            cudaMallocHost(&output_host_gauss, nbytes);
        }
        {
            size_t nbytes = ((size_t) iterations) << 22;
            cudaMalloc(&output_device_uniform,   nbytes);
            cudaMallocHost(&output_host_uniform, nbytes);
        }
        run_kernel();
        pcie();
    }

    ~UniformRandomSource() {
        cudaFreeHost(output_host_uniform);
        cudaFreeHost(output_host_gauss);
        cudaFree(output_device_uniform);
        cudaFree(output_device_gauss);
    }

    std::vector<uint32_t> retrieve(bool gauss) {
        pcie();
        run_kernel();
        std::vector<uint32_t> res(((size_t) iterations) << 20);

        for (size_t i = 0; i < res.size(); i++) {
            if (gauss) {
                double gaussian = output_host_gauss[i];
                double uniform = (1.0 + std::erf(gaussian * 0.7071067811865476)) * 2147483648.0;
                res[i] = (uint32_t) uniform;
            } else {
                res[i] = output_host_uniform[i];
            }
        }

        return res;
    }

};

int main() {

    UniformRandomSource urs(64);

    for (size_t i = 0; i < 274877906944ull; i++) {
        auto x = urs.retrieve(true);
        fwrite((void*) &(x[0]), 4 * x.size(), 1, stdout);
    }

    return 0;

}
