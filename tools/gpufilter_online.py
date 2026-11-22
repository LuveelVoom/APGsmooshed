#!/usr/bin/env python3
"""Simulate on-the-fly learning of a per-tile-period GPU filter on SOUP logs.

Log format (stderr of an APG_SOUPLOG build): SOUP <suffix> <us> <code>:<n> ...

Warm-up: the first WARM soups (env WARMS=2000,20000) are censused fully; objects seen in >= 0.1% of
them are admitted (oscillator periods as lcm(p, 6) <= CAP, ships to the ship
table). Then soups stream in order: a soup is discarded if every object is
admitted (ship in table, or oscillator whose period divides a tile period),
else forwarded and censused by the CPU. Objects seen in M forwarded soups get
admitted (applied after each epoch of EPOCH soups, like a GPU batch).
yl/zz/PATHOLOGICAL/... are never admitted.

Reported per (WARM, M):
  bound  = CPU time of all streamed soups / CPU time of forwarded ones
  last40 = same over the last 40% of the log (after learning settled)
  missed = distinct objects in the log never censused (warm-up + forwarded)
  missed_full = of those, objects a full CPU census of the log sees >= 2 times
"""
import re, sys, math
from collections import Counter

CAP = 1200
EPOCH = 1000
WARMS = tuple(int(w) for w in __import__('os').environ.get('WARMS', '2000').split(','))


def kind(code):
    m = re.match(r'x([spq])(\d+)_', code)
    if not m:
        return ('other', None)
    if m.group(1) == 's':
        return ('osc', 1)
    if m.group(1) == 'p':
        return ('osc', int(m.group(2)))
    return ('ship', int(m.group(2)))


def lcm(a, b):
    return a * b // math.gcd(a, b)


def load(path):
    soups = []
    for line in open(path):
        if line.startswith('SOUP '):
            f = line.split()
            soups.append((int(f[2]), [c.rsplit(':', 1)[0] for c in f[3:]]))
    return soups


def simulate(soups, warm, m):
    tset, ships = {6}, set()

    def admit(c):
        k = kind(c)
        if k[0] == 'ship':
            ships.add(c)
        elif k[0] == 'osc' and lcm(k[1], 6) <= CAP:
            tset.add(lcm(k[1], 6))

    def ok(c):
        k = kind(c)
        return c in ships or (k[0] == 'osc' and any(t % k[1] == 0 for t in tset))

    censused = Counter()
    for _, codes in soups[:warm]:
        censused.update(set(codes))
    for c, n in censused.items():
        if n >= 0.001 * warm:
            admit(c)

    tot = fwd = tot40 = fwd40 = nfwd = 0
    start40 = int(len(soups) * 0.6)
    pending = []
    for i in range(warm, len(soups)):
        us, codes = soups[i]
        tot += us
        if i >= start40:
            tot40 += us
        if not all(ok(c) for c in codes):
            fwd += us
            nfwd += 1
            if i >= start40:
                fwd40 += us
            for c in set(codes):
                censused[c] += 1
                if censused[c] >= m and not ok(c):
                    pending.append(c)
        if (i - warm + 1) % EPOCH == 0:
            for c in pending:
                admit(c)
            pending = []
    return tot / max(fwd, 1), tot40 / max(fwd40, 1), 100.0 * nfwd / (len(soups) - warm), censused, tset, ships


def main(path):
    soups = load(path)
    full = Counter()
    for _, codes in soups:
        full.update(set(codes))
    print('%s: %d soups, %d distinct objects' % (path.split('/')[-1], len(soups), len(full)))
    print('  %-6s %-4s %8s %8s %6s %7s %12s  %s' % ('warm', 'M', 'bound', 'last40', 'fwd%', 'missed', 'missed(>=2)', 'tile periods'))
    for warm in WARMS:
        if warm >= 0.6 * len(soups):
            print('  warm-up %d: log too short' % warm)
            continue
        for m in (None, 20, 5, 2):
            mm = m if m else 10 ** 9
            b, b40, f, cens, tset, ships = simulate(soups, warm, mm)
            missed = [c for c in full if c not in cens]
            missed2 = [c for c in missed if full[c] >= 2]
            print('  %-6d %-4s %7.0fx %7.0fx %6.2f %7d %12d  %s, %d ships'
                  % (warm, m if m else 'off', b, b40, f, len(missed), len(missed2),
                     sorted(tset), len(ships)))
            if m == 5:
                ex = sorted(missed2, key=lambda c: -full[c])[:6]
                print('         missed with M=5, most frequent in full census: %s'
                      % ', '.join('%s (%d)' % (c, full[c]) for c in ex))


for p in sys.argv[1:]:
    main(p)
