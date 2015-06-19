#include "ConnexVector.h"
#include <string.h>

/****************************************************************************
 * Help macro to ease defining the binary operators
 */
#define BINARY_OP(op)						        \
BINARY_OP_COMMON_START(op)                                              \
BINARY_OP_COMMON_END(op)

/****************************************************************************
 * Help macro to ease defining the binary operators (overwrites flags)
 */

#define BINARY_OP_COMMON_START(op)                                      \
ConnexVector ConnexVector::operator op(ConnexVector anotherVector)      \
{									\
	ConnexVector result;					        \
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)		        \
	{	                                                        \
        if(active.cells[i]==1)                                          \
        {                                                               \
          result.cells[i] = cells[i] op anotherVector.cells[i];         \
        }

#define BINARY_OP_COMMON_END(op)                                        \
	}							        \
	return result;							\
}

/****************************************************************************
* Eq and lt flag for eq instruction
*/
#define BINARY_OP_FLAGS_EQ_LT(op)                                       \
	    if (cells[i] == anotherVector.cells[i])                     \
	    {                                                           \
            eqFlag.cells[i] = 1;                                        \
            ltFlag.cells[i] = 1;                                        \
        }                                                               \
        else                                                            \
        {                                                               \
            eqFlag.cells[i] = 0;                                        \
            ltFlag.cells[i] = 0;                                        \
        }

/****************************************************************************
* Eq flag
*/
#define BINARY_OP_FLAGS_EQ(op)                                          \
	    if (cells[i] == anotherVector.cells[i])                     \
            eqFlag.cells[i] = 1;                                        \
        else                                                            \
            eqFlag.cells[i] = 0;                                        \

/****************************************************************************
* Lt flag
*/
#define BINARY_OP_FLAGS_LT(op)                                          \
     	if ((short)cells[i] < (short)anotherVector.cells[i])            \
            ltFlag.cells[i] = 1;                                        \
        else ltFlag.cells[i] = 0;

/****************************************************************************
* Carry flag
*/
#define BINARY_OP_FLAGS_CARRY(op)                                                    \
        if (((unsigned)cells[i]) + ((unsigned)anotherVector.cells[i]) > REG_MAX_VAL) \
            carryFlag.cells[i] = 1;                                                  \
        else carryFlag.cells[i] = 0;

/****************************************************************************
* Borrow flag
*/
#define BINARY_OP_FLAGS_BORROW(op)                                                   \
        if (((unsigned)cells[i]) < ((unsigned)anotherVector.cells[i]))               \
            carryFlag.cells[i] = 1;                                                  \
        else carryFlag.cells[i] = 0;

/****************************************************************************
* Eq operator
*/
#define BINARY_OP_EQ(op)                                                \
        BINARY_OP_COMMON_START(op)                                      \
        BINARY_OP_FLAGS_EQ_LT(op)                                       \
        BINARY_OP_COMMON_END(op)

/****************************************************************************
* Lt operator
*/
#define BINARY_OP_LT(op)                                                \
        BINARY_OP_COMMON_START(op)                                      \
        BINARY_OP_FLAGS_EQ(op)                                          \
        BINARY_OP_FLAGS_LT(op)                                          \
        BINARY_OP_COMMON_END(op)

/****************************************************************************
* +  operator
*/
#define BINARY_OP_ARITH_ADD(op)                                         \
        BINARY_OP_COMMON_START(op)                                      \
        BINARY_OP_FLAGS_CARRY(op)                                       \
        BINARY_OP_COMMON_END(op)

/****************************************************************************
* -  operator
*/
#define BINARY_OP_ARITH_SUB(op)                                         \
        BINARY_OP_COMMON_START(op)                                      \
        BINARY_OP_FLAGS_BORROW(op)                                      \
        BINARY_OP_COMMON_END(op)

