#pragma once
#include <stdint.h>

namespace apg {

    #define MAKE_SIXTEEN(x) { x,  x,  x,  x,  x,  x,  x,  x,  x,  x,  x,  x,  x,  x,  x,  x, \
                              1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, \
                              2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, \
                              1,  2,  3,  4,  5,  6,  7,  0 }

    const static uint32_t __sixteen28[] __attribute__((aligned(64))) = MAKE_SIXTEEN(0x3ffffffcu);
    const static uint32_t __sixteen24[] __attribute__((aligned(64))) = MAKE_SIXTEEN(0x0ffffff0u);
    const static uint32_t __sixteen20[] __attribute__((aligned(64))) = MAKE_SIXTEEN(0x03ffffc0u);
    const static uint32_t __sixteen16[] __attribute__((aligned(64))) = MAKE_SIXTEEN(0x00ffff00u);
    const static uint32_t __sixteen12[] __attribute__((aligned(64))) = MAKE_SIXTEEN(0x003ffc00u);
    const static uint32_t __sixteen8[]  __attribute__((aligned(64))) = MAKE_SIXTEEN(0x000ff000u);

}
