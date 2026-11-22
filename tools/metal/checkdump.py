#!/usr/bin/env python3
"""Check gpuA's CPU reference against casim: DUMP holds generation 0 and generation G
of universe 0 (rows of 0/1, blank line between). Valid while the pattern stays
away from the torus edge (G small)."""
import sys
import numpy as np
sys.path.insert(0, "/Users/leo/Desktop/Life tools/Vibecoded life tools/casim")
import casim as C

dump, rule, G = sys.argv[1], sys.argv[2], int(sys.argv[3])
a, b = open(dump).read().strip().split('\n\n')
g0 = np.array([[int(c) for c in row] for row in a.split()], dtype=np.uint8)
gG = np.array([[int(c) for c in row] for row in b.split()], dtype=np.uint8)
N = g0.shape[0]
ys, xs = np.nonzero(g0)
p = C.Pattern.from_coords(list(zip(xs.tolist(), ys.tolist())), rule)
p.step(G)
got = {(x, y) for y, x in zip(*np.nonzero(gG))}
want = set(p.coords())
assert all(0 < x < N - 1 and 0 < y < N - 1 for x, y in want), 'pattern reached the torus edge; use fewer gens'
print('casim vs gpuA CPU reference after %d gens: %s (%d cells)' % (G, 'ok' if got == want else 'DIFF', len(want)))
sys.exit(0 if got == want else 1)
