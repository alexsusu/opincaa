/*
 *
 * File: speed_tests.cpp
 *
 * Speed tests for some instructions on connex machine.
 *
 */
#include "../../include/core/vector_registers.h"
#include "../../include/core/vector.h"
#include "../../include/core/io_unit.h"
#include "../../include/c_simu/c_simulator.h"
#include "../../include/util/utils.h"
#include "../../include/util/timing.h"

#include <time.h>
#include <iostream>
#include <iomanip>
using namespace std;
//STATIC_VECTOR_DEFINITIONS;

struct SpeedTestFunction
{
   int BatchNumber;
   const char *OperationName;
   INT64 Param1;
   INT64 Param2;
   void (*prepareTest)(int BatchNumber);
   int (*runTest)(int BatchNumber,INT64 Param1,INT64 Param2);

   INT64 ExpectedResult;
};

void dummy(int BatchNumber) {}

/**

Param1 = Number of vectors to be read.
Param2 = Location in local store where we read from.

*/
int BenchmarkIoRWspeed(int BatchNumber,INT64 Param1, INT64 Param2)
{
    UINT16 destAddr = 0;
    UINT32 cnt;
    const int num_vectors = Param1;
    UINT16 data[NUMBER_OF_MACHINES*num_vectors];
    UINT16 testResult = PASS;

    clock_t tstart,tend;
    //double dif;
    UINT32 cycles;


    // fill buffer with data to be written
    for (cnt = 0; cnt < NUMBER_OF_MACHINES*num_vectors; cnt++) data[cnt] = cnt;


    // write data to local store
    {
        io_unit IOU;
        tstart = clock();
            IOU.preWriteVectors(destAddr,data,num_vectors);
        tend = clock();
        printf ("\nElasped time for write precache is %ld ms.\n", 1000 * (tend-tstart)/ CLOCKS_PER_SEC );

        tstart = clock();
        for (cycles = 0; cycles < Param2; cycles++)
            IO_WRITE_NOW(&IOU);
        tend = clock();
    }

    printf ("Elasped write time is %ld ms.\n", 1000 * (tend-tstart)/CLOCKS_PER_SEC );

    if (tend != tstart)
    {
        printf ("Approx datarate is %ld vectors/s\n", (long int)(num_vectors * Param2 * CLOCKS_PER_SEC / (tend-tstart))); // ~4 Bytes per IO entry
        printf ("Approx datarate is %ld KB/s\n", (long int)(num_vectors* VECTOR_SIZE_IN_BYTES * Param2 * CLOCKS_PER_SEC / 1000 / (tend-tstart))); // ~4 Bytes per IO entry
        printf ("Approx delta time is %ld ms \n", 1000 * (tend-tstart)/CLOCKS_PER_SEC);
    }
    else
        printf ("Way too fast to perform measurement \n");

    // read data from local store
    {
        io_unit IOU;
        tstart = clock();
            IOU.preReadVectors(destAddr,num_vectors);
        tend = clock();
        printf ("Elasped time for read precache is %ld ms.\n", 1000 * ((tend-tstart) / CLOCKS_PER_SEC ));

        tstart = clock();
        for (cycles = 0; cycles < Param2; cycles++)
            IO_READ_NOW(&IOU);
        tend = clock();

        printf ("Elasped read time is %ld ms.\n", 1000 * (tend-tstart)/CLOCKS_PER_SEC );

        if (tend != tstart)
            printf ("Approx datarate is %ld KB/s\n", (long int)((NUMBER_OF_MACHINES * 1024 * 4) * Param2 * CLOCKS_PER_SEC / 1000 / ((tend-tstart)))); // ~4 Bytes per IO entry
        else
            printf ("Way too fast to perform measurement \n");


        /* Check correctness of last read */
        UINT16* Content = (UINT16*)(IOU.getIO_UNIT_CORE())->Content;
        for (cnt = 0; cnt < NUMBER_OF_MACHINES*num_vectors; cnt++)
            if (data[cnt] != Content[cnt])
            {
                testResult = FAIL;
                printf ("ERROR*: Last readout did not match the IO data written !\n");
                printf ("ERROR is at index %d \n", cnt);
                printf ("ERROR Expected data is %d \n", data[cnt]);
                printf ("ERROR Read data is %d \n", Content[cnt]);
                if (cnt>0)
                    printf ("ERROR Left neighbour is at index %d and has value of %d (expected %d)\n", cnt-1, Content[cnt-1], data[cnt-1]);
                if (cnt != NUMBER_OF_MACHINES*num_vectors-1)
                    printf ("ERROR Right neighbour is at index %d and has value of %d (expected %d)\n", cnt+1, Content[cnt+1], data[cnt+1]);
                break;
            }
        return testResult;
    }
}

#define NOP_SPEED_CYCLES (1000*1000*10)
void InitBatchNop(int BatchNumber)
{
    UINT32 cycles;
    BEGIN_BATCH(BatchNumber);
    EXECUTE_IN_ALL(
                for (cycles = 0; cycles < NOP_SPEED_CYCLES; cycles++)
                {
                    NOP;
                }
              )
    END_BATCH(BatchNumber);
}

int BenchmarkNOPspeed(int BatchNumber,INT64 Param1, INT64 Param2)
{
    clock_t tstart,tend;
    int start_time = GetMilliCount();

    tstart = clock();
        EXECUTE_KERNEL(BatchNumber);
    tend = clock();
    int delta_time = GetMilliSpan(start_time);

    if (tend != tstart)
    {
        printf ("Approx performance is %ld KIPS\n", NOP_SPEED_CYCLES / 1000 * CLOCKS_PER_SEC/ (tend-tstart));
        printf ("Approx delta time is %ld ms \n", 1000 * (tend-tstart)/CLOCKS_PER_SEC);
    }
    else printf ("Way too fast to perform measurement \n");
    return PASS;
}

#define ADD_SPEED_CYCLES (100*1000)
void InitBatchAdd(int BatchNumber)
{
    UINT32 cycles;
    BEGIN_BATCH(BatchNumber);
    EXECUTE_IN_ALL(
                for (cycles = 0; cycles < ADD_SPEED_CYCLES/10; cycles++)
                {
                    R0 = R1 + R2;
                    R3 = R4 + R5;
                    R6 = R7 + R8;
                    R9 = R10 + R11;
                    R12 = R13 + R14;

                    R15 = R16 + R17;
                    R18 = R19 + R20;
                    R21 = R22 + R23;
                    R24 = R25 + R26;
                    R27 = R28 + R29;
                }
              )
    END_BATCH(BatchNumber);
}

int BenchmarkADDspeed(int BatchNumber,INT64 Param1, INT64 Param2)
{
    clock_t tstart,tend;

    tstart = clock();
        EXECUTE_KERNEL(BatchNumber);
    tend = clock();

    if (tend != tstart)
    {
        printf ("Approx performance is %ld KIPS\n", ADD_SPEED_CYCLES / 1000 * CLOCKS_PER_SEC/ (tend-tstart));
        printf ("Approx delta time is %ld ms \n", 1000 * (tend-tstart)/CLOCKS_PER_SEC);
    }
    else printf ("Way too fast to perform measurement \n");
    return PASS;
}

#define MLT_SPEED_CYCLES (100*1000)
void InitBatchMlt(int BatchNumber)
{
    UINT32 cycles;
    BEGIN_BATCH(BatchNumber);
    EXECUTE_IN_ALL(
                for (cycles = 0; cycles < MLT_SPEED_CYCLES/3; cycles++)
                {
                    MULT = R0 * R1;
                    R2 = _LO(MULT);
                    R3 = _HI(MULT);
                }
              )
    END_BATCH(BatchNumber);
}

int BenchmarkMLTspeed(int BatchNumber,INT64 Param1, INT64 Param2)
{
    clock_t tstart,tend;

    tstart = clock();
        EXECUTE_KERNEL(BatchNumber);
    tend = clock();

    if (tend != tstart)
    {
        printf ("Approx performance is %ld KIPS\n", MLT_SPEED_CYCLES/ 1000 * CLOCKS_PER_SEC / (tend-tstart));
        printf ("Approx delta time is %ld ms \n", 1000 * (tend-tstart)/CLOCKS_PER_SEC);
    }
    else printf ("Way too fast to perform measurement \n");
    return PASS;
}

enum BatchNumbers
{
    NOP_SPEED_BNR = 0,
    ADD_SPEED_BNR = 1,
    MLT_SPEED_BNR = 2,
    IO_RW_1_SPEED_BNR = 3,
    IO_RW_2_SPEED_BNR = 4,
    IO_RW_100_SPEED_BNR = 5,

    MAX_BNR = NUMBER_OF_BATCHES
};


// TODO with random numbers.
// Remember to handle truncation properly !
// eg: 128* 0xff ff ff ff = ???
SpeedTestFunction SpeedTestFunctionTable[] =
{
    {NOP_SPEED_BNR,"NOP_SPEED",0,0,InitBatchNop,BenchmarkNOPspeed,PASS},
    {ADD_SPEED_BNR,"ADD_SPEED",0,0,InitBatchAdd,BenchmarkADDspeed,PASS},
    {MLT_SPEED_BNR,"MLT_SPEED",0,0,InitBatchMlt,BenchmarkMLTspeed,PASS},
    {IO_RW_1_SPEED_BNR,"IO_RW_1_SPEED",1024,1,dummy,BenchmarkIoRWspeed,PASS},
    //{IO_RW_2_SPEED_BNR,"IO_RW_2_SPEED",1024,2,dummy,BenchmarkIoRWspeed,PASS},
    //{IO_RW_100_SPEED_BNR,"IO_RW_100_SPEED",1024,100,dummy,BenchmarkIoRWspeed,PASS}

};

int test_Speed_All()
{
    UINT16 i = 0;
    INT64 result;
    UINT16 testFails = 0;

    cout<<endl<< "Starting speed tests:"<<endl<<endl;
    for (i = 0; i < sizeof (SpeedTestFunctionTable) / sizeof (SpeedTestFunction); i++)
    {
        cout<<endl<< "Running " << SpeedTestFunctionTable[i].OperationName<<": "<<endl;
        SpeedTestFunctionTable[i].prepareTest(SpeedTestFunctionTable[i].BatchNumber);
        result = SpeedTestFunctionTable[i].runTest(SpeedTestFunctionTable[i].BatchNumber, SpeedTestFunctionTable[i].Param1, SpeedTestFunctionTable[i].Param2 );
        if (result != SpeedTestFunctionTable[i].ExpectedResult)
        {
           cout<< "Test "<< setw(8) << left << SpeedTestFunctionTable[i].OperationName <<" FAILED with result "
           <<result << " (expected " <<SpeedTestFunctionTable[i].ExpectedResult<<" ) !"<<endl;
           testFails++;
        }
        else
            //printf("Test %s     passed ! \n", SpeedTestFunctionTable[i].OperationName);
            cout<< "Test "<< setw(8) << left << SpeedTestFunctionTable[i].OperationName << " passed." <<endl;
    }

//    DEASM_KERNEL(WHERE_LT_BNR);
//    DEASM_KERNEL(WHERE_CARRY_BNR);

    return testFails;
}

