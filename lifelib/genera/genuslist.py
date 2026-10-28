
# This file defines the genera accepted by lifelib. Earlier genera take
# precedence over later genera, so (for example) the rule b38s23 belongs
# to the genus 'lifelike' rather than 'isotropic'.

genus_list = []
genus_list.append({'name': 'b3s23life', 'regex': 'b3s23'})
genus_list.append({'name': 'lifelike', 'regex': 'b0?1?2?3?4?5?6?7?8?s0?1?2?3?4?5?6?7?8?'})
genus_list.append({'name': 'generations', 'regex': 'g[1-9][0-9]*b1?2?3?4?5?6?7?8?s0?1?2?3?4?5?6?7?8?'})
genus_list.append({'name': 'isotropic', 'regex': 'b[1-8ceaiknjqrytwz-]*s[0-8ceaiknjqrytwz-]*'})
genus_list.append({'name': 'isotropic-hex-ot', 'regex': 'b[1-6]*s[0-6]*h'})
genus_list.append({'name': 'isotropic-hex', 'regex': 'b[1-6omp-]*s[0-6omp-]*h'})
genus_list.append({'name': 'isotropic_other', 'regex': 'b[0-8ceaiknjqrytwz-]*s[0-8ceaiknjqrytwz-]*'})
genus_list.append({'name': 'isotropic_other-hex', 'regex': 'b[0-6omp-]*s[0-6omp-]*h'})
genus_list.append({'name': 'isotropic_other-fc', 'regex': 'b[0-8ceaiknjqrytwz-]*s[0-8ceaiknjqrytwz-]*fc'})
genus_list.append({'name': 'isotropic_other-fe', 'regex': 'b[0-8ceaiknjqrytwz-]*s[0-8ceaiknjqrytwz-]*fe'})
genus_list.append({'name': 'isotropic_other-r2vn', 'regex': 'b([0-9]+x-([0-8]([xaceijknqrtwyz][acdefgijklmnopqr])+)+|[0-8]([xaceijknqrtwyz][acdefgijklmnopqr])+|[0-9]+x)*s([0-9]+x-([0-8]([xaceijknqrtwyz][acdefgijklmnopqr])+)+|[0-8]([xaceijknqrtwyz][acdefgijklmnopqr])+|[0-9]+x)*v2'})
genus_list.append({'name': 'ltl', 'regex': 'r[234567]b[1-9][0-9]*t[1-9][0-9]*s[1-9][0-9]*t[1-9][0-9]*'})
genus_list.append({'name': 'gltl', 'regex': 'g[1-9][0-9]*r[234567]b[1-9][0-9]*t[1-9][0-9]*s[1-9][0-9]*t[1-9][0-9]*'})
genus_list.append({'name': 'isogeny', 'regex': 'g[1-9][0-9]*b[1-8ceaiknjqrytwz-]*s[0-8ceaiknjqrytwz-]*'})
genus_list.append({'name': 'isogeny-hex-ot', 'regex': 'g[1-9][0-9]*b[1-6]*s[0-6]*h'})
genus_list.append({'name': 'isogeny-hex', 'regex': 'g[1-9][0-9]*b[1-6omp-]*s[0-6omp-]*h'})
genus_list.append({'name': 'isogeny_other', 'regex': 'g[1-9][0-9]*b[0-8ceaiknjqrytwz-]*s[0-8ceaiknjqrytwz-]*'})
genus_list.append({'name': 'isogeny_other-hex', 'regex': 'g[1-9][0-9]*b[0-6omp-]*s[0-6omp-]*h'})
genus_list.append({'name': 'isogeny_other-fc', 'regex': 'g[1-9][0-9]*b[1-8ceaiknjqrytwz-]*s[0-8ceaiknjqrytwz-]*fc'})
genus_list.append({'name': 'isogeny_other-fe', 'regex': 'g[1-9][0-9]*b[1-8ceaiknjqrytwz-]*s[0-8ceaiknjqrytwz-]*fe'})
genus_list.append({'name': 'isogeny_other-r2vn', 'regex': 'g[1-9][0-9]*b([0-9]+x-([0-8]([xaceijknqrtwyz][acdefgijklmnopqr])+)+|[0-8]([xaceijknqrtwyz][acdefgijklmnopqr])+|[0-9]+x)*s([0-9]+x-([0-8]([xaceijknqrtwyz][acdefgijklmnopqr])+)+|[0-8]([xaceijknqrtwyz][acdefgijklmnopqr])+|[0-9]+x)*v2'})
genus_list.append({'name': 'bsfkl', 'regex': 'b1?2?3?4?5?6?7?8?s0?1?2?3?4?5?6?7?8?f0?1?2?3?4?5?6?7?8?k0?1?2?3?4?5?6?7?8?l0?1?2?3?4?5?6?7?8?'})
genus_list.append({'name': 'hrot', 'regex': 'r[2345]b[0-9a-f]*s[0-9a-f]*z?'})
genus_list.append({'name': 'ghrot', 'regex': 'g[1-9][0-9]*r[2345]b[0-9a-f]*s[0-9a-f]*z?'})
genus_list.append({'name': 'deficient', 'regex': 'b[1-8ceaiknjqrytwz-]*s[0-8ceaiknjqrytwz-]*dp?'})
genus_list.append({'name': 'genext', 'regex': 'b[1-8ceaiknjqrytwz-]*s[0-8ceaiknjqrytwz-]*d(0[-])?[1-9][0-9]*([-][1-9][0-9]*)*'})

# This one must always be last as a 'catch-all' for custom rules with
# no more than 256 states:

genus_list.append({'name': 'eightbit', 'regex': 'x[0-9a-z_-]*'})

# A single genus (such as 'isotropic') can be associated with multiple
# rule families (square isotropic and hexagonal isotropic). We use a
# hyphen to provide an additional suffix for the benefit of Catagolue.

for g in genus_list:
    g['fullname'] = g['name']
    g['name'] = g['fullname'].split('-')[0]
