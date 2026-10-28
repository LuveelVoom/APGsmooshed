from .lowlevel import *
from .pattern import Pattern
from .samples import download_samples as dsamples
from .samples import validate_symmetry
import os
import atexit

from ..genera import sanirule

from .identify import *

floats = {}


class Lifetree(object):

    def __init__(self, session, memory=1000, n_layers=None):

        if n_layers is None:
            n_layers = session.default_planes

        self.session = session
        lifelib = session.lifelib
        self.lifelib = lifelib
        self.ptr = lifelib('CreateLifetree', memory, n_layers)
        self.n_layers = n_layers

        session.lifetree_ptrs.add(self.ptr)

    def load_timeline(self, filename, n):

        import numpy as np
        frames = np.zeros((n,), dtype=np.uint64, order='C')
        frames = self.lifelib('LoadTimelineMC', self.ptr, filename, n, frames)

        patterns = []

        for f in frames:
            if f != 0:
                patterns.append(Pattern(self.session, (int(f),), self))

        return patterns

    def save_timeline(self, patterns, filename, header="", footer="", exponent=0, startgen=0):

        import numpy as np
        n = len(patterns) + 2
        frames = np.zeros((n,), dtype=np.uint64, order='C')
        for (i, p) in enumerate(patterns):
            frames[i] = p.ptr[0]
        frames[n-1] = startgen
        filename = os.path.abspath(filename)
        self.lifelib('SaveTimelineMC', self.ptr, filename, header, footer, frames, exponent)

    def unify(self, *patterns):

        output = self.pattern()

        for (i, pat) in enumerate(patterns):

            this_layer = self.pattern()
            this_layer += pat
            output += (this_layer << i)

        return output

    def deunify(self, pattern):

        n = self.n_layers
        return tuple([((pattern << (n - 1 - i)) >> (n - 1)) for i in range(n)])

    def loadu(self, filename):
        '''
        Load an uncompressed pattern file.
        '''

        filename = os.path.abspath(filename)
        metadata = identify_metadata(filename)
        timeline = False

        if 'frames' in metadata:
            try:
                n = int(metadata['frames'][0])
                timeline = True
            except:
                pass

        if timeline:
            # timeline file
            return self.load_timeline(filename, n)
        else:
            # pattern file
            pptr = self.lifelib('CreatePatternFromFile', self.ptr, filename)
            pat = Pattern(self.session, pptr, self)
            pat.comments = metadata['comments']
            return pat

    def load(self, filename, compressed='deduce', tempfile='tempfile'):
        '''
        Load a (possibly compressed) pattern file.
        '''

        filename = os.path.abspath(filename)

        if not os.path.isfile(filename):
            raise OSError("%s does not exist or is not a regular file" % filename)

        if compressed == 'deduce':
            compressed = identify_compression(filename)

        if compressed:

            from importlib import import_module
            decompressor = import_module(compressed)

            tempfile = os.path.abspath(tempfile)

            import shutil
            with decompressor.open(filename, 'rb') as f_in:
                with open(tempfile, 'wb') as f_out:
                    shutil.copyfileobj(f_in, f_out)
            rt = self.loadu(tempfile)
            try:
                os.remove(tempfile)
            except OSError:
                pass
        else:
            rt = self.loadu(filename)

        return rt

    def hashsoup(self, rule, symmetry, seed):

        validate_symmetry(rule, symmetry)

        pptr = self.lifelib('Hashsoup', self.ptr, rule, symmetry, seed)
        return Pattern(self.session, pptr, self)

    def download_samples(self, apgcode, rule, **kwargs):

        return dsamples(self, apgcode, rule, **kwargs)

    def pattern(self, rle="", rule=None, tempfile='tempfile', verify_apgcode=True):

        is_apgcode = False

        if ('=' in rle) or ('[' in rle):
            # Headerful RLE/MC:
            pptr = self.lifelib('CreatePatternFromFileContents', self.ptr, rle)
        else:
            # Headerless RLE:
            if rule is None:
                if len(self.session.rules) == 1:
                    rule = self.session.rules[0]
                else:
                    raise TypeError("For headerless RLE, rule must be specified unless session has a unique rule")

            rule = self.session.verify_rule(rule)
            pptr = self.lifelib('CreatePatternFromRLE', self.ptr, rle, rule)

            is_apgcode = ('_' in rle) and ('!' not in rle)

        if is_apgcode and (rle[0] != 'x'):
            raise ValueError("Only apgcodes beginning with 'x' contain sufficient information to reconstruct the pattern.")

        pat = Pattern(self.session, pptr, self)

        if is_apgcode and verify_apgcode and not rle.startswith('xg'):
            prefix = rle.split('_')[0]
            prefint = int(prefix[2:])
            period = 1 if (prefix[1] == 's') else prefint

            if (pat[period].centre() != pat.centre()) or (pat.period != period):
                raise ValueError("Pattern does not have the period implied by its apgcode.")

            if (prefix[1] == 's') and (pat.population != prefint):
                raise ValueError("Pattern does not have the population implied by its apgcode.")

            if (verify_apgcode == 'canonical') and (rle != pat.apgcode):
                raise ValueError("The canonical form of %s is %s" % (rle, pat.apgcode))

        return pat

    def __del__(self):

        if self.ptr in self.session.lifetree_ptrs:
            self.lifelib('DeleteLifetree', self.ptr)
            self.session.lifetree_ptrs.remove(self.ptr)

all_sessions = []

def cleanup_sessions():

    # Deallocate patterns
    for sess in all_sessions:
        for ptr in sess.pattern_ptrs:
            sess.lifelib('DeletePattern', ptr)
        sess.pattern_ptrs = set([])

    # Deallocate lifetrees
    for sess in all_sessions:
        for ptr in sess.lifetree_ptrs:
            sess.lifelib('DeleteLifetree', ptr)
        sess.lifetree_ptrs = set([])

    # Wait for child processes
    for sess in all_sessions:
        sess.lifelib.annihilate()

    # Remove references
    while all_sessions:
        all_sessions.pop()

atexit.register(cleanup_sessions)

class Session(object):

    def newfloat(self, name):

        if name not in floats:
            floats[name] = 0

        floats[name] += 1
        return ('%s%d' % (name, floats[name]))

    def __init__(self, soname, rules=['b3s23'], local_bash=None,
                 local_python='python', default_planes=4):

        self.default_planes = default_planes
        self.rules = list(rules)
        self.lifelib = WrappedLibrary(soname, local_bash=local_bash, local_python=local_python)
        self.lifetree_ptrs = set([])
        self.pattern_ptrs = set([])

        all_sessions.append(self)

    def lifetree(self, *args, **kwargs):

        return Lifetree(self, *args, **kwargs)

    def verify_rule(self, rule):

        rule = sanirule(rule)

        if (len(rule) > 7) and (rule[-6:] == 'istory'):
            newrule = rule[:-7]
        else:
            newrule = rule

        if newrule not in self.rules:
            raise ValueError("Rule %s is not in the configured rules %s for this session" % (rule, self.rules))

        return rule
