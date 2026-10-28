
from .gltl import family, mantissa, postprocess, number_of_states, bitplanes
from .hrot import get_hrot_params, write_rule_data

def create_rule(rulestring):

    nstates = number_of_states(rulestring)

    if (nstates < 3):
        raise ValueError("Number of states must be at least 3")

    logstring = rulestring[rulestring.index('r'):]
    rulerange, b_int, s_int, s_zero = get_hrot_params(logstring)

    with open('iterators_%s.h' % rulestring, 'w') as f:
        f.write('#pragma once\n')
        f.write('#include <stdint.h>\n')
        f.write('#include "../ltl.h"\n')
        f.write('namespace %s {\n\n' % rulestring.replace('-', '_'))

        write_rule_data(f, b_int, s_int, s_zero)

        f.write('    bool iterate_var_leaf(uint64_t *inleaves, uint64_t *hleaves, uint64_t *outleaf) {\n\n')
        f.write('        apg::hrot_kernel(inleaves, outleaf, %d, ruledata);\n' % rulerange)
        f.write('        outleaf[0] &= (~hleaves[3]); outleaf[1] &= (~hleaves[6]); outleaf[2] &= (~hleaves[9]); outleaf[3] &= (~hleaves[12]);\n')
        f.write('        uint64_t outleaf2[4] = {inleaves[3] & (~outleaf[0]), inleaves[6] & (~outleaf[1]), inleaves[9] & (~outleaf[2]), inleaves[12] & (~outleaf[3])};\n')
        postprocess(f, nstates)
        f.write('        return false;\n')
        f.write('    }\n')
        f.write('}\n')
