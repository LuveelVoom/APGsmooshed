
class iwriter_base(object):

    def __init__(self, f, iset):
        self.f = f
        self.iset = iset
        for k in ['sse2', 'sse3', 'ssse3', 'sse4', 'avx', 'avx2', 'avx512']:
            if k in iset:
                self.besti = k

    def printinstr(self, s):

        self.f.write('        "%s \\n\\t"\n' % s)

    def printcomment(self, s):

        self.f.write('        // %s \n' % s)

    def preparethings(self, dwidth, reg13=13, reg14=14):

        if ('avx512' in self.iset):
            self.printinstr('vmovdqu64 (%2), %%zmm14')
            self.printinstr('vmovdqu64 64(%2), %%zmm16')
            self.printinstr('vmovdqu64 128(%2), %%zmm17')
        elif ('avx2' in self.iset):
            self.printinstr('vmovdqu (%2), %%ymm'+str(reg14))
            self.printinstr('vmovdqu 192(%2), %%ymm'+str(reg13))
        else:
            r13 = '%%xmm' + str(reg13)
            r14 = '%%xmm' + str(reg14)
            shuf = 'vpshufd' if ('avx' in self.iset) else 'pshufd'
            self.printinstr('mov $0xffffffff, %%ebx')
            self.printinstr('movd %%ebx, ' + r13)
            if (dwidth):
                ddic = {28: '0x3ffffffc',
                        24: '0x0ffffff0',
                        20: '0x03ffffc0',
                        16: '0x00ffff00',
                        12: '0x003ffc00',
                         8: '0x000ff000',
                         4: '0x0003c000'}
                self.printinstr('mov $' + ddic[dwidth] + ', %%ebx')
                self.printinstr('movd %%ebx, ' + r14)
            self.printinstr('%s $1, %s, %s' % (shuf, r13, r13))
            self.printinstr('%s $0, %s, %s' % (shuf, r14, r14))

    def trogicgate(self, op, inreg1, inreg2, inreg3, outreg=None):

        if outreg is None:
            outreg = inreg3

        if (outreg != inreg3):
            self.printinstr('vmovdqa32 %s, %s' % ('%%zmm' + str(inreg3), '%%zmm' + str(outreg)))

        self.printinstr('vpternlogd $%d, %s, %s, %s' % (op, '%%zmm' + str(inreg1), '%%zmm' + str(inreg2), '%%zmm' + str(outreg)))

    def logicgate(self, op, inreg1, inreg2, outreg, regname=None):

        if regname is None:
            regname = '%%zmm' if ('avx512' in self.iset) else ('%%ymm' if ('avx2' in self.iset) else '%%xmm')
        i1 = regname + str(inreg1)
        i2 = regname + str(inreg2)
        o1 = regname + str(outreg)

        if 'avx' in self.iset:
            self.printinstr('v%s %s, %s, %s' % (op, i1, i2, o1))
        elif (i2 == o1):
            self.printinstr('%s %s, %s' % (op, i1, o1))
        elif (i1 == o1):
            if (op == 'pandn'):
                self.printinstr('por %s, %s' % (i2, o1))
                self.printinstr('pxor %s, %s' % (i2, o1))
            else:
                self.printinstr('%s %s, %s' % (op, i2, o1))
        else:
            self.printinstr('movdqa %s, %s' % (i2, o1))
            self.printinstr('%s %s, %s' % (op, i1, o1))

    def write16n(self, n, inreg, offset, outloc):

        inregz = '%%zmm' + str(inreg)
        inregy = '%%ymm' + str(inreg)
        inregx = '%%xmm' + str(inreg)

        reg16z = '%%zmm13'
        reg16y = '%%ymm13'
        reg16x = '%%xmm13'

        d1 = ('' if (offset == 0) else str(offset)) + outloc
        d2 = str(offset + 32) + outloc

        if (n == 16):
            self.printinstr('vmovdqu %s, %s' % (inregx, d1))
        elif (n == 32):
            self.printinstr('vmovdqu %s, %s' % (inregy, d1))
        elif (n == 48):
            self.printinstr('vshufi32x4 $78, %s, %s, %s' % (inregz, inregz, reg16z))
            self.printinstr('vmovdqu %s, %s' % (inregy, d1))
            self.printinstr('vmovdqu %s, %s' % (reg16x, d2))
        elif (n == 64):
            self.printinstr('vmovdqu64 %s, %s' % (inregz, d1))

    def read16n(self, n, inloc, offset, outreg):

        outregz = '%%zmm' + str(outreg)
        outregy = '%%ymm' + str(outreg)
        outregx = '%%xmm' + str(outreg)

        reg16z = '%%zmm13'
        reg16y = '%%ymm13'
        reg16x = '%%xmm13'

        d1 = ('' if (offset == 0) else str(offset)) + inloc
        d2 = str(offset + 32) + inloc

        if (n == 16):
            self.printinstr('vmovdqu %s, %s' % (d1, outregx))
        elif (n == 32):
            self.printinstr('vmovdqu %s, %s' % (d1, outregy))
        elif (n == 48):
            self.printinstr('vmovdqu %s, %s' % (d1, outregy))
            self.printinstr('vmovdqu %s, %s' % (d2, reg16x))
            self.printinstr('vshufi32x4 $68, %s, %s, %s' % (reg16z, outregz, outregz))
        elif (n == 64):
            self.printinstr('vmovdqu64 %s, %s' % (d1, outregz))

    def write_function(self, rulestring, rowcount, dwidth):

        name = 'iterate_%s_%d_%d' % (self.besti, rowcount, dwidth)

        params = 'uint32_t * __restrict__ diffs, bool onegen'
        for i in 'jhed':
            params = 'uint32_t * __restrict__ ' + i + ', ' + params

        self.f.write('    bool %s(%s) {\n' % (name, params))

        self.f.write('        if (h) {\n')
        self.f.write('            for (int i = 0; i < %d; i++) {\n' % (rowcount))
        self.f.write('                h[i] |= d[i];\n')
        self.f.write('            }\n')
        self.f.write('        }\n')

        self.f.write('        if (j) {\n')
        self.f.write('            for (int i = 0; i < %d; i++) {\n' % (rowcount))
        self.f.write('                j[i] &= d[i];\n')
        self.f.write('            }\n')
        self.f.write('        }\n')

        self.assemble(rulestring, 0, rowcount, dwidth)

        self.f.write('        if (h) {\n')
        self.f.write('            for (int i = 1; i < %d; i++) {\n' % (rowcount - 1))
        if (rulestring[:2] == 'b0'):
            # We want the history state to match the envelope of the Gollyfied
            # version of the B0 rule:
            self.f.write('                h[i] |= (~e[i-1]);\n')
        else:
            self.f.write('                h[i] |= e[i-1];\n')
        self.f.write('            }\n')
        self.f.write('        }\n')

        self.f.write('        if (j) {\n')
        self.f.write('            for (int i = 1; i < %d; i++) {\n' % (rowcount - 1))
        self.f.write('                j[i] &= e[i-1];\n')
        self.f.write('            }\n')
        self.f.write('        }\n')

        self.f.write('        if (onegen) {\n')
        self.f.write('            for (int i = 2; i < %d; i++) {\n' % (rowcount - 2))
        self.f.write('                d[i] = e[i-1];\n')
        self.f.write('            }\n')
        self.f.write('            return false;\n')
        self.f.write('        }\n')

        self.assemble(rulestring, 1, rowcount, dwidth)

        self.f.write('        if (h) {\n')
        self.f.write('            for (int i = 2; i < %d; i++) {\n' % (rowcount - 2))
        self.f.write('                h[i] |= d[i];\n')
        self.f.write('            }\n')
        self.f.write('        }\n')

        self.f.write('        if (j) {\n')
        self.f.write('            for (int i = 2; i < %d; i++) {\n' % (rowcount - 2))
        self.f.write('                j[i] &= d[i];\n')
        self.f.write('            }\n')
        self.f.write('        }\n')

        if 'avx512' in self.iset:
            newrows = rowcount - 4

            while (newrows >= 32):
                newrows -= 16

            if (newrows <= 16):
                bindices = list(range(0, newrows // 2))
            elif (newrows == 28):
                bindices = [8, 9, 10, 11, 12, 13]
            else:
                bindices = list(range(4, newrows // 2))
            lindices = list(range(newrows - 2, newrows))

        elif 'avx2' in self.iset:

            bindices = [4, 5, 6, 7]
            lindices = list(range(16 + ((rowcount - 6) % 8), 18 + ((rowcount - 6) % 8)))

        else:

            bindices = [2, 3]
            lindices = list(range(8 + ((rowcount - 6) % 4), 10 + ((rowcount - 6) % 4)))

        self.f.write('        uint64_t* e64 = ((uint64_t*) e);\n')
        self.f.write('        uint64_t bigdiff = %s;\n' % (' | '.join(['e64[%d]' % x for x in bindices])))
        self.f.write('        if (diffs != 0) {\n')
        self.f.write('            diffs[0] = (bigdiff | (bigdiff >> 32));\n')
        self.f.write('            diffs[1] = e[0] | e[1];\n')
        self.f.write('            diffs[2] = %s;\n' % (' | '.join(['e[%d]' % x for x in lindices])))
        self.f.write('        }\n')
        self.f.write('        return (bigdiff == 0);\n')
        self.f.write('    }\n\n')

    def write_iterator(self):

        name = 'iterate_var_%s' % self.besti
        self.f.write('    int %s(int n, uint32_t * __restrict__ d, uint32_t * __restrict__ h, uint32_t * __restrict__ j) {\n' % name)
        self.f.write('        uint32_t e[32];\n')
        self.f.write('        if (n >= 7) { if (iterate_%s_32_28(d, e, h, j, 0, (n == 7))) {return 8;} }\n' % self.besti)
        self.f.write('        if (n >= 5) { if (iterate_%s_28_24(d+2, e+2, h+2, j+2, 0, (n == 5))) {return 6;} }\n' % self.besti)
        self.f.write('        if (n >= 3) { if (iterate_%s_24_20(d+4, e+4, h+4, j+4, 0, (n == 3))) {return 4;} }\n' % self.besti)
        self.f.write('        if (n >= 1) { if (iterate_%s_20_16(d+6, e+6, h+6, j+6, 0, (n == 1))) {return 2;} }\n' % self.besti)
        self.f.write('        return 0;\n')
        self.f.write('    }\n\n')

        self.f.write('    int %s(int n, uint32_t * __restrict__ d, uint32_t * __restrict__ h) {\n' % name)
        self.f.write('        uint32_t e[32];\n')
        self.f.write('        if (n >= 7) { if (iterate_%s_32_28(d, e, h, 0, 0, (n == 7))) {return 8;} }\n' % self.besti)
        self.f.write('        if (n >= 5) { if (iterate_%s_28_24(d+2, e+2, h+2, 0, 0, (n == 5))) {return 6;} }\n' % self.besti)
        self.f.write('        if (n >= 3) { if (iterate_%s_24_20(d+4, e+4, h+4, 0, 0, (n == 3))) {return 4;} }\n' % self.besti)
        self.f.write('        if (n >= 1) { if (iterate_%s_20_16(d+6, e+6, h+6, 0, 0, (n == 1))) {return 2;} }\n' % self.besti)
        self.f.write('        return 0;\n')
        self.f.write('    }\n\n')

        self.f.write('    int %s(int n, uint32_t * __restrict__ d) {\n' % name)
        self.f.write('        uint32_t e[32];\n')
        self.f.write('        if (n >= 7) { if (iterate_%s_32_28(d, e, 0, 0, 0, (n == 7))) {return 8;} }\n' % self.besti)
        self.f.write('        if (n >= 5) { if (iterate_%s_28_24(d+2, e+2, 0, 0, 0, (n == 5))) {return 6;} }\n' % self.besti)
        self.f.write('        if (n >= 3) { if (iterate_%s_24_20(d+4, e+4, 0, 0, 0, (n == 3))) {return 4;} }\n' % self.besti)
        self.f.write('        if (n >= 1) { if (iterate_%s_20_16(d+6, e+6, 0, 0, 0, (n == 1))) {return 2;} }\n' % self.besti)
        self.f.write('        return 0;\n')
        self.f.write('    }\n\n')


def rule2gates(rulestring):

    bee = [0] * 10
    ess = [0] * 10

    for char in rulestring:
        if (char == 'b'):
            birth = True
        elif (char == 's'):
            birth = False
        else:
            k = int(char)
            if (birth):
                bee[k] = 1
            else:
                ess[k+1] = 1

    negate = bee[0]
    beexor = (bee[0] != bee[8])
    essxor = (ess[1] != ess[9])

    stars = ("*" if essxor else "") + ("**" if beexor else "") + ("!" if negate else "")

    if negate:
        bee = [1 - x for x in bee]
        ess = [1 - x for x in ess]

    ess[0] = (1 - ess[8]) if essxor else ess[8]

    ruleint = 0;
    for i in range(8):
        ruleint += (bee[i] << i)
        ruleint += (ess[i] << (i + 8))

    rident = None

    with open('../boolean.out', 'r') as f:
        for fline in f:
            x = fline.split(":  ")
            if (len(x) == 2) and (int(x[0]) == ruleint):
                rident = x[1][:-1]

    if (ruleint == 0):
        rident = '-004'
    if (ruleint == 65280):
        rident = '-331'
    if (ruleint == 61680):
        rident = '-221'
    if (ruleint == 52428):
        rident = '-111'
    if (ruleint == 43690):
        rident = '-001'

    if rident is None:
        raise ValueError("Error: unrecognised rule")
    # print(("Rule circuit:     ["+rident+"]"+stars))

    rchars = list(rident)
    for i in range(len(rchars)):
        if (rchars[i] == '-'):
            rchars[i] = (i//4) + 4
        else:
            rchars[i] = int(rchars[i])

    for i in range(0, len(rchars), 4):
        if (rchars[i+3] == 3):
            rchars[i+3] = 2
            rchars[i+2] ^= rchars[i+1]
            rchars[i+1] ^= rchars[i+2]
            rchars[i+2] ^= rchars[i+1]

    # Map logical registers to physical registers:
    for i in range(0, len(rchars), 4):
        dependencies = [False] * 10
        for j in range(i+4, len(rchars), 4):
            dependencies[rchars[j+1]] = True
            dependencies[rchars[j+2]] = True
        d = -1
        e = rchars[i]
        for j in range(6):
            if (e == j):
                d = j
                break
            if not dependencies[j]:
                d = j
                break
        if (d == -1):
            raise RuntimeError("Error: insufficiently many physical registers")
        else:
            # print(str(e)+" --> "+str(d))
            rchars[i] = d
            for j in range(i+4, len(rchars), 4):
                if (rchars[j+1] == e):
                    rchars[j+1] = d
                if (rchars[j+2] == e):
                    rchars[j+2] = d

    return (rchars, negate, beexor, essxor)


class iwriter(iwriter_base):

    def load_and_hshift(self, i, oddgen, terminal):

        regbytes = 32 if ('avx2' in self.iset) else 16
        regname = '%%ymm' if (('avx2' in self.iset) and not terminal) else '%%xmm'
        accessor = 'vmovdqu' if ('avx' in self.iset) else 'movups'
        inreg = regname + str(5 - 3 * (i % 2))

        d = '(%1)' if (oddgen) else '(%0)'
        d = d if (i == 0) else (str(regbytes * i) + d)

        self.printinstr('%s %s, %s' % (accessor, d, inreg))
        if ('avx' in self.iset):
            self.printinstr('vpsrld $1, %s, %s0' % (inreg, regname))
            self.printinstr('vpslld $1, %s, %s1' % (inreg, regname))
        else:
            self.printinstr('movdqa %s, %s0' % (inreg, regname))
            self.printinstr('movdqa %s, %s1' % (inreg, regname))
            self.printinstr('psrld $1, %s0' % regname)
            self.printinstr('pslld $1, %s1' % regname)

    def horizontal_adders(self, i):

        self.logicgate('pxor', 0, 1, 6 - 3 * (i % 2))
        self.logicgate('pand', 0, 1, 7 - 3 * (i % 2))
        self.logicgate('pand', 5 - 3 * (i % 2), 6 - 3 * (i % 2), 1)
        self.logicgate('pxor', 5 - 3 * (i % 2), 6 - 3 * (i % 2), 6 - 3 * (i % 2))
        self.logicgate('por', 1, 7 - 3 * (i % 2), 7 - 3 * (i % 2))

    def vertical_bitshifts(self, i):

        if 'avx2' in self.iset:
            self.logicgate('pblendd $1,', 6 - 3 * (i % 2), 3 + 3 * (i % 2), 8)
            self.logicgate('pblendd $1,', 7 - 3 * (i % 2), 4 + 3 * (i % 2), 9)
            self.logicgate('pblendd $3,', 6 - 3 * (i % 2), 3 + 3 * (i % 2), 10)
            self.logicgate('pblendd $3,', 7 - 3 * (i % 2), 4 + 3 * (i % 2), 11)
            self.logicgate('pblendd $1,', 5 - 3 * (i % 2), 2 + 3 * (i % 2), 12)
            self.logicgate('permd', 8, 13, 8)
            self.logicgate('permd', 9, 13, 9)
            self.printinstr('vpermq $57, %%ymm10, %%ymm10')
            self.printinstr('vpermq $57, %%ymm11, %%ymm11')
            self.logicgate('permd', 12, 13, 12)
        else:
            self.logicgate('pand', 13, 3 + 3 * (i % 2), 8)
            self.logicgate('pand', 13, 4 + 3 * (i % 2), 9)
            self.logicgate('pand', 13, 2 + 3 * (i % 2), 12)
            self.logicgate('pandn', 6 - 3 * (i % 2), 13, 0)
            self.logicgate('por', 0, 8, 8)
            self.logicgate('pandn', 7 - 3 * (i % 2), 13, 0)
            self.logicgate('por', 0, 9, 9)
            self.logicgate('pandn', 5 - 3 * (i % 2), 13, 0)
            self.logicgate('por', 0, 12, 12)
            self.logicgate('shufps $0x39,', 8, 8, 8)
            self.logicgate('shufps $0x39,', 9, 9, 9)
            self.logicgate('shufps $0x4e,', 6 - 3 * (i % 2), 3 + 3 * (i % 2), 10)
            self.logicgate('shufps $0x4e,', 7 - 3 * (i % 2), 4 + 3 * (i % 2), 11)
            self.logicgate('shufps $0x39,', 12, 12, 12)

    def vertical_adders(self, i):

        self.logicgate('pxor', 3 + 3 * (i % 2), 8, 8)
        self.logicgate('pxor', 4 + 3 * (i % 2), 9, 9)
        self.logicgate('pxor', 8, 10, 10)
        self.logicgate('pxor', 9, 11, 11)
        self.logicgate('por', 8, 3 + 3 * (i % 2), 3 + 3 * (i % 2))
        self.logicgate('por', 9, 4 + 3 * (i % 2), 4 + 3 * (i % 2))
        self.logicgate('pand', 10, 8, 8)
        self.logicgate('pand', 11, 9, 9)
        self.logicgate('pandn', 3 + 3 * (i % 2), 8, 8)
        self.logicgate('pandn', 4 + 3 * (i % 2), 9, 9)

    def genlogic(self, rulestring):

        rchars, negate, beexor, essxor = rule2gates(rulestring)

        usetopbit = (essxor or beexor)

        self.logicgate('pand', 8, 11, 1)
        self.logicgate('pxor', 11, 8, 8)
        if (beexor and not essxor):
            self.logicgate('pand', 1, 9, 0)
            self.logicgate('pandn', 0, 12, 11)
        elif (usetopbit):
            self.logicgate('pand', 1, 9, 11)
        if (essxor and not beexor):
            self.logicgate('pand', 12, 11, 11)
        self.logicgate('pxor', 1, 9, 9)

        regnames = [10, 8, 9, 12, 1, 0]
        opnames = ["and", "or", "andn", "nonsense", "xor"]

        for i in range(0, len(rchars), 4):
            self.logicgate('p'+opnames[rchars[i+3]], regnames[rchars[i+1]], regnames[rchars[i+2]], regnames[rchars[i]])

        if (usetopbit):
            # printcomment(g, 'correct for B8/S8 nonsense:')
            self.logicgate('pxor', 11, 10, 10)

        if (negate):
            # Rule contains B0:
            self.logicgate('pcmpeqb', 11, 11, 11)
            self.logicgate('pxor', 11, 10, 10)


    def save_result(self, i, oddgen, terminal, diff=False):

        regbytes = 32 if ('avx2' in self.iset) else 16
        if oddgen:
            e = str(regbytes * (i - 1) + 8) + '(%0)'
        else:
            e = '(%1)' if (i == 1) else (str(regbytes * (i - 1)) + '(%1)')

        regname = '%%ymm' if (('avx2' in self.iset) and not terminal) else '%%xmm'
        accessor = 'vmovdqu' if ('avx' in self.iset) else 'movups'
        if diff:
            self.logicgate('pand', 14, 10, 10, regname)
            self.printinstr('%s %s, %s8' % (accessor, e, regname))
            self.logicgate('pandn', 8, 14, 11, regname)
            self.logicgate('por', 10, 11, 11, regname)
            self.printinstr('%s %s11, %s' % (accessor, regname, e))
            regname = '%%ymm' if ('avx2' in self.iset) else '%%xmm'
            if (diff == 'initial'):
                self.logicgate('pxor', 11, 8, 15)
                self.printinstr('%s %s15, %s' % (accessor, regname, '(%1)'))
            else:
                self.logicgate('pxor', 11, 8, 8)
                self.logicgate('por', 8, 15, 15)
            if (diff == 'final'):
                pos2 = '64(%1)' if ('avx2' in self.iset) else '32(%1)'
                pos1 = '32(%1)' if ('avx2' in self.iset) else '16(%1)'
                self.printinstr('%s %s8, %s' % (accessor, regname, pos2))
                self.printinstr('%s %s15, %s' % (accessor, regname, pos1))
        else:
            self.printinstr('%s %s10, %s' % (accessor, regname, e))

    def prologue(self):

        self.f.write('        asm (\n')

    def epilogue(self, dwidth):

        self.f.write('                : /* no output operands */ \n')
        self.f.write('                : "r" (d), "r" (e)')
        if (dwidth):
            self.f.write(', "r" (apg::__sixteen%d)' % dwidth)
        self.f.write('\n')
        self.f.write('                : "ebx", ')
        for i in range(16):
            self.f.write('"xmm%d", ' % i)
            if (i % 6 == 4):
                self.f.write('\n' + (' ' * 20))
        self.f.write('"memory");\n\n')

    def assemble(self, rulestring, oddgen, rowcount, dwidth):

        self.prologue()
        self.preparethings(dwidth)

        if ('avx2' in self.iset):
            iters = rowcount // 8 + (2 if ((rowcount % 8) and not oddgen) else 1)
            rpr = 8
        else:
            iters = rowcount // 4 + (0 if oddgen else 1)
            rpr = 4

        for i in range(iters):
            if (i * rpr < rowcount):
                terminal = ((i + 1) * rpr > rowcount)
                self.load_and_hshift(i, oddgen, terminal)
                self.horizontal_adders(i)
            if (i > 0):
                self.vertical_bitshifts(i)
                self.vertical_adders(i)
                self.f.write('#include "ll_%s_%s.asm"\n' % (self.besti, rulestring))
                terminal = (i * rpr == rowcount + (0 if oddgen else 4))
                if oddgen:
                    if (i == 1):
                        diff = 'initial'
                    elif (i + 1 == iters):
                        diff = 'final'
                    else:
                        diff = True
                else:
                    diff = False
                self.save_result(i, oddgen, terminal, diff)

        self.epilogue(dwidth)
