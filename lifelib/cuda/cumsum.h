#pragma once

#include "basics.h"

__global__ void exclusive_scan_uint32_256(uint32_cu *ina, uint32_cu *outa, uint64_cu *total) {

    __shared__ uint32_cu tmp1[256];
    __shared__ uint32_cu tmp2[256];

    uint32_cu b = ina[(blockIdx.x << 8) + threadIdx.x];
    uint32_cu a = b;

    tmp1[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 1) { b += tmp1[threadIdx.x - 1]; }
    tmp2[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 2) { b += tmp2[threadIdx.x - 2]; }
    tmp1[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 4) { b += tmp1[threadIdx.x - 4]; }
    tmp2[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 8) { b += tmp2[threadIdx.x - 8]; }
    tmp1[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 16) { b += tmp1[threadIdx.x - 16]; }
    tmp2[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 32) { b += tmp2[threadIdx.x - 32]; }
    tmp1[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 64) { b += tmp1[threadIdx.x - 64]; }
    tmp2[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 128) { b += tmp2[threadIdx.x - 128]; }

    outa[(blockIdx.x << 8) + threadIdx.x] = b - a;
    if (threadIdx.x == 255) { total[blockIdx.x] = b; }

}

__global__ void exclusive_scan_uint32(uint32_cu *ina, uint32_cu *outa, uint64_cu *total) {

    __shared__ uint32_cu tmp1[128];
    __shared__ uint32_cu tmp2[128];

    uint32_cu b = ina[(blockIdx.x << 7) + threadIdx.x];
    uint32_cu a = b;

    tmp1[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 1) { b += tmp1[threadIdx.x - 1]; }
    tmp2[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 2) { b += tmp2[threadIdx.x - 2]; }
    tmp1[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 4) { b += tmp1[threadIdx.x - 4]; }
    tmp2[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 8) { b += tmp2[threadIdx.x - 8]; }
    tmp1[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 16) { b += tmp1[threadIdx.x - 16]; }
    tmp2[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 32) { b += tmp2[threadIdx.x - 32]; }
    tmp1[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 64) { b += tmp1[threadIdx.x - 64]; }

    outa[(blockIdx.x << 7) + threadIdx.x] = b - a;
    if (threadIdx.x == 127) { total[blockIdx.x] = b; }

}

__global__ void exclusive_scan_uint64_128(uint64_cu *ina, uint64_cu *outa, uint64_cu *total) {

    __shared__ uint64_cu tmp1[128];
    __shared__ uint64_cu tmp2[128];

    uint64_cu b = ina[(blockIdx.x << 7) + threadIdx.x];
    uint64_cu a = b;

    tmp1[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 1) { b += tmp1[threadIdx.x - 1]; }
    tmp2[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 2) { b += tmp2[threadIdx.x - 2]; }
    tmp1[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 4) { b += tmp1[threadIdx.x - 4]; }
    tmp2[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 8) { b += tmp2[threadIdx.x - 8]; }
    tmp1[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 16) { b += tmp1[threadIdx.x - 16]; }
    tmp2[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 32) { b += tmp2[threadIdx.x - 32]; }
    tmp1[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 64) { b += tmp1[threadIdx.x - 64]; }

    outa[(blockIdx.x << 7) + threadIdx.x] = b - a;
    if (threadIdx.x == 127) { total[blockIdx.x] = b; }

}

__global__ void exclusive_scan_uint64(uint64_cu *ina, uint64_cu *outa, uint64_cu *total) {

    __shared__ uint64_cu tmp1[64];
    __shared__ uint64_cu tmp2[64];

    uint64_cu b = ina[(blockIdx.x << 6) + threadIdx.x];
    uint64_cu a = b;

    tmp1[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 1) { b += tmp1[threadIdx.x - 1]; }
    tmp2[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 2) { b += tmp2[threadIdx.x - 2]; }
    tmp1[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 4) { b += tmp1[threadIdx.x - 4]; }
    tmp2[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 8) { b += tmp2[threadIdx.x - 8]; }
    tmp1[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 16) { b += tmp1[threadIdx.x - 16]; }
    tmp2[threadIdx.x] = b;
    __syncthreads();
    if (threadIdx.x >= 32) { b += tmp2[threadIdx.x - 32]; }

    outa[(blockIdx.x << 6) + threadIdx.x] = b - a;
    if (threadIdx.x == 63) { total[blockIdx.x] = b; }

}
