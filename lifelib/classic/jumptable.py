
print('typedef void (*jumpptr)(T*);')

for i in range(64):

    print('static void updateBoundary%d(T* sqt) {' % i)

    if (i & 6 == 6):
        print('    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);')
    if (i & 6 == 2):
        print('    sqt->copyBoundary1(sqt->neighbours[1]);')
    if (i & 6 == 4):
        print('    sqt->copyBoundary2(sqt->neighbours[2]);')

    if (i & 1):
        print('    sqt->copyBoundary0(sqt->neighbours[0]);')
    if (i & 8):
        print('    sqt->copyBoundary3(sqt->neighbours[3]);')

    if (i & 48 == 48):
        print('    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);')
    if (i & 48 == 16):
        print('    sqt->copyBoundary4(sqt->neighbours[4]);')
    if (i & 48 == 32):
        print('    sqt->copyBoundary5(sqt->neighbours[5]);')

    if (i == 0):
        print('    (void) sqt;')
    print('}')

print('jumpptr jumptable[64] = {%s};' % (',\n' + (' '*32)).join([('updateBoundary%d') % i for i in range(64)]))
