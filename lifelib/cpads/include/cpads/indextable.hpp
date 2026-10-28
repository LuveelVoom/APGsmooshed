#pragma once
#include "ivector.hpp"
#include <utility>
#include <cstring>

namespace hh {

/**
 * Function for reducing a 64-bit hash into a log2(modulus)-bit hash,
 * where modulus is a power of two.
 */
struct DyadicHashReducer {

    uint64_t shiftamt;

    void resize(uint64_t modulus) {
        // This operation _might_ be slow (e.g. if there is no
        // hardware support for CTZ), so we perform this once
        // when we first create or later resize the table.
        shiftamt = 64 - hh::ctz64(modulus);
    }

    explicit DyadicHashReducer(uint64_t modulus) {
        // Compute the shift amount:
        resize(modulus);
    }

    /**
     * Compute the next (power-of-two) size beyond the current size.
     */
    uint64_t nextsize() const {
        return 1ull << (65 - shiftamt);
    }

    /**
     * Takes a 64-bit integer and reduces to the range [0, modulus-1].
     *
     * We apply a bit-mixing permutation and then take the highest
     * log2(modulus) bits by right-shifting the result.
     */
    uint64_t reduce(uint64_t full_hash) const {
        return fibmix(full_hash) >> shiftamt;
    }

};


namespace {

template<class K>
_HI_ uint64_t get_unreduced_hash(const K& key, std::false_type) {
    return key.hash();
}

template<class K>
_HI_ uint64_t get_unreduced_hash(const K& key, std::true_type) {
    return ((uint64_t) key);
}

template<class K>
_HI_ bool is_zero_key(const K& key, std::false_type) {
    return key.iszero();
}

template<class K>
_HI_ bool is_zero_key(const K& key, std::true_type) {
    return (key == 0);
}

} // anonymous namespace


/**
 * Very flexible generalised hashtable.
 * 
 * Entries are stored in blocks of cache-aligned memory and never moved.
 * Integer keys have the identity hash (or, rather, the cast-to-uint64
 * hash); the hashtable itself applies a bit-mixing and truncation
 * function.
 *
 * T = entry type
 * K = key type (either integral type or must support .hash() and .iszero())
 * MostDerivedClass = curiously recurrent template parameter
 * B = log2(entries_per_allocation)
 */
template<typename T, typename K, typename MostDerivedClass, int B = hh::suggest_lowbits(sizeof(T))>
class indextable {

    using I = decltype(T::next);

    I freenodes;
    I totalnodes;
    I sentinels;
    I gccounter;

    DyadicHashReducer hr;
    std::vector<I> hashtable;

public:

    ivector<T, B> contents;

    size_t get_hashsize() const {
        return hashtable.size();
    }

    size_t size() const {
        return totalnodes;
    }

    uint64_t total_bytes() const {
        uint64_t nodemem = sizeof(T) * totalnodes;
        uint64_t hashmem = sizeof(I) * hashtable.size();
        return nodemem + hashmem;
    }

    /// index --> entry
    INHERIT_ACCESSORS_FROM(T, contents, _HI_)

    /// entry --> key
    _HI_ K obtain_key(const T& element) const {
        return (static_cast<const MostDerivedClass&>(*this)).compute_key(element);
    }

    /// key --> index
    _HI_ I find(const K &key) const {

        if (is_zero_key(key, std::is_integral<K>())) { return ((I) 0); }
        uint64_t h = get_reduced_hash(key);
        I p = hashtable[h];
        while (p) {
            const T* pptr = &(contents[p]);
            if (obtain_key(*pptr) == key) { return p; }
            p = pptr->next;
        }
        return ((I) -1);
    }

    template<typename S, typename FnMatch, typename FnDefault>
    S lookupkey(const K &key, const S &null_value, FnMatch lambda_match, FnDefault lambda_default) {

        if (is_zero_key(key, std::is_integral<K>())) { return null_value; }
        uint64_t h = get_reduced_hash(key);

        // iterate over hash bucket:
        I p = hashtable[h];
        T* predptr = nullptr;
        while (p) {
            T* pptr = &(contents[p]);
            if (obtain_key(*pptr) == key) {
                return lambda_match(p, h, predptr, pptr);
            }
            predptr = pptr;
            p = pptr->next;
        }

        // not in the hashtable
        return lambda_default(h);
    }

    /// non-const implementation of key --> index (move-to-front)
    I find(const K& key) {

        return lookupkey(key, (I) 0,
            [&](I p, uint64_t h, T* predptr, T* pptr) {
                if (predptr) {
                    // Move this node to the front:
                    predptr->next = pptr->next;
                    pptr->next = hashtable[h];
                    hashtable[h] = p;
                }
                return p;
            }, [](uint64_t /* dummy */ ) { return ((I) -1); });
    }

    explicit indextable(I n_sentinels, uint64_t hashsize = (1ull << B)) :
        freenodes(0), totalnodes(0), sentinels(n_sentinels), gccounter(0), hr(hashsize), hashtable(hashsize, 0) {

        create_sentinels();
    }

    void create_sentinels() {
        freenodes = 0; totalnodes = 0;
        for (I i = 0; i < sentinels; i++) { newnode(); }
    }

    void clear() {
        contents.clear();
        for (uint64_t i = 0; i < hashtable.size(); i++) {
            hashtable[i] = 0;
        }
        create_sentinels();
    }

    I newnode() {
        if (freenodes == 0) { freenodes = contents.newnode(); }
        I thisnode = freenodes;
        freenodes = contents[thisnode].next;
        totalnodes += 1;
        return thisnode;
    }

    uint64_t get_reduced_hash(const K& key) const {
        uint64_t fullhash = get_unreduced_hash(key, std::is_integral<K>());
        return hr.reduce(fullhash);
    }

    /**
     * We use hash chaining, so the container will continue to work even
     * when the load ratio `((double) nodes.size()) / hashtable.size()`
     * grows larger than 1. However, large load ratios will result in
     * lots of pointer indirection before finding the desired element,
     * so we should grow the hashtable to maintain a low load ratio.
     */
    void resize_hash(uint64_t newsize) {

        if (newsize == hashtable.size()) {
            // nothing to do here:
            return;
        }

        // Create a new hashtable:
        std::vector<I> newtable(newsize, 0);
        hr.resize(newsize);

        // Based on code by Tom Rokicki:
        for (uint64_t i = 0; i < hashtable.size(); i++) {
            I p = hashtable[i];
            while (p) {
                T& element = contents[p];
                I np = element.next;
                uint64_t h = get_reduced_hash(obtain_key(element));
                element.next = newtable[h];
                newtable[h] = p;
                p = np; // contrary to the beliefs of most complexity theorists.
            }
        }

        hashtable.swap(newtable);
    }

    /**
     * Maintain a load ratio between 25% and 50%.
     */
    void resize_if_necessary() {
        if (2*totalnodes > hashtable.size()) {
            resize_hash(hr.nextsize());
        }
    }

    void update_entry_same_key(I idx, const T& entry) {

        I next = contents[idx].next;
        contents[idx] = entry;
        contents[idx].next = next;
    }

    template<typename FnExists, typename FnCreate>
    I insert_entry(const T& entry, FnExists lambda_exists, FnCreate lambda_create) {

        return lookupkey(obtain_key(entry), (I) 0,

            [&](I p, uint64_t h, T* predptr, T* pptr) {
                if (predptr) {
                    // Move this node to the front:
                    predptr->next = pptr->next;
                    pptr->next = hashtable[h];
                    hashtable[h] = p;
                }

                lambda_exists(p);
                return p;
            },

            [&](uint64_t h) {
                I p = newnode();
                contents[p].next = hashtable[h];
                hashtable[h] = p;
                update_entry_same_key(p, entry);
                resize_if_necessary();

                lambda_create(p);
                return p;
            }
        );
    }

    I insert(const T& entry, bool overwrite = false) {

        return insert_entry(entry, [&](I p) {
                if (overwrite) {
                    update_entry_same_key(p, entry);
                }
            }, [&](I /* dummy */ ){});
    }

    template<typename Fn>
    bool fancy_erase(const K &key, Fn lambda) {

        return lookupkey(key, false,

            [&](I p, uint64_t h, T* predptr, T* pptr) {

                // Remove from hashtable:
                if (predptr) {
                    predptr->next = pptr->next;
                } else {
                    hashtable[h] = pptr->next;
                }

                lambda(p, pptr);
                return true;
            },

            [&](uint64_t h) {
                (void) h;
                return false;
            }
        );
    }

    bool erasenode(const K &key) {

        return fancy_erase(key, [&](I p, T* pptr) {
            // Reset memory:
            std::memset(pptr, 0, sizeof(T));

            // Prepend to list of free nodes:
            pptr->next = freenodes;
            freenodes = p;

            // We've reduced the total number of nodes:
            totalnodes -= 1;
        });
    }

    void update_entry_inplace(I idx, const T& new_entry) {

        T& old_entry = contents[idx];

        K old_key = obtain_key(old_entry);
        K new_key = obtain_key(new_entry);

        uint64_t h1 = get_reduced_hash(old_key);
        uint64_t h2 = get_reduced_hash(new_key);

        if (new_key.iszero()) {
            // just delete the entry:
            erasenode(old_key);
        } else if (h1 == h2) {
            // easy option: no need to traverse hash links:
            update_entry_same_key(idx, new_entry);
        } else {
            // more complicated solution:

            // find predecessor in old hashtable bucket:
            T* predptr = nullptr;
            {
                I p = hashtable[h1];
                while (p != idx) {
                    predptr = &(contents[p]);
                    p = predptr->next;
                }
            }

            // remove from old hashtable bucket:
            if (predptr) {
                predptr->next = old_entry.next;
            } else {
                hashtable[h1] = old_entry.next;
            }

            // change in situ:
            old_entry = new_entry;

            // reintroduce into hashtable:
            old_entry.next = hashtable[h2];
            hashtable[h2] = idx;
        }
    }

    template<bool DeleteUnmarked>
    void gc_traverse() {
        for (uint64_t i = 0; i < hashtable.size(); i++) {
            I p = hashtable[i];
            T* predptr = nullptr;

            while (p) {
                T* pptr = &(contents[p]);
                I np = pptr->next;
                if (DeleteUnmarked && (pptr->gcflags == 0)) {
                    // Remove from hashtable:
                    if (predptr) {
                        predptr->next = np;
                    } else {
                        hashtable[i] = np;
                    }

                    // Reset memory:
                    std::memset(pptr, 0, sizeof(T));

                    // Prepend to list of free nodes:
                    pptr->next = freenodes;
                    freenodes = p;

                    // We've reduced the total number of nodes:
                    totalnodes -= 1;
                } else {

                    // Node still exists; zero the flags:
                    pptr->gcflags = 0;
                    predptr = pptr;
                }
                // Contrary to the belief of most complexity theorists:
                p = np;
            }
        }

        gccounter = 0;
    }

    I increment_gccounter() {
        return (++gccounter);
    }

};

} // namespace hh
