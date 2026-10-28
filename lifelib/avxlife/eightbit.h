#pragma once
#include "portable_sse.h"
#include <stdint.h>

namespace apg {

    const static uint8_t __rotbytes4[] __attribute__((aligned(64))) = {0,
        8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15};

    void replane_sse2(const uint64_t* __restrict__ a, uint64_t* __restrict__ b) {

#ifndef LIFELIB_PORTABLE
        asm (
        "movups (%2), %%xmm0 \n\t"

        "movups (%0)   , %%xmm4 \n\t"
        "movups 16(%0) , %%xmm5 \n\t"
        "movups 32(%0) , %%xmm6 \n\t"
        "movups 48(%0) , %%xmm7 \n\t"
        "movups 64(%0) , %%xmm8 \n\t"
        "movups 80(%0) , %%xmm9 \n\t"
        "movups 96(%0) , %%xmm10 \n\t"
        "movups 112(%0), %%xmm11 \n\t"

        #include "eightbit_sse2.asm"

        : /* no output operands -- implicitly volatile */
        : "r" (a), "r" (b), "r" (__rotbytes4)
        : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6",
          "xmm7", "xmm8", "xmm9", "xmm10", "xmm11", "memory" );
#else
        __builtin_trap(); // x86 asm not available in this build
#endif

    }

    void deplane_sse2(const uint64_t* __restrict__ a, uint64_t* __restrict__ b) {

#ifndef LIFELIB_PORTABLE
        asm (
        "movups (%2), %%xmm0 \n\t"

        "movups (%0)   , %%xmm4 \n\t"
        "movups 32(%0) , %%xmm5 \n\t"
        "movups 64(%0) , %%xmm6 \n\t"
        "movups 96(%0) , %%xmm7 \n\t"
        "movups 128(%0), %%xmm8 \n\t"
        "movups 160(%0), %%xmm9 \n\t"
        "movups 192(%0), %%xmm10 \n\t"
        "movups 224(%0), %%xmm11 \n\t"

        #include "eightbit_sse2.asm"

        : /* no output operands -- implicitly volatile */
        : "r" (a), "r" (b), "r" (__rotbytes4)
        : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6",
          "xmm7", "xmm8", "xmm9", "xmm10", "xmm11", "memory" );
#else
        __builtin_trap(); // x86 asm not available in this build
#endif

    }

    void bytes2planes(uint64_t* __restrict__ result, uint64_t* __restrict__ outleafx) {

        for (int i = 0; i < 32; i++) {
            uint64_t x = result[i];
            uint64_t t = (x ^ (x >> 7)) & 0x00AA00AA00AA00AALL;
            x ^= t ^ (t << 7);
            t = (x ^ (x >> 14)) & 0x0000CCCC0000CCCCLL;
            x ^= t ^ (t << 14);
            t = (x ^ (x >> 28)) & 0x00000000F0F0F0F0LL;
            x ^= t ^ (t << 28);
            result[i] = x;
        }

        replane_sse2(result, outleafx);
        replane_sse2(result + 16, outleafx + 2);

    }

    void planes2bytes(uint64_t const* const* __restrict__ inleafxs, uint64_t* __restrict__ bytematrix) {

        // Apply the involution (3 8) (4 9) (5 10) (7 11)
        deplane_sse2(inleafxs[0], bytematrix);
        deplane_sse2(inleafxs[0] + 2, bytematrix + 32);
        deplane_sse2(inleafxs[1], bytematrix + 2);
        deplane_sse2(inleafxs[1] + 2, bytematrix + 34);
        deplane_sse2(inleafxs[2], bytematrix + 64);
        deplane_sse2(inleafxs[2] + 2, bytematrix + 96);
        deplane_sse2(inleafxs[3], bytematrix + 66);
        deplane_sse2(inleafxs[3] + 2, bytematrix + 98);

        // Apply the involution (0 3) (1 4) (2 5); according
        // to godbolt this is automatically vectorised:
        for (int i = 0; i < 128; i++) {
            uint64_t x = bytematrix[i];
            uint64_t t = (x ^ (x >> 7)) & 0x00AA00AA00AA00AALL;
            x ^= t ^ (t << 7);
            t = (x ^ (x >> 14)) & 0x0000CCCC0000CCCCLL;
            x ^= t ^ (t << 14);
            t = (x ^ (x >> 28)) & 0x00000000F0F0F0F0LL;
            x ^= t ^ (t << 28);
            bytematrix[i] = x;
        }
    }

}
