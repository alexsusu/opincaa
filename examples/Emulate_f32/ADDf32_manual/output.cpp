#include <iostream>
//#include "ConnexMachine.h"


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




// This needs to be put after the #define LLVM_ISEL_CODEGEN
#include "LibMisc.h"


using namespace std;

/* VERY IMPORTANT: if we enable this macro we define the registers in the table
 assuming Connex-S has 64 registers. Otherwise, this macro commented
 means Connex-S has 32 registers. */
//#define REG64


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
void AddSub_f32Kernel(int32_t opAPtr, int32_t opBPtr, int32_t resPtr,
                      bool isSub = false) {
    printf("AddSub_f32Kernel(): isSub = %d\n", isSub);

    kernelName = "add_or_sub.f32";

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
    //#define CT31           28
    #define CT255           28
    //
    // Note: The SRC1* registers are used to store also the final result
    // unpacked
    #define SRC1           27
    #define SRC1_MANTISSA_H    26
  #ifdef REG64
    #define SRC1_MANTISSA_L  43
  #else
    #define SRC1_MANTISSA_L  Rbit
  #endif
    #define SRC1_EXPONENT  25
    #define SRC1_SIGN      24
    //
    #define SRC2           23
    #define SRC2_MANTISSA_H    22
  #ifdef REG64
    #define SRC2_MANTISSA_L  42
  #else
    #define SRC2_MANTISSA_L  RND
  #endif
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
   #ifdef REG64
    #define L               4
    #define G               3
    #define Rbit            2
    #define T               1
    #define RND             0
    #define ROUND_NUM_ADDITIONAL_BITS 63
   #else
    #define L               5
    #define G               4
    #define Rbit            3
    #define T               2
    #define RND             1
    #define ROUND_NUM_ADDITIONAL_BITS 3
   #endif

    /* DISCARDED_BITS stores the mantissa bits: G, R and T.
        G, R and T are normally discarded by the SHR operations we perform
        (either because the result mantissa has more than F32_MANTISSA_BITS or
        because the exponent < 0).
    */
   #ifdef REG64
    #define DISCARDED_BITS  62
   #else
    //#define DISCARDED_BITS  SRC2_SIGN
    #define DISCARDED_BITS  T
   #endif
  #endif
    /* NUM_DISCARDED_BITS represents the number of bits discarded from the mantissa
        (or number of steps we performed SHR on the result mantissa computed
       originally by multiplication). */
   #ifdef REG64
    #define NUM_DISCARDED_BITS 61
   #else
    //#define NUM_DISCARDED_BITS 61
    #define NUM_DISCARDED_BITS 0
   #endif

        // Get operands and split
        EXECUTE_IN_ALL(
            R(SRC1) = LS[opAPtr]; // load 1st F32 operand
            R(SRC2) = LS[opBPtr]; // load 2nd F32 operand


            R(CONTINUE) = 1;
            R(CT1) = 1;
            R(CT0) = 0;
            R(CT16) = 16;
            // A special value for the 5-bit exponent for fp16 is 0x1F (31)
            //R(CT31) = 31;
            R(CT255) = 255;

            R(MANTISSA_MASK)  = F32_MANTISSA_MASK_I16;
            R(EXPONENT_MASK)  = F32_EXPONENT_MASK_I16;
            R(SIGN_MASK)      = F32_SIGN_MASK_I16;
            R(HIDDENBIT_MASK) = F32_HIDDENBIT_MASK_I16;

            /*
          #ifdef ROUND_TO_NEAREST
            R(DISCARDED_BITS) = 0;
            //R(NUM_DISCARDED_BITS) = -1;
          #endif
            */

            /* We initialize registers updated in WHERE blocks
                - it is NOT necessary because we use R(CONTINUE) to nest ifs
                    and treat the value returned R(DST) for all cases carefully,
                  but it's nicer.
                Note that for ISel and for Kernel::genLLVMISelManualCode()
                  it doesn't seem to matter.
            */
          #define INITIALIZE_SOME_REGISTERS_ALTHOUGH_NOT_REQUIRED
          #ifdef INITIALIZE_SOME_REGISTERS_ALTHOUGH_NOT_REQUIRED
            R(DST) = 0;
            R(DISCARDED_BITS) = 0;
            R(NUM_DISCARDED_BITS) = 0;
            R(AUX2) = 0;
            R(L) = 0;
            R(G) = 0;
            R(Rbit) = 0;
            R(T) = 0;
            R(RND) = 0;
          #endif

        /*
        // mantissa1 == 0?
        R(PRED1) = R(SRC1_MANTISSA) == R(CT0);
            // Add hidden bit for the mantissa (from bit 0, as it is initially)
            R(SRC1_MANTISSA) |= R(HIDDENBIT_MASK);
            PrintDebugReg(SRC1_MANTISSA);
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
        UnpackF32(__kernel,
                  CT0, CT1, CT255,
                  SRC1, SRC1_SIGN, SRC1_EXPONENT,
                  SRC1_MANTISSA_H, SRC1_MANTISSA_L,
                  SIGN_MASK, EXPONENT_MASK, MANTISSA_MASK,
                  HIDDENBIT_MASK,
                  PRED2, PRED2A, PRED3);
        /*
        UnpackF32(__kernel,
                    CT0, CT1, CT31,
                    SRC1, SRC1_SIGN, SRC1_EXPONENT,
                    SRC1_MANTISSA,
                    SIGN_MASK, EXPONENT_MASK, MANTISSA_MASK,
                    HIDDENBIT_MASK,
                    //F32_MANTISSA_BITS,
                    PRED1, PRED1A, PRED3);
        */

            /*
        // mantissa2 == 0
        R(PRED2) = R(SRC2_MANTISSA) == R(CT0);
            // Add hidden bit for the mantissa (from bit 0, as it is initially)
            R(SRC2_MANTISSA) |= R(HIDDENBIT_MASK);
            PrintDebugReg(SRC2_MANTISSA);
            */
        // Unpacking 2nd operand
        UnpackF32(__kernel,
                  CT0, CT1, CT255,
                  SRC2, SRC2_SIGN, SRC2_EXPONENT,
                  SRC2_MANTISSA_H, SRC2_MANTISSA_L,
                  SIGN_MASK, EXPONENT_MASK, MANTISSA_MASK,
                  HIDDENBIT_MASK,
                  PRED2, PRED2A, PRED3);
        /*
        UnpackF32(__kernel,
                    CT0, CT1, CT31,
                    SRC2, SRC2_SIGN, SRC2_EXPONENT,
                    SRC2_MANTISSA,
                    SIGN_MASK, EXPONENT_MASK, MANTISSA_MASK,
                    HIDDENBIT_MASK,
                    //F32_MANTISSA_BITS,
                    PRED2, PRED2A, PRED3);
        */



        /* Handling NaNs, underflow, infinity
             - see also comments before this function, which discuss about all
               the special cases for fp32
        */

  // We now TREAT_SPECIAL_CASES_FP
  #define NEW_TREAT_SPECIAL_CASES_FP
  #ifdef NEW_TREAT_SPECIAL_CASES_FP
        /*
        Applying logic minimization - we use Espresso.
          See solution at /home/asusu/LLVM/Tests/opincaa_standalone_~Emulate_f32/1Espresso/WithDCs/ADDf32/espresso_ADDf32_gen_with_Connex_assembly.output
        Inputs: Sgn1, E1 = exp1 == 31, M1 = mantissa1 == 0, Sgn2, E2 = exp2 == 31, M2 = mantissa2 == 0
        Note: isNAN should be always 0 or 1 - we check it first: if 0, then we check isINF == 1
        isNAN = Sgn1.E1.!Sgn2.E2 + !Sgn1.E1.Sgn2.E2 + E1.!M1 + E2.!M2
        isINF = E1 + E2
        signRes = Sgn1.E1 + Sgn2.E2
        */
       #ifdef REG64
        #define Sgn1 50
        #define E1 49
        #define M1 48
        #define Sgn2 47
        #define E2 46
        #define M2 45
        //
        #define AUX4 44
       #else
        #define Sgn1 NUM_BITS
        #define E1 PRED1
        #define M1 PRED1A
        #define Sgn2 VAL_FOR_SIZE
        #define E2 PRED2
        #define M2 PRED2A
        //
        #define AUX4 HIDDENBIT_MASK
       #endif

          R(Sgn1) = R(SRC1_SIGN) == R(SIGN_MASK);
          R(E1) = R(SRC1_EXPONENT) == R(CT255);
          R(M1) = R(SRC1_MANTISSA_H) == R(CT0);
          R(AUX) = R(CT0) == R(SRC1_MANTISSA_L);
          R(M1) &= R(AUX);

          R(Sgn2) = R(SRC2_SIGN) == R(SIGN_MASK);
    #ifdef DO_SUB
          R(Sgn2) = R(Sgn2) == R(CT0);
    #endif
          R(E2) = R(SRC2_EXPONENT) == R(CT255);
          R(M2) = R(SRC2_MANTISSA_H) == R(CT0);
          R(AUX) = R(CT0) == R(SRC2_MANTISSA_L);
          R(M2) &= R(AUX);

          R(AUX2) = R(E1) & R(E2);
          R(AUX) = R(AUX2) & R(Sgn1);
          R(AUX4) = ~R(Sgn2);
          R(AUX) &= R(AUX4);
          //
          //isNAN &= E2;
          //R(AUX) = R(AUX2);
          R(AUX4) = ~R(Sgn1);
          R(AUX4) &= R(AUX2);
          R(AUX4) &= R(Sgn2);
          //
          R(AUX) |= R(AUX4);
          //
          R(AUX2) = ~R(M2);
          R(AUX2) &= R(E2);
          R(AUX) |= R(AUX2);
          //
          R(AUX2) = ~R(M1);
          R(AUX2) &= R(E1);
          R(AUX) |= R(AUX2);
          R(PRED3) = R(AUX) == R(CT1);
          NOP;
        );
        EXECUTE_WHERE_EQ(
          R(DST) = F32_NAN_1_I16;
          R(CONTINUE) = 0;
        );
        EXECUTE_IN_ALL(
          R(AUX) = R(E1) | R(E2);
          R(PRED3) = R(AUX) & R(CONTINUE);
          R(PRED3) = R(PRED3) == R(CT1);
          NOP;
        );
        EXECUTE_WHERE_EQ(
          R(DST) = F32_INF_POSITIVE_I16;
          R(AUX) = R(Sgn1) & R(E1);
          R(AUX2) = R(Sgn2) & R(E2);
          R(AUX) |= R(AUX2);
          R(AUX) <<= 15;
          R(DST) ^= R(AUX);
          R(CONTINUE) = 0;
        );
        EXECUTE_IN_ALL(
  #else
            // We "catch" 1st opnd == NaN
            // Exponent 1st opnd == 31?
            R(PRED1A) = R(SRC1_EXPONENT) == R(CT31);
            //
            // mantissa1 != 0?
            PrintDebugMessage("PRED1:\n");
            PrintDebugReg(PRED1);
    // Unfortunately we don't reuse the registers from UnpackF32()
    R(PRED1) = R(CT0) < R(SRC1_MANTISSA);
            PrintDebugMessage("PRED1:\n");
            //R(PRED1) = R(CT1) - R(PRED1);
            //R(PRED1) = R(SRC1_MANTISSA) == R(CT0);
            //
            // 1st opnd: exponent == 31 && mantissa != 0 -> NaN
            R(PRED1) &= R(PRED1A);
            PrintDebugMessage("PRED1 (catch 1st opnd == NaN):\n");
            PrintDebugReg(PRED1);
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

            PrintDebugMessage("PRED2:\n");
    // Unfortunately we don't reuse the registers from UnpackF32()
    R(PRED2) = R(CT0) < R(SRC2_MANTISSA);
            PrintDebugMessage("PRED2:\n");

            // mantissa2 != 0?
            //R(PRED2) = R(CT1) - R(PRED2);
            //R(PRED2) = R(SRC2_MANTISSA) == R(CT0);

            // 2nd opnd: exponent == 31 && mantissa != 0 -> NaN
            R(PRED2) &= R(PRED2A);
            // Execute only for R(CONTINUE) == 1
            R(PRED2) &= R(CONTINUE);
            R(PRED2) = R(PRED2) == R(CT1);
            NOP;
        );
        EXECUTE_WHERE_EQ(
            R(DST) = R(SRC2); // We return NaN also
            R(CONTINUE) = 0;
        );

        EXECUTE_IN_ALL(
            PrintDebugMessage("CONTINUE:\n");
            PrintDebugReg(CONTINUE);

            /* IMPORTANT: since we checked already for NAN and below we
             check CONTINUE == 1, we basically check only for +/- INF now. */

            // If exp of both operands are == 31
            R(PRED1) = R(PRED1A) & R(PRED2A);
            PrintDebugReg(PRED1);
            PrintDebugReg(SRC1_SIGN);
            PrintDebugReg(SRC2_SIGN);



            R(PRED2) = R(SRC1_SIGN) ^ R(SRC2_SIGN);
            PrintDebugReg(PRED2);
            PrintDebugReg(CT0);
            // And have different signs (0x8000 is actually -32768)
            R(PRED2) = R(PRED2) < R(CT0);
            PrintDebugMessage("Have different signs:\n");
            PrintDebugReg(PRED2);

            R(PRED1) = R(PRED2) & R(PRED1);
            // Execute only for R(CONTINUE) == 1 and both mantissa == 0:
            R(PRED1) &= R(CONTINUE);
            R(PRED1) = R(PRED1) == R(CT1);

            NOP;
          );

        EXECUTE_WHERE_EQ(
          #ifdef DO_SUB
            // For SUB we have 2 cases: Inf - (-Inf) or -Inf - (Inf)
            R(DST) = R(SRC1); // NaN (exp == 31, mantissa != 0)
          #else
            // For ADD we have 2 cases: Inf + (-Inf) or -Inf + (Inf)
            R(DST) = F32_NAN; // NaN (exp == 31, mantissa != 0)
          #endif
            R(CONTINUE) = 0;
        );
        EXECUTE_IN_ALL(
            PrintDebugMessage("CONTINUE:\n");
            PrintDebugReg(CONTINUE);

            PrintDebugMessage("Check if both opnds are +/-Inf:\n");
            /* Checking if both opnds are Inf (or -Inf), so the result is
               exactly the same for ADD, while for SUB it is NAN. */
            R(PRED1) = R(PRED1A) & R(PRED2A); // if exp of both operands are == 31
            PrintDebugReg(PRED1);
            // and have the same sign
            R(PRED2) = R(SRC1_SIGN) ^ R(SRC2_SIGN);
            R(PRED2) = R(PRED2) == R(CT0);
            R(PRED1) = R(PRED2) & R(PRED1);
            // Execute only for R(CONTINUE) == 1
            R(PRED1) &= R(CONTINUE);
            R(PRED1) = R(PRED1) == R(CT1);
            // Inf or -Inf
            NOP;
          );
        EXECUTE_WHERE_EQ(
          #ifdef DO_SUB
            R(DST) = F32_NAN;
          #else
            R(DST) = R(SRC1);
          #endif

            R(CONTINUE) = 0;
        );
        EXECUTE_IN_ALL(

            PrintDebugMessage("Check if only 1st opnd is +/-Inf:\n");
            // 1st opnd is Inf (or -Inf)
            // Execute only for R(CONTINUE) == 1
            R(PRED1) = R(PRED1A) & R(CONTINUE);
            R(PRED1) = R(PRED1) == R(CT1);
            // Inf or -Inf
            NOP;
          );
        EXECUTE_WHERE_EQ(
            R(DST) = R(SRC1);
          #ifdef DO_SUB
          #endif
            R(CONTINUE) = 0;
        );
        EXECUTE_IN_ALL(

            PrintDebugMessage("Check if only 2nd opnd is +/-Inf: CONTINUE:\n");
            PrintDebugReg(CONTINUE);
            PrintDebugReg(PRED2A);
            // 2nd opnd is Inf (or -Inf)
            // Execute only for R(CONTINUE) == 1
            R(PRED1) = R(PRED2A) & R(CONTINUE);
            R(PRED1) = R(PRED1) == R(CT1);
            PrintDebugReg(PRED1);
            // Inf or -Inf
            NOP;
          );
        EXECUTE_WHERE_EQ(
            R(DST) = R(SRC2);
          #ifdef DO_SUB
            PrintDebugReg(SRC2);
            PrintDebugReg(SRC2_SIGN);
            //R(DST) ^= R(SIGN_MASK);
          #endif
          PrintDebugMessage("Check if only 2nd opnd is +/-Inf: DST:\n");
            PrintDebugReg(DST);
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

#endif // NEW_TREAT_SPECIAL_CASES_FP, etc

// Used ONLY to count the number of instructions: REDUCE(R30);

        PrintDebugMessage("CONTINUE, after treating special cases FP:\n");
        PrintDebugReg(CONTINUE);

   // END TREAT_SPECIAL_CASES_FP





            PrintDebugReg(DST);
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
    /* OLD_f16:
         We make MAX_DIFF_EXPONENTS 4 because the mantissa that we SHL has
         11 bits and 4 bits SHL brings the aligned mantissa to 15 bits
          - we do so because we want to avoid putting the mantissa on the sign bit
           of the i16 type, which would change the value of the POSITIVE mantissa
           to negative.
        Also, the result of adding (subtracting) the 2 mantissas should fit on 15
         bits of i16 and since 2 mantissas of 14 significant bits summed up
         yield a result that fits 15 significant bits we establish that:
            #define MAX_DIFF_EXPONENTS 3
        is the correct value for the parameter.

     For f32 the mantissa has 24 bits.
      The result of adding (subtracting) the 2 mantissas should fit on 31
        bits of two i16s and since 2 mantissas of 30 significant bits summed up
        yield a result that fits on 31 significant bits we establish that:
            // 30 - 24 = 6
            #define MAX_DIFF_EXPONENTS 6
    */
    //#define MAX_DIFF_EXPONENTS 3
    #define MAX_DIFF_EXPONENTS 6
    #define MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP 32000
    //#define DELTA_EXP AUX2
    #define DELTA_EXP PRED2A

            R(DELTA_EXP) = R(SRC1_EXPONENT) - R(SRC2_EXPONENT);
            PrintDebugReg(DELTA_EXP);


            // Cases 4: -MAX_DIFF_EXPONENTS..-1
            // and 5: 0..MAX_DIFF_EXPONENTS
            // If DELTA_EXP in range (-inf, -MAX_DIFF_EXPONENTS) U (MAX_DIFF_EXPONENTS, +inf):
            R(AUX) = -MAX_DIFF_EXPONENTS;
            R(PRED2) = R(DELTA_EXP) < R(AUX);
          PrintDebugReg(PRED2);
            R(AUX) = MAX_DIFF_EXPONENTS;
            R(PRED3) = R(AUX) < R(DELTA_EXP);
          PrintDebugReg(PRED3);
            R(PRED3) |= R(PRED2);
          PrintDebugReg(PRED3);
            //
            R(PRED3) &= R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
          PrintDebugMessage("Keeping precision of mantissas:\n");
          PrintDebugReg(DELTA_EXP);
          PrintDebugReg(PRED3);
            NOP;
        )
        EXECUTE_WHERE_EQ(

            // Increasing the precision of the operation (keeping more bits -
            //   something done also by x86)
            R(AUX) = ISHR(R(SRC1_MANTISSA_L), 10);
            R(SRC1_MANTISSA_L) = ISHL(R(SRC1_MANTISSA_L), 6);
          //PrintDebugReg(SRC1_MANTISSA_L);
            R(SRC1_MANTISSA_H) = ISHL(R(SRC1_MANTISSA_H), 6);
            R(SRC1_MANTISSA_H) |= R(AUX);
          PrintDebugReg(SRC1_MANTISSA_L);
          PrintDebugReg(SRC1_MANTISSA_H);
            R(AUX) = 6;
            R(SRC1_EXPONENT) -= R(AUX);
            R(SRC2_EXPONENT) -= R(AUX);
            //
            R(AUX) = ISHR(R(SRC2_MANTISSA_L), 10);
            R(SRC2_MANTISSA_L) = ISHL(R(SRC2_MANTISSA_L), 6);
          //PrintDebugReg(SRC2_MANTISSA_L);
            R(SRC2_MANTISSA_H) = ISHL(R(SRC2_MANTISSA_H), 6);
            R(SRC2_MANTISSA_H) |= R(AUX);
          PrintDebugReg(SRC2_MANTISSA_L);
          PrintDebugReg(SRC2_MANTISSA_H);
        )
        EXECUTE_IN_ALL(

            // In case DELTA_EXP is in range -inf..-32
            R(AUX) = -31; //-15;
            R(PRED3) = R(DELTA_EXP) < R(AUX);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) &= R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
          PrintDebugReg(PRED3);
            NOP;
        )
        EXECUTE_WHERE_EQ(
          PrintDebugMessage("Exclusive Case 1: Bringing opnd1 to larger exponent of opnd2 - case special:\n");
            /* Shift mantissa of larger exponent with difference of exponents.
             * We do SHR.
             */
            R(DELTA_EXP) = R(CT0) - R(DELTA_EXP);
          PrintDebugReg(SRC1_MANTISSA_L);

          #ifdef F16
            R(SRC1_MANTISSA) = 0;
          #else
            R(SRC1_MANTISSA_L) = 0;
            R(SRC1_MANTISSA_H) = 0;
          #endif

          PrintDebugReg(SRC1_MANTISSA_L);
          PrintDebugReg(SRC1_MANTISSA_H);
            // Adjust exponent accordingly; now we're radix-aligned
            R(SRC1_EXPONENT) = R(SRC2_EXPONENT);
          PrintDebugReg(SRC1_EXPONENT);

            R(DELTA_EXP) = MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP;
        )
        EXECUTE_IN_ALL(

            /*
            // Note that R(DELTA_EXP) can have values between -255..255.
            // OLD_f16: Note that R(DELTA_EXP) can have values between -31..31.
            If DELTA_EXP = E1 - E2 in range -31..-16
                // This means E1 < E2.
                We bring opnd1 to largest exponent, E2
                R(DELTA_EXP) = MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP;
                  We make directly R(SRC1_MANTISSA) = 0 due to "limitation"
                    of Connex-S processor for SHR val, 16+ (TODO-REMEMBER).
            */
            R(AUX) = -15;
            R(PRED3) = R(DELTA_EXP) < R(AUX);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) &= R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
          PrintDebugReg(PRED3);
            NOP;
        )
        EXECUTE_WHERE_EQ(
          PrintDebugMessage("Exclusive Case 2: Bringing opnd1 to larger exponent of opnd2 - case special:\n");
            /* Shift mantissa of larger exponent with difference of exponents.
             * We do SHR.
             */
          #ifdef F16
            R(SRC1_MANTISSA) >>= R(DELTA_EXP);
          #else
            R(DELTA_EXP) = R(CT0) - R(DELTA_EXP);
            //R(DELTA_EXP) = R(DELTA_EXP) - R(CT16);
          PrintDebugReg(DELTA_EXP);
            R(DELTA_EXP) -= R(CT16);
          PrintDebugReg(DELTA_EXP);

          #ifdef ROUND_TO_NEAREST
            R(DISCARDED_BITS) = R(SRC1_MANTISSA_L);
            R(NUM_DISCARDED_BITS) = 16;
            PrintDebugReg(DISCARDED_BITS);
            PrintDebugReg(NUM_DISCARDED_BITS);
          #endif


            R(SRC1_MANTISSA_L) = R(SRC1_MANTISSA_H);
            R(SRC1_MANTISSA_H) = 0;
            R(SRC1_MANTISSA_L) >>= R(DELTA_EXP);
          PrintDebugReg(SRC1_MANTISSA_L);
          PrintDebugReg(SRC1_MANTISSA_H);
          #endif

            // Adjust exponent accordingly; now we're radix-aligned
            R(SRC1_EXPONENT) = R(SRC2_EXPONENT);
          PrintDebugReg(SRC1_EXPONENT);

            R(DELTA_EXP) = MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP;
        )
        EXECUTE_IN_ALL(

            /*
            // Note that R(DELTA_EXP) can have values between -255..255
            // OLD: Note that R(DELTA_EXP) can have values between -31..31
            If DELTA_EXP = E1 - E2 in range -15..-(MAX_DIFF_EXPONENTS + 1):
                // This means E1 < E2.
                We bring opnd1 to largest exponent, E2
                R(DELTA_EXP) = MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP;
            */
            R(AUX) = -MAX_DIFF_EXPONENTS;
            R(PRED3) = R(DELTA_EXP) < R(AUX);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) &= R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
          PrintDebugReg(PRED3);
            NOP;
        )
        EXECUTE_WHERE_EQ(
          PrintDebugMessage("Exclusive Case 3: Bringing opnd1 to larger exponent of opnd2:\n");
            /* Shift mantissa of larger exponent with difference of exponents.
             * We do SHR.
             */
            R(DELTA_EXP) = R(CT0) - R(DELTA_EXP);

          PrintDebugReg(SRC1_MANTISSA_L);
          #ifdef F16
            R(SRC1_MANTISSA) >>= R(DELTA_EXP);
          #else
            R(AUX) = R(CT16) - R(DELTA_EXP);

           #ifdef ROUND_TO_NEAREST
            R(DISCARDED_BITS) = R(SRC1_MANTISSA_L);

            R(DISCARDED_BITS) <<= R(AUX); // NOT initialized
            R(DISCARDED_BITS) >>= R(AUX);

            R(NUM_DISCARDED_BITS) = R(DELTA_EXP);
            PrintDebugReg(DISCARDED_BITS);
            PrintDebugReg(NUM_DISCARDED_BITS);
           #endif

            PrintDebugReg(DELTA_EXP);

            R(SRC1_MANTISSA_L) >>= R(DELTA_EXP);
            R(AUX) = R(SRC1_MANTISSA_H) << R(AUX);
            R(SRC1_MANTISSA_L) |= R(AUX);
            R(SRC1_MANTISSA_H) >>= R(DELTA_EXP);
          #endif

          PrintDebugReg(SRC1_MANTISSA_L);
          PrintDebugReg(SRC1_MANTISSA_H);

            // Adjust exponent accordingly; now we're radix-aligned
            R(SRC1_EXPONENT) = R(SRC2_EXPONENT);
            PrintDebugReg(SRC1_EXPONENT);

            R(DELTA_EXP) = MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP;
        )
        EXECUTE_IN_ALL(

            // We do not care about R(CONTINUE), because we change only R(DISCARDED_BITS)
            // NOTE: discarded bits were created in Case 2 or 3
            R(PRED3) = R(SRC1_SIGN) == R(SIGN_MASK);
            NOP;
        )
        EXECUTE_WHERE_EQ(
            // We complement R(DISCARDED_BITS)
            R(DISCARDED_BITS) = R(CT0) - R(DISCARDED_BITS);
            PrintDebugReg(DISCARDED_BITS);
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
            R(PRED3) &= R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
            NOP;
        )
        EXECUTE_WHERE_EQ(
          PrintDebugMessage("Exclusive Case 4: Bringing opnd2 to smaller exponent of opnd1:\n");
            /* Shift mantissa of larger exponent with difference of exponents.
             * We do SHL (and not SHR) because we want to be more accurate.
             */
          #ifdef F16
            R(DELTA_EXP) = R(CT0) - R(DELTA_EXP);
            R(SRC2_MANTISSA) <<= R(DELTA_EXP);
          #else
            R(DELTA_EXP) = R(CT0) - R(DELTA_EXP);
          PrintDebugReg(PRED3);
          PrintDebugReg(SRC2_MANTISSA_L);
          PrintDebugReg(SRC2_MANTISSA_H);
            R(AUX) = R(CT16) - R(DELTA_EXP);
            R(AUX) = R(SRC2_MANTISSA_L) >> R(AUX);
            R(SRC2_MANTISSA_L) <<= R(DELTA_EXP);
            R(SRC2_MANTISSA_H) <<= R(DELTA_EXP);
            R(SRC2_MANTISSA_H) |= R(AUX);
          #endif

            // Adjust exponent accordingly; now we're radix-aligned
            R(SRC2_EXPONENT) = R(SRC1_EXPONENT);
            PrintDebugReg(SRC2_MANTISSA_L);
            PrintDebugReg(SRC2_EXPONENT);

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
            R(PRED3) &= R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
          PrintDebugReg(PRED3);
            NOP;
        )
        EXECUTE_WHERE_EQ(
          PrintDebugMessage("Exclusive Case 5: Bringing opnd1 to smaller exponent of opnd2:\n");
            /* Shift mantissa of larger exponent with difference of exponents.
             * We do SHL (and not SHR) because we want to be more accurate.
             */
          PrintDebugReg(DELTA_EXP);
          PrintDebugReg(SRC1_MANTISSA_L);
          PrintDebugReg(SRC1_MANTISSA_H);
          PrintDebugReg(SRC1_EXPONENT);
          #ifdef F16
            R(SRC1_MANTISSA) <<= R(DELTA_EXP);
          #else
            R(AUX) = R(CT16) - R(DELTA_EXP);
          PrintDebugReg(AUX);
            R(AUX) = R(SRC1_MANTISSA_L) >> R(AUX);
          PrintDebugReg(AUX);
            R(SRC1_MANTISSA_L) <<= R(DELTA_EXP);
            R(SRC1_MANTISSA_H) <<= R(DELTA_EXP);
            R(SRC1_MANTISSA_H) |= R(AUX);
          #endif

            // Adjust exponent accordingly; now we're radix-aligned
            R(SRC1_EXPONENT) = R(SRC2_EXPONENT);
          PrintDebugReg(SRC1_MANTISSA_L);
          PrintDebugReg(SRC1_MANTISSA_H);
          PrintDebugReg(SRC1_EXPONENT);

            R(DELTA_EXP) = MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP;
        )
        EXECUTE_IN_ALL(

            /*
            // Note again that R(DELTA_EXP) can have values between -31..31
            If DELTA_EXP = E1 - E2 in range (MAX_DIFF_EXPONENTS + 1)..15:
                // This still means E1 > E2.
                We bring opnd2 to largest exponent, E1.
            */
            R(PRED3) = R(DELTA_EXP) < R(CT16);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) &= R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
          PrintDebugReg(PRED3);
            NOP;
        )
        EXECUTE_WHERE_EQ(
          PrintDebugMessage("Exclusive Case 6: Bringing opnd2 to larger exponent of opnd1:\n");
            /* Shift mantissa of larger exponent with difference of exponents.
             * We do SHR.
             */
          PrintDebugReg(DELTA_EXP);
          PrintDebugReg(SRC2_MANTISSA_L);

          #ifdef F16
            R(SRC2_MANTISSA) >>= R(DELTA_EXP);
          #else
            R(AUX) = R(CT16) - R(DELTA_EXP);

           #ifdef ROUND_TO_NEAREST
            PrintDebugReg(AUX);

            R(DISCARDED_BITS) = R(SRC2_MANTISSA_L);

            R(DISCARDED_BITS) <<= R(AUX); // NOT initialized
            R(DISCARDED_BITS) >>= R(AUX);

            R(NUM_DISCARDED_BITS) = R(DELTA_EXP);
            PrintDebugReg(DISCARDED_BITS);
            PrintDebugReg(NUM_DISCARDED_BITS);
           #endif

            R(SRC2_MANTISSA_L) >>= R(DELTA_EXP);
            R(AUX) = R(SRC2_MANTISSA_H) << R(AUX);
            R(SRC2_MANTISSA_L) |= R(AUX);
            R(SRC2_MANTISSA_H) >>= R(DELTA_EXP);
          #endif

          PrintDebugReg(SRC2_MANTISSA_L);
          PrintDebugReg(SRC2_MANTISSA_H);

            // Adjust exponent accordingly; now we're radix-aligned
            R(SRC2_EXPONENT) = R(SRC1_EXPONENT);
            PrintDebugReg(SRC2_EXPONENT);

            R(DELTA_EXP) = MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP + 1;
        )
        EXECUTE_IN_ALL(

            // We differentiate: discarded bits were created in Cases 2, 3, (for SRC1_MANTISSA) or 6
            R(PRED2) = MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP + 1;
            R(PRED2) = R(DELTA_EXP) == R(PRED2);
          if (isSub) {
            // We do not care about R(CONTINUE), because we change only R(DISCARDED_BITS)
            R(PRED3) = R(SRC2_SIGN) == R(CT0);
          }
          else { // isSub == false
            R(PRED3) = R(SRC2_SIGN) == R(SIGN_MASK);
          }
            R(PRED3) &= R(PRED2);
            R(PRED3) = R(PRED3) == R(CT1);
          PrintDebugReg(PRED3);
            NOP;
        )
        EXECUTE_WHERE_EQ(
            R(DISCARDED_BITS) = R(CT0) - R(DISCARDED_BITS);
            PrintDebugReg(DISCARDED_BITS);
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
            R(PRED3) &= R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
          PrintDebugReg(PRED3);
            NOP;
        )
        EXECUTE_WHERE_EQ(
          PrintDebugMessage("Exclusive Case 7: Bringing opnd2 to larger exponent of opnd1 - case special:\n");
            /* Shift mantissa of larger exponent with difference of exponents.
             * We do SHR.
             */
          PrintDebugReg(DELTA_EXP);
          PrintDebugReg(SRC2_MANTISSA_L);
          PrintDebugReg(SRC2_MANTISSA_H);

          #ifdef F16
            R(SRC2_MANTISSA) >>= R(DELTA_EXP);
          #else
            R(DELTA_EXP) -= R(CT16);
          PrintDebugReg(DELTA_EXP);
            R(SRC2_MANTISSA_L) = R(SRC2_MANTISSA_H);
            R(SRC2_MANTISSA_H) = 0;
            R(SRC2_MANTISSA_L) >>= R(DELTA_EXP);
          PrintDebugReg(SRC2_MANTISSA_L);
          PrintDebugReg(SRC2_MANTISSA_H);
          #endif

            // Adjust exponent accordingly; now we're radix-aligned
            R(SRC2_EXPONENT) = R(SRC1_EXPONENT);
            PrintDebugReg(SRC2_EXPONENT);

            R(DELTA_EXP) = MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP;
        )
        EXECUTE_IN_ALL(
            /*
            // Note again that R(DELTA_EXP) can have values between -255..255.
            // OLD_f16: Note again that R(DELTA_EXP) can have values between -31..31
            If DELTA_EXP = E1 - E2 in range 32..255:
                // This still means E1 > E2.
                We bring opnd2 to largest exponent, E1.
                    We make directly R(SRC2_MANTISSA) = 0 due to "limitation"
                        of Connex processor for SHR val, 16+ (TODO-REMEMBER).
            */
            //R(AUX) = 31;
            R(AUX) = 256;
            R(PRED3) = R(DELTA_EXP) < R(AUX);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) &= R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
          PrintDebugReg(PRED3);
            NOP;
        )
        EXECUTE_WHERE_EQ(
          PrintDebugMessage("Exclusive Case 8: Bringing opnd2 to larger exponent of opnd1 - case special:\n");
            /* Shift mantissa of larger exponent by 32+ positions.
             * We do SHR.
             */
            PrintDebugReg(DELTA_EXP);
          PrintDebugReg(SRC2_MANTISSA_L);
          #ifdef F16
            R(SRC2_MANTISSA) = 0;
          #else
            R(SRC2_MANTISSA_L) = 0;
            R(SRC2_MANTISSA_H) = 0;
          #endif

          PrintDebugReg(SRC2_MANTISSA_L);
          PrintDebugReg(SRC2_MANTISSA_H);

            // Adjust exponent accordingly; now we're radix-aligned
            R(SRC2_EXPONENT) = R(SRC1_EXPONENT);
            PrintDebugReg(SRC2_EXPONENT);

            // NOT required since last check: R(DELTA_EXP) = MAX_VAL_NOT_POSSIBLE_FOR_DIFF_EXP;
        )
        EXECUTE_IN_ALL(
          PrintDebugMessage("After alignment mantissas:\n");
            PrintDebugReg(SRC1_MANTISSA_L);
            PrintDebugReg(SRC1_MANTISSA_H);
            PrintDebugReg(SRC2_MANTISSA_L);
            PrintDebugReg(SRC2_MANTISSA_H);
            PrintDebugReg(SRC1_EXPONENT);
            PrintDebugReg(SRC2_EXPONENT);

// IMPORTANT: Finished alignment of mantissas for the same exponent


            // If 1st operand is negative
            R(PRED3) = R(SRC1_SIGN) == R(SIGN_MASK);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) &= R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
            NOP;
        )
        EXECUTE_WHERE_EQ(
          PrintDebugMessage("Before complementing mantissas - note that we keep negative mantissas only to add them, and then complement them to positive s.t. CountLeadingZeros() will work:\n");
            PrintDebugReg(SRC1_MANTISSA_L);
            PrintDebugReg(SRC1_MANTISSA_H);
            PrintDebugReg(SRC2_MANTISSA_L);
            PrintDebugReg(SRC2_MANTISSA_H);
            /* Where number is negative, get two's complement of mantissa
             * (i.e., the complement w.r.t. 2^16, or 0;
             *   or neg R(SRC1_MANTISSA) + 1).
             *   TODO: Think if this introduces more error */
            R(SRC1_MANTISSA_L) = R(CT0) - R(SRC1_MANTISSA_L);
            R(SRC1_MANTISSA_H) = SUBC(R(CT0), R(SRC1_MANTISSA_H));
        )
        EXECUTE_IN_ALL(
            // If 2nd operand is negative:
          if (isSub) {
            /* If operation is actually sub.f32, we complement mantissa
               only if the number is positive.
            */
            PrintDebugReg(SRC2_SIGN);
            PrintDebugReg(SIGN_MASK);
            //R(PRED3) = R(SIGN_MASK) < R(SRC2_SIGN); // If src2 is positive
            R(PRED3) = R(SRC2_SIGN) == R(CT0); // If src2 is positive
            PrintDebugMessage("isSub==True --> PRED3 = \n");
            PrintDebugReg(PRED3);
          }
          else { // isSub == false
            R(PRED3) = R(SRC2_SIGN) == R(SIGN_MASK);
          }
            // Execute only for R(CONTINUE) == 1
            R(PRED3) &= R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
            NOP;
        )
        EXECUTE_WHERE_EQ(
            /* Where number is negative, get two's complement of mantissa
             *   (i.e., the complement w.r.t. 2^16, or 0;
             *   or neg R(SRC1_MANTISSA) + 1).
             *   TODO TODO: Think if this introduces more error
             */
            R(SRC2_MANTISSA_L) = R(CT0) - R(SRC2_MANTISSA_L);
            R(SRC2_MANTISSA_H) = SUBC(R(CT0), R(SRC2_MANTISSA_H));
        )
        EXECUTE_IN_ALL(
          PrintDebugMessage("Mantissas before summing up:\n");
            PrintDebugReg(SRC1_MANTISSA_L);
            PrintDebugReg(SRC1_MANTISSA_H);
            PrintDebugReg(SRC2_MANTISSA_L);
            PrintDebugReg(SRC2_MANTISSA_H);
            PrintDebugReg(DISCARDED_BITS);
            //
            // Execute only for R(CONTINUE) == 1
            R(PRED3) = R(CONTINUE) == R(CT1);
            NOP;
        )
        EXECUTE_WHERE_EQ(
            /* Add mantissas (IMPORTANT note: since we did complement the
             *   mantissas where the sign bit was 1 we do NOT need to have
             *   a separate case to subtract instead of add mantissas) */
            //R(PRED3) = R(CT0) + R(DISCARDED_BITS);
          PrintDebugReg(PRED3);
            //R(SRC1_MANTISSA_L) += R(SRC2_MANTISSA_H);
            R(SRC1_MANTISSA_L) += R(SRC2_MANTISSA_L);
            //R(SRC1_MANTISSA_L) = ADDC(R(SRC1_MANTISSA_L), R(SRC2_MANTISSA_L));
            R(SRC1_MANTISSA_H) = ADDC(R(SRC1_MANTISSA_H), R(SRC2_MANTISSA_H));
          );
        EXECUTE_IN_ALL(
            /* We just added the mantissas.
             * */
          PrintDebugMessage("Mantissa result after summing up:\n");
            PrintDebugReg(SRC1_MANTISSA_L);
            PrintDebugReg(SRC1_MANTISSA_H);
            PrintDebugReg(SRC1_EXPONENT);
            PrintDebugReg(SRC2_EXPONENT);
            // IMPORTANT: Sign bit of result is sign bit of mantissa at this stage
          #ifdef F16
            R(SRC1_SIGN) = R(SRC1_MANTISSA) & R(SIGN_MASK);
          #else
            R(SRC1_SIGN) = R(SRC1_MANTISSA_H) & R(SIGN_MASK);
          #endif
            R(PRED3) = R(SRC1_SIGN) == R(SIGN_MASK);
            //
            // Execute only for R(CONTINUE) == 1
            R(PRED3) &= R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
            NOP;
        )
        EXECUTE_WHERE_EQ(
            // Where mantissa is negative after addition, get absolute value
            //R(SRC1_MANTISSA) = R(SRC1_MANTISSA) - R(PRED3);
            R(SRC1_MANTISSA_L) = R(CT0) - R(SRC1_MANTISSA_L);
            R(SRC1_MANTISSA_H) = SUBC(R(CT0), R(SRC1_MANTISSA_H));
          PrintDebugReg(SRC1_MANTISSA_L);
          PrintDebugReg(SRC1_MANTISSA_H);
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
            //PrintDebugReg(PRED3);
            R(SRC1_MANTISSA) >>= R(PRED3);
            R(SRC1_EXPONENT) += R(PRED3);
        );
        EXECUTE_IN_ALL(
            PrintDebugReg(SRC1_MANTISSA);
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

            //R(PRED3) = R(SRC1_MANTISSA_H);
            // 2021_05_28
            R(PRED3) = R(SRC1_MANTISSA_H) == R(CT0);
            R(PRED3) <<= 3;
          PrintDebugReg(PRED3);
            R(VAL_FOR_SIZE) = R(SRC1_MANTISSA_L);
            R(PRED2) = R(PRED3) == R(CT0);
          PrintDebugReg(PRED2);
            NOP;
            )
          EXECUTE_WHERE_EQ(
            //R(VAL_FOR_SIZE) = R(SRC1_MANTISSA);
            R(VAL_FOR_SIZE) = R(SRC1_MANTISSA_H);
            R(PRED3) = -8;
          );
          EXECUTE_IN_ALL(

            //R(CONTINUE_BACKUP) = R(CONTINUE);
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

            PrintDebugReg(NUM_BITS);
            PrintDebugReg(PRED3);

           #ifdef F16
            R(NUM_BITS) = R(CT16) - R(NUM_BITS);
           #else
            //R(AUX) = 32;
            R(NUM_BITS) += R(PRED3);
            //R(NUM_BITS) = R(AUX) - R(NUM_BITS);
           #endif
            PrintDebugReg(NUM_BITS);
            //
            /*
            R(AUX) = F32_MANTISSA_BITS + 1;
            R(AUX) = R(NUM_BITS) - R(AUX);
            */
            //R(AUX) = R(AUX) - R(NUM_BITS);
            //R(AUX) = R(NUM_BITS);
            /* R(AUX) is the number of bits of the result mantissa in excess
                 over F32_MANTISSA_BITS + 1. */
            R(AUX) = R(CT0) - R(NUM_BITS);
                //int shrPos = numBits - (F32_MANTISSA_BITS + 1);
          PrintDebugMessage("Renormalizing mantissa by SHR by AUX bits:\n");
            PrintDebugReg(AUX);

            // if R(AUX) > 15
            R(PRED3) = 15;
            R(PRED3) = R(PRED3) < R(AUX);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) &= R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
          PrintDebugReg(PRED3);
          PrintDebugMessage("Case 1 renormalizing:\n");
            NOP;
            )
          EXECUTE_WHERE_EQ(
          #ifdef ROUND_TO_NEAREST
            R(AUX2) = R(DISCARDED_BITS) == R(CT0);
            R(AUX2) = R(CT1) - R(AUX2);

            R(DISCARDED_BITS) = R(SRC1_MANTISSA_L);
            R(DISCARDED_BITS) |= R(AUX2);

            R(AUX2) = R(DISCARDED_BITS) == R(CT0);
            R(AUX2) = R(CT1) - R(AUX2);

            R(DISCARDED_BITS) = R(SRC1_MANTISSA_H);
            R(DISCARDED_BITS) |= R(AUX2);
// MEGA5-TODO: finish DISCARDED_BITS

           #define AUX3 PRED2
            // We store only the discarded bits to compute well T
            R(AUX3) = R(CT16) - R(AUX);
            R(DISCARDED_BITS) <<= R(AUX3); // NOT initialized
            R(DISCARDED_BITS) >>= R(AUX3);

            R(NUM_DISCARDED_BITS) = R(AUX);
          PrintDebugReg(NUM_DISCARDED_BITS);
          PrintDebugReg(SRC1_MANTISSA_L);
          #endif

          #ifdef F16
            R(SRC1_MANTISSA) >>= R(AUX);
          #else
            R(SRC1_MANTISSA_H) >>= R(AUX);
            R(SRC1_MANTISSA_H) = 0;
            R(AUX2) = R(AUX) - R(CT16);
            R(SRC1_MANTISSA_L) >>= R(AUX);
          #endif
          PrintDebugReg(AUX);
          PrintDebugReg(AUX2);
          PrintDebugReg(SRC1_MANTISSA_L);
          PrintDebugReg(SRC1_MANTISSA_H);

            R(SRC1_EXPONENT) += R(AUX);
          );
          EXECUTE_IN_ALL(

            // Case R(AUX) > 0
            R(PRED3) = R(CT1) - R(PRED3);
            R(PRED2) = R(CT0) < R(AUX);
            R(PRED3) &= R(PRED2);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) &= R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
          PrintDebugReg(PRED3);
          PrintDebugMessage("Case 2 renormalizing:\n");
            NOP;
            )
          EXECUTE_WHERE_EQ(
          #ifdef ROUND_TO_NEAREST
           #ifdef F16
            R(DISCARDED_BITS) = R(SRC1_MANTISSA);
           #else
            PrintDebugReg(DISCARDED_BITS);
            R(AUX2) = R(DISCARDED_BITS) == R(CT0);
            R(AUX2) = R(CT1) - R(AUX2);
            R(DISCARDED_BITS) = R(SRC1_MANTISSA_L);
            R(DISCARDED_BITS) |= R(AUX2);
           #endif

           #undef AUX3
           #define AUX3 PRED2
            // We store only the discarded bits to compute well T
            R(AUX3) = R(CT16) - R(AUX);
            R(DISCARDED_BITS) <<= R(AUX3); // NOT initialized
            R(DISCARDED_BITS) >>= R(AUX3);

            R(NUM_DISCARDED_BITS) = R(AUX);
            PrintDebugReg(DISCARDED_BITS);
            PrintDebugReg(NUM_DISCARDED_BITS);
            PrintDebugReg(SRC1_MANTISSA_L);
          #endif

          #ifdef F16
            R(SRC1_MANTISSA) >>= R(AUX);
          #else
            R(SRC1_MANTISSA_L) >>= R(AUX);
            R(AUX2) = R(CT16) - R(AUX);
            R(AUX2) = R(SRC1_MANTISSA_H) << R(AUX2);
            R(SRC1_MANTISSA_L) |= R(AUX2);
            R(SRC1_MANTISSA_H) >>= R(AUX);

            /*
           #ifdef ROUND_TO_NEAREST
            // 2021_05_29
            R(AUX2) = R(AUX2) == R(CT0);
            R(AUX2) = R(CT1) - R(AUX2);
            R(DISCARDED_BITS) |= R(AUX2);
           #endif
           */
          #endif
          PrintDebugReg(AUX);
          PrintDebugReg(SRC1_MANTISSA_L);
          PrintDebugReg(SRC1_MANTISSA_H);

            R(SRC1_EXPONENT) += R(AUX);
          );
          EXECUTE_IN_ALL(

            R(AUX2) = -15;
            R(PRED3) = R(AUX) < R(AUX2);
            // Note: R(AUX) should be bigger than -24 (range -24..-16)
            // Execute only for R(CONTINUE) == 1
            R(PRED3) &= R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
          PrintDebugMessage("Case 3 renormalizing:\n");
          PrintDebugReg(PRED3);
            NOP;
          )
          EXECUTE_WHERE_EQ(
            R(AUX) = R(CT0) - R(AUX);
          #ifdef F16
            R(SRC1_MANTISSA) <<= R(AUX);
          #else
            R(AUX2) = R(AUX) - R(CT16);
            R(SRC1_MANTISSA_H) = R(SRC1_MANTISSA_L);
            R(SRC1_MANTISSA_L) <<= R(AUX2);
          #endif

          PrintDebugReg(AUX);
          PrintDebugReg(SRC1_MANTISSA_L);
          PrintDebugReg(SRC1_MANTISSA_H);
          PrintDebugReg(SRC2_MANTISSA_L);
          PrintDebugReg(SRC2_MANTISSA_H);
          PrintDebugReg(SRC2_EXPONENT);
            R(SRC1_EXPONENT) -= R(AUX);
          );
          EXECUTE_IN_ALL(

            R(AUX2) = -16;
            R(AUX2) = R(AUX2) < R(AUX);
            R(PRED3) = R(AUX) < R(CT0);
            // Note: R(AUX) is in range -15..-1
            // Execute only for R(CONTINUE) == 1
            R(PRED3) &= R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
          PrintDebugMessage("Case 4 renormalizing:\n");
          PrintDebugReg(PRED3);
            NOP;
          )
          EXECUTE_WHERE_EQ(
          PrintDebugReg(SRC1_MANTISSA_L);
          PrintDebugReg(SRC1_MANTISSA_H);
            R(AUX) = R(CT0) - R(AUX);

          #ifdef F16
            R(SRC1_MANTISSA) <<= R(AUX);
          #else
            R(AUX2) = R(CT16) - R(AUX);
            R(AUX2) = R(SRC1_MANTISSA_L) >> R(AUX2);
            R(SRC1_MANTISSA_L) <<= R(AUX);
            R(SRC1_MANTISSA_H) <<= R(AUX);
            R(SRC1_MANTISSA_H) |= R(AUX2);
          #endif
          PrintDebugReg(AUX);
          PrintDebugReg(SRC1_MANTISSA_L);
          PrintDebugReg(SRC1_MANTISSA_H);
          PrintDebugReg(SRC2_MANTISSA_L);
          PrintDebugReg(SRC2_MANTISSA_H);

            R(SRC1_EXPONENT) -= R(AUX);
          );
          EXECUTE_IN_ALL(
            PrintDebugMessage("Finished renormalizing mantissa by AUX bits\n");
            PrintDebugReg(AUX);
            PrintDebugReg(SRC1_MANTISSA_L);
            PrintDebugReg(SRC1_MANTISSA_H);
            PrintDebugReg(SRC1_EXPONENT);

            /*
            R(DST) = 0;
            R(SRC1_MANTISSA) = R(SRC1_MANTISSA) << 1;
            */



            // We now treat underflows
            R(PRED3) = R(SRC1_EXPONENT) < R(CT1);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) &= R(CONTINUE);
            R(PRED3) = R(PRED3) == R(CT1);
          //PrintDebugReg(PRED3);
            NOP;
        )
        EXECUTE_WHERE_EQ(
         PrintDebugMessage("Treating underflows:");

/* MEGA-TODO (tried the code, but doesn't work on real Connex):
  In order to be able to compute G (maybe T) we should really keep in DISCARDED_BITS some of the (previous) bits if R(AUX) < ROUND_NUM_ADDITIONAL_BITS
   Maybe this case is NOT really encountered for MUL.f32, which needs to discard some bits - although we can have cases with denormals with mantissas smaller e.g. than 3 bits each.
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

          PrintDebugReg(SRC1_EXPONENT);
            R(NUM_DISCARDED_BITS) = R(CT1) - R(SRC1_EXPONENT);
            //R(ROUND_AUX) = R(CT0) - R(ROUND_AUX);
            //R(SRC1_EXPONENT) = 0;
            R(SRC1_EXPONENT) = 1;
          PrintDebugReg(NUM_DISCARDED_BITS);
          PrintDebugReg(SRC1_MANTISSA_L);

            // Note: R(NUM_DISCARDED_BITS) < 16 because R(SRC1_EXPONENT) >= -1,
            //   which makes R(NUM_DISCARDED_BITS) <= 2

          #ifdef ROUND_TO_NEAREST
           #ifdef F16
            R(DISCARDED_BITS) = R(SRC1_MANTISSA);
           #else
            R(DISCARDED_BITS) = R(SRC1_MANTISSA_L);
           #endif
           #undef AUX3
           #define AUX3 PRED3
            // We store only the discarded bits to compute well T
            R(AUX3) = R(CT16) - R(NUM_DISCARDED_BITS);
            R(DISCARDED_BITS) <<= R(AUX3);
            R(DISCARDED_BITS) >>= R(AUX3);

            //R(NUM_DISCARDED_BITS) = R(AUX);
          #endif

            PrintDebugReg(NUM_DISCARDED_BITS);
            PrintDebugReg(DISCARDED_BITS);
          #ifdef F16
            R(SRC1_MANTISSA) >>= R(NUM_DISCARDED_BITS);
          #else
            R(SRC1_MANTISSA_L) >>= R(NUM_DISCARDED_BITS);
            R(AUX) = R(CT16) - R(NUM_DISCARDED_BITS);
            R(AUX) = R(SRC1_MANTISSA_H) << R(AUX);
            R(SRC1_MANTISSA_L) |= R(AUX);
            R(SRC1_MANTISSA_H) >>= R(NUM_DISCARDED_BITS);

            #ifdef ROUND_TO_NEAREST
             // 2021_05_29
             R(AUX) = R(AUX) == R(CT0);
             R(AUX) = R(CT1) - R(AUX);
             R(DISCARDED_BITS) |= R(AUX);
            #endif
           #endif
          PrintDebugReg(SRC1_MANTISSA_L);
          PrintDebugReg(SRC1_MANTISSA_H);
          PrintDebugReg(SIGN_MASK);
         );
        EXECUTE_IN_ALL(
          #ifdef F16
            R(AUX) = F16_MANTISSA_MASK + 1;
          #else
            R(AUX) = F32_MANTISSA_MASK_I16 + 1;
            R(AUX) = R(SRC1_MANTISSA_H) < R(AUX);
            R(PRED3) = R(SRC1_EXPONENT) == R(CT1);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) &= R(CONTINUE);
            R(PRED3) &= R(AUX);
            R(PRED3) = R(PRED3) == R(CT1);
          #endif
          //PrintDebugReg(PRED3);
            NOP;
        )
        EXECUTE_WHERE_EQ(
            // We correct a denormal: we make exponent 1 be 0:
            PrintDebugMessage("Correcting exponent 1 (denormal):");
            /* The standard IEEE 754 puts 0 - this informs us not to add a
                hidden bit to the mantissa when unpacking the f32.
            */
            R(SRC1_EXPONENT) = 0;
        );
        EXECUTE_IN_ALL(

            /* Get rid of hidden bit, which is always 1
               (since SRC1_MANTISSA contains the result mantissa).
    This doesn't require to use CONTINUE - it has no bad effect.
             */
            //R(SRC1_MANTISSA) &= R(MANTISSA_MASK);
           PrintDebugReg(SRC1_MANTISSA_H);
            R(SRC1_MANTISSA_H) &= R(MANTISSA_MASK); // MEGA2-TODO: not necessary to use also R(SRC1_MANTISSA_H)
           PrintDebugReg(SRC1_MANTISSA_H);

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

            // 2021_05_28
            CELL_SHR(R(SRC1_SIGN), R(CT1));
            NOP; // It is required
            R(SRC1_SIGN) = SHIFT_REG;
           PrintDebugReg(SRC1_SIGN);

            // 2021_05_28
            CELL_SHR(R(SRC1_EXPONENT), R(CT1));
            NOP; // It is required
            R(SRC1_EXPONENT) = SHIFT_REG;
           PrintDebugReg(SRC1_EXPONENT);

            // 2021_05_28
           PrintDebugReg(SRC1_MANTISSA_H);
            CELL_SHR(R(SRC1_MANTISSA_H), R(CT1));
            NOP; // It is required
            R(SRC1_MANTISSA_H) = SHIFT_REG;
           PrintDebugReg(SRC1_MANTISSA_H);

            // 2021_05_28
            #define CONTINUE2 PRED2
            CELL_SHR(R(CONTINUE), R(CT1));
            NOP; // It is required
            R(CONTINUE2) = SHIFT_REG;

            // We handle the odd indices:
            #define IDXODD PRED1A
            R(IDXODD) = INDEX;
            R(IDXODD) = R(IDXODD) & R(CT1);
            R(IDXODD) = R(IDXODD) == R(CT1);
            //R(PRED3) = 30;
            R(PRED3) = 255;
            R(PRED3) = R(PRED3) < R(SRC1_EXPONENT);
            // Execute only for R(CONTINUE) == 1
            R(PRED3) &= R(CONTINUE2);
            R(PRED3) &= R(IDXODD);
            R(PRED3) = R(PRED3) == R(CT1);
            NOP;
          );
          EXECUTE_WHERE_EQ(
              PrintDebugReg(CONTINUE2);
              //R(CONTINUE) = 0;
              R(CONTINUE2) = -1;

              R(DST) = F32_INF_POSITIVE_I16;
              // Put in result sign bit
              R(DST) |= R(SRC1_SIGN);
          );
        EXECUTE_IN_ALL(
            CELL_SHL(R(CONTINUE2), R(CT1));
            NOP; // It is required
            R(PRED2A) = SHIFT_REG;
            //
            R(AUX) = -1;
            R(AUX) = R(PRED2A) == R(AUX);
            NOP;
          );
          EXECUTE_WHERE_EQ(
              R(DST) = 0;
              R(CONTINUE) = 0;
          );

        EXECUTE_IN_ALL(
            // Execute only for R(CONTINUE) == 1
            //R(AUX) = R(CONTINUE) == R(CT1);
            R(AUX) = R(CONTINUE2) == R(CT1);
            R(AUX) &= R(IDXODD);
            R(AUX) = R(AUX) == R(CT1);
          PrintDebugMessage("Computing RND - active lanes are:\n");
          PrintDebugReg(AUX);
            NOP;
          );
          EXECUTE_WHERE_EQ(
            // Put f32 result back together in R(DST)
            PrintDebugMessage("Starting to pack the result:\n");
            // Shift the exponent in place
          #ifdef F16
            R(DST) = R(SRC1_EXPONENT) << F16_MANTISSA_BITS;
          #else
            R(DST) = R(SRC1_EXPONENT) << F32_MANTISSA_BITS_I16;
          #endif
          PrintDebugReg(SRC1_MANTISSA_H);
          PrintDebugReg(SRC1_EXPONENT);
          PrintDebugReg(DST);
            /* NOT HERE since we do rounding below: Put in result sign bit
            R(DST) = R(DST) | R(SRC1_SIGN); */
            // Put in result mantissa
            R(DST) |= R(SRC1_MANTISSA_H); // MEGA2-TODO: use also R(SRC1_MANTISSA_L)
          PrintDebugReg(DST);

            R(CONTINUE2) = -2;


            // Put in result sign bit
            R(DST) |= R(SRC1_SIGN);
        )
        EXECUTE_IN_ALL(

            CELL_SHL(R(CONTINUE2), R(CT1));
            NOP; // It is required
            R(PRED2A) = SHIFT_REG;
           PrintDebugReg(PRED2A);

            R(AUX) = -2;
            R(AUX) = R(PRED2A) == R(AUX);
           PrintDebugReg(AUX);
            NOP;
          );
          EXECUTE_WHERE_EQ(

            R(DST) = R(SRC1_MANTISSA_L);
           PrintDebugReg(SRC1_MANTISSA_L);
           PrintDebugReg(DST);


          #ifdef ROUND_TO_NEAREST
           /*
            [Ercegovac_Digital_Arithmetic_2004, Section 8.4.3]:
                rnd = G (L + R + T)
           */
          PrintDebugReg(DISCARDED_BITS);
          PrintDebugReg(NUM_DISCARDED_BITS);
            R(L) = R(SRC1_MANTISSA_L) & R(CT1);
          PrintDebugMessage("Rounding to nearest (if tie to even):");
          PrintDebugReg(NUM_DISCARDED_BITS);
          PrintDebugMessage("  L = ");
          PrintDebugReg(L);

            // We need to compute the sticky bit, T, as OR over all the values.
            // We first compute G.
            R(AUX2) = R(NUM_DISCARDED_BITS) - R(CT1); // NOT initialized
          PrintDebugReg(AUX2);
            R(AUX) = R(CT1) << R(AUX2);
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
          PrintDebugReg(AUX);
            R(Rbit) = R(DISCARDED_BITS) & R(AUX); // NOT initialized
          PrintDebugReg(Rbit);
            //
            // We take out bit Rbit from R(DISCARDED_BITS):
            R(DISCARDED_BITS) ^= R(Rbit);
            //
            R(Rbit) = R(Rbit) == R(CT0);
            R(Rbit) = R(CT1) - R(Rbit);

            R(T) = R(DISCARDED_BITS) == R(CT0); // NOT initialized
            R(T) = R(CT1) - R(T);

          PrintDebugReg(DISCARDED_BITS);
          PrintDebugMessage("  L = ");
          PrintDebugReg(L);
          PrintDebugMessage("  G = ");
          PrintDebugReg(G);
          PrintDebugMessage("  R = ");
          PrintDebugReg(Rbit);
          PrintDebugMessage("  T = ");
          PrintDebugReg(T);

           //#define SHORTER_CODE_WORKS_IF_INITIALIZED
           #ifdef INITIALIZE_SOME_REGISTERS_ALTHOUGH_NOT_REQUIRED //SHORTER_CODE_WORKS_IF_INITIALIZED
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
           PrintDebugReg(RND);

            R(DST) += R(RND);
          // MEGA5-TODO: use ADDC to propagate an eventual CARRY
          #endif
        )
        EXECUTE_IN_ALL(
            // store result
            LS[resPtr] = R(DST);

            // End of program synchronization point; host will wait for this
            REDUCE(R1);
        )
    END_KERNEL(kernelName);
}



void FloatAddSubTest(ConnexMachine *connex) {
    int32_t opA[CONNEX_VECTOR_LENGTH];
    int32_t opB[CONNEX_VECTOR_LENGTH];
    int32_t resCorrect[CONNEX_VECTOR_LENGTH];
    int32_t result[CONNEX_VECTOR_LENGTH];

    //srand(time(NULL));
    srand(0);

    // Note: Connex-S is little endian

    /*
    opA[0] = 0x40400000; // F32 encoding for 3.000 is 0x40400000
    opB[0] = 0x3F800000; // F32 encoding for 1.000 is 0x3F800000
    resCorrect[0] = 0x40400000; // F32 encoding for 3.000 is 0x40400000
    */

    // Denormals
    //opA[0] = 0x380011c0; // F32 encoding for 3.05341e-05 is 0x380011c0

    opA[0] = 0x40400000; // F32 encoding for 3.000 is 0x40400000
    opB[0] = 0x3F800000; // F32 encoding for 1.000 is 0x3F800000
    resCorrect[0] = 0x40800000; // F32 encoding for 4.000 is 0x40800000
    //
    opA[0] = 0x24fdfde8;
    opB[0] = 0xae8bcc07; // F32 encoding for 
    resCorrect[0] = 0xae8bcbf7; // F32 encoding for 
    //
    opA[0] = 0x2d876d0f;
    opB[0] = 0xa4fd479b; // F32 encoding for 
    resCorrect[0] = 0x2d876cd0; // F32 encoding for 
    //
    opA[0] = 0x270bdfb9;
    opB[0] = 0xa70bd7be; // F32 encoding for 
    resCorrect[0] = 0x20ff6000; // F32 encoding for 
    //
    opA[0] = 0x29b72c31;
    opB[0] = 0x27905bfc; // F32 encoding for 
    resCorrect[0] = 0x29c031f1; // F32 encoding for 
    //
    opA[0] = 0xa6c93b14;
    opB[0] = 0x2c979ea4; // F32 encoding for 
    resCorrect[0] = 0x2c979210; // F32 encoding for 
    //
    opA[0] = 0xa877946f;
    opB[0] = 0x25cd7750; // F32 encoding for 
    resCorrect[0] = 0xa87128b4; // F32 encoding for 
    //
    opA[0] = 0x2a5324e1; // F32 encoding for (S=0,E=0x54,F=0xd324e1)
    opB[0] = 0xa6e632cc; // F32 encoding for (S=1,E=0x4d,F=0xe632cc)
    resCorrect[0] = 0x2a51587b; // F32 encoding for (S=0,E=0x54,F=0xd1587b)
    //
    opA[0] = 0x292ca183; // F32 encoding for (S=0,E=0x52,F=0xaca183)
    opB[0] = 0x2d59e601; // F32 encoding for (S=0,E=0x5a,F=0xd9e601)
    resCorrect[0] = 0x2d5a92a3; // F32 encoding for (S=0,E=0x5a,F=0xda92a3)
    //
    // IMPORTANT: x86 is better because it does subtraction on more than 24 (or 30 bits)
    //   to be extremely precise
    opA[0] = 0xa707ba05; // F32 encoding for (S=1,E=0x4e,F=0x87ba05)
    opB[0] = 0x2c16ef74; // F32 encoding for (S=0,E=0x58,F=0x96ef74)
    resCorrect[0] = 0x2c16cd85; // F32 encoding for (S=0,E=0x58,F=0x96cd85)
    /*
    We can also become as precise as e.g. x86 if we make add(subtraction) of
      mantissas NOT on 32 bits but on 40 bits (without discarding bits).

    Mantissa result after summing up:
    Print reg SRC1_MANTISSA_L: 
    R[43] (starting from index 0) = 6160 0580 1f0c 9240 ee90 ff40 9966 ea80 3683 fcc0 3fc0 c480 1e35 ba40 0237 3f40 1635 8e80 d203 5638 ef17 c29d f7fb da40 699e 4854 4cb8 4940 9855 22c0 e970 1143 ef52 d500 ca21 c100 43b3 9700 d34c 9253 b158 fb40 153a e440 acfe 7246 a08b 63ff 181e 08c0 a479 cbfa 357c f740 fa00 bf80 c745 75c0 3c05 ee00 c8fb 9e00 745f 2500 b2f9 b540 5606 a900 0787 8d03 1eda 6b40 c1d1 fe7c 70f3 6a00 09d1 2e80 9e4d 5000 e815 e380 8724 19c0 6ce2 b640 3f01 1140 b1af 46c0 896e b240 d450 5640 b8d5 83c0 f1d6 46c0 3848 0a00 7fd8 0640 afac 5100 9560 63c0 1443 8dc1 bde0 1ac0 070a 4800 a998 540e a769 6680 f668 8800 67f7 8c80 b044 5f00 82cf 1c2c 0724 15c0 8872 9740 [END]
correct: 615f. 0x25b3615fb00 is given by print("%x" % ((0x25bbdd00000 - 0x00087ba0500)))
    Print reg SRC1_MANTISSA_H: 
    R[26] (starting from index 0) = 25b3 ff0b 0c03 47f5 f594 aaaa f1cb 4f15 dd9b 0a55 fc15 e6ea 08d0 85f5 dd0d 6889 fa9c 2eea 2f4a d5b4 07a5 ad89 eded 1f54 115c a166 c74a f569 219c f40a e1da 1639 d465 14d5 093a bc09 374e 634a 04a3 e614 1e55 6209 d8a5 f7e9 fe4e 0515 f0b5 a5ab f9a3 4815 0085 2931 13f0 2e2a 01e7 9595 2660 9215 ff0c c115 04db 17b4 0323 9bf5 fe80 6e6b d125 e389 d340 b32a 3456 4bb5 3f37 7b0a 361a c434 30ba 5ef5 fff2 f0cb dc68 b5aa deb7 e0aa 399a 1256 f978 5295 03f6 e009 2178 4676 3457 a0c9 23ec 658a dc42 db75 0097 632a c642 58d5 0cdf e00a 23d5 eb0a 2e81 46d6 db0a 9915 38f8 0d36 3664 cd96 26a3 740b 2ffb a849 32ee 2209 fd4e 8436 f906 a86a d172 e00a 0e6a 02f5 [END]
    Print reg SRC1_EXPONENT: 
    R[25] (starting from index 0) = 0052 00b1 004f 00d6 0057 00f6 004c 00a7 004a 00f1 0056 00b9 004e 00f5 0057 0054 0054 00da 0057 0063 004a 008e 0054 0029 0051 009e 0050 00e1 0051 00e8 004b 0099 004b 00b9 004a 007e 004e 00d5 0058 00c9 004a 00ad 0054 00e1 0054 00a9 0056 009e 0054 0087 0051 00da 0057 00ea 0052 00a4 0055 00d5 004f 008a 0058 0080 0054 006c 005d 00d4 0055 00d9 004e 005f 004e 00f6 0051 00cd 0056 00ca 0050 00d7 005a 00f8 0051 009f 0051 00d0 0053 00d8 0054 00af 004a 00e1 004b 00ee 004a 00d6 004e 00c4 0055 00b3 0050 0075 0051 00d6 004e 00c0 004d 00f1 0054 00c6 0051 00ac 0054 00b9 0050 0023 0055 007c 004d 00d6 0055 00c2 004d 00ad 004d 0077 0057 00d9 0053 00d8 [END]

    Case 2 renormalizing:
    Print reg DISCARDED_BITS:
    R[62] (starting from index 0) = fec0 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 7a00 0000 0000 0000 0000 0000 0000 4b00 0000 95c0 0000 fc00 0000 0000 0000 0000 0000 0000 ffc0 0000 0000 0000 00c0 0000 0000 0000 0000 0000 ff80 0000 0000 9e40 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 ffc0 0000 0000 0000 0000 0000 0000 0000 0000 0000 3c40 0000 0000 b9c0 0000 0000 0000 e000 fb40 0000 0000 0000 0000 0000 0000 0000 0000 0000 a800 0000 0000 0000 0000 0000 0040 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 9500 0000 0000 0000 0000 0000 ff80 0000 1240 0000 0000 0000 0000 0000 0000 0000 0000 5a80 0840 0000 0000 0000 [END]
    Print reg DISCARDED_BITS:
--> R[62] (starting from index 0) = 0021 0000 000c 0040 0000 0040 000a 0000 003d 0000 0000 0000 0005 0040 0009 0040 0003 0000 0003 0008 0007 0063 0005 0001 001e 002d 0008 0000 0015 0000 0010 0003 002f 0000 0001 0000 0033 0000 0004 000d 0018 0040 0007 0000 0000 0007 0005 0001 0002 0040 0000 003a 001c 0000 0000 0000 0005 0040 0000 0000 0003 0000 0003 0000 0001 0040 003b 0000 0039 007d 001a 0040 0011 007d 0033 0000 0011 0000 0000 0000 002b 0000 001c 0000 0023 0000 0007 0040 0003 0000 002f 0040 0010 0040 0015 0040 002a 0000 0000 0000 0028 0040 000c 0000 0020 0000 0003 0041 0020 0040 000a 0000 0019 0032 0029 0000 0028 0000 0037 0000 0000 0000 0001 0055 001d 0000 0002 0000 [END]
correct is 001fb0
    Print reg NUM_DISCARDED_BITS:
    R[61] (starting from index 0) = 0006 0000 0004 0007 0004 0007 0004 0007 0006 0004 0002 0005 0004 0007 0006 0007 0003 0006 0006 0006 0003 0007 0005 0005 0005 0007 0006 0004 0006 0004 0005 0005 0006 0005 0004 0007 0006 0007 0003 0005 0005 0007 0006 0004 0001 0003 0004 0007 0003 0007 0000 0006 0005 0006 0001 0007 0006 0007 0000 0006 0003 0005 0002 0007 0001 0007 0006 0005 0006 0007 0006 0007 0006 0007 0006 0006 0006 0007 0000 0004 0006 0007 0006 0005 0006 0005 0003 0007 0002 0005 0006 0007 0006 0007 0006 0007 0006 0006 0000 0007 0006 0007 0004 0005 0006 0005 0006 0007 0006 0007 0006 0004 0006 0006 0006 0007 0006 0007 0006 0006 0002 0007 0003 0007 0006 0005 0004 0002 [END]
    ...
    Print reg AUX: 
    R[ 8] (starting from index 0) = 0006 0000 0004 0007 0004 0007 0004 0007 0006 0004 0002 0005 0004 0007 0006 0007 0003 0006 0006 0006 0003 0007 0005 0005 0005 0007 0006 0004 0006 0004 0005 0005 0006 0005 0004 0007 0006 0007 0003 0005 0005 0007 0006 0004 0001 0003 0004 0007 0003 0007 0000 0006 0005 0006 0001 0007 0006 0007 0000 0006 0003 0005 0002 0007 0001 0007 0006 0005 0006 0007 0006 0007 0006 0007 0006 0006 0006 0007 fffc 0004 0006 0007 0006 0005 0006 0005 0003 0007 0002 0005 0006 0007 0006 0007 0006 0007 0006 0006 0000 0007 0006 0007 0004 0005 0006 0005 0006 0007 0006 0007 0006 0004 0006 0006 0006 0007 0006 0007 0006 0006 0002 0007 0003 0007 0006 0005 0004 0002 [END]
    Print reg SRC1_MANTISSA_L: 
--> R[43] (starting from index 0) = cd85 fa80 31f0 eb24 b117 aa01 4669 2bd5 9325 5fcc b010 a9dc 01e3 148b cbf7 127e 7d39 aa3a 2b48 2ea7 bde2 ec7a 9040 a6d2 e34c 336f d6cd 6b6c 7261 5dd4 28b4 c88a 6842 aea8 aca2 ec7e 390e 952e 7a69 5b6d ad8a 13f6 6bab 61bc a981 ae48 a5f7 a938 9cfc 2a11 a479 c72f 81ab abdd fd00 d481 831d d514 c3fb a848 791f a4f0 dd17 15b6 a683 d76a 6aa7 b2b8 ffe1 aae5 587b 6ad6 df07 15fc 69c3 2e58 e827 ea5d 61b3 4b00 5c5f aa39 21e3 af32 69b3 b5b2 f81f 2a22 ac6b b5ca e225 ed64 5f51 6d53 b2e3 1507 f438 2ae5 3848 5414 f600 aa0c fafa ad78 5655 ace2 0451 ad1b d508 d5ca e01c 6480 92a6 a6af 8e9d 16cd efd9 6cf0 b99f 2632 53ef 9342 2fa6 2bc7 37e3 af52 a887 65d0 [END]
    Print reg SRC1_MANTISSA_H: 
    R[26] (starting from index 0) = 0096 00f4 00c0 008f 00a6 00aa 00e3 009e 0089 00a5 00fa 00c8 008d 00f4 008b 00d1 00ac 00bb 00bd 00a9 00f4 00a4 0090 00fa 008a 00bd 00e2 00a9 0086 00bf 00f1 00b1 00ae 00a6 0093 0087 00dd 00c6 0094 00cf 00f2 00c4 009d 0081 00d8 00a2 00f4 00b4 00cb 0090 0085 00a4 009f 00b8 00f3 00d4 0099 00db 00f3 00fb 009b 00bd 00c8 00c8 00bf 00dc 00bb 00e3 00b2 0099 00d1 0097 00fc 00f6 00d8 00ef 00c2 00bd 000d 00f3 008e 0094 0085 00fa 00e6 0092 00d0 00a5 00fd 00ff 0085 008c 00d1 00be 008f 00cb 008e 0092 0097 00c6 00e6 00b1 00cd 00ff 008f 00a7 00ba 008d 0093 00cd 00e3 00d3 00d9 00c9 009a 00e8 00bf 00af 00cb 0088 00ac 00f7 00df 00af 00ba 00ff 00e6 00bd [END]

      L =  
    Print reg L: 
    R[ 4] (starting from index 0) = 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 [END]
      G =  
    Print reg G: 
correct is 0 (since DISCARDED_BITS is actually 001fb0)
    R[ 3] (starting from index 0) = 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 [END]
      R =  
    Print reg Rbit:
    R[ 2] (starting from index 0) = 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0001 0000 0000 0000 0001 0000 0000 0000 0000 0000 0000 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 [END]
      T =  
    Print reg T: 
    R[ 1] (starting from index 0) = 0001 0000 0000 0000 0000 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0001 0000 0000 0000 0001 0000 0001 0000 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 [END]
    G (L + R + T) =  
    Print reg RND: 
    R[ 0] (starting from index 0) = 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 [END]
correct is 0
    */
    //
    // IMPORTANT: x86 is better because it does subtraction on more than 24 (or 30 bits)
    //   to be extremely precise
    opA[0] = 0x2b7e4d1e; // F32 encoding for (S=0,E=0x56,F=0xfe4d1e)
    opB[0] = 0xa712cc82; // F32 encoding for (S=1,E=0x4e,F=0x92cc82)
    resCorrect[0] = 0x2b7dba51; // F32 encoding for (S=0,E=0x56,F=0xfdba51)
    /*
    We can also become as precise as e.g. x86 if we make add(subtraction) of
      mantissas NOT on 32 bits but on 40 bits (without discarding bits).

    Mantissa result after summing up:
    Print reg SRC1_MANTISSA_L: 
    R[43] (starting from index 0) = 9460 c480 1f0c 9240 ee90 ff40 9966 ea80 3683 fcc0 3fc0 c480 1e35 ba40 0237 3f40 1635 8e80 d203 5638 ef17 c29d f7fb da40 699e 4854 4cb8 4940 9855 22c0 e970 1143 ef52 d500 ca21 c100 43b3 9700 d34c 9253 b158 fb40 153a e440 acfe 7246 a08b 63ff 181e 08c0 a479 cbfa 357c f740 fa00 bf80 c745 75c0 3c05 ee00 c8fb 9e00 745f 2500 b2f9 b540 5606 a900 0787 8d03 1eda 6b40 c1d1 fe7c 70f3 6a00 09d1 2e80 9e4d 5000 e815 e380 8724 19c0 6ce2 b640 3f01 1140 b1af 46c0 896e b240 d450 5640 b8d5 83c0 f1d6 46c0 3848 0a00 7fd8 0640 afac 5100 9560 63c0 1443 8dc1 bde0 1ac0 070a 4800 a998 540e a769 6680 f668 8800 67f7 8c80 b044 5f00 82cf 1c2c 0724 15c0 8872 2e15 [END]
correct is 945f (because print("%x" % ((0x3f93478000 - 0x0024b32080)) gives 0x3f6e945f80
    Print reg SRC1_MANTISSA_H: 
    R[26] (starting from index 0) = 3f6e ff29 0c03 47f5 f594 aaaa f1cb 4f15 dd9b 0a55 fc15 e6ea 08d0 85f5 dd0d 6889 fa9c 2eea 2f4a d5b4 07a5 ad89 eded 1f54 115c a166 c74a f569 219c f40a e1da 1639 d465 14d5 093a bc09 374e 634a 04a3 e614 1e55 6209 d8a5 f7e9 fe4e 0515 f0b5 a5ab f9a3 4815 0085 2931 13f0 2e2a 01e7 9595 2660 9215 ff0c c115 04db 17b4 0323 9bf5 fe80 6e6b d125 e389 d340 b32a 3456 4bb5 3f37 7b0a 361a c434 30ba 5ef5 fff2 f0cb dc68 b5aa deb7 e0aa 399a 1256 f978 5295 03f6 e009 2178 4676 3457 a0c9 23ec 658a dc42 db75 0097 632a c642 58d5 0cdf e00a 23d5 eb0a 2e81 46d6 db0a 9915 38f8 0d36 3664 cd96 26a3 740b 2ffb a849 32ee 2209 fd4e 8436 f906 a86a d172 e00a 0e6a ceba [END]
    Print reg SRC1_EXPONENT: 
    R[25] (starting from index 0) = 0050 00b1 004f 00d6 0057 00f6 004c 00a7 004a 00f1 0056 00b9 004e 00f5 0057 0054 0054 00da 0057 0063 004a 008e 0054 0029 0051 009e 0050 00e1 0051 00e8 004b 0099 004b 00b9 004a 007e 004e 00d5 0058 00c9 004a 00ad 0054 00e1 0054 00a9 0056 009e 0054 0087 0051 00da 0057 00ea 0052 00a4 0055 00d5 004f 008a 0058 0080 0054 006c 005d 00d4 0055 00d9 004e 005f 004e 00f6 0051 00cd 0056 00ca 0050 00d7 005a 00f8 0051 009f 0051 00d0 0053 00d8 0054 00af 004a 00e1 004b 00ee 004a 00d6 004e 00c4 0055 00b3 0050 0075 0051 00d6 004e 00c0 004d 00f1 0054 00c6 0051 00ac 0054 00b9 0050 0023 0055 007c 004d 00d6 0055 00c2 004d 00ad 004d 0077 0057 00d9 0053 0099 [END]

    Renormalizing mantissa by SHR by AUX bits:
    Print reg AUX: 
    R[ 8] (starting from index 0) = 0006 0000 0004 0007 0004 0007 0004 0007 0006 0004 0002 0005 0004 0007 0006 0007 0003 0006 0006 0006 0003 0007 0005 0005 0005 0007 0006 0004 0006 0004 0005 0005 0006 0005 0004 0007 0006 0007 0003 0005 0005 0007 0006 0004 0001 0003 0004 0007 0003 0007 0000 0006 0005 0006 0001 0007 0006 0007 0000 0006 0003 0005 0002 0007 0001 0007 0006 0005 0006 0007 0006 0007 0006 0007 0006 0006 0006 0007 fffc 0004 0006 0007 0006 0005 0006 0005 0003 0007 0002 0005 0006 0007 0006 0007 0006 0007 0006 0006 0000 0007 0006 0007 0004 0005 0006 0005 0006 0007 0006 0007 0006 0004 0006 0006 0006 0007 0006 0007 0006 0006 0002 0007 0003 0007 0006 0005 0004 0006 [END]

    Print reg PRED3: 
    R[ 9] (starting from index 0) = 0001 0000 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0000 0001 0001 0001 0001 0001 0001 0001 0000 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0000 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0000 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 0001 [END]
    Case 2 renormalizing:
    Print reg DISCARDED_BITS:
    R[62] (starting from index 0) = ff80 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 8600 0000 0000 0000 02c0 0000 0000 4b00 0000 6a40 0000 fc00 fec0 0000 0000 0000 0000 0000 0040 0000 0000 0000 00c0 0000 0000 fe80 0000 0000 0080 0000 0000 61c0 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 ffc0 0000 0000 0000 0000 0000 0000 0000 0000 0000 c3c0 0000 0000 b9c0 0000 0000 0000 e000 fb40 0000 0000 0000 0000 0000 03c0 0000 0000 0000 a800 0000 0000 0000 0000 0000 0040 0000 0000 0000 0000 0000 0000 0000 0000 0000 0300 0000 0000 0000 0000 0000 9500 0000 0000 0000 09c0 0000 ff80 0000 1240 0000 0000 0000 0840 0000 0000 0000 0000 5a80 f7c0 0000 0000 0000 [END]
    Print reg DISCARDED_BITS:
--> R[62] (starting from index 0) = 0021 0000 000c 0040 0000 0040 000a 0000 003d 0000 0000 0000 0005 0040 0009 0040 0003 0000 0003 0008 0007 0063 0005 0001 001e 002d 0009 0000 0015 0000 0010 0003 002f 0000 0001 0000 0033 0000 0004 000d 0018 0040 0007 0000 0000 0007 0005 0001 0002 0040 0000 003a 001c 0000 0000 0000 0005 0040 0000 0000 0003 0000 0003 0000 0001 0040 003b 0000 0039 007d 001a 0040 0011 007d 0033 0000 0011 0000 0000 0000 002b 0000 001c 0000 0023 0000 0007 0040 0003 0000 002f 0040 0010 0040 0015 0040 002a 0000 0000 0000 0029 0040 000c 0000 0020 0000 0003 0041 0020 0040 000b 0000 0019 0032 0029 0000 0028 0000 0037 0000 0000 0000 0001 0055 001d 0000 0002 002b [END]
correct is 001f80
    Print reg NUM_DISCARDED_BITS:
    R[61] (starting from index 0) = 0006 0000 0004 0007 0004 0007 0004 0007 0006 0004 0002 0005 0004 0007 0006 0007 0003 0006 0006 0006 0003 0007 0005 0005 0005 0007 0006 0004 0006 0004 0005 0005 0006 0005 0004 0007 0006 0007 0003 0005 0005 0007 0006 0004 0001 0003 0004 0007 0003 0007 0000 0006 0005 0006 0001 0007 0006 0007 0000 0006 0003 0005 0002 0007 0001 0007 0006 0005 0006 0007 0006 0007 0006 0007 0006 0006 0006 0007 0000 0004 0006 0007 0006 0005 0006 0005 0003 0007 0002 0005 0006 0007 0006 0007 0006 0007 0006 0006 0000 0007 0006 0007 0004 0005 0006 0005 0006 0007 0006 0007 0006 0004 0006 0006 0006 0007 0006 0007 0006 0006 0002 0007 0003 0007 0006 0005 0004 0006 [END]
    Print reg SRC1_MANTISSA_L: 
    R[43] (starting from index 0) = 9460 3b80 1f0c 9240 1170 00c0 669a ea80 c97d fcc0 c040 3b80 1e35 45c0 fdc9 3f40 e9cb 8e80 d203 a9c8 ef17 3d63 0805 da40 699e b7ac b348 b6c0 9855 dd40 1690 1143 10ae d500 ca21 3f00 43b3 9700 d34c 6dad b158 fb40 eac6 1bc0 5302 7246 5f75 9c01 e7e2 08c0 a479 cbfa 357c f740 fa00 4080 c745 8a40 c3fb 1200 c8fb 9e00 745f db00 4d07 b540 a9fa 5700 f879 72fd 1eda 6b40 c1d1 fe7c 70f3 9600 09d1 2e80 61b3 b000 17eb 1c80 78dc e640 6ce2 b640 c0ff 1140 b1af b940 896e b240 d450 a9c0 b8d5 83c0 0e2a b940 3848 0a00 8028 0640 afac af00 9560 9c40 1443 8dc1 4220 e540 070a 4800 a998 abf2 a769 6680 f668 7800 67f7 8c80 4fbc a100 7d31 e3d4 f8dc ea40 8872 d1eb [END]
    Print reg AUX: 
    R[ 8] (starting from index 0) = 0006 0000 0004 0007 0004 0007 0004 0007 0006 0004 0002 0005 0004 0007 0006 0007 0003 0006 0006 0006 0003 0007 0005 0005 0005 0007 0006 0004 0006 0004 0005 0005 0006 0005 0004 0007 0006 0007 0003 0005 0005 0007 0006 0004 0001 0003 0004 0007 0003 0007 0000 0006 0005 0006 0001 0007 0006 0007 0000 0006 0003 0005 0002 0007 0001 0007 0006 0005 0006 0007 0006 0007 0006 0007 0006 0006 0006 0007 fffc 0004 0006 0007 0006 0005 0006 0005 0003 0007 0002 0005 0006 0007 0006 0007 0006 0007 0006 0006 0000 0007 0006 0007 0004 0005 0006 0005 0006 0007 0006 0007 0006 0004 0006 0006 0006 0007 0006 0007 0006 0006 0002 0007 0003 0007 0006 0005 0004 0006 [END]
    Print reg SRC1_MANTISSA_L: 
    R[43] (starting from index 0) = ba51 3b80 31f0 eb24 b117 aa01 4669 2bd5 9325 5fcc b010 a9dc 01e3 148b cbf7 127e 7d39 aa3a 2b48 2ea7 bde2 ec7a 9040 a6d2 e34c 336f d6cd 6b6c 7261 5dd4 28b4 c88a 6842 aea8 aca2 ec7e 390e 952e 7a69 5b6d ad8a 13f6 6bab 61bc a981 ae48 a5f7 a938 9cfc 2a11 a479 c72f 81ab abdd fd00 d481 831d d514 c3fb a848 791f a4f0 dd17 15b6 a683 d76a 6aa7 b2b8 ffe1 aae5 587b 6ad6 df07 15fc 69c3 2e58 e827 ea5d 61b3 4b00 5c5f aa39 21e3 af32 69b3 b5b2 f81f 2a22 ac6b b5ca e225 ed64 5f51 6d53 b2e3 1507 f438 2ae5 3848 5414 f600 aa0c fafa ad78 5655 ace2 0451 ad1b d508 d5ca e01c 6480 92a6 a6af 8e9d 16cd efd9 6cf0 b99f 2632 53ef 9342 2fa6 2bc7 37e3 af52 a887 1747 [END]
    Print reg SRC1_MANTISSA_H: 
    R[26] (starting from index 0) = 00fd 00d6 00c0 008f 00a6 00aa 00e3 009e 0089 00a5 00fa 00c8 008d 00f4 008b 00d1 00ac 00bb 00bd 00a9 00f4 00a4 0090 00fa 008a 00bd 00e2 00a9 0086 00bf 00f1 00b1 00ae 00a6 0093 0087 00dd 00c6 0094 00cf 00f2 00c4 009d 0081 00d8 00a2 00f4 00b4 00cb 0090 0085 00a4 009f 00b8 00f3 00d4 0099 00db 00f3 00fb 009b 00bd 00c8 00c8 00bf 00dc 00bb 00e3 00b2 0099 00d1 0097 00fc 00f6 00d8 00ef 00c2 00bd 000d 00f3 008e 0094 0085 00fa 00e6 0092 00d0 00a5 00fd 00ff 0085 008c 00d1 00be 008f 00cb 008e 0092 0097 00c6 00e6 00b1 00cd 00ff 008f 00a7 00ba 008d 0093 00cd 00e3 00d3 00d9 00c9 009a 00e8 00bf 00af 00cb 0088 00ac 00f7 00df 00af 00ba 00ff 00e6 00c5 [END]

    Print reg AUX: 
    R[ 8] (starting from index 0) = 0006 0000 0004 0007 0004 0007 0004 0007 0006 0004 0002 0005 0004 0007 0006 0007 0003 0006 0006 0006 0003 0007 0005 0005 0005 0007 0006 0004 0006 0004 0005 0005 0006 0005 0004 0007 0006 0007 0003 0005 0005 0007 0006 0004 0001 0003 0004 0007 0003 0007 0000 0006 0005 0006 0001 0007 0006 0007 0000 0006 0003 0005 0002 0007 0001 0007 0006 0005 0006 0007 0006 0007 0006 0007 0006 0006 0006 0007 0004 0004 0006 0007 0006 0005 0006 0005 0003 0007 0002 0005 0006 0007 0006 0007 0006 0007 0006 0006 0000 0007 0006 0007 0004 0005 0006 0005 0006 0007 0006 0007 0006 0004 0006 0006 0006 0007 0006 0007 0006 0006 0002 0007 0003 0007 0006 0005 0004 0006 [END]
    Print reg SRC1_MANTISSA_L: 
--> R[43] (starting from index 0) = ba51 3b80 31f0 eb24 b117 aa01 4669 2bd5 9325 5fcc b010 a9dc 01e3 148b cbf7 127e 7d39 aa3a 2b48 2ea7 bde2 ec7a 9040 a6d2 e34c 336f d6cd 6b6c 7261 5dd4 28b4 c88a 6842 aea8 aca2 ec7e 390e 952e 7a69 5b6d ad8a 13f6 6bab 61bc a981 ae48 a5f7 a938 9cfc 2a11 a479 c72f 81ab abdd fd00 d481 831d d514 c3fb a848 791f a4f0 dd17 15b6 a683 d76a 6aa7 b2b8 ffe1 aae5 587b 6ad6 df07 15fc 69c3 2e58 e827 ea5d 1b30 4b00 5c5f aa39 21e3 af32 69b3 b5b2 f81f 2a22 ac6b b5ca e225 ed64 5f51 6d53 b2e3 1507 f438 2ae5 3848 5414 f600 aa0c fafa ad78 5655 ace2 0451 ad1b d508 d5ca e01c 6480 92a6 a6af 8e9d 16cd efd9 6cf0 b99f 2632 53ef 9342 2fa6 2bc7 37e3 af52 a887 1747 [END]
    Print reg SRC1_MANTISSA_H: 
    R[26] (starting from index 0) = 00fd 00d6 00c0 008f 00a6 00aa 00e3 009e 0089 00a5 00fa 00c8 008d 00f4 008b 00d1 00ac 00bb 00bd 00a9 00f4 00a4 0090 00fa 008a 00bd 00e2 00a9 0086 00bf 00f1 00b1 00ae 00a6 0093 0087 00dd 00c6 0094 00cf 00f2 00c4 009d 0081 00d8 00a2 00f4 00b4 00cb 0090 0085 00a4 009f 00b8 00f3 00d4 0099 00db 00f3 00fb 009b 00bd 00c8 00c8 00bf 00dc 00bb 00e3 00b2 0099 00d1 0097 00fc 00f6 00d8 00ef 00c2 00bd 00d6 00f3 008e 0094 0085 00fa 00e6 0092 00d0 00a5 00fd 00ff 0085 008c 00d1 00be 008f 00cb 008e 0092 0097 00c6 00e6 00b1 00cd 00ff 008f 00a7 00ba 008d 0093 00cd 00e3 00d3 00d9 00c9 009a 00e8 00bf 00af 00cb 0088 00ac 00f7 00df 00af 00ba 00ff 00e6 00c5 [END]
    Print reg SRC1_EXPONENT: 
    R[25] (starting from index 0) = 0056 00b1 0053 00dd 005b 00fd 0050 00ae 0050 00f5 0058 00be 0052 00fc 005d 005b 0057 00e0 005d 0069 004d 0095 0059 002e 0056 00a5 0056 00e5 0057 00ec 0050 009e 0051 00be 004e 0085 0054 00dc 005b 00ce 004f 00b4 005a 00e5 0055 00ac 005a 00a5 0057 008e 0051 00e0 005c 00f0 0053 00ab 005b 00dc 004f 0090 005b 0085 0056 0073 005e 00db 005b 00de 0054 0066 0054 00fd 0057 00d4 005c 00d0 0056 00de 0056 00fc 0057 00a6 0057 00d5 0059 00dd 0057 00b6 004c 00e6 0051 00f5 0050 00dd 0054 00cb 005b 00b9 0050 007c 0057 00dd 0052 00c5 0053 00f6 005a 00cd 0057 00b3 005a 00bd 0056 0029 005b 0083 0053 00dd 005b 00c8 004f 00b4 0050 007e 005d 00de 0057 009f [END]

      L =  
    Print reg L: 
    R[ 4] (starting from index 0) = 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 [END]
      G =  
    Print reg G: 
--> R[ 3] (starting from index 0) = 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 [END]
correct is 0 (since DISCARDED_BITS is actually 001f80)
      R =  
    Print reg Rbit:
    R[ 2] (starting from index 0) = 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0001 0000 0000 0000 0001 0000 0000 0000 0000 0000 0000 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 [END]
      T =  
    Print reg T: 
    R[ 1] (starting from index 0) = 0001 0000 0000 0000 0000 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0001 0000 0000 0000 0001 0000 0001 0000 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 [END]
    G (L + R + T) =  
    Print reg RND: 
    R[ 0] (starting from index 0) = 0001 0000 0001 0000 0000 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0000 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0001 0000 0000 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 [END]
correct is 0
    */
    //
    /*
    opA[0] = 0x269160c0; // F32 encoding for (S=0,E=0x4d,F=0x9160c0)
    opB[0] = 0xad9d382d; // F32 encoding for (S=1,E=0x5b,F=0x9d382d)
    resCorrect[0] = 0xad9d35e7; // F32 encoding for (S=1,E=0x5b,F=0x9d35e7)
    */



    opA[1] = 0x29b72c31; // F32 encoding for 
    opB[1] = 0x27905bfc; // F32 encoding for 
    resCorrect[1] = 0x29c031f1; // F32 encoding for 

    for (int idx = 2; idx <= CONNEX_VECTOR_LENGTH / 2; idx++) {
        opA[idx] = GenRandF32Valid(-10, -15);
        //opA[idx] = GenRandF32Valid(-20, -25);
        //opA[idx] = GenRandF32Valid(-30, -35);
        opB[idx] = GenRandF32Valid(-10, -15);
        float res = *((float *)&opA[idx]) +  *((float *)&opB[idx]);
        resCorrect[idx] = *((int *)&res); // F32 encoding for 1.89353e-36 is 0x0421155a
    }


#ifdef DO_SUB
    AddSub_f32Kernel(0, 1, 2, true);
#else
    AddSub_f32Kernel(0, 1, 2);
#endif


    connex->writeDataToConnex(opA, 1, 0);
    connex->writeDataToConnex(opB, 1, 1);

#ifdef LLVM_ISEL_CODEGEN
    //string kernelName = "add_or_sub.f32";
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
        if (isnan_f32(result[i]) || isnan_f32(resCorrect[i])) {
            assert(isnan_f32(result[i]) && isnan_f32(resCorrect[i]));
            printf("i=%d: opA = 0x%04x, opB = 0x%04x --> res = NAN (resCorrect = NAN)\n",
                    i, opA[i], opB[i]);
        }
        else {
            printf("i=%d: opA = 0x%04x, opB = 0x%04x --> res = 0x%04x (resCorrect = 0x%04x)\n",
                    i, opA[i], opB[i], result[i], resCorrect[i]);
        }
    }
    */


  #define NUM_VALS 25


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

        printf("i=%d: opA = %s, opB = %s --> res = %s (resCorrect = %s, diffRes = %d)%s\n",
               i,
               GetStringForF32(opA[i]).c_str(),
               GetStringForF32(opB[i]).c_str(),
               GetStringForF32(result[i]).c_str(),
               GetStringForF32(resCorrect[i]).c_str(),
               //labs(resCorrect[i] - result[i]),
               resCorrect[i] - result[i],
              #ifdef ROUND_TO_NEAREST
               (resCorrect[i] == result[i]) ? "" :
               //labs(resCorrect[i] - result[i]) <= 1 ? "" :
              #else
               labs(resCorrect[i] - result[i]) <= 1 ? "" :
              #endif
                  ((isnan_f32(resCorrect[i]) && isnan_f32(result[i])) ?
                    "" : " (different results!)")
              );
    }


    printf("\n");
}


void Test() {
    FloatAddSubTest(connexGlobal);
}


