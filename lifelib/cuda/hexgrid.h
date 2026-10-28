
#include <stdint.h>
#include <string.h>
#include <stdio.h>

void prepare_topology(uint64_t *topology, int verbose) {

    memset(topology, 0, 128 * sizeof(uint64_t));

    int x[256];
    memset(x, 0, 256 * sizeof(int));
    x[0x88] = 1;

    int y[256];
    memset(y, 0, 256 * sizeof(int));
    y[1] = 0x88;

    int directions[7] = {1, -16, -17, -1, 16, 17, 0};
    int residues[3] = {3, 4, 2};

    for (int l = 0; l < 6; l++) {
        for (int i = 0; i < 256; i++) {
            if (x[i] != 0) { continue; }
            for (int j = 0; j < 6; j++) {
                if (x[(i + directions[j]) & 255] > 0) { x[i] = -1; break; }
            }
        }
        for (int i = 0; i < 256; i++) {
            if (x[i] != -1) { continue; }
            int j = residues[i % 3];
            residues[i % 3] += 3;
            x[i] = j;
            if (j < 128) { y[j] = i; y[j+128] = l; }
        }
    }

    for (int style = 0; style < 2; style++) {

    if (verbose && (style == 0)) { printf("Universe sizes:\n"); }
    if (verbose && (style == 1)) { printf("Escaping spaceship detection:\n"); }

    int rs = 0;
    for (int i = 0; i < 256; i++) {
        if (verbose && ((i & 15) == 0)) {
            for (int j = 0; j < 15 - (i >> 4); j++) { printf("  "); }
        }
        if ((x[i] > 0) && (x[i] < 128)) {
            if (verbose) { while (rs --> 0) { printf("    "); } }

            int py = (i >> 4) - 8;
            int px = 2 * ((i & 15) - 8) - py;
            int l = y[x[i] + 128];

            uint64_t sig = 0;

            if ((l >= 1) && (l != 3)) {
                sig = 36;
                if ((py * py >= 10 * px * px) && (l != 2)) {
                    sig += ((py > 0) ? -16 : 16);
                    if (verbose && (style == 1)) { printf("\033[31;1m"); }
                } else if (px * px >= 182 * py * py) {
                    sig += ((px > 0) ? -2 : 2);
                    if (verbose && (style == 1)) { printf("\033[34;1m"); }
                } else {
                    sig += ((py > 0) ? -8 : 8);
                    sig += ((px > 0) ? -1 : 1);
                    if (verbose && (style == 1)) { printf("\033[32;1m"); }
                }
            }

            uint64_t unisize = (0x322100 >> (l * 4)) & 3;
            sig += (unisize << 6);

            if (verbose && (style == 0)) {
                if (unisize == 3) { printf("\033[34;1m"); }
                if (unisize == 2) { printf("\033[32;1m"); }
                if (unisize == 1) { printf("\033[33;1m"); }
                if (unisize == 0) { printf("\033[1m"); }
            }

            sig = (sig << 2);

            int j = 6;
            while (j --> 0) {
                sig = (sig << 1) | ((x[(i + directions[j]) & 255] == 0) ? 1 : 0);
            }
                j = 6;
            while (j --> 0) {
                sig = (sig << 8) | x[(i + directions[j]) & 255];
            }

            topology[x[i]] = sig;
            if (verbose) { printf("%4d\033[0m", x[i]); }
        } else {
            rs += 1;
        }
        if (verbose && ((i & 15) == 15)) { printf("\n"); rs = 0; }
    }

    }

}
