#pragma once
#include "pattern2.h"
#include "incubator.h"
#include <unordered_map>
#include <map>
#include <set>

#include "dsds.h"

/*
* This contains code from apgmera, minus the dependence on Golly, for
* separating and classifying objects.
*/

namespace apg {

    template<int M>
    class base_classifier {

        public:

        bool b0;
        uint64_t gmax;
        uint8_t transtable[512];
        std::string rule;
        std::string zoi;
        lifetree_abstract<uint32_t>* lab;
        lifetree<uint32_t, M + 1> lh;
        std::unordered_map<uint64_t, std::string> bitcache;
        std::unordered_map<std::string, std::vector<std::string> > decompositions;

        bool diagbirth() {
            /*
            * Does birth occur in either of the following situations?
            *  o..    ..o
            *  ... or ...
            *  ..o    o..
            */

            return (transtable[257] || transtable[68] || b0);
        }

        std::vector<std::vector<apg::pattern>> get_patterns(dsds &uf, std::unordered_map<uint64_t, uint64_t> &umap, std::vector<apg::pattern> &phases, std::vector<int> *short_connections) {

            uint64_t islcount = 0;

            std::unordered_map<uint64_t, uint64_t> cmap;
            std::vector<uint64_t> invcmap;

            for (auto it = umap.begin(); it != umap.end(); ++it) {
                uint64_t cid = uf.find(it->second);
                if (cmap.count(cid) == 0) {
                    cmap[cid] = (islcount++);
                    invcmap.push_back(cid);
                }
            }

            // Create empty array of patterns:
            std::vector<std::vector<apg::pattern>> subpats(islcount);
            for (uint64_t c = 0; c < islcount; c++) {
                for (uint64_t t = 0; t < phases.size(); t++) {
                    subpats[c].emplace_back(lab, "", rule);
                }
            }

            // Populate patterns. With APG_FAST_SEPARATION (the default; off
            // with recompile.sh --no-fast-separation), one-plane rules collect
            // each island's cells in a bitworld and build the pattern once,
            // instead of one hashlife subrect + union per cell (identical
            // result, far cheaper).
            // (APGsmooshed; see CHANGES.md)
            #ifdef APG_FAST_SEPARATION
            if (M == 1) {
                std::vector<std::vector<std::vector<bitworld>>> bws(islcount,
                    std::vector<std::vector<bitworld>>(phases.size(), std::vector<bitworld>(1)));
                for (auto it = umap.begin(); it != umap.end(); ++it) {
                    uint64_t c = cmap[uf.find(it->second)];
                    uint64_t t = it->first >> 48;
                    bws[c][t][0].setcell((int64_t) (it->first & 0xffffff), (int64_t) ((it->first >> 24) & 0xffffff), 1);
                }
                for (uint64_t c = 0; c < islcount; c++) {
                    for (uint64_t t = 0; t < phases.size(); t++) {
                        if (!bws[c][t][0].world.empty()) { subpats[c][t] = apg::pattern(lab, bws[c][t], rule); }
                    }
                }
            } else
            #endif
            {
                for (auto it = umap.begin(); it != umap.end(); ++it) {
                    uint64_t cid = uf.find(it->second);
                    uint64_t c = cmap[cid];
                    uint64_t t = it->first >> 48;
                    subpats[c][t] += phases[t].subrect((it->first & 0xffffff), ((it->first >> 24) & 0xffffff), 1, 1);
                }
            }

            if (short_connections == nullptr) { return subpats; }

            for (uint64_t t = 0; t < phases.size() - 1; t++) {
                for (uint64_t i = 0; i < islcount; i++) {
                    auto successor = subpats[i][t][1];
                    if (successor == subpats[i][t+1]) {
                        continue; // island is stable in this generation
                    }
                    auto extrabirths = successor - subpats[i][t+1];
                    std::vector<int64_t> celllist(extrabirths.totalPopulation() * 2);
                    extrabirths.get_coords(&(celllist[0]));

                    for (uint64_t ii = 0; ii < celllist.size(); ii += 2) {
                        uint64_t coord = (t << 48) + (celllist[ii+1] << 24) + celllist[ii];
                        
                        std::set<uint64_t> friends;

                        for (auto&& x : (*short_connections)) {
                            uint64_t other = coord + x;
                            if (umap.count(other)) {
                                uint64_t j = cmap[uf.find(umap[other])];
                                if (j != i) { friends.insert(j); }
                            }
                        }

                        if (friends.size() == 1) {
                            uint64_t j = *(friends.begin());
                            uf.merge(invcmap[i], invcmap[j]);
                            return subpats; // improvement made
                        } else if (friends.size() == 0) {
                            std::cerr << "Unreachable code!!!" << std::endl;
                        }
                    }
                }
            }

            return subpats;
        }

        /**
         * Complete object separation.
         */
        std::vector<std::string> full_3dsep(std::string parent) {

            // cache:
            {
                auto it = decompositions.find(parent);
                if (it != decompositions.end()) {
                    return it->second;
                }
            }

            std::vector<std::string> elements;

            #define RETURN_SINGLETON() elements.push_back(parent); decompositions[parent] = elements; return elements
            if (parent[0] != 'x') { RETURN_SINGLETON(); }

            apg::pattern pat(lab, parent, rule);
            uint64_t period = pat.ascertain_period();

            if ((pat.empty()) || (period == 0) || (period > 30000)) { RETURN_SINGLETON(); }

            int64_t dy = pat.dy;
            int64_t dx = pat.dx;

            pat = pat.shift(262144, 262144);

            int64_t bbox[4] = {0};
            pat.getrect(bbox);

            if ((bbox[2] > 100000) || (bbox[3] > 100000)) { RETURN_SINGLETON(); }

            std::unordered_map<uint64_t, uint64_t> umap;
            uint64_t id = 0;

            std::vector<apg::pattern> phases;

            for (uint64_t t = 0; t <= period; t++) {
                phases.emplace_back(lab, "", rule);
                phases[t] += pat;

                std::vector<int64_t> celllist(pat.totalPopulation() * 2);
                pat.get_coords(&(celllist[0]));
                for (uint64_t i = 0; i < celllist.size(); i += 2) {
                    uint64_t coord = (t << 48) + (celllist[i+1] << 24) + celllist[i];
                    umap[coord] = id; id += 1;
                }

                if (t != period) { pat = pat[1]; }
            }

            std::vector<uint64_t> connections;
            std::vector<int> short_connections;

            connections.push_back((period << 48) + (dy << 24) + dx);

            int range = zoi.size() >> 1;

            for (int i = -range; i <= range; i++) {
                for (int j = -range; j <= range; j++) {
                    int k = (i << 24) + j;
                    if (k > 0) { connections.push_back(k); }
                    connections.push_back((1ull << 48) + k);
                    if (k != 0) { short_connections.push_back(k); }
                }
            }

            // disjoint-set data structure:
            dsds uf(id);
            dsds uf2(id);

            std::map<uint64_t, std::vector<uint64_t>> suppressed;

            for (auto it = umap.begin(); it != umap.end(); ++it) {
                for (auto&& x : connections) {
                    uint64_t other = it->first + x;
                    if (umap.count(other)) {
                        uf.merge(it->second, umap[other]);
                        uf2.merge(it->second, umap[other]);
                    }
                }
                for (auto&& x : short_connections) {
                    uint64_t other = it->first + x;
                    if (!(umap.count(other))) {
                        suppressed[other].push_back(it->second);
                    }
                }
            }

            for (auto it = suppressed.begin(); it != suppressed.end(); ++it) {
                uint64_t l = it->second.size();
                if (l >= 3) {
                    for (uint64_t i = 1; i < l; i++) {
                        uf2.merge(it->second[i-1], it->second[i]);
                    }
                }
            }

            if (uf2.count() > 1) {
                // fast separation:

                auto subpats = get_patterns(uf2, umap, phases, nullptr);

                for (auto&& y : subpats) {
                    std::string apgcode = y[0].apgcode();
                    auto new_elems = full_3dsep(apgcode);
                    for (auto&& x : new_elems) { elements.push_back(x); }
                }

                decompositions[parent] = elements;
                return elements;
            }

            std::vector<std::vector<apg::pattern>> subpats;

            // uint64_t isize = uf.count();

            while (subpats.size() != uf.count()) {
                subpats = get_patterns(uf, umap, phases, &short_connections);
            }

            uint64_t islcount = subpats.size();

            /*
            if (islcount != isize) {
                std::cout << parent << " improved from " << isize << " islands to " << islcount << std::endl;
            }
            */

            // Only one island; terminate.
            if (islcount == 1) { RETURN_SINGLETON(); }

            bool all_standalone = true;
            for (uint64_t i = 0; i < islcount; i++) {
                for (uint64_t t = 0; t < period; t++) {
                    if (subpats[i][t][1] != subpats[i][t+1]) {
                        all_standalone = false;
                    }
                }
            }

            if (all_standalone) {

                for (uint64_t i = 0; i < islcount; i++) {
                    std::string apgcode = subpats[i][0].apgcode();
                    auto new_elems = full_3dsep(apgcode);
                    for (auto&& x : new_elems) { elements.push_back(x); }
                }

                decompositions[parent] = elements;
                return elements;
            }

            std::set<uint64_t> invalid_cache;

            bool low_period_osc = (period <= 3) && (dx == 0) && (dy == 0);

            for (uint64_t n = 2; n <= islcount; n++) {

                if (low_period_osc) {
                    if ((n > 4) && (zoi == "95")) {
                        // unnecessary by Four Colour Theorem
                        break;
                    } else if ((n > 7) && (zoi == "99")) {
                        break;
                    }
                }

                uint64_t cost = 1;
                for (uint64_t i = 0; i < islcount; i++) {
                    cost *= n;
                    if (cost > 1048576) { break; }
                }

                if ((cost > 1048576) || (islcount > 16)) {
                    // too expensive; give up
                    std::cerr << "Gave up attempting to separate " << parent << " (" << islcount << " islands) into " << n << " subobjects." << std::endl;
                    break;
                }

                // std::cerr << "Separating " << parent << " (" << islcount << " islands) into " << n << " subobjects." << std::endl;

                std::vector<uint8_t> currstack(islcount);
                std::vector<uint8_t> maxstack(islcount);

                currstack[0] = 0;
                currstack[1] = 0;
                maxstack[0] = 0;

                uint64_t focus = 1;
                while (focus) {

                    uint8_t limit = maxstack[focus - 1] + 1;
                    limit = (limit >= n) ? (n - 1) : limit;
                    if (currstack[focus] > limit) {
                        focus -= 1;
                        currstack[focus] += 1;
                    } else {

                        maxstack[focus] = maxstack[focus - 1];
                        if (maxstack[focus] < currstack[focus]) { maxstack[focus] = currstack[focus]; }
                        if (focus < (islcount - 1)) {
                            focus += 1;
                            currstack[focus] = 0;
                        } else {
                            if (maxstack[focus] == n - 1) {

                                // We have a n-colouring which uses all n colours:
                                std::vector<uint64_t> bitmasks(n);

                                for (uint64_t i = 0; i < islcount; i++) {
                                    bitmasks[currstack[i]] |= (1ull << i);
                                }

                                bool valid = true;

                                for (uint64_t i = 0; i < n; i++) {
                                    if (invalid_cache.count(bitmasks[i])) { valid = false; }
                                }

                                if (valid) {

                                    std::vector<std::vector<apg::pattern>> unions(n);
                                    for (uint64_t i = 0; i < n; i++) {
                                        for (uint64_t t = 0; t <= period; t++) {
                                            unions[i].push_back(pattern(lab, "", rule));
                                        }
                                    }
                                    for (uint64_t i = 0; i < islcount; i++) {
                                        for (uint64_t t = 0; t <= period; t++) {
                                            unions[currstack[i]][t] |= subpats[i][t];
                                        }
                                    }

                                    bool faithful = true;

                                    for (uint64_t i = 0; i < n; i++) {
                                        for (uint64_t t = 0; t < period; t++) {
                                            if (unions[i][t][1] != unions[i][t+1]) {
                                                faithful = false;
                                                invalid_cache.insert(bitmasks[i]);
                                            }
                                        }
                                    }

                                    if (faithful) {
                                        // We have a decomposition into non-interacting pieces!

                                        for (uint64_t i = 0; i < n; i++) {
                                            std::string apgcode = unions[i][0].apgcode();
                                            auto new_elems = full_3dsep(apgcode);
                                            for (auto&& x : new_elems) { elements.push_back(x); }
                                        }

                                        decompositions[parent] = elements;
                                        return elements;
                                    }
                                }
                            }
                            currstack[focus] += 1;
                        }
                    }
                }
            }

            RETURN_SINGLETON();

        }


        std::vector<std::string> pseudoBangBang(pattern pat, std::vector<bitworld> *clvec) {
            /*
            * Borrowed from apgmera, and upgraded.
            */

            uint64_t period = pat.ascertain_period();
            bool isOscillator = ((pat.dx == 0) && (pat.dy == 0));
            pattern hist(&lh, "", rule + "History");
            hist += pat;
            hist = hist[period + 2];
            bitworld lrem = hist.flatlayer(0);
            bitworld env = hist.flatlayer(1);

            // If we have a moving object, do not reiterate:
            bool reiterate = isOscillator && (zoi.length() <= 2);

            std::map<std::pair<int64_t, int64_t>, uint64_t> geography;
            uint64_t label = 0;
            while (lrem.population() != 0) {
                bitworld cluster = grow_cluster(lrem.get1cell(), env, reiterate ? "9" : zoi);
                lrem -= cluster;
                label += 1;
                std::vector<std::pair<int64_t, int64_t> > celllist = cluster.getcells();
                for (uint64_t i = 0; i < celllist.size(); i++) {
                    geography[celllist[i]] = label;
                }
            }

            while (reiterate) {
                reiterate = false;
                for (uint64_t i = 0; i < period; i++) {
                    hist = hist[1];
                    bitworld lcurr = hist.flatlayer(0);
                    bitworld dcurr = bleed(lcurr, "9");
                    dcurr -= env;
                    std::vector<std::pair<int64_t, int64_t> > liberties = dcurr.getcells();
                    for (uint64_t j = 0; j < liberties.size(); j++) {
                        int64_t ix = liberties[j].first;
                        int64_t iy = liberties[j].second;
                        std::map<uint64_t, uint64_t> tally;
                        for (int64_t ux = 0; ux <= 2; ux++) {
                            for (int64_t uy = 0; uy <= 2; uy++) {
                                int value = geography[std::pair<int64_t, int64_t>(ux + ix - 1, uy + iy - 1)];
                                if (lcurr.getcell(ux + ix - 1, uy + iy - 1)) {
                                    tally[value] = tally[value] + (1 << (uy * 3 + ux));
                                }
                            }
                        }

                        uint64_t dominantColour = 0;
                        std::map<uint64_t, uint64_t>::iterator it2;
                        for (it2 = tally.begin(); it2 != tally.end(); it2++) {
                            int colour = it2->first;
                            uint64_t count = it2->second;
                            if (transtable[count]) { dominantColour = colour; }
                            // if (__builtin_popcountll(count) == 3) { dominantColour = colour; }
                        }
                        // Resolve dependencies:
                        if (dominantColour != 0) {
                            std::map<std::pair<int64_t, int64_t>, uint64_t>::iterator it3;
                            for (it3 = geography.begin(); it3 != geography.end(); it3++) {
                                std::pair<int64_t, int64_t> coords = it3->first;
                                uint64_t colour = it3->second;

                                if (tally[colour] > 0) {
                                    geography[coords] = dominantColour;
                                    if (colour != dominantColour) {
                                        // A change has occurred; keep iterating until we achieve stability:
                                        reiterate = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            bitworld lcurr = (isOscillator ? pat.flatlayer(0) : hist.flatlayer(0));
            std::vector<bitworld> cbs(label+1);
            std::map<std::pair<int64_t, int64_t>, uint64_t>::iterator it3;
            for (it3 = geography.begin(); it3 != geography.end(); it3++) {
                std::pair<int64_t, int64_t> coords = it3->first;
                uint64_t colour = it3->second;
                cbs[colour].setcell(coords.first, coords.second, 1);
            }
            std::vector<std::string> components;
            for (uint64_t l = 1; l <= label; l++) {
                cbs[l] &= lcurr;
                if (cbs[l].population() > 0) {
                    if (clvec != 0) { clvec->push_back(cbs[l]); }
                    apg::pattern ppart(lab, lab->demorton(cbs[l], 1), rule);
                    components.push_back(ppart.apgcode());
                }
            }

            for (auto it = components.begin(); it != components.end(); ++it) {
                if ((*it) == "PATHOLOGICAL") {
                    // invalid separation
                    std::vector<std::string> newcomps;
                    newcomps.push_back(pat.apgcode());
                    return newcomps;
                }
            }

            return components;
        }

        std::vector<bitworld> getclusters(bitworld &live, bitworld &env, bool rigorous) {

            bitworld lrem = live;
            std::vector<bitworld> clusters;

            while (lrem.population() != 0) {
                // Obtain cluster:
                bitworld cluster = grow_cluster(lrem.get1cell(), env, zoi);
                cluster &= lrem;
                lrem -= cluster;
                if (rigorous) {
                    pattern ppart(lab, lab->demorton(cluster, 1), rule);
                    pseudoBangBang(ppart, &clusters);
                } else {
                    clusters.push_back(cluster);
                }
            }

            return clusters;
        }

        // Forward declaration for co-recursive function:
        // std::map<std::string, int64_t> census(pattern pat, int numgens, std::string (*adv)(pattern), bool recurse);

        std::pair<bool, std::vector<std::string> > identify(uint64_t bb, std::vector<bitworld> &cplanes, bool recurse) {

            std::string repr;
            std::vector<std::string> elements;

            if ((bb != 0) && (bitcache.find(bb) != bitcache.end())) {
                repr = bitcache[bb];
                elements = decompositions[repr];
                return std::pair<bool, std::vector<std::string> >(true, elements);
            }

            if (cplanes.size() == 0) {
                cplanes.resize(1);
                cplanes[0].world.emplace(std::pair<int32_t, int32_t>(0, 0), bb);
            }

            apg::pattern cl2(lab, cplanes, rule);
            cl2.pdetect(gmax); // Restrict period.

            if (cl2.dt == 0) {
                return std::pair<bool, std::vector<std::string> >(false, elements);
            }

            uint64_t period = cl2.ascertain_period();
            repr = cl2.apgcode();

            if ((repr[0] == 'x') && (period <= 10000)) {
                elements = full_3dsep(repr);
            } else if (recurse) {
                std::map<std::string, int64_t> rc = census(cl2, period << 3, 0, false);
                for (auto it2 = rc.begin(); it2 != rc.end(); ++it2) {
                    if (it2->second > 0) {
                        for (int64_t i = 0; i < it2->second; i++) {
                            elements.push_back(it2->first);
                        }
                    }
                }
            } else {
                elements.push_back(repr);
            }

            if ((repr[0] == 'x') && (bb != 0)) {
                bitcache.emplace(bb, repr);
            }
            
            return std::pair<bool, std::vector<std::string> >(true, elements);
        }

        template<int H>
        void deeppurge(std::map<std::string, int64_t> &cm, incubator<56, H> &icb, std::string (*adv)(pattern),
                        bool remove_annoyances, bool remove_gliders) {

            uint64_t excess[8] = {0ull};

            for (auto it = icb.tiles.begin(); it != icb.tiles.end(); ++it) {
                Incube<56, H>* sqt = &(it->second);
                for (int y = 0; y < H; y++) {
                    uint64_t r = sqt->d[y];
                    while (r != 0) {
                        uint64_t x = __builtin_ctzll(r);
                        int annoyance = (remove_annoyances ? icb.isAnnoyance(sqt, x, y) : 0);
                        if (annoyance > 0) {
                            excess[annoyance] += 1;
                        } else if ((!remove_gliders) || (icb.isGlider(sqt, x, y) == 0)) {
                            // TODO: Identify unknown object
                            auto intList = icb.get_component(sqt, x, y);
                            int population = intList.back();
                            int ll = intList.size() - 1;
                            if (population > 0) {
                                if ((remove_annoyances) && (population == 3)) {
                                    excess[3] += 1;
                                } else {
                                    std::vector<int> celllist(population*2);
                                    int i = 0;
                                    for (int j = 0; j < ll; j += 3) {
                                        if (intList[j + 2] == 1) {
                                            celllist[i++] = intList[j];
                                            celllist[i++] = intList[j+1];
                                        }
                                    }

                                    int left = celllist[0];
                                    int top = celllist[1];
                                    int right = celllist[0];
                                    int bottom = celllist[1];

                                    for (int i = 0; i < (population*2); i += 2) {
                                        if (left > celllist[i]) { left = celllist[i]; }
                                        if (right < celllist[i]) { right = celllist[i]; }
                                        if (top > celllist[i+1]) { top = celllist[i+1]; }
                                        if (bottom < celllist[i+1]) { bottom = celllist[i+1]; }
                                    }

                                    for (int i = 0; i < (population*2); i += 2) {
                                        celllist[i] -= left;
                                        celllist[i+1] -= top;
                                    }

                                    right -= left;
                                    bottom -= top;

                                    uint64_t bitstring = 0;
                                    std::vector<bitworld> cplanes;

                                    if (right <= 7 && bottom <= 7) {
                                        for (int i = 0; i < (population*2); i += 2) {
                                            bitstring |= (1ull << (celllist[i] + 8*celllist[i+1]));
                                        }
                                    } else {
                                        cplanes.resize(1);
                                        for (int i = 0; i < (population*2); i += 2) {
                                            cplanes[0].setcell(celllist[i], celllist[i+1], 1);
                                        }
                                    }

                                    std::pair<bool, std::vector<std::string> > res = identify(bitstring, cplanes, true);

                                    if (!(res.first)) {
                                        apg::pattern cl2(lab, cplanes, rule);
                                        std::string diagnosed = "PATHOLOGICAL";
                                        if (adv != 0) { diagnosed = (*adv)(cl2); }
                                        res.second.push_back(diagnosed);
                                    }

                                    // Enter elements into tally:
                                    for (uint64_t i = 0; i < res.second.size(); i++) {
                                        cm[res.second[i]] += 1;
                                    }
                                }
                            }
                        }
                        r ^= (1ull << x);
                        r &= sqt->d[y];
                    }
                }
            }

            if (excess[3] > 0) { cm["xp2_7"] += excess[3]; }
            if (excess[4] > 0) { cm["xs4_33"] += excess[4]; }
            if (excess[5] > 0) { cm["xq4_153"] += excess[5]; }
            if (excess[6] > 0) { cm["xs6_696"] += excess[6]; }
            if (excess[7] > 0) { cm["xs5_253"] += excess[7]; }

        }

        void census(std::map<std::string, int64_t> &tally, std::vector<bitworld> &planes, std::string (*adv)(pattern), bool recurse) {

            bitworld lrem = planes[0];
            for (uint64_t i = 1; i < M; i++) {
                lrem += planes[i];
            }
            bitworld env = lrem;
            env += planes[M];

            bool glider_plane = ((M == 1) && (planes.size() == 3));

            bitworld lrem2 = lrem;
            if (glider_plane) { lrem2 -= planes[2]; }

            while (lrem2.population() != 0) {

                // Obtain cluster:
                bitworld cluster = grow_cluster(lrem2.get1cell(), env, zoi);
                cluster &= lrem;
                lrem -= cluster;
                lrem2 -= cluster;

                uint64_t bb = 0;

                if (M == 1) {
                    if (cluster.world.size() > 1) { cluster = fix_topleft(cluster); }
                    if (cluster.world.size() == 1) {
                        // We use a bitcache for fast lookup of small objects:
                        auto it = cluster.world.begin(); bb = it->second;
                    }
                }

                std::vector<bitworld> cplanes;

                if (bb == 0) {
                    for (uint64_t i = 0; i < M; i++) {
                        cplanes.push_back(cluster);
                        if (M != 1) { cplanes.back() &= planes[i]; }
                    }
                }

                std::pair<bool, std::vector<std::string> > res = identify(bb, cplanes, recurse);

                if (!(res.first)) {
                    apg::pattern cl2(lab, cplanes, rule);
                    std::string diagnosed = "PATHOLOGICAL";
                    if (adv != 0) { diagnosed = (*adv)(cl2); }
                    res.second.push_back(diagnosed);
                }

                // Enter elements into tally:
                for (uint64_t i = 0; i < res.second.size(); i++) {
                    tally[res.second[i]] += 1;
                }
            }

            if (lrem.population() > 0) { tally["xq4_153"] += (lrem.population() / 5); }

        }

        std::map<std::string, int64_t> census(std::vector<bitworld> &planes, std::string (*adv)(pattern), bool recurse) {
            std::map<std::string, int64_t> tally;
            census(tally, planes, adv, recurse);
            return tally;
        }

        std::map<std::string, int64_t> census(std::vector<bitworld> &planes, std::string (*adv)(pattern)) {
            return census(planes, adv, true);
        }

        std::map<std::string, int64_t> census(bitworld &live, bitworld &env, std::string (*adv)(pattern)) {
            std::vector<bitworld> bwv;
            bwv.push_back(live); bwv.push_back(env);
            return census(bwv, adv);
        }

        std::map<std::string, int64_t> census(bitworld &live, bitworld &env) {
            return census(live, env, 0);
        }

        std::map<std::string, int64_t> census(pattern pat, int numgens, std::string (*adv)(pattern), bool recurse) {
            pattern hist(&lh, "", rule + "History");
            hist += pat;
            hist = hist[numgens];
            std::vector<bitworld> bwv;
            for (uint64_t i = 0; i <= M; i++) { bwv.push_back(hist.flatlayer(i)); }
            return census(bwv, adv, recurse);
        }

        std::map<std::string, int64_t> census(pattern pat, int numgens, std::string (*adv)(pattern)) {
            return census(pat, numgens, adv, true);
        }

        std::map<std::string, int64_t> census(pattern pat, int numgens) {
            return census(pat, numgens, 0);
        }

        base_classifier(lifetree_abstract<uint32_t>* lab, std::string rule) : lh(100)
        {
            this->lab = lab;
            this->rule = rule;
            gmax = 1048576;

            /*
            * We construct the transition table by bootstrapping: we run a
            * pattern containing all 512 3-by-3 tiles and examine the
            * centre cells of the tiles after one generation.
            */
            std::string transrle = "3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o3bo3bob2o3b2obob"
            "5o$24bo2bo2bo2bo2bo2bo2bo2bo3bo2bo2bo2bo2bo2bo2bo2bob2ob2ob2ob2ob2ob2o"
            "b2ob2o2$3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o3bo3bob"
            "2o3b2obob5o$2bo2bo2bo2bo2bo2bo2bo2b2ob2ob2ob2ob2ob2ob2ob2obob2ob2ob2ob"
            "2ob2ob2ob2ob26o2$3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o3bo3bob2o3b2obob"
            "5o3bo3bob2o3b2obob5o$24bo2bo2bo2bo2bo2bo2bo2bo3bo2bo2bo2bo2bo2bo2bo2bo"
            "b2ob2ob2ob2ob2ob2ob2ob2o$o2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2b"
            "o2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo$3bo3bob2o3b2obob5o3b"
            "o3bob2o3b2obob5o3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o$2bo2bo2bo2bo2bo2b"
            "o2bo2b2ob2ob2ob2ob2ob2ob2ob2obob2ob2ob2ob2ob2ob2ob2ob26o$o2bo2bo2bo2bo"
            "2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo"
            "2bo2bo2bo2bo$3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o3bo"
            "3bob2o3b2obob5o$24bo2bo2bo2bo2bo2bo2bo2bo3bo2bo2bo2bo2bo2bo2bo2bob2ob"
            "2ob2ob2ob2ob2ob2ob2o$bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2b"
            "o2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo$3bo3bob2o3b2obob5o3bo3b"
            "ob2o3b2obob5o3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o$2bo2bo2bo2bo2bo2bo2b"
            "o2b2ob2ob2ob2ob2ob2ob2ob2obob2ob2ob2ob2ob2ob2ob2ob26o$bo2bo2bo2bo2bo2b"
            "o2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo"
            "2bo2bo2bo$3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o3bo3bo"
            "b2o3b2obob5o$24bo2bo2bo2bo2bo2bo2bo2bo3bo2bo2bo2bo2bo2bo2bo2bob2ob2ob"
            "2ob2ob2ob2ob2ob2o$2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob"
            "2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2o$3bo3bob2o3b2obob5o3bo3bob"
            "2o3b2obob5o3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o$2bo2bo2bo2bo2bo2bo2bo"
            "2b2ob2ob2ob2ob2ob2ob2ob2obob2ob2ob2ob2ob2ob2ob2ob26o$2ob2ob2ob2ob2ob2o"
            "b2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob"
            "2ob2ob2o$3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o3bo3bob"
            "2o3b2obob5o$24bo2bo2bo2bo2bo2bo2bo2bo3bo2bo2bo2bo2bo2bo2bo2bob2ob2ob2o"
            "b2ob2ob2ob2ob2o$2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo"
            "2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo$3bo3bob2o3b2obob5o3bo3bob2o"
            "3b2obob5o3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o$2bo2bo2bo2bo2bo2bo2bo2b"
            "2ob2ob2ob2ob2ob2ob2ob2obob2ob2ob2ob2ob2ob2ob2ob26o$2bo2bo2bo2bo2bo2bo"
            "2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo2bo"
            "2bo2bo2bo$3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o3bo3bo"
            "b2o3b2obob5o$24bo2bo2bo2bo2bo2bo2bo2bo3bo2bo2bo2bo2bo2bo2bo2bob2ob2ob"
            "2ob2ob2ob2ob2ob2o$ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2o"
            "b2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2obo$3bo3bob2o3b2obob5o3bo3bob"
            "2o3b2obob5o3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o$2bo2bo2bo2bo2bo2bo2bo"
            "2b2ob2ob2ob2ob2ob2ob2ob2obob2ob2ob2ob2ob2ob2ob2ob26o$ob2ob2ob2ob2ob2ob"
            "2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob"
            "2ob2ob2obo$3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o3bo3b"
            "ob2o3b2obob5o$24bo2bo2bo2bo2bo2bo2bo2bo3bo2bo2bo2bo2bo2bo2bo2bob2ob2ob"
            "2ob2ob2ob2ob2ob2o$b2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob"
            "2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2o$3bo3bob2o3b2obob5o3bo3bob"
            "2o3b2obob5o3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o$2bo2bo2bo2bo2bo2bo2bo"
            "2b2ob2ob2ob2ob2ob2ob2ob2obob2ob2ob2ob2ob2ob2ob2ob26o$b2ob2ob2ob2ob2ob"
            "2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob"
            "2ob2ob2ob2o$3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o3bo"
            "3bob2o3b2obob5o$24bo2bo2bo2bo2bo2bo2bo2bo3bo2bo2bo2bo2bo2bo2bo2bob2ob"
            "2ob2ob2ob2ob2ob2ob2o$96o$3bo3bob2o3b2obob5o3bo3bob2o3b2obob5o3bo3bob2o"
            "3b2obob5o3bo3bob2o3b2obob5o$2bo2bo2bo2bo2bo2bo2bo2b2ob2ob2ob2ob2ob2ob"
            "2ob2obob2ob2ob2ob2ob2ob2ob2ob26o$96o!";


            b0 = false;
            std::string subrule = rule;

            // Strip Generations prefix:
            if (subrule[0] == 'g') {
                subrule = subrule.substr(1);
                while ((subrule[0] >= '0') && (subrule[1] <= '9')) {
                    subrule = subrule.substr(1);
                }
            }

            if (uli_get_family(rule2int(rule)) >= 6) {
                zoi = get_zoi(rule);
            } else if ((subrule[0] == 'b') && (subrule[1] == '0')) {
                zoi = "99"; b0 = true;
            } else if (subrule[0] == 'r') {
                zoi = std::string(2 * (subrule[1] - '0'), '9');
            } else if (subrule[0] == 'b') {
                pattern transpat(lab, transrle, rule);
                bitworld bw = transpat[1].flatlayer(0);
                for (int i = 0; i < 512; i++) {
                    int x = 3 * (i & 31) + 1;
                    int y = 3 * (i >> 5) + 1;
                    transtable[i] = bw.getcell(x, y);
                }
                zoi = (diagbirth()) ? "99" : "95";
            } else {
                zoi = get_zoi(rule);
            }
        }
    };

    typedef base_classifier<1> classifier;

}
