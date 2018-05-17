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

// Alex: replaced types of vectors from unsigned short to TYPE_ELEMENT

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
        if ((TYPE_ELEMENT)cells[i] < (TYPE_ELEMENT)anotherVector.cells[i])   \
            connexStateObj.ltFlag.cells[i] = 1;                              \
        else connexStateObj.ltFlag.cells[i] = 0;

/* IMPORTANT note: twos-complement integer numbers don't care about the sign
                   bit when computing carry after add operation */
#define BINARY_OP_FLAGS_CARRY_ADD(op)                               \
        if ( ((unsigned int)((UNSIGNED_TYPE_ELEMENT)cells[i])) +    \
             ((unsigned int)((UNSIGNED_TYPE_ELEMENT)anotherVector.cells[i])) > \
                (unsigned int)REG_MAX_VAL )                         \
            connexStateObj.carryFlag.cells[i] = 1;                  \
        else                                                        \
            connexStateObj.carryFlag.cells[i] = 0;

#define BINARY_OP_FLAGS_CARRY_ADDC(op)                              \
        if ( ((unsigned int)((UNSIGNED_TYPE_ELEMENT)cells[i])) +    \
             ((unsigned int)((UNSIGNED_TYPE_ELEMENT)connexStateObj.carryFlag.cells[i])) + \
             ((unsigned int)((UNSIGNED_TYPE_ELEMENT)anotherVector.cells[i])) > \
                (unsigned int)REG_MAX_VAL )  \
            connexStateObj.carryFlag.cells[i] = 1;                  \
        else                                                        \
            connexStateObj.carryFlag.cells[i] = 0;

/* For subtraction, carry is actually borrow (for subtraction with borrow)
     - see e.g. https://en.wikipedia.org/wiki/Carry_flag */
#define BINARY_OP_FLAGS_CARRY_SUB(op)                               \
        if ( ((unsigned int)((UNSIGNED_TYPE_ELEMENT)cells[i])) <    \
            ((unsigned int)((UNSIGNED_TYPE_ELEMENT)anotherVector.cells[i])) ) { \
            connexStateObj.carryFlag.cells[i] = 1; }                          \
        else connexStateObj.carryFlag.cells[i] = 0;

#define BINARY_OP_FLAGS_CARRY_SUBC(op)                           \
        if ( ((unsigned int)((UNSIGNED_TYPE_ELEMENT)cells[i])) < \
             ((unsigned int)((UNSIGNED_TYPE_ELEMENT)anotherVector.cells[i])) + \
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


/****************************************************************************
 * Constructor for creating a new ConnexVector
 */
ConnexVector::ConnexVector() {
    // 2018_02_10
    /*
    fflush(stdout);
    assert(CONNEX_VECTOR_LENGTH == 1024);
    */
    cells = (TYPE_ELEMENT *)malloc(CONNEX_VECTOR_LENGTH * sizeof(TYPE_ELEMENT));
    assert(cells != NULL);
  #ifdef ALEX_DEBUG
    printf("Entered ConnexVector::ConnexVector(): this = %p, "
           "cells = %p (after malloc())\n", this, cells);
    fflush(stdout);
  #endif

    /* Alex: adding this initialization to avoid valgrind give
               errors like: "Use of uninitialised value" */
    memset(cells, 0, sizeof(TYPE_ELEMENT) * CONNEX_VECTOR_LENGTH);
}

// 2018_03_27
ConnexVector::ConnexVector(const ConnexVector &anotherVector) {
    cells = (TYPE_ELEMENT *)malloc(CONNEX_VECTOR_LENGTH * sizeof(TYPE_ELEMENT));
    assert(cells != NULL);

  #ifdef ALEX_DEBUG
    printf("Entered ConnexVector::ConnexVector(&): this = %p, "
           "cells = %p (after malloc())\n", this, cells);
    fflush(stdout);
  #endif

    memcpy(cells, anotherVector.cells,
           sizeof(TYPE_ELEMENT) * CONNEX_VECTOR_LENGTH);
}


/****************************************************************************
 * Destructor for the ConnexVector class
 */
ConnexVector::~ConnexVector() {
    // 2018_02_10
  #ifdef ALEX_DEBUG
    printf("Entered ConnexVector::~ConnexVector(): this = %p, "
           "cells = %p\n", this, cells);
    fflush(stdout);
  #endif
  /*
  */
  if (cells != NULL) {
   #ifdef ALEX_DEBUG
    printf("  ConnexVector::~ConnexVector(): calling free(cells)\n");
   #endif
    free(cells);
    cells = NULL;
   #ifdef ALEX_DEBUG
    printf("    --> ConnexVector::~ConnexVector(): cells = %p "
           "(this = %p)\n", cells, this);
   #endif
  }
  // free(cells);
}

/****************************************************************************
 * Computes the reduction of this Vector
 *
 * @return the value of the reduction operation
 */
int ConnexVector::reduce() {
    int sum = 0;
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
      #define REDUCE_TAKES_U16_VALUES_INSTEAD_OF_I16_TO_WORK_FOR_I32_REDUCE

      #ifdef REDUCE_TAKES_U16_VALUES_INSTEAD_OF_I16_TO_WORK_FOR_I32_REDUCE
        /* Alex: we do this typecast to avoid sign extension (at least on x86)
             from i16 to i32. */
        // This works with our example RED_i32
        //sum += *((unsigned short *)(&cells[i]));
        // Reduction is performed only on the selected cells - Lucian said it's OK
        sum += connexStateObj.active.cells[i] *
                 ((UNSIGNED_TYPE_ELEMENT)cells[i]);
      #else
        //sum += cells[i];
        // Reduction is performed only on the selected cells - Lucian said it's OK
        sum += connexStateObj.active.cells[i] * cells[i];
      #endif

        //printf("sum = %d\n", sum);
    }
    return sum;
}

/****************************************************************************
 * Loads each cell with its index in the array
 */
void ConnexVector::loadIndex()
{
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        cells[i] = (connexStateObj.active.cells[i] * i) |
                   (!connexStateObj.active.cells[i] * cells[i]);
    }
}

/****************************************************************************
 * Loads the specified values in the vector's cells
 *
 * @param data the array of TYPE_ELEMENTs to load
 */
void ConnexVector::write(TYPE_ELEMENT *data)
{
    memcpy(cells, data, CONNEX_VECTOR_LENGTH * sizeof(TYPE_ELEMENT));
}

/****************************************************************************
 * Return the data contains in all cells as a TYPE_ELEMENT data
 *
 * @return the array of TYPE_ELEMENTs taken from each cell
 */
TYPE_ELEMENT *ConnexVector::read() {
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

// Defining ConnexVector::operator<<(ConnexVector anotherVector),
//    not conditioned by active flags
BINARY_OP_FLAGS_LIKE_ADD(<<)

// TODO (Alex): NOT sure if 100% correct
// Defining ConnexVector::operator>>(ConnexVector anotherVector),
//    not conditioned by active flags
BINARY_OP_FLAGS_LIKE_SUB(>>)

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
//void ConnexVector::operator=(ConnexVector &anotherVector) const {
  #ifdef ALEX_DEBUG
    printf("Entered ConnexVector::operator=(&): cells = %p, "
           "anotherVector.cells = %p (this = %p, anotherVector = %p)\n",
           cells, anotherVector.cells, this, anotherVector);
  #endif

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        cells[i] = (connexStateObj.active.cells[i] * anotherVector.cells[i]) |
                   (!connexStateObj.active.cells[i] * cells[i]);
    }
}
void ConnexVector::operator=(ConnexVector &&anotherVector) {
//void ConnexVector::operator=(ConnexVector &anotherVector) const {
  #ifdef ALEX_DEBUG
    printf("Entered ConnexVector::operator=(&&): cells = %p, "
           "anotherVector.cells = %p (this = %p, anotherVector = %p)\n",
           cells, anotherVector.cells, this, anotherVector);
  #endif

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        cells[i] = (connexStateObj.active.cells[i] * anotherVector.cells[i]) |
                   (!connexStateObj.active.cells[i] * cells[i]);
    }
}


/*
 * Copy vector not taking selection into account.
 */
void ConnexVector::copyFrom(ConnexVector &anotherVector) {
  #ifdef ALEX_DEBUG
    printf("Entered ConnexVector::copyFrom(): cells = %p, anotherVector = %p\n",
            cells, anotherVector.cells);
  #endif

    memcpy(cells, anotherVector.cells, CONNEX_VECTOR_LENGTH * sizeof(TYPE_ELEMENT));
}


/****************************************************************************
 * Assignment operator (used only for reset of active)
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
void ConnexVector::operator=(TYPE_ELEMENT value) {
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        cells[i] = (connexStateObj.active.cells[i] * value) |
                   (!connexStateObj.active.cells[i] * cells[i]);
    }
}

/****************************************************************************
 * Multiplication operator, not conditioned by active flags
 */
void ConnexVector::operator*(ConnexVector &anotherVector) {
    int result;

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        // TAKEOUT: Alex: DON'T understand why this comment :((( <<WRONG for negative i16:>>

      #ifdef SIGNED_MULTIPLIER
        result = cells[i] * anotherVector.cells[i];
        connexStateObj.multLow.cells[i] = (TYPE_ELEMENT)result;
        connexStateObj.multHigh.cells[i] = (result >> 16) & 0xFFFF;
      #else
        // We have an unsigned multiplier (From Jul 2017)
        result = ((UNSIGNED_TYPE_ELEMENT)cells[i]) *
                 ((UNSIGNED_TYPE_ELEMENT)anotherVector.cells[i]);

        //printf("result = 0x%08x\n", result);
        connexStateObj.multLow.cells[i] = (TYPE_ELEMENT)result;
        connexStateObj.multHigh.cells[i] = (((unsigned)result) >> 16);
      #endif
    }
}

/****************************************************************************
 * Unsigned-multiplication operator, not conditioned by active flags
 */
void ConnexVector::umult(ConnexVector &anotherVector) {
    unsigned int result;

    assert(0 &&
           "This is NOT following Connex ISA exactly - "
           "talk a bit with Lucian P. and decide what to do exactly.");

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        // WRONG for negative i16: result = cells[i] * anotherVector.cells[i];
        result = ((UNSIGNED_TYPE_ELEMENT)cells[i]) *
                 ((UNSIGNED_TYPE_ELEMENT)anotherVector.cells[i]);

        //printf("result = 0x%08x\n", result);
        connexStateObj.multLow.cells[i] = (TYPE_ELEMENT)result;
        connexStateObj.multHigh.cells[i] = (((unsigned)result) >> 16);
    }
}


/****************************************************************************
 * Unary negation operator, not conditioned by active flags
 */
ConnexVector ConnexVector::operator~() {
    ConnexVector result;

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        result.cells[i] = ~cells[i];
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
        TYPE_ELEMENT val = cells[i];
        TYPE_ELEMENT res = 0;
        UNSIGNED_TYPE_ELEMENT resBit = 1UL << 15;

        for (int bitIndex = 0; bitIndex < sizeof(TYPE_ELEMENT) * 8; bitIndex++) {
            if ((val & 1) == 1)
                res |= resBit;
            resBit >>= 1;
            val >>= 1;
        }

        result.cells[i] = res;
    }

    return result;
}

/****************************************************************************
 * Unsigned less than, not conditioned by active flags
 */
ConnexVector ConnexVector::ult(ConnexVector &anotherVector) {
    ConnexVector result;
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        result.cells[i] = (unsigned)cells[i] < (unsigned)anotherVector.cells[i];
    }
    return result;
}

/****************************************************************************
 * Shift left with immediate value, not conditioned by active flags
 */
ConnexVector ConnexVector::operator<<(unsigned short value) {
    ConnexVector result;
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        result.cells[i] = cells[i] << value;
    }
    return result;
}

/****************************************************************************
 * Shift right with immediate value, not conditioned by active flags
 */
ConnexVector ConnexVector::operator>>(unsigned short value) {
    ConnexVector result;
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        result.cells[i] = ((unsigned short)cells[i]) >> value;
    }
    return result;
}

/****************************************************************************
 * Shr (logical), needs casting to unsigned, not conditioned by active flags
 */
ConnexVector ConnexVector::shr(ConnexVector &anotherVector) {
    ConnexVector result;
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        result.cells[i] = ((unsigned short)cells[i]) >> anotherVector.cells[i];
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
        result.cells[i] = ((TYPE_ELEMENT)cells[i]) >> value;
    }
    return result;
}

/****************************************************************************
* Computes the population count (number of set bits) in each element of the
* argument vector. Argument is treated as unsigned, result is unsigned
*/
ConnexVector ConnexVector::popcount() {
    ConnexVector result;
    TYPE_ELEMENT arg;
    unsigned short count;
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        arg = (TYPE_ELEMENT)cells[i];
        count = 0;
        for (int j = 0; j < CONNEX_REGISTER_SIZE; j++) {
            count += arg & 1;
            arg = arg >> 1;
        }
        result.cells[i] = count;
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

        memcpy(cells, tmp.cells, CONNEX_VECTOR_LENGTH * sizeof(TYPE_ELEMENT));

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
        if (connexStateObj.active.cells[i]) {
            if ( !(addresses.cells[i] >= 0 &&
                   addresses.cells[i] < CONNEX_MEM_SIZE)) {
                printf("Read access outside of bounds of Connex LS memory for "
                       "lane i = %d: addresses.cells[i] = %d\n",
                       i, addresses.cells[i]);
                // TODO: a bit inefficient
                assert(addresses.cells[i] >= 0 &&
                       addresses.cells[i] < CONNEX_MEM_SIZE &&
                       "Read access outside of bounds of Connex LS memory "
                       "for lane i");
            }
        }

        cells[i] = (localStore[addresses.cells[i]].cells[i] *
                                connexStateObj.active.cells[i]) |
                     (cells[i] * !connexStateObj.active.cells[i]);
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
        if (connexStateObj.active.cells[i]) {
            if ( !(addresses.cells[i] >= 0 &&
                   addresses.cells[i] < CONNEX_MEM_SIZE)) {
                printf("Write access outside of bounds of Connex LS memory "
                       "for lane i = %d: addresses.cells[i] = %d\n",
                       i, addresses.cells[i]);
                // TODO: a bit inefficient
                assert(addresses.cells[i] >= 0 &&
                       addresses.cells[i] < CONNEX_MEM_SIZE &&
                       "Write access outside of bounds of Connex LS memory "
                       "for lane i");
            }
        }

        TYPE_ELEMENT value = localStore[addresses.cells[i]].cells[i];
        localStore[addresses.cells[i]].cells[i] =
            (cells[i] * connexStateObj.active.cells[i]) |
            (value * !connexStateObj.active.cells[i]);
    }
}

void ConnexVector::Unconditioned_Setactive(ConnexVector &anotherVector)
{
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++)
        connexStateObj.active.cells[i] = anotherVector.cells[i];
}

void ConnexVector::Unconditioned_Setactive(bool value) {
    if (value == true)
        for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++)
            connexStateObj.active.cells[i] = 1;
    else
        for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++)
            connexStateObj.active.cells[i] = 0;
}

