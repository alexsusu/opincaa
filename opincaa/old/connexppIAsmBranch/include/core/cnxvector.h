/*
 * File:   cnxvector.h
 *
 * Counterpart for cnxvector.c
 * Contains class definition and some very useful macros.
 *
 */

#ifndef CNXVECTOR_H
#define CNXVECTOR_H

//#include <stdlib.h>
#include <stdio.h>
#include "../util/utils.h"
#include <stdlib.h>
#include "cnxvector_errors.h"


#define NUMBER_OF_BATCHES 100
#define MAX_MULTIRED_DWORDS (2*1024*1024) // seems that there is a problem with linux, trying to read more tham 8 MB.

#ifndef _MSC_VER //MS C++ compiler
	#define STATIC_cnxvector_DEFINITIONS\
        UINT_INSTRUCTION* cnxvector::dwBatch[NUMBER_OF_BATCHES];\
        UINT32 cnxvector::dwInBatchCounter[NUMBER_OF_BATCHES];\
        UINT16 cnxvector::dwBatchIndex;\
        int cnxvector::pipe_read_32;\
        int cnxvector::pipe_write_32;\
        int cnxvector::dwErrorCounter;\
        unsigned char cnxvector::bEstimationMode;\
        int cnxvector::dwLastJmpLabel;

#else
	#define STATIC_cnxvector_DEFINITIONS\
        UINT_INSTRUCTION* cnxvector::dwBatch[NUMBER_OF_BATCHES];\
        UINT32 cnxvector::dwInBatchCounter[NUMBER_OF_BATCHES];\
        UINT16 cnxvector::dwBatchIndex;\
        int cnxvector::pipe_read_32;\
        int cnxvector::pipe_write_32;\
        int cnxvector::dwErrorCounter;\
        unsigned char cnxvector::bEstimationMode;\
        int cnxvector::dwLastJmpLabel;

#endif

/*
 The idea of "estimation mode" in a batch is:
  Pass once for counting how many slots we need in a particular slot, then pass again and fill it with instructions.
  see implementation of function appendInstruction(...)

*/
#define JMP_MODE_SET_LABEL 0
#define JMP_MODE_PERFORM 1
#define SET_JMP_LABEL(x) cnxvector::jmp(JMP_MODE_SET_LABEL,0)
#define JMP_TIMES_TO_LABEL(times,label) cnxvector::jmp(JMP_MODE_PERFORM, times)
#define REPEAT_X_TIMES(times, instructions_to_repeat) \
    if (times > 1) {SET_JMP_LABEL(0);SET_JMP_LABEL(0); instructions_to_repeat; JMP_TIMES_TO_LABEL(times-1,0);}\
    else if (times == 1) {instructions_to_repeat}


#define BEGIN_BATCH(x) {unsigned char BatchLoopTimes = 2;\
                        while (BatchLoopTimes-- > 0)\
                        {\
                            if (BatchLoopTimes == 1)\
                            {\
                                cnxvector::bEstimationMode = 1;\
                                cnxvector::setBatchIndex(x);\
                            }\
                            else \
                            {\
                                cnxvector::bEstimationMode = 0;\
                                if (cnxvector::dwBatch[cnxvector::dwBatchIndex] != NULL)\
                                    free((UINT_INSTRUCTION*)cnxvector::dwBatch[cnxvector::dwBatchIndex]);\
                                /*printf("Alloc mem for batch size of %ld slots \n", cnxvector::dwInBatchCounter[cnxvector::dwBatchIndex]);*/\
                                cnxvector::dwBatch[cnxvector::dwBatchIndex] = (UINT_INSTRUCTION*)malloc(sizeof(UINT_INSTRUCTION) \
                                                                        * cnxvector::dwInBatchCounter[cnxvector::dwBatchIndex]);\
                                if (cnxvector::dwBatch[cnxvector::dwBatchIndex] == NULL)\
                                    cnxvector::cnxvectorError(ERR_NOT_ENOUGH_MEMORY);\
                                    \
                                cnxvector::dwInBatchCounter[cnxvector::dwBatchIndex] = 0;\
                            }

#define END_BATCH(x)    }}

//#define BATCH(x) x

#define REDUCE(x)       cnxvector::reduce(x)
#define ADDC(x,y)       cnxvector::addc(x,y)
#define SUBC(x,y)       cnxvector::subc(x,y)
#define ULT(x,y)        cnxvector::ult(x,y)
#define SHRA(x,y)       cnxvector::shra(x,y)
#define ISHRA(x,y)      cnxvector::ishra(x,y)
#define CELL_SHL(x,y)   cnxvector::cellshl(x,y)
#define CELL_SHR(x,y)   cnxvector::cellshr(x,y)

#define _LOW(x)       cnxvector::multlo(x)
#define _HIGH(x)       cnxvector::multhi(x)

#define NOP             cnxvector::nop()

#define ENABLE_ALL                cnxvector::EndWhere();
#define EXECUTE_IN_ALL(x)         cnxvector::EndWhere();x;
#define EXECUTE_WHERE_CARRY(x)    cnxvector::WhereCry();x;
#define EXECUTE_WHERE_EQ(x)       cnxvector::WhereEq();x;
#define EXECUTE_WHERE_LT(x)       cnxvector::WhereLt();x;

#define FOUND_ERROR()             cnxvector::foundError()
#define GET_NUM_ERRORS()          cnxvector::getNumErrors()

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
// There is no instruction opcode starting with FF, so there is no chance to mistake the NO_IMVAL_MARKER
// There is no instruction opcode starting with FF, so there is no chance to mistake the NO_IVAL_MARKER
#define INDEX_MARKER            0xFFFFFFFF
#define SHIFTREG_MARKER         0xFFFFFFFE
#define LOCALSTORE_MARKER       0xFFFFFFFD
#define MULTIPLICATION_MARKER   0xFFFFFFFC
#define NO_IMVAL_MARKER         0xFFFFFFFB
#define NO_IVAL_MARKER          0xFFFFFFFA

class cnxvector
{
    public:
        //vars: static
        static UINT_INSTRUCTION *dwBatch[NUMBER_OF_BATCHES];
        static UINT32 dwInBatchCounter[NUMBER_OF_BATCHES];
        static UINT16 dwBatchIndex;
        static UINT8 bEstimationMode;
        static int pipe_read_32, pipe_write_32;
        static int dwErrorCounter;
        static int dwLastJmpLabel;

        //methods: static
        static void appendInstruction(UINT_INSTRUCTION instr);
        static void replaceInstruction(UINT_INSTRUCTION instr, int index);

        static cnxvector addc(cnxvector other_left, cnxvector other_right);
        static cnxvector addc(cnxvector other_left, UINT_PARAM value);

        static cnxvector subc(cnxvector other_left, cnxvector other_right);
        static cnxvector subc(cnxvector other_left, UINT_PARAM value);

        static cnxvector shra(cnxvector other_left, cnxvector other_right);
        static cnxvector ishra(cnxvector other_left, UINT_PARAM value);

        static cnxvector multhi(cnxvector mult);
        static cnxvector multlo(cnxvector mult);

        static void cellshl(cnxvector other_left, cnxvector other_right);
        static void cellshr(cnxvector other_left, cnxvector other_right);

        static void reduce(cnxvector other_left);
        static cnxvector ult(cnxvector other_left, cnxvector other_right);
        static cnxvector ult(cnxvector other_left, UINT_PARAM value);

        static void onlyOpcode(UINT_INSTRUCTION opcode);
        static void jmp(int mode, int Loops);

        static void nop();
        static void WhereCry();
        static void WhereEq();
        static void WhereLt();
        static void EndWhere();

        static void cnxvectorError(const char*);
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
        cnxvector(UINT_INSTRUCTION MainValue);
        cnxvector(UINT_INSTRUCTION MainValue, UINT_INSTRUCTION IntermediateValue);
        cnxvector(UINT_INSTRUCTION MainValue, UINT_INSTRUCTION IntermediateValue, UINT_INSTRUCTION OpCode);
        cnxvector(UINT_INSTRUCTION MainValue, UINT_INSTRUCTION IntermediateValue, UINT_INSTRUCTION ImmVal, UINT_INSTRUCTION OpCode);
        virtual ~cnxvector();

        // methods: non-static : overloaded operators
        cnxvector operator+(cnxvector);
        cnxvector operator+(UINT_PARAM);
        void operator+=(cnxvector);

        cnxvector operator-(cnxvector);
        cnxvector operator-(UINT_PARAM);
        void operator-=(cnxvector);

        cnxvector operator*(cnxvector);
        cnxvector operator*(UINT_PARAM);

        void operator=(cnxvector);
        void operator=(UINT_PARAM);

        cnxvector operator~(void);

        cnxvector operator|(cnxvector);
        cnxvector operator|(UINT_PARAM);
        void operator|=(cnxvector);

        cnxvector operator&(cnxvector);
        cnxvector operator&(UINT_PARAM);
        void operator&=(cnxvector);

        cnxvector operator==(cnxvector);
        cnxvector operator==(UINT_PARAM);

        cnxvector operator<(cnxvector);
        cnxvector operator<(UINT_PARAM);

        cnxvector operator^(cnxvector);
        cnxvector operator^(UINT_PARAM);
        void operator^=(cnxvector);

        //cnxvector operator>(cnxvector);
        //cnxvector operator>(UINT_PARAM);

        cnxvector operator[](cnxvector);
        cnxvector operator[](UINT_PARAM);

        cnxvector operator<<(cnxvector);//shl
        cnxvector operator<<(UINT_PARAM);//ishl

        cnxvector operator>>(cnxvector);//shr
        cnxvector operator>>(UINT_PARAM);//ishr

        static UINT_INSTRUCTION getBatchInstruction(UINT16 BI, UINT32 index);
        static UINT_INSTRUCTION* getBatch(UINT16 BI);
        static void setBatch(UINT16 BI, UINT_INSTRUCTION* Batch);
        static UINT32 getInBatchCounter(UINT16 BI);
        static void setInBatchCounter(UINT16 BI, UINT32 InBatchCounter);

    protected:
    private:
        UINT_INSTRUCTION mval; /* 0 for R0, .. 31 for R31 */
        UINT_INSTRUCTION ival; /* intermediate assembly of instruction, usually R[RIGHT] concatenated with R[LEFT] */
        UINT_INSTRUCTION imval; /* immediate value: will make distinction between 6 and 9 bits-opcode */;
        UINT_INSTRUCTION opcode;
};

#endif // CNXVECTOR_H
