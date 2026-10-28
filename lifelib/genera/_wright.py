'''
Common code and imports for 'genext' and 'deficient' genera.
'''

from .eightbit import *
from .eightbit import create_rule as make_header
from .rulefiles import rule2files
from .isotropic import str2tab

'''
The following code was written by M. I. Wright (for Python 3) and
subsequently modified to be compatible with both versions of Python.
'''

import sys
from os import path

N_HOODS = 'cekainyqjrtwz'
NAPKINS = {
  k: dict(zip(N_HOODS, v)) for k, v in {
    '0': '',
    '1': ['00000001', '10000000'],
    '2': ['01000001', '10000010', '00100001', '10000001', '10001000', '00010001'],
    '3': ['01000101', '10100010', '00101001', '10000011', '11000001', '01100001', '01001001', '10010001', '10100001', '10001001'],
    '4': ['01010101', '10101010', '01001011', '11100001', '01100011', '11000101', '01100101', '10010011', '10101001', '10100011', '11001001', '10110001', '10011001'],
    '5': ['10111010', '01011101', '11010110', '01111100', '00111110', '10011110', '10110110', '01101110', '01011110', '01110110'],
    '6': ['10111110', '01111101', '11011110', '01111110', '01110111', '11101110'],
    '7': ['11111110', '01111111'],
    '8': ''
    }.items()
  }


def order_segment(sz, segment):
    if not segment or segment[0] == '-':
        return [t for t in N_HOODS[:sz] if t not in segment]
    return [t for t in N_HOODS if t in segment]


def combine_rstring(segment):
    cop, last = {}, 0
    for i, v in enumerate(segment, 1):
        if v.isdigit():
            after = next((idx for idx, j in enumerate(segment[i:], i) if j.isdigit()), len(segment))
            cop[v] = order_segment(len(NAPKINS[v]), segment[i:after])
    return cop

