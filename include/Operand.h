/*
 * File:   Operand.h
 *
 * This is the header file for a class used to represent
 * and instruction operand. It is used for the ability to
 * overload operators.
 *
 */

#ifndef OPERAND_H
#define OPERAND_H

#include "Instruction.h"
#include "Kernel.h"

using namespace std;

enum
{
    TYPE_REGISTER,
    TYPE_LOCAL_STORE,
    TYPE_INDEX_REG,
    TYPE_SHIFT_REG,
    TYPE_LS_DESCRIPTOR
};

class Kernel;

class Operand
{
    public:
        /*
        * Constructor for creating a new Operand
        *
        * @param type the type of this operand (reg or local store)
        * @param index the index of the register or local store array that is
        *   represented by this object
        * @param localStoreIndexImmediate Only applies for operands of type
        *   TYPE_LOCAL_STORE and it specifies
        *   if the index is the actual index in the localStore (true) or if
        *   it is the register storing the index (false)
        * @param kernel the kernel for which this operand is used
        * @throws string if the index is out of bounds
        */
        Operand(int type, unsigned short index, bool localStoreIndexImmediate, Kernel *kernel);

        /*
         * Constructor for creating a new Operand with default localStoreIndexImmediate == false
         *
         * @param type the type of this operand (reg or local store)
         * @param index the index of the register or local store array that is
         *   represented by this object
         * @param kernel the kernel for which this operand is used
         * @throws string if the index is out of bounds
         */
        Operand(int type, unsigned short index, Kernel *kernel);

        /***********************************************************
         * Start of overloaded operators
         ***********************************************************/

        /* Addition */
	Instruction operator+(Operand op);//ok
        Instruction operator+(unsigned short value);//ok
        void operator+=(Operand op);//ok
	void operator+=(unsigned short value);//ok
 
        /* Subtraction */
        Instruction operator-(Operand op);//ok
        Instruction operator-(unsigned short value);//ok
        void operator-=(Operand op);//ok
	void operator-=(unsigned short value);//ok

        /* Multiplication */
        Instruction operator*(Operand op);//ok
        Instruction operator*(unsigned short value);//ok

        /* Assignment */
        void operator=(Operand op);//ok
        void operator=(unsigned short value);//ok
        void operator=(Instruction insn);

        /* Logical */
        Instruction operator~();//ok

        Instruction operator|(Operand op);//ok
        Instruction operator|(unsigned short value);//ok
        void operator|=(Operand op);//ok
	void operator|=(unsigned short value);//ok

        Instruction operator&(Operand op);//ok
        Instruction operator&(unsigned short value);//ok
        void operator&=(Operand op);//ok
	void operator&=(unsigned short value);//ok

        Instruction operator==(Operand op);//ok
        Instruction operator==(unsigned short value);//ok

        Instruction operator<(Operand op);//ok
        Instruction operator<(unsigned short value);//ok

        Instruction operator^(Operand op);//ok
        Instruction operator^(unsigned short value);//ok
        void operator^=(Operand op);//ok
	void operator^=(unsigned short value);//ok

        Operand operator[](Operand op);
        Operand operator[](unsigned short value);

        Instruction operator<<(Operand op);//ok
        Instruction operator<<(unsigned short value);//ok

        Instruction operator>>(Operand op);//ok
        Instruction operator>>(unsigned short value);//ok

        static Instruction addc(Operand op1, Operand op2);
        static Instruction addc(Operand op1, unsigned short value);

        static Instruction subc(Operand op1, Operand op2);
        static Instruction subc(Operand op1, unsigned short value);

        static Instruction shra(Operand op1, Operand op2);
        static Instruction shra(Operand op1, unsigned short value);

        static Instruction multhi();
        static Instruction multlo();

        static void cellshl(Operand op1, Operand op2);
	static void cellshl(Operand op1, unsigned short value);
        static void cellshr(Operand op1, Operand op2);
	static void cellshr(Operand op1, unsigned short valu);

        static Instruction ult(Operand op1, Operand op2);//ok
        static Instruction ult(Operand op1, unsigned short value);//ok

        static void red(Operand op);
    private:

        /*
         * The type of this operand (register, local store or special type)
         * See enum above.
         */
        int type;

        /*
         * The index of this operand in the register file or local store
         */
        unsigned short index;

        /*
         * The kernel for which this operand is used
         */
        Kernel *kernel;

        /*
         * Only applies for operands of type TYPE_LOCAL_STORE and it specifies
         * if the index is the actual index in the localStore (true) or if
         * it is the register storing the index (false)
         */
        bool localStoreIndexImmediate;
};

#endif // OPERAND_H
