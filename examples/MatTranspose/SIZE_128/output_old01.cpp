#include <assert.h>
#include <stdio.h>


typedef short TYPE;

//const int SIZE = 128;
#define SIZE 128


//#define LLVM_ISEL_CODEGEN
#ifdef LLVM_ISEL_CODEGEN
  //#define PRINTREG(x) x
  #define PRINTREG(regNum) regNum
  #define PrintDebugMessage(aStr) aStr
  #define PrintRegDebug(regNum) regNum
#endif



void PrintMatrix(TYPE M[SIZE][SIZE], int numRowsCols) {
    int row, col;
    //printf("A =\n");
    for (row = 0; row < numRowsCols; ++row) {
        for (col = 0; col < numRowsCols; ++col) {
            //printf("C[%d][%d] = %d\n");
            printf("%d ", M[row][col]);
        }
        printf("\n");
    }
    printf("\n");
}



/* From http://users.dcae.pub.ro/~gstefan/2ndLevel/teachingMaterials/0_FE.pdf,
     (FUNCTIONAL ELECTRONICS by Gheorghe M. Stefan)
     book page 49, Section 5.1.1
*/
void MatTranspose(TYPE src[SIZE][SIZE], TYPE dst[SIZE][SIZE], int N) {
    int size = N;
    //int cycles = N - 1;
    int cycles = N;

    printf("Entered MatTranspose(N = %d)\n", N);
    fflush(stdout);

    #define LS_OFFSET_SRC 0
    #define LS_OFFSET_DST 128
    //
    #define CT_MOD128_ONEBITS 23
    #define CT1 14
    #define IDX 13
    #define ACC 15
    #define CYCLE 16
    #define CYCLE2 30
    #define CYCLE3 31
    #define sAddr 17
    #define sAddr2 18
    #define sAddr3 19
    #define dAddr  20
    #define dAddr2 21
    #define dAddr3 22


    connexGlobal->writeDataToConnexPartial(src,
             /*numVectors*/ (int)ceil(((float)N * N) / CONNEX_VECTOR_LENGTH),
             /* actual num elems written */ N * N, /*offset*/ LS_OFFSET_SRC);
            // writeDataToConnexPartial() is blocking



  _BEGIN_KERNEL(BatchNumberGlobal);
      EXECUTE_IN_ALL(
             // !!!!!!!!!!!!!Why doesn't he use WHERE?

  #ifdef ANOTHER
    R(CYCLE3) = 128;
  #endif

    R(CT1) = 1;
    R(CT_MOD128_ONEBITS) = CONNEX_VECTOR_LENGTH - 1;

    //R(sAddr) = 0;
        // the address of the first line of the input matrix vector line in LS
    R(sAddr2) = LS_OFFSET_SRC;
    //
    //R(dAddr) = 0; //LS_OFFSET_DST;
        // the address of the first line of the target matrix vector line in LS
    R(dAddr2) = LS_OFFSET_DST;

    R(IDX) = INDEX; // ixModN
    //R(IDX) = 0; // ixModN
    //R(IDX) = R(IDX) - (R(IDX) / N) * N; // compute INDEX % N (we can consider that it's just INDEX, for the case CVL == N)

    R(CYCLE) = N; //cycles;

    // compute "diagonal"
    //sAddr = (ixModN - cycles) mod N;
    R(sAddr) = R(IDX) - R(CYCLE);
    R(sAddr) = R(sAddr) & R(CT_MOD128_ONEBITS);
    /* For the 1st step we have - (N - 1) mod N == + 1 --> this is the diagonal below the main diagonal
        Ex: - (4 - 1) mod 4 = -3 mod 4 = + 1
    */

    // compute the "opposite diagonal" (symmetric with the main diagonal) of the diagonal above
    //dAddr = (ixModN + cycles) mod N;
    R(dAddr) = R(IDX) + R(CYCLE);
    R(dAddr) = R(dAddr) & R(CT_MOD128_ONEBITS);

// TODO: take into consideration the stall of Connex for a new instruction - maybe during this time the CELLSH* instructions still execute, which would save us writing NOPs

    //REPEAT (cycles > 0)
  for (int Rcycle = N; Rcycle >= 0; Rcycle--) {
    //REPEAT_X_TIMES(N); // (N - 1);
        //acc <= LS[sAddr]; // read on "diagonal"
        R(sAddr3) = R(sAddr) + R(sAddr2);
        R(ACC) = LS[R(sAddr3)]; // compute "diagonal"
        // For the 1st step we have 1230

        /* For the 1st step we have 1230 (as already said) and we left-shift
          register by cycles = 3 positions, so the register will have 0123.
          It might be a bit difficult to picture this - look at a matrix
           figure and see exactly the source diagonal and the destination diagonal.
         */
        // glshift(cycles); // global left shift


      #ifdef STRAIGHT_FORWARD_NOPS
        CELL_SHL(R(ACC), R(CYCLE));
        for (int iNOP = 0; iNOP < Rcycle; iNOP++) {
            NOP;
        }
     #else
        if (Rcycle > 63) {
         #ifdef ANOTHER
          R(CYCLE2) = R(CYCLE3) - R(CYCLE);
         #else
          R(CYCLE2) = 128 - Rcycle;
         #endif

          CELL_SHR(R(ACC), R(CYCLE2));
          for (int iNOP = 0; iNOP < 128 - Rcycle; iNOP++) {
            NOP;
          }
        }
        else {
          CELL_SHL(R(ACC), R(CYCLE));
          for (int iNOP = 0; iNOP < Rcycle; iNOP++) {
            NOP;
          }
        }
     #endif

        R(ACC) = SHIFT_REG;

        R(dAddr3) = R(dAddr) + R(dAddr2);
        LS[R(dAddr3)] = R(ACC); // store on the "opposite diagonal"


      #ifdef NNNNNO
        // I do NOT understand why does he do again this load and shift
        Racc = LS[sAddr]; // read on "diagonal"
        // grshift(N - cycles); // global right shift

        where (ixModN >= N - cycles) {
            LS[dAddr] = Racc; // store on the "opposite diagonal"
        }

        sAddr = (sAddr + 1) % N; // "increment diagonal"
        dAddr = (dAddr - 1) % N; // "decrement diagonal"
      #endif

        R(sAddr) = R(sAddr) + R(CT1); // "increment diagonal"
        R(sAddr) = R(sAddr) & R(CT_MOD128_ONEBITS);

        R(dAddr) = R(dAddr) - R(CT1); // "decrement diagonal"
        R(dAddr) = R(dAddr) & R(CT_MOD128_ONEBITS);

        //cycles = cycles - 1; // decrement cycles
        R(CYCLE) = R(CYCLE) - R(CT1);
    //END_REPEAT;
  }


    REDUCE R(0); // We add a 'bogus' REDUCE to wait for it

      );
    _END_KERNEL(BatchNumberGlobal);
  connexGlobal->executeKernel(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
  connexGlobal->readReduction();

  connexGlobal->readDataFromConnexPartial(dst,
             /*numVectors*/ (int)ceil(((float)N * N) / CONNEX_VECTOR_LENGTH),
             /* actual num elems written */ N * N, /*offset*/ LS_OFFSET_DST);
            // readDataFromConnexPartial() is blocking
}








#ifdef TEST_OPINCAA_CONNEX_KERNELS
void Test() {
  int i, j;
  long row, column;

  TYPE A[SIZE][SIZE];
  TYPE At[SIZE][SIZE];

  printf("SIZE = %d\n", SIZE);

  // We initialize the A matrix
  for (i = 0; i < SIZE; ++i)
    for (j = 0; j < SIZE; ++j)
      A[i][j] = (i + 2 * j + i*j*j) % 10;


  printf("Calling MatTranspose()...\n");
  fflush(stdout);

  MatTranspose(A, At, SIZE);

  printf("Finished executing the Opincaa kernel.\n");
  fflush(stdout);
  //
  printf("Testing the correctness of the computation of the Opincaa kernel.\n");
  fflush(stdout);


  /*
  */
  printf("A =\n");
  PrintMatrix(A, SIZE);
  printf("\n\n");
  printf("At =\n");
  PrintMatrix(At, SIZE);



//#define PRINT_AT

#ifdef PRINT_AT
  printf("C =\n");
#endif
  for (row = 0; row < SIZE; ++row) {
    for (column = 0; column < SIZE; ++column) {
// printf("At[%d][%d] = %d\n");
#ifdef PRINT_AT
      printf("%d ", At[row][column]);
#endif
      assert(At[row][column] == A[column][row]);
    }
#ifdef PRINT_AT
    printf("\n");
#endif
  }

  printf("Test passed OK\n");

}

/*
int main() {
    Test();

    return 0;
}
*/
#endif

