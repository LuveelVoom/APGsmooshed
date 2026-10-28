# Determine linear automorphisms of a finite set of integer points

from itertools import combinations, permutations

def subdets(pts):
    '''
    Compute determinants of all (n choose d) size-d subsets of n
    points in Z^d.
    '''

    n = len(pts)
    d = len(pts[0])
    s = {tuple([]): 1}

    for e in range(1, d+1):
        for t in combinations(range(n), e):
            t = tuple(t)

            sgn = 1
            val = 0

            for i in range(e):
                subt = t[:i] + t[i+1:]
                val += s[subt] * pts[t[i]][-e] * sgn
                sgn = 0 - sgn

            s[t] = val

    return s

def adjugate_row(pts, j):

    n = len(pts)
    s = subdets([p[:j] + p[j+1:] for p in pts])
    t = tuple(range(n))
    for i in range(n):
        subt = t[:i] + t[i+1:]
        sgn = 1 - (((i + j) % 2) * 2)
        yield s[subt] * sgn

def adjugate(mat):
    '''
    The inverse of a unimodular matrix
    '''
    return [list(adjugate_row(mat, j)) for j in range(len(mat))]

def matperms(pts):

    n = len(pts)
    d = len(pts[0])

    s = subdets(pts)
    adj = None
    adjt = None
    basis = None
    ptsset = None
    origdet = None
    minofdet = None

    for t in combinations(range(n), d):
        t = tuple(t)
        absdet = abs(s[t])
        if absdet == 0:
            continue

        if minofdet is None:
            minofdet = absdet
        else:
            minofdet = min(absdet, minofdet)

    for t in combinations(range(n), d):
        t = tuple(t)

        det = s[t]
        if abs(det) == minofdet:
            coords = [pts[i] for i in t]
            if adj is None:
                origdet = det
                basis = coords
                adj = adjugate(basis)
                adjt = [[a[i] for a in adj] for i in range(d)]
                # Transform by adjugate matrix:
                pts = [[sum([x*y for (x, y) in zip(a, m)]) for a in adjt] for m in pts]
                ptsset = {tuple([x*det for x in p]): i for (i, p) in enumerate(pts)}

            for t in permutations(t):
                t = tuple(t)
                coordst = [[pts[i][j] for i in t] for j in range(d)]

                perm = []
                for p in pts:
                    q = tuple([sum([x*y for (x, y) in zip(a, p)]) for a in coordst])
                    if q in ptsset:
                        perm.append(ptsset[q])
                    else:
                        break

                if len(perm) == len(pts):
                    yield perm
