#include "../pattern2.h"
#include "qufince.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <chrono>


typedef std::pair<std::string, std::pair<int64_t, int64_t>> Symmetry;

namespace apg {

void print_bpattern(const apg::bpattern &x) {

    std::string rle = "x = 64, y = 64, rule = B3/S23\n";
    rle += apg::bp2rle(x) + "\n";
    std::cout << rle << std::endl;
    
}

std::vector<bpattern> get_unique_bpatterns(std::vector<bpattern> &res) {

    std::sort(res.begin(), res.end());
    std::vector<bpattern> res2;

    for (uint64_t i = 0; i < res.size(); i++) {
        if ((i > 0) && (res[i] == res[i-1])) { continue; }
        res2.push_back(res[i]);
    }

    return res2;
}

}

int compute_badness(uint64_t vecsize) {
    if (vecsize >= 16777216) { return 100000; }
    if (vecsize <= 8192) { return (32768 / vecsize); }
    return (vecsize / 32768);
}

void try_symmetry(apg::pattern &x, std::vector<Symmetry> &syms, std::string tfm) {

    int64_t bbox1[4] = {0};
    int64_t bbox2[4] = {0};

    apg::pattern y = x.transform(tfm, 0, 0);
    x.getrect(bbox1); y.getrect(bbox2);

    int64_t dx = bbox1[0] - bbox2[0];
    int64_t dy = bbox1[1] - bbox2[1];

    if (y.shift(dx, dy) == x) {
        std::cerr << "(\"" << tfm << "\", " << dx << ", " << dy << ")" << std::endl;
        syms.emplace_back(tfm, std::pair<int64_t, int64_t>(dx, dy));
    }
}

void ascertain_symmetries(apg::pattern &x, std::vector<Symmetry> &syms) {

    if (x.empty()) { return; }

    try_symmetry(x, syms, "flip");
    try_symmetry(x, syms, "flip_x");
    try_symmetry(x, syms, "flip_y");
    try_symmetry(x, syms, "rcw");
    try_symmetry(x, syms, "rccw");
    try_symmetry(x, syms, "swap_xy");
    try_symmetry(x, syms, "swap_xy_flip");

}


void expand_border(apg::pattern& border) {

    auto lab = border.getlab();
    auto solid_box = apg::pattern(lab, lab->rectangle(0, 0, 64, 64), "b3s23");
    auto interior  = apg::pattern(lab, lab->rectangle(1, 1, 62, 62), "b3s23");
    auto boundary  = solid_box - interior;

    border |= ((border.shift(1, 0) | border.shift(-1, 0)) & boundary);
    border |= ((border.shift(0, 1) | border.shift(0, -1)) & boundary);

}


std::vector<std::vector<apg::pattern>> to_boxes(apg::pattern& original) {

    std::vector<std::vector<apg::pattern>> boxes;

    if (original.empty()) {
        std::cerr << "Error: empty pattern provided." << std::endl;
        return boxes;
    }

    int64_t bbox[4] = {0, 0, 0, 0};
    original.getrect(bbox);

    auto shifted = original.shift(-bbox[0], -bbox[1]);
    auto lab = original.getlab();

    auto solid_box = apg::pattern(lab, lab->rectangle(0, 0, 64, 64), "b3s23");
    auto interior  = apg::pattern(lab, lab->rectangle(1, 1, 62, 62), "b3s23");
    auto boundary  = solid_box - interior;

    auto m = shifted.match(boundary);
    auto invalid_cells = shifted - m.convolve(solid_box);
    if (invalid_cells.nonempty()) {
        std::cerr << "Warning: " << invalid_cells.totalPopulation() << " cells found outside boxes." << std::endl;
    }

    uint64_t n_boxes = m.totalPopulation();

    std::vector<int64_t> coords(2 * n_boxes);
    m.get_coords(&(coords[0]));

    for (uint64_t i = 0; i < n_boxes; i++) {
        int64_t x = coords[2*i];
        int64_t y = coords[2*i+1];
        if (y & 63) {
            std::cerr << "Error: unaligned box found at (" << x << ", " << y << ")" << std::endl;
            return boxes;
        }
    }


    int64_t lastx = 0;
    for (uint64_t i = 0; i < n_boxes; i++) {
        int64_t x = coords[2*i];
        int64_t y = coords[2*i+1];
        if ((i > 0) && (coords[2*i-1] == y) && (x - lastx < 64)) {
            continue;
        }
        lastx = x;
        uint64_t my = y >> 6;

        auto p = (shifted & interior.shift(x, y)).shift(-x, -y);

        if (my >= boxes.size()) { boxes.resize(my + 1); }
        boxes[my].push_back(p);
    }

    std::cerr << "Profile:";
    for (auto&& v : boxes) { std::cerr << " " << v.size(); }
    std::cerr << std::endl;

    return boxes;
}


bool disjoint_check(apg::pattern &a, apg::pattern &b, int n) {

    if ((a & b).nonempty()) { return false; }
    auto an = a[n];
    auto bn = b[n];
    if ((an & bn).nonempty()) { return false; }
    return ((a + b)[n] == an + bn);

}


std::vector<apg::bpattern> expand_translations_inner(std::vector<apg::pattern> &vp, apg::pattern &initial, int minreact, apg::pattern &border, const std::vector<Symmetry> &syms) {

    std::vector<apg::bpattern> res;

    int maxreact = 0;

    if ((vp.size() & 1) == 0) {
        maxreact = vp.back().totalPopulation();
        vp.pop_back();
    }

    if (vp[0].totalPopulation() != 1) {
        std::cerr << "Error: first box in a catalyst row must have a single dot" << std::endl;
        return res;
    }

    int64_t bbox[4] = {0, 0, 0, 0};

    vp[0].getrect(bbox);
    uint64_t n = 0;

    for (size_t i = 1; i < vp.size(); i += 2) {
        uint64_t m = vp[i+1].totalPopulation();
        n += m;
        std::vector<int64_t> coords(2*m);
        vp[i+1].get_coords(&(coords[0]));
        for (size_t j = 0; j < m; j++) {
            int64_t dx = coords[2*j] - bbox[0];
            int64_t dy = coords[2*j+1] - bbox[1];
            auto p = vp[i].shift(dx, dy);

            for (size_t k = 0; k < syms.size(); k++) {
                p += p.transform(syms[k].first, syms[k].second.first, syms[k].second.second);
            }

            if ((maxreact > 0) && disjoint_check(p, initial, maxreact)) {
                continue;
            }

            if ((p & border).empty() && disjoint_check(p, initial, minreact)) {
                res.push_back(p.flatlayer(0).to_bpattern());
            }
        }
    }

    auto res2 = get_unique_bpatterns(res);
    std::cerr << n << " --> " << res.size() << " --> " << res2.size() << std::endl;
    return res2;
}


double expand_translations(std::vector<std::vector<apg::pattern>> &vvp, std::vector<std::vector<std::vector<apg::bpattern>>> &vvvp, apg::pattern &initial, int minreact, apg::pattern &border, const std::vector<Symmetry> &syms) {

    double product = 1;

    for (size_t i = 1; i < vvp.size(); i++) {
        if (vvp[i].empty()) {
            if (vvvp.empty() || (vvvp[vvvp.size() - 1].size())) {
                vvvp.emplace_back(0);
            }
            continue;
        }
        auto vp = expand_translations_inner(vvp[i], initial, minreact, border, syms);
        if (vp.empty()) { return -1; }
        vvvp[vvvp.size() - 1].push_back(vp);
        product *= vp.size();
    }

    return product;
}


std::vector<apg::bpattern> gpu_product(std::vector<std::vector<apg::bpattern>> &vvp, apg::pattern &initial, int minreact) {

    vvp.resize(vvp.size() + 1);
    vvp[vvp.size() - 1].push_back(initial.flatlayer(0).to_bpattern());

    while (vvp.size() > 1) {
        std::vector<apg::bpattern> x1; x1.swap(vvp.back()); vvp.pop_back();
        std::vector<apg::bpattern> x2; x2.swap(vvp.back()); vvp.pop_back();
        vvp.push_back(disjoint_product(x1, x2, minreact));
    }

    return vvp[0];
}


int main(int argc, char *argv[]) {

    std::cerr << "\n\033[32;1m***** This is QuFince v3 *****\033[0m\n" << std::endl;

    if (argc < 2){
        std::cerr << "Please specify the input file." << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    apg::lifetree<uint32_t, 1> lt(1000);

    std::cerr << "Reading file..." << std::endl;
    apg::pattern qf(&lt, filename);

    auto vvp = to_boxes(qf);

    if (vvp.empty()) { return 1; }

    if (vvp[0].size() < 6) {
        std::cerr << "Error: top row should contain 6 boxes" << std::endl;
        return 1;
    }

    apg::pattern initial = vvp[0][0];
    apg::pattern and_mask = vvp[0][1];
    apg::pattern match_cells = vvp[0][2];

    if ((and_mask & match_cells) != match_cells) {
        std::cerr << "Error: match_cells should be a subset of and_mask" << std::endl;
        return 1;
    }

    int gens1 = vvp[0][3].totalPopulation();
    int gens2 = vvp[0][4].totalPopulation();
    int minreact = vvp[0][5].totalPopulation();
    int miscflags = (vvp[0].size() >= 7) ? vvp[0][6].totalPopulation() : 0;

    apg::pattern border = (vvp[0].size() >= 8) ? vvp[0][7] : apg::pattern(&lt, "", "b3s23");
    expand_border(border);

    std::cerr << "Minimum generation for reaction to begin: " << minreact << std::endl;
    std::cerr << "Must match in generations " << gens1 << " and " << (gens1 + gens2) << std::endl;

    if (miscflags & 4) { std::cerr << "Ignoring patterns with period dividing 4" << std::endl; }
    if (miscflags & 8) { std::cerr << "Ignoring patterns with period dividing " << gens2 << std::endl; }

    std::vector<Symmetry> syms;

    if (vvp[0].size() >= 9) {
        ascertain_symmetries(vvp[0][8], syms);
    }

    apg::pattern excludecat = (vvp[0].size() >= 10) ? vvp[0][9] : apg::pattern(&lt, "", "b3s23");
    excludecat += border;

    std::vector<std::vector<std::vector<apg::bpattern>>> vvvp;

    double estimate1 = expand_translations(vvp, vvvp, initial, minreact, excludecat, syms);
    if (estimate1 <= 0) { return 1; }

    std::cerr << "Estimated search space: " << estimate1 << std::endl;

    if (vvvp.size() != 2) {
        std::cerr << "Error: there should be exactly two sets of catalyst rows." << std::endl;
        return 1;
    }

    uint64_t tmem = apg::print_memory_statistics();

    if (tmem == 0) { return 1; }

    std::cerr << "Preprocessing..." << std::endl;    
    auto before = std::chrono::high_resolution_clock::now();
    auto avec = gpu_product(vvvp[0], initial, minreact);
    auto bvec = gpu_product(vvvp[1], initial, minreact);
    uint64_t actual = avec.size() * bvec.size();
    auto after = std::chrono::high_resolution_clock::now();
    uint64_t microseconds = std::chrono::duration_cast<std::chrono::microseconds>(after - before).count();
    std::cerr << "...preprocessing completed in " << (1.0e-6 * microseconds) << " seconds." << std::endl;

    if (actual == 0) {
        std::cerr << "Empty search space." << std::endl;
        return 0;
    }

    std::cerr << "Actual search space: " << actual << " (reduced by a factor of " << (estimate1 / actual) << ")" << std::endl;

    auto initial_b = initial.flatlayer(0).to_bpattern();
    auto and_mask_b = and_mask.flatlayer(0).to_bpattern();
    auto target_b = match_cells.flatlayer(0).to_bpattern();
    auto border_b = border.flatlayer(0).to_bpattern();

    uint32_t flags = gens1 + (gens2 << 12) + (miscflags << 24);

    // Swap the two vectors if it results in a better workload:
    {
        auto adisc = compute_badness(avec.size());
        auto bdisc = compute_badness(bvec.size());

        if (bdisc > adisc) {
            avec.swap(bvec);
            uint32_t ff = (flags ^ (flags >> 1)) & 0x1000000;
            flags ^= ff ^ (ff << 1);
        }
    }

    uint32_t chunksize = 0x100000000ull / (hh::max(256, gens1 + gens2) * bvec.size());
    if (chunksize < 64) { chunksize = 64; }
    std::cerr << "Chunk size = " << chunksize << std::endl;

    run_collision_kernel(initial_b, and_mask_b, target_b, border_b, bvec, avec, flags, chunksize);

    return 0;

}
