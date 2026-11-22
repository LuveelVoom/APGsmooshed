#pragma once

#include <atomic>

#ifdef MAXPOP
// Spaceinvaders: soups skipped as explosive (over MAXPOP or past MAXGEN) in the
// current haul. Counted once per soup in censusSoup; reported with the progress
// messages instead of one line per soup.
std::atomic<uint64_t> aperiodicSoups(0);
#endif

#ifdef LEARNED_PERIOD
// Learned stabilisation period (APGsmooshed; see CHANGES.md): lcm of the
// periods of the census's common oscillators and ships, set between batches
// by updateLearnedPeriod (searching.h); 0 = off. Only in <sym>_smooshed_stabiliser
// builds, whose census differs slightly from the original's.
std::atomic<int> learnedPeriod(0);
#endif

/*
 * Stabilisation detection by checking for population periodicity:
 */
int naivestab_awesome(UPATTERN &pat) {

    // Copied almost verbatim from the apgsearch Python script...
    int depth = 0;
    int prevpop = 0;
    int currpop = 0;
    int period = 12;
    int security = 15;

    int steps = 0;

    #ifdef STDIN_SYM
    pat.advance(0, 0, 256);
    #endif

    for (int i = 0; i < 1000; i++) {

        #ifdef LEARNED_PERIOD
        // After 40 ordinary iterations, try population periodicity with the
        // learned period: soups whose ash has a period not dividing 8, 12,
        // 18, 24 or 30 (e.g. p42 with c/4 ships) otherwise run all 1000
        // iterations before phase II. (APGsmooshed; see CHANGES.md)
        if (i == 40) {
            int lper = learnedPeriod.load(std::memory_order_relaxed);
            if (lper > 0) {
                int lprev = -1, ldepth = 0;
                for (int k = 0; k < 40; k++) {
                    pat.advance(0, 0, lper);
                    steps += lper;
                    #ifdef MAXPOP
                    if (pat.totalPopulation() > MAXPOP) {
                        return 222222;
                    } else if (steps > MAXGEN) {
                        return 222222;
                    }
                    #endif
                    #ifndef HASHLIFE_ONLY
                    if (pat.modified.size() == 0) { return 2; }
                    #endif
                    int lpop = pat.totalPopulation();
                    if (lpop == lprev) { ldepth += 1; } else { ldepth = 0; }
                    lprev = lpop;
                    if (ldepth == 4) { return lper; }
                }
                prevpop = 0;
                depth = 0;
            }
        }
        #endif

        if (i == 40) { security = 20; }
        if (i == 60) { security = 25; }
        if (i == 80) { security = 30; }

        if (i == 400) { period = 18; }
        if (i == 500) { period = 24; }
        if (i == 600) { period = 30; }

        pat.advance(0, 0, period);
        steps += period;

        #ifdef MAXPOP
        if (pat.totalPopulation() > MAXPOP) {
            return 222222;
        } else if (steps > MAXGEN) {
            return 222222;
        }
        #endif

        #ifndef HASHLIFE_ONLY
        if (pat.modified.size() == 0) { return 2; }
        #endif

        currpop = pat.totalPopulation();
        if (currpop == prevpop) {
            depth += 1;
        } else {
            depth = 0;
            period ^= 4;
        }
        prevpop = currpop;
        if (depth == security) {
            // Population is periodic.
            return period;
        }
    }

    return 0;

}

/*
 * Run the universe until it stabilises:
 */
int stabilise3(UPATTERN &pat) {

    int pp = naivestab_awesome(pat);

    if (pp > 0) {
        return pp;
    }

    // Phase II of stabilisation detection, which is much more rigorous and based on oscar.py.

    std::vector<uint64_t> hashlist;
    std::vector<int> genlist;

    int generation = 0;

    for (int j = 0; j < 4000; j++) {

        pat.advance(0, 0, 30);
        generation += 30;

        #ifdef MAXPOP
        if (pat.totalPopulation() > MAXPOP) {
            return 222222;
        } else if (generation > MAXGEN) {
            return 222222;
        }
        #endif

        uint64_t h = pat.totalHash(120);

        // determine where to insert h into hashlist
        int pos = 0;
        int listlen = hashlist.size();

        while (pos < listlen) {
            if (h > hashlist[pos]) {
                pos += 1;
            } else if (h < hashlist[pos]) {
                // shorten lists and append info below
                for (int i = pos; i < listlen; i++) {
                    hashlist.pop_back();
                    genlist.pop_back();
                }
                break;
            } else {
                int period = (generation - genlist[pos]);

                int prevpop = pat.totalPopulation();

                for (int i = 0; i < 20; i++) {
                    pat.advance(0, 0, period);
                    int currpop = pat.totalPopulation();
                    if (currpop != prevpop) {
                        if (period < 1280) { period = 1280; }
                        break;
                    }
                    prevpop = currpop;
                }

                return period;
            }
        }

        hashlist.push_back(h);
        genlist.push_back(generation);
    }

    std::cout << "Failed to detect periodic behaviour!" << std::endl;

    pat.advance(0, 0, 1280);

    return 1280;


}
