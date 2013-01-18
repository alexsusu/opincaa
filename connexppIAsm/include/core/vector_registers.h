/*
 * File:   vector_registers.h
 *
 * Contains the declaration of the only allowed vector objects.
 *
 */

#ifndef VECTOR_REGISTERS_H
#define VECTOR_REGISTERS_H

#include "vector.h"

// define 32 objects of type vector so that we can write operations like R1 = R2 + R3
extern vector R[31];
extern vector R0,   R1,   R2,   R3,   R4,   R5,   R6,   R7;
extern vector R8,   R9,   R10,  R11,  R12,  R13,  R14,  R15;
extern vector R16,  R17,  R18,  R19,  R20,  R21,  R22,  R23;
extern vector R24,  R25,  R26,  R27,  R28,  R29,  R30,  R31;

extern vector INDEX;
extern vector SHIFT_REG;
extern vector LS;
extern vector MULT;

#endif // VECTOR_H
