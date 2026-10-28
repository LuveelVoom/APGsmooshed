#pragma once
/*
* Intrinsic transcriptions of lifelib's SSE2/SSSE3/SSE4.1 inline asm, one
* intrinsic per instruction, so that lifelib builds where x86 inline asm is
* unavailable (arm64, via sse2neon). Defining LIFELIB_PORTABLE on x86 selects
* these too, which is how they are checked against the original asm.
*
* Only the routines reached when best_instruction_set() == 8 (SSE4.2) are
* transcribed; every other asm block traps if reached under LIFELIB_PORTABLE.
*/

#if !defined(__x86_64__) && !defined(__i386__) && !defined(LIFELIB_PORTABLE)
#define LIFELIB_PORTABLE 1
#endif

#ifdef LIFELIB_PORTABLE

#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#else
#include "sse2neon.h"
#endif
#include <stdint.h>

namespace apg { namespace portable {

    static inline __m128i ld(const void* p, int off) {
        return _mm_loadu_si128((const __m128i*) (((const uint8_t*) p) + off));
    }

    static inline void st(void* p, int off, __m128i x) {
        _mm_storeu_si128((__m128i*) (((uint8_t*) p) + off), x);
    }

    static inline void transpose_bytes_sse2(const uint64_t* a, uint64_t* b) {
        __m128i x0 = ld(a, 0), x1 = ld(a, 16), x2 = ld(a, 32), x3 = ld(a, 48);
        __m128i x4 = x0, x5 = x2;
        x0 = _mm_unpacklo_epi8(x0, x1);
        x4 = _mm_unpackhi_epi8(x4, x1);
        x2 = _mm_unpacklo_epi8(x2, x3);
        x5 = _mm_unpackhi_epi8(x5, x3);
        x1 = x0; x3 = x2;
        x0 = _mm_unpacklo_epi8(x0, x4);
        x1 = _mm_unpackhi_epi8(x1, x4);
        x2 = _mm_unpacklo_epi8(x2, x5);
        x3 = _mm_unpackhi_epi8(x3, x5);
        x4 = x0;
        x0 = _mm_unpacklo_epi32(x0, x2);
        x4 = _mm_unpackhi_epi32(x4, x2);
        x5 = x1;
        x1 = _mm_unpacklo_epi32(x1, x3);
        x5 = _mm_unpackhi_epi32(x5, x3);
        st(b, 0, x0); st(b, 16, x4); st(b, 32, x1); st(b, 48, x5);
    }

    static inline void twofifths_sse2(const uint32_t* a, uint64_t* b) {
        __m128i x0 = ld(a, 0), x1 = ld(a, 16);
        __m128i x2 = x0;
        x0 = _mm_unpacklo_epi8(x0, x1);
        x2 = _mm_unpackhi_epi8(x2, x1);
        x1 = x0;
        x0 = _mm_unpacklo_epi8(x0, x2);
        x1 = _mm_unpackhi_epi8(x1, x2);
        x2 = x0;
        x0 = _mm_unpacklo_epi8(x0, x1);
        x2 = _mm_unpackhi_epi8(x2, x1);
        st(b, 0, x0); st(b, 16, x2);
    }

    static inline void z64_to_r32_half(const uint64_t* a, uint32_t* b, int off) {
        __m128i x0 = ld(a, off), x3 = ld(a, off + 16), x1 = ld(a, off + 32), x4 = ld(a, off + 48);
        __m128i x2 = x0, x5 = x3;
        x0 = _mm_unpacklo_epi8(x0, x1);
        x3 = _mm_unpacklo_epi8(x3, x4);
        x2 = _mm_unpackhi_epi8(x2, x1);
        x5 = _mm_unpackhi_epi8(x5, x4);
        x1 = x0; x4 = x3;
        x0 = _mm_unpacklo_epi8(x0, x2);
        x3 = _mm_unpacklo_epi8(x3, x5);
        x1 = _mm_unpackhi_epi8(x1, x2);
        x4 = _mm_unpackhi_epi8(x4, x5);
        st(b, off, x0); st(b, off + 16, x1); st(b, off + 32, x3); st(b, off + 48, x4);
    }

    static inline void z64_to_r32_sse2(const uint64_t* a, uint32_t* b) {
        z64_to_r32_half(a, b, 0);
        z64_to_r32_half(a, b, 64);
    }

    static inline void z64_to_r32_centre_ssse3(const uint64_t* c, uint32_t* b, const uint8_t* perm) {
        __m128i p = ld(perm, 0);
        __m128i x0 = _mm_shuffle_epi8(ld(c, 0), p);
        __m128i x2 = _mm_shuffle_epi8(ld(c, 16), p);
        st(b, 1, x0); st(b, 15, x0); st(b, 33, x2); st(b, 47, x2);
    }

    static inline void r32_centre_to_z64_ssse3(const uint32_t* b, uint64_t* c, const uint8_t* perm) {
        __m128i x0 = _mm_srli_epi32(ld(b, 31), 16);
        __m128i x1 = _mm_slli_epi32(ld(b, 49), 16);
        __m128i x2 = _mm_srli_epi32(ld(b, 63), 16);
        __m128i x3 = _mm_slli_epi32(ld(b, 81), 16);
        __m128i p = ld(perm, 0);
        x0 = _mm_shuffle_epi8(_mm_or_si128(x0, x1), p);
        x2 = _mm_shuffle_epi8(_mm_or_si128(x2, x3), p);
        st(c, 0, x0); st(c, 16, x2);
    }

    static inline void r32_centre_to_z64_sse4(const uint32_t* b, uint64_t* c, const uint8_t* perm) {
        __m128i x0 = _mm_blend_epi16(ld(b, 33), ld(b, 47), 170);
        __m128i x2 = _mm_blend_epi16(ld(b, 65), ld(b, 79), 170);
        __m128i p = ld(perm, 0);
        st(c, 0, _mm_shuffle_epi8(x0, p));
        st(c, 16, _mm_shuffle_epi8(x2, p));
    }

    // First asm block of isoluts.h iterate_var_leaf: f = rows (uint32[32],
    // 7..24 used), m = lut9 + 64 (three 16-byte masks), e = uint32[32] out.
    static inline void isolut_mix(const uint32_t* f, const uint8_t* m, uint32_t* e) {
        __m128i x1 = ld(m, 0), x2 = ld(m, 16), x3 = ld(m, 32);
        for (int k = 0; k < 8; k++) {
            __m128i x0 = _mm_shuffle_epi8(ld(f, 28 + 8 * k), x1);
            __m128i x4 = _mm_srli_epi32(_mm_andnot_si128(x2, x0), 2);
            x0 = _mm_or_si128(_mm_and_si128(x0, x2), x4);
            x4 = _mm_srli_epi32(_mm_andnot_si128(x3, x0), 4);
            x0 = _mm_or_si128(_mm_and_si128(x0, x3), x4);
            st(e, 16 * k, x0);
        }
    }

    // Second asm block of isoluts.h: shufps $216 on each 16-byte half.
    static inline void isolut_shuf216(uint64_t* o) {
        st(o, 0, _mm_shuffle_epi32(ld(o, 0), 216));
        st(o, 16, _mm_shuffle_epi32(ld(o, 16), 216));
    }

} }

#endif
