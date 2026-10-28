#include "gs_impl.h"

int main() {

    uint64_t total_epochs = 10;

    apg::GpuSearcher gs(0, 8192);

    for (int j = 0; j < total_epochs; j++) {

        auto vec = gs.pump("test", j);
        uint64_t x = vec.size();

        std::cout << "Out of 1000000 soups, " << x << " were deemed interesting" << std::endl;

    }

}
