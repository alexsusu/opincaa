#define F16_MANTISSA_BITS 10
#define F16_EXPONENT_BITS 5
//
#define F16_MANTISSA_MASK  0x03FF
#define F16_EXPONENT_MASK  0x7C00
#define F16_SIGN_MASK      0x8000
#define F16_HIDDENBIT_MASK 0x0400
//
// "Main" NaN value:
#define F16_NAN 0x7C01
#define F16_NAN_2 0xFC01
#define F16_INF_POSITIVE 0x7C00
#define F16_INF_NEGATIVE 0xFC00




int isnan_f16(short val) {
//    printf("isnan_f16(): val = %hd\n", val);

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
//    printf("isnan_f16(): exp = %d\n", exp);
//    printf("isnan_f16(): mantissa = %d\n", mantissa);

    if ( (exp == (1UL << F16_EXPONENT_BITS) - 1) &&
         (mantissa != 0)) {
        //printf("isnan_f16(): returning true\n");
        return true;
    }

    //printf("isnan_f16(): returning false\n");
    return false;
}


std::string GetStringForF16(short val) {
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

    char strAux[20];
    sprintf(strAux, "0x%04hx", val);
    res = res + strAux;

    /*
    (result[i] == F16_INF_POSITIVE || result[i] == (short)F16_INF_NEGATIVE) ?
    ((result[i] == F16_INF_POSITIVE) ? "F16_INF_POSITIVE" : "F16_INF_NEGATIVE") : ""
    */

    return res;
}


void CountNumberLeadingZeros() {
}


void UnpackF16(Kernel *__kernel,
                    int REG_CT0, int REG_CT1,
                    int REG_SRC, int REG_SRC_SIGN, int REG_SRC_EXPONENT,
                    int REG_SRC_MANTISSA,
                    int REG_SIGN_MASK, int REG_EXPONENT_MASK, int REG_MANTISSA_MASK,
                    int REG_HIDDENBIT_MASK,
                    //int F16_MANTISSA_BITS,
                    int REG_PRED, int REG_PREDA, int REG_PRED3) {
    printf("Entered UnpackF16()\n");
    fflush(stdout);

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


    // R(REG_SRC_SIGN) contains the sign on bit 15
    R(REG_SRC_SIGN) = R(REG_SRC) & R(REG_SIGN_MASK);
 PrintRegDebug(REG_SRC_SIGN);

    R(REG_SRC_EXPONENT) = R(REG_SRC) & R(REG_EXPONENT_MASK);

    // Get the exponent from bit 0 (shift down to LSB)
    R(REG_SRC_EXPONENT) >>= F16_MANTISSA_BITS;
    PrintRegDebug(REG_SRC_EXPONENT);
    // exponent == 0?
    R(REG_PREDA) = R(REG_SRC_EXPONENT) == R(REG_CT0);

    R(REG_SRC_MANTISSA) = R(REG_SRC) & R(REG_MANTISSA_MASK);
    // mantissa != 0?
    R(REG_PRED) = R(REG_SRC_MANTISSA) == R(REG_CT0);

  /* For all INFinities we make sure that finally we don't set the
       hidden bit */
    NOP;
  EXECUTE_WHERE_EQ(
    // Prepare hidden bit for clear for the mantissa
    R(REG_SRC_MANTISSA) |= R(REG_HIDDENBIT_MASK);
  PrintDebugMessage("UnpackF16():");
  PrintRegDebug(REG_SRC_MANTISSA);
  );

  EXECUTE_IN_ALL(
    R(REG_PRED) = R(REG_CT1) - R(REG_PRED);

    // Increment exp - useful ONLY for denormals, otherwise undone below
    R(REG_SRC_EXPONENT) += R(REG_CT1);

    R(REG_PRED3) = R(REG_PREDA) & R(REG_PRED);
  PrintRegDebug(REG_PREDA);
  PrintRegDebug(REG_PRED);
    R(REG_PRED3) = R(REG_PRED3) == R(REG_CT0);
  PrintRegDebug(REG_PRED3);
    NOP;
  );
  // In all cases except denormals
  EXECUTE_WHERE_EQ(
    R(REG_SRC_EXPONENT) -= R(REG_CT1);
    // Add hidden bit for the mantissa (from bit 0, as it is initially)
    R(REG_SRC_MANTISSA) ^= R(REG_HIDDENBIT_MASK);
    PrintRegDebug(REG_SRC_MANTISSA);
  );

  EXECUTE_IN_ALL();
}

