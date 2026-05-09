/*
 * File:   Kernel.h
 *
 * This is the header file for a class containing a kernel
 * (a vector of Instructions) for executing on the Connex Array
 *
 */

#ifndef KERNEL_H
#define KERNEL_H

#include "Architecture.h"
#include "Instruction.h"
#include "ConnexMachine.h"
#include <vector>
#include <string>

using namespace std;


#define CEIL_INT_DIV(op1, op2) (((op1) / (op2)) + (((op1) % (op2)) > 0))

#ifdef DEBUG_OPINCAA
    #define dout std::cout
    // See http://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html
    #define dprintf(...) printf(__VA_ARGS__)
    #define dfflush fflush
#else
    #define dout 0 && std::cout
    #define dprintf(...) 0 && printf(__VA_ARGS__)
    #define dfflush 0 && fflush
#endif

// Adding these for debugging purposes in the Opincaa simulator
// We use macros because it is easier to lay this code inside the Opincaa program
#define PrintDebugMessage(aStr) \
    { int aStrLen = strlen(aStr); \
    int idxPrint; \
    for (idxPrint = 0; idxPrint < aStrLen; idxPrint += 2) { \
        PRINTCHARS((aStr[idxPrint] << 8) | (aStr[idxPrint + 1] == 0 ? '\n' : aStr[idxPrint + 1])); \
    } \
    if (idxPrint == aStrLen) \
        PRINTCHARS((' ' << 8) | ('\n')); }

// I use the stringizing operator - see info at https://msdn.microsoft.com/en-us/library/09dwwt6y.aspx
#define PrintRegDebug(regIdx) \
    PrintDebugMessage("Print reg " #regIdx ":"); \
    PRINTREG(regIdx);

#define PrintDebugReg PrintRegDebug



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
#define     R16         (Operand(TYPE_REGISTER, 16, __kernel))
#define     R17         (Operand(TYPE_REGISTER, 17, __kernel))
#define     R18         (Operand(TYPE_REGISTER, 18, __kernel))
#define     R19         (Operand(TYPE_REGISTER, 19, __kernel))
#define     R20         (Operand(TYPE_REGISTER, 20, __kernel))
#define     R21         (Operand(TYPE_REGISTER, 21, __kernel))
#define     R22         (Operand(TYPE_REGISTER, 22, __kernel))
#define     R23         (Operand(TYPE_REGISTER, 23, __kernel))
#define     R24         (Operand(TYPE_REGISTER, 24, __kernel))
#define     R25         (Operand(TYPE_REGISTER, 25, __kernel))
#define     R26         (Operand(TYPE_REGISTER, 26, __kernel))
#define     R27         (Operand(TYPE_REGISTER, 27, __kernel))
#define     R28         (Operand(TYPE_REGISTER, 28, __kernel))
#define     R29         (Operand(TYPE_REGISTER, 29, __kernel))
#define     R30         (Operand(TYPE_REGISTER, 30, __kernel))
#define     R31         (Operand(TYPE_REGISTER, 31, __kernel))

#define     R(x)        (Operand(TYPE_REGISTER, x, __kernel))

#define     INDEX       (Operand(TYPE_INDEX_REG, 0, __kernel))

#define     SHIFT_REG   (Operand(TYPE_SHIFT_REG, 0, __kernel))

#define     LS          (Operand(TYPE_LS_DESCRIPTOR, 0, __kernel))

/**************************************************
 * This defines Macros for OPINCAA code definition
 **************************************************/
#define BEGIN_KERNEL(kernelName)    {Kernel *__kernel = new Kernel(kernelName); if (useLaneGatingOnConnex) __kernel->append(Instruction(_ENABLE_ALL_CELLS, 0, 0, 0)); // 2018_10_26


// These 3 Connex instructions are implemented only in the Opincaa simulator
#define QUIT                        __kernel->append(Instruction(_QUIT, 0, 0, 0));
#define PRINTREG(regIndex)          __kernel->append(Instruction(_PRINT_REG, 0, regIndex, 0));
#define PRINTCHARS(value)           __kernel->append(Instruction(_PRINT_CHARS, value, 0, 0));

#define EXECUTE_IN_ALL(code)        __kernel->append(Instruction(_END_WHERE, 0, 0, 0));     \
                                    code

#define EXECUTE_WHERE_EQ(code)      __kernel->append(Instruction(_WHERE_EQ, 0, 0, 0));      \
                                    code

#define EXECUTE_WHERE_LT(code)      __kernel->append(Instruction(_WHERE_LT, 0, 0, 0));      \
                                    code

#define EXECUTE_WHERE_CRY(code)     __kernel->append(Instruction(_WHERE_CRY, 0, 0, 0));     \
                                    code

// The NOP instruction is without immediate operand. I think this is a good decision,
//   since the instruction pipeline has to be filled with instructions such as NOP,
//   executed in pipelined fashion in 1 cycle / stage.
#define NOP                         __kernel->append(Instruction(_NOP, 0, 0, 0));
// NOPMULTI(x) is syntactic sugar
#define NOPMULTI(x)                 for (int idxNopM = 0; idxNopM < x; idxNopM++) __kernel->append(Instruction(_NOP, 0, 0, 0));

#define REDUCE(x)                   Operand::reduce(x);
#define RED(x)                      Operand::reduce(x);
#define REDUCE_U(x)                 Operand::reduce_u(x);
#define RED_U(x)                    Operand::reduce_u(x);
// Experimental instruction added
#define SCAN(x)                     Operand::scan(x);
//
#define POPCNT(x)                   Operand::popcnt(x);
#define POPCOUNT(x)                 Operand::popcnt(x);
#define ADDC(x, y)                  Operand::addc(x, y);
#define SUBC(x, y)                  Operand::subc(x, y);
#define ULT(x, y)                   Operand::ult(x, y);
#define SHRA(x, y)                  Operand::shra(x, y);
#define ISHRA(x, y)                 Operand::ishra(x, y);
#define ISHL(x, imm)                x.operator<<(imm);
#define ISHR(x, imm)                x.operator>>(imm);
#define CELL_SHL(x, y)              Operand::cellshl(x, y);
#define CELLSHL(x, y)               Operand::cellshl(x, y);
#define CELL_SHR(x, y)              Operand::cellshr(x, y);
#define CELLSHR(x, y)               Operand::cellshr(x, y);

#define MULT_U(x, y)                x.Operand::mult_u(y)
#define MULT_LOW()                  Operand::multlo()
#define MULTLO()                    Operand::multlo()
#define MULT_HIGH()                 Operand::multhi()
#define MULTHI()                    Operand::multhi()
// #define MULT_HIGH_U()               Operand::multhi_u()

// Experimental - it is similar with the macrodef for POPCNT(x) - see above
#define BITREVERSE(x)               Operand::bitreverse(x);

#define DISABLE_CELL      __kernel->append(Instruction(_DISABLE_CELL, 0, 0, 0));
#define ENABLE_ALL_CELLS  __kernel->append(Instruction(_ENABLE_ALL_CELLS, 0, 0, 0));

// Experimental instruction added
#define REPEAT_REDUCE(regIndex)     __kernel->append(Instruction(_SETLC_REDUCE, 0, regIndex, 0)); \
                                    __kernel->resetLoopJumpTarget();

// Experimental instruction added
//#define REPEAT_NOTNULL(regIndex)    __kernel->append(Instruction(_SETLC_REDUCE_NOTNULL, 0, regIndex, 0)); \
//                                    __kernel->resetLoopJumpTarget();

#define REPEAT(x)                   __kernel->append(Instruction(_SETLC, x-1, 0, 0)); \
                                    /* Hw workaround */                             \
                                    __kernel->append(Instruction(_SETLC, x-1, 0, 0)); \
                                    __kernel->resetLoopJumpTarget();

#define REPEAT_X_TIMES(x)           REPEAT(x)
#define REPEAT_TIMES(x)             REPEAT(x)
#define END_REPEAT                  __kernel->appendLoopInstruction(); \
                                    __kernel->append(Instruction(_NOP, 0, 0, 0));

#define REPEAT_SIMPLE               __kernel->resetLoopJumpTarget();
#define END_REPEAT_RED_NOT_ZERO(RED_REG)     __kernel->appendLoopInstruction(_IJMPNZ_RED, RED_REG); \
                                    __kernel->append(Instruction(_NOP, 0, 0, 0));



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

        /* Copy from the myBinaryData array (e.g. from a preassembled kernel)
         *   to the instructions member of the Kernel class. */
        void copyBinaryKernel(InstructionType *myBinaryData, int numInstructions);

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

        /*
         * Returns a string representing the dumped kernel, one
         * instruction per line.
         *
         * @return the dumped kernel
         */
        string dump();

        /*
         * Return a string representing the disassembled kernel.
         * One instruction per line.
         */
        string disassemble();

        void DetermineDataToSaveForFission(int idxInstrBegin, int idxInstrEnd);
        void makeRepeatLoopsValidByUsingFissionAndTiling(); //makeLoopsValid();

        #define NEW2018_08_10
        //#define CONVERT_PARTLY_SSA_FORM
        /*
         * We generate a string with C++ code for manual LLVM Instruction
         *    Selection.
         */
        void ConvertInPartlySSAForm(vector<Instruction> &partlySSAInstrs,
                                    bool *isDefined, bool *isUsedOverall);
        string genLLVMISelManualCode();
        string GenerateIfRequiredCopyToReg(int &iInstr,
                                      Instruction &instrCrt,
                            #ifndef CONVERT_PARTLY_SSA_FORM
                                      bool isInstrInsideWhereBlock,
                            #endif
                                      int &numInstructionsToCodegen,
                                      int &offsetKernelToStartCodegenFrom,
                                      vector<Instruction> &partlySSAInstrs,
                                      string &varName,
                                      int *countInstr,
                                   #ifdef NEW2018_08_10
                                      string *virtRegVarNameIdRegDef
                                   #else
                                      int *virtRegVarNameIdRegDef
                                   #endif
                                   );

        //static
        string *sdNodeVarNameRegDef;

        //static
        int numInstructionsToCodegen = -1;
        int offsetKernelToStartCodegenFrom = -1;
        int useGlue = 1;

        /*
         * The method generates precomputed tables with the assembled kernels,
         *  during a run before the actual run to avoid the reasonably big
         *  overhead of assembling the kernel at runtime during the real
         *  processing, given by the rather complex C++ framework of OPINCAA.
         */
        string genPrecomputedKernel();

        /*
         * Resets the loop size counter so each appended instruction
         * after this one increments it with 1. It is used to determine
         * where the jump needs to be made.
         */
        void resetLoopJumpTarget();

        /*
         * This will append the jump instruction to the kernel by
         * using the loop jump target.
         */
        void appendLoopInstruction(int jmpOpcode = _IJMPNZ, int redReg = 0);


        vector<InstructionType> &getInstructions();


        void validateAndOptimizeRepeatLoops();

    private:

        /*
         * The name of the kernel;
         */
        string name;

        /*
         * The vector containing the instructions
         */
        vector<InstructionType> instructions;

        int loopNestDepth;
        //
        /*
         *  jumpTargetForLoopOfNestDepth[i] = the target of the loop with
         *    depth i from the current loop nest.
         *
         *  Note: We start from index 1 (index 0 is NOT used) since we, as LLVM,
         *   consider a simple loop has a depth of 1 (depth 0 doesn't exist).
         */
        unsigned short jumpTargetForLoopOfNestDepth[MAX_DEPTH_LOOP_NESTING + 1];
};

#endif // BATCH_H
