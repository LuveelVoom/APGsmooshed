#pragma once
#include <stdint.h>

namespace apg {

    // Disjoint-set data structure:
    class dsds {
        uint64_t *parents;
        uint64_t *ranks;
        uint64_t setcount;

        public:

        // Initialise a disjoint-set data structure of N elements:
        dsds(uint64_t N) {
            setcount = N;
            parents = new uint64_t[N];
            ranks = new uint64_t[N];
            for (uint64_t i = 0; i < N; i++) {
                parents[i] = i;
                ranks[i] = 0;
            }
        }

        // Destructor:
        ~dsds() { delete[] parents; delete[] ranks; }

        // Return the root of the tree containing p:
        uint64_t find(uint64_t p) {
            uint64_t root = p;
            while (root != parents[root]) { root = parents[root]; }
            while (p != root) { uint64_t newp = parents[p]; parents[p] = root; p = newp; }
            return root;
        }

        // Replace sets containing x and y with their union:
        void merge(uint64_t x, uint64_t y) {
            uint64_t i = find(x);
            uint64_t j = find(y);
            if (i == j) { return; }
            if (ranks[i] < ranks[j]) {
                parents[i] = j;
            } else {
                parents[j] = i;
                if (ranks[i] == ranks[j]) { ranks[i] += 1; }
            }
            setcount -= 1;
        }

        // Are objects x and y in the same set?
        bool connected(uint64_t x, uint64_t y) { return find(x) == find(y); }

        // Return the number of disjoint sets:
        uint64_t count() { return setcount; }
    };
}
