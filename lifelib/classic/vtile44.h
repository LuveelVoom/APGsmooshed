#include "../avxlife/portable_sse.h"
    #ifdef __AVX512F__
    template<>
        void VTile<44>::copyBoundary3(VTile<44> *n) {
#ifndef LIFELIB_PORTABLE
            asm (
                "vmovdqu64 8(%0), %%zmm6 \n\t"
                "vmovdqu64 8(%1), %%zmm8 \n\t"
                "vmovdqu64 (%2), %%zmm14 \n\t"
                "vpsrld $28, %%zmm8, %%zmm8 \n\t"
                "vpternlogd $228, %%zmm14, %%zmm8, %%zmm6 \n\t"
                "vmovdqu64 %%zmm6, 8(%0) \n\t"

                "vmovdqu64 72(%0), %%zmm6 \n\t"
                "vmovdqu64 72(%1), %%zmm8 \n\t"
                "vpsrld $28, %%zmm8, %%zmm8 \n\t"
                "vpternlogd $228, %%zmm14, %%zmm8, %%zmm6 \n\t"
                "vmovdqu64 %%zmm6, 72(%0) \n\t"

                "vmovdqu 136(%0), %%ymm6 \n\t"
                "vmovdqu 168(%0), %%xmm13 \n\t"
                "vshufi32x4 $68, %%zmm13, %%zmm6, %%zmm6 \n\t"
                "vmovdqu 136(%1), %%ymm8 \n\t"
                "vmovdqu 168(%1), %%xmm13 \n\t"
                "vshufi32x4 $68, %%zmm13, %%zmm8, %%zmm8 \n\t"
                "vpsrld $28, %%zmm8, %%zmm8 \n\t"
                "vpternlogd $228, %%zmm14, %%zmm8, %%zmm6 \n\t"
                "vshufi32x4 $78, %%zmm6, %%zmm6, %%zmm13 \n\t"
                "vmovdqu %%ymm6, 136(%0) \n\t"
                "vmovdqu %%xmm13, 168(%0) \n\t"
                : /* no output operands */ 
                : "r" (d), "r" (n->d), "r" (apg::__16xfffffffc)
                : "xmm6", "xmm8", "xmm13", "xmm14", "memory");
#else
        __builtin_trap(); // x86 asm not available in this build
#endif
        }

    template<>
        void VTile<44>::copyBoundary0(VTile<44> *n) {
#ifndef LIFELIB_PORTABLE
            asm (
                "vmovdqu64 8(%0), %%zmm6 \n\t"
                "vmovdqu64 8(%1), %%zmm8 \n\t"
                "vmovdqu64 (%2), %%zmm14 \n\t"
                "vpslld $28, %%zmm8, %%zmm8 \n\t"
                "vpternlogd $228, %%zmm14, %%zmm8, %%zmm6 \n\t"
                "vmovdqu64 %%zmm6, 8(%0) \n\t"

                "vmovdqu64 72(%0), %%zmm6 \n\t"
                "vmovdqu64 72(%1), %%zmm8 \n\t"
                "vpslld $28, %%zmm8, %%zmm8 \n\t"
                "vpternlogd $228, %%zmm14, %%zmm8, %%zmm6 \n\t"
                "vmovdqu64 %%zmm6, 72(%0) \n\t"

                "vmovdqu 136(%0), %%ymm6 \n\t"
                "vmovdqu 168(%0), %%xmm13 \n\t"
                "vshufi32x4 $68, %%zmm13, %%zmm6, %%zmm6 \n\t"
                "vmovdqu 136(%1), %%ymm8 \n\t"
                "vmovdqu 168(%1), %%xmm13 \n\t"
                "vshufi32x4 $68, %%zmm13, %%zmm8, %%zmm8 \n\t"
                "vpslld $28, %%zmm8, %%zmm8 \n\t"
                "vpternlogd $228, %%zmm14, %%zmm8, %%zmm6 \n\t"
                "vshufi32x4 $78, %%zmm6, %%zmm6, %%zmm13 \n\t"
                "vmovdqu %%ymm6, 136(%0) \n\t"
                "vmovdqu %%xmm13, 168(%0) \n\t"
                : /* no output operands */ 
                : "r" (d), "r" (n->d), "r" (apg::__16x3fffffff)
                : "xmm6", "xmm8", "xmm13", "xmm14", "memory");
#else
        __builtin_trap(); // x86 asm not available in this build
#endif
        }

    #endif

    template<>
        inline void VTile<44>::updateTile(upattern<VTile<44>, 28, 44>* owner, int rule, int family, int mantissa) {

            (void) mantissa;
            uint32_t diffs[3] = {0};

            int r;
            if (family == 1) {
                r = iterate_var_48_28(rule, d, hist, diffs);
            } else {
                r = iterate_var_48_28(rule, d, diffs);
            }

            if ((r != 1) && (diffs[0] & 0x3ffffffcu)) {
                if (!(currentflags & 1)) { owner->popchanged.push_back(this); }
                currentflags = 3;
                if (updateflags == 0) { owner->modified.push_back(this); }
                updateflags |= 64;
                if (diffs[0] & 0x30000000u) { owner->updateNeighbour(this, 0); }
                if (diffs[0] & 0x0000000cu) { owner->updateNeighbour(this, 3); }
                if (diffs[1] & 0x3fffc000u) { owner->updateNeighbour(this, 1); }
                if (diffs[1] & 0x0003fffcu) { owner->updateNeighbour(this, 2); }
                if (diffs[2] & 0x3fffc000u) { owner->updateNeighbour(this, 5); }
                if (diffs[2] & 0x0003fffcu) { owner->updateNeighbour(this, 4); }
            }
        }

        typedef VTile<44> VTile44;

    static_assert(std::is_standard_layout<VTile44>::value, "VTile44 must be POD type");
    static_assert(sizeof(VTile44) == 448, "VTile44 must be exactly 448 bytes");

