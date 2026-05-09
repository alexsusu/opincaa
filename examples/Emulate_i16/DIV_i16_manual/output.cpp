/*
This is an implementation of the integer DIVision for i16 operands.
It is using the standard (Zedboard implementation) Connex ISA.
  It is well optimized - first done in C then in Opincaa.

IMO my implementation is a Nonperforming divide (NPD) algo
 - see \cite{Ercegovac_Digital_Arithmetic_2004},
   https://books.google.ro/books?id=p79cu3nZ6yoC&pg=PA37&lpg=PA37&dq=Ercegovac+Lang+Division+NPD&source=bl&ots=YsM44sDMHo&sig=gPjn1nWrjdhd6NhBHSgie-EOEeA&hl=en&sa=X&ved=0ahUKEwie8Pzdlv3VAhUBEpoKHaE9AewQ6AEILDAB#v=onepage&q=Ercegovac%20Lang%20Division%20NPD&f=false,
     page 38 .
  because:
    - it does NOT do restoration like the Restoring Divide algo
    - it does NOT do correction like the Nonrestoring Division algo.
*/


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


//typedef long TYPE;
//typedef int TYPE;

typedef short TYPE;
#define TYPE_MAX 32767
#define TYPE_MIN -32768


//#define LLVM_ISEL_CODEGEN
#ifdef LLVM_ISEL_CODEGEN
  #define PRINTREG(regNum) regNum
  #define PrintDebugMessage(aStr) aStr
  #define PrintRegDebug(regNum) regNum
#endif



//#define ADAPTIVE_RUN
#ifdef ADAPTIVE_RUN
  // We use BITREVERSE (experimental) with POPCNT
  #define GetIndexHighestBitSet

  // We use REPEAT_SETLC_REDUCE (experimental)

  // DO NOT uncomment this: //#define EMULATE_MAX_REDUCE_UNFINISHED
#endif



// Uncommenting this puts in C the elements of abs(A)
//#define WRITE_ABS_VALUE_IN_MEM_FOR_TESTING




// VERY IMPORTANT: CELL_SHR(Rsrc, vector_splat_1) moves value at index i to index i+1



TYPE DivInt16(TYPE *A, TYPE *B, TYPE *C, TYPE *D, TYPE N) {
    connexGlobal->writeDataToConnexPartial((&A [(0 +  0)]), /* actual num elems written */ (N), /*offset*/ 0);
    connexGlobal->writeDataToConnexPartial((&B [(0 +  0)]), /* actual num elems written */ (N), /*offset*/ 1 * (int)ceil(((float)(N))/CONNEX_VECTOR_LENGTH));

    _BEGIN_KERNEL(BatchNumberGlobal);
      EXECUTE_IN_ALL(


  #define REG_NUM_ITERS  10

  #define REG_CT0  31
  #define REG_CT1  30
  #define REG_CT2  29
  #define REG_CT16  9
  #define REG_TYPE_MIN  28
  #define REG_TYPE_MAX  27
  #define REG_QUOTIENT  26
  #define REG_QUOTIENT_TOTAL  25
  #define REG_REMAINDER  24
  //#define REG_CORRECT_REMAINDER  14
  #define REG_CORRECT_QUOTIENT  23

  // Keep REG_SRC1 < REG_SRC2
  #define REG_SRC1 21
  #define REG_SRC2 22

  #define REG_DIVISOR_ALIGNED_LOW16 20
  #define REG_DIVISOR_ALIGNED_HIGH16 19
  #define REG_RESIDUAL 18
  #define REG_ITER 17
  #define REG_ITER16 16
  #define REG_CHANGE_SIGN 15

  #define REG_AUX 14
  #define REG_AUX2 13
  #define REG_AUX3 12
  #define REG_MASK_CONTINUE 11
  //#define REG_SRCAUX 27
  /*
  #define REG_IDX     15
  #define REG_IDXMOD2 14
  #define REG_IDXPRED 13
  */
  //#define REG_CRY 15






        R(REG_CT0) = 0;
        R(REG_CT1) = 1;

        R(6) = 0;
        R(1) = (N / CONNEX_VL) + ((N & (CONNEX_VL - 1)) > 0);
        R(2) = ((N / CONNEX_VL) + ((N & (CONNEX_VL - 1)) > 0)) * 2;
        R(5) = ((N / CONNEX_VL) + ((N & (CONNEX_VL - 1)) > 0)) * 3;

        R(4) = 0 ;
        //
        R(3) = 1 ;
        //R(5) = 1 ;


    // IMPORTANT: it seems the REPEAT loop contains more than 1024 instructions in the queue and it gives strange string exception error, while the cause is buffer overflow: REPEAT_X_TIMES(((N / CONNEX_VL) + ((N & (CONNEX_VL - 1)) > 0) )   );

    for (int idxLoop = 0;
            idxLoop < (N / CONNEX_VL) + ((N & (CONNEX_VL - 1)) > 0);
            idxLoop++) {

        //R(REG_AUX) = 0;
        /*R(REG_REMAINDER) = 0;
        R(REG_RESIDUAL) = 0;
        R(REG_QUOTIENT) = 0;
        R(REG_QUOTIENT_TOTAL) = 0;
        R(REG_MASK_CONTINUE) = 1;
        */


        R(REG_SRC1) = LS[R(6)];
        R(REG_SRC2) = LS[R(1)];

        PRINTREG(0);
        PRINTREG(1);


        R(1) = R(1) + R(3);
        R(6) = R(6) + R(3);



        PrintDebugMessage("REG_SRC1:\n");
        PRINTREG(REG_SRC1);
        PrintDebugMessage("REG_SRC2:\n");
        PRINTREG(REG_SRC2);




    // Alex: added manually

    /* We divide 2 16-bit integers.
    // R(REG_SRC1) is dividend, R(REG_SRC2) is divisor
    */





        R(REG_CT0) = 0;
        R(REG_CT1) = 1;
        R(REG_CT2) = 2;
        R(REG_CT16) = 16;
        R(REG_TYPE_MIN) = TYPE_MIN;

        // IMPORTANT: Although NOT necessary this instruction, it helps my Opincaa automatic CodeGenerator
        R(REG_REMAINDER) = 0;

        R(REG_QUOTIENT_TOTAL) = 0;
        R(REG_CHANGE_SIGN) = 0;

        /* NOTE: nested WHERE are NOT reliable
        IMPORTANT: Need to initialize it to have the "nested" WHEREs work
        //R(REG_AUX2) = 0;
        */

  // Handling specific case divisor == TYPE_MIN
  // TODO: MAYBE handle this specific case at the end
        R(REG_MASK_CONTINUE) = R(REG_SRC2) == R(REG_TYPE_MIN);
PrintRegDebug(REG_MASK_CONTINUE);
    NOP;
  );
    EXECUTE_WHERE_EQ(
        R(REG_REMAINDER) = R(REG_SRC1) | R(REG_SRC1); // COPY implemented with OR
        //R(REG_QUOTIENT_TOTAL) = 0;
    );
    EXECUTE_IN_ALL(
        R(REG_AUX) = R(REG_SRC1) == R(REG_TYPE_MIN);
        R(REG_AUX) = R(REG_AUX) & R(REG_MASK_CONTINUE);
        R(REG_AUX) = R(REG_AUX) == R(REG_CT1);
        NOP;
      );
      EXECUTE_WHERE_EQ(
          R(REG_QUOTIENT_TOTAL) = 1;
          R(REG_REMAINDER) = 0;
      );

  /* IMPORTANT NOTE: Division by 0 returns invariably a quotient = 0x7FFF,
     remainder = dividend */



  EXECUTE_IN_ALL(
    PrintRegDebug(REG_QUOTIENT_TOTAL);
    PrintRegDebug(REG_REMAINDER);

        /* We make (in the end) REG_MASK_CONTINUE = 0 for the lanes that have
        divisor == TYPE_MIN, in order to prevent those lanes to do any further
        processing later on. */
        R(REG_MASK_CONTINUE) = R(REG_CT1) - R(REG_MASK_CONTINUE);
    PrintRegDebug(REG_MASK_CONTINUE);






#define TREAT_DIVIDEND_EQUAL_TYPEMIN
#ifdef TREAT_DIVIDEND_EQUAL_TYPEMIN
    /*
    TYPE correctQuotient = 0; // this means no special treatment since dividend != TYPE_MIN
    if (dividend == TYPE_MIN) {
      #ifdef MYDEBUG_ALEX
        printf("  Division(): We increment dividend by 1 to avoid overflow when we negate it and correct this later\n");
      #endif

        dividend = TYPE_MIN + 1;
        correctQuotient = (divisor < 0) ? 1 : -1;
    }
    */
        //R(REG_CORRECT_REMAINDER) = 0;
        R(REG_CORRECT_QUOTIENT) = 0;
        R(REG_AUX) = R(REG_SRC1) == R(REG_TYPE_MIN);
        // IMPORTANT: executing only if REG_MASK_CONTINUE == 1
        R(REG_AUX) = R(REG_AUX) & R(REG_MASK_CONTINUE);
        R(REG_AUX) = R(REG_AUX) == R(REG_CT1);
        NOP;
    );
    EXECUTE_WHERE_EQ(
        R(REG_SRC1) += R(REG_CT1);

        PrintDebugMessage("REG_SRC1:\n");
        PRINTREG(REG_SRC1);

        //R(REG_CORRECT_REMAINDER) = R(REG_CT0) - R(REG_CT1);

        R(REG_CORRECT_QUOTIENT) = R(REG_SRC2) >> 15;

        PrintDebugMessage("Temp REG_CORRECT_QUOTIENT:\n");
        PRINTREG(REG_CORRECT_QUOTIENT);

        R(REG_CORRECT_QUOTIENT) = R(REG_CORRECT_QUOTIENT) << 1;

        PrintDebugMessage("Temp2 REG_CORRECT_QUOTIENT:\n");
        PRINTREG(REG_CORRECT_QUOTIENT);

        R(REG_CORRECT_QUOTIENT) = R(REG_CORRECT_QUOTIENT) - R(REG_CT1);

        PrintDebugMessage("IMPORTANT: REG_CORRECT_QUOTIENT:\n");
        PRINTREG(REG_CORRECT_QUOTIENT);

        /* We do NOT make REG_MASK_CONTINUE = 0 since we want to do division
          with the adjusted dividend */
    );

  EXECUTE_IN_ALL(
#endif


    /*
    if (dividend < 0) {
        dividend = -dividend; // NOTE: dividend != TYPE_MIN (else treated above)
        changeSign = 1;
    }
    if (divisor < 0) {
        divisor = -divisor;
        //changeSign = 1 - changeSign;
        changeSign = changeSign | 2;
    }
    */
        R(REG_AUX) = R(REG_SRC1) < R(REG_CT0);
        NOP;
);
        EXECUTE_WHERE_LT(
            R(REG_SRC1) = R(REG_CT0) - R(REG_SRC1);
            R(REG_CHANGE_SIGN) = R(REG_CT1) | R(REG_CT1); // COPY implemented with OR
        );

  EXECUTE_IN_ALL(
        R(REG_AUX) = R(REG_SRC2) < R(REG_CT0);
        NOP;
);
        EXECUTE_WHERE_LT(
            R(REG_SRC2) = R(REG_CT0) - R(REG_SRC2);
            R(REG_CHANGE_SIGN) = R(REG_CHANGE_SIGN) | R(REG_CT2);
        );

  EXECUTE_IN_ALL(
  //PRINTCHARS(32767);
  //PrintDebugMessage((char *)"Test");
  PrintDebugMessage("REG_QUOTIENT_TOTAL:\n");
  PRINTREG(REG_QUOTIENT_TOTAL);
  PrintDebugMessage("REG_REMAINDER:\n");
  PRINTREG(REG_REMAINDER);

/*
    // TODO: avoid executing more if the result is already computed - it saves
    // energy
    R(REG_AUX) = R(REG_SRC2) == R(REG_TYPE_MIN);
    R(REG_AUX) = R(REG_AUX) == R(REG_CT0);
    NOP;
  );
 // TODO: EXECUTE_WHERE_EQ(

  EXECUTE_IN_ALL(
*/





// This is for the adaptive case (ADAPTIVE_RUN)
#ifdef GetIndexHighestBitSet
    /* VERY IMPORTANT NOTE: we don't use the standard implementation,
     *    without using the BITREV instruction and with a loop,
     *    because it takes 100 more cycles, which makes
     *    this adaptive case USELESS, since we get no more performance gain.
     *
     * NOTE: instead of lookup table which can have a large table e.g.
         64K entries,
         we use a "direct" formula: POPCNT(x ^ (x - 1)) - 1, where x is
         BitReverse(input_reg) */

    PRINTREG(REG_SRC1);
    R(REG_NUM_ITERS) = BITREVERSE(R(REG_SRC1));
    PrintDebugMessage("Bit-reverse for REG_SRC1:\n");
    PRINTREG(REG_NUM_ITERS);
    R(REG_AUX) = R(REG_NUM_ITERS) - R(REG_CT1);
    R(REG_NUM_ITERS) = R(REG_NUM_ITERS) ^ R(REG_AUX);
    R(REG_NUM_ITERS) = POPCNT(R(REG_NUM_ITERS));
    //PRINTREG(REG_NUM_ITERS);
    //R(REG_NUM_ITERS) = R(REG_NUM_ITERS) - R(REG_CT1);
    R(REG_AUX) = 16;
    R(REG_NUM_ITERS) = R(REG_AUX) - R(REG_NUM_ITERS);
    // We make correction: if REG_SRC1 is 0 --> NUM_ITERS is -1 and not 15
    R(REG_AUX) = R(REG_SRC1) == R(REG_CT0);
    NOP;
    );
    EXECUTE_WHERE_EQ(
      R(REG_NUM_ITERS) = -1;
    );
    EXECUTE_IN_ALL(
      PRINTREG(REG_NUM_ITERS);


/*
REG_SRC1:
R[24] (left is index 0) = 6125 0004 000a 0008 0009 0009 0000 0001 233b ac95 a748 7108 b793 9a69 41ee c81a 183d 7344 b88e fb86 67fe 977c 70df d327 4f86 d776 27d0 883d 547b eda1 b222 f4d5 deb5 a84d c578 b61
5 7247 34c9 82b3 f742 07a8 4907 9735 8eda 73e3 0219 b233 7062 df03 a829 3c80 51ef 2bc8 11f0 0ea4 00b5 57e6 c5c1 9afe 5ac7 5111 b4e6 d3cc 142a bdf8 d746 9a8c 293d 2fe9 f4ce c45d d4d7 bf3c 8b5d 6f31 e919 c3
03 4387 4343 c99a 9402 7baf 4a12 74de 601b 3889 df2c c0ba 27f6 78a0 b237 d76c 7fb4 8c07 f2a1 4432 767c 352b e4d0 ba89 93b7 e960 8070 9149 762b bf8d eba0 1d9a 31e7 ae4b 7f3b 2b29 325d 3644 8a5f dc16 63d9 1
0f2 ef8a 0cad 2c23 0e14 e32f 0315 554a b99e 23da 4a3d [END]

R[13] (left is index 0) = 000e 0002 0003 0003 0003 0003 ffff 0000 000d 000e 000e 000e 000e 000e 000e 000d 000c 000e 000e 000a 000e 000e 000e 000d 000e 000d 000d 000e 000e 000c 000e 000b 000d 000e 000d 000
e 000e 000d 000e 000b 000a 000e 000e 000e 000e 0009 000e 000e 000d 000e 000d 000e 000d 000c 000b 0007 000e 000d 000e 000e 000e 000e 000d 000c 000e 000d 000e 000d 000d 000b 000d 000d 000e 000e 000e 000c 00
0d 000e 000e 000d 000e 000e 000e 000e 000e 000d 000d 000d 000d 000e 000e 000d 000e 000e 000b 000e 000e 000d 000c 000e 000e 000c 000e 000e 000e 000e 000c 000c 000d 000e 000e 000d 000d 000d 000e 000d 000e 0
00c 000c 000b 000d 000b 000c 0009 000e 000e 000d 000e [END]

For SRC1, final: REG_NUM_ITERS:
R[13] (left is index 0) = 000e 0002 0003 0003 0003 0003 ff9c 0000 000d 000e 000e 000e 000e 000e 000e 000d 000c 000e 000e 000a 000e 000e 000e 000d 000e 000d 000d 000e 000e 000c 000e 000b 000d 000e 000d 000
e 000e 000d 000e 000b 000a 000e 000e 000e 000e 0009 000e 000e 000d 000e 000d 000e 000d 000c 000b 0007 000e 000d 000e 000e 000e 000e 000d 000c 000e 000d 000e 000d 000d 000b 000d 000d 000e 000e 000e 000c 00
0d 000e 000e 000d 000e 000e 000e 000e 000e 000d 000d 000d 000d 000e 000e 000d 000e 000e 000b 000e 000e 000d 000c 000e 000e 000c 000e 000e 000e 000e 000c 000c 000d 000e 000e 000d 000d 000d 000e 000d 000e 0
00c 000c 000b 000d 000b 000c 0009 000e 000e 000d 000e [END]

So, the number iterations for the pairs of inputs is:
  R[10] (left is index 0) = 0004 0003 0002 0004 0003 0005 0001 0001 0002 0001 0001 0002 0001 0004 0001 0000 0000 0001 0001 0000 0004 0002 0002 0001 0004 0000 0002 0005 0001 0003 0002 0000 0000 0001 0000 000
  1 0002 0000 0002 0001 0000 0002 0006 0001 0001 0000 0001 0002 0001 0002 0000 0001 0001 0000 0000 0000 0001 0001 0001 0001 0001 0002 0000 0000 0002 0001 0001 0000 0003 0007 0000 0002 0002 0003 0001 0000 00
  04 0006 0001 0000 0001 0001 0004 0002 0002 0000 0000 0003 0000 0001 0001 0000 0001 0001 0000 0002 0001 0000 0000 0004 0001 0002 0001 0001 0001 0001 0000 0000 0002 0006 0002 0000 0000 0000 0002 0001 0001 0
  000 0000 0000 0001 0000 0000 0000 0001 0001 0001 0002 [END]
*/

      R(REG_RESIDUAL) = R(REG_NUM_ITERS);

      R(REG_NUM_ITERS) = BITREVERSE(R(REG_SRC2));
      PrintDebugMessage("Bit-reverse for REG_SRC2:\n");
      PRINTREG(REG_NUM_ITERS);
      R(REG_AUX) = R(REG_NUM_ITERS) - R(REG_CT1);
      R(REG_NUM_ITERS) = R(REG_NUM_ITERS) ^ R(REG_AUX);
      R(REG_NUM_ITERS) = POPCNT(R(REG_NUM_ITERS));
      //PRINTREG(REG_NUM_ITERS);
      //R(REG_NUM_ITERS) = R(REG_NUM_ITERS) - R(REG_CT1);
      R(REG_AUX) = 16;
      R(REG_NUM_ITERS) = R(REG_AUX) - R(REG_NUM_ITERS);
      R(REG_AUX) = R(REG_SRC2) == R(REG_CT0);
      NOP;
      );
      EXECUTE_WHERE_EQ(
        R(REG_NUM_ITERS) = -1;
      );
      EXECUTE_IN_ALL(
        PRINTREG(REG_NUM_ITERS);

        R(REG_NUM_ITERS) = R(REG_RESIDUAL) - R(REG_NUM_ITERS);
        // We make correction: if REG_SRC1 is 0 --> NUM_ITERS is -1 and not 15
        R(REG_AUX) = R(REG_NUM_ITERS) < R(REG_CT0);
        NOP;
      );
      EXECUTE_WHERE_LT(
          R(REG_NUM_ITERS) = -1;
      );
      EXECUTE_IN_ALL(
        R(REG_NUM_ITERS) += R(REG_CT1);

/*
R[13] (left is index 0) = 000b 0000 0002 0000 0001 ffff ffff 0000 000c 000e 000e 000d 000e 000b 000e 000e 000d 000e 000e 000e 000b 000d 000d 000d 000b 000e 000c 000a 000e 000a 000d 000e 000e 000e 000e 000
e 000d 000e 000d 000b 000d 000d 0009 000e 000e 000b 000e 000d 000d 000d 000e 000e 000d 000e 000e 000e 000e 000d 000e 000e 000e 000d 000e 000d 000d 000d 000e 000e 000b 0005 000e 000c 000d 000c 000e 000e 00
0a 0009 000e 000e 000e 000e 000b 000d 000d 000e 000e 000b 000e 000e 000e 000e 000e 000e 000d 000d 000e 000e 000d 000b 000e 000b 000e 000e 000e 000e 000d 000e 000c 0009 000d 000e 000e 000e 000d 000d 000e 0
00e 000e 000e 000d 000e 000e 000e 000e 000e 000d 000d [END]

HYPERIMPORTANT: Number iterations to run:
R[13] (left is index 0) = 0004 0003 0002 0004 0003 0005 0001 0001 0002 0001 0001 0002 0001 0004 0001 0000 0000 0001 0001 0000 0004 0002 0002 0001 0004 0000 0002 0005 0001 0003 0002 0000 0000 0001 0000 000
1 0002 0000 0002 0001 0000 0002 0006 0001 0001 0000 0001 0002 0001 0002 0000 0001 0001 0000 0000 0000 0001 0001 0001 0001 0001 0002 0000 0000 0002 0001 0001 0000 0003 0007 0000 0002 0002 0003 0001 0000 00
04 0006 0001 0000 0001 0001 0004 0002 0002 0000 0000 0003 0000 0001 0001 0000 0001 0001 0000 0002 0001 0000 0000 0004 0001 0002 0001 0001 0001 0001 0000 0000 0002 0006 0002 0000 0000 0000 0002 0001 0001 0
000 0000 0000 0001 0000 0000 0000 0001 0001 0001 0002 [END]

OLD GOOD:
HYPERIMPORTANT: Number iterations to run:
R[13] (left is index 0) = 0004 0003 0002 0004 0003 0000 0000 0000 0002 0001 0001 0002 0001 0004 0001 0000 0000 0001 0001 0000 0004 0002 0002 0001 0004 0000 0002 0005 0001 0003 0002 0000 0000 0001 0000 000
1 0002 0000 0002 0001 0000 0002 0006 0001 0001 0000 0001 0002 0001 0002 0000 0001 0001 0000 0000 0000 0001 0001 0001 0001 0001 0002 0000 0000 0002 0001 0001 0000 0003 0007 0000 0002 0002 0003 0001 0000 00
04 0006 0001 0000 0001 0001 0004 0002 0002 0000 0000 0003 0000 0001 0001 0000 0001 0001 0000 0002 0001 0000 0000 0004 0001 0002 0001 0001 0001 0001 0000 0000 0002 0006 0002 0000 0000 0000 0002 0001 0001 0
000 0000 0000 0001 0000 0000 0000 0001 0001 0001 0002 [END]
*/
      PrintDebugMessage("HYPERIMPORTANT: Number iterations to run:\n");
      PRINTREG(REG_NUM_ITERS);
      PRINTREG(REG_SRC1);
      PRINTREG(REG_SRC2);
#endif // END GetIndexHighestBitSet


// This is for the adaptive case, also (ADAPTIVE_RUN)
#ifdef GetIndexHighestBitSet_COMPLEX_NOT_REQUIRED_ANYMORE
    /*
    // NOTE: here we use a lookup table
    // IMPORTANT: all this calculation of number of iterations takes 89 - 21(for LS mem setup) = 68 Connex cycles

    // We initialize the simple (and small since we put it in in LS mem) lookup-table
    R(REG_AUX) = -100;
    LS[0] = R(REG_AUX);
    R(REG_AUX) = 0;
    LS[1] = R(REG_AUX);
    R(REG_AUX) = 1;
    LS[2] = R(REG_AUX);
    LS[3] = R(REG_AUX);
    R(REG_AUX) = 2;
    LS[4] = R(REG_AUX);
    LS[5] = R(REG_AUX);
    LS[6] = R(REG_AUX);
    LS[7] = R(REG_AUX);
    R(REG_AUX) = 3;
    LS[8] = R(REG_AUX);
    LS[9] = R(REG_AUX);
    LS[10] = R(REG_AUX);
    LS[11] = R(REG_AUX);
    LS[12] = R(REG_AUX);
    LS[13] = R(REG_AUX);
    LS[14] = R(REG_AUX);
    LS[15] = R(REG_AUX);

    // We compute GetIndexHighestBitSet() for REG_SRC1:
    R(REG_NUM_ITERS) = R(REG_SRC1) >> 12;
    PrintDebugMessage("(Computing for REG_SRC1): HYPERIMPORTANT: REG_NUM_ITERS:\n");
    PRINTREG(REG_NUM_ITERS);
    //R(REG_NUM_ITERS) = POPCNT(R(REG_NUM_ITERS));
    R(REG_NUM_ITERS) = LS[R(REG_NUM_ITERS)];
    PRINTREG(REG_NUM_ITERS);

    R(REG_AUX) = 12;
    R(REG_AUX2) = 0x0F;
    R(REG_NUM_ITERS) += R(REG_AUX);
    R(REG_AUX) = R(REG_NUM_ITERS) < R(REG_CT0);
    NOP;
    );
    EXECUTE_WHERE_LT(
        R(REG_NUM_ITERS) = R(REG_SRC1) >> 8;
        R(REG_NUM_ITERS) = R(REG_NUM_ITERS) & R(REG_AUX2);
        R(REG_NUM_ITERS) = LS[R(REG_NUM_ITERS)];
        R(REG_AUX) = 8;
        R(REG_NUM_ITERS) += R(REG_AUX);
    );
    EXECUTE_IN_ALL(
      R(REG_AUX) = R(REG_NUM_ITERS) < R(REG_CT0);
      NOP;
    );
    EXECUTE_WHERE_LT(
        R(REG_NUM_ITERS) = R(REG_SRC1) >> 4;
        R(REG_NUM_ITERS) = R(REG_NUM_ITERS) & R(REG_AUX2);
        R(REG_NUM_ITERS) = LS[R(REG_NUM_ITERS)];
        R(REG_AUX) = 4;
        R(REG_NUM_ITERS) += R(REG_AUX);
    );
    EXECUTE_IN_ALL(
      R(REG_AUX) = R(REG_NUM_ITERS) < R(REG_CT0);
      NOP;
    );
    EXECUTE_WHERE_LT(
        R(REG_NUM_ITERS) = R(REG_SRC1) & R(REG_AUX2);
    PrintDebugMessage("SRC1: REG_NUM_ITERS:\n");
    PRINTREG(REG_NUM_ITERS);
        R(REG_NUM_ITERS) = LS[R(REG_NUM_ITERS)];
    PrintDebugMessage("SRC1: REG_NUM_ITERS:\n");
    PRINTREG(REG_NUM_ITERS);
    );
    EXECUTE_IN_ALL(
    PrintDebugMessage("For SRC1, final: REG_NUM_ITERS:\n");
    PRINTREG(REG_NUM_ITERS);



    R(REG_RESIDUAL) = R(REG_NUM_ITERS);

    // IMPORTANT: This code is identical (except REG_SRC2) with the one above
    // We compute GetIndexHighestBitSet() for REG_SRC2:
    R(REG_NUM_ITERS) = R(REG_SRC2) >> 12;
    PrintDebugMessage("(Computing for REG_SRC2): HYPERIMPORTANT: REG_NUM_ITERS:\n");
    PRINTREG(REG_NUM_ITERS);
    //R(REG_NUM_ITERS) = POPCNT(R(REG_NUM_ITERS));
    R(REG_NUM_ITERS) = LS[R(REG_NUM_ITERS)];
    PRINTREG(REG_NUM_ITERS);

    R(REG_AUX) = 12;
    //R(REG_AUX2) = 0xFF;
    R(REG_NUM_ITERS) += R(REG_AUX);
    R(REG_AUX) = R(REG_NUM_ITERS) < R(REG_CT0);
    NOP;
    );
    EXECUTE_WHERE_LT(
        R(REG_NUM_ITERS) = R(REG_SRC2) >> 8;
    PrintDebugMessage("!!!!REG_NUM_ITERS:\n");
    PRINTREG(REG_NUM_ITERS);
        R(REG_NUM_ITERS) = R(REG_NUM_ITERS) & R(REG_AUX2);
    PrintDebugMessage("!!!!2: REG_NUM_ITERS:\n");
    PRINTREG(REG_NUM_ITERS);
        R(REG_NUM_ITERS) = LS[R(REG_NUM_ITERS)];
    PrintDebugMessage("!!!!3: REG_NUM_ITERS:\n");
    PRINTREG(REG_NUM_ITERS);
        R(REG_AUX) = 8;
        R(REG_NUM_ITERS) += R(REG_AUX);
    );
    EXECUTE_IN_ALL(
      R(REG_AUX) = R(REG_NUM_ITERS) < R(REG_CT0);
      NOP;
    );
    EXECUTE_WHERE_LT(
        R(REG_NUM_ITERS) = R(REG_SRC2) >> 4;
        R(REG_NUM_ITERS) = R(REG_NUM_ITERS) & R(REG_AUX2);
        R(REG_NUM_ITERS) = LS[R(REG_NUM_ITERS)];
        R(REG_AUX) = 4;
        R(REG_NUM_ITERS) += R(REG_AUX);
    );
    EXECUTE_IN_ALL(
      R(REG_AUX) = R(REG_NUM_ITERS) < R(REG_CT0);
      NOP;
    );
    EXECUTE_WHERE_LT(
        R(REG_NUM_ITERS) = R(REG_SRC2) & R(REG_AUX2);
        R(REG_NUM_ITERS) = LS[R(REG_NUM_ITERS)];
    );
    EXECUTE_IN_ALL(
      PrintDebugMessage("HYPERIMPORTANT: REG_NUM_ITERS:\n");
      PRINTREG(REG_NUM_ITERS);

      PRINTREG(REG_RESIDUAL);
      R(REG_RESIDUAL) -= R(REG_NUM_ITERS);

      R(REG_AUX2) = 0x0FFF;
      // We make negative numbers positive
      R(REG_NUM_ITERS) = R(REG_RESIDUAL) & R(REG_AUX2);
      R(REG_NUM_ITERS) += R(REG_CT1);
      R(REG_AUX2) = 15;
      R(REG_AUX) = R(REG_AUX2) < R(REG_NUM_ITERS);
      NOP;
    );
    EXECUTE_WHERE_LT(
        R(REG_NUM_ITERS) = 0;
    );
    EXECUTE_IN_ALL(
    */
      PrintDebugMessage("HYPERIMPORTANT: Number iterations to run:\n");
      PRINTREG(REG_NUM_ITERS);
      PRINTREG(REG_SRC1);
      PRINTREG(REG_SRC2);
#endif // END GetIndexHighestBitSet_COMPLEX_NOT_REQUIRED_ANYMORE



#ifdef EMULATE_MAX_REDUCE_UNFINISHED_DO_NOT_USE_SINCE_REQUIRES_INSTR_SETLC_MAXRED
      PrintDebugMessage("Before MAX-reduce:\n");
      PRINTREG(REG_NUM_ITERS);

      // We compute the max-REDUCE of REG_NUM_ITERS:
      //R(AUX) = R(REG_NUM_ITERS);
      R(REG_AUX2) = CONNEX_VECTOR_LENGTH / 2;
      CELL_SHL( R(REG_NUM_ITERS), R(REG_AUX2) );
      NOP; // TODO: NOP for CONNEX_VECTOR_LENGTH / 2 times
      R(REG_AUX) = SHIFT_REG;

      PRINTREG(REG_AUX);

      // For lower halves of vectors REG_AUX and REG_NUM_ITERS we choose the max
      R(REG_AUX3) = INDEX;
      PRINTREG(REG_AUX3);
      PRINTREG(REG_AUX2);
      R(REG_AUX2) = R(REG_AUX3) < R(REG_AUX2);
      PRINTREG(REG_AUX2);
      NOP;
    );
    EXECUTE_WHERE_LT(
        // We could use instead of & below: R(REG_AUX) = -32768;
        R(REG_AUX3) = R(REG_NUM_ITERS) < R(REG_AUX); // not conditioned by active flags
      PRINTREG(REG_AUX3);
// TODO TODO TODO: if necessary, do better when it is clear what instructions are predicated - see ACTIVE flag in ConnexISA.docx
        R(REG_AUX3) = R(REG_AUX2) & R(REG_AUX3); // (not conditioned also)
        R(REG_AUX3) = R(REG_AUX3) == R(REG_CT1); // (not conditioned also)
        NOP;
      );
      EXECUTE_WHERE_EQ(
          R(REG_NUM_ITERS) = R(REG_AUX) | R(REG_AUX); // COPY implemented with OR
      );
    EXECUTE_IN_ALL(
      PrintDebugMessage("After a 1st step MAX-reduce:\n");
      PRINTREG(REG_NUM_ITERS);
assert(0 && "Finish the other log2_CVL - 1 steps of MAX_REDUCE"
#endif


//int NUM_ITERS = 3;
int NUM_ITERS = 14;
//int NUM_ITERS = 7;
//int NUM_ITERS = 3;

    // NOT GOOD: TYPE divisori32 = divisor << NUM_ITERS;
    //TYPE quotient = 1 << NUM_ITERS;
#ifdef ADAPTIVE_RUN
   #ifdef OLD_MANUAL
    R(REG_ITER) = 7;
    R(REG_QUOTIENT) = R(REG_CT1) << R(REG_ITER);
   #endif

    /*
    // Another possiblility would be to give on CPU - but this would introduce probably some bigger latencies
    END_KERNEL;

    int resMaxRed = readMaxReductionResult();
    BEGIN_KERNEL...
        R(REG_ITER) = resMaxRed;
        REPEAT_X_TIMES(resMaxRed)
        ...
    */
#else
    R(REG_QUOTIENT) = 1 << NUM_ITERS;
#endif

    //TYPE residual = dividend;
    R(REG_RESIDUAL) = R(REG_SRC1) | R(REG_SRC1); // COPY implemented with OR

    //R(REG_ITER) = NUM_ITERS;

    // Loop computing the bit NUM_ITERS - iter of the quotient
//    for (iter = 0; iter <= NUM_ITERS; iter++)

#ifdef ADAPTIVE_RUN
  /*
  // This works always, with the standard ISA:
  R(REG_ITER) = NUM_ITERS;
  REPEAT_X_TIMES(NUM_ITERS + 1);
  */

  //R(REG_NUM_ITERS) += R(REG_CT1);
  REPEAT_REDUCE(REG_NUM_ITERS);
#else
    for (int iter = NUM_ITERS; iter >= 0; iter--) {
#endif
        /*
        R(REG_ITER) = iter;
        */

      #ifdef ADAPTIVE_RUN
       #ifdef OLD_MANUAL
        PrintDebugMessage("REG_ITER:\n");
        PRINTREG(REG_ITER);
       #endif
      #else
        PrintDebugMessage(("iter = " + to_string(iter) + "\n").c_str());
      #endif

        PrintDebugMessage("REG_QUOTIENT:\n");
        PRINTREG(REG_QUOTIENT);
        PrintDebugMessage("REG_RESIDUAL:\n");
        PRINTREG(REG_RESIDUAL);

//        divisori32 = divisor << (NUM_ITERS - iter);
//        TYPE divisorHigh16 = divisor >> (16 - NUM_ITERS + iter);

        //divisorAlignedLow16 = divisor << iter;
        //R(REG_DIVISOR_ALIGNED_LOW16) = R(REG_SRC2) << R(REG_ITER);
      #ifdef ADAPTIVE_RUN
       #ifdef OLD_MANUAL
        R(REG_DIVISOR_ALIGNED_LOW16) = R(REG_SRC2) << R(REG_ITER);
       #else
        R(REG_QUOTIENT) = R(REG_CT1) << R(0);

        //R(0) -= R(REG_CT1);
        PrintDebugMessage("REG 0:\n");
        PRINTREG(0);
        R(REG_DIVISOR_ALIGNED_LOW16) = R(REG_SRC2) << R(0);
       #endif
      #else
        R(REG_DIVISOR_ALIGNED_LOW16) = R(REG_SRC2) << iter;
      #endif

        //TYPE divisorAlignedHigh16 = divisor >> (16 - iter);
      #ifdef ADAPTIVE_RUN
       #ifdef OLD_MANUAL
        R(REG_ITER16) = 16;
        R(REG_ITER16) -= R(REG_ITER);
       #else
        PRINTREG(0);
        R(REG_ITER16) = R(REG_CT16) - R(0);
        PrintDebugMessage("REG_ITER16:\n");
        PRINTREG(REG_ITER16);

        R(0) -= R(REG_CT1);
       #endif
        R(REG_DIVISOR_ALIGNED_HIGH16) = R(REG_SRC2) >> R(REG_ITER16);

       #ifdef OLD_MANUAL
        R(REG_ITER) -= R(REG_CT1);
       #endif
      #else
        R(REG_DIVISOR_ALIGNED_HIGH16) = R(REG_SRC2) >> (16 - iter);
      #endif
        //R(REG_DIVISOR_ALIGNED_HIGH16) = ISHR(R(REG_SRC2), (16 - iter));
        //R(REG_DIVISOR_ALIGNED_HIGH16) = R(REG_SRC2) >> R(REG_ITER16);

        /*
        if (divisori32 == residual && divisorHigh16 == 0) {
            // *aRemainder = 0;
            residual = 0;
            quotientTotal |= quotient;
            break;
        }
        */
PrintDebugMessage("IMPORTANT: REG_DIVISOR_ALIGNED_LOW16:\n");
PRINTREG(REG_DIVISOR_ALIGNED_LOW16);
PrintDebugMessage("IMPORTANT: REG_DIVISOR_ALIGNED_HIGH16:\n");
PRINTREG(REG_DIVISOR_ALIGNED_HIGH16);
        R(REG_AUX) = R(REG_DIVISOR_ALIGNED_LOW16) == R(REG_RESIDUAL);
        R(REG_AUX2) = R(REG_DIVISOR_ALIGNED_HIGH16) == R(REG_CT0);
        R(REG_AUX) = R(REG_AUX) & R(REG_AUX2);

        // IMPORTANT: accounting for the special cases (TYPE_MIN) done at the beginning - executing only if REG_MASK_CONTINUE == 1
        R(REG_AUX) = R(REG_AUX) & R(REG_MASK_CONTINUE);

        R(REG_AUX) = R(REG_AUX) == R(REG_CT1);
        NOP;
    ); // End EXECUTE_IN_ALL
        EXECUTE_WHERE_EQ(
            R(REG_RESIDUAL) = 0;
            R(REG_QUOTIENT_TOTAL) = R(REG_QUOTIENT_TOTAL) | R(REG_QUOTIENT);
        );

        EXECUTE_IN_ALL(
            // IMPORTANT: adjusting R(REG_MASK_CONTINUE)
            R(REG_AUX) = R(REG_CT1) - R(REG_AUX);
            R(REG_MASK_CONTINUE) = R(REG_MASK_CONTINUE) & R(REG_AUX);

PrintRegDebug(REG_QUOTIENT_TOTAL);
//PrintRegDebug(REG_MASK_CONTINUE);
            // IMPORTANT: accounting for the special cases (TYPE_MIN) done at the beginning

PrintDebugMessage("IMPORTANT: REG_MASK_CONTINUE:\n");
PRINTREG(REG_MASK_CONTINUE);
/*
            //R(REG_AUX3) = R(REG_AUX) == R(REG_CT0);
            R(REG_AUX3) = R(REG_AUX) == R(REG_CT1);
PrintDebugMessage("IMPORTANT: REG_AUX3:\n");
PRINTREG(REG_AUX3);
            NOP;
//        );
PRINTREG(REG_AUX);
PRINTREG(REG_DIVISOR_ALIGNED_LOW16);
PRINTREG(REG_DIVISOR_ALIGNED_HIGH16);
*/
PrintRegDebug(REG_RESIDUAL);
PrintRegDebug(REG_DIVISOR_ALIGNED_LOW16);

        // Else
        //EXECUTE_WHERE_EQ(
// TODO: simply go directly for the 2nd else branch in the C code
// IMPORTANT: we check R(REG_DIVISOR_ALIGNED_LOW16) <= R(REG_RESIDUAL) && R(REG_DIVISOR_ALIGNED_LOW16) == 0
            //R(REG_AUX) = R(REG_RESIDUAL) < R(REG_DIVISOR_ALIGNED_LOW16);
            R(REG_AUX) = ULT(R(REG_RESIDUAL), R(REG_DIVISOR_ALIGNED_LOW16));
            R(REG_AUX) = R(REG_CT1) - R(REG_AUX);

            R(REG_AUX2) = R(REG_DIVISOR_ALIGNED_HIGH16) == R(REG_CT0);
            R(REG_AUX) =  R(REG_AUX) & R(REG_AUX2);

            // IMPORTANT
            R(REG_AUX) = R(REG_AUX) & R(REG_MASK_CONTINUE);
            // IMPORTANT: we canNOT adjust REG_MASK_CONTINUE here
            /*
            R(REG_AUX2) = R(REG_CT1) - R(REG_AUX);
            R(REG_MASK_CONTINUE) = R(REG_MASK_CONTINUE) & R(REG_AUX2);
            */

            R(REG_AUX) = R(REG_AUX) == R(REG_CT1);
PrintDebugMessage("REG_AUX (for else branch):\n");
PRINTREG(REG_AUX);
PrintDebugMessage("REG_MASK_CONTINUE (for else branch):\n");
PRINTREG(REG_MASK_CONTINUE);
            NOP;
        );
            EXECUTE_WHERE_EQ(
                R(REG_RESIDUAL) = R(REG_RESIDUAL) - R(REG_DIVISOR_ALIGNED_LOW16);
PrintDebugMessage("REG_RESIDUAL:\n");
PRINTREG(REG_RESIDUAL);
                // TODO: think if we can optimize this assignment done also before
                R(REG_QUOTIENT_TOTAL) = R(REG_QUOTIENT_TOTAL) | R(REG_QUOTIENT);
            );
// TODO: try to do better
        EXECUTE_IN_ALL(
        #ifdef ADAPTIVE_RUN
          #ifdef OLD_MANUAL
            R(REG_QUOTIENT) = R(REG_QUOTIENT) >> R(REG_CT1);
          #endif
        #else
            R(REG_QUOTIENT) = R(REG_QUOTIENT) >> R(REG_CT1);
        #endif
        /*
        else
        if ((unsigned short)divisorAlignedLow16 < residual &&
                //  remember dividend >=0 and divisor >= 0
                divisorHigh16 == 0) {
            residual -= divisori32;
            quotientTotal |= quotient;
            //quotient >>= 1;
        }

        quotient >>= 1;
        */

#ifdef ADAPTIVE_RUN
PrintDebugMessage("Before END_REPEAT: REG_MASK_CONTINUE:\n");
PRINTREG(REG_MASK_CONTINUE);

 END_REPEAT;
#else
    } // end for loop
#endif

      // IMPORTANT: accounting for the special cases (TYPE_MIN) done at the beginning - executing only if REG_MASK_CONTINUE == 1
      R(REG_AUX) = R(REG_MASK_CONTINUE) == R(REG_CT1);
      NOP;
    );
    EXECUTE_WHERE_EQ(
        //*aRemainder = residual;
        R(REG_REMAINDER) = R(REG_RESIDUAL) | R(REG_RESIDUAL); // COPY implemented with OR
    );
  EXECUTE_IN_ALL(

PrintDebugMessage("REG_QUOTIENT_TOTAL:\n");
PRINTREG(REG_QUOTIENT_TOTAL);
PrintDebugMessage("REG_REMAINDER:\n");
PRINTREG(REG_REMAINDER);


    /*
    if (changeSign == 1 | changeSign == 2) {
        quotientTotal = -quotientTotal; // for short it is OK
    }
    if (changeSign == 1 | changeSign == 3) {
        *aRemainder = - (*aRemainder);
    }
    */

    PrintRegDebug(REG_CHANGE_SIGN);

    R(REG_AUX) = POPCNT(R(REG_CHANGE_SIGN));
    //R(REG_AUX2) = R(REG_AUX) == R(REG_CT1);
    // IMPORTANT: executing only if REG_MASK_CONTINUE == 1
    R(REG_AUX) = R(REG_AUX) & R(REG_MASK_CONTINUE);
    R(REG_AUX) = R(REG_AUX) == R(REG_CT1);
    NOP;
  ); // End EXECUTE_IN_ALL
  EXECUTE_WHERE_EQ(
        R(REG_QUOTIENT_TOTAL) = R(REG_CT0) - R(REG_QUOTIENT_TOTAL);
  );
  EXECUTE_IN_ALL(
    R(REG_AUX)  = R(REG_CHANGE_SIGN) & R(REG_CT1);
    //R(REG_AUX2) = R(REG_AUX) == R(REG_CT1);
    // IMPORTANT: executing only if REG_MASK_CONTINUE == 1
    R(REG_AUX) = R(REG_AUX) & R(REG_MASK_CONTINUE);
    R(REG_AUX) = R(REG_AUX) == R(REG_CT1);
    NOP;
  ); // End EXECUTE_IN_ALL
  EXECUTE_WHERE_EQ(
        R(REG_REMAINDER) = R(REG_CT0) - R(REG_REMAINDER);
  );
  EXECUTE_IN_ALL(










#ifdef TREAT_DIVIDEND_EQUAL_TYPEMIN
    /*
    if (correctQuotient == 0) { // we are in the case (orig) dividend == TYPE_MIN
        // IMPORTANT NOTE: since dividend < 0 --> remainder <= 0
        *aRemainder -= 1;
        if (*aRemainder == -abs(origDivisor)) { // aRemainder <= 0
            *aRemainder = 0;

            //assert(((long)quotientTotal + correctQuotient) != TYPE_MAX + 1); // Can't return quotient in i16 as 32768
            quotientTotal += correctQuotient;
        }
    }
    */
    PrintDebugMessage("TREAT_DIVIDEND_EQUAL_TYPEMIN: REG_QUOTIENT_TOTAL:\n");
    PRINTREG(REG_QUOTIENT_TOTAL);
    PrintDebugMessage("REG_REMAINDER:\n");
    PRINTREG(REG_REMAINDER);

    R(REG_AUX) = R(REG_CORRECT_QUOTIENT) == R(REG_CT0);
    R(REG_AUX) = R(REG_AUX) == R(REG_CT0); // R(REG_CORRECT_QUOTIENT) != 0
    /* IMPORTANT: executing only if REG_MASK_CONTINUE == 1 - accounting for the special case (TYPE_MIN) done at the beginning
    R(REG_AUX) = R(REG_AUX) & R(REG_MASK_CONTINUE);
    R(REG_AUX) = R(REG_AUX) == R(REG_CT1);
    */
    NOP;
  );
  EXECUTE_WHERE_EQ(
      R(REG_REMAINDER) = R(REG_REMAINDER) - R(REG_CT1);
      /* IMPORTANT NOTE: Remember that we have divisor > 0, since we have a
             "sign-and-magnitude" algorithm.
           So, we negate it to have: -abs(origDivisor).
      */
      R(REG_SRC2) = R(REG_CT0) - R(REG_SRC2);
  );
  // TODO TODO TODO TODO TODO TODO TODO TODO TODO: think if we can do WHERE in WHERE - and work on Connex
  EXECUTE_IN_ALL(
    PrintDebugMessage("IMPORTANT: REG_CORRECT_QUOTIENT:\n");
    PRINTREG(REG_CORRECT_QUOTIENT);
    PrintDebugMessage("REG_SRC2:\n");
    PRINTREG(REG_SRC2);

    //R(REG_AUX) = R(REG_AUX) == R(REG_CT0); // R(REG_CORRECT_QUOTIENT) != 0
    R(REG_AUX2) = R(REG_REMAINDER) == R(REG_SRC2);
    R(REG_AUX) = R(REG_AUX) & R(REG_AUX2);
    R(REG_AUX) = R(REG_AUX) == R(REG_CT1);
    NOP;
  );
    EXECUTE_WHERE_EQ(
        R(REG_REMAINDER) = 0;
        R(REG_QUOTIENT_TOTAL) = R(REG_QUOTIENT_TOTAL) + R(REG_CORRECT_QUOTIENT);
    );

  EXECUTE_IN_ALL(
#endif






           //LS[R(2)] = R(REG_RES0_L);
           LS[R(2)] = R(REG_QUOTIENT_TOTAL);
           LS[R(5)] = R(REG_REMAINDER);

           R(2) = R(2) + R(3);
           R(5) = R(5) + R(3);
        PRINTREG(2);
        PRINTREG(5);

        //R(1) = R(1) + R(3);
        //R(6) = R(6) + R(3);

    //END_REPEAT;
    } // END for idxLoop

           REDUCE R(0); // We add a 'bogus' REDUCE to wait for it
       );
    _END_KERNEL(BatchNumberGlobal);




    try {
        Kernel *kernel = connexGlobal->getKernel(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));

      #ifdef LLVM_ISEL_CODEGEN
        kernel->sdNodeVarNameRegDef[REG_SRC1] = "nodeOpSrcCast1";
        kernel->sdNodeVarNameRegDef[REG_SRC2] = "nodeOpSrcCast2";
        kernel->offsetKernelToStartCodegenFrom = 12 + 1; // +1 for the END_WHERE instruction Opincaa adds automatically to each kernel
        //kernel->numInstructionsToCodegen = 489 - 5 - 13;
        kernel->numInstructionsToCodegen = kernel->size()
                                             - 5 /*num instruction we remove from end of kernel */
                                             - kernel->offsetKernelToStartCodegenFrom;
        assert(kernel->numInstructionsToCodegen == 489 - 5 - 13);
        //
        // We use chain, since with glue with get a lot or weird scheduling errors:
        kernel->useGlue = 0;
        //kernel->useGlue = 1;
        // IMPORTANT: to convert in 'partly SSA form' we require ~64 registers
        assert(CONNEX_REG_COUNT != 32);

        /*
        printf("Calling connexGlobal->genLLVMISelManualCode()\n");
        fflush(stdout);
        string resGenLLVM = connexGlobal->genLLVMISelManualCode(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
        printf("resGenLLVM = \n%s\n", resGenLLVM.c_str());
        fflush(stdout);
        */
        printf("Calling connexGlobal->genLLVMISelManualCode()\n");
        fflush(stdout);
        string resGenLLVM = connexGlobal->genLLVMISelManualCode(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
        printf("resGenLLVM = \n%s\n", resGenLLVM.c_str());
        fflush(stdout);



        printf("Calling connexGlobal->dumpKernel()\n");
        fflush(stdout);
        string resDump = connexGlobal->dumpKernel(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
        printf("resDump = %s\n", resDump.c_str());
        fflush(stdout);

        printf("Calling connexGlobal->disassembleKernel()\n");
        fflush(stdout);
        string resDis = connexGlobal->disassembleKernel(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
        printf("resDis = %s\n", resDis.c_str());
        fflush(stdout);
      #endif

        connexGlobal->executeKernel(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
        connexGlobal->readReduction();

        printf("Returned from connexGlobal->readReduction()\n");
        fflush(stdout);

        #ifndef WRITE_ABS_VALUE_IN_MEM_FOR_TESTING
            connexGlobal->readDataFromConnexPartial((&C [(0 +  0)]),
                        /* actual num elems read */ (N),
                        /*offset*/ 2 * (int)ceil(((float)(N))/CONNEX_VECTOR_LENGTH));

            connexGlobal->readDataFromConnexPartial((&D [(0 +  0)]),
                        /* actual num elems read */ (N),
                        /*offset*/ 3 * (int)ceil(((float)(N))/CONNEX_VECTOR_LENGTH));
        #else
            connexGlobal->readDataFromConnexPartial((&C [(0 +  0)]),
                        /* actual num elems read */ (N),
                        /*offset*/ 0); // 0 is offset of A
        #endif
    }
    catch (string err) {
        cout << err << endl;
    }
    catch (...) {
        cout << "Unknown exception" << endl;
    }

    return 0;
}


//#define TEST_OPINCAA_CONNEX_KERNELS

#ifdef TEST_OPINCAA_CONNEX_KERNELS
int Test() {
    //#define NUM_ELEMS 1000000
    //#define NUM_ELEMS 10000
    //#define NUM_ELEMS 8192
    //#define NUM_ELEMS 1024
    //#define NUM_ELEMS 256
    //#define NUM_ELEMS 128
    //#define NUM_ELEMS 64

   //#define NUM_ELEMS (CONNEX_VECTOR_LENGTH/2)
   #define NUM_ELEMS CONNEX_VECTOR_LENGTH
   //#define NUM_ELEMS (CONNEX_VECTOR_LENGTH/2) * 10

    TYPE A[NUM_ELEMS + 100];
    TYPE B[NUM_ELEMS + 100];
    TYPE C[NUM_ELEMS + 10000];
    TYPE D[NUM_ELEMS + 10000];


    //srand(time(NULL));
    srand(0);


    int i, testResult;

    printf("Entered Test()\n");
    printf("  NUM_ELEMS = %d (sizeof(TYPE) = %lu)\n", NUM_ELEMS, sizeof(TYPE));

    printf("RAND_MAX = %d\n", RAND_MAX);
    // We assume RAND_MAX = 2,147,483,647 (before it was RAND_MAX = 32767)

    assert(RAND_MAX == 2147483647);


    for (i = 0; i < NUM_ELEMS; i++) {
        //A[i] = 3; // i;
        //A[i] = 40000; // i;

        /*
        A[i] = 32767; // i;
        B[i] = 32767; // i / 2;
        */

        //A[i] = 42767; // i;
        //B[i] = 32767; // i / 2;

        //A[i] = 327;
        //B[i] = 2767;

        //A[i] = i;
        //B[i] = i / 2;

        /*
        // TODO TODO TODO Works very well (the i16 numbers are positive, but the + can enable carry):
        A[i] = ((rand() % 32768) << 16) + (rand() % 32768);
        B[i] = ((rand() % 32768) << 16) + (rand() % 32768);
        */


/*
// Works well:
        //A[i] = 65536;
        A[i] = 6;
        //A[i] = 65536;
        B[i] = 16384;
*/
// B is multiplicand, A is multiplier

    // NOT true: signed multiplication is different from UNsigned:

/*
// TODO TODO TODO TODO TODO: finish this case - it seems we need to take into consideration the sign of the i32's
// We are doing signed multiplication on Connex and this is BAD for the following case, where we have A and B signed, but they are actually > 0.
        A[i] = 65535;
        B[i] = 255;
Important note:
    Example: when
        A[i] = 65535;
        B[i] = 255;
    -1 * 255 = -255, which is 0xffffff01

// 255 * 65535 = 16711425
// 65535 - 254  + 65535 = 130816
*/


/*
// Works well:
        A[i] = 1020;
        B[i] = 255;
*/

/*
    // GOOD:
        A[i] = rand() % 32768;
        B[i] = rand() % 32768;
*/

/*
*/
    // GOOD:
        //A[i] = rand() % 65536 - 32768;
        A[i] = rand() % 65534 - 32767;
        //
        B[i] = rand() % 65536 - 32768;
        if (B[i] == 0)
            B[i]++;
/*
        A[i] = 16384;
        B[i] = 1;
*/

/*
    // GOOD:
        A[i] = -32768;
        B[i] = -32768;
*/

/*
        A[i] = rand() % 1000000000;
        B[i] = rand() % 1000000000;
*/

/*
        A[i] = rand() % 1000000000 - 1000000000;
        B[i] = rand() % 1000000000 - 1000000000;
        */
        /*
        A[i] = rand() % 1000000000 - 500000000;
        B[i] = rand() % 1000000000 - 500000000;
        */

/*
        A[i] = 0x000049fc;
        B[i] = 0x00002360;
*/

//        A[i] = ((rand() % 4) << 30) + ((rand() % 32768) << 15) + (rand() % 32768);
//        B[i] = ((rand() % 4) << 30) + ((rand() % 32768) << 15) + (rand() % 32768);

        /*
        // Works on x64
        A[i] = ((rand() % 2) << 31) + (rand());
        B[i] = ((rand() % 2) << 31) + (rand());
        */

        /*
        A[i] = rand() % 100000000;
        B[i] = rand() % 100000000;
        */

        /*
        // Works very well:
        A[i] = -1;
        B[i] = -1;
        */

/*
if (i == 0) {
A[i] = 6106073;
B[i] = 28094237;
}
else {
A[i] = 0;
B[i] = 0;
}
*/
        /*
        if (i & 1 == 0) {
            A[i] = rand() % (RAND_MAX + 1);
            B[i] = rand() % (RAND_MAX + 1);
        }
        else {
            A[i] = rand() % (RAND_MAX + 1) - 32768;
            B[i] = rand() % (RAND_MAX + 1) - 32768;
        }
        */
        //C[i] = -1;


        printf("A[%d] = 0x%04hx\n", i, A[i]);
        printf("B[%d] = 0x%04hx\n", i, B[i]);
    }

#ifdef NO_MORE_MANUAL_TESTS_FOR_RANDOM_TESTS_TO_PUT_IN_PAPER
    A[0] = 0x6125;
    B[0] = 0x095d;
    /*
    A[0] = 2;
    B[0] = -32768;
    */

    A[1] = 4;
    B[1] = 1;

    A[2] = 10;
    B[2] = 4;

    A[3] = 8;
    B[3] = 1;

    A[4] = 9;
    B[4] = 2;

    A[5] = 9;
    B[5] = 0;

    A[6] = 0;
    B[6] = 0;

    A[7] = 1;
    B[7] = 1;
    /*
    A[5] = 0x6125;
    B[5] = 0x095d;
    */
#endif

    A[0] = -32768;
    B[0] = -32768;

    A[1] = -32768; // quotient = 0xffc0, remainder = 0xff00
    B[1] = 508;

    A[2] = -32768;
    B[2] = -20508;


    printf("Calling DivInt16 (with quotient and reminder results)...\n");
    fflush(stdout);

    DivInt16(A, B, C, D, NUM_ELEMS);

    printf("Finished executing the Opincaa kernel.\n");
    fflush(stdout);
    //
    printf("Testing the correctness of the computation of the Opincaa kernel.\n");
    fflush(stdout);


    #define FAIL -1
    #define PASS 0

    int numDiffResults = 0;

    testResult = PASS;
    for (i = 0; i < NUM_ELEMS; i++) {
        if (B[i] != 0 &&
             (A[i] / B[i] != C[i] || A[i] % B[i] != D[i])) {
            testResult = FAIL;
            //break;

            numDiffResults++;
        }
    }

    printf("  testResult = %d (PASS = %d)\n", testResult, PASS);
    printf("  numDiffResults = %d\n", numDiffResults);

    //if (testResult == FAIL) {
        printf("NUM_ELEMS = %d\n", NUM_ELEMS);
        for (i = 0; i < NUM_ELEMS + 5; i++)
            if ((i < NUM_ELEMS) && B[i] != 0 &&
                    (A[i] / B[i] != C[i] || A[i] % B[i] != D[i])) {
                printf("C[%d] = 0x%04hx, D[i] = 0x%04hx ( != 0x%04hx, 0x%04hx)\n",
                        i, C[i], D[i],
                        B[i] == 0 ? -1001 : A[i] / B[i],
                        B[i] == 0 ? -1001 : A[i] % B[i]);
                printf("  A[%d] = %d (0x%04hx)\n", i, A[i], A[i]);
                printf("  B[%d] = %d (0x%04hx)\n", i, B[i], B[i]);
            }
            else {
                printf("C[%d] = %d (0x%04hx), D[i] = 0x%04hx\n",
                        i, C[i], C[i], D[i]);
                printf("  A[%d] = 0x%04hx\n", i, A[i]);
                printf("  B[%d] = 0x%04hx\n", i, B[i]);
            }
    //}

    printf("Exiting Test()\n");

    return testResult;
}

/*
int main() {
    Test();

    return 0;
}
*/

#endif

