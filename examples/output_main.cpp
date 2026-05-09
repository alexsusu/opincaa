#include <assert.h>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <typeinfo>

#include "Architecture.h"
#include "ConnexMachine.h"
#include "PadArrayOpincaa.h"
#include "test.h"

using namespace std;


#ifndef RUN_ON_ZEDBOARD
  #define SIMULATOR_MODE
#else
  #ifdef PRINTREG
     #undef PRINTREG
     #define PRINTREG(regNum) regNum
  #endif

  #ifdef PrintDebugMessage
      #undef PrintDebugMessage
      #define PrintDebugMessage(aStr) aStr
  #endif

  #ifdef PrintRegDebug
      #undef PrintRegDebug
      #define PrintRegDebug(regNum) regNum
  #endif

  #ifdef PrintDebugReg
      #undef PrintDebugReg
      #define PrintDebugReg(regNum) regNum
  #endif
#endif


#define TEST_PREFIX "simpleIoTest_allowOverwrite"
#define _BEGIN_KERNEL(x) BEGIN_KERNEL(TEST_PREFIX + to_string((long long int)x))
#define _END_KERNEL(x) END_KERNEL(TEST_PREFIX + to_string((long long int)x))


static ConnexMachine *connexGlobal = NULL;
static int BatchNumberGlobal;

//const int CONNEX_VL = CONNEX_VECTOR_LENGTH;
int CONNEX_VL;




// These macros are for backwards compatibility
#define writeDataToArray writeDataToConnex
#define readDataFromArray readDataFromConnex

#define TEST_OPINCAA_CONNEX_KERNELS
#define NOT_FOR_CONNEX_LLVM_LV_PACKING_CODE_FOR_TILING
#include "output.cpp"




int main(int argc, char **argv) {
    if (argc > 1) {
        CONNEX_VECTOR_LENGTH = atoi(argv[1]);

        printf("OPINCAA program: Setting CONNEX_VECTOR_LENGTH to %d\n",
               CONNEX_VECTOR_LENGTH);
        fflush(stdout);
    }
    else {
        printf("OPINCAA program: CONNEX_VECTOR_LENGTH is set to default value 128\n");
        fflush(stdout);

        CONNEX_VECTOR_LENGTH = 128;
    }

    if (argc > 2) {
        CONNEX_MEM_NUM_ROWS = atoi(argv[2]);

        printf("OPINCAA program: Setting CONNEX_MEM_NUM_ROWS to %d\n", CONNEX_MEM_NUM_ROWS);
        fflush(stdout);

        assert((CONNEX_MEM_NUM_ROWS & (CONNEX_MEM_NUM_ROWS - 1)) == 0 &&
               "CONNEX_MEM_NUM_ROWS must be power of 2");
    }
    else {
        printf("OPINCAA program: CONNEX_MEM_NUM_ROWS is set to the default value 1024\n");
        fflush(stdout);

        CONNEX_MEM_NUM_ROWS = 1024;
    }

    if (argc > 3) {
        CONNEX_REG_COUNT = atoi(argv[3]);

        printf("OPINCAA program: Setting CONNEX_REG_COUNT to %d\n",
               CONNEX_REG_COUNT);
        if (CONNEX_REG_COUNT > 32) {
            printf("OPINCAA program: Warning CONNEX_REG_COUNT > 32 is NOT supported in "
                   //"OPINCAA simulator NOR the "
                   "the Connex Verilog since we need to make size of instruction "
                   "bigger than 32 bits.\n");
        }
        fflush(stdout);

        assert(CONNEX_REG_COUNT <= (1UL << LEFT_SIZE));
    }
    else {
        printf("OPINCAA program: CONNEX_REG_COUNT is set to the default value 32\n");
        fflush(stdout);

        CONNEX_REG_COUNT = 32;
    }

    if (argc > 4) {
        INTERNAL_INSTRUCTION_MEMORY_SIZE = atoi(argv[4]);

        printf("OPINCAA program: Setting INTERNAL_INSTRUCTION_MEMORY_SIZE "
               "to %d\n",
               INTERNAL_INSTRUCTION_MEMORY_SIZE);
        fflush(stdout);
    }
    else {
        printf("OPINCAA program: INTERNAL_INSTRUCTION_MEMORY_SIZE "
               "is set to the default value 1024\n");
        fflush(stdout);

        INTERNAL_INSTRUCTION_MEMORY_SIZE = 1024;
    }

    if (argc > 5) {
        checkForDataHazards = atoi(argv[5]);

        printf("OPINCAA program: Setting checkForDataHazards "
               "to %d\n",
               checkForDataHazards);
        fflush(stdout);
    }
    else {
        printf("OPINCAA program: checkForDataHazards "
               "is set to the default value false\n");
        fflush(stdout);

        checkForDataHazards = false;
    }

    if (argc > 6) {
        useLaneGatingOnConnex = atoi(argv[6]);

        printf("OPINCAA program: Setting useLaneGatingOnConnex "
               "to %d\n",
               useLaneGatingOnConnex);
        fflush(stdout);
    }
    else {
        printf("OPINCAA program: useLaneGatingOnConnex "
               "is set to the default value false.\n");
        fflush(stdout);

        useLaneGatingOnConnex = false;
    }

    if (argc > 7) {
        numMaxNestedHwLoops = atoi(argv[7]);

        printf("OPINCAA program: Setting numMaxNestedHwLoops "
               "to %d\n",
               numMaxNestedHwLoops);
        fflush(stdout);
    }
    else {
        printf("OPINCAA program: numMaxNestedHwLoops "
               "is set to the default value 0 (loops with NO nests).\n");
        fflush(stdout);

        numMaxNestedHwLoops = 0;
    }

    if (argc > 8) {
        dontExecuteKernel = atoi(argv[8]);

        printf("OPINCAA program: Setting dontExecuteKernel "
               "to %d\n",
               dontExecuteKernel);
        fflush(stdout);
    }
    else {
        printf("OPINCAA program: dontExecuteKernel "
               "is set to the default value false.\n");
        fflush(stdout);

        dontExecuteKernel = false;
    }

    CONNEX_VL = CONNEX_VECTOR_LENGTH;
    ComputeLog2CVL();


    assert(CONNEX_VECTOR_LENGTH == (1UL << LOG2_CONNEX_VECTOR_LENGTH));
    printf("CONNEX_VECTOR_LENGTH = %d (LOG2_CONNEX_VECTOR_LENGTH = %d)\n",
            CONNEX_VECTOR_LENGTH, LOG2_CONNEX_VECTOR_LENGTH);
    printf("CONNEX_MEM_NUM_ROWS = %d\n", CONNEX_MEM_NUM_ROWS);
    printf("CONNEX_MEM_NUM_ROWS_EXTRA = %d\n", CONNEX_MEM_NUM_ROWS_EXTRA);
    printf("CONNEX_MEM_SPILL_START_OFFSET = %d\n", CONNEX_MEM_SPILL_START_OFFSET);
    printf("INTERNAL_INSTRUCTION_MEMORY_SIZE = %d\n", INTERNAL_INSTRUCTION_MEMORY_SIZE);
    //
    printf("sizeof(ConnexVectorElementType) = %ld\n", sizeof(ConnexVectorElementType));
    // Inspired from https://stackoverflow.com/questions/11310898/how-do-i-get-the-type-of-a-variable
    cout << "ConnexVectorElementType = " << typeid(ConnexVectorElementType).name() << endl;



    BEGIN_KERNEL("QuitKernel");
      //EXECUTE_IN_ALL(
        QUIT;
      //)
    END_KERNEL("QuitKernel");



    try {
      #ifdef SIMULATOR_MODE
        connexGlobal = new ConnexMachine("distributionFIFO",
                                         "reductionFIFO",
                                         "writeFIFO",
                                         "readFIFO",
                                         "regFile");
      #else
        connexGlobal = new ConnexMachine(
                                         // Program (instruction) FIFO
                                         "/dev/xillybus_connex_instruction_32",
                                         // Reduction FIFO
                                         "/dev/xillybus_connex_reduction_32",
                                         // Write FIFO from LS memory
                                         "/dev/xillybus_connex_iowrite_32",
                                         // Read FIFO to LS memory
                                         "/dev/xillybus_connex_ioread_32",
                                         // One can also use "/dev/uio0"
                                         "regFile"
                                         );
      #endif


        BatchNumberGlobal = 123456;

        try {
            Test(); // Defined normally in output.cpp
        }
        catch (string ex) {
            cout << "Exception occured: " << ex << endl;
        }

        delete connexGlobal;
    }
    catch (string err) {
        cout << err << endl;
    }

    return 0;
}
