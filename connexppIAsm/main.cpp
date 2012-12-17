#include <iostream>
#include "include\vector.h"

#include <stdio.h>
#include <stdlib.h>

using namespace std;

int vector::dwBatch[][1000];
int vector::dwInBatchCounter[];
int vector::dwBatchIndex;

FILE *pipe_read_32, *pipe_write_32;

void Initialize()
{
    if ((pipe_read_32 = fopen ("/dev/xillybus_read_mem2arm_32","rb")) == NULL)
        perror("Failed to open the read pipe");

    if ((pipe_write_32 = fopen ("/dev/xillybus_write_arm2mem_32","wb")) == NULL)
        perror("Failed to open the write pipe");
}

vector R0(0,0),   R1(0,1),   R2(0,2),   R3(0,3),   R4(0,4),   R5(0,5),   R6(0,6),   R7(0,7);
vector R8(0,8),   R9(0,9),   R10(0,10), R11(0,11), R12(0,12), R13(0,13), R14(0,14), R15(0,15);
vector R16(0,16), R17(0,17), R18(0,18), R19(0,19), R20(0,20), R21(0,21), R22(0,22), R23(0,23);
vector R24(0,24), R25(0,25), R26(0,26), R27(0,27), R28(0,28), R29(0,29), R30(0,30), R31(0,31);

#define BEGIN_BATCH(x)  vector::setBatchIndex(x)
#define END_BATCH(x)
#define BATCH(x) x
#define REDUCE(x)       vector::reduce(x)
#define ADDC(x,y)       vector::addc(x,y)

void InitKernel_0()
{
    BEGIN_BATCH(0);

        R0 = R1 + R2;
        R1 = 10;
        /* ... */

    END_BATCH(0);
}

void InitKernel_1()
{
    BEGIN_BATCH(1);

        R0 = R1 - R2;
        R1 = 13; //vload
        R2 = ADDC(R1, R0);
        /* ... */

    END_BATCH(1);
}

void InitKernel_2()
{
    BEGIN_BATCH(1);

        R0 = R1 + R2;
        R1 = 13;
        /* ... */
        REDUCE(R0);

    END_BATCH(2);
}

inline void ExecuteKernel(int batch_number)
{
    fwrite(vector::dwBatch[batch_number], 1, 4*vector::dwInBatchCounter[vector::dwBatchIndex], pipe_write_32);
}

int ExecuteKernelRed(int batch_number)
{
    int data_read;
    ExecuteKernel(batch_number);
    if (fread(&data_read, 1, 4, pipe_read_32) == 4) return data_read;
    else
    {
        perror("Failed to read from pipe !");
        return 0;
    }
}

int main()
{
    int result;
    cout << "Initializing ... "<< endl;
    Initialize();

    cout << "Precacheing ... "<< endl; // Equivalent to assembling. Done once per program execution, at runtime.
    InitKernel_0();
    InitKernel_1();
    InitKernel_2();

    cout << "Starting computation ... "<< endl;
    ExecuteKernel(0);
    ExecuteKernel(1);
    result = ExecuteKernelRed(2);
    /* ... */
    return 0;
}
