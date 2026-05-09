#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <system_error>
#include "ConnexSimulator.h"


using namespace std;

int main(int argc, char **argv) {
    if (argc > 1) {
        int tmp = atoi(argv[1]);
        assert(CONNEX_VECTOR_LENGTH >= tmp);
        CONNEX_VECTOR_LENGTH = tmp;

        printf("OPINCAA simulator: Setting CONNEX_VECTOR_LENGTH to %d\n",
               CONNEX_VECTOR_LENGTH);
        fflush(stdout);
    }
    else {
        printf("OPINCAA simulator: CONNEX_VECTOR_LENGTH is set to the "
               "default value 128\n");
        CONNEX_VECTOR_LENGTH = 128;
        fflush(stdout);
    }

    if (argc > 2) {
        CONNEX_MEM_NUM_ROWS = atoi(argv[2]);
        printf("OPINCAA simulator: Setting CONNEX_MEM_NUM_ROWS to %d\n",
               CONNEX_MEM_NUM_ROWS);
        fflush(stdout);
    }
    else {
        printf("OPINCAA simulator: CONNEX_MEM_NUM_ROWS is set to the "
               "default value 1024\n");
        CONNEX_MEM_NUM_ROWS = 1024;
        fflush(stdout);
    }

    if (argc > 3) {
        CONNEX_REG_COUNT = atoi(argv[3]);
        printf("OPINCAA simulator: Setting CONNEX_REG_COUNT to %d\n",
               CONNEX_REG_COUNT);
        if (CONNEX_REG_COUNT > 32) {
            printf("OPINCAA simulator: Warning CONNEX_REG_COUNT > 32 is "
                   // "NOT supported in the Opincaa simulator NOR the "
                   "NOT supported in Connex Verilog since we need to make size of instruction "
                   "bigger than 32 bits.\n");
            assert(CONNEX_REG_COUNT <= (1UL << LEFT_SIZE));
        }
        fflush(stdout);
    }
    else {
        printf("OPINCAA simulator: CONNEX_REG_COUNT is set to the "
               "default value 32\n");
        CONNEX_REG_COUNT = 32;
        fflush(stdout);
    }

    if (argc > 4) {
        INTERNAL_INSTRUCTION_MEMORY_SIZE = atoi(argv[4]);
        printf("OPINCAA simulator: Setting INTERNAL_INSTRUCTION_MEMORY_SIZE "
               "to %d\n",
               INTERNAL_INSTRUCTION_MEMORY_SIZE);
        fflush(stdout);
    }
    else {
        printf("OPINCAA simulator: INTERNAL_INSTRUCTION_MEMORY_SIZE "
               "is set to the default value 1024\n");
        INTERNAL_INSTRUCTION_MEMORY_SIZE = 1024;
        fflush(stdout);
    }

    /*
    // 2020_03_29
    if (argc > 5) {
        stopSimAtExecuteKernel = atoi(argv[5]);

        printf("OPINCAA program: Setting stopSimAtExecuteKernel "
               "to %d\n",
               stopSimAtExecuteKernel);
        fflush(stdout);
    }
    else {
        printf("OPINCAA program: stopSimAtExecuteKernel "
               "is set to the default value false\n");
        fflush(stdout);

        stopSimAtExecuteKernel = false;
    }
    */

    // 2018_02_10
    ComputeLog2CVL();

    try {
        ConnexSimulator simulator("distributionFIFO", "reductionFIFO",
                                  "writeFIFO", "readFIFO", "regFile");
        simulator.waitFinish();
    }
    catch (string ex) {
        cout << "Exception occured: " << ex << endl;
    }
    catch (system_error syserr) {
        cout << "System error!" << endl;
        //cout << "System error:: " << syserr.what() << endl;
    }

    return 0;
}

