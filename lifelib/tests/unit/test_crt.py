
import unittest
import lifelib.pythlib.crt as crt

class TestCRT(unittest.TestCase):

    def test_mulinv(self):

        self.assertEqual(crt.mul_inv(105, 128), 89)
        self.assertEqual(crt.mul_inv(89, 128), 105)

    def test_chinese_remainder(self):

        moduli = [13, 14, 15]
        residues = [1729 % n for n in moduli]
        self.assertEqual(crt.chinese_remainder(moduli, residues), 1729)

if __name__ == '__main__':
    unittest.main()
