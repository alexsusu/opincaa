/*
 *
 * File: simple_tests.cpp
 *
 * Simple tests for all instructions on connex machine.
 * Each instruction is tested via one bacth that uses reduction for checking the result
 *
 *
 */
#include "../../include/core/cnxvector_registers.h"
#include "../../include/core/cnxvector.h"
#include "../../include/core/io_unit.h"
#include "../../include/c_simu/c_simulator.h"
#include "../../include/util/utils.h"
#include "../../include/util/kernel_acc.h"

#include <iostream>
#include <iomanip>
#include <limits>
using namespace std;

struct Dataset
{
   INT32 Param1;
   INT32 Param2;
   INT32 ExpectedResult;
};

struct TestFunction
{
   int BatchNumber;
   const char *OperationName;
   void (*initKernel)(int BatchNumber,INT32 Param1,INT32 Param2);
   Dataset ds;
};

static void InitKernel_Write(int BatchNumber,INT32 Param1, INT32 Param2);
static void InitKernel_Nop(int BatchNumber,INT32 Param1, INT32 Param2)
{
    InitKernel_Write(BatchNumber, Param1, Param2);
}

static void InitKernel_Iwrite(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
       EXECUTE_IN_ALL(
                        R0 = INDEX;
                        NOP;
                        LS[Param1] = R0;
                        R1 = LS[Param1];
                        REDUCE(R1);
                    )

    END_BATCH(BatchNumber);
}

static void InitKernel_Iread(int BatchNumber,INT32 Param1, INT32 Param2)
{
    InitKernel_Iwrite(BatchNumber, Param1, Param2);
}

static void InitKernel_Write(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = INDEX;
                        R1 = Param1;
                        R2 = 1;
                        R1 += R2;
                        NOP;
                        LS[R1] = R0;
                        R2 = LS[Param1 + 1];
                        REDUCE(R2);
                        )


    END_BATCH(BatchNumber);
}

static void InitKernel_Read(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = INDEX;
                        R1 = Param1;
                        NOP;
                        R2 = 1;
                        R1 += R2;
                        LS[Param1 + 1] = R0;
                        R2 = LS[R1];
                        REDUCE(R2);
                        )


    END_BATCH(BatchNumber);
}

static void InitKernel_Jump(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = 1;
                        R0 = Param1;
                        SET_JMP_LABEL(99);
                        R0 = R0 - R1;
                        JMP_TIMES_TO_LABEL(Param2,99);// times, label
                        REDUCE(R0);
                       )
    NOP;//hardware bug workaround
    END_BATCH(BatchNumber);
}

static void InitKernel_Jump2(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = 1;
                        R0 = Param1;
                        REPEAT_X_TIMES
                        (Param2,
                            R0 = R0 - R1;
                        )
                        REDUCE(R0);
                       )
    NOP;//hardware bug workaround
    END_BATCH(BatchNumber);
}

static void InitKernel_Jump3(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = Param1;
                        for (int x=0; x< Param2; x++)
                        {
                            //NOP;
                            REPEAT_X_TIMES
                            (Param2,
                                REDUCE(R0);
                            )
                            //for (int nops=0; nops< 10; nops++)

                        }
                       )
    NOP;//hardware bug workaround
    END_BATCH(BatchNumber);
}

static void InitKernel_FusedAddReduce(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = (UINT_PARAM)Param1;
                        R2 = (UINT_PARAM)Param2;
                        FUSED_REDUCE(R3 = R1 + R2);
                      )
    END_BATCH(BatchNumber);
}

static void InitKernel_Add(int BatchNumber,INT32 Param1, INT32 Param2);
static void InitKernel_Vload(int BatchNumber,INT32 Param1, INT32 Param2)
{
    InitKernel_Add(BatchNumber,Param1, Param2);
}

static void InitKernel_Add(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = (UINT_PARAM)Param1;
                        R2 = (UINT_PARAM)Param2;
                        R3 = R1 + R2;
                        REDUCE(R3);
                     )

    END_BATCH(BatchNumber);
}

/*
static void InitKernel_pAdd(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = (UINT_PARAM)Param1;
                        //R2 = R1 + (UINT_PARAM)Param2;
                        R3 = Param2 + R1;
                        REDUCE(R2);
                     )

    END_BATCH(BatchNumber);
}*/

// pseudo instruction with zero
/*
static void InitKernel_pzAdd(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = (UINT_PARAM)(Param1 + Param2);
                        R2 = R1 + 0;
                        //R3 = Param2 + R1;
                        REDUCE(R2);
                     )

    END_BATCH(BatchNumber);
}*/

static void InitKernel_sAdd(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R2 += R1;
                        REDUCE(R2);
                     )

    END_BATCH(BatchNumber);
}

/*
static void InitKernel_Inc(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R4 = Param1;
                        R4 = R4 + 1;
                        REDUCE(R4);
                    )

    END_BATCH(BatchNumber);
}

static void InitKernel_Inc2(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R4 = Param1;
                        R4+= 1;
                        REDUCE(R4);
                    )

    END_BATCH(BatchNumber);
}

static void InitKernel_Inc3(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R4 = Param1;
                        R4++;
                        REDUCE(R4);
                    )

    END_BATCH(BatchNumber);
}
*/

static void InitKernel_Addc(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R4 = 0xFF;
                        R5 = 0xFFFF;
                        R6 = R4 + R5;

                        R1 = Param1;
                        R2 = Param2;
                        R3 = ADDC(R1, R2);
                        /*
                        is equivalent to:
                        R4 = 0;
                        EXECUTE_WHERE_CARRY(R4 = 1;)
                        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = R1 + R2;
                        R3 = R3 + R4;
                        */
                        REDUCE(R3);
                    )

    END_BATCH(BatchNumber);
}

static void InitKernel_pAddc(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R4 = 0xFF;
                        R5 = 0xFFFF;
                        R6 = R4 + R5;

                        R1 = Param1;
                        R2 = ADDC(R1, Param2);
                        /*
                        is equivalent to
                        R4 = 0;
                        EXECUTE_WHERE_CARRY(R4 = 1;)
                        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R2 = R1 + R2;
                        R2 = R2 + R4;
                        */
                        REDUCE(R2);
                    )

    END_BATCH(BatchNumber);
}

static void InitKernel_Sub(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = R1 - R2;
                        REDUCE(R3);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_sSub(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R1 -= R2;
                        REDUCE(R1);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_pSub(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = R1 - Param2;
                        REDUCE(R2);
                        )

    END_BATCH(BatchNumber);
}


static void InitKernel_CondSub(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R2 = CONDSUB(R1,R2);
                        REDUCE(R2);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Subc(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = 0xff;
                        R2 = 0xffff;
                        R3 = R1 - R2;
                        /* The sequence of 3 instructions:
                        R1 = Param1;
                        R2 = Param2;
                        R3 = SUBC(R1,R2);
                        is replaceble with the following 8 instructions:
                        */

                        R4 = 0;
                        EXECUTE_WHERE_CARRY(R4 = 1;)
                        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = R1 - R2;
                        R3 = R3 - R4;
                        REDUCE(R3);)
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_pSubc(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = 0xff;
                        R2 = 0xffff;
                        R3 = R1 - R2;

                        /* The sequence of 3 instructions here the SUBC is pseudo-instruction:
                        R1 = Param1;
                        R2 = SUBC(R1,Param2);

                        is replaceble with the following 8 instructions:
                        */
                        R4 = 0;
                        EXECUTE_WHERE_CARRY(R4 = 1;)
                        EXECUTE_IN_ALL(
                                R1 = Param1;
                                R2 = Param2;
                                R2 = R1 - R2;
                                R2 = R2 - R4;
                            REDUCE(R2);)
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Not(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2; //astatic void compiler warning
                        R3 = ~R1;
                        REDUCE(R3);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Or(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = R1 | R2;
                        REDUCE(R3);
                    )

    END_BATCH(BatchNumber);
}

static void InitKernel_sOr(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R1 |= R2;
                        REDUCE(R1);
                    )

    END_BATCH(BatchNumber);
}

static void InitKernel_pOr(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = R1 | Param2;
                        REDUCE(R2);
                    )

    END_BATCH(BatchNumber);
}

static void InitKernel_And(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = R1 & R2;
                        REDUCE(R3);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_sAnd(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R1 &= R2;
                        REDUCE(R1);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_pAnd(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = R1 & Param2;
                        REDUCE(R2);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Xor(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = R1 ^ R2;
                        REDUCE(R3);
                    )

    END_BATCH(BatchNumber);
}

static void InitKernel_sXor(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R1 ^= R2;
                        REDUCE(R1);
                    )

    END_BATCH(BatchNumber);
}

static void InitKernel_pXor(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = R1 ^ Param2;
                        REDUCE(R2);
                    )

    END_BATCH(BatchNumber);
}

static void InitKernel_Eq(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = (R1 == R2);
                        REDUCE(R3);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_pEq(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = (R1 == Param2);
                        REDUCE(R2);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Lt(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = (R1 < R2);
                        REDUCE(R3);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_pLt(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = (R1 < Param2);
                        REDUCE(R2);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Ult(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2; //avoid compiler warning
                        R3 = ULT(R1,R2);
                        REDUCE(R3);
                        )

    END_BATCH(BatchNumber);
}

/*
static void InitKernel_Ult2(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = INDEX;
                        R2 = 64; //avoid compiler warning
                        R3 = ULT(R1,R2);
                        REDUCE(R3);
                        )

    END_BATCH(BatchNumber);
}
*/

static void InitKernel_pUlt(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = ULT(R1,Param2);
                        REDUCE(R2);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Shl(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = (R1 << R2);
                        REDUCE(R3);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Shr(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = (R1 >> R2 );
                        REDUCE(R3);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Shra(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R2 = Param2;
                        R3 = SHRA(R1, R2);
                        REDUCE(R3);
                        )

    END_BATCH(BatchNumber);
}

static void InitKernel_Ishl(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R3 = (R1 << Param2);
                        REDUCE(R3);
                        )
    END_BATCH(BatchNumber);
}

static void InitKernel_Ishr(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R3 = (R1 >> Param2);
                        REDUCE(R3);
                      )

    END_BATCH(BatchNumber);
}

// pseudo-instruction for mov
static void InitKernel_pMov(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1 + Param2;
                        R2 = R1; // (R1 >> 0);
                        REDUCE(R2);
                      )

    END_BATCH(BatchNumber);
}

static void InitKernel_Ishra(int BatchNumber,INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = Param1;
                        R3 = ISHRA(R1, Param2);
                        REDUCE(R3);
                      )
    END_BATCH(BatchNumber);
}

static void InitKernel_Cellshl(int BatchNumber, INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = INDEX;
                        R2 = Param1;
                        CELL_SHL(R1,R2);
                        R3 = 0;
                        R5 = Param2;
                        NOP;
                        R4 = SHIFT_REG;
                        R6 = (R5 == R4);
                        NOP;
                      )
        EXECUTE_WHERE_EQ    (R3 = INDEX; )
        EXECUTE_IN_ALL      (REDUCE(R3););
    END_BATCH(BatchNumber);
}

/* Check if CELL_SHL is shift or rotation */
static void InitKernel_Cellshlrol(int BatchNumber, INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = INDEX;
                        R2 = Param1;
                        CELL_SHL(R1,R2);
                        R3 = Param2;// avoid compiler warning
                        for (int x=0;x < NUMBER_OF_MACHINES - 1;x++)
                            NOP;

                        R4 = SHIFT_REG;
                        REDUCE(R4);
                      )
    END_BATCH(BatchNumber);
}

static void InitKernel_Cellshr(int BatchNumber, INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R1 = INDEX;
                        R2 = Param1;
                        CELL_SHR(R1,R2);
                        R3 = 0;
                        R5 = Param2;
                        NOP;
                        R4 = SHIFT_REG;
                        R6 = (R5 == R4);
                        NOP;
                        )
        EXECUTE_WHERE_EQ( R3 = INDEX;)
        EXECUTE_IN_ALL( REDUCE(R3);)
    END_BATCH(BatchNumber);
}

static void InitKernel_Multlo(int BatchNumber, INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = Param1;
                        R1 = Param2;
                        MULT = R1*R0;
                        R2 = _LOW(MULT);
                        REDUCE(R2);
                    )
    END_BATCH(BatchNumber);
}

static void InitKernel_pMultlo(int BatchNumber, INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
       EXECUTE_IN_ALL(
                        R0 = Param1;
                        R1 = Param2;
                        R2 = R1*R0;
                        REDUCE(R2);
                    )
    END_BATCH(BatchNumber);
}

static void InitKernel_p2Multlo(int BatchNumber, INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = Param1;
                        R1 = R0 * Param2;
                        REDUCE(R1);
                    )
    END_BATCH(BatchNumber);
}

static void InitKernel_p2zMultlo(int BatchNumber, INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = (Param1 + Param2);
                        R1 = R0 * 0;
                        REDUCE(R1);
                    )
    END_BATCH(BatchNumber);
}

static void InitKernel_Multhi(int BatchNumber, INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = Param1;
                        R1 = Param2;
                        MULT = R1*R0;
                        R2 = _HIGH(MULT);
                        REDUCE(R2);
                        )
    END_BATCH(BatchNumber);
}

static void InitKernel_Whereq(int BatchNumber, INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = INDEX;
                        R1 = Param1;
                        R3 = (R0 == R1);
                        R4 = 0;
                    )
        EXECUTE_WHERE_EQ( R4 = (unsigned int)Param2;)
        EXECUTE_IN_ALL( REDUCE(R4); )
    END_BATCH(BatchNumber);
}

static void InitKernel_Wherelt(int BatchNumber, INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = INDEX;
                        R1 = Param1;
                        R3 = (R0 < R1);
                        R4 = 0;
                      )
        EXECUTE_WHERE_LT( R4 = Param2;)
        EXECUTE_IN_ALL( REDUCE(R4);)
    END_BATCH(BatchNumber);
}


static void InitKernel_Wherelt2(int BatchNumber, INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
        EXECUTE_IN_ALL(
                        R0 = INDEX;
                        R1 = Param1;
                        R2 = 0;
                        R0 = R0 - R1;
                        R3 = (R0 < R2);
                        R4 = 0;
                      )
        EXECUTE_WHERE_LT( R4 = Param2;)
        EXECUTE_IN_ALL( REDUCE(R4);)
    END_BATCH(BatchNumber);
}

static void InitKernel_Wherecry(int BatchNumber, INT32 Param1, INT32 Param2)
{
    BEGIN_BATCH(BatchNumber);
	   EXECUTE_IN_ALL(
                        R0 = INDEX;
                        R1 = Param1;
                        R3 = R0 + R1;
                        R4 = 0;
                    )
        EXECUTE_WHERE_CARRY( R4 = Param2; )
        EXECUTE_IN_ALL( REDUCE(R4);)
    END_BATCH(BatchNumber);
}

enum SimpleBatchNumbers
{
    NOP_BNR         ,
    IWRITE_BNR      ,
    IREAD_BNR       ,
    VLOAD_BNR       ,
    RED_BNR         ,
    MULT_BNR        ,
    CELL_SHL_BNR    ,
    CELL_SHR_BNR    ,
    CELL_SHLROL_BNR ,
    WRITE_BNR       ,
    WHERE_CARRY_BNR ,
    WHERE_EQ_BNR    ,
    WHERE_LT_BNR    ,
    WHERE_LT2_BNR   ,
    ENDWHERE_BNR    ,
    LDIX_BNR        ,
    READ_BNR        ,

    MULTLO_BNR      ,
    pMULTLO_BNR     ,
    p2MULTLO_BNR    ,
    p2zMULTLO_BNR   ,

    LDSH_BNR        ,
    MULTHI_BNR      ,
    SHL_BNR         ,
    ISHL_BNR        ,
    ISHL2_BNR        ,
    ISHL3_BNR        ,

    ADD_BNR         ,
    pADD_BNR        ,
    pzADD_BNR       ,
    sADD_BNR        ,

    EQ_BNR          ,
    pEQ_BNR         ,

    NOT_BNR         ,
    SHR_BNR         ,
    ISHR_BNR        ,
    pMOV_BNR        ,

    SUB_BNR         ,
    pSUB_BNR        ,
    sSUB_BNR        ,

    LT_BNR          ,
    pLT_BNR         ,

    OR_BNR          ,
    pOR_BNR         ,
    sOR_BNR         ,

    SHRA_BNR        ,
    ISHRA_BNR       ,
    PMOV_BNR        ,

    ADDC_BNR        ,
    pADDC_BNR       ,
    sADDC_BNR       ,

    ULT_BNR         ,
    pULT_BNR        ,

    AND_BNR         ,
    pAND_BNR        ,
    sAND_BNR        ,

    SUBC_BNR        ,
    pSUBC_BNR       ,

    CONDSUB_BNR     ,
    INC_BNR         ,
    INC2_BNR         ,
    INC3_BNR         ,

    XOR_BNR         ,
    pXOR_BNR        ,
    sXOR_BNR        ,

    IO_WRITE_BNR    ,
    IO_READ_BNR     ,

    IJMP_BNR        ,
    IJMP2_BNR       ,
    IJMP3_BNR       ,

    PRINT_LS_BNR = 98,
    CLEAR_LS_BNR = 99,
    MAX_BNR = NUMBER_OF_BATCHES
};

static TestFunction TestFunctionTable[] =
{

    {NOP_BNR,"NOP",InitKernel_Nop,{0x00,0x00,(NUMBER_OF_MACHINES-1)*NUMBER_OF_MACHINES/2}},
    {IWRITE_BNR,"IWRITE",InitKernel_Iwrite,{0x01,0x02,(NUMBER_OF_MACHINES-1)*NUMBER_OF_MACHINES/2}},
    {IREAD_BNR,"IREAD",InitKernel_Iread,{0x01,0x02,(NUMBER_OF_MACHINES-1)*NUMBER_OF_MACHINES/2}},

    {WRITE_BNR,"WRITE",InitKernel_Write,{0x01,0x02,(NUMBER_OF_MACHINES-1)*NUMBER_OF_MACHINES/2}},
    {READ_BNR,"READ",InitKernel_Read,{0x01,0x02,(NUMBER_OF_MACHINES-1)*NUMBER_OF_MACHINES/2}},

    {VLOAD_BNR,"VLOAD",InitKernel_Vload,{0x01,0x02,3*NUMBER_OF_MACHINES}},

    {ADD_BNR,"ADD",InitKernel_Add,{0xff,0xf1,(0xff + 0xf1)*NUMBER_OF_MACHINES}},
//    {pADD_BNR,"pADD",InitKernel_pAdd,{0xff,0xf1,(0xff + 0xf1)*NUMBER_OF_MACHINES}},
//    {pzADD_BNR,"pzADD",InitKernel_pzAdd,{0,0,(0 + 0)*NUMBER_OF_MACHINES}},
    {sADD_BNR,"sADD",InitKernel_sAdd,{0xff,0xf1,(0xff + 0xf1)*NUMBER_OF_MACHINES}},

    {ADDC_BNR,"ADDC",InitKernel_Addc,{0xf0,0x1,(0xf0 + 1 + 1)*NUMBER_OF_MACHINES}},
    {pADDC_BNR,"pADDC",InitKernel_pAddc,{0xf0,0x1,(0xf0 + 1 + 1)*NUMBER_OF_MACHINES}},

    {SUB_BNR,"SUB",InitKernel_Sub,{0xffff,0xff8f, (0xffff - 0xff8f)*NUMBER_OF_MACHINES}},
    {pSUB_BNR,"pSUB",InitKernel_pSub,{0xffff,0xff8f, (0xffff - 0xff8f)*NUMBER_OF_MACHINES}},
    {sSUB_BNR,"sSUB",InitKernel_sSub,{0xffff,0xff8f, (0xffff - 0xff8f)*NUMBER_OF_MACHINES}},

    {SUBC_BNR,"SUBC",InitKernel_Subc,{0xffff,0xff8f,(0xffff - 0xff8f -1)*NUMBER_OF_MACHINES}},
    {pSUBC_BNR,"pSUBC",InitKernel_pSubc,{0xffff,0xff8f,(0xffff - 0xff8f -1)*NUMBER_OF_MACHINES}},

    {CONDSUB_BNR,"CONDSUB",InitKernel_CondSub,{0xffff,0xff8f,(0xffff - 0xff8f)*NUMBER_OF_MACHINES}},
    {CONDSUB_BNR,"CONDSUB2",InitKernel_CondSub,{0xff8f,0xffff,0*NUMBER_OF_MACHINES}},

    //{INC_BNR,"INC",InitKernel_Inc,{0xf0,0x0,(0xf0 + 1)*NUMBER_OF_MACHINES}},
    //{INC2_BNR,"INC2",InitKernel_Inc,{0xf0,0x0,(0xf0 + 1)*NUMBER_OF_MACHINES}},
    //{INC3_BNR,"INC3",InitKernel_Inc,{0xf0,0x0,(0xf0 + 1)*NUMBER_OF_MACHINES}},

    {NOT_BNR,"NOT",InitKernel_Not,{0xfff0,0x00,(0xf)*NUMBER_OF_MACHINES}},

    {OR_BNR,"OR",InitKernel_Or,{0x10,0x01,(0x10 | 0x01)*NUMBER_OF_MACHINES}},
    {pOR_BNR,"pOR",InitKernel_pOr,{0x10,0x01,(0x10 | 0x01)*NUMBER_OF_MACHINES}},
    {sOR_BNR,"sOR",InitKernel_sOr,{0x10,0x01,(0x10 | 0x01)*NUMBER_OF_MACHINES}},

    {AND_BNR,"AND",InitKernel_And,{0xfffe,0x11,(0xfffe & 0x11)*NUMBER_OF_MACHINES}},
    {pAND_BNR,"pAND",InitKernel_pAnd,{0xfffe,0x11,(0xfffe & 0x11)*NUMBER_OF_MACHINES}},
    {sAND_BNR,"sAND",InitKernel_sAnd,{0xfffe,0x11,(0xfffe & 0x11)*NUMBER_OF_MACHINES}},

    {XOR_BNR,"XOR",InitKernel_Xor,{0x01,0x10,(0x01 ^ 0x10)*NUMBER_OF_MACHINES}},
    {pXOR_BNR,"pXOR",InitKernel_pXor,{0x01,0x10,(0x01 ^ 0x10)*NUMBER_OF_MACHINES}},
    {sXOR_BNR,"sXOR",InitKernel_sXor,{0x01,0x10,(0x01 ^ 0x10)*NUMBER_OF_MACHINES}},

    {EQ_BNR,"EQ",InitKernel_Eq,{0xff3f,0xff3f,(0xff3f == 0xff3f)*NUMBER_OF_MACHINES}},
    {pEQ_BNR,"pEQ",InitKernel_pEq,{0xff3f,0xff3f,(0xff3f == 0xff3f)*NUMBER_OF_MACHINES}},

    {LT_BNR,"LT",InitKernel_Lt,{0xfffd,0xfffe,(-3 < -2)*NUMBER_OF_MACHINES}},
    {pLT_BNR,"pLT",InitKernel_pLt,{0xfffd,0xfffe,(-3 < 2)*NUMBER_OF_MACHINES}},

    {ULT_BNR,"ULT",InitKernel_Ult,{0xabcd,0xabcc,(0xabcdUL < 0xabccUL)*NUMBER_OF_MACHINES}},

    {pULT_BNR,"pULT",InitKernel_pUlt,{0xabcd,0xabcc,(0xabcdUL < 0xabccUL)*NUMBER_OF_MACHINES}},

    {SHL_BNR,"SHL",InitKernel_Shl,{0xcd,3,((0xcd << 3)*NUMBER_OF_MACHINES)}},

    {SHR_BNR,"SHR",InitKernel_Shr,{0xabcd,3,((0xabcd >> 3)*NUMBER_OF_MACHINES)}},
    {SHRA_BNR,"SHRA",InitKernel_Shra,{0x01cd,4,(0x01c)*NUMBER_OF_MACHINES}},//will fail: 128*big

    {ISHL_BNR,"ISHL",InitKernel_Ishl,{0xabcd,4,((0xbcd0UL)*NUMBER_OF_MACHINES)}},

    {ISHR_BNR,"ISHR",InitKernel_Ishr,{0xabcd,4,((0x0abcUL)*NUMBER_OF_MACHINES)}},
    {ISHRA_BNR,"ISHRA",InitKernel_Ishra,{0xabcd,4,((0xfabcUL)*NUMBER_OF_MACHINES)}},
    {PMOV_BNR,"pMOV",InitKernel_pMov,{0xabcd,4,(0xabcd + 4)*NUMBER_OF_MACHINES}},

    {MULTLO_BNR,"MULTLO",InitKernel_Multlo,{0x2,0x3,(0x2UL * 0x3UL)*NUMBER_OF_MACHINES}},
    {pMULTLO_BNR,"pMULTLO",InitKernel_pMultlo,{0x2,0x3,(0x2UL * 0x3UL)*NUMBER_OF_MACHINES}},
    {p2MULTLO_BNR,"p2MULTLO",InitKernel_p2Multlo,{0x2,0x3,(0x2UL * 0x3UL)*NUMBER_OF_MACHINES}},
    {p2zMULTLO_BNR,"p2zMULTLO",InitKernel_p2zMultlo,{0x2,0x3,0}},



    {MULTHI_BNR,"MULTHI",InitKernel_Multhi,{0x8000,0x2,((0x8000UL * 0x2UL) >> 16)*NUMBER_OF_MACHINES}},

    {WHERE_EQ_BNR,"WHEREQ",InitKernel_Whereq,{27,50,50}},
    {WHERE_LT_BNR,"WHERELT",InitKernel_Wherelt,{27,50,27*50}},
    {WHERE_LT2_BNR,"WHERELT2",InitKernel_Wherelt2,{27,50,27*50}},
    {WHERE_CARRY_BNR,"WHERECRY",InitKernel_Wherecry,{(0x10000UL-10),50,118*50}},

    {IJMP_BNR,"IJMP",InitKernel_Jump,{(10), 2, (10-1 - 2)*NUMBER_OF_MACHINES}},
    {IJMP2_BNR,"IJMP2",InitKernel_Jump2,{(10), 2, (10-2)*NUMBER_OF_MACHINES}},
    //{IJMP3_BNR,"IJMP3",InitKernel_Jump3,{(10), 2, (10-2)*NUMBER_OF_MACHINES}},

	{CELL_SHL_BNR,"CELLSHL",InitKernel_Cellshl,{2,5,5-2}},
    {CELL_SHR_BNR,"CELLSHR",InitKernel_Cellshr,{2,5,5+2}},
    {CELL_SHLROL_BNR,"CELLSHLROL",InitKernel_Cellshlrol,{NUMBER_OF_MACHINES,0,(NUMBER_OF_MACHINES-1)*NUMBER_OF_MACHINES/2}},


	//{IO_WRITE_BNR,"IO_WRITE1",1,0,InitKernel_Iowrite,SumRedofFirstXnumbers(NUMBER_OF_MACHINES,0)},
    //{IO_WRITE_BNR,"IO_WRITE2",1024,1,InitKernel_Iowrite,SumRedofFirstXnumbers(NUMBER_OF_MACHINES,NUMBER_OF_MACHINES)},
    //{IO_WRITE_BNR,"IO_WRITE3",1024,1023,InitKernel_Iowrite,SumRedofFirstXnumbers(NUMBER_OF_MACHINES,NUMBER_OF_MACHINES*1023)},
    //{IO_READ_BNR,"IO_READ",1024,0,InitKernel_Ioread, NUMBER_OF_MACHINES},

};

static int getIndexTestFunctionTable(int BatchNumber)
{
    unsigned int i;
    for (i = 0; i < sizeof(TestFunctionTable)/sizeof(TestFunction); i++)
        if (TestFunctionTable[i].BatchNumber == BatchNumber)
            return i;
    return -1;
}

static void UpdateDatasetTable(int BatchNumber)
{
    int i = getIndexTestFunctionTable(BatchNumber);
    if (i>0)
    switch(BatchNumber)
    {
        case NOP_BNR        ://fallthrough
        case WRITE_BNR      ://fallthrough
        case READ_BNR       ://fallthrough
        case IWRITE_BNR     ://fallthrough
        case IREAD_BNR      :{
                                TestFunctionTable[i].ds.Param1 = randPar(2048);
                                TestFunctionTable[i].ds.ExpectedResult = (NUMBER_OF_MACHINES-1)*NUMBER_OF_MACHINES/2;
                                break;
                             }

        case RED_BNR        :break;
        case MULT_BNR       :break;
        case CELL_SHL_BNR   :break;
        case CELL_SHR_BNR   :break;

        case WHERE_CARRY_BNR:break;
        case WHERE_EQ_BNR   :break;
        case WHERE_LT_BNR   :break;
        case ENDWHERE_BNR   :break;


        case MULTLO_BNR     ://fallthrough
        case pMULTLO_BNR    ://fallthrough
        case p2MULTLO_BNR   :break;

        case LDSH_BNR       :break;
        case MULTHI_BNR     :break;
        case SHL_BNR        :break;
        case ISHL_BNR       :break;

        case VLOAD_BNR      ://fallthrough
        case ADD_BNR        ://fallthrough
        case pADD_BNR       ://fallthrough
        case sADD_BNR       ://fallthrough
        case pzADD_BNR      ://fallthrough
                {
                    do{
                            TestFunctionTable[i].ds.Param1 = randPar(0x10000);
                            TestFunctionTable[i].ds.Param2 = randPar(0x10000);
                      }
                    while (TestFunctionTable[i].ds.Param1 + TestFunctionTable[i].ds.Param2 >= 0x10000);
                    TestFunctionTable[i].ds.ExpectedResult =
                        (TestFunctionTable[i].ds.Param1 + TestFunctionTable[i].ds.Param2)*NUMBER_OF_MACHINES;
                    break;
                }

        case EQ_BNR         :
        case pEQ_BNR        :
                {
                     TestFunctionTable[i].ds.Param1 = randPar(0x10000);
                     TestFunctionTable[i].ds.Param2 = TestFunctionTable[i].ds.Param1;
                     TestFunctionTable[i].ds.ExpectedResult = NUMBER_OF_MACHINES;
                    break;
                }

        case NOT_BNR        :
                    {
                         TestFunctionTable[i].ds.Param1 = randPar(0x10000);
                         TestFunctionTable[i].ds.ExpectedResult = ((~TestFunctionTable[i].ds.Param1) & REGISTER_SIZE_MASK) * NUMBER_OF_MACHINES;
                         break;
                    }

        case SHR_BNR        :break;
        case ISHR_BNR       :break;

        case SUB_BNR        :
        case pSUB_BNR       :
        case sSUB_BNR       :break;
                            {
                                do{
                                        TestFunctionTable[i].ds.Param1 = randPar(0x10000);
                                        TestFunctionTable[i].ds.Param2 = randPar(0x10000);
                                  }
                                while (TestFunctionTable[i].ds.Param1 - TestFunctionTable[i].ds.Param2 <= -0x10000);
                                TestFunctionTable[i].ds.ExpectedResult =
                                    (TestFunctionTable[i].ds.Param1 - TestFunctionTable[i].ds.Param2)*NUMBER_OF_MACHINES;
                                break;
                            }

        case LT_BNR         :
        case pLT_BNR        :break;
                            {
                               TestFunctionTable[i].ds.Param1 = (randPar(0x10000) -32768);
                               TestFunctionTable[i].ds.Param2 = (randPar(0x10000) - 32768);
                               TestFunctionTable[i].ds.ExpectedResult =
                                    (TestFunctionTable[i].ds.Param1 < TestFunctionTable[i].ds.Param2) * NUMBER_OF_MACHINES;
                                break;
                            }



        case OR_BNR         :
        case pOR_BNR        :
        case sOR_BNR        :
                            {
                                 TestFunctionTable[i].ds.Param1 = randPar(0x10000);
                                 TestFunctionTable[i].ds.Param2 = randPar(0x10000);
                                 TestFunctionTable[i].ds.ExpectedResult =
                                    (TestFunctionTable[i].ds.Param1 | TestFunctionTable[i].ds.Param2) * NUMBER_OF_MACHINES;
                                break;
                            }

        case SHRA_BNR       :
        case ISHRA_BNR      :break;

        case ADDC_BNR       :
        case pADDC_BNR      :
        case sADDC_BNR      :
                            {
                                do{
                                        TestFunctionTable[i].ds.Param1 = randPar(0x10000);
                                        TestFunctionTable[i].ds.Param2 = randPar(0x10000);
                                  }
                                while (TestFunctionTable[i].ds.Param1 + TestFunctionTable[i].ds.Param2 >= 0x10000);
                                TestFunctionTable[i].ds.ExpectedResult =
                                    (TestFunctionTable[i].ds.Param1 + TestFunctionTable[i].ds.Param2 + 1)*NUMBER_OF_MACHINES;
                                break;
                            }

        case ULT_BNR        :
        case pULT_BNR       :
                            {
                               TestFunctionTable[i].ds.Param1 = randPar(0x10000);
                               TestFunctionTable[i].ds.Param2 = randPar(0x10000);
                               if (TestFunctionTable[i].ds.Param1 < TestFunctionTable[i].ds.Param2)
                                    TestFunctionTable[i].ds.ExpectedResult = NUMBER_OF_MACHINES;
                               else
                                    TestFunctionTable[i].ds.ExpectedResult = 0;
                               break;
                            }

        case AND_BNR        :
        case pAND_BNR       :
        case sAND_BNR       :
                            {
                                 TestFunctionTable[i].ds.Param1 = randPar(0x10000);
                                 TestFunctionTable[i].ds.Param2 = randPar(0x10000);
                                 TestFunctionTable[i].ds.ExpectedResult =
                                    (TestFunctionTable[i].ds.Param1 & TestFunctionTable[i].ds.Param2) * NUMBER_OF_MACHINES;
                                break;
                            }

        case SUBC_BNR       :
        case pSUBC_BNR      :break;

        case XOR_BNR        :
        case pXOR_BNR       :
        case sXOR_BNR       :
                            {
                                TestFunctionTable[i].ds.Param1 = randPar(0x10000);
                                TestFunctionTable[i].ds.Param2 = randPar(0x10000);
                                TestFunctionTable[i].ds.ExpectedResult =
                                (TestFunctionTable[i].ds.Param1 ^ TestFunctionTable[i].ds.Param2)*NUMBER_OF_MACHINES;
                                break;
                            }
    }
}
int test_ExtendedSimpleAll()
{
    int val;
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
        {
            InitKernel_Ishl(ISHL2_BNR, i, j);
            val = EXECUTE_BATCH_RED(ISHL2_BNR);
            if (val != (i << j)* NUMBER_OF_MACHINES)
                cout<<"ISHL2 failed: expected ("<<i<<" << "<<j<<") = NUMBER_OF_MACHINES* "<<(i<<j)<<" but received "<<val<<endl;
            else cout<<"ISHL2 PASS: ("<<i<<" << "<<j<<") = NUMBER_OF_MACHINES * "<<(i<<j)<<endl;
        }
	return 0;
}

static int TestJmpMultiRed(int RedValue, int SquareReds);
int test_Simple_All(bool stress)
{
    UINT16 i = 0;
    INT16 j = 0;
    UINT16 stressLoops;
    INT32 result;

    UINT16 testFails = 0;

    if (stress == true) { stressLoops = 10;} else stressLoops = 0;

    for (i = 0; i < sizeof (TestFunctionTable) / sizeof (TestFunction); i++)
    {
        j = stressLoops;
        do
        {
            TestFunctionTable[i].initKernel
                ( TestFunctionTable[i].BatchNumber,
                  TestFunctionTable[i].ds.Param1,
                  TestFunctionTable[i].ds.Param2);

            result = EXECUTE_BATCH_RED(TestFunctionTable[i].BatchNumber);

            if (result != TestFunctionTable[i].ds.ExpectedResult)
            {
               cout<< "Test "<< setw(8) << left << TestFunctionTable[i].OperationName <<" FAILED with result "
               <<result << " (expected " <<TestFunctionTable[i].ds.ExpectedResult<<" ) !" << " params are "
               << TestFunctionTable[i].ds.Param1 << " and " << TestFunctionTable[i].ds.Param2 <<endl;
               testFails++;

               DEASM_BATCH(TestFunctionTable[i].BatchNumber);

               if (j == 0) break;
               //return testFails;
            }
            else
            {
                if (j == stressLoops)
                    cout<< "Test "<< setw(10) << left << TestFunctionTable[i].OperationName;
                if ((j > 0) && (j <= stressLoops)) cout<<".";
                if (j == 0) {cout << " PASSED"<<endl;break;}
            }
            UpdateDatasetTable(TestFunctionTable[i].BatchNumber);
        }
        while (j-- >= 0);
    }
        cout<<"================================"<<endl;
    if (testFails ==0)
        cout<< "== All SimpleTests PASSED ======" <<endl;
    else
        cout<< "=="<< testFails << " SimpleTests FAILED ! " <<endl;
        cout<<"================================"<<endl<<endl;

    if (TestJmpMultiRed(2,1)==FAIL) testFails++;
    if (PASS != kernel_acc::storeKernel("database/TestJmpMultiRed_2_1.ker", IJMP3_BNR))
        cout<<"Could not store kernel "<<endl;

    if (TestJmpMultiRed(2,3)==FAIL) testFails++;
    if (PASS != kernel_acc::storeKernel("database/TestJmpMultiRed_2_3.ker", IJMP3_BNR))
        cout<<"Could not store kernel "<<endl;

    if (TestJmpMultiRed(2,13)==FAIL) testFails++;
    if (PASS != kernel_acc::storeKernel("database/TestJmpMultiRed_2_13.ker", IJMP3_BNR))
        cout<<"Could not store kernel "<<endl;

    if (TestJmpMultiRed(2,133)==FAIL) testFails++;
    if (PASS != kernel_acc::storeKernel("database/TestJmpMultiRed_2_133.ker", IJMP3_BNR))
        cout<<"Could not store kernel "<<endl;

    if (TestJmpMultiRed(2,1333)==FAIL) testFails++;
    if (PASS != kernel_acc::storeKernel("database/TestJmpMultiRed_2_1333.ker", IJMP3_BNR))
        cout<<"Could not store kernel "<<endl;

    return testFails;
}

static int TestJmpMultiRed(int RedValue, int SquareReds)
{
    InitKernel_Jump3(IJMP3_BNR, RedValue,SquareReds);
    EXECUTE_BATCH(IJMP3_BNR);
    int ExpectedBytesOfReductions = SquareReds*SquareReds*BYTES_IN_DWORD;
    static UINT_RED_REG_VAL *BasicMatchRedResults;
    BasicMatchRedResults = (UINT_RED_REG_VAL*)malloc(8192*1024 * sizeof(UINT_RED_REG_VAL));
    if (BasicMatchRedResults == NULL) {cout<<"Could not allocate memory for reductions "<<endl;return 0;};

    int RealBytesOfReductions = GET_MULTIRED_RESULT(BasicMatchRedResults ,
                                        ExpectedBytesOfReductions
                                        );
    int i;
    for (i=0; i < ExpectedBytesOfReductions / BYTES_IN_DWORD; i++)
    {
        if (BasicMatchRedResults[i] != RedValue* NUMBER_OF_MACHINES)
        {
            cout <<"  Unexpected red result "<<BasicMatchRedResults[i]<<endl;
            break;
        }
    }
    free(BasicMatchRedResults);
    if ((i == ExpectedBytesOfReductions/BYTES_IN_DWORD) && (RealBytesOfReductions == ExpectedBytesOfReductions))
        cout<<"Test JMP-MultiRed PASSED "<<endl;
    else
    {
        cout<<"Test JMP-MultiRed FAILED with args "<<RedValue<<" "<<SquareReds<<endl;
        if (RealBytesOfReductions != ExpectedBytesOfReductions)
            cout<<"Test JMP-MultiRed FAILED with different number of reductions "
            <<RealBytesOfReductions/BYTES_IN_DWORD<<" instead of "
            <<ExpectedBytesOfReductions/BYTES_IN_DWORD<<endl;
        else
            cout<<"Test JMP-MultiRed FAILED with different value of reduction"<<endl;
        DEASM_BATCH(IJMP3_BNR);
        cout << "Press ENTER to continue...";
        cin.ignore( numeric_limits <streamsize> ::max(), '\n' );
    }
}

