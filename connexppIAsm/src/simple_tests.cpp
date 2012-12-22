/*
 *
 * File: simple_tests.cpp
 *
 * Simple tests for all instructions on connex machine.
 * Each instruction is tested via one bacth that uses reduction for checking the result
 *
 */
#include "../include/vector_registers.h"

using namespace std;

#define NUMBER_OF_MACHINES 128
//STATIC_VECTOR_DEFINITIONS;


struct TestFunction
{
   int BatchNumber;
   char *OperationName;
   int Param1;
   int Param2;
   void (*initKernel)(int BatchNumber,int Param1, int Param2);
   int ExpectedResult;
};

void InitKernel_Add(int BatchNumber,int Param1, int Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2;
        R3 = R1 + R2;
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Addc(int BatchNumber,int Param1, int Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2;
        R3 = ADDC(R1, R2);
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Sub(int BatchNumber,int Param1, int Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2;
        R3 = R1 - R2;
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Subc(int BatchNumber,int Param1, int Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2;
        R3 = R1 - R2;
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Not(int BatchNumber,int Param1, int Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2; //avoid compiler warning
        R3 = ~R1;
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Or(int BatchNumber,int Param1, int Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2; //avoid compiler warning
        R3 = R1 | R2;
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_And(int BatchNumber,int Param1, int Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2; //avoid compiler warning
        R3 = R1 & R2;
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Xor(int BatchNumber,int Param1, int Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2; //avoid compiler warning
        R3 = R1 ^ R2;
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Eq(int BatchNumber,int Param1, int Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2; //avoid compiler warning
        R3 = (R1 == R2);
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Lt(int BatchNumber,int Param1, int Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2; //avoid compiler warning
        R3 = (R1 < R2);
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

// same as Lt for the moment
void InitKernel_Ult(int BatchNumber,int Param1, int Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2; //avoid compiler warning
        R3 = (R1 < R2);
        REDUCE(R3);
    END_BATCH(BatchNumber);
}


void InitKernel_Shl(int BatchNumber,int Param1, int Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R3 = (R1 << Param2 );
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Shr(int BatchNumber,int Param1, int Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R3 = (R1 >> Param2 );
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Shra(int BatchNumber,int Param1, int Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2;
        R3 = SHRA(R1, R2);
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Ishra(int BatchNumber,int Param1, int Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R3 = ISHRA(R1, Param2);
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

static enum
{
    ADD_BNR     = 0,
    SUB_BNR     = 1,
    ADDC_BNR    = 2,
    SUBC_BNR    = 3,
    NOT_BNR     = 4,
    OR_BNR      = 5,
    AND_BNR     = 6,
    XOR_BNR     = 7,
    EQ_BNR      = 8,
    LT_BNR      = 9,
    ULT_BNR     = 10,
    SHL_BNR     = 11,
    SHR_BNR     = 12,
    SHRA_BNR    = 13,
    ISHRA_BNR   = 14,

    MAX_BNR = 99
}BatchNumbers;



// TODO with random numbers.
// Remember to handle truncation properly !
// eg: 128* 0xff ff ff ff = ???
TestFunction TestFunctionTable[] =
{
    {ADD_BNR,"ADD",0xff,0xffff,InitKernel_Add,(0xff + 0xffff)*NUMBER_OF_MACHINES},
    {ADDC_BNR,"ADDC",0xff,0xffff,InitKernel_Addc,(0xff + 0xffff)*NUMBER_OF_MACHINES}, // ???
    {SUB_BNR,"SUB",0xff,0xffff,InitKernel_Sub,(0xff - 0xffff)*NUMBER_OF_MACHINES},
    {SUBC_BNR,"SUBC",0xff,0xffff,InitKernel_Subc,(0xff - 0xffff)*NUMBER_OF_MACHINES}, // ???
    {NOT_BNR,"NOT",0xffffff,0x00,InitKernel_Not,(~0xffffff)*NUMBER_OF_MACHINES},
    {OR_BNR,"OR",0x1010,0x0101,InitKernel_Or,(0x1010 | 0x0101)*NUMBER_OF_MACHINES},
    {AND_BNR,"AND",0xffff,0x1111,InitKernel_And,(0xffff & 0x1111)*NUMBER_OF_MACHINES},
    {XOR_BNR,"XOR",0x0101,0x1110,InitKernel_Xor,(0x0101 ^ 0x1110)*NUMBER_OF_MACHINES},
    {EQ_BNR,"EQ",0xff3fff,0xff3fff,InitKernel_Eq,(0xff3fff == 0xff3fff)*NUMBER_OF_MACHINES},
    {LT_BNR,"LT",0xabcdabcd,0xabccabcc,InitKernel_Lt,(0xabcdabcd <  0xabccabcc)*NUMBER_OF_MACHINES},  // ??? in the sense ffff ffff < ffff fffe (neg numbers)
    {ULT_BNR,"ULT",0xabcdabcd,0xabccabcc,InitKernel_Ult,(0xabcdabcd > 0xabccabcc)*NUMBER_OF_MACHINES}, // ??? in the sense ffff ffff > ffff fffe (pos numbers)
    {SHL_BNR,"SHL",0xabcdabcd,3,InitKernel_Shl,(0xabcdabcd << 3)*NUMBER_OF_MACHINES},
    {SHR_BNR,"SHR",0xabcdabcd,3,InitKernel_Shr,(0xabcdabcd >> 3)*NUMBER_OF_MACHINES},
    {SHRA_BNR,"SHRA",0xabcdabcd,4,InitKernel_Shra,(0xfabcdabc)*NUMBER_OF_MACHINES},//will fail: 128*big
    {ISHRA_BNR,"ISHRA",0xabcdabcd,4,InitKernel_Shra,(0xfabcdabc)*NUMBER_OF_MACHINES},
};

int test_Simple_All()
{
    int i = 0;
    int result;

    for (i = 0; i < sizeof (TestFunctionTable) / sizeof (TestFunction); i++)
    {
        TestFunctionTable[i].initKernel( TestFunctionTable[i].BatchNumber, TestFunctionTable[i].Param1, TestFunctionTable[i].Param2 );
        result = EXECUTE_KERNEL_RED(TestFunctionTable[i].BatchNumber);
        if (result != TestFunctionTable[i].ExpectedResult)
            printf("Test %s     FAILED ! \n", TestFunctionTable[i].OperationName);
        else
            printf("Test %s     passed ! \n", TestFunctionTable[i].OperationName);
    }
}
