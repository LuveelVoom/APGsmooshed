typedef void (*jumpptr)(T*);
static void updateBoundary0(T* sqt) {
    (void) sqt;
}
static void updateBoundary1(T* sqt) {
    sqt->copyBoundary0(sqt->neighbours[0]);
}
static void updateBoundary2(T* sqt) {
    sqt->copyBoundary1(sqt->neighbours[1]);
}
static void updateBoundary3(T* sqt) {
    sqt->copyBoundary1(sqt->neighbours[1]);
    sqt->copyBoundary0(sqt->neighbours[0]);
}
static void updateBoundary4(T* sqt) {
    sqt->copyBoundary2(sqt->neighbours[2]);
}
static void updateBoundary5(T* sqt) {
    sqt->copyBoundary2(sqt->neighbours[2]);
    sqt->copyBoundary0(sqt->neighbours[0]);
}
static void updateBoundary6(T* sqt) {
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
}
static void updateBoundary7(T* sqt) {
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    sqt->copyBoundary0(sqt->neighbours[0]);
}
static void updateBoundary8(T* sqt) {
    sqt->copyBoundary3(sqt->neighbours[3]);
}
static void updateBoundary9(T* sqt) {
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary3(sqt->neighbours[3]);
}
static void updateBoundary10(T* sqt) {
    sqt->copyBoundary1(sqt->neighbours[1]);
    sqt->copyBoundary3(sqt->neighbours[3]);
}
static void updateBoundary11(T* sqt) {
    sqt->copyBoundary1(sqt->neighbours[1]);
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary3(sqt->neighbours[3]);
}
static void updateBoundary12(T* sqt) {
    sqt->copyBoundary2(sqt->neighbours[2]);
    sqt->copyBoundary3(sqt->neighbours[3]);
}
static void updateBoundary13(T* sqt) {
    sqt->copyBoundary2(sqt->neighbours[2]);
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary3(sqt->neighbours[3]);
}
static void updateBoundary14(T* sqt) {
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    sqt->copyBoundary3(sqt->neighbours[3]);
}
static void updateBoundary15(T* sqt) {
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary3(sqt->neighbours[3]);
}
static void updateBoundary16(T* sqt) {
    sqt->copyBoundary4(sqt->neighbours[4]);
}
static void updateBoundary17(T* sqt) {
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary4(sqt->neighbours[4]);
}
static void updateBoundary18(T* sqt) {
    sqt->copyBoundary1(sqt->neighbours[1]);
    sqt->copyBoundary4(sqt->neighbours[4]);
}
static void updateBoundary19(T* sqt) {
    sqt->copyBoundary1(sqt->neighbours[1]);
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary4(sqt->neighbours[4]);
}
static void updateBoundary20(T* sqt) {
    sqt->copyBoundary2(sqt->neighbours[2]);
    sqt->copyBoundary4(sqt->neighbours[4]);
}
static void updateBoundary21(T* sqt) {
    sqt->copyBoundary2(sqt->neighbours[2]);
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary4(sqt->neighbours[4]);
}
static void updateBoundary22(T* sqt) {
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    sqt->copyBoundary4(sqt->neighbours[4]);
}
static void updateBoundary23(T* sqt) {
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary4(sqt->neighbours[4]);
}
static void updateBoundary24(T* sqt) {
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary4(sqt->neighbours[4]);
}
static void updateBoundary25(T* sqt) {
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary4(sqt->neighbours[4]);
}
static void updateBoundary26(T* sqt) {
    sqt->copyBoundary1(sqt->neighbours[1]);
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary4(sqt->neighbours[4]);
}
static void updateBoundary27(T* sqt) {
    sqt->copyBoundary1(sqt->neighbours[1]);
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary4(sqt->neighbours[4]);
}
static void updateBoundary28(T* sqt) {
    sqt->copyBoundary2(sqt->neighbours[2]);
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary4(sqt->neighbours[4]);
}
static void updateBoundary29(T* sqt) {
    sqt->copyBoundary2(sqt->neighbours[2]);
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary4(sqt->neighbours[4]);
}
static void updateBoundary30(T* sqt) {
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary4(sqt->neighbours[4]);
}
static void updateBoundary31(T* sqt) {
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary4(sqt->neighbours[4]);
}
static void updateBoundary32(T* sqt) {
    sqt->copyBoundary5(sqt->neighbours[5]);
}
static void updateBoundary33(T* sqt) {
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary5(sqt->neighbours[5]);
}
static void updateBoundary34(T* sqt) {
    sqt->copyBoundary1(sqt->neighbours[1]);
    sqt->copyBoundary5(sqt->neighbours[5]);
}
static void updateBoundary35(T* sqt) {
    sqt->copyBoundary1(sqt->neighbours[1]);
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary5(sqt->neighbours[5]);
}
static void updateBoundary36(T* sqt) {
    sqt->copyBoundary2(sqt->neighbours[2]);
    sqt->copyBoundary5(sqt->neighbours[5]);
}
static void updateBoundary37(T* sqt) {
    sqt->copyBoundary2(sqt->neighbours[2]);
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary5(sqt->neighbours[5]);
}
static void updateBoundary38(T* sqt) {
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    sqt->copyBoundary5(sqt->neighbours[5]);
}
static void updateBoundary39(T* sqt) {
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary5(sqt->neighbours[5]);
}
static void updateBoundary40(T* sqt) {
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary5(sqt->neighbours[5]);
}
static void updateBoundary41(T* sqt) {
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary5(sqt->neighbours[5]);
}
static void updateBoundary42(T* sqt) {
    sqt->copyBoundary1(sqt->neighbours[1]);
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary5(sqt->neighbours[5]);
}
static void updateBoundary43(T* sqt) {
    sqt->copyBoundary1(sqt->neighbours[1]);
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary5(sqt->neighbours[5]);
}
static void updateBoundary44(T* sqt) {
    sqt->copyBoundary2(sqt->neighbours[2]);
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary5(sqt->neighbours[5]);
}
static void updateBoundary45(T* sqt) {
    sqt->copyBoundary2(sqt->neighbours[2]);
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary5(sqt->neighbours[5]);
}
static void updateBoundary46(T* sqt) {
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary5(sqt->neighbours[5]);
}
static void updateBoundary47(T* sqt) {
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary5(sqt->neighbours[5]);
}
static void updateBoundary48(T* sqt) {
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
}
static void updateBoundary49(T* sqt) {
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
}
static void updateBoundary50(T* sqt) {
    sqt->copyBoundary1(sqt->neighbours[1]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
}
static void updateBoundary51(T* sqt) {
    sqt->copyBoundary1(sqt->neighbours[1]);
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
}
static void updateBoundary52(T* sqt) {
    sqt->copyBoundary2(sqt->neighbours[2]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
}
static void updateBoundary53(T* sqt) {
    sqt->copyBoundary2(sqt->neighbours[2]);
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
}
static void updateBoundary54(T* sqt) {
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
}
static void updateBoundary55(T* sqt) {
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
}
static void updateBoundary56(T* sqt) {
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
}
static void updateBoundary57(T* sqt) {
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
}
static void updateBoundary58(T* sqt) {
    sqt->copyBoundary1(sqt->neighbours[1]);
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
}
static void updateBoundary59(T* sqt) {
    sqt->copyBoundary1(sqt->neighbours[1]);
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
}
static void updateBoundary60(T* sqt) {
    sqt->copyBoundary2(sqt->neighbours[2]);
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
}
static void updateBoundary61(T* sqt) {
    sqt->copyBoundary2(sqt->neighbours[2]);
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
}
static void updateBoundary62(T* sqt) {
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
}
static void updateBoundary63(T* sqt) {
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    sqt->copyBoundary0(sqt->neighbours[0]);
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
}
jumpptr jumptable[64] = {updateBoundary0,
                                updateBoundary1,
                                updateBoundary2,
                                updateBoundary3,
                                updateBoundary4,
                                updateBoundary5,
                                updateBoundary6,
                                updateBoundary7,
                                updateBoundary8,
                                updateBoundary9,
                                updateBoundary10,
                                updateBoundary11,
                                updateBoundary12,
                                updateBoundary13,
                                updateBoundary14,
                                updateBoundary15,
                                updateBoundary16,
                                updateBoundary17,
                                updateBoundary18,
                                updateBoundary19,
                                updateBoundary20,
                                updateBoundary21,
                                updateBoundary22,
                                updateBoundary23,
                                updateBoundary24,
                                updateBoundary25,
                                updateBoundary26,
                                updateBoundary27,
                                updateBoundary28,
                                updateBoundary29,
                                updateBoundary30,
                                updateBoundary31,
                                updateBoundary32,
                                updateBoundary33,
                                updateBoundary34,
                                updateBoundary35,
                                updateBoundary36,
                                updateBoundary37,
                                updateBoundary38,
                                updateBoundary39,
                                updateBoundary40,
                                updateBoundary41,
                                updateBoundary42,
                                updateBoundary43,
                                updateBoundary44,
                                updateBoundary45,
                                updateBoundary46,
                                updateBoundary47,
                                updateBoundary48,
                                updateBoundary49,
                                updateBoundary50,
                                updateBoundary51,
                                updateBoundary52,
                                updateBoundary53,
                                updateBoundary54,
                                updateBoundary55,
                                updateBoundary56,
                                updateBoundary57,
                                updateBoundary58,
                                updateBoundary59,
                                updateBoundary60,
                                updateBoundary61,
                                updateBoundary62,
                                updateBoundary63};
