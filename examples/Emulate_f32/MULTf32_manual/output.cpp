#include <iostream>
#include <stdlib.h>
#include "ConnexMachine.h"


//#define LLVM_ISEL_CODEGEN
#ifdef LLVM_ISEL_CODEGEN
  #ifdef PRINTREG
     #undef PRINTREG
     #define PRINTREG(regNum) regNum
  #endif

  #ifdef PrintDebugMessage
      #undef PrintDebugMessage
      #define PrintDebugMessage(aStr) aStr
  #endif

  #ifdef PrintRegDebug
      #undef PrintRegDebug
      #define PrintRegDebug(regNum) regNum
  #endif

  #ifdef PrintDebugReg
      #undef PrintDebugReg
      #define PrintDebugReg(regNum) regNum
  #endif
#endif


// These Opincaa subroutines should be defined after the #define LLVM_ISEL_CODEGEN
//#include "../LibMisc.h"
#include "LibMisc.h"

using namespace std;


/* An implementation of MULT.f32 which I strongly believe is IEEE 754-2008
  compliant
  - to test seriously
*/

/*
Denormals, infinities and underflows are very well explained in pages
 401 and 402 (Figure 8.2.a) of book [Ercegovac_Digital_Arithmetic_2004]
 (also at
  https://books.google.ro/books/about/Digital_Arithmetic.html?id=p79cu3nZ6yoC&redir_esc=y).

  VERY IMPORTANT: Denormals actually have exponent 1 and not 0.
        From https://en.wikipedia.org/wiki/Denormal_number
        "In binary interchange formats, subnormal numbers are encoded with a biased exponent of 0, but are interpreted with the value of the smallest allowed exponent, which is one greater (i.e., as if it were encoded as a 1)."

*/


#define REG64

// If we comment ROUND_TO_NEAREST we use round toward zero
#define ROUND_TO_NEAREST
#ifdef ROUND_TO_NEAREST
  #define COMPUTE_BETTER_T_BIT
#endif

/*
We can use for f32:
    - round to nearest
    OR
    - is truncation (round toward zero) - see 8.2.2 Round Toward Zero (Truncation)

Note: x86 (and probably all the other processors) seems to implement by
  default rounding to nearest
    - see https://docs.microsoft.com/en-us/cpp/build/mxcsr:
      "set to the following standard values at the start of program execution:"
      "MXCSR[13:14]   : Rounding  control - 0 (round to nearest)".
    (see also https://software.intel.com/en-us/articles/x87-and-sse-floating-point-assists-in-ia-32-flush-to-zero-ftz-and-denormals-are-zero-daz)
*/

string kernelName = "mult.f32";


//#define LANE_GATING

// It seems I'm treating well NaNs, INFs, denormal numbers
void Mult_f32Kernel(int32_t opAPtr, int32_t opBPtr, int32_t resPtr) {
    BEGIN_KERNEL(kernelName);
        EXECUTE_IN_ALL(

    // Register allocation table for the variables used in the program
    #define CT0              31
    #define CT1              30
    #define CT16             29
    #define CT255            28
    //
    #define SRC1             27
    #define SRC1_MANTISSA_H    26
  #ifdef REG64
    #define SRC1_MANTISSA_L  43
  #else
    #define SRC1_MANTISSA_L  19
  #endif
    #define SRC1_EXPONENT    25
    #define SRC1_SIGN        24
    //
    #define SRC2             23
    #define SRC2_MANTISSA_H    22
  #ifdef REG64
    #define SRC2_MANTISSA_L  42
  #else
    #define SRC2_MANTISSA_L  18 // TODO: check
  #endif
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
//    #define INPUT_EXP31    11
    #define CONTINUE        0
    #define CONTINUE_PRED   52
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
    #define AUX3           40

    // Note that these variables are already allocated to other register
    #define ZERO1           2
    #define ZERO2           1
    #define V2             63
    #define V1             62
    #define IS_NAN         61
    #define IS_INF         39

    #define IDX            41
    #define IDXMOD2        38
    #define IDXPRED        34

  #ifdef ROUND_TO_NEAREST
    #define L                         60
    #define G                         59
    #define T                         58
    #define RND                       57
    #define ROUND_NUM_ADDITIONAL_BITS 56

    /* DISCARDED_BITS stores the mantissa bits: G and T.
        G and T are normally discarded by the SHR operations we perform
        (either because the result mantissa has more than F32_MANTISSA_BITS or
        because the exponent < 0).
    */
    #define DISCARDED_BITS            55
  #endif
    /* NUM_DISCARDED_BITS represents the number of bits discarded from the mantissa
        (or number of steps we performed SHR on the result mantissa computed
       originally by multiplication). */
    #define NUM_DISCARDED_BITS        54

            R(SRC1) = LS[opAPtr]; // load 1st F32 operand
            R(SRC2) = LS[opBPtr]; // load 2nd F32 operand

            R(CT0) = 0;
            R(CT1) = 1;
            R(CT16) = 16;
            R(CT255) = 255; //31;

            R(MANTISSA_MASK)  = F32_MANTISSA_MASK_I16;
            R(EXPONENT_MASK)  = F32_EXPONENT_MASK_I16;
            R(SIGN_MASK)      = F32_SIGN_MASK_I16;
            R(HIDDENBIT_MASK) = F32_HIDDENBIT_MASK_I16;


          #ifdef ROUND_TO_NEAREST
            R(DISCARDED_BITS) = 0; // There's no guarantee below we initialize

            //R(NUM_DISCARDED_BITS) = -1;
          #endif

            R(CONTINUE) = 1;
            // 2021_04_21
            // We handle the odd indices:
            R(IDX) = INDEX;
            R(IDXMOD2) = R(IDX) & R(CT1);
            R(IDXPRED) = R(IDXMOD2) == R(CT1);
            //R(IDXPRED) = R(IDXMOD2) == R(CT0); // 2021_04_18
            NOP;
          );
          EXECUTE_WHERE_EQ(
            R(CONTINUE) = 0;
          );
          EXECUTE_IN_ALL(

/* TODO:
 *   small-TODO: UnpackF32() and the following code below both check for EXP == 31
 *      - the SIGN should not be put in a separate register for both SRC1 and
 *          SRC2 but only for the result
 *      - we should check maybe only once
 *
 * It is possible that we do NOT need to remove hidden bit for
 *      INF for our MUL operation --> try to optimize.
 *   Actually INF unpacked with mantissa 0 is very useful since it creates
 *     a result mantissa 0 and this becomes INF result (unless the other
 *     operand is NAN).
 */
            UnpackF32(__kernel,
                        CT0, CT1, CT255,
                        SRC1, SRC1_SIGN, SRC1_EXPONENT,
                        SRC1_MANTISSA_H, SRC1_MANTISSA_L,
                        SIGN_MASK, EXPONENT_MASK, MANTISSA_MASK,
                        HIDDENBIT_MASK,
                        PRED2, PRED2A, PRED3);

            UnpackF32(__kernel,
                        CT0, CT1, CT255,
                        SRC2, SRC2_SIGN, SRC2_EXPONENT,
                        SRC2_MANTISSA_H, SRC2_MANTISSA_L,
                        SIGN_MASK, EXPONENT_MASK, MANTISSA_MASK,
                        HIDDENBIT_MASK,
                        PRED2, PRED2A, PRED3);

            R(DST) = 0;
        // We handle the odd indices:
        R(IDX) = INDEX;
        R(IDXMOD2) = R(IDX) & R(CT1);
        R(IDXPRED) = R(IDXMOD2) == R(CT1);
        //R(IDXPRED) = R(IDXMOD2) == R(CT0); // 2021_04_18
        NOP;
      );
      EXECUTE_WHERE_EQ(
            R(DST) = F32_INF_POSITIVE_I16;
      );
      EXECUTE_IN_ALL(
      PrintDebugReg(DST);

      // Compute DST_SIGN // 2021_04_22
      // We handle the even indices:
      R(IDX) = INDEX;
      R(IDXMOD2) = R(IDX) & R(CT1);
      R(IDXPRED) = R(IDXMOD2) == R(CT0);
      NOP;
    );
    EXECUTE_WHERE_EQ(
        R(DST_SIGN) = R(SRC1_SIGN) ^ R(SRC2_SIGN);
        PrintDebugReg(DST_SIGN);
    );
    EXECUTE_IN_ALL(

        // To intialize for the predicated instructions to be generated safely - no CSE for predication
        R(DST_EXPONENT) = 0;
        R(DST_MANTISSA_L) = 0;
        R(DST_MANTISSA_H) = 0;
        R(NUM_DISCARDED_BITS) = 0;
        //R(AUX3) = 0;
       #ifdef ROUND_TO_NEAREST
        R(L) = 0;
        R(G) = 0;
        R(T) = 0;
        R(RND) = 0;
       #endif

            /* We now treat special cases with NAN: NAN * 0, NAN * INF .

               IMPORTANT NOTE:
               Therefore, we need to perform more complex handling
                  of cases, including NANs, INFs, etc - for this we use logic
                  minimization (Quine-McKluskey).

              From Ercegovac book, Section 8.5.2 Exceptions and Special Values:
             <<NAN: The result is a NAN if one (or both) of the operands is a
               NAN or if one of the operands is a 0 and the other +infinity.>>

             Note: we use logic minimization tool Espresso (maybe we can use
               Multi-level logic minimization; we also used Karnaugh maps,
               but we advise to use Espresso and especially to generate
               with program the values of the table - 1st time I did it
               completely by hand I was a bit wrong) to
               minimize the number of Connex instructions for this
               sub-procedure.
              (Standard two-level logic minimization - Karnaugh maps
                work only for minimizing boolean functions of at most 4 inputs
                normally - on a 2D plane,
                while the Espresso tool works in the general case).
               Maybe I shoot a fly with a cannon, but I don't think I
                   can generate smaller code than this.
               VERY IMPORTANT: It seems logic minimization is suitable for
                 generating smallest code possible (altough logic minimization
                 cares about minimizing number of implicants, while these
                 implicants can have a lot of variables each so the Connex
                 assembly code generated from the Espresso solution could
                 be quite big in the end) because Connex has
                 predication, and basically no conditional branches.

               Solution from Espresso is:
                  See /home/asusu/LLVM/Tests/opincaa_standalone_apps/Emulate_f16/1Espresso/WithDCs/MULf16/Observe_zeros/espresso_MULf16_gen_2do.output
                    IS_NAN = E1_255 !M1_0 + E2_255 !M2_0 + Zero1 E2_255 + E1_255 Zero2
                    IS_INF = E1_255 + E2_255
              I don't think we can benefit from multi-level logic minimization
                - read GDM's book chapter on this to see if
                  multiplexors/predicates/conditionals are allowed.
                  - find in which Section of GDM book is this don't cares issue
                    (remember that I discovered it because I had computed
                     INPUT_EXP31 before thinking on doing logic minimization)

              VERY IMPORTANT:
               If any of the operands is INF, we don't process it further because:
                - if the other operand is 0 then we set it to NAN in IS_NAN
                    below
                - multiplying a +/-INF with the other operand results in:
                    - mantissa 0
                    - exponent 31 (or higher, which is corrected to 31)
                    - the final result will be an INF
                    - the sign of the result will be correct.
            */
            R(ZERO1) = R(SRC1) == R(SRC1_SIGN);
            R(ZERO2) = R(SRC2) == R(SRC2_SIGN);
          PrintDebugReg(SRC1_SIGN);
          PrintDebugReg(SRC2_SIGN);
          PrintDebugReg(ZERO1);
          PrintDebugReg(ZERO2);
          PrintDebugReg(DST);
            //
            // small TODO (maybe DAG Combiner takes care): SRC1_EXPONENT == 31 was already computed above
          #define NOT_M1_0 38
          #define NOT_M2_0 37
          #define E1_255 36
          #define E2_255 35
            R(E1_255) = R(SRC1_EXPONENT) == R(CT255);
          PrintDebugReg(E1_255);
            R(NOT_M1_0) = R(CT0) < R(SRC1_MANTISSA_H);
            // Note: !(M1 == 0) is !(Low == 0 && High == 0), which is (Low != 0 || High > 0), since High cannot be negative (since most significant 9 bits are 0)
            //R(AUX) = R(CT0) < R(SRC1_MANTISSA_L);
            R(AUX) = R(CT0) == R(SRC1_MANTISSA_L);
            R(AUX) = R(CT1) - R(AUX);
            R(NOT_M1_0) |= R(AUX);
          PrintDebugReg(NOT_M1_0);
            //
            // small TODO (maybe DAG Combiner takes care): SRC2_EXPONENT == 31 was already computed above
            R(E2_255) = R(SRC2_EXPONENT) == R(CT255);
            R(NOT_M2_0) = R(CT0) < R(SRC2_MANTISSA_H);
            //R(AUX) = R(CT0) < R(SRC2_MANTISSA_L);
            R(AUX) = R(CT0) == R(SRC2_MANTISSA_L);
            R(AUX) = R(CT1) - R(AUX);
            R(NOT_M2_0) |= R(AUX);
          PrintDebugReg(NOT_M2_0);
            //
            R(V1) = R(E1_255) & R(NOT_M1_0);
            R(V2) = R(E2_255) & R(NOT_M2_0);
            R(IS_NAN) = R(V1) | R(V2);
          PrintDebugReg(IS_NAN);
            //
            R(V1) = R(E1_255) & R(ZERO2);
          PrintDebugReg(V1);
            R(V2) = R(E2_255) & R(ZERO1);
          PrintDebugReg(V2);
            R(IS_NAN) |= R(V1);
            R(IS_NAN) |= R(V2);
          PrintDebugReg(IS_NAN);




            R(AUX2) = R(IS_NAN) == R(CT1);
          PrintDebugReg(CT1);
          PrintDebugReg(AUX2);
            NOP;
        );
        EXECUTE_WHERE_EQ(
            //R(DST) = F32_NAN_1_I16;
            R(DST) = 0x0001;
          PrintDebugReg(DST);
      #ifdef ROUND_TO_NEAREST
            /* IMPORTANT: we don't need this because RND can be
             at most 1 and F32_NAN is positive number, so adding RND still
            makes DST a NAN */
            //R(RND) = 0;
          PrintDebugReg(RND);
      #endif

         #ifdef LANE_GATING
            DISABLE_CELL;
         #else
            R(CONTINUE) = 0;
         #endif
        );
        EXECUTE_IN_ALL(


        // 2021_04_20: Continuing with setting DST to NAN (the odd indices)
        R(AUX2) = R(IS_NAN) == R(CT1);
        //
        CELL_SHR(R(AUX2), R(CT1));
        NOP; // It is required
        R(AUX) = SHIFT_REG;
        //
        // We handle the odd indices:
        R(IDX) = INDEX;
        R(IDXMOD2) = R(IDX) & R(CT1);
        R(IDXPRED) = R(IDXMOD2) == R(CT1);
        //R(IDXPRED) = R(IDXMOD2) == R(CT0); // 2021_04_18
        R(AUX) = R(AUX) & R(IDXPRED);
        R(AUX) = R(AUX) == R(CT1);
        NOP;
      );
      EXECUTE_WHERE_EQ(
            R(DST) = F32_NAN_1_I16;
        );
        EXECUTE_IN_ALL(
          PrintDebugMessage("Setting NAN:\n");
          PrintDebugReg(DST);

            R(IS_INF) = R(E1_255) | R(E2_255);
            R(AUX) = R(IS_NAN) == R(CT0);
            R(AUX2) = R(IS_INF) == R(CT1);
            R(AUX2) &= R(AUX);
          PrintDebugReg(AUX2);
            R(AUX2) = R(AUX2) == R(CT1);
          PrintDebugReg(AUX2);
            NOP;
        );
        EXECUTE_WHERE_EQ(
            // Note: R(DST) is initialized to F32_INF_POSITIVE_I16
            R(DST) |= R(DST_SIGN);
          PrintDebugReg(DST_SIGN);
          PrintDebugReg(DST);

         #ifdef LANE_GATING
            DISABLE_CELL;
         #else
            R(CONTINUE) = 0;
         #endif
        );
        EXECUTE_IN_ALL(


    // END: We now treat special cases with NAN: NAN * 0, NAN * INF .


// Used ONLY to count the number of instructions: REDUCE(R30);
            /*
            R(SRC1_MANTISSA_H) = R(SRC1) & R(MANTISSA_MASK);
            // Add hidden bit for the mantissa (from bit 0, as it is initially)
            R(SRC1_MANTISSA_H) |= R(HIDDENBIT_MASK);
            PrintDebugReg(SRC1_MANTISSA_H);
            R(SRC1_EXPONENT) = R(SRC1) & R(EXPONENT_MASK);
            // Get the exponent from bit 0 (shift down to LSB)
            R(SRC1_EXPONENT) >>= F32_MANTISSA_BITS;
            PrintDebugReg(SRC1_EXPONENT);
            // R(SRC1_SIGN) contains the sign on bit 15
            R(SRC1_SIGN) = R(SRC1) & R(SIGN_MASK);

            R(SRC2_MANTISSA_H) = R(SRC2) & R(MANTISSA_MASK);
            // Add hidden bit for the mantissa (from bit 0, as it is initially)
            R(SRC2_MANTISSA_H) |= R(HIDDENBIT_MASK);
            PrintDebugReg(SRC2_MANTISSA_H);
            R(SRC2_EXPONENT) = R(SRC2) & R(EXPONENT_MASK);
            // Get the exponent (shift down to LSB)
            //R(SRC2_EXPONENT) = R(SRC2_EXPONENT) >> F32_MANTISSA_BITS;
            R(SRC2_EXPONENT) >>= F32_MANTISSA_BITS;
            PrintDebugReg(SRC2_EXPONENT);
            */

    #ifdef NOT_WITH_INPUT_EXP31
       // IMPORTANT: We use INPUT_EXP31 to mean if we have any opnd NAN.
            // Check if 1st opnd is NAN
            R(INPUT_EXP31) = R(SRC1_EXPONENT) == R(CT255);
            /*
            R(PRED3) = R(SRC1_EXPONENT) == R(CT255);
          //PrintDebugReg(PRED3);
            R(CONTINUE) = R(SRC1_MANTISSA_H) == R(CT0);
          //PrintDebugReg(CONTINUE);
            R(CONTINUE) = R(CT1) - R(CONTINUE);
          //PrintDebugReg(CONTINUE);
            R(CONTINUE) &= R(PRED3);
          PrintDebugReg(CONTINUE);
            */
            //
            // Check if 2nd opnd is NAN
            R(PRED3) = R(SRC2_EXPONENT) == R(CT255);
            /*
          //PrintDebugReg(PRED3);
            R(PRED4) = R(SRC2_MANTISSA_H) == R(CT0);
          //PrintDebugReg(PRED4);
            R(PRED4) = R(CT1) - R(PRED4);
          //PrintDebugReg(PRED4);
            R(PRED4) &= R(PRED3);
            R(CONTINUE) |= R(PRED4);
            */
            R(INPUT_EXP31) |= R(PRED3);
          PrintDebugReg(INPUT_EXP31);
    #endif // NOT_WITH_INPUT_EXP31


/*            // Compute sign
            R(DST_SIGN) = R(SRC1_SIGN) ^ R(SRC2_SIGN);
          PrintDebugReg(DST_SIGN);
*/
            /* Add exponents and re-bias.
               The re-bias is necessary because:
                for 2 arbitrary operands the bias is used like this:

                 Note: For the general case,
                    E is the field packed in f32, e is the actual exponent.
                    So e = E - bias, where bias = 15 for f32
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

          #ifdef LANE_GATING
          #else
            R(CONTINUE_PRED) = R(CONTINUE) == R(CT1);
            NOP;
           );
          EXECUTE_WHERE_EQ(
          #endif
            R(DST_EXPONENT) = R(SRC1_EXPONENT) + R(SRC2_EXPONENT);
            R(AUX) = F32_EXPONENT_BIAS; //15;
            R(DST_EXPONENT) -= R(AUX);
         PrintDebugMessage("After re-bias:");
          PrintDebugReg(DST_EXPONENT);

            /* Multiply mantissas (with hidden bits included if available).
               We get a 22-bit result in two 16-bit registers. */
        /*
            R(SRC1_MANTISSA_H) * R(SRC2_MANTISSA_H);
            R(DST_MANTISSA_L) = MULT_LOW();
            R(DST_MANTISSA_H) = MULT_HIGH();
        */







        /*
        To multiply two 32 bits integers md * mr (actually 24 bits) we can work on their 16-bits subword integers:
            Note: md = multiplicand (SRC2_MANTISSA_H), mr = multiplier (SRC1_MANTISSA_H)
            res16_1 * 2^16 + res16_0 = (md16_1 * 2^16 + md16_0) * (mr16_1 * 2^16 + mr16_0);
            res16_1 * 2^16 + res16_0 = (sign: to be computed)
                   ... (md16_1 * mr16_1) * 2^32 + (md16_1 * mr16_0 + md16_0 * mr16_1) * 2^16 + md16_0 * mr16_0;

        IMPORTANT:
          We actually multiply two 23-bit mantissas also with hidden bits,
            so we multiply two 24-bit integers and obtain a 48-bit value.
            The result will finally be a 24-bit integers, but we
               need to perform CountLeadingZeros on the 48-bit result and
               keep only the most signficand 24-bits of the result.
        */
        PrintDebugMessage("Performing MULT.i32:");
        PrintDebugReg(SRC1_MANTISSA_L);
        PrintDebugReg(SRC1_MANTISSA_H);
        PrintDebugReg(SRC2_MANTISSA_L);
        PrintDebugReg(SRC2_MANTISSA_H);
        //
        // We perform the actual multiplication:
        // Computing md16_0 * mr16_0
        MULT_U( R(SRC2_MANTISSA_L), R(SRC1_MANTISSA_L) );
        R(DST_MANTISSA_L) = MULT_LOW();
        R(DST_MANTISSA_H) = MULT_HIGH();
        //PrintDebugMessage("DST_MANTISSA_L:\n");
        PrintDebugReg(DST_MANTISSA_L);
        //PrintDebugMessage("DST_MANTISSA_H:\n");
        PrintDebugReg(DST_MANTISSA_H);

      #define RES1_L         33
      #define RES2_L         32
      #define RES1_H         13
      #define RES2_H         14
      #define RES3_L         12

        // Computing md16_1 * mr16_0
        /*
        CELL_SHR(R(SRC1_MANTISSA_H), R(CT1));
        NOP; // It is required
        R(RES1_L) = SHIFT_REG;
       PrintDebugReg(RES1_L);
        */
        MULT_U( R(SRC2_MANTISSA_H), R(SRC1_MANTISSA_L) );
        R(RES1_L) = MULT_LOW();
        R(RES1_H) = MULT_HIGH();
       PrintDebugReg(RES1_L);
       PrintDebugReg(RES1_H);


        // Computing md16_0 * mr16_1
        /*
        CELL_SHR(R(SRC2_MANTISSA_H), R(CT1));
        NOP; // It is required
        R(RES2_L) = SHIFT_REG;
       PrintDebugReg(RES2_L);
        */
        MULT_U( R(SRC2_MANTISSA_L), R(SRC1_MANTISSA_H) );
        R(RES2_L) = MULT_LOW();
        R(RES2_H) = MULT_HIGH();
       PrintDebugReg(RES2_L);
       PrintDebugReg(RES2_H);


       // Computing md16_1 * mr16_1
        MULT_U( R(SRC2_MANTISSA_H), R(SRC1_MANTISSA_H) );
        R(RES3_L) = MULT_LOW();
        // Note: Each mantissa has a maximum of 24 bits. So the bits 63..48 of
        //   the result of the multiplication are actually 0.
       PrintDebugReg(RES3_L);


        //R(DST) = R(DST_MANTISSA_L) | R(DST_MANTISSA_L);
       //PrintDebugReg(DST);

        // We also add the contribution of the other 2 16-bit MULs we performed
        R(DST_MANTISSA_H) += R(RES1_L);
        R(AUX) = ADDC(R(CT0), R(CT0)); // 2021_05_02
       PrintDebugReg(AUX);
        R(DST_MANTISSA_H) += R(RES2_L);
        //PrintDebugReg(DST_MANTISSA_L);
       PrintDebugReg(DST_MANTISSA_H);


      #define DST_MANTISSA_H2         33
        // Taking the carry from R(DST_MANTISSA_H)
        R(DST_MANTISSA_H2) = ADDC(R(CT0), R(CT0));
        R(DST_MANTISSA_H2) += R(AUX);
       PrintDebugReg(DST_MANTISSA_H2);
        //
        R(DST_MANTISSA_H2) += R(RES1_H);
       //PrintDebugReg(DST_MANTISSA_H2);
        R(DST_MANTISSA_H2) += R(RES2_H);
       //PrintDebugReg(DST_MANTISSA_H2);
        R(DST_MANTISSA_H2) += R(RES3_L);
       PrintDebugReg(DST_MANTISSA_H2);

        /*
        The number of bits of the result of the multiplied mantissa for normalized
          input mantissas is:
          - 47 bits:
            e.g., for input mantissas: 0x800000 * 0x800000, for which the result is
            0x4000 0000 0000

          - 48 bits:
            e.g., for input mantissas: 0xFFFFFF * 0xFFFFFF, for which the result is
            0xffff fe00 0001
        */

       /*
        // We write the most-significant 16-bits of the result in R(DST)
        CELL_SHR(R(DST_MANTISSA_H), R(CT1));
        NOP; // It is required
  #define RESX DST_MANTISSA_H
        R(RESX) = SHIFT_REG;
        //
        PrintDebugReg(DST_MANTISSA_H);
        //
        // We handle the odd indices:
        R(IDX) = INDEX;
        R(IDXMOD2) = R(IDX) & R(CT1);
        R(IDXPRED) = R(IDXMOD2) == R(CT1);
        NOP;
        PrintDebugReg(RES1_L);
        PrintDebugReg(RES2_L);
      );
      EXECUTE_WHERE_EQ(
        R(DST) = R(RESX) | R(RESX); // High 16 bits are from RESX
      );
      EXECUTE_IN_ALL(
        PrintDebugMessage("DST after MULT.i32 is:");
        PrintDebugReg(DST);
    */
        PrintDebugMessage("DST Mantissa after MULT.i32 is:");
        PrintDebugReg(DST_MANTISSA_L);
        PrintDebugReg(DST_MANTISSA_H);
        PrintDebugReg(DST_MANTISSA_H2);

        /*
        R(DST_MANTISSA_L) = R(DST_MANTISSA_H);
        R(DST_MANTISSA_H) = R(DST_MANTISSA_H2);
        */
        /*R(DST_MANTISSA_L) <<= 1;
        R(DST_MANTISSA_H) <<= 1;*/

        PrintDebugReg(DST_MANTISSA_L);
        PrintDebugReg(DST_MANTISSA_H);
        PrintDebugReg(DST_MANTISSA_H2);



// MEGA MEGA TODO: compute correct DST_EXPONENT and normalize mantissa
//   stored in DST_MANTISSA_L, DST_MANTISSA_H, DST_MANTISSA_H2 - maybe use
// CountLeadingZeros()






         PrintDebugMessage("DST_MANTISSA_L/H (original):");
          PrintDebugReg(SRC1_MANTISSA_H);
          PrintDebugReg(SRC2_MANTISSA_H);
          PrintDebugReg(DST_MANTISSA_L);
          PrintDebugReg(DST_MANTISSA_H);
          //
          PrintDebugReg(SRC1_EXPONENT);
          PrintDebugReg(SRC2_EXPONENT);
          PrintDebugReg(DST_EXPONENT);


          #ifdef LANE_GATING
          #else
           );
          EXECUTE_IN_ALL(
          #endif






          /* We defer the correction of the DST_EXPONENT < 0 after we
               renormalize the DST_MANTISSA. */


//#if 0 // 2021_04_18
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
                - OLD f16: (otherwise, SHR if it has 2 * F32_MANTISSA_BITS + 1 useful bits).

            Treating denormals takes about ... more instructions.
            */

            /*
            We compute in
              NUM_BITS = number of significant bits of the multiplication result
               - to compute this is I guess simpler than computing for each
                 operand in part.
              The multiplication result is a value with at most 47 (OLD for f16: 21) bits
              (or 48 (OLD for f16: 22) if both operands are not denormals and are big,
                 and at least 0 bits),
                 since the input mantissas have at most 24 (OLD for f16: 11)
                 significant bits each
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
               48-bit word R(DST_MANTISSA_L)|R(DST_MANTISSA_H)|R(DST_MANTISSA_H2).
                OLD for f16: 32-bit word R(DST_MANTISSA_L)|R(DST_MANTISSA_H).
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
            // 2021_04_20: R(VAL_FOR_SIZE) = R(DST_MANTISSA_L);
            R(VAL_FOR_SIZE) = R(DST_MANTISSA_H);

            // 2021_04_20: R(AUX) = R(CT0) < R(DST_MANTISSA_H);
            //R(AUX) = ULT(R(CT0), R(DST_MANTISSA_H));
            R(AUX) = ULT(R(CT0), R(DST_MANTISSA_H2));

          #ifdef LANE_GATING
            NOP;
           );
          EXECUTE_WHERE_LT(
          #else
            R(CONTINUE_PRED) = R(AUX) & R(CONTINUE);
            R(CONTINUE_PRED) = R(CONTINUE_PRED) == R(CT1);
            NOP;
           );
          EXECUTE_WHERE_EQ(
          #endif

            // 2021_04_20: R(VAL_FOR_SIZE) = R(DST_MANTISSA_H);
            R(VAL_FOR_SIZE) = R(DST_MANTISSA_H2);
         PrintDebugMessage("In conditional:");
           PrintDebugReg(VAL_FOR_SIZE);
           PrintDebugReg(DST_MANTISSA_H);

          #ifdef OR_SCAN_BITS_FOR_RATHER_EFFICIENT_CTLZ
            //R(AUX2) = 16 + 16;
            R(AUX2) = 32 + 16;
          #else
            /* We initialize to 17 + 16 because my CONNEX_HAS_BITREVERSE
                technique (and also the inefficient one with host-side for loop)
                returns the result of CTLZ + 1.
            */
            R(AUX2) = 16 + 17 + 16;
          #endif

          #ifdef LANE_GATING
          #else
           );
          EXECUTE_IN_ALL(
          #endif
// MEGA-TODO: use CONTINUE_PRED

            // So VAL_FOR_SIZE is either DST_MANTISSA_H2 or DST_MANTISSA_H
           PrintDebugReg(AUX2);
            CountLeadingZeros(__kernel,
                              VAL_FOR_SIZE,
                              // The result, set to AUX2 if VAL_FOR_SIZE == 0
                              NUM_BITS,
                              AUX2,
                              CT1,
                              CT16,
                              //int SRC1_MANTISSA_H,
                              //int SRC1_EXPONENT,
                              AUX,
                              //int PRED3,
                              CONTINUE);


            R(NUM_BITS) = R(AUX2) - R(NUM_BITS);
           PrintDebugMessage("Num significant bits for the result of mantissa multiplication:\n");
           PrintDebugReg(NUM_BITS);
           PrintDebugReg(AUX2);










            /* VERY VERY IMPORTANT: Now we renormalize the result of the
                  multiplication of mantissas, even for denormals, as
                  said in [Ercegovac_Digital_Arithmetic_2004, Section 8.5.3].
               Normalization means that we make the result of mantissa
                 multiplication be stored on only F32_MANTISSA_BITS + 1
                   significant bits.
                This is done by a SHR by NUM_BITS - (F32_MANTISSA_BITS + 1),
                   which becomes a SHL if this value is negative.

            IMPORTANT:
               This step is NOT described in [Ercegovac], but we use it, being
                our own implementation detail.
               So this is our convention and we do not change the exponent
                value here.


             Note: On May 4th and 5th, 2018 I tested on zedboard.arh.pub.ro
                that on Connex, when given
                  Rdst = SHR Rop1, -1 // or other negative value
               actually just puts 0 into Rdst.

               Also, Rdst = SHR Rop1, 16 (TODO-REMEMBER)
                   just copies Rop1 into Rdst, which is NOT correct

               Note that Rdst = SHR Rop1, 15 does the correct operation.

               For this see folder /home/alarm/Experiments/Test_SHR_special_2nd_opnd .
            */

            /* We SHR by AUX (= F32_MANTISSA_BITS + 1) positions both
               DST_MANTISSA_H and DST_MANTISSA_L together in order
               to keep the significant
                bits of the entire 32-bits (both _L and _H) of result of
                mantissa.
               This makes DST_MANTISSA_L hold F32_MANTISSA_BITS + 2 bits,
                 at most, or less (especially if we have denormals).

               After, this, the mantissa contains, as expected, the hidden bit.
             Note: we change DST_EXPONENT below
            */
            //R(AUX) = F32_MANTISSA_BITS_I16 + 1;
            R(AUX) = F32_MANTISSA_BITS + 1;
            R(AUX) = R(NUM_BITS) - R(AUX);
            // OLD f16: Note: Now R(AUX) is smaller than 22 - 11 = 11 .
           PrintDebugMessage("AUX used to SHR mantissa of the result multiplication:\n");
           PrintDebugReg(AUX);
           /*
          VERY IMPORTANT: Note that the adjustment of DST_EXPONENT must be thought SEPARATELY
             from the SHR above of DST_MANTISSA, since the SHR ONLY prepares the
             DST_MANTISSA to be copied in res.
          */

            //R(AUX2) = R(AUX) < R(CT0); // This favors generation of COPY instruction before WHERE
            R(PRED3) = R(AUX) < R(CT0);

          #ifdef LANE_GATING
            NOP;
           );
          EXECUTE_WHERE_LT(
          #else
            R(CONTINUE_PRED) = R(PRED3) & R(CONTINUE);
            R(CONTINUE_PRED) = R(CONTINUE_PRED) == R(CT1);
            NOP;
           );
          EXECUTE_WHERE_EQ(
          #endif

            //R(AUX) = 0;
            /* This should happen only for denormals:
                 we renormalize the mantissa. */
            R(AUX2) = R(CT0) - R(AUX);
            R(DST_MANTISSA_L) <<= R(AUX2); // MEGA TODO: adjust for f32
          );
          EXECUTE_IN_ALL(
           PrintDebugMessage("Adjusted AUX:\n");
           PrintDebugReg(AUX);
           PrintDebugReg(AUX2);
           PrintDebugReg(DST_EXPONENT);
           PrintDebugReg(DST_MANTISSA_L);
           PrintDebugReg(DST_MANTISSA_H);
           PrintDebugReg(DST_MANTISSA_H2);


            //R(AUX2) = R(CT0) < R(AUX); // This favors generation of COPY instruction before WHERE
            R(PRED3) = R(CT0) < R(AUX);
          #ifdef LANE_GATING
            NOP;
           );
          EXECUTE_WHERE_LT(
          #else
            R(CONTINUE_PRED) = R(PRED3) & R(CONTINUE);
            R(CONTINUE_PRED) = R(CONTINUE_PRED) == R(CT1);
            NOP;
           );
          EXECUTE_WHERE_EQ(
          #endif
            /* This should happen only for NON-denormals. */
           PrintDebugReg(AUX);
            /*
            R(DST_MANTISSA_L) >>= R(AUX);
          //PrintDebugReg(DST_MANTISSA_L);
            //
            R(AUX) = R(CT16) - R(AUX);
           PrintDebugReg(AUX);
            R(DST_MANTISSA_H) <<= R(AUX);
          PrintDebugReg(DST_MANTISSA_H);
            //
            R(DST_MANTISSA_L) |= R(DST_MANTISSA_H);
            */

            // 2021_04_20
            // We do like this because we do not care about DST_MANTISSA_L, so
            //   we basically subtract 16 from R(AUX)
            R(AUX2) = 0xF; //15; // MEGA TODO: this is a bit WRONG

            // 2021_04_27
            // IMPORTANT: If AUX is <= 15 (which implies also NUM_BITS <= 39)
            //     then we don't need to copy DST_MANTISSA_* (equivalent of
            //     doing SHR 16). This is normal since we need AUX >= 16 to do
            //     equivalent of SHR 16.
            R(PRED3) = R(AUX2) < R(AUX);
            //R(PRED3) = R(AUX) == R(CT16);
//            R(PRED3) = R(AUX2) < R(AUX);
            // Disregarding normalized numbers (non-denormals)
//            R(PRED2) = 0x17; // 23 + 24 = 47 (So normalized (non-denormals) have AUX 0x17 si 0x18)
//            R(PRED2) = R(AUX) < R(PRED2);
            R(PRED2) = R(DST_EXPONENT) < R(CT0);
           PrintDebugReg(PRED2);
            R(PRED2) &= R(PRED3);
           PrintDebugReg(PRED2);
            R(PRED2) = R(PRED2) == R(CT1);
            NOP;
           );
          EXECUTE_WHERE_EQ(
          //EXECUTE_WHERE_LT(
            //R(AUX2) = 0x10;
            /*
            */
            R(DST_MANTISSA_L) = R(DST_MANTISSA_H);
            R(DST_MANTISSA_H) = R(DST_MANTISSA_H2);
            R(DST_MANTISSA_H2) = 0;
           PrintDebugReg(DST_MANTISSA_L);
           PrintDebugReg(DST_MANTISSA_H);
           PrintDebugReg(DST_MANTISSA_H2);
          );
          EXECUTE_IN_ALL(
          PrintDebugReg(AUX2);
          PrintDebugReg(AUX);
            R(AUX) &= R(AUX2);
          PrintDebugReg(AUX);

            // 2021_04_20: R(AUX2) = R(DST_MANTISSA_H);
            R(AUX2) = R(DST_MANTISSA_H2);
            // 2021_04_20: R(DST_MANTISSA_H) >>= R(AUX);
          PrintDebugReg(DST_MANTISSA_H2);
            R(DST_MANTISSA_H2) >>= R(AUX);
          PrintDebugReg(DST_MANTISSA_H2);
            //
            //R(PRED3) = R(CT16) - R(AUX);
        /*
          VERY IMPORTANT: Note that the adjustment of DST_EXPONENT must be thought SEPARATELY
             from the SHR above of DST_MANTISSA, since the SHR ONLY prepares the
             DST_MANTISSA to be copied in res.
        */

    #ifdef REG64
          #define AUX_ORIG 53
    #else
          #define AUX_ORIG PRED3
    #endif
            // small-TODO #ifndef ROUND_TO_NEAREST we could take out R(AUX_ORIG) = R(AUX);
            R(AUX_ORIG) = R(AUX);
            R(AUX) = R(CT16) - R(AUX);

// MEGA-TODO: small: use CONTINUE_PRED
          #ifdef ROUND_TO_NEAREST
           PrintDebugMessage("ROUND_TO_NEAREST1:\n");
          PrintDebugReg(DST_MANTISSA_L);
            // 2021_04_20: TODO:
            R(DISCARDED_BITS) = R(DST_MANTISSA_L); // 2021_05_03
            // R(DISCARDED_BITS) = R(DST_MANTISSA_H); // 2021_05_03

/* small-MEGA-TODO (tried the code, but doesn't work on real Connex):
In order to be able to compute G (and maybe T) we should really keep in DISCARDED_BITS some of the (previous) bits
      if R(AUX) < ROUND_NUM_ADDITIONAL_BITS
   Maybe this case is NOT really encountered for MULT.f32, which needs to discard some bits - although we can have cases with denormals with mantissas smaller e.g. than 3 bits each.
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
   PrintDebugReg(TMP);
   R(TMP) = R(AUX) < R(TMP);
   PrintDebugReg(AUX);
   PrintDebugReg(TMP);
   R(AUX) += R(TMP);
   R(DISCARDED_BITS) <<= R(TMP);
   R(AUX) += R(TMP);
   R(DISCARDED_BITS) <<= R(TMP);
*/
            // We store only the discarded bits to compute well T
            R(DISCARDED_BITS) <<= R(AUX);
            R(DISCARDED_BITS) >>= R(AUX);
          PrintDebugReg(DISCARDED_BITS);

            R(NUM_DISCARDED_BITS) = R(AUX_ORIG);
          PrintDebugReg(NUM_DISCARDED_BITS);
          #endif

            R(PRED3) = R(DST_MANTISSA_H) << R(AUX); // 2021_05_02
            // 2021_04_20: R(DST_MANTISSA_L) >>= R(AUX_ORIG);
            R(DST_MANTISSA_H) >>= R(AUX_ORIG); // R(AUX_ORIG) = R(AUX); (used for SHR DST_MANTISSA_H2)
          //PrintDebugReg(DST_MANTISSA_L);
            //
//            R(AUX) = R(CT16) - R(AUX);
           PrintDebugReg(AUX_ORIG);
           PrintDebugReg(AUX);
            R(AUX2) <<= R(AUX); // R(AUX2) = R(DST_MANTISSA_H2);
          PrintDebugReg(AUX2);
          PrintDebugReg(DST_MANTISSA_H);
            //
            // 2021_04_20: R(DST_MANTISSA_L) |= R(AUX2);
            R(DST_MANTISSA_H) |= R(AUX2);
        // Maybe MEGA-TODO: we should maybe do SHR and OR also for R(DST_MANTISSA_L)

            R(DST_MANTISSA_L) >>= R(AUX_ORIG); // R(AUX_ORIG) = R(AUX); (used for SHR DST_MANTISSA_H2)
            R(DST_MANTISSA_L) |= R(PRED3); // 2021_05_02


          PrintDebugMessage("Normalized result-mantissa:\n");
          PrintDebugReg(DST_MANTISSA_L);
          PrintDebugReg(DST_MANTISSA_H);
          PrintDebugReg(DST_MANTISSA_H2);
          )
          EXECUTE_IN_ALL(

          #ifdef LANE_GATING
          #else
            R(CONTINUE_PRED) = R(CONTINUE) == R(CT1);
            NOP;
           );
          EXECUTE_WHERE_EQ(
          #endif

            /* VERY IMPORTANT: This is a bit difficult to understand, but I made
                efforts to explain it well:
            When we multiply the mantissas the contract is that
              the result should have 1 "integer" bit
                     (and let's say 2 * F32_MANTISSA_BITS fractional bits,
                       although some of them get lost due to SHR, etc)
                and the exponent should be correlated to that mantissa.
             (After, this, the mantissa contains, as expected, the hidden bit.)

              Above, in DST_MANTISSA_H and DST_MANTISSA_H2
                OLD f16: DST_MANTISSA_L
                we have INVARIABLY
                F32_MANTISSA_BITS + 1 significant bits (or less for denormals).
                They wait to be laid in the final result WITHOUT ANY FURTHER
                   change.
              The above contract is violated if:
                - we have denormal operand(s) - because the exponent is not
                   correlated to the mantissa (which we suppose is already
                   normalized, i.e. with 1 "integer" bit and rest fraction).
                - if the final result of multiplication has a total
                  2 * F32_MANTISSA_BITS + 2 bits (with 2 integer bits included)
                  because the mantissa is not normalized again
                    since it has 2 integer bits.
                  So if NUM_BITS = 22, the bits were laid out correctly
                   above, in DST_MANTISSA_L, and then we decrement the
                   exponent DST_EXPONENT (with -= (-1) ).
               Note that, as expected, for the case NUM_BITS = 21 nothing
                  changes.

            Remember that the result of multiplication can have:
               - as low as 0 bits

               - 47 bits:
                 e.g., for input mantissas: 0x800000 * 0x800000, for which the result is
                 0x4000 0000 0000
               - OLD f16: 21 bits: (2^10) * (2^10)         = 0001 0000 0000 0000 0000 0000

               - 48 bits:
                 e.g., for input mantissas: 0xFFFFFF * 0xFFFFFF, for which the result is
                 0xffff fe00 0001
               - OLD f16: as high as 22 bits: (2^11 - 1) * (2^11 - 1) = 0011 1111 1111 0000 0000 0001

             Therefore, we need to correct the exponent some more (for denormals
                and "bigger", 2 * F32_MANTISSA_BITS + 2 bits, multiplication
                results)
                s.t. we have a new result mantissa stored in DST_MANTISSA_L
                  with 1 (hidden) "integer" bit and F32_MANTISSA_BITS bits
                  of fraction after.

            For example - F16 examples:
                0x0002 = 2^(-14) * 0.0000000010 (E1 = 0 --> denormal we make
                                                    it 1, F1 = 0x2)
                0x7B53 = 2^(30 - 15) * 1.1101010011 (E2 = 30, F2 = 0x353)
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
                  - (AUX = NUM_BITS - (F32_MANTISSA_BITS + 1) = 11)
                  - we correct finally the exponent by subtracting
                    2 * F32_MANTISSA_BITS + 1 - NUM_BITS = 21 - 22 = -1.

              If NUM_BITS = 21 then:
                - DST_MANTISSA_L contains now 11 significant bits
                    (6 most significant from _L and 5 least significant from _H)
                  - (AUX = NUM_BITS - (F32_MANTISSA_BITS + 1) = 10)
                  - we correct finally the exponent by subtracting
                    2 * F32_MANTISSA_BITS + 1 - NUM_BITS = 21 - 21 = 0.

              If NUM_BITS = 18 then:
                - DST_MANTISSA_L contains now 11 significant bits
                    (9 most significant from _L and 7 least significant from _H)
                  - (AUX = NUM_BITS - (F32_MANTISSA_BITS + 1) = 7)
                  - we correct finally the exponent by subtracting
                    2 * F32_MANTISSA_BITS + 1 - NUM_BITS = 21 - 18 = 3.

              If NUM_BITS = 12 then:
                - DST_MANTISSA_L contains now 11 significant bits
                    (15 most significant from _L and 1 least significant from _H)
                  - (AUX = NUM_BITS - (F32_MANTISSA_BITS + 1) = 1)
                  - we correct finally the exponent by subtracting 21 - 12 = 9.

              If NUM_BITS = 11 then:
                - DST_MANTISSA_L contains now 11 significant bits
                    (all 16 from _L)
                  - (AUX = NUM_BITS - (F32_MANTISSA_BITS + 1) = 0)
                  - we correct finally the exponent by subtracting 21 - 11 = 10.

              If NUM_BITS = 10 then:
                - DST_MANTISSA_L contains now 10 significant bits
                    (all from _L)
                  - (AUX = NUM_BITS - (F32_MANTISSA_BITS + 1) = -1,
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


          VERY IMPORTANT: Note that the adjustment of DST_EXPONENT must be thought SEPARATELY
             from the SHR above of DST_MANTISSA, since the SHR ONLY prepares the
             DST_MANTISSA to be copied in res.
            */
            //R(AUX) = 2 * F32_MANTISSA_BITS_I16 + 1;
            R(AUX) = 2 * F32_MANTISSA_BITS + 1;
           PrintDebugReg(AUX);
            //R(AUX) -= R(NUM_BITS);
           PrintDebugMessage("Correcting DST_EXPONENT for denormals and mantissa results with 2 'integer' bits:");
           PrintDebugReg(NUM_BITS);
            R(AUX) -= R(NUM_BITS);
//            R(AUX) = F32_MANTISSA_BITS_I16;
           PrintDebugReg(AUX);
            //
           PrintDebugReg(DST_EXPONENT);
            R(DST_EXPONENT) -= R(AUX); //R(CT1);
           PrintDebugReg(DST_EXPONENT);

          #ifdef LANE_GATING
          #else
            );
          EXECUTE_IN_ALL(
          #endif
#else // TREAT_DENORMALS
     assert(0 && "TODO: implement CONTINUE_PRED checks");
            /* Although this does NOT treat denormals, this is a very nice
                way for Re-normalization.
              Re-normalization, first stage:
                discard F32_MANTISSA_BITS_I16 bits and reassemble into a single register;
                we're left with a number in 2.10 format
                (2 bits for the integer part)
        Kindda-Useless-TODO: think better if what he (Lucian) does is really GOOD - the re-normalization should depend also on the value of the exponent.
               Note: The exponent can hold only values 0..31 .
               Both mantissas hold invariably 10+1 bits
                 (except maybe in case of denormals!!!!).

              VERY IMPORTANT:
               If we don't have denormals, we multiply the mantissas, which
                have both a total of 11 bits, the
                result having at most 2 * (F32_MANTISSA_BITS_I16 + 1) = 22 useful
                   bits.
                 More exactly, the result of multiplication can have:
                  - as low as 21 bits:
                      (2^10) * (2^10)         = 0001 0000 0000 0000 0000 0000
                  - as high as 22 bits:
                      (2^11 - 1) * (2^11 - 1) = 0011 1111 1111 0000 0000 0001

                  More exactly:
                      - we SHR by F32_MANTISSA_BITS_I16 (both low _L and _H
                                obtained from MUL) and put the result
                                DST_MANTISSA_L.
                      - we SHR by 1 more position if the 11th bit is 1 .
                      (and also do some rounding before[!!!!Kindda-Useless-TODO]).

            Kindda-Useless-TODO: think better these conditions
                  But if the exponent was e.g. 15 this is not feasible...
                  If the exponent was < -5 then this is OK.
            */
            R(DST_MANTISSA_L) >>= F32_MANTISSA_BITS_I16; // MEGA TODO check
            //
            R(DST_MANTISSA_H) <<= 16 - F32_MANTISSA_BITS_I16; // MEGA TODO check // ((2 * (F32_MANTISSA_BITS + 1)) - 16);
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
            R(AUX) = R(DST_MANTISSA_L) >> (F32_MANTISSA_BITS_I16 + 1); // MEGA-TODO
          PrintDebugReg(AUX);
            R(DST_MANTISSA_L) >>= R(AUX);
          PrintDebugReg(DST_MANTISSA_L);
            //
            R(DST_EXPONENT) += R(AUX);
#endif // TREAT_DENORMALS
//#endif // #if 0 // 2021_04_18



        /* I prefer doing here correction of negative exponent because:
              - I don't increase the exponent below
              - it's OK to SHR mantissa even for 31 positions
                - we do a few Connex SHR operations
           Note that the smallest exponent is ~ -31 (= 1 + 1 - 15 - 18?) or so.
           Note that this handles also underflows.
        */
// I assume the max we move is 31 + 1 (31 for the minimum exponent -31).
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
          PrintDebugReg(AUX);
            R(PRED3) = R(AUX2) < R(AUX);
          //PrintDebugReg(PRED3);
          #ifdef LANE_GATING
            NOP;
           );
          EXECUTE_WHERE_LT(
          #else
            R(CONTINUE_PRED) = R(PRED3) & R(CONTINUE);
            R(CONTINUE_PRED) = R(CONTINUE_PRED) == R(CT1);
            NOP;
           );
          EXECUTE_WHERE_EQ(
          #endif
            //R(DST_MANTISSA_L) = 0;
          #ifdef ROUND_TO_NEAREST
           PrintDebugMessage("ROUND_TO_NEAREST2:\n");
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
           PrintDebugReg(DISCARDED_BITS);
            R(NUM_DISCARDED_BITS) = R(DISCARDED_BITS) == R(CT0);
            R(NUM_DISCARDED_BITS) = R(CT1) - R(NUM_DISCARDED_BITS);
           PrintDebugReg(NUM_DISCARDED_BITS);
           PrintDebugReg(DST_MANTISSA_L);
            R(DISCARDED_BITS) = R(DST_MANTISSA_L);
            R(DISCARDED_BITS) |= R(NUM_DISCARDED_BITS);
           PrintDebugReg(DISCARDED_BITS);
            R(NUM_DISCARDED_BITS) = 16;
           #else
            R(DISCARDED_BITS) = R(DST_MANTISSA_L);
            R(NUM_DISCARDED_BITS) = 16;
           #endif
          #endif

            // We "SHR" by 16 bits the mantissa
            R(DST_MANTISSA_L) = R(DST_MANTISSA_H);
            // 2021_04_24: R(DST_MANTISSA_H) = 0;
            R(DST_MANTISSA_H) = R(DST_MANTISSA_H2);
          PrintDebugReg(AUX);
            R(AUX) -= R(CT16);
          PrintDebugReg(AUX);
            R(DST_EXPONENT) += R(CT16);
        );
        EXECUTE_IN_ALL(
          PrintDebugReg(DST_MANTISSA_L);
          PrintDebugReg(DST_MANTISSA_H);
          PrintDebugReg(DST_MANTISSA_H2);
          PrintDebugReg(DST_EXPONENT);
          PrintDebugReg(AUX);

        /* IMPORTANT: Again, if we need to SHR more than 15 positions,
             first we do 16 and then the rest.
             Note: AUX2 = 15.
        */
            R(PRED3) = R(AUX2) < R(AUX);
          //PrintDebugReg(PRED3);
          #ifdef LANE_GATING
            NOP;
           );
          EXECUTE_WHERE_LT(
          #else
            R(CONTINUE_PRED) = R(PRED3) & R(CONTINUE);
            R(CONTINUE_PRED) = R(CONTINUE_PRED) == R(CT1);
            NOP;
           );
          EXECUTE_WHERE_EQ(
          #endif

          #ifdef ROUND_TO_NEAREST
           PrintDebugMessage("ROUND_TO_NEAREST3:\n");
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
           PrintDebugReg(PRED3);
           PrintDebugReg(CONTINUE);
           PrintDebugReg(DISCARDED_BITS);
           PrintDebugReg(NUM_DISCARDED_BITS);
            R(NUM_DISCARDED_BITS) = R(DISCARDED_BITS) == R(CT0);
            R(NUM_DISCARDED_BITS) = R(CT1) - R(NUM_DISCARDED_BITS);
           PrintDebugReg(NUM_DISCARDED_BITS);
           PrintDebugReg(DST_MANTISSA_L);
            R(DISCARDED_BITS) = R(DST_MANTISSA_L);
            R(DISCARDED_BITS) |= R(NUM_DISCARDED_BITS);
           PrintDebugReg(DISCARDED_BITS);
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
          PrintDebugReg(DST_MANTISSA_L);
          PrintDebugReg(DST_MANTISSA_H);
          PrintDebugReg(DST_MANTISSA_H2);
          PrintDebugReg(DST_EXPONENT);
          PrintDebugReg(AUX);

          /*
           IMPORTANT: we complete the "correction" of negative exponent by
             bringing to 1. This handles also underflows.
          */
          PrintDebugMessage("Correcting negative exponent (#2: we make it 1 if smaller - NOTE: denormals have actually exponent 1):");
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
          PrintDebugReg(PRED3);
          #ifdef LANE_GATING
            NOP;
           );
          EXECUTE_WHERE_LT(
          #else
            R(CONTINUE_PRED) = R(PRED3) & R(CONTINUE);
            R(CONTINUE_PRED) = R(CONTINUE_PRED) == R(CT1);
            NOP;
           );
          EXECUTE_WHERE_EQ(
          #endif
            //R(AUX) = R(CT0) - R(DST_EXPONENT);
            R(AUX) = R(CT1) - R(DST_EXPONENT);
          PrintDebugReg(AUX);
          PrintDebugReg(DST_MANTISSA_L);

          #ifdef ROUND_TO_NEAREST
           PrintDebugMessage("ROUND_TO_NEAREST4:\n");
           PrintDebugReg(CONTINUE_PRED);
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
           PrintDebugReg(DISCARDED_BITS);
            R(NUM_DISCARDED_BITS) = R(DISCARDED_BITS) == R(CT0);
            R(NUM_DISCARDED_BITS) = R(CT1) - R(NUM_DISCARDED_BITS);
           PrintDebugReg(NUM_DISCARDED_BITS);
            R(DISCARDED_BITS) = R(DST_MANTISSA_L);
           PrintDebugReg(DISCARDED_BITS);

/* small-MEGA-TODO (tried the code, but doesn't work on real Connex):
In order to be able to compute G (and maybe T) we should really keep in DISCARDED_BITS some of the (previous) bits
      if R(AUX) < ROUND_NUM_ADDITIONAL_BITS
   Maybe this case is NOT really encountered for MULT.f32, which needs to discard some bits - although we can have cases with denormals with mantissas smaller e.g. than 3 bits each.
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
   PrintDebugReg(TMP);
   R(TMP) = R(AUX) < R(TMP);
   PrintDebugReg(AUX);
   PrintDebugReg(TMP);
   R(AUX) += R(TMP);
   R(DISCARDED_BITS) <<= R(TMP);
   R(AUX) += R(TMP);
   R(DISCARDED_BITS) <<= R(TMP);
*/



            R(T) = R(NUM_DISCARDED_BITS); // 2021_05_16
            // 2021_05_16 R(DISCARDED_BITS) |= R(NUM_DISCARDED_BITS); // 2021_05_03
           PrintDebugReg(DISCARDED_BITS);
           PrintDebugReg(T);

            // We store only the discarded bits to compute well T
            R(AUX2) = R(CT16) - R(AUX);
           PrintDebugReg(AUX);
           PrintDebugReg(AUX2);
            R(DISCARDED_BITS) <<= R(AUX2);
            R(DISCARDED_BITS) >>= R(AUX2);
           PrintDebugReg(DISCARDED_BITS);

            R(NUM_DISCARDED_BITS) = R(AUX);
           #else
            R(DISCARDED_BITS) = R(DST_MANTISSA_L);
            R(NUM_DISCARDED_BITS) = R(AUX);
           #endif
          #endif // ROUND_TO_NEAREST

            R(DST_MANTISSA_L) >>= R(AUX);
          PrintDebugReg(DST_MANTISSA_L);
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

          PrintDebugReg(DST_MANTISSA_L);
          PrintDebugReg(DST_MANTISSA_H);
          PrintDebugReg(DST_EXPONENT);
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
          PrintDebugReg(DST_MANTISSA_L);
          PrintDebugReg(DST_EXPONENT);


         PrintDebugMessage("Check for overflows:");
            // IMPORTANT: Check if the exponent overflows and, if so, declare infinity.
            R(PRED3) = F32_EXPONENT_BIAS * 2; //30;
            R(PRED3) = R(PRED3) < R(DST_EXPONENT);
          PrintDebugReg(PRED3);
          #ifdef LANE_GATING
            NOP;
           );
          EXECUTE_WHERE_LT(
          #else
            R(CONTINUE_PRED) = R(PRED3) & R(CONTINUE);
            R(CONTINUE_PRED) = R(CONTINUE_PRED) == R(CT1);
            NOP;
           );
          EXECUTE_WHERE_EQ(
          #endif
         #ifdef MORE_COMPLICATED
            R(DST_EXPONENT) = 0x1F; // 31
          PrintDebugReg(DST_EXPONENT);
            R(DST_MANTISSA_L) = 0;
         #else

            //R(DST) &= F32_INF_POSITIVE_I16;

            // Note: R(DST) is initialized to F32_INF_POSITIVE_I16
        PrintDebugMessage("DST close to the end:");
          PrintDebugReg(DST_SIGN);
            R(DST) |= R(DST_SIGN);
            PrintDebugReg(DST);

          #ifdef LANE_GATING
            DISABLE_CELL;
          #else
            R(CONTINUE) = 0;
          #endif
         #endif
        );
        EXECUTE_IN_ALL(




            R(AUX) = F32_MANTISSA_MASK_I16 + 1;
            //R(AUX) = R(DST_MANTISSA_L) < R(AUX);
            R(AUX) = R(DST_MANTISSA_H) < R(AUX); // 2021_05_01
            R(PRED3) = R(DST_EXPONENT) == R(CT1);
            R(PRED3) &= R(AUX);
            R(AUX) = R(DST_MANTISSA_H2) == R(CT0); // 2021_05_01
            R(PRED3) &= R(AUX); // 2021_05_01
            R(PRED3) = R(PRED3) == R(CT1);
          //PrintDebugReg(PRED3);
          #ifdef LANE_GATING
            NOP;
           );
          EXECUTE_WHERE_EQ(
          #else
            R(CONTINUE_PRED) = R(PRED3) & R(CONTINUE);
            R(CONTINUE_PRED) = R(CONTINUE_PRED) == R(CT1);
            NOP;
           );
          EXECUTE_WHERE_EQ(
          #endif
            // We correct a denormal: we make exponent 1 be 0:
         PrintDebugMessage("Correcting exponent 1 (denormal):");
            /*
               Denormal has in fact exponent 1, but when stored the denormal
                   the encoded exponent is 0.
               The standard IEEE 754 puts 0 - this informs us not to add a
                hidden bit to the mantissa when unpacking the f32.
            */
            R(DST_EXPONENT) = 0;

            // 2021_04_23
          PrintDebugReg(DST_MANTISSA_L);
          PrintDebugReg(DST_MANTISSA_H);
          PrintDebugReg(DST_MANTISSA_H2);
            R(DST_MANTISSA_H2) = R(DST_MANTISSA_H) | R(DST_MANTISSA_H);
            R(DST_MANTISSA_H) = R(DST_MANTISSA_L) | R(DST_MANTISSA_L);
        );
        EXECUTE_IN_ALL(
          PrintDebugReg(DST_MANTISSA_L);
          PrintDebugReg(DST_MANTISSA_H);
          PrintDebugReg(DST_MANTISSA_H2);
          PrintDebugReg(DST_EXPONENT);

#ifdef NO_LONGER_REQUIRED
       assert(0 && "must support CONTINUE and/or LANE_GATING");
      /* IMPORTANT: Check if the exponent underflows and if so declare it 0
          and the number normally denormal - this is useful normally when we have
          input operands denormal(s). */
// TODO TODO: check for case DST_EXPONENT == 0, DST_MANTISSA != 0 and create proper denormal with DST_EXPONENT == 1; find test for this
            R(PRED3) = R(DST_EXPONENT) < R(CT0);
          //PrintDebugReg(PRED3);
            NOP;
        )
        EXECUTE_WHERE_LT(
            //R(AUX) = R(CT0) - R(DST_EXPONENT);
            // We create a denormal with exponent 1 actually (stored 0):
            R(AUX) = R(CT1) - R(DST_EXPONENT);
         PrintDebugMessage("Correcting negative exponent:");
          PrintDebugReg(AUX);
            R(DST_MANTISSA_L) >>= R(AUX);

            R(DST_EXPONENT) = 0; // normally we should put 1, but the standard IEEE 754 puts 0
          //PrintDebugReg(DST_EXPONENT);
            //R(DST_MANTISSA_L) = 0;
        );
        EXECUTE_IN_ALL(
          PrintDebugReg(DST_MANTISSA_L);
          PrintDebugReg(DST_EXPONENT);
#endif






        #ifdef ROUND_TO_NEAREST
           PrintDebugMessage("ROUND_TO_NEAREST5:\n");

           PrintDebugReg(DST_EXPONENT);

           // 2021_05_16
            R(PRED3) = R(CT1) < R(DST_EXPONENT);
            NOP;
          );
          EXECUTE_WHERE_LT(
           PrintDebugReg(DST_MANTISSA_L);
            R(DISCARDED_BITS) = R(DST_MANTISSA_L);
           PrintDebugReg(DISCARDED_BITS);
            R(NUM_DISCARDED_BITS) = 16;
           PrintDebugReg(NUM_DISCARDED_BITS);
            R(DST_MANTISSA_L) = R(DST_MANTISSA_H);
           PrintDebugReg(DST_MANTISSA_L);
          );
          EXECUTE_IN_ALL(

            // Execute only for R(CONTINUE) == 1
            R(CONTINUE_PRED) = R(CONTINUE) == R(CT1);
         PrintDebugMessage("Computing RND - active lanes are:\n");
   PrintDebugReg(CONTINUE_PRED);
            NOP;
          );
          EXECUTE_WHERE_EQ(

          PrintDebugReg(DISCARDED_BITS);
          PrintDebugReg(NUM_DISCARDED_BITS);
          PrintDebugReg(DST_MANTISSA_L);
            R(L) = R(DST_MANTISSA_L) & R(CT1); // 2021_05_03
            // 2021_05_03 R(PRED3) = 0;
            // 2021_05_03 R(L) = R(PRED3) & R(CT1);
          PrintDebugMessage("Rounding to nearest (if tie to even):");
          PrintDebugReg(NUM_DISCARDED_BITS);
          PrintDebugMessage("  L = ");
          PrintDebugReg(L);

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
          //R(T) = R(DISCARDED_BITS) == R(CT0);
          //R(T) = R(CT1) - R(T);
          R(DISCARDED_BITS) = R(DISCARDED_BITS) == R(CT0);
          R(DISCARDED_BITS) = R(CT1) - R(DISCARDED_BITS);
          R(T) |= R(DISCARDED_BITS);

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
          PrintDebugReg(NUM_DISCARDED_BITS);
          R(DISCARDED_BITS) >>= R(NUM_DISCARDED_BITS);
          PrintDebugReg(DISCARDED_BITS);
            R(T) = R(DISCARDED_BITS) & R(CT1);

          R(DISCARDED_BITS) >>= 1;
          PrintDebugReg(DISCARDED_BITS);
            R(G) = R(DISCARDED_BITS) & R(CT1);
        #endif // COMPUTE_BETTER_T_BIT


          PrintDebugMessage("  L = ");
          PrintDebugReg(L);
          PrintDebugMessage("  G = ");
          PrintDebugReg(G);
          PrintDebugMessage("  T = ");
          PrintDebugReg(T);

            R(RND) = R(T) | R(L);
            R(RND) &= R(G); // & R(AUX);
          PrintDebugMessage("  G (T + L) = ");
          PrintDebugReg(RND);


);
EXECUTE_IN_ALL(
            //R(DISCARDED_BITS) = R(DST_MANTISSA_L);
            //R(NUM_DISCARDED_BITS) = 16;
          #endif // ROUND_TO_NEAREST




#ifdef NOT_WITH_INPUT_EXP31
            // We set the exponent 31 if INPUT_EXP31 == true
          PrintDebugReg(INPUT_EXP31);
            R(INPUT_EXP31) = R(INPUT_EXP31) == R(CT1);
          #ifdef LANE_GATING
            NOP;
           );
          EXECUTE_WHERE_EQ(
          #else
            R(CONTINUE_PRED) = R(INPUT_EXP31) & R(CONTINUE);
            R(CONTINUE_PRED) = R(CONTINUE_PRED) == R(CT1);
            NOP;
           );
          EXECUTE_WHERE_EQ(
          #endif
            R(DST_EXPONENT) = R(CT255);
            //R(RND) = 0;
          //PrintDebugReg(RND);
        );
        EXECUTE_IN_ALL(
#endif // NOT_WITH_INPUT_EXP31











            // Put the f32 number back together
        PrintDebugMessage("Put f32 number back together:");
            // Shift exponent in the final place
            R(AUX) = R(DST_EXPONENT) << F32_MANTISSA_BITS_I16;
      PrintDebugReg(DST_MANTISSA_H2);
      //PrintDebugReg(DST_MANTISSA_L);
      PrintDebugReg(DST_EXPONENT);
      PrintDebugReg(AUX);
        CELL_SHR(R(AUX), R(CT1));
        NOP; // It is required
        R(AUX) = SHIFT_REG;










// small-TODO: it's nicer to put result 0 if we have 0 * negative value
          #ifdef LANE_GATING
            NOP;
           );
          EXECUTE_WHERE_EQ(
          #else
        PrintDebugMessage("Assembling DST (first):");
        PrintDebugReg(DST);
        PrintDebugReg(CONTINUE);
            R(CONTINUE_PRED) = R(CONTINUE) == R(CT1);
            NOP;
           );
          EXECUTE_WHERE_EQ(
          #endif





            /* VERY IMPORTANT: We don't add sign bit now because we want to
                    optimize rounding:
            R(DST) |= R(DST_SIGN); */
            // Get rid of hidden bit of mantissa, which is always 1
            //R(DST_MANTISSA_L) &= R(MANTISSA_MASK);
// MEGA-TODO: CELLSHL(DST, CT1)            R(DST_MANTISSA_H) &= R(MANTISSA_MASK);
            // Add mantissa:
            //R(DST) |= R(DST_MANTISSA_L);
            // 2021_04_20: R(DST) = R(DST_MANTISSA_L) | R(DST_MANTISSA_L);
            R(DST) = R(DST_MANTISSA_H) | R(DST_MANTISSA_H);
      //PrintDebugReg(DST_MANTISSA_L);
      PrintDebugReg(DST_MANTISSA_H);
      PrintDebugReg(DST);


        );
        EXECUTE_IN_ALL(

      // 2021_04_18
        CELL_SHR(R(DST_MANTISSA_H2), R(CT1));
        NOP; // It is required
        R(DST_MANTISSA_H2) = SHIFT_REG;

        CELL_SHR(R(CONTINUE), R(CT1));
        NOP; // It is required
        R(CONTINUE_PRED) = SHIFT_REG;

      // 2021_04_22
        CELL_SHR(R(DST_SIGN), R(CT1));
        NOP; // It is required
        R(DST_SIGN) = SHIFT_REG;


        // We handle the odd indices:
        R(IDX) = INDEX;
        R(IDXMOD2) = R(IDX) & R(CT1);
        R(IDXPRED) = R(IDXMOD2) == R(CT1);
        //R(IDXPRED) = R(IDXMOD2) == R(CT0); // 2021_04_18
        R(CONTINUE_PRED) &= R(IDXPRED);
        R(CONTINUE_PRED) = R(CONTINUE_PRED) == R(CT1);
        NOP;
      );
      EXECUTE_WHERE_EQ(
        // TODO: think if should be done here: R(DST) |= R(DST_MANTISSA_L);
        //
        PrintDebugMessage("Assembling DST (second):");
      PrintDebugReg(DST);
      PrintDebugReg(DST_MANTISSA_H2);
      PrintDebugReg(MANTISSA_MASK);
      PrintDebugReg(AUX);
        R(DST) = R(DST_MANTISSA_H2) | R(DST_MANTISSA_H2);
//        R(DST) >>= 7; // 2021_04_18 // 8; // TODO better (for denormals): We now keep only 8 bits (24 bits in total), because the other 16 bits are in the low 16-bits of the result already
      PrintDebugReg(DST);
        R(DST) &= R(MANTISSA_MASK);
        R(DST) |= R(AUX);
        R(DST) |= R(DST_SIGN);
      );
      PrintDebugReg(DST);
      PrintDebugReg(DST_SIGN);
      EXECUTE_IN_ALL(

    #ifdef ROUND_TO_NEAREST
        PrintDebugMessage("ROUND_TO_NEAREST6:\n");
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
            subtract it from the mantissa if the f32 is negative.

        Therefore the code below is not required:
            R(AUX) = R(DST_MANTISSA_L) == R(MANTISSA_MASK);
            NOP;
          );
          EXECUTE_WHERE_EQ(
            R(RND) = 0;
            R(DST_EXPONENT) += R(CT1);
            R(DST_MANTISSA_L) = 0;
            Now we pack again the mantissa:...
            PrintDebugReg(RND);
            PrintDebugReg(DST_EXPONENT);
          );
          EXECUTE_IN_ALL(

         NOTE: We can't add R(RND) before (to avoid this extra check) because:
            - this rounding needs to be handled at the very end so we can't
               round in the middle of the computation since we don't know the
               result (before rounding).
        */

      PrintDebugReg(RND);
      PrintDebugReg(DST);
            R(DST) += R(RND);
          // MEGA-TODO: use ADDC to propagate an eventual CARRY
      PrintDebugReg(DST);
     #endif // END #ifdef ROUND_TO_NEAREST

            /* Only now we can put the sign bit in R(DST) (since we can do
                rounding before and we want to optimize it).
            */
// TODO: think if we should execute it here            R(DST) |= R(DST_SIGN);

          #ifdef LANE_GATING
          ENABLE_ALL_CELLS;
          #else
         /*  );
          EXECUTE_IN_ALL(
         */
          #endif


    /*
    CELL_SHR(R(SRC1_MANTISSA_L), R(CT1)); // MEGA-TODO: DST_MANTISSA_LOW
    // It is required:
    NOP;
    R(AUX) = SHIFT_REG;

      // We zero out all the odd indices in REG_CRY:
      R(IDX) = INDEX;
      R(IDXMOD2) = R(IDX) & R(CT1);
      R(IDXPRED) = R(IDXMOD2) == R(CT1);
      NOP;
    )
    EXECUTE_WHERE_EQ(
        R(DST) = R(AUX) | R(AUX); // MEGA-TODO: DST_MANTISSA_LOW
    )
    PrintDebugReg(DST);

    EXECUTE_IN_ALL(
      CELL_SHR(R(DST), R(CT1));
      // NOP is required
      NOP;
      R(DST) = SHIFT_REG;
    */


      NOP;
      // Store result
      LS[resPtr] = R(DST);

      // End of program synchronization point; host will wait for this
      REDUCE(R0);
    )
    END_KERNEL(kernelName);
}



void FloatMulTest(ConnexMachine *connex) {
    int32_t opA[CONNEX_VECTOR_LENGTH];
    int32_t opB[CONNEX_VECTOR_LENGTH];
    int32_t resCorrect[CONNEX_VECTOR_LENGTH];
    int32_t result[CONNEX_VECTOR_LENGTH];


    srand(time(NULL));
    //srand(0);

    // Note: Connex-S is little endian

    /*
    opA[0] = 0x40400000; // F32 encoding for 3.000 is 0x40400000
    opB[0] = 0x3F800000; // F32 encoding for 1.000 is 0x3F800000
    resCorrect[0] = 0x40400000; // F32 encoding for 3.000 is 0x40400000
    */

    // Denormals
    //opA[0] = 0x380011c0; // F32 encoding for 3.05341e-05 is 0x380011c0

    opA[0] = 0x390011c0; // F32 encoding for 0.000122136 is 0x390011c0
    opB[0] = 0x0003ff07; // F32 encoding for 3.66993e-40 0x5037f707
    resCorrect[0] = 0x00000020; // F32 encoding for 4.48416e-44 is 0x00000020
    /*
    opA[0] = 0xbe38c25d; // F32 encoding for 2.85987e-13 is 0x2AA0FF07
    opB[0] = 0xaf3c86bd; // F32 encoding for 6.62103e-24 is 0x190011c0
    resCorrect[0] = 0x2e080ffa; // F32 encoding for 1.89353e-36 is 0x0421155a
    */
    opA[0] = 0x8c6c7e73; // F32 encoding for non-denormal S=1,E=0x18,F=0xec7e73
    opB[0] = 0x2b89f642; // F32 encoding for non-denormal S=0,E=0x57,F=0x89f642
    resCorrect[0] = 0x8000007f; // F32 encoding for 
    //
    opA[0] = 0x8c6cfe73; // F32 encoding for non-denormal S=1,E=0x18,F=0xecfe73
    opB[0] = 0x2b89f642; // F32 encoding for non-denormal S=0,E=0x57,F=0x89f642
    resCorrect[0] = 0x8000007f; // F32 encoding for 
    //
    opA[0] = 0x0000fe73; // F32 encoding for non-denormal S=1,E=0x18,F=0xecfe73
    opB[0] = 0x2b89f642; // F32 encoding for non-denormal S=0,E=0x57,F=0x89f642
    resCorrect[0] = 0x8000007f; // F32 encoding for 
    //
    opA[0] = 0x000011c0; // F32 encoding for 6.3675e-42 is 0x000011c0
    opB[0] = 0x0000f707; // F32 encoding for 8.86167e-41 is 0x5037f707
    resCorrect[0] = 0x00000000; // F32 encoding for 0 is 0x00000000


    opA[1] = 0x390011c0; // F32 encoding for 0.000122136 is 0x390011c0
    opB[1] = 0x0001ff07; // F32 encoding for 1.83322e-40 is 0x5037f707
    resCorrect[1] = 0x00000010; // F32 encoding for 2.24208e-44 is 0x00000010

    opA[2] = 0x390011c0; // F32 encoding for 0.000122136 is 0x390011c0
    opB[2] = 0x0000f707; // F32 encoding for 8.86167e-41 is 0x5037f707
    //resCorrect[0] = 0x00000002; // F32 encoding for 2.8026e-45 is 0x00000002
    resCorrect[2] = 0x00000008; // F32 encoding for 1.12104e-44 is 0x00000008

    opA[12] = 0x390011c0; // F32 encoding for 0.000122136 is 0x390011c0
    opB[12] = 0x00A0ff07; // F32 encoding for 1.47852e-38 is 0x00A0ff07
    resCorrect[12] = 0x00000509; // F32 encoding for 1.80627e-42 is 0x00000509

    opA[13] = 0x2AA0FF07; // F32 encoding for 2.85987e-13 is 0x2AA0FF07
    opB[13] = 0x190011c0; // F32 encoding for 6.62103e-24 is 0x190011c0
    resCorrect[13] = 0x0421155a; // F32 encoding for 1.89353e-36 is 0x0421155a


    opA[14] = GenRandF32Valid(); // F32 encoding for 2.85987e-13 is 0x2AA0FF07
    opB[14] = GenRandF32Valid(); // F32 encoding for 6.62103e-24 is 0x190011c0
    float res = *((float *)&opA[14]) *  *((float *)&opB[14]);
    resCorrect[14] = *((int *)&res); // F32 encoding for 1.89353e-36 is 0x0421155a

    //for (int idx = 15; idx <= 30; idx++) {
    for (int idx = 15; idx <= CONNEX_VECTOR_LENGTH / 2; idx++) {
        opA[idx] = GenRandF32Valid(-10, -15);
        //opA[idx] = GenRandF32Valid(-20, -25);
        //opA[idx] = GenRandF32Valid(-30, -35);
        opB[idx] = GenRandF32Valid(-10, -15);
        float res = *((float *)&opA[idx]) *  *((float *)&opB[idx]);
        resCorrect[idx] = *((int *)&res); // F32 encoding for 1.89353e-36 is 0x0421155a
    }



    // Denormals
    opA[3] = 0x000011c0; // F32 encoding for 6.3675e-42 is 0x000011c0
    opB[3] = 0x0000f707; // F32 encoding for 8.86167e-41 is 0x5037f707
    resCorrect[3] = 0x00000000; // F32 encoding for 0 is 0x00000000


    opA[4] = 0xC86511c0; // F32 encoding for -234567 is 0xC86511c0
    opB[4] = 0xD037f707; // F32 encoding for 12345678901 is 0x5037f707
    resCorrect[4] = 0x59249cbb; // F32 encoding for 2895888947085312 is 0x59249cbb
    /*
    opA[0] = 0xC86511c0; // F32 encoding for -234567 is 0xC86511c0
    opB[0] = 0xD037f707; // F32 encoding for 12345678901 is 0x5037f707
    resCorrect[0] = 0x59249cbb; // F32 encoding for 2895888947085312 is 0x59249cbb
    */

    /*
    opA[0] = 0xC86511c0; // F32 encoding for -234567 is 0xC86511c0
    opB[0] = 0xD037f707; // F32 encoding for 12345678901 is 0x5037f707
    resCorrect[0] = 0x59249cbb; // F32 encoding for 2895888947085312 is 0x59249cbb
    */

    opA[5] = 0xC86511c0; // F32 encoding for -234567 is 0xC86511c0
    opB[5] = 0x5037f707; // F32 encoding for 12345678901 is 0x5037f707
    resCorrect[5] = 0xD9249cbb; // F32 encoding for -2895888947085312 is 0xD9249cbb

    opA[6] = 0x486511c0; // F32 encoding for 234567 is 0x486511c0
    opB[6] = 0x5037f707; // F32 encoding for 12345678901 is 0x5037f707
    resCorrect[6] = 0x59249cbb; // F32 encoding for 2895888947085312 is 0x59249cbb

    opA[7] = 0x3f9e064b; // F32 encoding for 1.234567 is 0x3f9e064b
    opB[7] = 0x3f9e064b; // F32 encoding for 1.234567 is 0x3f9e064b
    resCorrect[7] = 0x3fc31789; // F32 encoding for 1.524156 is 0x3fc31789
    /*
    opA[0] = 0x40400000; // F32 encoding for 3.000 is 0x40400000
    opB[0] = 0x3F800000; // F32 encoding for 1.000 is 0x3F800000
    resCorrect[0] = 0x40400000; // F32 encoding for 3.000 is 0x40400000
    */

    opA[8] = 0x486511c0; // F32 encoding for 234567 is 0x486511c0
    opB[8] = 0x486511c0; // F32 encoding for 234567 is 0x486511c0
    resCorrect[8] = 0x514cf8c3; // F32 encoding for 55021678592 is 0x514cf8c3

    opA[9] = F32_NAN_1;
    opB[9] = F16_INF_POSITIVE; // F32 encoding for +Inf
    resCorrect[9] = F32_NAN_1;

    opA[8] = 0x40400000; // F32 encoding for 3.000 is 0x40400000
    opB[8] = 0x3F800000; // F32 encoding for 1.000 is 0x3F800000
    resCorrect[8] = 0x40400000; // F32 encoding for 3.000 is 0x40400000

    opA[10] = F32_INF_POSITIVE; // F32 encoding for Inf
    opB[10] = F32_INF_POSITIVE; // F32 encoding for Inf
    //opB[1] = F32_INF_NEGATIVE; // F32 encoding for -Inf
    resCorrect[10] = F32_INF_POSITIVE;

    opA[11] = 0x390011c0; // F32 encoding for 0.000122136 is 0x390011c0
    opB[11] = 0x000Bff07; // F32 encoding for 1.10168e-39 is 
    resCorrect[11] = 0x00000060; // F32 encoding for 1.34525e-43 is 0x00000060


//  #define NUM_VALS 31



//#define ZERO_AFTER_NUM_VALS_INPUT_TESTS
#ifdef ZERO_AFTER_NUM_VALS_INPUT_TESTS
    for (int i = NUM_VALS; i < CONNEX_VECTOR_LENGTH; i++) {
        opA[i] = 0;
        opB[i] = 0;
        resCorrect[i] = 0;
    }
#else
    //GenRandF32(opA, opB, resCorrect, NUM_VALS);
#endif




    connex->writeDataToConnex(opA, 1, 0);
    connex->writeDataToConnex(opB, 1, 1);

    Mult_f32Kernel(0, 1, 2); // This function just defines the vector kernel


#ifdef LLVM_ISEL_CODEGEN
    Kernel *kernel = connexGlobal->getKernel(kernelName);
    kernel->sdNodeVarNameRegDef[SRC1] = "nodeOpSrcCast1";
    kernel->sdNodeVarNameRegDef[SRC2] = "nodeOpSrcCast2";
    //
    // For MULT f32:
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

    connex->executeKernel(kernelName);
    connex->readReduction();

    // void *ConnexMachine::readDataFromConnex(void *buffer, unsigned vectorCount, unsigned startVectorIndex)
    connex->readDataFromConnex(result, 1, 2);

    printf("\n\n\n\n");

    printf("MUL results are:\n");

#ifdef ZERO_AFTER_NUM_VALS_INPUT_TESTS
    for (int i = 0; i < NUM_VALS; i++)
#else
    //for (int i = NUM_VALS; i < CONNEX_VECTOR_LENGTH; i++)
    for (int i = 0; i < CONNEX_VECTOR_LENGTH / 2; i++)
#endif
    {
        if (isnan_f32(result[i]) || isnan_f32(resCorrect[i])) {
            //assert(isnan_f32(result[i]) && isnan_f32(resCorrect[i]));
        }

        if (result[i] == 0x8000)
            result[i] = 0x0000;
        if (resCorrect[i] == 0x8000)
            resCorrect[i] = 0x0000;

        printf("i=%d: opA = %s, opB = %s --> res = %s (resCorrect = %s)%s\n",
                i,
                GetStringForF32(opA[i]).c_str(),
                GetStringForF32(opB[i]).c_str(),
                GetStringForF32(result[i]).c_str(),
                GetStringForF32(resCorrect[i]).c_str(),
              #ifdef ROUND_TO_NEAREST
                (resCorrect[i] == result[i]) ? "" :
              #else
                labs(resCorrect[i] - result[i]) <= 1 ? "" :
              #endif
                   ((isnan_f32(resCorrect[i]) && isnan_f32(result[i])) ?
                     "" : " (different results!)")
              );
    }
}


void Test() {
    FloatMulTest(connexGlobal);
}


