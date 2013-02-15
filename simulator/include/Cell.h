/*
 * File:   Cell.h
 *
 * Header file for a class mapping on a Connex Cell.
 *
 */

#ifndef CELL_H
#define CELL_H

#include "Instruction.h"
#include "Architecture.h"

class Cell
{
	public:

		/*
		 * Constructor for creating a new cell
		 *
		 * @param index The index of the new cell in the array
		 */
		Cell(unsigned short index);

		/*
		 * Destructor for the Cell class
		 */
		~Cell();

		/*
		 * Method for executing an instruction that only has
		 * local requirements
		 *
		 * @param instruction the Instruction to execute
		 */
		void execute(Instruction instruction);

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
		void shiftInit(unsigned dataSource, unsigned countSource);

		/*
		 * Shifts the contents left once (actually copies the contents
		 * of the right cell's shift register to a tmp reg).
		 */
		void shiftLeftStepStart();

		/*
		 * Shifts the contents right once (actually copies the contents
		 * of the left cell's shift register to a tmp reg).
		 */
		void shiftRightStepStart();

		/*
		 * Copies the contents of the tmp reg to he shift register and
		 * decreases the shift count.
		 *
		 * @return true if the cell's done shifting
		 */
		bool shiftStepFinish();

		/*
		 * Sets the connection to the cell on the left of this cell
		 *
		 * @param cell The cell on the left
		 */
		void connectLeft(Cell *cell);

		/*
		 * Sets the connection to the cell on the right of this cell
		 *
		 * @param cell The cell on the right
		 */
		void connectRight(Cell *cell);

		/*
		 * Returns the contents of the register at index
		 *
		 * @param index The index of the register to read
		 * @return the contents of the register at index
		 */
		short readRegister(int index);

		/*
		 * Writes the specified value to the local store, at the
		 * specified address
		 *
		 * @param value the value to write
		 * @param lsAddress the address in local store where the value is written
		 */
		void write(short value, int lsAddress);

		/*
		 * Reads the specified value from the local store, at the
		 * specified address
		 *
		 * @param lsAddress the address in local store from which the value is read
		 * @return the value read
		 */
		short read(int lsAddress);
	private:

		/*
		 * Sets the flags according to the provided instruction
		 *
		 * @param instruction the instruction that changes the flags
		 */
		void setFlags(Instruction instruction);

		/*
		 * The cell at the left of this cell
		 */
		Cell *leftCell;

		/*
		 * The cell at the left of this cell
		 */
		Cell *rightCell;

		/*
		 * The register file of this cell
		 */
		short registerFile[CONNEX_REG_COUNT];

		/*
		 * The local store of this cell
		 */
		short localStore[CONNEX_MEM_SIZE];

		/*
		 * The index of this cell
		 */
		unsigned short index;

		/*
		 * The shit register of this sell
		 */
		short shiftRegister;

		/*
		 * A temporary register used for shifting, since
		 * the simulation cells don't shift synchronously
		 */
		short tmpShiftRegister;

		/*
		 * The remainging shifts for this cell
		 */
		unsigned short shiftsRemaining;

		/*
		 * The 32 bits multiplication result of this cell
		 */
		int multiplicationResult;

		/*
		 * The carry flag
		 */
		bool carryFlag;

		/*
		 * The equal flag
		 */
		bool eqFlag;

		/*
		 * The less-than flag
		 */
		bool ltFlag;

		/*
		 * The cell active flag
		 */
		bool active;
};

#endif // CELL_H
