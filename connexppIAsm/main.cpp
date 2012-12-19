#include <iostream>

#include "include\vector.h"
#include "include\vector_registers.h"

using namespace std;

STATIC_VECTOR_DEFINITIONS;

// Make sure that batches do not ovelap !
// BNR = Batch NumBer

enum BatchNumbers
{
    RADU_BNR = 0,
    REDUCE_BNR = 1,
    MAX_BNR = 99
};

void InitKernel_Radu()
{
    BEGIN_BATCH(RADU_BNR);
        NOP;
        //LOCAL_STORE(100) = R4;
        //R10 = LOCAL_STORE(0x32);
        R0 = 0x140;
        REDUCE(R3);
        //MULT_RESULT = R1 * R2;
        NOP;//CELL SHR R2 R3
        NOP;//CELL SHL R3 R5
        //LOCAL_STORE(R1) = R7;
        SET_ACTIVE(WHERE_CARRY);
        SET_ACTIVE(WHERE_EQ);
        SET_ACTIVE(WHERE_LT);
        SET_ACTIVE(ALL);
        R1 = INDEX;
        //R3 = LOCAL_STORE(R6);
        //R3 = MULT_RESULT(LO);
        R15 = SHIFT_REG;
        //R31 = MULT_RESULT(HI);
        R29 = R31 << R29;
        //R20 = ISHL(R14, 8);
        R2 = R1 + R2;
        R5 = (R3 == R4); // collision with VLOAD !!
        //R1 = !R3;
        R3 = R1 >> R2;
        //R5 = ISHR(R3,5);
        R3 = R3 - R3;
        R5 = R3 < R4;
        R1 = R1 || R1;
        //R3 = SHRA(R3, R3);
        //R3 = ISHRA(R4, 9);
        R4 = ADDC(R1, R2);
        //R2 = ULT(R4, R3);
        R10 = R6 && R5;
        //R4 = SUBC(R4, R4);
        R1 = R1 ^ R1;

    END_BATCH(RADU_BNR);
}


void InitKernel_Reduce()
{
    BEGIN_BATCH(REDUCE_BNR);

        R0 = R1 + R2;
        R1 = 13;
        /* ... */
        REDUCE(R0);

    END_BATCH(REDUCE_BNR);
}

enum errorCodes
{
    INIT_FAILED
};
int main()
{
    int result;
    cout << "Initializing ... "<< endl;

    //if (INIT() != PASS) return INIT_FAILED;

    cout << "Precacheing ... "<< endl; // Equivalent to assembling. Done once per program execution, at runtime.
    InitKernel_Radu();
    VERIFY_KERNEL(RADU_BNR);
    //InitKernel_Reduce();

    cout << "Starting computation ... "<< endl;
    EXECUTE_KERNEL(RADU_BNR);
    result = EXECUTE_KERNEL_RED(REDUCE_BNR);
    /* ... */
    return 0;
}
