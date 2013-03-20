/*
 * File:   opcodes.h
 *
 * Contains connex-opcodes and opcode-manipulation macros for instruction composition
 *
 *
 */

#ifndef OPCODES_H
#define OPCODES_H

    #define LEFT_POS        5
    #define LEFT_SIZE       5

    #define DEST_POS        0
    #define DEST_SIZE       5

    #define RIGHT_POS       10
    #define RIGHT_SIZE      5

    #define IMMEDIATE_VALUE_POS 10
    #define IMMEDIATE_VALUE_SIZE 16
    #define IMMEDIATE_VALUE_MASK ((1 << IMMEDIATE_VALUE_SIZE) -1)

    #define OPCODE_9BITS_POS    23
    #define OPCODE_9BITS_SIZE    9

    #define OPCODE_6BITS_POS    26
    #define OPCODE_6BITS_SIZE    6

    #define GET_LEFT(x) ((x >> LEFT_POS) & ((1 << LEFT_SIZE)-1))
    #define GET_RIGHT(x) ((x >> RIGHT_POS) & ((1 << RIGHT_SIZE)-1))
    #define GET_DEST(x) ((x >> DEST_POS) & ((1 << DEST_SIZE)-1))
    #define GET_IMM(x) ((x >> IMMEDIATE_VALUE_POS) & ((1 << IMMEDIATE_VALUE_SIZE)-1))
    #define GET_OPCODE_6BITS(x) ((x >> OPCODE_6BITS_POS) & ((1 << OPCODE_6BITS_SIZE)-1))
    #define GET_OPCODE_9BITS(x) ((x >> OPCODE_9BITS_POS) & ((1 << OPCODE_9BITS_SIZE)-1))

    #define LOOPS_VAL_MAX IMMEDIATE_VALUE_MASK
    #define DELTAJMP_VAL_MAX 1022

    /* 9-bit invalid opcodes (to cause error on verify)
        This is used to mark invalid parameres */
    // cannot use this: problem is in subscript operator: in case of write it does not return a value !
    //#define _INVALID_PARAM          0b111000000
    //#define _INVALID_DESTINATION    0b111000001
    //#define _INVALID_SOURCE         0b111000010

#ifndef _MSC_VER //MS C++ compiler

    /* 9-bit opcodes (instruction will NOT have immediate value) */
    #define _ADD     0b101000100
    #define _ADDC    0b101100100
    //#define _INC    0b101100100
    #define _SUB     0b101010100
    #define _CONDSUB 0b101110100

    #define _NOT    0b101001100
    #define _OR     0b101011100
    #define _AND    0b101101100
    #define _XOR    0b101111100
    #define _EQ      0b101001000
    #define _LT      0b101011000
    #define _ULT     0b101101000
    #define _SHL     0b101000000
    #define _SHR     0b101010000
    #define _SHRA    0b101100000
    #define _ISHL    0b101000001
    #define _ISHR    0b101010001
    #define _ISHRA   0b101100001

    #define _LDIX    0b100100000
    #define _LDSH    0b100110000
    #define _CELL_SHL 0b100010010
    #define _CELL_SHR 0b100010001

    #define _READ    0b100100100
    #define _WRITE   0b100010100

    #define _MULT    0b100001000
    #define _MULT_LO   0b100101000
    #define _MULT_HI   0b100111000

    #define _WHERE_CRY 0b100011100
    #define _WHERE_EQ  0b100011101
    #define _WHERE_LT  0b100011110
    #define _END_WHERE 0b100011111
    #define _REDUCE    0b100000000
    #define _NOP       0b000000000

    /* 6-bit opcodes (instruction will have immediate value) */
    #define _VLOAD     0b110101
    #define _IREAD     0b110100
    #define _IWRITE    0b110010

    #define _SETLC     0b010101
    #define _IJMPNZ    0b010011
#else
	#include "../../ms_visual_c/vc_opcodes.h"
#endif

#endif // OPCODES_H
