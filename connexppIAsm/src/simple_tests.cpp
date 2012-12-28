/*
 *
 * File: simple_tests.cpp
 *
 * Simple tests for all instructions on connex machine.
 * Each instruction is tested via one bacth that uses reduction for checking the result
 *
 */
#include "../include/vector_registers.h"
#include "../include/vector.h"
#include <iostream>
using namespace std;

#define NUMBER_OF_MACHINES 128LL
//STATIC_VECTOR_DEFINITIONS;


struct TestFunction
{
   int BatchNumber;
   const char *OperationName;
   INT64 Param1;
   INT64 Param2;
   void (*initKernel)(int BatchNumber,INT64 Param1,INT64 Param2);
   INT64 ExpectedResult;
};

void InitKernel_Add(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2;
        R3 = R1 + R2;
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Addc(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R4 = 0xFF;
        R5 = 0xFFFF;
        R6 = R4 + R5;
        R1 = Param1;
        R2 = Param2;
        R3 = ADDC(R1, R2);
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Sub(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2;
        R3 = R1 - R2;
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Subc(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = 0xff;
        R2 = 0xffff;
        R3 = R1 + R2;

        R1 = Param1;
        R2 = Param2;
        R3 = SUBC(R1,R2);
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Not(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2; //avoid compiler warning
        R3 = ~R1;
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Or(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2; //avoid compiler warning
        R3 = R1 | R2;
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_And(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2; //avoid compiler warning
        R3 = R1 & R2;
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Xor(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2; //avoid compiler warning
        R3 = R1 ^ R2;
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Eq(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2; //avoid compiler warning
        R3 = (R1 == R2);
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Lt(int BatchNumber,INT64 Param1, INT64 Param2)
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
void InitKernel_Ult(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2; //avoid compiler warning
        R3 = (R1 < R2);
        REDUCE(R3);
    END_BATCH(BatchNumber);
}


void InitKernel_Shl(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2;
        R3 = (R1 << R2);
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Shr(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2;
        R3 = (R1 >> R2 );
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Shra(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        SET_ACTIVE(ALL);
        R1 = Param1;
        R2 = Param2;
        R3 = SHRA(R1, R2);
        REDUCE(R3);
    END_BATCH(BatchNumber);
}

void InitKernel_Ishra(int BatchNumber,INT64 Param1, INT64 Param2)
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
    MAX_BNR = NUMBER_OF_BATCHES
} BatchNumbers;


// TODO with random numbers.
// Remember to handle truncation properly !
// eg: 128* 0xff ff ff ff = ???
#define REGISTER_SIZE_MASK 0xffff
TestFunction TestFunctionTable[] =
{
    {ADD_BNR,"ADD",0xff,0xf1,InitKernel_Add,(0xff + 0xf1)*NUMBER_OF_MACHINES},
    {ADDC_BNR,"ADDC",0xf0,0x1,InitKernel_Addc,(0xf0 + 1 + 1)*NUMBER_OF_MACHINES}, // ???
    {SUB_BNR,"SUB",0xffff,0xff8f,InitKernel_Sub, (0xffff - 0xff8f)*NUMBER_OF_MACHINES},
    {SUBC_BNR,"SUBC",0xffff,0xff8f,InitKernel_Subc,(0xffff - 0xff8f -1)*NUMBER_OF_MACHINES}, // ???
    {NOT_BNR,"NOT",0xfff0,0x00,InitKernel_Not,(0xf)*NUMBER_OF_MACHINES},
    {OR_BNR,"OR",0x10,0x01,InitKernel_Or,(0x10 | 0x01)*NUMBER_OF_MACHINES},
    {AND_BNR,"AND",0xfffe,0x11,InitKernel_And,(0xfffe & 0x11)*NUMBER_OF_MACHINES},
    {XOR_BNR,"XOR",0x01,0x10,InitKernel_Xor,(0x01 ^ 0x10)*NUMBER_OF_MACHINES},
    {EQ_BNR,"EQ",0xff3f,0xff3f,InitKernel_Eq,(0xff3f == 0xff3f)*NUMBER_OF_MACHINES},
    {LT_BNR,"LT",0xabcd,0xabcc,InitKernel_Lt,(0xabcd <  0xabcc)*NUMBER_OF_MACHINES},  // ??? in the sense ffff ffff < ffff fffe (neg numbers)
    //{ULT_BNR,"ULT",0xabcd,0xabcc,InitKernel_Ult,(0xabcd > 0xabcc)*NUMBER_OF_MACHINES}, // ??? in the sense ffff ffff > ffff fffe (pos numbers)
    {SHL_BNR,"SHL",0xcd,3,InitKernel_Shl,(0xcd << 3)*NUMBER_OF_MACHINES},
    {SHR_BNR,"SHR",0xabcd,3,InitKernel_Shr,(0xabcd >> 3)*NUMBER_OF_MACHINES},
    {SHRA_BNR,"SHRA",0x01cd,4,InitKernel_Shra,(0x01c)*NUMBER_OF_MACHINES},//will fail: 128*big
    {ISHRA_BNR,"ISHRA",0xabcd,4,InitKernel_Shra,(0xfabcUL)*NUMBER_OF_MACHINES},

};

int test_Simple_All()
{
    UINT16 i = 0;
    INT64 result;
    UINT16 testFails = 0;

    for (i = 0; i < sizeof (TestFunctionTable) / sizeof (TestFunction); i++)
    {
        TestFunctionTable[i].initKernel( TestFunctionTable[i].BatchNumber, TestFunctionTable[i].Param1, TestFunctionTable[i].Param2 );
        result = EXECUTE_KERNEL_RED(TestFunctionTable[i].BatchNumber);
        if (result != TestFunctionTable[i].ExpectedResult)
        {
           cout<< "Test "<<TestFunctionTable[i].OperationName<<"  FAILED with result "
           <<result << " (expected " <<TestFunctionTable[i].ExpectedResult<<" ) !"<<endl;
           testFails++;
        }
        else
            printf("Test %s     passed ! \n", TestFunctionTable[i].OperationName);
    }

    //DEASM_KERNEL(OR_BNR);
    //DEASM_KERNEL(ADDC_BNR);

    return testFails;
}
