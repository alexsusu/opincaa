#ifndef VECTOR_REGISTERS_H
#define VECTOR_REGISTERS_H

#include "..\include\vector.h"

// define 32 objects of type vector so that we can write operations like R1 = R2 + R3

vector R0(0,0),   R1(0,1),   R2(0,2),   R3(0,3),   R4(0,4),   R5(0,5),   R6(0,6),   R7(0,7);
vector R8(0,8),   R9(0,9),   R10(0,10), R11(0,11), R12(0,12), R13(0,13), R14(0,14), R15(0,15);
vector R16(0,16), R17(0,17), R18(0,18), R19(0,19), R20(0,20), R21(0,21), R22(0,22), R23(0,23);
vector R24(0,24), R25(0,25), R26(0,26), R27(0,27), R28(0,28), R29(0,29), R30(0,30), R31(0,31);

vector INDEX(INDEX_MARKER,INDEX_MARKER);
vector SHIFT_REG(SHIFTREG_MARKER,SHIFTREG_MARKER);

#endif // VECTOR_H
