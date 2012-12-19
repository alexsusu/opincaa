#ifndef VECTOR_H
#define VECTOR_H

#include <stdlib.h>
#include <stdio.h>

#define BEGIN_BATCH(x)  vector::setBatchIndex(x)
#define END_BATCH(x)
//#define BATCH(x) x

#define REDUCE(x)       vector::reduce(x)
#define ADDC(x,y)       vector::addc(x,y)
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

#define STATIC_VECTOR_DEFINITIONS\
        int vector::dwBatch[1000][1000];\
        int vector::dwInBatchCounter[1000];\
        int vector::dwBatchIndex;\
        FILE* vector::pipe_read_32;\
        FILE* vector::pipe_write_32;

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
        static int dwBatch[1000][1000];
        static int dwInBatchCounter[1000];
        static int dwBatchIndex;

        static FILE *pipe_read_32, *pipe_write_32;

        //methods: static

        static vector addc(vector other_left, vector other_right);
        static vector subc(vector other_left, vector other_right);
        static vector shra(vector other_left, vector other_right);
        static vector ishl(vector other_left, vector other_right);
        static vector ishr(vector other_left, vector other_right);
        static vector ishra(vector other_left, vector other_right);
        static vector ishra(vector other, int val);

        static vector iwr(int val);
        static void reduce(vector other_left);
        static vector ult(vector other_left, vector other_right);
        static void onlyOpcode(int opcode);

        static void nop();
        static void WhereCry();
        static void WhereEq();
        static void WhereLt();
        static void EndWhere();

        static void setBatchIndex(int BI);
        static int initialize();
        static void executeKernel(int dwBatchNumber);
        static int executeKernelRed(int dwBatchNumber);
        static int verifyKernel(int dwBatchNumber);


        // methods: non-static
        vector(int MainValue, int IntermediateValue);
        vector(int MainValue, int IntermediateValuem, int OpCode);
        virtual ~vector();

        // methods: non-static : overloaded operators
        vector operator+(vector);
        vector operator-(vector);
        vector operator=(vector);
        vector operator=(int);

        vector operator||(vector);
        vector operator&&(vector);
        vector operator==(vector);
        vector operator<(vector);
        vector operator^(vector);
        vector operator>(vector);

        vector operator<<(vector);
        vector operator<<(int val);

        vector operator>>(vector);
        vector operator>>(int val);

    protected:
    private:
        int mval; /* 0 for R0, .. 15 for R15 */
        int ival; /* intermediate assembly of instruction, usually R[RIGHT] concatenated with R[LEFT] */
        int imval; /* immediate value: will make distinction between 6 and 9 bits-opcode */;
        int opcode;
};

#endif // VECTOR_H
