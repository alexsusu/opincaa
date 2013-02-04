/*
 *
 * File: simple_io_tests.cpp
 *
 * Simple io tests for io unit of connex machine.
 *
 */
#include "../../include/core/cnxvector_registers.h"
#include "../../include/core/cnxvector.h"
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

static UINT16 data[NUMBER_OF_MACHINES * MAX_CNXVECTORS];

/**
    Param 1: Number of cnxvectors to be written.
    Param 2: Location in Local Store.
    Warning: Test assumes that InitKernel is called right before Iowrite test is executed

*/
static int testIowrite(int BatchNumber,INT32 Param1, INT32 Param2)
{
    UINT32 cnt;
    const int num_cnxvectors = Param1;
    UINT16 cnxvectorIndex;
    UINT16 testResult;

    for (cnt = 0; cnt < NUMBER_OF_MACHINES*num_cnxvectors; cnt++) data[cnt] = cnt;
    {
        io_unit IOU;
        IOU.preWritecnxvectors(Param2,data,num_cnxvectors);
        if (PASS != IO_WRITE_NOW(&IOU))
        {
            printf("Writing to IO pipe, FAILED !");
            return FAIL;
            //c_simulator::printLS(Param2);
        }
    }
    testResult = PASS;

    //printf("Reading IO-wrote data, via Regs \n");
    for (cnxvectorIndex = 0; cnxvectorIndex < num_cnxvectors; cnxvectorIndex++)
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
                EXECUTE_WHERE_EQ ( R3 = LS[cnxvectorIndex + Param2];)
                EXECUTE_IN_ALL ( REDUCE(R3);)
            END_BATCH(BatchNumber);

            //DEASM_KERNEL(BatchNumber);
            if (data[cnxvectorIndex * CNXVECTOR_SIZE_IN_WORDS + cnt] != EXECUTE_BATCH_RED(BatchNumber))
            {
                testResult = FAIL;
                //cout<<cnxvectorIndex << " "<<cnt << " "<< EXECUTE_KERNEL_RED(BatchNumber)<< " "<<data[cnxvectorIndex * CNXVECTOR_SIZE_IN_WORDS + cnt]<<endl;
            }
        }
   return  testResult;

}

/**

Param1 = Number of cnxvectors to be read.
Param2 = Location in local store where we read from.

*/
static int testIoread(int BatchNumber,INT32 Param1, INT32 Param2)
{
    UINT32 cnt;
    UINT16 cnxvectorIndex;
    const int num_cnxvectors = Param1;
//    UINT16 data[NUMBER_OF_MACHINES*num_cnxvectors];
    UINT16 testResult;

    // fill buffer with data to be written
    for (cnt = 0; cnt < NUMBER_OF_MACHINES*num_cnxvectors; cnt++) data[cnt] = cnt;

    for (cnxvectorIndex = 0; cnxvectorIndex < num_cnxvectors; cnxvectorIndex++)
        for (cnt = 0; cnt < NUMBER_OF_MACHINES; cnt++)
        // write data to local store
        {
            BEGIN_BATCH(BatchNumber);
                EXECUTE_IN_ALL(
                                R1 = INDEX;
                                R2 = cnt;
                                R4 = (R1 == R2);
                                R3 = data[cnxvectorIndex * CNXVECTOR_SIZE_IN_WORDS + cnt];// ensured 1 slot between == and where_eq !
                              )
                EXECUTE_WHERE_EQ ( LS[cnxvectorIndex + Param2] = R3;)
            END_BATCH(BatchNumber);
            EXECUTE_BATCH(BatchNumber);
        }
        //c_simulator::printLS(Param2+1);
    // read data from local store
    {
        io_unit IOU;
        IOU.preReadcnxvectors(Param2,num_cnxvectors);
        IO_READ_NOW(&IOU);
        //c_simulator::printLS(Param2);

        UINT16* Content = (UINT16*)(IOU.getIO_UNIT_CORE())->Content;
        testResult = PASS;
        for (cnxvectorIndex = 0; cnxvectorIndex < num_cnxvectors; cnxvectorIndex++)
            for (cnt = 0; cnt < NUMBER_OF_MACHINES; cnt++)
                if (data[cnxvectorIndex * CNXVECTOR_SIZE_IN_WORDS + cnt] != Content[cnxvectorIndex * CNXVECTOR_SIZE_IN_WORDS + cnt])
                {
                    //cout<<cnxvectorIndex << " "<<cnt << " "<< " "<<data[cnxvectorIndex * CNXVECTOR_SIZE_IN_WORDS + cnt]<<endl;
                    testResult = FAIL;
                }
    }

    return testResult;
}

struct Dataset
{
   INT32 Param1;
   INT32 Param2;
};

struct TestIoFunction
{
   int BatchNumber;
   const char *TestName;
   int (*runTest)(int BatchNumber,INT32 Param1,INT32 Param2);
   Dataset ds;
};

TestIoFunction TestIoFunctionTable[] =
{
    {IO_WRITE1_BNR,"IO_WRITE_1.0   ",testIowrite,{1,0}},
    {IO_READ1_BNR, "IO_READ_1.0    ",testIoread,{1,0}},

    {IO_WRITE2_BNR,"IO_WRITE_2.1   ",testIowrite,{2,1}},
    {IO_READ2_BNR, "IO_READ_2.1    ",testIoread,{2,1}},

    {IO_WRITE3_BNR,"IO_WRITE_3.1   ",testIowrite,{3,1}},
    {IO_READ3_BNR, "IO_READ_3.1    ",testIoread,{3,1}},

    {IO_WRITE4_BNR,"IO_WRITE_1024.0",testIowrite,{MAX_CNXVECTORS,0}},
    {IO_READ4_BNR, "IO_READ_1024.0 ",testIoread,{MAX_CNXVECTORS,0}},
    {IO_WRITE4_BNR,"IO_WRITE_1024.0",testIowrite,{MAX_CNXVECTORS,0}},
};

static int getIndexTestIoFunctionTable(int BatchNumber)
{
    unsigned int i;
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
        case IO_READ1_BNR      :{TestIoFunctionTable[i].ds.Param2 = randPar(MAX_CNXVECTORS-1);break;}

        case IO_WRITE2_BNR:
        case IO_READ2_BNR      :{TestIoFunctionTable[i].ds.Param2 = randPar(MAX_CNXVECTORS-2);break;}

        case IO_WRITE3_BNR:
        case IO_READ3_BNR      :{TestIoFunctionTable[i].ds.Param2 = randPar(MAX_CNXVECTORS-3);break;}

        case IO_WRITE4_BNR:
        case IO_READ4_BNR      :{
                                    TestIoFunctionTable[i].ds.Param2 = randPar(MAX_CNXVECTORS);
                                    do
                                    {
                                        TestIoFunctionTable[i].ds.Param1 = randPar(MAX_CNXVECTORS-1)+1;
                                    }while (TestIoFunctionTable[i].ds.Param2 + TestIoFunctionTable[i].ds.Param1 > MAX_CNXVECTORS);
									//cout<<endl<<"  IO test running with params "<< TestIoFunctionTable[i].ds.Param1 << " " << TestIoFunctionTable[i].ds.Param2;
					break;
                                }
    }
}
int test_Simple_IO_All(bool stress)
{
    //simpleClearLS();
    int testFails = 0;
    int stressLoops;
    unsigned int i;
    int j;
    cout<<endl;

    if (stress == true) { stressLoops = 10;} else stressLoops = 0;
    for (i = 0; i < sizeof (TestIoFunctionTable) / sizeof (TestIoFunction); i++)
    {
        j = stressLoops;
        do
        {
            //simpleClearLS();
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


        cout<<"====================================="<<endl;

    if (testFails ==0)
        cout<< "== All SimpleIOTests PASSED ========" <<endl;
    else
        cout<< "=="<< testFails << " SimpleIOTests FAILED ! " <<endl;

        cout<<"====================================="<<endl<<endl;

    return testFails;

    //simplePrintLS(0);
    //simplePrintLS(1);
    //simplePrintLS(2);

    //return testFails;
}

