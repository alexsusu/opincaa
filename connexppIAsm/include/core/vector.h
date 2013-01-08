/*
 * File:   vector.h
 *
 * Counterpart for vector.c
 * Contains class definition and some very useful macros.
 *
 */

#ifndef VECTOR_H
#define VECTOR_H

//#include <stdlib.h>
#include <stdio.h>
#include "../utils.h"
#include <stdlib.h>
#include "vector_errors.h"

/*
 The idea of "estimation mode" in a batch is:
  Pass once for counting how many slots we need in a particular slot, then pass again and fill it with instructions.
  see implementation of function appendInstruction(...)

*/

#define BEGIN_BATCH(x)  unsigned char BatchLoopTimes = 2;\
                        while (BatchLoopTimes-- > 0)\
                        {\
                            if (BatchLoopTimes == 1)\
                            {\
                                vector::bEstimationMode = 1;\
                                vector::setBatchIndex(x);\
                            }\
                            else \
                            {\
                                vector::bEstimationMode = 0;\
                                if (vector::dwBatch[vector::dwBatchIndex] != NULL)\
                                    free((UINT_INSTRUCTION*)vector::dwBatch[vector::dwBatchIndex]);\
                                /*printf("Alloc mem for batch size of %ld slots \n", vector::dwInBatchCounter[vector::dwBatchIndex]);*/\
                                vector::dwBatch[vector::dwBatchIndex] = (UINT_INSTRUCTION*)malloc(sizeof(UINT_INSTRUCTION) \
                                                                        * vector::dwInBatchCounter[vector::dwBatchIndex]);\
                                if (vector::dwBatch[vector::dwBatchIndex] == NULL)\
                                    vector::vectorError(ERR_NOT_ENOUGH_MEMORY);\
                                    \
                                vector::dwInBatchCounter[vector::dwBatchIndex] = 0;\
                            }

#define END_BATCH(x)    }

//#define BATCH(x) x

#define REDUCE(x)       vector::reduce(x)
#define ADDC(x,y)       vector::addc(x,y)
#define SUBC(x,y)       vector::subc(x,y)
#define ULT(x,y)        vector::ult(x,y)
#define SHRA(x,y)       vector::shra(x,y)
#define ISHRA(x,y)      vector::ishra(x,y)
#define CELL_SHL(x,y)   vector::cellshl(x,y)
#define CELL_SHR(x,y)   vector::cellshr(x,y)

#define _LO(x)       vector::multlo(x)
#define _HI(x)       vector::multhi(x)

#define NOP             vector::nop()

#define EXECUTE_IN_ALL(x)         vector::EndWhere();x;
#define EXECUTE_WHERE_CARRY(x)    vector::WhereCry();x;
#define EXECUTE_WHERE_EQ(x)        vector::WhereEq();x;
#define EXECUTE_WHERE_LT(x)        vector::WhereLt();x;

#define FOUND_ERROR()                vector::foundError()
#define GET_NUM_ERRORS()             vector::getNumErrors()

#define UINT_PARAM UINT32

/* Unfortunately, static is a great example of how NOT to design a language:
    If you have a class (with regular class.h, and class.c) static fields must
    be declared and defined as static in class.h.

    Static methods must not be declared as static in class.c (otherwise this would
    hide them from the outside).

    Additionally, the static fields MUST be declared in every *.c file
    (except class.c ) in order to be accessible via "static methods "
    ("static methods" are methods that can temper with static fields since they
     belong to the class, not to a class's instantiation (object)

*/

#define NUMBER_OF_BATCHES 100
//#define NUMBER_OF_INSTRUCTIONS_PER_BATCH 40

#define STATIC_VECTOR_DEFINITIONS\
        extern UINT_INSTRUCTION* vector::dwBatch[NUMBER_OF_BATCHES];\
        extern UINT32 vector::dwInBatchCounter[NUMBER_OF_BATCHES];\
        extern UINT16 vector::dwBatchIndex;\
        extern int vector::pipe_read_32;\
        extern int vector::pipe_write_32;\
        extern int vector::dwErrorCounter;\
        extern unsigned char vector::bEstimationMode;

// There is no instruction opcode starting with FF, so there is no chance to mistake the INDEX_MARKER
// There is no instruction opcode starting with FF, so there is no chance to mistake the SHIFTREG_MARKER
// There is no instruction opcode starting with FF, so there is no chance to mistake the LOCALSTORE_MARKER
// There is no instruction opcode starting with FF, so there is no chance to mistake the MULTIPLICATION_MARKER
#define INDEX_MARKER    0xFFFFFFFF
#define SHIFTREG_MARKER     0xFFFFFFFE
#define LOCALSTORE_MARKER   0xFFFFFFFD
#define MULTIPLICATION_MARKER 0xFFFFFFFC

class vector
{
    public:
        //vars
        //static UINT_INSTRUCTION dwBatch[NUMBER_OF_BATCHES][NUMBER_OF_INSTRUCTIONS_PER_BATCH];
        static UINT_INSTRUCTION *dwBatch[NUMBER_OF_BATCHES];
        static UINT32 dwInBatchCounter[NUMBER_OF_BATCHES];
        static UINT16 dwBatchIndex;
        static UINT8 bEstimationMode;
        static int pipe_read_32, pipe_write_32;
        static int dwErrorCounter;

        //methods: static
        static void appendInstruction(UINT_INSTRUCTION instr);

        static vector addc(vector other_left, vector other_right);
        static vector subc(vector other_left, vector other_right);
        static vector shra(vector other_left, vector other_right);
        static vector ishra(vector other_left, UINT_PARAM right);

        static vector multhi(vector mult);
        static vector multlo(vector mult);

        static void cellshl(vector other_left, vector other_right);
        static void cellshr(vector other_left, vector other_right);

        static void reduce(vector other_left);
        static vector ult(vector other_left, vector other_right);
        static void onlyOpcode(UINT_INSTRUCTION opcode);

        static void nop();
        static void WhereCry();
        static void WhereEq();
        static void WhereLt();
        static void EndWhere();

        static void vectorError(const char*);
        static int foundError();
        static int getNumErrors();

        static void setBatchIndex(UINT16 BI);
        static int initialize();
        static int deinitialize();
        static int executeKernel(UINT16 dwBatchNumber);
        static int executeKernelRed(UINT16 dwBatchNumber);
        static int verifyKernel(UINT16 dwBatchNumber);
        static int verifyKernelInstruction(UINT_INSTRUCTION Instruction);
        static int deasmKernel(UINT16 dwBatchNumber);

        // methods: non-static
        vector(UINT_INSTRUCTION MainValue, UINT_INSTRUCTION IntermediateValue);
        vector(UINT_INSTRUCTION MainValue, UINT_INSTRUCTION IntermediateValue, UINT_INSTRUCTION OpCode);
        vector(UINT_INSTRUCTION MainValue, UINT_INSTRUCTION IntermediateValue, UINT16 ImmVal, UINT_INSTRUCTION OpCode);
        virtual ~vector();

        // methods: non-static : overloaded operators
        vector operator+(vector);
        //vector operator+=(vector);
        vector operator-(vector);
        //vector operator-=(vector);

        vector operator*(vector);

        void operator=(vector);
        void operator=(UINT_PARAM);

        vector operator~(void);
        vector operator|(vector);
        vector operator&(vector);
        vector operator==(vector);
        vector operator<(vector);
        vector operator^(vector);
        vector operator>(vector);

        vector operator[](vector);
        vector operator[](UINT_PARAM);

        vector operator<<(vector);//shl
        vector operator<<(UINT_PARAM);//ishl

        vector operator>>(vector);//shr
        vector operator>>(UINT_PARAM);//ishr

    protected:
    private:
        //UINT_INSTRUCTION mval; /* 0 for R0, .. 15 for R15 */
        UINT_INSTRUCTION mval;
        UINT_INSTRUCTION ival; /* intermediate assembly of instruction, usually R[RIGHT] concatenated with R[LEFT] */
        UINT_INSTRUCTION imval; /* immediate value: will make distinction between 6 and 9 bits-opcode */;
        UINT_INSTRUCTION opcode;
};

#endif // VECTOR_H
