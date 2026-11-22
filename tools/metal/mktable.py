#!/usr/bin/env python3
"""Write a rule's 512-entry table (MAP index order: NW 256 ... SE 1) as 512 '0'/'1' chars.

usage: mktable.py RULE OUT        RULE = any casim rule string, or
       'shiftNE' (next state = NE neighbour; pins orientation), or
       'randmap:SEED' (random non-isotropic MAP rule, B0 off)
Prints the canonical rule name casim reports (MAP string for the synthetic ones).
"""
import sys, base64, random
sys.path.insert(0, "/Users/leo/Desktop/Life tools/Vibecoded life tools/casim")
import casim as C


def table_to_map(t):
    bits = ''.join(str(int(b)) for b in t)
    raw = int(bits, 2).to_bytes(64, 'big')
    return 'MAP' + base64.b64encode(raw).decode().rstrip('=')


rule, out = sys.argv[1], sys.argv[2]
if rule == 'shiftNE':
    t = [(i >> 6) & 1 for i in range(512)]
    rule = table_to_map(t)
elif rule.startswith('randmap:'):
    rng = random.Random(int(rule.split(':')[1]))
    t = [rng.random() < 0.25 for _ in range(512)]
    t[0] = 0
    rule = table_to_map(t)
r = C.Rule(rule)
t2 = [int(x) for x in r.table]
if rule.startswith('MAP'):
    assert t2 == [int(x) for x in t], 'MAP round trip failed'
assert t2[0] == 0, 'B0 rules not supported by this test'
open(out, 'w').write(''.join(map(str, t2)) + '\n')
print(rule)
