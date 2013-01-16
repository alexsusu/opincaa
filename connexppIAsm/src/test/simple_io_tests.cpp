/*
 *
 * File: simple_io_tests.cpp
 *
 * Simple io tests for io unit of connex machine.
 *
 */
#include "../../include/core/vector_registers.h"
#include "../../include/core/vector.h"
#include "../../include/core/io_unit.h"
#include "../../include/c_simu/c_simulator.h"
#include "../../include/util/utils.h"

#include <time.h>
#include <iostream>
#include <iomanip>
using namespace std;


enum SimpleIoBatchNumbers
{
    IO_READ1_BNR    ,
    IO_READ2_BNR    ,
    IO_READ3_BNR    ,
    IO_READ4_BNR    ,
    IO_WRITE1_BNR   ,
    IO_WRITE2_BNR   ,
    IO_WRITE3_BNR   ,
    IO_WRITE4_BNR   ,

    PRINT_LS_BNR = 98,
    CLEAR_LS_BNR = 99,
    MAX_BNR = NUMBER_OF_BATCHES
};

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

*/
static int testIowrite(int BatchNumber,INT64 Param1, INT64 Param2)
{
    UINT32 cnt;
    const int num_vectors = Param1;
    UINT16 VectorIndex;
    UINT16 data[NUMBER_OF_MACHINES*num_vectors];
    UINT16 testResult;

    for (cnt = 0; cnt < NUMBER_OF_MACHINES*num_vectors; cnt++) data[cnt] = cnt;


    {
        io_unit IOU;
        IOU.preWriteVectors(Param2,data,num_vectors);
        if (PASS != IO_WRITE_NOW(&IOU))
        {
            printf("Writing to IO pipe, FAILED !");
            return FAIL;
            //c_simulator::printLS(Param2);
        }
    }
    testResult = PASS;

    //printf("Reading IO-wrote data, via Regs \n");
    for (VectorIndex = 0; VectorIndex < num_vectors; VectorIndex++)
        for (cnt = 0; cnt < NUMBER_OF_MACHINES; cnt++)
        // write data to local store
        {
            BEGIN_BATCH(BatchNumber);
                EXECUTE_IN_ALL(
                                R1 = INDEX;
                                R2 = cnt;
                                R3 = 0;
                                R4 = (R1 == R2);
                                NOP;
                              )
                EXECUTE_WHERE_EQ ( 
                                NOP;
                                R3 = LS[VectorIndex + Param2];
                                NOP;
                )
                EXECUTE_IN_ALL ( REDUCE(R3);)
            END_BATCH(BatchNumber);

            if (data[VectorIndex * VECTOR_SIZE_IN_WORDS + cnt] != EXECUTE_KERNEL_RED(BatchNumber))
            {
                testResult = FAIL;
                //cout<<VectorIndex << " "<<cnt << " "<< EXECUTE_KERNEL_RED(BatchNumber)<< " "<<data[VectorIndex * VECTOR_SIZE_IN_WORDS + cnt]<<endl;
            }
        }
   return  testResult;

}

/**

Param1 = Number of vectors to be read.
Param2 = Location in local store where we read from.

*/
static int testIoread(int BatchNumber,INT64 Param1, INT64 Param2)
{
    UINT32 cnt;
    UINT16 VectorIndex;
    const int num_vectors = Param1;
    UINT16 data[NUMBER_OF_MACHINES*num_vectors];
    UINT16 testResult;

    // fill buffer with data to be written
    for (cnt = 0; cnt < NUMBER_OF_MACHINES*num_vectors; cnt++) data[cnt] = cnt;

    for (VectorIndex = 0; VectorIndex < num_vectors; VectorIndex++)
        for (cnt = 0; cnt < NUMBER_OF_MACHINES; cnt++)
        // write data to local store
        {
            BEGIN_BATCH(BatchNumber);
                EXECUTE_IN_ALL(
                                R1 = INDEX;
                                R2 = cnt;
                                R4 = (R1 == R2);
                                R3 = data[VectorIndex * VECTOR_SIZE_IN_WORDS + cnt];// ensured 1 slot between == and where_eq !
                              )
                EXECUTE_WHERE_EQ ( LS[VectorIndex + Param2] = R3;)
            END_BATCH(BatchNumber);
            EXECUTE_KERNEL(BatchNumber);
        }
        //c_simulator::printLS(Param2+1);
    // read data from local store
    {
        io_unit IOU;
        IOU.preReadVectors(Param2,num_vectors);
        IO_READ_NOW(&IOU);
        //c_simulator::printLS(Param2);

        UINT16* Content = (UINT16*)(IOU.getIO_UNIT_CORE())->Content;
        testResult = PASS;
        for (VectorIndex = 0; VectorIndex < num_vectors; VectorIndex++)
            for (cnt = 0; cnt < NUMBER_OF_MACHINES; cnt++)
                if (data[VectorIndex * VECTOR_SIZE_IN_WORDS + cnt] != Content[VectorIndex * VECTOR_SIZE_IN_WORDS + cnt])
                {
                    //cout<<VectorIndex << " "<<cnt << " "<< " "<<data[VectorIndex * VECTOR_SIZE_IN_WORDS + cnt]<<endl;
                    testResult = FAIL;
                }
    }

    return testResult;
}

static void simpleClearLS( )
{
    int BatchNumber = CLEAR_LS_BNR;
    int vector_index;
    
    //printf("Running simpleClearLS \n");
    for (vector_index = 0; vector_index < MAX_VECTORS; vector_index++)
    {
        BEGIN_BATCH(BatchNumber);
            EXECUTE_IN_ALL(
                                R0 = 0xffff;
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
        if ((cell_index & 2) == 0) cout<< endl;
        cout<<"Mach "<< cell_index <<" : LS["<<Param2<<"] = "<<result<<"    ";
    }
    cout<< endl;
}

struct Dataset
{
   INT64 Param1;
   INT64 Param2;
};

struct TestIoFunction
{
   int BatchNumber;
   const char *TestName;
   int (*runTest)(int BatchNumber,INT64 Param1,INT64 Param2);
   Dataset ds;
};

TestIoFunction TestIoFunctionTable[] =
{
    {IO_READ1_BNR, "IO_READ_1.0    ",testIoread,{1,0}},
    {IO_READ2_BNR, "IO_READ_2.1    ",testIoread,{2,1}},
    {IO_READ3_BNR, "IO_READ_3.1    ",testIoread,{3,1}},
    {IO_READ4_BNR, "IO_READ_1024.0 ",testIoread,{MAX_VECTORS,0}},

    {IO_WRITE1_BNR,"IO_WRITE_1.0   ",testIowrite,{1,0}},
    {IO_WRITE2_BNR,"IO_WRITE_2.1   ",testIowrite,{2,1}},
    {IO_WRITE3_BNR,"IO_WRITE_3.1   ",testIowrite,{3,1}},
    {IO_WRITE4_BNR,"IO_WRITE_1024.0",testIowrite,{MAX_VECTORS,0}},
};

static int getIndexTestIoFunctionTable(int BatchNumber)
{
    int i;
    for (i = 0; i < sizeof(TestIoFunctionTable)/sizeof(TestIoFunction); i++)
        if (TestIoFunctionTable[i].BatchNumber == BatchNumber)
            return i;
    return -1;
}

static void UpdateDatasetTable(int BatchNumber)
{
    int i = getIndexTestIoFunctionTable(BatchNumber);
    if (i>=0)
    switch(BatchNumber)
    {
        case IO_WRITE1_BNR:
        case IO_READ1_BNR      :{TestIoFunctionTable[i].ds.Param2 = randPar(MAX_VECTORS-1);break;}

        case IO_WRITE2_BNR:
        case IO_READ2_BNR      :{TestIoFunctionTable[i].ds.Param2 = randPar(MAX_VECTORS-2);break;}

        case IO_WRITE3_BNR:
        case IO_READ3_BNR      :{TestIoFunctionTable[i].ds.Param2 = randPar(MAX_VECTORS-3);break;}

        case IO_WRITE4_BNR:
        case IO_READ4_BNR      :{
                                    TestIoFunctionTable[i].ds.Param2 = randPar(MAX_VECTORS);
                                    do
                                    {
                                        TestIoFunctionTable[i].ds.Param1 = randPar(MAX_VECTORS-1)+1;
                                    }while (TestIoFunctionTable[i].ds.Param2 + TestIoFunctionTable[i].ds.Param1 > MAX_VECTORS);
//TestIoFunctionTable[i].ds.Param1 = 970;
//TestIoFunctionTable[i].ds.Param2 = 23;                                    
//cout<<endl<<"  IO test running with params "<< TestIoFunctionTable[i].ds.Param1 << " " << TestIoFunctionTable[i].ds.Param2;
					break;
                                }
    }
}
int test_Simple_IO_All(bool stress)
{
    int testFails = 0;
    int stressLoops;
    int i,j;
    cout<<endl;

    if (stress == true) { stressLoops = 10;} else stressLoops = 0;
    for (i = 0; i < sizeof (TestIoFunctionTable) / sizeof (TestIoFunction); i++)
    {
        j = stressLoops;
        do
        {
            simpleClearLS();
            if (PASS != TestIoFunctionTable[i].runTest(TestIoFunctionTable[i].BatchNumber,
                                                            TestIoFunctionTable[i].ds.Param1,
                                                            TestIoFunctionTable[i].ds.Param2))
            {
               cout<< "Test "<< setw(8) << left << TestIoFunctionTable[i].TestName <<" FAILED ! params are "
               << TestIoFunctionTable[i].ds.Param1 << " and " << TestIoFunctionTable[i].ds.Param2 <<endl;
               testFails++;
               return testFails;
            }
            else
            {
                if (j == stressLoops)
                    cout<< "Test "<< setw(8) << left << TestIoFunctionTable[i].TestName;
                if ((j > 0) && (j <= stressLoops)) cout<<".";
                if (j == 0) {cout << " PASSED"<<endl;break;}
            }
            UpdateDatasetTable(TestIoFunctionTable[i].BatchNumber);
        }
        while (j-- >= 0);
    }

    if (testFails ==0)
        cout<<endl<< " All SimpleIOTests PASSED." <<endl;
    else
        cout<< testFails << " SimpleIOTests failed." <<endl;

    return testFails;

}

