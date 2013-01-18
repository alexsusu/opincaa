
/*
 * File:   de_asm.h
 *
 * Contains methods for disassembling the assembled instructions.
 * This is very useful to see whether an instruction has the correct format.
 *
 */

#ifndef ALLOW_DE_ASM_CPP
    #error DO NOT compile de_asm.cpp file separately from c_simulator.cpp
#endif // ALLOW_DE_ASM_CPP

#include "../../include/core/vector.h"
#include "../../include/core/opcodes.h"
#include "../../include/c_simu/c_simulator.h"

#ifndef _MSC_VER //MS C++ compiler
	DECLARE_STATIC_C_SIMU_VARS
#else
	UINT_REGVALUE c_simulator::CSimuRegs[NUMBER_OF_MACHINES][32];
	UINT_REGVALUE c_simulator::CSimuLocalStore[NUMBER_OF_MACHINES][1024];
	UINT8 c_simulator::CSimuActiveFlags[NUMBER_OF_MACHINES];
	UINT8 c_simulator::CSimuCarryFlags[NUMBER_OF_MACHINES];
	UINT8 c_simulator::CSimuEqFlags[NUMBER_OF_MACHINES];
	UINT8 c_simulator::CSimuLtFlags[NUMBER_OF_MACHINES];
	UINT16 c_simulator::CSimuIndexRegs[NUMBER_OF_MACHINES];
	UINT_REGVALUE c_simulator::CSimuShiftRegs[NUMBER_OF_MACHINES];
	UINT_MULTVALUE c_simulator::CSimuMultRegs[NUMBER_OF_MACHINES];
	UINT16 c_simulator::CSimuRotationMagnitude[NUMBER_OF_MACHINES];
	UINT_RED_REG_VAL c_simulator::CSimuRed[C_SIMU_RED_MAX];
	UINT32 c_simulator::CSimuRedCnt;
	INT32 c_simulator::CSimulatorVWriteCounter;

#endif

/* opcodes */
struct OpCodeDeAsm
{
    UINT_INSTRUCTION opcode;
    const char* opcodeName;
};

OpCodeDeAsm OpCodeDeAsms[] =
{
    {_ADD,"+"},
    {_ADDC,"+ CARRY +"},
    {_SUB,"-"},
    {_SUBC,"- CARRY -"},
    {_NOT,"~"},
    {_OR,"|"},
    {_AND,"&"},
    {_XOR,"^"},
    {_EQ,"=="},
    {_LT,"<"},
    {_SHL,"<<"},
    {_SHR,">>"},
    {_SHRA,">>>"},
    {_ULT,"ULT"},
    {_ISHL,"<<"},
    {_ISHR,">>"},
    {_ISHRA,">>>"},
    {_LDIX,"LDIX"},
    {_LDSH,"LDSH"},
    {_CELL_SHL,"CELL_SHL"},
    {_CELL_SHR,"CELL_SHR"},
    {_READ,"READ"},
    {_WRITE,"WRITE"},
    {_MULT,"*"},
    {_MULT_LO,"_LO (MULT)"},
    {_MULT_HI,"_HI (MULT)"},
    {_VLOAD,"VLOAD"},
    {_IREAD,"IREAD"},
    {_IWRITE,"IWRITE"},
    {_WHERE_CRY,"SET_ACTIVE(WHERE_CARRY)"},
    {_WHERE_EQ,"SET_ACTIVE(WHERE_EQUAL)"},
    {_WHERE_LT,"SET_ACTIVE(WHERE_LESS_THAN)"},
    {_END_WHERE,"SET_ACTIVE(ALL)"},
    {_REDUCE,"reduce"},
    {_NOP,"nop"}
};

int c_simulator::verifyBatchInstruction(UINT_INSTRUCTION CurrentInstruction)
{
    UINT16 index;
    if  ( (GET_OPCODE_6BITS(CurrentInstruction) == _VLOAD) ||
          (GET_OPCODE_6BITS(CurrentInstruction) == _IREAD) ||
          (GET_OPCODE_6BITS(CurrentInstruction) == _IWRITE)
         )
        return PASS;

    for (index = 0; index < sizeof(OpCodeDeAsms) / sizeof (OpCodeDeAsm); index++)
        if (GET_OPCODE_9BITS(CurrentInstruction) == OpCodeDeAsms[index].opcode) return PASS;

    perror (" Instruction's opcode is not recognized. This might be a sort of ... crappy code ;)");
    return FAIL;
}

int c_simulator::verifyBatch(UINT16 dwBatchNumber)
{
    // verify opcodes
    UINT32 InstructionIndex = 0;
    UINT32 InstructionIndexMax = vector::getInBatchCounter(dwBatchNumber);
    for (InstructionIndex = 0; InstructionIndex < InstructionIndexMax; InstructionIndex++)
                    if (FAIL == verifyBatchInstruction(vector::getBatchInstruction(dwBatchNumber,InstructionIndex))) return FAIL;

    return PASS;
}

const char* getOpCodeName(UINT_INSTRUCTION instruction)
{
    UINT16 index;
    instruction = GET_OPCODE_9BITS(instruction);

    for (index = 0; index < sizeof(OpCodeDeAsms) / sizeof (OpCodeDeAsm); index++)
      if (instruction == OpCodeDeAsms[index].opcode) return OpCodeDeAsms[index].opcodeName;

    return "__UNKNOWN__";
}

void printVLOAD(UINT_INSTRUCTION instr) {    printf("R%u = %u (zis si 0x%x)\n", GET_DEST(instr), GET_IMM(instr), GET_IMM(instr));}
void printIREAD(UINT_INSTRUCTION instr) {    printf("R%d = LS [%d]\n", GET_DEST(instr), GET_IMM(instr));}
void printIWRITE(UINT_INSTRUCTION instr) {    printf("LS [%d] = R%d\n", GET_IMM(instr), GET_LEFT(instr));}

/* LRD = Left Right Destination
    P = paranthesis
    I = immediate
*/
void printLRD(UINT_INSTRUCTION instr){   printf("R%d = R%d %s R%d\n", GET_DEST(instr), GET_LEFT(instr), getOpCodeName(instr), GET_RIGHT(instr));}
void printLRDP(UINT_INSTRUCTION instr){   printf("R%d = (R%d %s R%d)\n", GET_DEST(instr), GET_LEFT(instr), getOpCodeName(instr), GET_RIGHT(instr));}
void printLRDI(UINT_INSTRUCTION instr){   printf("R%d = R%d %s %d\n", GET_DEST(instr), GET_LEFT(instr), getOpCodeName(instr), GET_RIGHT(instr));}

void printLDIX(UINT_INSTRUCTION instr){   printf("R%d = INDEX\n", GET_DEST(instr));}
void printLDSH(UINT_INSTRUCTION instr){   printf("R%d = SHIFT_REG\n", GET_DEST(instr));}

void printWN(UINT_INSTRUCTION instr){   printf("%s\n", getOpCodeName(instr));}
void printRED(UINT_INSTRUCTION instr){   printf("REDUCE(R%d)\n", GET_LEFT(instr));}

void printNOT(UINT_INSTRUCTION instr){   printf("R%d = %sR%d\n", GET_DEST(instr),getOpCodeName(instr),GET_LEFT(instr));}

void printCELL_SHLR(UINT_INSTRUCTION instr){   printf("SHIFT_REG = R%d then %s by R%d positions \n", GET_LEFT(instr),getOpCodeName(instr),GET_RIGHT(instr));}
void printREAD(UINT_INSTRUCTION instr){   printf("R%d = LS [R%d]\n", GET_DEST(instr),GET_RIGHT(instr));}
void printWRITE(UINT_INSTRUCTION instr){   printf("LS [R%d] = R%d\n", GET_RIGHT(instr),GET_LEFT(instr));}

void printMULT(UINT_INSTRUCTION instr){   printf("MULT = R%d %s R%d\n", GET_LEFT(instr), getOpCodeName(instr), GET_RIGHT(instr));}
void printMULTLH(UINT_INSTRUCTION instr){   printf("R%d = %s\n", GET_DEST(instr), getOpCodeName(instr));}

int c_simulator::printDeAsmBatch(UINT16 dwBatchNumber)
{
    // verify opcodes
    long int index = 0;
    int result = 0;
    UINT_INSTRUCTION CurrentInstruction;

    printf("De-asm batch number %d :\n", dwBatchNumber);

    UINT32 InstructionIndex = 0;
    UINT32 InstructionIndexMax = vector::getInBatchCounter(dwBatchNumber);

    for (InstructionIndex = 0; InstructionIndex < InstructionIndexMax; InstructionIndex++)
    {
        CurrentInstruction = vector::getBatchInstruction(dwBatchNumber,InstructionIndex);
        printf("%6ld: ",index);
        index++;

        switch (((CurrentInstruction) >> OPCODE_6BITS_POS) & ((1 << OPCODE_6BITS_SIZE)-1))
            {
                case _VLOAD: {printVLOAD(CurrentInstruction);continue;}
                case _IREAD: {printIREAD(CurrentInstruction);continue;}
                case _IWRITE: {printIWRITE(CurrentInstruction);continue;}
            }

        switch (((CurrentInstruction) >> OPCODE_9BITS_POS) & ((1 << OPCODE_9BITS_SIZE)-1))
            {
                case _ADD:
                case _ADDC:
                case _SUB:
                case _SUBC:
                case _OR:
                case _AND:
                case _XOR:
                case _LT:
                case _ULT:
                case _SHL:
                case _SHR:
                case _SHRA:
                        printLRD(CurrentInstruction);continue;

                case _EQ:  printLRDP(CurrentInstruction);continue;

                case _NOT: printNOT(CurrentInstruction);continue;

                case _ISHL:
                case _ISHR:
                case _ISHRA:
                        printLRDI(CurrentInstruction);continue;

                case _LDIX:printLDIX(CurrentInstruction);continue;
                case _LDSH:printLDSH(CurrentInstruction);continue;

                case _CELL_SHL:
                case _CELL_SHR: printCELL_SHLR(CurrentInstruction);continue;

                case _READ:  printREAD(CurrentInstruction);continue;
                case _WRITE: printWRITE(CurrentInstruction);continue;

                case _MULT:  printMULT(CurrentInstruction);continue;

                case _MULT_HI:
                case _MULT_LO: printMULTLH(CurrentInstruction);continue;

                case _WHERE_CRY:
                case _WHERE_EQ:
                case _WHERE_LT:
                case _END_WHERE:
                case _NOP:      printWN(CurrentInstruction);continue;
                case _REDUCE:   printRED(CurrentInstruction);continue;
                default: {
                            result= FAIL;
                            break;
                        }
            }
    }
    return result;
}

UINT32 c_simulator::getMultiRedResult(UINT_RED_REG_VAL* RedResults, UINT32 Limit)
{
    if (Limit >= CSimuRedCnt) Limit = CSimuRedCnt;
    for(UINT32 i = 0; i < Limit; i++) RedResults[i] = CSimuRed[i];

    //try to simulate same behaviour as in named pipes (see vector::getMultiRedResult)
    // please note that this is not a FIFO like the vector class has !
    CSimuRedCnt = CSimuRedCnt-Limit;
    return Limit*4;
}

UINT_RED_REG_VAL c_simulator::executeBatchOneReduce(UINT16 dwBatchNumber)
{
    c_simulator::DeAsmBatch(dwBatchNumber);
    return CSimuRed[0]; // return first result
}

UINT_RED_REG_VAL* c_simulator::executeBatchMultipleReduce(UINT16 dwBatchNumber)
{
    c_simulator::DeAsmBatch(dwBatchNumber);
    return CSimuRed;
}

int c_simulator::DeAsmBatch(UINT16 dwBatchNumber)
{
    // verify opcodes
    int result = 0;
    UINT_INSTRUCTION CI; //current instruction
    UINT32 InstructionIndex = 0;
    CSimuRedCnt = 0;

    UINT32 InstructionIndexMax = vector::getInBatchCounter(dwBatchNumber);
    for (InstructionIndex = 0; InstructionIndex < InstructionIndexMax; InstructionIndex++)
    {
        CI = vector::getBatchInstruction(dwBatchNumber,InstructionIndex);
        switch (((CI) >> OPCODE_6BITS_POS) & ((1 << OPCODE_6BITS_SIZE)-1))
            {
                case _VLOAD: {FOR_ALL_ACTIVE_MACHINES( CSimuRegs[MACHINE][GET_DEST(CI)] = GET_IMM(CI));continue;}
                case _IREAD: {FOR_ALL_ACTIVE_MACHINES( CSimuRegs[MACHINE][GET_DEST(CI)] = CSimuLocalStore[MACHINE][GET_IMM(CI)]);continue;}
                case _IWRITE: {FOR_ALL_ACTIVE_MACHINES( CSimuLocalStore[MACHINE][GET_IMM(CI)] = CSimuRegs[MACHINE][GET_LEFT(CI)]);continue;}
            }

        switch (((CI) >> OPCODE_9BITS_POS) & ((1 << OPCODE_9BITS_SIZE)-1))
            {
                case _ADD:{FOR_ALL_ACTIVE_MACHINES(
                                CSimuRegs[MACHINE][GET_DEST(CI)] = CSimuRegs[MACHINE][GET_LEFT(CI)] + CSimuRegs[MACHINE][GET_RIGHT(CI)];
                                if (CSimuRegs[MACHINE][GET_LEFT(CI)] + CSimuRegs[MACHINE][GET_RIGHT(CI)] > UINT_REGVALUE_TOP)
                                     CSimuCarryFlags[MACHINE] = 1;
                                else CSimuCarryFlags[MACHINE] = 0;
                                     );
                                continue;}

                case _ADDC:{FOR_ALL_ACTIVE_MACHINES(
                                CSimuRegs[MACHINE][GET_DEST(CI)] = CSimuRegs[MACHINE][GET_LEFT(CI)] +
                                                                        CSimuRegs[MACHINE][GET_RIGHT(CI)] +
                                                                            CSimuCarryFlags[MACHINE];
                                if (CSimuRegs[MACHINE][GET_LEFT(CI)] +
                                        CSimuRegs[MACHINE][GET_RIGHT(CI)] +
                                            CSimuCarryFlags[MACHINE] > UINT_REGVALUE_TOP)
                                     CSimuCarryFlags[MACHINE] = 1;
                                else CSimuCarryFlags[MACHINE] = 0;
                                     );
                                                   continue;}

                case _SUB:{FOR_ALL_ACTIVE_MACHINES(CSimuRegs[MACHINE][GET_DEST(CI)] =
                                                    CSimuRegs[MACHINE][GET_LEFT(CI)] - CSimuRegs[MACHINE][GET_RIGHT(CI)];

                                                if (CSimuRegs[MACHINE][GET_LEFT(CI)] < CSimuRegs[MACHINE][GET_RIGHT(CI)])
                                                     CSimuCarryFlags[MACHINE] = 1;
                                                else CSimuCarryFlags[MACHINE] = 0;

                                                   );
                                                    continue;}

                case _SUBC:{FOR_ALL_ACTIVE_MACHINES(CSimuRegs[MACHINE][GET_DEST(CI)] =
                                                    CSimuRegs[MACHINE][GET_LEFT(CI)] - CSimuRegs[MACHINE][GET_RIGHT(CI)]
                                                   - CSimuCarryFlags[MACHINE]); continue;
                                    if (CSimuRegs[MACHINE][GET_LEFT(CI)] <
                                            CSimuRegs[MACHINE][GET_RIGHT(CI)] + CSimuCarryFlags[MACHINE])
                                         CSimuCarryFlags[MACHINE] = 1;
                                    else CSimuCarryFlags[MACHINE] = 0;
                                                   }

                case _OR:{FOR_ALL_ACTIVE_MACHINES(CSimuRegs[MACHINE][GET_DEST(CI)] =
                                                    CSimuRegs[MACHINE][GET_LEFT(CI)] | CSimuRegs[MACHINE][GET_RIGHT(CI)]);
                                                   continue;}

                case _AND:{FOR_ALL_ACTIVE_MACHINES(CSimuRegs[MACHINE][GET_DEST(CI)] =
                                                    CSimuRegs[MACHINE][GET_LEFT(CI)] & CSimuRegs[MACHINE][GET_RIGHT(CI)]);
                                                   continue;}

                case _XOR:{FOR_ALL_ACTIVE_MACHINES(CSimuRegs[MACHINE][GET_DEST(CI)] =
                                                    CSimuRegs[MACHINE][GET_LEFT(CI)] ^ CSimuRegs[MACHINE][GET_RIGHT(CI)]);
                                                   continue;}
                case _LT:
                    {
                        FOR_ALL_ACTIVE_MACHINES( if(CSimuRegs[MACHINE][GET_LEFT(CI)] < CSimuRegs[MACHINE][GET_RIGHT(CI)])
                                                    { CSimuRegs[MACHINE][GET_DEST(CI)] = 1; CSimuLtFlags[MACHINE] = 1; }
                                                else { CSimuRegs[MACHINE][GET_DEST(CI)] = 0; CSimuLtFlags[MACHINE] = 0; };  );
                        continue;}
                case _ULT:
                    {
                        FOR_ALL_ACTIVE_MACHINES( if(CSimuRegs[MACHINE][GET_LEFT(CI)] < CSimuRegs[MACHINE][GET_RIGHT(CI)])
                                                    { CSimuRegs[MACHINE][GET_DEST(CI)] = 1; CSimuLtFlags[MACHINE] = 1; }
                                                else { CSimuRegs[MACHINE][GET_DEST(CI)] = 0; CSimuLtFlags[MACHINE] = 0; };  );
                        continue;}
                case _SHL:
                    {
                        FOR_ALL_ACTIVE_MACHINES(CSimuRegs[MACHINE][GET_DEST(CI)] =
                                                    CSimuRegs[MACHINE][GET_LEFT(CI)] << CSimuRegs[MACHINE][GET_RIGHT(CI)]);
                        continue;
                    }
                case _SHR:
                    {
                        FOR_ALL_ACTIVE_MACHINES(CSimuRegs[MACHINE][GET_DEST(CI)] =
                                                    CSimuRegs[MACHINE][GET_LEFT(CI)] >> CSimuRegs[MACHINE][GET_RIGHT(CI)]);
                        continue;
                    }
                case _SHRA:
                    {
                        FOR_ALL_ACTIVE_MACHINES(CSimuRegs[MACHINE][GET_DEST(CI)] =
                                                    (INT_REGVALUE)(CSimuRegs[MACHINE][GET_LEFT(CI)]) >> (CSimuRegs[MACHINE][GET_RIGHT(CI)]));
                        continue;
                    }

                case _EQ:
                    {
                        FOR_ALL_ACTIVE_MACHINES( if(CSimuRegs[MACHINE][GET_LEFT(CI)] == CSimuRegs[MACHINE][GET_RIGHT(CI)])
                                                    { CSimuRegs[MACHINE][GET_DEST(CI)] = 1; CSimuEqFlags[MACHINE] = 1; }
                                                else { CSimuRegs[MACHINE][GET_DEST(CI)] = 0; CSimuEqFlags[MACHINE] = 0; });
                        continue;}

                case _NOT: {FOR_ALL_ACTIVE_MACHINES(CSimuRegs[MACHINE][GET_DEST(CI)] = ~CSimuRegs[MACHINE][GET_LEFT(CI)]); continue;}
                case _ISHL:
                    {
                        FOR_ALL_ACTIVE_MACHINES(CSimuRegs[MACHINE][GET_DEST(CI)] =
                                                    CSimuRegs[MACHINE][GET_LEFT(CI)] << GET_RIGHT(CI));
                        continue;
                    }
                case _ISHR:
                    {
                        FOR_ALL_ACTIVE_MACHINES(CSimuRegs[MACHINE][GET_DEST(CI)] =
                                                    CSimuRegs[MACHINE][GET_LEFT(CI)] >> GET_RIGHT(CI));
                        continue;
                    }
                case _ISHRA:
                    {
                        FOR_ALL_ACTIVE_MACHINES(CSimuRegs[MACHINE][GET_DEST(CI)] =
                                                    ((INT_REGVALUE)CSimuRegs[MACHINE][GET_LEFT(CI)]) >> GET_RIGHT(CI));
                        continue;
                    }
                case _LDIX:{FOR_ALL_ACTIVE_MACHINES(CSimuRegs[MACHINE][GET_DEST(CI)] = MACHINE);continue;}
                case _LDSH:{FOR_ALL_ACTIVE_MACHINES(CSimuRegs[MACHINE][GET_DEST(CI)] = CSimuShiftRegs[MACHINE]);continue;}

//void printCELL_SHLR(UINT_INSTRUCTION instr){   printf("SHIFT_REG = R%d then %s by R%d positions \n", GET_LEFT(instr),getOpCodeName(instr),GET_RIGHT(instr));}
// any contraints regarding ACTIVE ?
// any constraints regarding

                case _CELL_SHL: {
                                    //step 1: load shift reg with R[LEFT].
                                    bool rotation_existed;
                                    {FOR_ALL_MACHINES( CSimuShiftRegs[MACHINE] = CSimuRegs[MACHINE][GET_LEFT(CI)] % NUMBER_OF_MACHINES;);}

                                    //step 2: copy number of positions to rotate
                                    {FOR_ALL_MACHINES( CSimuRotationMagnitude[MACHINE] = CSimuRegs[MACHINE][GET_RIGHT(CI)];);}

                                    //step 3: rotate one position at a time
                                    do
                                    {
                                        rotation_existed = false;
                                        FOR_ALL_MACHINES(
                                                        if ( CSimuRotationMagnitude[MACHINE] > 0)
                                                        {
                                                            INT_REGVALUE man;
                                                            rotation_existed = true;

                                                            if (MACHINE == 0) man = CSimuShiftRegs[NUMBER_OF_MACHINES -1];
                                                            CSimuShiftRegs[(MACHINE + NUMBER_OF_MACHINES -1)% NUMBER_OF_MACHINES] = CSimuShiftRegs[MACHINE];
                                                            if (MACHINE == NUMBER_OF_MACHINES - 1) CSimuShiftRegs[NUMBER_OF_MACHINES - 2] = man;

                                                            CSimuRotationMagnitude[MACHINE]--;
                                                        });
                                    } while (rotation_existed == true);
                                    continue;}

                case _CELL_SHR: {
                                    //step 1: load shift reg with R[LEFT].
                                    bool rotation_existed;
                                    {FOR_ALL_MACHINES( CSimuShiftRegs[MACHINE] = CSimuRegs[MACHINE][GET_LEFT(CI)];);}

                                    //step 2: copy number of positions to rotate
                                    {FOR_ALL_MACHINES( CSimuRotationMagnitude[MACHINE] = CSimuRegs[MACHINE][GET_RIGHT(CI)] % NUMBER_OF_MACHINES;);}

                                    //step 3: rotate one position at a time
                                    do
                                    {
                                        rotation_existed = false;
                                        FOR_ALL_MACHINES(
                                                        if ( CSimuRotationMagnitude[MACHINE] > 0)
                                                        {
                                                            INT_REGVALUE man;
                                                            rotation_existed = true;

                                                            if (MACHINE == NUMBER_OF_MACHINES -2) man = CSimuShiftRegs[NUMBER_OF_MACHINES-1];
                                                            CSimuShiftRegs[(NUMBER_OF_MACHINES - MACHINE)% NUMBER_OF_MACHINES] = CSimuShiftRegs[NUMBER_OF_MACHINES - MACHINE -1];
                                                            if (MACHINE == NUMBER_OF_MACHINES - 1) CSimuShiftRegs[0] = man;

                                                            CSimuRotationMagnitude[MACHINE]--;
                                                        });
                                    } while (rotation_existed == true);
                                    continue;}

                case _READ:  {FOR_ALL_ACTIVE_MACHINES(CSimuRegs[MACHINE][GET_DEST(CI)] = CSimuLocalStore[MACHINE][GET_RIGHT(CI)]);continue;}
                case _WRITE: {FOR_ALL_ACTIVE_MACHINES(CSimuLocalStore[MACHINE][GET_RIGHT(CI)] = CSimuRegs[MACHINE][GET_LEFT(CI)]);continue;}

                case _MULT:  {FOR_ALL_ACTIVE_MACHINES(CSimuMultRegs[MACHINE] = CSimuRegs[MACHINE][GET_RIGHT(CI)] * CSimuRegs[MACHINE][GET_LEFT(CI)]);continue;}
                case _MULT_HI:{FOR_ALL_ACTIVE_MACHINES(CSimuRegs[MACHINE][GET_DEST(CI)] = CSimuMultRegs[MACHINE] >> 16);continue;}
                case _MULT_LO:{FOR_ALL_ACTIVE_MACHINES(CSimuRegs[MACHINE][GET_DEST(CI)] = CSimuMultRegs[MACHINE] & 0xFFFF);continue;}

                case _WHERE_CRY:{FOR_ALL_MACHINES(if (CSimuCarryFlags[MACHINE]==1) CSimuActiveFlags[MACHINE]= _ACTIVE;else CSimuActiveFlags[MACHINE]=_INACTIVE);continue;}
                case _WHERE_EQ:{FOR_ALL_MACHINES(if (CSimuEqFlags[MACHINE]==1) CSimuActiveFlags[MACHINE]= _ACTIVE;else CSimuActiveFlags[MACHINE]=_INACTIVE);continue;}
                case _WHERE_LT:{FOR_ALL_MACHINES(if (CSimuLtFlags[MACHINE]==1) CSimuActiveFlags[MACHINE]= _ACTIVE;else CSimuActiveFlags[MACHINE]=_INACTIVE);continue;}
                case _END_WHERE:{FOR_ALL_MACHINES(CSimuActiveFlags[MACHINE]= _ACTIVE);continue;}
                case _NOP:      continue;
                case _REDUCE:   {UINT32 sum = 0; FOR_ALL_ACTIVE_MACHINES(sum += CSimuRegs[MACHINE][GET_LEFT(CI)]);
                                if (CSimuRedCnt < C_SIMU_RED_MAX) CSimuRed[CSimuRedCnt++] = sum & REDUCTION_SIZE_MASK; continue;}
                default: result= FAIL; break;
            }
    }
    return result;
}

