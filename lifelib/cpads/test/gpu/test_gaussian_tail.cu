
#include <cmath>
#include <stdio.h>
#include <vector>
#include <cpads/random/gaussian.hpp>
#include <cpads/random/prng.hpp>

__global__ void gaussian_tail_kernel(uint64_t epoch, int iterations, double* output_gauss, uint64_t* output_count) {

    CREATE_SMEM_ICDF(smem_icdf);

    hh::PRNG prng(epoch, threadIdx.x, blockIdx.x);

    for (int i = 0; i < iterations; i++) {
        uint32_t entropy = prng.generate();
        double normal = hh::warpGaussian(entropy, smem_icdf);

        if ((normal >= 4.0) || (normal <= -4.0)) {
            uint64_t index = hh::atomic_add(output_count, 1);
            output_gauss[index] = normal;
        }
    }
}


class TailRandomSource {

    double* output_device_gauss;
    double* output_host_gauss;
    uint64_t* output_device_count;
    uint64_t* output_host_count;

    int iterations;
    uint64_t epoch;

    public:

    void run_kernel() {

        cudaMemset(output_device_count, 0, 8);
        gaussian_tail_kernel<<<1024, 1024>>>(epoch, iterations, output_device_gauss, output_device_count);
        epoch += 1;

    }

    void pcie() {

        cudaMemcpy(output_host_count, output_device_count, 8, cudaMemcpyDeviceToHost);
        cudaMemcpy(output_host_gauss, output_device_gauss, 8 * ((size_t) output_host_count[0]), cudaMemcpyDeviceToHost);

    }

    TailRandomSource(int iterations) : iterations(iterations), epoch(0) {
        {
            size_t nbytes = ((size_t) iterations) << 10;
            cudaMalloc(&output_device_gauss,   nbytes);
            cudaMallocHost(&output_host_gauss, nbytes);
            cudaMalloc(&output_device_count,   8);
            cudaMallocHost(&output_host_count, 8);
        }
        run_kernel();
        pcie();
    }

    ~TailRandomSource() {
        cudaFreeHost(output_host_gauss);
        cudaFree(output_device_gauss);
        cudaFreeHost(output_host_count);
        cudaFree(output_device_count);
    }

    std::vector<uint32_t> retrieve() {
        pcie();
        run_kernel();

        std::vector<uint32_t> res(output_host_count[0]);

        for (size_t i = 0; i < res.size(); i++) {
            double gaussian = output_host_gauss[i];
            double uniform = std::erf(gaussian * 0.7071067811865476);

            if (uniform > 0) {
                uniform -= 0.9999366575163338;
            } else {
                uniform += 0.9999366575163338;
            }

            // map to interval (0, 2)
            uniform = uniform * 15787.192767323968 + 1.0;

            res[i] = (uint32_t) (uniform * 2147483648.0);
        }

        return res;
    }

};

int main() {

    TailRandomSource trs(65536);
    size_t epochs = 274877906944ull;

    for (size_t i = 0; i < epochs; i++) {
        auto x = trs.retrieve();
        fwrite((void*) &(x[0]), 4 * x.size(), 1, stdout);
    }

    return 0;

}
