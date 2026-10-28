from .parsetable import GetNumberOfInputs
from .parsetree import normalise_neighbourhood


def MakeStaticTable(nodes):

    # Write rule tree in .data segment of the program:
    lines = ["const static uint32_t __ruletree[] __attribute__((aligned(64))) = {\n"]
    for (i, l) in enumerate(nodes):
        body = '    ' + (','.join([('%du' % x) for x in l]))
        body += ('};\n\n' if (i == len(nodes) - 1) else ',\n')
        lines.append(body)
    lines.append('\n')

    return lines


def NodesToCode(nhood, is16bit):

    numInputs = GetNumberOfInputs(nhood)
    numOutputs = len(nhood) - numInputs

    if (numOutputs != 1):
        raise RuntimeError("NodesToCode currently supports at most one output")

    nhood = normalise_neighbourhood(nhood)

    for (x, y) in nhood:
        if (x > 8) or (x < -8) or (y > 8) or (y < -8):
            raise RuntimeError("Neighbourhood must be a subset of [-8, 8] x [-8, 8]")

    # Byte offsets to beginnings of rows:
    in_offsets  = [(y+8)*32 + (x+8) for (x, y) in nhood[:numInputs]]
    out_offsets = [ y * 16  +   x   for (x, y) in nhood[numInputs:]]

    if is16bit:
        out_offsets += [t + 256 for t in out_offsets]

    # Hopefully simple enough to vectorise:
    lines = ['void iterate_var_row(uint8_t* __restrict__ inrow, uint8_t* __restrict__ outrow) {\n']
    lines.append('    uint32_t temprow[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};\n')
    for offset in in_offsets:
        if (is16bit):
            offset2 = ' + %d' % (offset + 1024)
            lines.append('    for (int i = 0; i < 16; i++) { temprow[i] += (((uint32_t) inrow[i%s]) << 8); }\n' % offset2)
        offset = '' if (offset == 0) else (' + %d' % offset)
        lines.append('    for (int i = 0; i < 16; i++) { temprow[i] += inrow[i%s]; }\n' % offset)
        lines.append('    for (int i = 0; i < 16; i++) { temprow[i] = __ruletree[temprow[i]]; }\n')
    for (i, offset) in enumerate(out_offsets):
        offset = '' if (offset == 0) else (' + %d' % offset)
        shift = '' if (i == 0) else (' >> %d' % (8*i))
        lines.append('    for (int i = 0; i < 16; i++) { outrow[i%s] = temprow[i]%s; }\n' % (offset, shift))
    lines.append('}\n')
    lines.append('\n')

    return lines
