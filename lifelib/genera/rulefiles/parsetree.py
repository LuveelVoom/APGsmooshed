from .parsetable import GetNumberOfInputs
from ast import literal_eval

from .writetree import RuleTree

from .automorph import matperms
from ..isotropic import tab2str, str2tab
from ..exceptions import SurplusTreeError

# Named neighbourhoods:
nhoods = {'2': [(-1, 0), (1, 0), (0, 0), (0, 0)],
          '4': [(0, -1), (-1, 0), (1, 0), (0, 1), (0, 0), (0, 0)],
          '8': [(-1, -1), (1, -1), (-1, 1), (1, 1), (0, -1), (-1, 0), (1, 0), (0, 1), (0, 0), (0, 0)]}

def ParseRuleTree(list_of_lines):

    numInputs = 0
    num_nodes = 0
    n_states = 0

    list_of_nodes = []

    for line in list_of_lines:

        line = line.split('#')[0].strip()

        if (line == '') or (line[0] == '@'):
            continue

        if '=' in line:

            command, thing = tuple([x.strip() for x in line.split('=')])

            if command == 'num_states':
                n_states = int(thing)
                if (n_states < 2) or (n_states > 65536):
                    raise ValueError('n_states must be between 2 and 65536, inclusive')
            elif command in ['num_neighbors', 'num_neighbours']:
                nstring = thing
                if nstring in nhoods:
                    nhood = nhoods[nstring]
                else:
                    nhood = [tuple(x) for x in literal_eval(nstring)]
                numInputs = GetNumberOfInputs(nhood)
            elif command == 'num_nodes':
                num_nodes = int(thing)

        else:

            # Replace all non-digits with whitespace:
            line = ''.join([(c if (c in '0123456789') else ' ') for c in line]).strip()

            if line == '':
                continue

            if (n_states <= 0) or (num_nodes <= 0) or (numInputs <= 0):
                raise RuntimeError('num_states, num_neighbours, and num_nodes must all be defined prior to the first node.')

            # Tree node
            numbers = [int(x) for x in line.split()]
            list_of_nodes.append(numbers)

    if (len(list_of_nodes) != num_nodes):
        raise RuntimeError('%d nodes found; %d expected' % (len(list_of_nodes), num_nodes))

    return n_states, nhood, list_of_nodes


def compress_tree(list_of_nodes, n_states, numInputs, seqonly=True):
    rt = RuleTree(n_states, numInputs, initialise=False)
    rt.seq = list(map(tuple, list_of_nodes))
    rt._shrink()
    if seqonly:
        return list(map(list, rt.seq))
    else:
        return rt


def make_tree(n_states, nhood, list_of_nodes):

    numInputs = GetNumberOfInputs(nhood)
    return compress_tree(list_of_nodes, n_states, numInputs, seqonly=False)


def normalise_neighbourhood(nhood):

    x0, y0 = nhood[-1]
    return [(x - x0, y - y0) for (x, y) in nhood]


def get_symmetries(n_states, nhood, list_of_nodes):

    numInputs = GetNumberOfInputs(nhood)
    rt = compress_tree(list_of_nodes, n_states, numInputs, seqonly=False)

    pts = nhood[:numInputs]
    perms = matperms(pts)

    perms = [p for p in perms if rt.invariant(p)]
    return perms


def optimise_tree(n_states, nhood, list_of_nodes):

    numInputs = GetNumberOfInputs(nhood)

    # Perform optimisation of this tree:
    list_of_nodes = compress_tree(list_of_nodes, n_states, numInputs)

    # Determine which inputs are essential:
    essentials = [False] * numInputs
    for node in list_of_nodes:
        if len(set(node[1:])) > 1:
            essentials[-node[0]] = True

    # Flatten unnecessary layers:
    for node in list_of_nodes:
        depth = node[0]
        if (depth > 1):
            next_depth = list_of_nodes[node[-1]][0]
            if not essentials[-next_depth]:
                for j in range(1, len(node)):
                    node[j] = list_of_nodes[node[j]][-1]
    while not essentials[-list_of_nodes[-1][0]]:
        x = list_of_nodes[-1][-1]
        list_of_nodes = list_of_nodes[:x+1]

    # Rename inputs:
    emap = [0] * (numInputs + 1)
    for i in range(1, numInputs + 1):
        emap[i] = emap[i-1] + (1 if essentials[-i] else 0)
    for node in list_of_nodes:
        node[0] = max(1, emap[node[0]])

    # Shrink neighbourhood:
    new_inputs = [t for (x, t) in zip(essentials, nhood[:numInputs]) if x]
    nhood = new_inputs + nhood[numInputs:]
    numInputs = len(new_inputs)

    # Reoptimise:
    list_of_nodes = compress_tree(list_of_nodes, n_states, numInputs)

    if len(nhood) == numInputs + 1:
        nhood = normalise_neighbourhood(nhood)

    return n_states, nhood, list_of_nodes


def TreeToArray(nhood, list_of_nodes):

    inputs = nhood[:GetNumberOfInputs(nhood)]

    lord2 = []

    indices =  {(-1, -1): 0, (0, -1): 1, (1, -1): 2,
                (-1,  0): 3, (0,  0): 4, (1,  0): 5,
                (-1,  1): 6, (0,  1): 7, (1,  1): 8}

    for i in range(512):

        p = len(list_of_nodes) - 1

        for k in inputs:
            s = (i >> indices[k]) & 1
            p = list_of_nodes[p][s+1]

        lord2.append(p)

    return lord2


def ReadRuleTree(list_of_lines):

    # Parse the actual file into a rule tree:
    ruletree = ParseRuleTree(list_of_lines)

    # Optimise the resulting rule tree:
    return optimise_tree(*ruletree)


def FlattenRuleTree(n_states, nhood, list_of_nodes):

    # Determine whether rule is 2-state isotropic:
    if (n_states == 2) and (GetNumberOfInputs(nhood) + 1 == len(nhood)):
        chebyshev_range = max([abs(x) for y in nhood for x in y])
        if (chebyshev_range <= 1):
            rarray = TreeToArray(nhood, list_of_nodes)

            for hexagonal in [False, True]:
                rstring = tab2str(rarray, hexagonal)
                if (rarray == str2tab(rstring)):
                    raise SurplusTreeError(rstring)

    # Serialise tree to an array:
    num_nodes = len(list_of_nodes)

    def serialise_node(numbers):

        depth = numbers[0]
        children = numbers[1:]

        if depth > 1:
            # Convert node number to index into uint32_t array:
            children = [n_states * (num_nodes - 1 - x) for x in children]

        return children

    return [serialise_node(data) for data in list_of_nodes[::-1]]
