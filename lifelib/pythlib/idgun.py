def determine_if_gun(a, maxexp):

    lt4 = a.owner

    if lt4.n_layers < 4:
        lt4 = lt4.session.lifetree(n_layers=4)

    a = a[524288]
    x = lt4.pattern('', 'LifeHistory')
    x += a
    x = x[524288]
    oldpop = x.population
    x = x[524288]
    newpop = x.population
    p = newpop - oldpop

    if (p < 500000):
        return None

    if (p > 550000):
        return None

    envelope = x.layers()[1]
    livecells = x[8].layers()[0]
    y = livecells - envelope
    envelope = x[256].layers()[1]

    if (y.population != 5):
        return None

    z = lt4.pattern("", "b3s23")
    z += y

    osc = z.oscar(maxexp=maxexp, return_apgcode=True, eventual_oscillator=False, verbose=False, allow_guns=False)

    if osc.get('apgcode', '') != 'xq4_153':
        return None

    w = lt4.pattern("", "LifeHistory")
    w += z[-2097152]
    w = w[4194304]


    band = w.layers()[1]

    pbb = envelope - band
    r1 = z.getrect()
    r2 = z[1024].getrect()

    pbb += pbb(r1[0] - r2[0], r1[1] - r2[1])

    r = pbb.getrect()
    if (r is None) or (len(r) != 4):
        return None

    envelope = envelope[r[0] : r[0] + r[2], r[1] : r[1] + r[3]]
    r = envelope.getrect()

    geater = lt4.pattern("bo$2bo$3o4$5b2o$5bo$6b3o$8bo!", "b3s23")

    eater = z.replace(geater[:3, :3], geater[100], orientations='rotate4reflect', n_phases=2)

    if (eater.population != 7):
        return None

    x += eater

    x = x[128]

    osc = x.oscar(maxexp=maxexp, eventual_oscillator=False, allow_guns=False, verbose=False)

    if 'period' not in osc:
        return None

    multiple = osc['period']
    gun = lt4.pattern("", "b3s23")
    gun += livecells

    if ((gun - gun[multiple]).population > 0):
        return None

    lastshadow = None
    canonical_gun = None
    cw = None

    for i in range(multiple + 1):

        gun = gun[1]
        nextshadow = gun & envelope
        admissible = (i > 0) and (lastshadow[1] == nextshadow)
        lastshadow = nextshadow

        if admissible and ((nextshadow[1] - envelope).population > 0):
            w = nextshadow.phase_wechsler()
            if (cw is None) or (len(cw) > len(w)) or ((len(cw) == len(w)) and (cw > w)):
                cw = w
                canonical_gun = nextshadow

    if cw is None:
        return None

    return (canonical_gun, multiple, cw, z, envelope)

