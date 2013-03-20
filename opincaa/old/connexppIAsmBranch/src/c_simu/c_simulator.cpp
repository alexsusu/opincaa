/*
 * File:   cnxvector.cpp
 *
 * OPINCAA's core implementation file.
 * Contains "cnxvector" class where the usual operators (+, -, [], ^, &,* ....) are overloaded.
 *
 * Unlike usual systems where operators perform the math operation, this class does
 *  instruction assembly according to spec ConnexISA.docx
 *
 *
 *
 *
 */

using namespace std;
#include <iostream>
#include <assert.h>

#include "../../include/core/io_unit.h"
#include "../../include/c_simu/c_simulator.h"

#include "../../include/core/cnxvector.h"
#include "../../include/core/opcodes.h"
#include "../../include/c_simu/c_simulator.h"

#ifndef _MSC_VER //MS C++ compiler
	DECLARE_STATIC_C_SIMU_VARS
#else
	UINT_REGVALUE c_simulator::CSimuRegs[NUMBER_OF_MACHINES][REGISTER_FILE_SIZE];
	UINT_REGVALUE c_simulator::CSimuLocalStore[NUMBER_OF_MACHINES][LOCAL_STORE_SIZE];
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

int c_simulator::deinitialize()
{
    return PASS;
};

int c_simulator::initialize()
{
    int RegNr;
    int LsCnt;
    FOR_ALL_MACHINES(
                     for(RegNr = 0; RegNr < REGISTER_FILE_SIZE; RegNr++) CSimuRegs[MACHINE][RegNr] = 0;
                     for(LsCnt = 0; LsCnt < LOCAL_STORE_SIZE; LsCnt++) CSimuLocalStore[MACHINE][LsCnt] = 0;
                     CSimuActiveFlags[MACHINE] = _ACTIVE;
                     CSimuCarryFlags[MACHINE] = 0;
                     CSimuEqFlags[MACHINE] = 0;
                     CSimuLtFlags[MACHINE] = 0;
                     CSimuShiftRegs[MACHINE] = 0;
					 CSimuMultRegs[MACHINE] = 0;
					 CSimuRotationMagnitude[MACHINE] = 0;
                );
    return PASS;
}

void c_simulator::printSHIFT_REGS()
{
    FOR_ALL_MACHINES( cout << " Machine "<<MACHINE<<": " << " SHIFT_REG = "<<  CSimuShiftRegs[MACHINE] << endl; );
}

void c_simulator::printLS(int address)
{
    FOR_ALL_MACHINES( cout << dec<<" Machine "<<MACHINE<<": " << " LS["<<address<<"]= "<<  CSimuLocalStore[MACHINE][address]
                     <<hex <<"(" << CSimuLocalStore[MACHINE][address] << ")" <<endl; );
}

void c_simulator::printLS(int address, int machine)
{
    cout <<dec<< " Machine "<<machine<<": " << " LS["<<address<<"]= "<<  CSimuLocalStore[machine][address]
                     <<hex <<"(" << CSimuLocalStore[machine][address] << ")" <<endl;
}

void c_simulator::printREG(int address)
{
    FOR_ALL_MACHINES( cout <<dec<< " Machine "<<MACHINE<<": " << " R["<<address<<"]= "<<CSimuRegs[MACHINE][address]
                            <<hex<<" (0x"<<CSimuRegs[MACHINE][address] <<")"<<endl; );
}

void c_simulator::printREG(int address, int machine)
{
    cout <<dec<< " Machine "<<machine<<": " << " R["<<address<<"]= "<<CSimuRegs[machine][address]
                            <<hex<<" (0x"<<CSimuRegs[machine][address] <<")"<<endl;
}

void c_simulator::printACTIVE()
{
    FOR_ALL_MACHINES( cout << " Machine "<<MACHINE<<": " << " A = "<< CSimuActiveFlags[MACHINE] << endl; );
}

/* opcodes */
struct OpCodeDeAsm
{
    UINT_INSTRUCTION opcode;
    const char* opcodeName;
};

static OpCodeDeAsm OpCodeDeAsms[] =
{
    {_ADD,"+"},
    {_ADDC," + CARRY + "},
    {_SUB,"-"},
    {_CONDSUB," CONDSUB "},
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
    UINT32 InstructionIndexMax = cnxvector::getInBatchCounter(dwBatchNumber);
    for (InstructionIndex = 0; InstructionIndex < InstructionIndexMax; InstructionIndex++)
                    if (FAIL == verifyBatchInstruction(cnxvector::getBatchInstruction(dwBatchNumber,InstructionIndex))) return FAIL;

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

void printSETLC(UINT_INSTRUCTION instr)  {   printf("SETLC (loop counter) to %d \n", GET_IMM(instr));}
void printIJMPNZ(UINT_INSTRUCTION instr) {   printf("IJMPNZ (backwards) %d slots \n", GET_IMM(instr));}


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
    UINT32 InstructionIndexMax = cnxvector::getInBatchCounter(dwBatchNumber);

    for (InstructionIndex = 0; InstructionIndex < InstructionIndexMax; InstructionIndex++)
    {
        CurrentInstruction = cnxvector::getBatchInstruction(dwBatchNumber,InstructionIndex);
        printf("%6ld: ",index);
        index++;

        switch (((CurrentInstruction) >> OPCODE_6BITS_POS) & ((1 << OPCODE_6BITS_SIZE)-1))
            {
                case _VLOAD: {printVLOAD(CurrentInstruction);continue;}
                case _IREAD: {printIREAD(CurrentInstruction);continue;}
                case _IWRITE: {printIWRITE(CurrentInstruction);continue;}

                case _SETLC:{printSETLC(CurrentInstruction);continue;}
                case _IJMPNZ:{printIJMPNZ(CurrentInstruction);continue;}
            }

        switch (((CurrentInstruction) >> OPCODE_9BITS_POS) & ((1 << OPCODE_9BITS_SIZE)-1))
            {
                case _ADD:
                case _ADDC:
//                case _INC:
                case _SUB:
                case _CONDSUB:
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

    //try to simulate same behaviour as in named pipes (see cnxvector::getMultiRedResult)
    // please note that this is not a FIFO like the cnxvector class has !
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
    UINT32 LocalLoop = 0;
    CSimuRedCnt = 0;

    UINT32 InstructionIndexMax = cnxvector::getInBatchCounter(dwBatchNumber);
    for (InstructionIndex = 0; InstructionIndex < InstructionIndexMax; InstructionIndex++)
    {
        CI = cnxvector::getBatchInstruction(dwBatchNumber,InstructionIndex);
        switch (((CI) >> OPCODE_6BITS_POS) & ((1 << OPCODE_6BITS_SIZE)-1))
            {
                case _VLOAD: {FOR_ALL_ACTIVE_MACHINES( CSimuRegs[MACHINE][GET_DEST(CI)] = GET_IMM(CI));continue;}
                case _IREAD: {FOR_ALL_ACTIVE_MACHINES( CSimuRegs[MACHINE][GET_DEST(CI)] = CSimuLocalStore[MACHINE][GET_IMM(CI)]);continue;}
                case _IWRITE: {FOR_ALL_ACTIVE_MACHINES( CSimuLocalStore[MACHINE][GET_IMM(CI)] = CSimuRegs[MACHINE][GET_LEFT(CI)]);continue;}

                case _SETLC: {  LocalLoop = GET_IMM(CI);continue;}
                case _IJMPNZ: {
                                if (LocalLoop > 0)
                                {
                                    InstructionIndex = InstructionIndex - GET_IMM(CI) -1;
                                    LocalLoop--;
                                }
                                continue;
                            }
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
            /*
                case _INC:{FOR_ALL_ACTIVE_MACHINES(
                                CSimuRegs[MACHINE][GET_DEST(CI)] = CSimuRegs[MACHINE][GET_LEFT(CI)] + 1;)
                                continue;
                                }
*/
                case _SUB:{FOR_ALL_ACTIVE_MACHINES(CSimuRegs[MACHINE][GET_DEST(CI)] =
                                                    CSimuRegs[MACHINE][GET_LEFT(CI)] - CSimuRegs[MACHINE][GET_RIGHT(CI)];

                                                if (CSimuRegs[MACHINE][GET_LEFT(CI)] < CSimuRegs[MACHINE][GET_RIGHT(CI)])
                                                     CSimuCarryFlags[MACHINE] = 1;
                                                else CSimuCarryFlags[MACHINE] = 0;

                                                   );
                                                    continue;}

                case _CONDSUB:{FOR_ALL_ACTIVE_MACHINES(
                                                       if (CSimuRegs[MACHINE][GET_LEFT(CI)] < CSimuRegs[MACHINE][GET_RIGHT(CI)])
                                                                                                    CSimuRegs[MACHINE][GET_DEST(CI)] = 0;
                                                       else CSimuRegs[MACHINE][GET_DEST(CI)] =
                                                                CSimuRegs[MACHINE][GET_LEFT(CI)] - CSimuRegs[MACHINE][GET_RIGHT(CI)];
                                                   );
                                                    continue;}

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
                        FOR_ALL_ACTIVE_MACHINES( if((INT16)CSimuRegs[MACHINE][GET_LEFT(CI)] < (INT16)CSimuRegs[MACHINE][GET_RIGHT(CI)])
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
                                                            UINT_REGVALUE man = 0;
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
                                                            UINT_REGVALUE man = 0;
                                                            rotation_existed = true;

                                                            if (MACHINE == NUMBER_OF_MACHINES -2) man = CSimuShiftRegs[NUMBER_OF_MACHINES-1];
                                                            CSimuShiftRegs[(NUMBER_OF_MACHINES - MACHINE)% NUMBER_OF_MACHINES] = CSimuShiftRegs[NUMBER_OF_MACHINES - MACHINE -1];
                                                            if (MACHINE == NUMBER_OF_MACHINES - 1) CSimuShiftRegs[0] = man;

                                                            CSimuRotationMagnitude[MACHINE]--;
                                                        });
                                    } while (rotation_existed == true);
                                    continue;}

                case _READ:  {FOR_ALL_ACTIVE_MACHINES(CSimuRegs[MACHINE][GET_DEST(CI)] = CSimuLocalStore[MACHINE][CSimuRegs[MACHINE][GET_RIGHT(CI)]]);continue;}
                case _WRITE: {FOR_ALL_ACTIVE_MACHINES(CSimuLocalStore[MACHINE][CSimuRegs[MACHINE][GET_RIGHT(CI)]] = CSimuRegs[MACHINE][GET_LEFT(CI)]);continue;}

                case _MULT:  {FOR_ALL_ACTIVE_MACHINES(CSimuMultRegs[MACHINE] = CSimuRegs[MACHINE][GET_RIGHT(CI)] * CSimuRegs[MACHINE][GET_LEFT(CI)]);continue;}
                case _MULT_HI:{FOR_ALL_ACTIVE_MACHINES(CSimuRegs[MACHINE][GET_DEST(CI)] = CSimuMultRegs[MACHINE] >> 16);continue;}
                case _MULT_LO:{FOR_ALL_ACTIVE_MACHINES(CSimuRegs[MACHINE][GET_DEST(CI)] = CSimuMultRegs[MACHINE] & 0xFFFF);continue;}

                case _WHERE_CRY:{FOR_ALL_MACHINES(if (CSimuCarryFlags[MACHINE]==1) CSimuActiveFlags[MACHINE]= _ACTIVE;else CSimuActiveFlags[MACHINE]=_INACTIVE);continue;}
                case _WHERE_EQ:{FOR_ALL_MACHINES(if (CSimuEqFlags[MACHINE]==1) CSimuActiveFlags[MACHINE]= _ACTIVE;else CSimuActiveFlags[MACHINE]=_INACTIVE);continue;}
                case _WHERE_LT:{FOR_ALL_MACHINES(if (CSimuLtFlags[MACHINE]==1) CSimuActiveFlags[MACHINE]= _ACTIVE;else CSimuActiveFlags[MACHINE]=_INACTIVE);continue;}

                //case _WHERE_CRY:{FOR_ALL_ACTIVE_MACHINES(if (CSimuCarryFlags[MACHINE]==1) CSimuActiveFlags[MACHINE]= _ACTIVE;else CSimuActiveFlags[MACHINE]=_INACTIVE);continue;}
                //case _WHERE_EQ:{FOR_ALL_ACTIVE_MACHINES(if (CSimuEqFlags[MACHINE]==1) CSimuActiveFlags[MACHINE]= _ACTIVE;else CSimuActiveFlags[MACHINE]=_INACTIVE);continue;}
                //case _WHERE_LT:{FOR_ALL_ACTIVE_MACHINES(if (CSimuLtFlags[MACHINE]==1) CSimuActiveFlags[MACHINE]= _ACTIVE;else CSimuActiveFlags[MACHINE]=_INACTIVE);continue;}

                case _END_WHERE:{FOR_ALL_MACHINES(CSimuActiveFlags[MACHINE]= _ACTIVE);continue;}
                case _NOP:      continue;
                case _REDUCE:   {UINT32 sum = 0; FOR_ALL_ACTIVE_MACHINES(sum += CSimuRegs[MACHINE][GET_LEFT(CI)]);
                                if (CSimuRedCnt < C_SIMU_RED_MAX) CSimuRed[CSimuRedCnt++] = sum & REDUCTION_SIZE_MASK; continue;}
                default: result= FAIL; break;
            }
    }
    return result;
}



int c_simulator::vwrite(void* Iou)
{
    int result = vwriteNonBlocking(Iou);
    vwriteWaitEnd();
    return result;
}

int c_simulator::vwriteNonBlocking(void* Iou)
{
    UINT32 cnxvectors;
    IO_UNIT_CORE* pIOUC = ((io_unit*)Iou)->getIO_UNIT_CORE();
    UINT32 LSaddress = (pIOUC->Descriptor).LsAddress;

    //cout << "Trying to write " << ((io_unit*)Iou)->getSize() << "bytes "<<endl;
    for (cnxvectors = 0; cnxvectors < pIOUC->Descriptor.NumOfCnxvectors + 1; cnxvectors++)
    {

        FOR_ALL_MACHINES(
                          if (MACHINE % 2 == 0)
                            CSimuLocalStore[MACHINE][LSaddress + cnxvectors] = pIOUC->Content[cnxvectors * CNXVECTOR_SIZE_IN_DWORDS + MACHINE/2] & REGISTER_SIZE_MASK;
                          else
                            CSimuLocalStore[MACHINE][LSaddress + cnxvectors] = pIOUC->Content[cnxvectors * CNXVECTOR_SIZE_IN_DWORDS + MACHINE/2] >> REGISTER_SIZE;
                        );
    }
    CSimulatorVWriteCounter++;
    return PASS;
}

int c_simulator::vwriteIsEnded()
{
    CSimulatorVWriteCounter--;//allow going under zero: simulate as if io_unit is read too many times
    assert(CSimulatorVWriteCounter >= 0);

    if (CSimulatorVWriteCounter == 0)
        return PASS;
    else return FAIL;
}

void c_simulator::vwriteWaitEnd()
{
    while (PASS != vwriteIsEnded())
    ;//wait
}

#define IO_MODE_POS             0
#define IO_LS_ADDRESS_POS       1
#define IO_NUM_OF_cnxvectorS_POS   2
#define IO_CONTENT_POS          3
/*
int c_simulator::vwrite(void* Iou)
{
    UINT32 cnxvectors;
    UINT32* pIOUC = (UINT32*)(((io_unit*)Iou)->getIO_UNIT_CORE());

    UINT32 IoMode = *(pIOUC + IO_MODE_POS);
    UINT32 LSaddress = *(pIOUC + IO_LS_ADDRESS_POS);
    UINT32 NumOfCnxvectors = *(pIOUC + IO_NUM_OF_cnxvectorS_POS);
    UINT32 *Content = pIOUC + IO_CONTENT_POS;

    for (cnxvectors = 0; cnxvectors < NumOfCnxvectors; cnxvectors++)
    {

        FOR_ALL_MACHINES(
                          if (MACHINE % 2 == 0)
                            CSimuLocalStore[MACHINE][LSaddress + cnxvectors] = *((UINT32*)(Content + cnxvectors * CNXVECTOR_SIZE_IN_DWORDS + MACHINE/2)) & REGISTER_SIZE_MASK;
                          else
                            CSimuLocalStore[MACHINE][LSaddress + cnxvectors] = *((UINT32*)(Content + cnxvectors * CNXVECTOR_SIZE_IN_DWORDS + MACHINE/2)) >> REGISTER_SIZE;
                        );
    }
    return PASS;
}
*/

int c_simulator::vread(void* Iou)
{
    UINT32 cnxvectors;
    IO_UNIT_CORE* pIOUC = ((io_unit*)Iou)->getIO_UNIT_CORE();
    UINT32 LSaddress = (pIOUC->Descriptor).LsAddress;
    assert(CSimulatorVWriteCounter ==0);
    for (cnxvectors = 0; cnxvectors < pIOUC->Descriptor.NumOfCnxvectors + 1; cnxvectors++)
    {
        int MACHINE;
        for (MACHINE = 0; MACHINE < NUMBER_OF_MACHINES; MACHINE+=2)
            pIOUC->Content[cnxvectors * CNXVECTOR_SIZE_IN_DWORDS + MACHINE/2] =
                (CSimuLocalStore[MACHINE + 1][LSaddress + cnxvectors] << REGISTER_SIZE) +
                                            CSimuLocalStore[MACHINE][LSaddress + cnxvectors];

    }
    return PASS;
}

