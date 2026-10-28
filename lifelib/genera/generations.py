
from ._iwriter import iwriter

family = 2
mantissa = {0, 1}

def number_of_states(rulestring):

    return int(rulestring[1:rulestring.index('b')])

def bitplanes(rulestring):

    nstates = number_of_states(rulestring)
    return (2 if (nstates == 3) else len(bin(nstates - 3)))

def create_rule(rulestring):

    nstates = number_of_states(rulestring)

    if (nstates < 3):
        raise ValueError("Number of states must be at least 3")

    logstring = rulestring[rulestring.index('b'):]
    for iset in [['sse2'], ['sse2', 'avx'], ['sse2', 'avx', 'avx2']]:
        with open('ll_%s_%s.asm' % (iset[-1], logstring), 'w') as f:
            ix = genewriter(f, iset)
            ix.genlogic(logstring)

    with open('iterators_%s.h' % rulestring, 'w') as f:
        f.write('#pragma once\n')
        f.write('#include <stdint.h>\n')
        f.write('#include "../lifeconsts.h"\n')
        f.write('#include "../lifeperm.h"\n')
        f.write('#include "../eors.h"\n')
        f.write('namespace %s {\n\n' % rulestring.replace('-', '_'))

        for iset in [['sse2'], ['sse2', 'avx'], ['sse2', 'avx', 'avx2']]:
            iw = genewriter(f, iset)
            iw.write_function(rulestring, 20, 16)
            iw.write_iterator()

        gwrite_leaf_iterator(f, nstates)
        f.write('}\n')

class genewriter(iwriter):

    def write_function(self, rulestring, rowcount, dwidth):

        name = 'iterate_%s_%d_%d' % (self.besti, rowcount, dwidth)
        params = 'uint32_t * __restrict__ d, uint32_t * __restrict__ e, uint32_t * __restrict__ h, uint32_t * __restrict__ j'
        self.f.write('    void %s(%s) {\n' % (name, params))
        logstring = rulestring[rulestring.index('b'):]
        self.assemble(logstring, 0, rowcount, dwidth)
        self.f.write('            for (int i = 1; i < %d; i++) {\n' % (rowcount - 1))
        self.f.write('                e[i-1] &= (~h[i]);\n')
        self.f.write('                j[i] = d[i] & (~e[i-1]);\n')
        self.f.write('                d[i] = e[i-1];\n')
        self.f.write('            }\n')
        self.f.write('        return;\n')
        self.f.write('    }\n\n')

    def write_iterator(self):

        name = 'iterate_var_%s' % self.besti
        self.f.write('    void %s(uint32_t * __restrict__ d, uint32_t * __restrict__ h) {\n' % name)
        self.f.write('        uint32_t e[32];\n')
        self.f.write('        uint32_t j[32];\n')
        self.f.write('        iterate_%s_20_16(d+6, e+6, h+6, j+6);\n' % self.besti)
        self.f.write('        for (int i = 8; i < 24; i++) { h[i] = j[i]; }\n')
        self.f.write('        return;\n')
        self.f.write('    }\n\n')

def gwli_bsi(f, bsi, msi):

    f.write('            apg::z64_to_r32_%s(inleaves, d);\n' % bsi)
    f.write('            apg::z64_to_r32_%s(hleaves, h);\n' % bsi)
    f.write('            iterate_var_%s(d, h);\n' % bsi)
    f.write('            apg::r32_centre_to_z64_%s(d, outleaf);\n' % msi)
    f.write('            apg::r32_centre_to_z64_%s(h, outleaf2);\n' % msi)

def gwrite_leaf_iterator(f, nstates):

    name = 'iterate_var_leaf'
    params = 'uint64_t * inleaves, uint64_t * hleaves, uint64_t * outleaf'
    f.write('    bool %s(%s) {\n' % (name, params))
    f.write('        uint64_t outleaf2[4];')
    f.write('        int bis = apg::best_instruction_set();\n')
    f.write('        uint32_t d[32];\n')
    f.write('        uint32_t h[32];\n')
    f.write('        if (bis >= 10) {\n')
    gwli_bsi(f, 'avx2', 'avx2')
    f.write('        } else if (bis >= 9) {\n')
    gwli_bsi(f, 'avx', 'avx')
    f.write('        } else if (bis >= 7) {\n')
    gwli_bsi(f, 'sse2', 'sse4')
    f.write('        } else {\n')
    gwli_bsi(f, 'sse2', 'ssse3')
    f.write('        }\n')

    postprocess(f, nstates)
    f.write('        return false;\n')
    f.write('    }\n\n')

def postprocess(f, nstates):

    br = '' if (nstates == 3) else (bin(nstates - 3)[2:])[::-1]

    # We run 256 parallel binary counters in outleaf:
    f.write('        for (int i = 0; i < 4; i++) {\n')
    f.write('            uint64_t carry = outleaf[4+i];\n')
    for i, c in enumerate(br):
        f.write('            outleaf[%d+i] ^= carry;\n' % (4 * i + 8))
        f.write('            carry &= outleaf[%d+i];\n' % (4 * i + 8))
        if (c == '1'):
            f.write('            outleaf[%d+i] |= outleaf2[i];\n' % (4 * i + 8))
    f.write('            outleaf[4+i] ^= carry;\n')
    for i, c in enumerate(br):
        f.write('            outleaf[%d+i] ^= carry;\n' % (4 * i + 8))
    f.write('            outleaf[4+i] |= outleaf2[i];\n')
    f.write('        }\n')
