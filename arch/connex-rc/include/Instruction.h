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
 * -------------------------------------------------------------------------
 * |   opcode     |  --------     |    right     |    left    |    dest    |
 * -------------------------------------------------------------------------
 * |31 (6 bits) 26|25 (11 bits) 15|14 (5 bits) 10|9 (5 bits) 5|4 (5 bits) 0|
 * -------------------------------------------------------------------------
 *
 * Instuction format 2 (immediate value):
 * -------------------------------------------------------------------------
 * |   opcode     |     immediate value          |    left    |    dest    |
 * -------------------------------------------------------------------------
 * |31 (6 bits) 26|25      (16 bits)           10|9 (5 bits) 5|4 (5 bits) 0|
 * -------------------------------------------------------------------------
 */

enum{
    INSTRUCTION_TYPE_UNKNOWN = 0,
    INSTRUCTION_TYPE_NO_VALUE,
    INSTRUCTION_TYPE_WITH_VALUE
};

/* The index of the bit in the instruction that specifies the format (1 or 2)*/
#define TYPE_BIT_INDEX 31

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

/* 6-bit opcodes */

#define _NOP          0x00       /* 0b000000 */
#define _SETLC        0x01	 /* 0b000001 */
#define _IJMPNZDEC    0x02	 /* 0b000010 */
#define _RED          0x03       /* 0b000011 */
#define _WHERE_LT     0x04       /* 0b000100 */
#define _WHERE_EQ     0x05       /* 0b000101 */
#define _WHERE_CRY    0x06       /* 0b000110 */
#define _END_WHERE    0x07       /* 0b000111 */
#define _LDSH         0x08       /* 0b001000 */
#define _LDIX         0x09       /* 0b001001 */
#define _MULT_LO      0x0A       /* 0b001010 */
#define _MULT_HI      0x0B       /* 0b001011 */

#define _CELL_SHR     0x0C       /* 0b001100 */
#define _CELL_SHR_i   0x2C       /* 0b101100 */

#define _CELL_SHL     0x0D       /* 0b001101 */	
#define _CELL_SHL_i   0x2D       /* 0b101101 */

#define _WRITE        0x0E       /* 0b001110 */
#define _WRITE_i      0x2E       /* 0b101110 */

#define _READ         0x0F       /* 0b001111 */
#define _READ_i       0x2F       /* 0b101111 */

#define _VLOAD        0x10       /* 0b010000 */
#define _VLOAD_i      0x30       /* 0b110000 */

#define _EQ           0x11       /* 0b010001 */
#define _EQ_i         0x31       /* 0b110001 */

#define _ULT          0x12       /* 0b010010 */
#define _ULT_i        0x32       /* 0b110010 */

#define _LT           0x13       /* 0b010011 */
#define _LT_i         0x33       /* 0b110011 */

#define _SHL          0x14       /* 0b010100 */
#define _SHL_i        0x34       /* 0b110100 */

#define _SHR          0x15       /* 0b010101 */
#define _SHR_i        0x35       /* 0b110101 */

#define _SHRA         0x16       /* 0b010110 */
#define _SHRA_i       0x36       /* 0b110110 */

#define _MULT         0x17       /* 0b010111 */
#define _MULT_i       0x37       /* 0b110111 */

#define _ADD          0x18       /* 0b011000 */
#define _ADD_i        0x38       /* 0b111000 */

#define _SUB          0x19       /* 0b011001 */
#define _SUB_i        0x39       /* 0b111001 */

#define _ADDC         0x1A       /* 0b011010 */
#define _ADDC_i       0x3A       /* 0b111010 */

#define _SUBC         0x1B       /* 0b011011 */
#define _SUBC_i       0x3B       /* 0b111011 */

#define _NOT          0x1C       /* 0b011100 */

#define _OR           0x1D       /* 0b011101 */
#define _OR_i         0x3D       /* 0b111101 */

#define _AND          0x1E       /* 0b011110 */	
#define _AND_i        0x3E       /* 0b111110 */	

#define _XOR          0x1F       /* 0b011111 */
#define _XOR_i        0x3F       /* 0b111111 */


/* INSTRUCTION_TYPE_UNKNOWN if opcode is not valid,
 * INSTRUCTION_TYPE_NO_VALUE if it's type 1,
 * INSTRUCTION_TYPE_WITH_VALUE if it's type 2
 */

static const int type_for_opcode[1 << OPCODE_6BITS_SIZE] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  //   0 -  15
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  //  16 -  31
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2,  //  32 -  47
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2,  //  48 -  63
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
