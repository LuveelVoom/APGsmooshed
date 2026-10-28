#include "qufince.h"
#include "cudagol.h"
#include <chrono>

#define LOG2_MAX_SOLS 16

namespace apg {

#define KERNEL_NAME collision_kernel
#include "qfkernel.inc"
#undef KERNEL_NAME
#define ENVCHECK 1
#define KERNEL_NAME collision_kernel_envcheck
#include "qfkernel.inc"
#undef KERNEL_NAME
#undef ENVCHECK


__global__ void disjoint_kernel(const bpattern *lefts, const bpattern *rights, uint32_t nleft, uint32_t nright, uint32_t *output, uint32_t ngens) {

    uint32_t rmask = 0;
    uint32_t outloc = (blockIdx.x * blockDim.x + threadIdx.x) >> 5;

    for (uint32_t j = 0; j < 32; j++) {

        uint32_t cidx = 32 * outloc + j;

        if (cidx >= nleft * nright) {
            continue;
        }

        uint32_t il = cidx % nleft;
        uint32_t ir = cidx / nleft;

        uint4 a = load_bpattern(lefts + il);
        uint4 b = load_bpattern(rights + ir);

        if (hh::ballot_32((a.x & b.x) | (a.y & b.y) | (a.z & b.z) | (a.w & b.w))) {
            continue;
        }

        uint4 c;
        c.x = a.x ^ b.x;
        c.y = a.y ^ b.y;
        c.z = a.z ^ b.z;
        c.w = a.w ^ b.w;

        for (uint32_t i = 0; i < ngens; i++) {
            a = advance_torus(a);
            b = advance_torus(b);
            c = advance_torus(c);
        }

        if (hh::ballot_32((a.x & b.x) | (a.y & b.y) | (a.z & b.z) | (a.w & b.w))) {
            continue;
        }

        if (hh::ballot_32((a.x ^ b.x ^ c.x) | (a.y ^ b.y ^ c.y) | (a.z ^ b.z ^ c.z) | (a.w ^ b.w ^ c.w))) {
            continue;
        }

        rmask |= (1u << j);
    }

    if ((threadIdx.x & 31) == 0) { output[outloc] = rmask; }

}


std::vector<bpattern> disjoint_product(const std::vector<bpattern> &a, const std::vector<bpattern> &b, int ngens) {

    uint64_t psize = a.size() * b.size();
    uint32_t nblocks = (psize + 255) >> 8;
    uint64_t rmem = nblocks * 32ull;

    bpattern* a_device;
    bpattern* b_device;
    uint32_t* r_device;
    uint32_t* r_host;

    cudaMalloc(&a_device, a.size() * sizeof(bpattern));
    cudaMalloc(&b_device, b.size() * sizeof(bpattern));
    cudaMalloc(&r_device,    rmem); 
    cudaMallocHost(&r_host,  rmem); 
    memset(r_host, 0,        rmem);

    cudaMemcpy(a_device, &(a[0]), a.size() * sizeof(bpattern), cudaMemcpyHostToDevice);
    cudaMemcpy(b_device, &(b[0]), b.size() * sizeof(bpattern), cudaMemcpyHostToDevice);
    cudaMemcpy(r_device, r_host, rmem, cudaMemcpyHostToDevice);

    disjoint_kernel<<<nblocks, 256>>>(a_device, b_device, a.size(), b.size(), r_device, ngens);

    cudaMemcpy(r_host, r_device, rmem, cudaMemcpyDeviceToHost);

    std::vector<bpattern> res;

    uint32_t cidx = 0;

    for (uint32_t ir = 0; ir < b.size(); ir++) {
        for (uint32_t il = 0; il < a.size(); il++) {
            if ((r_host[cidx >> 5] >> (cidx & 31)) & 1) {
                bpattern c;
                for (int k = 0; k < 64; k++) {
                    c.x[k] = a[il].x[k] | b[ir].x[k];
                }
                res.push_back(c);
            }
            cidx += 1;
        }
    }

    cudaFreeHost(r_host);
    cudaFree(r_device);
    cudaFree(b_device);
    cudaFree(a_device);

    auto res2 = get_unique_bpatterns(res);

    std::cerr << a.size() << " x " << b.size() << " = " << psize << " --> " << res.size() << " --> " << res2.size() << std::endl;
    return res2;
}


uint64_t print_memory_statistics() {

    size_t free_mem = 0;
    size_t total_mem = 0;

    if (hh::reportCudaError(cudaMemGetInfo(&free_mem, &total_mem))) { return 0; }

    std::cerr << "Memory statistics: " << free_mem << " free; " << total_mem << " total." << std::endl;
    
    return free_mem;
}


void run_collision_kernel(const bpattern& initial, const bpattern& and_mask, const bpattern& target, const bpattern& border, const std::vector<bpattern> &a, const std::vector<bpattern> &b, uint32_t flags, uint32_t chunksize) {

    int border_popc = 0;
    for (int k = 0; k < 64; k++) { border_popc += hh::popc64(border.x[k]); }
    std::cerr << "Field size: " << (4096 - border_popc) << std::endl;

    bpattern* patterns_device;
    bpattern* patterns_host;

    size_t ii = 4; // offset into array
    size_t n_patterns = ii + a.size() + b.size();

    if (hh::reportCudaError(cudaMallocHost(&patterns_host, n_patterns * sizeof(bpattern)))) { return; }
    if (hh::reportCudaError(cudaMalloc(&patterns_device,   n_patterns * sizeof(bpattern)))) { return; }

    memset(patterns_host, 0, n_patterns * sizeof(bpattern));

    {
        // Do an interesting permutation:
        patterns_host[0] = and_mask;
        patterns_host[1] = target;
        patterns_host[2] = border;
        patterns_host[3] = initial;

        uint32_t cd1[512];
        memcpy(cd1, patterns_host, 2048);
        uint32_t cd2[512];
        for (uint32_t i = 0; i < 512; i++) {
            cd2[i] = cd1[(i & 384) | ((i & 31) << 2) | ((i & 96) >> 5)];
        }
        memcpy(patterns_host, cd2, 2048);
    }

    uint32_t a_start = ii;
    for (auto x : a) { patterns_host[ii++] = x; }
    uint32_t a_end = ii;
    for (auto x : b) { patterns_host[ii++] = x; }

    cudaMemcpy(patterns_device, patterns_host, n_patterns * sizeof(bpattern), cudaMemcpyHostToDevice);

    uint32_t sols_bytes = 256 + (8 << LOG2_MAX_SOLS);

    uint32_t* sols_device;
    uint32_t* sols_host;
    if (hh::reportCudaError(cudaMallocHost(&sols_host, sols_bytes))) { return; }
    if (hh::reportCudaError(cudaMalloc(&sols_device, sols_bytes))) { return; }
    memset(sols_host, 0, sols_bytes);
    cudaMemcpy(sols_device, sols_host, sols_bytes, cudaMemcpyHostToDevice);

    uint32_t blocksize = 256;

    uint32_t* miscs = sols_host + (2 << LOG2_MAX_SOLS);

    std::cerr << "\n\033[32;1m***** Launching primary GPU kernel *****\033[0m\n" << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (uint32_t bo = 0; bo < b.size(); bo += chunksize) {

        uint32_t b_start = a_end + bo;
        uint32_t b_end = a_end + hh::min(bo + chunksize, ((uint32_t) b.size()));

        uint32_t n_blocks = a.size() / (blocksize >> 5) + 1;
        uint64_t workload = ((uint64_t) (a_end - a_start)) * ((uint64_t) (b_end - b_start));

        auto before = std::chrono::high_resolution_clock::now();
        if (border_popc == 0) {
            collision_kernel<<<n_blocks, blocksize>>>(patterns_device, a_start, a_end, b_start, b_end, sols_device, flags);
        } else {
            collision_kernel_envcheck<<<n_blocks, blocksize>>>(patterns_device, a_start, a_end, b_start, b_end, sols_device, flags);
        }

        uint32_t sols_before = miscs[0];
        uint32_t ctr0 = miscs[1];
        uint32_t ctr1 = miscs[2];

        cudaMemcpy(sols_host, sols_device, sols_bytes, cudaMemcpyDeviceToHost);
        uint32_t n_sols = miscs[0] - sols_before;
        ctr0 = miscs[1] - ctr0;
        ctr1 = miscs[2] - ctr1;

        uint64_t iters = ((uint64_t) 256) * ctr1 + ctr0;

        if (n_sols > 0) {
            std::cerr << "# " << n_sols << " solutions found." << std::endl;
            if (n_sols > (1 << LOG2_MAX_SOLS)) { n_sols = 1 << LOG2_MAX_SOLS; }
            for (uint32_t i = 0; i < n_sols; i++) {
                uint32_t so = (i + sols_before) & ((1 << LOG2_MAX_SOLS) - 1);
                uint32_t idx = sols_host[so*2];
                uint32_t jdx = sols_host[so*2+1];

                apg::bpattern result;

                for (int k = 0; k < 64; k++) {
                    result.x[k] = patterns_host[idx].x[k] | patterns_host[jdx].x[k];
                }

                print_bpattern(result);
            }
        }

        auto after = std::chrono::high_resolution_clock::now();
        uint64_t microseconds = std::chrono::duration_cast<std::chrono::microseconds>(after - before).count();
        uint64_t total_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(after - start_time).count();
        uint64_t elapsed_bs = b_end - a_end;
        int rem_seconds = ((int) (1.0e-6 * ((double) total_microseconds) * ((double) (b.size() - elapsed_bs)) / ((double) elapsed_bs)));

        // Print current progress and speed:
        std::cerr << "# progress: " << elapsed_bs << "/" << b.size();
        std::cerr << "; speed = " << (iters / microseconds) << "M iters/sec = " << (1000 * workload / microseconds) << "K colls/sec";

        // Print estimated remaining time:
        std::cerr << "; remaining time = ";
        if (rem_seconds >= 31536000) { std::cerr << (rem_seconds / 31536000) << "y "; }
        if (rem_seconds >= 86400) { std::cerr << ((rem_seconds / 86400) % 365) << "d "; }
        if (rem_seconds >= 3600) { std::cerr << ((rem_seconds / 3600) % 24) << "h "; }
        if (rem_seconds >= 60) { std::cerr << ((rem_seconds / 60) % 60) << "m "; }
        std::cerr << (rem_seconds % 60) << "s" << std::endl;
    }

    cudaFreeHost(sols_host);
    cudaFree(sols_device);
    cudaFreeHost(patterns_host);
    cudaFree(patterns_device);

}

}
