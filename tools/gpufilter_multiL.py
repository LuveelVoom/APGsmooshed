#!/usr/bin/env python3
"""Score global-L vs per-tile-period GPU filters on per-soup census logs.

Log format (stderr of an APG_SOUPLOG build): SOUP <suffix> <us> <code>:<n> ...
Settings come from the first WARM_FRAC of each log, scores from the rest.
Common = seen in >= THRESH of warm-up soups. Common ships are deleted (allowed).

global L (cap C): add common periods by frequency while lcm(L, 6) stays <= C
    (the GPU compares at 6-generation step boundaries); an oscillator is
    allowed iff its period divides L.
per-tile (cap C): each tile may settle with its own period from the set
    {lcm(p, 6) : p common, lcm(p, 6) <= C} plus the global L; an oscillator is
    allowed iff its period divides some member. Optimistic: it assumes no tile
    holds two oscillators whose periods only fit different members.

bound = CPU time of all test soups / CPU time of forwarded ones (GPU free).
lost = test soups discarded although they contain a rare object (not common):
    those objects never reach the census.
"""
import re, sys, math
from collections import Counter

WARM_FRAC = 0.6
THRESH = 0.001
CAPS = (12, 60, 240, 1200)


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


def score(test, allowed, common):
    total = sum(us for us, _ in test)
    fwd_t = fwd_n = lost = 0
    for us, codes in test:
        if all(allowed(c) for c in codes):
            if any(c not in common for c in codes):
                lost += 1
        else:
            fwd_t += us
            fwd_n += 1
    b = ('%.0fx' % (total / fwd_t)) if fwd_t else 'inf'
    return b, 100.0 * fwd_n / len(test), lost


def main(path):
    soups = load(path)
    nw = int(len(soups) * WARM_FRAC)
    warm, test = soups[:nw], soups[nw:]
    seen = Counter()
    for _, codes in warm:
        seen.update(set(codes))
    common = {c for c, n in seen.items() if n >= THRESH * len(warm)}
    ships = {c for c in common if kind(c)[0] == 'ship'}
    pfreq = Counter()
    for c in common:
        k = kind(c)
        if k[0] == 'osc':
            pfreq[k[1]] += seen[c]
    print('%s: %d soups (%d test); common periods %s; common ships %d'
          % (path.split('/')[-1], len(soups), len(test),
             sorted(pfreq), len(ships)))
    print('  %-6s %-22s %-22s %s' % ('cap', 'global L: bound fwd% lost',
                                     'per-tile: bound fwd% lost', 'tile periods'))
    for cap in CAPS:
        L = 6
        for p, _ in pfreq.most_common():
            if lcm(L, p) <= cap:
                L = lcm(L, p)
        tset = sorted({lcm(p, 6) for p in pfreq if lcm(p, 6) <= cap} | {L})

        def g_ok(c, L=L):
            k = kind(c)
            return c in ships or (k[0] == 'osc' and L % k[1] == 0)

        def t_ok(c, tset=tset):
            k = kind(c)
            return c in ships or (k[0] == 'osc' and any(m % k[1] == 0 for m in tset))

        gb, gf, gl = score(test, g_ok, common)
        tb, tf, tl = score(test, t_ok, common)
        print('  %-6d L=%-4d %6s %5.2f %4d   %6s %5.2f %4d   %s'
              % (cap, L, gb, gf, gl, tb, tf, tl, tset))
    # What still blocks the widest per-tile filter:
    block = Counter()
    for _, codes in test:
        bad = {c for c in codes if not t_ok(c)}
        block.update(bad)
    print('  top blockers (per-tile, cap %d): %s' % (CAPS[-1], ', '.join(
        '%s (%d)' % kv for kv in block.most_common(5))))


for p in sys.argv[1:]:
    main(p)
