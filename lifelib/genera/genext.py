'''
The following code was written by M. I. Wright (for Python 3) and
subsequently modified to be compatible with both versions of Python.
'''

from ._wright import *

ACTIVE, INACTIVE = 'active', 'inactive'

def unbind_vars(transition, start=0):
    """
    Suffix varnames in transition with locally-unique numbers so that Golly
    doesn't bind the identical names
    """
    ret, seen = [], {}
    for state in transition:
        if isinstance(state, int) or state.isdigit():
            ret.append(state)
        else:
            seen[state] = cur = seen.get(state, -1) + 1
            ret.append('{}_{}'.format(state, cur))
    return ret


def _lazy_tr(states):
    """
    Take sequence of 2- or 3-tuples, interpreting them as
    (
      cellstate value to repeat+unbind,
      number of times to repeat,
      optional value to start bindings at,
    )
    and produce an expanded/chained form thereof
    """
    for value in states:
        if isinstance(value, tuple):
            state = value[0]
            count = value[1]
            start_at = value[2:]
            for uv in unbind_vars([state] * count, *start_at):
                yield uv
        else:
            yield value


def tr(*states):
    """Generate a transition (as list) from varargs"""
    return list(_lazy_tr(states))


def make_totalistic(birth, survival):
    transitions = []
    # Birth
    transitions += [tr(0, (ACTIVE, n), (INACTIVE, 8 - n), 1) for n in map(int, birth)]
    # Survival
    transitions += [tr((ACTIVE, n + 1), (INACTIVE, 8 - n), ACTIVE+'_0') for n in map(int, survival)]
    return 'permute', transitions


def make_nontot(birth, survival):
    transitions = []
    birth, survival = combine_rstring(birth), combine_rstring(survival)
    # Birth
    transitions += [unbind_vars((0,) + tuple(([INACTIVE, ACTIVE][int(c)] for c in NAPKINS[total][configuration])) + (1,))
        for total, configurations in birth.items()
        for configuration in configurations]
    # Survival
    transitions += [unbind_vars((ACTIVE,) + tuple(([INACTIVE, ACTIVE][int(c)] for c in NAPKINS[total][configuration]))) + [ACTIVE+'_0']
        for total, configurations in survival.items()
        for configuration in configurations]
    return 'rotate4reflect', transitions


def make_rule(birth, survival, age_pattern):
    active, inactive = [], [0]
    n_states = 1 + sum(age_pattern)
    tr_func = make_totalistic if (birth + survival).isdigit() else make_nontot
    
    if n_states > 255:
        raise ValueError("State count %d exceeds maximum of 255." % n_states)
    
    symmetry_type, transitions = tr_func(birth, survival)

    # Transition toward death where unspecified
    transitions += [tr(state, ('all', 8), (state + 1) % n_states) for state in range(1, n_states)]  # state 0 isn't included
    
    lower = 1  # state 0 is always 'inactive', so we start at 1
    for idx, upper in enumerate(age_pattern):
        if (idx % 2):
            inactive += list(range(lower, lower + upper))
        else:
            active += list(range(lower, lower + upper))
        lower += upper
    return symmetry_type, n_states, active, inactive, transitions


def write_table(fp, rulename, symmetries, n_states, active, inactive, transitions):
    fp.write('@RULE {}\n@TABLE\n'.format(rulename))
    fp.write('n_states:{}\nneighborhood:Moore\nsymmetries:{}\n'.format(n_states, symmetries))
    
    # Variables
    def define_var(name, var):
        if not var:
            raise SystemExit(
              'ERROR: Var for {!r} states is coming up empty. '
              'Are you sure this is a valid rule?'.format(name)
              )
        fp.write('\nvar {0}_0={1}\n'.format(name, list(var)).replace('[', '{').replace(']', '}'))
        for n in range(8):
            fp.write('var {0}_{1}={0}_0\n'.format(name, n + 1))

    define_var(ACTIVE, active)
    define_var(INACTIVE, inactive)
    define_var('all', range(n_states))

    # Transitions
    for tr in transitions:
        fp.write('\n' + ','.join(map(str, tr)))


def create_rule(rulestring):

    transitions = []
    bs, age_pattern = rulestring.split('d')
    age_pattern = tuple(map(int, age_pattern.split('-')))

    str2tab(bs) # Raise a NonCanonicalError if initial segment is non-canonical

    birth, survival = tuple(map(str.strip, bs[1:].split('s')))

    ruletable = make_rule(birth, survival, age_pattern)

    # Write temporary .rule file:
    with open((rulestring+'.rule'), 'w') as fp:
        write_table(fp, rulestring, *ruletable)

    # Convert .rule file into C/C++ code:
    rule2files(rulestring + '.rule')

    # Link code into lifelib:
    make_header(rulestring)
