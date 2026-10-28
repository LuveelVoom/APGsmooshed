
import os
import lifelib
import unittest
from lifelib.autocompile import sanirule

class TestSanirule(unittest.TestCase):

    def test_goltree(self):

        filename = os.path.join(lifelib.lifelib_dir, 'rules', 'source', 'Life.tree')
        srule = sanirule(filename)
        self.assertEqual(srule, 'b3s23')

    def test_hextree(self):

        filename = os.path.join(lifelib.lifelib_dir, 'rules', 'source', 'Hex-B2omS2.tree')
        srule = sanirule(filename)
        self.assertEqual(srule, 'b2-ps2h')

if __name__ == '__main__':
    unittest.main()
