/* Multiplying 2 matrices, stored in row-major order,
   the 2nd being transposed for efficiency of multiplication.
  We use the standard (row-major order) matrix multiplication algorithm.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#include "Kernel_preassembled.h"
//#include "Kernel_precomputed.h"




#ifdef RUN_ON_ZEDBOARD
  #include "../timing.cpp"
#endif


//#define PRECOMPUTE_KERNEL

//#define LLVM_ISEL_CODEGEN
#if (defined LLVM_ISEL_CODEGEN) || (defined PRECOMPUTE_KERNEL)
  #define PRINTREG(regNum) regNum
  #define PrintDebugMessage(aStr) aStr
  #define PrintRegDebug(regNum) regNum
#endif



//#define SIZE 1000
//#define SIZE 256
//#define SIZE 512
//#define SIZE 128
#define SIZE 256
// Does NOT vectorize (at least on Connex with 128 lanes): #define SIZE 20

// typedef long TYPE;
// typedef int TYPE;
typedef short TYPE;

TYPE A[SIZE][SIZE];
TYPE B[SIZE][SIZE];
TYPE C[SIZE][SIZE];

void MatMul_RowMajor_Transposed() {
    TYPE i, j, k;

{ // adding extra brackets to account for loops without them (in ReplaceLoopsWithOpincaaKernels.cpp) 
    int numI16WordsAccessedInArrayA = 1 * ((((((int *)&A) ) +   131072) - ((int *)&A)) / 2);
    connexGlobal->writeDataToConnexPartial(((int *)&A), /* actual num elems written */ numI16WordsAccessedInArrayA, /*offset*/ 0); // writeDataToConnexPartial() is blocking // Generated in InstrumentVectorGatherLoadOrScatterStore()
    int numI16WordsAccessedInArrayB = 1 * ((((((int *)&B) ) +   131072) - ((int *)&B)) / 2);
    connexGlobal->writeDataToConnexPartial(((int *)&B), /* actual num elems written */ numI16WordsAccessedInArrayB, /*offset*/ 0 + (int)ceil( ((float)numI16WordsAccessedInArrayA) / CONNEX_VL)); // writeDataToConnexPartial() is blocking // Generated in InstrumentVectorGatherLoadOrScatterStore()


#define EXEC_PRECOMPUTED_KERNEL

#ifdef EXEC_PRECOMPUTED_KERNEL
    Kernel *__kernel = new Kernel(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
    __kernel->copyBinaryKernel(preassembledBinaryKernel, PREASSEMBLED_BINARY_KERNEL_SIZE);
    ConnexMachine::addKernel(__kernel);
#else
    _BEGIN_KERNEL(BatchNumberGlobal); // Generated in vectorizeLoop()
        EXECUTE_IN_ALL( // Generated in vectorizeLoop()

for (i = 0; i < SIZE; ++i) { // Brackets preferred

	R(0) = 0 ; // MSA_I10           // <MCInst #144 VLOAD_H
	R(1) = 1 ; // MSA_I10           // <MCInst #144 VLOAD_H
	R(2) = R(0) | R(0) ; // MSA_3R generic instruction // <MCInst #109 ORV_H
	R(3) = // (fake but necessary ; ) VLOAD_H_SYM_IMM MSA_I10 // <MCInst #145 VLOAD_H_SYM_IMM
            ((((((((int *)&A) ) +   131072) -   (((int *)&A) )) /  (((int *)&CONNEX_VL)[0])) >>  1) +  (((((((int *)&A) ) +   131072) -   (((int *)&A) )) %  ((((int *)&CONNEX_VL)[0]) <<  1)) >  0)); // MSA_I10 // custom code in ConnexInstPrinter::printInst() for INLINEASM // <MCInst #1 INLINEASM
                                        //  <MCOperand Expr:(    ((((((((int *)&A) ) +   131072) -   (((int *)&A) )) /  (((int *)&CONNEX_VL)[0])) >>  1) +  (((((((int *)&A) ) +   131072) -   (((int *)&A) )) %  ((((int *)&CONNEX_VL)[0]) <<  1)) >  0)); // MSA_I10)>
	REPEAT_X_TIMES( // (fake but necessary ; ) REPEAT_DESC_BASE_SYM_IMM // <MCInst #116 REPEAT_SYM_IMM>
	    256);
	R(5) = // (fake but necessary ; ) VLOAD_H_SYM_IMM MSA_I10 // <MCInst #145 VLOAD_H_SYM_IMM
            ((i <<  9) /  ((((int *)&CONNEX_VL)[0]) <<  1)); // MSA_I10 // custom code in ConnexInstPrinter::printInst() for INLINEASM // <MCInst #1 INLINEASM
                                        //  <MCOperand Expr:(    ((i <<  9) /  ((((int *)&CONNEX_VL)[0]) <<  1)); // MSA_I10)>
	R(4) = R(0) | R(0) ; // MSA_3R generic instruction // <MCInst #109 ORV_H
	int indexLLVM_LV2;
int origLoopTripCount = 256;
for (indexLLVM_LV2 = 0; indexLLVM_LV2 < origLoopTripCount; indexLLVM_LV2 += CONNEX_VL) { // vectorized loop for induction var [NO INFO]
	 // Map part of reduction code; // Generated in vectorizeLoop()
	// An empty inline Asm expression, required for ConnexAsmPrinter.cpp, MoveToFront();
	R(2) = R(3) + R(1) ; // MSA_3R generic instruction // <MCInst #29 ADDV_H
	R(6) = R(5) + R(1) ; // MSA_3R generic instruction // <MCInst #29 ADDV_H
	R(5) = LS[R(5)]; // READ 32bits index (gather) // <MCInst #81 LD_INDIRECT_H
	R(3) = LS[R(3)]; // READ 32bits index (gather) // <MCInst #81 LD_INDIRECT_H
	R(3) * ( R(5) ); R(3) = MULT_LOW(); // MUL_3R // <MCInst #98 MULV_H
	R(5) = R(6) | R(6) ; // MSA_3R generic instruction // <MCInst #109 ORV_H
	R(4) = R(4) + R(3) ; // MSA_3R generic instruction // <MCInst #29 ADDV_H
	R(3) = R(2) | R(2) ; // MSA_3R generic instruction // <MCInst #109 ORV_H
	} // END for (indexLLVM_LV2) loop 
	REDUCE R(4) ; // MSA_1R generic instruction // <MCInst #114 RED_H
	R(3) = R(2) | R(2) ; // MSA_3R generic instruction // <MCInst #109 ORV_H
	END_REPEAT; // END_REPEAT_DESC_BASE // <MCInst #46 END_REPEAT>
    }

);
    _END_KERNEL(BatchNumberGlobal);
#endif

#ifndef PRECOMPUTE_KERNEL
  connexGlobal->executeKernel(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));

connexGlobal->readCorrectReductionResults(C, (((((int *)&C) ) +   131072) - ((int *)&C)) / 2, 2); // !!!!TODO TODO TODO: for padded case we have problems
#endif
} // END extra brackets

}

//#define TEST_OPINCAA_CONNEX_KERNELS

#ifdef TEST_OPINCAA_CONNEX_KERNELS
void Test() {
    int i, j;
    long row, column;

    printf("SIZE = %d\n", SIZE);
    fflush(stdout);

#ifdef PRECOMPUTE_KERNEL
    string kernelName = TEST_PREFIX + to_string((long long int)BatchNumberGlobal);

    MatMul_RowMajor_Transposed();

    Kernel *kernel = connexGlobal->getKernel(kernelName);

    string binaryKernel = kernel->genPrecomputedKernel();
    FILE *fout = fopen("Kernel_preassembled.h", "wt");
    fprintf(fout, "%s\n", binaryKernel.c_str());
    fclose(fout);
#endif

#ifdef LLVM_ISEL_CODEGEN
    string kernelName = "bubblesort";
    Kernel *kernel = connexGlobal->getKernel(kernelName);

    kernel->genPrecomputedKernel();

    kernel->sdNodeVarNameRegDef[REG_SRC] = "nodeOpSrcCast1";
    //
    // For RED f16:
    kernel->offsetKernelToStartCodegenFrom = 1 + 1; // +1 for the END_WHERE instruction Opincaa adds automatically to each kernel
    kernel->numInstructionsToCodegen = kernel->size() - 0 /*num instruction we remove from end of kernel */ - kernel->offsetKernelToStartCodegenFrom;
    //
    // We use chain, since with glue we get a lot or weird scheduling errors:
    //kernel->useGlue = 0;
    kernel->useGlue = 1;
    /* IMPORTANT: to convert in 'partly SSA form' we require ~64 (usually more
                   than 32) registers. */
    assert(CONNEX_REG_COUNT != 32);

    printf("Calling connexGlobal->genLLVMISelManualCode()\n");
    fflush(stdout);
    //string resGenLLVM = connexGlobal->genLLVMISelManualCode(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
    string resGenLLVM = connexGlobal->genLLVMISelManualCode(kernelName);
    //printf("resGenLLVM = \n%s\n", resGenLLVM.c_str());
    //fflush(stdout);


    printf("Calling connexGlobal->dumpKernel()\n");
    fflush(stdout);
    string resDump = connexGlobal->dumpKernel(kernelName);
    printf("resDump = %s\n", resDump.c_str());
    fflush(stdout);

    printf("Calling connexGlobal->disassembleKernel()\n");
    fflush(stdout);
    string resDis = connexGlobal->disassembleKernel(kernelName);
    printf("resDis = %s\n", resDis.c_str());
    fflush(stdout);
#endif


    /*
    TYPE A[SIZE][SIZE];
    TYPE B[SIZE][SIZE];
    TYPE C[SIZE][SIZE];
    */

    long numRowsCols = SIZE;

    srand(time(NULL));
    //
    // We intialize the B matrix
    for (i = 0; i < numRowsCols; ++i) { // Brackets required to enclose the Opincaa kernel, if ever
        for (j = 0; j < numRowsCols; ++j) { // Brackets required to enclose the Opincaa kernel, if ever
            // A[i][j] = 2;
            //A[i][j] = rand() % 10 - 5; // RAND_MAX;
            A[i][j] = rand() % 65536 - 32768; // RAND_MAX;
        }
    }

    // We intialize (and transpose) the B matrix to perform efficient matrix
    // multiplication
    for (i = 0; i < numRowsCols; ++i) { // Brackets required to enclose the Opincaa kernel, if ever
        for (j = 0; j < numRowsCols; ++j) { // Brackets required to enclose the Opincaa kernel, if ever
            // B[i][j] = 1;
            //B[i][j] = rand() % 10 - 5; // RAND_MAX;
            B[i][j] = rand() % 65536 - 32768; // RAND_MAX;
        }
    }

    printf("Calling MatMul_RowMajor_Transposed...\n");
    fflush(stdout);

  #ifndef SIMULATOR_MODE
    int timeStart = GetMilliCount();
  #endif

    //int res;
  //#define NUM_TESTS 100
  #define NUM_TESTS 1
    for (int idTest = 0; idTest < NUM_TESTS; idTest++) {
        MatMul_RowMajor_Transposed();
    }


  #ifndef SIMULATOR_MODE
    int totalExecTime = GetMilliSpan(timeStart);
    printf("Total time in MatMul_RowMajor_Transposed() for %d runs: %d ms --> time per run = %.6f\n",
           NUM_TESTS, totalExecTime, ((float)totalExecTime) / NUM_TESTS);
  #endif

    printf("Finished executing the Opincaa kernel.\n");
    fflush(stdout);

    printf("Testing the correctness of the computation of the Opincaa kernel.\n");
    fflush(stdout);

  //#define PRINT_C
  #define FAIL -1
  #define PASS 0

    int testResult = PASS;

  #ifdef PRINT_C
    printf("C =\n");
  #endif
    for (row = 0; row < SIZE; ++row) {
        for (column = 0; column < SIZE; ++column) {
  // printf("C[%d][%d] = %d\n");
  #ifdef PRINT_C
            printf("%d ", C[row][column]);
  #endif

            int resTmp = 0;
            for (int k = 0; k < SIZE; ++k) {
                resTmp += A[row][k] * B[column][k];
            }
            // printf("resTmp = %d\n", resTmp);
            // assert(C[row][column] == resTmp);
            if ((C[row][column] & 0xFFFF) != (resTmp & 0xFFFF)) {
                printf("Diff: C[%ld][%ld] = %d, resTmp = %d\n", row, column,
                       C[row][column], resTmp);
                testResult = FAIL;
            }

            // assert(C[row][column] == A[row][column] * B[row][column] * SIZE); //
            // NOT the best check but it's OK
        }
  #ifdef PRINT_C
        printf("\n");
  #endif
    }

    if (testResult == PASS)
        printf("Test passed OK\n");
    else
        printf("Test FAILED!!!!\n");
}

#ifdef STANDALONE_APP
int main() {
    Test();

    return 0;
}
#endif

#endif


