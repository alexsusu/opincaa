#ifndef LIB_MISC_H_INCLUDED
#define LIB_MISC_H_INCLUDED


#include <stdint.h>
#include <string.h>
//#include <string>


#define F16_MANTISSA_BITS 10
#define F16_EXPONENT_BITS 5
//
#define F16_MANTISSA_MASK  0x03FF
#define F16_EXPONENT_MASK  0x7C00
#define F16_SIGN_MASK      0x8000
#define F16_HIDDENBIT_MASK 0x0400
#define F16_EXPONENT_BIAS  15
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
//
//#define F16_MAX_EXP 0x1F
#define F16_MAX_EXP ((1UL << F16_EXPONENT_BITS) - 1)


#define F32_MANTISSA_BITS 23
#define F32_EXPONENT_BITS 8
//
#define F32_MANTISSA_MASK  0x007FFFFF
#define F32_EXPONENT_MASK  0x7F800000
#define F32_SIGN_MASK      0x80000000
#define F32_HIDDENBIT_MASK 0x00800000
#define F32_EXPONENT_BIAS  127
//
// "Main" NaN value:
#define F32_NAN_1        0x7F800001
#define F32_NAN_2        0xFF800001
//#define F32_NAN_3 0xFC80
//#define F32_NAN_4 0xFE00
#define F32_INF_POSITIVE 0x7F800000
#define F32_INF_NEGATIVE 0xFF800000
//
#define F32_MAX_EXP 0xFF
#define F32_BIAS 127




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
        //
        //return true;
        return 1;
    }

    //printf("isnan_f16(): returning false\n");
    //
    //return false;
    return 0;
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
        //printf("isnan_f16(): returning true\n");
        //
        //return true;
        return 1;
    }

    //printf("isnan_f32(): returning false\n");
    //
    //return false;
    return 0;
}



//std::string GetStringForF16(short val) {
char *GetStringForF16(short val) {
    //std::string res;
    static char res[256];

    res[0] = 0;

    if (isnan_f16(val)) {
        //res = "NAN ";
        strcpy(res, "NAN ");
    }
    else
    if (val == F16_INF_POSITIVE) {
        //res = "F16_INF_POSITIVE ";
        strcpy(res, "F16_INF_POSITIVE ");
    }
    else
    if (val == (short)F16_INF_NEGATIVE) {
        //res = "F16_INF_NEGATIVE ";
        strcpy(res, "F16_INF_NEGATIVE ");
    }

    int sign = (val < 0);
    int mantissa = val & F16_MANTISSA_MASK;
    int exponent = (val & F16_EXPONENT_MASK) >> F16_MANTISSA_BITS;

    char strAux[100];
    sprintf(strAux, "0x%04hx(S=%d,E=0x%x,F=0x%x)", val, sign, exponent,
            (exponent != 0 && exponent != 31) ? F16_HIDDENBIT_MASK | mantissa:
             mantissa);
    //res = res + strAux;
    strcat(res, strAux);

    /*
    (result[i] == F16_INF_POSITIVE || result[i] == (short)F16_INF_NEGATIVE) ?
    ((result[i] == F16_INF_POSITIVE) ? "F16_INF_POSITIVE" : "F16_INF_NEGATIVE") : ""
    */

    return res;
}


// This updates SRC1_MANTISSA, SRC1_EXPONENT
/*
   This procedure is used for floating point ADD/SUB, MUL and DIV.i16.

   Note: The LLVM IR has intrinsic llvm.ctlz doing exactly this:
    https://llvm.org/docs/LangRef.html#llvm-ctlz-intrinsic
   So I think it is extremely worthy to put in Connex ISA a CTLZ (maybe takeout
     POPCNT) instruction.

  Note: LLVM IR has llvm.ctlz intrinsic
    - see http://llvm.org/docs/LangRef.html#llvm-ctlz-intrinsic
*/

#endif // LIB_MISC_H_INCLUDED

