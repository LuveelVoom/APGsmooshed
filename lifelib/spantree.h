#pragma once

#include "dsds.h"

#include <utility>
#include <vector>
#include <algorithm>
#include <list>
#include <cmath>
#include <map>

#include <iostream>

namespace apg {

    typedef std::pair<int64_t, int64_t> coords64;
    typedef std::pair<int64_t, uint64_t> coorded;
    typedef std::pair<uint64_t, uint64_t> edge64;

    const static int64_t OCT_LONG = 225058681;
    const static int64_t OCT_SHORT = 93222358;

    std::vector<edge64> spanning_graph(const std::vector<coords64> &coords) {

        // We reuse this for all four octants to reduce malloc/sort time:
        uint64_t N = coords.size();
        std::vector<std::pair<coorded, coords64> > c(N);
        for (uint64_t i = 0; i < N; i++) {
            c[i] = std::make_pair(std::make_pair((int64_t) 0, i), coords[i]);
        }

        // Output list of edges:
        std::vector<edge64> edgelist;

        // Maintain a set of 'active points':
        std::map<int64_t, std::pair<int64_t, std::pair<uint64_t, uint64_t> > > activeset;

        uint64_t time0;

        /*
        * The next four loops (which are structurally identical and differ
        * only in the direction of the sweep-line) connect each point to the
        * closest neighbour in each octant.
        */

        // OCTANT 0:
        for (uint64_t j = 0; j < N; j++) {
            c[j].first.first = OCT_SHORT * c[j].second.first - OCT_LONG * c[j].second.second;
        }
        std::sort(c.begin(), c.end()); time0 = 0;
        for (auto it = c.begin(); it != c.end(); ++it) {
            uint64_t i = it->first.second;
            int64_t orth = -it->second.first;
            int64_t diag = it->second.second - orth;
            uint64_t time2 = 0;
            uint64_t k = i;
            for (auto jt = activeset.lower_bound(orth); jt != activeset.end(); ) {
                if (diag > jt->second.first) { break; }
                uint64_t time1 = jt->second.second.second;
                if (time1 > time2) {time2 = time1; k = jt->second.second.first;}
                activeset.erase(jt++);
            }
            if (time2) { edgelist.push_back(std::make_pair(i, k)); }
            activeset.emplace(orth, std::make_pair(diag, std::make_pair(i, ++time0)));
        }
        activeset.clear();

        // OCTANT 1:
        for (uint64_t j = 0; j < N; j++) {
            c[j].first.first = OCT_LONG * c[j].second.first - OCT_SHORT * c[j].second.second;
        }
        std::sort(c.begin(), c.end()); time0 = 0;
        for (auto it = c.begin(); it != c.end(); ++it) {
            uint64_t i = it->first.second;
            int64_t orth = it->second.second;
            int64_t diag = -it->second.first - orth;
            uint64_t time2 = 0;
            uint64_t k = i;
            for (auto jt = activeset.lower_bound(orth); jt != activeset.end(); ) {
                if (diag > jt->second.first) { break; }
                uint64_t time1 = jt->second.second.second;
                if (time1 > time2) {time2 = time1; k = jt->second.second.first;}
                activeset.erase(jt++);
            }
            if (time2) { edgelist.push_back(std::make_pair(i, k)); }
            activeset.emplace(orth, std::make_pair(diag, std::make_pair(i, ++time0)));
        }
        activeset.clear();

        // OCTANT 2:
        for (uint64_t j = 0; j < N; j++) {
            c[j].first.first = OCT_LONG * c[j].second.first + OCT_SHORT * c[j].second.second;
        }
        std::sort(c.begin(), c.end()); time0 = 0;
        for (auto it = c.begin(); it != c.end(); ++it) {
            uint64_t i = it->first.second;
            int64_t orth = -it->second.second;
            int64_t diag = -it->second.first - orth;
            uint64_t time2 = 0;
            uint64_t k = i;
            for (auto jt = activeset.lower_bound(orth); jt != activeset.end(); ) {
                if (diag > jt->second.first) { break; }
                uint64_t time1 = jt->second.second.second;
                if (time1 > time2) {time2 = time1; k = jt->second.second.first;}
                activeset.erase(jt++);
            }
            if (time2) { edgelist.push_back(std::make_pair(i, k)); }
            activeset.emplace(orth, std::make_pair(diag, std::make_pair(i, ++time0)));
        }
        activeset.clear();

        // OCTANT 3:
        for (uint64_t j = 0; j < N; j++) {
            c[j].first.first = OCT_SHORT * c[j].second.first + OCT_LONG * c[j].second.second;
        }
        std::sort(c.begin(), c.end()); time0 = 0;
        for (auto it = c.begin(); it != c.end(); ++it) {
            uint64_t i = it->first.second;
            int64_t orth = -it->second.first;
            int64_t diag = -it->second.second - orth;
            uint64_t time2 = 0;
            uint64_t k = i;
            for (auto jt = activeset.lower_bound(orth); jt != activeset.end(); ) {
                if (diag > jt->second.first) { break; }
                uint64_t time1 = jt->second.second.second;
                if (time1 > time2) {time2 = time1; k = jt->second.second.first;}
                activeset.erase(jt++);
            }
            if (time2) { edgelist.push_back(std::make_pair(i, k)); }
            activeset.emplace(orth, std::make_pair(diag, std::make_pair(i, ++time0)));
        }
        activeset.clear();

        return edgelist;
    }

    double spanning_graph_to_tree(const std::vector<coords64> &coords, std::vector<edge64> *streeptr, double (*scorer)(double), const std::vector<edge64> &sgraph) {

        double score = 0.0;

        // Initially colour every vertex differently:
        dsds colours(coords.size());

        // Compute squared length of edges:
        std::vector<std::pair<int64_t, edge64> > sedges;
        for (auto it = sgraph.begin(); it != sgraph.end(); ++it) {
            coords64 a = coords[it->first];
            coords64 b = coords[it->second];
            int64_t xdiff = a.first - b.first;
            int64_t ydiff = a.second - b.second;
            int64_t sqlength = (xdiff * xdiff) + (ydiff * ydiff);
            sedges.push_back(std::make_pair(sqlength, *it));
        }

        // Sort edges into ascending order:
        std::sort(sedges.begin(), sedges.end());

        // Apply Kruskal's algorithm:
        for (std::vector<std::pair<int64_t, edge64> >::iterator it = sedges.begin(); it != sedges.end(); ++it) {
            uint64_t a = it->second.first;
            uint64_t b = it->second.second;
            if (!colours.connected(a, b)) {
                colours.merge(a, b);
                if (streeptr != 0) { streeptr->push_back(it->second); }
                if (scorer != 0) { score += (*scorer)(it->first); }
            }
        }

        return score;
    }

    double spanning_tree(const std::vector<coords64> &coords, std::vector<edge64> *streeptr, double (*scorer)(double)) {

        // Produce spanning graph (superset of spanning tree):
        std::vector<edge64> sgraph = spanning_graph(coords);
        return spanning_graph_to_tree(coords, streeptr, scorer, sgraph);

    }

    std::vector<edge64> spanning_tree(const std::vector<coords64> &coords) {
        std::vector<edge64> stree;
        spanning_tree(coords, &stree, 0);
        return stree;
    }

    double spanning_tree_length(const std::vector<coords64> &coords, double (*scorer)(double)) {
        return spanning_tree(coords, 0, scorer);
    }

    double spanning_tree_length(const std::vector<coords64> &coords) {
        return spanning_tree_length(coords, &std::sqrt);
    }

    double pow5over8(double x) {
        // Calling sqrt three times is a lot faster than a single call to pow;
        // see https://github.com/simeksgol/GoL_destroy/blob/master/destroy.c
        return std::sqrt(x * std::sqrt(std::sqrt(x)));
    }

    double simeks_score(const std::vector<coords64> &coords) {
        return spanning_tree_length(coords, &pow5over8);
    }

    std::vector<coords64> get_sunflower(int n, double scale) {

        std::vector<coords64> sunflower;
        for (int i = 0; i < n; i++) {
            double sangle = 3.88322207745;
            int64_t x = 0.5 + std::cos(i * sangle) * std::sqrt(i + 1) * scale;
            int64_t y = 0.5 + std::sin(i * sangle) * std::sqrt(i + 1) * scale;
            sunflower.push_back(std::make_pair(x, y));
        }
        return sunflower;
    }

}

