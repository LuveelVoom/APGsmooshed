#!/usr/bin/env python3
"""Score GPU discard filters on per-soup census logs (SOUP suffix us code:n ...).

A filter discards a soup (GPU keeps it off the CPU) or forwards it. Filters:
  fixed6   : current GPU logic for a rule without ship removal: discard iff
             every object is a still life or an oscillator of period | 6.
  learnedL : discard iff every object's period divides L and there are no
             ships; L = lcm of periods seen in >= THRESH of warm-up soups.
  learnedL+ships : as learnedL, but ships seen in >= THRESH of warm-up soups
             are deleted (allowed), like the GPU's glider/*WSS removal.
  ideal    : discard iff every object is common (seen in >= THRESH of warm-up
             soups); upper bound for an object-based filter.
Warm-up = first WARM soups; scores are on the remaining soups.
Speedup bound = CPU time of all soups / CPU time of forwarded soups (GPU free).
"""
import re, sys, math
from collections import Counter

WARM = 2000
THRESH = 0.005

def kind(code):
    m = re.match(r'x([spq])(\d+)_', code)
    if not m:
        return ('other', None)          # yl (growth), zz (pathological), ov, ...
    if m.group(1) == 's':
        return ('osc', 1)
    if m.group(1) == 'p':
        return ('osc', int(m.group(2)))
    return ('ship', int(m.group(2)))

def lcm(a, b):
    return a * b // math.gcd(a, b)

def main(path):
    soups = []
    for line in open(path):
        if not line.startswith('SOUP '):
            continue
        f = line.split()
        us = int(f[2])
        codes = [c.rsplit(':', 1)[0] for c in f[3:]]
        soups.append((us, codes))
    warm, test = soups[:WARM], soups[WARM:]

    seen = Counter()
    for _, codes in warm:
        for c in set(codes):
            seen[c] += 1
    common = {c for c, n in seen.items() if n >= THRESH * len(warm)}
    periods = sorted({kind(c)[1] for c in common if kind(c)[0] == 'osc'})
    L = 1
    for p in periods:
        L = lcm(L, p)
    ships = sorted(c for c in common if kind(c)[0] == 'ship')

    def fixed6(codes):
        return all(kind(c)[0] == 'osc' and 6 % kind(c)[1] == 0 for c in codes)
    def learned(codes):
        return all(kind(c)[0] == 'osc' and L % kind(c)[1] == 0 for c in codes)
    def learned_ships(codes):
        return all((kind(c)[0] == 'osc' and L % kind(c)[1] == 0) or c in ships for c in codes)
    def ideal(codes):
        return all(c in common for c in codes)

    total = sum(us for us, _ in test)
    print('%s: %d test soups, common periods %s -> L = %d, common ships %s'
          % (path.split('/')[-1], len(test), periods, L, ships if ships else 'none'))
    for name, f in [('fixed6', fixed6), ('learned L=%d' % L, learned),
                    ('learned L=%d + ships' % L, learned_ships), ('ideal', ideal)]:
        fwd = [us for us, codes in test if not f(codes)]
        tf = sum(fwd)
        print('  %-24s forwards %5.1f%% of soups, %5.1f%% of CPU time -> speedup bound %s'
              % (name, 100.0 * len(fwd) / len(test), 100.0 * tf / total,
                 ('%.1fx' % (total / tf)) if tf else 'inf'))
    # What blocks the learned+ships filter most often:
    block = Counter()
    for us, codes in test:
        if not learned_ships(codes):
            for c in set(codes):
                k = kind(c)
                if not ((k[0] == 'osc' and L % k[1] == 0) or c in ships):
                    block[c] += 1
    print('  most common objects forcing a forward:', ', '.join('%s (%d)' % kv for kv in block.most_common(6)))

for p in sys.argv[1:]:
    main(p)
