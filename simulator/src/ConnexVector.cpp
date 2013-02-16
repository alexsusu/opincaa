/****************************************************************************
 * File:   ConnexVector.cpp
 *
 * A class mapping on a Connex Vector.
 *
 */


#include "ConnexVector.h"
#include <string.h>

/****************************************************************************
 * Help macro to ease defining the binary operators
 */
#define BINARY_OP(op)												\
ConnexVector ConnexVector::operator op(ConnexVector anotherVector)	\
{																	\
	ConnexVector result;											\
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)						\
	{																\
		result.cells[i] = cells[i] op anotherVector.cells[i];		\
	}																\
	return result;													\
}


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
 * Least significat half of the 32 bits multiplication result
 */
ConnexVector ConnexVector::multLow;

/****************************************************************************
 * Most significat half of the 32 bits multiplication result
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
int ConnexVector::reduce()
{
	int sum = 0;
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
	{
		sum += cells[i];
	}
	return sum;
}

/**************************************************************************** 
 * Loads each cell with its index in the array
 */
void ConnexVector::loadIndex()
{
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
	{
		cells[i] = (active.cells[i] * i) | (!active.cells[i] * cells[i]);
	}
}

/**************************************************************************** 
 * Loads the specified values in the vector's cells
 * 
 * @param data the array of shorts to load
 */
void ConnexVector::write(short *data)
{
    memcpy(cells, data, CONNEX_VECTOR_LENGTH * sizeof(short));
}

/**************************************************************************** 
 * Return the data contains in all cells as a short addat
 * 
 * @return the array of shorts taken from each cell
 */
short* ConnexVector::read()
{
    return cells;
}

/****************************************************************************
 * Binary operators (except assignment)
 * These are not conditioned by active flags
 */
BINARY_OP(+)
BINARY_OP(-)
BINARY_OP(<<)
BINARY_OP(>>)
BINARY_OP(==)
BINARY_OP(<)
BINARY_OP(|)
BINARY_OP(&)
BINARY_OP(^)

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

/****************************************************************************
 * Assignment operator (for vload insn), conditioned by active flags
 */
void ConnexVector::operator=(short value)				
{		
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)						
	{										
		cells[i] = (active.cells[i] * value) | (!active.cells[i] * cells[i]);
	}																
}

/****************************************************************************
 * Multiplication operator, not conditioned by active flags
 */
void ConnexVector::operator*(ConnexVector anotherVector)				
{		
	int result;
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)						
	{																
		result = cells[i] * anotherVector.cells[i];
		multLow.cells[i] = (short)result;
		multHigh.cells[i] = (short)(result >> 16);
	}																
}

/****************************************************************************
 * Unary negation operator, not conditioned by active flags
 */
ConnexVector ConnexVector::operator~()				
{		
	ConnexVector result;
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)						
	{																
		result.cells[i] = ~cells[i];
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
ConnexVector ConnexVector::operator <<(short value)
{
    ConnexVector result;
    for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)                       
    {                                                               
        result.cells[i] = cells[i] << value;
    }                                                               
    return result;
}

/****************************************************************************
 * Shift right with immediate value, not conditioned by active flags
 */
ConnexVector ConnexVector::operator >>(short value)
{
    ConnexVector result;
    for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)                       
    {                                                               
        result.cells[i] = cells[i] >> value;
    }                                                               
    return result;
}

/****************************************************************************
 * Shift right (logical), need casting to unsigned, not conditioned by active flags
 */
ConnexVector ConnexVector::shr(ConnexVector anotherVector)
{
	ConnexVector result;
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)						
	{																
		result.cells[i] = (unsigned short)cells[i] >> anotherVector.cells[i];
	}																
	return result;
}

/****************************************************************************
 * Shift right (logical) with immediate value, need casting to unsigned,
 * not conditioned by active flags
 */
ConnexVector ConnexVector::ishr(short value)
{
	ConnexVector result;
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)						
	{																
		result.cells[i] = (unsigned short)cells[i] >> value;
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
void ConnexVector::shift(int direction)
{
    bool done;
    ConnexVector tmp;
    do
    {
        done = true;
        for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
        {
            if(ConnexVector::shiftCountReg.cells[i] > 0)
            {
                //tmp.cells[i] = cells[(i + direction + CONNEX_VECTOR_LENGTH) % CONNEX_VECTOR_LENGTH];
                tmp.cells[i] = cells[(i + direction) & (CONNEX_VECTOR_LENGTH - 1)];
                ConnexVector::shiftCountReg.cells[i]--;
            }
            
            done = done && (!ConnexVector::shiftCountReg.cells[i]);
        }
        memcpy(cells, tmp.cells, sizeof(cells));
    }
    while(!done);
}

/****************************************************************************
 * Reads this vector from the localStore, using addresses vector for addresses
 * 
 * @param localStore the local store to read from
 * @param addresses the addresses to load from
 */
void ConnexVector::loadFrom(ConnexVector *localStore, ConnexVector addresses)
{
    for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
    {
        cells[i] = localStore[addresses.cells[i]].cells[i] * active.cells[i] | cells[i] * active.cells[i];
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
    for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
    {
        short value = localStore[addresses.cells[i]].cells[i];
        localStore[addresses.cells[i]].cells[i] = cells[i] * active.cells[i] | value * !active.cells[i];
    }
}
