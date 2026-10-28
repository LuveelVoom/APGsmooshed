#pragma once

#define ROTR64(x, y) (((x) >> (y)) | ((x) << ((64 - (y)) & 63)))
#define ROTL64(x, y) (((x) << (y)) | ((x) >> ((64 - (y)) & 63)))
#define ROTR32(x, y) (((x) >> (y)) | ((x) << ((32 - (y)) & 31)))
#define ROTL32(x, y) (((x) << (y)) | ((x) >> ((32 - (y)) & 31)))

#include <stdint.h>

// Specific integer types:
#define uint64_cu unsigned long long int
#define uint32_cu unsigned int

static_assert(sizeof(uint64_cu) == sizeof(uint64_t),
    "uint64_cu must be an unsigned 64-bit integer");

static_assert(sizeof(uint32_cu) == sizeof(uint32_t),
    "uint32_cu must be an unsigned 32-bit integer");

#if CUDART_VERSION >= 9000
#define FULL_BALLOT(predicate) __ballot_sync(0xffffffffu, predicate)
#else
#define FULL_BALLOT(predicate) __ballot(predicate)
#endif

__constant__ uint64_cu c_topology[128];

