
/*
 * File:   de_asm.h
 *
 * Contains methods for disassembling the assembled instructions.
 * This is very useful to see whether an instruction has the correct format.
 *
 */

#include "../../include/core/vector.h"
#include "../../include/core/opcodes.h"
#include "../../include/c_simu/c_simulator.h"

DECLARE_STATIC_C_SIMU_VARS;
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

int c_simulator::verifyKernelInstruction(UINT_INSTRUCTION CurrentInstruction)
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

int c_simulator::verifyKernel(UINT16 dwBatchNumber)
{
    // verify opcodes
    UINT_INSTRUCTION* pCurrentInstruction;
    for (pCurrentInstruction = &vector::dwBatch[dwBatchNumber][0];
            pCurrentInstruction < (vector::dwBatch[dwBatchNumber] + vector::dwInBatchCounter[dwBatchNumber]);
                pCurrentInstruction++)
                    if (FAIL == verifyKernelInstruction(*pCurrentInstruction)) return FAIL;

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

int c_simulator::printDeasmKernel(UINT16 dwBatchNumber)
{
    // verify opcodes
    int index = 0;
    int result = 0;
    UINT_INSTRUCTION* CurrentInstruction;

    printf("De-asm batch number %d :\n", dwBatchNumber);
    for (CurrentInstruction = &vector::dwBatch[dwBatchNumber][0];
            CurrentInstruction < (vector::dwBatch[dwBatchNumber] + vector::dwInBatchCounter[dwBatchNumber]);
                CurrentInstruction++)
    {
        printf("%5d: ",index);
        index++;

        switch (((*CurrentInstruction) >> OPCODE_6BITS_POS) & ((1 << OPCODE_6BITS_SIZE)-1))
            {
                case _VLOAD: {printVLOAD(*CurrentInstruction);continue;}
                case _IREAD: {printIREAD(*CurrentInstruction);continue;}
                case _IWRITE: {printIWRITE(*CurrentInstruction);continue;}
            }

        switch (((*CurrentInstruction) >> OPCODE_9BITS_POS) & ((1 << OPCODE_9BITS_SIZE)-1))
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
                        printLRD(*CurrentInstruction);continue;

                case _EQ:  printLRDP(*CurrentInstruction);continue;

                case _NOT: printNOT(*CurrentInstruction);continue;

                case _ISHL:
                case _ISHR:
                case _ISHRA:
                        printLRDI(*CurrentInstruction);continue;

                case _LDIX:printLDIX(*CurrentInstruction);continue;
                case _LDSH:printLDSH(*CurrentInstruction);continue;

                case _CELL_SHL:
                case _CELL_SHR: printCELL_SHLR(*CurrentInstruction);continue;

                case _READ:  printREAD(*CurrentInstruction);continue;
                case _WRITE: printWRITE(*CurrentInstruction);continue;

                case _MULT:  printMULT(*CurrentInstruction);continue;

                case _MULT_HI:
                case _MULT_LO: printMULTLH(*CurrentInstruction);continue;

                case _WHERE_CRY:
                case _WHERE_EQ:
                case _WHERE_LT:
                case _END_WHERE:
                case _NOP:      printWN(*CurrentInstruction);continue;
                case _REDUCE:   printRED(*CurrentInstruction);continue;
                default: result= FAIL; break;
            }
    }
    return result;
}

int c_simulator::executeDeasmKernel(UINT16 dwBatchNumber)
{
    // verify opcodes
    int index = 0;
    int result = 0;
    UINT_INSTRUCTION* CI; //current instruction
    for (CI = &vector::dwBatch[dwBatchNumber][0];
            CI < (vector::dwBatch[dwBatchNumber] + vector::dwInBatchCounter[dwBatchNumber]);
                CI++)
    {
        //printf("Executing %5d: ",index);
        index++;

        switch (((*CI) >> OPCODE_6BITS_POS) & ((1 << OPCODE_6BITS_SIZE)-1))
            {
                case _VLOAD: {FOR_ALL_ACTIVE_MACHINES( C_SIMU_REGS[MACHINE][GET_DEST(*CI)] = GET_IMM(*CI));continue;}
                case _IREAD: {FOR_ALL_ACTIVE_MACHINES( C_SIMU_REGS[MACHINE][GET_DEST(*CI)] = C_SIMU_LS[MACHINE][GET_IMM(*CI)]);continue;}
                case _IWRITE: {FOR_ALL_ACTIVE_MACHINES( C_SIMU_LS[MACHINE][GET_IMM(*CI)] = C_SIMU_REGS[MACHINE][GET_LEFT(*CI)]);continue;}
            }

        switch (((*CI) >> OPCODE_9BITS_POS) & ((1 << OPCODE_9BITS_SIZE)-1))
            {
                case _ADD:{FOR_ALL_ACTIVE_MACHINES(
                                C_SIMU_REGS[MACHINE][GET_DEST(*CI)] = C_SIMU_REGS[MACHINE][GET_LEFT(*CI)] + C_SIMU_REGS[MACHINE][GET_RIGHT(*CI)];
                                if (C_SIMU_REGS[MACHINE][GET_LEFT(*CI)] + C_SIMU_REGS[MACHINE][GET_RIGHT(*CI)] > UINT_REGVALUE_TOP)
                                     C_SIMU_CARRY[MACHINE] = 1;
                                else C_SIMU_CARRY[MACHINE] = 0;
                                     );
                                continue;}

                case _ADDC:{FOR_ALL_ACTIVE_MACHINES(
                                C_SIMU_REGS[MACHINE][GET_DEST(*CI)] = C_SIMU_REGS[MACHINE][GET_LEFT(*CI)] +
                                                                        C_SIMU_REGS[MACHINE][GET_RIGHT(*CI)] +
                                                                            C_SIMU_CARRY[MACHINE];
                                if (C_SIMU_REGS[MACHINE][GET_LEFT(*CI)] +
                                        C_SIMU_REGS[MACHINE][GET_RIGHT(*CI)] +
                                            C_SIMU_CARRY[MACHINE] > UINT_REGVALUE_TOP)
                                     C_SIMU_CARRY[MACHINE] = 1;
                                else C_SIMU_CARRY[MACHINE] = 0;
                                     );
                                                   continue;}

                case _SUB:{FOR_ALL_ACTIVE_MACHINES(C_SIMU_REGS[MACHINE][GET_DEST(*CI)] =
                                                    C_SIMU_REGS[MACHINE][GET_LEFT(*CI)] - C_SIMU_REGS[MACHINE][GET_RIGHT(*CI)];

                                                if (C_SIMU_REGS[MACHINE][GET_LEFT(*CI)] < C_SIMU_REGS[MACHINE][GET_RIGHT(*CI)])
                                                     C_SIMU_CARRY[MACHINE] = 1;
                                                else C_SIMU_CARRY[MACHINE] = 0;

                                                   );
                                                    continue;}

                case _SUBC:{FOR_ALL_ACTIVE_MACHINES(C_SIMU_REGS[MACHINE][GET_DEST(*CI)] =
                                                    C_SIMU_REGS[MACHINE][GET_LEFT(*CI)] - C_SIMU_REGS[MACHINE][GET_RIGHT(*CI)]
                                                   - C_SIMU_CARRY[MACHINE]); continue;
                                    if (C_SIMU_REGS[MACHINE][GET_LEFT(*CI)] <
                                            C_SIMU_REGS[MACHINE][GET_RIGHT(*CI)] + C_SIMU_CARRY[MACHINE])
                                         C_SIMU_CARRY[MACHINE] = 1;
                                    else C_SIMU_CARRY[MACHINE] = 0;
                                                   }

                case _OR:{FOR_ALL_ACTIVE_MACHINES(C_SIMU_REGS[MACHINE][GET_DEST(*CI)] =
                                                    C_SIMU_REGS[MACHINE][GET_LEFT(*CI)] | C_SIMU_REGS[MACHINE][GET_RIGHT(*CI)]);
                                                   continue;}

                case _AND:{FOR_ALL_ACTIVE_MACHINES(C_SIMU_REGS[MACHINE][GET_DEST(*CI)] =
                                                    C_SIMU_REGS[MACHINE][GET_LEFT(*CI)] & C_SIMU_REGS[MACHINE][GET_RIGHT(*CI)]);
                                                   continue;}

                case _XOR:{FOR_ALL_ACTIVE_MACHINES(C_SIMU_REGS[MACHINE][GET_DEST(*CI)] =
                                                    C_SIMU_REGS[MACHINE][GET_LEFT(*CI)] ^ C_SIMU_REGS[MACHINE][GET_RIGHT(*CI)]);
                                                   continue;}
                case _LT:
                    {
                        FOR_ALL_ACTIVE_MACHINES( if(C_SIMU_REGS[MACHINE][GET_LEFT(*CI)] < C_SIMU_REGS[MACHINE][GET_RIGHT(*CI)])
                                                    { C_SIMU_REGS[MACHINE][GET_DEST(*CI)] = 1; C_SIMU_LT[MACHINE] = 1; }
                                                else { C_SIMU_REGS[MACHINE][GET_DEST(*CI)] = 0; C_SIMU_LT[MACHINE] = 0; };  );
                        continue;}
                case _ULT:
                    {
                        FOR_ALL_ACTIVE_MACHINES( if(C_SIMU_REGS[MACHINE][GET_LEFT(*CI)] < C_SIMU_REGS[MACHINE][GET_RIGHT(*CI)])
                                                    { C_SIMU_REGS[MACHINE][GET_DEST(*CI)] = 1; C_SIMU_LT[MACHINE] = 1; }
                                                else { C_SIMU_REGS[MACHINE][GET_DEST(*CI)] = 0; C_SIMU_LT[MACHINE] = 0; };  );
                        continue;}
                case _SHL:
                    {
                        FOR_ALL_ACTIVE_MACHINES(C_SIMU_REGS[MACHINE][GET_DEST(*CI)] =
                                                    C_SIMU_REGS[MACHINE][GET_LEFT(*CI)] << C_SIMU_REGS[MACHINE][GET_RIGHT(*CI)]);
                        continue;
                    }
                case _SHR:
                    {
                        FOR_ALL_ACTIVE_MACHINES(C_SIMU_REGS[MACHINE][GET_DEST(*CI)] =
                                                    C_SIMU_REGS[MACHINE][GET_LEFT(*CI)] >> C_SIMU_REGS[MACHINE][GET_RIGHT(*CI)]);
                        continue;
                    }
                case _SHRA:
                    {
                        FOR_ALL_ACTIVE_MACHINES(C_SIMU_REGS[MACHINE][GET_DEST(*CI)] =
                                                    (INT_REGVALUE)(C_SIMU_REGS[MACHINE][GET_LEFT(*CI)]) >> (C_SIMU_REGS[MACHINE][GET_RIGHT(*CI)]));
                        continue;
                    }

                case _EQ:
                    {
                        FOR_ALL_ACTIVE_MACHINES( if(C_SIMU_REGS[MACHINE][GET_LEFT(*CI)] == C_SIMU_REGS[MACHINE][GET_RIGHT(*CI)])
                                                    { C_SIMU_REGS[MACHINE][GET_DEST(*CI)] = 1; C_SIMU_EQ[MACHINE] = 1; }
                                                else { C_SIMU_REGS[MACHINE][GET_DEST(*CI)] = 0; C_SIMU_EQ[MACHINE] = 0; });
                        continue;}

                case _NOT: {FOR_ALL_ACTIVE_MACHINES(C_SIMU_REGS[MACHINE][GET_DEST(*CI)] = ~C_SIMU_REGS[MACHINE][GET_LEFT(*CI)]); continue;}
                case _ISHL:
                    {
                        FOR_ALL_ACTIVE_MACHINES(C_SIMU_REGS[MACHINE][GET_DEST(*CI)] =
                                                    C_SIMU_REGS[MACHINE][GET_LEFT(*CI)] << GET_RIGHT(*CI));
                        continue;
                    }
                case _ISHR:
                    {
                        FOR_ALL_ACTIVE_MACHINES(C_SIMU_REGS[MACHINE][GET_DEST(*CI)] =
                                                    C_SIMU_REGS[MACHINE][GET_LEFT(*CI)] >> GET_RIGHT(*CI));
                        continue;
                    }
                case _ISHRA:
                    {
                        FOR_ALL_ACTIVE_MACHINES(C_SIMU_REGS[MACHINE][GET_DEST(*CI)] =
                                                    ((INT_REGVALUE)C_SIMU_REGS[MACHINE][GET_LEFT(*CI)]) >> GET_RIGHT(*CI));
                        continue;
                    }
                case _LDIX:{FOR_ALL_ACTIVE_MACHINES(C_SIMU_REGS[MACHINE][GET_DEST(*CI)] = MACHINE);continue;}
                case _LDSH:{FOR_ALL_ACTIVE_MACHINES(C_SIMU_REGS[MACHINE][GET_DEST(*CI)] = C_SIMU_SH[MACHINE]);continue;}

//void printCELL_SHLR(UINT_INSTRUCTION instr){   printf("SHIFT_REG = R%d then %s by R%d positions \n", GET_LEFT(instr),getOpCodeName(instr),GET_RIGHT(instr));}
// any contraints regarding ACTIVE ?
// any constraints regarding

                case _CELL_SHL: {
                                    //step 1: load shift reg with R[LEFT].
                                    bool rotation_existed;
                                    {FOR_ALL_MACHINES( C_SIMU_SH[MACHINE] = C_SIMU_REGS[MACHINE][GET_LEFT(*CI)] % NUMBER_OF_MACHINES;);}

                                    //step 2: copy number of positions to rotate
                                    {FOR_ALL_MACHINES( C_SIMU_ROTATION_MAGNITUDE[MACHINE] = C_SIMU_REGS[MACHINE][GET_RIGHT(*CI)];);}

                                    //step 3: rotate one position at a time
                                    do
                                    {
                                        rotation_existed = false;
                                        FOR_ALL_MACHINES(
                                                        if ( C_SIMU_ROTATION_MAGNITUDE[MACHINE] > 0)
                                                        {
                                                            INT_REGVALUE man;
                                                            rotation_existed = true;

                                                            if (MACHINE == 0) man = C_SIMU_SH[NUMBER_OF_MACHINES -1];
                                                            C_SIMU_SH[(MACHINE + NUMBER_OF_MACHINES -1)% NUMBER_OF_MACHINES] = C_SIMU_SH[MACHINE];
                                                            if (MACHINE == NUMBER_OF_MACHINES - 1) C_SIMU_SH[NUMBER_OF_MACHINES - 2] = man;

                                                            C_SIMU_ROTATION_MAGNITUDE[MACHINE]--;
                                                        });
                                    } while (rotation_existed == true);
                                    continue;}

                case _CELL_SHR: {
                                    //step 1: load shift reg with R[LEFT].
                                    bool rotation_existed;
                                    {FOR_ALL_MACHINES( C_SIMU_SH[MACHINE] = C_SIMU_REGS[MACHINE][GET_LEFT(*CI)];);}

                                    //step 2: copy number of positions to rotate
                                    {FOR_ALL_MACHINES( C_SIMU_ROTATION_MAGNITUDE[MACHINE] = C_SIMU_REGS[MACHINE][GET_RIGHT(*CI)] % NUMBER_OF_MACHINES;);}

                                    //step 3: rotate one position at a time
                                    do
                                    {
                                        rotation_existed = false;
                                        FOR_ALL_MACHINES(
                                                        if ( C_SIMU_ROTATION_MAGNITUDE[MACHINE] > 0)
                                                        {
                                                            INT_REGVALUE man;
                                                            rotation_existed = true;

                                                            if (MACHINE == NUMBER_OF_MACHINES -2) man = C_SIMU_SH[NUMBER_OF_MACHINES-1];
                                                            C_SIMU_SH[(NUMBER_OF_MACHINES - MACHINE)% NUMBER_OF_MACHINES] = C_SIMU_SH[NUMBER_OF_MACHINES - MACHINE -1];
                                                            if (MACHINE == NUMBER_OF_MACHINES - 1) C_SIMU_SH[0] = man;

                                                            C_SIMU_ROTATION_MAGNITUDE[MACHINE]--;
                                                        });
                                    } while (rotation_existed == true);
                                    continue;}

                case _READ:  {FOR_ALL_ACTIVE_MACHINES(C_SIMU_REGS[MACHINE][GET_DEST(*CI)] = C_SIMU_LS[MACHINE][GET_RIGHT(*CI)]);continue;}
                case _WRITE: {FOR_ALL_ACTIVE_MACHINES(C_SIMU_LS[MACHINE][GET_RIGHT(*CI)] = C_SIMU_REGS[MACHINE][GET_LEFT(*CI)]);continue;}

                case _MULT:  {FOR_ALL_ACTIVE_MACHINES(C_SIMU_MULTREGS[MACHINE] = C_SIMU_REGS[MACHINE][GET_RIGHT(*CI)] * C_SIMU_REGS[MACHINE][GET_LEFT(*CI)]);continue;}
                case _MULT_HI:{FOR_ALL_ACTIVE_MACHINES(C_SIMU_REGS[MACHINE][GET_DEST(*CI)] = C_SIMU_MULTREGS[MACHINE] >> 16);continue;}
                case _MULT_LO:{FOR_ALL_ACTIVE_MACHINES(C_SIMU_REGS[MACHINE][GET_DEST(*CI)] = C_SIMU_MULTREGS[MACHINE] & 0xFFFF);continue;}

                case _WHERE_CRY:{FOR_ALL_MACHINES(if (C_SIMU_CARRY[MACHINE]==1) C_SIMU_ACTIVE[MACHINE]= _ACTIVE;else C_SIMU_ACTIVE[MACHINE]=_INACTIVE);continue;}
                case _WHERE_EQ:{FOR_ALL_MACHINES(if (C_SIMU_EQ[MACHINE]==1) C_SIMU_ACTIVE[MACHINE]= _ACTIVE;else C_SIMU_ACTIVE[MACHINE]=_INACTIVE);continue;}
                case _WHERE_LT:{FOR_ALL_MACHINES(if (C_SIMU_LT[MACHINE]==1) C_SIMU_ACTIVE[MACHINE]= _ACTIVE;else C_SIMU_ACTIVE[MACHINE]=_INACTIVE);continue;}
                case _END_WHERE:{FOR_ALL_MACHINES(C_SIMU_ACTIVE[MACHINE]= _ACTIVE);continue;}
                case _NOP:      continue;
                case _REDUCE:   {UINT32 sum = 0; FOR_ALL_ACTIVE_MACHINES(sum += C_SIMU_REGS[MACHINE][GET_LEFT(*CI)]);result = sum & REDUCTION_SIZE_MASK; continue;}
                default: result= FAIL; break;
            }
    }
    return result;
}


