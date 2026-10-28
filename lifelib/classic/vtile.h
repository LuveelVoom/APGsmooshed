
    const static uint8_t __gdirections[] __attribute__((aligned(64))) = {0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 4,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0};

    template<int H, int K>
    inline void _copyBoundary3(uint32_t * __restrict__ d, uint32_t * __restrict__ n_d) {
        constexpr static uint32_t middle = ((1u << (32 - K)) - (1u << K)); // e.g. 0x3ffffffcu
        constexpr static uint32_t left   =                   - (1u << K) ; // e.g. 0xfffffffcu
        for (int i = K; i < H + K; i++) {
            d[i] = ((n_d[i] & middle) >> (32 - 2*K)) | (d[i] & left);
        }
    }

    template<int H, int K>
    inline void _copyBoundary0(uint32_t * __restrict__ d, uint32_t * __restrict__ n_d) {
        constexpr static uint32_t middle = ((1u << (32 - K)) - (1u << K)); // e.g. 0x3ffffffcu
        constexpr static uint32_t right  = ((1u << (32 - K)) -        1u); // e.g. 0x3fffffffu
        for (int i = K; i < H + K; i++) {
            d[i] = ((n_d[i] & middle) << (32 - 2*K)) | (d[i] & right);
        }
    }

    template<int H, int K>
    inline void _copyBoundary12(uint32_t * __restrict__ d, uint32_t * __restrict__ n1_d, uint32_t * __restrict__ n2_d) {
        for (int i = 0; i < K; i++) {
            d[i] = ((n1_d[H+i] << (16 - K)) & 0xffff0000u) | ((n2_d[H+i] >> (16 - K)) & 0x0000ffffu);
        }
    }

    template<int H, int K>
    inline void _copyBoundary1(uint32_t * __restrict__ d, uint32_t * __restrict__ n_d) {
        for (int i = 0; i < K; i++) {
            d[i] = ((n_d[H+i] << (16 - K)) & 0xffff0000u) | (d[i] & 0x0000ffffu);
        }
    }

    template<int H, int K>
    inline void _copyBoundary2(uint32_t * __restrict__ d, uint32_t * __restrict__ n_d) {
        for (int i = 0; i < K; i++) {
            d[i] = ((n_d[H+i] >> (16 - K)) & 0x0000ffffu) | (d[i] & 0xffff0000u);
        }
    }

    template<int H, int K>
    inline void _copyBoundary45(uint32_t * __restrict__ d, uint32_t * __restrict__ n4_d, uint32_t * __restrict__ n5_d) {
        for (int i = K; i < 2*K; i++) {
            d[H+i] = ((n5_d[i] << (16 - K)) & 0xffff0000u) | ((n4_d[i] >> (16 - K)) & 0x0000ffffu);
        }
    }

    template<int H, int K>
    inline void _copyBoundary4(uint32_t * __restrict__ d, uint32_t * __restrict__ n_d) {
        for (int i = K; i < 2*K; i++) {
            d[H+i] = ((n_d[i] >> (16 - K)) & 0x0000ffffu) | (d[H+i] & 0xffff0000u);
        }
    }

    template<int H, int K>
    inline void _copyBoundary5(uint32_t * __restrict__ d, uint32_t * __restrict__ n_d) {
        for (int i = K; i < 2*K; i++) {
            d[H+i] = ((n_d[i] << (16 - K)) & 0xffff0000u) | (d[H+i] & 0x0000ffffu);
        }
    }

    template<int H, int K = 2>
    struct VTile {

        uint64_t coords; // 8 bytes

        uint32_t hash; // 4 bytes
        int16_t population; // 2 bytes
        uint8_t updateflags; // 1 byte
        uint8_t currentflags; // 1 byte

        VTile<H,K> *neighbours[6]; // 48 bytes
        uint32_t d[H + 2*K]; // 128 bytes
        uint32_t hist[H + 2*K]; // 128 bytes

        constexpr static uint32_t middle = ((1u << (32 - K)) - (1u << K)); // e.g. 0x3ffffffcu

        bool nonempty(uint64_t z) {
            uint32_t* q = z ? hist : d;
            for (int i = K; i < H + K; i++) {
                if (q[i] & middle) { return true; }
            }
            return false;
        }

        void clearHistory() {
            std::memset(hist, 0, 4 * (H+2*K));
        }

        void copyBoundary12(VTile<H,K> *n1, VTile<H,K> *n2) __attribute__((always_inline)) {
            _copyBoundary12<H, K>(d, n1->d, n2->d);
        }

        void copyBoundary1(VTile<H,K> *n) __attribute__((always_inline)) {
            _copyBoundary1<H, K>(d, n->d);
        }

        void copyBoundary2(VTile<H,K> *n) __attribute__((always_inline)) {
            _copyBoundary2<H, K>(d, n->d);
        }

        void copyBoundary45(VTile<H,K> *n4, VTile<H,K> *n5) __attribute__((always_inline)) {
            _copyBoundary45<H, K>(d, n4->d, n5->d);
        }

        void copyBoundary4(VTile<H,K> *n) __attribute__((always_inline)) {
            _copyBoundary4<H, K>(d, n->d);
        }

        void copyBoundary5(VTile<H,K> *n) __attribute__((always_inline)) {
            _copyBoundary5<H, K>(d, n->d);
        }

        #ifdef __AVX2__
        void copyBoundary3(VTile<H,K> *n);
        void copyBoundary0(VTile<H,K> *n);
        #else
        void copyBoundary3(VTile<H,K> *n) {
            _copyBoundary3<H, K>(d, n->d);
        }

        void copyBoundary0(VTile<H,K> *n) {
            _copyBoundary0<H, K>(d, n->d);
        }
        #endif

        inline void updateTile(upattern<VTile<H,K>, 32 - 2*K, H>* owner, int rule, int family, int mantissa) __attribute__((always_inline));
        // ^^^ we really do need both the prefix 'inline' and the attribute 'always_inline' for this to work ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

        uint32_t hashTile() {
            if (!(currentflags & 2)) { return hash; }
            uint32_t partialhash = 0;

            for (int i = K; i < H + K; i++) {
                partialhash = partialhash * (partialhash + 77232917) + i * (d[i] & middle);
            }

            hash = partialhash;
            currentflags &= (~2);
            return partialhash;
        }

        inline static int countpop64(const uint64_t *y) __attribute__((always_inline)) {

            int pop = 0;

            constexpr uint64_t middlemiddle = ((uint64_t) middle) * 0x100000001ull;

            // We include two popcounts in the loop so that the number of
            // iterations is halved (in the case of H = 44, from 22 to 11).
            // This knocks it below the threshold such that gcc will unroll
            // the loop (completely).
            for (int i = 0; i < H / 2; i += 2) {
                pop += __builtin_popcountll(y[i] & middlemiddle);
                pop += __builtin_popcountll(y[i+1] & middlemiddle);
            }

            return pop;

        }

        inline static bool unsafeBoundary(const uint64_t *y) {

            int X = (H + 2*K) >> 3;
            uint64_t z = 0;
            for (int i = 0; i < X; i++) { z |= (y[i] | y[H/2 + K - 1 - i]); }
            if (z) { return true; }
            for (int i = X; i < H/2 + K - X; i += 2) { z |= (y[i] | y[i+1]); }
            return (z & 0xff0000ffff0000ffull);
        }

        int countPopulation(upattern<VTile<H,K>, 32 - 2*K, H>* owner) {

            // Check memoized value:
            if (!(currentflags & 1)) { return population; }

            owner->population -= population;
            // Casting to 64-bit values so we can halve the number of calls
            // to the POPCNT instruction. If H = 44, for example, we only
            // need to perform 22 copies of the instruction.
            population = countpop64((uint64_t *) (d + K));
            owner->population += population;
            currentflags &= (~1);

            if (population != 5) { return population; }
            int ext = owner->is_extremal(coords);
            if ((ext == 0) || unsafeBoundary((uint64_t *) d)) { return population; }

            int i = 0;
            for (i = 8; i < H + 2*K - 8; i++) {
                if (d[i]) { break; }
            }

            uint32_t shadow = d[i] | d[i+1] | d[i+2];

            int tzeroes = __builtin_ctz(shadow);
            if ((shadow >> tzeroes) != 7) { return population; }

            uint32_t signature = (d[i] >> tzeroes) | (d[i+1] >> (tzeroes - 3)) | (d[i+2] >> (tzeroes - 6));

            if (ext & __gdirections[signature]) {
                // remove glider:
                d[i] = 0; d[i+1] = 0; d[i+2] = 0;
                population = 0;
                owner->glider_count += 1;
            }

            return population;
        }

        bitworld to_bitworld(int z) {
            uint32_t* q = (z ? hist : d);
            bitworld bw;
            uint32_t e[H + 2*K] = {0};
            for (uint64_t i = 0; i < H; i++) {
                e[i] = (q[i + K] & middle) >> K;
            }
            uint64_t f[4];
            for (uint64_t j = 0; j < ((H + 2*K) / 8); j++) {
                int bis = best_instruction_set();
                if (bis >= 9) {
                    twofifths_avx(e + (8*j), f);
                } else {
                    twofifths_sse2(e + (8*j), f);
                }
                for (uint64_t i = 0; i < 4; i++) {
                    if (f[i]) { bw.world.emplace(std::pair<int32_t, int32_t>(i, j), f[i]); }
                }
            }
            return bw;
        }

        void eu64(upattern<VTile<H,K>, 32 - 2*K, H>* owner, int z, uint8_t dx, uint8_t dy, uint64_t v) {

            if ((v == 0) || (z >= 2)) { return; }

            if (!(currentflags & 1)) { owner->popchanged.push_back(this); }
            currentflags = 3;
            if (updateflags == 0) { owner->modified.push_back(this); }
            updateflags |= 64;

            uint32_t* q = (z ? hist : d);

            if ((dx < 8) || (dx > 16) || (dy < 8) || (dy > H + 2*K - 16)) {
                for (int i = 0; i < 6; i++) { owner->updateNeighbour(this, i); }
                if (dy > H - 8) {
                    owner->getNeighbour(this, 4)->eu64(owner, z, dx + (16 - K), 0, v >> (8 * (H - dy)));
                }
                if (dx >= (32 - 2*K)) {
                    owner->getNeighbour(this, 0)->eu64(owner, z, dx - (32 - 2*K), dy, v);
                } else if (dx > (24 - 2*K)) {
                    uint64_t bitmask = 0x0101010101010101ull;
                    bitmask = (bitmask << (dx - (24 - 2*K))) - bitmask;
                    owner->getNeighbour(this, 0)->eu64(owner, z, 0, dy, (v >> ((32 - 2*K) - dx)) & bitmask);
                }
            }

            if (dx < (32 - 2*K)) {
                for (uint64_t i = 0; i < 8; i++) {
                    uint64_t newy = i + dy;
                    if (newy < H) {
                        q[newy + K] |= ((((v >> (8 * i)) & 255) << (dx + K)) & middle);
                    }
                }
            }
        }

    };
