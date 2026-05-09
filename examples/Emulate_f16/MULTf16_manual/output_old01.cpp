#include <iostream>
#include "ConnexMachine.h"


//#define LLVM_ISEL_CODEGEN
#ifdef LLVM_ISEL_CODEGEN
  #define PRINTREG(regNum) regNum
  #define PrintDebugMessage(aStr) aStr
  #define PrintRegDebug(regNum) regNum
#endif


// These Opincaa subroutines should be defined after the #define LLVM_ISEL_CODEGEN
//#include "../LibMisc.h"
#include "LibMisc.h"

using namespace std;


/* An implementation of MUL.f16 which I strongly believe is IEEE 754-2008
  compliant - I tested it seriously with GCC 7.2 on zedboard.arh.pub.ro
  and I obtained less than 1% 1-unit errors for 128,000 elements and I believe
  it's the mistake of GCC due to conversion f16<->f32.
*/

/*
Denormals, infinities and underflows are very well explained in pages
 401 and 402 (Figure 8.2.a) of book [Ercegovac_Digital_Arithmetic_2004]
 (also at
  https://books.google.ro/books/about/Digital_Arithmetic.html?id=p79cu3nZ6yoC&redir_esc=y).
*/

// If we comment ROUND_TO_NEAREST we use round toward zero
#define ROUND_TO_NEAREST
#ifdef ROUND_TO_NEAREST
  #define COMPUTE_BETTER_T_BIT
#endif

/*
I think the round mode we use for f16 is truncation (round toward zero)
    - see 8.2.2 Round Toward Zero (Truncation)

Note: x86 (and probably all the other processors) seems to implement by
  default rounding to nearest
    - see https://docs.microsoft.com/en-us/cpp/build/mxcsr:
      "set to the following standard values at the start of program execution:"
      "MXCSR[13:14]   : Rounding  control - 0 (round to nearest)".
    (see also https://software.intel.com/en-us/articles/x87-and-sse-floating-point-assists-in-ia-32-flush-to-zero-ftz-and-denormals-are-zero-daz)
*/



// It seems I'm treating well NaNs, INFs, denormal numbers
void Mul_f16Kernel(int32_t opAPtr, int32_t opBPtr, int32_t resPtr) {
    BEGIN_KERNEL("mul.f16");
        EXECUTE_IN_ALL(

    // Register allocation table for the variables used in the program
    #define CT0            31
    #define CT1            30
    #define CT16           29
    #define CT31           28
    // Keep SRC1 < SRC2
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
    #define DST_MANTISSA_L 18
    #define DST_MANTISSA_H 17
    #define DST_EXPONENT   16
    #define DST_SIGN       15
    //
    //#define PRED1          15
    //#define PRED1A         14
    #define PRED2          14
    #define PRED2A         13
    #define PRED3          12
    //#define PRED4          10
    //
    #define INPUT_EXP31    11
    #define CONTINUE        0
    //#define CONTINUE_BACKUP 8
    //#define HAVE_INPUT_NAN 8
    //#define HAVE_INPUT_ZERO 8
    //
    #define MANTISSA_MASK  10
    #define EXPONENT_MASK   9
    #define SIGN_MASK       8
    #define HIDDENBIT_MASK  7

    #define VAL_FOR_SIZE    6
    #define NUM_BITS        5
    //#define PRED3 31
    #define AUX             4
    #define AUX2            3

    // Note that these variables are already allocated to other register
    #define V5              5
    #define V4              4
    #define V3              3
    #define V2              2
    #define V1              1
    #define IS_NAN_MIN      0

  #ifdef ROUND_TO_NEAREST
    #define L               3
    #define G               2
    #define T               0
    #define RND             SRC1_SIGN
    #define ROUND_NUM_ADDITIONAL_BITS 2

    /* DISCARDED_BITS stores the mantissa bits: G and T.
        G and T are normally discarded by the SHR operations we perform
        (either because the result mantissa has more than F16_MANTISSA_BITS or
        because the exponent < 0).
    */
    #define DISCARDED_BITS SRC1_SIGN
    /* NUM_DISCARDED_BITS represents the number of bits discarded from the mantissa
        (or number of steps we performed SHR on the result mantissa computed
       originally by multiplication). */
    #define NUM_DISCARDED_BITS  SRC2_SIGN
  #endif

            R(SRC1) = LS[opAPtr]; // load 1st F16 operand
            R(SRC2) = LS[opBPtr]; // load 2nd F16 operand

            R(CT0) = 0;
            R(CT1) = 1;
            R(CT16) = 16;
            R(CT31) = 31;

            R(MANTISSA_MASK)  = F16_MANTISSA_MASK;
            R(EXPONENT_MASK)  = F16_EXPONENT_MASK;
            R(SIGN_MASK)      = F16_SIGN_MASK;
            R(HIDDENBIT_MASK) = F16_HIDDENBIT_MASK;


          #ifdef ROUND_TO_NEAREST
            R(DISCARDED_BITS) = 0; // There's no guarantee below we initialize
            //R(NUM_DISCARDED_BITS) = -1;
          #endif


/* TODO:
 *   small-TODO: UnpackF16() and the following code below both check for EXP == 31
 *      - the SIGN should not be put in a separate register for both SRC1 and
 *          SRC2 but only for the result
 *      - we should check maybe only once
 *
 * it is possible that we do NOT need to remove hidden bit for
 * INF for our MUL operation --> try to optimize.
 *   Actually INF unpacked with mantissa 0 is very useful since it creates
 *     a result mantissa 0 and this becomes INF result (unless the other
 *     operand is NAN).
 */
            UnpackF16(__kernel,
                        CT0, CT1, CT31,
                        SRC1, SRC1_SIGN, SRC1_EXPONENT,
                        SRC1_MANTISSA,
                        SIGN_MASK, EXPONENT_MASK, MANTISSA_MASK,
                        HIDDENBIT_MASK,
                        PRED2, PRED2A, PRED3);

            UnpackF16(__kernel,
                        CT0, CT1, CT31,
                        SRC2, SRC2_SIGN, SRC2_EXPONENT,
                        SRC2_MANTISSA,
                        SIGN_MASK, EXPONENT_MASK, MANTISSA_MASK,
                        HIDDENBIT_MASK,
                        PRED2, PRED2A, PRED3);
            /*
            R(SRC1_MANTISSA) = R(SRC1) & R(MANTISSA_MASK);
            // Add hidden bit for the mantissa (from bit 0, as it is initially)
            R(SRC1_MANTISSA) |= R(HIDDENBIT_MASK);
            PrintRegDebug(SRC1_MANTISSA);
            R(SRC1_EXPONENT) = R(SRC1) & R(EXPONENT_MASK);
            // Get the exponent from bit 0 (shift down to LSB)
            R(SRC1_EXPONENT) >>= F16_MANTISSA_BITS;
            PrintRegDebug(SRC1_EXPONENT);
            // R(SRC1_SIGN) contains the sign on bit 15
            R(SRC1_SIGN) = R(SRC1) & R(SIGN_MASK);

            R(SRC2_MANTISSA) = R(SRC2) & R(MANTISSA_MASK);
            // Add hidden bit for the mantissa (from bit 0, as it is initially)
            R(SRC2_MANTISSA) |= R(HIDDENBIT_MASK);
            PrintRegDebug(SRC2_MANTISSA);
            R(SRC2_EXPONENT) = R(SRC2) & R(EXPONENT_MASK);
            // Get the exponent (shift down to LSB)
            //R(SRC2_EXPONENT) = R(SRC2_EXPONENT) >> F16_MANTISSA_BITS;
            R(SRC2_EXPONENT) >>= F16_MANTISSA_BITS;
            PrintRegDebug(SRC2_EXPONENT);
            */

            // R(SRC2_SIGN) contains the sign on bit 15
            R(SRC2_SIGN) = R(SRC2) & R(SIGN_MASK);

       // IMPORTANT: We use INPUT_EXP31 to mean if we have any opnd NAN.
            // Check if 1st opnd is NAN
            R(INPUT_EXP31) = R(SRC1_EXPONENT) == R(CT31);
            /*
            R(PRED3) = R(SRC1_EXPONENT) == R(CT31);
          //PrintRegDebug(PRED3);
            R(CONTINUE) = R(SRC1_MANTISSA) == R(CT0);
          //PrintRegDebug(CONTINUE);
            R(CONTINUE) = R(CT1) - R(CONTINUE);
          //PrintRegDebug(CONTINUE);
            R(CONTINUE) &= R(PRED3);
          PrintRegDebug(CONTINUE);
            */
            //
            // Check if 2nd opnd is NAN
            R(PRED3) = R(SRC2_EXPONENT) == R(CT31);
            /*
          //PrintRegDebug(PRED3);
            R(PRED4) = R(SRC2_MANTISSA) == R(CT0);
          //PrintRegDebug(PRED4);
            R(PRED4) = R(CT1) - R(PRED4);
          //PrintRegDebug(PRED4);
            R(PRED4) &= R(PRED3);
            R(CONTINUE) |= R(PRED4);
            */
            R(INPUT_EXP31) |= R(PRED3);
          PrintRegDebug(INPUT_EXP31);

            // Compute sign
            R(DST_SIGN) = R(SRC1_SIGN) ^ R(SRC2_SIGN);
          PrintRegDebug(DST_SIGN);
            /* Add exponents and re-bias.
               The re-bias is necessary because:
                for 2 arbitrary operands the bias is used like this:

                 Note: For the general case,
                    E is the field packed in f16, e is the actual exponent.
                    So e = E - bias, where bias = 15 for f16
                 We have:
                    E1 = e1 + bias
                    E2 = e2 + bias
                    E1 + E2 = e1 + e2 + 2 * bias
                    E1 + E2 - bias = e1 + e2 + bias

                    Eres = eres + bias
                    eres = e1 + e2 = E1 - bias + E2 - bias = E1 + E2 - 2 * bias
                   So:
                    Eres = E1 + E2 - bias
                    This is why we subtract the bias from E1 + E2.
            */
            R(DST_EXPONENT) = R(SRC1_EXPONENT) + R(SRC2_EXPONENT);
            R(AUX) = 15;
            R(DST_EXPONENT) -= R(AUX);
         PrintDebugMessage("After re-bias:");
          PrintRegDebug(DST_EXPONENT);

            /* Multiply mantissas (with hidden bits included if available).
               We get a 22-bit result in two 16-bit registers. */
            R(SRC1_MANTISSA) * R(SRC2_MANTISSA);
            R(DST_MANTISSA_L) = MULT_LOW();
            R(DST_MANTISSA_H) = MULT_HIGH();

         PrintDebugMessage("DST_MANTISSA_L/H (original):");
          PrintRegDebug(SRC1_MANTISSA);
          PrintRegDebug(SRC2_MANTISSA);
          PrintRegDebug(DST_MANTISSA_L);
          PrintRegDebug(DST_MANTISSA_H);
          //
          PrintRegDebug(SRC1_EXPONENT);
          PrintRegDebug(SRC2_EXPONENT);
          PrintRegDebug(DST_EXPONENT);





          /* We defer the correction of the DST_EXPONENT < 0 after we
               renormalize the DST_MANTISSA. */



/* This macro should always be DEFINED.
   Comment it only if we look to optimize for case when we have also denormal
   input operands. */
#define TREAT_DENORMALS
#ifdef TREAT_DENORMALS
            /*
            As said in [Ercegovac_Digital_Arithmetic_2004, Section 8.5.3]:
            "As in addition, denormal operands do not have a hidden 1.
            When one (or both) operands are denormal, then the output of
            the multiplier will have leading zeros. Consequently, a
            variable left shift is necessary for normalization, as in floating-
            point addition (and a subtraction in the exponent)."

            So we need to normalize the multiplication result:
                - SHL the result for normalization if it is denormal
                - (otherwise, SHR if it has 2 * F16_MANTISSA_BITS + 1 useful bits).

            Treating denormals takes about ... more instructions (if having
              the BITREV Connex instruction - otherwise it would take even
              longer).
            */

            /*
            We compute in
              NUM_BITS = number of significant bits multiplication result
               - to compute this is I guess simpler than computing for each
                 operand in part.
              The multiplication result is a value with at most 21 bits
              (or 22 if both operands are not denormals and are big,
                 and at least 0 bits),
                 since the input mantissas have at most 11 significant bits each
                 and one is denormal so it has at most 10 significant bits.
               - best is to check if R(DST_MANTISSA_H) == 0
                  - if true, compute numBitsRes = f(R(DST_MANTISSA_L))
                  - if false, compute numBitsRes = f(R(DST_MANTISSA_H)) + 16
                 where f(x) = 16 - CountNumberLeadingZeros(x) .

            We could also compute leading zeros for
               fraction of both denormal operands, numLeadingZerosDenormal.
            */

            /*
              We prepare to compute the number of significant bits in the
                32-bit word R(DST_MANTISSA_L)|R(DST_MANTISSA_H).
             Note: this is similar to computing IHBS (index of the highest bit set;
              Ercegovac and Lang calls it the LOD (Leading-One-Detector)
               - see page 420).
             The value we need = IHBS() + 1. */
  #define OR_SCAN_BITS_FOR_RATHER_EFFICIENT_CTLZ
          #ifdef OR_SCAN_BITS_FOR_RATHER_EFFICIENT_CTLZ
            R(AUX2) = 16;
          #else
            /* We initialize to 17 because my CONNEX_HAS_BITREVERSE technique
              (and also the inefficient one with host-side for loop)
                returns the result of CTLZ + 1.
            */
            R(AUX2) = 17;
          #endif
            R(VAL_FOR_SIZE) = R(DST_MANTISSA_L);

            R(AUX) = R(CT0) < R(DST_MANTISSA_H);
            NOP;
           );
          EXECUTE_WHERE_LT(
            R(VAL_FOR_SIZE) = R(DST_MANTISSA_H);
          #ifdef OR_SCAN_BITS_FOR_RATHER_EFFICIENT_CTLZ
            R(AUX2) = 16 + 16;
          #else
            /* We initialize to 17 + 16 because my CONNEX_HAS_BITREVERSE
                technique (and also the inefficient one with host-side for loop)
                returns the result of CTLZ + 1.
            */
            R(AUX2) = 17 + 16;
          #endif
          );
          EXECUTE_IN_ALL(

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



            /*
            If R(AUX2) == 17
                R(NUM_BITS) = ;
            */
            R(NUM_BITS) = R(AUX2) - R(NUM_BITS);
           PrintDebugMessage("Num significant bits for the result of mantissa multiplication:\n");
           PrintRegDebug(NUM_BITS);










            /* VERY VERY IMPORTANT: Now we renormalize the result of the
                  multiplication of mantissas, even for denormals, as
                  said in [Ercegovac_Digital_Arithmetic_2004, Section 8.5.3].
               Normalization means that we make the result of mantissa
                 multiplication be stored on only F16_MANTISSA_BITS + 1
                   significant bits.
                This is done by a SHR by NUM_BITS - (F16_MANTISSA_BITS + 1),
                   which becomes a SHL if this value is negative.

             Note: On May 4th and 5th, 2018 I tested on zedboard.arh.pub.ro
                that on Connex, when given
                  Rdst = SHR Rop1, -1 // or other negative value
               actually just puts 0 into Rdst.

               Also, Rdst = SHR Rop1, 16
                   just copies Rop1 into Rdst, which is NOT correct

               Note that Rdst = SHR Rop1, 15 does the correct operation.

               For this see folder /home/alarm/OpincaaLLVM/Test_SHR_special_2nd_opnd .
            */

            /* We SHR by AUX (= F16_MANTISSA_BITS + 1) positions both
               DST_MANTISSA_H and DST_MANTISSA_L together in order
               to keep the significant
                bits of the entire 32-bits (both _L and _H) of result of
                mantissa.
               This makes DST_MANTISSA_L hold F16_MANTISSA_BITS + 2 bits,
                 at most, or less (especially if we have denormals).

               After, this, the mantissa contains, as expected, the hidden bit.
             Note: we change DST_EXPONENT below
            */
            R(AUX) = F16_MANTISSA_BITS + 1;
            R(AUX) = R(NUM_BITS) - R(AUX);
            // Note: Now R(AUX) is smaller than 22 - 11 = 11 .
           PrintDebugMessage("AUX used to SHR mantissa result multiplication:\n");
           PrintRegDebug(AUX);

            R(AUX2) = R(AUX) < R(CT0);
            NOP;
          );
          EXECUTE_WHERE_LT(
            //R(AUX) = 0;
            /* This should happen only for denormals:
                 we renormalize the mantissa. */
            R(AUX2) = R(CT0) - R(AUX);
            R(DST_MANTISSA_L) <<= R(AUX2);
          );
          EXECUTE_IN_ALL(
           PrintDebugMessage("Adjusted AUX:\n");
           PrintRegDebug(AUX);
           PrintRegDebug(AUX2);
           PrintRegDebug(DST_MANTISSA_L);


            R(AUX2) = R(CT0) < R(AUX);
            NOP;
          );
          EXECUTE_WHERE_LT(
            /* This should happen only for NON-denormals. */
           PrintRegDebug(AUX);
            /*
            R(DST_MANTISSA_L) >>= R(AUX);
          //PrintRegDebug(DST_MANTISSA_L);
            //
            R(AUX) = R(CT16) - R(AUX);
           PrintRegDebug(AUX);
            R(DST_MANTISSA_H) <<= R(AUX);
          PrintRegDebug(DST_MANTISSA_H);
            //
            R(DST_MANTISSA_L) |= R(DST_MANTISSA_H);
            */

            R(AUX2) = R(DST_MANTISSA_H);
            R(DST_MANTISSA_H) >>= R(AUX);
            //
//            R(PRED3) = R(CT16) - R(AUX);
          #define AUX_ORIG PRED3
            // small-TODO #ifndef ROUND_TO_NEAREST we could take out R(AUX_ORIG) = R(AUX);
            R(AUX_ORIG) = R(AUX);
            R(AUX) = R(CT16) - R(AUX);
          #ifdef ROUND_TO_NEAREST
            R(DISCARDED_BITS) = R(DST_MANTISSA_L);

/* small-MEGA-TODO (tried the code, but doesn't work on real Connex):
In order to be able to compute G (and maybe T) we should really keep in DISCARDED_BITS some of the (previous) bits
      if R(AUX) < ROUND_NUM_ADDITIONAL_BITS
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
    It seems this code is rather difficult to implement PERFECTLY correct.

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
            // We store only the discarded bits to compute well T
            R(DISCARDED_BITS) <<= R(AUX);
            R(DISCARDED_BITS) >>= R(AUX);

            R(NUM_DISCARDED_BITS) = R(AUX_ORIG);
          #endif
            R(DST_MANTISSA_L) >>= R(AUX_ORIG);
          //PrintRegDebug(DST_MANTISSA_L);
            //
//            R(AUX) = R(CT16) - R(AUX);
           PrintRegDebug(AUX_ORIG);
           PrintRegDebug(AUX);
            R(AUX2) <<= R(AUX);
          PrintRegDebug(AUX2);
          PrintRegDebug(DST_MANTISSA_H);
            //
            R(DST_MANTISSA_L) |= R(AUX2);

          PrintDebugMessage("Normalized result-mantissa:\n");
          PrintRegDebug(DST_MANTISSA_L);
          )
          EXECUTE_IN_ALL(


            /* VERY IMPORTANT: This is a bit difficult to understand, but I made
                efforts to explain it well:
            When we multiply the mantissas the contract is that
              the result should have 1 "integer" bit
                     (and let's say 2 * F16_MANTISSA_BITS fractional bits,
                       although some of them get lost due to SHR, etc)
                and the exponent should be correlated to that mantissa.
             (After, this, the mantissa contains, as expected, the hidden bit.)

              Above, in DST_MANTISSA_L, we have INVARIABLY
                F16_MANTISSA_BITS + 1 significant bits (or less for denormals).
                They wait to be laid in the final result WITHOUT ANY FURTHER
                   change.
              The above contract is violated if:
                - we have denormal operand(s) - because the exponent is not
                   correlated to the mantissa (which we suppose is already
                   normalized, i.e. with 1 "integer" bit and rest fraction).
                - if the final result of multiplication has a total
                  2 * F16_MANTISSA_BITS + 2 bits (with 2 integer bits included)
                  because the mantissa is not normalized again
                    since it has 2 integer bits.
                  So if NUM_BITS = 22, the bits were laid out correctly
                   above, in DST_MANTISSA_L, and then we decrement the
                   exponent DST_EXPONENT (with -= (-1) ).
               Note that, as expected, for the case NUM_BITS = 21 nothing
                  changes.

            Rembember that the result of multiplication can have:
               - as low as 0 bits
               - 21 bits:
                  (2^10) * (2^10)         = 0001 0000 0000 0000 0000 0000
               - as high as 22 bits:
                  (2^11 - 1) * (2^11 - 1) = 0011 1111 1111 0000 0000 0001

             Therefore, we need to correct the exponent some more (for denormals
                and "bigger", 2 * F16_MANTISSA_BITS + 2 bits, multiplication
                results)
                s.t. we have a new result mantissa stored in DST_MANTISSA_L
                  with 1 (hidden) "integer" bit and F16_MANTISSA_BITS bits
                  of fraction after.

            For example:
                0x0002 = 2^(-14) * 0.0000000010 (E1 = 0 --> denormal we make
                                                    it 1, F1 = 0x2)
                0x7B53 = 2^(30 - 15) * 1.1101010011 (E1 = 30, F1 = 0x353)
               their product is (after rebias performed above):
                2^(15 - 14) * 0.0000000010 * 1.1101010011 =
                  2^1 * 0.0000000010 * 1.1101010011 =
                  2^1 * [2^(-9) * 0.0000000010] * 1.1101010011 =
                  // for this -9 exponent we need to rebias to correct the exp
                  2^(1 - 9) * 1.0000000000 * 1.1101010011 =
                   (so we need to correct by adding to the exponent -9, since 9
                      is the number of leading zeros for F1).
          More examples:
              If NUM_BITS = 22 then:
                - DST_MANTISSA_L contains now 11 significant bits
                    (5 most significant from _L and 6 least significant from _H)
                  - (AUX = NUM_BITS - (F16_MANTISSA_BITS + 1) = 11)
                  - we correct finally the exponent by subtracting
                    2 * F16_MANTISSA_BITS + 1 - NUM_BITS = 21 - 22 = -1.

              If NUM_BITS = 21 then:
                - DST_MANTISSA_L contains now 11 significant bits
                    (6 most significant from _L and 5 least significant from _H)
                  - (AUX = NUM_BITS - (F16_MANTISSA_BITS + 1) = 10)
                  - we correct finally the exponent by subtracting
                    2 * F16_MANTISSA_BITS + 1 - NUM_BITS = 21 - 21 = 0.

              If NUM_BITS = 18 then:
                - DST_MANTISSA_L contains now 11 significant bits
                    (9 most significant from _L and 7 least significant from _H)
                  - (AUX = NUM_BITS - (F16_MANTISSA_BITS + 1) = 7)
                  - we correct finally the exponent by subtracting
                    2 * F16_MANTISSA_BITS + 1 - NUM_BITS = 21 - 18 = 3.

              If NUM_BITS = 12 then:
                - DST_MANTISSA_L contains now 11 significant bits
                    (15 most significant from _L and 1 least significant from _H)
                  - (AUX = NUM_BITS - (F16_MANTISSA_BITS + 1) = 1)
                  - we correct finally the exponent by subtracting 21 - 12 = 9.

              If NUM_BITS = 11 then:
                - DST_MANTISSA_L contains now 11 significant bits
                    (all 16 from _L)
                  - (AUX = NUM_BITS - (F16_MANTISSA_BITS + 1) = 0)
                  - we correct finally the exponent by subtracting 21 - 11 = 10.

              If NUM_BITS = 10 then:
                - DST_MANTISSA_L contains now 10 significant bits
                    (all from _L)
                  - (AUX = NUM_BITS - (F16_MANTISSA_BITS + 1) = -1,
                              corrected normally to 0 - nothing happens)
                  - we correct finally the exponent by subtracting 21 - 10 = 11.

              If NUM_BITS = 3 then: (Note: this is e.g. test index 15 I guess)
                - DST_MANTISSA_L contains now 3 significant bits: 0x4
                  - (AUX = NUM_BITS - (F16_MANTISSA_BITS + 1) = -8,
                              corrected normally to 0 - nothing happens)
                  - we correct the exponent 2 - 15 = -13 to 0
                    - with this occasion we SHR the result-mantissa by 13 positions
                  - we correct finally the exponent by subtracting 21 - 3 = 18:
                    DST_EXPONENT = DST_EXPONENT - 18 =
                                     = (2 - 15) - 18 = -31
                    2^(-14 + -14) * 0.0000000010 * 0.0000000010 =
                    2^(1 + 1 - 15 - 15) * 0.0000000010 * 0.0000000010 =
                    2^(1 + 1 - 15 - 15) * 0.00000000000000000010 =
                    2^(1 + 1 - 15 - 15 - 18) * 1.0.
                    So DST_EXPONENT = -31 (we can see this is a problem
                      - exponent -31 is negative, biased so it will be corrected
                      below) and
                       MANTISSA_L = 0x0400.

              If NUM_BITS = 2 then: (Note: this is ...)
                - DST_MANTISSA_L contains now 2 significant bits
                  - (AUX = NUM_BITS - (F16_MANTISSA_BITS + 1) = -9,
                              corrected normally to 0 - nothing changed)
                  - we correct finally the exponent by subtracting 21 - 2 = 19:
                    DST_EXPONENT = -19


           VERY VERY VERY IMPORTANT Note: 2 * F16_MANTISSA_BITS + 1 is the "standard" number of bits of an
              already normalized result - the reason is that when we multiply 2
              fractional numbers of F16_MANTISSA_BITS + 1 bits each with 1
              "integer" bit each, the result is on most 2 * F16_MANTISSA_BITS + 2
              bits and more importantly it has a fractional part of
                2 * F16_MANTISSA_BITS bits.

           Note that the adjustment of DST_EXPONENT must be thought SEPARATELY
             from the SHR above of DST_MANTISSA, since the SHR ONLY prepares the
             DST_MANTISSA to be copied in res.
            */
            R(AUX) = 2 * F16_MANTISSA_BITS + 1;
            //R(AUX) -= R(NUM_BITS);
           PrintDebugMessage("Correcting DST_EXPONENT for denormals and mantissa results with 2 'integer' bits:");
           PrintRegDebug(NUM_BITS);
            R(AUX) -= R(NUM_BITS);
//            R(AUX) = F16_MANTISSA_BITS;
           PrintRegDebug(AUX);
            //
           PrintRegDebug(DST_EXPONENT);
            R(DST_EXPONENT) -= R(AUX); //R(CT1);
           PrintRegDebug(DST_EXPONENT);
#else
            /* Although this does NOT treat denormals, this is a very nice
                way for Re-normalization.
              Re-normalization, first stage:
                discard F16_MANTISSA_BITS bits and reassemble into a single register;
                we're left with a number in 2.10 format
                (2 bits for the integer part)
        Kindda-Useless-TODO: think better if what he (Lucian) does is really GOOD - the re-normalization should depend also on the value of the exponent.
               Note: The exponent can hold only values 0..31 .
               Both mantissas hold invariably 10+1 bits
                 (except maybe in case of denormals!!!!).

              VERY IMPORTANT:
               If we don't have denormals, we multiply the mantissas, which
                have both a total of 11 bits, the
                result having at most 2 * (F16_MANTISSA_BITS + 1) = 22 useful
                   bits.
                 More exactly, the result of multiplication can have:
                  - as low as 21 bits:
                      (2^10) * (2^10)         = 0001 0000 0000 0000 0000 0000
                  - as high as 22 bits:
                      (2^11 - 1) * (2^11 - 1) = 0011 1111 1111 0000 0000 0001

                  More exactly:
                      - we SHR by F16_MANTISSA_BITS (both low _L and _H
                                obtained from MUL) and put the result
                                DST_MANTISSA_L.
                      - we SHR by 1 more position if the 11th bit is 1 .
                      (and also do some rounding before[!!!!Kindda-Useless-TODO]).

            Kindda-Useless-TODO: think better these conditions
                  But if the exponent was e.g. 15 this is not feasible...
                  If the exponent was < -5 then this is OK.
            */
            R(DST_MANTISSA_L) >>= F16_MANTISSA_BITS;
            //
            R(DST_MANTISSA_H) <<= 16 - F16_MANTISSA_BITS; // ((2 * (F16_MANTISSA_BITS + 1)) - 16);
            //
            R(DST_MANTISSA_L) |= R(DST_MANTISSA_H);

            /* Re-normalization, second part:
               - we SHR DST_MANTISSA_L, by 0 or 1 positions, depending on
                 the value 0, respectively 1 of the original 22nd bit
                 (original bit index 21) in order to finish normalization, i.e.
                 have a mantissa with bit index 10 (11th bit) set to 1.

                 TODO: but what if the result is UNfortunately much smaller
                        and requires SHL instead of SHR?
            */
            R(AUX) = R(DST_MANTISSA_L) >> (F16_MANTISSA_BITS + 1);
          PrintRegDebug(AUX);
            R(DST_MANTISSA_L) >>= R(AUX);
          PrintRegDebug(DST_MANTISSA_L);
            //
            R(DST_EXPONENT) += R(AUX);
#endif // TREAT_DENORMALS




        /* I prefer doing here correction of negative exponent because:
              - I don't increase the exponent below
              - it's OK to SHR mantissa even for 31 positions
                - we do a few Connex SHR operations

           Note that the smallest exponent is ~ -31 (= 1 + 1 - 15 - 18) or so.
        */
// I assume the max we move is 32
        /* IMPORTANT: If we need to SHR more than 15 positions the result
            mantissa (which is stored in DST_MANTISSA_L and DST_MANTISSA_H,
            first we do 16 and then the rest - part of the reason is that Connex
            does not work for SHR 16+ positions (which should return 0 for lanes
            of i16, but actually returns the input value - see
              /home/alarm/Experiments/Test_SHR_special_2nd_opnd for correct
            results on Connex for SHR 0-15 positions, etc).
        */
          PrintDebugMessage("Correcting negative exponent (#1):");
            R(AUX2) = 15;
            R(AUX) = R(CT1) - R(DST_EXPONENT);
          PrintRegDebug(AUX);
            R(PRED3) = R(AUX2) < R(AUX);
          //PrintRegDebug(PRED3);
            NOP;
        );
        EXECUTE_WHERE_LT(
            //R(DST_MANTISSA_L) = 0;
          #ifdef ROUND_TO_NEAREST
          /*
          We should enable macro COMPUTE_BETTER_T_BIT
           in order to have maximum precision we should NOT discard current
             value of the bits of the mantissa which we store in R(DISCARDED_BITS).
           Since we store extra 16 bits at the end since we make
                R(NUM_DISCARDED_BITS) = 16;
           we can compute the OR of all bits in R(DISCARDED_BITS) before
           overwriting it. This is simply done by doing:
               res = R(DISCARDED_BITS) != 0
          */
           #ifdef COMPUTE_BETTER_T_BIT
            R(NUM_DISCARDED_BITS) = R(DISCARDED_BITS) == R(CT0);
            R(NUM_DISCARDED_BITS) = R(CT1) - R(NUM_DISCARDED_BITS);
            R(DISCARDED_BITS) = R(DST_MANTISSA_L);
            R(DISCARDED_BITS) |= R(NUM_DISCARDED_BITS);
            R(NUM_DISCARDED_BITS) = 16;
           #else
            R(DISCARDED_BITS) = R(DST_MANTISSA_L);
            R(NUM_DISCARDED_BITS) = 16;
           #endif
          #endif
            R(DST_MANTISSA_L) = R(DST_MANTISSA_H);
            R(DST_MANTISSA_H) = 0;
            R(AUX) -= R(CT16);
            R(DST_EXPONENT) += R(CT16);
        );
        EXECUTE_IN_ALL(
          PrintRegDebug(DST_MANTISSA_L);
          PrintRegDebug(DST_MANTISSA_H);
          PrintRegDebug(DST_EXPONENT);
          PrintRegDebug(AUX);

        /* IMPORTANT: Again, if we need to SHR more than 15 positions,
             first we do 16 and then the rest.
        */
            R(PRED3) = R(AUX2) < R(AUX);
          //PrintRegDebug(PRED3);
            NOP;
        );
        EXECUTE_WHERE_LT(

          #ifdef ROUND_TO_NEAREST
          /*
          We should enable macro COMPUTE_BETTER_T_BIT
           in order to have maximum precision we should NOT discard current
             value of the bits of the mantissa which we store in R(DISCARDED_BITS).
           Since we store extra 16 bits at the end since we make
                R(NUM_DISCARDED_BITS) = 16;
           we can compute the OR of all bits in R(DISCARDED_BITS) before
           overwriting it. This is simply done by doing:
               res = R(DISCARDED_BITS) != 0
          */
           #ifdef COMPUTE_BETTER_T_BIT
            R(NUM_DISCARDED_BITS) = R(DISCARDED_BITS) == R(CT0);
            R(NUM_DISCARDED_BITS) = R(CT1) - R(NUM_DISCARDED_BITS);
            R(DISCARDED_BITS) = R(DST_MANTISSA_L);
            R(DISCARDED_BITS) |= R(NUM_DISCARDED_BITS);
            R(NUM_DISCARDED_BITS) = 16;
           #else
            R(DISCARDED_BITS) = R(DST_MANTISSA_L);
            R(NUM_DISCARDED_BITS) = 16;
           #endif
          #endif

            R(DST_MANTISSA_L) = 0;
            R(AUX) -= R(CT16);
            R(DST_EXPONENT) += R(CT16);
        );
        EXECUTE_IN_ALL(
          PrintRegDebug(DST_MANTISSA_L);
          PrintRegDebug(DST_MANTISSA_H);
          PrintRegDebug(DST_EXPONENT);
          PrintRegDebug(AUX);

          /*
           IMPORTANT: we complete the "correction" of negative exponent by
             bringing to 1.
          */
          PrintDebugMessage("Correcting negative exponent (#2: we make it 1 if smaller):");
            //R(PRED3) = R(DST_EXPONENT) < R(CT1);
            /*
            R(AUX2) = 2;
            R(PRED3) = R(DST_EXPONENT) < R(AUX2);
            */
#ifdef NOT_REQUIRED_ACTUALLY_BAD
           // We check DST_EXPONENT - 1 < 1 (or DST_EXPONENT <= 1)
            R(AUX2) = R(DST_EXPONENT) - R(CT1);
            R(PRED3) = R(AUX2) < R(CT1);
            // This is NOT really good since we can do SHR 16 and then OR...
            assert(0);
#endif
           // We check DST_EXPONENT < 1
            R(PRED3) = R(DST_EXPONENT) < R(CT1);
          PrintRegDebug(PRED3);
            NOP;
        );
        EXECUTE_WHERE_LT(
            //R(AUX) = R(CT0) - R(DST_EXPONENT);
            R(AUX) = R(CT1) - R(DST_EXPONENT);
          PrintRegDebug(AUX);
          PrintRegDebug(DST_MANTISSA_L);

          #ifdef ROUND_TO_NEAREST
          /*
          We should enable macro COMPUTE_BETTER_T_BIT
           in order to have maximum precision we should NOT discard current
             value of the bits of the mantissa which we store in R(DISCARDED_BITS).
           Since we store extra 16 bits at the end since we make
                R(NUM_DISCARDED_BITS) = 16;
           we can compute the OR of all bits in R(DISCARDED_BITS) before
           overwriting it. This is simply done by doing:
               res = R(DISCARDED_BITS) != 0
          */
           #ifdef COMPUTE_BETTER_T_BIT
            R(NUM_DISCARDED_BITS) = R(DISCARDED_BITS) == R(CT0);
            R(NUM_DISCARDED_BITS) = R(CT1) - R(NUM_DISCARDED_BITS);
            R(DISCARDED_BITS) = R(DST_MANTISSA_L);

/* small-MEGA-TODO (tried the code, but doesn't work on real Connex):
In order to be able to compute G (and maybe T) we should really keep in DISCARDED_BITS some of the (previous) bits
      if R(AUX) < ROUND_NUM_ADDITIONAL_BITS
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



            R(DISCARDED_BITS) |= R(NUM_DISCARDED_BITS);

          #define AUX3 PRED3
            // We store only the discarded bits to compute well T
            R(AUX3) = R(CT16) - R(AUX);
            R(DISCARDED_BITS) <<= R(AUX3);
            R(DISCARDED_BITS) >>= R(AUX3);

            R(NUM_DISCARDED_BITS) = R(AUX);
           #else
            R(DISCARDED_BITS) = R(DST_MANTISSA_L);
            R(NUM_DISCARDED_BITS) = R(AUX);
           #endif
          #endif

            R(DST_MANTISSA_L) >>= R(AUX);
          PrintRegDebug(DST_MANTISSA_L);
            //
            R(AUX2) = R(DST_MANTISSA_H);
            R(DST_MANTISSA_H) >>= R(AUX);
            R(AUX) = R(CT16) - R(AUX);
            R(AUX2) = R(AUX2) << R(AUX);
            //R(DST_MANTISSA_H) <<= R(AUX);
            R(DST_MANTISSA_L) |= R(AUX2);

            //R(DST_EXPONENT) = 0; // Note: DST is NOT a denormal (which would have exp = 1)
            /* We can't discriminate now between denormal and non-denormal with
             *   DST_EXPONENT = 1. We will do this below. */
            R(DST_EXPONENT) = 1;

          PrintRegDebug(DST_MANTISSA_L);
          PrintRegDebug(DST_MANTISSA_H);
          PrintRegDebug(DST_EXPONENT);
          );
        EXECUTE_IN_ALL(






            /* Getting rid of mask bit is NOT good here: case 0x00a8 * 0x2e66 fails.
                 - we should preserve for now the hidden bit since we still
                   check and SHR mantissa below:
            // Get rid of hidden bit, which is always 1
            R(DST_MANTISSA_L) &= R(MANTISSA_MASK);
            */

         //PrintDebugMessage("DST_MANTISSA_L after renormalization:");
         PrintDebugMessage("After renormalization (and correction):");
          PrintRegDebug(DST_MANTISSA_L);
          PrintRegDebug(DST_EXPONENT);


            // IMPORTANT: Check if the exponent overflows and, if so, declare infinity.
            R(PRED3) = 30;
            R(PRED3) = R(PRED3) < R(DST_EXPONENT);
          PrintRegDebug(PRED3);
            NOP;
        )
        EXECUTE_WHERE_LT(
            R(DST_EXPONENT) = 0x1F;
          PrintRegDebug(DST_EXPONENT);
            R(DST_MANTISSA_L) = 0;
        );
        EXECUTE_IN_ALL(




            R(AUX) = F16_MANTISSA_MASK + 1;
            R(AUX) = R(DST_MANTISSA_L) < R(AUX);
            R(PRED3) = R(DST_EXPONENT) == R(CT1);
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
            R(DST_EXPONENT) = 0;
        );
        EXECUTE_IN_ALL(
          PrintRegDebug(DST_MANTISSA_L);
          PrintRegDebug(DST_EXPONENT);

#ifdef NO_LONGER_REQUIRED
      /* IMPORTANT: Check if the exponent underflows and if so declare it 0
          and the number normally denormal - this is useful normally when we have
          input operands denormal(s). */
// TODO TODO: check for case DST_EXPONENT == 0, DST_MANTISSA != 0 and create proper denormal with DST_EXPONENT == 1; find test for this
            R(PRED3) = R(DST_EXPONENT) < R(CT0);
          //PrintRegDebug(PRED3);
            NOP;
        )
        EXECUTE_WHERE_LT(
            //R(AUX) = R(CT0) - R(DST_EXPONENT);
            // We create a denormal with exponent 1 actually (stored 0):
            R(AUX) = R(CT1) - R(DST_EXPONENT);
         PrintDebugMessage("Correcting negative exponent:");
          PrintRegDebug(AUX);
            R(DST_MANTISSA_L) >>= R(AUX);

            R(DST_EXPONENT) = 0; // normally we should put 1, but the standard IEEE 754 puts 0
          //PrintRegDebug(DST_EXPONENT);
            //R(DST_MANTISSA_L) = 0;
        );
        EXECUTE_IN_ALL(
          PrintRegDebug(DST_MANTISSA_L);
          PrintRegDebug(DST_EXPONENT);
#endif






        #ifdef ROUND_TO_NEAREST
          PrintRegDebug(DISCARDED_BITS);
          PrintRegDebug(NUM_DISCARDED_BITS);
            R(L) = R(DST_MANTISSA_L) & R(CT1);
          PrintDebugMessage("Rounding to nearest (if tie to even):");
          PrintRegDebug(NUM_DISCARDED_BITS);
          PrintDebugMessage("  L = ");
          PrintRegDebug(L);

#ifdef COMPUTE_BETTER_T_BIT
          /* IMPORTANT NOTE: if R(NUM_DISCARDED_BITS) < 2, then the code below works
             because:
              - if R(NUM_DISCARDED_BITS) = 0, because R(DISCARDED_BITS) is 0, since we
                initialize it to 0 at the beginning:
                  - G is 0 (Note: although not relevant, on Connex
                                  1 SHL -1 is 0)
                  - T is 0
              - if R(NUM_DISCARDED_BITS) = 1:
                  - G is the right bit value
                  - T is 0 because R(DISCARDED_BITS) becomes 0 after XOR.
          */
          // We need to compute the sticky bit, T, as OR over all the values.
          // We first compute G.
          R(NUM_DISCARDED_BITS) -= R(CT1);
          R(NUM_DISCARDED_BITS) = R(CT1) << R(NUM_DISCARDED_BITS);
          R(G) = R(DISCARDED_BITS) & R(NUM_DISCARDED_BITS);
          //
          // We take out bit G from R(DISCARDED_BITS):
          R(DISCARDED_BITS) ^= R(G);
          //
          R(G) = R(G) == R(CT0);
          R(G) = R(CT1) - R(G);
          //
          R(T) = R(DISCARDED_BITS) == R(CT0);
          R(T) = R(CT1) - R(T);

          //R(T) = R(DISCARDED_BITS) & R(NUM_DISCARDED_BITS);
#else
          /* IMPORTANT NOTE: if R(NUM_DISCARDED_BITS) < 2, then the code below works
             because:
              - if R(NUM_DISCARDED_BITS) = 0, because R(DISCARDED_BITS) is 0, since we
                initialize this register to 0:
                  - G is 0
                  - T is 0
              - if R(NUM_DISCARDED_BITS) = 1:
                  - T is 0 because:
                     - 0 SHR -1 gives 0
                     - 1 SHR -1 gives 0 - see e.g.
                /home/alarm/Experiments/Test_SHR_special_2nd_opnd/STD_run_024_SHR-1_from_val1.
                  - BUG: useless-TODO: G's value is set incorrectly always to 0.
            Note: on Connex 0 SHR -2/-1 gives 0
            - see e.g.
              /home/alarm/Experiments/Test_SHR_special_2nd_opnd/STD_run_021_SHR-2_from_val0.

            useless-TODO: There's only 1 problem here: we might rely on undefined
              behavior of Connex for SHR/SHL.
          */
          // We compute INcorrectly the sticky bit, T, as the bit after G
          R(NUM_DISCARDED_BITS) -= R(CT1);
          R(NUM_DISCARDED_BITS) -= R(CT1);
          PrintRegDebug(NUM_DISCARDED_BITS);
          R(DISCARDED_BITS) >>= R(NUM_DISCARDED_BITS);
          PrintRegDebug(DISCARDED_BITS);
            R(T) = R(DISCARDED_BITS) & R(CT1);

          R(DISCARDED_BITS) >>= 1;
          PrintRegDebug(DISCARDED_BITS);
            R(G) = R(DISCARDED_BITS) & R(CT1);
#endif


          PrintDebugMessage("  L = ");
          PrintRegDebug(L);
          PrintDebugMessage("  G = ");
          PrintRegDebug(G);
          PrintDebugMessage("  T = ");
          PrintRegDebug(T);

            R(RND) = R(T) | R(L);
            R(RND) &= R(G); // & R(AUX);
          PrintDebugMessage("  G (T + L) = ");
          PrintRegDebug(RND);


            //R(DISCARDED_BITS) = R(DST_MANTISSA_L);
            //R(NUM_DISCARDED_BITS) = 16;
          #endif






            // We set the exponent 31 if INPUT_EXP31 == true
          PrintRegDebug(INPUT_EXP31);
            R(INPUT_EXP31) = R(INPUT_EXP31) == R(CT1);
            NOP;
        );
        EXECUTE_WHERE_EQ(
            R(DST_EXPONENT) = R(CT31);
            //R(RND) = 0;
          //PrintRegDebug(RND);
        );
        EXECUTE_IN_ALL(


// small-TODO: it's nicer to put result 0 if we have 0 * negative value


            // Put the f16 number back together
            // Shift exponent in the final place
            R(DST) = R(DST_EXPONENT) << F16_MANTISSA_BITS;
            /* VERY IMPORTANT: We don't add sign bit now because we want to
                    optimize rounding:
            R(DST) |= R(DST_SIGN); */
            // Get rid of hidden bit of mantissa, which is always 1
            R(DST_MANTISSA_L) &= R(MANTISSA_MASK);
            // Add mantissa:
            R(DST) |= R(DST_MANTISSA_L);
            PrintRegDebug(DST);



            /* We now treat special cases with NAN: NAN * 0, NAN * INF .

               IMPORTANT NOTE: For MUL.f16 we don't use the CONTINUE to treat
                   nested loops with various cases as we do for ADD.f16.
                 Therefore, at the end we need to perform more complex handling
                  of cases, including NANs, INFs, etc - for this we use logic
                  minimization (Quine-McKluskey).

              From Ercegovac book, Section 8.5.2 Exceptions and Special Values:
             <<NAN: The result is a NAN if one (or both) of the operands is a
               NAN or if one of the operands is a 0 and the other +infinity.>>

             Note: we use logic minimization (Quine-McKluskey, but we could
               also use Espresso or Multi-level logic minimization) to
               minimize the number of Connex instructions for this
               sub-procedure.
               Maybe I shoot a fly with a cannon, but I don't think I
                   can generate smaller code than this.
               VERY IMPORTANT: It seems logic minimization is VERY suitable for
                 generating smallest code possible because Connex has
                 predication, and basically no conditional branches.

                     RES_NAN = E1 F1 + E2 F2 + E1' F1' E2 + E1 E2' F2'
              This is standard two-level logic minimization - Karnaugh maps
                work only for minimizing boolean functions of at most 4 inputs,
                while the Espresso tool works in the general case.


              It is VERY interesting to note that I guess I was able to
                further minimize the number of instructions (reduce it by 1
                instruction - I'm sure we can have other cases where the
                reduction can be even bigger) generated by computing first
                  INPUT_EXP31 = E1 + E2, then adding don't cares in the
                  Karnaugh map (for when INPUT_EXP31 == false) and then
                  obtaining a smaller minimal formula:
                     RES_NAN = E1 F1 + E2 F2 + E1' F1' + E2' F2'

              This doesn't really seem to be multi-level logic minimization
                - read GDM's book chapter on this to see if
                  multiplexors/predicates/conditionals are allowed.
                  - find in which Section of GDM book is this don't cares issue
                    (remember that I discovered it because I had computed
                     INPUT_EXP31 before thinking on doing logic minimization)
            */
            // small TODO: SRC1_EXPONENT == 31 was already computed above
            R(SRC1_EXPONENT) = R(SRC1_EXPONENT) == R(CT31);
            R(SRC1_MANTISSA) = R(CT0) < R(SRC1_MANTISSA);
            //
            // small TODO: SRC2_EXPONENT == 31 was already computed above
            R(SRC2_EXPONENT) = R(SRC2_EXPONENT) == R(CT31);
            R(SRC2_MANTISSA) = R(CT0) < R(SRC2_MANTISSA);
            //
            R(V1) = R(SRC1_EXPONENT) & R(SRC1_MANTISSA);
            R(V2) = R(SRC2_EXPONENT) & R(SRC2_MANTISSA);
            R(V3) = R(CT1) - R(SRC2_EXPONENT);
            R(V4) = R(CT1) - R(SRC2_MANTISSA);
            R(V4) &= R(V3);
          #ifdef STD_KARNAUGHMAP_WITHOUT_DONTCARES
            //R(V4) &= R(SRC1_EXPONENT);
          #endif
            R(V3) = R(CT1) - R(SRC1_EXPONENT);
            R(V5) = R(CT1) - R(SRC1_MANTISSA);
            R(V5) &= R(V3);
          #ifdef STD_KARNAUGHMAP_WITHOUT_DONTCARES
            //R(5) &= R(SRC2_EXPONENT);
          #endif
            R(IS_NAN_MIN) = R(V1) | R(V2);
            R(IS_NAN_MIN) |= R(V4);
            R(IS_NAN_MIN) |= R(V5);
          PrintRegDebug(IS_NAN_MIN);

          #ifdef STD_KARNAUGHMAP_WITHOUT_DONTCARES
          #else
            R(AUX2) = R(INPUT_EXP31) & R(IS_NAN_MIN);
          #endif
            R(AUX2) = R(AUX2) == R(CT1);
            NOP;
        );
        EXECUTE_WHERE_EQ(
            R(DST) = F16_NAN;
      #ifdef ROUND_TO_NEAREST
            R(RND) = 0;
          PrintRegDebug(RND);
      #endif
        );
        EXECUTE_IN_ALL(


          #ifdef ROUND_TO_NEAREST
            // We do this check because we can make an INF become NAN
            R(AUX) = R(DST_EXPONENT) == R(CT31);
            NOP;
        );
        EXECUTE_WHERE_EQ(
            R(RND) = 0;
          PrintRegDebug(RND);
        );
        EXECUTE_IN_ALL(


        /* If we overflow the mantissa after rounding (we add 1 to maximum
                          valid mantissa) then:
              - if EXP <= 30 mantissa becomes 1.0000000000 (base 2).
                  If EXP was 30 this implies setting to +/- INF.
                  So we need to increment the exponent.
                  But this is automatically done when we add 1 to R(DST), where
                  R(DST) contains the packed result.
              - if EXP == 31 we ca see we set in the instructions immediately
                  above R(RND) = 0, so we can't overflow mantissa
                    - we don't need to care about this case.
          VERY IMPORTANT: But we need to make sure we don't put in R(DST) the
            sign R(DST_SIGN), because then if we add R(RND) it can actually
            subtract it from the mantissa if the f16 is negative.

        Therefore the code below is not required:
            R(AUX) = R(DST_MANTISSA_L) == R(MANTISSA_MASK);
            NOP;
          );
          EXECUTE_WHERE_EQ(
            R(RND) = 0;
            R(DST_EXPONENT) += R(CT1);
            R(DST_MANTISSA_L) = 0;
            Now we pack again the mantissa:...
            PrintRegDebug(RND);
            PrintRegDebug(DST_EXPONENT);
          );
          EXECUTE_IN_ALL(

         NOTE: We can't add R(RND) before (to avoid this extra check) because:
            - this rounding needs to be handled at the very end so we can't
               round in the middle of the computation since we don't know the
               result (before rounding).
        */

            R(DST) += R(RND);
          #endif // END #ifdef ROUND_TO_NEAREST

            /* Only now we can put the sign bit in R(DST) (since we can do
                rounding before and we want to optimize it).
            */
            R(DST) |= R(DST_SIGN);

            NOP;
            // Store result
            LS[resPtr] = R(DST);

            // End of program synchronization point; host will wait for this
            REDUCE(R0);
        )
    END_KERNEL("mul.f16");
}



int FloatMulTest(ConnexMachine *connex) {
    uint16_t opA[CONNEX_VECTOR_LENGTH];
    uint16_t opB[CONNEX_VECTOR_LENGTH];
    uint16_t resCorrect[CONNEX_VECTOR_LENGTH];
    uint16_t result[CONNEX_VECTOR_LENGTH];



    //opA[0] = 0x00A8; // F16 encoding for 0.00001 = 10^(-5)
    //opB[0] = 0x00A8; // F16 encoding for 0.00001 = 10^(-5)
    /*
    opA[0] = 0x823a; // F16 encoding for 0.00001 = 10^(-5)
    opB[0] = 0x0ae5; // F16 encoding for 0.00001 = 10^(-5)
    resCorrect[0] = 0x8000;
    */
    /*
    opA[0] = 0x3e5a; // F16 encoding for ...
    opB[0] = 0x82c5; // F16 encoding for ...
    resCorrect[0] = 0x8466; //0x8065;
    */
/*
    opA[0] = 0x761a; // F16 encoding for ...
    opB[0] = 0x51e9; // F16 encoding for ...
    resCorrect[0] = 0x7c00; //0x8065;
*/

    opA[0] = 0x00A8; // F16 encoding for 0.00001 = 10^(-5)
    opB[0] = 0x00A8; // F16 encoding for 0.00001 = 10^(-5)
    resCorrect[0] = 0x0000;

    opA[0] = 0x3f64;
    opB[0] = 0x5b76;
    resCorrect[0] = 0x5ee5;
    /*
    opA[0] = 0x3D47; // F16 encoding for 1.3193359375
    opB[0] = 0x4470; // F16 encoding for 4.4375
    resCorrect[0] = 0x45DB; // result of mul as obtained in Clang
    */

    opA[1] = 0xBD47; // F16 encoding for -1.3193359375
    opB[1] = 0x4470; // F16 encoding for 4.4375
    resCorrect[1] = 0xC5DB; // result of mul as obtained in Clang

    opA[2] = 0x3D47; // F16 encoding for 1.3193359375
    opB[2] = 0xC470; // F16 encoding for -4.4375
    resCorrect[2] = 0xC5DB; // result of mul as obtained in Clang

    opA[3] = 0xBD47; // F16 encoding for -1.3193359375
    opB[3] = 0xC470; // F16 encoding for -4.4375
    resCorrect[3] = 0x45DB; // result of mul as obtained in Clang

  #ifndef DONT_TREAT_DENORMAL_HERE
    /*
    opA[4] = 0x0000; // F16 encoding for 0.0000001 = 10^(-7)
    //opB[4] = 0x70E2; // F16 encoding for 10000
    //resCorrect[4] = 0x14e2;
    opB[4] = 0x0000; // F16 encoding for 60000
    resCorrect[4] = 0x0000;
    */

    /*
    opA[4] = 0x00a8; // F16 encoding for 0.00001 = 10^(-5)
    opB[4] = 0x2e66; // F16 encoding for 0.1
    resCorrect[4] = 0x0011;
    */

    // See /home/asusu/LLVM/llvm38Nov2016/llvm/build40/bin/Tests/NEW_v128i16/opincaa_standalone_apps/FP16/C/LLVM/_Float16/Gen_constant_values/Float16.c, etc
    // We treat now denormal operand(s):
    opA[4] = 0x0002; // F16 encoding for 0.0000001 = 10^(-7)
    //opB[4] = 0x70E2; // F16 encoding for 10000
    //resCorrect[4] = 0x14e2;
    opB[4] = 0x7B53; // F16 encoding for 60000
    resCorrect[4] = 0x1F53;
  #else
    opA[4] = F16_NAN; // F16 encoding for a NaN
    opB[4] = F16_NAN_2; // F16 encoding for NaN
    resCorrect[4] = F16_NAN_2;
  #endif

    opA[5] = 0x3D47; // F16 encoding for ...
    opB[5] = F16_NAN; // F16 encoding for NaN
    resCorrect[5] = F16_NAN_2;

    // Important case: NAN * INF = NAN
    opA[6] = F16_NAN;
    //opB[6] = 0xBD47;
    opB[6] = F16_INF_POSITIVE; // F16 encoding for +Inf
    resCorrect[6] = F16_NAN;

    opA[7] = 0x3D47; // F16 encoding for ...
    opB[7] = F16_INF_POSITIVE; // F16 encoding for +Inf
    resCorrect[7] = F16_INF_POSITIVE;

    opA[8] = 0x3D47; // F16 encoding for ...
    opB[8] = F16_INF_NEGATIVE; // F16 encoding for -Inf
    resCorrect[8] = F16_INF_NEGATIVE;

    opA[9] = F16_INF_NEGATIVE;
    opB[9] = 0x3D47;
    resCorrect[9] = F16_INF_NEGATIVE;

    opA[10] = F16_INF_POSITIVE;
    opB[10] = 0x3D47;
    resCorrect[10] = F16_INF_POSITIVE;

    opA[11] = F16_INF_NEGATIVE; // F16 encoding for -Inf
    opB[11] = F16_INF_NEGATIVE; // F16 encoding for -Inf
    resCorrect[11] = F16_INF_POSITIVE;

    opA[12] = F16_INF_POSITIVE; // F16 encoding for Inf
    opB[12] = F16_INF_POSITIVE; // F16 encoding for Inf
    resCorrect[12] = F16_INF_POSITIVE;

    /*
    opA[13] = F16_INF_NEGATIVE; // F16 encoding for -Inf
    opB[13] = F16_INF_POSITIVE; // F16 encoding for Inf
    resCorrect[13] = F16_INF_NEGATIVE;
    */
    // Important case: 0 * INF = NAN (one should test also the reverse)
    opA[13] = 0x0000; // F16 encoding for 0
    opB[13] = F16_INF_POSITIVE; // F16 encoding for Inf
    resCorrect[13] = F16_NAN;

    // We treat now denormal operand(s):
    opA[14] = 0x0002; // F16 encoding for 0.0000001 = 10^(-7)
    opB[14] = 0x70E2; // F16 encoding for 60000
    resCorrect[14] = 0x14e2;
    // 0.000001  0x0011
    // 0x7C00 1000000
    // 0x7B53 60000

    opA[15] = 0x0002; // F16 encoding for 0.0000001 = 10^(-7)
    opB[15] = 0x0002; // F16 encoding for 0.0000001 = 10^(-7)
    resCorrect[15] = 0x0000;

    opA[16] = 0x00A8; // F16 encoding for 0.00001 = 10^(-5)
    opB[16] = 0x00A8; // F16 encoding for 0.00001 = 10^(-5)
    resCorrect[16] = 0x0000;

    /*
    Comment on how this multiplication works for the 2 numbers 0x00A8 and
        0x2E66:
     mantissa res = A8 * 666 = 432f0
        0100 0011 0010 1111 0000
     Eres = 1 + 11 - 15 = 12 - 15 = -3
      We must correct Eres - make it 0 - so we SHL mantissa by 3
         mantissa = 865e = 1000 0110 0101 1110

    After renormalization: // this seems to be correct also - it SHL by 5 = 16-11
        mantissa = 0432 (with hidden bit also: 0100 0011 0010)
        exp = fffb = -5

    We correct this negative exponent by creating a denormal:
        exp = -5 + 6 = 1
        mantissa = 0432 SHR 6 = 10H = 01 0000(2)
        Rounding should also yield the lsb of mantissa 1 (I guess), so instead
           of final result 0x0010 we should have 0x0011.
    */
    opA[17] = 0x00A8; // F16 encoding for 0.00001 = 10^(-5)
    opB[17] = 0x2E66; // F16 encoding for 0.1
    resCorrect[17] = 0x0011;

    opA[18] = 0; // F16 encoding for 0
    opB[18] = 0; // F16 encoding for 0
    resCorrect[18] = 0;

    opA[19] = 0; // F16 encoding for 0
    opB[19] = 0x0002; // F16 encoding for ...
    resCorrect[19] = 0;

    opA[20] = 0; // F16 encoding for 0
    opB[20] = 0x70E2; // F16 encoding for ...
    resCorrect[20] = 0;

    opA[21] = 0x4233; // F16 encoding for 3.1
    opB[21] = 0x4166; // F16 encoding for 2.7
    resCorrect[21] = 0x482F;

    opA[22] = 0xD829; // F16 encoding for -133.1
    opB[22] = 0x518A; // F16 encoding for 44.3
    resCorrect[22] = 0xEDC3;

// small TODO: check better NANs and INFs (I'm afraid that a NAN can turn in an INF, although I'm checking well for it)

    // We check that value 0xF200 and NAN 0xFE00 gives NAN
    opA[23] = 0xF200; // F16 encoding for ...
    opB[23] = 0xFE00; // F16 encoding for a NAN
    resCorrect[23] = F16_NAN;

    // Multiplying big denormal with biggest numerical value
    opA[24] = 0x03FF; // F16 encoding for 0.1111111111 * 2^(-14) ~= 0.00006097412 (10)
    opB[24] = 0x7B53; // F16 encoding for a 60000
    resCorrect[24] = 0x4351;

    opA[25] = 0x0000; // F16 encoding for 0
    opB[25] = 0xbee2; // F16 encoding for a 60000
    resCorrect[25] = 0x0000;

    opA[26] = 0x3e5a; // F16 encoding for ...
    opB[26] = 0x82c5; // F16 encoding for ...
    resCorrect[26] = 0x8466;

    //opA[27] = 0x3d47; // F16 encoding for 1.14862349684 = sqrt(1.3193359375)
    //opB[27] = 0x3d47; // F16 encoding for 1.14862349684 = sqrt(1.3193359375)
    //resCorrect[27] = 0x5947;
    opA[27] = 0x3C98; // F16 encoding for 1.14862349684 = sqrt(1.3193359375 = 0x3d47)
    opB[27] = 0x3C98; // F16 encoding for 1.14862349684 = sqrt(1.3193359375)
    resCorrect[27] = 0x3d47; // It is 0x3d47 due to rounding-to-nearest (without it is 0x3d46)
    /*
    opA[28] = 0x3C99; // F16 encoding for 1.14862349684 = sqrt(1.3193359375 = 0x3d47)
    opB[28] = 0x3C99; // F16 encoding for 1.14862349684 = sqrt(1.3193359375)
    resCorrect[28] = 0x3d47;
    */

    opA[28] = 0x6dc5; // 0x6dc5(S=0,E=0x1b,F=0x5c5)
    opB[28] = 0xd68f; // 0xd68f(S=1,E=0x15,F=0x68f)
    resCorrect[28] = F16_INF_NEGATIVE;

    opA[29] = 0xe083; // 0xe083(S=1,E=0x18,F=0x483)
    opB[29] = 0x73d0; // 0x73d0(S=0,E=0x1c,F=0x7d0)
    resCorrect[29] = F16_INF_NEGATIVE;

    opA[30] = 0xe462; // 0xe462(S=1,E=0x19,F=0x462)
    opB[30] = 0xe789; // 0xe789(S=1,E=0x19,F=0x789)
    resCorrect[30] = F16_INF_POSITIVE;

//#define TEST_COMPILED_CODE_WITH_TRACE
#ifdef TEST_COMPILED_CODE_WITH_TRACE
    //for (int i = 1; i < CONNEX_VECTOR_LENGTH; i++) {
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        opA[i] = opA[2]; //0x3D47;
        opB[i] = opB[2]; //0x4470;
        resCorrect[i] = resCorrect[0]; // 0x45DB;

        //C[i] = -1;
    }
#endif

  #define NUM_VALS 31



#define ZERO_AFTER_NUM_VALS_INPUT_TESTS
#ifdef ZERO_AFTER_NUM_VALS_INPUT_TESTS
    for (int i = NUM_VALS; i < CONNEX_VECTOR_LENGTH; i++) {
        opA[i] = 0;
        opB[i] = 0;
        resCorrect[i] = 0;
    }
#else
    GenRandF16(opA, opB, resCorrect, NUM_VALS);
#endif





    Mul_f16Kernel(0, 1, 2);

    connex->writeDataToConnex(opA, 1, 0);
    connex->writeDataToConnex(opB, 1, 1);

#ifdef LLVM_ISEL_CODEGEN
    string kernelName = "mul.f16";
    Kernel *kernel = connexGlobal->getKernel(kernelName);
    kernel->sdNodeVarNameRegDef[SRC1] = "nodeOpSrcCast1";
    kernel->sdNodeVarNameRegDef[SRC2] = "nodeOpSrcCast2";
    //
    // For MUL f16:
    kernel->offsetKernelToStartCodegenFrom = 2 + 1; // +1 for the END_WHERE instruction Opincaa adds automatically to each kernel
    kernel->numInstructionsToCodegen = kernel->size()
                                            - 3 /*num instruction we remove from end of kernel */
                                            - kernel->offsetKernelToStartCodegenFrom;
    //assert(kernel->numInstructionsToCodegen == 332);
    //
    //kernel->numInstructionsToCodegen = 332; // kernel.size() - 2 /*num instruction we remove from end of kernel */ - kernel->offsetKernelToStartCodegenFrom
    //
    // We use chain, since with glue we get a lot or weird scheduling errors:
    //kernel->useGlue = 0;
    kernel->useGlue = 1;
    /* IMPORTANT: to convert in 'partly SSA form' we require ~64 (usually more
                   than 32) registers. */
    assert(CONNEX_REG_COUNT != 32);

    printf("Calling connexGlobal->genLLVMISelManualCode() with "
           "offsetKernelToStartCodegenFrom = %d and numInstructionsToCodegen = %d\n",
           kernel->offsetKernelToStartCodegenFrom, kernel->numInstructionsToCodegen);
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

    connex->executeKernel("mul.f16");
    connex->readReduction();

    // void *ConnexMachine::readDataFromConnex(void *buffer, unsigned vectorCount, unsigned startVectorIndex)
    connex->readDataFromConnex(result, 1, 2);

    printf("\n\n\n\n");

    printf("MUL results are:\n");

#ifdef ZERO_AFTER_NUM_VALS_INPUT_TESTS
    for (int i = 0; i < NUM_VALS; i++) {
#else
    for (int i = NUM_VALS; i < CONNEX_VECTOR_LENGTH; i++) {
#endif
        if (isnan_f16(result[i]) || isnan_f16(resCorrect[i])) {
            //assert(isnan_f16(result[i]) && isnan_f16(resCorrect[i]));
        }

        if (result[i] == 0x8000)
            result[i] = 0x0000;
        if (resCorrect[i] == 0x8000)
            resCorrect[i] = 0x0000;

        printf("i=%d: opA = %s, opB = %s --> res = %s (resCorrect = %s)%s\n",
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
                     "" : " (different results!)")
              );
    }
}


void Test() {
    FloatMulTest(connexGlobal);
}

/*
int main(int argc, char *argv[]){

    if(argc < 6) {
        printf("Usage: %s insn red iowr iord regs\n",argv[0]);
        return 0;
    }

    try {
        ConnexMachine *connex = new ConnexMachine(argv[1], argv[2], argv[3], argv[4], argv[5]);

        FloatMulTest(connex);

        delete connex;
    }
    catch(string err) {
        cout << err << endl;
    }
}
*/

