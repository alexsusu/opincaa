#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


//typedef long TYPE;
typedef int TYPE;
//typedef short TYPE;


#define LLVM_ISEL_CODEGEN
#ifdef LLVM_ISEL_CODEGEN
  #define PRINTREG(regNum) regNum
  #define PrintRegDebug(regNum) regNum
  #define PrintDebugMessage(regNum) regNum
#endif



//#define PROBABLY_MORE_ENERGY_EFFICIENT


void SHRAInt32(TYPE *A, TYPE *B, TYPE *C, TYPE N) {
  #define REG_CT0  31
  #define REG_CT1  30
  #define REG_CT16  10
  #define REG_CT31   8
  //#define REG_CT01  9
  #define REG_DST  29
  // Better to have REG_SRC1 < REG_SRC2
  #define REG_SRC1 28
  #define REG_SRC2 27
  //#define REG_SRCAUX 27
  #define REG_IDX 26
  #define REG_IDXMOD2 25
  #define REG_IDXPRED 24
  #define REG_IDXPRED2 19
  #define REG_SRC1_MSWORD 23
  // We define these register to have "SSA-form" (single assignment for each register)
  #define REG_AUX  22
  #define REG_AUX2  20
  #define REG_SRC2_16 21


    // Alex: added manually
    N = N * sizeof(TYPE) / sizeof(short);
    printf("SHRAInt32(): adjusted N = %d\n", N);

    connexGlobal->writeDataToConnexPartial((&A [(0 +  0)]), /*numVectors*/ (int)ceil(((float)(N)) / CONNEX_VECTOR_LENGTH), /* actual num elems written */ (N), /*offset*/ 0);
    connexGlobal->writeDataToConnexPartial((&B [(0 +  0)]), /*numVectors*/ (int)ceil(((float)(N)) / CONNEX_VECTOR_LENGTH), /* actual num elems written */ (N), /*offset*/ 1 * (int)ceil(((float)(N))/CONNEX_VECTOR_LENGTH));
    _BEGIN_KERNEL(BatchNumberGlobal);
      EXECUTE_IN_ALL(
        R(REG_CT0) = 0;
        R(REG_CT1) = 1;



        R(0) = 0;
        R(1) = (N / CONNEX_VL) + ((N & (CONNEX_VL - 1)) > 0);
        R(2) = ((N / CONNEX_VL) + ((N & (CONNEX_VL - 1)) > 0)) * 2;

        R(3) = 1;
        R(4) = 0;

        REPEAT_X_TIMES( (N / CONNEX_VL) + ((N & (CONNEX_VL - 1)) > 0) );
        //REPEAT_X_TIMES(1);

        R(REG_SRC1) = LS[R(0)];
        R(REG_SRC2) = LS[R(1)];







     // See http://llvm.org/docs/LangRef.html#ashr-instruction and https://en.wikipedia.org/wiki/Arithmetic_shift
     // IMPORTANT: We compute SHRA.i32(R(REG_SRC1), R(REG_SRC2)) (considered to be both vectors of CONNEX_VECTOR_LENGTH/2 x i32)

    // Alex: added manually
        R(REG_CT0) = 0;
        R(REG_CT1) = 1;
        R(REG_CT16) = 16;
        R(REG_CT31) = 31;

        //PrintRegDebug(REG_CT1);

        // At least on x86 the >> C operator takes the second operand modulo 32
        R(REG_SRC2) = R(REG_SRC2) & R(REG_CT31);

        //PRINTREG(REG_SRC1);
        PrintRegDebug(REG_SRC1);
        PrintRegDebug(REG_SRC2);

        /*
          We need to treat both cases :
            - the case we shift at most 16 positions;
            - the case we shift more than 16 positions.

          We do this in a somewhat unified way.
            First, for all odd positions with 16 < R(REG_SRC2) we make R(REG_SRC2) = 16.
            We also do quite a bit of simple CSE.

           Then we do the SHRA for the REG_SRC1 - for both odd and even positions,
             although it would be best (from the energy perspective to do this ONLY for
              the odd positions because for the even positions we want to have simple SHR).

          For the case we shift at most 16 positions:
              We put in REG_SRC1_MSWORD the 1-position-left-shifted REG_SRC1.
              Then, we compute:
                   - the SHL by 16-REG_SRC2 of REG_SRC1 ONLY for even positions
                        and then we OR this result with the result of a
                        SHR (REG_SRC1) >> REG_SRC2).

          For the case we shift more than 16 positions:
            - we put for each even lane in REG_DST the value of REG_SRC1_MSWORD
                 SHRA by 16 - REG_SRC2.
        */


        // Computing it to be used a few times
        //R(REG_CT01) = INDEX;
        //R(REG_CT01) = R(REG_CT01) & R(REG_CT1);

        R(REG_IDXMOD2) = INDEX;
        R(REG_IDXMOD2) = R(REG_IDXMOD2) & R(REG_CT1);

        /* Adapting REG_SRC2 - we propagate the 2nd operand of SHRA (REG_SRC2)
             in the most significant i16 word of each i32. */
        CELL_SHR(R(REG_SRC2), R(REG_IDXMOD2));
        // It is required:
        NOP;
        R(REG_SRC2) = SHIFT_REG;
        PrintRegDebug(REG_SRC2);

        R(REG_AUX2) = R(REG_CT16) < R(REG_SRC2);
/*
  // TODO TODO TODO: check on real Connex on Zedboard
  // For all odd positions with 16 < R(REG_SRC2) we make R(REG_SRC2) = 16:
        R(REG_AUX2) = R(REG_CT16) < R(REG_SRC2);
        //R(REG_AUX2) = R(REG_CT1) - R(REG_AUX2);
        R(REG_IDXPRED) = R(REG_IDXMOD2) == R(REG_CT1);
        R(REG_IDXPRED) = R(REG_AUX2) & R(REG_IDXPRED);
      //PrintRegDebug(REG_IDXPRED);
        R(REG_IDXPRED2) = R(REG_IDXPRED) == R(REG_CT1);
      //PrintRegDebug(REG_IDXPRED2);
        NOP;
       );
       EXECUTE_WHERE_EQ(
        R(REG_SRC2) = R(REG_CT16);
        PrintDebugMessage("After adjusting:");
        PrintRegDebug(REG_SRC2);
       );

       EXECUTE_IN_ALL(
*/
        R(REG_DST) = SHRA(R(REG_SRC1), R(REG_SRC2));

        PrintRegDebug(REG_DST);


      //PrintRegDebug(REG_CT1);

        // Computing REG_SRC1_MSWORD ("most significant 16-bit word" of the i32 of
        //                              REG_SRC1 in position 0)
        CELL_SHL(R(REG_SRC1), R(REG_CT1));
        // It is required:
        NOP;
        R(REG_SRC1_MSWORD) = SHIFT_REG;
        //PRINTREG(REG_SRC1_MSWORD);
        PrintRegDebug(REG_SRC1_MSWORD);

        // R(REG_IDX) = INDEX;
        // R(REG_IDXMOD2) = R(REG_IDX) & R(REG_CT1);
        // R(REG_IDXPRED) = R(REG_IDXMOD2) == R(REG_CT0);



  // Now, for all even positions:
    // For all even positions with 16 < R(REG_SRC2):
        R(REG_IDXMOD2) = R(REG_IDXMOD2) == R(REG_CT0);
        R(REG_IDXPRED) = R(REG_AUX2) & R(REG_IDXMOD2);
      //PrintRegDebug(REG_IDXPRED);
        R(REG_IDXPRED2) = R(REG_IDXPRED) == R(REG_CT1);
        PrintDebugMessage("Case > 16:");
        PrintRegDebug(REG_IDXPRED2);
        NOP;
       );
       EXECUTE_WHERE_EQ( // For all even positions with 16 < R(REG_SRC2)
          //R(REG_DST) = R(REG_AUX) | R(REG_DST);
          R(REG_SRC2_16) = R(REG_SRC2) - R(REG_CT16);
          R(REG_DST) = SHRA(R(REG_SRC1_MSWORD), R(REG_SRC2_16));
        PrintDebugMessage("Case > 16:");
        PrintRegDebug(REG_DST);
       );


       EXECUTE_IN_ALL(
   // For all even positions with R(REG_SRC2) <= 16:
        R(REG_AUX2) = R(REG_CT1) - R(REG_AUX2);
        R(REG_IDXPRED) = R(REG_AUX2) & R(REG_IDXMOD2);
      PrintRegDebug(REG_IDXPRED);
        R(REG_IDXPRED2) = R(REG_IDXPRED) == R(REG_CT1);
        PrintDebugMessage("Case < 16:");
      //PrintRegDebug(REG_IDXPRED2);
      //PrintRegDebug(REG_SRC2);
      //PrintRegDebug(REG_SRC1_MSWORD);
        NOP;
       );
       EXECUTE_WHERE_EQ( // For all even positions
          R(REG_SRC2_16) = R(REG_CT16) - R(REG_SRC2);

          //R(REG_AUX) = ISHL(R(REG_SRC1_MSWORD), R(REG_SRC2_16));
          R(REG_AUX) = R(REG_SRC1_MSWORD) << R(REG_SRC2_16);

        PrintRegDebug(REG_AUX);
          R(REG_DST) = R(REG_SRC1) >> R(REG_SRC2);
        PrintRegDebug(REG_DST);
          R(REG_DST) = R(REG_DST) | R(REG_AUX);
       );


       EXECUTE_IN_ALL(
        PrintRegDebug(REG_SRC1);
        PrintRegDebug(REG_SRC2);
        //PRINTREG(REG_DST);
        PrintRegDebug(REG_DST);


        NOP;
        LS[R(2)] = R(REG_DST);

        // IMPORTANT: Here we do stripmining for the REPEAT loop
        R(1) = R(1) + R(3);
        R(0) = R(0) + R(3);

        R(2) = R(2) + R(3);

        END_REPEAT;

        REDUCE R(0); // We add a 'bogus' REDUCE to wait for it
       );
    _END_KERNEL(BatchNumberGlobal);


  #ifdef LLVM_ISEL_CODEGEN
    Kernel *kernel = connexGlobal->getKernel(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
    //connexGlobal->sdNodeVarNameRegDef[REG_SRC1] = "nodeOpSrcCast1";
    kernel->sdNodeVarNameRegDef[REG_SRC1] = "nodeOpSrcCast1";
    kernel->sdNodeVarNameRegDef[REG_SRC2] = "nodeOpSrcCast2";
    //#define offsetKernelToStartCodegenFrom (11 + 1)
    //
    // For MUL_i32 (both simple and power efficient versions):
    //connexGlobal->offsetKernelToStartCodegenFrom = 11 + 1;
    kernel->offsetKernelToStartCodegenFrom = 11 + 1; // +1 for the END_WHERE instruction Opincaa adds automatically to each kernel
    kernel->numInstructionsToCodegen = 41 - 7 - 1;

    //
    // We use chain, since with glue we get a lot of weird scheduling errors:
    kernel->useGlue = 0;
    //kernel->useGlue = 1;
    //
    // IMPORTANT: to convert in 'partly SSA form' we require ~64 registers
    assert(CONNEX_REG_COUNT != 32);

    printf("Calling connexGlobal->genLLVMISelManualCode()\n");
    fflush(stdout);
    string resGenLLVM = connexGlobal->genLLVMISelManualCode(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
    printf("resGenLLVM = \n%s\n", resGenLLVM.c_str());
    fflush(stdout);

    printf("Calling connexGlobal->dumpKernel()\n");
    fflush(stdout);
    string resDump = connexGlobal->dumpKernel(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
    printf("resDump = \n%s\n", resDump.c_str());
    fflush(stdout);

    printf("Calling connexGlobal->disassembleKernel()\n");
    fflush(stdout);
    string resDis = connexGlobal->disassembleKernel(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
    printf("resDis = \n%s\n", resDis.c_str());
    fflush(stdout);
  #endif

    connexGlobal->executeKernel(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
    connexGlobal->readReduction();
    connexGlobal->readDataFromConnexPartial((&C [(0 +  0)]), /*numVectors*/ (int)ceil(((float)(N)) / CONNEX_VECTOR_LENGTH), /* actual num elems read */ (N), /*offset*/ 2 * (int)ceil(((float)(N))/CONNEX_VECTOR_LENGTH));
}



//#define TEST_OPINCAA_CONNEX_KERNELS

#ifdef TEST_OPINCAA_CONNEX_KERNELS
//int Test_Array_map(TYPE *A, TYPE *B, TYPE *C, int N) {
int Test() {
    //#define NUM_ELEMS 1000000
    //#define NUM_ELEMS 10000
    //#define NUM_ELEMS 8192
    //#define NUM_ELEMS 1024
    //#define NUM_ELEMS 128
    #define NUM_ELEMS 64

    TYPE A[NUM_ELEMS + 100];
    TYPE B[NUM_ELEMS + 100];
    TYPE C[NUM_ELEMS + 100];


    srand(time(NULL));
    //srand(0);


    int i, testResult;

    printf("Entered Test()\n");
    printf("  NUM_ELEMS = %d (sizeof(TYPE) = %lu)\n", NUM_ELEMS, sizeof(TYPE));

    printf("RAND_MAX = %d\n", RAND_MAX);
    // We assume RAND_MAX = 2,147,483,647 (before it was RAND_MAX = 32767)

    assert(RAND_MAX == 2147483647);

    for (i = 0; i < NUM_ELEMS; i++) {
        //A[i] = 3; // i;
        //A[i] = 40000; // i;

        /*
        A[i] = 32767; // i;
        B[i] = 32767; // i / 2;
        */

        //A[i] = 42767; // i;
        //B[i] = 32767; // i / 2;

        //A[i] = 327;
        //B[i] = 2767;

        //A[i] = i;
        //B[i] = i / 2;

        /*
        // Works very well (the i16 numbers are positive, but the + can enable carry):
        A[i] = ((rand() % 32768) << 16) + (rand() % 32768);
        B[i] = ((rand() % 32768) << 16) + (rand() % 32768);
        */

        A[i] = ((rand() % 4) << 30) + ((rand() % 32768) << 15) + (rand() % 32768);
        //B[i] = ((rand() % 4) << 30) + ((rand() % 32768) << 15) + (rand() % 32768);

        //B[i] = 6;
        //B[i] = 20;
        //B[i] = 31;
        //B[i] = rand() % 17;
        //B[i] = rand() % 40; //31;
        B[i] = rand();

        /*
        // Works on x64
        A[i] = ((rand() % 2) << 31) + (rand());
        B[i] = ((rand() % 2) << 31) + (rand());
        */

        /*
        A[i] = rand() % 100000000;
        B[i] = rand() % 100000000;
        */

        /*
        // Works very well:
        A[i] = -1;
        B[i] = -1;
        */

        /*
        if (i == 0) {
            A[i] = 6106073;
            B[i] = 28094237;
        }
        else {
            A[i] = 0;
            B[i] = 0;
        }
        */


        /*
        if (i & 1 == 0) {
            A[i] = rand() % (RAND_MAX + 1);
            B[i] = rand() % (RAND_MAX + 1);
        }
        else {
            A[i] = rand() % (RAND_MAX + 1) - 32768;
            B[i] = rand() % (RAND_MAX + 1) - 32768;
        }
        */
        //C[i] = -1;


        printf("A[%d] = 0x%08x\n", i, A[i]);
        printf("B[%d] = 0x%08x\n", i, B[i]);
    }

    printf("Calling SHRAInt32 (with i32 result)...\n");
    fflush(stdout);

    SHRAInt32(A, B, C, NUM_ELEMS);

    printf("Finished executing the Opincaa kernel.\n");
    fflush(stdout);
    //
    printf("Testing the correctness of the computation of the Opincaa kernel.\n");
    fflush(stdout);


    #define FAIL -1
    #define PASS 0

    testResult = PASS;
    for (i = 0; i < NUM_ELEMS; i++) {
        int tmp;
        tmp = A[i] >> B[i];
        //if (A[i] >> B[i] != C[i]) {
        if (tmp != C[i]) {
            testResult = FAIL;
            break;
        }
    }

    printf("  testResult = %d (PASS = %d)\n", testResult, PASS);

    //if (testResult == FAIL) {
        printf("NUM_ELEMS = %d\n", NUM_ELEMS);
        for (i = 0; i < NUM_ELEMS + 5; i++)
            if ((i < NUM_ELEMS) && ((A[i] >> B[i]) != C[i])) {
                /* From https://en.wikipedia.org/wiki/Arithmetic_shift:
                  "The >> operator in C and C++ is not necessarily an arithmetic shift.
                   Usually it is only an arithmetic shift if used with a signed integer type on its left-hand side.
                   If it is used on an unsigned integer type instead, it will be a logical shift."

                 A[i] and B[i] are int so >> is shra.
                */
                printf("C[%d] = 0x%08x ( != 0x%08x)\n", i, C[i], A[i] >> B[i]);
            }
            else
                printf("C[%d] = %d\n", i, C[i]);
    //}

    printf("Exiting Test()\n");

    return testResult;
}

/*
int main() {
    Test();

    return 0;
}
*/

#endif

