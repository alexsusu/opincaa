#include "ConnexVector.h"
#include <string.h>

/****************************************************************************
 * Help macro to ease defining the binary operators
 */
#define IBINARY_OP(op)						         \
IBINARY_OP_COMMON_START(op)                                              \
IBINARY_OP_COMMON_END(op)

/****************************************************************************
 * Help macro to ease defining the binary operators (overwrites flags)
 */

#define IBINARY_OP_COMMON_START(op)                                      \
ConnexVector ConnexVector::operator op(unsigned short value)             \
{									 \
	ConnexVector result;					         \
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)		         \
	{	                                                         \
        if(active.cells[i]==1)                                           \
        {                                                                \
          result.cells[i] = cells[i] op value;                           \
        }

#define IBINARY_OP_COMMON_END(op)                                        \
	}							         \
	return result;							 \
}

/****************************************************************************
* Eq and lt flag for eq instruction
*/
#define IBINARY_OP_FLAGS_EQ_LT(op)                                       \
	    if (cells[i] == value)                                       \
	    {                                                            \
            eqFlag.cells[i] = 1;                                         \
            ltFlag.cells[i] = 1;                                         \
        }                                                                \
        else                                                             \
        {                                                                \
            eqFlag.cells[i] = 0;                                         \
            ltFlag.cells[i] = 0;                                         \
        }


/****************************************************************************
* Eq flag
*/
#define IBINARY_OP_FLAGS_EQ(op)                                          \
	    if (cells[i] == value)                                       \
            eqFlag.cells[i] = 1;                                         \
        else                                                             \
            eqFlag.cells[i] = 0;                                         \

/****************************************************************************
* Lt flag
*/
#define IBINARY_OP_FLAGS_LT(op)                                          \
     	if ((short)cells[i] < value)                                     \
            ltFlag.cells[i] = 1;                                         \
        else ltFlag.cells[i] = 0;

/****************************************************************************
* Carry flag
*/
#define IBINARY_OP_FLAGS_CARRY(op)                                       \
        if (((unsigned)cells[i]) + value > REG_MAX_VAL)                  \
            carryFlag.cells[i] = 1;                                      \
        else carryFlag.cells[i] = 0;

/****************************************************************************
* Borrow flag
*/
#define IBINARY_OP_FLAGS_BORROW(op)                                      \
        if (((unsigned)cells[i]) < value)                                \
            carryFlag.cells[i] = 1;                                      \
        else carryFlag.cells[i] = 0;

/****************************************************************************
* Eq operator
*/
#define IBINARY_OP_EQ(op)                                                    \
        IBINARY_OP_COMMON_START(op)                                      \
        IBINARY_OP_FLAGS_EQ_LT(op)                                       \
        IBINARY_OP_COMMON_END(op)

/****************************************************************************
* Lt operator
*/
#define IBINARY_OP_LT(op)                                                    \
        IBINARY_OP_COMMON_START(op)                                      \
        IBINARY_OP_FLAGS_EQ(op)                                          \
        IBINARY_OP_FLAGS_LT(op)                                          \
        IBINARY_OP_COMMON_END(op)

/****************************************************************************
* +  operator
*/
#define IBINARY_OP_ARITH_ADD(op)                                             \
        IBINARY_OP_COMMON_START(op)                                      \
        IBINARY_OP_FLAGS_CARRY(op)                                       \
        IBINARY_OP_COMMON_END(op)

/****************************************************************************
* -  operator
*/
#define IBINARY_OP_ARITH_SUB(op)                                            \
        IBINARY_OP_COMMON_START(op)                                      \
        IBINARY_OP_FLAGS_BORROW(op)                                      \
        IBINARY_OP_COMMON_END(op)

