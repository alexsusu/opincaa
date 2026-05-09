#include <iostream>


//#define LLVM_ISEL_CODEGEN
#ifdef LLVM_ISEL_CODEGEN
  #define PRINTREG(regNum) regNum
  #define PrintDebugMessage(aStr) aStr
  #define PrintRegDebug(regNum) regNum
#endif

//#include "ConnexMachine.h"
//#include "MaxReduce.h"
#include "LibMisc.h"
#include "LibPatterns.h"
//
#ifdef REAL_TEST
#include "GenRandF16.h"
#endif

using namespace std;



/*
Denormals, infinities and underflows are very well represented in pages
 401 and 402 (Figure 8.2.a) of book [Ercegovac_Digital_Arithmetic_2004]
 (also at
  https://books.google.ro/books/about/Digital_Arithmetic.html?id=p79cu3nZ6yoC&redir_esc=y).

TODO TODO: do rounding - ROUND_TO_NEAREST
*/







/*
This kernel takes on Connex many cycles, 389, because of RedMin() and BroadcastScalar().
Here we bother less with nested ifs (we simply don't use CONTINUE;
  so, we have less complicated control flow) because:
    - we count NANs and INFs and send the results on the CPU
      and let the CPU decide if the result of the reduction is a NAN or an INF,
      or otherwise just pack it correctly with the result of reduction for both
      mantissa and exponent
      (the exponent is not necessary to be sum-reduced, but
      this is a good way to send the value to the CPU).
        --> MEGA-TODO: think if it is better to give on CPU connex->readDataFromConnex() in order to retrieve the final value of result exponent
*/
void Red_f16Kernel(int32_t opAPtr) {
    /* Florin Ghido (Skype talk Jul 16, 2017):
       <<mai ales partea de handling pentru subnormals mi se pare dificila
       in paralel,
     dar putem presupune ca daca vreunul din numere are operanzi sau da un
     rezultat subnormal, atunci se poate face totul mult mai incet si serial.>>
    */
    BEGIN_KERNEL("red.f16");
    #define CT1             31
    #define CT0             30
    #define CT31            29
    #define SRC            28
    #define SRC_MANTISSA   27
    #define SRC_EXPONENT   26
    #define SRC_SIGN       25
    //
    #define AUX2            24
//    #define SRC2_MANTISSA   23
//    #define SRC2_EXPONENT   22
//    #define SRC2_SIGN       21
    //
//    #define DST             20
    //
    #define PRED1           19
    #define PRED1A          18
    #define PRED2           17
    #define PRED2A          16
    //
//    #define CONTINUE        15
//    #define CONTINUE_BACKUP 14
    //
    #define MANTISSA_MASK   13
    #define EXPONENT_MASK   12
    #define SIGN_MASK       11
    #define HIDDENBIT_MASK  10

    #define PRED3            9
    #define AUX              8
    #define NUM_BITS         7


        // Get operands and split
        EXECUTE_IN_ALL(
            R(SRC) = LS[opAPtr]; // load F16 operand

            R(CT1) = 1;
            R(CT0) = 0;
            // A special value for the 5-bit exponent for f16 is 0x1F (31)
            R(CT31) = 31;

            R(MANTISSA_MASK)  = F16_MANTISSA_MASK;
            R(EXPONENT_MASK)  = F16_EXPONENT_MASK;
            R(SIGN_MASK)      = F16_SIGN_MASK;
            R(HIDDENBIT_MASK) = F16_HIDDENBIT_MASK;

        // Unpacking f16 operand
            UnpackF16(__kernel,
                        CT0, CT1, CT31,
                        SRC, SRC_SIGN, SRC_EXPONENT,
                        SRC_MANTISSA,
                        SIGN_MASK, EXPONENT_MASK, MANTISSA_MASK,
                        HIDDENBIT_MASK,
                        PRED2, PRED2A, PRED3);

            PrintRegDebug(SRC_MANTISSA);


        /* Handling NaNs and INFinities
             - see also comments before this function, which discuss about all
               the special cases for f32
        */

    // We now treat the special cases of f16
         // Mega-TODO: think if it is better to do counting as a separate kernel, s.t. we avoid doing on Connex RedMin and Broadcast if we have INF or NAN among inputs.
            // Count all NANs:
            PrintDebugMessage("Count NANs and INFs:");
            PrintRegDebug(SRC_EXPONENT);
            PrintRegDebug(SRC_MANTISSA);
            R(PRED1A) = R(SRC_EXPONENT) == R(CT31);
            PrintRegDebug(PRED1A);
            R(PRED2) = R(SRC_MANTISSA) == R(CT0);
            PrintRegDebug(PRED2);
            R(PRED3) = R(CT1) - R(PRED2);
            PrintRegDebug(PRED3);
            R(PRED3) &= R(PRED1A);
            PrintRegDebug(PRED3);
            REDUCE R(PRED3);

            // Count all positive infinities:
            R(AUX2) = R(PRED1A) & R(PRED2);
            PrintRegDebug(AUX2);
            PrintRegDebug(PRED1A);
            PrintRegDebug(PRED2);
            //
            PrintRegDebug(SRC_SIGN);
            R(PRED3) = R(SRC_SIGN) == R(CT0);
            R(PRED2A) = R(AUX2) & R(PRED3);
            PrintRegDebug(PRED2A);
            REDUCE R(PRED2A);

            //PrintDebugMessage("Count -INFs:");
            // Count all negative infinities:
            R(PRED3) = R(CT1) - R(PRED3);
            R(PRED2A) = R(AUX2) & R(PRED3);
            PrintRegDebug(PRED2A);
            REDUCE R(PRED2A);
        // Finished treating special cases f16

            PrintRegDebug(SRC_MANTISSA);

            // If mantissa is negative we reverse its sign
            R(PRED3) = R(SRC_SIGN) == R(SIGN_MASK);
            NOP;
        )
        EXECUTE_WHERE_EQ(
            /* Where f16 number is negative (sgn == 1),
             *    we reverse sign of mantissa.
             *   TODO: Think if this introduces more error */
            R(SRC_MANTISSA) = R(CT0) - R(SRC_MANTISSA);
        )
        EXECUTE_IN_ALL(
            PrintDebugMessage("Mantissas, after taking sign into account:");
            PrintRegDebug(SRC_MANTISSA);

          #define REF_EXP      2
            // We compute the max among exponents
            R(REF_EXP) = R(SRC_EXPONENT);
            PrintRegDebug(REF_EXP);

//#define STANDARD_WAY_OF_DOING_REDUCTION_FOR_ALL_F16
#ifdef STANDARD_WAY_OF_DOING_REDUCTION_FOR_ALL_F16
        //#define DO_RED_MIN
        #ifdef DO_RED_MIN
            RedMin(__kernel,
        #else
            RedMax(__kernel,
        #endif
                      CT0, CT1,
                      /* SRC */ REF_EXP,
                      /* AUX */ 10,
                      /* IDX */ 6,
                      /* PRED */ 5,
                      /* PRED2 */ 4,
                      /* STEPS */ 3);

            PrintDebugMessage("Result of Red...():");
            PrintRegDebug(REF_EXP);

            BroadcastScalar(__kernel,
                            /* SRC */ REF_EXP,
                            /* IDX */ 6);
            PrintDebugMessage("Result of BroadcastScalar():");
            PrintRegDebug(REF_EXP);

// small-TODO: although seems quite difficult, for the sake of better precision we could bring to the smallest exponent IFF the deltaExp is small enough (e.g. deltaExp < 6)
            // Identify if operand has bigger exponent than REF_EXP (MAX-reduction result above)
          #ifdef DO_RED_MIN
            R(PRED3) = R(REF_EXP) < R(SRC_EXPONENT);
          #else
            R(PRED3) = R(SRC_EXPONENT) < R(REF_EXP);
          #endif
            NOP;
        )
        EXECUTE_WHERE_LT(
            PrintRegDebug(PRED3);
          #ifdef DO_RED_MIN
            // Compute difference of exponents
            R(PRED3) = R(SRC_EXPONENT) - R(REF_EXP);

            PrintRegDebug(PRED3);

            // Shift mantissa of bigger exponent with difference of exponents
            R(SRC_MANTISSA) = R(SRC_MANTISSA) << R(PRED3);
          #else
            // Compute difference of exponents
            R(PRED3) = R(REF_EXP) - R(SRC_EXPONENT);
            // Shift mantissa of smaller exponent with difference of exponents
            R(SRC_MANTISSA) = R(SRC_MANTISSA) >> R(PRED3);
          #endif


            PrintDebugMessage("SRC_MANTISSA after alignment");
            PrintRegDebug(SRC_MANTISSA);
            PrintRegDebug(SRC_SIGN);

            // Adjust exponent accordingly; now we're radix-aligned
            R(SRC_EXPONENT) = R(REF_EXP);
        )
        EXECUTE_IN_ALL(
            PrintRegDebug(PRED3);

            PrintRegDebug(SRC_MANTISSA);
            REDUCE R(SRC_MANTISSA);

            PrintRegDebug(REF_EXP);
            REDUCE R(REF_EXP); // TODO
#else // ! STANDARD_WAY_OF_DOING_REDUCTION_FOR_ALL_F16

        // We avoid computing the RedMin/Max() and BroadcastScalar(), and also increase the precision of the result

      for (int idxRed = 0; idxRed < 6; idxRed++) {
            R(REF_EXP) = idxRed * 5; // Inferior is 0, which is used ONLY by NaNs (denormals have now exponent 1)
            R(AUX2) = 6 + idxRed * 5; // Superior is 6 + 5 * 5 = 31, which is ONLY used by INFs
            // Next step: R(AUX2) += 5; R(REF_EXP) += 4;


            PrintRegDebug(REF_EXP);
            PrintRegDebug(AUX2);


            // For all F16 with EXPONENT in REF_EXP..PRED3-1
            R(PRED1) = R(SRC_EXPONENT) < R(AUX2);
            R(PRED2) = R(REF_EXP) < R(SRC_EXPONENT);

#ifdef REDUCE_IN_WHERE_NOT_WORKING
R(AUX2) = 0; //R(SRC_MANTISSA);
#endif

            R(REF_EXP) += R(CT1);
            R(PRED3) = R(PRED1) & R(PRED2);
            R(PRED3) = R(PRED3) == R(CT1);
            NOP;
        )
        EXECUTE_WHERE_EQ(
            PrintDebugMessage("ONLY for these lanes we reduce now:");
            PrintRegDebug(PRED3);

            // Compute difference of exponents
            R(PRED1) = R(SRC_EXPONENT) - R(REF_EXP);

            PrintRegDebug(PRED1);
            PrintRegDebug(SRC_MANTISSA);

            // Shift mantissa of bigger exponent with difference of exponents
            R(SRC_MANTISSA) = R(SRC_MANTISSA) << R(PRED1);



            PrintDebugMessage("SRC_MANTISSA after alignment");
            PrintRegDebug(SRC_MANTISSA);
            PrintRegDebug(SRC_SIGN);

            // Adjust exponent accordingly; now we're radix-aligned
//            R(SRC_EXPONENT) = R(REF_EXP);

            PrintRegDebug(SRC_MANTISSA);
            REDUCE R(SRC_MANTISSA);
#ifdef REDUCE_IN_WHERE_NOT_WORKING
R(AUX2) = R(SRC_MANTISSA);
PrintRegDebug(AUX2);
#endif
            //PrintRegDebug(REF_EXP);
            //REDUCE R(REF_EXP); // TODO

            R(REF_EXP) = R(REF_EXP);
        )
        EXECUTE_IN_ALL(
#ifdef REDUCE_IN_WHERE_NOT_WORKING
REDUCE R(AUX2);
#endif
      } // END idxRed for loop
#endif // ! STANDARD_WAY_OF_DOING_REDUCTION_FOR_ALL_F16

            /* IMPORTANT: We don't need to send the SGN also -
              because we made SRC_MANTISSA contain the sign. */
        );
    END_KERNEL("red.f16");
}


int FloatRedTest(ConnexMachine *connex) {
    uint16_t opA[CONNEX_VECTOR_LENGTH];
    uint16_t opB[CONNEX_VECTOR_LENGTH];
    uint16_t resCorrect;
    uint16_t result;

    /*
        // Note: 1.3193359375 + 4.4375 = 5.7568359375 = 0 100-01 01-1100-0001 (2) = 45 C1 (16)
        //    The float result (f16) is converted to binary using from http://oletus.github.io/float16-simulator.js/

    // Obtained from /home/asusu/LLVM/llvm38Nov2016/llvm/build40/bin/Tests/NEW_v128i16/opincaa_standalone_apps/FP16/C/LLVM/_Float16/Gen_constant_values/Float16.ll (and the _O3....ll file also):
      0x3D47, 0x4470 --> 45C2
      0xBD47, 0x4470     423C
      0x3D47, 0xC470     C23C
      0xBD47, 0xC470     C5C2
    */

    //opA[0] = 0x0002; // F16 encoding for 0.0000001 = 10^(-7)
    //opA[0] = 0x7B53; // F16 encoding for 60000
    //opA[0] = 0xFB53; // F16 encoding for -60000
    //
    //
    /*
    opA[0] = 0xBD47; // F16 encoding for 1.3193359375
    resCorrect[0] = 0xD947; // For F16: 1.3193359375 * 128 is represented as 0x5947 - see /home/asusu/LLVM/llvm38Nov2016/llvm/build40/bin/Tests/NEW_v128i16/opincaa_standalone_apps/FP16/C/LLVM/_Float16/Gen_constant_values/ForREDf16/Float16.ll
    */
    //
    //
    /*
    opA[0] = 0x3D47; // F16 encoding for 1.3193359375
    resCorrect[0] = 0x5947; // For F16: 1.3193359375 * 128 is represented as 0x5947 - see /home/asusu/LLVM/llvm38Nov2016/llvm/build40/bin/Tests/NEW_v128i16/opincaa_standalone_apps/FP16/C/LLVM/_Float16/Gen_constant_values/ForREDf16/Float16.ll

    opA[1] = 0xBD47; // F16 encoding for -1.3193359375
    resCorrect[1] = 0x423C; // as obtained in Clang

    opA[2] = 0x3D47; // F16 encoding for 1.3193359375
    resCorrect[2] = 0xC23C; // as obtained in Clang

    opA[3] = 0xBD47; // F16 encoding for -1.3193359375
    resCorrect[3] = 0xC5C2; // as obtained in Clang

    opA[4] = 0x4470; // F16 encoding for 4.4375 because: sign = 0; exp = 10001(2) = 17(10); significand = 0001110000 so value is 1.0001110000 * 2^(17 - 15) = [1 + 2^(-4) + 2^(-5) + 2^(-6)] * 4 = 4.4375
    resCorrect[4] = 0x423C; // as obtained in Clang

    opA[5] = F16_NAN; // F16 encoding for a NaN

    opA[6] = F16_NAN; // F16 encoding for a NaN

    opA[7] = 0x3D47; // F16 encoding for ...

    opA[8] = F16_NAN;

    opA[9] = 0x3D47; // F16 encoding for ...

    opA[10] = 0x3D47; // F16 encoding for ...

    opA[11] = F16_INF_NEGATIVE;

    opA[12] = F16_INF_POSITIVE;

    opA[13] = F16_INF_NEGATIVE; // F16 encoding for -Inf

    opA[14] = F16_INF_POSITIVE; // F16 encoding for Inf

    opA[15] = F16_INF_NEGATIVE; // F16 encoding for -Inf

    opA[16] = F16_INF_NEGATIVE; // F16 encoding for -Inf

    opA[17] = F16_INF_POSITIVE; // F16 encoding for +Inf

    opA[18] = 0x0001; // F16 encoding for +Inf

    opA[19] = 0x3D47; // F16 encoding for +Inf

    opA[20] = 0x0400; // F16 encoding for 2^(1 - 15) * 1.0000000000
    //opB[20] = 0x8401; // F16 encoding for -2^(1 - 15) * 1.0000000001
    // Currently returns result: 0x8000, which means negative zero (-0)
    //
    opA[21] = 0x7BFF; // F16 encoding for 2^(31 - 15) * 1.1111111111(2) (exponent is: 11110)
    //opB[21] = 0x7BFF; // F16 encoding for -2^(31 - 15) * 1.1111111111(2) (exponent is: 11110)
    // Currently returns result: 0x7fff, which is normal because the mantissa of the result (before truncation) is , so we increase the exponent by 1


    // Checking that x + (-x) is equal 0
    opA[22] = 0x0880; // F16 encoding for 2^(2 - 15) * 1.0010000000
    //opB[22] = 0x8880; // F16 encoding for -2^(2 - 15) * 1.0010000000

    // Some denormal operands:
    opA[23] = 0x0002; // F16 encoding for 0.0000001 = 10^(-7)
    opA[24] = 0x00A8; // F16 encoding for 0.00001 = 10^(-5)
    */


    opA[0] = 0xbf38; // F16 encoding for -1.8046875. 0xbf38(S=1,E=0xf,F=0x738)
    opA[1] = 0xcc49; // F16 encoding for -17.1406250. 0xcc49(S=1,E=0x13,F=0x449)
    opA[2] = 0x91e6; // F16 encoding for -0.0007200
    opA[3] = 0xa217; // F16 encoding for -0.0118942
    opA[4] = 0xa003; // F16 encoding for -0.0078354
    opA[5] = 0x0a72; // F16 encoding for a 0.0001967
    opA[6] = 0xc215; // F16 encoding for a -3.0410156
    opA[7] = 0xb82c; // F16 encoding for -0.5214844
    opA[8] = 0x284f; // F16 encoding for 0.0336609
    opA[9] = 0x98c9; // F16 encoding for -0.0023365
    /*
    // From /home/alarm/Experiments/26l_reduce_f16/2/STD_run_001:
    res = -2.8046875
    res = 0xbf37(S=1,E=0xf,F=0x738) (used GetStringForF16())

    res = -18.1406250
    res = 0xcc48(S=1,E=0x13,F=0x449) (used GetStringForF16())

    res = -1.0007200
    res = 0x91e5(S=1,E=0x4,F=0x5e6) (used GetStringForF16())

    res = -1.0118942
    res = 0xa216(S=1,E=0x8,F=0x617) (used GetStringForF16())

    res = -1.0078354
    res = 0xa002(S=1,E=0x8,F=0x403) (used GetStringForF16())

    res = -1.0001967
    res = 0x0a71(S=0,E=0x2,F=0x672) (used GetStringForF16())

    res = -4.0410156
    res = 0xc214(S=1,E=0x10,F=0x615) (used GetStringForF16())

    res = -1.5214844
    res = 0xb82b(S=1,E=0xe,F=0x42c) (used GetStringForF16())

    res = -1.03366090xcda0
    res = 0x284e(S=0,E=0xa,F=0x44f) (used GetStringForF16())

    res = -1.0023365
    res = 0x98c8(S=1,E=0x6,F=0x4c9) (used GetStringForF16())

    resTest = -12897 (0xcda0(S=1,E=0x13,F=0x5a0))
    */

    resCorrect = 0xcda0; // -22.500000 (0xcda0(S=1,E=0x13,F=0x5a0))
    // Just for NNZ = 2: resCorrect = 0xccbc; // -18.937500, 0xccbc(S=1,E=0x13,F=0x4bc)

  #ifdef TEST_ONLY_ONE_VALUE
    uint16_t valUsed = opA[0];
    printf("valUsed = %04x (%s)\n", valUsed, GetStringForF16(valUsed).c_str());
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++)
    //for (int i = 25; i < CONNEX_VECTOR_LENGTH; i++)
        opA[i] = valUsed;
  #endif

  //#define NNZ CONNEX_VECTOR_LENGTH
  #define NNZ 10
  //#define NNZ 2
    int i;
    /*
    for (i = 0; i < NNZ; i++)
        opA[i] = valUsed;
    */
    for (i = NNZ; i < CONNEX_VECTOR_LENGTH; i++)
        opA[i] = 0;


  #ifdef REAL_TEST
    //srand(0);
    srand(time(NULL));
    //
    //for (i = 0; i < NUM_ELEMS; i++) {
    for (i = 0; i < NNZ; i++) {
      //opA[i] = rand();
      //opA[i] = 0x3D47;
      opA[i] = GenRandF16Valid();
    }
    /*
    for (i = NNZ; i < NUM_ELEMS; i++) {
      opA[i] = 0;
    }
    */
  #endif

    // Write opA to LS memory from offset 0
    connex->writeDataToConnex(opA, 1, 0);

    Red_f16Kernel(0);


#ifdef LLVM_ISEL_CODEGEN
    string kernelName = "red.f16";
    Kernel *kernel = connexGlobal->getKernel(kernelName);
    kernel->sdNodeVarNameRegDef[SRC] = "nodeOpSrcCast";
    //
    // For RED f16:
    kernel->offsetKernelToStartCodegenFrom = 1 + 1; // +1 for the END_WHERE instruction Opincaa adds automatically to each kernel
    kernel->numInstructionsToCodegen = kernel->size() -
                            0 /*num instruction we remove from end of kernel */
                            - kernel->offsetKernelToStartCodegenFrom;
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

    connex->executeKernel("red.f16");

    //unsigned short resF16 = ComputeReductionResultF16(connex);
    //unsigned short resF16;
    connex->readReductionResultsAndComputeF16(1, &result); //&resF16);

    //printf("resF16 = %hu (0x%hx)\n", resF16, resF16);
    //std::cout << "resF16 = " << GetStringForF16(resF16) << "\n";
    std::cout << "result = " << GetStringForF16(result) << "\n";


 #ifdef REAL_TEST
    __fp16 resCorrectF16 = 0.0;
    for (i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        resCorrectF16 += *((__fp16 *)&opA[i]);
    }
    /*
    printf("resCorrectF16 = %s\n",
           //resCorrectF16,
           //* ((short *)&resCorrectF16),
           GetStringForF16(* ((short *)&resCorrectF16)).c_str());
    */
    resCorrect = * ((short *)&resCorrectF16);
  #endif

    printf("resCorrect = %s\n",
           //resCorrect,
           // * ((short *)&resCorrect),
           GetStringForF16(* ((short *)&resCorrect)).c_str());

    assert(result == resCorrect && "result is wrong (different than resCorrect)");
}


void Test() {
    FloatRedTest(connexGlobal);
}

/*
int main(int argc, char *argv[]){

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

