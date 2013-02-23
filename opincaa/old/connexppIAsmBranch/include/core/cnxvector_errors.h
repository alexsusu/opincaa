/*
 * File:   cnxvector_errors.h
 *
 *
 *
 */

#ifndef cnxvector_ERRORS_H
#define cnxvector_ERRORS_H

#include "cnxvector.h"

#define IMM_VAL_MAX 0xFFFF
#define IMM_VAL_SIGNED_MIN -32768
#define IMM_SHIFT_VAL_MAX 0x1F

#define ERR_LOOPS_TIMES_OUT_OF_RANGE "Too many jumps"
#define ERR_LOOP_LENGTH_OUT_OF_RANGE "Jump is too long"
#define ERR_SUBSCRIPT_OUT_OF_RANGE "Subscript operation failed: immediate value too large !"
#define ERR_SHIFT_OUT_OF_RANGE "Shift operation failed: too long !"
#define ERR_MULT_LO_HI_PARAM "Mult lo/hi operation failed: parameter not MULT !"
#define ERR_NOT_LOCALSTORE "Operation is not performed in local store !"
#define ERR_IMM_VALUE_OUT_OF_RANGE "Immediate value too large !"
#define ERR_TOO_MANY_BATCHES    "Too many batches. Please increase NUMBER_OF_BATCHES"
#define ERR_NOT_ENOUGH_MEMORY "Not enough memory to make the allocation"
#endif
