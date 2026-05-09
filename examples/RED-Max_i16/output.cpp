#include <assert.h>
#include <stdio.h>
#include <time.h>

// typedef long TYPE;
// typedef int TYPE;
typedef short TYPE;


#define LLVM_ISEL_CODEGEN
#ifdef LLVM_ISEL_CODEGEN
  #define PRINTREG(regNum) regNum
  #define PrintRegDebug(regNum) regNum
  #define PrintDebugMessage(regNum) regNum
#endif

/*
  Performance of the application:
     - number of cycles simulated = 217 (add 1 more for the REDUCE)
         (and 2 less for UNnecessary prologue and epilogue)
     - with broadcast of the max value: <<number of cycles simulated = 338
         (add 1 more for the REDUCE)>> (and 2 less for UNnecessary
                                        prologue and epilogue)
     Note: with debug messages we reach about 1000 cycles.

    This code is inspired from
      /home/asusu/LLVM/llvm38Nov2016/llvm/build40/bin/Tests/NEW_v128i16/opincaa_standalone_apps/Emulate_i16_DIV/Opincaa/output.cpp

    Pseudocode for MAX-reduction (with halving the vector, similar to what LLVM LoopVectorize generates itself):
      This uses the fact MAX is commutative - see e.g., also
      http://developer.amd.com/resources/articles-whitepapers/opencl-optimization-case-study-simple-reductions/ .
*/


int MaxReduce(TYPE *C, int N) { // TYPE N) {
    //TYPE sum = 0;
    int sum = 0;


    int numElemsAccessedC = ((((C) +  (2 *  (N +  -1))) +  2) - C) / 2;
    connexGlobal->writeDataToConnexPartial((&C [(0 +  0)]), /* actual num elems written */ numElemsAccessedC, /*offset*/ 0); // writeDataToConnexPartial() is blocking // Generated in InstrumentVectorGatherLoadOrScatterStore()
    _BEGIN_KERNEL(BatchNumberGlobal);
        EXECUTE_IN_ALL(
  #define REG_CT0  31
  #define REG_CT1  30
  #define REG_SRC   29
  #define REG_MAX2  28
  #define REG_IDX  27
  #define REG_PRED  26
  #define REG_PRED2 25
  // Both REG_PRED2 and REG_STEPS share the same physical register
  #define REG_STEPS 25

    // Before this we should strip-mine and do MAX over pairs of vectors.
    // The result from MAX from strip-mined vector computation will be MAX-reduced.


    R(REG_SRC) = LS[0];

      //PrintDebugMessage("Before MAX-reduce:\n");
      //PrintRegDebug(REG_SRC);

    R(REG_CT0) = 0;
    R(REG_CT1) = 1;

    R(REG_IDX) = INDEX;
    PrintRegDebug(REG_IDX);


  for (int i = 2; i <= CONNEX_VECTOR_LENGTH; i *= 2) {

      PrintDebugMessage("One more iteration of MAX-reduce:\n");

      // We compute the max-REDUCE of REG_SRC:
      //R(AUX) = R(REG_SRC);
      R(REG_STEPS) = CONNEX_VECTOR_LENGTH / i;
    PrintRegDebug(REG_STEPS);
      CELL_SHL( R(REG_SRC), R(REG_STEPS) );
      //
      // NOP for CONNEX_VECTOR_LENGTH / i times
      for (int iNOP = 0; iNOP < CONNEX_VECTOR_LENGTH / i; iNOP++) {
          NOP;
      }
      //
      R(REG_MAX2) = SHIFT_REG;
      PrintRegDebug(REG_SRC);
      PrintRegDebug(REG_MAX2);

      // For lower halves of vectors REG_SRC and REG_MAX2 we choose the max

      R(REG_PRED) = R(REG_IDX) < R(REG_STEPS);
      R(REG_PRED2) = R(REG_SRC) < R(REG_MAX2);
    //PrintRegDebug(REG_PRED2);
      R(REG_PRED) &= R(REG_PRED2); // R(REG_PRED) & R(REG_PRED2);
      R(REG_PRED2) = R(REG_PRED) == R(REG_CT1);
    PrintRegDebug(REG_PRED);
      NOP;
    );
    EXECUTE_WHERE_EQ(
          R(REG_SRC) = R(REG_MAX2);
      PrintRegDebug(REG_SRC);
    );

    EXECUTE_IN_ALL(

  }

    /*
// Using 2 nested predicated instructions for the conditional part:
    R(REG_PRED) = R(REG_IDX) < R(REG_STEPS);
    NOP;
    EXECUTE_WHERE_LT(
        // We could use instead of & below: R(REG_MAX) = -32768;
        R(REG_IDX) = R(REG_SRC) < R(REG_MAX2); // not conditioned by active flags
      PrintRegDebug(REG_IDX);
// TODO TODO TODO: if necessary, do better when it is clear what instructions are predicated - see ACTIVE flag in ConnexISA.docx
        R(REG_IDX) = R(REG_STEPS) & R(REG_IDX); // (not conditioned also)
        R(REG_IDX) = R(REG_IDX) == R(REG_CT1); // (not conditioned also)
        NOP;
      );
      EXECUTE_WHERE_EQ(
          R(REG_SRC) = R(REG_MAX);
      );
      EXCUTE_IN_ALL
        PrintDebugMessage("A 1st step MAX-reduce:\n");
        PRINTREG(REG_SRC);
    */


      PrintDebugMessage("Broadcasting the MAX value in REG_SRC[0]\n");
      // We now put in R(REG_SRC) the max value that we have in REG_SRC[0]
    PrintRegDebug(REG_IDX);
      CELL_SHR( R(REG_SRC), R(REG_IDX) );
      //
      // NOP for CONNEX_VECTOR_LENGTH - 1 times
      for (int iNOP = 0; iNOP < CONNEX_VECTOR_LENGTH - 1; iNOP++) {
          NOP;
      }
      //
      R(REG_SRC) = SHIFT_REG;
    PrintRegDebug(REG_SRC);

      //REDUCE R(1);
      REDUCE R(REG_SRC);

    );
    _END_KERNEL(BatchNumberGlobal);

    connexGlobal->executeKernel(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
    //sum = connexGlobal->readReduction(); // Warning: need to sign extend reduction result
    connexGlobal->readCorrectReductionResults(&sum, 1, sizeof(int), true);
    //printf("res = %d (0x%08x)\n", res, res);

    return sum;
}



//#define TEST_OPINCAA_CONNEX_KERNELS

#ifdef TEST_OPINCAA_CONNEX_KERNELS
int Test() {
    //#define NUM_ELEMS 1000000
    //#define NUM_ELEMS 10000
    //#define NUM_ELEMS 8192
    //#define NUM_ELEMS 1024
    //#define NUM_ELEMS (128 * 1024)
    //#define NUM_ELEMS (127 * 1024 + 23)
    #define NUM_ELEMS CONNEX_VECTOR_LENGTH

    TYPE C[NUM_ELEMS];

    int i, testResult;

    printf("Entered Test()\n");
    printf("  NUM_ELEMS = %d\n", NUM_ELEMS);


    srand(time(NULL));
    //srand(0);
    //
    for (i = 0; i < NUM_ELEMS; i++) {
        //C[i] = -1;
        //C[i] = rand() % 10 - 5;
        C[i] = rand() % 65536 - 32768;
    }

    printf("Calling MaxReduce()...\n");
    fflush(stdout);

    int res = MaxReduce(C, NUM_ELEMS);
    printf("res (orig, before eventually sign extending it) = %d (0x%08x)\n",
            res, res);

    printf("Finished executing the Opincaa kernel.\n");
    fflush(stdout);

    printf("Result returned by MaxReduce() is res = %d\n", res);
    int resTest = -32768;
    //for (i = 0; i < CONNEX_VECTOR_LENGTH; i++)
    for (i = 0; i < NUM_ELEMS; i++) {
        resTest = resTest > (short)C[i] ? resTest : C[i];
    }
    printf("Result returned by MaxReduce() should be resTest = %d (0x%x) * NUM_ELEMS which is %d (0x%x)\n",
            resTest, resTest,
            resTest * NUM_ELEMS, resTest * NUM_ELEMS);
    assert(res == resTest * NUM_ELEMS);
/*
    assert( (res & 0xFFFF) == (resTest & 0xFFFF) );
    printf("The result (seems) is correct: (%d & 0xFFFF) == (%d & 0xFFFF).\n", res, resTest);
*/



    return 0;
}

/*
int main() {
    Test();

    return 0;
}
*/

#endif


