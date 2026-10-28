

    // N = total number of layers;
    // M = number of non-passive layers:
    template<int N, int M>
    struct UTile {

        uint64_t a[4 * N]; // 32N bytes
        uint64_t b[4 * N]; // 32N bytes
        uint64_t c[4 * N]; // 32N bytes
        uint64_t d[4 * N]; // 32N bytes
        UTile<N, M> *neighbours[6]; // 48 bytes

        // We store both coordinates in a single uint64_t, because why not?
        // Provided we don't get anywhere near 2^32 away from the origin,
        // the implementation details won't leak. This means we don't have
        // to provide a (slow) std::hash<int, int> implementation in order
        // to reap the benefits of an unordered_map; moreover, manipulating
        // a uint64_t should be faster than a pair of ints.
        uint64_t coords; // 8 bytes

        uint64_t hash; // 8 bytes

        int32_t population; // 4 bytes
        uint16_t updateflags; // 2 bytes
        bool populationOld; // 1 byte
        bool hashCurrent; // 1 byte

        // sizeof(UTile<N, M>) == 128N + 72 bytes (no alignment space wasted)

        void clearHistory() {
            std::memset(a + (4 * M), 0, 32 * (N - M));
            std::memset(b + (4 * M), 0, 32 * (N - M));
            std::memset(c + (4 * M), 0, 32 * (N - M));
            std::memset(d + (4 * M), 0, 32 * (N - M));
        }

        void copyBoundary0(UTile<N, M>* n) {
            for (int i = 0; i < M; i++) {
                b[3 + 4*i] = n->a[3 + 4*i];
                d[1 + 4*i] = n->c[1 + 4*i];
            }
        }
        void copyBoundary1(UTile<N, M>* n) {
            for (int i = 0; i < M; i++) {
                b[0 + 4*i] = n->c[1 + 4*i];
                b[1 + 4*i] = n->d[0 + 4*i];
            }
        }
        void copyBoundary2(UTile<N, M>* n) {
            for (int i = 0; i < M; i++) {
                a[0 + 4*i] = n->c[1 + 4*i];
                a[1 + 4*i] = n->d[0 + 4*i];
            }
        }
        void copyBoundary3(UTile<N, M>* n) {
            for (int i = 0; i < M; i++) {
                a[2 + 4*i] = n->b[2 + 4*i];
                c[0 + 4*i] = n->d[0 + 4*i];
            }
        }
        void copyBoundary4(UTile<N, M>* n) {
            for (int i = 0; i < M; i++) {
                c[2 + 4*i] = n->a[3 + 4*i];
                c[3 + 4*i] = n->b[2 + 4*i];
            }
        }
        void copyBoundary5(UTile<N, M>* n) {
            for (int i = 0; i < M; i++) {
                d[2 + 4*i] = n->a[3 + 4*i];
                d[3 + 4*i] = n->b[2 + 4*i];
            }
        }

        void copyBoundary12(UTile<N, M>* n1, UTile<N, M>* n2) { copyBoundary1(n1); copyBoundary2(n2); }
        void copyBoundary45(UTile<N, M>* n4, UTile<N, M>* n5) { copyBoundary4(n4); copyBoundary5(n5); }

        uint64_t hashTile() {

            if (hashCurrent) { return hash; }
            uint64_t partialhash = 0;

            for (int i = 0; i < M; i++) {
                partialhash = partialhash * (partialhash +447840759955ull);
                partialhash += (a[3 + 4*i] ^ (1093 * b[2 + 4*i]));
                partialhash += ((641 * d[4*i]) ^ (3511 * c[1 + 4*i]));
            }

            hash = partialhash;
            hashCurrent = true;
            return partialhash;
        }

        void updateTile(upattern<UTile<N, M>, 16>* owner, int rule, int family, uint64_t mantissa) {
            uint64_t outleafx[4*N] = {0ull};
            uint64_t* inleafxs[4] = {a, b, c, d};
            int r = universal_leaf_iterator<N>(rule, family, mantissa, inleafxs, outleafx);
            if (r != 1) {
                uint64_t diff[4] = {0ull};
                for (int i = 0; i < M; i++) {
                    diff[3] |= (outleafx[0 + 4*i] ^ a[3 + 4*i]);
                    diff[2] |= (outleafx[1 + 4*i] ^ b[2 + 4*i]);
                    diff[1] |= (outleafx[2 + 4*i] ^ c[1 + 4*i]);
                    diff[0] |= (outleafx[3 + 4*i] ^ d[0 + 4*i]);
                }
                if (diff[0] | diff[1] | diff[2] | diff[3]) {
                    if (!populationOld) { owner->popchanged.push_back(this); }
                    populationOld = true;
                    hashCurrent = false;
                    if (updateflags == 0) { owner->modified.push_back(this); }
                    updateflags |= 64;
                }
                if (diff[0] | diff[2]) {
                    owner->updateNeighbour(this, 0);
                }
                if (diff[2] | diff[3]) {
                    owner->updateNeighbour(this, 1);
                    owner->updateNeighbour(this, 2);
                }
                if (diff[3] | diff[1]) {
                    owner->updateNeighbour(this, 3);
                }
                if (diff[1] | diff[0]) {
                    owner->updateNeighbour(this, 4);
                    owner->updateNeighbour(this, 5);
                }
            }
            for (int i = 0; i < N; i++) {
                a[3 + 4*i] = outleafx[0 + 4*i];
                b[2 + 4*i] = outleafx[1 + 4*i];
                c[1 + 4*i] = outleafx[2 + 4*i];
                d[0 + 4*i] = outleafx[3 + 4*i];
            }
        }

        bool nonempty(uint64_t i) {
            return (a[4*i+3] | b[4*i+2] | c[4*i+1] | d[4*i]);
        }

        int countPopulation(upattern<UTile<N, M>, 16>* owner) {
            if (!populationOld) { return population; }
            owner->population -= population;
            int pop = 0;
            uint64_t diff[4] = {0ull};
            for (int i = 0; i < M; i++) {
                diff[3] |= a[3 + 4*i];
                diff[2] |= b[2 + 4*i];
                diff[1] |= c[1 + 4*i];
                diff[0] |= d[0 + 4*i];
            }
            for (int i = 0; i < 4; i++) {
                pop += __builtin_popcountll(diff[i]);
            }
            owner->population += pop;
            population = pop;
            populationOld = false;
            return pop;
        }

        bitworld to_bitworld(int z) {
            bitworld bw;
            if (a[4*z+3]) { bw.world.emplace(std::pair<int32_t, int32_t>(0, 0), a[4*z+3]); }
            if (b[4*z+2]) { bw.world.emplace(std::pair<int32_t, int32_t>(1, 0), b[4*z+2]); }
            if (c[4*z+1]) { bw.world.emplace(std::pair<int32_t, int32_t>(0, 1), c[4*z+1]); }
            if (d[4*z+0]) { bw.world.emplace(std::pair<int32_t, int32_t>(1, 1), d[4*z+0]); }
            return bw;
        }

        void eu64(upattern<UTile<N, M>, 16>* owner, int z, uint8_t dx, uint8_t dy, uint64_t v) {

            for (int i = 0; i < 6; i++) { owner->updateNeighbour(this, i); }
            if (!populationOld) { owner->popchanged.push_back(this); }
            populationOld = true;
            if (updateflags == 0) { owner->modified.push_back(this); }
            updateflags |= 64;

            uint8_t dz = (dx / 8) + (dy / 8) * 2;

            if (dz == 0) { a[4*z+3] = v; }
            if (dz == 1) { b[4*z+2] = v; }
            if (dz == 2) { c[4*z+1] = v; }
            if (dz == 3) { d[4*z+0] = v; }

        }

    };
