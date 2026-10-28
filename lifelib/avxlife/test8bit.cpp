#include "eightbit.h"
#include <iostream>

int main() {

    uint8_t x[256];
    uint8_t y[256];
    uint8_t z[256];

    for (int i = 0; i < 256; i++) { x[i] = i; }

    for (int i = 0; i < 256; i++) { std::cout << ((int) x[i]) << " "; }
    std::cout << std::endl;

    apg::deplane_sse2((uint64_t*) x, (uint64_t*) y);
    apg::deplane_sse2(((uint64_t*) x) + 2, ((uint64_t*) y) + 2);

    for (int i = 0; i < 256; i++) { std::cout << ((int) y[i]) << " "; }
    std::cout << std::endl;

    apg::deplane_sse2((uint64_t*) y, (uint64_t*) z);
    apg::deplane_sse2(((uint64_t*) y) + 2, ((uint64_t*) z) + 2);

    for (int i = 0; i < 256; i++) { std::cout << (((int) z[i]) - i) << " "; }
    std::cout << std::endl;

    return 0;

}
