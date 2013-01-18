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
#include "../util/utils.h"
#include <stdlib.h>
#include "vector_errors.h"


#define NUMBER_OF_BATCHES 100
#define MAX_MULTIRED_DWORDS (2*1024*1024) // seems that there is a problem with linux, trying to read more tham 8 MB.

#ifndef _MSC_VER //MS C++ compiler
	#define STATIC_VECTOR_DEFINITIONS\
        UINT_INSTRUCTION* vector::dwBatch[NUMBER_OF_BATCHES];\
        UINT32 vector::dwInBatchCounter[NUMBER_OF_BATCHES];\
        UINT16 vector::dwBatchIndex;\
        int vector::pipe_read_32;\
        int vector::pipe_write_32;\
        int vector::dwErrorCounter;\
        unsigned char vector::bEstimationMode;

#else
	#define STATIC_VECTOR_DEFINITIONS\
        UINT_INSTRUCTION* vector::dwBatch[NUMBER_OF_BATCHES];\
        UINT32 vector::dwInBatchCounter[NUMBER_OF_BATCHES];\
        UINT16 vector::dwBatchIndex;\
        int vector::pipe_read_32;\
        int vector::pipe_write_32;\
        int vector::dwErrorCounter;\
        unsigned char vector::bEstimationMode;

#endif

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

#define _LOW(x)       vector::multlo(x)
#define _HIGH(x)       vector::multhi(x)

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
        //vars: static
        static UINT_INSTRUCTION *dwBatch[NUMBER_OF_BATCHES];
        static UINT32 dwInBatchCounter[NUMBER_OF_BATCHES];
        static UINT16 dwBatchIndex;
        static UINT8 bEstimationMode;
        static int pipe_read_32, pipe_write_32;
        static int dwErrorCounter;

        //methods: static
        static void appendInstruction(UINT_INSTRUCTION instr);

        static vector addc(vector other_left, vector other_right);
        static vector addc(vector other_left, UINT_PARAM value);

        static vector subc(vector other_left, vector other_right);
        static vector subc(vector other_left, UINT_PARAM value);

        static vector shra(vector other_left, vector other_right);
        static vector ishra(vector other_left, UINT_PARAM value);

        static vector multhi(vector mult);
        static vector multlo(vector mult);

        static void cellshl(vector other_left, vector other_right);
        static void cellshr(vector other_left, vector other_right);

        static void reduce(vector other_left);
        static vector ult(vector other_left, vector other_right);
        static vector ult(vector other_left, UINT_PARAM value);

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

        static UINT_RED_REG_VAL executeBatch(UINT16 dwBatchNumber);
        static UINT_RED_REG_VAL executeBatchRed(UINT16 dwBatchNumber);
        static UINT32 getMultiRedResult(UINT_RED_REG_VAL* RedResults, UINT32 Limit);

        static int verifyBatch(UINT16 dwBatchNumber);
        static int verifyBatchInstruction(UINT_INSTRUCTION Instruction);

        // methods: non-static
        vector(UINT_INSTRUCTION MainValue, UINT_INSTRUCTION IntermediateValue);
        vector(UINT_INSTRUCTION MainValue, UINT_INSTRUCTION IntermediateValue, UINT_INSTRUCTION OpCode);
        vector(UINT_INSTRUCTION MainValue, UINT_INSTRUCTION IntermediateValue, UINT16 ImmVal, UINT_INSTRUCTION OpCode);
        virtual ~vector();

        // methods: non-static : overloaded operators
        vector operator+(vector);
        vector operator+(UINT_PARAM);
        void operator+=(vector);

        vector operator-(vector);
        vector operator-(UINT_PARAM);
        void operator-=(vector);

        vector operator*(vector);
        vector operator*(UINT_PARAM);

        void operator=(vector);
        void operator=(UINT_PARAM);

        vector operator~(void);

        vector operator|(vector);
        vector operator|(UINT_PARAM);
        void operator|=(vector);

        vector operator&(vector);
        vector operator&(UINT_PARAM);
        void operator&=(vector);

        vector operator==(vector);
        vector operator==(UINT_PARAM);

        vector operator<(vector);
        vector operator<(UINT_PARAM);

        vector operator^(vector);
        vector operator^(UINT_PARAM);
        void operator^=(vector);

        vector operator>(vector);
        vector operator>(UINT_PARAM);

        vector operator[](vector);
        vector operator[](UINT_PARAM);

        vector operator<<(vector);//shl
        vector operator<<(UINT_PARAM);//ishl

        vector operator>>(vector);//shr
        vector operator>>(UINT_PARAM);//ishr

        static UINT_INSTRUCTION getBatchInstruction(UINT16 BI, UINT32 index);
        static UINT32 getInBatchCounter(UINT16 BI);

    protected:
    private:
        UINT_INSTRUCTION mval; /* 0 for R0, .. 31 for R31 */
        UINT_INSTRUCTION ival; /* intermediate assembly of instruction, usually R[RIGHT] concatenated with R[LEFT] */
        UINT_INSTRUCTION imval; /* immediate value: will make distinction between 6 and 9 bits-opcode */;
        UINT_INSTRUCTION opcode;
};

#endif // VECTOR_H
