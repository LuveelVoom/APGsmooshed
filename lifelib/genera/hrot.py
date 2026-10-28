
from .ltl import mantissa, family, bitplanes

def get_hrot_params(rulestring):

    rulerange = int(rulestring[1])
    dhexchars = rulerange * (rulerange + 1)
    gparams = rulestring[3:].replace('z', '').split('s')

    if len(gparams[0]) != dhexchars:
        raise ValueError("Birth conditions for range-%d rules must have exactly %d hex characters, not %d" % (rulerange, dhexchars, len(gparams[0])))

    if len(gparams[1]) != dhexchars:
        raise ValueError("Survival conditions for range-%d rules must have exactly %d hex characters, not %d" % (rulerange, dhexchars, len(gparams[1])))

    gparams = tuple([rulerange] + [int(x, 16) for x in gparams] + ['z' in rulestring])

    return gparams

def write_rule_data(f, b_int, s_int, s_zero):

    b_int = 2 * b_int
    s_int = 4 * s_int + (2 if s_zero else 0)

    births    = [(b_int >> (8 * i)) & 255 for i in range(16)]
    survivals = [(s_int >> (8 * i)) & 255 for i in range(16)]

    marray = [b ^ s for (b, s) in zip(births, survivals)]
    marray += [255 ^ s for s in survivals]

    marray += ([15] * 16)
    marray += [1, 2, 4, 8, 16, 32, 64, 128]
    marray += [1, 2, 4, 8, 16, 32, 64, 128]

    f.write('const static uint8_t ruledata[] __attribute__((aligned(64))) = {%d,\n' % marray[0])
    for i in range(3):
        currstring = '   '
        for j in range(i*21+1, i*21+22):
            currstring += ((' %3d};' if (j == 63) else ' %3d,') % marray[j])
        f.write(currstring + '\n')


def create_rule(rulestring):

    rulerange, b_int, s_int, s_zero = get_hrot_params(rulestring)

    with open('iterators_%s.h' % rulestring, 'w') as f:
        f.write('#pragma once\n')
        f.write('#include <stdint.h>\n')
        f.write('#include "../ltl.h"\n')
        f.write('namespace %s {\n\n' % rulestring.replace('-', '_'))

        write_rule_data(f, b_int, s_int, s_zero)

        f.write('    bool iterate_var_leaf(uint64_t *inleaves, uint64_t *outleaf) {\n\n')
        f.write('        apg::hrot_kernel(inleaves, outleaf, %d, ruledata);\n' % rulerange)
        f.write('        return false;\n')
        f.write('    }\n')
        f.write('}\n')
