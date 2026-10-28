#pragma once

#include "iterators.h"
#include "copyhashes.h"
#include "cumsum.h"

__global__ void schedule(uint32_cu *multiverse, uint64_cu *topology, uint32_cu *interesting, uint32_cu *to_restore, uint64_cu *tilemasks) {
    /*
    * Determine which tiles in a universe are to be updated, and also
    * check for termination (stabilisation or 'crashing'). This should
    * be run at the beginning of the main loop.
    */

    __shared__ uint32_cu tmp[128];
    __shared__ uint32_cu tmp2[128];

    uint32_cu a = multiverse[(blockIdx.x << 14) + threadIdx.x];
    uint32_cu am = a & ((threadIdx.x == 0) ? 0xfff80000u : 0xffffffffu);
    tmp[threadIdx.x] = am;

    __syncthreads();

    uint32_cu tmp0 = tmp[0];

    if (tmp0 <= 0x1fffff) {
        // dead universe
        if (threadIdx.x == 0) { tilemasks[blockIdx.x] = 0; }
        return;
    }

    uint32_cu b = am & 64;

    uint64_cu t = topology[threadIdx.x];

    b |= (tmp[t         & 127] & 8);
    b |= (tmp[(t >> 8)  & 127] & 16);
    b |= (tmp[(t >> 16) & 127] & 32);
    b |= (tmp[(t >> 24) & 127] & 1);
    b |= (tmp[(t >> 32) & 127] & 2);
    b |= (tmp[(t >> 40) & 127] & 4);

    // 31 = crashed;
    // 15 = size-3;
    //  7 = size-2;
    //  3 = size-1;
    //  1 = size-0;
    //  0 = stabilised

    uint32_cu expusize = (b == 0) ? 0 : ((2 << (t >> 62)) - 1);
    if (threadIdx.x == 0) { expusize = 0; }
    if (((t >> 48) & am & 63) || (tmp0 > 0xfa000000u)) { expusize = 31; }
    // if (((t >> 48) & am) || (tmp0 > 0x7d000000u)) { expusize = 31; }

    tmp2[threadIdx.x] = expusize;
    __syncthreads();
    expusize |= tmp2[threadIdx.x ^ 1];
    expusize |= tmp2[threadIdx.x ^ 2];
    expusize |= tmp2[threadIdx.x ^ 3];
    expusize |= tmp2[threadIdx.x ^ 4];
    expusize |= tmp2[threadIdx.x ^ 5];
    expusize |= tmp2[threadIdx.x ^ 6];
    expusize |= tmp2[threadIdx.x ^ 7];
    tmp[threadIdx.x] = expusize;
    __syncthreads();
    expusize |= tmp[threadIdx.x ^  8];
    expusize |= tmp[threadIdx.x ^ 16];
    expusize |= tmp[threadIdx.x ^ 24];
    tmp2[threadIdx.x] = expusize;
    __syncthreads();
    expusize |= tmp2[threadIdx.x ^ 32];
    expusize |= tmp2[threadIdx.x ^ 64];
    expusize |= tmp2[threadIdx.x ^ 96];

    if ((expusize == 0) || (expusize == 31)) {
        // Universe has terminated:
        if (threadIdx.x == 0) {
            multiverse[blockIdx.x << 14] = a & 0x001fffff;
            interesting[a & 0x0007ffff] = (expusize == 31);
            to_restore[blockIdx.x] = blockIdx.x | 65536;
            tilemasks[blockIdx.x] = 0;
        }
        return;
    }

    // Determine new universe size and generation count:
    expusize |= ((2 << ((tmp0 >> 19) & 3)) - 1);
    expusize = __popc(expusize) + 3;

    /* 
    expusize is now either 4, 5, 6, or 7; when we add this to the
    masked value of a, this will simultaneously:

        -- insert the universe size into bits 18 and 19;
        -- increment the generation count (starting at bit 20);

    to reflect that 6 generations have passed.
    */

    if (threadIdx.x == 0) { b = (a & 0xffe7ffffu) + (expusize << 19); }

    bool active = (b != 0);
    if (threadIdx.x != 0) {
        // Bits  0 .. 6:  tile within universe
        // Bits  7 .. 19: universe ID
        // Bits 20 .. 25: update flags
        // Bits 26 .. 31: direction code
        b = ((b & 63) << 20) | (blockIdx.x << 7) | threadIdx.x;
    }

    bool boundary = ((expusize & 3) == (t >> 62));

    if (boundary) {
        // Tile rank matches universe size, so candidate for escaping
        // glider/*WSS detection. We insert the six-bit direction code
        // into the upper six bits of the value b.
        b |= ((t >> 30) & 0xfc000000u);
    }

    // Partial stream compaction:

    a = FULL_BALLOT(active);

    if ((threadIdx.x & 31) == 0) { tmp[12 + (threadIdx.x >> 5)] = __popc(a); }

    a &= (0x92492492u >> (31 - (threadIdx.x & 31)));
    a = __popc(a);

    int res = ((threadIdx.x % 3) << 2);

    if ((threadIdx.x & 31) >= 29) { tmp[res + (threadIdx.x >> 5)] = a; }

    __syncthreads();

    int totaltiles = tmp[12] + tmp[13] + tmp[14] + tmp[15];

    if (threadIdx.x >= 32) { a += tmp[res]; }
    if (threadIdx.x >= 64) { a += tmp[res + 1]; }
    if (threadIdx.x >= 96) { a += tmp[res + 2]; }
    tmp2[threadIdx.x] = 0;

    if ((totaltiles == 2) && (threadIdx.x != 0) && ((b & 0xfc000000u) == 0)) {
        b |= 0xfc000000u;
    }

    __syncthreads();

    if (threadIdx.x >= 125) { tmp[res + 3] = a; }

    a += ((43 * res) >> 2);
    if (active) { tmp2[a - 1] = b; }

    /*
    Now the array tmp2 contains three rows of tiles compressed to the
    beginning of each block of 43 tiles. For instance, if there are:
        8 active tiles of index 0 mod 3,
        13 active tiles of index 1 mod 3, and
        7 active tiles of index 2 mod 3,
    then the block tmp2 will resemble the following, with each character
    below representing a uint32:

    h********..................................
    *************..............................
    *******...................................

    We synchronise threads and memcpy these 512 bytes into the initial
    segment of the universe, overwriting what previously existed there:
    */

    __syncthreads();
    multiverse[(blockIdx.x << 14) + threadIdx.x] = tmp2[threadIdx.x];

    /*
    We now collect the tile counts into tilemasks[blockIdx.x], packed
    as three 21-bit integers. So in our example, we would store:

    (7 << 42) + (13 << 21) + (8 << 0)

    into the array. An exclusive scan (parallel prefix sum) over the
    multiverse could be conducted; each 21-bit segment would not
    overflow since there are at most (43 << 12) = 176128 active tiles
    of each residue in the multiverse, whereas a 21-bit counter can
    safely reach 2097151 before it overflows.
    */

    uint64_cu xx =    tmp[11];
    xx = (xx << 21) + tmp[ 7];
    xx = (xx << 21) + tmp[ 3];

    // The '- 1' correction is so that we don't include the existence
    // of the header tile (tile 0) in the modulo-0 tile count:
    if (threadIdx.x == 0) { tilemasks[blockIdx.x] = xx - 1; }

}

__global__ void psc_universes(uint32_cu *to_restore, uint32_cu *psums) {

    uint32_cu a = to_restore[(blockIdx.x << 5) + threadIdx.x];

    uint32_cu b = FULL_BALLOT(a != 0);
    b &= (0xffffffffu >> (31 - (threadIdx.x & 31)));
    b = __popc(b);

    if (threadIdx.x == 31) { psums[blockIdx.x] = b; }

    if (a != 0) { to_restore[(blockIdx.x << 5) + b - 1] = a; }

}

__global__ void compact_universes(uint32_cu *to_restore, uint32_cu *psums, uint32_cu *psums2, uint32_cu *univec) {

    uint32_cu a = to_restore[(blockIdx.x << 5) + threadIdx.x];
    to_restore[(blockIdx.x << 5) + threadIdx.x] = 0;

    uint32_cu tlim = psums[blockIdx.x];
    uint32_cu goffset = psums2[blockIdx.x];

    if (threadIdx.x < tlim) { univec[goffset + threadIdx.x] = a & 65535; }

}

__global__ void compact_tiles(uint32_cu *multiverse, uint64_cu *tilemasks, uint64_cu *l1sums, uint64_cu *l2sums, uint32_cu *compacts, int logf) {
    // 4096 blocks; 128 threads

    uint32_cu a = multiverse[(blockIdx.x << 14) + threadIdx.x];

    if (threadIdx.x != 0) { multiverse[(blockIdx.x << 14) + threadIdx.x] = 0; }

    uint64_cu goffset = l1sums[blockIdx.x] + l2sums[blockIdx.x >> logf];
    uint64_cu rtotals = l2sums[64];
    uint64_cu tmask = tilemasks[blockIdx.x];

    uint32_cu res = (threadIdx.x * 3) >> 7; // equivalent (on the domain [0, 127]) to division by 43
    uint32_cu residx = threadIdx.x - (res * 43); // remainder modulo 43

    if (res == 0) { residx -= 1; } // ensure thread zero wraps around to 2^32 - 1

    uint32_cu tlim = (tmask   >> (res * 21)) & 0x1fffff;
    uint32_cu toff = (goffset >> (res * 21)) & 0x1fffff;

    if (res >= 1) { toff += (rtotals & 0x1fffff); }
    if (res >= 2) { toff += ((rtotals >> 21) & 0x1fffff); }

    if (residx < tlim) {
        compacts[residx + toff] = a;
    }

}

__global__ void combine(uint64_cu *multiverse2, uint32_cu *tilevec2, int count2,
                        uint64_cu *multiverse, uint32_cu *tilevec, int count1) {

    if (blockIdx.x < count2) {

    uint32_cu tileidx = tilevec2[blockIdx.x];

    uint32_cu memloc = threadIdx.x + ((tileidx & 0x000fffff) << 6);
    uint64_cu t = c_topology[tileidx & 127];

    uint64_cu a = multiverse2[memloc];

    uint32_cu uniloc = memloc & 0xffffe03fu;

    if (tileidx & (8 << 20)) {
        uint32_cu nloc = uniloc + ((t         & 127) << 6);
        if ((threadIdx.x >= 6) && (threadIdx.x < 58)) {
            a = (a & 0x03ffffffffffffffull) | ((multiverse2[nloc] << 52) & 0xfc00000000000000ull);
        }
    }

    if (tileidx & (16 << 20)) {
        uint32_cu nloc = uniloc + (((t >>  8) & 127) << 6) + 52;
        if (threadIdx.x < 6) {
            a = (a & 0x00000000ffffffffull) | ((multiverse2[nloc] << 26) & 0xffffffff00000000ull);
        }
    }

    if (tileidx & (32 << 20)) {
        uint32_cu nloc = uniloc + (((t >> 16) & 127) << 6) + 52;
        if (threadIdx.x < 6) {
            a = (a & 0xffffffff00000000ull) | ((multiverse2[nloc] >> 26) & 0x00000000ffffffffull);
        }
    }

    if (tileidx & (1 << 20)) {
        uint32_cu nloc = uniloc + (((t >> 24) & 127) << 6);
        if ((threadIdx.x >= 6) && (threadIdx.x < 58)) {
            a = (a & 0xffffffffffffffc0ull) | ((multiverse2[nloc] >> 52) & 0x000000000000003full);
        }
    }

    if (tileidx & (2 << 20)) {
        uint32_cu nloc = uniloc + (((t >> 32) & 127) << 6) - 52;
        if (threadIdx.x >= 58) {
            a = (a & 0xffffffff00000000ull) | ((multiverse2[nloc] >> 26) & 0x00000000ffffffffull);
        }
    }

    if (tileidx & (4 << 20)) {
        uint32_cu nloc = uniloc + (((t >> 40) & 127) << 6) - 52;
        if (threadIdx.x >= 58) {
            a = (a & 0x00000000ffffffffull) | ((multiverse2[nloc] << 26) & 0xffffffff00000000ull);
        }
    }

    multiverse2[memloc] = a;

    }

    if (blockIdx.x >= count1) { return; }

    uint32_cu tileidx = tilevec[blockIdx.x];

    uint32_cu memloc = threadIdx.x + ((tileidx & 0x000fffff) << 6);

    __shared__ uint64_cu tmp1[64];
    __shared__ uint64_cu tmp2[64];
    __shared__ uint64_cu diffs[2];

    uint32_cu u = (threadIdx.x +  1) & 63;
    uint32_cu d = (threadIdx.x + 63) & 63;
    uint64_cu a = multiverse[memloc];
    uint64_cu b;
    uint64_cu c;

    /*
    Run the universe for six generations. Ensure that we have:

    uint64_cu a = generation 0;
    uint64_cu c = generation 2;
    uint64_cu b = generation 6;

    so we can subsequently compare b and c to detect if there's an
    isolated removable period-4 spaceship in the tile.
    */

    int icount = 0;
    uint32_cu flags = 0;
    uint64_cu xordiff = 0;

    while ((flags & 191) == 0) {

    ADVANCE_TILE_64(a, b, tmp1, tmp2)
    ADVANCE_TILE_64(b, c, tmp1, tmp2)
    ADVANCE_TILE_64(c, b, tmp1, tmp2)
    ADVANCE_TILE_64(b, b, tmp1, tmp2)
    ADVANCE_TILE_64(b, b, tmp1, tmp2)
    ADVANCE_TILE_64(b, b, tmp1, tmp2)

    /*
    Mask off the 52-by-52 region 'owned' by the tile, and only
    write that back into global memory:
    */
    uint64_cu mask = ((threadIdx.x >= 6) && (threadIdx.x < 58)) ? 0x03ffffffffffffc0ull : 0ull;
    xordiff = (a ^ b) & mask;
    a ^= xordiff;

    // Compute diff:
    tmp2[threadIdx.x] = xordiff;
    __syncthreads();
    xordiff |= tmp2[threadIdx.x ^ 1];
    xordiff |= tmp2[threadIdx.x ^ 2];
    xordiff |= tmp2[threadIdx.x ^ 3];
    tmp1[threadIdx.x] = xordiff;
    __syncthreads();
    xordiff |= tmp1[threadIdx.x ^ 4];
    xordiff |= tmp1[threadIdx.x ^ 8];
    if (threadIdx.x ==  0) { diffs[0] = xordiff; }
    if (threadIdx.x == 63) { diffs[1] = xordiff; }
    xordiff |= tmp1[threadIdx.x ^ 12];
    tmp2[threadIdx.x] = xordiff;
    __syncthreads();
    xordiff |= tmp2[threadIdx.x ^ 16];
    xordiff |= tmp2[threadIdx.x ^ 32];
    xordiff |= tmp2[threadIdx.x ^ 48];

    // Update flags:

    flags = ((xordiff == 0) || ((tileidx >> 26) != 63) || (icount >= 10)) ? 192 : 64;

    flags |= ((xordiff  & 0x03f0000000000000ull) ? 1  : 0);
    flags |= ((diffs[0] & 0x03fffffffc000000ull) ? 2  : 0);
    flags |= ((diffs[0] & 0x0000003fffffffc0ull) ? 4  : 0);
    flags |= ((xordiff  & 0x0000000000000fc0ull) ? 8  : 0);
    flags |= ((diffs[1] & 0x0000003fffffffc0ull) ? 16 : 0);
    flags |= ((diffs[1] & 0x03fffffffc000000ull) ? 32 : 0);

    icount += 1;

    __syncthreads();

    }

    multiverse[memloc] = a;
    if (xordiff == 0) { return; }

    // Write back:
    uint32_cu* universe = (uint32_cu*) (multiverse + (memloc & 0xffffe000u));
    if (threadIdx.x == 0) { universe[tileidx & 127] = flags; }

    tileidx = tileidx >> 26; // Now contains information for glider detection

    if ((tileidx == 0) || (tileidx == 63)) { return; } // Central tiles skip glider detection

    uint32_t r_horizontal = ((tileidx & 7) + 60) & 63;
    uint32_t r_vertical   = ((tileidx >> 3) & 7) + 60;

    tmp1[(threadIdx.x + r_vertical) & 63] = ROTL64(b, r_horizontal);
    __syncthreads();
    c ^= tmp1[threadIdx.x];

    uint64_cu mask = ((threadIdx.x >= 12) && (threadIdx.x < 52)) ? 0xfff0000000000fffull : 0xffffffffffffffffull;
    c |= (b & mask);
    a = __popcll(b) + ((c != 0) ? 4096 : 0);

    tmp2[threadIdx.x] = a;
    __syncthreads();
    a += tmp2[threadIdx.x ^ 1];
    a += tmp2[threadIdx.x ^ 2];
    a += tmp2[threadIdx.x ^ 3];
    tmp1[threadIdx.x] = a;
    __syncthreads();
    a += tmp1[threadIdx.x ^ 4];
    a += tmp1[threadIdx.x ^ 8];
    a += tmp1[threadIdx.x ^ 12];
    tmp2[threadIdx.x] = a;
    __syncthreads();
    a += tmp2[threadIdx.x ^ 16];
    a += tmp2[threadIdx.x ^ 32];
    a += tmp2[threadIdx.x ^ 48];

    if ((a == 0) || (a > 18)) { return; }

    /*
    At this point, we know that the tile contains either a *WSS or
    between 1 and 3 gliders, pointing in the correct direction. We
    eliminate them from the universe:
    */

    multiverse[memloc] &= mask;

}

struct multiverse_container {

    uint64_cu* multiverse;
    uint32_cu* to_restore;
    uint64_cu* tilemasks;
    uint64_cu* l1sums;
    uint64_cu* l1totals;
    uint64_cu* l2sums;
    uint32_cu* compacts;
    uint32_cu* psums;
    uint32_cu* psums2;
    uint32_cu* univec;

    uint64_t* things;
    uint64_t unicount;

    std::string symmetry;

    void spin_up(int num_universes, std::string symstring) {

        symmetry = symstring;
        unicount = num_universes;

        cudaMalloc((void**) &multiverse, unicount * 65536);
        cudaMalloc((void**) &to_restore, unicount * 4);
        cudaMalloc((void**) &tilemasks, unicount * 8);
        cudaMalloc((void**) &l1sums, unicount * 8);
        cudaMalloc((void**) &l1totals, unicount * 8);
        cudaMalloc((void**) &l2sums, 66 * 8);
        cudaMalloc((void**) &compacts, unicount * 1536);
        cudaMalloc((void**) &psums, unicount / 8);
        cudaMalloc((void**) &psums2, unicount / 8);
        cudaMalloc((void**) &univec, unicount * 4);

        cudaMemset(to_restore, 0, unicount * 4);

        cudaMallocHost((void**) &things, 16);
    }

    void spin_up(int num_universes) { spin_up(num_universes, "C1"); }

    void tear_down() {

        cudaFree((void*) multiverse);
        cudaFree((void*) to_restore);
        cudaFree((void*) tilemasks);
        cudaFree((void*) l1sums);
        cudaFree((void*) l1totals);
        cudaFree((void*) l2sums);
        cudaFree((void*) compacts);
        cudaFree((void*) psums);
        cudaFree((void*) psums2);
        cudaFree((void*) univec);

        cudaFreeHost((void*) things);
    }

    uint64_t mainloop(uint64_cu *topology, uint32_cu *interesting, uint64_cu *hashes, uint32_t &offset, uint32_t epoch_size,
                        int other_count, multiverse_container &other, bool completed) {

        uint64_t universes_left = 0;
        int count012 = 0;

        if (!completed) {

            schedule<<<unicount, 128>>>((uint32_cu*) multiverse, topology, interesting, to_restore, tilemasks);

            int logf;

            if (unicount == 8192) {
                logf = 7;
                exclusive_scan_uint64_128<<<64, 128>>>(tilemasks, l1sums, l1totals);
            } else if (unicount == 4096) {
                logf = 6;
                exclusive_scan_uint64<<<64, 64>>>(tilemasks, l1sums, l1totals);
            }
            exclusive_scan_uint64<<<1, 64>>>(l1totals, l2sums, l2sums + 64);

            compact_tiles<<<unicount, 128>>>((uint32_cu*) multiverse, tilemasks, l1sums, l2sums, compacts, logf);

            psc_universes<<<(unicount / 32), 32>>>(to_restore, psums);

            if (unicount == 8192) {
                exclusive_scan_uint32_256<<<1, 256>>>(psums, psums2, l2sums + 65);
            } else if (unicount == 4096) {
                exclusive_scan_uint32<<<1, 128>>>(psums, psums2, l2sums + 65);
            }
            compact_universes<<<(unicount / 32), 32>>>(to_restore, psums, psums2, univec);

            cudaMemcpy(things, l2sums + 64, 16, cudaMemcpyDeviceToHost); // 16 bytes = 2 uint64s

            uint64_t tiles_per_residue = things[0];
            universes_left = things[1];

            int count0 = tiles_per_residue & 0x1fffff;
            int count1 = (tiles_per_residue >> 21) & 0x1fffff;
            int count2 = (tiles_per_residue >> 42) & 0x1fffff;

            count012 = count0 + count1 + count2;

        }

        int n_kernels = (count012 < other_count) ? other_count : count012;

        if (n_kernels > 0) {
            combine<<<n_kernels, 64>>>(multiverse, compacts, count012, other.multiverse, other.compacts, other_count);
        }

        if (!completed) {
            if (universes_left > epoch_size - offset) {
                universes_left = epoch_size - offset;
            }

            copyhashes(symmetry, universes_left, multiverse, univec, hashes, offset, false);

            offset += universes_left;
        }

        return count012;
    }

};
