#!/usr/bin/env python3
"""Estimate GPU work saved by skipping inactive tiles, on real soups.

Each soup: 16x16 random (density 1/2) at the centre of a B x B box whose
outermost cell ring is cleared every generation (absorbing edge, so ships
leave). A tile (T x T) must be stepped in a generation if any cell changed
in the previous generation inside the tile or within one cell of it.
The box is 'settled' when its state equals the state L generations earlier
(checked every L generations); work stops there (early exit) or at CAP.

usage: activetiles.py TABLEFILE L [B T SOUPS CAP]
"""
import sys
import numpy as np

tab = np.array([int(c) for c in open(sys.argv[1]).read().strip()], np.uint8)
L = int(sys.argv[2])
B = int(sys.argv[3]) if len(sys.argv) > 3 else 128
T = int(sys.argv[4]) if len(sys.argv) > 4 else 32
NS = int(sys.argv[5]) if len(sys.argv) > 5 else 400
CAP = int(sys.argv[6]) if len(sys.argv) > 6 else 6000
rng = np.random.default_rng(1)

a = np.zeros((NS, B, B), np.uint8)
o = B // 2 - 8
a[:, o:o + 16, o:o + 16] = rng.integers(0, 2, (NS, 16, 16), dtype=np.uint8)
nt = B // T


def step(a):
    # MAP index: NW 256, N 128, NE 64, W 32, C 16, E 8, SW 4, S 2, SE 1 (y-1 = north)
    p = np.pad(a, ((0, 0), (1, 1), (1, 1)))
    idx = np.zeros(a.shape, np.int32)
    w = 256
    for dy in (0, 1, 2):
        for dx in (0, 1, 2):
            idx += p[:, dy:dy + B, dx:dx + B].astype(np.int32) * w
            w >>= 1
    b = tab[idx]
    b[:, 0, :] = 0; b[:, -1, :] = 0; b[:, :, 0] = 0; b[:, :, -1] = 0
    return b


alive = np.ones(NS, bool)
settled_at = np.full(NS, CAP)
work_full = np.zeros(NS)
work_skip = np.zeros(NS)
snap = a.copy()
changed = np.ones((NS, B, B), bool)
for g in range(1, CAP + 1):
    # tiles needing a step this generation: change in previous gen within tile + 1-cell halo
    c = np.pad(changed, ((0, 0), (1, 1), (1, 1)))
    halo = c[:, 1:-1, 1:-1] | c[:, :-2, 1:-1] | c[:, 2:, 1:-1] | c[:, 1:-1, :-2] | c[:, 1:-1, 2:]
    halo |= c[:, :-2, :-2] | c[:, :-2, 2:] | c[:, 2:, :-2] | c[:, 2:, 2:]
    act = halo.reshape(NS, nt, T, nt, T).any(axis=(2, 4)).sum(axis=(1, 2))
    work_full += alive * nt * nt
    work_skip += alive * act
    b = step(a)
    changed = b != a
    a = b
    if g % L == 0:
        eq = (a == snap).all(axis=(1, 2))
        newly = alive & eq
        settled_at[newly] = g
        alive &= ~eq
        snap = a.copy()
        if not alive.any():
            break

n_cap = int((settled_at >= CAP).sum())
s = np.sort(settled_at)
print('B=%d T=%d L=%d soups=%d: settle gens mean %.0f median %d p90 %d; %d hit cap %d'
      % (B, T, L, NS, settled_at.mean(), s[NS // 2], s[int(.9 * NS)], n_cap, CAP))
print('  work with tile skipping / without: %.3f  (=> %.1fx less stepping)'
      % (work_skip.sum() / work_full.sum(), work_full.sum() / work_skip.sum()))
fit = settled_at < CAP
print('  excluding capped soups: %.3f (%.1fx)' % (work_skip[fit].sum() / work_full[fit].sum(),
                                              work_full[fit].sum() / work_skip[fit].sum()))
