/****************************************************************************
 * File:   ConnexVector.cpp
 *
 * A class mapping on a Connex Vector.
 *
 */

#include "ConnexVector.h"
#include <string.h>
#include <iostream>
#include "Non-immediate.h"
#include "Immediate.h"
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
void ConnexVector::write(unsigned short *data)
{
    memcpy(cells, data, CONNEX_VECTOR_LENGTH * sizeof(unsigned short));
}

/****************************************************************************
 * Return the data contains in all cells as a short addat
 *
 * @return the array of shorts taken from each cell
 */
unsigned short* ConnexVector::read()
{
    return cells;
}

/****************************************************************************
 * Binary operators (except assignment)
 * These are not conditioned by active flags
 */
BINARY_OP_ARITH_ADD(+)
BINARY_OP_ARITH_SUB(-)
BINARY_OP(<<)
//BINARY_OP(>>)
BINARY_OP(|)
BINARY_OP(&)
BINARY_OP(^)
BINARY_OP_EQ(==)
BINARY_OP_LT(<)

IBINARY_OP_ARITH_ADD(+)
IBINARY_OP_ARITH_SUB(-)
IBINARY_OP(<<)
//IBINARY_OP(>>)
IBINARY_OP(|)
IBINARY_OP(&)
IBINARY_OP(^)
IBINARY_OP_EQ(==)
IBINARY_OP_LT(<)
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
 * Assignment operator (for vload insn and immediate value assignement), conditioned by active flags
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
        if(active.cells[i]==1){
		  result = cells[i] * anotherVector.cells[i];
   	      multLow.cells[i] = (short)result;
		  multHigh.cells[i] = (short)(result >> 16);
       }
	}
}

/****************************************************************************
 * Multiplication operator with immediate value, conditioned by active flags
 */
void ConnexVector::operator*(unsigned short value)
{
	int result;
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
	{
        if(active.cells[i]==1){    
		  result = cells[i] * value;
		  multLow.cells[i] = (short)result;
		  multHigh.cells[i] = (short)(result >> 16);
       }
	}
}

/****************************************************************************
 * Unary negation operator, conditioned by active flags
 */
ConnexVector ConnexVector::operator~()
{
	ConnexVector result;
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
	{
        if(active.cells[i]==1){
		    result.cells[i] = ~cells[i];
        }
	}
	return result;
}

/****************************************************************************
 * Unsigned less than, conditioned by active flags
 */
ConnexVector ConnexVector::ult(ConnexVector anotherVector)
{
	ConnexVector result;
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
	{
		if(active.cells[i]==1){
           		result.cells[i] = (unsigned)cells[i] < (unsigned)anotherVector.cells[i];
		   	if(result.cells[i] == 1) 
             			ltFlag.cells[i] = 1;
		   	else 
             			ltFlag.cells[i] = 0;
		   	if(((unsigned)cells[i]) == ((unsigned)anotherVector.cells[i])) 
              			eqFlag.cells[i] = 1;
		   	else 
              			eqFlag.cells[i] = 0; 
        	}
	}
	return result;
}

/****************************************************************************
 * Unsigned less than with immediate value, conditioned by active flags
 */
ConnexVector ConnexVector::ult(unsigned short value)
{
	ConnexVector result;
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
	{
		if(active.cells[i]==1){
          		result.cells[i] = (unsigned)cells[i] < value;
		  	if(result.cells[i] == 1) 
             			ltFlag.cells[i] = 1;
   	      		else 
             			ltFlag.cells[i] = 0;
		  	if(((unsigned)cells[i]) == value) 
              			eqFlag.cells[i] = 1;
		  	else 
              			eqFlag.cells[i] = 0;
        	}
	}
	return result;
}

/****************************************************************************
 * Shift right (logical), need casting to unsigned, conditioned by active flags
 */
ConnexVector ConnexVector::shr(ConnexVector anotherVector)
{
	ConnexVector result;
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
	{
        if(active.cells[i]==1){
		  result.cells[i] = (unsigned short)cells[i] >> anotherVector.cells[i];
        } 
	}
	return result;
}

/****************************************************************************
 * Shift right (logical) with immediate value, need casting to unsigned, conditioned by active flags
 */
ConnexVector ConnexVector::ishr(unsigned short value)
{
	ConnexVector result;
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
	{
        if(active.cells[i]==1){
		  result.cells[i] = (unsigned short)cells[i] >> value;
        } 
	}
	return result;
}

/****************************************************************************
 * Shift right (arithmetic), needs casting to signed,conditioned by active flags
 */
ConnexVector ConnexVector::shra(ConnexVector anotherVector)
{
	ConnexVector result;
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
	{
        if(active.cells[i]==1){
		  result.cells[i] = ((short)cells[i]) >> anotherVector.cells[i];
        }
	}
	return result;
}

/****************************************************************************
 * Shift right (arithmetic) with immediate value, needs casting to signed,
 * conditioned by active flags
 */
ConnexVector ConnexVector::ishra(unsigned short value)
{
	ConnexVector result;
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
	{
        if(active.cells[i]==1){
		  result.cells[i] = (short)cells[i] >> value;
        }
	}
	return result;
}


/****************************************************************************
* Computes the population count (number of set bits) in each element of the 
* argument vector. Argument is treated as unsigned, result is unsigned
*/
/*
ConnexVector ConnexVector::popcount()
{
    ConnexVector result;
    unsigned short arg;
    unsigned short count;
    for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
    {
        arg = (unsigned short)cells[i];
        count = 0;
        for(int j=0; j<CONNEX_REGISTER_SIZE; j++)
        {
            count += arg & 1;
            arg = arg >> 1;
        }
        result.cells[i] = count;
    }
    return result;
}
*/
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
    for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
    {
        short value = localStore[addresses.cells[i]].cells[i];
        localStore[addresses.cells[i]].cells[i] = cells[i] * active.cells[i] | value * !active.cells[i];
    }
}

void ConnexVector::Unconditioned_Setactive(ConnexVector anotherVector)
{
    for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
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


/****************************************************************************
 * Sets flags for vload
 */
void ConnexVector::setFlagVload(ConnexVector anotherVector){
        int i;
	for(i=0; i<CONNEX_VECTOR_LENGTH; i++){
	    eqFlag.cells[i] = anotherVector.cells[i] == 0;                                        
	    ltFlag.cells[i] = (short)anotherVector.cells[i] < 0;
	}
}

/****************************************************************************
 * Sets flags for ivload
 */
void ConnexVector::setFlagVload(short value){
	int i;
	for(i=0; i<CONNEX_VECTOR_LENGTH; i++){
	    eqFlag.cells[i] = value == 0;                                        
	    ltFlag.cells[i] = value < 0;                                        
	}
}

/****************************************************************************
/* Return element from pos position in cells
 */
unsigned short ConnexVector::getElement(int pos){
	return cells[pos];
}











