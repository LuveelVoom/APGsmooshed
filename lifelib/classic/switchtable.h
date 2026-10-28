switch (sqt->updateflags & 63) {
case 63:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 62:
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
    break;
case 61:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 60:
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary2(sqt->neighbours[2]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
    break;
case 59:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 58:
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary1(sqt->neighbours[1]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
    break;
case 57:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 56:
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
    break;
case 55:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 54:
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
    break;
case 53:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 52:
    sqt->copyBoundary2(sqt->neighbours[2]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
    break;
case 51:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 50:
    sqt->copyBoundary1(sqt->neighbours[1]);
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
    break;
case 49:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 48:
    sqt->copyBoundary45(sqt->neighbours[4], sqt->neighbours[5]);
    break;
case 47:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 46:
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    sqt->copyBoundary5(sqt->neighbours[5]);
    break;
case 45:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 44:
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary2(sqt->neighbours[2]);
    sqt->copyBoundary5(sqt->neighbours[5]);
    break;
case 43:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 42:
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary1(sqt->neighbours[1]);
    sqt->copyBoundary5(sqt->neighbours[5]);
    break;
case 41:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 40:
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary5(sqt->neighbours[5]);
    break;
case 39:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 38:
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    sqt->copyBoundary5(sqt->neighbours[5]);
    break;
case 37:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 36:
    sqt->copyBoundary2(sqt->neighbours[2]);
    sqt->copyBoundary5(sqt->neighbours[5]);
    break;
case 35:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 34:
    sqt->copyBoundary1(sqt->neighbours[1]);
    sqt->copyBoundary5(sqt->neighbours[5]);
    break;
case 33:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 32:
    sqt->copyBoundary5(sqt->neighbours[5]);
    break;
case 31:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 30:
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    sqt->copyBoundary4(sqt->neighbours[4]);
    break;
case 29:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 28:
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary2(sqt->neighbours[2]);
    sqt->copyBoundary4(sqt->neighbours[4]);
    break;
case 27:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 26:
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary1(sqt->neighbours[1]);
    sqt->copyBoundary4(sqt->neighbours[4]);
    break;
case 25:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 24:
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary4(sqt->neighbours[4]);
    break;
case 23:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 22:
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    sqt->copyBoundary4(sqt->neighbours[4]);
    break;
case 21:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 20:
    sqt->copyBoundary2(sqt->neighbours[2]);
    sqt->copyBoundary4(sqt->neighbours[4]);
    break;
case 19:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 18:
    sqt->copyBoundary1(sqt->neighbours[1]);
    sqt->copyBoundary4(sqt->neighbours[4]);
    break;
case 17:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 16:
    sqt->copyBoundary4(sqt->neighbours[4]);
    break;
case 15:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 14:
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    break;
case 13:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 12:
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary2(sqt->neighbours[2]);
    break;
case 11:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 10:
    sqt->copyBoundary3(sqt->neighbours[3]);
    sqt->copyBoundary1(sqt->neighbours[1]);
    break;
case 9:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 8:
    sqt->copyBoundary3(sqt->neighbours[3]);
    break;
case 7:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 6:
    sqt->copyBoundary12(sqt->neighbours[1], sqt->neighbours[2]);
    break;
case 5:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 4:
    sqt->copyBoundary2(sqt->neighbours[2]);
    break;
case 3:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 2:
    sqt->copyBoundary1(sqt->neighbours[1]);
    break;
case 1:
    sqt->copyBoundary0(sqt->neighbours[0]);
case 0:
    break;
}
