#include "hexgrid.h"

int main() {

    uint64_t topology[128];
    prepare_topology(topology, 1);

    printf("Topology:\n\n");

    for (int i = 0; i < 128; i++) {
        printf("%4d: %016llx\n", i, topology[i]);
    }

    return 0;

}
