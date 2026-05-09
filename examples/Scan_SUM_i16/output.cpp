/*
It looks that this code has the minimum (optimum) execution time.
But maybe we can do better.
 See book V. Cristea for prefix operator implementations.
 Maybe, as "always", we can put instead of NOPs immediately after the CELL_SH operations, other useful instructions.
 MEGA-TODO: try to reduce the number of cycles - maybe we can generate the REG_CT_DISPLACEMENT faster(?); maybe we can somehow reuse values of consecutive REG_CT_DISPLACEMENT (although it looks difficult since each vector has a different numeber of zeros and transforming the previous REG_CT_DISPLACEMENT to the current one requires other CELL_SH operations, which are very expensive).

small-TODO - see that in the Elusive...pdf they do also reverse-scan.

Mega-TODO TODO TODO: do a Scan for a scan larger than CVL.
    also, they have a "carry" technique if our vector is larger than CVL.
*/

//#define LLVM_ISEL_CODEGEN
#ifdef LLVM_ISEL_CODEGEN
  #define PRINTREG(regNum) regNum
  #define PrintDebugMessage(aStr) aStr
  #define PrintRegDebug(regNum) regNum
#endif

/*
%\section{About the Pack and the Scan Data Parallel Management Patterns}
\section{About the Scan Data Parallel Management Patterns}
    The scan (prefix) pattern is a higher order function, similar to reduce (or map).
    It takes an associative binary operation, %in its various alternatives (
        such as sum, max, xor, or and a data collection and returns all partial reductions of the collection.
        This is why we say it is a generalization of the reduce function~\cite{McCool_2012}. % McCool, Sec 3.3.5
    Scan is a pattern used to parallelize quicksort, radix sort, implement tree operations, compare strings, perform lexical analysis, etc~\cite{BlellochTR90}.
    %    %From https://en.wikipedia.org/wiki/Prefix_sum talks about counting sort:
    %    - <<Counting sort is an integer sorting algorithm that uses the prefix sum of a histogram of key frequencies to calculate the position of each key in the sorted output array.
    %    It runs in linear time for integer keys that are smaller than the number of items, and is frequently used as part of radix sort, a fast algorithm for sorting integers that are less restricted in magnitude.>> [CLR2001]
    %    - "by combining list ranking, prefix sums, and Euler tours, many important problems on trees may be solved by efficient parallel algorithms." Tarjan, Robert E.; Vishkin, Uzi (1985), "An efficient parallel biconnectivity algorithm", SIAM Journal on Computing, 14 (4): 862–874,
    %    - "An early application of parallel prefix sum algorithms was in the design of binary adders, Boolean circuits that can add two n-bit binary numbers. In this application, the sequence of carry bits of the addition can be represented as a scan operation on the sequence of pairs of input bits, using the majority function to combine the previous carry with these two bits. Each bit of the output number can then be found as the exclusive or of two input bits with the corresponding carry bit. By using a circuit that performs the operations of the parallel prefix sum algorithm, it is possible to design an adder that uses O(n) logic gates and O(log n) time steps." [Ladner, Fisher, 1980]

    The book of professor \c{S}tefan, 0_BOOK mentions about a scan implementation in hardware.
    %To implement the scan pattern we need an elementary operation: to permute vectors.


We implement scan for CVL = 128 lanes.
   This Scan operator for CVL = 128 lanes takes a total of 292 cycles.

Details about the scan operator also in books SPP_2012, Akl,
      Thomson_Leighton_1994.
*/

/*
From https://software.intel.com/sites/default/files/managed/10/a4/Elusive%20Algorithms%20-%20parallel%20scan.pdf
    For standard AVX:
        Fetch: 1  2  3  4  5  6  7  8                                   // x     = input vector

        perm:  1  1  3  3  5  5  7  7 ([0]->[1], [2]->[3] ... [6]->[7]) // perm0 = CELLSHR x, [0, 1, 0, 1, 0, 1, 0, 1]
        and:   0  1  0  3  0  5  0  7 (remove unwanted)                 // and0  = perm0 & [00, FF, 00, FF, ...]
        add:   1  3  3  7  5 11  7 15                                   // add0  = x + and0

        perm:  1  3  3  3  5 11 11 11 ([1]->[2:2], [5]->[6:2])          // perm1 = CELLSHR add0, [0, 0, 1, 1, 0, 1, 1 (or 2???), 0]??? Not sure if I'm missing something
        and:   0  0  3  3  0  0 11 11 (remove unwanted)                 // and1  = perm1 & [00, 00, FF, FF, 00, 00, FF, FF]
        add:   1  3  6 10  5 11 18 26                                   // add1  = add0 + and1

        //perm:  0  0  0  0  1  3  6 10
        perm:  0  0  0  0 10 10 10 10 ([3]->[4:4])                      // perm2 = CELLSHR add1, [0, 0, 0, 0, 1, 2, 3, 4]
                                                                        // and2  = perm2 & [00, 00, 00, 00, FF, FF, FF, FF]
        add    1  3  6 10 15 21 28 36                                   // add2 = add1 + and2
        Out:   1  3  6 10 15 21 28 36
        // We should use a Carry also

    Note: for a vector of width 8 I use 8 instructions (it seems SSE/AVX is able to perform 1 instruction in 1 cycle)

*/

// Inspired from https://software.intel.com/en-us/articles/elusive-algorithms-parallel-scan

void ScanKernel() {
    int i, iNumNOPs, indexNOP;

    #define REG_CT1 31
    #define REG_CT0 30
    #define REG_CT01 29
    #define REG_CT10 28
    #define REG_CT_AUX 12

    //#define REG_SRC R9
    //#define REG_DST R8
    #define REG_SRC 20
    #define REG_SRC01 18
    #define REG_SRC01_SHR 17
    #define REG_CT_DISPLACEMENT 16

    #define REG_POW2 15
    #define REG_POW2_MINUS_1 14

    #define REG_AUX 10
    #define REG_AUX2 9
    #define REG_AUX3 8


    //#define BETTER_COMPL


    printf("We compute sum-scan over a vector [1, 2, 3, ..., CVL]\n");
    fflush(stdout);

    BEGIN_KERNEL("SumScan.i16");
        EXECUTE_IN_ALL(
            // A test:
            //R(REG_SRC) = INDEX;
            // A test:
            R(REG_CT1) = 1;
            R(REG_SRC) = INDEX;
            R(REG_SRC) += R(REG_CT1); // REG_CT1 MUST be defined


            R(REG_CT1) = 1;
            R(REG_CT0) = 0;

            R(REG_POW2) = 2;

            R(REG_CT01) = INDEX;
            R(REG_CT01) = R(REG_CT01) & R(REG_CT1); // 0101...
            /*
            // Another alternative to compute it is:
            R(REG_CT01) = INDEX;
            R(REG_AUX) = R(REG_CT01) == R(REG_CT0);
            NOP;
            EXECUT_WHERE_EQ(
              R(REG_CT01) = R(REG_CT1);
            );
            */
            //PRINT_REG(REG_CT01);
            PrintRegDebug(REG_CT01);



            /*
            R(REG_CT10) = INDEX;
            R(REG_CT10) += R(REG_CT1);
            R(REG_CT10) = R(REG_CT10) & R(REG_CT1); // 1010...
            */
            R(REG_CT10) = ~R(REG_CT01); // 1010...
            PrintRegDebug(REG_CT10);


//i = 0;

          PrintRegDebug(REG_SRC);

            //R(REG_A)
            // We do the equivalent of: t1 = _mm512_swizzle_ps(Out,_MM_SWIZ_REG_CDAB);
            /*
            R(REG_AUX) = R(REG_CT01) == R(REG_CT0);
            NOP;
            EXECUT_WHERE_EQ(
              R(REG_A01) = R(REG_A);
            );
            */
            //
            CELL_SHR( R(REG_SRC), R(REG_CT01));
            for (indexNOP = 0; indexNOP < iNumNOPs; indexNOP++)
                NOP;
            R(REG_SRC01_SHR) = SHIFT_REG;
            PrintRegDebug(REG_SRC01_SHR);

            R(REG_SRC01) = 0;
            R(REG_AUX) = R(REG_CT01) == R(REG_CT1);
            NOP;
          );
            EXECUTE_WHERE_EQ(
              R(REG_SRC01) = R(REG_SRC01_SHR); // 0  1  0  3  0  5  0  7 (remove unwanted)                 // and0  = perm0 & [00, FF, 00, FF, ...]
            );

      EXECUTE_IN_ALL(
        PrintRegDebug(REG_SRC01);

            R(REG_SRC) += R(REG_SRC01);
            PrintDebugMessage("After 1st 'iteration':\n");
            //PrintRegDebug(REG_SRC01);
            PrintRegDebug(REG_SRC);




    iNumNOPs = 2;
    for (i = 1; i < 7; iNumNOPs *= 2, i++) {
            /* 2nd "permute" step
              src:  1  3  3  7  5 11  7 15
             perm:  1  3  3  3  5 11 11 11 ([1]->[2:2], [5]->[6:2])

             perm1 = CELLSHR add0, [0, 0, 1, 2, 0, 0, 1, 2]

             displacement = INDEX;
             displacement & 3 < 2:
                displacement = 0;
             displacement & 3 >= 2:
                displacement -= 1;
            */

            /* 3rd "permute" step
              src:  1  3  6 10  5 11 18 26
             perm:  0  0  0  0 10 10 10 10 ([3]->[4:4])

              #1 :  1  3  6 10 10  5 11 18
              #2 :  1  3  6 10 10 10  5 11
              #3 :  1  3  6 10 10 10 10  5
              #4 :  1  3  6 10 10 10 10 10

             perm1 = CELLSHR add1, [0, 0, 0, 0, 1, 2, 3, 4]
             displacement = INDEX;
             displacement & 7 < 4:
                displacement = 0;
             displacement & 7 >= 4:
                displacement -= 3;
            */


            /* VERY IMPORTANT: We now generate a REG_CT_DISPLACEMENT that we use for CELL_SHL that should have value:
               - for iteration i = 1:
                 R(REG_CT_DISPLACEMENT) = [0 0 1 2 0 0 5 6 0 0 9 a 0 0 d e ...]
                  which is NOT [0, 0, 1, 2, 0, 0, 1, 2],
                  BUT the effect it has with CELL_SHR left to execute for 2 NOP
                  cycles is EXACTLY the same.

               - for iteration i = 2:
                 R(REG_CT_DISPLACEMENT) = [0 0 0 0 1 2 3 4 0 0 0 0 9 a b c ...]
                 which is NOT [0, 0, 0, 0, 1, 2, 3, 4],
                   BUT the effect it has with CELL_SHR left to execute for 2 NOP
                   cycles is EXACTLY the same.

             small-TODO: we could use R(REG_CT_DISPLACEMENT) = ( R(REG_CT_DISPLACEMENT) - R(REG_CT1) ) & R(REG_POW2) to compute exactly the desired vector as for the x86 white-paper, BUT it's NOT necessary.
            */
            //R(REG_CT_AUX) = (i == 3) ? i : i - 1;
            //R(REG_CT_AUX) = 2 * (i - 1) - 1; // 2 * i - 3
            //R(REG_CT_AUX) = (1UL << (i - 1)) - 1;
            R(REG_CT_AUX) = (1UL << i) - 1;
            //printf("i = %d --> (1UL << (i - 1)) - 1 = %ld\n",
            //        i, (1UL << (i - 1)) - 1);
            printf("i = %d --> (1UL << i) - 1 = %ld\n",
                    i, (1UL << i) - 1);
            /*
             Values of REG_CT_AUX are:
                i = 1 --> not meaningful
                i = 2 --> REG_CT_AUX = 2^2 - 1
                i = 3 --> REG_CT_AUX = 2^3 - 1
                i = 4 --> REG_CT_AUX = 2^4 - 1
                i = 5 --> REG_CT_AUX = 2^5 - 1
             */

// This code is very similar to the one above: // MEGA-TODO: maybe we should put them together, BUT like this the num of instructions is basically minimal
            R(REG_AUX) = R(REG_POW2);
            // NOT defined in Opincaa lib: R(REG_POW2) <<= 1;
            R(REG_POW2) <<= 1;
            R(REG_POW2_MINUS_1) = R(REG_POW2) - R(REG_CT1);
            //R(REG_POW2_MINUS_1) = R(REG_POW2_MINUS_1) << 1 + R(REG_CT1);
            //PrintRegDebug(REG_CT1);
          PrintRegDebug(REG_POW2);
          PrintRegDebug(REG_POW2_MINUS_1);

            R(REG_CT_DISPLACEMENT) = INDEX;
            R(REG_AUX2) = R(REG_CT_DISPLACEMENT) & R(REG_POW2_MINUS_1);
          PrintRegDebug(REG_AUX2);
            R(REG_AUX2) = R(REG_AUX2) < R(REG_AUX);
            NOP;
        );
          EXECUTE_WHERE_LT(
            R(REG_CT_DISPLACEMENT) = 0;
          );
        EXECUTE_IN_ALL(
        PrintDebugMessage("  Before final adjust (partial): ");
        PrintRegDebug(REG_CT_DISPLACEMENT);
        PrintRegDebug(REG_AUX2);
            R(REG_AUX2) = R(REG_AUX2) == R(REG_CT0);
            NOP;
        );
          EXECUTE_WHERE_EQ(
            if (i == 1) {
                R(REG_CT_DISPLACEMENT) -= R(REG_CT1);
            }
            else {
                /*
                R(REG_AUX) -= R(REG_CT1);
                R(REG_CT_DISPLACEMENT) -= R(REG_AUX);
                */

          //PrintRegDebug(REG_CT_AUX);
          //PrintRegDebug(REG_AUX);
                R(REG_CT_DISPLACEMENT) -= R(REG_CT_AUX);
            }
          );
        EXECUTE_IN_ALL(
        PrintDebugMessage("  Before final adjust: ");
          PrintRegDebug(REG_CT_DISPLACEMENT);

            CELL_SHR( R(REG_SRC), R(REG_CT_DISPLACEMENT) );
            //
            // NOP for iNumNOPs times
            for (indexNOP = 0; indexNOP < iNumNOPs; indexNOP++)
                NOP;
            //
            R(REG_SRC01_SHR) = SHIFT_REG;
          PrintRegDebug(REG_SRC01);

            R(REG_SRC01) = 0;
            // We remove unwanted from REG_SRC01
            R(REG_AUX) = R(REG_CT0) < R(REG_CT_DISPLACEMENT);
            NOP;
          );
            EXECUTE_WHERE_LT(
              R(REG_SRC01) = R(REG_SRC01_SHR);
            );

        EXECUTE_IN_ALL(
            R(REG_SRC) += R(REG_SRC01);

            char strAux[512];
            sprintf(strAux, "After 'iteration' %d:\n", i + 1);
            PrintDebugMessage(strAux);
            PrintRegDebug(REG_SRC);
    } // end for i



            LS[2] = R(REG_SRC);

            REDUCE R0;
         );
    END_KERNEL("SumScan.i16");
}


int ScanTest(ConnexMachine *connex) {
    int i;
    uint16_t result[CONNEX_VECTOR_LENGTH];

    ScanKernel();

    /*
    connex->writeDataToConnex(opA, 1, 1);
    */

    connex->executeKernel("SumScan.i16");
    connex->readReduction();

    connex->readDataFromConnex(result, 1, 2);

    for (i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        printf("result[%d] = %d\n", i, result[i]);

        assert(result[i] == ((i + 1) * (i + 2)) / 2); // This is for the vector 1..CVL
        //assert(result[i] == (i * (i + 1)) / 2); // This is for the vector 0..(CVL-1)
    }
}

void Test() {
    ScanTest(connexGlobal);
}

