#pragma once
#include "core.hpp"

namespace hh {

template<typename T, typename T2>
struct Montgomery {

    T n;
    T nr;
    T one;
    T r2;

    constexpr static int bitwidth = 8 * sizeof(T);
    constexpr static int log2bitwidth = constexpr_log2(bitwidth);

    Montgomery(T n) : n(n) {
        nr = 1;
        for (int i = 0; i < log2bitwidth; i++) { nr *= 2 - n * nr; }

        // compute (r/2) modulo n:
        r2 = ((T) 1) << (bitwidth - 1);
        r2 = r2 % n;

        // compute 2r modulo n, i.e. the Montgomery representation of 2:
        one = add(r2, r2);
        r2 = add(one, one);

        // compute Montgomery representation of r with repeated squaring:
        for (int i = 0; i < log2bitwidth; i++) { r2 = multiply(r2, r2); }
    }

    T sub(T x, T y) const {
        T res = x - y;
        if (res > x) { res += n; }
        return res;
    }

    T add(T x, T y) const {
        T res = x + y;
        if ((res >= n) || (res < x)) { res -= n; }
        return res;
    }

    T reduce(T2 x) const {
        T q = ((T) x) * nr;
        T m = (((T2) q) * n) >> bitwidth;
        T res = x >> bitwidth;
        return sub(res, m);
    }

    T multiply(T x, T y) const {
        return reduce(((T2) x) * y);
    }

    T transform(T x) const {
        return multiply(x, r2);
    }

    T detransform(T x) const {
        return reduce(x);
    }

    T modexp(T base, T exponent) const {
        if (exponent == 0) { return one; }
        T t = base;
        T w = exponent;
        while ((w & 1) == 0) {
            w >>= 1;
            t = multiply(t, t);
        }
        T z = t; w >>= 1;
        while (w) {
            t = multiply(t, t);
            if (w & 1) { z = multiply(z, t); }
            w >>= 1;
        }
        return z;
    }

    bool strong_probable_prime(T base) const {
        T e = n - 1;
        while ((e & 1) == 0) { e >>= 1; }
        T t = modexp(transform(base), e);
        if (t == one) { return true; }
        T minus_one = n - one;
        while (e < n - 1) {
            if (t == minus_one) { return true; }
            t = multiply(t, t);
            if (t == one) { return false; }
            e <<= 1;
        }
        return false;
    }
};

} // namespace hh
