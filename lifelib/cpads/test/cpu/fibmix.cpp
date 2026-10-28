#include <cpads/core.hpp>
#include <stdio.h>
#include <string>
#include <iostream>

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cerr << "Usage: ./fibmix N_ROUNDS" << std::endl;
    }

    int n = std::stoll(argv[1]);

    for (uint64_t i = 0; i < 281474976710656ull; i++) {

        uint32_t x[65536];

        for (uint64_t j = 0; j < 65536; j++) {
            uint64_t y = (i << 16) | j;
            for (int k = 0; k < n; k++) {
                y = hh::fibmix(y);
            }
            x[j] = (y >> 32);
        }

        fwrite((void*) &(x[0]), 262144, 1, stdout);
    }

    return 0;

}
