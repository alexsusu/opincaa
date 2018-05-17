#include "ConnexSimulator.h"
#include <stdlib.h>
#include <string>
#include <iostream>
#include <system_error>


using namespace std;

int main(int argc, char **argv) {
    if (argc > 1) {
        CONNEX_VECTOR_LENGTH = atoi(argv[1]);
        printf("Opincaa simulator: Setting CONNEX_VECTOR_LENGTH to %d\n",
               CONNEX_VECTOR_LENGTH);
        fflush(stdout);
    }
    else {
        printf("Opincaa simulator: CONNEX_VECTOR_LENGTH is set to the "
               "default value 128\n");
        CONNEX_VECTOR_LENGTH = 128;
        fflush(stdout);
    }

    if (argc > 2) {
        CONNEX_MEM_SIZE = atoi(argv[2]);
        printf("Opincaa simulator: Setting CONNEX_MEM_SIZE to %d\n",
               CONNEX_MEM_SIZE);
        fflush(stdout);
    }
    else {
        printf("Opincaa simulator: CONNEX_MEM_SIZE is set to the "
               "default value 1024\n");
        CONNEX_MEM_SIZE = 1024;
        fflush(stdout);
    }

    if (argc > 3) {
        CONNEX_REG_COUNT = atoi(argv[3]);
        printf("Opincaa simulator: Setting CONNEX_REG_COUNT to %d\n",
               CONNEX_REG_COUNT);
        fflush(stdout);
    }
    else {
        printf("Opincaa simulator: CONNEX_REG_COUNT is set to the "
               "default value 32\n");
        CONNEX_REG_COUNT = 32;
        fflush(stdout);
    }

    if (argc > 4) {
        INSTRUCTION_QUEUE_LENGTH = atoi(argv[4]);
        printf("Opincaa simulator: Setting INSTRUCTION_QUEUE_LENGTH to %d\n",
               INSTRUCTION_QUEUE_LENGTH);
        fflush(stdout);
    }
    else {
        printf("Opincaa simulator: INSTRUCTION_QUEUE_LENGTH is set to the "
               "default value 1024\n");
        INSTRUCTION_QUEUE_LENGTH = 1024;
        fflush(stdout);
    }


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

