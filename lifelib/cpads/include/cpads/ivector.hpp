#pragma once
#include "memory.hpp"
#include <vector>

namespace hh {

/**
 * An 'indirected vector'. This relaxes the requirement that all of
 * the elements in a vector are contiguous, instead allocating
 * contiguous blocks of 2**B elements. This enables the following
 * benefits compared with a std::vector:
 *
 * -- elements never need to be relocated (so pointers are not invalidated);
 * -- the extra memory overhead from repeated appending is small.
 *
 * This class is recommended for backing very large data structures
 * that could use a reasonable fraction of available memory.
 */
template <typename T, int B = hh::suggest_lowbits(sizeof(T))>
class ivector {

    constexpr static size_t lowmask = (1ull << B) - 1;
    std::vector<T*> arraylist;
    size_t totalnodes = 0;

    void newarray() {
        T* nextarray = (T*) zalloc(sizeof(T) << B);
        arraylist.push_back(nextarray);
    }

    void poparray() {
        zfree(arraylist.back());
        arraylist.pop_back();
    }

public:

    struct iterator {
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        std::pair<T**, size_t> c; // inherit total ordering

        iterator(T** arrl, size_t idx) : c(arrl, idx) { }

        reference operator*() const { return c.first[c.second >> B][c.second & lowmask]; }
        pointer operator->() const { return &(c.first[c.second >> B][c.second & lowmask]); }

        iterator& operator++() { c.second += 1; return (*this); }
        iterator& operator+=(const size_t &x) { c.second += x; return (*this); }
        iterator& operator--() { c.second -= 1; return (*this); }
        iterator& operator-=(const size_t &x) { c.second -= x; return (*this); }
        iterator operator+(const size_t &x) const { return iterator(c.first, c.second + x); }
        iterator operator-(const size_t &x) const { return iterator(c.first, c.second - x); }

        INHERIT_COMPARATORS_FROM(iterator, c, _HI_)

        difference_type operator-(const iterator& rhs) const { return ((difference_type) c.second) - ((difference_type) rhs.c.second); }
    };

    static_assert(sizeof(iterator) == 16, "ivector::iterator should be 16 bytes");

    iterator begin() { return iterator(&(arraylist[0]), 0); }
    iterator end() { return iterator(&(arraylist[0]), totalnodes); }

    // accessing elements:

    T& operator[](size_t i) {
        return arraylist[i >> B][i & lowmask];
    }

    const T& operator[](size_t i) const {
        return arraylist[i >> B][i & lowmask];
    }

    size_t size() const {
        return totalnodes;
    }

    // change number of elements:

    void resize(size_t desired_size) {
        size_t desired_arrays = (desired_size + lowmask) >> B;
        while (arraylist.size() > desired_arrays) { poparray(); }
        while (arraylist.size() < desired_arrays) { newarray(); }
        totalnodes = desired_size;
    }

    size_t newnode() {
        if ((totalnodes & lowmask) == 0) { newarray(); }
        return (totalnodes++);
    }

    void push_back(const T& elem) {
        auto x = newnode();
        (*this)[x] = elem;
    }

    T pop_back() {
        totalnodes--;
        T x = (*this)[totalnodes];
        if ((totalnodes & lowmask) == 0) { poparray(); }
        return x;
    }

    // constructor and destructor:

    explicit ivector() = default;

    void clear() {
        while (!arraylist.empty()) { poparray(); }
    }

    ~ivector() {
        clear();
    }

    // disable copy constructor:

    ivector(ivector<T, B> const&) = delete;
    ivector<T, B>& operator=(ivector<T, B> const&) = delete;

};

} // namespace hh
