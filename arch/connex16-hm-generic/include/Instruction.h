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

#include "Architecture.h"

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
    INSTRUCTION_TYPE_NO_IMMEDIATE,
    INSTRUCTION_TYPE_WITH_IMMEDIATE
};



/*
 Used by getDest(), getLeft(), getRight(), since -1 is not a valid value
   for register indices (which start from 0 - note that left, right, dest
   are unsigned).
*/
#define NO_REG_INDEX              100000



/* This macro enables having Register Files of more than 32 registers.
  Basically, it makes an instruction have 64 bits, since a 32-bit
   instruction allows at most 32 registers.
*/
// #define CONNEX_REG_COUNT_512



#ifdef CONNEX_REG_COUNT_512
  typedef uint64_t InstructionType;
  typedef int64_t ValueType;
#else
  typedef uint32_t InstructionType;
  typedef int32_t ValueType;
#endif

/* The index of the bit in the instruction that specifies the format (1 or 2) */
#ifdef CONNEX_REG_COUNT_512
  #define TYPE_BIT_INDEX 42
#else
  #define TYPE_BIT_INDEX 30
#endif

/* "Left" operand position and size */
#ifdef CONNEX_REG_COUNT_512
  #define LEFT_POS        9
  #define LEFT_SIZE       9
#else
  #define LEFT_POS        5
  #define LEFT_SIZE       5
#endif

/* "Dest" operand position and size */
#ifdef CONNEX_REG_COUNT_512
  #define DEST_POS        0
  #define DEST_SIZE       9
#else
  #define DEST_POS        0
  #define DEST_SIZE       5
#endif

/* "Right" operand position and size */
#ifdef CONNEX_REG_COUNT_512
  #define RIGHT_POS       18
  #define RIGHT_SIZE      9
#else
  #define RIGHT_POS       10
  #define RIGHT_SIZE      5
#endif

/* Immediate "Value" operand position and size */
#ifdef CONNEX_REG_COUNT_512
  #define IMMEDIATE_VALUE_POS 18
#else
  #define IMMEDIATE_VALUE_POS 10
#endif

#define IMMEDIATE_VALUE_SIZE 16
#define IMMEDIATE_VALUE_MASK (((1 << IMMEDIATE_VALUE_SIZE) -1) << IMMEDIATE_VALUE_POS)

/* 9-bit "Opcode" position and size */
#ifdef CONNEX_REG_COUNT_512
  #define OPCODE_9BITS_POS    35
  #define OPCODE_9BITS_SIZE    9
#else
  #define OPCODE_9BITS_POS    23
  #define OPCODE_9BITS_SIZE    9
#endif

/* 6-bit "Opcode" position and size */
#ifdef CONNEX_REG_COUNT_512
  #define OPCODE_6BITS_POS    38
  #define OPCODE_6BITS_SIZE    6
#else
  #define OPCODE_6BITS_POS    26
  #define OPCODE_6BITS_SIZE    6
#endif

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

//#define _DISABLE_CELL 0x13D     /* 0b100111101 */
// Using these values we avoid the WriteBack (WB) bit set
#define _DISABLE_CELL     0x30    /* 0b110000 */
#define _ENABLE_ALL_CELLS 0x31    /* 0b110001 */

/* 9-bit opcodes (instruction will NOT have immediate value) */
// For debug support - opcode of _PRINT_REG instruction
#define _PRINT_REG  0x103       /* 0b100000011 */ // TODO: NOT sure if it's the best encoding

#define _ADD        0x144       /* 0b101000100 */
#define _ADDC       0x164       /* 0b101100100 */
#define _SUB        0x154       /* 0b101010100 */
#define _SUBC       0x174       /* 0b101110100 */

#define _POPCNT     0x170       /* 8b101110000 */

#define _NOT        0x14C       /* 0b101001100 */
#define _BIT_REVERSE 0x14D      /* 0b101001101 - Note: currently not supported by hw */
#define _OR         0x15C       /* 0b101011100 */
#define _AND        0x16C       /* 0b101101100 */
#define _XOR        0x17C       /* 0b101111100 */
#define _EQ         0x148       /* 0b101001000 */
#define _LT         0x158       /* 0b101011000 */
#define _ULT        0x168       /* 0b101101000 */
#define _SHL        0x140       /* 0b101000000 */
#define _SHR        0x150       /* 0b101010000 */
#define _SHRA       0x160       /* 0b101100000 */
//
/* VERY IMPORTANT: even if the _ISHL/_ISHR(A) instructions are
   immediate, they have the 2nd opcode bit (also called the IMM bit)
   set to 0, as we can see in the Instruction format diagram above.
  So they are of type INSTRUCTION_TYPE_NO_IMMEDIATE
    - the immediate value is stored in the 5 bits of the
    right register, since it is enough for the delta operand
    of SHIFT operations (normally values 0-16 are enough for
    16-bit registers). */
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
#define _MULT_U     0x109       /* 0b100001001 */
#define _MULT_LO    0x128       /* 0b100101000 */
#define _MULT_HI    0x138       /* 0b100111000 */
//#define _MULT_HI_U  0x139       /* 0b100111001 */

#define _WHERE_CRY  0x11C       /* 0b100011100 */
#define _WHERE_EQ   0x11D       /* 0b100011101 */
#define _WHERE_LT   0x11E       /* 0b100011110 */
#define _END_WHERE  0x11F       /* 0b100011111 */

/* IMPORTANT NOTE: Looking at ConnexISA.pdf, we see the MODIFIER has 2 bits
    only.
   The _REDUCE opcode 0b100000000 has the following fields:
    - bit PIPE = 1
    - bit IMM = 0
    - bit ALU = 0
    - bit WB = 0
    - bit NON_ALU_SEL = 000 (register Index read)
    - bit MODIFIERS = 00
*/
#define _REDUCE     0x100       /* 0b100000000 */
#define _RED        _REDUCE
#define _REDUCE_U   0x101       /* 0b100000001 */
//#define _REDUCE_U   0x103       /* 0b100000001 */
#define _RED_U      _REDUCE_U
//#define _SCAN 0x102             /* 0b100000010 - NOTE: currently not supported by hw */
#define _SCAN 0x122             /* 0b100000010 - NOTE: currently not supported by hw */
#define _SETLC_REDUCE 0x121     /* 0b100000010 - NOTE: currently not supported by hw - IMPORTANT: The opcode is NOT really great, but we use it to avoid conflicting with e.g. the _SCAN defined above since we can define only 4 opcodes similar to _REDUCE. */
//#define _SETLC_REDUCE 0x121     /* 0b100000010 - NOTE: currently not supported by hw - IMPORTANT: The opcode is NOT really great, but we use it to avoid conflicting with e.g. the _SCAN defined above since we can define only 4 opcodes similar to _REDUCE. */
#define _SETLC_REDUCE_NOTNULL 0x102     /* 0b100000010 - NOTE: currently not supported by hw - IMPORTANT: The opcode is NOT really great, but we use it to avoid conflicting with e.g. the _SCAN defined above since we can define only 4 opcodes similar to _REDUCE. */

#define _NOP        0x00        /* 0b000000000 */

/* 6-bit opcodes (instruction will have immediate value) */
#define _VLOAD      0x35        /* 0b110101 */
#define _IREAD      0x34        /* 0b110100 */
#define _IWRITE     0x32        /* 0b110010 */
#define _SETLC      0x15        /* 0b010101 */
#define _IJMPNZ     0x13        /* 0b010011 */
#define _PRINT_CHARS  0x14      /* 0b010100 */ // TODO: NOT sure if it's the best encoding
#define _QUIT       0x16        /* 0b010110 */ // TODO: NOT sure if it's the best encoding
#define _IJMPNZ_RED 0x17  /* 0b010111 */ // TODO: NOT sure if it's the best encoding

/* INSTRUCTION_TYPE_UNKNOWN if opcode is not valid,
 * INSTRUCTION_TYPE_NO_IMMEDIATE if it's type 1,
 * INSTRUCTION_TYPE_WITH_IMMEDIATE if it's type 2
 */
static const int type_for_opcode[1 << OPCODE_9BITS_SIZE] = {
    // Added from 257 (0x101) - 259 (0x103) the following:
    //   _REDUCE_U, _SETLC_REDUCE, _SCAN, PRINT_REG with type_for_opcode 1
    //     (i.e., INSTRUCTION_TYPE_NO_IMMEDIATE)
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //   0 -  15
    0, 0, 0, 2, 2, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0,  //  16 -  31
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //  32 -  47
    2, 2, 2, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //  48 -  63 // Changed at index 48 and 49 with 2 from 0 for _DISABLE_CELL, _ENABLE_ALL_CELLS
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
    1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,  // 256 - 271 // Starts with: _REDUCE, _REDUCE_U, _SETLC_REDUCE, _SCAN, PRINT_REG
    0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,  // 272 - 287
    1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,  // 288 - 303
    1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,  // 304 - 319
    1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0,  // 320 - 335
    1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,  // 336 - 351
    1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,  // 352 - 367
    1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,  // 368 - 383
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

class Instruction {
    public:
        /*
         * Returns a string representation of specified opcode
         */
        /*static*/ string mnemonic(int opcode);

        /*
         * Returns a string representation of specified register
         */
        /*static*/ string registerName(int register_index);

        /*
        * Constructor for creating a new Instruction
        *
        * @param instruction the 32 bits used to create the instruction
        *
        * @throws string if the instruction is invalid
        */
        Instruction(InstructionType instruction);

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
         * We put the definition of assemble() in the header because of
         *       the inline qualifier.
         *  If we don't put this inlined method in the header but in the .cpp
         *    file we get:
         *  <<libopincaa.so: undefined reference to `Instruction::assemble()'
         *  collect2: error: ld returned 1 exit status>>
         *  The reason is that the inline qualifier makes the assemble() method
         *    not to be compiled as a standalone method in the resulting Opincaa
         *    library but inlined where required. However, if we define as
         *    inline the assemble() method in the header file
         *    it will always be present.
         *
         * Returns the 32bit word representing the assembled instruction
         *
         * @return the 32bit word representing the assembled instruction
         */
        inline InstructionType assemble() {
            InstructionType instruction;

            switch (type) {
                case INSTRUCTION_TYPE_NO_IMMEDIATE:
                    instruction = opcode << OPCODE_9BITS_POS;
                    instruction |= right << RIGHT_POS;
                    break;
                case INSTRUCTION_TYPE_WITH_IMMEDIATE:
                    instruction = opcode << OPCODE_6BITS_POS;

                    /* Taking into consideration the fact value is a signed int */
                    instruction |= ((value & 0xFFFF) << IMMEDIATE_VALUE_POS);

                    break;
                default:
                    throw string("Illegal instruction-type in Instruction::assemble");
            }

            instruction |= left << LEFT_POS;
            instruction |= dest << DEST_POS;

          #ifdef DEBUG_OPINCAA_INSTRUCTION_ASSEMBLE
            printf("Instruction::assemble(): instruction = 0x%X\n", instruction);
            //printf("  (mnemonic = %s)\n", (this->mnemonic(opcode)).c_str());
            printf("  which means: %s", (this->disassemble()).c_str());
            //printf("  which means: %s", (this->dump()).c_str());
            switch (type) {
                case INSTRUCTION_TYPE_NO_IMMEDIATE:
                    printf("  opcode = 0x%X\n", opcode);
                    printf("  right = %d\n", right);
                    break;
                case INSTRUCTION_TYPE_WITH_IMMEDIATE:
                    printf("  opcode = %d\n", opcode);
                    printf("  value = %d\n", value);
                    break;
            }
            printf("  left = %d\n", left);
            printf("  dest = %d\n", dest);
            printf("  type = %d\n", type);
            fflush(stdout);
          #endif

            return instruction;
        }


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
         *    MNEMONIC    DESTINATION    LEFT    RIGHT
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
        ValueType getValue();

        /*
         * Setter for opcode
         */
        void setOpcode(int opcode);

        /*
         * Setter for type
         */
        void setType(int type);

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
         * The type of this instruction (INSTRUCTION_TYPE_NO_IMMEDIATE or
         *   INSTRUCTION_TYPE_WITH_IMMEDIATE)
         */
        int type;

        /*
         * The opcode
         */
        InstructionType opcode;

        /*
         * The left operand
         */
        InstructionType left;

        /*
         * The right operand
         */
        InstructionType right;

        /*
         * The dest operand
         */
        InstructionType dest;

        /*
         * The value operand
         */
        ValueType value;
};

#endif // INSTRUCTION_H

