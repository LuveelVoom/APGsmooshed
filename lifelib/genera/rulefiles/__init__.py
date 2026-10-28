from .parsetable import ReadRuleTable
from .parsetree import ReadRuleTree, SurplusTreeError, FlattenRuleTree, get_symmetries
from .writetree import TransitionsToTree
from .writecode import NodesToCode, MakeStaticTable

import os
import sys
from collections import OrderedDict
from ast import literal_eval

lifelib_dir = os.path.dirname(os.path.abspath(__file__))
while (os.path.basename(lifelib_dir) != 'lifelib'):
    lifelib_dir = os.path.abspath(os.path.join(lifelib_dir, '..'))
rules_dir = os.path.join(lifelib_dir, 'rules')

def table_to_tree(lines):

    n_states, nhood, transitions = ReadRuleTable(lines)
    lines = TransitionsToTree(n_states, nhood, transitions)
    return lines

def minkowski_symmetrise(nhood):

    return set([tuple(((x - y) for (x, y) in zip(t, s))) for t in nhood for s in nhood])

def analyse_tree(segments):

    lines = segments['@TREE']

    n_states, nhood, list_of_nodes = ReadRuleTree(lines)
    syms = get_symmetries(n_states, nhood, list_of_nodes)

    if len(syms) == 12:
        symstring = "HEXAGONAL"
    elif len(syms) == 8:
        symstring = "SQUARE"
    else:
        symstring = "NONE"

    is16bit = (n_states > 256)

    flattened_tree = FlattenRuleTree(n_states, nhood, list_of_nodes)
    segments['@FLATTREE'] = MakeStaticTable(flattened_tree)

    # Determine zone of influence for rule:
    mink = minkowski_symmetrise(nhood)
    chebyshev = max([max(map(abs, t)) for t in mink])
    manhattan = max([sum(map(abs, t)) for t in mink])
    zoi = "9" * (manhattan - chebyshev) + "5" * (chebyshev * 2 - manhattan)

    preamble  = ['/* %d-bit code generated from ruletree */\n' % (16 if is16bit else 8)]
    preamble += ['/* ZOI="%s" */\n' % zoi]
    preamble += ['/* BITS="%d" */\n' % (16 if is16bit else 8)]
    preamble += ['/* NHOOD="%s" */\n' % nhood]
    preamble += ['/* SYMS="%s" */\n' % symstring]

    segments['@PREAMBLE'] = preamble

def create_code(segments):

    lines = []
    template = 0
    nhood = None
    bits = None

    for l in segments['@PREAMBLE']:
        if (nhood is None) and ('NHOOD=' in l):
            nhood = literal_eval(l.split('"')[1])
        if (bits is None) and ('BITS=' in l):
            bits = literal_eval(l.split('"')[1])

    if '@FLATTREE' in segments:
        template += 1
        lines += segments['@FLATTREE']
        lines += NodesToCode(nhood, (bits > 8))

    if '@AIGER' in segments:
        template += 2
        raise NotImplementedError("AIGER is currently unsupported")

    template = ["row_based_approach.h", "aiger_approach.h", "row_and_aiger.h"][template - 1]

    # We have implemented iterate_var_row instead of iterate_var_grid:
    lines.append('#include "%s"\n' % template)

    segments['@CODE'] = lines


def segmentate(rulefile):
    '''
    Split a file into segments demarcated by '@' annotations.
    '''

    segments = OrderedDict()
    seg = '@PREAMBLE'

    for l in rulefile:
        ls = l.strip()
        if ls.startswith('@'):
            seg = ls.split()[0]
        if seg not in segments:
            segments[seg] = []
        segments[seg].append(l)

    return segments

def decapitalise(grulename):
    '''
    Convert a Golly-style rulename with capital letters into a mangled
    lifelib rulename with leading 'x' and sufficient information to
    recover the original Golly rulename.
    '''

    if (grulename == grulename.lower()):
        # Idempotent
        return grulename

    grulename = grulename[0].upper() + grulename[1:]
    rulename = grulename.lower()
    compares = [(x != y) for (x, y) in zip(rulename, grulename)]

    rulename = ('x' if compares[0] else '_') + rulename
    for (i, j) in enumerate(compares):
        if (i > 0) and j:
            rulename = 'x' + str(i) + rulename

    if (rulename[0] != 'x'):
        rulename = 'x' + rulename

    return rulename

def rule2segs(rulefile):
    '''
    Accepts a filename or file-like object as a parameter and
    returns the ordered pair (rulename, ruledata) where ruledata
    is a dictionary mapping segment names to lists of lines.

    This will ensure that the ruledata dictionary contains a
    segment called @CODE containing C/C++ code for the rule.
    '''

    if isinstance(rulefile, str):
        filename = rulefile
        basename = os.path.splitext(os.path.basename(filename))[0]
        with open(filename) as f:
            rulefile = list(f)
        if filename.endswith('.table'):
            rulefile = [('@RULE %s\n' % basename), '@TABLE\n'] + rulefile
        elif filename.endswith('.tree'):
            rulefile = [('@RULE %s\n' % basename), '@TREE\n'] + rulefile
    else:
        rulefile = list(rulefile)
    segments = segmentate(rulefile)

    if '@RULE' not in segments:
        if '@NUTSHELL' not in segments:
            raise RuntimeError("Rule file must contain either a @RULE or @NUTSHELL segment.")

        if (sys.version_info < (3, 6)):
            raise NotImplementedError("Nutshell transpilation requires Python >= 3.6")

        from nutshell.main import transpile

        transpiled = transpile(rulefile).splitlines(True)
        new_segments = segmentate(transpiled)
        segments.update(new_segments)

        if '@RULE' not in segments:
            raise RuntimeError("Nutshell transpilation failed to produce a @RULE segment.")

    grulename = segments['@RULE'][0].strip().split()[1]
    rulename = decapitalise(grulename)

    while '@CODE' not in segments:

        # Apply transformations

        if ('@PREAMBLE' in segments) and (('@FLATTREE' in segments) or ('@AIGER' in segments)):
            create_code(segments)
        elif '@TREE' in segments:
            analyse_tree(segments)
        elif '@TABLE' in segments:
            segments['@TREE'] = table_to_tree(segments['@TABLE'])
        else:
            raise RuntimeError("Rule file must contain a @CODE, @TREE, or @TABLE segment.")

    segments = {k : [x for x in v if (not x.strip().startswith('@'))] for (k, v) in segments.items()}
    return rulename, segments

def rule2files(rulefile, ruledir=rules_dir):

    rulename, segments = rule2segs(rulefile)

    filename = rulename if (rulename[0] == 'x') else ('x' + rulename)

    # Write rule file rulename.h:
    with open(os.path.join(ruledir, filename + '.h'), 'w') as f:
        if '@PREAMBLE' in segments:
            for l in segments['@PREAMBLE']:
                f.write(l)
        f.write('namespace %s {\n' % rulename.replace('-', '_'))
        for l in segments['@CODE']:
            f.write('    ' + l)
        f.write('}\n')

    # Optionally write rulename.colors and rulename.icons:
    for extension in ['colors', 'icons']:
        segname = '@' + extension.upper()
        if segname in segments:
            with open(os.path.join(ruledir, filename + '.' + extension), 'w') as f:
                for l in segments[segname]:
                    f.write(l)

    return rulename
