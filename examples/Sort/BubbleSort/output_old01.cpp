#include <iostream>


//#define LLVM_ISEL_CODEGEN
#ifdef LLVM_ISEL_CODEGEN
  #define PRINTREG(regNum) regNum
  #define PrintDebugMessage(aStr) aStr
  #define PrintRegDebug(regNum) regNum
#endif

//#include "ConnexMachine.h"
//#include "../LibPatterns.h"
//#include "../LibMisc.h"



    #define REG_CT1             31
    #define REG_CT0             30
    #define REG_CT1A            29
    #define REG_CT1B            28
    #define REG_SRC             27
    //
    #define REG_PRED1           26
    #define REG_PRED2           25
    #define REG_PRED3           24
    //
    #define REG_IDXMOD2         23
    #define REG_INDEX           22
    #define REG_AUX1            21
    #define REG_AUX2            20



using namespace std;


/*
From http://lauravasilescu.ro/p/master-thesis/thesis.pdf

5.3.1  Validation
We verified the correctness of the compilation process by running different sized vectors
with random elements through the bubble-sort algorithm.
The algorithm use a sequence of two kernels:
  kernel_odd compares elements stored on odd positions,
  while kernel_even compares elements stored on even positions
  (listing 5.3. The reduce call will gather information about how many interchanges were
  done during the kernels execution. The kernels are applied until no exchange are done,
  namely until the reduce value is zero.

__kernel void kernel_odd(short *in) {
2  short i = get_global_id(0);
3  short exchange = 0;
4
5  if (i % 2) {
6  short a = in[i - 1];
7  short b = in[i];
8  if  (b < a) {
9     exchange = 1;
10    in[i] = a;
11 }
12 if (exchange)
13     in[i - 1] = b;
14 }
15 reduce(exchange);
16 }
17
18 __kernel void kernel_even(short *in) {
19 short i = get_global_id(0);
20 short exchange = 0;
21
22 if (i % 2 == 0 & i != 0) {
23 short a = in[i - 1];
24 short b = in[i];
25 if (b < a) {
26     exchange = 1;
27     in[i] = a;
28 }
29 if (exchange)
30     in[i - 1] = b;
31 }
32 reduce(exchange);
33 }
Listing 5.3: OpenCL Kernel: Modified Bubble Sort Algorithm
*/

/*
Although not often, we might require to read the sorted vector from the LS memory.
TODO TODO: implement BubbleSort on multiple vectors.

Examples:
4, 3, 2, 1 - requires 2 big steps

8, 7, 6, 5, 4, 3, 2, 1 - requires 8 steps (4 BIG steps of propagation even->odd and odd->even)
--
7, 8, 5, 6, 3, 4, 1, 2
7, 5, 8, 3, 6, 1, 4, 2
--
5, 7, 3, 8, 1, 6, 2, 4
5, 3, 7, 1, 8, 2, 6, 4
--
3, 5, 1, 7, 2, 8, 4, 6
3, 1, 5, 2, 7, 4, 8, 6
--
1, 3, 2, 5, 4, 7, 6, 8
1, 2, 3, 4, 5, 6, 7, 8
--

*/


void InitBubbleSortKernel(ConnexMachine *connex, int32_t opALSIndex) {
    BEGIN_KERNEL("bubblesort_init");
        EXECUTE_IN_ALL(
            R(REG_SRC) = LS[opALSIndex]; // load vector to be sorted
          PrintRegDebug(REG_SRC);

            R(REG_CT0) = 0;
            R(REG_CT1) = 1;
            R(REG_CT1A) = 1;
            R(REG_CT1B) = 1;
            R(REG_INDEX) = INDEX;


            R(REG_PRED1) = R(REG_INDEX) == R(REG_CT0);
            NOP;
          )
        EXECUTE_WHERE_EQ(
            R(REG_CT1A) = 0;
          PrintRegDebug(REG_CT1A);
        )
        EXECUTE_IN_ALL(

            R(REG_AUX1) = CONNEX_VECTOR_LENGTH - 1;
          PrintRegDebug(REG_INDEX);
          PrintRegDebug(REG_AUX1);
            R(REG_PRED1) = R(REG_INDEX) == R(REG_AUX1);
          PrintRegDebug(REG_PRED1);
          PrintRegDebug(REG_CT1B);
            NOP;
          )
        EXECUTE_WHERE_EQ(
            R(REG_CT1B) = 0;
          PrintRegDebug(REG_CT1B);
        )
        EXECUTE_IN_ALL(
            REDUCE(R0);
        )
    END_KERNEL("bubblesort_init");
}

void BubbleSortKernel(ConnexMachine *connex, int32_t opALSIndex, int32_t resLSIndex) {
    InitBubbleSortKernel(connex, opALSIndex);

    BEGIN_KERNEL("bubblesort");
        EXECUTE_IN_ALL(
            /*
            R(REG_SRC) = LS[opALSIndex]; // load vector to be sorted
          PrintRegDebug(REG_SRC);

            R(REG_CT0) = 0;
            R(REG_CT1) = 1;
            R(REG_CT1A) = 1;
            R(REG_INDEX) = INDEX;
            */
          PrintDebugMessage("New i iteration:");
          PrintRegDebug(REG_SRC);

            R(REG_IDXMOD2) = INDEX;
            R(REG_IDXMOD2) &= R(REG_CT1);
          PrintRegDebug(REG_IDXMOD2);

            //CELL_SHL(R(REG_SRC), R(REG_CT1A));
            CELL_SHL(R(REG_SRC), R(REG_CT1B));
            NOP;
            R(REG_AUX1) = SHIFT_REG;
          PrintRegDebug(REG_AUX1);

            // REG_AUX2 = REG_SRC;
            //CELL_SHL(R(REG_SRC), R(REG_CT1));

            //CELL_SHR(R(REG_SRC), R(REG_CT1B));
            CELL_SHR(R(REG_SRC), R(REG_CT1A));
            NOP;
            R(REG_AUX2) = SHIFT_REG;
          PrintRegDebug(REG_AUX2);

            // R(REG_AUX2) = R(REG_SRC);


            // 1st for even positions - Doing propagation from even to odd positions at the right
            R(REG_PRED1) = R(REG_IDXMOD2) == R(REG_CT0);
          PrintRegDebug(REG_PRED1);
            R(REG_PRED2) =  R(REG_AUX1) < R(REG_SRC);
          PrintRegDebug(REG_AUX1);
          PrintRegDebug(REG_SRC);
          PrintRegDebug(REG_PRED2);
            R(REG_PRED3) = R(REG_PRED1) & R(REG_PRED2);
            R(REG_PRED3) = R(REG_PRED3) == R(REG_CT1);
          PrintRegDebug(REG_PRED3);
            NOP;
          )
        EXECUTE_WHERE_EQ(
              // REG_AUX2 = REG_SRC; // This should be more energy efficient
              R(REG_SRC) = R(REG_AUX1);
          PrintDebugMessage("(Even case) After working on even positions:");
          PrintRegDebug(REG_SRC);
          )
        EXECUTE_IN_ALL(
            // REG_PRED3 is characteristic bool vector where we change
        PrintDebugMessage("Before REDUCE:");
        PrintRegDebug(REG_PRED3);
            REDUCE(R(REG_PRED3));


            CELL_SHR(R(REG_PRED2), R(REG_CT1A));
            //CELL_SHR(R(REG_PRED2), R(REG_CT1));
            NOP;
            R(REG_PRED2) = SHIFT_REG;


          /* Put on odd positions the max of the two, if we changed in the
           previous step the even positions to the left */
            R(REG_PRED1) = R(REG_IDXMOD2) == R(REG_CT1);
            R(REG_PRED3) = R(REG_PRED1) & R(REG_PRED2); // REG_PRED2 already computed above

            R(REG_PRED3) = R(REG_PRED3) == R(REG_CT1);
          PrintRegDebug(REG_PRED3);
            NOP;
          )
        EXECUTE_WHERE_EQ(
            R(REG_SRC) = R(REG_AUX2);
          PrintDebugMessage("(Even case) After adjusting the associated odd positions:");
          PrintRegDebug(REG_SRC);
        )
        EXECUTE_IN_ALL(
            // REG_PRED3 is characteristic bool vector where we change
            //REDUCE(R(REG_PRED3));











          PrintDebugMessage("Doing propagation from odd to even positions at the right:");
          PrintRegDebug(REG_SRC);

            //CELL_SHL(R(REG_SRC), R(REG_CT1A));
            CELL_SHL(R(REG_SRC), R(REG_CT1B));
            NOP;
            R(REG_AUX1) = SHIFT_REG;
          PrintRegDebug(REG_AUX1);

            // REG_AUX2 = REG_SRC;
            //CELL_SHL(R(REG_SRC), R(REG_CT1));

            CELL_SHR(R(REG_SRC), R(REG_CT1A));
            NOP;
            R(REG_AUX2) = SHIFT_REG;

            // R(REG_AUX2) = R(REG_SRC);


            /* Now for odd positions: Doing propagation from odd to even
             positions at the right */
            R(REG_PRED1) = R(REG_IDXMOD2) == R(REG_CT1);
          PrintRegDebug(REG_PRED1);
            R(REG_PRED2) =  R(REG_AUX1) < R(REG_SRC);
          PrintRegDebug(REG_AUX1);
          PrintRegDebug(REG_SRC);
          PrintRegDebug(REG_PRED2);
            R(REG_PRED3) = R(REG_PRED1) & R(REG_PRED2);
            R(REG_PRED3) = R(REG_PRED3) == R(REG_CT1);
          PrintRegDebug(REG_PRED3);
            NOP;
          )
        EXECUTE_WHERE_EQ(
              // REG_AUX2 = REG_SRC; // This should be more energy efficient
              R(REG_SRC) = R(REG_AUX1);
          PrintDebugMessage("(Odd case) After working on even positions:");
          PrintRegDebug(REG_SRC);
          )
        EXECUTE_IN_ALL(
            // REG_PRED3 is characteristic bool vector where we change
            //REDUCE(R(REG_PRED3));




            //CELL_SHR(R(REG_PRED2), R(REG_CT1A));
            CELL_SHR(R(REG_PRED2), R(REG_CT1A));
            NOP;
            R(REG_PRED2) = SHIFT_REG;


          /* Put on even positions the max of the two, if we changed in the
           previous step the even positions to the left. */
            R(REG_PRED1) = R(REG_IDXMOD2) == R(REG_CT0);
            R(REG_PRED3) = R(REG_PRED1) & R(REG_PRED2); // REG_PRED2 already computed above

            R(REG_PRED3) = R(REG_PRED3) == R(REG_CT1);
          PrintRegDebug(REG_PRED3);
            NOP;
          )
        EXECUTE_WHERE_EQ(
            R(REG_SRC) = R(REG_AUX2);
          PrintDebugMessage("(Odd case) After adjusting the associated even positions:");
          PrintRegDebug(REG_SRC);
        )
        EXECUTE_IN_ALL(
            // REG_PRED3 is characteristic bool vector where we change
        PrintDebugMessage("Before REDUCE:");
        PrintRegDebug(REG_PRED3);
            REDUCE(R(REG_PRED3));


            // store result
            LS[resLSIndex] = R(REG_SRC);

            // End of program synchronization point; host will wait for this
            //REDUCE(R1);
        )
    END_KERNEL("bubblesort");


    connex->executeKernel("bubblesort_init");
    connex->readReduction();

    //for (int i = 0; i < CONNEX_VECTOR_LENGTH / 2; i++)
    for (int i = 0; i < CONNEX_VECTOR_LENGTH / 2 + 1; i++) {
        printf("i = %d\n", i);
        connex->executeKernel("bubblesort");

        int res1 = connex->readReduction();
        printf("Now res1 = %d\n", res1);
        fflush(stdout);
        //
        int res2 = connex->readReduction();
        printf("Now res2 = %d\n", res2);
        //printf("res1 = %d, res2 = %d\n", res1, res2);
        fflush(stdout);

        if (res1 == 0 && res2 == 0) {
            printf("BubbleSortKernel(): Bailing out at iteration i = %d\n", i);
            fflush(stdout);
            break;
        }
    }


}


int BubblesortTest(ConnexMachine *connex) {
    uint16_t opA[CONNEX_VECTOR_LENGTH];
    //uint16_t opB[CONNEX_VECTOR_LENGTH];
    //uint16_t resCorrect[CONNEX_VECTOR_LENGTH];
    uint16_t res[CONNEX_VECTOR_LENGTH];
    int i;
    int opALSIndex = 0;
    int resLSIndex = 10;

    // Generate the input vector to be sorted
    for (i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        // Decreasing values:
        opA[i] = CONNEX_VECTOR_LENGTH - 1 - i;

        // Increasing values:
        //opA[i] = i;
    }

    // Write opA to LS memory from offset 0
    connex->writeDataToConnex(opA, 1, opALSIndex);


    BubbleSortKernel(connex, opALSIndex, resLSIndex);


    // Read res from LS memory from offset resLSIndex
    connex->readDataFromConnex(res, 1, resLSIndex);

    // Testing the vector is sorted correctly
    for (i = 0; i < CONNEX_VECTOR_LENGTH - 1; i++) {
        if (res[i] < res[i + 1])
            ;
        else {
            printf("res is NOT sorted on position i = %d and %d\n", i, i + 1);
            assert(res[i] < res[i + 1]);
        }
    }

    printf("After sorting, res =\n");
    for (i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        printf("%d\n", res[i]);
    }

    // Now execute the kernel that stops the simulator.
    connex->executeKernel("QuitKernel");



#ifdef LLVM_ISEL_CODEGEN
    string kernelName = "bubblesort";
    Kernel *kernel = connexGlobal->getKernel(kernelName);

    //kernel->genPrecomputedKernel();

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
}


void Test() {
    BubblesortTest(connexGlobal);
}

/*
int main(int argc, char *argv[]) {

    if (argc < 6) {
        printf("Usage: %s insn red iowr iord regs\n",argv[0]);
        return 0;
    }

    try {
        ConnexMachine *connex = new ConnexMachine(argv[1], argv[2], argv[3], argv[4], argv[5]);

        FloatRedTest(connex);

        delete connex;
    }
    catch(string err) {
        cout << err << endl;
    }
}
*/


