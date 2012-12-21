
#include "../include/vector.h"
#include "../include/opcodes.h"

/* opcodes */
struct OpCodeDeAsm
{
    UINT_INSTRUCTION opcode;
    char* opcodeName;
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
    {_MULT,"MULT"},
    {_MULTL,"MULTL"},
    {_MULTH,"MULTH"},
    {_VLOAD,"VLOAD"},
    {_IRD,"IRD"},
    {_IWR,"IWR"},
    {_WHERE_CRY,"SET_ACTIVE(WHERE_CARRY)"},
    {_WHERE_EQ,"SET_ACTIVE(WHERE_EQUAL)"},
    {_WHERE_LT,"SET_ACTIVE(WHERE_LESS_THAN)"},
    {_END_WHERE,"SET_ACTIVE(ALL)"},
    {_REDUCE,"reduce"},
    {_NOP,"nop"}
};

int vector::verifyKernelInstruction(UINT_INSTRUCTION CurrentInstruction)
{
    int index;
    if  ( (GET_OPCODE_9BITS(CurrentInstruction) == _VLOAD) ||
          (GET_OPCODE_9BITS(CurrentInstruction) == _IRD) ||
          (GET_OPCODE_9BITS(CurrentInstruction) == _IWR)
         )
        return PASS;

    for (index = 0; index < sizeof(OpCodeDeAsms) / sizeof (OpCodeDeAsm); index++)
        if (GET_OPCODE_9BITS(CurrentInstruction) == OpCodeDeAsms[index].opcode) return PASS;

    perror (" Instruction's opcode is not recognized. This might be a sort of ... crappy code ;)");
    return FAIL;
}

int vector::verifyKernel(UINT16 dwBatchNumber)
{
    // verify opcodes
    UINT_INSTRUCTION* pCurrentInstruction;
    for (pCurrentInstruction = &dwBatch[dwBatchNumber][0];
            pCurrentInstruction < (dwBatch[dwBatchNumber] + dwInBatchCounter[dwBatchNumber]);
                pCurrentInstruction++)
                    if (FAIL == verifyKernelInstruction(*pCurrentInstruction)) return FAIL;

    return PASS;
}

char* getOpCodeName(int instruction)
{
    int index;
    instruction = GET_OPCODE_9BITS(instruction);

    for (index = 0; index < sizeof(OpCodeDeAsms) / sizeof (OpCodeDeAsm); index++)
      if (instruction == OpCodeDeAsms[index].opcode) return OpCodeDeAsms[index].opcodeName;

    return "__UNKNOWN__";
}

void printVLOAD(UINT_INSTRUCTION instr) {    printf("R%u = %u (zis si 0x%x)\n", GET_DEST(instr), GET_IMM(instr), GET_IMM(instr));}
void printIRD(UINT_INSTRUCTION instr) {    printf("R%d = LS[%d]\n", GET_DEST(instr), GET_IMM(instr));}
void printIWR(UINT_INSTRUCTION instr) {    printf("LS[%d] = R%d\n", GET_IMM(instr), GET_LEFT(instr));}

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
void printRED(UINT_INSTRUCTION instr){   printf("REDUCE(%d)\n", GET_LEFT(instr));}

void printNOT(UINT_INSTRUCTION instr){   printf("R%d = %sR%d\n", GET_DEST(instr),getOpCodeName(instr),GET_LEFT(instr));}

void printCELL_SHLR(UINT_INSTRUCTION instr){   printf("SHIFT_REG = R%d then %s by R%d positions \n", GET_LEFT(instr),getOpCodeName(instr),GET_RIGHT(instr));}
void printREAD(UINT_INSTRUCTION instr){   printf("R%d = LS[R%d]\n", GET_DEST(instr),GET_RIGHT(instr));}
void printWRITE(UINT_INSTRUCTION instr){   printf("LS[R%d] = R%d\n", GET_RIGHT(instr),GET_LEFT(instr));}

void printMULT(UINT_INSTRUCTION instr){   printf("R%d %s R%d\n", GET_LEFT(instr), getOpCodeName(instr), GET_RIGHT(instr));}
void printMULTLH(UINT_INSTRUCTION instr){   printf("R%d = %s\n", GET_DEST(instr), getOpCodeName(instr));}

int vector::deasmKernel(UINT16 dwBatchNumber)
{
    // verify opcodes
    int index = 0;
    int result = 0;
    UINT_INSTRUCTION* CurrentInstruction;
    for (CurrentInstruction = &dwBatch[dwBatchNumber][0];
            CurrentInstruction < (dwBatch[dwBatchNumber] + dwInBatchCounter[dwBatchNumber]);
                CurrentInstruction++)
    {
        printf("%5d: ",index);
        index++;

        switch (((*CurrentInstruction) >> OPCODE_6BITS_POS) & ((1 << OPCODE_6BITS_SIZE)-1))
            {
                case _VLOAD: {printVLOAD(*CurrentInstruction);continue;}
                case _IRD: {printIRD(*CurrentInstruction);continue;}
                case _IWR: {printIWR(*CurrentInstruction);continue;}
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

                case _MULTH:
                case _MULTL: printMULTLH(*CurrentInstruction);continue;

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
