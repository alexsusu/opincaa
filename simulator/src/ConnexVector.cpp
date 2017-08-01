/****************************************************************************
 * File:   ConnexVector.cpp
 *
 * A class mapping on a Connex Vector.
 *
 */


#include "ConnexVector.h"
#include <assert.h>
#include <stdio.h>
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
ConnexVector ConnexVector::operator op(ConnexVector anotherVector)  \
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
            eqFlag.cells[i] = 1;                                    \
        else eqFlag.cells[i] = 0;

#define BINARY_OP_FLAGS_LT(op)                                      \
        if ((TYPE_ELEMENT)cells[i] < (TYPE_ELEMENT)anotherVector.cells[i])                      \
            ltFlag.cells[i] = 1;                                    \
        else ltFlag.cells[i] = 0;

        //printf("Carry DEBUG info: i = %d, 0x%08x %08x\n", i, cells[i], anotherVector.cells[i]);
        //printf("Carry DEBUG info: i = %d, 0x%04hx %04hx\n", i, cells[i], anotherVector.cells[i]);
        //if ( ((unsigned)cells[i]) + ((unsigned)anotherVector.cells[i]) > (unsigned)REG_MAX_VAL)
        //printf("Carry DEBUG info: i = %d, tmp1=0x%08x tmp2=%08x\n", i, tmp1, tmp2);
        //printf("Carry DEBUG info: i = %d, tmp1=0x%08x tmp2=%08x\n", i, tmp1, tmp2);
// IMPORTANT note: twos-complement integer numbers don't care about the sign bit when computing carry after add operation
#define BINARY_OP_FLAGS_CARRY_ADD(op)                               \
        unsigned int tmp1 = 0;                                      \
        unsigned int tmp2 = 0;                                      \
        *((short *) &tmp1) = (TYPE_ELEMENT)cells[i];                \
        /* Similar solution: For very strange reason at least on x86 although I typecast to short it sign extends to i32 before doing OR (which means putting 0xFFFF on uppermost bits if the value is < 0)
           tmp1 |= (TYPE_ELEMENT)cells[i];
           tmp1 &= 0xFFFF; */                                       \
        *((short *) &tmp2) = (TYPE_ELEMENT)anotherVector.cells[i];  \
        /* Similar solution: For very strange reason at least on x86 although I typecast to short it sign extends to i32 before doing OR (which means putting 0xFFFF on uppermost bits if the value is < 0)
           tmp2 |= (TYPE_ELEMENT)anotherVector.cells[i];
           tmp2 &= 0xFFFF; */                                       \
        if (tmp1 + tmp2 > (unsigned)REG_MAX_VAL)                    \
            carryFlag.cells[i] = 1;                                 \
        else                                                        \
            carryFlag.cells[i] = 0;

// TODO (Alex): NOT sure if 100% correct
//if (((unsigned)cells[i]) + ((unsigned)carryFlag.cells[i]) + ((unsigned)anotherVector.cells[i]) > REG_MAX_VAL)
#define BINARY_OP_FLAGS_CARRY_ADDC(op)                              \
        if ( (* ((unsigned short *)&cells[i]) ) + (* ((unsigned short *)&carryFlag.cells[i])) + (* ((unsigned short *)&anotherVector.cells[i])) > REG_MAX_VAL )                      \
            carryFlag.cells[i] = 1;                                 \
        else                                                        \
            carryFlag.cells[i] = 0;

// TODO (Alex): NOT sure if 100% correct
// if ( *((unsigned short *)(&cells[i])) < *((unsigned short *)(&anotherVector.cells[i])) )
//        if ( ((unsigned short)(cells[i])) < ((unsigned short)(anotherVector.cells[i])) )
// For subtraction, carry is actually borrow (for subtraction with borrow) - see e.g. https://en.wikipedia.org/wiki/Carry_flag
#define BINARY_OP_FLAGS_CARRY_SUB(op)                               \
        if ( (* ((unsigned short *)&cells[i]) ) < (* ((unsigned short *)&anotherVector.cells[i])) )                      \
            carryFlag.cells[i] = 1;                                 \
        else carryFlag.cells[i] = 0;

// TODO (Alex): NOT sure if 100% correct
//        if (((unsigned)cells[i]) + ((unsigned)carryFlag.cells[i]) + ((unsigned)anotherVector.cells[i]) > REG_MAX_VAL)
#define BINARY_OP_FLAGS_CARRY_SUBC(op)                              \
        if ( (* ((unsigned short *)&cells[i]) ) < (* ((unsigned short *)&carryFlag.cells[i])) + (* ((unsigned short *)&anotherVector.cells[i])) )                      \
            carryFlag.cells[i] = 1;                                 \
        else carryFlag.cells[i] = 0;


// IMPORTANT NOTE: here we treat the carries for the ops defined, but the actual semantics of the ops is implemented in ConnexSimulator.cpp

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

/****************************************************************************
 * The active cell flags
 */
ConnexVector ConnexVector::active;

/****************************************************************************
 * The carry cell flags
 */
ConnexVector ConnexVector::carryFlag;

/****************************************************************************
 * The equal cell flags
 */
ConnexVector ConnexVector::eqFlag;

/****************************************************************************
 * The less than cell flags
 */
ConnexVector ConnexVector::ltFlag;

/****************************************************************************
 * Least significant half of the 32 bits multiplication result
 */
ConnexVector ConnexVector::multLow;

/****************************************************************************
 * Most significant half of the 32 bits multiplication result
 */
ConnexVector ConnexVector::multHigh;

/****************************************************************************
 * The cell shift register
 */
ConnexVector ConnexVector::shiftReg;

/****************************************************************************
 * The remaining shifts required for each cells
 */
ConnexVector ConnexVector::shiftCountReg;

/****************************************************************************
 * Constructor for creating a new ConnexVector
 */
ConnexVector::ConnexVector()
{
    // Alex: adding this initialization to avoid valgrind give errors like: "Use of uninitialised value"
    memset(cells, 0, sizeof(TYPE_ELEMENT) * CONNEX_VECTOR_LENGTH);
}

/****************************************************************************
 * Destructor for the ConnexVector class
 */
ConnexVector::~ConnexVector()
{

}

/****************************************************************************
 * Computes the reduction of this Vector
 *
 * @return the value of the reduction operation
 */
int ConnexVector::reduce() {
    int sum = 0;
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
      #define U16_REDUCE_WORKING_FOR_I32_REDUCE

      #ifdef U16_REDUCE_WORKING_FOR_I32_REDUCE
        // Alex: we do this typecast to avoid sign extension (at least on x86) from i16 to i32
        //sum += *((unsigned short *)(&cells[i])); // This works with our example RED_i32
        sum += active.cells[i] * (*((unsigned short *)(&cells[i]))); // This works with our example RED_i32
      #else
        //sum += cells[i];
        // Reduction is performed only on selected cells
        sum += active.cells[i] * cells[i];
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
        cells[i] = (active.cells[i] * i) | (!active.cells[i] * cells[i]);
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

// Defining ConnexVector::operator+(ConnexVector anotherVector), not conditioned by active flags
BINARY_OP_FLAGS_LIKE_ADD(+)

// Defining ConnexVector::operator-(ConnexVector anotherVector), not conditioned by active flags
BINARY_OP_FLAGS_LIKE_SUB(-)

// Defining ConnexVector::operator<<(ConnexVector anotherVector), not conditioned by active flags
BINARY_OP_FLAGS_LIKE_ADD(<<)

// TODO (Alex): NOT sure if 100% correct
// Defining ConnexVector::operator>>(ConnexVector anotherVector), not conditioned by active flags
BINARY_OP_FLAGS_LIKE_SUB(>>)

// Defining ConnexVector::operator==(ConnexVector anotherVector), not conditioned by active flags
BINARY_OP_FLAGS_LIKE_ADD(==)

// TODO (Alex): NOT sure if 100% correct
// Defining ConnexVector::operator<(ConnexVector anotherVector), not conditioned by active flags
BINARY_OP_FLAGS_LIKE_SUB(<)

// TODO (Alex): NOT sure if 100% correct
// Defining ConnexVector::operator|(ConnexVector anotherVector), not conditioned by active flags
BINARY_OP_FLAGS_LIKE_SUB(|)

// TODO (Alex): NOT sure if 100% correct
// Defining ConnexVector::operator&(ConnexVector anotherVector), not conditioned by active flags
BINARY_OP_FLAGS_LIKE_ADDC(&)

// TODO (Alex): NOT sure if 100% correct
// Defining ConnexVector::operator^(ConnexVector anotherVector), not conditioned by active flags
BINARY_OP_FLAGS_LIKE_SUBC(^)



/****************************************************************************
 * Assignment operator, conditioned by active flags
 */
void ConnexVector::operator=(ConnexVector anotherVector)
{
    for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
    {
        cells[i] = (active.cells[i] * anotherVector.cells[i]) | (!active.cells[i] * cells[i]);
    }
}

/*
 * Copy vector not taking selection into account.
 */
void ConnexVector::copyFrom(ConnexVector anotherVector)
{
    memcpy(cells, anotherVector.cells, sizeof(cells));
}


/****************************************************************************
 * Assignment operator (used only for reset of active)
 */
void ConnexVector::operator=(bool value)
{
    for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
    {
        if (value == true)
            active.cells[i] = 1;
        else
            active.cells[i] = 0;
    }
}

/****************************************************************************
 * Assignment operator (for vload insn), conditioned by active flags
 */
void ConnexVector::operator=(TYPE_ELEMENT value)
{
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        cells[i] = (active.cells[i] * value) | (!active.cells[i] * cells[i]);
    }
}

/****************************************************************************
 * Multiplication operator, not conditioned by active flags
 */
void ConnexVector::operator*(ConnexVector anotherVector) {
    int result;

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        // TAKEOUT: Alex: DON'T understand why this comment :((( <<WRONG for negative i16:>>

      #ifdef SIGNED_MULTIPLIER
        result = cells[i] * anotherVector.cells[i];
        multLow.cells[i] = (TYPE_ELEMENT)result;
        multHigh.cells[i] = (result >> 16) & 0xFFFF;
      #else
        // We have an unsigned multiplier (From Jul 2017)
        result = *((unsigned short *) &cells[i]) * (*((unsigned short *) &anotherVector.cells[i]));

        //printf("result = 0x%08x\n", result);
        multLow.cells[i] = (TYPE_ELEMENT)result;
        multHigh.cells[i] = (((unsigned)result) >> 16); // & 0xFFFF;
      #endif
    }
}

/****************************************************************************
 * Unsigned-multiplication operator, not conditioned by active flags
 */
void ConnexVector::umult(ConnexVector anotherVector) {
    int result;

    assert(0 && "This is NOT following Connex ISA exactly - talk a bit with Lucian P. and decide what to do exactly.");

    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        // WRONG for negative i16: result = cells[i] * anotherVector.cells[i];
        result = *((unsigned TYPE_ELEMENT *) &cells[i]) * (*((unsigned TYPE_ELEMENT *) &anotherVector.cells[i]));

        //printf("result = 0x%08x\n", result);
        multLow.cells[i] = (TYPE_ELEMENT)result;
        multHigh.cells[i] = (((unsigned)result) >> 16); // & 0xFFFF;
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
        unsigned short resBit = 1UL << 15;

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
ConnexVector ConnexVector::ult(ConnexVector anotherVector)
{
    ConnexVector result;
    for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
    {
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
 * Shift right (logical), needs casting to unsigned, not conditioned by active flags
 */
ConnexVector ConnexVector::shr(ConnexVector anotherVector) {
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
    bool done;
    ConnexVector tmp;

    // Alex: adding simple copy for case CELLSH has shift-operand 0
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        tmp.cells[i] = cells[i];
    }

    do {
        done = true;

        for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
            if (ConnexVector::shiftCountReg.cells[i] > 0) {
                //tmp.cells[i] = cells[(i + direction + CONNEX_VECTOR_LENGTH) % CONNEX_VECTOR_LENGTH];
                tmp.cells[i] = cells[(i + direction) & (CONNEX_VECTOR_LENGTH - 1)];
                ConnexVector::shiftCountReg.cells[i]--;
            }

            done = done && (!ConnexVector::shiftCountReg.cells[i]);
        }
        memcpy(cells, tmp.cells, sizeof(cells));
    }
    while (!done);
}

/****************************************************************************
 * Reads this vector from the localStore, using addresses vector for addresses
 *
 * @param localStore the local store to read from
 * @param addresses the addresses to load from
 */
void ConnexVector::loadFrom(ConnexVector *localStore, ConnexVector addresses)
{
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        cells[i] = localStore[addresses.cells[i]].cells[i] * active.cells[i] | cells[i] * !active.cells[i];
    }
}

/****************************************************************************
 * Writes this vector to the localStore, using addresses vector for addresses
 *
 * @param localStore the local store to write to
 * @param addresses the addresses to write to
 */
void ConnexVector::storeTo(ConnexVector *localStore, ConnexVector addresses)
{
    for (int i = 0; i<CONNEX_VECTOR_LENGTH; i++) {
        TYPE_ELEMENT value = localStore[addresses.cells[i]].cells[i];
        localStore[addresses.cells[i]].cells[i] = cells[i] * active.cells[i] | value * !active.cells[i];
    }
}

void ConnexVector::Unconditioned_Setactive(ConnexVector anotherVector)
{
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++)
        active.cells[i] = anotherVector.cells[i];
}

void ConnexVector::Unconditioned_Setactive(bool value)
{
    if (value == true)
        for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
            active.cells[i] = 1;
    else
        for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
            active.cells[i] = 0;
}
