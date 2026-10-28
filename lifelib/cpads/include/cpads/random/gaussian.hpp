#pragma once
#include "../core.hpp"

namespace hh {

// actual parameters for the Gaussian generators:
#include "epoch0.h"

/**
 * Copies the lookup table from constant memory into shared memory
 * for fast random access.
 */
_DI_ void populate_smem_icdf(int32_t* smem_icdf) {
    for (int i = threadIdx.x; i < 4096; i += blockDim.x) {
        smem_icdf[i] = gaussian_icdf[i];
    }

    __syncthreads();
}

#define CREATE_SMEM_ICDF(symbol_name) __shared__ int32_t symbol_name[4096]; hh::populate_smem_icdf(symbol_name)

#define COND_NEGATE(i, x) if (entropy & (1u << (i))) { x = -x; }

// Use the SHFL.BFLY instruction to exchange information between threads:
#define HADAMARD_MIX(i) { int32_t s = a + b; a -= b; b = shuffle_xor_32(s, (i)); }

/**
 * Creates a double-precision Gaussian random variable with specified
 * mean and standard deviation. The 'entropy' argument should be a
 * uniform random uint32.
 *
 * https://cp4space.hatsya.com/2022/01/09/training-a-random-gaussian-generator/
 *
 * This should be called synchronously by a whole warp (32 threads),
 * as the algorithm relies on applying a random orthogonal linear
 * transformation across the warp.
 *
 * The array smem_icdf should be created by CREATE_SMEM_ICDF at the
 * beginning of the CUDA kernel.
 */
_DI_ double warpGaussian(uint32_t entropy, const int32_t* smem_icdf, double mu=0.0, double sigma=1.0) {

    // bank conflict avoidance strategy:
    int laneId = threadIdx.x & 15;

    // use 16 bits of entropy to retrieve two rectified weak Gaussians
    // (so the entire warp has 64 such rectified weak Gaussians).
    // ....bbbbbbbb........aaaaaaaa....
    int32_t a = smem_icdf[(entropy & 4080) | laneId];
    int32_t b = smem_icdf[((entropy >> 16) & 4080) | laneId];

    // conditionally negate to 'unrectify' the weak Gaussians, making
    // them symmetrically distributed:
    COND_NEGATE(19, a) COND_NEGATE(18, b)

    // perform the first three layers of Hadamard mixing:
    HADAMARD_MIX(1)
    COND_NEGATE(17, a) COND_NEGATE(16, b)
    HADAMARD_MIX(2)
    COND_NEGATE(15, a) COND_NEGATE(14, b)
    HADAMARD_MIX(4)
    COND_NEGATE(13, a) COND_NEGATE(12, b)

    // create a uniform random odd int32 (centred on zero).
    int32_t c = (entropy ^ b) | 1;

    // use the lowest 4 bits of entropy (those that contributed the
    // least to the uniform variable c) for the last two layers of
    // the Hadamard tiramisu.
    HADAMARD_MIX(8)
    COND_NEGATE(3, a) COND_NEGATE(2, b)
    HADAMARD_MIX(16)
    COND_NEGATE(0, a) COND_NEGATE(1, b)

    // at this point, a is a {+1,-1}-weighted sum of the 32 values from
    // this half-warp; b is a {+1,-1}-weighted sum of the 32 values from
    // the other half-warp.

    double result = mu;

    // our output is the combination of two high-variance Gaussian
    // components and one low-variance uniform component:
    {
        double a_scale = sigma * ordinary_params[0];
        double b_scale = sigma * ordinary_params[1];
        double c_scale_hi = sigma * ordinary_params[2];
        double c_scale_lo = sigma * ordinary_params[3];
        result += a * a_scale;
        result += b * b_scale;
        result += c * c_scale_hi;
        result += c * c_scale_lo;
    }

    return result;
}

#undef COND_NEGATE
#undef HADAMARD_MIX

} // namespace hh
