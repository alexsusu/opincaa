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
    UINT16 data[NUMBER_OF_MACHINES*num_vectors];

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
                                R3 = data[cnt];// ensured 1 slot between == and where_eq !
                              )
                EXECUTE_WHERE_EQ ( LS[VectorIndex] = R3;)
            END_BATCH(BatchNumber);
            EXECUTE_KERNEL(BatchNumber);
        }

    // read data from local store
    {
        io_unit IOU;
        IOU.preReadVectors(Param2,num_vectors);
        IO_READ_NOW(&IOU);
        //c_simulator::printLS(Param2);

        UINT16* Content = (UINT16*)(IOU.getIO_UNIT_CORE())->Content;
        testResult = PASS;
        for (cnt = 0; cnt < NUMBER_OF_MACHINES*num_vectors; cnt++)
            if (data[cnt] != Content[cnt])
                testResult = FAIL;
    }

    return testResult;
}

enum BatchNumbers
{
    IO_WRITE_BNR    ,
    IO_READ_BNR     ,

    PRINT_LS_BNR = 98,
    CLEAR_LS_BNR = 99,
    MAX_BNR = NUMBER_OF_BATCHES
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
        if ((cell_index & 2) == 0) cout<< endl;
        cout<<"Mach "<< cell_index <<" : LS["<<Param2<<"] = "<<result<<"    ";
    }
    cout<< endl;
}


int test_Simple_IO_All()
{
    //simpleClearLS();
    int testFails = 0;
    cout<<endl;
    if (PASS != testIowrite(IO_WRITE_BNR,1,0))
    {
        testFails++;
        cout<< " testIowrite FAILED." <<endl;
        simplePrintLS(0);
    }
    else cout<< " testIowrite passed." <<endl;

    if (PASS != testIoread(IO_READ_BNR,1,0))
    {
        testFails++;
        cout<< " testIoread FAILED." <<endl;
    }
    else cout<< " testIoread passed." <<endl;

    if (testFails ==0)
        cout<<endl<< " All SimpleIOTests PASSED." <<endl;
    else
        cout<< testFails << " SimpleIOTests failed." <<endl;

    return testFails;

    //simplePrintLS(0);
    //simplePrintLS(1);
    //simplePrintLS(2);

    //return testFails;
}

