
import unittest

class TestCygwin(unittest.TestCase):

    def test_cygdir(self):

        import lifelib
        lifelib.add_cygdir('something')
        from lifelib import autocompile
        self.assertEqual(autocompile.cygwin_dirs, ['something'])

    def test_numpy(self):

        from lifelib import autocompile
        np_version = autocompile.verify_installation()

if __name__ == '__main__':
    unittest.main()
