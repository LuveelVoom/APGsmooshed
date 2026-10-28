from .parsetable import GetNumberOfInputs

class RuleTree:
    '''
    Based on the RuleTree code from Golly's glife module, largely
    written by Tim Hutton, Tom Rokicki, and Andrew Trevorrow.
    '''

    def __init__(self, numStates, numInputs, initialise=True):

        self.numInputs = numInputs;

        self.world = {} # dictionary mapping node tuples to node index (for speedy access by value)
        self.seq = [] # same node tuples but stored in a list (for access by index)
        # each node tuple is ( depth, index0, index1, .. index(numStates-1) )
        # where each index is an index into self.seq

        self.nodeSeq = 0
        self.curndd = -1
        self.numStates = numStates

        self.cache = {}
        self.shrinksize = 100

        if initialise:
            self._init_tree()

    def _init_tree(self):
        self.curndd = -1
        for i in range(self.numInputs):
            node = tuple( [i+1] + [self.curndd]*self.numStates )
            self.curndd = self._getNode(node)

    def _getNode(self,node):
        if node in self.world:
            return self.world[node]
        else:
            iNewNode = self.nodeSeq
            self.nodeSeq += 1
            self.seq.append(node)
            self.world[node] = iNewNode
            return iNewNode

    def _add(self,inputs,output,nddr,at):
        if at == 0: # this is a leaf node
            if nddr<0:
                return output # return the output of the transition
            else:
                return nddr # return the node index
        if nddr in self.cache:
            return self.cache[nddr]
        # replace the node entry at each input with the index of the node from a recursive call to the next level down
        ### AKT: this code causes syntax error in Python 2.3:
        ### node = tuple( [at] + [ self._add(inputs,output,self.seq[nddr][i+1],at-1) if i in inputs[at-1] \
        ###                        else self.seq[nddr][i+1] for i in range(self.numStates) ] )
        temp = []
        for i in range(self.numStates):
            if i in inputs[at-1]:
                temp.append( self._add(inputs,output,self.seq[nddr][i+1],at-1) )
            else:
                temp.append( self.seq[nddr][i+1] )
        node = tuple( [at] + temp )
        r = self._getNode(node)
        self.cache[nddr] = r
        return r

    def _recreate(self,oseq,nddr,lev, swap_at):
        if lev == 0:
            return nddr
        if nddr in self.cache:
            return self.cache[nddr]
        if lev == swap_at:
            nodes = [lev]
            for j in range(self.numStates):
                node = tuple( [lev-1] + [ self._recreate(oseq, oseq[oseq[nddr][i+1]][j+1],lev-2,swap_at) for i in range(self.numStates) ] )
                r = self._getNode(node)
                nodes.append(r)
            node = tuple(nodes)
        else: 
            # each node entry is the node index retrieved from a recursive call to the next level down
            node = tuple( [lev] + [ self._recreate(oseq,oseq[nddr][i+1],lev-1,swap_at) for i in range(self.numStates) ] )
        r = self._getNode(node)
        self.cache[nddr] = r
        return r

    def _shrink(self, swap_at=-1):
        self.world = {}
        oseq = self.seq
        self.seq = []
        self.cache = {}
        self.nodeSeq = 0 ;
        self.curndd = self._recreate(oseq, self.curndd, self.numInputs, swap_at)
        self.shrinksize = len(self.seq) * 2

    def add_rule(self,inputs,output):
        self.cache = {}
        self.curndd = self._add(inputs,output,self.curndd,self.numInputs)
        if self.nodeSeq > self.shrinksize:
            self._shrink()

    def _setdefaults(self,nddr,off,at):
        if at == 0:
            if nddr<0:
                return off
            else:
                return nddr
        if nddr in self.cache:
            return self.cache[nddr]
        # each node entry is the node index retrieved from a recursive call to the next level down
        node = tuple( [at] + [ self._setdefaults(self.seq[nddr][i+1],i,at-1) for i in range(self.numStates) ] )
        node_index = self._getNode(node)
        self.cache[nddr] = node_index
        return node_index

    def _setDefaults(self):
        self.cache = {}
        self.curndd = self._setdefaults(self.curndd, -1, self.numInputs)

    def write(self, nhood):
        self._setDefaults()
        self._shrink()
        list_of_lines = ["@TREE\n"]
        list_of_lines.append("num_states=" + str(self.numStates) + '\n')
        list_of_lines.append("num_neighbors=" + repr(nhood) + '\n')
        list_of_lines.append("num_nodes="  + str(len(self.seq))  + '\n')
        for rule in self.seq:
            list_of_lines.append(' '.join(map(str,rule))+'\n')
        return list_of_lines

    def _treecompare(self, other, selfnode, othernode, memdict=None):

        if memdict is None:
            memdict = set([])

        if (selfnode, othernode) in memdict:
            return True

        sn = self.seq[selfnode]
        on = other.seq[othernode]

        lev = sn[0]

        if lev == 1:
            return (sn == on)

        for (sni, oni) in zip(sn[1:], on[1:]):
            if not self._treecompare(other, sni, oni, memdict):
                return False

        memdict.add((selfnode, othernode))
        return True

    def __eq__(self, other):
        '''
        Extensional comparison for equality; assumes trees are optimised.
        '''

        if self.numInputs != other.numInputs:
            return False

        if self.numStates != other.numStates:
            return False

        return self._treecompare(other, self.curndd, other.curndd)

    def invariant(self, perm):

        other = RuleTree(self.numStates, self.numInputs, initialise=False)
        other.seq = list(self.seq)
        other._shrink()

        perm = list(perm)
        for _ in range(len(perm)):
            for i in range(len(perm) - 1):
                if perm[i+1] < perm[i]:
                    perm[i], perm[i+1] = perm[i+1], perm[i]
                    other._shrink(swap_at=(len(perm) - i))

        return (other == self)

    def __ne__(self, other):

        return not self.__eq__(other)


def TransitionsToTree(n_states, nhood, transitions):
    '''Convert a set of transitions directly to a rule tree.'''

    numInputs = GetNumberOfInputs(nhood)
    tree_order = nhood[:numInputs][::-1] + nhood[numInputs:]
    tree = RuleTree(n_states, numInputs)
    for t in transitions:
        result = sum([(x[0] << (i << 3)) for (i, x) in enumerate(t[numInputs:])])
        tree.add_rule(t[:numInputs], result)

    return tree.write(tree_order)
