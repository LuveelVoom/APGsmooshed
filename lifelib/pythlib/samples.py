import os
import re

from collections import defaultdict

try:
    from collections.abc import Sequence
except ImportError:
    from collections import Sequence

catagolue_url = "https://catagolue.hatsya.com"

def get_symmetries(rulestring):

    validsyms = ["1x256", "2x128", "4x64", "8x32", "C1"] # asymmetric soups
    for sz in ["32x32", "64x64", "128x128", "256x256", "512x512", "1k", "2k", "4k", "8k", "Glider8_4_5", "Glider6_5_6"]:
        validsyms += ["Mateon1_" + sz + "_Test"]

    if rulestring[-1].lower() == 'h':
        validsyms += ["C2_4", "C2_1", "C3_1", "C6", "D2_x", "D2_xo", "D4_x4", "D4_x1", "D6_1", "D6_1o", "D12"]
    else:
        validsyms += ["C2_4", "C2_2", "C2_1", "C4_4", "C4_1",
                        "D2_+2", "D2_+1", "D2_x", "D4_+4", "D4_+2", "D4_+1", "D4_x4", "D4_x1", "D8_4", "D8_1", "D2_+1_gO1s0", "D2_+1_gO1s1", "D2_+1_gO1s2"]

    if (re.match('^b0?1?2?3?4?5?6?7?8?s0?1?2?3?4?5?6?7?8?$', rulestring)):
        validsyms += ["G1", "G2_1", "G2_2", "G2_4", "H2_+1", "H2_+2", "H4_+1", "H4_+2", "H4_+4"] # GPU symmetries

    return validsyms

def validate_symmetry(rulestring, symmetry, throw_error=True):

    if (len(symmetry) > 50):
        if throw_error:
            raise ValueError("%s exceeds maximum of 50 characters" % symmetry)
        return False
    elif (len(symmetry) < 2):
        if throw_error:
            raise ValueError("%s is less than minimum of 2 characters" % symmetry)
        return False

    if not (re.compile("^[a-zA-Z0-9_+]+$").match(symmetry)):
        if throw_error:
            raise ValueError("%s contains an invalid character (not in [a-zA-Z0-9_+])" % symmetry)
        return False

    if "stdin" in symmetry:
        return True

    validsyms = get_symmetries(rulestring)

    redsym = symmetry

    if 'D8_1' in validsyms:
        while ((len(redsym) > 0) and (redsym[0] == 'i')):
            redsym = redsym[1:]

    if throw_error and (redsym not in validsyms):
        raise ValueError("%s is not one of the supported symmetries: %s" % (symmetry, validsyms))

    return (redsym in validsyms)

class SampleSoupList(Sequence):

    def __init__(self, lt, rule, symmetry, samples):

        self.lt = lt
        self.rule = rule
        self.symmetry = symmetry
        self.samples = samples

    def __getitem__(self, i):
        return self.lt.hashsoup(self.rule, self.symmetry, self.samples[i])

    def __len__(self):
        return len(self.samples)

    def __repr__(self):
        return 'SampleSoupList(%r, %r, %r, %r)' % (self.lt, self.rule, self.symmetry, self.samples)

def download_synthesis(apgcode, rule, domain=catagolue_url, tempfile='tempfile'):

    if ('_' in rule) and ('_' not in apgcode):
        rule, apgcode = apgcode, rule

    try:
        from urllib import urlretrieve
    except ImportError:
        from urllib.request import urlretrieve

    url = domain + '/textsamples/' + apgcode + '/' + rule + '/synthesis'
    urlretrieve(url, tempfile)

    with open(tempfile, 'r') as f:
        synthesis = f.read()

    if 'x' not in synthesis:
        return None
    else:
        return synthesis

def download_samples(lt, apgcode, rule, domain=catagolue_url, tempfile='tempfile'):

    if ('_' in rule) and ('_' not in apgcode):
        rule, apgcode = apgcode, rule

    try:
        from urllib import urlretrieve
    except ImportError:
        from urllib.request import urlretrieve

    url = domain + '/textsamples/' + apgcode + '/' + rule
    urlretrieve(url, tempfile)

    samples = defaultdict(list)

    with open(tempfile, 'r') as f:
        for l in f:
            if '/' not in l:
                continue
            symmetry, seed = tuple(l.strip().split('/')[:2])
            if (len(symmetry) > 50) or (len(symmetry) == 0):
                continue
            if (symmetry[0] == 'G'):
                symmetry = 'C' + symmetry[1:]
            elif (symmetry[0] == 'H'):
                symmetry = 'D' + symmetry[1:]
            samples[symmetry].append(seed)

    samples = {k : SampleSoupList(lt, rule, k, v) for (k, v) in samples.items() if validate_symmetry(rule, k, False)}
    return samples
