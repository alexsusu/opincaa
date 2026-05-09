// We should build this with gcc -O3 to allow vectorization of loop for idx in PadArray

// TODO: See if memcpy & memset use SIMD (as they should: https://github.com/tc39/ecmascript_simd/issues/94 - SIMD versions of memset, memcpy, str* functions). Otherwise try to use wmemcpy & wmemset

#ifndef PAD_ARRAY_OPINCAA_H
#define PAD_ARRAY_OPINCAA_H

#include <string.h>



/*
//inline void Mymemcpy(void *dst, void *src, int countBytes) {
//inline void Mymemcpy(char *dst, char *src, int countBytes) {
inline void Mymemcpy(ConnexVectorElementType *dst, ConnexVectorElementType *src, int countBytes) {
    int i;

    assert(0 && "UNfinished");
    for (i = 0; i < countBytes / sizeof(ConnexVectorElementType); i++) {
        dst[i] = src[i];
    }
}
*/

typedef short ConnexVectorElementType;

//int sizeOfType = 2;

void PrintArray(void *A,
                int sizeOfType,
                int numRows, int numCols, char *aStr) {
    int row, col;

    dprintf("Entered PrintArray(numRows = %d, numCols = %d): %s\n",
           numRows, numCols, aStr);
    dfflush(stdout);

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

    dprintf("END PrintArray()\n");
    dfflush(stdout);
}


/*
  We return:
     - 0 if no padding was required and the operation was a success
     - 1 if padding was required and the operation was a success
     - -1 if not success

   ustAllocate = 1 --> we just allocate the right size for the padded array,
        without copying the data in it
*/
// TODO TODO TODO: pass sizeofTYPE as parameter
inline int PadArray(void *A,
                    void **Apadded, // pointer to the result array
                    int sizeOfType,
                    int numRows, int numCols,
                    int numColsAfterPadding,
                    char justAllocate,
                    /*ConnexVectorElementType*/ int neutralElement = 0) {
    int row, col;
    int idx;

  #ifdef DEBUG_OPINCAA
    printf("PadArray(): numRows = %d\n", numRows);
    printf("PadArray(): numCols = %d\n", numCols);
    printf("PadArray(): numColsAfterPadding = %d\n", numColsAfterPadding);
    printf("PadArray(): justAllocate = %d\n", justAllocate);
    printf("PadArray(): A = %p\n", A);
    fflush(stdout);
  #endif

    /*
    assert(neutralElement == 0 &&
           "We have some difficulties to handle non-zero neutralElement when "
           "ConnexVectorElementType is short");

    printf("Entered PadArray(numRows = %d, numCols = %d, "
           "numColsAfterPadding = %d, neutralElement = %d):\n",
           numRows, numCols, numColsAfterPadding, neutralElement);
    fflush(stdout);
    */
    if (numColsAfterPadding == numCols) {
        dprintf("PadArray(): returning original array since "
               "numColsAfterPadding == numCols\n");
        dfflush(stdout);

        *Apadded = A;
        return 0;
    }

  #ifdef DEBUG_OPINCAA
    printf("   PadArray(): A = %p\n", A);
    printf("   PadArray(): *Apadded = %p\n", *Apadded);
    fflush(stdout);
  #endif

    if (justAllocate == 1)
        return 1;

    for (row = 0; row < numRows; row++) {
        //printf("row = %d\n", row);
        /* printf("   PadArray(): in loop: row = %d\n", row);
        fflush(stdout); */

        //memcpy(&Apadded[row][0], &A[row][0], sizeof(ConnexVectorElementType) * numCols);
        memcpy((char *)(*Apadded) + (row * numColsAfterPadding + 0) * sizeOfType,
               (char *)A + (row * numCols + 0) * sizeOfType,
               //sizeof(ConnexVectorElementType) * numCols
               sizeOfType * numCols
               );
        /*
        Mymemcpy(&(*Apadded)[row * numColsAfterPadding + 0],
                 &A[row * numCols + 0], sizeof(ConnexVectorElementType) * numCols);

        //memset(&Apadded[row][numCols], 0,
                 sizeof(ConnexVectorElementType) * (CONNEX_VECTOR_LENGTH - numCols));
        */

        if (neutralElement == 0) {
            memset((char *)(*Apadded) +
                     (row * numColsAfterPadding + numCols) * sizeOfType,
                   neutralElement,
                   //sizeof(ConnexVectorElementType) * (numColsAfterPadding - numCols)
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

    dprintf("UnpadArray(): prodSizesAllDimsButLastA = %d\n",
           prodSizesAllDimsButLastA);
    dprintf("UnpadArray(): sizeLastDimA = %d\n", sizeLastDimA);
    dprintf("UnpadArray(): sizeLastDimAfterPaddingA = %d\n",
           sizeLastDimAfterPaddingA);

    for (row = 0; row < prodSizesAllDimsButLastA; row++) {
        memcpy((char *)A + (row * sizeLastDimA + 0) * sizeOfType,
                (char *)Apadded +
                  (row * sizeLastDimAfterPaddingA + 0) * sizeOfType,
                //sizeof(ConnexVectorElementType) * sizeLastDimA
                sizeOfType * sizeLastDimA
                );
        /*
        memset(&C[row][numRowsCols], 0,
               sizeof(ConnexVectorElementType) * (CONNEX_VECTOR_LENGTH - numRowsCols));
        */
    }

    if (sizeLastDimA % CONNEX_VECTOR_LENGTH > 0) {
     #ifdef DEBUG_OPINCAA
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

    dprintf("PadArrayIfRequired(): A = %p\n", A);
    dprintf("PadArrayIfRequired(): sizeOfType = %d\n", sizeOfType);
    dprintf("PadArrayIfRequired(): prodSizesAllDimsButLastA = %d\n",
           prodSizesAllDimsButLastA);
    dprintf("PadArrayIfRequired(): sizeLastDimA = %d\n", sizeLastDimA);
    dprintf("PadArrayIfRequired(): numElemsAccessedA = %d\n",
           numElemsAccessedA);
    dprintf("PadArrayIfRequired(): numElemsAccessedAfterPaddingA = %d\n",
           numElemsAccessedAfterPaddingA);

    dprintf("PadArrayIfRequired(): justAllocate = %d\n", justAllocate);
    dfflush(stdout);

    /*
    // 2018_05_26
    assert(prodSizesAllDimsButLastA != 1 &&
            "A seems NOT to be a multidimensional array");
    */

    //int sizeLastDimAfterPaddingA = sizeLastDimA % CONNEX_VECTOR_LENGTH;

    // 2018_10_26
    //int CVLadjusted = CONNEX_VECTOR_LENGTH;
    int CVLadjusted = CONNEX_VECTOR_LENGTH / (sizeOfType / sizeof(ConnexVectorElementType));

    if (sizeLastDimA % CVLadjusted > 0) {
        sizeLastDimAfterPaddingA = sizeLastDimA +
                                    (CVLadjusted -
                                     (sizeLastDimA % CVLadjusted));

        numElemsAccessedAfterPaddingA = prodSizesAllDimsButLastA *
                                            sizeLastDimAfterPaddingA *
                                            (sizeOfType/sizeof(short)); // 2018_10_26
        dprintf("PadArrayIfRequired(): sizeLastDimAfterPaddingA = %d\n",
                sizeLastDimAfterPaddingA);
        dprintf("PadArrayIfRequired(): After: numElemsAccessedAfterPaddingA = %d\n",
                numElemsAccessedAfterPaddingA);
        dfflush(stdout);

        Apadded = malloc(numElemsAccessedAfterPaddingA * sizeof(ConnexVectorElementType));
        //Apadded = malloc(numElemsAccessedAfterPaddingA * sizeOfType);
        assert(Apadded != NULL);

        //printf("sizeof(wchar_t) = %lu\n", sizeof(wchar_t));

        int resApadding = PadArray(A, &Apadded,
                                    sizeOfType,
                                    //sizeLastDimA, // 2018_05_26
                                    prodSizesAllDimsButLastA, // 2018_05_26
                                    //prodSizesAllDimsButLastA / (sizeOfType/sizeof(short)), // 2018_10_26
                                    sizeLastDimA,
                                    sizeLastDimAfterPaddingA,
                                    justAllocate);
        dprintf("PadArrayIfRequired(): Returned from PadArray()\n");
        dfflush(stdout);

      #ifdef DEBUG_OPINCAA
        PrintArray((ConnexVectorElementType *)Apadded,
                    sizeOfType,
                    //sizeLastDimA, // 2018_05_26
                      prodSizesAllDimsButLastA, // 2018_05_26
                    sizeLastDimAfterPaddingA,
                    (char *)"PadArrayIfRequired(): Apadded = ");
      #endif
    }
    else {
        Apadded = A;

        sizeLastDimAfterPaddingA = sizeLastDimA;

        dprintf("PadArrayIfRequired(): sizeLastDimAfterPaddingA = %d\n",
                sizeLastDimAfterPaddingA);
        dprintf("PadArrayIfRequired(): Returning the original array A as Apadded\n");
        dfflush(stdout);

        // numElemsAccessedAfterPaddingA is left as obtained from SRA
        numElemsAccessedAfterPaddingA = numElemsAccessedA;
    }

    dprintf("PadArrayIfRequired(): A = %p, Apadded = %p\n", A, Apadded);

    dprintf("PadArrayIfRequired(): "
           "(int)ceil(((float)numElemsAccessedA) / CVLadjusted) = %d\n",
            (int)ceil(((float)numElemsAccessedA) / CVLadjusted));
    dprintf("PadArrayIfRequired(): "
           "numElemsAccessedAfterPaddingA / CVLadjusted = %d\n",
           numElemsAccessedAfterPaddingA / CVLadjusted);
    dprintf("PadArrayIfRequired(): "
           "sizeLastDimAfterPaddingA / CVLadjusted = %d\n",
           sizeLastDimAfterPaddingA / CVLadjusted);
    dfflush(stdout);

    return Apadded;
}

#endif // PAD_ARRAY_OPINCAA_H

