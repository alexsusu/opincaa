#include <iostream>
//#include "ConnexMachine.h"


//#define LLVM_ISEL_CODEGEN
#ifdef LLVM_ISEL_CODEGEN
  #define PRINTREG(regNum) regNum
  #define PrintDebugMessage(aStr) aStr
  #define PrintRegDebug(regNum) regNum
#endif




// This needs to be put after the #define LLVM_ISEL_CODEGEN
#include "LibMisc.h"


using namespace std;



/*
From https://docs.oracle.com/cd/E19957-01/806-3568/ncg_goldberg.html:
    "Requiring that a floating-point representation be normalized makes the representation unique."
    VERY IMPORTANT: "the numerical ordering of nonnegative real numbers corresponds to the lexicographic ordering of their floating-point representations."

From https://floating-point-gui.de/errors/comparison/
    "IEEE 754 floats are designed to maintain their order when their bit patterns are interpreted as integers."
        - Note: for the same sign.
*/




static string kernelName;
void LTf16Kernel(int32_t opAPtr, int32_t opBPtr, int32_t resPtr) {
    kernelName = "lt.f16";

    BEGIN_KERNEL(kernelName);
        /*
        The algorithm:
           - load constants into register file
           - get operands and split them into components; find out which is
                       largest and whether signs are the same
             Normally:
               - if signs are the same, we keep the sign and do addition
               - if signs are not the same, we keep the sign of the largest
                     operand, and do subtraction of the smaller from the larger
             But, we do:
                - where number is negative, make:
                    mantissa = -mantissa
           - calculate the difference between exponents
           - shift mantissa by the exponent difference
           - add/subtract mantissas as needed (see above)
           - re-normalize (handle here also subnormal numbers)
           - assemble result.

           - at the end (not sure if it's better to put at the beginning or
             otherwise [TODO: CHECK]
             we check for special values (INF, NaN, underflow)
        */

    #define CT0            31
    #define CT1            30
    #define CT5            29
//    #define CT16           29
//    #define CT31           28
    #define SRC1           27
    #define SRC1_MANTISSA  26
    #define SRC1_EXPONENT  25
    #define SRC1_SIGN      24
    //
    #define SRC2           23
    #define SRC2_MANTISSA  22
    #define SRC2_EXPONENT  21
    #define SRC2_SIGN      20
    //
    #define RES            19
    //
    #define PRED1          18
    #define PRED1A         17
    #define PRED2          16
    #define PRED2A         15
    //
    #define CONTINUE       14
    //#define CONTINUE_BACKUP 13
    //
    #define MANTISSA_MASK  13
    #define EXPONENT_MASK  12
    #define SIGN_MASK      11
    #define HIDDENBIT_MASK 10

//    #define PRED3           9
//    #define AUX             8
//    #define AUX2            7

        // Get operands and split
        EXECUTE_IN_ALL(
            R(SRC1) = LS[opAPtr]; // load 1st F16 operand
            R(SRC2) = LS[opBPtr]; // load 2nd F16 operand


            R(CT0) = 0;
            R(CT1) = 1;
            R(CT5) = 5;

            R(MANTISSA_MASK)  = F16_MANTISSA_MASK;
            R(EXPONENT_MASK)  = F16_EXPONENT_MASK;
            R(SIGN_MASK)      = F16_SIGN_MASK;
            R(HIDDENBIT_MASK) = F16_HIDDENBIT_MASK;

            R(RES) = 0;
            R(CONTINUE) = 1;


          // We extract exponent and mantissa for 1st operand
          R(SRC1_EXPONENT) = R(SRC1) & R(EXPONENT_MASK);
         //PrintRegDebug(SRC1_EXPONENT);
          R(SRC1_MANTISSA) = R(SRC1) & R(MANTISSA_MASK);
         //PrintDebugMessage("SRC1_EXPONENT:");


          // We extract exponent and mantissa for 2nd operand
          R(SRC2_EXPONENT) = R(SRC2) & R(EXPONENT_MASK);
          // Get the exponent from bit 0 (shift down to LSB).
         //PrintRegDebug(SRC2_EXPONENT);
          R(SRC2_MANTISSA) = R(SRC2) & R(MANTISSA_MASK);
         //PrintDebugMessage("SRC2_EXPONENT:");



        // Handling NaNs

            // We "catch" 1st opnd == NaN
            // Exponent 1st opnd == 31?
            R(PRED1A) = POPCNT(R(SRC1_EXPONENT));
            R(PRED1A) = R(PRED1A) == R(CT5);
            // mantissa1 != 0?
            R(PRED1) = R(SRC1_MANTISSA) == R(CT0);
            R(PRED1) = R(CT1) - R(PRED1);
            R(PRED1) &= R(PRED1A);
            PrintDebugMessage("PRED1 (catch 1st opnd == NaN):\n");
            PrintRegDebug(PRED1);
            // 1st opnd: exponent == 31 && mantissa != 0 -> NaN
            R(PRED1) = R(PRED1) == R(CT1);
            NOP;
        );
        EXECUTE_WHERE_EQ(
            //R(RES) = 0;
            R(CONTINUE) = 0;
        );
        EXECUTE_IN_ALL(
            PrintRegDebug(CONTINUE);

            // We "catch" 2nd opnd == NaN
            // Exponent 2nd opnd == 31?
            R(PRED2A) = POPCNT(R(SRC2_EXPONENT));
            R(PRED2A) = R(PRED2A) == R(CT5);
            // mantissa1 != 0?
            R(PRED2) = R(SRC2_MANTISSA) == R(CT0);
            R(PRED2) = R(CT1) - R(PRED2);
            R(PRED2) &= R(PRED2A);
            PrintDebugMessage("PRED2 (catch 2nd opnd == NaN):\n");
            PrintRegDebug(PRED2);
            // 2nd opnd: exponent == 31 && mantissa != 0 -> NaN
            R(PRED2) = R(PRED2) == R(CT1);
            NOP;
        );
        EXECUTE_WHERE_EQ(
            //R(RES) = 0;
            R(CONTINUE) = 0;
        );

        EXECUTE_IN_ALL(
            PrintRegDebug(CONTINUE);

            /* We take out cases the mantissas are different in order for the
               following check for negative numbers to work well (where we
               take out sign and made result 1 to later XOR it if the numbers
               as positive are smaller). */
            R(PRED2) = R(SRC1) == R(SRC2);
            R(CONTINUE) ^= R(PRED2);

            /* If both operands are negative then we have to take out
               sign to compare. */
            R(PRED2) = R(SRC1) & R(SRC2);
          PrintRegDebug(PRED2);
            R(PRED2) &= R(SIGN_MASK);
          PrintRegDebug(PRED2);
            // If sign of both operands is negative
            R(PRED2) = R(PRED2) == R(SIGN_MASK);
          PrintRegDebug(PRED2);
            R(PRED2) &= R(CONTINUE);
          PrintRegDebug(PRED2);
            R(PRED2) = R(PRED2) == R(CT1);
          PrintRegDebug(PRED2);
            NOP;
        );
        EXECUTE_WHERE_EQ(
          R(SRC1) ^= R(SIGN_MASK);
          R(SRC2) ^= R(SIGN_MASK);
          R(RES) = 1;
          /*
          R(SRC1) = R(CT0) - R(SIGN_MASK);
          R(SRC2) = R(CT0) - R(SIGN_MASK);
          */
        )
        EXECUTE_IN_ALL(
            PrintRegDebug(SRC1);
            PrintRegDebug(SRC2);

            R(PRED2) = R(SRC1) < R(SRC2);
            PrintRegDebug(PRED2);
            R(PRED2) &= R(CONTINUE);
            R(PRED2) = R(PRED2) == R(CT1);
            NOP;
        );
        EXECUTE_WHERE_EQ(
            R(RES) ^= R(CT1);
            PrintRegDebug(RES);
        );
        EXECUTE_IN_ALL(
            // store result
            LS[resPtr] = R(RES);

            // End of program synchronization point; host will wait for this
            REDUCE(R1);
        )
    END_KERNEL(kernelName);
}



int LTf16Test(ConnexMachine *connex) {
    uint16_t opA[CONNEX_VECTOR_LENGTH];
    uint16_t opB[CONNEX_VECTOR_LENGTH];
    uint16_t resCorrect[CONNEX_VECTOR_LENGTH];
    uint16_t result[CONNEX_VECTOR_LENGTH];

    /*
    opA[0] = 0x3D47; // F16 encoding for 1.3193359375
    opB[0] = 0x4470; // F16 encoding for 4.4375
    resCorrect[0] = 1;
    */
    //
    // Another interesting test of negative f16
    opA[0] = 0xb105; // F16 encoding for (S=1,E=0xc,F=0x505)
    opB[0] = 0xa317; // F16 encoding for (S=1,E=0x8,F=0x717)
    resCorrect[0] = 1;

    opA[1] = 0xBD47; // F16 encoding for -1.3193359375
    opB[1] = 0x4470; // F16 encoding for 4.4375
    resCorrect[1] = 1;

    opA[2] = 0x3D47; // F16 encoding for 1.3193359375
    opB[2] = 0xC470; // F16 encoding for -4.4375
    resCorrect[2] = 0;

    opA[3] = 0xBD47; // F16 encoding for -1.3193359375
    opB[3] = 0xC470; // F16 encoding for -4.4375
    resCorrect[3] = 0;

    opA[4] = 0x4470; // F16 encoding for 4.4375 because: sign = 0; exp = 10001(2) = 17(10); significand = 0001110000 so value is 1.0001110000 * 2^(17 - 15) = [1 + 2^(-4) + 2^(-5) + 2^(-6)] * 4 = 4.4375
    opB[4] = 0xBD47; // F16 encoding for -1.3193359375
    resCorrect[4] = 0;

    opA[5] = F16_NAN; // F16 encoding for a NaN
    opB[5] = F16_NAN; // F16 encoding for a NaN
    resCorrect[5] = 0;

    opA[6] = F16_NAN; // F16 encoding for a NaN
    opB[6] = F16_NAN_2; // F16 encoding for NaN
    resCorrect[6] = 0;

    opA[7] = 0x3D47; // F16 encoding for ...
    opB[7] = F16_NAN; // F16 encoding for NaN
    resCorrect[7] = 0;

    opA[8] = F16_NAN;
    opB[8] = 0x3D47;
    resCorrect[8] = 0;

    opA[9] = 0x3D47; // F16 encoding for ...
    opB[9] = F16_INF_POSITIVE; // F16 encoding for +Inf
    resCorrect[9] = 1;

    opA[10] = 0x3D47; // F16 encoding for ...
    opB[10] = F16_INF_NEGATIVE; // F16 encoding for -Inf
    resCorrect[10] = 0;

    opA[11] = F16_INF_NEGATIVE;
    opB[11] = 0x3D47;
    resCorrect[11] = 1;

    opA[12] = F16_INF_POSITIVE;
    opB[12] = 0x3D47;
    resCorrect[12] = 0;

    opA[13] = F16_INF_NEGATIVE; // F16 encoding for -Inf
    opB[13] = F16_INF_NEGATIVE; // F16 encoding for -Inf
    resCorrect[13] = 0;

    opA[14] = F16_INF_POSITIVE; // F16 encoding for Inf
    opB[14] = F16_INF_POSITIVE; // F16 encoding for Inf
    resCorrect[14] = 0;

    opA[15] = F16_INF_NEGATIVE; // F16 encoding for -Inf
    opB[15] = F16_INF_POSITIVE; // F16 encoding for Inf
    resCorrect[15] = 1;

    opA[16] = F16_INF_NEGATIVE; // F16 encoding for -Inf
    opB[16] = 0x3D47; // F16 encoding for ...
    resCorrect[16] = 1;

    opA[17] = F16_INF_POSITIVE; // F16 encoding for +Inf
    opB[17] = 0x3D47; // F16 encoding for ...
    resCorrect[17] = 0;

    opA[18] = 0x0001; // F16 encoding for denormal...
    opB[18] = 0x3D47; // F16 encoding for ...
    resCorrect[18] = 1;

    opA[19] = 0x3D47; // F16 encoding for ...
    opB[19] = 0x0001; // F16 encoding for denormal...
    resCorrect[19] = 0;

    opA[20] = 0x0400; // F16 encoding for 2^(1 - 15) * 1.0000000000
    opB[20] = 0x8401; // F16 encoding for -2^(1 - 15) * 1.0000000001
    resCorrect[20] = 0;
    //
    opA[21] = 0x7BFF; // F16 encoding for 2^(31 - 15) * 1.1111111111(2) (exponent is: 11110)
    opB[21] = 0x7BFF; // F16 encoding for 2^(31 - 15) * 1.1111111111(2) (exponent is: 11110)
    resCorrect[21] = 0;

    opA[22] = 0x0880; // F16 encoding for 2^(2 - 15) * 1.0010000000
    opB[22] = 0x8880; // F16 encoding for -2^(2 - 15) * 1.0010000000
    resCorrect[22] = 0;

    opA[23] = 0x7B53; // F16 encoding for 60000
    opB[23] = 0x7B53; // F16 encoding for 60000
    resCorrect[23] = 0;

    // Treat denormals
    opA[24] = 0x0002; // F16 encoding for ...
    opB[24] = 0x0001; // F16 encoding for ...
    resCorrect[24] = 0;


  #define NUM_VALS 25

    for (int i = NUM_VALS; i < CONNEX_VECTOR_LENGTH; i++) {
        opA[i] = 0;
        opB[i] = 0;
        resCorrect[i] = 0;
    }


    LTf16Kernel(0, 1, 2);

    connex->writeDataToConnex(opA, 1, 0);
    connex->writeDataToConnex(opB, 1, 1);

#ifdef LLVM_ISEL_CODEGEN
    Kernel *kernel = connexGlobal->getKernel(kernelName);
    kernel->sdNodeVarNameRegDef[SRC1] = "nodeOpSrcCast1";
    kernel->sdNodeVarNameRegDef[SRC2] = "nodeOpSrcCast2";
    //
    // For LT fp16:
    kernel->offsetKernelToStartCodegenFrom = 2 + 1; // +1 for the END_WHERE instruction Opincaa adds automatically to each kernel
    kernel->numInstructionsToCodegen = kernel->size()
                                           - 2 /*num instruction we remove from end of kernel */
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

    connex->executeKernel(kernelName);
    connex->readReduction();

    connex->readDataFromConnex(result, 1, 2);

    printf("LT results are:\n");
    for (int i = 0; i < NUM_VALS; i++) {
        printf("i=%d: opA = %s (0x%hx, %hd), opB = %s (0x%hx, %hd) --> res = %d (resCorrect = %d) - error = %d %s\n",
               i,
               GetStringForF16(opA[i]).c_str(),
               opA[i], opA[i],
               GetStringForF16(opB[i]).c_str(),
               opB[i], opB[i],
               result[i],
               resCorrect[i],
               ((int)resCorrect[i]) - ((int)result[i]),
               (resCorrect[i] - result[i] == 0) ? "" : "(difference encountered!)"
             );
    }
    printf("\n");
}


void Test() {
    LTf16Test(connexGlobal);
}


