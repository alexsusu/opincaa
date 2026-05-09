/* Performance of the MaxReduce procedure:
     - number of cycles simulated = 217 (add 1 more for the REDUCE)
         (and 2 less for UNnecessary prologue and epilogue)
     - with broadcast of the max value: <<number of cycles simulated = 338
         (add 1 more for the REDUCE)>> (and 2 less for UNnecessary
                                        prologue and epilogue)
     Note: with debug messages we reach about 1000 cycles.

    This code is inspired from
      /home/asusu/LLVM/llvm38Nov2016/llvm/build40/bin/Tests/NEW_v128i16/opincaa_standalone_apps/Emulate_i16_DIV/Opincaa/output.cpp

    Pseudocode for MAX-reduction (with halving the vector, similar to what LLVM LoopVectorize generates itself):
      This uses the fact MAX is commutative - see e.g., also
      http://developer.amd.com/resources/articles-whitepapers/opencl-optimization-case-study-simple-reductions/ .
*/

void RedMax(Kernel *__kernel,
                    int REG_CT0, int REG_CT1, int REG_SRC, int REG_MAX2,
                    int REG_IDX, int REG_PRED, int REG_PRED2, int REG_STEPS) {
  EXECUTE_IN_ALL(
    R(REG_IDX) = INDEX;
    PrintRegDebug(REG_IDX);

  for (int i = 2; i <= CONNEX_VECTOR_LENGTH; i *= 2) {

      PrintDebugMessage("One more iteration of MAX-reduce:\n");

      // We compute the max-REDUCE of REG_SRC:
      R(REG_STEPS) = CONNEX_VECTOR_LENGTH / i;
    PrintRegDebug(REG_STEPS);
      CELL_SHL( R(REG_SRC), R(REG_STEPS) );
      //
      // NOP for CONNEX_VECTOR_LENGTH / i times
      for (int iNOP = 0; iNOP < CONNEX_VECTOR_LENGTH / i; iNOP++) {
          NOP;
      }
      //
      R(REG_MAX2) = SHIFT_REG;
      PrintRegDebug(REG_SRC);
      PrintRegDebug(REG_MAX2);

      // For lower halves of vectors REG_SRC and REG_MAX2 we choose the max

      R(REG_PRED) = R(REG_IDX) < R(REG_STEPS);
      R(REG_PRED2) = R(REG_SRC) < R(REG_MAX2);
    //PrintRegDebug(REG_PRED2);
      R(REG_PRED) &= R(REG_PRED2); // R(REG_PRED) & R(REG_PRED2);
      R(REG_PRED2) = R(REG_PRED) == R(REG_CT1);
    PrintRegDebug(REG_PRED);
      NOP;
    );
    EXECUTE_WHERE_EQ(
          R(REG_SRC) = R(REG_MAX2);
      PrintRegDebug(REG_SRC);
    );

    EXECUTE_IN_ALL();
  }
  /*
    PrintDebugMessage("Broadcasting the MAX value in REG_SRC[0]\n");
    // We now put in R(REG_SRC) the max value that we have in REG_SRC[0]
    PrintRegDebug(REG_IDX);
      CELL_SHR( R(REG_SRC), R(REG_IDX) );
      //
      // NOP for CONNEX_VECTOR_LENGTH - 1 times
      for (int i = 0; i < CONNEX_VECTOR_LENGTH - 1; i++) {
          NOP;
      }
      //
      R(REG_SRC) = SHIFT_REG;
    PrintRegDebug(REG_SRC);

  );
  */
}


void RedMin(Kernel *__kernel,
                    int REG_CT0, int REG_CT1, int REG_SRC, int REG_MAX2,
                    int REG_IDX, int REG_PRED, int REG_PRED2, int REG_STEPS) {
  EXECUTE_IN_ALL(
    R(REG_IDX) = INDEX;
    PrintRegDebug(REG_IDX);

  for (int i = 2; i <= CONNEX_VECTOR_LENGTH; i *= 2) {

      PrintDebugMessage("One more iteration of MIN-reduce:\n");

      // We compute the max-REDUCE of REG_SRC:
      R(REG_STEPS) = CONNEX_VECTOR_LENGTH / i;
    PrintRegDebug(REG_STEPS);
      CELL_SHL( R(REG_SRC), R(REG_STEPS) );
      //
      // NOP for CONNEX_VECTOR_LENGTH / i times
      for (int iNOP = 0; iNOP < CONNEX_VECTOR_LENGTH / i; iNOP++) {
          NOP;
      }
      //
      R(REG_MAX2) = SHIFT_REG;
      PrintRegDebug(REG_SRC);
      PrintRegDebug(REG_MAX2);

      // For lower halves of vectors REG_SRC and REG_MAX2 we choose the max

      R(REG_PRED) = R(REG_IDX) < R(REG_STEPS);
      R(REG_PRED2) = R(REG_SRC) < R(REG_MAX2);
    //PrintRegDebug(REG_PRED2);
      R(REG_PRED) &= R(REG_PRED2); // R(REG_PRED) & R(REG_PRED2);
      R(REG_PRED2) = R(REG_PRED) == R(REG_CT1);
    PrintRegDebug(REG_PRED);
      NOP;
    );
    EXECUTE_WHERE_EQ(
          R(REG_SRC) = R(REG_SRC);
      PrintRegDebug(REG_SRC);
    );

    EXECUTE_IN_ALL();
  }
}


void BroadcastScalar(Kernel *__kernel,
                    int REG_SRC, int REG_IDX, bool doRegIdx=false) {
    PrintDebugMessage("Broadcasting the value in REG_SRC[0]\n");

    if (doRegIdx) {
        R(REG_IDX) = INDEX;
        PrintRegDebug(REG_IDX);
    }

    // We now put in R(REG_SRC) the max value that we have in REG_SRC[0]
    PrintRegDebug(REG_IDX);
    CELL_SHR( R(REG_SRC), R(REG_IDX) );
    //
    // NOP for CONNEX_VECTOR_LENGTH - 1 times
    for (int i = 0; i < CONNEX_VECTOR_LENGTH - 1; i++) {
        NOP;
    }
    //
    R(REG_SRC) = SHIFT_REG;
    PrintRegDebug(REG_SRC);
}

