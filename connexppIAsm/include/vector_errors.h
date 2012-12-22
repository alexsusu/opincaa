/*
 * File:   vector_errors.h
 *
 *
 *
 */

#ifndef VECTOR_ERRORS_H
#define VECTOR_ERRORS_H

#include "../include/vector.h"

#define IMM_VAL_MAX 0xFFFF
#define IMM_SHIFT_VAL_MAX 0x1F

#define ERR_SUBSCRIPT_OUT_OF_RANGE "Subscript operation failed: immediate value too large !"
#define ERR_SHIFT_OUT_OF_RANGE "Shift operation failed: too long !"
#define ERR_MULT_LO_HI_PARAM "Mult lo/hi operation failed: parameter not MULT !"
#define ERR_NOT_LOCALSTORE "Operation is not performed in local store !"
#define ERR_IMM_VALUE_OUT_OF_RANGE "Immediate value too large !"

#endif
