#ifndef VECTOR_H
#define VECTOR_H

#include <stdlib.h>
#include <stdio.h>

#define BEGIN_BATCH(x)  vector::setBatchIndex(x)
#define END_BATCH(x)
//#define BATCH(x) x

#define REDUCE(x)       vector::reduce(x)
#define ADDC(x,y)       vector::addc(x,y)
#define SUBC(x,y)       vector::subc(x,y)
#define ULT(x,y)        vector::ult(x,y)
#define SHRA(x,y)       vector::shra(x,y)
#define ISHRA(x,y)      vector::ishra(x,y)
#define INIT()          vector::initialize()

#define NOP             vector::nop()
#define SET_ACTIVE(x)   x
#define WHERE_CARRY     vector::WhereCry();
#define WHERE_EQ        vector::WhereEq();
#define WHERE_LT        vector::WhereLt();
#define ALL             vector::EndWhere();

#define EXECUTE_KERNEL(Batch)        vector::executeKernel(Batch)
#define EXECUTE_KERNEL_RED(Batch)    vector::executeKernelRed(Batch)
#define VERIFY_KERNEL(Batch)         vector::verifyKernel(Batch)
#define DEASM_KERNEL(Batch)          vector::deasmKernel(Batch)

#define UINT32 unsigned int
#define UINT_INSTRUCTION UINT32
#define UINT_PARAM UINT32
#define UINT16 unsigned short int

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
#define NUMBER_OF_BATCHES 10
#define NUMBER_OF_INSTRUCTIONS_PER_BATCH 40

#define STATIC_VECTOR_DEFINITIONS\
        extern UINT_INSTRUCTION vector::dwBatch[NUMBER_OF_BATCHES][NUMBER_OF_INSTRUCTIONS_PER_BATCH];\
        extern UINT16 vector::dwInBatchCounter[NUMBER_OF_BATCHES];\
        extern UINT16 vector::dwBatchIndex;\
        extern FILE* vector::pipe_read_32;\
        extern FILE* vector::pipe_write_32;

#define PASS 0
#define FAIL -1

// There is no instruction with FF, so there is no chance to mistake the INDEX_MARKER
// There is no instruction with FF, so there is no chance to mistake the SHIFTREG_MARKER
#define INDEX_MARKER 0xFFFFFFFF
#define SHIFTREG_MARKER 0xFFFFFFFE

class vector
{
    public:
        //vars
        static UINT_INSTRUCTION dwBatch[NUMBER_OF_BATCHES][NUMBER_OF_INSTRUCTIONS_PER_BATCH];
        static UINT16 dwInBatchCounter[NUMBER_OF_BATCHES];
        static UINT16 dwBatchIndex;

        static FILE *pipe_read_32, *pipe_write_32;

        //methods: static

        static vector addc(vector other_left, vector other_right);
        static vector subc(vector other_left, vector other_right);
        static vector shra(vector other_left, vector other_right);
        static vector ishra(vector other_left, UINT_PARAM right);

        static vector iwr(UINT_PARAM val);
        static void reduce(vector other_left);
        static vector ult(vector other_left, vector other_right);
        static void onlyOpcode(UINT_INSTRUCTION opcode);

        static void nop();
        static void WhereCry();
        static void WhereEq();
        static void WhereLt();
        static void EndWhere();

        static void setBatchIndex(UINT16 BI);
        static int initialize();
        static void executeKernel(UINT16 dwBatchNumber);
        static int executeKernelRed(UINT16 dwBatchNumber);
        static int verifyKernel(UINT16 dwBatchNumber);
        static int verifyKernelInstruction(UINT_INSTRUCTION Instruction);
        static int deasmKernel(UINT16 dwBatchNumber);

        // methods: non-static
        vector(UINT_INSTRUCTION MainValue, UINT_INSTRUCTION IntermediateValue);
        vector(UINT_INSTRUCTION MainValue, UINT_INSTRUCTION IntermediateValuem, UINT_INSTRUCTION OpCode);
        virtual ~vector();

        // methods: non-static : overloaded operators
        vector operator+(vector);
        vector operator-(vector);
        vector operator=(vector);
        vector operator=(UINT_PARAM);

        vector operator~(void);
        vector operator|(vector);
        vector operator&(vector);
        vector operator==(vector);
        vector operator<(vector);
        vector operator^(vector);
        vector operator>(vector);

        vector operator<<(vector);//shl
        vector operator<<(UINT_PARAM val);//ishl

        vector operator>>(vector);//shr
        vector operator>>(UINT_PARAM val);//ishr

    protected:
    private:
        UINT_INSTRUCTION mval; /* 0 for R0, .. 15 for R15 */
        UINT_INSTRUCTION ival; /* intermediate assembly of instruction, usually R[RIGHT] concatenated with R[LEFT] */
        UINT_INSTRUCTION imval; /* immediate value: will make distinction between 6 and 9 bits-opcode */;
        UINT_INSTRUCTION opcode;
};

#endif // VECTOR_H
