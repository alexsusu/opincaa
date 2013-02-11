/*
 * File:   Cell.cpp
 *
 * A class mapping on a Connex Array cell
 */

#include "Cell.h"

#define HAS_CARRY(x) ((((x) % 0xFFFF8000) != 0) && (((x) % 0xFFFF8000) != 0xFFFF8000))

/* 
 * Constructor for creating a new cell
 * 
 * @param index The index of the new cell in the array
 */	
Cell::Cell(unsigned short index)
{
	this->index = index;
	leftCell = NULL;
	rightCell = NULL;
	
	active = false;
	eqFlag = ltFlag = carryFlag = false;
}

/* 
 * Destructor for the Cell class
 */		
Cell::~Cell()
{

}

/* 
 * Method for executing an instruction that only has
 * local requirements
 *
 * @param instruction the Instruction to execute
 */		
void Cell::execute(Instruction instruction)
{
	if(!active)
	{
		return;
	}
	
	switch(instruction.getOpcode())
    {
        case _ADD: 
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] + registerFile[instruction.getRight()];
			setFlags(instruction);
			carryFlag = HAS_CARRY((int)registerFile[instruction.getLeft()] + (int)registerFile[instruction.getRight()]);
			break;
        case _ADDC:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] + registerFile[instruction.getRight() + carryFlag];
			setFlags(instruction);
			carryFlag = HAS_CARRY((int)registerFile[instruction.getLeft()] + (int)registerFile[instruction.getRight()] + carryFlag);
			break;
        case _SUB:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] - registerFile[instruction.getRight()];
			setFlags(instruction);
			carryFlag = HAS_CARRY((int)registerFile[instruction.getLeft()] - (int)registerFile[instruction.getRight()]);
			break;
        case _SUBC:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] - registerFile[instruction.getRight() - carryFlag];
			setFlags(instruction);
			carryFlag = HAS_CARRY((int)registerFile[instruction.getLeft()] - (int)registerFile[instruction.getRight() - carryFlag]);
			break;
        case _NOT: 
			registerFile[instruction.getDest()] = ~registerFile[instruction.getLeft()];
			break;
        case _OR:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] | registerFile[instruction.getRight()];
			break;
        case _AND:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] & registerFile[instruction.getRight()];
			break;
        case _XOR:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] ^ registerFile[instruction.getRight()];
			break;
        case _EQ:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] == registerFile[instruction.getRight()];
			break;
        case _LT:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] < registerFile[instruction.getRight()];
			break;
        case _ULT:
			registerFile[instruction.getDest()] = (unsigned)registerFile[instruction.getLeft()] < (unsigned)registerFile[instruction.getRight()];
			break;
        case _SHL:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] << registerFile[instruction.getRight()];
			break;
        case _SHR:
			registerFile[instruction.getDest()] = (unsigned)registerFile[instruction.getLeft()] >> registerFile[instruction.getRight()];
			break;
        case _SHRA:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] >> registerFile[instruction.getRight()];
			break;
        case _ISHL:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] << instruction.getRight();
			break;
        case _ISHR:
			registerFile[instruction.getDest()] = (unsigned)registerFile[instruction.getLeft()] >> instruction.getRight();
			break;
        case _ISHRA: 
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] >> instruction.getRight();
			break;
        case _LDIX:
			registerFile[instruction.getDest()] = index;
			break;
        case _LDSH:
			registerFile[instruction.getDest()] = shiftRegister;
			break;
        case _CELL_SHL: 
        case _CELL_SHR: 
			/* Since this required step-by-step cell sync, it will 
			 * be done at the next hierarchical level
			 */
			throw string("Invalid instruction in Cell::execute");
			break;
        case _READ: 
			registerFile[instruction.getDest()] = localStore[registerFile[instruction.getRight()]];
			break;
        case _WRITE: 
			localStore[registerFile[instruction.getRight()]] = registerFile[instruction.getLeft()];
			break;
        case _MULT:
			multiplicationResult = registerFile[instruction.getLeft()] * registerFile[instruction.getRight()];
			break;
        case _MULT_LO:
			registerFile[instruction.getDest()] = (short)multiplicationResult;
			break;
        case _MULT_HI: 
			registerFile[instruction.getDest()] = (short)(multiplicationResult >> 16);
			break;
        case _WHERE_CRY:
			active = carryFlag;
			break;
        case _WHERE_EQ:
			active = eqFlag;
			break;
        case _WHERE_LT:
			active = ltFlag;
			break;
        case _END_WHERE:
			active = true;
			break;
        case _REDUCE:
			/* As this depends on all cells, it will be done
			 * at the next ierarchical level
			 */
			throw string("Invalid instruction in Cell::execute");
			break;
        case _NOP: 
			break;
        case _VLOAD: 
			registerFile[instruction.getDest()] = instruction.getValue();
			break;
        case _IREAD:
			registerFile[instruction.getDest()] = localStore[instruction.getValue()];
			break;
        case _IWRITE:
			localStore[instruction.getValue()] = registerFile[instruction.getLeft()];
			break;
        default: throw string("Invalid instruction opcode!");
    }
}

/* 
 * Method for initiating a vector shift (cell by cell)
 * operation. It initializes the contents of the shift 
 * register and the shift count
 *
 * @param dataSource the index of the register that will
 *		be copied to the shift register
 * @param countSource the index of the register that holds
 *		the shift count value
 */
void Cell::shiftInit(unsigned dataSource, unsigned countSource)
{
	shiftRegister = registerFile[dataSource];
	shiftsRemaining = active ? registerFile[countSource] : 0;
}

/* 
 * Shifts the contents left once (actually copies the contents
 * of the right cell's shift register to a tmp reg). 
 */
void Cell::shiftLeftStepStart()
{
	tmpShiftRegister = rightCell->shiftRegister;
}

/* 
 * Shifts the contents right once (actually copies the contents
 * of the left cell's shift register to a tmp reg). 
 */
void Cell::shiftRightStepStart()
{
	tmpShiftRegister = leftCell->shiftRegister;
}

/* 
 * Copies the contents of the tmp reg to he shift register and 
 * decreases the shift count.
 *
 * @return true if the cell's done shifting
 */
bool Cell::shiftStepFinish()
{
	if(active && shiftsRemaining)
	{
		shiftRegister = tmpShiftRegister;
		shiftsRemaining--;
	}
	
	return !shiftsRemaining;
}

/* 
 * Sets the connection to the cell on the left of this cell
 *
 * @param cell The cell on the left
 */
void Cell::connectLeft(Cell *cell)
{
	leftCell = cell;
}

/* 
 * Sets the connection to the cell on the right of this cell
 *
 * @param cell The cell on the right
 */
void Cell::connectRight(Cell *cell)
{
	rightCell = cell;
}

/* 
 * Returns the contents of the register at index
 *
 * @param index The index of the register to read
 * @return the contents of the register at index
 */
short Cell::readRegister(int index)
{
	return registerFile[index];
}
		
/* 
 * Writes the specified value to the local store, at the 
 * specified address
 *
 * @param value the value to write
 * @param lsAddress the address in local store where the value is written
 */
void Cell::write(short value, int lsAddress)
{
	localStore[lsAddress] = value;
}

/* 
 * Reads the specified value from the local store, at the 
 * specified address
 *
 * @param lsAddress the address in local store from which the value is read
 * @return the value read
 */
short Cell::read(int lsAddress)
{
	return localStore[lsAddress];
}

/* 
 * Sets the flags according to the provided instruction
 *
 * @param instruction the instruction that changes the flags
 */	
void Cell::setFlags(Instruction instruction)
{
	eqFlag = registerFile[instruction.getLeft()] == registerFile[instruction.getRight()];
	ltFlag = registerFile[instruction.getLeft()] < registerFile[instruction.getRight()];
}	
