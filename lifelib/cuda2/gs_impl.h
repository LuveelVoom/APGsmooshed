#pragma once

#include "gs_def.h"
#include "cusha256.h"
#include "cudagol.h"

namespace apg {

    int reportCudaError(cudaError_t err) {

        if (err != cudaSuccess) {
            std::cerr << "\033[31;1mCUDA Error " << ((int) err) << " : " <<
                cudaGetErrorString(err) << "\033[0m" << std::endl;

            return ((int) err);
        }

        return 0;
    }

    int getDeviceCount() {

        int count = 0;
        if (reportCudaError(cudaGetDeviceCount(&count))) { return -1; }
        return count;

    }

    void GpuSearcher::pump(std::string seed, uint64_t epoch, std::vector<uint64_t> &vec) {

        int minibatch = this->num_universes;
        int maxgen = 21000;

        hash_container *hc = (hash_container*) this->xhc;
        uint4 *multiverse = (uint4*) this->xmc;

        reportCudaError(cudaSetDevice(this->device));

        hc->create_hashes(seed, epoch);

        for (int sb = 0; sb < 1000000; sb += minibatch) {

            uint32_t* hi = hc->interesting + sb;
            uint32_t* hh = hc->d_B + sb * 8;

            exhaustFirstTile<<<(minibatch >> 2), 128>>>(hh, hi, multiverse);
            exhaustMultipleTiles<20, 20,  64><<<minibatch, 128>>>(2,  hi, hc->topology, multiverse, maxgen);
            exhaustMultipleTiles<38, 38, 128><<<minibatch, 256>>>(20, hi, hc->topology, multiverse, maxgen);
            exhaustMultipleTilesUltimate<92, 128, 128><<<minibatch, 512>>>(38, hi, hc->topology, multiverse, maxgen);
        }

        auto oldsize = vec.size();
        hc->extract_gems(epoch, 1000000, vec);
        std::cout << "Interesting universes: " << (vec.size() - oldsize) << " out of 1000000" << std::endl;

        // for (auto&& it : vec) { std::cout << it << ", "; }
        // std::cout << std::endl;

        reportCudaError(cudaGetLastError());
    }

    GpuSearcher::GpuSearcher(int dev, int unused_unicount, std::string symmetry) {

        reportCudaError(cudaSetDevice(dev));

        (void) unused_unicount;
        size_t free_mem = 0;
        size_t total_mem = 0;

        reportCudaError(cudaMemGetInfo(&free_mem, &total_mem));

        std::cerr << "Memory statistics: " << free_mem << " free; " << total_mem << " total." << std::endl;

        int minibatch = 0;

        // minibatch size M must be a divisor of 10**6, and there must be at
        // least 65536*M bytes of free memory (ideally plus some overhead).
        if        (free_mem < 2000000000ull) {
            minibatch = 10000;
        } else if (free_mem < 4000000000ull) {
            minibatch = 20000;
        } else if (free_mem < 7000000000ull) {
            minibatch = 50000;
        } else if (free_mem < 14000000000ull) {
            minibatch = 100000; // 2080 Ti 11GB
        } else if (free_mem < 18000000000ull) {
            minibatch = 200000; // Volta V100 16GB
        } else if (free_mem < 34000000000ull) {
            minibatch = 250000; // 3090 24GB
        } else if (free_mem < 68000000000ull) {
            minibatch = 500000; // Ampere A100 40GB / A40 48GB
        } else {
            minibatch = 1000000; // Ampere A100 80GB / Hopper H100 80GB
        }

        std::cerr << "Minibatch size: \033[32;1m " << minibatch << " \033[0m" << std::endl;

        this->num_universes = minibatch;
        this->symstring = symmetry;
        this->device = dev;
        auto hc = new hash_container();

        cudaMalloc((void**) &(this->xmc), ((uint64_t) minibatch) * 65536);

        this->xhc = (void*) hc;

        hc->spin_up(true);
    }

    GpuSearcher::~GpuSearcher() {
        hash_container *hc = (hash_container*) this->xhc;

        reportCudaError(cudaSetDevice(this->device));

        hc->tear_down();
        cudaFree(this->xmc);

        delete hc;
    }

}
