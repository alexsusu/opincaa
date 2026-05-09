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


//#define DO_SUB


/*
Denormals, infinities and underflows are very well represented in pages
 401 and 402 (Figure 8.2.a) of book [Ercegovac_Digital_Arithmetic_2004]
 (also at
  https://books.google.ro/books/about/Digital_Arithmetic.html?id=p79cu3nZ6yoC&redir_esc=y).
*/

/* If we comment ROUND_TO_NEAREST we use round toward zero:
     - see book [Ercegovac_Digital_Arithmetic_2004],
       Section 8.2.2 Round Toward Zero (Truncation)
*/
#define ROUND_TO_NEAREST






static string kernelName;
void AddSub_f16Kernel(int32_t opAPtr, int32_t opBPtr, int32_t resPtr,
                   bool isSub = false) {
    /* Florin Ghido (Skype talk Jul 16, 2017):
       "mai ales partea de handling pentru subnormals mi se pare dificila
       in paralel,
     dar putem presupune ca daca vreunul din numere are operanzi sau da un
     rezultat subnormal, atunci se poate face totul mult mai incet si serial."
    */

    printf("AddSub_f16Kernel(): isSub = %d\n", isSub);

    kernelName = "add_or_sub.f16";

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
    #define CT16           29
    #define CT31           28
    //
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
    #define DST            19
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

    #define PRED3           9
    #define AUX             8
    #define AUX2            7
    #define NUM_BITS        6

    #define VAL_FOR_SIZE    5

  #ifdef ROUND_TO_NEAREST
    #define L               5
    #define G               4
    #define Rbit            3
    #define T               2
    #define RND             1
    #define ROUND_NUM_ADDITIONAL_BITS 3

    /* DISCARDED_BITS stores the mantissa bits: G, R and T.
        G, R and T are normally discarded by the SHR operations we perform
        (either because the result mantissa has more than F16_MANTISSA_BITS or
        because the exponent < 0).
    */
    #define DISCARDED_BITS  SRC2_SIGN
  #endif
    /* NUM_DISCARDED_BITS represents the number of bits discarded from the mantissa
        (or number of steps we performed SHR on the result mantissa computed
       originally by multiplication). */
    #define NUM_DISCARDED_BITS 0

        // Get operands and split
        EXECUTE_IN_ALL(
            R(SRC1) = LS[opAPtr]; // load 1st F16 operand
            R(SRC2) = LS[opBPtr]; // load 2nd F16 operand


            R(CONTINUE) = 1;
            R(CT1) = 1;
            R(CT0) = 0;
            R(CT16) = 16;
            // A special value for the 5-bit exponent for fp16 is 0x1F (31)
            R(CT31) = 31;

            R(MANTISSA_MASK)  = F16_MANTISSA_MASK;
            R(EXPONENT_MASK)  = F16_EXPONENT_MASK;
            R(SIGN_MASK)      = F16_SIGN_MASK;
            R(HIDDENBIT_MASK) = F16_HIDDENBIT_MASK;

          #ifdef ROUND_TO_NEAREST
            R(DISCARDED_BITS) = 0;
            //R(NUM_DISCARDED_BITS) = -1;
          #endif

            /* We initialize registers updated in WHERE blocks
                - it is NOT necessary because we use R(CONTINUE) to nest ifs
                    and treat the value returned R(DST) for all cases carefully,
                  but it's nicer.
                Note that for ISel and for Kernel::genLLVMISelManualCode()
                  it doesn't seem to matter.
            */
            R(DST) = 0;
            R(DISCARDED_BITS) = 0;
            R(NUM_DISCARDED_BITS) = 0;
            R(AUX2) = 0;
            R(L) = 0;
            R(G) = 0;
            R(Rbit) = 0;
            R(T) = 0;
            R(RND) = 0;

        /*
        // mantissa1 == 0?
        R(PRED1) = R(SRC1_MANTISSA) == R(CT0);
            // Add hidden bit for the mantissa (from bit 0, as it is initially)
            R(SRC1_MANTISSA) |= R(HIDDENBIT_MASK);
            PrintRegDebug(SRC1_MANTISSA);
        */
            /*
            [Ercegovac_Digital_Arithmetic_2004]
            8.4.5 Denormal and Zero Operands
              "When an operand is a denormal number (E = 0 and F != 0), then
            there is no hidden 1. Consequently, the operand of addition should
            be set to E = 1 and 0.F .
            The rest of the algorithm remains unchanged."

            Note: Preprocessing for denormals takes ~7 instructions more for
                1 operand.
            */

        // Unpacking 1st operand
        UnpackF16(__kernel,
                    CT0, CT1, CT31,
                    SRC1, SRC1_SIGN, SRC1_EXPONENT,
                    SRC1_MANTISSA,
                    SIGN_MASK, EXPONENT_MASK, MANTISSA_MASK,
                    HIDDENBIT_MASK,
                    //F16_MANTISSA_BITS,
                    PRED1, PRED1A, PRED3);


            /*
        // mantissa2 == 0
        R(PRED2) = R(SRC2_MANTISSA) == R(CT0);
            // Add hidden bit for the mantissa (from bit 0, as it is initially)
            R(SRC2_MANTISSA) |= R(HIDDENBIT_MASK);
            PrintRegDebug(SRC2_MANTISSA);
            */
        // Unpacking 2nd operand
        UnpackF16(__kernel,
                    CT0, CT1, CT31,
                    SRC2, SRC2_SIGN, SRC2_EXPONENT,
                    SRC2_MANTISSA,
                    SIGN_MASK, EXPONENT_MASK, MANTISSA_MASK,
                    HIDDENBIT_MASK,
                    //F16_MANTISSA_BITS,
                    PRED2, PRED2A, PRED3);


        /* Handling NaNs, underflow, infinity
             - see also comments before this function, which discuss about all
               the special cases for fp32
        */

  // We now TREAT_SPECIAL_CASES_FP
            // We "catch" 1st opnd == NaN
            // Exponent 1st opnd == 31?
            R(PRED1A) = R(SRC1_EXPONENT) == R(CT31);
            // mantissa1 != 0?
            //R(PRED1) = R(CT1) - R(PRED1);
            //R(PRED1) = R(SRC1_MANTISSA) == R(CT0);
            // 1st opnd: exponent == 31 && mantissa != 0 -> NaN
            R(PRED1) &= R(PRED1A);
            PrintDebugMessage("PRED1 (catch 1st opnd == NaN):\n");
            PrintRegDebug(PRED1);
            R(PRED1) = R(PRED1) == R(CT1);
            NOP;
        );
        EXECUTE_WHERE_EQ(
            R(DST) = R(SRC1); // We return NaN also // NOT initialized
            R(CONTINUE) = 0;
        );
        EXECUTE_IN_ALL(
            // We "catch" 2nd opnd == NaN
            // Exponent 2nd opnd == 31?
            R(PRED2A) = R(SRC2_EXPONENT) == R(CT31);

            // mantissa2 != 0?
            //R(PRED2) = R(CT1) - R(PRED2);
            //R(PRED2) = R(SRC2_MANTISSA) == R(CT0);

            // 2nd opnd: exponent == 31 && mantissa != 0 -> NaN
            R(PRED2) &= R(PRED2A);
            // Execute only for R(CONTINUE) == 1
            R(PRED2) = R(PRED2) & R(CONTINUE);
            R(PRED2) = R(PRED2) == R(CT1);
            NOP;
        );
        EXECUTE_WHERE_EQ(
            R(DST) = R(SRC2); // We return NaN also
            R(CONTINUE) = 0;
        );

        EXECUTE_IN_ALL(
            PrintDebugMessage("CONTINUE:\n");
            PrintRegDebug(CONTINUE);
            // If exp of both operands are == 31
            R(PRED1) = R(PRED1A) & R(PRED2A);
            PrintRegDebug(PRED1);
            PrintRegDebug(SRC1_SIGN);
            PrintRegDebug(SRC2_SIGN);

            R(PRED2) = R(SRC1_SIGN) ^ R(SRC2_SIGN);
            PrintRegDebug(PRED2);
            PrintRegDebug(CT0);

            // And have different signs (0x8000 is actually -32768)
            R(PRED2) = R(PRED2) < R(CT0);
            PrintDebugMessage("Have different signs:\n");
            PrintRegDebug(PRED2);

            R(PRED1) = R(PRED2) & R(PRED1);
            // Execute only for R(CONTINUE) == 1 and both mantissa == 0:
            R(PRED1) = R(PRED1) & R(CONTINUE);
            R(PRED1) = R(PRED1) == R(CT1); // Inf - Inf
            NOP;
          );

        EXECUTE_WHERE_EQ(
            R(DST) = F16_NAN; // NaN (exp == 31, mantissa != 0)
            R(CONTINUE) = 0;
        );
        EXECUTE_IN_ALL(
            PrintDebugMessage("CONTINUE:\n");
            PrintRegDebug(CONTINUE);
            // Both are Inf (or -Inf), so the result is exactly the same
            R(PRED1) = R(PRED1A) & R(PRED2A); // if exp of both operands are == 31
            PrintRegDebug(PRED1);
            // and have the same sign
            R(PRED2) = R(SRC1_SIGN) ^ R(SRC2_SIGN);
            R(PRED2) = R(PRED2) == R(CT0);
            R(PRED1) = R(PRED2) & R(PRED1);
            // Execute only for R(CONTINUE) == 1
            R(PRED1) = R(PRED1) & R(CONTINUE);
            R(PRED1) = R(PRED1) == R(CT1);
            // Inf or -Inf
            NOP;
          );
        EXECUTE_WHERE_EQ(
            R(DST) = R(SRC1);
            R(CONTINUE) = 0;
        );
        EXECUTE_IN_ALL(
            // 1st opnd is Inf (or -Inf)
            // Execute only for R(CONTINUE) == 1
            R(PRED1) = R(PRED1A) & R(CONTINUE);
            R(PRED1) = R(PRED1) == R(CT1);
            // Inf or -Inf
            NOP;
          );
        EXECUTE_WHERE_EQ(
            R(DST) = R(SRC1);
            R(CONTINUE) = 0;
        );
        EXECUTE_IN_ALL(
            PrintDebugMessage("Check 2nd opnd is +/-Inf: CONTINUE:\n");
            PrintRegDebug(CONTINUE);
            PrintRegDebug(PRED2A);
            // 2nd opnd is Inf (or -Inf)
            // Execute only for R(CONTINUE) == 1
            R(PRED1) = R(PRED2A) & R(CONTINUE);
            R(PRED1) = R(PRED1) == R(CT1);
            PrintRegDebug(PRED1);
            // Inf or -Inf
            NOP;
          );
        EXECUTE_WHERE_EQ(
            R(DST) = R(SRC2);
            PrintRegDebug(DST);
            R(CONTINUE) = 0;
        );
        EXECUTE_IN_ALL(
#ifdef NOT_REQUIRED
         if (isSub == false) {
            /*
            else if (exponent(x) == 0)
                return y;
            */
            // 1st operand exponent == 0
            R(PRED2) = R(SRC1_EXPONENT) == R(CT0);
            // Execute only for R(CONTINUE) == 1
            R(PRED2) = R(PRED2) & R(CONTINUE);
            R(PRED2) = R(PRED2) == R(CT1);
            NOP;
          );
          EXECUTE_WHERE_EQ(
            R(DST) = R(SRC2); // res = opnd2
            R(CONTINUE) = 0;
          );
          EXECUTE_IN_ALL(
            /*
            else if (exponent(y) == 0)
                return x;
            */
            // 2nd operand exponent == 0
            R(PRED2) = R(SRC2_EXPONENT) == R(CT0);
            // Execute only for R(CONTINUE) == 1
            R(PRED2) = R(PRED2) & R(CONTINUE);
            R(PRED2) = R(PRED2) == R(CT1);
            NOP;
          );
          EXECUTE_WHERE_EQ(
            R(DST) = R(SRC1); // res = opnd1
            R(CONTINUE) = 0;
        );
        EXECUTE_IN_ALL(
         } // END if (isSub == false)
#endif // NOT_REQUIRED

        PrintDebugMessage("CONTINUE, after treating special cases FP:\n");
        PrintRegDebug(CONTINUE);

   // END TREAT_SPECIAL_CASES_FP





            PrintRegDebug(DST);
    /*
    From [Ercegovac_Digital_Arithmetic_2004, Section 8.4.1]:
    <<Align significands.
    This consists of the following:
        - Shift right d positions the significand of the operand with the
            smallest exponent.
        - Select as the exponent of the result the largest exponent.>>

    Note that we can also align to the smaller exponent
     (VERY IMPORTANT: x86 seems to do both, chosing smaller exponent if
           feasible - I tested it on my FloatOps.cpp emulating float on x86):
      - This can be beneficial - keeping more bits for the mantissas
        and doing add on them can result in smaller rounding errors.
          - we want to be more accurate and have the same result
          (after the same? rounding) as on the CPU/clang/arm.gcc) - see test
          for add, index 1.
      - But doing SHL (and not SHR) is VERY difficult since we have i16 and
         we might have to SHL for ~30 positions.

    Important: So the book Erecegovac is omitting this "optimization"
         (the errata does not correct the SHR of significand in the book either).
      So it seems this is an omission in the book regarding the IEEE 754 standard
        specification.
    */

    //#define MAX_DIFF_EXPONENTS 5
    //#define MAX_DIFF_EXPONENTS 4
    /* We make MAX_DIFF_EXPONENTS 4 because the mantissa that we SHL has
       11 bits and 4 bits SHL brings the aligned mantissa to 15 bits
         - we do so because we want to avoid putting the mantissa on the sign bit
           of the i16 type, which would change the value of the POSITIVE mantissa
           to negative.
      Also, the result of adding (subtracting) the 2 mantissas should fit on 15
        bits of i16 and since 2 mantissas of 14 significant bits summed up
        yield a result that fits 15 significant bits we establish that:
            #define MAX_DIFF_EXPONENTS 3
        is the correct value for the parameter.
    */
    #define MAX_DIFF_EXPONENTS 3
    #define MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP 32000
    //#define DELTA_EXP AUX2
    #define DELTA_EXP PRED2A


            R(DELTA_EXP) = R(SRC1_EXPONENT) - R(SRC2_EXPONENT);
            PrintRegDebug(DELTA_EXP);

            /*
            // Note that R(DELTA_EXP) can have values between -31..31
            If DELTA_EXP = E1 - E2 in range -inf..-16
                // This means E1 < E2.
                We bring opnd1 to largest exponent, E2
                R(DELTA_EXP) = MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP;
                    We make directly R(SRC1_MANTISSA) = 0 due to "limitation"
                        of Connex processor for SHR val, 16+ (TODO-REMEMBER).
            */
            R(AUX) = -15;
            R(PRED3) = R(DELTA_EXP) <  R(AUX);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) = R(PRED3) & R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
          PrintRegDebug(PRED3);
            NOP;
        )
        EXECUTE_WHERE_EQ(
          PrintDebugMessage("Bringing opnd1 to larger exponent of opnd2 - case special:\n");
            /* Shift mantissa of larger exponent with difference of exponents.
             * We do SHR.
             */
            R(DELTA_EXP) = R(CT0) - R(DELTA_EXP);
          PrintRegDebug(SRC1_MANTISSA);
            //R(SRC1_MANTISSA) >>= R(DELTA_EXP);
            R(SRC1_MANTISSA) = 0;
            PrintRegDebug(SRC1_MANTISSA);
          PrintRegDebug(SRC1_MANTISSA);
            // Adjust exponent accordingly; now we're radix-aligned
            R(SRC1_EXPONENT) = R(SRC2_EXPONENT);
            PrintRegDebug(SRC1_EXPONENT);

            R(DELTA_EXP) = MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP;
        )
        EXECUTE_IN_ALL(

            /*
            // Note that R(DELTA_EXP) can have values between -31..31
            If DELTA_EXP = E1 - E2 in range -15..-(MAX_DIFF_EXPONENTS + 1):
                // This means E1 < E2.
                We bring opnd1 to largest exponent, E2
                R(DELTA_EXP) = MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP;
            */
            R(AUX) = -MAX_DIFF_EXPONENTS;
            R(PRED3) = R(DELTA_EXP) < R(AUX);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) = R(PRED3) & R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
          PrintRegDebug(PRED3);
            NOP;
        )
        EXECUTE_WHERE_EQ(
          PrintDebugMessage("Bringing opnd1 to larger exponent of opnd2:\n");
            /* Shift mantissa of larger exponent with difference of exponents.
             * We do SHR.
             */
            R(DELTA_EXP) = R(CT0) - R(DELTA_EXP);
          PrintRegDebug(SRC1_MANTISSA);
            R(SRC1_MANTISSA) >>= R(DELTA_EXP);
            PrintRegDebug(SRC1_MANTISSA);
          PrintRegDebug(SRC1_MANTISSA);
            // Adjust exponent accordingly; now we're radix-aligned
            R(SRC1_EXPONENT) = R(SRC2_EXPONENT);
            PrintRegDebug(SRC1_EXPONENT);

            R(DELTA_EXP) = MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP;
        )
        EXECUTE_IN_ALL(


            /*
            If DELTA_EXP = E1 - E2 in range -MAX_DIFF_EXPONENTS..-1:
                // This still means E1 < E2.
                We bring opnd2 to smallest exponent, E1
                R(DELTA_EXP) = MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP;
            */
            R(PRED3) = R(DELTA_EXP) < R(CT0);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) = R(PRED3) & R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
            NOP;
        )
        EXECUTE_WHERE_EQ(
          PrintDebugMessage("Bringing opnd2 to smaller exponent of opnd1:\n");
            /* Shift mantissa of larger exponent with difference of exponents.
             * We do SHL (and not SHR) because we want to be more accurate.
             */
            R(DELTA_EXP) = R(CT0) - R(DELTA_EXP);
//            R(PRED3) = R(CT0) - R(DELTA_EXP);
    PrintRegDebug(PRED3);
    PrintRegDebug(SRC2_MANTISSA);
            R(SRC2_MANTISSA) <<= R(DELTA_EXP);
//            R(SRC2_MANTISSA) <<= R(PRED3);
            // Adjust exponent accordingly; now we're radix-aligned
            R(SRC2_EXPONENT) = R(SRC1_EXPONENT);
            PrintRegDebug(SRC2_MANTISSA);
            PrintRegDebug(SRC2_EXPONENT);

            R(DELTA_EXP) = MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP;
        )
        EXECUTE_IN_ALL(



            /*
            If DELTA_EXP = E1 - E2 in range 0..MAX_DIFF_EXPONENTS:
                // This means E1 > E2.
                We bring opnd1 to smallest exponent, E2.
                R(DELTA_EXP) = MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP;
            */
            R(AUX) = MAX_DIFF_EXPONENTS + 1;
            R(PRED3) = R(DELTA_EXP) < R(AUX);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) = R(PRED3) & R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
            NOP;
        )
        EXECUTE_WHERE_EQ(
          PrintDebugMessage("Bringing opnd1 to smaller exponent of opnd2:\n");
            /* Shift mantissa of larger exponent with difference of exponents.
             * We do SHL (and not SHR) because we want to be more accurate.
             */
            PrintRegDebug(DELTA_EXP);
            PrintRegDebug(SRC1_MANTISSA);
            R(SRC1_MANTISSA) <<= R(DELTA_EXP);
            // Adjust exponent accordingly; now we're radix-aligned
            R(SRC1_EXPONENT) = R(SRC2_EXPONENT);
            PrintRegDebug(SRC1_MANTISSA);
            PrintRegDebug(SRC1_EXPONENT);

            R(DELTA_EXP) = MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP;
        )
        EXECUTE_IN_ALL(







   /* If R(DELTA_EXP) >= 16 then make R(SRC2_MANTISSA) because on Connex SHR val, 16 (or more than 16) works incorrectly.
R(PRED3) = 15;
R(PRED3) = R(PRED3) < R(DELTA_EXP);
// If PRED3 is 1
*/
            /*
            // Note again that R(DELTA_EXP) can have values between -31..31
            If DELTA_EXP = E1 - E2 in range (MAX_DIFF_EXPONENTS + 1)..15:
                // This still means E1 > E2.
                We bring opnd2 to largest exponent, E1.
            */
            R(PRED3) = R(DELTA_EXP) < R(CT16);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) = R(PRED3) & R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
            NOP;
        )
        EXECUTE_WHERE_EQ(
          PrintDebugMessage("Bringing opnd2 to larger exponent of opnd1:\n");
            /* Shift mantissa of larger exponent with difference of exponents.
             * We do SHR.
             */
            PrintRegDebug(DELTA_EXP);
            PrintRegDebug(SRC2_MANTISSA);
            R(SRC2_MANTISSA) >>= R(DELTA_EXP);
            PrintRegDebug(SRC2_MANTISSA);
            // Adjust exponent accordingly; now we're radix-aligned
            R(SRC2_EXPONENT) = R(SRC1_EXPONENT);
            PrintRegDebug(SRC2_EXPONENT);

            R(DELTA_EXP) = MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP;
        )
        EXECUTE_IN_ALL(



            /*
            // Note again that R(DELTA_EXP) can have values between -31..31
            If DELTA_EXP = E1 - E2 in range 16..31:
                // This still means E1 > E2.
                We bring opnd2 to largest exponent, E1.
                    We make directly R(SRC2_MANTISSA) = 0 due to "limitation"
                        of Connex processor for SHR val, 16+ (TODO-REMEMBER).
            */
            R(AUX) = 32;
            R(PRED3) = R(DELTA_EXP) < R(AUX);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) = R(PRED3) & R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
            NOP;
        )
        EXECUTE_WHERE_EQ(
          PrintDebugMessage("Bringing opnd2 to larger exponent of opnd1 - case special:\n");
            /* Shift mantissa of larger exponent with difference of exponents.
             * We do SHR.
             */
            PrintRegDebug(DELTA_EXP);
            PrintRegDebug(SRC2_MANTISSA);
            //R(SRC2_MANTISSA) >>= R(DELTA_EXP);
            R(SRC2_MANTISSA) = 0;
            PrintRegDebug(SRC2_MANTISSA);
            // Adjust exponent accordingly; now we're radix-aligned
            R(SRC2_EXPONENT) = R(SRC1_EXPONENT);
            PrintRegDebug(SRC2_EXPONENT);

            // NOT required since last check: R(DELTA_EXP) = MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP;
        )
        EXECUTE_IN_ALL(
          PrintDebugMessage("After alignment mantissas:\n");
            PrintRegDebug(SRC1_MANTISSA);
            PrintRegDebug(SRC2_MANTISSA);
            PrintRegDebug(SRC1_EXPONENT);
            PrintRegDebug(SRC2_EXPONENT);

// IMPORTANT: Finished alignment of mantissas for the same exponent


            // If 1st operand is negative
            R(PRED3) = R(SRC1_SIGN) == R(SIGN_MASK);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) = R(PRED3) & R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
            NOP;
        )
        EXECUTE_WHERE_EQ(
          PrintDebugMessage("Before complementing mantissas - note that we keep negative mantissas only to add them, and then complement them to positive s.t. CountLeadingZeros() will work:\n");
            PrintRegDebug(SRC1_MANTISSA);
            PrintRegDebug(SRC2_MANTISSA);
            /* Where number is negative, get two's complement of mantissa
             * (i.e., the complement w.r.t. 2^16, or 0;
             *   or neg R(SRC1_MANTISSA) + 1).
             *   TODO TODO: Think if this introduces more error */
            R(SRC1_MANTISSA) = R(CT0) - R(SRC1_MANTISSA);
        )
        EXECUTE_IN_ALL(
            // If 2nd operand is negative:
          if (isSub) {
            /* If operation is actually sub.f16, we complement mantissa
               only if the number is positive.
            */
            PrintRegDebug(SRC2_SIGN);
            PrintRegDebug(SIGN_MASK);
            //R(PRED3) = R(SIGN_MASK) < R(SRC2_SIGN); // If src2 is positive
            R(PRED3) = R(SRC2_SIGN) == R(CT0); // If src2 is positive
            PrintDebugMessage("isSub==True --> PRED3 = \n");
            PrintRegDebug(PRED3);
          }
          else { // isSub == false
            R(PRED3) = R(SRC2_SIGN) == R(SIGN_MASK);
          }
            // Execute only for R(CONTINUE) == 1
            R(PRED3) = R(PRED3) & R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
            NOP;
        )
        EXECUTE_WHERE_EQ(
            /* Where number is negative, get two's complement of mantissa
             *   (i.e., the complement w.r.t. 2^16, or 0;
             *   or neg R(SRC1_MANTISSA) + 1).
             *   TODO TODO: Think if this introduces more error
             */
            R(SRC2_MANTISSA) = R(CT0) - R(SRC2_MANTISSA);
        )
        EXECUTE_IN_ALL(
          PrintDebugMessage("Mantissas before summing up:\n");
            PrintRegDebug(SRC1_MANTISSA);
            PrintRegDebug(SRC2_MANTISSA);
            //
            // Execute only for R(CONTINUE) == 1
            R(PRED3) = R(CONTINUE) == R(CT1);
            NOP;
        )
        EXECUTE_WHERE_EQ(
            /* Add mantissas (IMPORTANT note: since he did complement the
             *   mantissas where the sign bit was 1 we do NOT need to have
             *   a separate case to subtract instead of add mantissas) */
            R(SRC1_MANTISSA) += R(SRC2_MANTISSA);
          );
        EXECUTE_IN_ALL(
            /* We just added the mantissas.
             * */
          PrintDebugMessage("Mantissa result after summing up:\n");
            PrintRegDebug(SRC1_MANTISSA);
            PrintRegDebug(SRC1_EXPONENT);
            PrintRegDebug(SRC2_EXPONENT);
            // IMPORTANT: Sign bit of result is sign bit of mantissa at this stage
            R(SRC1_SIGN) = R(SRC1_MANTISSA) & R(SIGN_MASK);
            R(PRED3) = R(SRC1_SIGN) == R(SIGN_MASK);
            //
            // Execute only for R(CONTINUE) == 1
            R(PRED3) = R(PRED3) & R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
            NOP;
        )
        EXECUTE_WHERE_EQ(
            // Where mantissa is negative after addition, get absolute value
            //R(SRC1_MANTISSA) = R(SRC1_MANTISSA) - R(PRED3);
            R(SRC1_MANTISSA) = R(CT0) - R(SRC1_MANTISSA);
          PrintRegDebug(SRC1_MANTISSA);
        )
        EXECUTE_IN_ALL(
          PrintDebugMessage("Starting renormalizing mantissa:\n");
            /* Re-normalization: shift (left/right) the mantissa until we have
               a bit of 1 at index 10 (the 11th bit), if possible.

            From [Ercegovac_Digital_Arithmetic_2004], page 421:
              <<3. The normalization step requires
               - the detection of the position of the leading 1,
                   done with the block labeled LOD (Leading-One-Detector)
               - a shift performed by the shifter (no shift, right shift of
                     one position, or left shift of up to m positions)
               - the appropriate updating of the exponent.>>
            */
#ifdef OLD_RENORMALIZATION_THAT_DOESNT_WORK_FOR_DENORMAL_NUMBERS
            // Execute only for R(CONTINUE) == 1
            R(PRED3) = R(CONTINUE) == R(CT1);
            NOP;
        )
        EXECUTE_WHERE_EQ(
            /* In case we have mantissa overflow, we increment the exponent and
                     SHR with 1 pos the mantissa
               NOTE: we are guaranteed that we have only 1 extra bit
                (due to add operand overflow) besides the 10+1 bits of the
                mantissa */
            R(PRED3) = R(SRC1_MANTISSA) >> 11;
            //PrintRegDebug(PRED3);
            R(SRC1_MANTISSA) >>= R(PRED3);
            R(SRC1_EXPONENT) += R(PRED3);
        );
        EXECUTE_IN_ALL(
            PrintRegDebug(SRC1_MANTISSA);
#endif


        /* We now normalize mantissa, s.t. most significant bit of mantissa
          arrives in place of hidden bit.
        */
        /* We count the number of leading zero bits from index 10
            and SHL, if required, to normalize the result mantissa
            (we add a 1 in front of the 10 stored mantissa bits in
            fp16 word) down to 1.

           Note: this basically computes the name IHBS (index of the highest bit set).
           Ercegovac and Lang calls it the LOD (Leading-One-Detector)
               - see page 420.
        */

            //R(CONTINUE_BACKUP) = R(CONTINUE);
            R(VAL_FOR_SIZE) = R(SRC1_MANTISSA);
            CountLeadingZeros(__kernel,
                             VAL_FOR_SIZE,
                             // The result, set to AUX2 if VAL_FOR_SIZE == 0
                             NUM_BITS,
                             AUX2,
                             CT1,
                             CT16,
                             //int SRC1_MANTISSA,
                             //int SRC1_EXPONENT,
                             AUX,
                             //int PRED3,
                             CONTINUE);

            R(NUM_BITS) = R(CT16) - R(NUM_BITS);
            PrintRegDebug(NUM_BITS);
            //
            /*
            R(AUX) = F16_MANTISSA_BITS;
            R(AUX) += R(CT1);
            */
            R(AUX) = F16_MANTISSA_BITS + 1;
            //
            R(AUX) = R(NUM_BITS) - R(AUX);
            //R(AUX) = R(AUX) - R(NUM_BITS);
                //int shrPos = numBits - (F32_MANTISSA_BITS + 1);
      PrintDebugMessage("Renormalizing mantissa by AUX bits:\n");
            PrintRegDebug(AUX);

            R(PRED3) = R(CT0) < R(AUX);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) &= R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
          //PrintRegDebug(PRED3);
            NOP;
            )
          EXECUTE_WHERE_EQ(
          #ifdef ROUND_TO_NEAREST
            R(DISCARDED_BITS) = R(SRC1_MANTISSA);

           #define AUX3 PRED3
            // We store only the discarded bits to compute well T
            R(AUX3) = R(CT16) - R(AUX);
            R(DISCARDED_BITS) <<= R(AUX3); // NOT initialized
            R(DISCARDED_BITS) >>= R(AUX3);

            R(NUM_DISCARDED_BITS) = R(AUX);
    PrintRegDebug(NUM_DISCARDED_BITS);
    PrintRegDebug(SRC1_MANTISSA);
          #endif

            R(SRC1_MANTISSA) >>= R(AUX);
          PrintRegDebug(AUX);
          PrintRegDebug(SRC1_MANTISSA);
            R(SRC1_EXPONENT) += R(AUX);
          );
          EXECUTE_IN_ALL(
            R(PRED3) = R(AUX) < R(CT0);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) &= R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
          //PrintRegDebug(PRED3);
            NOP;
          )
          EXECUTE_WHERE_EQ(
            R(AUX) = R(CT0) - R(AUX);
            R(SRC1_MANTISSA) <<= R(AUX);
          PrintRegDebug(AUX);
          PrintRegDebug(SRC1_MANTISSA);
            R(SRC1_EXPONENT) -= R(AUX);
          );
          EXECUTE_IN_ALL(
      PrintDebugMessage("Finished renormalizing mantissa by AUX bits\n");
            PrintRegDebug(AUX);
            PrintRegDebug(SRC1_MANTISSA);
            PrintRegDebug(SRC1_EXPONENT);

            /*
            R(DST) = 0;
            R(SRC1_MANTISSA) = R(SRC1_MANTISSA) << 1;
            */



            // We now treat underflows
            R(PRED3) = R(SRC1_EXPONENT) < R(CT1);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) &= R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
          //PrintRegDebug(PRED3);
            NOP;
        )
        EXECUTE_WHERE_EQ(
         PrintDebugMessage("Treating underflows:");

/* MEGA-TODO (tried the code, but doesn't work on real Connex):
  In order to be able to compute G (maybe T) we should really keep in DISCARDED_BITS some of the (previous) bits if R(AUX) < ROUND_NUM_ADDITIONAL_BITS
   Maybe this case is NOT really encountered for MUL.f16, which needs to discard some bits - although we can have cases with denormals with mantissas smaller e.g. than 3 bits each.
     In a sense we've already treated this - see comments discussing R(NUM_DISCARDED_BITS) < 2 below
        - at least for R(NUM_DISCARDED_BITS) == 0 this seems correct.
        - for R(NUM_DISCARDED_BITS) == 1
            ... TODO
        - ...
   Pseudocode:
    If R(AUX) < ROUND_NUM_ADDITIONAL_BITS:
     R(DISCARDED_BITS) << (ROUND_NUM_ADDITIONAL_BITS - R(AUX));
     Put least significant ROUND_NUM_ADDITIONAL_BITS bits from most significant bits (NUM_DISCARDED_BITS - 1, NUM_DISCARDED_BITS - 2) from R(DISCARDED_BITS)
     R(AUX) = ROUND_NUM_ADDITIONAL_BITS;
    TODO: It seems this code is rather difficult to implement PERFECTLY correct.

TODO: A PARTIALLY good implementation is this - however it doesn't work on real Connex on zedboard (it does work on Opincaa simulator):
  #define TMP PRED3
   R(TMP) = ROUND_NUM_ADDITIONAL_BITS;
   PrintRegDebug(TMP);
   R(TMP) = R(AUX) < R(TMP);
   PrintRegDebug(AUX);
   PrintRegDebug(TMP);
   R(AUX) += R(TMP);
   R(DISCARDED_BITS) <<= R(TMP);
   R(AUX) += R(TMP);
   R(DISCARDED_BITS) <<= R(TMP);
*/

    PrintRegDebug(SRC1_EXPONENT);
            R(NUM_DISCARDED_BITS) = R(CT1) - R(SRC1_EXPONENT);
            //R(ROUND_AUX) = R(CT0) - R(ROUND_AUX);
            //R(SRC1_EXPONENT) = 0;
            R(SRC1_EXPONENT) = 1;
    PrintRegDebug(NUM_DISCARDED_BITS);
    PrintRegDebug(SRC1_MANTISSA);

          #ifdef ROUND_TO_NEAREST
            R(DISCARDED_BITS) = R(SRC1_MANTISSA);
           #define AUX3 PRED3
            // We store only the discarded bits to compute well T
            R(AUX3) = R(CT16) - R(NUM_DISCARDED_BITS);
            R(DISCARDED_BITS) <<= R(AUX3);
            R(DISCARDED_BITS) >>= R(AUX3);

            //R(NUM_DISCARDED_BITS) = R(AUX);
          #endif

            PrintRegDebug(NUM_DISCARDED_BITS);
            R(SRC1_MANTISSA) >>= R(NUM_DISCARDED_BITS);
            PrintRegDebug(SRC1_MANTISSA);
            PrintRegDebug(SIGN_MASK);
         );
        EXECUTE_IN_ALL(

            R(AUX) = F16_MANTISSA_MASK + 1;
            R(AUX) = R(SRC1_MANTISSA) < R(AUX);
            R(PRED3) = R(SRC1_EXPONENT) == R(CT1);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) &= R(CONTINUE);
            R(PRED3) &= R(AUX);
            R(PRED3) = R(PRED3) == R(CT1);
          //PrintRegDebug(PRED3);
            NOP;
        )
        EXECUTE_WHERE_EQ(
            // We correct a denormal: we make exponent 1 be 0:
            PrintDebugMessage("Correcting exponent 1 (denormal):");
            /* The standard IEEE 754 puts 0 - this informs us not to add a
                hidden bit to the mantissa when unpacking the f16.
            */
            R(SRC1_EXPONENT) = 0;
        );
        EXECUTE_IN_ALL(

            /* Get rid of hidden bit, which is always 1
               (since SRC1_MANTISSA contains the result mantissa).
    This doesn't require to use CONTINUE - it has no bad effect.
             */
            R(SRC1_MANTISSA) &= R(MANTISSA_MASK);

//            ); // END_WHERE_EQ
//        EXECUTE_IN_ALL(


/*
            R(PRED3) = R(SRC1_EXPONENT) < R(CT0);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) &= R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
            NOP;
          );
            EXECUTE_WHERE_EQ(
                R(SRC1_EXPONENT) = 0;
            );
*/

//        EXECUTE_IN_ALL(
            // Treating overflows
            PrintDebugMessage("Treating overflows:\n");
            R(PRED3) = 30;
            R(PRED3) = R(PRED3) < R(SRC1_EXPONENT);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) = R(PRED3) & R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
            NOP;
          );
            EXECUTE_WHERE_EQ(
              PrintRegDebug(CONTINUE);
//                R(SRC1_EXPONENT) = 31;
//                R(SRC1_MANTISSA) = 0;

                R(CONTINUE) = 0;
                R(DST) = F16_INF_POSITIVE;
                // Put in result sign bit
                R(DST) |= R(SRC1_SIGN);
            );


        EXECUTE_IN_ALL(
            // Execute only for R(CONTINUE) == 1
            R(AUX) = R(CONTINUE) == R(CT1);
         PrintDebugMessage("Computing RND - active lanes are:\n");
PrintRegDebug(AUX);
            NOP;
          );
          EXECUTE_WHERE_EQ(
            // Put f16 result back together in R(DST)
            PrintDebugMessage("Starting to pack the result:\n");
            // Shift the exponent in place
            R(DST) = R(SRC1_EXPONENT) << F16_MANTISSA_BITS;
            /* NOT HERE since we do rounding below: Put in result sign bit
            R(DST) = R(DST) | R(SRC1_SIGN); */
            // Put in result mantissa
            R(DST) = R(DST) | R(SRC1_MANTISSA);

          #ifdef ROUND_TO_NEAREST
           /*
            [Ercegovac_Digital_Arithmetic_2004, Section 8.4.3]:
                rnd = G (L + R + T)
           */
          PrintRegDebug(DISCARDED_BITS);
          PrintRegDebug(NUM_DISCARDED_BITS);
            R(L) = R(SRC1_MANTISSA) & R(CT1);
          PrintDebugMessage("Rounding to nearest (if tie to even):");
          PrintRegDebug(NUM_DISCARDED_BITS);
          PrintDebugMessage("  L = ");
          PrintRegDebug(L);

            // We need to compute the sticky bit, T, as OR over all the values.
            // We first compute G.
            R(AUX2) = R(NUM_DISCARDED_BITS) - R(CT1); // NOT initialized
            R(AUX) = R(CT1) << R(AUX);
            R(G) = R(DISCARDED_BITS) & R(AUX); // NOT initialized
            //
            // We take out bit G from R(DISCARDED_BITS):
            R(DISCARDED_BITS) ^= R(G);
            //
            R(G) = R(G) == R(CT0);
            R(G) = R(CT1) - R(G);

            // We now compute Rbit.
            /*
            R(AUX2) -= R(CT1);
            R(AUX) = R(CT1) << R(AUX);
            */
            R(AUX) >>= 1;
            R(Rbit) = R(DISCARDED_BITS) & R(AUX); // NOT initialized
            //
            // We take out bit Rbit from R(DISCARDED_BITS):
            R(DISCARDED_BITS) ^= R(Rbit);
            //
            R(Rbit) = R(Rbit) == R(CT0);
            R(Rbit) = R(CT1) - R(Rbit);

            R(T) = R(DISCARDED_BITS) == R(CT0); // NOT initialized
            R(T) = R(CT1) - R(T);

          PrintDebugMessage("  L = ");
          PrintRegDebug(L);
          PrintDebugMessage("  G = ");
          PrintRegDebug(G);
          PrintDebugMessage("  R = ");
          PrintRegDebug(Rbit);
          PrintDebugMessage("  T = ");
          PrintRegDebug(T);

           #define SHORTER_CODE_WORKS_IF_INITIALIZED
           #ifdef SHORTER_CODE_WORKS_IF_INITIALIZED
            R(RND) = R(L) | R(Rbit); // NOT initialized. It's also an ORV_H, which is used for COPY instructions in llc. We advise not to use it to avoid confusing pass PassAfterPostRAScheduler in ConnexTargetMachine.cpp.
            R(RND) |= R(T);
           #else
            R(L) |= R(Rbit);
            R(L) |= R(T);
            R(RND) = R(L);
           #endif
            //
            R(RND) &= R(G);
           PrintDebugMessage("  G (L + R + T) = ");
           PrintRegDebug(RND);


            R(DST) += R(RND);
          #endif

            // Put in result sign bit
            R(DST) |= R(SRC1_SIGN);
        )
        EXECUTE_IN_ALL(



            // store result
            LS[resPtr] = R(DST);

            // End of program synchronization point; host will wait for this
            REDUCE(R1);
        )
    END_KERNEL(kernelName);
}



int FloatAddSubTest(ConnexMachine *connex) {
    uint16_t opA[CONNEX_VECTOR_LENGTH];
    uint16_t opB[CONNEX_VECTOR_LENGTH];
    uint16_t resCorrect[CONNEX_VECTOR_LENGTH];
    uint16_t result[CONNEX_VECTOR_LENGTH];

    /*
        // Note: 1.3193359375 + 4.4375 = 5.7568359375 = 0 100-01 01-1100-0001 (2) = 45 C1 (16)
        //    The float result (fp16) is converted to binary using from http://oletus.github.io/float16-simulator.js/

    i=0: opA = 0x3d47, opB = 0x4470 --> res = 0x45c1
    i=1: opA = 0xbd47, opB = 0x4470 --> res = 0x423e
    i=2: opA = 0x3d47, opB = 0xc470 --> res = 0xc23e
    i=3: opA = 0xbd47, opB = 0xc470 --> res = 0xc5c1

    // Obtained from /home/asusu/LLVM/llvm38Nov2016/llvm/build40/bin/Tests/NEW_v128i16/opincaa_standalone_apps/FP16/C/LLVM/_Float16/Gen_constant_values/Float16.ll (and the _O3....ll file also):
      0x3D47, 0x4470 --> 45C2
      0xBD47, 0x4470     423C
      0x3D47, 0xC470     C23C
      0xBD47, 0xC470     C5C2
    */

    opA[0] = 0x3D47; // F16 encoding for 1.3193359375
    opB[0] = 0x4470; // F16 encoding for 4.4375
    resCorrect[0] = 0x45C2; // as obtained in Clang
//
    opA[0] = 0xc233; // F16 encoding for (S=1,E=0x10,F=0x633)
    opB[0] = 0xd79f; // F16 encoding for (S=1,E=0x15,F=0x79f)
    resCorrect[0] = 0xd7d1; // as obtained in ARM __fp16 (S=1,E=0x15,F=0x7d1)
//
    // Problems if using MAX_DIFF_EXPONENTS 5 - due to sign issue on i16
    opA[0] = 0xc4c9; // F16 encoding for (S=1,E=0x11,F=0x4c9)
    opB[0] = 0x079a; // F16 encoding for (S=0,E=0x1,F=0x79a)
    resCorrect[0] = 0xc4c9; // as obtained in ARM __fp16 (S=1,E=0x11,F=0x4c9)
//
    // Problems if using MAX_DIFF_EXPONENTS 4 - due to sign issue of mantissa result on i16
    opA[0] = 0xb77c; // F16 encoding for (S=1,E=0xd,F=0x77c)
    opB[0] = 0xc794; // F16 encoding for (S=1,E=0x11,F=0x794)
    resCorrect[0] = 0xc806; // as obtained in ARM __fp16 (S=1,E=0x12,F=0x406)
//
    opA[0] = 0x8a4a; // F16 encoding for (S=1,E=0x2,F=0x64a)
    opB[0] = 0x0af8; // F16 encoding for (S=0,E=0x2,F=0x6f8)
    resCorrect[0] = 0x015c; // as obtained in ARM __fp16 (S=0,E=0x0,F=0x15c)
//
    opA[0] = 0x944a; // F16 encoding for (S=1,E=0x5,F=0x44a)
    opB[0] = 0x58ec; // F16 encoding for (S=0,E=0x16,F=0x4ec)
    resCorrect[0] = 0x58ec; // as obtained in ARM __fp16 (S=0,E=0x16,F=0x4ec)
//
    // Testing that doing SHR for 16+ positions is done in "two parts" to works on Connex
    opA[0] = 0xc4c9; // F16 encoding for (S=1,E=0x11,F=0x4c9)
    opB[0] = 0x079a; // F16 encoding for (S=0,E=0x1,F=0x79a)
    resCorrect[0] = 0xc4c9; // as obtained in ARM __fp16 (S=1,E=0x11,F=0x4c9)
    // On zedboard I get: res = 0x41a2(S=0,E=0x10,F=0x5a2)
//
#ifdef DO_SUB
        opA[0] = 0x0000; // F16 encoding for (S=1,E=0x11,F=0x4c9)
        opB[0] = 0x079a; // F16 encoding for (S=0,E=0x1,F=0x79a)
        resCorrect[0] = 0x879a; // as obtained in ARM __fp16 (S=1,E=0x11,F=0x4c9)
#else
        //opA[0] = 0x0000; // F16 encoding for (S=1,E=0x11,F=0x4c9)
        //opB[0] = 0x079a; // F16 encoding for (S=0,E=0x1,F=0x79a)
        opA[0] = 0x079a; // F16 encoding for (S=1,E=0x11,F=0x4c9)
        opB[0] = 0x0000; // F16 encoding for (S=0,E=0x1,F=0x79a)
        resCorrect[0] = 0x079a; // as obtained in ARM __fp16 (S=1,E=0x11,F=0x4c9)
#endif
/*
    opA[0] = 0x0400;
    opB[0] = 0x8401;
    resCorrect[0] = 0x8001;

    opA[0] = 0x3D47; // F16 encoding for 1.3193359375
    opB[0] = 0x3D46; // F16 encoding for ...
    resCorrect[0] = 0x3c01;

    // These are denormal values, which when subtracted we obtain denormal
    opA[0] = 0x0547; // F16 encoding for 1.3193359375
    opB[0] = 0x0546; // F16 encoding for ...
    resCorrect[0] = 0x0001;
*/
// MEGA-TODO: test values with big difference on exponents (e.g. EXP1 = 1, EXP2 = 30) that will require using PROBABLY more than i16 for mantissa - think about it

    /* VERY IMPORTANT: This test is very important - A is negative, B positive
       (so the operation is basically subtraction).
         It seems for this case aligning the mantissas before doing the actual
           "add" operation is performed by bringing to the SMALLEST exponent
           both operands - NOT doing so seems to result in bigger errors w.r.t
           clang/arm.gcc's f16 - I get 0x423e.
    */
    opA[1] = 0xBD47; // F16 encoding for -1.3193359375
    opB[1] = 0x4470; // F16 encoding for 4.4375
    resCorrect[1] = 0x423C; // as obtained in Clang
/*
151h
1.0101010001
470h
1.0001110000

adding mantissas gives:
    - 1.011100000111
//1011 1000 001 11
1 011100000111
1.0111000001

Reported result mantissa:
1.0111000010
*/

    opA[2] = 0x3D47; // F16 encoding for 1.3193359375
    opB[2] = 0xC470; // F16 encoding for -4.4375
    resCorrect[2] = 0xC23C; // as obtained in Clang
/*
Reported result mantissa:
    1.1000111100
*/
    opA[3] = 0xBD47; // F16 encoding for -1.3193359375
    opB[3] = 0xC470; // F16 encoding for -4.4375
    resCorrect[3] = 0xC5C2; // as obtained in Clang

    opA[4] = 0x4470; // F16 encoding for 4.4375 because: sign = 0; exp = 10001(2) = 17(10); significand = 0001110000 so value is 1.0001110000 * 2^(17 - 15) = [1 + 2^(-4) + 2^(-5) + 2^(-6)] * 4 = 4.4375
    opB[4] = 0xBD47; // F16 encoding for -1.3193359375
    resCorrect[4] = 0x423C; // as obtained in Clang

    opA[5] = F16_NAN; // F16 encoding for a NaN
    opB[5] = F16_NAN; // F16 encoding for a NaN
    resCorrect[5] = F16_NAN_2;

    opA[6] = F16_NAN; // F16 encoding for a NaN
    opB[6] = F16_NAN_2; // F16 encoding for NaN
    resCorrect[6] = F16_NAN_2;

    opA[7] = 0x3D47; // F16 encoding for ...
    opB[7] = F16_NAN; // F16 encoding for NaN
    resCorrect[7] = F16_NAN_2;

    opA[8] = F16_NAN;
    opB[8] = 0x3D47;
    resCorrect[8] = F16_NAN_2;

    opA[9] = 0x3D47; // F16 encoding for ...
    opB[9] = F16_INF_POSITIVE; // F16 encoding for +Inf
    resCorrect[9] = F16_INF_POSITIVE;

    opA[10] = 0x3D47; // F16 encoding for ...
    opB[10] = F16_INF_NEGATIVE; // F16 encoding for -Inf
    resCorrect[10] = F16_INF_NEGATIVE;

    opA[11] = F16_INF_NEGATIVE;
    opB[11] = 0x3D47;
    resCorrect[11] = F16_INF_NEGATIVE;

    opA[12] = F16_INF_POSITIVE;
    opB[12] = 0x3D47;
    resCorrect[12] = F16_INF_POSITIVE;

    opA[13] = F16_INF_NEGATIVE; // F16 encoding for -Inf
    opB[13] = F16_INF_NEGATIVE; // F16 encoding for -Inf
    resCorrect[13] = F16_INF_NEGATIVE;

    opA[14] = F16_INF_POSITIVE; // F16 encoding for Inf
    opB[14] = F16_INF_POSITIVE; // F16 encoding for Inf
    resCorrect[14] = F16_INF_POSITIVE;

    opA[15] = F16_INF_NEGATIVE; // F16 encoding for -Inf
    opB[15] = F16_INF_POSITIVE; // F16 encoding for Inf
    resCorrect[15] = F16_NAN;

    opA[16] = F16_INF_NEGATIVE; // F16 encoding for -Inf
    opB[16] = 0x3D47; // F16 encoding for ...
    resCorrect[16] = F16_INF_NEGATIVE;

    opA[17] = F16_INF_POSITIVE; // F16 encoding for +Inf
    opB[17] = 0x3D47; // F16 encoding for ...
    resCorrect[17] = F16_INF_POSITIVE;

    opA[18] = 0x0001; // F16 encoding for denormal...
    opB[18] = 0x3D47; // F16 encoding for ...
    resCorrect[18] = 0x3D47;

    opA[19] = 0x3D47; // F16 encoding for ...
    opB[19] = 0x0001; // F16 encoding for denormal...
    resCorrect[19] = 0x3D47;

    opA[20] = 0x0400; // F16 encoding for 2^(1 - 15) * 1.0000000000
    opB[20] = 0x8401; // F16 encoding for -2^(1 - 15) * 1.0000000001
    resCorrect[20] = 0x8001;
    // Currently returns result: 0x8000, which means negative zero (-0)
    //
    opA[21] = 0x7BFF; // F16 encoding for 2^(31 - 15) * 1.1111111111(2) (exponent is: 11110)
    opB[21] = 0x7BFF; // F16 encoding for 2^(31 - 15) * 1.1111111111(2) (exponent is: 11110)
    // Currently returns result: 0x7fff, which is normal because the mantissa of the result (before truncation) is , so we increase the exponent by 1
    resCorrect[21] = F16_INF_POSITIVE; //F16_NAN;


    // Checking that x + (-x) is equal 0
    opA[22] = 0x0880; // F16 encoding for 2^(2 - 15) * 1.0010000000
    opB[22] = 0x8880; // F16 encoding for -2^(2 - 15) * 1.0010000000
    resCorrect[22] = 0;

    // Treat overflows after add
    opA[23] = 0x7B53; // F16 encoding for 60000
    opB[23] = 0x7B53; // F16 encoding for 60000
    resCorrect[23] = F16_INF_POSITIVE;

    // Treat denormals
    opA[24] = 0x0002; // F16 encoding for ...
    opB[24] = 0x0001; // F16 encoding for ...
    resCorrect[24] = 0x0003;

  #define NUM_VALS 25

    for (int i = NUM_VALS; i < CONNEX_VECTOR_LENGTH; i++) {
        opA[i] = 0;
        opB[i] = 0;
        resCorrect[i] = 0;
    }

    /*
    // From /home/alarm/OpincaaLLVM/opincaa_standalone_apps/Emulate_F16/C/2/STDout_001
    opA[i] = 0x3d47
    opB[i] = 0x4470
    (__fp16)opA[i] + (__fp16)opB[i] = 5.757
    opA[i] + opB[i] = 0x45c2
      (res = 5.757812)

    opA[i] = 0xbd47
    opB[i] = 0x4470
    (__fp16)opA[i] + (__fp16)opB[i] = 3.118
    opA[i] + opB[i] = 0x423c
      (res = 3.117188)

    opA[i] = 0x3d47
    opB[i] = 0xc470
    (__fp16)opA[i] + (__fp16)opB[i] = -3.118
    opA[i] + opB[i] = 0xc23c
      (res = -3.117188)

    opA[i] = 0xbd47
    opB[i] = 0xc470
    (__fp16)opA[i] + (__fp16)opB[i] = -5.757
    opA[i] + opB[i] = 0xc5c2
      (res = -5.757812)
    */

#ifdef DO_SUB
    AddSub_f16Kernel(0, 1, 2, true);
#else
    AddSub_f16Kernel(0, 1, 2);
#endif

    connex->writeDataToConnex(opA, 1, 0);
    connex->writeDataToConnex(opB, 1, 1);

#ifdef LLVM_ISEL_CODEGEN
    //string kernelName = "add_or_sub.f16";
    Kernel *kernel = connexGlobal->getKernel(kernelName);
    kernel->sdNodeVarNameRegDef[SRC1] = "nodeOpSrcCast1";
    kernel->sdNodeVarNameRegDef[SRC2] = "nodeOpSrcCast2";
    //
    // For ADD fp16:
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

    //printf("ADD results are %x %x %x %x %x\n", result[0], result[1], result[2], result[3], result[4]);
#ifdef DO_SUB
    printf("SUB results are:\n");
#else
    printf("ADD results are:\n");
#endif
    /*
    for (int i = 0; i < 23; i++) {
        if (isnan_f16(result[i]) || isnan_f16(resCorrect[i])) {
            assert(isnan_f16(result[i]) && isnan_f16(resCorrect[i]));
            printf("i=%d: opA = 0x%04x, opB = 0x%04x --> res = NAN (resCorrect = NAN)\n",
                    i, opA[i], opB[i]);
        }
        else {
            printf("i=%d: opA = 0x%04x, opB = 0x%04x --> res = 0x%04x (resCorrect = 0x%04x)\n",
                    i, opA[i], opB[i], result[i], resCorrect[i]);
        }
    }
    */
    for (int i = 0; i < NUM_VALS; i++) {
        if (isnan_f16(result[i]) || isnan_f16(resCorrect[i])) {
            //assert(isnan_f16(result[i]) && isnan_f16(resCorrect[i]));
        }
        /*
        printf("i=%d: opA = %s, opB = %s --> res = %s (resCorrect = %s)%s\n",
                i,
                GetStringForF16(opA[i]).c_str(),
                GetStringForF16(opB[i]).c_str(),
                GetStringForF16(result[i]).c_str(),
                GetStringForF16(resCorrect[i]).c_str(),
                abs(resCorrect[i] - result[i]) <= 0 ? "" :
                   ((isnan_f16(resCorrect[i]) && isnan_f16(result[i])) ?
                     "" : " (different results!)")
              );
        */
         printf("i=%d: opA = %s, opB = %s --> res = %s (resCorrect = %s)%s - error = %d %s\n",
               i,
                GetStringForF16(opA[i]).c_str(),
                GetStringForF16(opB[i]).c_str(),
                GetStringForF16(result[i]).c_str(),
                GetStringForF16(resCorrect[i]).c_str(),
              #ifdef ROUND_TO_NEAREST
                (resCorrect[i] == result[i]) ? "" :
              #else
                abs(resCorrect[i] - result[i]) <= 1 ? "" :
              #endif
                   ((isnan_f16(resCorrect[i]) && isnan_f16(result[i])) ?
                     "" : " (different results!)"),
                ((int)resCorrect[i]) - ((int)result[i]),
                ((abs(resCorrect[i] - result[i]) > 2) && (!(isnan_f16(resCorrect[i]) && isnan_f16(result[i])))) ? "BIG!" : ""
                /*(abs(resCorrect[i] - result[i]) > 2) &&
                    !(isnan_f16(resCorrect[i]) && isnan_f16(result[i]))
                   ? "BIG!" : "" */
              );
    }
    printf("\n");
}


void Test() {
    FloatAddSubTest(connexGlobal);
}


