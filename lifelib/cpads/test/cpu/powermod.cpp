#include <cpads/primality.hpp>
#include <iostream>
#include <vector>
#include <atomic>
#include <thread>


struct TowerModder {

    std::vector<uint32_t> cache;
    std::vector<uint32_t> candidates;

    uint64_t towermod(uint64_t n, bool use_cache = true) const {

        if (n <= 2) { return 0; }

        int k = hh::ctz64(n);
        uint64_t m = n >> k;
        uint64_t res;

        if ((use_cache) && ((m >> 1) < cache.size())) {
            res = cache[m >> 1];
        } else {
            uint64_t phi = hh::euler_phi(m);
            uint64_t exponent = towermod(phi);
            hh::Montgomery<uint64_t, hh::u128> mont(m);
            res = mont.detransform(mont.modexp(mont.transform(2), exponent));
        }

        for (int i = 0; i < k; i++) {
            res += m * (res & (1ull << i));
        }

        return res;
    }

    TowerModder(uint32_t maxn) : cache(maxn >> 1) {
        for (uint32_t i = 1; i < maxn; i += 2) {
            cache[i >> 1] = towermod(i, false);
        }

        for (uint32_t p = 1; p < 570570; p += 6) {
            if ((p%5) && (p%7) && (p%11) && (p%13) && (p%19)) {
                candidates.push_back(p);
            }
        }
    }

    void exhaust(uint64_t batch) const {

        for (auto&& x : candidates) {
            uint64_t p = batch * 570570 + x;

            uint64_t target = p - 3;

            if (p % 17 == 0) { continue; }
            if (p % 23 == 0) { continue; }
            if (p % 29 == 0) { continue; }
            if (p % 31 == 0) { continue; }
            if (p % 37 == 0) { continue; }
            if (p % 41 == 0) { continue; }
            if (p % 43 == 0) { continue; }
            if (p % 47 == 0) { continue; }
            if (p % 53 == 0) { continue; }
            if (p % 59 == 0) { continue; }

            if (p == 1) { continue; }

            hh::Montgomery<uint64_t, hh::u128> mont(p);

            // filter out things for which -3 cannot be a residue:
            {
                uint64_t exponent = (p - 1) >> hh::ctz64(p - 1);
                uint64_t t = mont.modexp(mont.transform(target), exponent);
                if (t != mont.one) { continue; }
            }

            // compute under the assumption that p is prime (probably true!):
            {
                uint64_t exponent = towermod(p - 1);
                uint64_t res = mont.detransform(mont.modexp(mont.transform(2), exponent));
                if (res != target) { continue; }
            }

            // filter out strong pseudoprimes:
            if (!hh::isprime(p)) { continue; }

            std::cout << p << std::endl;
        }
    }

    void exhaust2(uint64_t superbatch) const {

        for (uint64_t i = 0; i < 1753; i++) {
            exhaust(superbatch * 1753 + i);
        }

        std::cout << "Superbatch " << superbatch << " completed." << std::endl;
    }
};


void run_worker(std::atomic<uint32_t> *ctr, uint32_t n_tasks, const TowerModder* tm) {
    for (;;) {
        // dequeue subtask:
        uint32_t idx = (uint32_t) ((*ctr)++);
        if (idx >= n_tasks) { break; }
        tm->exhaust2(idx);
    }
}


int main(int argc, char* argv[]) {

    int parallelism = 12;
    int superbatches = 10000;
    int memcache = 100000000;

    std::cout << "Preparing memoization cache..." << std::endl;
    TowerModder tm(memcache);

    std::cout << "Commencing search..." << std::endl;

    std::atomic<uint32_t> ctr{0};
    std::vector<std::thread> workers;

    for (int i = 0; i < parallelism; i++) {
        workers.emplace_back(run_worker, &ctr, superbatches, &tm);
    }

    for (auto&& w : workers) {
        w.join();
    }

    return 0;
}
