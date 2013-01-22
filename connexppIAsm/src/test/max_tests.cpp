/*
 *
 * File: max_tests.cpp
 *
 * Tests implementation of MAX routine.
 *
 *
 */
#include "../../include/core/cnxvector_registers.h"
#include "../../include/core/cnxvector.h"
#include "../../include/core/io_unit.h"
#include "../../include/c_simu/c_simulator.h"
#include "../../include/util/utils.h"

#include <iostream>
#include <iomanip>
using namespace std;

static void InitKernel_Max(int BatchNumber)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = LS[0];
                        R1 = LS[0];
                        R2 = 1;
                        R3 = INDEX;
                        for (int x=0;x < NUMBER_OF_MACHINES - 1;x++)
                        //for (int x=0; x < 7; x++)
                        {
                            CELL_SHL(R1,R2);//rotate shift reg containing r1, with r2 positions
                            NOP;
                            NOP;
                            R1 = SHIFT_REG;
                            R4 = ULT(R0,R1);
                            NOP;
                            EXECUTE_WHERE_LT(R3 = 0;);
                            ENABLE_ALL
                        }
                    )
        EXECUTE_IN_ALL(
                        REDUCE(R3);
                      )
    END_BATCH(BatchNumber);
}


int test_Max()
{
    UINT16 DataValues[NUMBER_OF_MACHINES];
    int max_val = 0;
    int max_i = 0;
    for (int i = 0; i < NUMBER_OF_MACHINES; i++)
    {
        DataValues[i] = randPar(0x10000);
        if (DataValues[i] > max_val)
        {
            max_val = DataValues[i];
            max_i = i;
        }
    }

    io_unit IOU;
    IOU.preWritecnxvectors(0,DataValues,1);
    if (PASS != IO_WRITE_NOW(&IOU))
    {
        printf("Writing to IO pipe, FAILED !");
        return FAIL;
    }
    InitKernel_Max(0);
    int max_pos = EXECUTE_BATCH_RED(0);
    if (max_pos == max_i)
    {
        //cout <<" Max test PASSED."<<endl;
        return PASS;
    }
    else
    {
        cout <<" Max test FAILED."<<endl;
        for (int i = 0; i < NUMBER_OF_MACHINES; i++)
        {
            cout <<"DV["<< i <<"]="<<DataValues[i]<<" ";
            if (i % 4 == 3) cout<<endl;
        }
        cout<<"RED said "<<max_pos<<" was max instead of expected "<<max_i<<endl;
        return FAIL;
    }
}


int test_Max_All(bool stress)
{
    UINT16 i = 0;
    INT16 j = 0;
    UINT16 stressLoops;
    INT32 result;

    UINT16 testFails = 0;

    if (stress == true) { stressLoops = 10;} else stressLoops = 0;

        j = stressLoops;
        do
        {
            result = test_Max();
            if (result != PASS)
            {
               cout<< "Test "<< setw(8) << left << " MaxTest "<<" FAILED "<<endl;
               testFails++;
               return testFails;
            }
            else
            {
                if (j == stressLoops)
                    cout<< "Test "<< setw(10) << left << "MaxTest ";
                if ((j > 0) && (j <= stressLoops)) cout<<".";
                if (j == 0) {cout << " PASSED"<<endl;break;}
            }
        }
        while (j-- >= 0);


        cout<<"================================"<<endl;
    if (testFails ==0)
        cout<<"== All MaxTests PASSED =========" <<endl;
    else
        cout<< "=="<< testFails << " MaxTests FAILED ! " <<endl;
        cout<<"================================"<<endl<<endl;

    //DEASM_BATCH(pADD_BNR);
    return testFails;
}
