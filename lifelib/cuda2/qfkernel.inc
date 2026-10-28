
__global__ void KERNEL_NAME(const bpattern *things, uint32_t a_start, uint32_t a_end, uint32_t b_start, uint32_t b_end, uint32_t *sols, uint32_t flags) {

    __shared__ uint32_t cds[512];

    cds[threadIdx.x] = ((const uint32_t*) things)[threadIdx.x];
    cds[threadIdx.x + 256] = ((const uint32_t*) things)[threadIdx.x + 256];

    __syncthreads();

    uint32_t idx = ((threadIdx.x + blockIdx.x * blockDim.x) >> 5) + a_start;
    if (idx >= a_end) { return; }

    uint4 ax = load_bpattern(things + idx);
    uint32_t lid = threadIdx.x & 31;
    uint32_t elg = 0;

    #ifdef ENVCHECK
    uint4 border;
    border.x = cds[lid + 256];
    border.y = cds[lid + 288];
    border.z = cds[lid + 320];
    border.w = cds[lid + 352];
    #define RUN_GRID bx = advance_torus(bx); env.x |= bx.x; env.y |= bx.y; env.z |= bx.z; env.w |= bx.w; elg += 1
    #define EXIT_CHECK(T, O1, O2) if (hh::ballot_32(((bx.T & (cats.T | cds[lid + O1])) ^ (cats.T | cds[lid + O2])) | (border.T & env.T)) != 0) { continue; }
    #else
    #define RUN_GRID bx = advance_torus(bx); elg += 1
    #define EXIT_CHECK(T, O1, O2) if (hh::ballot_32((bx.T & (cats.T | cds[lid + O1])) ^ (cats.T | cds[lid + O2])) != 0) { continue; }
    #endif

    for (uint32_t jdx = b_start; jdx < b_end; jdx++) {

        uint4 bx = load_bpattern(things + jdx);

        // Skip if reactants overlap:
        {
            #define DISJOINT_CHECK(T, O1) if (hh::ballot_32((ax.T & bx.T) ^ cds[lid + O1])) { continue; }
            DISJOINT_CHECK(x, 384)
            DISJOINT_CHECK(y, 416)
            DISJOINT_CHECK(z, 448)
            DISJOINT_CHECK(w, 480)
            #undef DISJOINT_CHECK
        }

        // Load catalysts:
        uint4 cats; cats.x = 0; cats.y = 0; cats.z = 0; cats.w = 0;
        if (flags & 0x2000000) {
            cats.x |= (ax.x &~ bx.x);
            cats.y |= (ax.y &~ bx.y);
            cats.z |= (ax.z &~ bx.z);
            cats.w |= (ax.w &~ bx.w);
        }
        if (flags & 0x1000000) {
            cats.x |= (bx.x &~ ax.x);
            cats.y |= (bx.y &~ ax.y);
            cats.z |= (bx.z &~ ax.z);
            cats.w |= (bx.w &~ ax.w);
        }

        bx.x |= ax.x;
        bx.y |= ax.y;
        bx.z |= ax.z;
        bx.w |= ax.w;

        #ifdef ENVCHECK
        uint4 env = bx;
        #endif

        {
            for (uint32_t i = 0; i < (flags & 3); i++) {
                RUN_GRID;
            }

            bool stabilised = false;

            for (uint32_t i = 0; i < (flags & 0xfff); i += 4) {
                uint4 x = bx;
                RUN_GRID; RUN_GRID; RUN_GRID; RUN_GRID;
                uint32_t xordiff = (bx.x ^ x.x) | (bx.y ^ x.y) | (bx.z ^ x.z) | (bx.w ^ x.w);
                if (hh::ballot_32(xordiff) == 0) {
                    stabilised = true;
                    break;
                }
                #ifdef ENVCHECK
                uint32_t overlap = (env.x & border.x) | (env.y & border.y) | (env.z & border.z) | (env.w & border.w);
                if (hh::ballot_32(overlap) != 0) { break; }
                #endif
            }

            if (stabilised && (flags & 0x4000000)) { continue; }
        }

        EXIT_CHECK(x, 0, 128)
        EXIT_CHECK(y, 32, 160)
        EXIT_CHECK(z, 64, 192)
        EXIT_CHECK(w, 96, 224)

        {
            uint4 x = bx;
            for (uint32_t i = 0; i < (flags & 0xfff000); i += 4096) {
                RUN_GRID;
            }
            if (flags & 0x8000000) {
                uint32_t xordiff = (bx.x ^ x.x) | (bx.y ^ x.y) | (bx.z ^ x.z) | (bx.w ^ x.w);
                if (hh::ballot_32(xordiff) == 0) { continue; }
            }
        }

        EXIT_CHECK(x, 0, 128)
        EXIT_CHECK(y, 32, 160)
        EXIT_CHECK(z, 64, 192)
        EXIT_CHECK(w, 96, 224)

        #undef EXIT_CHECK
        #undef RUN_GRID

        // solution found!
        if (lid == 0) {
            uint32_t k = hh::atomic_add(sols + (2 << LOG2_MAX_SOLS), 1) & ((1 << LOG2_MAX_SOLS) - 1);
            sols[k*2] = idx;
            sols[k*2+1] = jdx;
        }
    }

    if (lid == 0) {
        hh::atomic_add(sols + (2 << LOG2_MAX_SOLS) + 1, elg & 255);
        hh::atomic_add(sols + (2 << LOG2_MAX_SOLS) + 2, elg >> 8);
    }
}

