/*
 * File:   ConnexVector.h
 *
 * Header file for a class mapping on a Connex Vector.
 *
 */

#ifndef CONNEX_VECTOR_H
#define CONNEX_VECTOR_H

#include <limits.h>
#include "Architecture.h"



class ConnexVector {
    public:

        /*
         * Constructor for creating a new ConnexVector
         */
        ConnexVector();

        /* 2018_03_27:
           Adding this constructor, after we added:
              void operator=(ConnexVector &&anotherVector);
            This is required to allow references from temporary objects,
              which are r-values, i.e. the references are to r-values. */
        /*constexpr*/ ConnexVector(const ConnexVector &anotherVector);

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
         * @param data the array of TYPE_ELEMENT to load
         */
        void write(TYPE_ELEMENT *data);

        /*
         * Return the data contained in all cells as a TYPE_ELEMENT data
         *
         * @return the array of TYPE_ELEMENT taken from each cell
         */
        TYPE_ELEMENT *read();

        /*
         * Copy vector not taking selection into account.
         */
         void copyFrom(ConnexVector &anotherVector);

        /*
         * List of operators
         */
        ConnexVector operator+(ConnexVector &anotherVector);
        ConnexVector operator-(ConnexVector &anotherVector);
        ConnexVector operator<<(ConnexVector &anotherVector);
        ConnexVector operator>>(ConnexVector &anotherVector);
        ConnexVector operator<<(unsigned short value);
        ConnexVector operator>>(unsigned short value);

        // 2018_03_27
        void operator=(ConnexVector &anotherVector);
        /* Required to allow references from temporary objects,
           which are r-values - so the references are to r-values. */
        void operator=(ConnexVector && anotherVector); 
        // NOT GOOD: void operator=(ConnexVector &anotherVector) const;

        void operator=(TYPE_ELEMENT value);
        void operator=(bool value);
        void operator*(ConnexVector &anotherVector);
        // Alex: Added umult() but I am NOT using yet well...
        void umult(ConnexVector &anotherVector);

        ConnexVector operator==(ConnexVector &anotherVector);
        ConnexVector operator<(ConnexVector &anotherVector);
        ConnexVector operator|(ConnexVector &anotherVector);
        ConnexVector operator&(ConnexVector &anotherVector);
        ConnexVector operator^(ConnexVector &anotherVector);
        ConnexVector operator~();
        ConnexVector bitreverse();

        /*
         * Unsigned less than
         */
        ConnexVector ult(ConnexVector &anotherVector);

        /*
         * Shift right, arithmetic
         */
        ConnexVector shr(ConnexVector &anotherVector);

        /*
         * Shift right, arithmetic, with immediate value
         */
        ConnexVector ishra(unsigned short value);

        /*
         * Computes the population count of each element of this Vector
         */
        ConnexVector popcount();

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
        void loadFrom(ConnexVector *localStore, ConnexVector &addresses);

        /*
         * Writes this vector to the localStore, using addresses vector for addresses
         *
         * @param localStore the local store to write to
         * @param addresses the addresses to write to
         */
        void storeTo(ConnexVector *localStore, ConnexVector &addresses);

        static void Unconditioned_Setactive(ConnexVector &anotherVector);
        static void Unconditioned_Setactive(bool value);

        TYPE_ELEMENT getCellValue(int index) {
            return cells[index];
        }

    private:
        /*
         * The cell data for this vector
         */
        // 2018_02_10
        TYPE_ELEMENT *cells;
};

class ConnexState {
    public:
      // Actual state vectors:
        /*
         * The active cell flags
         */
        ConnexVector active;

        /*
         * The carry cell flags
         */
        ConnexVector carryFlag;

        /*
         * The equal cell flags
         */
        ConnexVector eqFlag;

        /*
         * The less than cell flags
         */
        ConnexVector ltFlag;

      // Miscellaneous Connex vectors:
        /*
         * Least significat half of the 32 bits multiplication result
         */
        ConnexVector multLow;

        /*
         * Most significat half of the 32 bits multiplication result
         */
        ConnexVector multHigh;

        /*
         * The cell shift register
         */
        ConnexVector shiftReg;

        /*
         * The remaining shifts required for each cells
         */
        ConnexVector shiftCountReg;
};


#endif // CONNEX_VECTOR_H

