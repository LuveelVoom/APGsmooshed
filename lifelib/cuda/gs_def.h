#pragma once

#include <stdint.h>
#include <vector>
#include <string>

namespace apg {

    class GpuSearcher {

        void* xhc;
        void* xmc1;
        void* xmc2;
        int device;
        int num_universes;
        std::string symstring;

        public:
        GpuSearcher(int dev, int unicount, std::string symmetry);
        ~GpuSearcher();
        void pump(std::string seed, uint64_t epoch, std::vector<uint64_t> &vec);

    };

    int getDeviceCount();
}

