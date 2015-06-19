/*
 * File:   ConnexVector.h
 *
 * Header file for a class mapping on a Connex Vector.
 *
 */

#ifndef CONNEX_VECTOR_H
#define CONNEX_VECTOR_H

#include "Architecture.h"

class ConnexVector
{
	public:

        /*
         * The active cell flags
         */
		static ConnexVector active;

        /*
         * The carry cell flags
         */
		static ConnexVector carryFlag;

        /*
         * The equal cell flags
         */
		static ConnexVector eqFlag;

        /*
         * The less than cell flags
         */
		static ConnexVector ltFlag;

        /*
         * Least significant half of the 32 bits multiplication result
         */
		static ConnexVector multLow;

        /*
         * Most significant half of the 32 bits multiplication result
         */
		static ConnexVector multHigh;

        /*
         * The cell shift register
         */
		static ConnexVector shiftReg;

        /*
         * The remaining shifts required for each cells
         */
		static ConnexVector shiftCountReg;

	/*
	 * Constructor for creating a new ConnexVector
	 */
		ConnexVector();

	/*
	 * Destructor for the ConnexVector class
	 */
		~ConnexVector();

	/*
	 * Computes the reduction of this Vector
	 *
	 * @return the value of the reduction operation
	 */
		int reduce();//ok

        /*
         * Loads each cell with its index in the array
         */
		void loadIndex();//ok

        /*
         * Loads the specified values in the vector's cells
         *
         * @param data the array of shorts to load
         */
		void write(unsigned short *data);//ok

        /*
         * Return the data contains in all cells as a short add at
         *
         * @return the array of shorts taken from each cell
         */
		unsigned short* read();//ok

        /*
         * Copy vector not taking selection into account.
         */
        	void copyFrom(ConnexVector anotherVector);//ok
         
        /*
         * List of operators
         */
		ConnexVector operator+(ConnexVector anotherVector);
                ConnexVector operator+(unsigned short value);
		ConnexVector operator-(ConnexVector anotherVector);
                ConnexVector operator-(unsigned short value);

		ConnexVector operator<<(ConnexVector anotherVector);
		ConnexVector operator<<(unsigned short value);

		void operator=(ConnexVector anotherVector);
		void operator=(short value);
		void operator=(bool value);

		void operator*(ConnexVector anotherVector);
		void operator*(unsigned short value);

		ConnexVector operator==(ConnexVector anotherVector);
		ConnexVector operator==(unsigned short value);
		ConnexVector operator<(ConnexVector anotherVector);
		ConnexVector operator<(unsigned short value);
		ConnexVector operator|(ConnexVector anotherVector);
		ConnexVector operator|(unsigned short value);
		ConnexVector operator&(ConnexVector anotherVector);
		ConnexVector operator&(unsigned short value);
		ConnexVector operator^(ConnexVector anotherVector);
		ConnexVector operator^(unsigned short value);
		ConnexVector operator~();

        /*
         * Unsigned less than
         */
		ConnexVector ult(ConnexVector anotherVector);//ok
		
        /*
         * Unsigned less than with immediate value
         */
	        ConnexVector ult(unsigned short value);//ok
         
        /*
         * Shift right, logical
         */
		ConnexVector shr(ConnexVector anotherVector);//ok
		
		/*
         * Shift right, logical, with immediate value
         */
		ConnexVector ishr(unsigned short value);//ok

        /*
         * Shift right, arithmitic
         */
		ConnexVector shra(ConnexVector anotherVector);

        /*
         * Shift right, arithmitic, with immediate value
         */
		ConnexVector ishra(unsigned short value);

        /*
         * Shift the vector in the specified direction, with the number
         * of cells specified by ConnexVector::shiftCount
         *
         * @param direction the direction: -1 if the shift is right and
         *                                  1 if the shift is left
         *
         */
        	void shift(int direction);//ok

        /*
         * Reads this vector from the localStore, using addresses vector for addresses
         *
         * @param localStore the local store to read from
         * @param addresses the addresses to load from
         */
        	void loadFrom(ConnexVector *localStore, ConnexVector addresses);//ok

        /*
         * Writes this vector to the localStore, using addresses vector for addresses
         *
         * @param localStore the local store to write to
         * @param addresses the addresses to write to
         */
        	void storeTo(ConnexVector *localStore, ConnexVector addresses);//ok
	
        	static void Unconditioned_Setactive(ConnexVector anotherVector);//ok
        	static void Unconditioned_Setactive(bool value);//ok

        /* Set flags for vload and ivload.
         */
           	static void setFlagVload(ConnexVector anotherVector);
           	static void setFlagVload(short value);

	/* Return element from pos position in cells
         */
		unsigned short getElement(int pos);

	private:

        /*
         * The cell data for this vector
         */
		unsigned short cells[CONNEX_VECTOR_LENGTH];
};

#endif // CONNEX_VECTOR_H

