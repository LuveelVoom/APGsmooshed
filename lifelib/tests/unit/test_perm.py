
import unittest
import lifelib.genera.rulefiles.parsetable as parsetable

from lifelib.genera import sanirule

class TestPerm(unittest.TestCase):

    def test_permute(self):

        generators = parsetable.permute_symmetry(parsetable.nhoods['Moore'])
        self.assertEqual(generators, [[(1, 2, 3, 4, 5, 6, 7, 8)], [(1, 2)]])

    def test_sanirule(self):

        self.assertEqual(sanirule('B3/S23'), 'b3s23')

if __name__ == '__main__':
    unittest.main()
