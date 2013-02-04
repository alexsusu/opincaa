/*
 * File:   cnxvector_registers.h
 *
 * Contains the declaration of the only allowed cnxvector objects.
 *
 */

#ifndef CNXVECTOR_REGISTERS_H
#define CNXVECTOR_REGISTERS_H

#include "cnxvector.h"

// define 32 objects of type cnxvector so that we can write operations like R1 = R2 + R3
extern cnxvector R[31];
extern cnxvector R0,   R1,   R2,   R3,   R4,   R5,   R6,   R7;
extern cnxvector R8,   R9,   R10,  R11,  R12,  R13,  R14,  R15;
extern cnxvector R16,  R17,  R18,  R19,  R20,  R21,  R22,  R23;
extern cnxvector R24,  R25,  R26,  R27,  R28,  R29,  R30,  R31;

extern cnxvector INDEX;
extern cnxvector SHIFT_REG;
extern cnxvector LS;
extern cnxvector MULT;

#endif // CNXVECTOR_REGISTERS_H
