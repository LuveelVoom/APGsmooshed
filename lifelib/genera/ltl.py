family = 4
bitplanes = 1
mantissa = {0, 1}

def get_ltl_params(rulestring):

    gparams = rulestring[1:].replace('b', 't').replace('s', 't').split('t')
    gparams = tuple(map(int, gparams))
    if (gparams[1] > gparams[2]):
        raise ValueError("Minimum birth count cannot exceed maximum birth count")
    if (gparams[3] > gparams[4]):
        raise ValueError("Minimum survival count cannot exceed maximum survival count")
    if (gparams[2] >= ((2 * gparams[0] + 1) ** 2)):
        raise ValueError("Maximum birth count cannot exceed or equal neighbourhood area")
    if (gparams[4] > ((2 * gparams[0] + 1) ** 2)):
        raise ValueError("Maximum survival count cannot exceed neighbourhood area")

    return gparams

def create_rule(rulestring):

    gparams = get_ltl_params(rulestring)

    with open('iterators_%s.h' % rulestring, 'w') as f:
        f.write('#pragma once\n')
        f.write('#include <stdint.h>\n')
        f.write('#include "../ltl.h"\n')
        f.write('namespace %s {\n\n' % rulestring.replace('-', '_'))

        f.write('    bool iterate_var_leaf(uint64_t *inleaves, uint64_t *outleaf) {\n\n')
        f.write('        apg::ltl_kernel(inleaves, outleaf')
        for g in gparams:
            f.write(', %d' % g)
        f.write(');\n')
        f.write('        return false;\n')
        f.write('    }\n')
        f.write('}\n')
