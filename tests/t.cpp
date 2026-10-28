#include "lifelib/avxlife/lifeperm.h"
#include "lifelib/avxlife/lifelogic/iterators_b3-qr4k5cek6cs23-a4er56aek.h"
#include <cstdio>
#include <cstring>
static uint64_t s = 88172645463325252ull;
static uint64_t rnd() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return s; }
static uint64_t h = 1469598103934665603ull;
static void mix(const void* p, size_t n) { const uint8_t* q = (const uint8_t*) p; for (size_t i = 0; i < n; i++) { h ^= q[i]; h *= 1099511628211ull; } }
int main() {
    uint64_t H[8] = {0};
    for (int it = 0; it < 200000; it++) {
        uint64_t a[32], o[32]; uint32_t r[64];
        for (int i = 0; i < 32; i++) a[i] = (it % 7 == 0) ? (rnd() & rnd() & rnd()) : rnd();
        uint32_t* a32 = (uint32_t*) a;
#define T(k, stmt, out, n) h = H[k]; memset(o, 0xA5, sizeof o); memset(r, 0x5A, sizeof r); stmt; mix(out, n); H[k] = h;
        T(0, apg::transpose_bytes_sse2(a, o), o, 64)
        T(1, apg::twofifths_sse2(a32, o), o, 32)
        T(2, apg::z64_to_r32_sse2(a, r), r, 128)
        T(3, apg::z64_to_r32_centre_ssse3(a, r), r, 128)
        T(4, apg::r32_centre_to_z64_ssse3(a32, o), o, 32)
        T(5, apg::r32_centre_to_z64_sse4(a32, o), o, 32)
        T(6, b3_qr4k5cek6cs23_a4er56aek::iterate_var_leaf(a, o), o, 32)
    }
    for (int k = 0; k < 7; k++) printf("%d %016llx\n", k, (unsigned long long) H[k]);
}
