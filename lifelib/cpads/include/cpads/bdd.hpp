#pragma once
#include "indextable.hpp"

namespace hh {

/**
 * Used for memoizing the results of bitwise operations, quantification, and
 * probability measurements.
 */
struct MemoCacheEntry {
    uint32_t op;
    uint32_t k1;
    uint32_t k2;
    uint32_t res;

    uint64_t hash() const {
        uint64_t h = op * 6364136223846793005ull + k1;
        if ((op & 0x80000000u) == 0) { h += ((uint64_t) k2) << 32; }
        return h;
    }

    bool operator==(const MemoCacheEntry &other) const {
        if ((op != other.op) || (k1 != other.k1)) {
            return false;
        }
        return (op & 0x80000000u) || (k2 == other.k2);
    }
};

static_assert(sizeof(MemoCacheEntry) == 16, "MemoCacheEntry should be 16 bytes");


/**
 * A container of MemoCacheEntry objects. We use the same hashing scheme as
 * in hh::indextable; however, we follow Knuth and only use a LRU cache with
 * no collision resolution.
 */
struct MemoCache {

    DyadicHashReducer hr;
    std::vector<MemoCacheEntry> hashtable;

    explicit MemoCache(uint32_t hashsize) : hr(hashsize), hashtable(hashsize) { }

    bool lookup(MemoCacheEntry &x) const {
        uint64_t h = x.hash();
        uint32_t idx = hr.reduce(h);
        const MemoCacheEntry &memo = hashtable[idx];
        if (memo == x) {
            x = memo; return true;
        } else {
            return false;
        }
    }

    void store(const MemoCacheEntry &x) {
        uint64_t h = x.hash();
        uint32_t idx = hr.reduce(h);
        hashtable[idx] = x;
    }

    void resize_hash(uint64_t newsize) {

        if (newsize == hashtable.size()) {
            // nothing to do here:
            return;
        }

        // Create a new hashtable:
        std::vector<MemoCacheEntry> newtable(newsize);
        hr.resize(newsize);

        for (auto it = hashtable.begin(); it != hashtable.end(); ++it) {
            if (it->op) {
                uint32_t idx = hr.reduce(it->hash());
                newtable[idx] = *it;
            }
        }

        hashtable.swap(newtable);
    }
};



struct BDDKey {

    uint32_t false_idx;
    uint32_t true_idx;
    uint32_t variable;

    uint64_t hash() const {
        uint64_t h = variable;
        h -= (h << 31);
        h += false_idx;
        h -= (h << 31);
        h += true_idx;
        return h;
    }

    bool iszero() const {
        return (false_idx == 0) && (true_idx == 0);
    }

    bool operator==(const BDDKey &other) const {
        return (variable == other.variable) && (false_idx == other.false_idx) && (true_idx == other.true_idx);
    }
};

struct BDDEntry {

    BDDKey key;
    uint32_t next;

};

static_assert(sizeof(BDDEntry) == 16, "BDDEntry should occupy 16 bytes");

class BDDBase : public indextable<BDDEntry, BDDKey, BDDBase> {

public:

    BDDBase() : indextable<BDDEntry, BDDKey, BDDBase>(1) { }

    auto compute_key(const BDDEntry &element) const {
        return element.key;
    }

    uint32_t assemble_node_inner(uint32_t lo, uint32_t hi, uint32_t variable) {

        if (lo == hi) { return lo; }

        BDDEntry entry;
        entry.key.false_idx = lo;
        entry.key.true_idx  = hi;
        entry.key.variable  = variable;
        entry.next = 0;

        uint32_t residx = insert_entry(entry,
            [](uint32_t /* dummy */) { },
            [&](uint32_t /* dummy */) { }
        );

        return (residx << 1);
    }

    uint32_t assemble_node(uint32_t lo, uint32_t hi, uint32_t variable) {

        uint32_t perturbation = lo & 1;
        uint32_t q = assemble_node_inner(lo ^ perturbation, hi ^ perturbation, variable);
        return q ^ perturbation;
    }

    BDDEntry retrieve(uint32_t idx) const {

        uint32_t perturbation = idx & 1;
        BDDEntry entry = contents[idx >> 1];
        entry.key.false_idx ^= perturbation;
        entry.key.true_idx ^= perturbation;

        return entry;
    }

};


/**
 * Binary decision diagram container.
 *
 * Implemented using 'complemented edges' to allow constant-time negation.
 * Supports up to 2^31 nodes (including the zero function).
 */
class BDD {

    BDDBase base;
    MemoCache mc;

    void store_resize(const MemoCacheEntry &x) {
        mc.store(x);

        // Ideal memo cache size:
        size_t imcs = base.get_hashsize() >> 3;
        if (imcs > mc.hashtable.size()) {
            mc.resize_hash(imcs);
        }
    }

    uint64_t get_prob_inner(uint32_t a) {
        if (a == 0) { return 0; }
        MemoCacheEntry x{0x80000000u, a, 0, 0};
        if (!(mc.lookup(x))) {
            auto a_entry = base.retrieve(a);
            uint64_t lowprob = get_prob_inner(a_entry.key.false_idx &~ 1) >> 1;
            uint64_t highprob = get_prob_inner(a_entry.key.true_idx &~ 1) >> 1;
            if (a_entry.key.false_idx & 1) { lowprob = 0x8000000000000000ull - lowprob; }
            if (a_entry.key.true_idx & 1) { highprob = 0x8000000000000000ull - highprob; }
            uint64_t p = lowprob + highprob;
            x.k2 = ((uint32_t) p);
            x.res = ((uint32_t) (p >> 32));
            store_resize(x);
        }
        return x.k2 + (((uint64_t) x.res) << 32);
    }

    uint32_t apply_gate_inner(uint32_t lhs, uint32_t rhs, uint32_t truth_table) {

        uint32_t a = lhs;
        uint32_t b = rhs;
        uint32_t t = truth_table & 15;

        if (a & 1) { a ^= 1; t = ((t & 0xa) >> 1) | ((t & 0x5) << 1); }
        if (b & 1) { b ^= 1; t = ((t & 0xc) >> 2) | ((t & 0x3) << 2); }

        // negate truth table if necessary:
        uint32_t p = (t >> 3); if (p) { t ^= 0xf; }

        if (a > b) {
            uint32_t c = a; a = b; b = c;
            t = (t & 1) | ((t & 4) >> 1) | ((t & 2) << 1);
        }

        MemoCacheEntry x{t, a, b, 0};

        if (a == 0) {
            x.res = (t & 2) ? b : 0;
        } else if (b == 0) {
            x.res = (t & 4) ? a : 0;
        } else if (a == b) {
            x.res = (t & 1) ? a : 0;
        } else {
            // interesting case:

            if (!(mc.lookup(x))) {
                
                auto a_entry = base.retrieve(a);
                auto b_entry = base.retrieve(b);

                uint32_t lo;
                uint32_t hi;
                uint32_t variable;

                if (a_entry.key.variable > b_entry.key.variable) {
                    lo = apply_gate_inner(a_entry.key.false_idx, b, t);
                    hi = apply_gate_inner(a_entry.key.true_idx, b, t);
                    variable = a_entry.key.variable;
                } else if (a_entry.key.variable < b_entry.key.variable) {
                    lo = apply_gate_inner(a, b_entry.key.false_idx, t);
                    hi = apply_gate_inner(a, b_entry.key.true_idx, t);
                    variable = b_entry.key.variable;
                } else {
                    lo = apply_gate_inner(a_entry.key.false_idx, b_entry.key.false_idx, t);
                    hi = apply_gate_inner(a_entry.key.true_idx, b_entry.key.true_idx, t);
                    variable = b_entry.key.variable;
                }

                x.res = base.assemble_node_inner(lo, hi, variable);
                store_resize(x);
            }
        }

        return x.res ^ p;
    }

    public:

    uint32_t apply_gate(uint32_t a, uint32_t b, uint32_t truth_table) {

        switch (truth_table) {
            case 0: return 0;
            case 3: return b;
            case 5: return a;
            case 10: return a ^ 1;
            case 12: return b ^ 1;
            case 15: return 1;
            default: return apply_gate_inner(a, b, truth_table);
        }
    }

    uint64_t get_prob(uint32_t a) {

        uint64_t p = get_prob_inner(a &~ 1);
        if (a & 1) { p = -p; }
        return p;

    }

    auto retrieve(uint32_t var) const {
        return base.retrieve(var);
    }

    uint32_t var2node(uint32_t var) {
        return base.assemble_node(0, 1, var);
    }

    // Basic logical functions:
    uint32_t and_nodes(uint32_t lhs, uint32_t rhs) { return apply_gate(lhs, rhs, 1); }
    uint32_t xor_nodes(uint32_t lhs, uint32_t rhs) { return apply_gate(lhs, rhs, 6); }
    uint32_t or_nodes(uint32_t lhs, uint32_t rhs) { return apply_gate(lhs, rhs, 7); }
    uint32_t negate_node(uint32_t idx) { return idx ^ 1; }

    BDD() : base(), mc(16384) { }

    uint32_t size() const { return base.size(); }

};


}
