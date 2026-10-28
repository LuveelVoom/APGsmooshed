#pragma once

#include "../cpads/include/cpads/core.hpp"

// Specific integer types:
#define uint64_cu unsigned long long int
#define uint32_cu unsigned int

static_assert(sizeof(uint64_cu) == sizeof(uint64_t),
    "uint64_cu must be an unsigned 64-bit integer");

static_assert(sizeof(uint32_cu) == sizeof(uint32_t),
    "uint32_cu must be an unsigned 32-bit integer");
