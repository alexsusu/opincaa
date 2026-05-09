/*
TODO TODO TODO TODO: put in R(SRC) and R(SRCAUX) only useful values (NO 0), by reading 2 vector lines and unpacking them properly in R(SRC) and R(SRCAUX).

The REDuction on i32 is computed by:
    - recovering the least significant i16 of the i32 and sending them to reduce
    - recovering the most significant i16 of the i32 and sending them to reduce
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define FEWER_INSTRUCTIONS_AND_PROBABLY_MORE_ENERGY_EFFICIENT



//#define LLVM_ISEL_CODEGEN
#ifdef LLVM_ISEL_CODEGEN
  #define PRINTREG(regNum) regNum
  #define PrintDebugMessage(aStr) aStr
  #define PrintRegDebug(regNum) regNum
#endif



//typedef long TYPE;
typedef int TYPE;
//typedef short TYPE;



// The algorithm is simple - we rely on simple + op of twos complement - no need to worry

TYPE ReduceInt32(TYPE *A, TYPE *B, TYPE *C, TYPE N) {
  #define REG_CT0 30
  #define REG_CT1 29
  #define REG_SRC 28
  #define REG_SRCAUX 27
  #define REG_IDX 26
  #define REG_IDXMOD2 25
  #define REG_IDXPRED 24


    N = N * sizeof(TYPE) / sizeof(short);
    printf("ReduceInt32(): adjusted N = %d\n", N);

    connexGlobal->writeDataToConnexPartial((&A [(0 +  0)]),
                                           /* actual num elems written */ (N),
                                           /*offset*/ 0);
    connexGlobal->writeDataToConnexPartial((&B [(0 +  0)]),
                                           /* actual num elems written */ (N),
                                           /*offset*/ 1 * (int)ceil(((float)(N))/CONNEX_VECTOR_LENGTH));

    _BEGIN_KERNEL(BatchNumberGlobal);
      EXECUTE_IN_ALL(
        //R(REG_CT0) = 0;
        R(REG_CT1) = 1;
        //R(15) = 0;
        //R(17) = 0;



        R(0) = 0;
        //R(1) = (N / CONNEX_VL) + ((N & (CONNEX_VL - 1)) > 0);
        //R(2) = ((N / CONNEX_VL) + ((N & (CONNEX_VL - 1)) > 0)) * 2;

        R(3) = 1;
        R(4) = 0;

       REPEAT_X_TIMES( (N / CONNEX_VL) + ((N & (CONNEX_VL - 1)) > 0) );
        //REPEAT_X_TIMES(1);

        R(REG_SRC) = LS[R(0)];
        //R(6) = LS[R(1)];



        R(0) = R(0) + R(3);
        //R(1) = R(1) + R(3);

       END_REPEAT;

    //PRINTREG(0);
        //PRINTREG(1);


       // IMPORTANT: We reduce R(REG_SRC) (considered to be a vector of CONNEX_VECTOR_LENGTH/2 x i32)

       // We keep a copy of R(REG_SRC) for reduction on most-significant i16 half-words
       //R(REG_SRCAUX) = R(REG_SRC) | R(REG_SRC);


// NOTE: From here we perform RED i32
        R(REG_CT1) = 1;
      #ifndef FEWER_INSTRUCTIONS_AND_PROBABLY_MORE_ENERGY_EFFICIENT
        R(REG_CT0) = 0;
      #endif

// TODO TODO TODO: we could use instead of CELL_SHR another WHERE_EQ that sets on even index positions to 0
        //CELL_SHR(R(REG_SRCAUX), R(REG_CT1));
        CELL_SHR(R(REG_SRC), R(REG_CT1));
        NOP; // IMPORTANT: It is required:
        R(REG_SRCAUX) = SHIFT_REG;

        //PRINTREG(REG_SRC);
        //PRINTREG(REG_SRCAUX);

    // Alex: added manually

      // We zero out the odd indices:
           R(REG_IDX) = INDEX;
           R(REG_IDXMOD2) = R(REG_IDX) & R(REG_CT1);
           R(REG_IDXPRED) = R(REG_IDXMOD2) == R(REG_CT1);
           NOP;
       )
       EXECUTE_WHERE_EQ(

         #ifdef FEWER_INSTRUCTIONS_AND_PROBABLY_MORE_ENERGY_EFFICIENT
           R(REG_SRC) = 0;
           R(REG_SRCAUX) = 0;
         #else
           R(REG_SRC) = R(REG_CT0) | R(REG_CT0);
           R(REG_SRCAUX) = R(REG_CT0) | R(REG_CT0);
         #endif
       )
       PRINTREG(REG_SRC);
       PRINTREG(REG_SRCAUX);


       EXECUTE_IN_ALL(
           //NOP;
           //LS[R(2)] = R(15);

           // IMPORTANT: Here we do stripmining for the REPEAT loop
           // R(15) = R(15) + R(REG_SRC);
           // R(17) = R(17) + R(REG_SRCAUX);

           //R(0) = R(0) + R(1);
//           END_REPEAT;

           //REDUCE R(15);
           //REDUCE R(17);

           REDUCE_U R(REG_SRC);
           REDUCE_U R(REG_SRCAUX);
       );
    _END_KERNEL(BatchNumberGlobal);



  #ifdef LLVM_ISEL_CODEGEN
    Kernel *kernel = connexGlobal->getKernel(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
    //connexGlobal->sdNodeVarNameRegDef[REG_SRC1] = "nodeOpSrcCast1";
    kernel->sdNodeVarNameRegDef[REG_SRC] = "nodeOpSrcCast";
    //kernel->sdNodeVarNameRegDef[REG_SRC2] = "nodeOpSrcCast2";
    //#define offsetKernelToStartCodegenFrom (11 + 1)
    //
    // For MUL_i32 (both simple and power efficient versions):
    //connexGlobal->offsetKernelToStartCodegenFrom = 11 + 1;
    kernel->offsetKernelToStartCodegenFrom = 10 + 1; // +1 for the END_WHERE instruction Opincaa adds automatically to each kernel
    kernel->numInstructionsToCodegen = 14;
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
  #endif


    connexGlobal->executeKernel(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
    int res0 = connexGlobal->readReduction();
    int res1 = connexGlobal->readReduction();
    //connexGlobal->readDataFromConnexPartial((&C [(0 +  0)]), /*numVectors*/ (int)ceil(((float)(N)) / CONNEX_VECTOR_LENGTH), /* actual num elems read */ (N), /*offset*/ 2 * (int)ceil(((float)(N))/CONNEX_VECTOR_LENGTH));


/*
  // Sign extending reduction result res0 to i32
  if (res0 > (1UL << (16 + LOG2_CONNEX_VECTOR_LENGTH - 1))) {
      printf("Sign extending res0\n");
      // If it is actually a negative number
      for (int i = 16 + LOG2_CONNEX_VECTOR_LENGTH; i < 32; i++) {
          res0 |= (1 << i);
      }
  }

  // Sign extending reduction result res1 to i32
  if (res1 > (1UL << (16 + LOG2_CONNEX_VECTOR_LENGTH - 1))) {
      printf("Sign extending res1\n");
      // If it is actually a negative number
      for (int i = 16 + LOG2_CONNEX_VECTOR_LENGTH; i < 32; i++) {
          res1 |= (1 << i);
      }
  }
*/

  printf("res0 = %d (0x%08x)\n", res0, res0);
  printf("res1 = %d (0x%08x)\n", res1, res1);
  int res = res0 + (res1 << 16);

  return res;
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
    #define NUM_ELEMS (CONNEX_VECTOR_LENGTH / 2)
    //#define NUM_ELEMS 32

    TYPE A[NUM_ELEMS + 100];
    TYPE B[NUM_ELEMS + 100];
    TYPE C[NUM_ELEMS + 100];


    //srand(0); // 0xf5d49 + 0x86360000 = ...
    srand(time(NULL));

    int i, testResult;

    printf("Entered Test()\n");
    printf("  NUM_ELEMS = %d (sizeof(TYPE) = %lu)\n", NUM_ELEMS, sizeof(TYPE));

    printf("RAND_MAX = %d\n", RAND_MAX);
    // We assume RAND_MAX = 2,147,483,647 (before it was RAND_MAX = 32767)

    assert(RAND_MAX == 2147483647);

    for (i = 0; i < NUM_ELEMS; i++) {
        /*
        A[i] = ((rand() % 4) << 30) + ((rand() % 32768) << 15) + (rand() % 32768);
        B[i] = ((rand() % 4) << 30) + ((rand() % 32768) << 15) + (rand() % 32768);
        */

        /*
        // Works on x64
        A[i] = ((rand() % 2) << 31) + (rand());
        B[i] = ((rand() % 2) << 31) + (rand());
        */

        /*
        A[i] = rand() % 100000000;
        B[i] = rand() % 100000000;
        */
        A[i] = rand();
        B[i] = rand();

        /*
        // Works very well:
        A[i] = -1;
        B[i] = -1;
        */



        printf("A[%d] = 0x%08x\n", i, A[i]);
        printf("B[%d] = 0x%08x\n", i, B[i]);
    }

    printf("Calling ReduceInt32 (with i32 result)...\n");
    fflush(stdout);

    int res = ReduceInt32(A, B, C, NUM_ELEMS);
    printf("Result of reduction i32 = %d (0x%08x)\n", res, res);

    printf("Finished executing the Opincaa kernel.\n");
    fflush(stdout);
    //
    printf("Testing the correctness of the computation of the Opincaa kernel.\n");
    fflush(stdout);


    #define FAIL -1
    #define PASS 0

    testResult = PASS;
    int resTmp = 0;
    unsigned res0Tmp = 0, res1Tmp = 0;
    for (i = 0; i < NUM_ELEMS; i++) {
        resTmp += A[i];

        //res0Tmp += ((unsigned)A[i]) % 65536;
        //res1Tmp += ((unsigned)A[i]) >> 16;

        /*res0Tmp += A[i] & 65535;
        res1Tmp += A[i] >> 16;
        */
        res0Tmp += ((unsigned short *)&A[i])[0];
        res1Tmp += ((unsigned short *)&A[i])[1];
    }

//#ifdef NNNNNO
    printf("res0Tmp = 0x%08x\n", res0Tmp);
    printf("res1Tmp = 0x%08x\n", res1Tmp);


    resTmp = 0;
    int res0Tmpi = 0, res1Tmpi = 0;
    for (i = 0; i < NUM_ELEMS; i++) {
        resTmp += A[i];

        //res0Tmp += ((unsigned)A[i]) % 65536;
        //res1Tmp += ((unsigned)A[i]) >> 16;

        /*res0Tmp += A[i] & 65535;
        res1Tmp += A[i] >> 16;
        */
printf("((short *)&A[i])[0] = %hd\n", ((short *)&A[i])[0]);
printf("((short *)&A[i])[1] = %hd\n", ((short *)&A[i])[1]);

    printf("res0Tmpi = 0x%08x\n", res0Tmpi);
    printf("res1Tmpi = 0x%08x\n", res1Tmpi);

        res0Tmpi += ((short *)&A[i])[0];
        res1Tmpi += ((short *)&A[i])[1];
    }

    printf("res0Tmpi = 0x%08x\n", res0Tmpi);
    printf("res1Tmpi = 0x%08x\n", res1Tmpi);
//#endif

    if (res != resTmp) {
        printf("res = %d, 0x%08x ( != 0x%08x)\n", res, res, resTmp);

        printf("res0Tmp = 0x%08x\n", res0Tmp);
        printf("res1Tmp = 0x%08x\n", res1Tmp);

        testResult = FAIL;
    }

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

