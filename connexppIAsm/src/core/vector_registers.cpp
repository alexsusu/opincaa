/*
 * File:   vector_registers.cpp
 *
 * Contains the only allowed vector objects.
 * INDEX is a special vector-register that is used for ldix instruction
 * SHIFT_REG is a special vector-register that is used for ldsh instruction
 * LS marks the local store
 * MULT marks the extended multiplicaion result. Use onlu _LO and _HI macros to read it !
 */

#include "../include/vector.h"

// define 32 objects of type vector so that we can write operations like R1 = R2 + R3

vector R0(0,0),   R1(1,0),   R2(2,0),   R3(3,0),   R4(4,0),   R5(5,0),   R6(6,0),   R7(7,0);
vector R8(8,0),   R9(9,0),   R10(10,0), R11(11,0), R12(12,0), R13(13,0), R14(14,0), R15(15,0);
vector R16(16,0), R17(17,0), R18(18,0), R19(19,0), R20(20,0), R21(21,0), R22(22,0), R23(23,0);
vector R24(24,0), R25(25,0), R26(26,0), R27(27,0), R28(28,0), R29(29,0), R30(30,0), R31(31,0);

vector INDEX(INDEX_MARKER,INDEX_MARKER);
vector SHIFT_REG(SHIFTREG_MARKER,SHIFTREG_MARKER);
vector LS(LOCALSTORE_MARKER,0);
vector MULT(MULTIPLICATION_MARKER, 0);

