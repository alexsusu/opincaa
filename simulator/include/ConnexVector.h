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
         * Least significat half of the 32 bits multiplication result
         */
		static ConnexVector multLow;

        /*
         * Most significat half of the 32 bits multiplication result
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
		int reduce();

        /*
         * Loads each cell with its index in the array
         */
		void loadIndex();

        /*
         * Loads the specified values in the vector's cells
         *
         * @param data the array of shorts to load
         */
		void write(unsigned short *data);

        /*
         * Return the data contains in all cells as a short addat
         *
         * @return the array of shorts taken from each cell
         */
		unsigned short* read();

        /*
         * Copy vector not taking selection into account.
         */
         void copyFrom(ConnexVector anotherVector);
         
        /*
         * List of operators
         */
		ConnexVector operator+(ConnexVector anotherVector);
		ConnexVector operator-(ConnexVector anotherVector);
		ConnexVector operator<<(ConnexVector anotherVector);
		ConnexVector operator>>(ConnexVector anotherVector);
		ConnexVector operator<<(unsigned short value);
		ConnexVector operator>>(unsigned short value);
		void operator=(ConnexVector anotherVector);
		void operator=(unsigned short value);
		void operator=(bool value);
		void operator*(ConnexVector anotherVector);
		ConnexVector operator==(ConnexVector anotherVector);
		ConnexVector operator<(ConnexVector anotherVector);
		ConnexVector operator|(ConnexVector anotherVector);
		ConnexVector operator&(ConnexVector anotherVector);
		ConnexVector operator^(ConnexVector anotherVector);
		ConnexVector operator~();

        /*
         * Unsigned less than
         */
		ConnexVector ult(ConnexVector anotherVector);

        /*
         * Shift right, arithmetic
         */
		ConnexVector shr(ConnexVector anotherVector);

        /*
         * Shift right, arithmetic, with immediate value
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
        void shift(int direction);

        /*
         * Reads this vector from the localStore, using addresses vector for addresses
         *
         * @param localStore the local store to read from
         * @param addresses the addresses to load from
         */
        void loadFrom(ConnexVector *localStore, ConnexVector addresses);

        /*
         * Writes this vector to the localStore, using addresses vector for addresses
         *
         * @param localStore the local store to write to
         * @param addresses the addresses to write to
         */
        void storeTo(ConnexVector *localStore, ConnexVector addresses);

        static void Unconditioned_Setactive(ConnexVector anotherVector);
        static void Unconditioned_Setactive(bool value);

	private:

        /*
         * The cell data for this vector
         */
		unsigned short cells[CONNEX_VECTOR_LENGTH];
};

#endif // CONNEX_VECTOR_H

