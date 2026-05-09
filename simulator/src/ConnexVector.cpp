/****************************************************************************
 * File:   ConnexVector.cpp
 *
 * A class mapping on a Connex Vector.
 *
 */


#include "ConnexVector.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



//#define SIM_DONT_ALLOW_OOB_BITWISE_SHIFTS

/****************************************************************************
 * Help macro to ease defining the binary operators
 */
#define BINARY_OP(op)                                               \
BINARY_OP_COMMON_START(op)                                          \
BINARY_OP_COMMON_END(op)

/****************************************************************************
 * Help macro to ease defining the binary operators (overwrites flags)
 */

// result.cells[i] = (* ((short *)&cells[i])) op (* ((short *)&anotherVector.cells[i]));
#define BINARY_OP_COMMON_START(op)                                  \
  ConnexVector ConnexVector::operator op(ConnexVector &anotherVector)  \
  {                                                                   \
      ConnexVector result;                                            \
      for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++)                  \
      {                                                               \
        result.cells[i] = cells[i] op anotherVector.cells[i];


#define BINARY_OP_COMMON_END(op)                                    \
      }                                                               \
      return result;                                                  \
  }

#define BINARY_OP_FLAGS_EQ(op)                                      \
        if (cells[i] == anotherVector.cells[i])                     \
            connexStateObj.eqFlag.cells[i] = 1;                     \
        else connexStateObj.eqFlag.cells[i] = 0;

#define BINARY_OP_FLAGS_LT(op)                                      \
        if ((ConnexVectorElementType)cells[i] < (ConnexVectorElementType)anotherVector.cells[i])   \
            connexStateObj.ltFlag.cells[i] = 1;                              \
        else connexStateObj.ltFlag.cells[i] = 0;

/* IMPORTANT note: twos-complement integer numbers don't care about the sign
                   bit when computing carry after add operation */
#define BINARY_OP_FLAGS_CARRY_ADD(op)                               \
        if ( ((unsigned int)((UnsignedConnexVectorElementType)cells[i])) +    \
             ((unsigned int)((UnsignedConnexVectorElementType)anotherVector.cells[i])) > \
                (unsigned int)REG_MAX_VAL )                         \
            connexStateObj.carryFlag.cells[i] = 1;                  \
        else                                                        \
            connexStateObj.carryFlag.cells[i] = 0;

#define BINARY_OP_FLAGS_CARRY_ADDC(op)                              \
        if ( ((unsigned int)((UnsignedConnexVectorElementType)cells[i])) +    \
             ((unsigned int)((UnsignedConnexVectorElementType)connexStateObj.carryFlag.cells[i])) + \
             ((unsigned int)((UnsignedConnexVectorElementType)anotherVector.cells[i])) > \
                (unsigned int)REG_MAX_VAL )  \
            connexStateObj.carryFlag.cells[i] = 1;                  \
        else                                                        \
            connexStateObj.carryFlag.cells[i] = 0;

/* For subtraction, carry is actually borrow (for subtraction with borrow)
     - see e.g. https://en.wikipedia.org/wiki/Carry_flag */
#define BINARY_OP_FLAGS_CARRY_SUB(op)                               \
        if ( ((unsigned int)((UnsignedConnexVectorElementType)cells[i])) <    \
            ((unsigned int)((UnsignedConnexVectorElementType)anotherVector.cells[i])) ) { \
            connexStateObj.carryFlag.cells[i] = 1; }                          \
        else connexStateObj.carryFlag.cells[i] = 0;

#define BINARY_OP_FLAGS_CARRY_SUBC(op)                           \
        if ( ((unsigned int)((UnsignedConnexVectorElementType)cells[i])) < \
             ((unsigned int)((UnsignedConnexVectorElementType)anotherVector.cells[i])) + \
                connexStateObj.carryFlag.cells[i] ) {  \
            connexStateObj.carryFlag.cells[i] = 1; }                         \
        else connexStateObj.carryFlag.cells[i] = 0;


/* IMPORTANT NOTE: here we treat the carries for the ops defined, but
    the actual semantics of the ops is implemented in ConnexSimulator.cpp */

#define BINARY_OP_FLAGS_LIKE_ADD(op)                                \
BINARY_OP_COMMON_START(op)                                          \
    BINARY_OP_FLAGS_EQ(op)                                          \
    BINARY_OP_FLAGS_LT(op)                                          \
    BINARY_OP_FLAGS_CARRY_ADD(op)                                   \
BINARY_OP_COMMON_END(op)

#define BINARY_OP_FLAGS_LIKE_ADDC(op)                               \
BINARY_OP_COMMON_START(op)                                          \
    BINARY_OP_FLAGS_EQ(op)                                          \
    BINARY_OP_FLAGS_LT(op)                                          \
    BINARY_OP_FLAGS_CARRY_ADDC(op)                                  \
BINARY_OP_COMMON_END(op)

#define BINARY_OP_FLAGS_LIKE_SUB(op)                                \
BINARY_OP_COMMON_START(op)                                          \
    BINARY_OP_FLAGS_EQ(op)                                          \
    BINARY_OP_FLAGS_LT(op)                                          \
    BINARY_OP_FLAGS_CARRY_SUB(op)                                   \
BINARY_OP_COMMON_END(op)

#define BINARY_OP_FLAGS_LIKE_SUBC(op)                               \
BINARY_OP_COMMON_START(op)                                          \
    BINARY_OP_FLAGS_EQ(op)                                          \
    BINARY_OP_FLAGS_LT(op)                                          \
    BINARY_OP_FLAGS_CARRY_SUBC(op)                                  \
BINARY_OP_COMMON_END(op)


ConnexState connexStateObj;
//
/*
inline bool ConnexVector::IsCellActive(int index) {
    return (connexStateObj.active.cells[index] == 1) &&
        (connexStateObj.cellDisabled.cells[index] == 0);
}
*/
#define IsCellActive(index) ((connexStateObj.active.cells[index] == 1) && (connexStateObj.cellDisabled.cells[index] == 0))

/*inline*/ int ConnexVector::getNumberActiveLanes() {
    int res = 0;

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        if (IsCellActive(i))
        res++;
    }

    return res;
}



/****************************************************************************
 * Constructor for creating a new ConnexVector
 */
ConnexVector::ConnexVector() {
    // 2018_02_10
    /*
    fflush(stdout);
    assert(CONNEX_VECTOR_LENGTH == 1024);
    */
  #ifdef DEBUG_OPINCAA_EXTRA_ACTIVE
    printf("Entered ConnexVector::ConnexVector(): CONNEX_VECTOR_LENGTH = %d\n",
           CONNEX_VECTOR_LENGTH);
  #endif

    cells = (ConnexVectorElementType *)malloc(CONNEX_VECTOR_LENGTH * sizeof(ConnexVectorElementType));
    assert(cells != NULL);

  #ifdef DEBUG_OPINCAA_EXTRA
    printf("Entered ConnexVector::ConnexVector(): this = %p, "
           "cells = %p (after malloc())\n", this, cells);
    fflush(stdout);
  #endif

    /* Alex: adding this initialization to avoid valgrind give
               errors like: "Use of uninitialised value" */
    memset(cells, 0, sizeof(ConnexVectorElementType) * CONNEX_VECTOR_LENGTH);
}

// 2018_03_27
ConnexVector::ConnexVector(const ConnexVector &anotherVector) {
    cells = (ConnexVectorElementType *)malloc(CONNEX_VECTOR_LENGTH * sizeof(ConnexVectorElementType));
    assert(cells != NULL);

  #ifdef DEBUG_OPINCAA_EXTRA
    printf("Entered ConnexVector::ConnexVector(&): this = %p, "
           "cells = %p (after malloc())\n", this, cells);
    fflush(stdout);
  #endif

    memcpy(cells, anotherVector.cells,
           sizeof(ConnexVectorElementType) * CONNEX_VECTOR_LENGTH);
}


/****************************************************************************
 * Destructor for the ConnexVector class
 */
ConnexVector::~ConnexVector() {
    // 2018_02_10
  #ifdef DEBUG_OPINCAA_EXTRA
    printf("Entered ConnexVector::~ConnexVector(): this = %p, "
           "cells = %p\n", this, cells);
    fflush(stdout);
  #endif
  /*
  */
  if (cells != NULL) {
   #ifdef DEBUG_OPINCAA_EXTRA
    printf("  ConnexVector::~ConnexVector(): calling free(cells)\n");
   #endif
    free(cells);
    cells = NULL;
   #ifdef DEBUG_OPINCAA_EXTRA
    printf("    --> ConnexVector::~ConnexVector(): cells = %p "
           "(this = %p)\n", cells, this);
   #endif
  }
  // free(cells);
}





inline int ConnexVector::SelectOnCellActive(int index, int valT, int valF) {
    bool pred = IsCellActive(index); //connexStateObj.active.cells[index] ||connexStateObj.cellDisabled.cells[index];

    if (pred == 1)
        return valT;

    return valF;
}

/****************************************************************************
 * Computes the sum-reduction of this Vector
 *
 * @return the value of the reduction operation
 */
int ConnexVector::reduce() {
    int sum = 0;

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        //sum += cells[i];
        // Reduction is performed only on the selected cells - Lucian said it's OK
        //sum += IsCellActive(i) * cells[i];
        //sum += SelectOnCellActive(i, this->cells[i], 0);
        if (IsCellActive(i))
            sum += this->cells[i];

        //printf("sum = %d\n", sum);
    }
    return sum;
}

int ConnexVector::reduce_u() {
    int sum = 0;

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        /* Alex: we do this typecast to avoid sign extension (at least on x86)
             from i16 to i32. */
        // This works with our example RED_i32
        //sum += *((unsigned short *)(&cells[i]));
        // Reduction is performed only on the selected cells - Lucian said it's OK
        //sum += connexStateObj.active.cells[i] * ((UnsignedConnexVectorElementType)cells[i]);
        //sum += SelectOnCellActive(i, (UnsignedConnexVectorElementType)this->cells[i], 0);
        if (IsCellActive(i))
            sum += (UnsignedConnexVectorElementType)this->cells[i];

        //printf("reduce_u(): sum = %d\n", sum);
    }
    return sum;
}

ConnexVector ConnexVector::scan() {
    ConnexVector result;

    result.cells[0] = this->cells[0];
    printf("scan(): this->cells[0] = %d\n", this->cells[0]);

    for (int i = 1; i < CONNEX_VECTOR_LENGTH; i++) {
        /* We can take out this if since operator=() takes care of predication.
        But it's more correct like this. */
        if (IsCellActive(i)) {
            result.cells[i] = result.cells[i - 1] + this->cells[i];
        }

        printf("scan(): this->cells[%d] = %d\n", i, this->cells[i]);
        printf("scan(): result.cells[%d] = %d\n", i, result.cells[i]);
        fflush(stdout);
    }

    return result;
}


/****************************************************************************
 * Loads each cell with its index in the array
 */
void ConnexVector::loadIndex() {
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        this->cells[i] = SelectOnCellActive(i, i, this->cells[i]);
    }
}

/****************************************************************************
 * Loads the specified values in the vector's cells
 *
 * @param data the array of ConnexVectorElementTypes to load
 */
void ConnexVector::write(ConnexVectorElementType *data) {
    memcpy(this->cells, data, CONNEX_VECTOR_LENGTH * sizeof(ConnexVectorElementType));
}

/****************************************************************************
 * Return the data contains in all cells as a ConnexVectorElementType data
 *
 * @return the array of ConnexVectorElementTypes taken from each cell
 */
ConnexVectorElementType *ConnexVector::read() {
    return cells;
}

/****************************************************************************
 * Binary operators (except assignment)
 * These are not conditioned by active flags
 */

// Defining ConnexVector::operator+(ConnexVector anotherVector),
//    not conditioned by active flags
BINARY_OP_FLAGS_LIKE_ADD(+)

// Defining ConnexVector::operator-(ConnexVector anotherVector),
//    not conditioned by active flags
BINARY_OP_FLAGS_LIKE_SUB(-)

// Defining IMPLICIT (no method body required - C++ compiler takes care of) ConnexVector::operator<<(ConnexVector anotherVector),
//    not conditioned by active flags
// Alex: we declare ::shl() below instead: BINARY_OP_FLAGS_LIKE_ADD(<<)

// TODO (Alex): NOT sure if 100% correct
// Defining IMPLICIT (no method body required - C++ compiler takes care of) ConnexVector::operator>>(ConnexVector anotherVector),
//    not conditioned by active flags
// Alex: ::shr() was already declared: BINARY_OP_FLAGS_LIKE_SUB(>>)

// Defining ConnexVector::operator==(ConnexVector anotherVector),
//    not conditioned by active flags
BINARY_OP_FLAGS_LIKE_ADD(==)

// TODO (Alex): NOT sure if 100% correct
// Defining ConnexVector::operator<(ConnexVector anotherVector),
//    not conditioned by active flags
BINARY_OP_FLAGS_LIKE_SUB(<)

// TODO (Alex): NOT sure if 100% correct
// Defining ConnexVector::operator|(ConnexVector anotherVector),
//    not conditioned by active flags
BINARY_OP_FLAGS_LIKE_SUB(|)

// TODO (Alex): NOT sure if 100% correct
// Defining ConnexVector::operator&(ConnexVector anotherVector),
//    not conditioned by active flags
BINARY_OP_FLAGS_LIKE_ADDC(&)

// TODO (Alex): NOT sure if 100% correct
// Defining ConnexVector::operator^(ConnexVector anotherVector),
//    not conditioned by active flags
BINARY_OP_FLAGS_LIKE_SUBC(^)



/****************************************************************************
 * Assignment operator, conditioned by active flags
 */
// 2018_03_27
void ConnexVector::operator=(ConnexVector &anotherVector) {
//void ConnexVector::operator=(ConnexVector &anotherVector) const
  #ifdef DEBUG_OPINCAA_EXTRA
    printf("Entered ConnexVector::operator=(&anotherVector): cells = %p, "
           "anotherVector.cells = %p (this = %p, anotherVector = %p)\n",
           this->cells, anotherVector.cells, this, anotherVector);
  #endif

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        //this->cells[i] = SelectOnCellActive(i, anotherVector.cells[i], this->cells[i]);
        if (IsCellActive(i)) {
            /* IMPORTANT: even if some of the operands are defined implicitly by the C++
             compiler we make sure here that only for the active lanes we writeback the
             values in the registers, thus ensuring the semantics of active cells is
             preserved. */
            this->cells[i] = anotherVector.cells[i];

          #ifdef DEBUG_OPINCAA_EXTRA_ACTIVE
            printf("  operator=(&): cell %d enabled\n", i);
          #endif
        }
      #ifdef DEBUG_OPINCAA_EXTRA
        printf("cells[%d] = %d\n", i, cells[i]);
      #endif
    }
}
void ConnexVector::operator=(ConnexVector &&anotherVector) {
//void ConnexVector::operator=(ConnexVector &anotherVector) const
  #ifdef DEBUG_OPINCAA_EXTRA
    printf("Entered ConnexVector::operator=(&&): cells = %p, "
           "anotherVector.cells = %p (this = %p, anotherVector = %p)\n",
           this->cells, anotherVector.cells, this, anotherVector);
  #endif

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        //this->cells[i] = SelectOnCellActive(i, anotherVector.cells[i], this->cells[i]);
        if (IsCellActive(i)) {
            this->cells[i] = anotherVector.cells[i];

          #ifdef DEBUG_OPINCAA_EXTRA_ACTIVE
            printf("  operator=(&&): cell %d enabled\n", i);
          #endif
        }
    }
}


/*
 * Copy vector not taking selection into account.
 */
void ConnexVector::copyFrom(ConnexVector &anotherVector) {
  #ifdef DEBUG_OPINCAA_EXTRA
    printf("Entered ConnexVector::copyFrom(): cells = %p, anotherVector = %p\n",
            cells, anotherVector.cells);
  #endif

    memcpy(this->cells, anotherVector.cells, CONNEX_VECTOR_LENGTH * sizeof(ConnexVectorElementType));
}


/****************************************************************************
 * Assignment operator (used only for reset of active) // Alex TODO: Should be removed - this is similar to Unconditioned_Set(..., bool)
 */
void ConnexVector::operator=(bool value) {
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        if (value == true)
            connexStateObj.active.cells[i] = 1;
        else
            connexStateObj.active.cells[i] = 0;
    }
}

/****************************************************************************
 * Assignment operator (for vload insn), conditioned by active flags
 */
void ConnexVector::operator=(ConnexVectorElementType value) {
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        //this->cells[i] = SelectOnCellActive(i, value, this->cells[i]);
        if (IsCellActive(i)) {
            this->cells[i] = value;
        }
    }
}

/****************************************************************************
 * Multiplication operator, not conditioned by active flags
 */
void ConnexVector::operator*(ConnexVector &anotherVector) {
    int result;

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
      /* We can take out this if since operator=() takes care of predication.
        But it's more correct like this. */
      if (IsCellActive(i)) {
        // TAKEOUT: Alex: DON'T understand why this comment :((( <<WRONG for negative i16:>>

        result = cells[i] * anotherVector.cells[i];
        connexStateObj.multLow.cells[i] = (ConnexVectorElementType)result;
        connexStateObj.multHigh.cells[i] = (result >> 16) & 0xFFFF;
      }
    }
}

/****************************************************************************
 * Unsigned-multiplication operator, not conditioned by active flags
 */
void ConnexVector::mult_u(ConnexVector &anotherVector) {
    unsigned int result;

    /*
    assert(0 &&
           "This is NOT following Connex ISA exactly - "
           "talk a bit with Lucian P. and decide what to do exactly.");
    */

    // We have an unsigned multiplier (From Jul 2017 we saw it is required e.g. for efficient i32 mul)
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
      /* We can take out this if since operator=() takes care of predication.
        But it's more correct like this. */
      if (IsCellActive(i)) {
        // WRONG for negative i16: result = cells[i] * anotherVector.cells[i];
        result = ((UnsignedConnexVectorElementType)cells[i]) *
                 ((UnsignedConnexVectorElementType)anotherVector.cells[i]);

        //printf("result = 0x%08x\n", result);
        connexStateObj.multLow.cells[i] = (ConnexVectorElementType)result;
        connexStateObj.multHigh.cells[i] = (((unsigned)result) >> 16);
      }
    }
}


/****************************************************************************
 * Unary negation operator, not conditioned by active flags
 */
ConnexVector ConnexVector::operator~() {
    ConnexVector result;

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
      /* We can take out this if since operator=() takes care of predication.
        But it's more correct like this. */
        if (IsCellActive(i)) {
            result.cells[i] = ~(this->cells[i]);
        }
    }

    return result;
}

/****************************************************************************
 *
 * We implement for "exploration" bit-reversal - not conditioned by active flags.
 */
ConnexVector ConnexVector::bitreverse() {
    ConnexVector result;

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
      /* We can take out this if since operator=() takes care of predication.
        But it's more correct like this. */
      if (IsCellActive(i)) {
        ConnexVectorElementType val = this->cells[i];
        ConnexVectorElementType res = 0;
        UnsignedConnexVectorElementType resBit = 1UL << 15;

        for (int bitIndex = 0; bitIndex < sizeof(ConnexVectorElementType) * 8; bitIndex++) {
            if ((val & 1) == 1)
                res |= resBit;
            resBit >>= 1;
            val >>= 1;
        }

        result.cells[i] = res;
      }
    }

    return result;
}

/****************************************************************************
 * Unsigned less than, not conditioned by active flags
 */
ConnexVector ConnexVector::ult(ConnexVector &anotherVector) {
    ConnexVector result;

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        /* We can take out this if since operator=() takes care of predication.
          But it's more correct like this. */
        if (IsCellActive(i)) {
            result.cells[i] = (unsigned)this->cells[i] < (unsigned)anotherVector.cells[i];
        }
    }
    return result;
}

/****************************************************************************
 * Shift left with immediate value, not conditioned by active flags
 */
ConnexVector ConnexVector::operator<<(unsigned short value) {
    ConnexVector result;

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        /* We can take out this if since operator=() takes care of predication.
          But it's more correct like this. */
        if (IsCellActive(i)) {
          #ifdef SIM_DONT_ALLOW_OOB_BITWISE_SHIFTS
            assert(value <= 15 && "We don't advise giving ISHL with more than 15 positions on Connex - [TODO: give a good reason why for SHL].");
            assert(value >= 0 && "We don't advise giving ISHL with less than 0 positions on Connex - [TODO: give a good reason why for SHL].");
          #endif

            // See tests in /home/alarm/Experiments/Test_SHR_special_2nd_opnd .

            result.cells[i] = this->cells[i] << value;
        }
    }

    return result;
}

/****************************************************************************
 * Shift right with immediate value, not conditioned by active flags
 */
ConnexVector ConnexVector::operator>>(unsigned short value) {
    ConnexVector result;

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        /* We can take out this if since operator=() takes care of predication.
          But it's more correct like this. */
        if (IsCellActive(i)) {
            if (value > 15) {
                printf("ISHR: value = %d\n", value);
                fflush(stdout);
            }

          #ifdef SIM_DONT_ALLOW_OOB_BITWISE_SHIFTS
            assert(value <= 15 && "We don't advise giving ISHR with more than 15 positions on Connex - 16+ positions leave the source operand unchanged instead of making it 0");
            assert(value >= 0 && "We don't advise giving ISHR with less than 0 positions on Connex");
          #endif

            // See tests in /home/alarm/Experiments/Test_SHR_special_2nd_opnd .

            result.cells[i] = ((unsigned short)this->cells[i]) >> value;
        }
    }

    return result;
}

/****************************************************************************
 * Shl (logical), needs casting to unsigned, not conditioned by active flags
 */
ConnexVector ConnexVector::shl(ConnexVector &anotherVector) {
    ConnexVector result;

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        /* We can take out this if since operator=() takes care of predication.
          But it's more correct like this. */
        if (IsCellActive(i)) {
            if (anotherVector.cells[i] > 15) {
                printf("SHL: cells[%d] = %d\n", i, anotherVector.cells[i]);
                fflush(stdout);
            }
            if (anotherVector.cells[i] < 0) {
                printf("SHL: cells[%d] = %d\n", i, anotherVector.cells[i]);
                fflush(stdout);
            }
          #ifdef SIM_DONT_ALLOW_OOB_BITWISE_SHIFTS
            assert(anotherVector.cells[i] <= 15 && "We don't advise giving SHL with more than 15 positions on Connex - 16+ positions leave the source operand unchanged instead of making it 0.");
            assert(anotherVector.cells[i] >= 0 && "We don't advise giving SHL with less than 0 positions on Connex - we obtained undefined behavior.");
          #endif

            result.cells[i] = (this->cells[i]) << anotherVector.cells[i];
        }
    }

    return result;
}

/****************************************************************************
 * Shr (logical), needs casting to unsigned, not conditioned by active flags
 */
ConnexVector ConnexVector::shr(ConnexVector &anotherVector) {
    ConnexVector result;

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        /* We can take out this if since operator=() takes care of predication.
          But it's more correct like this. */
        if (IsCellActive(i)) {
            if (anotherVector.cells[i] > 15) {
                printf("SHR: cells[%d] = %d\n", i, anotherVector.cells[i]);
                fflush(stdout);
            }
            if (anotherVector.cells[i] < 0) {
                printf("SHR: cells[%d] = %d\n", i, anotherVector.cells[i]);
                fflush(stdout);
            }

          #ifdef SIM_DONT_ALLOW_OOB_BITWISE_SHIFTS
            assert(anotherVector.cells[i] <= 15 && "We don't advise giving SHR with more than 15 positions on Connex - 16+ positions leave the source operand unchanged instead of making it 0.");
            assert(anotherVector.cells[i] >= 0 && "We don't advise giving SHR with less than 0 positions on Connex - we obtained undefined behavior.");
          #endif

            // See tests in /home/alarm/Experiments/Test_SHR_special_2nd_opnd .

            result.cells[i] = ((unsigned short)this->cells[i]) >> anotherVector.cells[i];
        }
    }

    return result;
}

/****************************************************************************
 * Shr (logical), needs casting to unsigned, not conditioned by active flags
 */
ConnexVector ConnexVector::shra(ConnexVector &anotherVector) {
    ConnexVector result;

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        /* We can take out this if since operator=() takes care of predication.
          But it's more correct like this. */
        if (IsCellActive(i)) {
            if (anotherVector.cells[i] > 15) {
                printf("SHRA: cells[%d] = %d\n", i, anotherVector.cells[i]);
                fflush(stdout);
            }
            if (anotherVector.cells[i] < 0) {
                printf("SHRA: cells[%d] = %d\n", i, anotherVector.cells[i]);
                fflush(stdout);
            }

          #ifdef SIM_DONT_ALLOW_OOB_BITWISE_SHIFTS
            assert(anotherVector.cells[i] <= 15 && "We don't advise giving SHRA with more than 15 positions on Connex - 16+ positions leave the source operand unchanged instead of making it 0.");
            assert(anotherVector.cells[i] >= 0 && "We don't advise giving SHRA with less than 0 positions on Connex - we obtained undefined behavior.");
          #endif

            result.cells[i] = this->cells[i] >> anotherVector.cells[i];
        }
    }

    return result;
}

/****************************************************************************
 * Shift right (arithmetic) with immediate value, needs casting to signed,
 * not conditioned by active flags
 */
ConnexVector ConnexVector::ishra(unsigned short value) {
    ConnexVector result;

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        /* We can take out this if since operator=() takes care of predication.
          But it's more correct like this. */
        if (IsCellActive(i)) {
            if (value > 15) {
                printf("ISHR: value = %d\n", value);
                fflush(stdout);
            }

          #ifdef SIM_DONT_ALLOW_OOB_BITWISE_SHIFTS
            assert(value <= 15 && "We don't advise giving ISHRA with more than 15 positions on Connex - 16+ positions leave the source operand unchanged instead of making it 0");
            assert(value >= 0 && "We don't advise giving ISHRA with less than 0 positions on Connex");
          #endif

            // See tests in /home/alarm/Experiments/Test_SHR_special_2nd_opnd .

            result.cells[i] = ((ConnexVectorElementType)this->cells[i]) >> value;
        }
    }
    return result;
}

/****************************************************************************
* Computes the population count (number of set bits) in each element of the
* argument vector. Argument is treated as unsigned, result is unsigned
*/
ConnexVector ConnexVector::popcount() {
    ConnexVector result;
    ConnexVectorElementType arg;
    unsigned short count;

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        /* We can take out this if since operator=() takes care of predication.
          But it's more correct like this. */
        if (IsCellActive(i)) {
            arg = (ConnexVectorElementType)this->cells[i];
            count = 0;
            for (int j = 0; j < CONNEX_REGISTER_SIZE; j++) {
                count += arg & 1;
                arg = arg >> 1;
            }
            result.cells[i] = count;
        }
    }
    return result;
}

/****************************************************************************
 * Shift the vector in the specified direction, with the number
 * of cells specified by ConnexVector::shiftCount
 *
 * @param direction the direction: -1 if the shift is right and
 *                                  1 if the shift is left
 *
 */
void ConnexVector::shift(int direction) {
    int i;
    bool done;
    ConnexVector tmp;


    /*
    printf("ConnexVector::shift(): CONNEX_VECTOR_LENGTH = %d\n",
                                           CONNEX_VECTOR_LENGTH);
    printf("ConnexVector::shift(): &CONNEX_VECTOR_LENGTH = %p\n",
                                           &CONNEX_VECTOR_LENGTH);
    */

    // Alex: adding simple copy for case CELLSH has shift-operand 0
    for (i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        tmp.cells[i] = cells[i];
    }

    /*
    for (i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        printf("tmp.cells[%d] = 0x%0hx\n", i, tmp.cells[i]);
    }
    for (i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        printf("shiftCountReg.cells[%d] = %hd\n",
               i, connexStateObj.shiftCountReg.cells[i]);
    }
    */

    do {
        done = true;

        for (i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
            if (connexStateObj.shiftCountReg.cells[i] > 0) {
                tmp.cells[i] = cells[(i + direction) &
                                        (CONNEX_VECTOR_LENGTH - 1)];
                connexStateObj.shiftCountReg.cells[i]--;
            }

            done = done && (!connexStateObj.shiftCountReg.cells[i]);
        }

        memcpy(cells, tmp.cells, CONNEX_VECTOR_LENGTH * sizeof(ConnexVectorElementType));

        /*
        for (i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
            printf("cells[%d] = 0x%0hx\n", i, cells[i]);
        }
        */
    }
    while (!done);
}

/****************************************************************************
 * Reads this vector from the localStore, using addresses vector for addresses
 *
 * @param localStore the local store to read from
 * @param addresses the addresses to load from
 */
void ConnexVector::loadFrom(ConnexVector *localStore, ConnexVector &addresses) {
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        // Alex: checking for out-of-bounds cases
        if (IsCellActive(i)) {

          #ifdef DEBUG_OPINCAA_EXTRA_ACTIVE
            printf("  loadFrom(): cell %d enabled\n", i);
          #endif

            if ( !(addresses.cells[i] >= 0 &&
                   addresses.cells[i] < CONNEX_MEM_NUM_ROWS + CONNEX_MEM_NUM_ROWS_EXTRA)) {
                printf("Read access outside of bounds of Connex LS memory for "
                       "lane i = %d: addresses.cells[i] = %d\n",
                       i, addresses.cells[i]);
                // TODO: a bit inefficient
                assert(addresses.cells[i] >= 0 &&
                       addresses.cells[i] < CONNEX_MEM_NUM_ROWS + CONNEX_MEM_NUM_ROWS_EXTRA &&
                       "Read access outside of bounds of Connex LS memory "
                       "for lane i");
            }

            this->cells[i] = localStore[addresses.cells[i]].cells[i];
        }
    }
}

/****************************************************************************
 * Writes this vector to the localStore, using addresses vector for addresses
 *
 * @param localStore the local store to write to
 * @param addresses the addresses to write to
 */
void ConnexVector::storeTo(ConnexVector *localStore, ConnexVector &addresses)
{
    for (int i = 0; i<CONNEX_VECTOR_LENGTH; i++) {
        // Alex: checking for out-of-bounds cases
        if (IsCellActive(i)) {
            if ( !(addresses.cells[i] >= 0 &&
                   addresses.cells[i] < CONNEX_MEM_NUM_ROWS + CONNEX_MEM_NUM_ROWS_EXTRA)) {
                printf("Write access outside of bounds of Connex LS memory "
                       "for lane i = %d: addresses.cells[i] = %d\n",
                       i, addresses.cells[i]);
                // TODO: a bit inefficient
                assert(addresses.cells[i] >= 0 &&
                       addresses.cells[i] < CONNEX_MEM_NUM_ROWS + CONNEX_MEM_NUM_ROWS_EXTRA &&
                       "Write access outside of bounds of Connex LS memory "
                       "for lane i");
            }

            localStore[addresses.cells[i]].cells[i] = this->cells[i];
        }
    }
}

void ConnexVector::Unconditioned_Set(ConnexVector &dstVector, ConnexVector &srcVector) {
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        dstVector.cells[i] = srcVector.cells[i];

      #ifdef DEBUG_OPINCAA_EXTRA
        printf("dstVector.cells[%d] = %d\n", i, dstVector.cells[i]);
      #endif
    }
}

void ConnexVector::Unconditioned_Set(ConnexVector &dstVector, bool value) {
    if (value == true)
        for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
            dstVector.cells[i] = 1;

          #ifdef DEBUG_OPINCAA_EXTRA
            printf("dstVector.cells[%d] = %d\n", i, dstVector.cells[i]);
          #endif
        }
    else
        for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
            dstVector.cells[i] = 0;

          #ifdef DEBUG_OPINCAA_EXTRA
            printf("dstVector.cells[%d] = %d\n", i, dstVector.cells[i]);
          #endif
        }
}

void ConnexVector::Print() {
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++)
        printf("%d ", this->cells[i]);
    printf("\n");
}
