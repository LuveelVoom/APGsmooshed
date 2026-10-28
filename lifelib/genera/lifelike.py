
from ._iwriter import iwriter, rule2gates

family = 0
bitplanes = 1

def mantissa(rulestring):

    if 'b0' in rulestring:
        return {0, 2, 4, 6, 8}
    else:
        return {0, 1, 2, 3, 4, 5, 6, 7, 8}

def create_rule(rulestring):

    if ('b0' in rulestring) and (rulestring[-1] == '8'):
        raise ValueError('B0 and S8 are incompatible; please invert states.')

    logstring = rulestring[rulestring.index('b'):]
    for iset in [['sse2'], ['sse2', 'avx'], ['sse2', 'avx', 'avx2']]:
        with open('ll_%s_%s.asm' % (iset[-1], logstring), 'w') as f:
            ix = iwriter(f, iset)
            ix.genlogic(logstring)

    with open('iterators_gpu.h', 'w') as f:
        f.write('''#define ADVANCE_TILE_64(a, b, tmp, dst) {              \\
    uint64_cu al = ROTL64(a, 1);                       \\
    uint64_cu ar = ROTR64(a, 1);                       \\
    uint64_cu xor2 = al ^ ar;                          \\
    (dst)[threadIdx.x] = xor2 ^ a;                     \\
    __syncthreads();                                   \\
    uint64_cu uda = (dst)[u] & (dst)[d];               \\
    uint64_cu udx = (dst)[u] ^ (dst)[d];               \\
    uint64_cu xmm8 = ((dst)[threadIdx.x] & udx) | uda; \\
    uint64_cu xmm10 = (dst)[threadIdx.x] ^ udx;        \\
    (tmp)[threadIdx.x] = (al & ar) | (a & xor2);       \\
    __syncthreads();                                   \\
    udx = (tmp)[u] ^ (tmp)[d];                         \\
    uda = (tmp)[u] & (tmp)[d];                         \\
    uint64_cu xmm9 = ((tmp)[threadIdx.x] & udx) | uda; \\
    uint64_cu xmm11 = (tmp)[threadIdx.x] ^ udx;        \\
    xor2 = xmm8 & xmm11;                               \\
    uda = a;                                           \\
    xmm8 ^= xmm11;                                     \\\n''')

        rchars, negate, beexor, essxor = rule2gates(rulestring)
        usetopbit = (essxor or beexor)

        regnames = ["xmm10", "xmm8", "xmm9", "uda", "xor2", "udx"]
        opnames = [" & ", " | ", " & ~", "nonsense", " ^ "]

        if (usetopbit):
            f.write('''    xmm11 = xor2 & xmm9;                               \\\n''')
        if (essxor and not beexor):
            f.write('''    xmm11 &= a;                                        \\\n''')
        if (beexor and not essxor):
            f.write('''    xmm11 &= (~a);                                     \\\n''')

        f.write('''    xmm9 ^= xor2;                                      \\\n''')

        for i in range(0, len(rchars), 4):
            f.write('    %s = %s%s%s; \\\n' % (regnames[rchars[i]], regnames[rchars[i+1]], opnames[rchars[i+3]], regnames[rchars[i+2]]))

        if usetopbit:
            f.write('''    xmm10 ^= xmm11;                                    \\\n''')

        if negate:
            f.write('''    b = ~xmm10;                                        \\\n}\n\n''')
        else:
            f.write('''    b = xmm10;                                         \\\n}\n\n''')

    with open('iterators_%s.h' % rulestring, 'w') as f:
        f.write('#pragma once\n')
        f.write('#include <stdint.h>\n')
        f.write('#include "../lifeconsts.h"\n')
        f.write('#include "../lifeperm.h"\n')
        f.write('#include "../eors.h"\n')
        f.write('namespace %s {\n\n' % rulestring.replace('-', '_'))

        for iset in [['sse2'], ['sse2', 'avx'], ['sse2', 'avx', 'avx2']]:
            iw = iwriter(f, iset)
            iw.write_function(rulestring, 32, 28)
            iw.write_function(rulestring, 28, 24)
            iw.write_function(rulestring, 24, 20)
            iw.write_function(rulestring, 20, 16)
            iw.write_function(rulestring, 16, 12)
            iw.write_function(rulestring, 12, 8)
            iw.write_iterator()

        f.write('\n#include "../leaf_iterators.h"\n')
        f.write('}\n')
