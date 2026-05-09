#ifndef LIB_MISC_H_INCLUDED
#define LIB_MISC_H_INCLUDED

#include <stdint.h>


#define F16_MANTISSA_BITS 10
#define F16_EXPONENT_BITS 5
//
#define F16_MANTISSA_MASK  0x03FF
#define F16_EXPONENT_MASK  0x7C00
#define F16_SIGN_MASK      0x8000
#define F16_HIDDENBIT_MASK 0x0400
#define F16_EXPONENT_BIAS  15
//#define F16_MAX_EXP 0x1F
#define F16_MAX_EXP ((1UL << F16_EXPONENT_BITS) - 1)
//
// "Main" NaN value:
// This should be a signalling NAN:
//
// We must make sure that F16_NAN is positive since we can add round bit to it
#define F16_NAN 0x7C01
//
#define F16_NAN_2 0xFC01
// This should be a quiet NAN:
#define F16_NAN_3 0xFC80
#define F16_NAN_4 0xFE00
#define F16_NAN_5 0x7E00
#define F16_NAN_6 0x7C01
#define F16_INF_POSITIVE 0x7C00
#define F16_INF_NEGATIVE 0xFC00





#define F32_MANTISSA_BITS     23
#define F32_MANTISSA_BITS_I16  7
#define F32_EXPONENT_BITS      8
//
#define F32_MANTISSA_MASK           0x007FFFFF
#define F32_MANTISSA_MASK_I16_HIGH  0x007F
#define F32_EXPONENT_MASK           0x7F800000
#define F32_EXPONENT_MASK_I16_HIGH  0x7F80
#define F32_SIGN_MASK               0x80000000
#define F32_SIGN_MASK_I16_HIGH      0x8000
#define F32_HIDDENBIT_MASK          0x00800000
#define F32_HIDDENBIT_MASK_I16_HIGH 0x0080
#define F32_EXPONENT_BIAS           0x7f
#define F32_MAX_EXP                 0xFF
#define F32_BIAS                    127
//
// "Main" NaN value:
// This should be a signalling NAN:
// We must make sure that F32_NAN_1 is positive since we can add round bit to it
#define F32_NAN_1        0x7F800001
#define F32_NAN_1_I16_HIGH   0x7F80
#define F32_NAN_1_I16_LOW       0x1
//
#define F32_NAN_2        0xFF800001
#define F32_NAN_3        0x7FFF0000
#define F32_NAN_3_I16_HIGH   0x7FFF
#define F32_NAN_3_I16_LOW       0x0

// This should be a quiet NAN:
//#define F32_NAN_3 0xFC80
//#define F32_NAN_4 0xFE00
//
#define F32_INF_POSITIVE          0x7F800000
#define F32_INF_POSITIVE_I16_HIGH 0x7F80
#define F32_INF_POSITIVE_I16_LOW  0x0
#define F32_INF_NEGATIVE          0xFF800000
#define F32_INF_NEGATIVE_I16_HIGH 0xFF80
#define F32_INF_NEGATIVE_I16_LOW  0x0




unsigned short BitReverse_CPU_u16(unsigned short x) {
    int i;
    unsigned short res = 0;

    for (i = 0; i < 16; i++) {
        int LSBVal = x & 1;
        x >>= 1;
        res |= LSBVal;
        if (i != 16 - 1)
            res <<= 1;
    }
    return res;
}

unsigned int BitReverse_u32_CPU(unsigned int x) {
    int i;
    unsigned int res = 0;

    for (i = 0; i < 32; i++) {
        int LSBVal = x & 1;
        x >>= 1;
        res |= LSBVal;
        if (i != 32 - 1)
            res <<= 1;
    }
    return res;
}

// Compute index of highest bit set in a u32
int IHSB_u32_CPU(unsigned int x) {
    int i;
    unsigned int tmp = 1UL << 31;

    for (i = 31; i >= 0; i--) {
        if (x & tmp)
            return i;
        tmp >>= 1;
    }
    return -1;
}

// Compute index of highest bit set in a u64
int IHSB_u64_CPU(uint64_t x) {
  //#define START_INDEX 63
  #define START_INDEX 49
  //#define START_INDEX 50
  //#define START_INDEX 47
  //#define START_INDEX 40
  //#define START_INDEX 35
  //#define START_INDEX 20
    uint64_t tmp = 1ULL << START_INDEX;

    for (int i = START_INDEX; i >= 0; i--) {
        if (x & tmp)
            return i;
        tmp >>= 1;
    }
    return -1;
}

int isnan_f16(short val) {
    // printf("isnan_f16(): val = %hd\n", val);

    /* In C (standard, e.g. on x86) we can implement isNan(float x)
        as return x != x,
        while for Connex we really have to check values of mantissas and
        exponents:

       Inspired from Ercegovac_Digital_Arithmetic_2004, page 416:
        - a NaN for f16 has the following values:
          If E = 31 and F != 0 (and sign can be 0 or 1), then v = NAN (not a number).


       Inspired from https://www.doc.ic.ac.uk/~eedwards/compsys/float/nan.html:
        In general (f32, f64, f16), a signalling NaN (NANS) has the extra property:
            - F is between 1 and largest value for which 2nd-MSB is 0:
                F is any of the following values 1 .. 1011 1111...1
        A quiet NaN (NANQ) has the extra property:
            - F is between 1 and largest value for which 2nd-MSB is 1:
                F is any of the following values 1011 1111...1 .. 1111 1111...1
    */

    int mantissa = val & F16_MANTISSA_MASK;
    int exp = (val & F16_EXPONENT_MASK) >> F16_MANTISSA_BITS;
    // printf("isnan_f16(): exp = %d\n", exp);
    // printf("isnan_f16(): mantissa = %d\n", mantissa);

    if ( (exp == (1UL << F16_EXPONENT_BITS) - 1) &&
         (mantissa != 0)) {
        //printf("isnan_f16(): returning true\n");
        return true;
    }

    //printf("isnan_f16(): returning false\n");
    return false;
}


int isnan_f32(int val) {
    // printf("isnan_f32(): val = %hd\n", val);

    /* In C (standard, e.g. on x86) we can implement isNan(float x)
        as return x != x,
        while for Connex we really have to check values of mantissas and
        exponents:

       Inspired from Ercegovac_Digital_Arithmetic_2004, page 416:
        - a NaN for f16 has the following values:
          If E = 31 and F != 0 (and sign can be 0 or 1), then v = NAN (not a number).


       Inspired from https://www.doc.ic.ac.uk/~eedwards/compsys/float/nan.html:
        In general (f32, f64, f16), a signalling NaN (NANS) has the extra property:
            - F is between 1 and largest value for which 2nd-MSB is 0:
                F is any of the following values 1 .. 1011 1111...1
        A quiet NaN (NANQ) has the extra property:
            - F is between 1 and largest value for which 2nd-MSB is 1:
                F is any of the following values 1011 1111...1 .. 1111 1111...1
    */

    int mantissa = val & F32_MANTISSA_MASK;
    int exp = (val & F32_EXPONENT_MASK) >> F32_MANTISSA_BITS;
    // printf("isnan_f16(): exp = %d\n", exp);
    // printf("isnan_f16(): mantissa = %d\n", mantissa);

    if ( (exp == (1UL << F32_EXPONENT_BITS) - 1) &&
         (mantissa != 0)) {
        //printf("isnan_f32(): returning true\n");
        return true;
    }

    //printf("isnan_f32(): returning false\n");
    return false;
}



std::string GetStringForF16(int16_t val) {
    std::string res;

    if (isnan_f16(val)) {
        res = "NAN ";
    }
    else
    if (val == F16_INF_POSITIVE) {
        res = "F16_INF_POSITIVE ";
    }
    else
    if (val == (short)F16_INF_NEGATIVE) {
        res = "F16_INF_NEGATIVE ";
    }

    int sign = (val < 0);
    int mantissa = val & F16_MANTISSA_MASK;
    int exponent = (val & F16_EXPONENT_MASK) >> F16_MANTISSA_BITS;

    char strAux[100];
    sprintf(strAux, "0x%04hx(S=%d,E=0x%x,F=0x%x)", val, sign, exponent,
            (exponent != 0 && exponent != 2 * F16_EXPONENT_BIAS + 1) ? F16_HIDDENBIT_MASK | mantissa:
             mantissa);
    res = res + strAux;

    /*
    (result[i] == F16_INF_POSITIVE || result[i] == (short)F16_INF_NEGATIVE) ?
    ((result[i] == F16_INF_POSITIVE) ? "F16_INF_POSITIVE" : "F16_INF_NEGATIVE") : ""
    */

    return res;
}


// This updates SRC1_MANTISSA, SRC1_EXPONENT
/*
   This procedure is used for floating point ADD/SUB, MUL and DIV.i16,
                                             DIV.f16/f32.

   Note: The LLVM IR has intrinsic llvm.ctlz doing exactly this:
    https://llvm.org/docs/LangRef.html#llvm-ctlz-intrinsic
   So I think it is extremely worthy to put in Connex ISA a CTLZ (maybe takeout
     POPCNT) instruction.

  Note: LLVM IR has llvm.ctlz intrinsic
    - see http://llvm.org/docs/LangRef.html#llvm-ctlz-intrinsic
*/
void CountLeadingZeros(Kernel *__kernel,
                       int VAL, // The value for CTLZ; register is updated
                       // The result, set to RES_IF_VAL_ZERO if VAL == 0
                       int NUM_BITS,
                       /* We use RES_IF_VAL_ZERO because we subtract after
                        we call result of CountLeadingZeros() from
                        RES_IF_VAL_ZERO because we want to compute the
                        number of significant bits (in VAL, normally). */
                       int RES_IF_VAL_ZERO,
                       int CT1,
                       int CT16,
                       int AUX,
                       int AUX_CONTINUE // We don't require this normally
                      ) {
  #define OR_SCAN_BITS_FOR_RATHER_EFFICIENT_CTLZ

    /* We compute the number of leading zeros in VAL
       - we do this in order to compute in the end the number of
       significant bits in the result of mantissa multiplication
       (max 16 + 16 = 32 bits).
      Note that if R(VAL) == 0 we return R(RES_IF_VAL_ZERO).
    */
   PrintRegDebug(VAL);

  #ifdef OR_SCAN_BITS_FOR_RATHER_EFFICIENT_CTLZ
    /* Using an OR-prefix/scan operation on the bits of x - inspired from:
         https://stackoverflow.com/questions/23856596/how-to-count-leading-zeros-in-a-32-bit-unsigned-integer:
       x = x | (x >> 1);
       x = x | (x >> 2);
       x = x | (x >> 4);
       x = x | (x >> 8);
       x = x | (x >>16);
       return pop(~x);

       It requires 10 cycles.
    */
    R(AUX) = R(VAL) >> 1;
    R(VAL) |= R(AUX);
    //
    R(AUX) = R(VAL) >> 2;
    R(VAL) |= R(AUX);
    //
    R(AUX) = R(VAL) >> 4;
    R(VAL) |= R(AUX);
    //
    R(AUX) = R(VAL) >> 8;
    R(VAL) |= R(AUX);
    //
    // We now compute the leading zero bits in R(VAL):
    R(VAL) = ~R(VAL);
    R(NUM_BITS) = POPCNT( R(VAL) );
  #else

    /*
    // This is a correct implementation which requires ~150 cycles.
    R(AUX_CONTINUE) = 1;
    R(NUM_BITS) = 1;

    for (int idx = 15; idx >= 0; idx--) { //TODO: think if we should take out case idx == 0
        R(AUX) = R(VAL) >> idx;
        R(AUX) &= R(CT1);
       PrintRegDebug(AUX);
        R(AUX) = R(AUX) == R(CT0); // PRED3);
        // We continue only if the current tested bit is 0
        R(AUX_CONTINUE) &= R(AUX);
        R(AUX) = R(AUX_CONTINUE) == R(CT1);
        NOP;
      //);
      EXECUTE_WHERE_EQ(
        R(NUM_BITS) += R(CT1);
      );

      EXECUTE_IN_ALL(
      );
    } // end host-side for loop

    //R(NUM_BITS) = R(CT16) - R(NUM_BITS);
    */

     //#define CONNEX_HAS_BITREVERSE
     #ifdef CONNEX_HAS_BITREVERSE_NOT_RECOMMENDED_ANYMORE_USE_OR_SCAN_BITS_FOR_RATHER_EFFICIENT_CTLZ
        /*
         IMPORTANT NOTE: If we reverse the bits of R(SRC1_MANTISSA)
           quickly (e.g., in 1 cycle) then we can count the trailing zeros
           with POPCNT(x ^ (x - 1)) - 1 .
            10101000 xor
            10100111
            --------
            00001111 --> POPCNT = 4 --> res = 3
          However:
            00...00000 xor
            11...11111
            --------
            11...11111 --> POPCNT = 16 --> res = 15
              (well here it is NOT correct, since it should be res = 16)
        */
        R(NUM_BITS) = BITREVERSE(R(VAL));
        PrintDebugMessage("Bit-reverse for VAL:");
        PrintRegDebug(NUM_BITS);
        PrintRegDebug(RES_IF_VAL_ZERO);
        R(AUX) = R(NUM_BITS) - R(CT1);
        R(NUM_BITS) ^= R(AUX);
        PrintDebugMessage("  XORed value containing number of bits of 1 equal "
                          "to result CTLZ + 1:");
        PrintRegDebug(NUM_BITS);
        R(NUM_BITS) = POPCNT(R(NUM_BITS));
        //
        // Treating the special case VAL = 0
        R(AUX) = R(VAL) == R(CT0);
        NOP;
      //);
      EXECUTE_WHERE_EQ(
        R(NUM_BITS) = R(RES_IF_VAL_ZERO); // We plan to make NUM_BITS 0.
      );
      EXECUTE_IN_ALL(
        PrintRegDebug(NUM_BITS);
      );
     #endif // CONNEX_HAS_BITREVERSE
  #endif

     PrintDebugMessage("Exiting CountLeadingZeros():");
     PrintRegDebug(NUM_BITS);

  #undef OR_SCAN_BITS_FOR_RATHER_EFFICIENT_CTLZ
} // END CountLeadingZeros()


std::string GetStringForF32(int32_t val) {
    std::string res;

    if (isnan_f32(val)) {
        res = "NAN ";
    }
    else
    if (val == F32_INF_POSITIVE) {
        res = "F32_INF_POSITIVE ";
    }
    else
    if (val == F32_INF_NEGATIVE) {
        res = "F32_INF_NEGATIVE ";
    }

    int sign = (val < 0);
    int mantissa = val & F32_MANTISSA_MASK;
    int exponent = (val & F32_EXPONENT_MASK) >> F32_MANTISSA_BITS;

    char strAux[100];
    sprintf(strAux, "0x%08x(S=%d,E=0x%x,F=0x%x)(%.6g)", val, sign, exponent,
            ((exponent != 0) && (exponent != 2 * F32_EXPONENT_BIAS + 1)) ?
            F32_HIDDENBIT_MASK | mantissa : mantissa, *((float *)&val));
    res += strAux;

    /*
    (result[i] == F16_INF_POSITIVE || result[i] == (short)F16_INF_NEGATIVE) ?
    ((result[i] == F16_INF_POSITIVE) ? "F16_INF_POSITIVE" : "F16_INF_NEGATIVE") : ""
    */

    return res;
}

/*
 * Put in separate registers the sign, exponent, mantissa from the f16 value.
 * Note: the hidden bit for the mantissa is not set if:
 *    - E == 0 (zeros, denormals) or
 *    - E == 31 (inf, NAN).
 */
void UnpackF16(Kernel *__kernel,
               int CT0, int CT1, int CT31,
               int SRC, int SRC_SIGN, int SRC_EXPONENT,
               int SRC_MANTISSA,
               int SIGN_MASK, int EXPONENT_MASK, int MANTISSA_MASK,
               int HIDDENBIT_MASK,
               //int F16_MANTISSA_BITS,
               int PRED, int PREDA, int PRED3,
               bool normalizeForDiv=false,
               int AUX=-1) {

    /* We do not put here EXECUTE_IN_ALL() because it actually represents an
      END_WHERE instruction. */

    /*
    Inspired from [Ercegovac_Digital_Arithmetic_2004, page 416].
    For half (or f16) type: S(1), E(5), F(10)
        (a) If 1 <= E <= 30, then v = (-1)^S * 2^(E-15) * (1.F) (normalized fp number).
        (b) If E = 31 and F != 0, then v = NAN (not a number).
        (c) If E = 31 and F = 0, then v = (-1)^S inf (plus and minus infinity).
        (d) If E = 0 and F != 0, then v = (-1)^S * 2^(-14) * (0.F) (denormal, also called gradual underflow).
        (e) If E = 0 and F = 0, then v = (-1)^S * 0 (positive and negative zero).

      We must not set the HIDDENBIT_MASK for: denormals and zero.
      I guess depending if we have a MUL.f16 or an ADD.f16, we must not set the
        HIDDENBIT_MASK for NAN and INF:
          - more exactly: for INF it helps not to set the HIDDENBIT_MASK
                          for MUL.f16 because a 0 result mantissa is IMMEDIATELY
                          recognized as an INF result.
    */

    /*
    printf("Entered UnpackF16()\n");
    fflush(stdout);
    */

    /*
    Note: Preprocessing for denormals takes ~7 instructions more for
                the unpack of the operand.
    */


    // R(SRC_SIGN) contains the sign on bit 15
    R(SRC_SIGN) = R(SRC) & R(SIGN_MASK);
   PrintRegDebug(SRC_SIGN);

    R(SRC_EXPONENT) = R(SRC) & R(EXPONENT_MASK);

   PrintDebugMessage("SRC_EXPONENT:");
    // Get the exponent from bit 0 (shift down to LSB).
    R(SRC_EXPONENT) >>= F16_MANTISSA_BITS;
   PrintRegDebug(SRC_EXPONENT);

    R(SRC_MANTISSA) = R(SRC) & R(MANTISSA_MASK);
   PrintDebugMessage("SRC_MANTISSA:");

    // Setting hidden bit (eventually!!!! for later clear with XOR for the mantissa - see below)
    //R(SRC_MANTISSA) |= R(HIDDENBIT_MASK);

    // We make PRED = Mantissa != 0:
    R(PRED) = R(CT0) < R(SRC_MANTISSA);
    // We check for Exp == 0:
    R(PREDA) = R(SRC_EXPONENT) == R(CT0);
    // We make PRED3 hold Exp = 0 and Mantissa != 0:
    R(PRED3) = R(PREDA) & R(PRED);
    R(PRED3) = R(PRED3) == R(CT1);
   PrintRegDebug(PRED3);
    NOP;

  // In case of denormal:
  EXECUTE_WHERE_EQ(
    //R(SRC_EXPONENT) += R(CT1);
      R(SRC_EXPONENT) = 1;

    if (normalizeForDiv == true) {
      //R(SRC_MANTISSA) <<= 1;
      R(PRED) = R(SRC_MANTISSA);
      CountLeadingZeros(__kernel,
                        //SRC_MANTISSA,
                        PRED, // value for CTLZ (it is updated)
                        PRED, // result
                        -1,
                        CT1,
                        -1,
                        AUX,
                        -1);
      R(AUX) = 5;
      R(AUX) = R(PRED) - R(AUX);
      R(SRC_EXPONENT) -= R(AUX);
      R(SRC_MANTISSA) <<= R(AUX);
    }

    // Takeout hidden bit for the mantissa (from bit 0, as it is initially)
    //R(SRC_MANTISSA) ^= R(HIDDENBIT_MASK);
  );
  EXECUTE_IN_ALL(
    // If NOT(E == 0 or E == 31) we set the hidden bit:
    R(PRED) = R(SRC_EXPONENT) == R(CT31);
   PrintRegDebug(PREDA);
   PrintRegDebug(PRED);
    // We reuse PREDA
    R(PRED) |= R(PREDA);
    R(PRED) = R(PRED) == R(CT0); //R(CT1);
   PrintRegDebug(PRED);
    NOP;
  );
  EXECUTE_WHERE_EQ(
    R(SRC_MANTISSA) |= R(HIDDENBIT_MASK);
  );


    /*
    [Ercegovac_Digital_Arithmetic_2004]
    8.4.5 Denormal and Zero Operands
      "When an operand is a denormal number (E = 0 and F != 0), then
    there is no hidden 1. Consequently, the operand of addition should
    be set to E = 1 and 0.F .
    The rest of the algorithm remains unchanged."
    */

   /*
    // We check for exponent == 0 or exponent == 31:
    R(PREDA) = R(SRC_EXPONENT) == R(CT0);
    R(PRED) = R(SRC_EXPONENT) == R(CT31);
    R(PRED) |= R(PREDA);
    R(PRED) = R(PRED) == R(CT1);
    // If E == 0 or E == 31 we make sure that finally we don't set the hidden bit.
    NOP;
  EXECUTE_WHERE_EQ(
    // Prepare hidden bit for later clear with XOR for the mantissa - see below
    R(SRC_MANTISSA) |= R(HIDDENBIT_MASK);
   PrintDebugMessage("UnpackF16():");
   PrintRegDebug(SRC_MANTISSA);
  );

  EXECUTE_IN_ALL(
    // We make PRED now mantissa != 0 (F != 0)
    //R(PRED) = R(CT1) - R(PRED);
    R(PRED) = R(CT0) < R(SRC_MANTISSA);

    // Increment exp - useful ONLY for denormals, else undone below.
    R(SRC_EXPONENT) += R(CT1);

    // Note: we reuse PREDA (exponent == 0) computed above.
   PrintRegDebug(PREDA);
   PrintRegDebug(PRED);
    // We make PRED3 hold NOT(E = 0 and F != 0):
    R(PRED3) = R(PREDA) & R(PRED);
    R(PRED3) = R(PRED3) == R(CT0);
   PrintRegDebug(PRED3);
    NOP;
  );
  // In all cases except denormals:
  EXECUTE_WHERE_EQ(
    R(SRC_EXPONENT) -= R(CT1);
    // Toggle hidden bit for the mantissa
    R(SRC_MANTISSA) ^= R(HIDDENBIT_MASK);
  );
   */

   PrintDebugMessage("SRC_MANTISSA final:");
   PrintRegDebug(SRC_MANTISSA);
   //
   PrintDebugMessage("SRC_EXPONENT final:");
   PrintRegDebug(SRC_EXPONENT);

  EXECUTE_IN_ALL();
}



/*
 * Put in separate registers the sign, exponent, mantissa from the f32 value.
 * Note: the hidden bit for the mantissa is not set if:
 *    - E == 0 (zeros, denormals) or
 *    - E == 255 (inf, NAN).
 */
void UnpackF32(Kernel *__kernel,
               int CT0, int CT1, int CT255,
               int SRC, int SRC_SIGN, int SRC_EXPONENT,
               int SRC_MANTISSA_H, int SRC_MANTISSA_L,
               int SIGN_MASK, int EXPONENT_MASK, int MANTISSA_MASK,
               int HIDDENBIT_MASK,
               //int F32_MANTISSA_BITS,
               int PRED, int PREDA, int PRED3,
               bool caseDivF32=false,
               int AUX=-1, int AUX2=-1,
               int CT15=-1, int CT16=-1,
               int PRED4=-1, int PRED5=-1) {

    /* We do not put here EXECUTE_IN_ALL() because it actually represents an
      END_WHERE instruction. */

    /*
    Inspired from [Ercegovac_Digital_Arithmetic_2004, page 416].
    For float (or f32) type: S(1), E(8), F(23)
        (a) If 1 <= E <= 254, then v = (-1)^S * 2^(E-127) * (1.F) (normalized fp number).
        (b) If E = 255 and F != 0, then v = NAN (not a number).
        (c) If E = 255 and F = 0, then v = (-1)^S inf (plus and minus infinity).
        (d) If E = 0 and F != 0, then v = (-1)^S * 2^(-126) * (0.F) (denormal, also called gradual underflow).
        (e) If E = 0 and F = 0, then v = (-1)^S * 0 (positive and negative zero).

      Small-Note: The denormals are like this to offer "continuity" between
        very small standard f.p. numbers and denormals. That's why we have
        v = (-1)^S * 2^(-126) * (0.F),
        while for the "closest" standard f.p. numbers we have:
        (-1)^S * 2^(1-127) * (1.F) = (-1)^S * 2^(-126) * (1.F).

      We must not set the HIDDENBIT_MASK for: denormals and zero.
      I guess depending if we have a MUL.f32 or an ADD.f32, we must not set the
        HIDDENBIT_MASK for NAN and INF:
          - more exactly: for INF it helps not to set the HIDDENBIT_MASK
                          for MUL.f32 because a 0 result mantissa is IMMEDIATELY
                          recognized as an INF result.
    */

    /*
    #define F32_MANTISSA_BITS 23
    #define F32_EXPONENT_BITS 8
    #define F32_EXPONENT_MASK 0x7F80
    */

    /*
    printf("Entered UnpackF32()\n");
    fflush(stdout);
    */

    /*
    Note: Preprocessing for denormals takes ~7 instructions more for
                the unpack of the operand.
    */

    PrintDebugMessage("Entered UnpackF32():");

    R(SRC_MANTISSA_L) = R(SRC);
    PrintDebugReg(SRC_MANTISSA_L);

    PrintDebugReg(SRC);
    CELL_SHL(R(SRC), R(CT1));
    // NOP is required
    NOP;
    R(SRC) = SHIFT_REG;
    PrintDebugMessage("UnpackF32: After CELL_SHL:");
    PrintDebugReg(SRC);

    // R(SRC_SIGN) contains the sign on bit 15
    // IMPORTANT: SRC_SIGN is stored only on the EVEN indices of the vector reg.
    R(SRC_SIGN) = R(SRC) & R(SIGN_MASK);
    PrintDebugReg(SRC_SIGN);

    R(SRC_EXPONENT) = R(SRC) & R(EXPONENT_MASK);
    PrintDebugMessage("UnpackF32: After & R(EXPONENT_MASK):");
    PrintDebugReg(SRC_EXPONENT);

    // Get the exponent from bit 0 (shift down to LSB).
    R(SRC_EXPONENT) >>= F32_MANTISSA_BITS_I16;
    PrintDebugMessage("UnpackF32: After SHR F32_MANTISSA_BITS_I16:");
    PrintDebugReg(SRC_EXPONENT);

    R(SRC_MANTISSA_H) = R(SRC);

    R(PRED3) = INDEX;
    R(PRED3) &= R(CT1);
    R(PRED) = R(PRED3) == R(CT0);
    NOP;
  // For even indices (for the lanes):
  //  small MEGA-TODO: We can probably avoid doing this check and run this code
  //       on all the lanes.
  EXECUTE_WHERE_EQ(
    R(SRC_MANTISSA_H) = R(SRC) & R(MANTISSA_MASK);
  );
  EXECUTE_IN_ALL(
    PrintDebugMessage("UnpackF32: SRC_MANTISSA_H:");
    PrintDebugReg(SRC_MANTISSA_H);

    // We make PRED = Mantissa != 0:
    R(PRED3) = R(SRC_MANTISSA_L) | R(SRC_MANTISSA_H);
    PrintDebugReg(PRED3);
    /*
    R(PRED) = R(CT0) == R(PRED3);
    R(PRED) = R(CT1) - R(PRED);
    */
    R(PRED) = ULT(R(CT0), R(PRED3));
    PrintDebugReg(PRED);
    // We check for Exp == 0:
    R(PREDA) = R(SRC_EXPONENT) == R(CT0);
    // We make PRED3 hold Exp == 0 and Mantissa != 0:
    R(PRED3) = R(PREDA) & R(PRED);
    // For even indices for the lanes:
    R(PREDA) = INDEX;
    R(PREDA) &= R(CT1);
    R(PREDA) = R(PREDA) == R(CT0);
    R(PRED3) &= R(PREDA);

    PrintRegDebug(PRED3);
    if (caseDivF32 == false) {
      R(PRED3) = R(PRED3) == R(CT1);
    }
    else { // if (caseDivF32 == true)
      R(PRED4) = R(SRC_MANTISSA_H) == R(CT0);
      R(PRED4) &= R(PRED3); // PRED4 = SRC_MANTISSA_H == CT0 && denormal num:
      PrintDebugMessage("UnpackF32: PRED4 = SRC_MANTISSA_H==CT0 && denormal num:");
      PrintRegDebug(PRED4);
      PrintRegDebug(SRC_MANTISSA_H);
      PrintRegDebug(PRED3);
      PrintRegDebug(AUX);
      R(PRED4) = R(PRED4) == R(CT1);
      // In case of denormal/subnormal with R(SRC_MANTISSA_H) == R(CT0):
    }
    NOP;
  );
  EXECUTE_WHERE_EQ(
    if (caseDivF32 == false) {
      R(SRC_EXPONENT) = 1;
    }
    else { // if (caseDivF32 == true)
      // We implement here softfloat_normSubnormalF32Sig().

      R(PRED) = R(SRC_MANTISSA_L);
      CountLeadingZeros(__kernel,
                        PRED, // value for CTLZ (it is updated)
                        PRED, // result
                        -1,
                        CT1,
                        -1,
                        AUX,
                        -1);

      PrintRegDebug(PRED);

      // Note: AUX is shiftDist (from softfloat_normSubnormalF32Sig()).
      R(AUX) = 8 - 16;
      PrintDebugMessage("UnpackF32(): AUX = 8 - 16:");
      PrintRegDebug(AUX);
      R(AUX) = R(PRED) - R(AUX);
      PrintRegDebug(AUX);
      R(SRC_EXPONENT) = R(CT1) - R(AUX);
      PrintDebugMessage("UnpackF32(): shiftDist (AUX):");
      PrintDebugReg(AUX);
      PrintDebugReg(SRC_EXPONENT);
  );
  EXECUTE_IN_ALL(
      // We treat 2 cases for MANTISSA << AUX: now AUX > 15 and then AUX < 16.
      PrintDebugReg(PRED5);
      R(PRED5) = R(CT15) < R(AUX);
      R(PRED5) &= R(PRED4);
      R(PRED5) = R(PRED5) == R(CT1);
      PrintDebugMessage("UnpackF32(): before 2nd WHERE block:");
      PrintDebugReg(PRED5);
      PrintDebugReg(PRED4);
      PrintDebugReg(SRC_MANTISSA_L);
      PrintDebugReg(SRC_MANTISSA_H);
      NOP;
  );
  EXECUTE_WHERE_EQ( // AUX > 15
      R(SRC_MANTISSA_H) = R(SRC_MANTISSA_L);
      R(SRC_MANTISSA_L) = 0;
      PrintDebugMessage("UnpackF32(): at 2nd WHERE block:");
      PrintDebugReg(SRC_MANTISSA_L);
      PrintDebugReg(SRC_MANTISSA_H);
      R(AUX2) = R(AUX) - R(CT16);
      PrintDebugReg(AUX2);
      R(SRC_MANTISSA_H) <<= R(AUX2);
      PrintDebugReg(SRC_MANTISSA_H);
  );
  EXECUTE_IN_ALL(
      R(PRED5) = R(AUX) < R(CT16); // AUX < 16
      // We reuse PRED4 - see above (PRED4 = SRC_MANTISSA_H == CT0 &&
      //                             denormal num:)
      R(PRED4) &= R(PRED5);
      R(PRED4) = R(PRED4) == R(CT1);
      NOP;
  );
  EXECUTE_WHERE_EQ(
      // Now we treat the second case AUX < 16
      R(AUX2) = R(CT16) - R(AUX);
      R(AUX2) = R(SRC_MANTISSA_L) >> R(AUX2);
      R(SRC_MANTISSA_L) <<= R(AUX);
      R(SRC_MANTISSA_H) <<= R(AUX);
      R(SRC_MANTISSA_H) |= R(AUX2);
  );
  EXECUTE_IN_ALL(
      R(PRED4) = ULT(R(CT0), R(SRC_MANTISSA_H));
      // We reuse PRED3 - see above ("PRED3 hold Exp == 0 and Mantissa != 0")
      R(PRED4) &= R(PRED3);
      R(PRED4) = R(PRED4) == R(CT1);
      NOP;
  );
  EXECUTE_WHERE_EQ(
      R(PRED) = R(SRC_MANTISSA_H);
      CountLeadingZeros(__kernel,
                        PRED, // value for CTLZ (it is updated)
                        PRED, // result
                        -1,
                        CT1,
                        -1,
                        AUX,
                        -1);

      // Note: AUX is shiftDist.
      R(AUX) = 8;
      R(AUX) = R(PRED) - R(AUX);
      R(SRC_EXPONENT) = R(CT1) - R(AUX);

      // We SHL mantissa by shiftDist positions:
      R(AUX2) = R(CT16) - R(AUX);
      R(AUX2) = R(SRC_MANTISSA_L) >> R(AUX2);
      R(SRC_MANTISSA_L) <<= R(AUX);
      R(SRC_MANTISSA_H) <<= R(AUX);
      R(SRC_MANTISSA_H) |= R(AUX2);
    } // END else case // if (caseDivF32 == true)
  );
  EXECUTE_IN_ALL(
    /*
    [Ercegovac_Digital_Arithmetic_2004]
    8.4.5 Denormal and Zero Operands
      "When an operand is a denormal number (E = 0 and F != 0), then
    there is no hidden 1. Consequently, the operand of addition should
    be set to E = 1 and 0.F .
    The rest of the algorithm remains unchanged."
    */
  if (caseDivF32 == false) {
        // If NOT(E == 0 or E == 255) we set the hidden bit for the mantissa:
        R(PRED) = R(SRC_EXPONENT) == R(CT255);
        PrintRegDebug(PRED);
        // We reuse PREDA (= R(SRC_EXPONENT) == R(CT0)) - see above.
        PrintRegDebug(PREDA);
        R(PRED) |= R(PREDA); // PRED = E==255 | E==0
        R(PRED) = R(PRED) == R(CT0);
        PrintRegDebug(PRED);
        NOP;
      );
      EXECUTE_WHERE_EQ(
        R(SRC_MANTISSA_H) |= R(HIDDENBIT_MASK);
      );
      EXECUTE_IN_ALL();
  }
    PrintDebugMessage("At the end of UnpackF32():");
    //
    PrintDebugMessage("SRC_EXPONENT final:");
    PrintRegDebug(SRC_EXPONENT);
    //
    PrintDebugMessage("SRC_MANTISSA_L final:");
    PrintRegDebug(SRC_MANTISSA_L);
    //
    PrintDebugMessage("SRC_MANTISSA_H final:");
    PrintRegDebug(SRC_MANTISSA_H);
    //
    PrintRegDebug(SRC_SIGN);
    //
    PrintRegDebug(SRC);
}





int GenRandF32Valid(int reMax = 0, int reMin = -10) {
    #define MAX_STRLEN 100
    char str[MAX_STRLEN];
    int rm = rand();
    // The exponent in decimal: 10^-3..10^3
    // NOT tried much: int re = rand() % 28 - 13;
    // This is too big for MatMul_128_f16: int re = rand() % 25 - 12;
    // GOOD for MatMul_128_f16: int re = rand() % 6 - 3; // Too big --> errors when converting to __fp16: 28 - 13; //30 - 14;
    // Reasonably big for MatMul_128_f16: int re = rand() % 10 - 6; // Values in range -6..3
    // Values too small - OK for MatMul_128_f16: int re = rand() % 8 - 10; // Values in range -10..-3
    //
    //int re = rand() % 14 - 10; // Values in range -10..3. I consider this to be the best test for MatMul_128_f16
    int re = rand() % (reMax - reMin + 1) + reMin; // Values in range -10..3. I consider this to be the best test for MatMul_128_f16

    // BOTH positive and negative numbers:
    sprintf(str, "%s0.%dE%d", (rand() % 2)? "-" : "", rm, re);
    //
    // ONLY positive numbers:
    //sprintf(str, "0.%dE%d", rm, re);

    printf("GenRandF32Valid():\n");
    printf("  str = %s", str);

    float resF32;
    // Gives error: sscanf(str, "%g", &res);
    sscanf(str, "%g\n", &resF32);

    printf("  (resF32 = %8g)\n", resF32);
    //printf("  res = %.7f\n", res);
    /* Note: the conversion from float to __fp16 can lose significantly:
       e.g., float is -18042.89383 while __fp16 is -18048 (so an error of
         ~5.1 units - reason is __fp16 uses an exponent 0xd to represent such
         a big mantissa of 18042 and then changing by 1 the lsb of the mantissa
         represents changing by 2^(13-10) = 2^3 = 8).
    */
    //printf("    res = %8g\n", res);
    //printf("  res = %s (used GetStringForF16())\n", GetStringForF16(* ((short *)&res)).c_str());

    //return res;
    return *((int *)&resF32);
}





#endif // LIB_MISC_H_INCLUDED

