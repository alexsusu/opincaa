#include <iostream>
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


/* An implementation of DIV.f16 following SoftFloat library's f16_div.c, which
 * I strongly believe is IEEE 754-2008
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
// From SoftFloat library
const uint16_t softfloat_approxRecip_1k0s[16] = {
    0xFFC4, 0xF0BE, 0xE363, 0xD76F, 0xCCAD, 0xC2F0, 0xBA16, 0xB201,
    0xAA97, 0xA3C6, 0x9D7A, 0x97A6, 0x923C, 0x8D32, 0x887E, 0x8417
};
const uint16_t softfloat_approxRecip_1k1s[16] = {
    0xF0F1, 0xD62C, 0xBFA1, 0xAC77, 0x9C0A, 0x8DDB, 0x8185, 0x76BA,
    0x6D3B, 0x64D4, 0x5D5C, 0x56B1, 0x50B6, 0x4B55, 0x4679, 0x4211
};
*/

string kernelName = "div.f16";

void Div_f16Kernel(int32_t opAPtr, int32_t opBPtr, int32_t resPtr) {
    BEGIN_KERNEL(kernelName);
        EXECUTE_IN_ALL(

    // Register allocation table for the variables used in the program
    #define CT0            31
    #define CT1            30
    #define CT2            29
    #define CT31           28
    #define CT15           27
    #define CT16           29
    //
    #define SRC1           26
    #define SRC1_MANTISSA  25
    #define SRC1_EXPONENT  24
    #define SRC1_SIGN      23
    //
    #define SRC2           22
    #define SRC2_MANTISSA  21
    #define SRC2_EXPONENT  20
    #define SRC2_SIGN      19
    //
    #define DST            18
    #define DST_MANTISSA   17
    #define DST_EXPONENT   16
//    #define DST_SIGN       15
    //
    #define PRED2          14
    #define PRED2A         13
    #define PRED3          12
    //
    //#define CONTINUE        0
    //#define CONTINUE_BACKUP 8
    //#define HAVE_INPUT_NAN 8
    //#define HAVE_INPUT_ZERO 8
    //
    #define MANTISSA_MASK  11
    #define EXPONENT_MASK  10
    #define SIGN_MASK       9
    #define HIDDENBIT_MASK  8

    #define index           7
    #define softfloat_approxRecip_1k0s_read        6
    #define softfloat_approxRecip_1k1s_read        5
    #define AUX             4
    #define AUX2            3

    #define r0              2
    #define rem             1

    //#define rem             18

    #define roundIncrement  0
    #define roundBits      15




   // Note that these variables are already allocated to other register
    #define ZERO1           7
    #define ZERO2           6
    #define V2              5
    #define V1             11
    #define IS_NAN         10
    #define IS_INF          1
    #define IS_ZERO         0
    #define NOT_M1_0        8
    #define NOT_M2_0        2
    #define E1_31           1
    #define E2_31           0



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




  // IMPORTANT: Table softfloat_approxRecip_1k0s is initialized ConnexMachine::InitFPTables()


  //#define LS_ADDRESS_softfloat_approxRecip_1k0s 900
//  #define LS_ADDRESS_softfloat_approxRecip_1k0s (CONNEX_MEM_NUM_ROWS + CONNEX_MEM_NUM_ROWS_EXTRA - 32)
//  #define LS_ADDRESS_softfloat_approxRecip_1k1s (CONNEX_MEM_NUM_ROWS + CONNEX_MEM_NUM_ROWS_EXTRA - 16)
  //#define LS_ADDRESS_softfloat_approxRecip_1k1s (CONNEX_MEM_SIZE + CONNEX_MEM_SIZE_EXTRA - 16)

#ifndef LLVM_ISEL_CODEGEN
 //#define INITIALIZE_TABLE_OF_32_F16_ENTRIES
#endif
 #ifdef INITIALIZE_TABLE_OF_32_F16_ENTRIES
            /*
             * If we are to select the value of softfloat_approxRecip_1k0s wo
             *   keeping the constant in LS memory:
             * if R(index) == 0
             *   softfloat_approxRecip_1k0s = 0xFFC4;
             * if R(index) == 1
             *   softfloat_approxRecip_1k0s = 0xF0BE;
             *
             * TODO: We could use logic minimization - given the 4 input bits of R(index)
             *   we could compute the 16 bits of both
             *        softfloat_approxRecip_1k0s
             *        softfloat_approxRecip_1k1s
             *     but most likely it would be less efficient.
             */
            int i;
            for (i = 0; i < 16; i++) {
                R(AUX) = softfloat_approxRecip_1k0s[i];
                NOP;
                LS[LS_ADDRESS_softfloat_approxRecip_1k0s + i] = R(AUX);

                R(AUX) = softfloat_approxRecip_1k1s[i];
                NOP;
                LS[LS_ADDRESS_softfloat_approxRecip_1k1s + i] = R(AUX);
            }
  #else
      R(AUX) = 0; // We should initialize this since we use it below inside a predicated instruction and Kernel::genLLVM...() will complain.
  #endif // INITIALIZE_TABLE_OF_32_F16_ENTRIES




            R(CT0) = 0;
            R(CT1) = 1;
            R(CT2) = 2;
            R(CT15) = 15;
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
 */
            UnpackF16(__kernel,
                        CT0, CT1, CT31,
                        SRC1, SRC1_SIGN, SRC1_EXPONENT,
                        SRC1_MANTISSA,
                        SIGN_MASK, EXPONENT_MASK, MANTISSA_MASK,
                        HIDDENBIT_MASK,
                        PRED2, PRED2A, PRED3,
                        true, // We require this in order to index correctly LUTs
                        AUX
                        );

            UnpackF16(__kernel,
                        CT0, CT1, CT31,
                        SRC2, SRC2_SIGN, SRC2_EXPONENT,
                        SRC2_MANTISSA,
                        SIGN_MASK, EXPONENT_MASK, MANTISSA_MASK,
                        HIDDENBIT_MASK,
                        PRED2, PRED2A, PRED3,
                        true, // We require this in order to index correctly LUTs
                        AUX
                        );

       #if 0
            PrintDebugReg(SRC1_SIGN);
            PrintDebugReg(SRC2_SIGN);

            // If 1st operand is negative
            R(PRED3) = R(SRC1_SIGN) == R(SIGN_MASK);
            NOP;
        )
        EXECUTE_WHERE_EQ(
          PrintDebugMessage("Before complementing mantissas - note that we keep negative mantissas only to add them, and then complement them to positive s.t. CountLeadingZeros() will work:\n");
            PrintDebugReg(SRC1_MANTISSA);
            /* Where number is negative, get two's complement of mantissa
             * (i.e., the complement w.r.t. 2^16, or 0;
             *   or neg R(SRC1_MANTISSA) + 1). */
            R(SRC1_MANTISSA) = R(CT0) - R(SRC1_MANTISSA);
        )
        EXECUTE_IN_ALL(
            // If 2nd operand is negative:
            R(PRED3) = R(SRC2_SIGN) == R(SIGN_MASK);
            NOP;
        )
        EXECUTE_WHERE_EQ(
            PrintDebugReg(SRC2_MANTISSA);
            /* Where number is negative, get two's complement of mantissa
             *   (i.e., the complement w.r.t. 2^16, or 0;
             *   or neg R(SRC1_MANTISSA) + 1). */
            R(SRC2_MANTISSA) = R(CT0) - R(SRC2_MANTISSA);
        )
        EXECUTE_IN_ALL(
            PrintDebugReg(SRC1_MANTISSA);
            PrintDebugReg(SRC2_MANTISSA);
       #endif


                /*
                NOTE: these checks are being performed at the end of the kernel.

                if (expA == 0x1F) {
                    if ( sigA ) goto propagateNaN;
                    if ( expB == 0x1F ) {
                        if ( sigB ) goto propagateNaN;
                        goto invalid;
                    }
                    goto infinity;
                }
                if (expB == 0x1F) {
                    if (sigB)
                        goto propagateNaN;
                    goto zero;
                }

                if ( ! expB ) {
                    if ( ! sigB ) {
                        if ( ! (expA | sigA) ) goto invalid;
                        softfloat_raiseFlags( softfloat_flag_infinite );
                        goto infinity;
                    }
                    normExpSig = softfloat_normSubnormalF16Sig( sigB );
                    expB = normExpSig.exp;
                    sigB = normExpSig.sig;
                }
                if ( ! expA ) {
                    if ( ! sigA ) goto zero;
                    normExpSig = softfloat_normSubnormalF16Sig( sigA );
                    expA = normExpSig.exp;
                    sigA = normExpSig.sig;
                }
                */

                        // Subtract exponents and re-bias.
                        // expZ = expA - expB + 0xE;
                        R(DST_EXPONENT) = R(SRC1_EXPONENT) - R(SRC2_EXPONENT);
                        R(AUX) = 0xE;
                        R(DST_EXPONENT) += R(AUX);
                      PrintDebugReg(DST_EXPONENT);

                    /*
                    if ( sigA < sigB ) {
                        --expZ;
                        sigA <<= 5;
                    } else {
                        sigA <<= 4;
                    }
                    index = sigB>>6 & 0xF;
                    r0 = softfloat_approxRecip_1k0s[index]
                             - (((uint_fast32_t) softfloat_approxRecip_1k1s[index]
                                     * (sigB & 0x3F))
                                    >>10);
                    sigZ = ((uint_fast32_t) sigA * r0)>>16;
                    rem = (sigA<<10) - sigZ * sigB;
                    sigZ += (rem * (uint_fast32_t) r0)>>26;

                    ++sigZ;
                    if ( ! (sigZ & 7) ) {
                        sigZ &= ~1;
                        rem = (sigA<<10) - sigZ * sigB;
                        if ( rem & 0x8000 ) {
                            sigZ -= 2;
                        } else {
                            if ( rem ) sigZ |= 1;
                        }
                    }
                    */
                    //PrintDebugMessage("DST_MANTISSA_L/H (original):");

                    PrintDebugReg(SRC1_MANTISSA);
                    PrintDebugReg(SRC2_MANTISSA);
                        R(PRED2) = ULT(R(SRC1_MANTISSA), R(SRC2_MANTISSA)); // small-TODO: We could use < instead of ULT
                        /* Important: we use a sort of CSE to avoid creating
                            the else branch WHERE block. */
                        R(SRC1_MANTISSA) <<= 4;
                        //
                        //R(PRED2) = R(SRC1_MANTISSA) < R(SRC2_MANTISSA);
                        R(PRED2) = R(PRED2) == R(CT1);
                      PrintDebugReg(PRED2);
                        NOP;
                       );
                      EXECUTE_WHERE_EQ(
                        R(DST_EXPONENT) -= R(CT1);
                        R(SRC1_MANTISSA) <<= 1;
                   PrintDebugReg(SRC1_MANTISSA);
                      );
                      EXECUTE_IN_ALL(
                        //R(index) = ISHR( R(SRC2_MANTISSA), 6);
                        R(index) = R(SRC2_MANTISSA) >> 6;
                        R(index) &= R(CT15);
                    PrintDebugReg(SRC2_MANTISSA);
                    PrintDebugReg(index);


            R(AUX) = LS_ADDRESS_softfloat_approxRecip_1k1s;
            R(AUX) += R(index);
            NOP;
            R(softfloat_approxRecip_1k1s_read) = LS[R(AUX)];
            PrintDebugReg(softfloat_approxRecip_1k1s_read);
            //
            R(AUX2) = LS_ADDRESS_softfloat_approxRecip_1k0s;
            R(AUX2) += R(index);
            NOP;
            R(softfloat_approxRecip_1k0s_read) = LS[R(AUX2)];
            PrintDebugReg(softfloat_approxRecip_1k0s_read);

            R(AUX) = 0x3F;
            R(AUX) = R(SRC2_MANTISSA) & R(AUX);

            MULT_U(R(softfloat_approxRecip_1k1s_read), R(AUX));
            //R(softfloat_approxRecip_1k1s_read) * R(AUX);

            R(AUX) = MULT_LOW();
            PrintDebugReg(AUX);
            R(AUX2) = MULT_HIGH();
            PrintDebugReg(AUX2);

            // We now cast ((result of multiplication)>>10) to i16
            //
            //R(AUX) = ISHR(R(AUX), 10);
            R(AUX) >>= 10;
            PrintDebugReg(AUX);
            //R(AUX2) = ISHR(R(AUX2), 10);
            //R(AUX2) >>= 10;
            //R(AUX2) = ISHL(R(AUX2), 10);
            R(AUX2) <<= 6;
            PrintDebugReg(AUX2);
            //
            //R(AUX3) = 0xFC00;
            //R(AUX) &= R(AUX3);
            //
            R(AUX) |= R(AUX2);
            PrintDebugReg(AUX);


            /* We finish computing r0:
            r0 = softfloat_approxRecip_1k0s[index]
                         - (((uint_fast32_t) softfloat_approxRecip_1k1s[index]
                                 * (sigB & 0x3F))
                                >>10);
            */
            R(r0) = R(softfloat_approxRecip_1k0s_read) - R(AUX);
            //PrintDebugMessage("DST_MANTISSA_L/H (original):");
          PrintDebugReg(r0);

            PrintDebugReg(SRC1_MANTISSA);
            // sigZ = ((uint_fast32_t) sigA * r0)>>16;
            MULT_U(R(SRC1_MANTISSA), R(r0));
            R(DST_MANTISSA) = MULT_HIGH();
          PrintDebugReg(DST_MANTISSA);

            // rem = (sigA<<10) - sigZ * sigB;
            //R(AUX) = ISHL(R(SRC1_MANTISSA), 10);
            R(AUX) = R(SRC1_MANTISSA) << 10;
            MULT_U(R(DST_MANTISSA), R(SRC2_MANTISSA));
            R(AUX2) = MULT_LOW();
            //R(rem) -= R(AUX2);
            R(rem) = R(AUX) - R(AUX2);
          PrintDebugReg(rem);

            // sigZ += (rem * (uint_fast32_t) r0)>>26;
            MULT_U(R(rem), R(r0));
            R(AUX) = MULT_HIGH();
            //R(AUX) = ISHR( R(AUX), 10 );
            R(AUX) = R(AUX) >> 10;
            R(DST_MANTISSA) += R(AUX);
          PrintDebugReg(DST_MANTISSA);


            // ++sigZ;
            R(DST_MANTISSA) += R(CT1);

            //  if ( (sigZ & 7) == 0)
            //        sigZ &= 0xFFFE; //~1;
            R(AUX) = 0x7;
            R(AUX2) = R(DST_MANTISSA) & R(AUX);
            //

              R(PRED2) = R(AUX2) == R(CT0);
              NOP;
             );
            EXECUTE_WHERE_EQ(
              R(AUX) = 0xFFFE;
              R(DST_MANTISSA) &= R(AUX);
            );
            EXECUTE_IN_ALL(

            /* rem = (sigA<<10) - sigZ * sigB;
             */
            //R(AUX) = ISHL(R(SRC1_MANTISSA), 10);
            R(AUX) = R(SRC1_MANTISSA) << 10;
            MULT_U(R(DST_MANTISSA), R(SRC2_MANTISSA));
            R(AUX2) = MULT_LOW();
            R(rem) -= R(AUX2);

            /* if (rem & 0x8000)
                sigZ -= 2; */
            R(AUX) = R(rem) & R(SIGN_MASK);

              R(PRED2) = R(AUX) == R(CT0);
              R(PRED2) = R(CT1) - R(PRED2);
              R(PRED2) = R(PRED2) == R(CT1);
              NOP;
             );
            EXECUTE_WHERE_EQ(
              R(DST_MANTISSA) -= R(CT2);
            );
            EXECUTE_IN_ALL(

              /* else
                 if (rem) sigZ |= 1; */
              R(AUX) = R(rem) == R(CT0);
              R(AUX) = R(CT1) - R(AUX);
              //R(AUX) = R(AUX) == R(CT1);
              //R(PRED2) = R(PRED2) == R(CT0);
              R(PRED2) = R(CT1) - R(PRED2);
              R(PRED2) &= R(AUX);
              R(PRED2) = R(PRED2) == R(CT1);
              NOP;
             );
            EXECUTE_WHERE_EQ(
              R(DST_MANTISSA) |= R(CT1);
            );
            EXECUTE_IN_ALL(


            R(CT16) = 16;

            #ifdef PACK_F16_SIMPLE_BUT_NOT_GOOD_FOR_ALL_CASES
                      PrintDebugReg(DST_MANTISSA);
                          //R(DST_MANTISSA) = ISHR(R(DST_MANTISSA), 4);
                          R(DST_MANTISSA) >>= 4;
                          R(DST_EXPONENT) += R(CT1);
                        PrintDebugReg(DST_EXPONENT);
            #endif


            // We now implement s_roundPackToF16.c
            R(roundIncrement) = 0x8;
            R(roundBits) = R(DST_MANTISSA) & R(CT15);
            PrintDebugReg(roundBits);


            /* We now implement s_roundPackToF16.c's C code:
                if ( 0x1D <= (unsigned int) exp ) {
                    if ( exp < 0 ) {
                    ...
              IMPORTANT NOTE: the overflow we implement it at the end.
            */
          #ifndef CONNEX_HAS_SHIFTER_WITH_5BITS_2ND_OPERAND
            // For the case SHR 16+ below
            R(AUX) = 0;
            R(AUX2) = 0;
          #endif
            R(PRED3) = R(DST_EXPONENT) < R(CT0);
            NOP;
        )
        EXECUTE_WHERE_LT(
            PrintDebugReg(DST_EXPONENT);

        /*
        We implement now the call:
          sig = softfloat_shiftRightJam32(a = sig, dist = -exp);
        i.e. the following C code:
            uint16_t res;
            if (dist < 31) { // IMPORTANT: This should always be true since exp in 0..31 so -exp in -31..0
                if ((-dist & 31) >= 16) {
                    int tmp = (-dist & 31) - 16;
                    res = (a << tmp);
                    res = res != 0;
                }
                else {
                    res = (a != 0);
                }

                res |= a>>dist;
            }
            else {
                // This should never happen
                res = (a != 0);
            }
            return res;
        */
        R(AUX) = R(CT0) - R(DST_EXPONENT); // This is dist
        // We do not check dist < 31 because it should always be true
        // We now implement branch if ((-dist & 31) >= 16)
        R(AUX2) = R(DST_EXPONENT) & R(CT31); // This is -dist & 31
        /*
        R(AUX2) -= R(CT15);
        R(AUX2) -= R(CT1); // This is tmp
        */
        R(AUX2) -= R(CT16); // This is tmp
        //
        PrintDebugReg(AUX2);
        PrintDebugReg(DST_MANTISSA);

        /* MEGA-TODO-mild-importance: maybe do this, although negative value
           for 2nd opnd of SHL does not affect much the final value
           since here we set only the LSB:
        //#define TREAT_NEGATIVE_SHL */
       #ifdef TREAT_NEGATIVE_SHL
        //R(AUX2) = R(DST_MANTISSA);
        // TODO: do better - treat it properly, with another WHERE block
       #else
        R(AUX2) = R(DST_MANTISSA) << R(AUX2);
       #endif

        R(AUX2) = R(AUX2) == R(CT0);
        R(AUX2) = R(CT1) - R(AUX2);
        // MEGA-TODO-mild-importance: do elseif ((-dist & 31) >= 16) (although not doing it doesn't affect much because it sets differently the LSB



        R(DST_EXPONENT) = 0;
        R(roundBits) = R(DST_MANTISSA) & R(CT15);

        PrintDebugReg(AUX);


      #ifdef CONNEX_HAS_SHIFTER_WITH_5BITS_2ND_OPERAND
        R(DST_MANTISSA) >>= R(AUX);
        R(DST_MANTISSA) |= R(AUX2);
      #else
                // We address case AUX > 15
                )
                EXECUTE_IN_ALL(

                    /*R(PRED2A) = 16;
                    R(PRED2) = R(PRED2A) < R(AUX);*/
                    R(PRED2) = R(CT15) < R(AUX);
                    NOP;
                )
                EXECUTE_WHERE_LT(
                    /*
                    R(AUX) -= R(CT15);
                    R(AUX) -= R(CT1);
                    */
                    R(AUX) -= R(CT16);

                    R(DST_MANTISSA) = 0;
                )
                EXECUTE_IN_ALL(
        PrintDebugReg(AUX);

        /*
       #define TREAT_BIG_SHR
       #ifdef TREAT_BIG_SHR
        //R(DST_MANTISSA) = R(AUX);
        R(DST_MANTISSA) = 0;
       #else
        R(DST_MANTISSA) >>= R(AUX);
       #endif
        */

        R(DST_MANTISSA) >>= R(AUX);
        R(DST_MANTISSA) |= R(AUX2);
      #endif // CONNEX_HAS_SHIFTER_WITH_5BITS_2ND_OPERAND

        /*
        Implementing the following C code:
            sig = (sig + roundIncrement)>>4;
            if ( roundBits ) {
                softfloat_exceptionFlags |= softfloat_flag_inexact;
            }
            sig &= ~(uint_fast16_t) (! (roundBits ^ 8) & roundNearEven);
            if (!sig) exp = 0;
        */
        PrintDebugReg(DST_MANTISSA);
        R(DST_MANTISSA) += R(roundIncrement);
        R(DST_MANTISSA) >>= 4;
        PrintDebugReg(DST_MANTISSA);

        R(AUX) = 8;
        R(AUX) ^= R(roundBits);
        R(AUX) = R(AUX) == R(CT0);
        R(AUX) = ~R(AUX);
        R(DST_MANTISSA) &= R(AUX);
        PrintDebugReg(DST_MANTISSA);

            R(PRED3) = R(DST_MANTISSA) == R(CT0);
            NOP;
        )
        EXECUTE_WHERE_EQ(
            R(DST_EXPONENT) = 0;
            PrintDebugReg(DST_EXPONENT);
        )
        EXECUTE_IN_ALL(


    // My atempt: R(DST_EXPONENT) += R(CT1);

        /*
            R(PRED3) = R(DST_EXPONENT) < R(CT0);
            NOP;
        )
        EXECUTE_WHERE_LT(
            R(DST_EXPONENT) = 0;
            PrintDebugReg(DST_EXPONENT);
        )
        EXECUTE_IN_ALL(
        */




            // Put the f16 number back together
            // Shift exponent in the final place
            R(DST) = R(DST_EXPONENT) << F16_MANTISSA_BITS;
            //R(AUX) = R(DST_EXPONENT) << F16_MANTISSA_BITS;
            PrintDebugReg(DST);


            // Get rid of hidden bit of mantissa, which is always 1
            // R(DST_MANTISSA) &= R(MANTISSA_MASK);

    PrintDebugReg(DST_EXPONENT);
    PrintDebugReg(DST_MANTISSA);

            // Add mantissa:
            /* As done in SoftFloat f16_div.c we actually
                add the mantissa WITH the hidden bit to
                the exponent (so the DST_EXPONENT is actually 1 unit less than the real value). */
            R(DST) += R(DST_MANTISSA);
            PrintDebugReg(DST);





        /* We treat overflows (exponent overflow) here.
           IMPORTANT NOTE: the overflow is treated in softfloat_roundPackToF16(),
                but we prefer implementing it now, at the end.

           The softfloat_roundPackToF16() C code for overflow is this:

            if ( 0x1D <= (unsigned int) exp ) {
              if ( exp < 0 ) {
              ...
              } else if ( (0x1D < exp) ||
                  // This case below is for mantissa overflow: 0x1D == exp - actually exp should be 0x1E
                  (0x8000 <= sig + roundIncrement) ) {  // MEGA-TODO-if-mantissa-overflow-happens: implement this case of mantissa oveflow although it seems it's never happening - I do NOT understand when sig can be 0x8000 - 8 or bigger
                    softfloat_raiseFlags(
                        softfloat_flag_overflow | softfloat_flag_inexact);

                    uiZ = packToF16UI( sign, 0x1F, 0 ); //- ! roundIncrement;
                    goto uiZ;
                }
           */
          PrintDebugMessage("We treat (exponent) overflow");
            R(DST_EXPONENT) += R(CT1); // We increment to test 31 < exp below

            /* We do this because when we pack we simply
               add mantissa WITH the hidden bit to
                the exponent
               (so the DST_EXPONENT is actually 1 unit less than the real value).

                Adding 1 is CORRECT because:
                    - denormals, the only values that don't have hidden mantissa bit have
                    exponent 0 here (normally 1), so 31 < DST_EXPONENT is definitely false.
                    - therefore, all values that matter have hidden bit of mantissa 1.
                       Since at pack time we basically add this hidden bit to the
                       final exponent we do it also here for overflow.

               If we don't do this we end up having an INF represented differently
                 e.g. as NAN.
             */
            R(DST_EXPONENT) += R(CT1);
            R(PRED2) = R(CT31) < R(DST_EXPONENT);
            PrintDebugReg(PRED2);
            NOP;
        );
        EXECUTE_WHERE_LT(
            R(DST) = F16_INF_POSITIVE; // The sign is updated below
            PrintDebugReg(DST);
        );
        EXECUTE_IN_ALL(


            /* We now treat special cases with NAN, INF.

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
                  See /home/asusu/LLVM/Tests/opincaa_standalone_apps/Emulate_f16/1Espresso/WithDCs/DIVf16_TODO/espresso_DIVf16_gen.output_annotated
                    IS_NAN = Zero1 . Zero2 + E2 . !M2 + E1 . !M1 + E1 . E2
                    IS_INF = E1
                    is_Zero = E2
              I don't think we can benefit from multi-level logic minimization
                - read GDM's book chapter on this to see if
                  multiplexors/predicates/conditionals are allowed.
                  - find in which Section of GDM book is this don't cares issue
                    (remember that I discovered it because I had computed
                     INPUT_EXP31 before thinking on doing logic minimization)

               The axioms are:
                  NAN / x = x / NAN = NAN
                  //
                  INF / INF = NAN
                  0 / 0 = NAN
                  //
                  x / INF = 0 (sign is sgn1 xor sgn2)
                  x / 0 = INF (sign is sgn1 xor sgn2)
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
            R(E1_31) = R(SRC1_EXPONENT) == R(CT31);
          PrintDebugReg(E1_31);
            R(NOT_M1_0) = R(CT0) < R(SRC1_MANTISSA);
          PrintDebugReg(NOT_M1_0);
            //
            // small TODO (maybe DAG Combiner takes care): SRC2_EXPONENT == 31 was already computed above
            R(E2_31) = R(SRC2_EXPONENT) == R(CT31);
            R(NOT_M2_0) = R(CT0) < R(SRC2_MANTISSA);
            //
            R(V1) = R(E1_31) & R(NOT_M1_0);
            R(V2) = R(E2_31) & R(NOT_M2_0);
            R(IS_NAN) = R(V1) | R(V2);
          PrintDebugReg(IS_NAN);
            //
            R(V1) = R(ZERO1) & R(ZERO2);
          PrintDebugReg(V1);
            R(V2) = R(E1_31) & R(E2_31);
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
            R(DST) = F16_NAN;
          PrintDebugReg(DST);
      #ifdef ROUND_TO_NEAREST
            /* IMPORTANT: we don't need this because RND can be
             at most 1 and F16_NAN is positive number, so adding RND still
            makes DST a NAN */
            //R(RND) = 0;
          PrintDebugReg(RND);
      #endif

         #ifdef LANE_GATING
            DISABLE_CELL;
         #else
            //R(CONTINUE) = 0;
         #endif
        );
        EXECUTE_IN_ALL(


            //R(IS_INF) = R(E1_31);
            R(AUX) = R(IS_NAN) == R(CT0);
            R(AUX2) = R(IS_INF) == R(CT1);
            R(AUX2) &= R(AUX);
          PrintDebugReg(AUX2);
            R(AUX2) = R(AUX2) == R(CT1);
          PrintDebugReg(AUX2);
            NOP;
        );
        EXECUTE_WHERE_EQ(
            R(DST) = F16_INF_POSITIVE;
            // We add sign later: R(DST) |= R(DST_SIGN);
          PrintDebugReg(DST);

         #ifdef LANE_GATING
            DISABLE_CELL;
         #else
            //R(CONTINUE) = 0;
         #endif
        );
        EXECUTE_IN_ALL(

            R(AUX2) = R(IS_INF) == R(CT0);
            R(AUX2) &= R(AUX); // AUX is R(IS_NAN) == R(CT0); - see above
            R(AUX) = R(IS_ZERO) == R(CT1);
            R(AUX2) &= R(AUX);
            R(AUX2) = R(AUX2) == R(CT1);
          PrintDebugReg(AUX2);
            NOP;
        );
        EXECUTE_WHERE_EQ(
            R(DST) = 0;
        );
        EXECUTE_IN_ALL(

    // END: We now treat special cases with NAN, INF .




            /* VERY IMPORTANT: We don't add sign bit now because we want to
                    optimize rounding:
            R(DST) |= R(DST_SIGN); */
            /*
            R(DST) ^= R(SRC1_SIGN);
            R(DST) ^= R(SRC2_SIGN);
            */
            /*
            // Compute sign
            R(DST_SIGN) = R(SRC1_SIGN) ^ R(SRC2_SIGN);
          PrintDebugReg(DST_SIGN);
            */
            R(DST) ^= R(SRC1_SIGN);
            R(DST) ^= R(SRC2_SIGN);
            PrintDebugReg(DST);



            NOP;
            // Store result
            LS[resPtr] = R(DST);

            // End of program synchronization point; host will wait for this
            REDUCE(R0);
        )
    END_KERNEL(kernelName);
}

void FloatDivTest(ConnexMachine *connex) {
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

/*
    opA[0] = 0x3f64;
    opB[0] = 0x5b76;
    resCorrect[0] = 0x5ee5;
*/
    /*
    opA[0] = 0x3D47; // F16 encoding for 1.3193359375
    opB[0] = 0x4470; // F16 encoding for 4.4375
    resCorrect[0] = 0x45DB; // result of mul as obtained in Clang
    */

opA[0] = 0xac04; // F16 encoding for -0.062744
opB[0] = 0xda41; // F16 encoding for -200.125000
resCorrect[0] = 0x0d23; // F16 encoding for 0.000314

/*
sigB = 0x641
index = 9
softfloat_approxRecip_1k0s[index] = 41926 (0xa3c6)
softfloat_approxRecip_1k1s[index] = 25812 (0x64d4)
r0 = 0xa3ad
//  sigA = 0x8080
!!sigZ = 0x5228
rem = 0x33d8
sigZ = 0x5230
signZ = 0
expZ = 3 (0x0003)
sigZ = 1315 (0x0523)
res.v = 291 (0x0123)
res.v = 3363 (0x0d23)
res.v = 3363 (0x0d23)
aF16 = 0.476074
bF16 = -39.562500
uiA = 0xac04
uiB = 0xda41
resCorrect = -0.012032
res.v = 0x0d23
resF16 = 0.000314
*/

    opA[1] = 0x9869;
            // F16 encoding for (S=1,E=0x6,F=0x469)
    opB[1] = 0x4873; // F16 encoding for (S=0,E=0x12,F=0x473)
    resCorrect[1] = 0x8bee; // result of

/*
sigB = 0x473
index = 1
softfloat_approxRecip_1k0s[index] = 61630 (0xf0be)
softfloat_approxRecip_1k1s[index] = 54828 (0xd62c)
r0 = 0xe614 // TODO: f094
//  sigA = 0x8d20
!!sigZ = 0x7ed5
rem = 0x3251
sigZ = 0x7ee0
signZ = 1
expZ = 2 (0x0002)
sigZ = 2030 (0x07ee)
res.v = 1006 (0x03ee)
res.v = 3054 (0x0bee)
res.v = 35822 (0x8bee)
aF16 = 0.001356
bF16 = 195.875000
uiA = 0x9869
  uiA = -0.002153
uiB = 0x4873
  uiB = 8.898438
resCorrect = 0.000007
res.v = 0x8bee
resF16 = -0.000242
*/
    opA[2] = 0x944a; // F16 encoding for (S=1,E=0x5,F=0x44a)
    opB[2] = 0x58ec; // F16 encoding for (S=0,E=0x16,F=0x4ec)
    resCorrect[2] = 0x8070; // result of (S=1,E=0x0,F=0x70)

    opA[3] = 0x3e5a; // F16 encoding for (S=0,E=0xf,F=0x65a)
    opB[3] = 0x82c5; // F16 encoding for (S=1,E=0x0,F=0x2c5) // denormal
    resCorrect[3] = 0xf896; // result of (S=1,E=0x1e,F=0x496)

    opA[4] = 0x1f29; // F16 encoding for (S=0,E=0x7,F=0x729)
    opB[4] = 0x7ccd; // F16 encoding for NAN
    resCorrect[4] = F16_NAN; // result of

    opA[5] = 0x1f29; // F16 encoding for (S=0,E=0x7,F=0x729)
    opB[5] = F16_INF_POSITIVE; // F16 encoding for NAN
    resCorrect[5] = 0; // result of

    opA[6] = F16_INF_POSITIVE; // F16 encoding for (S=0,E=0x7,F=0x729)
    opB[6] = 0x3c00; // F16 encoding for 1.0
    resCorrect[6] = F16_INF_POSITIVE; // result of

    opA[7] = 0x7ccd; // F16 encoding for NAN
    opB[7] = 0x1f29; // F16 encoding for (S=0,E=0x7,F=0x729)
    resCorrect[7] = F16_NAN; // result of

    opA[8] = 0x6125; // F16 encoding for (S=0,E=0x18,F=0x525)
    opB[8] = 0x895d; // F16 encoding for (S=1,E=0x2,F=0x55d)
    resCorrect[8] = F16_INF_NEGATIVE; // result

    opA[9] = 0xc315; // F16 encoding for (S=1,E=0x10,F=0x715)
    opB[9] = 0x015c; // F16 encoding for (S=0,E=0x0,F=0x15c) // denormal
    resCorrect[9] = F16_INF_NEGATIVE; // result

    opA[10] = 0x8422; // F16 encoding for (S=1,E=0x1,F=0x422)
    opB[10] = 0x0119; // F16 encoding for (S=0,E=0x0,F=0x119) // denormal
    resCorrect[10] = 0xc388; // (S=1,E=0x10,F=0x788) result

    opA[11] = 0x821b; // F16 encoding for (S=1,E=0x0,F=0x21b)
    opB[11] = 0x7713; // F16 encoding for (S=0,E=0x1d,F=0x713)
    resCorrect[11] = 0x0000; // (S=1,E=0x10,F=0x788) result

    /*
    IMPORTANT: this case can result in NAN instead of INF due to
        poor overflow treatment.
    */
    opA[12] = 0xf6a1; // F16 encoding for (S=1,E=0x1d,F=0x6a1)
    opB[12] = 0xb61a; // F16 encoding for (S=1,E=0xd,F=0x61a)
    resCorrect[12] = F16_INF_POSITIVE;
#if 0
    /* IMPORTANT: this case makes softfloat_shiftRightJam32() execute a SHR
     with 2nd operand negative normally */
    opA[13] = 0x000b; // F16 encoding for (S=0,E=0x0,F=0xb)
    opB[13] = 0xf728; // F16 encoding for (S=1,E=0x1d,F=0x728)
    resCorrect[13] = 0;
#endif

  #define NUM_VALS 13



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





    Div_f16Kernel(0, 1, 2);

    connex->writeDataToConnex(opA, 1, 0);
    connex->writeDataToConnex(opB, 1, 1);


#ifdef LLVM_ISEL_CODEGEN
    Kernel *kernel = connexGlobal->getKernel(kernelName);
    kernel->sdNodeVarNameRegDef[SRC1] = "nodeOpSrcCast1";
    kernel->sdNodeVarNameRegDef[SRC2] = "nodeOpSrcCast2";
    //
    // For DIV f16:
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

    // For DIV.f16 we don't require this: assert(CONNEX_REG_COUNT != 32);

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

    printf("DIV results are:\n");

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
    FloatDivTest(connexGlobal);
}

