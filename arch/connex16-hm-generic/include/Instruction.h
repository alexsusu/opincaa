/*
 * File:   Instruction.h
 *
 * This is the header file for a class containing one Connex
 * Array instruction
 *
 */

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <string>

/*
 * Instuction format 1 (no immediate value):
 * --------------------------------------------------------------------
 * |   opcode     |  --------  |    right   |    left    |    dest    |
 * --------------------------------------------------------------------
 * |31 (9 bits) 23|22(8 bits)15|14(5 bits)10|9 (5 bits) 5|4 (5 bits) 0|
 * --------------------------------------------------------------------
 *
 * Instuction format 2 (immediate value):
 * --------------------------------------------------------------------
 * |   opcode   |     immediate value       |    left    |    dest    |
 * --------------------------------------------------------------------
 * |31(6 bits)26|25      (16 bits)        10|9 (5 bits) 5|4 (5 bits) 0|
 * --------------------------------------------------------------------
 *
 */

enum{
    INSTRUCTION_TYPE_UNKNOWN = 0,
    INSTRUCTION_TYPE_NO_VALUE,
    INSTRUCTION_TYPE_WITH_VALUE
};

/* The index of the bit in the instruction that specifies the format (1 or 2)*/
#define TYPE_BIT_INDEX 30

/* "Left" operand position and size */
#define LEFT_POS        5
#define LEFT_SIZE       5

/* "Dest" operand position and size */
#define DEST_POS        0
#define DEST_SIZE       5

/* "Right" operand position and size */
#define RIGHT_POS       10
#define RIGHT_SIZE      5

/* Immediate "Value" operand position and size */
#define IMMEDIATE_VALUE_POS 10
#define IMMEDIATE_VALUE_SIZE 16
#define IMMEDIATE_VALUE_MASK (((1 << IMMEDIATE_VALUE_SIZE) -1) << IMMEDIATE_VALUE_POS)

/* 9-bit "Opcode" position and size */
#define OPCODE_9BITS_POS    23
#define OPCODE_9BITS_SIZE    9

/* 6-bit "Opcode" position and size */
#define OPCODE_6BITS_POS    26
#define OPCODE_6BITS_SIZE    6

/* Macros for extracting instruction fields */
#define GET_LEFT(x) ((x >> LEFT_POS) & ((1 << LEFT_SIZE)-1))
#define GET_RIGHT(x) ((x >> RIGHT_POS) & ((1 << RIGHT_SIZE)-1))
#define GET_DEST(x) ((x >> DEST_POS) & ((1 << DEST_SIZE)-1))
#define GET_IMM(x) ((x >> IMMEDIATE_VALUE_POS) & ((1 << IMMEDIATE_VALUE_SIZE)-1))
#define GET_OPCODE_6BITS(x) ((x >> OPCODE_6BITS_POS) & ((1 << OPCODE_6BITS_SIZE)-1))
#define GET_OPCODE_9BITS(x) ((x >> OPCODE_9BITS_POS) & ((1 << OPCODE_9BITS_SIZE)-1))


/*******************************************************************
 *  Opcodes definition starting here
 *******************************************************************
 */

/* 9-bit opcodes (instruction will NOT have immediate value) */
#define _ADD        0x144       /* 0b101000100 */
#define _ADDC       0x164       /* 0b101100100 */
#define _SUB        0x154       /* 0b101010100 */
#define _SUBC       0x174       /* 0b101110100 */

#define _NOT        0x14C       /* 0b101001100 */
#define _OR         0x15C       /* 0b101011100 */
#define _AND        0x16C       /* 0b101101100 */
#define _XOR        0x17C       /* 0b101111100 */
#define _EQ         0x148       /* 0b101001000 */
#define _LT         0x158       /* 0b101011000 */
#define _ULT        0x168       /* 0b101101000 */
#define _SHL        0x140       /* 0b101000000 */
#define _SHR        0x150       /* 0b101010000 */
#define _SHRA       0x160       /* 0b101100000 */
#define _ISHL       0x141       /* 0b101000001 */
#define _ISHR       0x151       /* 0b101010001 */
#define _ISHRA      0x161       /* 0b101100001 */

#define _LDIX       0x120       /* 0b100100000 */
#define _LDSH       0x130       /* 0b100110000 */
#define _CELL_SHL   0x112       /* 0b100010010 */
#define _CELL_SHR   0x111       /* 0b100010001 */

#define _READ       0x124       /* 0b100100100 */
#define _WRITE      0x114       /* 0b100010100 */

#define _MULT       0x108       /* 0b100001000 */
#define _MULT_LO    0x128       /* 0b100101000 */
#define _MULT_HI    0x138       /* 0b100111000 */

#define _WHERE_CRY  0x11C       /* 0b100011100 */
#define _WHERE_EQ   0x11D       /* 0b100011101 */
#define _WHERE_LT   0x11E       /* 0b100011110 */
#define _END_WHERE  0x11F       /* 0b100011111 */
#define _REDUCE     0x100       /* 0b100000000 */
#define _NOP        0x00        /* 0b000000000 */

/* 6-bit opcodes (instruction will have immediate value) */
#define _VLOAD      0x35        /* 0b110101 */
#define _IREAD      0x34        /* 0b110100 */
#define _IWRITE     0x32        /* 0b110010 */
#define _SETLC		0x15		/* 0b010101 */
#define _IJMPNZ		0x13		/* 0b010011 */

/* INSTRUCTION_TYPE_UNKNOWN if opcode is not valid,
 * INSTRUCTION_TYPE_NO_VALUE if it's type 1,
 * INSTRUCTION_TYPE_WITH_VALUE if it's type 2
 */

static const int type_for_opcode[1 << OPCODE_9BITS_SIZE] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //   0 -  15
    0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //  16 -  31
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //  32 -  47
    0, 0, 2, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //  48 -  63
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //  64 -  79
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //  80 -  95
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //  96 - 111
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 112 - 127
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 128 - 143
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 144 - 159
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 160 - 175
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 176 - 191
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 192 - 207
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 208 - 223
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 224 - 239
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 240 - 255
    1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,  // 256 - 271
    0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,  // 272 - 287
    1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,  // 288 - 303
    1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,  // 304 - 319
    1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,  // 320 - 335
    1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,  // 336 - 351
    1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,  // 352 - 367
    0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,  // 368 - 383
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 384 - 399
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 400 - 415
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 416 - 431
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 432 - 447
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 448 - 463
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 464 - 479
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 480 - 495
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0   // 496 - 511
};

using namespace std;

class Instruction
{
    public:
        /*
         * Returns a string representation of specified opcode
         */
        static string mnemonic(int opcode);

        /*
         * Returns a string representation of specified register
         */
        static string registerName(int register_index);

        /*
        * Constructor for creating a new Instruction
        *
        * @param instruction the 32 bits used to create the instruction
        *
        * @throws string if the instruction is invalid
        */
        Instruction(unsigned int instruction);

        /*
         * Constructor for creating a new Instruction
         *
         * @param opcode the 9 or 6 bits opcode
         * @param rightOrValue the 5 or 16 bits corresponding to right or value operand (only the least significat bits are used)
         * @param left the 5 bits corresponding to left operand (only the least significat bits are used)
         * @param dest the 5 bits corresponding to dest operand (only the least significat bits are used)
         *
         * @throws string if the opcode is not valid
         */
        Instruction(int opcode, int rightOrValue, int left, int dest);


        /*
         * Returns the 32bit word representing the assembled instruction
         *
         * @return the 32bit word representing the assembled instruction
         */
        unsigned assemble();

        /*
         * Returns the string representing the dump instruction in
         * OPINCAA format
         *
         * @return string representing the dumped instruction
         */
        string disassemble();

	/*
	 * Returns the string representing the disassemble instruction
	 * in the following format:
	 * 	MNEMONIC	DESTINATION	LEFT	 RIGHT
	 */
	string dump();

        /*
         * Returns a string representation of this instruction
         *
         * @return the string representing this instruction
         */
        string toString();

        /*
         * Getter for type
         */
        int getType();

        /*
         * Getter for opcode
         */
        int getOpcode();

        /*
         * Getter for left
         */
        int getLeft();

        /*
         * Getter for right
         */
        int getRight();

        /*
         * Getter for dest
         */
        int getDest();

        /*
         * Getter for value
         */
        int getValue();

        /*
         * Setter for left
         */
        void setLeft(int left);

        /*
         * Setter for opcode
         */
        void setRight(int right);

        /*
         * Setter for opcode
         */
        void setDest(int dest);

        /*
         * Setter for opcode
         */
        void setValue(int value);
    private:
        /*
         * The type of this instruction (INSTRUCTION_TYPE_NO_VALUE or
         * INSTRUCTION_TYPE_WITH_VALUE)
         */
        int type;

        /*
         * The opcode
         */
        unsigned opcode;

        /*
         * The left operand
         */
        unsigned left;

        /*
         * The right operand
         */
        unsigned right;

        /*
         * The dest operand
         */
        unsigned dest;

        /*
         * The value operand
         */
        int value;

};

#endif // INSTRUCTION_H
