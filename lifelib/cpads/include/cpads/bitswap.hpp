#pragma once
#include "vec.hpp"

namespace hh {


template<typename T, size_t N>
_HDC_ void bitflip_inplace(hh::vec<T, N> &x, size_t i) {

#include "internal/mu6.inc"

    if (i < log2_bits_per_word) {
        T mask = ((T) mu[i]);
        size_t shift = (1ull << i);
        x.modify([&](T &t) __attribute__((always_inline)) {
            t = ((t & mask) << shift) | ((t &~ mask) >> shift);
        });
    } else {
        size_t wi = i - log2_bits_per_word;
        size_t delta = (1ull << wi);
        for (size_t M = delta; M < N; M++) {
            if ((M >> wi) & 1) {
                x.swap(M - delta, M);
            }
        }
    }
}


template<typename T, size_t N>
_HDC_ void bitswap_inplace_inner(hh::vec<T, N> &x, size_t i, size_t j) {

#include "internal/mu6.inc"

    if (j < log2_bits_per_word) {
        // intra-word swap:
        size_t shift = (1ull << j) - (1ull << i);
        T mask = ((T) (mu[j] &~ mu[i]));

        x.modify([&](T &t) __attribute__((always_inline)) {
            T d = (t ^ (t >> shift)) & mask;
            t ^= (d ^ (d << shift));
        });

    } else if (i < log2_bits_per_word) {
        // mixed swap:
        size_t wj = j - log2_bits_per_word;
        size_t delta = (1ull << wj);
        size_t shift = (1ull << i);
        for (size_t M = delta; M < N; M++) {
            if ((M >> wj) & 1) {
                uint64_t d = (x[M] ^ (x[M-delta] >> shift)) & mu[i];
                x[M] ^= d;
                x[M-delta] ^= (d << shift);
            }
        }

    } else {
        // inter-word swap:
        size_t wi = i - log2_bits_per_word;
        size_t wj = j - log2_bits_per_word;
        size_t delta = (1ull << wj) - (1ull << wi);
        for (size_t M = delta; M < N; M++) {
            if ((M >> wj) & 1 &~ (M >> wi)) {
                x.swap(M - delta, M);
            }
        }
    }
}

/**
 * Swaps the ith and jth least significant axes of a bit-tensor x.
 */
template<typename T, size_t N>
_HDC_ void bitswap_inplace(hh::vec<T, N> &x, size_t i, size_t j) {

    if (i == j) { return; }

    size_t minij = (i < j) ? i : j;
    size_t maxij = (i < j) ? j : i;

    bitswap_inplace_inner(x, minij, maxij);

}

template<typename T, size_t N>
_HDC_ auto bitswap(const hh::vec<T, N> &x, size_t i, size_t j) {

    hh::vec<T, N> y = x;
    bitswap_inplace(y, i, j);
    return y;

}

template<typename T, size_t N>
_HDC_ auto bitflip(const hh::vec<T, N> &x, size_t i) {

    hh::vec<T, N> y = x;
    bitflip_inplace(y, i);
    return y;

}

} // namespace hh
