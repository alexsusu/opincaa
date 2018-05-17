// We should build this with gcc -O3 to allow vectorization of loop for idx in PadArray

// TODO: See if memcpy & memset use SIMD (as they should: https://github.com/tc39/ecmascript_simd/issues/94 - SIMD versions of memset, memcpy, str* functions). Otherwise try to use wmemcpy & wmemset

#ifndef ARRAY_OPINCAA_H
#define ARRAY_OPINCAA_H

#include <string.h>



#define DEBUG_ALEX

/*
//inline void Mymemcpy(void *dst, void *src, int countBytes) {
//inline void Mymemcpy(char *dst, char *src, int countBytes) {
inline void Mymemcpy(TYPE_ELEMENT *dst, TYPE_ELEMENT *src, int countBytes) {
    int i;

    assert(0 && "UNfinished");
    for (i = 0; i < countBytes / sizeof(TYPE_ELEMENT); i++) {
        dst[i] = src[i];
    }
}
*/

typedef short TYPE_ELEMENT;

//int sizeOfType = 2;

void PrintArray(void *A,
                int sizeOfType,
                int numRows, int numCols, char *aStr) {
    int row, col;

    printf("Entered PrintArray(numRows = %d, numCols = %d): %s\n",
           numRows, numCols, aStr);
    fflush(stdout);

    for (row = 0; row < numRows; ++row) {
        for (col = 0; col < numCols; ++col) {
          //printf("%d ", A[row * numCols + col]);
          if (sizeOfType == 2) {
              // assuming short (i16)
              printf("%hd ", ((short *)A)[row * numCols + col]);
              // TODO TODO: it could also be fp16, etc
          }
          else
          if (sizeOfType == 4)
              printf("%d ", (int) *((char *)A + (row * numCols + col) * sizeOfType));
        }
        printf("\n");
    }
}


/*
  We return:
     - 0 if no padding was required and the operation was a success
     - 1 if padding was required and the operation was a success
     - -1 if not success

   justAllocate = 1 --> we just allocate the right size for the padded array,
        without copying the data in it
*/
// TODO TODO TODO: pass sizeofTYPE as parameter
inline int PadArray(void *A,
                    void **Apadded, // pointer to the result array
                    int sizeOfType,
                    int numRows, int numCols,
                    int numColsAfterPadding,
                    char justAllocate,
                    /*TYPE_ELEMENT*/ int neutralElement = 0) {
    int row, col;
    int idx;

  #ifdef DEBUG_ALEX
    printf("PadArray(): numRows = %d\n", numRows);
    printf("PadArray(): numCols = %d\n", numCols);
    printf("PadArray(): numColsAfterPadding = %d\n", numColsAfterPadding);
    printf("PadArray(): justAllocate = %d\n", justAllocate);
    printf("PadArray(): A = %p\n", A);
  #endif

    /*
    assert(neutralElement == 0 &&
           "We have some difficulties to handle non-zero neutralElement when "
           "TYPE_ELEMENT is short");

    printf("Entered PadArray(numRows = %d, numCols = %d, "
           "numColsAfterPadding = %d, neutralElement = %d):\n",
           numRows, numCols, numColsAfterPadding, neutralElement);
    fflush(stdout);
    */
    if (numColsAfterPadding == numCols) {
        printf("PadArray(): returning original array since "
               "numColsAfterPadding == numCols\n");
        fflush(stdout);

        *Apadded = A;
        return 0;
    }

  #ifdef DEBUG_ALEX
    printf("   PadArray(): A = %p\n", A);
    printf("   PadArray(): *Apadded = %p\n", *Apadded);
  #endif

    if (justAllocate == 1)
        return 1;

    for (row = 0; row < numRows; row++) {
        //printf("row = %d\n", row);
        /* printf("   PadArray(): in loop: row = %d\n", row);
        fflush(stdout); */

        //memcpy(&Apadded[row][0], &A[row][0], sizeof(TYPE_ELEMENT) * numCols);
        memcpy((char *)(*Apadded) + (row * numColsAfterPadding + 0) * sizeOfType,
               (char *)A + (row * numCols + 0) * sizeOfType,
               //sizeof(TYPE_ELEMENT) * numCols
               sizeOfType * numCols
               );
        /*
        Mymemcpy(&(*Apadded)[row * numColsAfterPadding + 0],
                 &A[row * numCols + 0], sizeof(TYPE_ELEMENT) * numCols);

        //memset(&Apadded[row][numCols], 0,
                 sizeof(TYPE_ELEMENT) * (CONNEX_VECTOR_LENGTH - numCols));
        */

        if (neutralElement == 0) {
            memset((char *)(*Apadded) +
                     (row * numColsAfterPadding + numCols) * sizeOfType,
                   neutralElement,
                   //sizeof(TYPE_ELEMENT) * (numColsAfterPadding - numCols)
                   sizeOfType * (numColsAfterPadding - numCols)
                   );
        }
        else {
            for (idx = row * numColsAfterPadding + numCols;
                   idx < (row + 1) * numColsAfterPadding;
                   idx++) {
                *((char *)(*Apadded) + idx * sizeOfType) = neutralElement;
            }
        }
        //for (col = 0; col < SIZE; col++) { }
    }

    return 1;
}

inline void UnpadArray(void *A,
                       void *Apadded,
                       int sizeOfType,
                       int prodSizesAllDimsButLastA,
                       int sizeLastDimA,
                       int sizeLastDimAfterPaddingA) {
    int row, col;

    printf("UnpadArray(): prodSizesAllDimsButLastA = %d\n",
           prodSizesAllDimsButLastA);
    printf("UnpadArray(): sizeLastDimA = %d\n", sizeLastDimA);
    printf("UnpadArray(): sizeLastDimAfterPaddingA = %d\n",
           sizeLastDimAfterPaddingA);

    for (row = 0; row < prodSizesAllDimsButLastA; row++) {
        memcpy((char *)A + (row * sizeLastDimA + 0) * sizeOfType,
                (char *)Apadded +
                  (row * sizeLastDimAfterPaddingA + 0) * sizeOfType,
                //sizeof(TYPE_ELEMENT) * sizeLastDimA
                sizeOfType * sizeLastDimA
                );
        /*
        memset(&C[row][numRowsCols], 0,
               sizeof(TYPE_ELEMENT) * (CONNEX_VECTOR_LENGTH - numRowsCols));
        */
    }

    if (sizeLastDimA % CONNEX_VECTOR_LENGTH > 0) {
     #ifdef DEBUG_ALEX
      PrintArray(Apadded,
                 sizeOfType,
                 sizeLastDimA,
                 sizeLastDimAfterPaddingA,
                 (char *)"UnpadArray(): Apadded = ");
     #endif
    }
}



/*
 * We assume A is stored in row-major order, or equivalent for more dimensions.
 *
   Note: sizeLastDimA should actually be the number of elements accessed in the
           iteration (from the entire loop nest) with ind-var for the last
           dimension of the array.
      Similarly, prodSizesAllDimsButLastA should refer to all the elements
          accessed within the loop nest EXCEPT the iteration with ind-var for
          the last dimension of the array.

   justAllocate = 1 --> we just allocate the right size for the padded array,
        without copying the data in it
 */
// TODO TODO TODO: pass sizeofTYPE as parameter
void *PadArrayIfRequired(void *A,
                         int sizeOfType,
                         int prodSizesAllDimsButLastA,
                         int sizeLastDimA,
                         int &sizeLastDimAfterPaddingA,
                         int numElemsAccessedA,
                         int &numElemsAccessedAfterPaddingA,
                         char justAllocate) {
    void *Apadded;

  #ifdef DEBUG_ALEX
    printf("PadArrayIfRequired(): A = %p\n", A);
    printf("PadArrayIfRequired(): sizeOfType = %d\n", sizeOfType);
    printf("PadArrayIfRequired(): prodSizesAllDimsButLastA = %d\n",
           prodSizesAllDimsButLastA);
    printf("PadArrayIfRequired(): sizeLastDimA = %d\n", sizeLastDimA);
    printf("PadArrayIfRequired(): numElemsAccessedA = %d\n",
           numElemsAccessedA);
    printf("PadArrayIfRequired(): numElemsAccessedAfterPaddingA = %d\n",
           numElemsAccessedAfterPaddingA);

    printf("PadArrayIfRequired(): justAllocate = %d\n", justAllocate);
  #endif

    assert(prodSizesAllDimsButLastA != 1 &&
            "A seems NOT to be a multidimensional array");

    //int sizeLastDimAfterPaddingA = sizeLastDimA % CONNEX_VECTOR_LENGTH;

    if (sizeLastDimA % CONNEX_VECTOR_LENGTH > 0) {
        sizeLastDimAfterPaddingA = sizeLastDimA +
                                    (CONNEX_VECTOR_LENGTH -
                                     (sizeLastDimA % CONNEX_VECTOR_LENGTH));

        numElemsAccessedAfterPaddingA = prodSizesAllDimsButLastA *
                                            sizeLastDimAfterPaddingA;
      #ifdef DEBUG_ALEX
        printf("PadArrayIfRequired(): sizeLastDimAfterPaddingA = %d\n",
                sizeLastDimAfterPaddingA);
        printf("PadArrayIfRequired(): After: numElemsAccessedAfterPaddingA = %d\n",
                numElemsAccessedAfterPaddingA);
      #endif

        Apadded = malloc(numElemsAccessedAfterPaddingA * sizeof(TYPE_ELEMENT));
        assert(Apadded != NULL);

        //printf("sizeof(wchar_t) = %lu\n", sizeof(wchar_t));

        int resApadding = PadArray(A, &Apadded,
                                    sizeOfType,
                                    sizeLastDimA,
                                    sizeLastDimA,
                                    sizeLastDimAfterPaddingA,
                                    justAllocate);

      #ifdef DEBUG_ALEX
        PrintArray((TYPE_ELEMENT *)Apadded,
                    sizeOfType,
                    sizeLastDimA,
                    sizeLastDimAfterPaddingA,
                    (char *)"PadArrayIfRequired(): Apadded = ");
      #endif
    }
    else {
        Apadded = A;

        sizeLastDimAfterPaddingA = sizeLastDimA;

      #ifdef DEBUG_ALEX
        printf("PadArrayIfRequired(): sizeLastDimAfterPaddingA = %d\n",
                sizeLastDimAfterPaddingA);
        printf("PadArrayIfRequired(): Returning the original array A as Apadded\n");
      #endif

        // numElemsAccessedAfterPaddingA is left as obtained from SRA
        numElemsAccessedAfterPaddingA = numElemsAccessedA;
    }

  #ifdef DEBUG_ALEX
    printf("PadArrayIfRequired(): A = %p, Apadded = %p\n", A, Apadded);

    printf("PadArrayIfRequired(): "
           "(int)ceil(((float)numElemsAccessedA) / CONNEX_VECTOR_LENGTH) = %d\n",
            (int)ceil(((float)numElemsAccessedA) / CONNEX_VECTOR_LENGTH));
    printf("PadArrayIfRequired(): "
           "numElemsAccessedAfterPaddingA / CONNEX_VECTOR_LENGTH = %d\n",
           numElemsAccessedAfterPaddingA / CONNEX_VECTOR_LENGTH);
    printf("PadArrayIfRequired(): "
           "sizeLastDimAfterPaddingA / CONNEX_VECTOR_LENGTH = %d\n",
           sizeLastDimAfterPaddingA / CONNEX_VECTOR_LENGTH);
  #endif

    return Apadded;
}

#endif
