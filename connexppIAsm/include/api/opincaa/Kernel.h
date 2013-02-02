/*
 * File:   Kernel.h
 *
 * This is the header file for a class containing a kernel 
 * (a vector of Instructions) for executing on the Connex Array
 * 
 */

#ifndef KERNEL_H
#define KERNEL_H

#include "Instruction.h"
#include "Operand.h"
#include "opcodes.h"
#include "ConnexMachine.h"
#include <vector>
#include <string>

using namespace std;

/*******************************************************
 * This defines OPINCAA macros registers and local store
 *******************************************************/
#define     R0          (Operand(TYPE_REGISTER, 0, __kernel))
#define     R1          (Operand(TYPE_REGISTER, 1, __kernel))
#define     R2          (Operand(TYPE_REGISTER, 2, __kernel))
#define     R3          (Operand(TYPE_REGISTER, 3, __kernel))
#define     R4          (Operand(TYPE_REGISTER, 4, __kernel))
#define     R5          (Operand(TYPE_REGISTER, 5, __kernel))
#define     R6          (Operand(TYPE_REGISTER, 6, __kernel))
#define     R7          (Operand(TYPE_REGISTER, 7, __kernel))
#define     R8          (Operand(TYPE_REGISTER, 8, __kernel))
#define     R9          (Operand(TYPE_REGISTER, 9, __kernel))
#define     R10         (Operand(TYPE_REGISTER, 10, __kernel))
#define     R11         (Operand(TYPE_REGISTER, 11, __kernel))
#define     R12         (Operand(TYPE_REGISTER, 12, __kernel))
#define     R13         (Operand(TYPE_REGISTER, 13, __kernel))
#define     R14         (Operand(TYPE_REGISTER, 14, __kernel))
#define     R15         (Operand(TYPE_REGISTER, 15, __kernel))

#define     INDEX       (Operand(TYPE_INDEX_REG, 0, __kernel))

#define     SHIFT_REG   (Operand(TYPE_SHIFT_REG, 0, __kernel))

#define     LS          (Operand(TYPE_LS_DESCRIPTOR, 0, __kernel))

/**************************************************
 * This defines Macros for OPINCAA code definition
 **************************************************/
#define BEGIN_KERNEL(kernelName)    {Kernel *__kernel = new Kernel(kernelName);

#define EXECUTE_IN_ALL(code)        __kernel->append(Instruction(_END_WHERE, 0, 0, 0));     \
                                    code
                                    
#define EXECUTE_WHERE_EQ(code)      __kernel->append(Instruction(_WHERE_EQ, 0, 0, 0));      \
                                    code                                                    \
                                    __kernel->append(Instruction(_END_WHERE, 0, 0, 0));

#define EXECUTE_WHERE_CRY(code)     __kernel->append(Instruction(_WHERE_CRY, 0, 0, 0));     \
                                    code                                                    \
                                    __kernel->append(Instruction(_END_WHERE, 0, 0, 0));
                                    
#define NOP                         __kernel->append(Instruction(_NOP, 0, 0, 0));

#define REDUCE(x)                   Operand::reduce(x);
#define ADDC(x,y)                   Operand::addc(x,y);
#define SUBC(x,y)                   Operand::subc(x,y);
#define ULT(x,y)                    Operand::ult(x,y);
#define SHRA(x,y)                   Operand::shra(x,y);
#define ISHRA(x,y)                  Operand::ishra(x,y);
#define CELL_SHL(x,y)               Operand::cellshl(x,y);
#define CELL_SHR(x,y)               Operand::cellshr(x,y);

#define LOW()                       Operand::multlo()
#define HIGH()                      Operand::multhi()

#define END_KERNEL(x)               ConnexMachine::addKernel(__kernel);}

class Kernel
{
    public:
        /*
        * Constructor for creating a new Kernel
        * 
        * @param name the name of the new kernel
        * 
        * @throws string if the name is invalid (NULL or empty)
        */ 
        Kernel(string name);
        
        /*
         * Destructor for the Kernel class
         * 
         * Disposes of the buffer and the instruction vector
         */ 
        ~Kernel();
        
        /*
         * Appends an existing instruction to the kernel
         * 
         * @param instruction the instruction to add
         */
        void append(Instruction instruction);
        
        /*
         * Writes the kernel to a memory location
         * 
         * @param buffer the memory location to write the kernel to
         */
        void writeTo(void * buffer);
        
        /*
         * Writes the kernel to a file descriptor
         * 
         * @param fileDescriptor the file descriptor to write the kernel to
         */
        void writeTo(int fileDescriptor);
        
        /*
         * Returns the number of instructions in this kernel
         * 
         * @return the number of instructions in this kernel
         */
        unsigned size();
        
        /*
         * Returns the name of this kernel
         * 
         * @return the name of this kernel
         */
        string getName();
        
    private:

        /*
         * The name of the kernel;
         */
        string name;
        
        /*
         * The vector containing the instructions
         */
        vector<unsigned> instructions;
};

#endif // BATCH_H
