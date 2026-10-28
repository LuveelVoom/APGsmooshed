#pragma once
#include "core.hpp"
#include <stdint.h>
#include <stdlib.h>
#include <new>

namespace hh {

/**
 * Zero-allocate nbytes of memory with 128-byte alignment.
 *
 * See https://stackoverflow.com/q/29199779/5130486 for why this is useful.
 */
inline void* zalloc(uint64_t nbytes) {

    // allocate 128 + round_up(nbytes, 128) bytes:
    uint64_t nelem = (nbytes + 255) >> 7;
    uint64_t memstart = (uint64_t) calloc(nelem, 128);

    if (memstart == 0) {
        throw std::bad_alloc();
    }

    // compute offset from raw memory to aligned memory;
    // this is always >= 1, so there's an extra byte we
    // can use immediately before the aligned memory,
    // and it's just large enough to store the offset:
    uint64_t offset = 128 - (memstart & 127);

    // determine aligned memory pointer:
    void* ptr = (void*) (memstart + offset);

    // write offset immediately before aligned memory chunk:
    ((uint8_t*) ptr)[-1] = (uint8_t) offset;

    return ptr;
}

/**
 * Deallocate an aligned block of memory previously zalloc'd.
 */
inline void zfree(void* ptr) {

    if (ptr != 0) {
        uint8_t offset = ((uint8_t*) ptr)[-1];
        free((void*) (((uint64_t) ptr) - offset));
    }

}

} // namespace hh
