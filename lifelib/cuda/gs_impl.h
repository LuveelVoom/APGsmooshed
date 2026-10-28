#pragma once

#include "gs_def.h"
#include "cusha256.h"
#include "gpupattern.h"

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

        hash_container *hc = (hash_container*) this->xhc;
        multiverse_container *mc1 = (multiverse_container*) this->xmc1;
        multiverse_container *mc2 = (multiverse_container*) this->xmc2;

        std::string symmetry = this->symstring;

        cudaSetDevice(this->device);

        uint64_t epoch_size = 500000;

        hc->create_hashes(seed, epoch);

        uint32_t offset1 = this->num_universes;
        uint32_t offset2 = this->num_universes;
        copyhashes(symmetry, offset1, mc1->multiverse, hc->d_seq, ((uint64_cu*) hc->d_B), 0, true);
        copyhashes(symmetry, offset2, mc2->multiverse, hc->d_seq, ((uint64_cu*) hc->d_B) + (4 * epoch_size), 0, true);

        uint64_t x = 0;
        uint64_t y = 0;
        uint64_t loops = 0;
        uint64_t tiles1 = 0;
        uint64_t tiles2 = 0;
        bool complete = false;

        while (((x + y) != 0) || (!complete)) {
            bool complete1 = (y == 0) && (offset1 == epoch_size);
            bool complete2 = (x == 0) && (offset2 == epoch_size);
            complete = (offset1 == epoch_size) && (offset2 == epoch_size);
            y = mc1->mainloop(hc->topology, hc->interesting, ((uint64_cu*) hc->d_B), offset1, epoch_size, x, *mc2, complete1);
            x = mc2->mainloop(hc->topology, hc->interesting + epoch_size, ((uint64_cu*) hc->d_B) + (4 * epoch_size), offset2, epoch_size, y, *mc1, complete2);
            loops += 1;
            tiles1 += y;
            tiles2 += x;
        }

        std::cout << "Loops: " << loops << std::endl;
        std::cout << "Tiles in epoch: " << (tiles1 + tiles2) << std::endl;
        auto oldsize = vec.size();
        hc->extract_gems(epoch, epoch_size * 2, vec);
        std::cout << "Interesting universes: " << (vec.size() - oldsize) << " out of 1000000" << std::endl;

        cudaError_t err = cudaGetLastError();
        if (err != cudaSuccess) { std::cerr << "Error: " << cudaGetErrorString(err) << std::endl; }
    }

    GpuSearcher::GpuSearcher(int dev, int num_universes, std::string symmetry) {
        this->num_universes = num_universes;
        this->symstring = symmetry;
        this->device = dev;
        auto hc = new hash_container();
        auto mc1 = new multiverse_container();
        auto mc2 = new multiverse_container();

        this->xhc = (void*) hc;
        this->xmc1 = (void*) mc1;
        this->xmc2 = (void*) mc2;

        cudaSetDevice(dev);

        hc->spin_up(true, num_universes);
        mc1->spin_up(num_universes, symmetry);
        mc2->spin_up(num_universes, symmetry);
    }

    GpuSearcher::~GpuSearcher() {
        hash_container *hc = (hash_container*) this->xhc;
        multiverse_container *mc1 = (multiverse_container*) this->xmc1;
        multiverse_container *mc2 = (multiverse_container*) this->xmc2;

        cudaSetDevice(this->device);

        hc->tear_down();
        mc1->tear_down();
        mc2->tear_down();

        delete hc; delete mc1; delete mc2;
    }

}
