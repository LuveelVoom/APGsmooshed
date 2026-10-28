'''
The following code was written by M. I. Wright (for Python 3) and
subsequently modified to be compatible with both versions of Python.
'''

from ._wright import *

def replace_bind(transition, pre, sub='', count=0):
    cop = []
    for i in map(int, transition):
        if not i:
            cop.append(0)
            continue
        cop.append('{}{}_{}'.format(pre, sub, count))
        count += 1
    return cop


def write_table(fp, rname, n_states, n_live, d_vars, transitions):
    fp.write('@RULE {}\n@TABLE\n'.format(rname))
    fp.write('n_states:{}\nneighborhood:Moore\nsymmetries:rotate4reflect\n'.format(n_states))
    # Variables
    live = range(1, n_states)
    for sub, (state, count) in d_vars.items():
        range_ = {i for i in live if i != state}
        if not range_:
            continue
        fp.write('\nvar not_{}_0 = {}'.format(sub, list(range_)).replace('[','{').replace(']','}'))
        for n in range(1, count):
            fp.write('\nvar not_{0}_{1} = not_{0}_0'.format(sub, n))
    fp.write('\nvar any_0 = {}'.format(list(range(n_states))).replace('[','{').replace(']','}'))
    for n in range(9):
        fp.write('\nvar any_{} = any_0'.format(n))
    fp.write('\nvar live_0 = {}'.format(list(live)).replace('[','{').replace(']','}'))
    for n in range(1, n_live):
        fp.write('\nvar live_{} = live_0'.format(n))
    fp.write('\n')
    # Transitions
    for tr in transitions:
        fp.write('\n' + ','.join(map(str, tr)))


def create_rule(rulestring):

    transitions = []
    bs = rulestring.split('d')[0]
    PERM = (rulestring[-1] == 'p')

    str2tab(bs) # Raise a NonCanonicalError if initial segment is non-canonical

    birth, survival = tuple(map(str.strip, bs[1:].split('s')))
    birth, survival = combine_rstring(birth), combine_rstring(survival)
    n_live, sum_len = 0, 2
    for counter, (num, subs) in enumerate(birth.items()):
        if num == '0':
            transitions.append([0]*9 + [1])
        elif num == '8':
            transitions.append([0] + ['live_{}'.format(i) for i in range(8)] + [1])
            n_live = 8
        else:
            transitions += [[0] + replace_bind(NAPKINS[num][sub], 'not_', num+sub) + [idx] for idx, sub in enumerate(subs, sum_len)]
            sum_len = 1 + transitions[-1][-1]

    d_vars = {k: (tr[-1], sum(1 for i in tr if isinstance(i, str))) for k, tr in zip((n+j for n, li in birth.items() for j in N_HOODS[:len(NAPKINS[n])] if not li or j in li), transitions)}
    transitions.append('')
    
    END = 'live_0' if PERM else 1  # Bind if rule is to be 'permanently deficient'
    for num, subs in survival.items():
        if num == '0':
            transitions.append(['live_0'] + [0]*8 + [END])
        elif num == '8':
            transitions.append(['live_{}'.format(i) for i in range(9)] + [END])
            n_live = 9
        else:
            transitions += [['live_0'] + replace_bind(NAPKINS[num][sub], 'live', count=1) + [END] for sub in subs]

    if survival:
        n_live = max(n_live, *(sum(1 for i in tr if isinstance(i, str)) for tr in transitions[1+transitions.index(''):]))

    n_states = 1 + max(t[-1] for t in transitions if t and isinstance(t[-1], int))
    transitions.append(['any_{}'.format(i) for i in range(9)] + [0])

    # Write temporary .rule file:
    with open((rulestring+'.rule'), 'w') as fp:
        write_table(fp, rulestring, n_states, n_live, d_vars, transitions)

    # Convert .rule file into C/C++ code:
    rule2files(rulestring + '.rule')

    # Link code into lifelib:
    make_header(rulestring)
