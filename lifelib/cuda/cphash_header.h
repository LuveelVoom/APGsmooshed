
    // Initialise memory:
    uint32_cu hashnum = blockIdx.x + offset;
    uint32_cu uniidx = univec[blockIdx.x];
    uint64_cu b = 0x4000600000ull + hashnum;

    /*
    Memory map:
    0x0000 -- 0x0003: header (lowest 19 bits = index; middle 2 bits = usize; upper 11 bits = gencount / 6)
    0x0004 -- 0x01ff: flags for each of 127 tiles
    0x0200 -- 0xffff: 127 tiles
    */

    int usize = (multiverse[uniidx << 13] >> 19) & 3;
    if (initial) { usize = 3; }

    __syncthreads();

    uint32_cu threadnum = (uniidx << 13) + threadIdx.x;

    if (threadIdx.x) { b = 0; }

    multiverse[threadnum] = b; b = 0;

