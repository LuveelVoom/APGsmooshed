/*
* (key, index, value) hashtables that can be addressed by either the
* key (like a regular associative array) or the (typically 32-bit)
* index integer.
*/

#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <vector>
#include <cstring>
#include "numtheory.h"
#include <iostream>
#include <unordered_map>

#include "../cpads/include/cpads/indextable.hpp"


namespace apg {

template <typename K, typename I, typename V>
struct kiventry {

    K key;
    I next;
    I gcflags; // For gc and ensuring nice standard layout.
    V value;

};

    template <typename K, typename V>
    class indirected_map {
        /*
        * Similar to an unordered_map, but the values are contiguous in
        * memory instead of being adjacent to their keys.
        */

        public:

        hh::ivector<V, 5> elements;
        std::unordered_map<K, V*> hashtable;

        V& operator[](K key) {

            V** pointer_to_pointer = &(hashtable[key]);
            if (*pointer_to_pointer == 0) {
                *pointer_to_pointer = &(elements[elements.newnode()]);
            }
            return **pointer_to_pointer;

        }

        uint64_t size() const { return elements.size(); }

    };

    template <typename K, typename I, typename V>
    struct kivtable : public hh::indextable<kiventry<K, I, V>, K, kivtable<K, I, V>, 10> {

        K compute_key(const kiventry<K, I, V> &entry) const {
            return entry.key;
        }

        // Get node index from key:
        I getnode(const K &key, bool makenew) {

            if (makenew) {
                kiventry<K, I, V> blank;
                memset(&blank, 0, sizeof(blank));
                blank.key = key;
                return this->insert(blank, false);
            } else {
                return this->find(key);
            }
        }

        kiventry<K, I, V>* ind2ptr(I index) { return &(this->contents[index]); }

        // Create a (key, value) pair and return index:
        I setnode(const K &key, const V &value) {

            kiventry<K, I, V> blank;
            memset(&blank, 0, sizeof(blank));
            blank.key = key;
            blank.value = value;

            return this->insert(blank, true);
        }

        explicit kivtable() : hh::indextable<kiventry<K, I, V>, K, kivtable<K, I, V>, 10>(1, 4096) { }

    };

    /*
    template <typename K, typename V>
    using kivtable32 = kivtable<K, uint32_t, V>;

    template <typename K, typename V>
    using kivtable64 = kivtable<K, uint64_t, V>;
    */

}

