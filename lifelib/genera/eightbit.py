
import os
from .rulefiles import rules_dir

def syms(rulestring):

    filename = rulestring if (rulestring[0] == 'x') else ('x' + rulestring)
    with open(os.path.join(rules_dir, filename + '.h')) as f:
        for line in f:
            if 'SYMS="' in line:
                return line.split('"')[1]

    return "NONE"

def zoi(rulestring):

    filename = rulestring if (rulestring[0] == 'x') else ('x' + rulestring)
    with open(os.path.join(rules_dir, filename + '.h')) as f:
        for line in f:
            if 'ZOI="' in line:
                return line.split('"')[1]

    return "99"

def bitplanes(rulestring):

    filename = rulestring if (rulestring[0] == 'x') else ('x' + rulestring)
    with open(os.path.join(rules_dir, filename + '.h')) as f:
        for line in f:
            break

    if '16' in line:
        return 16
    else:
        return 8

def family(rulestring):

    bp = bitplanes(rulestring)
    log2bp = len(bin(bp)) - 3
    return 2 * log2bp

mantissa = {0, 1}

def create_rule(rulestring):

    with open('iterators_%s.h' % rulestring, 'w') as f:
        f.write('#pragma once\n')
        filename = rulestring if (rulestring[0] == 'x') else ('x' + rulestring)
        f.write('#include "../../rules/%s.h"\n' % filename)

