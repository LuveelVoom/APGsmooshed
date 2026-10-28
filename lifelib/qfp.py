

class QufinceConfigurator(object):

    def pat2box(self, x):

        if isinstance(x, str):
            x = self.lt.pattern(x)

        x = x.centre().shift(32, 32) + self.rect
        return x


    def int2box(self, n):
        res = self.lt.pattern('', 'b3s23')
        l60 = self.lt.pattern('60o!', 'b3s23')
        l1 = self.lt.pattern('o!', 'b3s23')
        complete_rows = n // 60
        extra_cells = n % 60
        for i in range(complete_rows):
            res += l60(0, i)
        for i in range(extra_cells):
            res += l1(i, complete_rows)
        return (self.rect + res(2, 2))


    def lims(self, minx=None, maxx=None, miny=None, maxy=None):

        if minx is not None:
            self.minx = minx
        if maxx is not None:
            self.maxx = maxx
        if miny is not None:
            self.miny = miny
        if maxy is not None:
            self.maxy = maxy

        return self


    def rectangle(self):

        res = self.lt.pattern('')
        res[self.minx + 32 : self.maxx + 33, self.miny + 32 : self.maxy + 33] = 1
        return res + self.rect


    def addcat(self, c, l=None, dx=0, dy=0, phases=1):

        c = self.pat2box(c) - self.rect
        if l is None:
            l = self.rectangle()
        else:
            l = self.pat2box(l)

        for i in range(phases):
            self.cats += [c(dx, dy) + self.rect, l]
            c = c[1]

        return self


    def finalise_row(self, group, max_react=0):

        idx = {'A': 0, 'B': 1}[group.upper()]
        if max_react > 0:
            self.cats.append(self.int2box(max_react))
        self.groups[idx].append([self.pat2box('o!')] + self.cats)
        self.cats = []
        return self


    def __init__(self, lt,
                initial_pattern='',
                and_mask='',
                match_cells='',
                gens1=1600,
                gens2=30,
                min_react=8,
                border='',
                symmetries='',
                restore_group_A_reactants = False,
                restore_group_B_reactants = False,
                discard_period_4_patterns = False,
                discard_period_gens2_patterns = False):

        flags  = restore_group_A_reactants
        flags += restore_group_B_reactants * 2
        flags += discard_period_4_patterns * 4
        flags += discard_period_gens2_patterns * 8

        self.cats = []
        self.groups = [[], []]

        self.minx = 0
        self.maxx = 0
        self.miny = 0
        self.maxy = 0

        self.lt = lt
        self.rect = lt.pattern('', 'b3s23')
        self.rect[0:64, 0:64] = 1
        self.rect[1:63, 1:63] = 0

        self.header  = self.pat2box(initial_pattern)
        self.header += self.pat2box(and_mask)(64, 0)
        self.header += self.pat2box(match_cells)(128, 0)
        self.header += self.int2box(gens1)(192, 0)
        self.header += self.int2box(gens2)(256, 0)
        self.header += self.int2box(min_react)(320, 0)
        self.header += self.int2box(flags)(384, 0)
        self.header += self.pat2box(border)(448, 0)
        self.header += self.pat2box(symmetries)(512, 0)


    def to_pattern(self):

        res = self.lt.pattern('', 'b3s23')
        res += self.header

        y = 1
        for g in self.groups:
            y += 1
            for r in g:
                for (x, p) in enumerate(r):
                    res = res + p(64*x, 64*y)
                y += 1

        return res
