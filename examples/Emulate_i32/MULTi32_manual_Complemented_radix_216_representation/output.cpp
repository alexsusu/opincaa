// VERY IMPORTANT: CELL_SHR(Rsrc, vector_splat_1) moves value at index i to index i+1
/*
It seems that the code I wrote is by far the best for arbitrary values.
*/


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


//typedef long TYPE;
typedef int TYPE;
//typedef short TYPE;


// Uncommenting this puts in C the elements of abs(A)
#define ORIG_CODE
#define SIMPLE_BUT_MORE_POWER_CONSUMING

#define LLVM_ISEL_CODEGEN
#ifdef LLVM_ISEL_CODEGEN
  #define PRINTREG(regNum) regNum
  #define PrintDebugMessage(aStr) aStr
  #define PrintRegDebug(regNum) regNum
#endif



void MulInt32(TYPE *A, TYPE *B, TYPE *C, TYPE N) {
    // Alex: added manually
    N = N * sizeof(TYPE) / sizeof(short);
    printf("MulInt32(): adjusted N = %d\n", N);

    connexGlobal->writeDataToConnexPartial((&A [(0 +  0)]), /* actual num elems written */ (N), /*offset*/ 0);
    connexGlobal->writeDataToConnexPartial((&B [(0 +  0)]), /* actual num elems written */ (N), /*offset*/ 1 * (int)ceil(((float)(N))/CONNEX_VECTOR_LENGTH));
    _BEGIN_KERNEL(BatchNumberGlobal);
      EXECUTE_IN_ALL(



  #define REG_CT0  31
  #define REG_CT1  30
  #define REG_DST  29
  // Better to keep REG_SRC1 < REG_SRC2
  #define REG_SRC1 27
  #define REG_SRC2 28
  //
  #define REG_RES0_L 26
  #define REG_RES0_H 25
  #define REG_RES1_L 24


  // NOT required: #define REG_RES1_H 23

  #define REG_RES2_L 23
  //#define REG_RES2_H 21
  #define REG_RES3_L 22
  //#define REG_RES3_H 19

  #define REG_RESX 21
  #define REG_AUX 20
  //#define REG_SRC1_SH1 19
  //#define REG_SRC2_SH1 18
  //#define REG_RES_SH1 17

  // Odd indices show if we have a negative sign (1) or positive (0)
  //#define REG_SRC1_SGN 16
  //#define REG_SRC2_SGN 15

  //#define REG_SRCAUX 27
  #define REG_IDX     14
  #define REG_IDXMOD2 13
  #define REG_IDXPRED 12
  //#define REG_CRY 15


        R(REG_CT0) = 0;
        R(REG_CT1) = 1;

        R(0) = 0;
        R(1) = (N / CONNEX_VL) + ((N & (CONNEX_VL - 1)) > 0);
        R(2) = ((N / CONNEX_VL) + ((N & (CONNEX_VL - 1)) > 0)) * 2;

        R(3) = 1 ;
        R(4) = 0 ;


    // NOTE: N was already adjusted
    REPEAT_X_TIMES(((N / CONNEX_VL) + ((N & (CONNEX_VL - 1)) > 0) )   );

        R(REG_SRC1) = LS[R(0)];
        R(REG_SRC2) = LS[R(1)];



        R(1) = R(1) + R(3);
        R(0) = R(0) + R(3);


        PRINTREG(0);
        PRINTREG(1);

        PRINTREG(REG_SRC1);
        PRINTREG(REG_SRC2);




    // Alex: added manually

/*
To multiply two 32 bits integers md * mr we can work on their 16-bits subword integers:
    Note: md = multiplicand (REG_SRC2), mr = multiplier (REG_SRC1)
    res16_1 * 2^16 + res16_0 = (md16_1 * 2^16 + md16_0) * (mr16_1 * 2^16 + mr16_0);
    res16_1 * 2^16 + res16_0 = (sign: to be computed) ... + (md16_1 * mr16_0 + md16_0 * mr16_1) * 2^16 + md16_0 * mr16_0;
*/


  // START EMULATION MUL i32: From here starts the actual code doing the multiplication
  // R(REG_SRC2) is multiplicand, R(REG_SRC1) is multiplier



        R(REG_CT0) = 0;
        R(REG_CT1) = 1;

       // We perform the actual multiplication:
        MULT_U( R(REG_SRC2), R(REG_SRC1) );
        R(REG_RES0_L) = MULT_LOW();
        R(REG_RES0_H) = MULT_HIGH();
        PRINTREG(REG_RES0_L);
        PRINTREG(REG_RES0_H);

        // Computing md16_1 * mr16_0
        CELL_SHR(R(REG_SRC1), R(REG_CT1));
        NOP; // It is required
        R(REG_RES1_L) = SHIFT_REG;
        PRINTREG(REG_RES1_L);
        MULT_U( R(REG_RES1_L), R(REG_SRC2) );
        R(REG_RES1_L) = MULT_LOW();
        //R(REG_RES1_H) = MULT_HIGH();
        PRINTREG(REG_RES1_L);
        //PRINTREG(REG_RES1_H);


        // Computing md16_0 * mr16_1
        CELL_SHR(R(REG_SRC2), R(REG_CT1));
        NOP; // It is required
        R(REG_RES2_L) = SHIFT_REG;
        PRINTREG(REG_RES2_L);
        MULT_U(R(REG_RES2_L), R(REG_SRC1));
        R(REG_RES2_L) = MULT_LOW();
        PRINTREG(REG_RES2_L);


        // NOT necessary to compute md16_1 * mr16_1 - [END]


        CELL_SHR(R(REG_RES0_H), R(REG_CT1));
        NOP; // It is required
        R(REG_RESX) = SHIFT_REG;

        PRINTREG(REG_RES0_H);
        PRINTREG(REG_RESX);

      // We handle the odd indices:
        R(REG_IDX) = INDEX;
        R(REG_IDXMOD2) = R(REG_IDX) & R(REG_CT1);
        R(REG_IDXPRED) = R(REG_IDXMOD2) == R(REG_CT1);
        NOP;
      );

     PRINTREG(REG_RES1_L);
     PRINTREG(REG_RES2_L);
       EXECUTE_WHERE_EQ(
           R(REG_RES0_L) = R(REG_RESX) | R(REG_RESX); // High 16 bits are from RESX


           // We also add the contribution of the other 2 16-bit MULs we performed
           R(REG_RES0_L) = R(REG_RES0_L) + R(REG_RES1_L);
           R(REG_RES0_L) = R(REG_RES0_L) + R(REG_RES2_L);
       );


  // END EMULATION MUL i32: Here ends the actual code doing the multiplication

       EXECUTE_IN_ALL(
           LS[R(2)] = R(REG_RES0_L);

           R(2) = R(2) + R(3);
    END_REPEAT;

           REDUCE R(0); // We add a 'bogus' REDUCE to wait for it
       );
    // This also works, but it's NOT nice: __kernel->sdNodeVarNameRegDef[REG_SRC1] = "nodeOpSrcCast1";
    _END_KERNEL(BatchNumberGlobal);



  #ifdef LLVM_ISEL_CODEGEN
    Kernel *kernel = connexGlobal->getKernel(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
    //connexGlobal->sdNodeVarNameRegDef[REG_SRC1] = "nodeOpSrcCast1";
    kernel->sdNodeVarNameRegDef[REG_SRC1] = "nodeOpSrcCast1";
    kernel->sdNodeVarNameRegDef[REG_SRC2] = "nodeOpSrcCast2";
    //#define OFFSET_INSTRUCTIONS_TO_START_CODEGEN (11 + 1)
    //
    // For MUL_i32 (both simple and power efficient versions):
    //connexGlobal->OFFSET_INSTRUCTIONS_TO_START_CODEGEN = 11 + 1;
    kernel->offsetKernelToStartCodegenFrom = 13 + 1; // +1 for the END_WHERE instruction Opincaa adds automatically to each kernel
    //#define NUM_INSTRUCTIONS_TO_CODEGEN 99
    //connexGlobal->NUM_INSTRUCTIONS_TO_CODEGEN = 99;
    kernel->numInstructionsToCodegen = 27;
    //
    // We use chain, since with glue with get a lot or weird scheduling errors:
    kernel->useGlue = 0;
    // IMPORTANT: to convert in 'partly SSA form' we require ~64 registers
    assert(CONNEX_REG_COUNT != 32);

    printf("Calling connexGlobal->genLLVMISelManualCode()\n");
    fflush(stdout);
    string resGenLLVM = connexGlobal->genLLVMISelManualCode(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
    printf("resGenLLVM = %s\n", resGenLLVM.c_str());
    fflush(stdout);

    printf("Calling connexGlobal->dumpKernel()\n");
    fflush(stdout);
    string resDump = connexGlobal->dumpKernel(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
    printf("resDump = %s\n", resDump.c_str());
    fflush(stdout);

    printf("Calling connexGlobal->disassembleKernel()\n");
    fflush(stdout);
    string resDis = connexGlobal->disassembleKernel(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
    printf("resDis = %s\n", resDis.c_str());
    fflush(stdout);
  #endif

    connexGlobal->executeKernel(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
    connexGlobal->readReduction();
    printf("Returned from connexGlobal->readReduction()\n");
    fflush(stdout);

#ifdef ORIG_CODE
    // NOTE: N was already adjusted
    connexGlobal->readDataFromConnexPartial((&C [(0 +  0)]),
                /* actual num elems read */ (N),
                /*offset*/ 2 * (int)ceil(((float)(N))/CONNEX_VECTOR_LENGTH));
#else
    // NOTE: N was already adjusted
    connexGlobal->readDataFromConnexPartial((&C [(0 +  0)]),
                /* actual num elems read */ (N),
                /*offset*/ 0); // 0 is offset of A
#endif
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
    //#define NUM_ELEMS 64

   #define NUM_ELEMS (CONNEX_VECTOR_LENGTH/2)
   //#define NUM_ELEMS (CONNEX_VECTOR_LENGTH/2) * 10

    TYPE A[NUM_ELEMS + 100];
    TYPE B[NUM_ELEMS + 100];
    TYPE C[NUM_ELEMS + 100];


    //srand(time(NULL));
    srand(0);


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
        // TODO TODO TODO Works very well (the i16 numbers are positive, but the + can enable carry):
        A[i] = ((rand() % 32768) << 16) + (rand() % 32768);
        B[i] = ((rand() % 32768) << 16) + (rand() % 32768);
        */


/*
// Works well:
        //A[i] = 65536;
        A[i] = 6;
        //A[i] = 65536;
        B[i] = 16384;
*/
// B is multiplicand, A is multiplier

    // NOT true: signed multiplication is different from UNsigned:

/*
// TODO TODO TODO TODO TODO: finish this case - it seems we need to take into consideration the sign of the i32's
// We are doing signed multiplication on Connex and this is BAD for the following case, where we have A and B signed, but they are actually > 0.
        A[i] = 65535;
        B[i] = 255;
Important note:
    Example: when
        A[i] = 65535;
        B[i] = 255;
    -1 * 255 = -255, which is 0xffffff01

// 255 * 65535 = 16711425
// 65535 - 254  + 65535 = 130816
*/


/*
// Works well:
        A[i] = 1020;
        B[i] = 255;
*/

/*
    // GOOD:
        A[i] = rand() % 32768;
        B[i] = rand() % 32768;
*/


/*
    // GOOD:
        A[i] = rand() % 65536;
        B[i] = rand() % 65536;
*/

/*
   // GOOD:
        A[i] = rand() % 1000000000;
        B[i] = rand() % 1000000000;
*/

/*
        A[i] = rand() % 1000000000 - 1000000000;
        B[i] = rand() % 1000000000 - 1000000000;
        */

/*
        // GOOD:
        A[i] = rand() % 1000000000 - 500000000;
        B[i] = rand() % 1000000000 - 500000000;
*/

        // GOOD:
        A[i] = rand();
        B[i] = rand();


/*
        A[i] = 0x000049fc;
        B[i] = 0x00002360;
*/

//        A[i] = ((rand() % 4) << 30) + ((rand() % 32768) << 15) + (rand() % 32768);
//        B[i] = ((rand() % 4) << 30) + ((rand() % 32768) << 15) + (rand() % 32768);

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

    printf("Calling MulInt32 (with i32 result)...\n");
    fflush(stdout);

    MulInt32(A, B, C, NUM_ELEMS);

    printf("Finished executing the Opincaa kernel.\n");
    fflush(stdout);
    //
    printf("Testing the correctness of the computation of the Opincaa kernel.\n");
    fflush(stdout);


    #define FAIL -1
    #define PASS 0

    testResult = PASS;
    for (i = 0; i < NUM_ELEMS; i++) {
        if (A[i] * B[i] != C[i]) {
            testResult = FAIL;
            break;
        }
    }

    printf("  testResult = %d (PASS = %d)\n", testResult, PASS);

    //if (testResult == FAIL) {
        printf("NUM_ELEMS = %d\n", NUM_ELEMS);
        for (i = 0; i < NUM_ELEMS + 5; i++)
            if ((i < NUM_ELEMS) && (A[i] * B[i] != C[i]))
                printf("C[%d] = 0x%08x ( != 0x%08x)\n", i, C[i], A[i] * B[i]);
            else
                printf("C[%d] = %d (0x%08x)\n", i, C[i], C[i]);
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

