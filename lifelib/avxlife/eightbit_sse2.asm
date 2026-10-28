
        "pshufb %%xmm0, %%xmm4 \n\t"
        "pshufb %%xmm0, %%xmm5 \n\t"
        "pshufb %%xmm0, %%xmm6 \n\t"
        "pshufb %%xmm0, %%xmm7 \n\t"
        "pshufb %%xmm0, %%xmm8 \n\t"
        "pshufb %%xmm0, %%xmm9 \n\t"
        "pshufb %%xmm0, %%xmm10 \n\t"
        "pshufb %%xmm0, %%xmm11 \n\t"

        /* xmm4, xmm5, xmm6, xmm7, xmm8, xmm9, xmm10, xmm11 */

        "movdqa %%xmm4, %%xmm0 \n\t"
        "movdqa %%xmm6, %%xmm1 \n\t"
        "movdqa %%xmm8, %%xmm2 \n\t"
        "movdqa %%xmm10, %%xmm3 \n\t"
        "punpckhbw %%xmm5, %%xmm0 \n\t"
        "punpcklbw %%xmm5, %%xmm4 \n\t"
        "punpckhbw %%xmm7, %%xmm1 \n\t"
        "punpcklbw %%xmm7, %%xmm6 \n\t"
        "punpckhbw %%xmm9, %%xmm2 \n\t"
        "punpcklbw %%xmm9, %%xmm8 \n\t"
        "punpckhbw %%xmm11, %%xmm3 \n\t"
        "punpcklbw %%xmm11, %%xmm10 \n\t"

        /* xmm4, xmm0, xmm6, xmm1, xmm8, xmm2, xmm10, xmm3 */

        "movdqa %%xmm4, %%xmm5 \n\t"
        "movdqa %%xmm0, %%xmm7 \n\t"
        "movdqa %%xmm8, %%xmm9 \n\t"
        "movdqa %%xmm2, %%xmm11 \n\t"
        "punpckhwd %%xmm6, %%xmm5 \n\t"
        "punpcklwd %%xmm6, %%xmm4 \n\t"
        "punpckhwd %%xmm1, %%xmm7 \n\t"
        "punpcklwd %%xmm1, %%xmm0 \n\t"
        "punpckhwd %%xmm10, %%xmm9 \n\t"
        "punpcklwd %%xmm10, %%xmm8 \n\t"
        "punpckhwd %%xmm3, %%xmm11 \n\t"
        "punpcklwd %%xmm3, %%xmm2 \n\t"

        /* xmm4, xmm5, xmm0, xmm7, xmm8, xmm9, xmm2, xmm11 */

        "movdqa %%xmm4, %%xmm1 \n\t"
        "movdqa %%xmm5, %%xmm3 \n\t"
        "movdqa %%xmm0, %%xmm6 \n\t"
        "movdqa %%xmm7, %%xmm10 \n\t"
        "punpckhdq %%xmm8, %%xmm1 \n\t"
        "punpckldq %%xmm8, %%xmm4 \n\t"
        "punpckhdq %%xmm9, %%xmm3 \n\t"
        "punpckldq %%xmm9, %%xmm5 \n\t"
        "punpckhdq %%xmm2, %%xmm6 \n\t"
        "punpckldq %%xmm2, %%xmm0 \n\t"
        "punpckhdq %%xmm11, %%xmm10 \n\t"
        "punpckldq %%xmm11, %%xmm7 \n\t"

        /* xmm4, xmm1, xmm5, xmm3, xmm0, xmm6, xmm7, xmm10 */

        "movups %%xmm4, (%1) \n\t"
        "movups %%xmm1, 32(%1) \n\t"
        "movups %%xmm5, 64(%1) \n\t"
        "movups %%xmm3, 96(%1) \n\t"
        "movups %%xmm0, 128(%1) \n\t"
        "movups %%xmm6, 160(%1) \n\t"
        "movups %%xmm7, 192(%1) \n\t"
        "movups %%xmm10, 224(%1) \n\t"
