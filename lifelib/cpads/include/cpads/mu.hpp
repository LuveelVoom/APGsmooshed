#pragma once
#include "vec.hpp"

namespace hh {

template<typename T, size_t N>
_HDC_ auto get_mu() {

    #include "internal/mu6.inc"

    hh::vec<T, N> res{0};

    for (size_t i = 0; i < N; i++) {
        res[i] = ((T) mu[i]);
    }

    return res;
}

}
