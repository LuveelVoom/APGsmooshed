'''
In this file, we demonstrate the usability of genera by implementing the
Generations variant of Larger than Life, reusing much of the existing code
from both Generations and Larger than Life.
'''

from .generations import family, mantissa, postprocess
from .ltl import get_ltl_params

def number_of_states(rulestring):

    return int(rulestring[1:rulestring.index('r')])

def bitplanes(rulestring):

    nstates = number_of_states(rulestring)
    return (2 if (nstates == 3) else len(bin(nstates - 3)))

def create_rule(rulestring):

    nstates = number_of_states(rulestring)

    if (nstates < 3):
        raise ValueError("Number of states must be at least 3")

    logstring = rulestring[rulestring.index('r'):]
    gparams = get_ltl_params(logstring)

    with open('iterators_%s.h' % rulestring, 'w') as f:
        f.write('#pragma once\n')
        f.write('#include <stdint.h>\n')
        f.write('#include "../ltl.h"\n')
        f.write('namespace %s {\n\n' % rulestring.replace('-', '_'))

        f.write('    bool iterate_var_leaf(uint64_t *inleaves, uint64_t *hleaves, uint64_t *outleaf) {\n\n')
        f.write('        apg::ltl_kernel(inleaves, outleaf')
        for g in gparams:
            f.write(', %d' % g)
        f.write(');\n')
        f.write('        outleaf[0] &= (~hleaves[3]); outleaf[1] &= (~hleaves[6]); outleaf[2] &= (~hleaves[9]); outleaf[3] &= (~hleaves[12]);\n')
        f.write('        uint64_t outleaf2[4] = {inleaves[3] & (~outleaf[0]), inleaves[6] & (~outleaf[1]), inleaves[9] & (~outleaf[2]), inleaves[12] & (~outleaf[3])};\n')
        postprocess(f, nstates)
        f.write('        return false;\n')
        f.write('    }\n')
        f.write('}\n')
