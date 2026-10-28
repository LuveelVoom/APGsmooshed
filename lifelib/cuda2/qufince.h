#pragma once
#include "../classic/bpattern.h"
#include <vector>

namespace apg {

void run_collision_kernel(const bpattern& initial, const bpattern& and_mask, const bpattern& target, const bpattern& border, const std::vector<bpattern> &a, const std::vector<bpattern> &b, uint32_t flags, uint32_t chunksize);

void print_bpattern(const apg::bpattern &x);

std::vector<bpattern> disjoint_product(const std::vector<bpattern> &a, const std::vector<bpattern> &b, int ngens);

std::vector<bpattern> get_unique_bpatterns(std::vector<bpattern> &res);

uint64_t print_memory_statistics();

}
