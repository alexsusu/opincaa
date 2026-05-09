#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


//typedef long TYPE;
typedef int TYPE;
//typedef short TYPE;


//#define LLVM_ISEL_CODEGEN
#ifdef LLVM_ISEL_CODEGEN
  #define PRINTREG(regNum) regNum
#endif



#define PROBABLY_MORE_ENERGY_EFFICIENT


void SubInt32(TYPE *A, TYPE *B, TYPE *C, TYPE N) {
  #define REG_CT0  31
  #define REG_CT1  30
  #define REG_DST  29
  // Keep REG_SRC1 < REG_SRC2
  #define REG_SRC1 27
  #define REG_SRC2 28
  //#define REG_SRCAUX 27
  #define REG_IDX 26
  #define REG_IDXMOD2 25
  #define REG_IDXPRED 24
  #define REG_CRY 23


    // Alex: added manually
    N = N * sizeof(TYPE) / sizeof(short);
    printf("SubInt32(): adjusted N = %d\n", N);

    connexGlobal->writeDataToConnexPartial((&A [(0 +  0)]), /*numVectors*/ (int)ceil(((float)(N)) / CONNEX_VECTOR_LENGTH), /* actual num elems written */ (N), /*offset*/ 0);
    connexGlobal->writeDataToConnexPartial((&B [(0 +  0)]), /*numVectors*/ (int)ceil(((float)(N)) / CONNEX_VECTOR_LENGTH), /* actual num elems written */ (N), /*offset*/ 1 * (int)ceil(((float)(N))/CONNEX_VECTOR_LENGTH));
    _BEGIN_KERNEL(BatchNumberGlobal);
      EXECUTE_IN_ALL(
        R(REG_CT0) = 0;
        R(REG_CT1) = 1;



        R(0) = 0;
        R(1) = (N / CONNEX_VL) + ((N & (CONNEX_VL - 1)) > 0);
        R(2) = ((N / CONNEX_VL) + ((N & (CONNEX_VL - 1)) > 0)) * 2;

        R(3) = 1 ; // MSA_I10           // <MCInst #111 VLOAD_D
        R(4) = 0 ; // MSA_I10           // <MCInst #111 VLOAD_D

        REPEAT_X_TIMES( (N / CONNEX_VL) + ((N & (CONNEX_VL - 1)) > 0) );
        //REPEAT_X_TIMES(1);

        R(REG_SRC1) = LS[R(0)];
        R(REG_SRC2) = LS[R(1)];







       // IMPORTANT: We compute R(REG_SRC1) - R(REG_SRC2) (considered to be both vectors of CONNEX_VECTOR_LENGTH/2 x i32)
        R(REG_CT0) = 0;
        R(REG_CT1) = 1;

    // Alex: added manually
        PRINTREG(REG_SRC1);
        PRINTREG(REG_SRC2);
        R(REG_DST) = R(REG_SRC1) - R(REG_SRC2) ;
        PRINTREG(REG_DST);


    // Alex: added manually
        //EXECUTE_WHERE_CRY
/* We simply want to retrieve the carry flag in a register, because we want to CELL_SHR the carry.
  The data is stored in little-endian mode:
     |El 0 - Least significant 16 bits|El 0 - Most significant 16 bits|El 1 - Least significant 16 bits|El 1 - Most significant 16 bits|...
     For all odd positions above (the most significant 16 bits of each Element), we add the carry of the Less significant 16 bits of the respective Element.
*/
       #ifdef PROBABLY_MORE_ENERGY_EFFICIENT
        R(REG_CRY) = ADDC(R(REG_CT0), R(REG_CT0));
       #else
        R(REG_CRY) = SUBC(R(REG_CT0), R(REG_CT0));
       #endif
        PRINTREG(REG_CRY);

       // We zero out all the odd indices in REG_CRY:
           R(REG_IDX) = INDEX;
           R(REG_IDXMOD2) = R(REG_IDX) & R(REG_CT1);
           R(REG_IDXPRED) = R(REG_IDXMOD2) == R(REG_CT1);
           NOP;
       )
       EXECUTE_WHERE_EQ(
         #ifdef PROBABLY_MORE_ENERGY_EFFICIENT
           R(REG_CRY) = 0;
         #else
           R(REG_CRY) = R(REG_CT0) | R(REG_CT0);
         #endif
       )
       PRINTREG(REG_CRY);

       EXECUTE_IN_ALL(
         CELL_SHR(R(REG_CRY), R(REG_CT1));
         // It is required:
         NOP;
         R(REG_CRY) = SHIFT_REG;

         PRINTREG(REG_CRY);

       #ifdef PROBABLY_MORE_ENERGY_EFFICIENT
         R(REG_DST) = R(REG_DST) - R(REG_CRY);
       #else
         R(REG_DST) = R(REG_DST) + R(REG_CRY);
       #endif

    #ifdef A_BIT_WRONG_SPEC
        /*
        Rres = ADD Rsrc1, Rsrc2
        Raddmore = 0
        WHERE INDEX & 1 == 0
            WHERECRY
                Raddmore = 1
            ENDWHERE
        ENDWHERE
        Raddmore = CELLSHR Raddmore, 1
        Rres = Rres + Raddmore
        */
    #endif

        NOP  ; // scalar or vector NOP // <MCInst #82 NOP>
        LS[R(2)] = R(REG_DST); // WRITE (scatter) // <MCInst #105 ST_INDIRECT_D

        // IMPORTANT: Here we do stripmining for the REPEAT loop
        R(1) = R(1) + R(3) ; // MSA_3R generic instruction // <MCInst #27 ADDV_D
        R(0) = R(0) + R(3) ; // MSA_3R generic instruction // <MCInst #27 ADDV_D

        R(2) = R(2) + R(3); // MSA_3R generic instruction // <MCInst #27 ADDV_D

        END_REPEAT; // END_REPEAT       // <MCInst #41 END_REPEAT_D>

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
    //#define numInstructionsToCodegen 99
    kernel->numInstructionsToCodegen = 15;
    //
    /* We use chain, since with glue with get a lot or weird scheduling errors:
    kernel->useGlue = 0;
    */
    kernel->useGlue = 1;
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

    connexGlobal->executeKernel(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
    connexGlobal->readReduction();
    connexGlobal->readDataFromConnexPartial((&C [(0 +  0)]), /*numVectors*/ (int)ceil(((float)(N)) / CONNEX_VECTOR_LENGTH), /* actual num elems read */ (N), /*offset*/ 2 * (int)ceil(((float)(N))/CONNEX_VECTOR_LENGTH));
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
        B[i] = ((rand() % 4) << 30) + ((rand() % 32768) << 15) + (rand() % 32768);

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

    printf("Calling SubInt32 (with i32 result)...\n");
    fflush(stdout);

    SubInt32(A, B, C, NUM_ELEMS);

    printf("Finished executing the Opincaa kernel.\n");
    fflush(stdout);
    //
    printf("Testing the correctness of the computation of the Opincaa kernel.\n");
    fflush(stdout);



    #define FAIL -1
    #define PASS 0
    testResult = PASS;
    for (i = 0; i < NUM_ELEMS; i++) {
        if (A[i] - B[i] != C[i]) {
            testResult = FAIL;
            break;
        }
    }

    //if (testResult == FAIL) {
        printf("NUM_ELEMS = %d\n", NUM_ELEMS);
        for (i = 0; i < NUM_ELEMS + 5; i++)
            if ((i < NUM_ELEMS) && (A[i] - B[i] != C[i]))
                printf("C[%d] = 0x%08x ( != 0x%08x); A[i] = %d, B[i] = %d\n", i, C[i], A[i] - B[i], A[i], B[i]);
            else
                printf("C[%d] = 0x%08x (%d); A[i] = %d, B[i] = %d\n", i, C[i], C[i], A[i], B[i]);
    //}



    printf("  testResult = %d (PASS = %d)\n", testResult, PASS);


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

