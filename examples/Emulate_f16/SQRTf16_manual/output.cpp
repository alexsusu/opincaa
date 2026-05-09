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


/* An implementation of sqrt.f16 following SoftFloat library's f16_sqrt.c, which
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
// From SoftFloat library, file s_approxRecipSqrt_1Ks.c
const uint16_t softfloat_approxRecipSqrt_1k0s[16] = {
    0xB4C9, 0xFFAB, 0xAA7D, 0xF11C, 0xA1C5, 0xE4C7, 0x9A43, 0xDA29,
    0x93B5, 0xD0E5, 0x8DED, 0xC8B7, 0x88C6, 0xC16D, 0x8424, 0xBAE1
};
const uint16_t softfloat_approxRecipSqrt_1k1s[16] = {
    0xA5A5, 0xEA42, 0x8C21, 0xC62D, 0x788F, 0xAA7F, 0x6928, 0x94B6,
    0x5CC7, 0x8335, 0x52A6, 0x74E2, 0x4A3E, 0x68FE, 0x432B, 0x5EFD
};
*/

string kernelName = "sqrt.f16";

void Sqrt_f16Kernel(int32_t opAPtr, int32_t resPtr) {
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
    #define SRC1_MANTISSA_NEW  13
    #define SRC1_EXPONENT  24
    #define SRC1_EXPONENT_NEW  12
    #define SRC1_SIGN      23
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
    #define softfloat_approxRecipSqrt_1k0s_read        6
    #define softfloat_approxRecipSqrt_1k1s_read        5
    #define AUX             4
    #define AUX2            3

    #define r0              2
    #define ESqrR0          1
    //
    #define sigma0         22
    #define recipSqrt16    21
    #define shiftedSigZ    20
    #define negRem         19

    #define roundIncrement  0
    #define roundBits      15




   // Note that these variables are already allocated to other register
    #define ZERO1           7
    #define V2              5
    #define V1             11
    #define IS_NAN         10
    #define IS_INF         12
    #define IS_ZERO         0
    #define NOT_M1_0        8
    #define M1_0            8
    #define E1_31           1



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

            R(SRC1) = LS[opAPtr]; // load the F16 operand
          PrintDebugReg(SRC1);


  // IMPORTANT: Table softfloat_approxRecipSqrt_1k0s is initialized ConnexMachine::InitFPTables()

  /*
  #define LS_ADDRESS_softfloat_approxRecipSqrt_1k0s 900
  //#define LS_ADDRESS_softfloat_approxRecipSqrt_1k0s (CONNEX_MEM_SIZE + CONNEX_MEM_SIZE_EXTRA - 64)
  #define LS_ADDRESS_softfloat_approxRecipSqrt_1k1s 916 // (CONNEX_MEM_SIZE + CONNEX_MEM_SIZE_EXTRA - 48)
  */

 //#define INITIALIZE_TABLE_OF_32_F16_ENTRIES
 #ifdef INITIALIZE_TABLE_OF_32_F16_ENTRIES
            /*
             * If we are to select the value of softfloat_approxRecip_1k0s wo
             *   keeping the constant in LS memory:
             * if R(index) == 0
             *   softfloat_approxRecip_1k0s = 0xFFC4;
             * if R(index) == 1
             *   softfloat_approxRecip_1k0s = 0xF0BE;
             *
             * silly-TODO: We could use logic minimization - given the 4 input bits of R(index)
             *   we could compute the 16 bits of both
             *        softfloat_approxRecipSqrt_1k0s
             *        softfloat_approxRecipSqrt_1k1s
             *     but most likely it would be less efficient.
             */
            int i;
            for (i = 0; i < 16; i++) {
                R(AUX) = softfloat_approxRecipSqrt_1k0s[i];
                NOP;
                LS[LS_ADDRESS_softfloat_approxRecipSqrt_1k0s + i] = R(AUX);

                R(AUX) = softfloat_approxRecipSqrt_1k1s[i];
                NOP;
                LS[LS_ADDRESS_softfloat_approxRecipSqrt_1k1s + i] = R(AUX);
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



                        /*
                        NOTE: these checks are being performed at the end of the kernel.

                        if ( expA == 0x1F ) {
                            if ( sigA ) {
                                uiZ = softfloat_propagateNaNF16UI( uiA, 0 );
                                goto uiZ;
                            }
                            if ( ! signA ) return a; // NOTE: if SRC1 == +INF return +INF, else return NAN
                            goto invalid;
                        }

                        if ( signA ) {
                            if ( ! (expA | sigA) ) return a; // NOTE: if SRC1 == 0 return 0
                          // This case is handled at the end
                            goto invalid; // Alex: In case we have negative number --> return NaN
                        }
                        if ( ! expA ) {
                            if ( ! sigA ) return a;
                            normExpSig = softfloat_normSubnormalF16Sig( sigA );
                            expA = normExpSig.exp;
                            sigA = normExpSig.sig;
                        }
                        */

                /*
                OLD: from DIV.f16
                if (expA == 0x1F) {
                    if ( sigA ) goto propagateNaN;
                    if ( expB == 0x1F ) {
                        if ( sigB ) goto propagateNaN;
                        goto invalid;
                    }
                    goto infinity;
                }

                if ( ! expA ) {
                    if ( ! sigA ) goto zero;
                    normExpSig = softfloat_normSubnormalF16Sig( sigA );
                    expA = normExpSig.exp;
                    sigA = normExpSig.sig;
                }
                */

                        // Subtract exponents and re-bias.
                        // From f16_sqrt.c: expZ = ((expA - 0xF)>>1) + 0xE;
                        R(DST_EXPONENT) = R(SRC1_EXPONENT) - R(CT15);
                        R(DST_EXPONENT) >>= 1;
                        R(AUX) = 0xE;
                        R(DST_EXPONENT) += R(AUX);
                      PrintDebugMessage("1st expZ:");
                      PrintDebugReg(DST_EXPONENT);

                        // From f16_sqrt.c: expA &= 1;
//                        R(SRC1_EXPONENT) &= R(CT1);
                        R(SRC1_EXPONENT_NEW) = R(SRC1_EXPONENT) & R(CT1);

                        // From f16_sqrt.c: sigA |= 0x0400;
                        R(AUX) = 0x0400;
//                        R(SRC1_MANTISSA) |= R(AUX);
                        R(SRC1_MANTISSA_NEW) = R(SRC1_MANTISSA) | R(AUX);

                    /*
                    // From f16_sqrt.c:
                    index = (sigA>>6 & 0xE) + expA;
                    r0 = softfloat_approxRecipSqrt_1k0s[index]
                            - (((uint_fast32_t) softfloat_approxRecipSqrt_1k1s[index]
                                    * (sigA & 0x7F))
                                    >>11);
                    ESqrR0 = ((uint_fast32_t) r0 * r0)>>1;
                    if ( expA ) ESqrR0 >>= 1;
                    sigma0 = ~(uint_fast16_t) ((ESqrR0 * sigA)>>16);
                    recipSqrt16 = r0 + (((uint_fast32_t) r0 * sigma0)>>25);
                    if ( ! (recipSqrt16 & 0x8000) ) recipSqrt16 = 0x8000;
                    sigZ = ((uint_fast32_t) (sigA<<5) * recipSqrt16)>>16;
                    if ( expA ) sigZ >>= 1;
                    //------------------------------------------------------------------------
                    //------------------------------------------------------------------------
                    ++sigZ;
                    if ( ! (sigZ & 7) ) {
                        shiftedSigZ = sigZ>>1;
                        negRem = shiftedSigZ * shiftedSigZ;
                        sigZ &= ~1;
                        if ( negRem & 0x8000 ) {
                            sigZ |= 1;
                        } else {
                            if ( negRem ) --sigZ;
                        }
                    }
                    return softfloat_roundPackToF16( 0, expZ, sigZ );
                    */

                     // Repating C code: index = (sigA>>6 & 0xE) + expA;
                      PrintDebugReg(SRC1_MANTISSA_NEW);
                      PrintDebugReg(SRC1_EXPONENT_NEW);
                        R(index) = ISHR(R(SRC1_MANTISSA_NEW), 6);
                        //R(index) = R(SRC1_MANTISSA_NEW) >> 6;
                        R(AUX) = 0x0E;
                        R(index) &= R(AUX);
                        R(index) += R(SRC1_EXPONENT_NEW);
                      PrintDebugReg(index);


            R(AUX) = LS_ADDRESS_softfloat_approxRecipSqrt_1k1s;
            R(AUX) += R(index);
          PrintDebugMessage("Table value");
          PrintDebugReg(AUX);
            NOP;
            R(softfloat_approxRecipSqrt_1k1s_read) = LS[R(AUX)];
            PrintDebugReg(softfloat_approxRecipSqrt_1k1s_read);
            //
            R(AUX2) = LS_ADDRESS_softfloat_approxRecipSqrt_1k0s;
            R(AUX2) += R(index);
            NOP;
            R(softfloat_approxRecipSqrt_1k0s_read) = LS[R(AUX2)];
            PrintDebugReg(softfloat_approxRecipSqrt_1k0s_read);

            R(AUX) = 0x7F;
            R(AUX) = R(SRC1_MANTISSA_NEW) & R(AUX);

            MULT_U(R(softfloat_approxRecipSqrt_1k1s_read), R(AUX));
            //R(softfloat_approxRecipSqrt_1k1s_read) * R(AUX);

            R(AUX) = MULT_LOW();
            PrintDebugReg(AUX);
            R(AUX2) = MULT_HIGH();
            PrintDebugReg(AUX2);

            // We now cast the ((result of multiplication)>>11) to i16
            //
            //R(AUX) = ISHR(R(AUX), 11);
            R(AUX) >>= 11;
            PrintDebugReg(AUX);
            //R(AUX2) = ISHR(R(AUX2), 11);
            //R(AUX2) >>= 10;
            //R(AUX2) = ISHL(R(AUX2), 11);
            R(AUX2) <<= 5;
            PrintDebugReg(AUX2);
            //
            //R(AUX3) = 0xFC00;
            //R(AUX) &= R(AUX3);
            //
            R(AUX) |= R(AUX2);
            PrintDebugReg(AUX);


            /* We finish computing r0:
            r0 = softfloat_approxRecipSqrt_1k0s[index]
                     - (((uint_fast32_t) softfloat_approxRecipSqrt_1k1s[index]
                             * (sigA & 0x7F))
                            >>11);
             */
            R(r0) = R(softfloat_approxRecipSqrt_1k0s_read) - R(AUX);
            //PrintDebugMessage("DST_MANTISSA_L/H (original):");
          PrintDebugReg(SRC1_MANTISSA_NEW);

          PrintDebugReg(r0);
            // ESqrR0 = ((uint_fast32_t) r0 * r0)>>1;
            MULT_U(R(r0), R(r0));
            R(ESqrR0) = MULT_LOW();
            R(AUX) = MULT_HIGH();
          PrintDebugReg(ESqrR0);
          PrintDebugReg(AUX);
            R(ESqrR0) >>= 1;
            R(AUX2) = ISHL(R(AUX), 15);
            R(AUX) = ISHR(R(AUX), 1);
          PrintDebugReg(AUX2);
            //R(ESqrR0) >>= 1;
            R(ESqrR0) |= R(AUX2);
          PrintDebugReg(ESqrR0);

         //    if ( expA ) ESqrR0 >>= 1;
              R(PRED2) = R(CT0) < R(SRC1_EXPONENT_NEW);
              NOP;
             );
            EXECUTE_WHERE_LT(
            PrintDebugReg(AUX);
              R(AUX2) = ISHL(R(AUX), 15);
              R(AUX) = ISHR(R(AUX), 1);
            PrintDebugReg(AUX2);
              R(ESqrR0) >>= R(CT1);
              R(ESqrR0) |= R(AUX2);
            );
            EXECUTE_IN_ALL(
//DONE: R(ESqrR0) = 0x870e; // correct - need to put also MULT_HIGH()

          PrintDebugReg(ESqrR0);
          PrintDebugReg(SRC1_MANTISSA_NEW);
         // sigma0 = ~(uint_fast16_t) ((ESqrR0 * sigA)>>16);
         //  IMPORTANT: ESqrR0 is uint32_t. R(AUX) contains the upper 16 bits.
            MULT_U(R(ESqrR0), R(SRC1_MANTISSA_NEW));
            //R(ESqrR0) * R(SRC1_MANTISSA_NEW);
            R(sigma0) = MULT_HIGH();
          PrintDebugReg(sigma0);

         //  IMPORTANT: ESqrR0 is uint32_t. R(AUX) contains the upper 16 bits.
         //    Adding the contribution of this upper 16-bits multiplied at result.
          PrintDebugReg(AUX);
            MULT_U(R(AUX), R(SRC1_MANTISSA_NEW));
            R(AUX2) = MULT_LOW();
          PrintDebugReg(AUX2);
            R(sigma0) += R(AUX2);
          PrintDebugReg(sigma0);

            R(sigma0) = ~R(sigma0);
          PrintDebugReg(sigma0);

            //recipSqrt16 = r0 + (((uint_fast32_t) r0 * sigma0)>>25);
            MULT_U(R(r0), R(sigma0));
            R(AUX) = MULT_HIGH();
            R(AUX) = ISHR(R(AUX), 9);
          PrintDebugReg(AUX);
            R(recipSqrt16) = R(r0) + R(AUX);
          PrintDebugReg(recipSqrt16);


         // if ( ! (recipSqrt16 & 0x8000) ) recipSqrt16 = 0x8000;
            R(AUX) = 0x8000;
            R(AUX) = R(recipSqrt16) & R(AUX);
              //R(PRED2) = R(AUX) < R(CT0); // IMPORTANT: recipSqrt16 & 0x8000 can be 0x8000 or 0
              R(PRED2) = R(AUX) == R(CT0); // IMPORTANT: recipSqrt16 & 0x8000 can be 0x8000 or 0
              NOP;
             );
            //EXECUTE_WHERE_LT(
            EXECUTE_WHERE_EQ(
              R(recipSqrt16) = 0x8000;
            );
            EXECUTE_IN_ALL(
          PrintDebugReg(recipSqrt16);


            // sigZ = ((uint_fast32_t) (sigA<<5) * recipSqrt16)>>16;
            R(AUX) = ISHL(R(SRC1_MANTISSA_NEW), 5);
            MULT_U(R(AUX), R(recipSqrt16));
            R(DST_MANTISSA) = MULT_HIGH();
          PrintDebugReg(DST_MANTISSA);

            // if ( expA ) sigZ >>= 1;
              R(PRED2) = R(CT0) < R(SRC1_EXPONENT_NEW);
              NOP;
             );
            EXECUTE_WHERE_LT(
              R(DST_MANTISSA) >>= R(CT1);
            );
            EXECUTE_IN_ALL(


            // ++sigZ;
            R(DST_MANTISSA) += R(CT1);

            //  if ( ! (sigZ & 7) )
              R(AUX) = 0x7;
              R(AUX2) = R(DST_MANTISSA) & R(AUX);
              R(PRED2) = R(AUX2) == R(CT0);
              NOP;
             );
            EXECUTE_WHERE_EQ(

              // shiftedSigZ = sigZ>>1;
              R(shiftedSigZ) = ISHR(R(DST_MANTISSA), 1);

              // negRem = shiftedSigZ * shiftedSigZ;
              R(shiftedSigZ) * R(shiftedSigZ);
              R(negRem) = MULT_LOW();

              // sigZ &= ~1;
              R(AUX) = ~1; // TODO: check if this is -32767
              R(DST_MANTISSA) += R(AUX);

              //if ( negRem & 0x8000 ) {
              //              sigZ |= 1;
              R(AUX) = ISHR(R(negRem), 15);
              R(DST_MANTISSA) |= R(AUX);

              //  } else {
              //      if ( negRem ) --sigZ;
              //  }
              R(PRED2) = R(CT0) < R(negRem); // NOTE: negRem >= 0 since negRem = shiftedSigZ * shiftedSigZ
              R(AUX) = R(CT1) - R(AUX);
              R(PRED2) &= R(AUX);
              R(DST_MANTISSA) -= R(PRED2);
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
//            R(PRED3) = R(DST_EXPONENT) < R(CT0);
            R(PRED2) = R(DST_EXPONENT) < R(CT0);
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
           for 2nd opnd of SHL does not affect affect much the final value
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

//            R(PRED3) = R(DST_MANTISSA) == R(CT0);
            R(PRED2) = R(DST_MANTISSA) == R(CT0);
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

              TODO: check Ercegovac book, etc for exact cases.

            */

          //PrintDebugReg(SRC1_SIGN);
          PrintDebugReg(SRC1_EXPONENT);
          PrintDebugReg(SRC1_MANTISSA);

            R(E1_31) = R(SRC1_EXPONENT) == R(CT31);
          PrintDebugReg(E1_31);


            R(M1_0) = R(SRC1_MANTISSA) == R(CT0);
          PrintDebugReg(M1_0);
            R(IS_INF) = R(E1_31) & R(M1_0);
          PrintDebugReg(IS_INF);

              R(PRED2) = R(IS_INF) == R(CT1);
              NOP;
            );
            EXECUTE_WHERE_EQ(
              R(DST) = F16_INF_POSITIVE;
            );
        EXECUTE_IN_ALL(



            //R(ZERO1) = R(SRC1) == R(SRC1_SIGN);

            R(NOT_M1_0) = R(CT0) < R(SRC1_MANTISSA);
            //R(M1_0) = R(CT0) == R(SRC1_MANTISSA);
          PrintDebugReg(NOT_M1_0);
            R(IS_NAN) = R(E1_31) & R(NOT_M1_0);
          PrintDebugReg(IS_NAN);
            //
            // Treating negative input operands
            R(AUX) = R(SRC1_SIGN) < R(CT0);
            R(IS_NAN) |= R(AUX);
          PrintDebugReg(IS_NAN);
            //
              R(PRED2) = R(IS_NAN) == R(CT1);
              NOP;
            );
            EXECUTE_WHERE_EQ(
              R(DST) = F16_NAN;
            );
        EXECUTE_IN_ALL(


#if 0
            R(ZERO2) = R(SRC2) == R(SRC2_SIGN);
          PrintDebugReg(SRC1_SIGN);
          PrintDebugReg(SRC2_SIGN);
          PrintDebugReg(ZERO1);
          PrintDebugReg(ZERO2);
          PrintDebugReg(DST);
            //
            // small TODO (maybe DAG Comb/iner takes care): SRC1_EXPONENT == 31 was already computed above
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

    // END: We now treat special cases with NAN: NAN * 0, NAN * INF .




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
#endif


            NOP;
            // Store result
            LS[resPtr] = R(DST);

            // End of program synchronization point; host will wait for this
            REDUCE(R0);
        )
    END_KERNEL(kernelName);
}

void FloatSqrtTest(ConnexMachine *connex) {
    uint16_t opA[CONNEX_VECTOR_LENGTH];
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


    opA[0] = 0x4880; // F16 encoding for 9.000000
    resCorrect[0] = 0x4200; // F16 encoding for 3.000000

    opA[1] = 0x3C00; // F16 encoding for 1.000000
    resCorrect[1] = 0x3C00; // F16 encoding for 1.000000

    opA[2] = 0xBC00; // F16 encoding for 1.000000
    resCorrect[2] = F16_NAN; // F16 encoding for 1.000000

    opA[3] = 0xC400; // F16 encoding for 1.000000
    resCorrect[3] = F16_NAN; // F16 encoding for 1.000000

    opA[4] = F16_NAN; // F16 encoding for 1.000000
    resCorrect[4] = F16_NAN; // F16 encoding for 1.000000

    opA[5] = F16_INF_POSITIVE; // F16 encoding for 1.000000
    resCorrect[5] = F16_INF_POSITIVE; // F16 encoding for 1.000000

    opA[6] = F16_INF_NEGATIVE; // F16 encoding for 1.000000
    resCorrect[6] = F16_NAN; // F16 encoding for 1.000000

    opA[7] = 0x4C00; // F16 encoding for 16.000000
    resCorrect[7] = 0x4400; // F16 encoding for 4.000000

    opA[8] = 0x4000; // F16 encoding for 2.000000
    resCorrect[8] = 0x3DA8; // F16 encoding for 1.4142

    opA[9] = 0x5C00; // F16 encoding for 256.000000
    resCorrect[9] = 0x4C00; // F16 encoding for 16.000000

    opA[10] = 0x4400; // F16 encoding for 4.000000
    resCorrect[10] = 0x4000; // F16 encoding for 2.000000

// 0x4900 is 10.0

  //#define NUM_VALS 13
  #define NUM_VALS 11



#define ZERO_AFTER_NUM_VALS_INPUT_TESTS
#ifdef ZERO_AFTER_NUM_VALS_INPUT_TESTS
    for (int i = NUM_VALS; i < CONNEX_VECTOR_LENGTH; i++) {
        opA[i] = 0x3C00;
        resCorrect[i] = 0x3C00;
    }
#else
    GenRandF16(opA, resCorrect, NUM_VALS);
#endif




    connex->writeDataToConnex(opA, 1, 0);

    Sqrt_f16Kernel(0, 2); // This function just defines the vector kernel



#ifdef LLVM_ISEL_CODEGEN
    Kernel *kernel = connexGlobal->getKernel(kernelName);
    kernel->sdNodeVarNameRegDef[SRC1] = "nodeOpSrcCast";
    //kernel->sdNodeVarNameRegDef[SRC2] = "nodeOpSrcCast2";
    //
    // For SQRT f16:
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
    //assert(CONNEX_REG_COUNT != 32);

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

    printf("SQRT results are:\n");

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

        printf("i=%d: opA = %s --> res = %s (resCorrect = %s)%s\n",
                i,
                GetStringForF16(opA[i]).c_str(),
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
    FloatSqrtTest(connexGlobal);
}

