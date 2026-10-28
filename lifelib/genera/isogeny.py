'''
Isotropic generations
'''

from .generations import family, mantissa, postprocess, number_of_states, bitplanes
from .isotropic import isotrope

def create_rule(rulestring):

    nstates = number_of_states(rulestring)

    if (nstates < 3):
        raise ValueError("Number of states must be at least 3")

    logstring = rulestring[rulestring.index('b'):]
    isotrope(logstring)

    with open('iterators_%s.h' % rulestring, 'w') as f:
        f.write('#pragma once\n')
        f.write('#include <stdint.h>\n')
        f.write('#include <cstdlib>\n')
        f.write('#include <iostream>\n')
        f.write('#include "../eors.h"\n')
        f.write('#include "../lifeperm.h"\n')
        f.write('namespace %s {\n\n' % rulestring.replace('-', '_'))

        # We reuse the 2-state isotropic kernel by #including it into an
        # inner namespace:
        f.write('    namespace twostate {\n\n')
        f.write('#include "ma_%s.h"\n\n' % logstring)
        f.write('#include "../isoluts.h"\n')
        f.write('    }\n\n')

        f.write('    bool iterate_var_leaf(uint64_t *inleaves, uint64_t *hleaves, uint64_t *outleaf) {\n\n')
        f.write('        twostate::iterate_var_leaf(inleaves, outleaf);\n')
        f.write('        outleaf[0] &= (~hleaves[3]); outleaf[1] &= (~hleaves[6]); outleaf[2] &= (~hleaves[9]); outleaf[3] &= (~hleaves[12]);\n')
        f.write('        uint64_t outleaf2[4] = {inleaves[3] & (~outleaf[0]), inleaves[6] & (~outleaf[1]), inleaves[9] & (~outleaf[2]), inleaves[12] & (~outleaf[3])};\n')
        postprocess(f, nstates)
        f.write('        return false;\n')
        f.write('    }\n')
        f.write('}\n')
