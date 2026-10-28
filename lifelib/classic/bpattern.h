#pragma once
#include <stdint.h>

namespace apg {

struct bpattern {

    uint64_t x[64];

    bool operator<(const bpattern &other) {
        for (int i = 0; i < 64; i++) {
            if (x[i] < other.x[i]) { return true; }
            if (x[i] > other.x[i]) { return false; }
        }
        return false;
    }

    bool operator==(const bpattern &other) {
        for (int i = 0; i < 64; i++) {
            if (x[i] != other.x[i]) { return false; }
        }
        return true;
    }

};

}
