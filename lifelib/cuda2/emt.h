
#ifndef SKIP_RESIDUE
#define SKIP_RESIDUE 1
#endif

#define PERTASK(t) for (int t = threadIdx.x >> 5; t < totalTasks; t += warpsPerBlock)

#ifdef ULTIMATE_EMT
#define FUNCTION_NAME exhaustMultipleTilesUltimate
#else
#define FUNCTION_NAME exhaustMultipleTiles
#endif

template<int N, int M = N, int K = 128>
__global__ void FUNCTION_NAME (int tilesToLoad, uint32_cu *interesting, uint64_cu *topology, uint4 *output, int maxgen) {

#undef FUNCTION_NAME

    // pointer to this universe:
    uint4* uverse = output + (((uint64_t) (blockIdx.x)) << 12);

    __shared__ uint32_cu smem[128 * N];
    __shared__ uint64_cu s_topology[K];
    __shared__ uint32_cu taskset[K];
    __shared__ int taskinfo[2];

    // Load generation count into shared memory:
    if (threadIdx.x == 0) {
        smem[0] = interesting[blockIdx.x] << 8;
        taskinfo[1] = 0;
    }

    __syncthreads();

    int gencount = smem[0] >> 8;
    if ((gencount == 0) || (gencount >= maxgen)) {
        // soup has already been resolved:
        return;
    }

    // Load topology into shared memory:
    if (threadIdx.x < K) {
        s_topology[threadIdx.x] = topology[threadIdx.x];
    }

    // Load universe into shared memory:
    for (int i = threadIdx.x; i < 32 * M; i += blockDim.x) {
        uint4 a; a.x = 0; a.y = 0; a.z = 0; a.w = 0;

        #ifdef ULTIMATE_EMT
        if (i >= 32 * N) {
            uverse[i] = a;
        } else
        #endif
        {
            if (i < 32 * tilesToLoad) {
                a = uverse[i];
            }
            if (i > 0) { smem[4*i] = a.x; }
            smem[4*i+1] = a.y;
            smem[4*i+2] = a.z;
            smem[4*i+3] = a.w;
        }
    }

    int laneId = threadIdx.x & 31;

    while (gencount < maxgen) {

        if (threadIdx.x == 0) { taskinfo[0] = 0; }

        __syncthreads();

        // determine tiles to update
        if (threadIdx.x < K) {

            uint32_cu theseflags = smem[threadIdx.x];
            uint64_cu t = s_topology[threadIdx.x];

            #ifdef ULTIMATE_EMT
            if (63 & theseflags & (t >> 48)) {
                // perturbation has escaped the universe
                taskinfo[1] = 1;
            }
            #endif

            uint32_cu b = theseflags & 64;
            b |= (smem[t         & 127] & 8);
            b |= (smem[(t >> 8)  & 127] & 16);
            b |= (smem[(t >> 16) & 127] & 32);
            b |= (smem[(t >> 24) & 127] & 1);
            b |= (smem[(t >> 32) & 127] & 2);
            b |= (smem[(t >> 40) & 127] & 4);

            uint32_t update_mask = hh::ballot_32(b != 0);

            int inclusiveTaskCount = __popc(update_mask & (0xffffffffu >> (31 - laneId)));

            {
                int offset = 0;
                if (laneId == 31) {
                    offset = atomicAdd(taskinfo, inclusiveTaskCount);
                }
                offset = hh::shuffle_32(offset, 31);
                inclusiveTaskCount += offset;
            }

            if (b != 0) {
                // append to task set
                taskset[inclusiveTaskCount - 1] = (b << 8) | (threadIdx.x);

                if (threadIdx.x >= M) {
                    // perturbation has escaped the universe
                    taskinfo[1] = 1;
                }
            }
        }

        __syncthreads();

        int totalTasks = taskinfo[0];

        // Early exit:
        if (taskinfo[1]) { break; }
        if (totalTasks == 0) {
            // universe has stabilised
            if (threadIdx.x == 0) { interesting[blockIdx.x] = 0; }
            return;
        }

        int warpsPerBlock = blockDim.x >> 5;

        #ifndef SKIP_RESIDUE
        for (int residue = 0; residue < 3; residue += 1)
        #endif
        {

        // boundary-copying loop
        PERTASK(taskId) {

            uint32_cu task = taskset[taskId];
            uint64_cu t = s_topology[task & 127];

            if ((task & (63 << 8)) == 0) {
                // nothing to do
                continue;
            }

            #ifndef SKIP_RESIDUE
            if ((t >> 62) != residue) {
                continue;
            }
            #endif

            int offset = ((task & 127) << 7) + (laneId << 2);

            uint4 a;

            #ifdef ULTIMATE_EMT
            auto gmem = (uint32_t*) uverse;
            if ((task & 127) >= N) {
                a = uverse[offset >> 2];
            } else
            #endif
            {
                a.x = smem[offset];
                a.y = smem[offset + 1];
                a.z = smem[offset + 2];
                a.w = smem[offset + 3];
            }

            if (task & (8 << 8)) {
                // copy from east neighbour
                if ((laneId >= 3) && (laneId < 29)) {
                    int offset2 = ((t & 127) << 7) + (laneId << 2);

                    #ifdef ULTIMATE_EMT
                    if (offset2 >= (N << 7)) {
                        a.y = (a.y & 0x03ffffff) | ((gmem[offset2    ] << 20) & 0xfc000000);
                        a.w = (a.w & 0x03ffffff) | ((gmem[offset2 + 2] << 20) & 0xfc000000);
                    } else
                    #endif
                    {
                        a.y = (a.y & 0x03ffffff) | ((smem[offset2    ] << 20) & 0xfc000000);
                        a.w = (a.w & 0x03ffffff) | ((smem[offset2 + 2] << 20) & 0xfc000000);
                    }
                }
            }

            if (task & (16 << 8)) {
                // copy from north-east neighbour
                if (laneId < 3) {
                    int offset2 = (((t >> 8) & 127) << 7) + ((laneId + 26) << 2);

                    #ifdef ULTIMATE_EMT
                    if (offset2 >= (N << 7)) {
                        a.y = (gmem[offset2    ] >> 6) | (gmem[offset2 + 1] << 26);
                        a.w = (gmem[offset2 + 2] >> 6) | (gmem[offset2 + 3] << 26);
                    } else
                    #endif
                    {
                        a.y = (smem[offset2    ] >> 6) | (smem[offset2 + 1] << 26);
                        a.w = (smem[offset2 + 2] >> 6) | (smem[offset2 + 3] << 26);
                    }
                }
            }

            if (task & (32 << 8)) {
                // copy from north-west neighbour
                if (laneId < 3) {
                    int offset2 = (((t >> 16) & 127) << 7) + ((laneId + 26) << 2);

                    #ifdef ULTIMATE_EMT
                    if (offset2 >= (N << 7)) {
                        a.x = (gmem[offset2    ] >> 26) | (gmem[offset2 + 1] << 6);
                        a.z = (gmem[offset2 + 2] >> 26) | (gmem[offset2 + 3] << 6);
                    } else
                    #endif
                    {
                        a.x = (smem[offset2    ] >> 26) | (smem[offset2 + 1] << 6);
                        a.z = (smem[offset2 + 2] >> 26) | (smem[offset2 + 3] << 6);
                    }
                }
            }

            if (task & (1 << 8)) {
                // copy from west neighbour
                if ((laneId >= 3) && (laneId < 29)) {
                    int offset2 = (((t >> 24) & 127) << 7) + (laneId << 2);

                    #ifdef ULTIMATE_EMT
                    if (offset2 >= (N << 7)) {
                        a.x = (a.x & 0xffffffc0) | ((gmem[offset2 + 1] >> 20) & 0x0000003f);
                        a.z = (a.z & 0xffffffc0) | ((gmem[offset2 + 3] >> 20) & 0x0000003f);
                    } else
                    #endif
                    {
                        a.x = (a.x & 0xffffffc0) | ((smem[offset2 + 1] >> 20) & 0x0000003f);
                        a.z = (a.z & 0xffffffc0) | ((smem[offset2 + 3] >> 20) & 0x0000003f);
                    }
                }
            }

            if (task & (2 << 8)) {
                // copy from south-west neighbour
                if (laneId >= 29) {
                    int offset2 = (((t >> 32) & 127) << 7) + ((laneId - 26) << 2);

                    #ifdef ULTIMATE_EMT
                    if (offset2 >= (N << 7)) {
                        a.x = (gmem[offset2    ] >> 26) | (gmem[offset2 + 1] << 6);
                        a.z = (gmem[offset2 + 2] >> 26) | (gmem[offset2 + 3] << 6);
                    } else
                    #endif
                    {
                        a.x = (smem[offset2    ] >> 26) | (smem[offset2 + 1] << 6);
                        a.z = (smem[offset2 + 2] >> 26) | (smem[offset2 + 3] << 6);
                    }
                }
            }

            if (task & (4 << 8)) {
                // copy from south-east neighbour
                if (laneId >= 29) {
                    int offset2 = (((t >> 40) & 127) << 7) + ((laneId - 26) << 2);

                    #ifdef ULTIMATE_EMT
                    if (offset2 >= (N << 7)) {
                        a.y = (gmem[offset2    ] >> 6) | (gmem[offset2 + 1] << 26);
                        a.w = (gmem[offset2 + 2] >> 6) | (gmem[offset2 + 3] << 26);
                    } else
                    #endif
                    {
                        a.y = (smem[offset2    ] >> 6) | (smem[offset2 + 1] << 26);
                        a.w = (smem[offset2 + 2] >> 6) | (smem[offset2 + 3] << 26);
                    }
                }
            }

            #ifdef ULTIMATE_EMT
            if ((task & 127) >= N) {
                uverse[offset >> 2] = a;
            } else
            #endif
            {
                smem[offset] = a.x;
                smem[offset + 1] = a.y;
                smem[offset + 2] = a.z;
                smem[offset + 3] = a.w;
            }
        }

        __syncthreads();

        }

        // tile-running loop
        PERTASK(taskId) {

            uint32_cu task = taskset[taskId];

            int offset = ((task & 127) << 7) + (laneId << 2);

            uint4 a;
            #ifdef ULTIMATE_EMT
            if ((task & 127) >= N) {
                a = uverse[offset >> 2];
            } else
            #endif
            {
                a.x = smem[offset];
                a.y = smem[offset + 1];
                a.z = smem[offset + 2];
                a.w = smem[offset + 3];
            }

            int tileidx = 36;

            if ((task & 127) >= tilesToLoad) {
                uint64_cu t = s_topology[task & 127];
                tileidx = (int) (t >> 56);
            }

            int rh = (tileidx & 7) + 60;
            int rv = ((tileidx >> 3) & 7) + 60;

            // run tile for 6 generations:
            int flags = advance_tile_inplace(a, rh, rv);

            if (laneId == 0) { smem[task & 127] = flags; }

            #ifdef ULTIMATE_EMT
            if ((task & 127) >= N) {
                uverse[offset >> 2] = a;
            } else
            #endif
            {
                smem[offset] = a.x;
                smem[offset + 1] = a.y;
                smem[offset + 2] = a.z;
                smem[offset + 3] = a.w;
            }
        }

        gencount += 6;
    }

    // Soup is interesting. If we are in the ultimate kernel, we do not
    // need to save anything to global memory so can terminate early.
    // Otherwise, we need to save the universe and generation count.

    #ifndef ULTIMATE_EMT
    __syncthreads();

    // Save universe into global memory:
    for (int i = threadIdx.x; i < 32 * N; i += blockDim.x) {
        uint4 a;
        a.x = smem[4*i];
        a.y = smem[4*i+1];
        a.z = smem[4*i+2];
        a.w = smem[4*i+3];

        uverse[i] = a;
    }

    // Save generation count into global memory:
    if (threadIdx.x == 0) { interesting[blockIdx.x] = gencount; }
    #endif

}

#undef PERTASK
