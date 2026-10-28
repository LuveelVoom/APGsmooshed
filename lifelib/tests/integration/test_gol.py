import unittest
import lifelib
import os

class TestGoL(unittest.TestCase):

    @classmethod
    def setUpClass(cls):

        lifelib.load_rules('b3s23', force_compile=True)

    def setUp(self):

        self.sess = lifelib.load_rules('b3s23')
        self.lt = self.sess.lifetree(memory=1000, n_layers=4)

    def test_bitshift(self):

        pd = self.lt.pattern('xp15_4r4z4r4')
        self.assertEqual((pd << 3).wechsler, '0_0_0_4r4z4r4')
        self.assertEqual((pd << 2).wechsler, '0_0_4r4z4r4')
        self.assertEqual((pd << 1).wechsler, '0_4r4z4r4')

        self.assertEqual(((pd << 2) >> 2), pd)

        block = self.lt.pattern('xs4_33')
        hwss = self.lt.pattern('xq4_27deee6')
        mwss = self.lt.pattern('xq4_27dee6')

        combined = self.lt.unify(block, pd, hwss, mwss)
        self.assertEqual(combined.wechsler, '33_4r4z4r4_27deee6_27dee6')
        self.assertEqual(combined.layers(), (block, pd, hwss, mwss))

    def test_ulimit(self):

        self.sess.lifelib.ensure_limit(10000, 1)
        self.sess.lifelib.ensure_limit(20000, 1)
        self.sess.lifelib.ensure_limit(10000, 1)

    def test_freywa_bug(self):

        a = self.lt.pattern("bo$2bo$3o5$5bobo$6b2o$6bo")
        b = self.lt.pattern("bo$2bo$3o")
        self.assertEqual(a(-10, -16).match(b).nonempty(), True)
        self.assertEqual(a(-20, -32).match(b).nonempty(), True)
        self.assertEqual(a(-40, -64).match(b).nonempty(), True)

    def test_components(self):

        pat = self.lt.pattern('''7b3o5b3o16b3o5b3o$6bo2bo5bo2bo15bo2bo3bo2bo$6bo3bo3bo3bo15bo9bo$7b4o3b
4o14b2o2bobobobo2b2o$8bobo3bobo15b2o2bobobobo2b2o$31bo5bo3bo5bo$10bo3b
o17bo2bobo3bobo2bo$5bo4b2ob2o4bo13b2o9b2o$5b2o3b2ob2o3b2o14bo9bo$10b2o
b2o$9b2o3b2o16b2o11b2o$9bo5bo16b2o11b2o2$36b2o3b2o$36b2o3b2o$19b3o8b3o
$3b3o3b2o3b2o3bo2bo7bo2bo12b3o$3bo2bo2b2o3b2o2bo3bo7bob2o12bo2bo$3bo
14bo3bo23bo2bo$4bobo11bo2bo$bo3bo13b2o12b3o5bob2o3bo2bo$bo25b2o4b2o7b
3o6bo$o26b3o2b4o10b2o2bo$29bo19bo$3bobo41b2o$3b3o$2bo2b3ob3o35bo$3b5o
3bo29b2o3bobo$2bobo2b2o2bo2bo26b3o2bobo$bobo5bo5bo$2bo7b2o4bo24b2obo$
3b2o6b3obo19bo6bo4b2o2bo$35bobo6bo6bo$35bo3b2o3bo$36bo4bo5b2o$36bo4bo$
37bo2bo!''', 'b3s23')

        ncomps = len(pat.components())
        self.assertEqual(ncomps, 41)

    def test_copy(self):

        pat = self.lt.pattern('xp40_33zxs48g69gvzx3021zy833')
        pat2 = pat.__copy__()

        self.assertEqual(pat, pat2)
        pat += pat(1093, 3511)
        self.assertNotEqual(pat, pat2)
        pat2 += pat2(1093, 3511)
        self.assertEqual(pat, pat2)

    def test_samples(self):

        pat = self.lt.pattern('xp40_33zxs48g69gvzx3021zy833')
        samples = pat.download_samples()
        samples = list(samples['C2_2'])[:20]
        periods = [s[10000][-1000:1000, -1000:1000].period for s in samples]
        self.assertEqual(periods, [40]*20)

        synth = pat.download_synthesis()
        res = synth[1000]
        self.assertEqual(res.period, 40)

    def test_hashsoup(self):

        x = self.lt.hashsoup("b3s23", "C1", "n_GcpqZ62YqD3310086454")
        ac = x[1000].apgcode
        self.assertEqual(ac, 'xp46_033y133zzzckgsxsgkczydciczy4ccy0222')

    def test_lhistory(self):

        scstring = '''x = 50, y = 72, rule = LifeHistory
26.4B$25.4B$24.4B$19.3B.4B$18.8B10.E$18.7B.B7.3E$18.10B5.E$16.12B5.CE
$15.14B.5B$15.18B$14.22B$14.23B$12.ECB.22B$11.E.EB.23B$11.E4.22B$10.
2E5.20B$18.19B$17.21B$15.ECB.15B.4B$14.E.EB3.10B5.4B$14.E6.10B6.4B$
13.2E5.13B5.4B$19.15B5.4B$18.16B6.4B$17.17B7.4B$18.16B8.4B$19.13B11.
4B$19.5B2C2B.3B12.4B$21.3B2C2B2.4B11.4B$21.8B3.CE12.4B$20.8B4.E14.3B$
20.8B5.3E12.2B$20.7B8.E13.B$20.7B$21.6B7.E$21.6B6.E.E$22.5B6.E.E$22.
6B4.2E.3E$21.6B6.B4.E$21.7B3.B2CB3E$8.3B11.8B.B2C.E$7.6B9.10B$5.11B5.
3B2C6B$5.10B2E5.2B2C6B$5.11BE5.10B$5.11BEBEB2.11B$5.12BECB.12B$5.13B.
15B$5.11B3.16B$5.11B3.16B.2B$5.10B4.18BCE$5.6B7.17B.BCE$5.6B6.4B2.8B.
4B.B$5.4B7.4B4.7B$5.3B7.4B5.6B$6.B7.3DB6.4B$13.3BD5.E3B$12.3BD5.E.CB$
11.4B6.E.C$10.3AB8.E$9.3BA6.3E$8.3BA7.E$7.4B$6.4B$5.4B$4.4B$3.4B$2.4B
$.4B$4B$3B$2B!'''

        scorbie = self.lt.pattern(scstring)
        self.assertEqual(scorbie[2000].population, 4425)
        rlestring = scorbie.rle_string()

        scstring  =  scstring.replace('\n','').split('tory')[-1]
        rlestring = rlestring.replace('\n','').split('tory')[-1]

        self.assertEqual(scstring, rlestring)
 
    def test_stream(self):

        push44 = [0, 109, 90, 93, 91, 90, 95, 91, 91, 138, 157, 96, 90, 120, 91, 97, 107, 90, 90, 93, 188]
        one_glider = self.lt.pattern('3o$o$bo!')
        elbow = self.lt.pattern('2o$2o!')

        eg = one_glider(5, 2).stream(push44) + elbow
        self.assertEqual(eg[4096], elbow(-22, -22))

    def test_empty(self):

        empty_pattern = self.lt.pattern()
        self.assertEqual(empty_pattern.apgcode, 'xs0_0')

    def test_randfill(self):

        pat = self.lt.pattern()
        pat[70:200, 50:300] = 0.5
        self.assertEqual(pat.bounding_box, [70, 50, 130, 250])
        self.assertAlmostEqual(pat.population, 16250, delta=900) # 10 standard deviations

    def test_bbox(self):

        lidka = self.lt.pattern("bo$obo$bo8$8bo$6bobo$5b2obo2$4b3o!", 'b3s23')
        rect100 = [23 - (10 ** 100) // 4, 13 - (10 ** 100) // 4, (10 ** 100) // 2 - 206, (10 ** 100) // 2 - 186]
        self.assertEqual(lidka[10 ** 100].getrect(), rect100)

    def test_lidka(self):

        lidka = self.lt.pattern("bo$obo$bo8$8bo$6bobo$5b2obo2$4b3o!", 'b3s23')
        self.assertEqual(lidka.population, 13)
        lidka_30k = lidka[30000]
        self.assertEqual(lidka_30k.population, 1623)

        rect18 = [-249999999999999977, -249999999999999987, 499999999999999794, 499999999999999814]

        self.assertEqual(lidka[10 ** 18].getrect(), rect18)
        self.assertEqual(lidka[1093][3511], lidka[3511][1093])
        self.assertNotEqual(lidka[1093], lidka[3511])

    def test_coe(self):

        coe = self.lt.pattern('xq16_gcbgzvgg826frc')
        osc = coe.oscar()
        self.assertEqual(osc['period'], 16)

        self.assertEqual(coe.wechsler, 'gcbgzvgg826frc')

    def test_timeline(self):

        try:
            os.makedirs('lifelib/tempdir')
        except:
            pass

        try:
            lidka = self.lt.pattern("bo$obo$bo8$8bo$6bobo$5b2obo2$4b3o!")
            lframes = [lidka[i << 8] for i in range(120)]
            self.lt.save_timeline(lframes, 'lifelib/tempdir/timeline.mc')
            frames2 = self.lt.load('lifelib/tempdir/timeline.mc')

            self.assertEqual(len(lframes), len(frames2))
            for (x, y) in zip(lframes, frames2):
                self.assertEqual(x, y)

        finally:
            import shutil
            shutil.rmtree('lifelib/tempdir')

    def test_io(self):

        try:
            os.makedirs('lifelib/tempdir')
        except:
            pass

        try:
            lidka = self.lt.pattern("x = 9, y = 15, rule = B3/S23\nbo$obo$bo8$8bo$6bobo$5b2obo2$4b3o!")[30000]
            lidka.save('lifelib/tempdir/lidka.rle', tempfile='lifelib/tempdir/tempfile')
            lidka.save('lifelib/tempdir/lidka.rle.gz', tempfile='lifelib/tempdir/tempfile')
            lidka.save('lifelib/tempdir/lidka.mc', tempfile='lifelib/tempdir/tempfile')
            lidka.save('lifelib/tempdir/lidka.mc.gz', tempfile='lifelib/tempdir/tempfile')

            # Would be embarrassing if gzip didn't reduce file sizes:
            slr = os.path.getsize('lifelib/tempdir/lidka.rle')
            slrz = os.path.getsize('lifelib/tempdir/lidka.rle.gz')
            slm = os.path.getsize('lifelib/tempdir/lidka.mc')
            slmz = os.path.getsize('lifelib/tempdir/lidka.mc.gz')
            self.assertLess(slrz, slr)
            self.assertLess(slmz, slm)

            lr = self.lt.load('lifelib/tempdir/lidka.rle', tempfile='lifelib/tempdir/tempfile')
            lrz = self.lt.load('lifelib/tempdir/lidka.rle.gz', tempfile='lifelib/tempdir/tempfile')
            lm = self.lt.load('lifelib/tempdir/lidka.mc', tempfile='lifelib/tempdir/tempfile')
            lmz = self.lt.load('lifelib/tempdir/lidka.mc.gz', tempfile='lifelib/tempdir/tempfile')

            # Macrocell should respect overall position; RLE need not:
            self.assertEqual(lidka, lm)
            self.assertEqual(lidka.centre(), lr.centre())

            # Compression should make no difference:
            self.assertEqual(lr, lrz)
            self.assertEqual(lm, lmz)
        finally:
            import shutil
            shutil.rmtree('lifelib/tempdir')

    def make_p51(self):

        part = self.lt.pattern("3b2o3b5o$3b2o3b2ob2o$8b2o2bo$2o7bobo$2o8bo$10bo2bo$11bo2bo$11bobo!", "b3s23").shift(-15, -15)
        p51 = part + part('flip') + part('flip_x') + part('flip_y') + part('rot90') + part('rot270') + part('swap_xy') + part('swap_xy_flip')
        return p51

    def test_apgcode(self):

        desired_apgcode = 'xp51_oo033wgwgggy1gggwgw330oozy04e9130111y11110319e4zy077yf77zy0ooyfoozy09t40gy9g04t9z660gg01310333y133301310gg066zx11yh11'
        actual_apgcode = self.make_p51()[27].apgcode
        self.assertEqual(actual_apgcode, desired_apgcode)

    def test_large_apgcode(self):

        cordership = self.lt.pattern('''x = 113, y = 114, rule = B3/S23
79bo$78bobo$77b2ob2o$78bo2bo$80b3o$80bobo$81b2o$81b2o12b2o$95b2o3$79bo$78bobo
$78bobo$79bo$103b2o$103b2o3$87bo$86bobo$86bobo$65bobo19bo$64b2ob2o27bo14b2o$
65bob2o27b3o12b2o$69bo25b2o2bo$68bo28bo2bo$50bo17bo25bobo2bo$50bo44b3o$50bo8b
o37bo$57bobo35bobo$51b3o2bo3bo34b2o7bo$52bob5o45b2o$58bo44bobo$53b2o4$32b3o$
32bobo$32b3o19b2o$48bo5bo$46bobo3bo2bo$34b2o10bo3bo3bo$32b2ob2o10b7o$32b2o2b
2o$31b2o3b2o14bo$32b2o2b2o12bobo19bo$33bobo13bo2bo19b2o$50b2o19bobo3$25bo$25b
o28b2o$25bo8bo19b2o$32bobo9b2o16b2ob3o$26b3o2bo3bo8b2o18bo$27bob5o31bo3bo$33b
o32b2obo$28b2o37bo2bo$68b2o3$52b2o$52b2o4$36bo$35b4o$33b5obo$32bo6b2o$31bo3bo
b3o$25bo6bobo2b2o$13bobo7bob2o$12b2ob2o6bobo$13bob2o6b2o$2bo14bo$bobo12bo$2ob
2o11bo$bo2bo$3b3o$3bobo$4b2o$4b2o3$11b2o$2bo7bo2bo$3bo7b2o$3b2o$2b3o$36bo$34b
2o$3b3o29b2o$bo4bo12b2o$b2o3bo11bo2bo$3b4o12b2o7$6b2o$6b2o7$14b2o$14b2o!''')
        long_apgcode = cordership.apgcode
        self.assertEqual(len(long_apgcode), 323)

    def test_p51(self):

        p51 = self.make_p51()
        p51_period = p51.oscar(verbose=False)['period']
        self.assertEqual(p51_period, 51)
        self.assertEqual((p51 ** 7).population, (p51.population ** 7))
        p102_period = (p51 * p51).oscar(verbose=False)['period']
        self.assertEqual(p102_period, 102)

    def test_numpy(self):

        p51 = self.make_p51()

        try:
            import numpy as np
        except ImportError:
            return

        coords = np.array([[0, 1], [2, 3], [4, 5], [-6, 69], [73, -48]], dtype=np.int64)
        values = np.array([1, 0, 1, 1, 0], dtype=np.int64)

        p51[coords] = values
        values2 = p51[coords]

        self.assertEqual(np.all(values == values2), True)

        mat2pat = lambda M : self.lt.pattern('$'.join([''.join(['bo'[y] for y in x]) for x in M.tolist()]) + '!')

        A = np.array(np.random.uniform(0, 2, size=(34, 55)), dtype='uint64')
        B = np.array(np.random.uniform(0, 2, size=(55, 89)), dtype='uint64')
        C = np.dot(A, B) % 2

        pA = mat2pat(A)
        pB = mat2pat(B)
        pC = mat2pat(C)

        pAB = pA.__matmul__(pB)

        self.assertEqual(pC, pAB)

    def test_mutable(self):

        lidka = self.lt.pattern("bo$obo$bo8$8bo$6bobo$5b2obo2$4b3o!", 'b3s23')
        self.assertEqual(lidka.population, 13)
        x = lidka
        x += lidka(40, 0)
        self.assertEqual(lidka.population, 26)

    def test_spacefiller(self):

        sf = "20b3o3b3o$19bo2bo3bo2bo$4o18bo3bo18b4o$o3bo17bo3bo17bo3bo$o8bo12bo3bo"
        sf += "12bo8bo$bo2bo2b2o2bo25bo2b2o2bo2bo$6bo5bo7b3o3b3o7bo5bo$6bo5bo8bo5bo8b"
        sf += "o5bo$6bo5bo8b7o8bo5bo$bo2bo2b2o2bo2b2o4bo7bo4b2o2bo2b2o2bo2bo$o8bo3b2o"
        sf += "4b11o4b2o3bo8bo$o3bo9b2o17b2o9bo3bo$4o11b19o11b4o$16bobo11bobo$19b11o$"
        sf += "19bo9bo$20b9o$24bo$20b3o3b3o$22bo3bo2$21b3ob3o$21b3ob3o$20bob2ob2obo$"
        sf += "20b3o3b3o$21bo5bo!"

        spacefiller = self.lt.pattern(sf, "b3s23")
        self.assertEqual(spacefiller.population, 200)

        googol = 10 ** 100
        exppop = (googol // 2) ** 2 + 17 * (googol // 2) + 200

        sg = spacefiller[googol]

        self.assertEqual(sg.population, exppop)

        # We now take a huge rectangular region out of the spacefiller:

        left = -(3 ** 70)
        right = 4 ** 56
        top = 5 ** 20
        bottom = 7 ** 20

        hugerect = sg[left:right, top:bottom]

        self.assertEqual(hugerect.population, ((right - left) * (bottom - top)) // 2)

        # Ensure Boolean operations work correctly:

        self.assertEqual(sg.population, hugerect.population + (sg - hugerect).population)


    def test_oscar(self):

        sm  = "$12bo7bo$11bobo5bobo$10b2ob2o3b2ob2o$11bobo5bobo$9bob2o3bo3b2obo$9b7ob"
        sm += "7o$13b7o$12bob2ob2obo$9b2obo7bob2o$8b3ob2o5b2ob3o$7bo17bo$10bob2o5b2ob"
        sm += "o2$14bo3bo$9bob3o5b3obo$10bob3o3b3obo$14bo3bo$13b2o3b2o$12b2o5b2o$11b"
        sm += "3o5b3o$8b4o9b4o$7bo17bo$6b2o3b2o7b2o3b2o$7bobobobo5bobobobo$7b3o2b3o3b"
        sm += "3o2b3o$12bo7bo$7bo17bo2$7bobobo9bobobo$9b3o9b3o$6bob3o2bo5bo2b3obo$7b"
        sm += "2ob3o7b3ob2o$11b2o7b2o$11bob2o3b2obo$11bo3bobo3bo$3b3o5bo3bobo3bo5b3o$"
        sm += "3bob2o6bo5bo6b2obo$3bo2bo3b2ob2o3b2ob2o3bo2bo$6bo2b2o3bo3bo3b2o2bo$10b"
        sm += "o2bo5bo2bo$5bobobobo9bobobobo$3b2obo19bob2o$9bo13bo$6b2o17b2o2$5bo21bo"
        sm += "$4b2o21b2o$3b2o23b2o2$3b2o23b2o$4bo23bo$5bo4bobo7bobo4bo$4b3o4bob2o3b"
        sm += "2obo4b3o$4b3o2bobob2o3b2obobo2b3o$9bo2b2o5b2o2bo$8b4o9b4o2$6b2o17b2o$"
        sm += "6b2o2b2o9b2o2b2o$7bo2bobo7bobo2bo$4bo2bobo2bo3bo3bo2bobo2bo$4bo4bo3bob"
        sm += "3obo3bo4bo$5bobobo4bobobo4bobobo$7bobo4bo3bo4bobo$5b4o2bobo5bobo2b4o$"
        sm += "5b3ob2ob2ob3ob2ob2ob3o$14b5o$11bobo5bobo$11bob2o3b2obo$16bo$16bo$10b2o"
        sm += "9b2o$10b2obo5bob2o$10bob2obobob2obo$12b2obobob2o$9bobobobobobobobo$9b"
        sm += "2o2bo5bo2b2o$10bob2o5b2obo$12b2o5b2o$11bo9bo$11b3o5b3o4$12bo7bo$11b2ob"
        sm += "o3bob2o$11bo2bo3bo2bo$11b3o5b3o$13b3ob3o$14b2ob2o$15bobo$14b2ob2o$12b"
        sm += "2o2bo2b2o$16bo$11b2o7b2o$10b3obo3bob3o$9bo2bo7bo2bo$10bo11bo$11bo9bo$"
        sm += "8b2ob2o7b2ob2o$11bo9bo$7bo3b2o7b2o3bo$8b6o5b6o$12bo7bo$11b2obo3bob2o$"
        sm += "12bob2ob2obo$11bo9bo$12bobo3bobo$10bobobo3bobobo$12bob5obo$11b2ob2ob2o"
        sm += "b2o$11b4o3b4o$10bo2bo5bo2bo$9bo2bo7bo2bo$10bob2o5b2obo$8bob2o9b2obo$8b"
        sm += "2o13b2o$8b2o13b2o$9b2o2b3ob3o2b2o$9b2o2b2o3b2o2b2o$12b2o5b2o$9bo2bo7bo"
        sm += "2bo$9bo2bo7bo2bo$8b2obo9bob2o$8bo2bo3b3o3bo2bo$8bo2bo3b3o3bo2bo$11bo2b"
        sm += "o3bo2bo$7b2obo4b3o4bob2o$6b6o3bobo3b6o$6bo19bo$10bo2b3ob3o2bo$10bo3bo"
        sm += "3bo3bo$10bo11bo$15b3o$16bo$13b3ob3o$14b2ob2o!"

        monster = self.lt.pattern(sm, "b3s23")

        results = monster.oscar(verbose=False)

        self.assertEqual(results['period'], 7)
        self.assertEqual(results['displacement'], (0, -3))

        galaxy = self.lt.pattern("6b2o$2o5bo$obo3bo$3bobo$4bo$3bobo$2bo3bobo$bo5b2o$b2o!", "b3s23")

        results = galaxy.oscar(verbose=False)

        self.assertEqual(results['period'], 8)

        results = galaxy.oscar(verbose=False, eventual_oscillator=False)

        self.assertEqual(len(results), 0)

    def test_p57(self):

        just_eater = self.lt.pattern('''8b3o$5b2obobo$5bo2bobo$3b3o$3b2o$4bo26b2o$32bo$29b3o$29bo$3bo3b2o$2bob
        o3bo$bobo3bo12b2o$bo4bo13bobo$2o5b3o12bo$9bo12b2o!''', 'b3s23')

        with_catalyst = self.lt.pattern('''8b3o$5b2obobo$5bo2bobo$3b3o$3b2o$4bo26b2o$32bo$29b3o$22bo6bo$3bo3b2o
        14bo$2bobo3bo12b3o$bobo3bo6b2o$bo5b2o5b3o$2o5b4ob2obob2o$o3b2o4b3o4b3o
        $b4obo6b2o6bo$10bo2b2ob6o$b2ob2o2bobo8bo2b2o$2bobo2b2o3b3o2bo2bo3bo$bo
        3b3obo8bo2b3o$2b3o4bo4bo3bo4bobo$5bo4b2obo10b2o$4bo2b3o2b2ob6o$5b2o3bo
        3bobobobo14bobo$7bobob2obo3bo17b2o$7bobo2bo6bobo14bo$8bobo2b2o5bobo$9b
        o5b2ob2ob2o$15b3o2bo2b2o$20bo2bo$13bob5obobo$13b2o2bobob2o$11b2o4bo2bo
        $10bo2b4ob2o2b3o$10b2o5bo2b2o2bo$15bo3bo2bo$15b2o3b2o$51bo$49bobo$50b
        2o!''', 'b3s23')

        p57loop = self.lt.pattern('''14bob2o9bo35b2o12bo35b2o12bo35b2o12bo35b2o12bo35b2o12bo35b2o12bo35b2o
        12bo35b2o12bo35b2o12bo35b2o12bo$14b2obo9b3o5b2o27bo12b3o5b2o27bo12b3o
        5b2o27bo12b3o5b2o27bo12b3o5b2o27bo12b3o5b2o27bo12b3o5b2o27bo12b3o5b2o
        27bo12b3o5b2o27bo12b3o5b2o27bo12b3o5b2o$30bo4bo28bobo13bo4bo28bobo13bo
        4bo28bobo13bo4bo28bobo13bo4bo28bobo13bo4bo28bobo13bo4bo28bobo13bo4bo
        28bobo13bo4bo28bobo13bo4bo28bobo13bo4bo$12b5o12bo3bobo29b2o12bo3bobo
        29b2o12bo3bobo29b2o12bo3bobo29b2o12bo3bobo29b2o12bo3bobo29b2o12bo3bobo
        29b2o12bo3bobo29b2o12bo3bobo29b2o12bo3bobo29b2o12bo3bobo$11bo4bo11bo3b
        obo43bo3bobo43bo3bobo43bo3bobo43bo3bobo43bo3bobo34bo8bo3bobo43bo3bobo
        43bo3bobo34b2o7bo3bobo43bo3bobo$10bo2bo14b2o3bo44b2o3bo44b2o3bo44b2o3b
        o44b2o3bo44b2o3bo34b3o7b2o3bo44b2o2bo45b2o3bo34bo9b2o3bo44b2o3bo$7bo2b
        ob2o43bo49bo11b2o36bo11bo37bo49bo49bo9bo39bo24bo24bo49bo11b2o36bo49bo
        16bo$6bobobo5bo21b2o15b3o47b3o9bobo35b3o10b3o15bo18b3o47b3o47b3o8bo2b
        2o34b3o47b3o47b3o11bo35b3o32bo14b3o8b2o5bobo$7bo2bo4b2o19b5o13bo49bo
        13bo35bo12bo19bo3bo12bo27b2o20bo49bo29b2o18bo30bo3bo14bo49bo13bo10bo
        24bo31b2o2bo13bo10bo2bo5bo$10b2o2b3o18bo4bo13b2o48b2o21bob3o9b2o11b2o
        10b2ob3o10b2o3bo4bo11b2o25bobo7b3o10b2o26bo8b2o11b2o10bob2ob3o6b2ob3o
        5b2o11b2o28bo3b2o14b2o48b2o12bo9bob2o9b2o11b2o12b3o14bo5bo12b2o7bo2bob
        o$35bo2b3o39b3o9b2o31b2obo3bo8b2o7bo16b2o3bo9bo3bo2bo2bo40bo6b2ob2o37b
        2o7b2o24b2o2bo2bo4b3ob2o6b2o44b2o52b2o8bo25bo4bo8b2o24bo2bo13b2o2bobo
        21bo4bo4b5o$21bo13b2o2bo42bo8b2o33bo4b2o16b4o18bo10bobo3b4o30bo17bo2b
        2o36b3o12bo5bo16bo3bo6b2ob3o52bo41bo17bo29bo42bo13b2o3bo21bo9bo4bo$20b
        2o15b2o35b4o3bo10bo34b4obo14b2o3b2o15bobo11b2o3b2o31b3o17b2o33bobo2bo
        12bo3bo2b2o15bo14b2o33b3o18bo38b2ob2o8b2obob3o30b4o17b3o13bo4bo14b3o
        25b3o8bo2bo$8b2o7bo55b2o71bo3bo69bobobob2o48bobob2o12bo4b3o17bobo29b3o
        13bo2bo2bo17bobo31b5o11b2ob5o3bo16bo29b2obo13bo58b2obo$7bo2bo4bo2bo53b
        2obo2bo67bo3bo68b2o3bob2o48b3o14bo5b2o19bo29bo4bo11bo4bo20bobo29bo2bo
        11bo4b2o4bo17bo28b2obo70bo5bob2o$7bobobo3bo38bobo16bobob2o33bo33b2o2bo
        50b2o3bo13b2obo2bo32bo34b3o2b2o15b3o31b2obo13bobo2b3o50bo2bo12bobo55bo
        bo17bo51bobo4bobo$8bo2b3o2bo2bo3b2o28bo3bo16b2obo32b4o34bo3bo13b2o33b
        2ob2ob2o13b2ob2obo15b3o10b2o35b2o58b3o11b2o6b3o14bo12bo19b2o15bob2o15b
        2o36b2o4bo12bo33bo2b2o13bo2bo2b2o2bo$17b3o32bo4bo17b2o17b3o12bob5o40bo
        8bo2bo32bo7bo12b4o19bo10b2o38bo17b2o41bo18b3o14bo10bobo36b3o15bo2bo39b
        obo9b3o33b2o2b3o13b2o6b2o$10b2o40bo4bo8b2o25bo14b2o6bo40b2o7b3o31b2o6b
        o7b2o6b3o3bo13b2ob2o6bo11bo46bo4bo46b2o27bobo6b3o9b2o26bo16bo2bo40b2o
        44b3o2b2o$10bo18b2o21bo3bo9b2o11b2o12bo2bo12b3obo3bo11b2o25b2o8b2o11b
        2o20b2o2b2o9b2o12bo9bobo2b3o8b5o5b2o11b2o26bobo4bo14b2o35b2o11b2o11bo
        5b2o4bo11b2o11b2o30b2ob2o13b2o48b2o21b2o2b2o$29bo13bobo7bo2bo22bo13b3o
        14b2o2bobo12bo49bo21b2o2b2o22bo11bo4bo10bo21bo28b2o5bo2bo10bo49bo12bob
        o34bo33b2o14bo49bo$30b3o9bo37b3o47b3o47b3o12bo34b3o9bo2b2o33b3o25bo8bo
        12b3o47b3o47b3o47b3o47b3o9b3o$19b2o3b2ob3o2bo9bo2bo36bo49bo49bo12bo36b
        o11b2o36bo49bo49bo11bo37bo24bo24bo49bo9b3o$20b2obobobo2bo12bobo7b2o3bo
        44b2o3bo44b2o3bo35bo8b2o3bo35bo8b2o3bo44b2o3bo44b2o3bo33bo2bo7b2o3bo
        44b2o3bo44b2o3bo35bo8b2o3bo$6b2o11bo4bo2bobo23bo3bobo43bo3bobo43bo3bob
        o43bo3bobo43bo3bobo43bo3bobo43bo3bobo35bo7bo3bobo43bo5bo43bo3bobo43bo
        3bobo$7bo19bob2o9b2o12bo3bobo29b2o12bo3bobo29b2o12bo3bobo29b2o12bo3bob
        o29b2o12bo3bobo29b2o12bo3bobo29b2o12bo3bobo29bo3bo9bo3bobo29b2o12bo2b
        2obo29b2o12bo3bobo29b2o12bo3b2o$7bobo16b2o3bo7bobo13bo4bo28bobo13bo4bo
        28bobo13bo4bo28bobo13bo4bo28bobo13bo4bo28bobo13bo4bo28bobo13bo4bo28bob
        2o12bo4bo28bobo13bo4bo28bobo13bo4bo28bobo13bo11b3o$8b2o13bobo2bobo8bo
        12b3o5b2o27bo12b3o5b2o27bo12b3o5b2o27bo12b3o5b2o27bo12b3o5b2o27bo12b3o
        5b2o27bo12b3o5b2o27bo12b3o5b2o27bo12b3o5b2o27bo12b3o5b2o27bo13bob5o7bo
        12b2o$23b2o2b2ob2o6b2o12bo35b2o12bo35b2o12bo35b2o12bo35b2o12bo35b2o12b
        o35b2o12bo35b2o12bo35b2o12bo35b2o12bo35b2o13b2o4bo7b3o6b2o2bo$11b2o
        543b2obob2o13bobobo$12b2o539b2obobobobo5bo10bo$13b3o537bobo4bo7b3o$13b
        3o543b2o9b3o4bo$12bo557b2o4bobo$577bo$11bo3bobo560b3o$15b2o563bo$12b2o
        b2o$14bo538bo$9b2o542b3o$9b2o14b2o529bo$25bo529b2o14b2o$26b3o542b2o$
        28bo538bo$565b2ob2o$bo563b2o$b3o560bobo3bo$4bo$3bobo4b2o557bo$4bo4b3o
        9b2o543b3o$11b3o7bo4bobo537b3o$2bo10bo5bobobobob2o539b2o$bobobo13b2obo
        b2o543b2o$bo2b2o6b3o7bo4b2o13b2o35bo12b2o35bo12b2o35bo12b2o35bo12b2o
        35bo12b2o35bo12b2o35bo12b2o35bo12b2o35bo12b2o35bo12b2o6b2ob2o2b2o$2o
        12bo7b5obo13bo27b2o5b3o12bo27b2o5b3o12bo27b2o5b3o12bo27b2o5b3o12bo27b
        2o5b3o12bo27b2o5b3o12bo27b2o5b3o12bo27b2o5b3o12bo27b2o5b3o12bo27b2o5b
        3o12bo8bobo2bobo13b2o$12b3o11bo13bobo28bo4bo13bobo28bo4bo13bobo28bo4bo
        12b2obo28bo4bo13bobo28bo4bo13bobo28bo4bo13bobo28bo4bo13bobo28bo4bo13bo
        bo28bo4bo13bobo28bo4bo13bobo7bo3b2o16bobo$22b2o3bo12b2o29bobo3bo12b2o
        29bob2o2bo12b2o29bobo3bo9bo3bo29bobo3bo12b2o29bobo3bo12b2o29bobo3bo12b
        2o29bobo3bo12b2o29bobo3bo12b2o29bobo3bo12b2o29bobo3bo12b2o9b2obo19bo$
        22bobo3bo43bobo3bo43bo5bo43bobo3bo7bo35bobo3bo43bobo3bo43bobo3bo43bobo
        3bo43bobo3bo43bobo3bo43bobo3bo23bobo2bo4bo11b2o$23bo3b2o8bo35bo3b2o44b
        o3b2o44bo3b2o7bo2bo33bo3b2o44bo3b2o44bo3b2o8bo35bo3b2o8bo35bo3b2o44bo
        3b2o44bo3b2o7bobo12bo2bobobob2o$37b3o9bo49bo24bo24bo37bo11bo49bo49bo
        36b2o11bo36bo12bo49bo49bo36bo2bo9bo2b3ob2o3b2o$37b3o9b3o47b3o47b3o47b
        3o47b3o12bo8bo25b3o33b2o2bo9b3o34bo12b3o47b3o47b3o37bo9b3o$52bo49bo14b
        2o33bo34bobo12bo49bo10bo2bo5b2o28bo21bo10bo4bo11bo22b2o2b2o21bo49bo12b
        obo2b2o14b3o13bo22bo2bo7bobo13bo$24b2o2b2o21b2o48b2o13b2ob2o30b2o11b2o
        11bo4b2o5bo11b2o11b2o35b2o14bo4bobo26b2o11b2o5b5o8b3o2bobo9bo12b2o9b2o
        2b2o20b2o11b2o8b2o25b2o11bo3bob3o12bo2bo12b2o11b2o9bo3bo21b2o18bo$24b
        2o2b3o44b2o40bo2bo16bo26b2o9b3o6bobo27b2o46bo4bo46bo11bo6b2ob2o13bo3b
        3o6b2o7bo6b2o31b3o7b2o40bo6b2o14bo25b2o8bo4bo40b2o$2o6b2o13b3o2b2o33b
        3o9bobo39bo2bo15b3o36bobo10bo14b3o18bo41b2o17bo38b2o10bo19b4o12bo7bo
        32bo2bo8bo40b5obo12b3o17b2o17bo4bo32b3o$o2b2o2bo2bo13b2o2bo33bo12bo4b
        2o36b2o15b2obo15b2o19bo12bo14b3o6b2o11b3o58b2o35b2o10b3o15bob2ob2o13b
        2ob2ob2o33b2o13bo3bo34b4o32bob2o16bo3bo28b2o3bo2bo2b3o2bo$bobo4bobo51b
        o17bobo55bobo12bo2bo50b3o2bobo13bob2o31b3o15b2o2b3o34bo32bo2bob2o13bo
        3b2o50bo2b2o33bo33b2obobo16bobo38bo3bobobo$2obo5bo70bob2o28bo17bo4b2o
        4bo11bo2bo29bobo20bo4bo11bo4bo29bo19b2o5bo14b3o48b2obo3b2o68bo3bo67bo
        2bob2o53bo2bo4bo2bo$3bob2o58bo13bob2o29bo16bo3b5ob2o11b5o31bobo17bo2bo
        2bo13b3o29bobo17b3o4bo12b2obobo48b2obobobo69bo3bo71b2o55bo7b2o$3bo2bo
        8b3o25b3o14bo4bo13b3o17b4o30b3obob2o8b2ob2o38bo18b3o33b2o14bo15b2o2bo
        3bo12bo2bobo33b2o17b3o31b2o3b2o11bobo15b2o3b2o14bob4o34bo10bo3b4o35b2o
        15b2o$4bo4bo9bo21bo3b2o13bo42bo29bo17bo41bo52b3ob2o6bo3bo16bo5bo12b3o
        36b2o2bo17bo30b4o3bobo10bo18b4o16b2o4bo33b2o8bo42bo2b2o13bo$5b5o4bo4bo
        21bobo2b2o13bo2bo24b2o8bo4bo25bo8b2o52b2o44b2o6b2ob3o4bo2bo2b2o24b2o7b
        2o37b2ob2o6bo40bo2bo2bo3bo9bo3b2o16bo7b2o8bo3bob2o31b2o9b3o39b3o2bo$
        13bobo2bo7b2o12bo5bo14b3o12b2o11b2o9b2obo9bo12b2o48b2o14b2o3bo28b2o11b
        2o5b3ob2o6b3ob2obo10b2o11b2o8bo26b2o10b3o7bobo25b2o11bo4bo3b2o10b3ob2o
        10b2o11b2o9b3obo21b2o48b2o13bo4bo18b3o2b2o$7bo5bo2bo10bo13bo2b2o31bo
        24bo10bo13bo49bo14bo3bo30bo18b2o29bo49bo20b2o27bo12bo3bo19bo12bo35bo
        13bo49bo13b5o19b2o4bo2bo$6bobo5b2o8b3o14bo32b3o35bo11b3o47b3o47b3o34b
        2o2bo8b3o47b3o47b3o18bo15b3o10b3o35bobo9b3o47b3o15b2o21bo5bobobo$7bo
        16bo49bo36b2o11bo49bo24bo24bo39bo9bo49bo49bo37bo11bo36b2o11bo49bo43b2o
        bo2bo$48bo3b2o44bo3b2o9bo34bo3b2o45bo2b2o44bo3b2o7b3o34bo3b2o44bo3b2o
        44bo3b2o44bo3b2o44bo3b2o44bo3b2o14bo2bo$47bobo3bo43bobo3bo7b2o34bobo3b
        o43bobo3bo43bobo3bo8bo34bobo3bo43bobo3bo43bobo3bo43bobo3bo43bobo3bo43b
        obo3bo11bo4bo$46bobo3bo12b2o29bobo3bo12b2o29bobo3bo12b2o29bobo3bo12b2o
        29bobo3bo12b2o29bobo3bo12b2o29bobo3bo12b2o29bobo3bo12b2o29bobo3bo12b2o
        29bobo3bo12b2o29bobo3bo12b5o$46bo4bo13bobo28bo4bo13bobo28bo4bo13bobo
        28bo4bo13bobo28bo4bo13bobo28bo4bo13bobo28bo4bo13bobo28bo4bo13bobo28bo
        4bo13bobo28bo4bo13bobo28bo4bo$45b2o5b3o12bo27b2o5b3o12bo27b2o5b3o12bo
        27b2o5b3o12bo27b2o5b3o12bo27b2o5b3o12bo27b2o5b3o12bo27b2o5b3o12bo27b2o
        5b3o12bo27b2o5b3o12bo27b2o5b3o12bo$54bo12b2o35bo12b2o35bo12b2o35bo12b
        2o35bo12b2o35bo12b2o35bo12b2o35bo12b2o35bo12b2o35bo12b2o35bo11bobo$
        567bo!''', 'b3s23')

        self.assertEqual(p57loop.period, 57)

        p57a = p57loop.apgcode
        self.assertEqual(len(p57a), 3083)
        self.assertEqual(self.lt.pattern(p57a).period, 57)

        p57gun = p57loop.replace(just_eater, with_catalyst, n_phases=57, orientations=['identity', 'rot180'])

        self.assertEqual(p57gun.population, 6690)
        self.assertEqual(p57gun[5700].population, 16690)

if __name__ == '__main__':
    unittest.main()
