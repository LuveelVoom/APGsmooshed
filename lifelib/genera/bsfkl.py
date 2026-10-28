'''
Brian Prentice's BSFKL rulespace
'''

from .lifelike import create_rule as create_lifelike

family = 2
mantissa = {0, 1}
bitplanes = 2
number_of_states = 3


def create_rule(rulestring):

    parts = rulestring.replace('b', '').replace('s', ' ').replace('f', ' ').replace('k', ' ').replace('l', ' ').split(' ')

    bs_rule = 'b' + parts[0] + 's' + parts[1]
    f0_rule = 'b' + parts[2] + 's'
    k0_rule = 'b' + parts[3] + 's'
    l0_rule = 'b' + parts[4] + 's'

    create_lifelike(bs_rule)
    create_lifelike(f0_rule)
    create_lifelike(k0_rule)
    create_lifelike(l0_rule)

    with open('iterators_%s.h' % rulestring, 'w') as f:
        f.write('#pragma once\n')
        f.write('#include <stdint.h>\n')
        f.write('#include <cstdlib>\n')
        f.write('#include <iostream>\n')
        f.write('#include "../eors.h"\n')
        f.write('#include "../lifeperm.h"\n')

        for r in [bs_rule, f0_rule, k0_rule, l0_rule]:
            f.write('#include "iterators_%s.h"\n' % r)

        f.write('namespace %s {\n\n' % rulestring.replace('-', '_'))

        f.write('    bool iterate_var_leaf(uint64_t *inleaves, uint64_t *hleaves, uint64_t *outleaf) {\n\n')
        f.write('        %s::iterate_var_leaf(1, inleaves, outleaf);\n' % bs_rule)
        f.write('        outleaf[0] &= (~hleaves[3]); outleaf[1] &= (~hleaves[6]); outleaf[2] &= (~hleaves[9]); outleaf[3] &= (~hleaves[12]);\n')
        # At this point outleaf contains cells using B/S conditions.

        f.write('        outleaf[4] = inleaves[3] & (~outleaf[0]); outleaf[5] = inleaves[6]  & (~outleaf[1]);\n')
        f.write('        outleaf[6] = inleaves[9] & (~outleaf[2]); outleaf[7] = inleaves[12] & (~outleaf[3]);\n')
        # State-1 cells mature into state-2 cells.

        f.write('        uint64_t templeaf[4] = {0ull};\n')

        f.write('        %s::iterate_var_leaf(1, hleaves, templeaf);\n' % k0_rule)
        f.write('        templeaf[0] &= inleaves[3]; templeaf[1] &= inleaves[6]; templeaf[2] &= inleaves[9]; templeaf[3] &= inleaves[12];\n')
        f.write('        outleaf[0] &= (~templeaf[0]); outleaf[1] &= (~templeaf[1]); outleaf[2] &= (~templeaf[2]); outleaf[3] &= (~templeaf[3]);\n')
        f.write('        outleaf[4] &= (~templeaf[0]); outleaf[5] &= (~templeaf[1]); outleaf[6] &= (~templeaf[2]); outleaf[7] &= (~templeaf[3]);\n')
        # Effect of 'K' cells applied.

        f.write('        %s::iterate_var_leaf(1, hleaves, templeaf);\n' % f0_rule)
        f.write('        outleaf[0] &= (inleaves[3]  | templeaf[0]);\n')
        f.write('        outleaf[1] &= (inleaves[6]  | templeaf[1]);\n')
        f.write('        outleaf[2] &= (inleaves[9]  | templeaf[2]);\n')
        f.write('        outleaf[3] &= (inleaves[12] | templeaf[3]);\n')
        # We need 'F' for birth.

        f.write('        %s::iterate_var_leaf(1, inleaves, templeaf);\n' % l0_rule)
        f.write('        outleaf[4] |= (hleaves[3]  & (~templeaf[0]));\n')
        f.write('        outleaf[5] |= (hleaves[6]  & (~templeaf[1]));\n')
        f.write('        outleaf[6] |= (hleaves[9]  & (~templeaf[2]));\n')
        f.write('        outleaf[7] |= (hleaves[12] & (~templeaf[3]));\n')
        # 'L' causes state-2 cells to die; otherwise they remain state-2.

        f.write('        return false;\n')
        f.write('    }\n')
        f.write('}\n')
