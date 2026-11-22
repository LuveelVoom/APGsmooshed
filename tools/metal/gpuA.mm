// Phase A GPU throughput test: many independent N x N torus universes, one
// threadgroup per universe, one thread per row, all generations in threadgroup
// memory. Kernels: generic 512-entry LUT (any rule, B0 excluded) or
// hand-written bitsliced B3/S23. GPU results are compared bit-exact with a
// CPU reference (same torus, same table).
//
// Conventions: cell (x, y), x grows right, y grows down. Row y is NW words of
// WB bits; cell x is bit (x % WB) of word (x / WB). Table index is Golly MAP
// order: NW 256, N 128, NE 64, W 32, C 16, E 8, SW 4, S 2, SE 1 (y-1 = north).
//
// usage: gpuA TABLE lut|life WB N UNIVERSES GENS CHUNK VERIFY [DUMPFILE]
#import <Metal/Metal.h>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

static const char *kernel_src = R"MSL(
#include <metal_stdlib>
using namespace metal;
#define NW (N / WB)
#define BIT(arr, x) ((uint)((arr[(x) / WB] >> ((x) % WB)) & 1))

constant uint tab[16] = { TABLE };

kernel void run(device WT *state [[buffer(0)]], constant uint &gens [[buffer(1)]],
                uint tg [[threadgroup_position_in_grid]],
                uint y [[thread_position_in_threadgroup]]) {
    threadgroup WT g[N * NW];
    device WT *u = state + tg * (N * NW);
    WT r[NW];
    for (int w = 0; w < NW; w++) { r[w] = u[y * NW + w]; g[y * NW + w] = r[w]; }
    threadgroup_barrier(mem_flags::mem_threadgroup);
    const uint ya = (y + N - 1) % N, yb = (y + 1) % N;
    for (uint gen = 0; gen < gens; gen++) {
        WT a[NW], d[NW], nr[NW];
        for (int w = 0; w < NW; w++) { a[w] = g[ya * NW + w]; d[w] = g[yb * NW + w]; nr[w] = 0; }
#ifdef LUT
        uint u3 = (BIT(a, N - 1) << 2) | (BIT(a, 0) << 1) | BIT(a, 1);
        uint m3 = (BIT(r, N - 1) << 2) | (BIT(r, 0) << 1) | BIT(r, 1);
        uint d3 = (BIT(d, N - 1) << 2) | (BIT(d, 0) << 1) | BIT(d, 1);
        #pragma clang loop unroll(full)
        for (int x = 0; x < N; x++) {
            uint idx = (u3 << 6) | (m3 << 3) | d3;
            nr[x / WB] |= ((WT) ((tab[idx >> 5] >> (idx & 31)) & 1)) << (x % WB);
            const int x2 = (x + 2) % N;
            u3 = ((u3 << 1) & 7) | BIT(a, x2);
            m3 = ((m3 << 1) & 7) | BIT(r, x2);
            d3 = ((d3 << 1) & 7) | BIT(d, x2);
        }
#else
        #pragma clang loop unroll(full)
        for (int w = 0; w < NW; w++) {
            const int wl = (w + NW - 1) % NW, wr = (w + 1) % NW;
            // bit x of ?l = cell x-1, bit x of ?r = cell x+1
            WT al = (a[w] << 1) | (a[wl] >> (WB - 1)), ar = (a[w] >> 1) | (a[wr] << (WB - 1));
            WT ml = (r[w] << 1) | (r[wl] >> (WB - 1)), mr = (r[w] >> 1) | (r[wr] << (WB - 1));
            WT dl = (d[w] << 1) | (d[wl] >> (WB - 1)), dr = (d[w] >> 1) | (d[wr] << (WB - 1));
            WT am = a[w], dm = d[w];
            // full adders on top and bottom triples, half adder on the middle pair
            WT t1 = al ^ am, sa = t1 ^ ar, ca = (al & am) | (t1 & ar);
            WT t2 = dl ^ dm, sc = t2 ^ dr, cc = (dl & dm) | (t2 & dr);
            WT sb = ml ^ mr, cb = ml & mr;
            WT t3 = sa ^ sb, s0 = t3 ^ sc, k = (sa & sb) | (t3 & sc);
            // twos: ca, cb, cc, k -> bit1 = parity, ge2 = at least two (count >= 4)
            WT p1 = ca ^ cb, p2 = cc ^ k;
            WT bit1 = p1 ^ p2;
            WT ge2 = (ca & cb) | (cc & k) | (p1 & p2);
            nr[w] = bit1 & ~ge2 & (s0 | r[w]);
        }
#endif
        threadgroup_barrier(mem_flags::mem_threadgroup);
        for (int w = 0; w < NW; w++) { g[y * NW + w] = nr[w]; r[w] = nr[w]; }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }
    for (int w = 0; w < NW; w++) u[y * NW + w] = r[w];
}
)MSL";

typedef std::vector<uint8_t> Grid;  // N*N cells, index y*N + x

static void cpu_step(const Grid &a, Grid &b, int N, const uint8_t *t) {
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            int idx = 0;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++)
                    idx = (idx << 1) | a[((y + dy + N) % N) * N + (x + dx + N) % N];
            b[y * N + x] = t[idx];
        }
}

int main(int argc, char **argv) {
    if (argc < 9) { fprintf(stderr, "usage: gpuA TABLE lut|life WB N UNIVERSES GENS CHUNK VERIFY [DUMP]\n"); return 1; }
    std::string tabfile = argv[1], kind = argv[2];
    int WB = atoi(argv[3]), N = atoi(argv[4]), U = atoi(argv[5]), G = atoi(argv[6]);
    int CHUNK = atoi(argv[7]), V = atoi(argv[8]);
    const char *dump = argc > 9 ? argv[9] : nullptr;
    if ((WB != 32 && WB != 64) || N % WB || N > 1024 || (kind != "lut" && kind != "life")) { fprintf(stderr, "bad args\n"); return 1; }
    if (V > U) V = U;
    std::string ts;
    std::ifstream(tabfile) >> ts;
    if (ts.size() != 512) { fprintf(stderr, "table must be 512 chars\n"); return 1; }
    uint8_t t[512];
    uint32_t tw[16] = {0};
    for (int i = 0; i < 512; i++) { t[i] = ts[i] == '1'; tw[i >> 5] |= (uint32_t) t[i] << (i & 31); }

    std::string src = "#define WB " + std::to_string(WB) + "\n#define N " + std::to_string(N) +
                      "\n#define WT " + (WB == 64 ? "ulong" : "uint") + "\n" + (kind == "lut" ? "#define LUT 1\n" : "");
    src += "#define TABLE ";
    for (int i = 0; i < 16; i++) src += std::to_string(tw[i]) + "u" + (i < 15 ? "," : "\n");
    src += kernel_src;

    int NW = N / WB;
    size_t words = (size_t) U * N * NW;
    // Initial state: 16x16 random soup (density 1/2) at the centre of each universe.
    std::vector<Grid> init(V);
    uint64_t s = 0x9e3779b97f4a7c15ull;
    auto rnd = [&]() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return s; };

    @autoreleasepool {
        id<MTLDevice> dev = MTLCreateSystemDefaultDevice();
        NSError *err = nil;
        auto c0 = std::chrono::steady_clock::now();
        id<MTLLibrary> lib = [dev newLibraryWithSource:[NSString stringWithUTF8String:src.c_str()] options:nil error:&err];
        if (!lib) { fprintf(stderr, "compile failed: %s\n", [[err description] UTF8String]); return 1; }
        id<MTLComputePipelineState> ps = [dev newComputePipelineStateWithFunction:[lib newFunctionWithName:@"run"] error:&err];
        if (!ps) { fprintf(stderr, "pipeline failed: %s\n", [[err description] UTF8String]); return 1; }
        double ct = std::chrono::duration<double>(std::chrono::steady_clock::now() - c0).count();
        if ((int) ps.maxTotalThreadsPerThreadgroup < N) { fprintf(stderr, "threadgroup too large (%lu)\n", (unsigned long) ps.maxTotalThreadsPerThreadgroup); return 1; }

        id<MTLBuffer> buf = [dev newBufferWithLength:words * (WB / 8) options:MTLResourceStorageModeShared];
        uint8_t *base = (uint8_t *) [buf contents];
        memset(base, 0, words * (WB / 8));
        auto setcell = [&](int uni, int x, int y) {
            size_t wi = ((size_t) uni * N + y) * NW + x / WB;
            if (WB == 64) ((uint64_t *) base)[wi] |= 1ull << (x % 64);
            else ((uint32_t *) base)[wi] |= 1u << (x % 32);
        };
        auto getcell = [&](int uni, int x, int y) -> int {
            size_t wi = ((size_t) uni * N + y) * NW + x / WB;
            if (WB == 64) return (((uint64_t *) base)[wi] >> (x % 64)) & 1;
            return (((uint32_t *) base)[wi] >> (x % 32)) & 1;
        };
        int off = N / 2 - 8;
        for (int i = 0; i < U; i++) {
            if (i < V) init[i].assign(N * N, 0);
            for (int y = 0; y < 16; y++) {
                uint64_t row = rnd();
                for (int x = 0; x < 16; x++)
                    if ((row >> x) & 1) { setcell(i, off + x, off + y); if (i < V) init[i][(off + y) * N + off + x] = 1; }
            }
        }

        id<MTLCommandQueue> q = [dev newCommandQueue];
        auto t0 = std::chrono::steady_clock::now();
        for (int done = 0; done < G;) {
            uint32_t g = (uint32_t) std::min(CHUNK, G - done);
            id<MTLCommandBuffer> cb = [q commandBuffer];
            id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];
            [enc setComputePipelineState:ps];
            [enc setBuffer:buf offset:0 atIndex:0];
            [enc setBytes:&g length:4 atIndex:1];
            [enc dispatchThreadgroups:MTLSizeMake(U, 1, 1) threadsPerThreadgroup:MTLSizeMake(N, 1, 1)];
            [enc endEncoding];
            [cb commit];
            [cb waitUntilCompleted];
            if (cb.error) { fprintf(stderr, "GPU error: %s\n", [[cb.error description] UTF8String]); return 1; }
            done += g;
        }
        double gt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        double cg = (double) U * N * N * G;
        printf("%s WB=%d N=%d U=%d G=%d: compile %.2f s, run %.3f s, %.3g cell-gens/s, %.3g universe-gens/s, %.0f soups/s at 1000 gens\n",
               kind.c_str(), WB, N, U, G, ct, gt, cg / gt, (double) U * G / gt, (double) U * G / gt / 1000);

        // CPU reference on the first V universes.
        int bad = 0;
        for (int i = 0; i < V; i++) {
            Grid a = init[i], b(N * N);
            for (int k = 0; k < G; k++) { cpu_step(a, b, N, t); a.swap(b); }
            for (int y = 0; y < N && !bad; y++)
                for (int x = 0; x < N; x++)
                    if (a[y * N + x] != getcell(i, x, y)) { printf("MISMATCH universe %d at x=%d y=%d\n", i, x, y); bad = 1; break; }
            if (dump && i == 0) {
                FILE *f = fopen(dump, "w");
                for (int y = 0; y < N; y++) { for (int x = 0; x < N; x++) fputc('0' + init[0][y * N + x], f); fputc('\n', f); }
                fputc('\n', f);
                for (int y = 0; y < N; y++) { for (int x = 0; x < N; x++) fputc('0' + a[y * N + x], f); fputc('\n', f); }
                fclose(f);
            }
        }
        if (V) printf("verify: %d universes vs CPU reference: %s\n", V, bad ? "FAIL" : "ok");
        return bad;
    }
}
