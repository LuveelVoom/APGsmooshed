#include "../avxlife/portable_sse.h"

    #ifdef __AVX512F__
    template<>
        void VTile<28>::copyBoundary3(VTile<28> *n) {
#ifndef LIFELIB_PORTABLE
            asm (
                "vmovdqu64 8(%0), %%zmm6 \n\t"
                "vmovdqu64 8(%1), %%zmm8 \n\t"
                "vmovdqu64 (%2), %%zmm14 \n\t"
                "vpsrld $28, %%zmm8, %%zmm8 \n\t"
                "vpternlogd $228, %%zmm14, %%zmm8, %%zmm6 \n\t"
                "vmovdqu64 %%zmm6, 8(%0) \n\t"
                "vmovdqu 72(%0), %%ymm6 \n\t"
                "vmovdqu 104(%0), %%xmm13 \n\t"
                "vshufi32x4 $68, %%zmm13, %%zmm6, %%zmm6 \n\t"
                "vmovdqu 72(%1), %%ymm8 \n\t"
                "vmovdqu 104(%1), %%xmm13 \n\t"
                "vshufi32x4 $68, %%zmm13, %%zmm8, %%zmm8 \n\t"
                "vpsrld $28, %%zmm8, %%zmm8 \n\t"
                "vpternlogd $228, %%zmm14, %%zmm8, %%zmm6 \n\t"
                "vshufi32x4 $78, %%zmm6, %%zmm6, %%zmm13 \n\t"
                "vmovdqu %%ymm6, 72(%0) \n\t"
                "vmovdqu %%xmm13, 104(%0) \n\t"
                : /* no output operands */ 
                : "r" (d), "r" (n->d), "r" (apg::__16xfffffffc)
                : "xmm6", "xmm8", "xmm13", "xmm14", "memory");
#else
        __builtin_trap(); // x86 asm not available in this build
#endif
        }

    template<>
        void VTile<28>::copyBoundary0(VTile<28> *n) {
#ifndef LIFELIB_PORTABLE
            asm (
                "vmovdqu64 8(%0), %%zmm6 \n\t"
                "vmovdqu64 8(%1), %%zmm8 \n\t"
                "vmovdqu64 (%2), %%zmm14 \n\t"
                "vpslld $28, %%zmm8, %%zmm8 \n\t"
                "vpternlogd $228, %%zmm14, %%zmm8, %%zmm6 \n\t"
                "vmovdqu64 %%zmm6, 8(%0) \n\t"
                "vmovdqu 72(%0), %%ymm6 \n\t"
                "vmovdqu 104(%0), %%xmm13 \n\t"
                "vshufi32x4 $68, %%zmm13, %%zmm6, %%zmm6 \n\t"
                "vmovdqu 72(%1), %%ymm8 \n\t"
                "vmovdqu 104(%1), %%xmm13 \n\t"
                "vshufi32x4 $68, %%zmm13, %%zmm8, %%zmm8 \n\t"
                "vpslld $28, %%zmm8, %%zmm8 \n\t"
                "vpternlogd $228, %%zmm14, %%zmm8, %%zmm6 \n\t"
                "vshufi32x4 $78, %%zmm6, %%zmm6, %%zmm13 \n\t"
                "vmovdqu %%ymm6, 72(%0) \n\t"
                "vmovdqu %%xmm13, 104(%0) \n\t"
                : /* no output operands */ 
                : "r" (d), "r" (n->d), "r" (apg::__16x3fffffff)
                : "xmm6", "xmm8", "xmm13", "xmm14", "memory");
#else
        __builtin_trap(); // x86 asm not available in this build
#endif
        }
    #else
    #ifdef __AVX2__
    template<>
        void VTile<28>::copyBoundary3(VTile<28> *n) {
#ifndef LIFELIB_PORTABLE
            asm (
                "vmovdqu 8(%0), %%ymm6 \n\t"
                "vmovdqu 8(%1), %%ymm8 \n\t"
                "vmovdqu (%2), %%ymm14 \n\t"
                "vpsrld $28, %%ymm8, %%ymm8 \n\t"
                "vpand %%ymm14, %%ymm6, %%ymm6 \n\t"
                "vpandn %%ymm8, %%ymm14, %%ymm8 \n\t"
                "vpor %%ymm8, %%ymm6, %%ymm6 \n\t"
                "vmovdqu %%ymm6, 8(%0) \n\t"

                "vmovdqu 40(%0), %%ymm6 \n\t"
                "vmovdqu 40(%1), %%ymm8 \n\t"
                "vpsrld $28, %%ymm8, %%ymm8 \n\t"
                "vpand %%ymm14, %%ymm6, %%ymm6 \n\t"
                "vpandn %%ymm8, %%ymm14, %%ymm8 \n\t"
                "vpor %%ymm8, %%ymm6, %%ymm6 \n\t"
                "vmovdqu %%ymm6, 40(%0) \n\t"

                "vmovdqu 72(%0), %%ymm6 \n\t"
                "vmovdqu 72(%1), %%ymm8 \n\t"
                "vpsrld $28, %%ymm8, %%ymm8 \n\t"
                "vpand %%ymm14, %%ymm6, %%ymm6 \n\t"
                "vpandn %%ymm8, %%ymm14, %%ymm8 \n\t"
                "vpor %%ymm8, %%ymm6, %%ymm6 \n\t"
                "vmovdqu %%ymm6, 72(%0) \n\t"

                "vmovdqu 104(%0), %%xmm6 \n\t"
                "vmovdqu 104(%1), %%xmm8 \n\t"
                "vpsrld $28, %%xmm8, %%xmm8 \n\t"
                "vpand %%xmm14, %%xmm6, %%xmm6 \n\t"
                "vpandn %%xmm8, %%xmm14, %%xmm8 \n\t"
                "vpor %%xmm8, %%xmm6, %%xmm6 \n\t"
                "vmovdqu %%xmm6, 104(%0) \n\t"
                : /* no output operands */ 
                : "r" (d), "r" (n->d), "r" (apg::__16xfffffffc)
                : "xmm6", "xmm8", "xmm13", "xmm14", "memory");
#else
        __builtin_trap(); // x86 asm not available in this build
#endif
        }

    template<>
        void VTile<28>::copyBoundary0(VTile<28> *n) {
#ifndef LIFELIB_PORTABLE
            asm (
                "vmovdqu 8(%0), %%ymm6 \n\t"
                "vmovdqu 8(%1), %%ymm8 \n\t"
                "vmovdqu (%2), %%ymm14 \n\t"
                "vpslld $28, %%ymm8, %%ymm8 \n\t"
                "vpand %%ymm14, %%ymm6, %%ymm6 \n\t"
                "vpandn %%ymm8, %%ymm14, %%ymm8 \n\t"
                "vpor %%ymm8, %%ymm6, %%ymm6 \n\t"
                "vmovdqu %%ymm6, 8(%0) \n\t"

                "vmovdqu 40(%0), %%ymm6 \n\t"
                "vmovdqu 40(%1), %%ymm8 \n\t"
                "vpslld $28, %%ymm8, %%ymm8 \n\t"
                "vpand %%ymm14, %%ymm6, %%ymm6 \n\t"
                "vpandn %%ymm8, %%ymm14, %%ymm8 \n\t"
                "vpor %%ymm8, %%ymm6, %%ymm6 \n\t"
                "vmovdqu %%ymm6, 40(%0) \n\t"

                "vmovdqu 72(%0), %%ymm6 \n\t"
                "vmovdqu 72(%1), %%ymm8 \n\t"
                "vpslld $28, %%ymm8, %%ymm8 \n\t"
                "vpand %%ymm14, %%ymm6, %%ymm6 \n\t"
                "vpandn %%ymm8, %%ymm14, %%ymm8 \n\t"
                "vpor %%ymm8, %%ymm6, %%ymm6 \n\t"
                "vmovdqu %%ymm6, 72(%0) \n\t"

                "vmovdqu 104(%0), %%xmm6 \n\t"
                "vmovdqu 104(%1), %%xmm8 \n\t"
                "vpslld $28, %%xmm8, %%xmm8 \n\t"
                "vpand %%xmm14, %%xmm6, %%xmm6 \n\t"
                "vpandn %%xmm8, %%xmm14, %%xmm8 \n\t"
                "vpor %%xmm8, %%xmm6, %%xmm6 \n\t"
                "vmovdqu %%xmm6, 104(%0) \n\t"
                : /* no output operands */ 
                : "r" (d), "r" (n->d), "r" (apg::__16x3fffffff)
                : "xmm6", "xmm8", "xmm13", "xmm14", "memory");
#else
        __builtin_trap(); // x86 asm not available in this build
#endif
        }
    #endif
    #endif

    template<>
        inline void VTile<28>::updateTile(upattern<VTile<28>, 28, 28>* owner, int rule, int family, int mantissa) {

            (void) mantissa;
            uint32_t diffs[3] = {0};

            int r;
            if (family == 1) {
                r = iterate_var_32_28(rule, d, hist, diffs);
            } else {
                r = iterate_var_32_28(rule, d, diffs);
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

        typedef VTile<28> VTile28;

    static_assert(std::is_standard_layout<VTile28>::value, "VTile28 must be POD type");
    static_assert(sizeof(VTile28) == 320, "VTile28 must be exactly 320 bytes");

