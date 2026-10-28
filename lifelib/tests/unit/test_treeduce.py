
import os
import lifelib
import unittest
import lifelib.genera.rulefiles.parsetree as parsetree
import lifelib.genera.rulefiles.automorph as automorph

from lifelib.genera.rulefiles import table_to_tree

class TestTreeduce(unittest.TestCase):

    def test_adjugate(self):
        '''
        Test Wikipedia's example of an adjugate matrix
        '''

        mat = [[-3, 2, -5], [-1, 0, -2], [3, -4, 1]]
        adj = [[-8, 18, -4], [-5, 12, -1], [4, -6, 2]]
        adj2 = automorph.adjugate(mat)

        self.assertEqual(adj2, adj)

    def test_matperms(self):
        '''
        Test ability to determine symmetry group of a cross.
        '''
        mp = list(automorph.matperms([[0, 0], [1, 0], [0, 1], [-1, 0], [0, -1]]))
        mp2 = [[0, 1, 2, 3, 4], [0, 2, 1, 4, 3], [0, 1, 4, 3, 2], [0, 4, 1, 2, 3],
                [0, 2, 3, 4, 1], [0, 3, 2, 1, 4], [0, 3, 4, 1, 2], [0, 4, 3, 2, 1]]

        self.assertEqual(set(map(tuple, mp)), set(map(tuple, mp2)))

        mp3 = list(automorph.matperms([[0, 0], [4, 3], [-3, 4], [-4, -3], [3, -4]]))

        self.assertEqual(set(map(tuple, mp3)), set(map(tuple, mp2)))

        mp = list(automorph.matperms([[0], [5], [-5], [7], [-7]]))

        self.assertEqual(len(mp), 2)

    def test_fcc(self):

        d3 = [(x, y, z) for x in [-1, 0, 1] for y in [-1, 0, 1] for z in [-1, 0, 1] if x*x + y*y + z*z == 2]

        mp = list(automorph.matperms(d3))

        self.assertEqual(len(mp), 48)

    def test_24cell(self):
        '''
        Tests that the vertex set of the 24-cell has a symmetry group of
        order 1152 as expected.
        '''

        t = [-1, 0, 1]
        d4 = [(w, x, y, z) for x in t for y in t for z in t for w in t if w*w + x*x + y*y + z*z == 2]

        self.assertEqual(len(set(d4)), 24)
        mp = list(automorph.matperms(d4))
        self.assertEqual(len(mp), 1152)

    def test_rotate4(self):
        '''
        Tests that we can learn the symmetries of Langton's Loops from its
        rule tree alone.
        '''

        # Load a rule table:
        filename = os.path.join(lifelib.lifelib_dir, 'rules', 'source', 'Langtons-Loops.table')
        with open(filename, 'r') as f:
            list_of_lines = list(f)

        # Convert it to a ruletree and optimise:
        list_of_lines = table_to_tree(list_of_lines)
        ruletree = parsetree.ParseRuleTree(list_of_lines)
        ruletree = parsetree.optimise_tree(*ruletree)

        # Determine that the rule has rotate4 symmetries but not reflections:
        syms = parsetree.get_symmetries(*ruletree)
        syms2 = [[0, 1, 2, 3, 4], [3, 0, 1, 2, 4], [1, 2, 3, 0, 4], [2, 3, 0, 1, 4]]
        self.assertEqual(set(map(tuple, syms)), set(map(tuple, syms2)))

    def test_reduce(self):
        '''
        Tests that we can recognise an hexagonal rule embedded as a Moore CA.
        '''

        # Load a ruletree as a list of lines:
        filename = os.path.join(lifelib.lifelib_dir, 'rules', 'source', 'Hex-B2omS2.tree')
        with open(filename, 'r') as f:
            list_of_lines = list(f)

        # Parse the ruletree and verify that it has the Moore neighbourhood:
        ruletree = parsetree.ParseRuleTree(list_of_lines)
        self.assertEqual(ruletree[1], [(-1, -1), (1, -1), (-1, 1), (1, 1), (0, -1), (-1, 0), (1, 0), (0, 1), (0, 0), (0, 0)])
        self.assertEqual(len(ruletree[2]), 31)

        # Optimise the tree and verify that the neighbourhood has reduced:
        ruletree2 = parsetree.optimise_tree(*ruletree)
        self.assertEqual(ruletree2[1], [(-1, -1), (1, 1), (0, -1), (-1, 0), (1, 0), (0, 1), (0, 0), (0, 0)])
        self.assertEqual(len(ruletree2[2]), 27)

        # Determine that the resulting rule has exactly 12 symmetries:
        syms = parsetree.get_symmetries(*ruletree2)
        self.assertEqual(len(syms), 12)

if __name__ == '__main__':
    unittest.main()
