/*
 * File:   main.cpp
 * Author: Calin (bcalin1984@yahoo.com)
 *
 * OPode INjection for Connex+Arm Architecture ;)
 * O.P.I.N.C.A.A
 * OPINCAA is an kind of asm for connex-arm system, but with c-like compiler interface.
 *
 * Kernel = just like in CUDA, kernel is the function that runs in parallel on connex vector-machine.
 * Batch = a sequence of (logically) grouped instructions, that contain no more than one REDUCTION instruction
 *
 * Concept is:
 *   a) create batch file with instructions you want to crunch with connex at start-up time.
 *   b) launch the kernels on different sets of data got via IO system,
 *
 *
 * History:
 *
 *   v0.1 - proof of concept: two instructions and the two main functions:
 *   v0.2 - added more instructions and de-asm module for checkup
 *   v0.3 - "final draft" implementation: has all instructions; added error checks and functions
 *   v0.4 - added io_unit module (for transfers via IO). UNTESTED.
 *   v0.4.1 - Lucian: fixed subc test
 *   v0.4.2 - Radu & Lucian: fixed simpletests.
 *   v0.5   - Radu: added verilog-simulation mode
 *   v0.5.1 - code clean-up
 *   v0.5.2 - added "estimation mode" for BatchInit functions. See BEGIN_BATCH and END_BATCH.
 *   v0.6   - added c-simulator. Incomplete (CELL_SHL/SHR. Nothing with IO yet)
 *   TODO: add parameters in kernel-init functions.
 *
 *
 * Created on December 19, 2012, 3:32 PM
 *
 */

#include <iostream>
#include <limits>
#include <string.h>

#include "include/vector_registers.h"
#include "include/utils.h"

using namespace std;

STATIC_VECTOR_DEFINITIONS;
// Make sure that batches do not overlap !
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

        SET_ACTIVE(ALL);
        NOP;
        LS[100] = R4;
        R10 = LS[0x32];
        R0 = 0x140;
        REDUCE(R3);
        MULT = R1 * R2;
        CELL_SHR(R2,R3);
        CELL_SHL(R3,R5);
        LS[R1] = R7;
        SET_ACTIVE(WHERE_CARRY);
        SET_ACTIVE(WHERE_EQ);
        SET_ACTIVE(WHERE_LT);
        SET_ACTIVE(ALL);
        R1 = INDEX;
        R3 = LS[R6];
        R3 = _LO(MULT);
        R15 = SHIFT_REG;
        R31 = _HI(MULT);
        R29 = R31 << R29;
        R20 = R14 << 8;
        R2 = R1 + R2;
        R5 = (R3 == R4);
        R1 = ~R3;
        R3 = R1 >> R2;
        R5 = R3 >> 5;
        R3 = R3 - R3;
        R5 = R3 < R4;
        R1 = R1 | R1;
        R3 = SHRA(R3, R3);
        R3 = ISHRA(R4, 9);
        R4 = ADDC(R1, R2);
        R2 = ULT(R4, R3);
        R10 = R6 & R5;
        R4 = SUBC(R4, R4);
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

extern int test_Simple_All();

int main(int argc, char *argv[])
{
    int i;
    int run_mode = REAL_HARDWARE_MODE;
    //look for simulation option in arguments
    for(i=0;i<argc;i++)
    {
        if(strcmp(argv[i],"--simulation") == 0)
        {
            std::cout << "Running in simulation mode" << endl;
            //set simulation flag
            run_mode = VERILOG_SIMULATION_MODE;
            break;
        }

        if(strcmp(argv[i],"--csimulation") == 0)
        {
            std::cout << "Running in c-simulation mode" << endl;
            //set simulation flag
            run_mode = C_SIMULATION_MODE;
            break;
        }
    }

    INIT(run_mode);
    //INIT(C_SIMULATION_MODE);
    test_Simple_All();
    //InitKernel_Radu();
    //VERIFY_KERNEL(RADU_BNR);
    //DEASM_KERNEL(RADU_BNR);
    //if (FOUND_ERROR()) cout<<GET_NUM_ERRORS()<<" error(s) found ! \n";
    DEINIT();
    std::cout << "Press ENTER to continue...";
    std::cin.ignore( std::numeric_limits <std::streamsize> ::max(), '\n' );

    /*
    cout << "Initializing ... "<< endl;
    cout << "Precacheing ... "<< endl; // Equivalent to assembling. Done once per program execution, at runtime.
    InitKernel_Radu();
    VERIFY_KERNEL(RADU_BNR);
    //InitKernel_Reduce();

    cout << "Starting computation ... "<< endl;
    EXECUTE_KERNEL(RADU_BNR);
    result = EXECUTE_KERNEL_RED(REDUCE_BNR);
    */

    /* ... */
    return 0;
}
