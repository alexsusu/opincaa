// MEGA-TODO: compute also RES_HIGH

/*
Note: Connex program (has 3 + 15 * 9 + 8 = 146):
     SRC1 = LS[];
     SRC2 = LS[];

     CT1 = 1;
     for idxLoop {
        *op1AlignedLow = SRC1 << idxLoop;
        *op1AlignedHigh = SRC1 >> (16 - idxLoop);
        PRED = SRC2 & CT1
        PRED = PRED == CT1;
        NOP
        WHERE_EQ
          ADD
          //ADDC
        END_WHERE
        SRC2 >>= 1;
     }

      PRED = SRC2 & CT1
      NOP
      WHERE_EQ
        *op1AlignedLow = SRC1 << idxLoop;
        *op1AlignedHigh = SRC1 >> (16 - idxLoop);
        SUB
        //SUBC
      END_WHERE

    LS[res] = ...;
*/

/*
This is an implementation of the integer MUL for i16 operands.
  Normally MUL.i16 is performed inside the DSP slices of the Zynq 7000 FPGA.
  But this implementation is independent and implements a standard 2's
complement MUL of the operands - RCA?? or CSA??? .
*/


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


//typedef long TYPE;
//typedef int TYPE;

typedef short TYPE;
#define TYPE_MAX 32767
#define TYPE_MIN -32768


//#define LLVM_ISEL_CODEGEN
#ifdef LLVM_ISEL_CODEGEN
  #define PRINTREG(regNum) regNum
  #define PrintDebugMessage(aStr) aStr
  #define PrintRegDebug(regNum) regNum
#endif



//#define ADAPTIVE_RUN
#ifdef ADAPTIVE_RUN
  // We use BITREVERSE (experimental) with POPCNT
  #define GetIndexHighestBitSet

  // We use REPEAT_SETLC_REDUCE (experimental)

  // DO NOT uncomment this: //#define EMULATE_MAX_REDUCE_UNFINISHED
#endif



// Uncommenting this puts in C the elements of abs(A)
//#define WRITE_ABS_VALUE_IN_MEM_FOR_TESTING




std::string kernelName = "MulInt16";

TYPE MulInt16(TYPE *A, TYPE *B, TYPE *C, TYPE *C_high, TYPE N) {
    connexGlobal->writeDataToConnexPartial((&A [(0 +  0)]),
                 /* actual num elems written */ (N),
                 /*offset*/ 0);

    connexGlobal->writeDataToConnexPartial((&B [(0 +  0)]),
                 /* actual num elems written */ (N),
                 /*offset*/ 1 * (int)ceil(((float)(N))/CONNEX_VECTOR_LENGTH));

    BEGIN_KERNEL(kernelName);
      EXECUTE_IN_ALL(
//  #define NUM_ITERS  10

  #define CT0  31
  #define CT1  30
  #define CT2  29
  #define CT16 28
  //
  #define SRC1_ALIGNEDLOW  27
  #define SRC1_ALIGNEDHIGH 26

  // Keep SRC1 < SRC2
  #define SRC1 24
  #define SRC2 25
  #define RES_LOW 23
  #define RES_HIGH 22

/*  #define DIVISOR_ALIGNED_LOW16 20
  #define DIVISOR_ALIGNED_HIGH16 19
  #define RESIDUAL 18
  #define ITER 17
  #define ITER16 16
  #define CHANGE_SIGN 15
*/
  #define PRED 21
  #define PRED2 20
  #define PRED3 19
  #define PRED4 18
  #define PRED5 17


  #define AUX  10
  #define AUX2  9
  #define AUX3  8
  //#define MASK_CONTINUE 18
  //#define SRCAUX 27
  #define IDX   5
  #define IDX16 4
  /*
  #define IDXMOD2 14
  #define IDXPRED 13
  */
  //#define CRY 15







        R(1) = 0;
        R(2) = (N / CONNEX_VL) + ((N & (CONNEX_VL - 1)) > 0);
        R(3) = ((N / CONNEX_VL) + ((N & (CONNEX_VL - 1)) > 0)) * 2;
        //R(3) = 2;

        R(SRC1) = LS[R(1)];
        R(SRC2) = LS[R(2)];

        R(CT0) = 0;
        R(CT1) = 1;

        R(RES_LOW) = 0;
        R(RES_HIGH) = 0;

#define USE_REPEAT
#ifdef USE_REPEAT
    R(CT16) = 16;

    /* Doing 1 iteration outside REPEAT loop because Connex doesn't support
       SHR 16: Handling separately case SHR (16 - idxLoop). */
    R(SRC1_ALIGNEDLOW) = R(SRC1) << 0;
    R(SRC1_ALIGNEDHIGH) = 0; //R(SRC1) >> 16;
    R(PRED) = R(SRC1) < R(CT0);
    NOP;
    )
    EXECUTE_WHERE_LT(
      R(SRC1_ALIGNEDHIGH) = -1; //R(SRC1) >> 16;
    )
    EXECUTE_IN_ALL(
        R(PRED) = R(SRC2) & R(CT1);
        R(PRED) = R(PRED) == R(CT1);
        NOP;
      );
      EXECUTE_WHERE_EQ(
        R(RES_LOW) += R(SRC1_ALIGNEDLOW);

        R(RES_HIGH) = ADDC(R(RES_HIGH), R(CT0));
        R(RES_HIGH) += R(SRC1_ALIGNEDHIGH);
      );
      EXECUTE_IN_ALL(
        R(SRC2) >>= 1;


      R(IDX) = 1;
      REPEAT_X_TIMES(14); //15);
#else
    int idxLoop;
    for (idxLoop = 0; idxLoop < 15; idxLoop++) {
#endif


      #ifdef USE_REPEAT
        R(SRC1_ALIGNEDLOW) = R(SRC1) << R(IDX);


        R(IDX16) = R(CT16) - R(IDX);
        R(SRC1_ALIGNEDHIGH) = SHRA(R(SRC1), R(IDX16));

      #else
        R(SRC1_ALIGNEDLOW) = R(SRC1) << idxLoop;
        if (idxLoop == 0) {
            // Handling separately case SHR (16 - idxLoop).
            R(SRC1_ALIGNEDHIGH) = 0; //R(SRC1) >> 16;
            R(PRED) = R(SRC1) < R(CT0);
            NOP;
          )
          EXECUTE_WHERE_LT(
            R(SRC1_ALIGNEDHIGH) = -1; //R(SRC1) >> 16;
          )
          EXECUTE_IN_ALL(
        }
        else {
            //R(SRC1_ALIGNEDHIGH) = R(SRC1) >> (16 - idxLoop);
            //
            //R(AUX) = (16 - idxLoop);
            R(SRC1_ALIGNEDHIGH) = ISHRA(R(SRC1), 16 - idxLoop);
        }
      #endif

      PrintRegDebug(SRC1_ALIGNEDLOW);
      PrintRegDebug(SRC1_ALIGNEDHIGH);

        R(PRED2) = R(SRC2) & R(CT1);
        R(PRED3) = R(PRED2) == R(CT1);
        NOP;
      );
      EXECUTE_WHERE_EQ(
       PrintRegDebug(RES_LOW);
       PrintRegDebug(SRC1_ALIGNEDLOW);
        R(RES_LOW) += R(SRC1_ALIGNEDLOW);
       PrintRegDebug(RES_LOW);

       PrintRegDebug(RES_HIGH);
        R(RES_HIGH) = ADDC(R(RES_HIGH), R(CT0));
       PrintDebugMessage("After ADDC:");
       PrintRegDebug(RES_HIGH);
        R(RES_HIGH) += R(SRC1_ALIGNEDHIGH);
      );
      EXECUTE_IN_ALL(
        R(SRC2) >>= 1;

    #ifdef USE_REPEAT
      PrintRegDebug(IDX);
    #else
      char strAux[100];
      sprintf(strAux, "idxLoop = %d", idxLoop);
      PrintDebugMessage(strAux);
    #endif
      PrintRegDebug(RES_LOW);
      PrintRegDebug(RES_HIGH);


    #ifdef USE_REPEAT
        R(IDX) += R(CT1);
      END_REPEAT;
    #else
      } // END for loop
    #endif

        R(PRED4) = R(SRC2) & R(CT1);
        R(PRED5) = R(PRED4) == R(CT1);
        NOP;
      );
      EXECUTE_WHERE_EQ(
      #ifdef USE_REPEAT
       /*
       PrintRegDebug(IDX);
        R(SRC1_ALIGNEDLOW) = R(SRC1) << R(IDX);
        R(SRC1_ALIGNEDHIGH) = SHRA(R(SRC1), R(IDX16));
       */
        R(SRC1_ALIGNEDLOW) = R(SRC1) << 15;
        R(SRC1_ALIGNEDHIGH) = ISHRA(R(SRC1), 1);
      #else
        R(SRC1_ALIGNEDLOW) = R(SRC1) << idxLoop;
        //R(SRC1_ALIGNEDHIGH) = R(SRC1) >> (16 - idxLoop);
        //
        //R(AUX) = (16 - idxLoop);
        //R(SRC1_ALIGNEDHIGH) = SHRA(R(SRC1), R(AUX));
        R(SRC1_ALIGNEDHIGH) = ISHRA(R(SRC1), 16 - idxLoop);
      #endif

      PrintRegDebug(SRC1_ALIGNEDLOW);
      PrintRegDebug(SRC1_ALIGNEDHIGH);

        R(RES_LOW) -= R(SRC1_ALIGNEDLOW);
      PrintRegDebug(RES_LOW);

        R(RES_HIGH) = SUBC(R(RES_HIGH), R(CT0));
      PrintRegDebug(RES_HIGH);
        //R(RES_HIGH) -= R(CT1);
        R(RES_HIGH) -= R(SRC1_ALIGNEDHIGH);

      PrintDebugMessage("idxLoop = 15");
      PrintRegDebug(RES_LOW);
      PrintRegDebug(RES_HIGH);

      );
      EXECUTE_IN_ALL(
       PRINTREG(1);
       PRINTREG(2);

        R(1) = R(1) + R(3);
        R(6) = R(6) + R(3);

       PrintDebugMessage("SRC1:\n");
       PRINTREG(SRC1);
       PrintDebugMessage("SRC2:\n");
       PRINTREG(SRC2);

        LS[R(3)] = R(RES_LOW);
      R(3) = R(3) + R(CT1);
      NOP;
        LS[R(3)] = R(RES_HIGH);

        R(1) = R(1) + R(CT1);
        R(2) = R(2) + R(CT1);
        R(3) = R(3) + R(CT1);
       PRINTREG(1);
       PRINTREG(2);
       PRINTREG(3);

        REDUCE R(0); // We add a 'bogus' REDUCE to wait for it
      );
    END_KERNEL(kernelName);



    try {
        Kernel *kernel = connexGlobal->getKernel(kernelName);

      #ifdef LLVM_ISEL_CODEGEN
        kernel->sdNodeVarNameRegDef[SRC1] = "nodeOpSrc1";
        kernel->sdNodeVarNameRegDef[SRC2] = "nodeOpSrc2";
        kernel->offsetKernelToStartCodegenFrom = 5 + 1; // +1 for the END_WHERE instruction Opincaa adds automatically to each kernel
        kernel->numInstructionsToCodegen = kernel->size()
                                             - 9 /*num instruction we remove from end of kernel */
                                             - kernel->offsetKernelToStartCodegenFrom;
        //
        // We use chain, since with glue with get a lot or weird scheduling errors:
        //kernel->useGlue = 0;
        kernel->useGlue = 1;
        // IMPORTANT: to convert in 'partly SSA form' we require ~64 registers
        assert(CONNEX_REG_COUNT != 32);

        /*
        printf("Calling connexGlobal->genLLVMISelManualCode()\n");
        fflush(stdout);
        string resGenLLVM = connexGlobal->genLLVMISelManualCode(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
        printf("resGenLLVM = \n%s\n", resGenLLVM.c_str());
        fflush(stdout);
        */
        printf("Calling connexGlobal->genLLVMISelManualCode()\n");
        fflush(stdout);
        string resGenLLVM = connexGlobal->genLLVMISelManualCode(kernelName);
        printf("resGenLLVM = \n%s\n", resGenLLVM.c_str());
        fflush(stdout);



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

        connexGlobal->executeKernel(kernelName);
        connexGlobal->readReduction();

        printf("Returned from connexGlobal->readReduction()\n");
        fflush(stdout);

        connexGlobal->readDataFromConnexPartial(C,
                    /* actual num elems read */ N,
                    /*offset*/ 2); // 0 is offset of A
        connexGlobal->readDataFromConnexPartial(C_high,
                    /* actual num elems read */ N,
                    /*offset*/ 3); // 0 is offset of A
    }
    catch (string err) {
        cout << err << endl;
    }
    catch (...) {
        cout << "Unknown exception" << endl;
    }

    return 0;
}


//#define TEST_OPINCAA_CONNEX_KERNELS

#ifdef TEST_OPINCAA_CONNEX_KERNELS
int Test() {
    //#define NUM_ELEMS 1000000
    //#define NUM_ELEMS 10000
    //#define NUM_ELEMS 8192
    //#define NUM_ELEMS 1024
    //#define NUM_ELEMS 256
    //#define NUM_ELEMS 128
    //#define NUM_ELEMS 64

   //#define NUM_ELEMS (CONNEX_VECTOR_LENGTH/2)
   #define NUM_ELEMS CONNEX_VECTOR_LENGTH
   //#define NUM_ELEMS (CONNEX_VECTOR_LENGTH/2) * 10

    TYPE A[NUM_ELEMS + 100];
    TYPE B[NUM_ELEMS + 100];
    TYPE C[NUM_ELEMS + 10000];
    TYPE C_high[NUM_ELEMS + 10000];


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

    // GOOD:
        //A[i] = rand() % 65536 - 32768;
        A[i] = rand() % 65534 - 32767;
        //
        B[i] = rand() % 65536 - 32768;

        /*
        // Works on x64
        A[i] = ((rand() % 2) << 31) + (rand());
        B[i] = ((rand() % 2) << 31) + (rand());
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


        printf("A[%d] = 0x%04hx\n", i, A[i]);
        printf("B[%d] = 0x%04hx\n", i, B[i]);
    }

    A[0] = -32768;
    B[0] = -32768;

    //A[1] = -32768;
    A[1] = 1;
    B[1] = 508;

    A[2] = -32768;
    B[2] = -20508;


    printf("Calling MulInt16 (with quotient and reminder results)...\n");
    fflush(stdout);

    MulInt16(A, B, C, C_high, NUM_ELEMS);

    printf("Finished executing the Opincaa kernel.\n");
    fflush(stdout);
    //
    printf("Testing the correctness of the computation of the Opincaa kernel.\n");
    fflush(stdout);


    #define FAIL -1
    #define PASS 0

    int numDiffResults = 0;

    testResult = PASS;
    for (i = 0; i < NUM_ELEMS; i++) {
        int resCorrect = A[i] * B[i];

        short resCorrectLow = resCorrect & 0xFFFF;
        short resCorrectHigh = resCorrect >> 16;

        //int tmp = A[i] * B[i] & 0xFFFF;
        short tmp = A[i] * B[i];

        if (B[i] != 0 &&
             (tmp != C[i])) {
            printf("Diffs: i = %d: A[i] = %d, B[i] = %d, C[i] = %d, tmp = %d\n", i, A[i], B[i], C[i], tmp);

            testResult = FAIL;
            //break;

            numDiffResults++;
        }
    }

    printf("  testResult = %d (PASS = %d)\n", testResult, PASS);
    printf("  numDiffResults = %d\n", numDiffResults);

    if (testResult == FAIL) {
        printf("NUM_ELEMS = %d\n", NUM_ELEMS);
        //for (i = 0; i < NUM_ELEMS + 5; i++)
        for (i = 0; i < NUM_ELEMS; i++) {
            //short resCorrect = (A[i] * B[i]) & 0xFFFF;
            int resCorrect = A[i] * B[i];
            short resCorrectLow = resCorrect & 0xFFFF;
            short resCorrectHigh = resCorrect >> 16;

            printf("A[%d] = %d (0x%04hx), ", i, A[i], A[i]);
            printf("B[%d] = %d (0x%04hx)\n", i, B[i], B[i]);

            bool diffLow = false;
            if (resCorrectLow != C[i]) {
                printf("A[%d] = %d (0x%04hx), ", i, A[i], A[i]);
                printf("B[%d] = %d (0x%04hx)\n", i, B[i], B[i]);

                printf("  C[%d] = 0x%04hx != 0x%04hx\n",
                        i,
                        C[i], resCorrectLow);


                diffLow = true;
            }
            if (resCorrectHigh != C_high[i]) {
                if (diffLow == false) {
                    printf("A[%d] = %d (0x%04hx), ", i, A[i], A[i]);
                    printf("B[%d] = %d (0x%04hx)\n", i, B[i], B[i]);
                }
                printf("  C_high[%d] = 0x%04hx != 0x%04hx\n",
                        i,
                        C_high[i], resCorrectHigh);
            }
            printf("\n");
            /*
            else {
                printf("C[%d] = %d (0x%04hx)\n",
                        i, C[i], C[i]);
                printf("  A[%d] = 0x%04hx\n", i, A[i]);
                printf("  B[%d] = 0x%04hx\n", i, B[i]);
            }
            */
        }
    }

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

