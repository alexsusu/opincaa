/*
 *
 * File: simple_tests.cpp
 *
 * Simple tests for all instructions on connex machine.
 * Each instruction is tested via one bacth that uses reduction for checking the result
 *
 *
 */
#include "../include/core/vector_registers.h"
#include "../include/core/vector.h"
#include "../include/core/io_unit.h"
#include "../include/c_simu/c_simulator.h"
#include "../include/utils.h"
#include <iostream>
#include <iomanip>
using namespace std;
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

static void InitKernel_Nop(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = 1;
                        REDUCE(R3);
                    )

    END_BATCH(BatchNumber);
}

static void InitKernel_Iwrite(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = INDEX;
                        NOP;
                        LS[Param1] = R0;
                        R1 = LS[Param1];
                        REDUCE(R1);
                      )

    END_BATCH(BatchNumber);
}

static void InitKernel_Iread(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = INDEX;
                        NOP;
                        LS[Param1] = R0;
                        R1 = LS[Param1];
                        REDUCE(R1);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Write(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = INDEX;
                        R1 = Param1;
                        NOP;
                        LS[R1] = R0;
                        R2 = LS[R1];
                        REDUCE(R2);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Read(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = INDEX;
                        R1 = Param1;
                        NOP;
                        LS[R1] = R0;
                        R2 = LS[R1];
                        REDUCE(R2);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Vload(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = Param1;
                        R1 = Param2;
                        R2 = R0 + R1;
                        REDUCE(R2);
                    )

    END_BATCH(BatchNumber);
}

static void InitKernel_Add(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = R1 + R2;
                        REDUCE(R3);
                     )

    END_BATCH(BatchNumber);
}

static void InitKernel_pAdd(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = R1 + Param2;
                        //R3 = Param2 + R1;
                        REDUCE(R2);
                     )

    END_BATCH(BatchNumber);
}

static void InitKernel_sAdd(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R2 += R1;
                        REDUCE(R2);
                     )

    END_BATCH(BatchNumber);
}

static void InitKernel_Addc(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R4 = 0xFF;
                        R5 = 0xFFFF;
                        R6 = R4 + R5;
                        R1 = Param1;
                        R2 = Param2;
                        R3 = ADDC(R1, R2);
                        REDUCE(R3);
                    )

    END_BATCH(BatchNumber);
}

static void InitKernel_pAddc(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R4 = 0xFF;
                        R5 = 0xFFFF;
                        R6 = R4 + R5;
                        R1 = Param1;
                        R2 = ADDC(R1, Param2);
                        REDUCE(R2);
                    )

    END_BATCH(BatchNumber);
}

static void InitKernel_Sub(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = R1 - R2;
                        REDUCE(R3);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_sSub(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R1 -= R2;
                        REDUCE(R1);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_pSub(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = R1 - Param2;
                        REDUCE(R2);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Subc(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = 0xff;
                        R2 = 0xffff;
                        R3 = R1 - R2;

                        R1 = Param1;
                        R2 = Param2;
                        R3 = SUBC(R1,R2);
                        REDUCE(R3);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_pSubc(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = 0xff;
                        R2 = 0xffff;
                        R3 = R1 - R2;

                        R1 = Param1;
                        R2 = SUBC(R1,Param2);
                        REDUCE(R2);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Not(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2; //astatic void compiler warning
                        R3 = ~R1;
                        REDUCE(R3);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Or(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = R1 | R2;
                        REDUCE(R3);
                    )

    END_BATCH(BatchNumber);
}

static void InitKernel_sOr(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R1 |= R2;
                        REDUCE(R1);
                    )

    END_BATCH(BatchNumber);
}

static void InitKernel_pOr(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = R1 | Param2;
                        REDUCE(R2);
                    )

    END_BATCH(BatchNumber);
}

static void InitKernel_And(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = R1 & R2;
                        REDUCE(R3);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_sAnd(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R1 &= R2;
                        REDUCE(R1);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_pAnd(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = R1 & Param2;
                        REDUCE(R2);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Xor(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = R1 ^ R2;
                        REDUCE(R3);
                    )

    END_BATCH(BatchNumber);
}

static void InitKernel_sXor(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R1 ^= R2;
                        REDUCE(R1);
                    )

    END_BATCH(BatchNumber);
}

static void InitKernel_pXor(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = R1 ^ Param2;
                        REDUCE(R2);
                    )

    END_BATCH(BatchNumber);
}

static void InitKernel_Eq(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = (R1 == R2);
                        REDUCE(R3);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_pEq(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = (R1 == Param2);
                        REDUCE(R2);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Lt(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = (R1 < R2);
                        REDUCE(R3);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_pLt(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = (R1 < Param2);
                        REDUCE(R2);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Ult(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2; //avoid compiler warning
                        R3 = ULT(R1,R2);
                        REDUCE(R3);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_pUlt(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = ULT(R1,Param2);
                        REDUCE(R2);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Shl(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = (R1 << R2);
                        REDUCE(R3);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Shr(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = (R1 >> R2 );
                        REDUCE(R3);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Shra(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = SHRA(R1, R2);
                        REDUCE(R3);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Ishl(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R3 = (R1 << Param2);
                        REDUCE(R3);
                        )
    END_BATCH(BatchNumber);
}

static void InitKernel_Ishr(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R3 = (R1 >> Param2);
                        REDUCE(R3);
                      )

    END_BATCH(BatchNumber);
}

static void InitKernel_Ishra(int BatchNumber,INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R3 = ISHRA(R1, Param2);
                        REDUCE(R3);
                      )
    END_BATCH(BatchNumber);
}

static void InitKernel_Cellshl(int BatchNumber, INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = INDEX;
                        R2 = Param1;
                        CELL_SHL(R1,R2);
                        R3 = 0;
                        R5 = Param2;
                        NOP;
                        R4 = SHIFT_REG;
                        R6 = (R5 == R4);
                        NOP;
                      )
        EXECUTE_WHERE_EQ    (R3 = INDEX; )
        EXECUTE_IN_ALL      (REDUCE(R3););
    END_BATCH(BatchNumber);
}

static void InitKernel_Cellshr(int BatchNumber, INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = INDEX;
                        R2 = Param1;
                        CELL_SHR(R1,R2);
                        R3 = 0;
                        R5 = Param2;
                        NOP;
                        R4 = SHIFT_REG;
                        R6 = (R5 == R4);
                        NOP;
                        )
        EXECUTE_WHERE_EQ( R3 = INDEX;)
        EXECUTE_IN_ALL( REDUCE(R3);)
    END_BATCH(BatchNumber);
}

static void InitKernel_Multlo(int BatchNumber, INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = Param1;
                        R1 = Param2;
                        MULT = R1*R0;
                        R2 = _LO(MULT);
                        REDUCE(R2);
                    )
    END_BATCH(BatchNumber);
}

static void InitKernel_pMultlo(int BatchNumber, INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = Param1;
                        R1 = Param2;
                        R2 = R1*R0;
                        REDUCE(R2);
                    )
    END_BATCH(BatchNumber);
}

static void InitKernel_p2Multlo(int BatchNumber, INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = Param1;
                        R1 = R0 * Param2;
                        REDUCE(R1);
                    )
    END_BATCH(BatchNumber);
}

static void InitKernel_Multhi(int BatchNumber, INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = Param1;
                        R1 = Param2;
                        MULT = R1*R0;
                        R2 = _HI(MULT);
                        REDUCE(R2);
                        )
    END_BATCH(BatchNumber);
}

static void InitKernel_Whereq(int BatchNumber, INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = INDEX;
                        R1 = Param1;
                        R3 = (R0 == R1);
                        R4 = 0;
                    )
        EXECUTE_WHERE_EQ( R4 = Param2;)
        EXECUTE_IN_ALL( REDUCE(R4); )
    END_BATCH(BatchNumber);
}

static void InitKernel_Wherelt(int BatchNumber, INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = INDEX;
                        R1 = Param1;
                        R3 = (R0 < R1);
                        R4 = 0;
                      )
        EXECUTE_WHERE_LT( R4 = Param2;)
        EXECUTE_IN_ALL( REDUCE(R4);)
    END_BATCH(BatchNumber);
}

static void InitKernel_Wherecry(int BatchNumber, INT64 Param1, INT64 Param2)
{
    BEGIN_BATCH(BatchNumber);
	   EXECUTE_IN_ALL(
                        R0 = INDEX;
                        R1 = Param1;
                        R3 = R0 + R1;
                        R4 = 0;
                    )
        EXECUTE_WHERE_CARRY( R4 = Param2; )
        EXECUTE_IN_ALL( REDUCE(R4);)
    END_BATCH(BatchNumber);
}

/**
    Computes sum of first "numbers" consecutive nubers starting with start.
    Numbers are forced to UINT16.
    Sum is forced to UINT16 + log2(NUMBER_OF_MACHINES)

    Eg.
    SumRedofFirstXnumbers(1, 145) = 145
    SumRedofFirstXnumbers(2, 14) = 14 + 15 = 29
    SumRedofFirstXnumbers(1, 5) = 1+2+3+4+5 = 15

*/
static INT64 SumRedofFirstXnumbers(UINT64 numbers, UINT64 start)
{
    UINT32 x;
    UINT64 sum = 0;
    for (x = start; x < start + numbers; x++ ) sum += x;
    return sum & REDUCTION_SIZE_MASK;
}

/**
    Param 1: Number of vectors to be written.
    Param 2: Location in Local Store.
    Warning: Test assumes that InitKernel is called right before Iowrite test is executed
    ( io write occurs twice during the InitKernel evaluation, then Iowrite batch is ran)
*/
static void InitKernel_Iowrite(int BatchNumber,INT64 Param1, INT64 Param2)
{
    UINT16 destAddr = 0;
    UINT32 cnt;
    const int num_vectors = Param1;
    UINT16 data[NUMBER_OF_MACHINES*num_vectors];

    for (cnt = 0; cnt < NUMBER_OF_MACHINES*num_vectors; cnt++) data[cnt] = cnt;


    {
        io_unit IOU;
        IOU.preWriteVectors(destAddr,data,num_vectors);
        if (PASS != IO_WRITE_NOW(&IOU))
            printf("Writing to IO pipe, FAILED !");
        //c_simulator::printLS(destAddr);
    }

    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = LS[Param2];
                        REDUCE(R1);
                      )
    END_BATCH(BatchNumber);
}

/**

Param1 = Number of vectors to be read.
Param2 = Location in local store where we read from.

*/
static void InitKernel_Ioread(int BatchNumber,INT64 Param1, INT64 Param2)
{
    UINT16 destAddr = 0;
    UINT32 cnt;
    const int num_vectors = Param1;
    UINT16 data[NUMBER_OF_MACHINES*num_vectors];
    UINT16 testResult = PASS;

    // fill buffer with data to be written
    for (cnt = 0; cnt < NUMBER_OF_MACHINES*num_vectors; cnt++) data[cnt] = cnt;

    // write data to local store
    {
        io_unit IOU;
        IOU.preWriteVectors(destAddr,data,num_vectors);
        IO_WRITE_NOW(&IOU);
        //c_simulator::printLS(destAddr);
    }

    // read data from local store
    {
        io_unit IOU;
        IOU.preReadVectors(destAddr,num_vectors);
        IO_READ_NOW(&IOU);
        //c_simulator::printLS(destAddr);

        UINT16* Content = (UINT16*)(IOU.getIO_UNIT_CORE())->Content;
        for (cnt = 0; cnt < NUMBER_OF_MACHINES*num_vectors; cnt++)
            if (data[cnt] != Content[cnt])
            {
                /* Fail */
                BEGIN_BATCH(BatchNumber);
                EXECUTE_IN_ALL(
                                R2 = 0;
                                REDUCE(R2);
                               )
                END_BATCH(BatchNumber);
                testResult = FAIL;
            }

        if (testResult == PASS)
            {
                /* Pass */
                BEGIN_BATCH(BatchNumber);
                EXECUTE_IN_ALL(
                                R2 = 1;
                                REDUCE(R2);
                              )
                END_BATCH(BatchNumber);
                testResult = FAIL;
            }
    }

}

enum BatchNumbers
{
    NOP_BNR         ,
    IWRITE_BNR      ,
    IREAD_BNR       ,
    VLOAD_BNR       ,
    RED_BNR         ,
    MULT_BNR        ,
    CELL_SHL_BNR    ,
    CELL_SHR_BNR    ,
    WRITE_BNR       ,
    WHERE_CARRY_BNR ,
    WHERE_EQ_BNR    ,
    WHERE_LT_BNR    ,
    ENDWHERE_BNR    ,
    LDIX_BNR        ,
    READ_BNR        ,

    MULTLO_BNR      ,
    pMULTLO_BNR     ,
    p2MULTLO_BNR    ,

    LDSH_BNR        ,
    MULTHI_BNR      ,
    SHL_BNR         ,
    ISHL_BNR        ,

    ADD_BNR         ,
    pADD_BNR        ,
    sADD_BNR        ,

    EQ_BNR          ,
    pEQ_BNR         ,

    NOT_BNR         ,
    SHR_BNR         ,
    ISHR_BNR        ,

    SUB_BNR         ,
    pSUB_BNR        ,
    sSUB_BNR        ,

    LT_BNR          ,
    pLT_BNR         ,

    OR_BNR          ,
    pOR_BNR         ,
    sOR_BNR         ,

    SHRA_BNR        ,
    ISHRA_BNR       ,

    ADDC_BNR        ,
    pADDC_BNR       ,
    sADDC_BNR       ,

    ULT_BNR         ,
    pULT_BNR        ,

    AND_BNR         ,
    pAND_BNR        ,
    sAND_BNR        ,

    SUBC_BNR        ,
    pSUBC_BNR       ,

    XOR_BNR         ,
    pXOR_BNR        ,
    sXOR_BNR        ,

    IO_WRITE_BNR    ,
    IO_READ_BNR     ,

    PRINT_LS_BNR = 98,
    CLEAR_LS_BNR = 99,
    MAX_BNR = NUMBER_OF_BATCHES
};


// TODO with random numbers.
// Remember to handle truncation properly !
// eg: 128* 0xff ff ff ff = ???
TestFunction TestFunctionTable[] =
{
    {NOP_BNR,"NOP",0x00,0x00,InitKernel_Nop,NUMBER_OF_MACHINES},
    {IWRITE_BNR,"IWRITE",0x01,0x02,InitKernel_Iwrite,127*NUMBER_OF_MACHINES/2},
    {IREAD_BNR,"IREAD",0x01,0x02,InitKernel_Iread,127*NUMBER_OF_MACHINES/2},
    {WRITE_BNR,"WRITE",0x01,0x02,InitKernel_Write,127*NUMBER_OF_MACHINES/2},
    {READ_BNR,"READ",0x01,0x02,InitKernel_Read,127*NUMBER_OF_MACHINES/2},
    {VLOAD_BNR,"VLOAD",0x01,0x02,InitKernel_Vload,3*NUMBER_OF_MACHINES},

    {ADD_BNR,"ADD",0xff,0xf1,InitKernel_Add,(0xff + 0xf1)*NUMBER_OF_MACHINES},
    {pADD_BNR,"pADD",0xff,0xf1,InitKernel_pAdd,(0xff + 0xf1)*NUMBER_OF_MACHINES},
    {sADD_BNR,"sADD",0xff,0xf1,InitKernel_sAdd,(0xff + 0xf1)*NUMBER_OF_MACHINES},

    {ADDC_BNR,"ADDC",0xf0,0x1,InitKernel_Addc,(0xf0 + 1 + 1)*NUMBER_OF_MACHINES},
    {pADDC_BNR,"pADDC",0xf0,0x1,InitKernel_pAddc,(0xf0 + 1 + 1)*NUMBER_OF_MACHINES},

    {SUB_BNR,"SUB",0xffff,0xff8f,InitKernel_Sub, (0xffff - 0xff8f)*NUMBER_OF_MACHINES},
    {pSUB_BNR,"pSUB",0xffff,0xff8f,InitKernel_pSub, (0xffff - 0xff8f)*NUMBER_OF_MACHINES},
    {sSUB_BNR,"sSUB",0xffff,0xff8f,InitKernel_sSub, (0xffff - 0xff8f)*NUMBER_OF_MACHINES},

    {SUBC_BNR,"SUBC",0xffff,0xff8f,InitKernel_Subc,(0xffff - 0xff8f -1)*NUMBER_OF_MACHINES},
    {pSUBC_BNR,"pSUBC",0xffff,0xff8f,InitKernel_pSubc,(0xffff - 0xff8f -1)*NUMBER_OF_MACHINES},

    {NOT_BNR,"NOT",0xfff0,0x00,InitKernel_Not,(0xf)*NUMBER_OF_MACHINES},

    {OR_BNR,"OR",0x10,0x01,InitKernel_Or,(0x10 | 0x01)*NUMBER_OF_MACHINES},
    {pOR_BNR,"pOR",0x10,0x01,InitKernel_pOr,(0x10 | 0x01)*NUMBER_OF_MACHINES},
    {sOR_BNR,"sOR",0x10,0x01,InitKernel_sOr,(0x10 | 0x01)*NUMBER_OF_MACHINES},

    {AND_BNR,"AND",0xfffe,0x11,InitKernel_And,(0xfffe & 0x11)*NUMBER_OF_MACHINES},
    {pAND_BNR,"pAND",0xfffe,0x11,InitKernel_pAnd,(0xfffe & 0x11)*NUMBER_OF_MACHINES},
    {sAND_BNR,"sAND",0xfffe,0x11,InitKernel_sAnd,(0xfffe & 0x11)*NUMBER_OF_MACHINES},

    {XOR_BNR,"XOR",0x01,0x10,InitKernel_Xor,(0x01 ^ 0x10)*NUMBER_OF_MACHINES},
    {pXOR_BNR,"pXOR",0x01,0x10,InitKernel_pXor,(0x01 ^ 0x10)*NUMBER_OF_MACHINES},
    {sXOR_BNR,"sXOR",0x01,0x10,InitKernel_sXor,(0x01 ^ 0x10)*NUMBER_OF_MACHINES},

    {EQ_BNR,"EQ",0xff3f,0xff3f,InitKernel_Eq,(0xff3f == 0xff3f)*NUMBER_OF_MACHINES},
    {pEQ_BNR,"pEQ",0xff3f,0xff3f,InitKernel_pEq,(0xff3f == 0xff3f)*NUMBER_OF_MACHINES},

    {LT_BNR,"LT",0xfffd,0xfffe,InitKernel_Lt,(-3 < -2)*NUMBER_OF_MACHINES},
    {pLT_BNR,"pLT",0xfffd,0xfffe,InitKernel_pLt,(-3 < -2)*NUMBER_OF_MACHINES},

    {ULT_BNR,"ULT",0xabcd,0xabcc,InitKernel_Ult,(0xabcdUL < 0xabccUL)*NUMBER_OF_MACHINES},
    {pULT_BNR,"pULT",0xabcd,0xabcc,InitKernel_pUlt,(0xabcdUL < 0xabccUL)*NUMBER_OF_MACHINES},

    {SHL_BNR,"SHL",0xcd,3,InitKernel_Shl,((0xcd << 3)*NUMBER_OF_MACHINES)},
    {SHR_BNR,"SHR",0xabcd,3,InitKernel_Shr,((0xabcd >> 3)*NUMBER_OF_MACHINES)},
    {SHRA_BNR,"SHRA",0x01cd,4,InitKernel_Shra,(0x01c)*NUMBER_OF_MACHINES},//will fail: 128*big
    {ISHL_BNR,"ISHL",0xabcd,4,InitKernel_Ishl,((0xbcd0UL)*NUMBER_OF_MACHINES)},
    {ISHR_BNR,"ISHR",0xabcd,4,InitKernel_Ishr,((0x0abcUL)*NUMBER_OF_MACHINES)},
    {ISHRA_BNR,"ISHRA",0xabcd,4,InitKernel_Ishra,((0xfabcUL)*NUMBER_OF_MACHINES)},

    {MULTLO_BNR,"MULTLO",0x2,0x3,InitKernel_Multlo,(0x2UL * 0x3UL)*NUMBER_OF_MACHINES},
    {pMULTLO_BNR,"pMULTLO",0x2,0x3,InitKernel_pMultlo,(0x2UL * 0x3UL)*NUMBER_OF_MACHINES},
    {p2MULTLO_BNR,"p2MULTLO",0x2,0x3,InitKernel_p2Multlo,(0x2UL * 0x3UL)*NUMBER_OF_MACHINES},

    {MULTHI_BNR,"MULTHI",0x8000,0x2,InitKernel_Multhi,((0x8000UL * 0x2UL) >> 16)*NUMBER_OF_MACHINES},

    {WHERE_EQ_BNR,"WHEREQ",27,50,InitKernel_Whereq,50},
    {WHERE_LT_BNR,"WHERELT",27,50,InitKernel_Wherelt,27*50},
    {WHERE_CARRY_BNR,"WHERECRY",(0x10000UL-10),50,InitKernel_Wherecry,118*50},
	{CELL_SHL_BNR,"CELLSHL",2,5,InitKernel_Cellshl,5-2},
    {CELL_SHR_BNR,"CELLSHR",2,5,InitKernel_Cellshr,5+2},
	//{IO_WRITE_BNR,"IO_WRITE1",1,0,InitKernel_Iowrite,SumRedofFirstXnumbers(NUMBER_OF_MACHINES,0)},
    //{IO_WRITE_BNR,"IO_WRITE2",1024,1,InitKernel_Iowrite,SumRedofFirstXnumbers(NUMBER_OF_MACHINES,NUMBER_OF_MACHINES)},
    //{IO_WRITE_BNR,"IO_WRITE3",1024,1023,InitKernel_Iowrite,SumRedofFirstXnumbers(NUMBER_OF_MACHINES,NUMBER_OF_MACHINES*1023)},
    //{IO_READ_BNR,"IO_READ",1024,0,InitKernel_Ioread, NUMBER_OF_MACHINES},

};

static void simpleClearLS( )
{
    int BatchNumber = CLEAR_LS_BNR;
    int vector_index;
    for (vector_index = 0; vector_index < MAX_VECTORS; vector_index++)
    {
        BEGIN_BATCH(BatchNumber);
            EXECUTE_IN_ALL(
                            R0 = 0;
                            NOP;
                            LS[vector_index] = R0;
                      );
        END_BATCH(BatchNumber);
        EXECUTE_KERNEL(BatchNumber);
    }
}
static void simplePrintLS(INT64 Param2)
{
    int BatchNumber = PRINT_LS_BNR;
    int cell_index;
    int result;
    for (cell_index = 0; cell_index < NUMBER_OF_MACHINES; cell_index++)
    {
        BEGIN_BATCH(BatchNumber);
            EXECUTE_IN_ALL(
                        R4 = 0;
                        R1 = INDEX;
                        R2 = cell_index;
                        R3 = (R1 == R2);
                      );
            EXECUTE_WHERE_EQ(
                        R4 = LS[Param2];
            )
            EXECUTE_IN_ALL(
                        REDUCE(R4);
            )
        END_BATCH(BatchNumber);
        result = EXECUTE_KERNEL_RED(BatchNumber);
        if ((cell_index & 1) == 0) cout<< endl;
        cout<<"Machine "<< cell_index <<" : LS["<<Param2<<"] = "<<result<<"    ";

    }
    cout<< endl;
}

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
           cout<< "Test "<< setw(8) << left << TestFunctionTable[i].OperationName <<" FAILED with result "
           <<result << " (expected " <<TestFunctionTable[i].ExpectedResult<<" ) !"<<endl;
           testFails++;
        }
        else
            //printf("Test %s     passed ! \n", TestFunctionTable[i].OperationName);
            cout<< "Test "<< setw(8) << left << TestFunctionTable[i].OperationName << " passed. " <<endl;
    }


    //DEASM_KERNEL(WHERE_CARRY_BNR);
    simpleClearLS();
    simplePrintLS(0);
    InitKernel_Iowrite(IO_WRITE_BNR,1,0);
    cout <<"Result of Iowrite test " << EXECUTE_KERNEL_RED(IO_WRITE_BNR)<<endl;
    simplePrintLS(0);
    //simplePrintLS(1);
    //simplePrintLS(2);

    if (testFails ==0)
        cout<<endl<< " All SimpleTests PASSED." <<endl;
    else
        cout<< testFails << " SimpleTests failed." <<endl;

    cout<< "LocalStore[0] in all machines must be INDEX. Check this !"<<endl;

    return testFails;
}

