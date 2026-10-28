from lifelib.cgold import describe
from lifelib import lifelib_dir
import os
import unittest

class TestCGoLd(unittest.TestCase):

    def test_genesis_block(self):

        blocks = list(describe.main(os.path.join(lifelib_dir, 'cgold', 'block0.cbc')))
        self.assertEqual(len(blocks), 1)
        block = blocks[0]
        seeds = [x.split(':')[-1].strip() for x in block.splitlines() if x.startswith('#C seed')]
        self.assertEqual(len(seeds), 1)
        seed = seeds[0]
        self.assertEqual(seed, 'OlCtB56rP1OdOQisR9mqQ1isR5CrQlnrwc6qF1n3a006SwRs442350544')

if __name__ == '__main__':
    unittest.main()
