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

#include "../../include/core/cnxvector.h"
#include "../../include/core/cnxvector_errors.h"
#include "../../include/core/opcodes.h"
#include "../../include/core/opcodes.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef _MSC_VER //MS C++ compiler
	#include <unistd.h>
#else
	#include "../../ms_visual_c/fake_unistd.h"
#endif

    UINT_INSTRUCTION* cnxvector::dwBatch[NUMBER_OF_BATCHES];
    UINT32 cnxvector::dwInBatchCounter[NUMBER_OF_BATCHES];
    UINT16 cnxvector::dwBatchIndex;
    int cnxvector::pipe_read_32;
    int cnxvector::pipe_write_32;
    int cnxvector::dwErrorCounter;
    unsigned char cnxvector::bEstimationMode;
    int cnxvector::dwLastJmpLabel;


cnxvector::cnxvector(UINT_INSTRUCTION MainVal)
{
    mval = MainVal;
    ival = NO_IVAL_MARKER;
    imval = NO_IMVAL_MARKER;
    //opcode = _ISHR; // for Assign (R0 = R1 via R0= ISHR(R1,0))
    opcode = _NOP;
}

cnxvector::cnxvector(UINT_INSTRUCTION MainVal, UINT_INSTRUCTION intermediate_val)
{
    mval = MainVal;
    ival = intermediate_val;
    imval = NO_IMVAL_MARKER;
    //opcode = _ISHR; // for Assign (R0 = R1 via R0= ISHR(R1,0))
    opcode = _NOP;
}

cnxvector::cnxvector(UINT_INSTRUCTION MainVal, UINT_INSTRUCTION intermediate_val, UINT_INSTRUCTION op_code)
{
    mval = MainVal;
    ival = intermediate_val;
    imval = NO_IMVAL_MARKER;
    opcode = op_code;
}

cnxvector::cnxvector(UINT_INSTRUCTION MainVal, UINT_INSTRUCTION intermediate_val, UINT_INSTRUCTION imm_val, UINT_INSTRUCTION op_code)
{
    mval = MainVal;
    ival = intermediate_val;
    imval = imm_val;
    opcode = op_code;
}

cnxvector::~cnxvector()
{
    //dtor
}

void cnxvector::appendInstruction(UINT_INSTRUCTION instr)
{
    if (bEstimationMode == 0)
        dwBatch[dwBatchIndex][dwInBatchCounter[dwBatchIndex]++] = instr;
    else dwInBatchCounter[dwBatchIndex]++;
}

void cnxvector::replaceInstruction(UINT_INSTRUCTION instr, int index)
{
    dwBatch[dwBatchIndex][index] = instr;
}


/****************************************************************************************************************************\
 *********************************************          OPERATORS           *************************************************
 \***************************************************************************************************************************/



/* BIN = binary operators: for use with binary operators that use two operands but not self */
/* These functions will be static, to highlight non-use of self */
#define RETURN_NEW_OBJ_BIN(opcode) return cnxvector(0, (other_left.mval << (LEFT_POS)) + (other_right.mval << (RIGHT_POS)), opcode);

/* BINM = binary operators modificated: for use with binary operators that use self and another operand */
#define RETURN_NEW_OBJ_BINM(opcode) return cnxvector(0, (other.mval << (RIGHT_POS)) + (mval << (LEFT_POS)), opcode);
#define RETURN_NEW_OBJ_UNM(opcode) return cnxvector(0, (mval << (LEFT_POS)), opcode);

#define SELF_OP(opcodeee) cnxvector self(0, (other.mval << (RIGHT_POS)) + (mval << (LEFT_POS)), opcodeee);\
                        appendInstruction((self.opcode << OPCODE_9BITS_POS) + (self.ival + mval));

// uses immediate value
#define RETURN_NEW_OBJ_BINM_IMMVAL(opcode) return cnxvector(0, (imm_val << (IMMEDIATE_VALUE_POS)) + (mval << (LEFT_POS)), opcode);
#define RETURN_NEW_OBJ_BIN_IMMVAL(opcode) return cnxvector(0, (imm_val << (IMMEDIATE_VALUE_POS)) + (other.mval << (LEFT_POS)), opcode);

// for pseudo instructions
#define RETURN_NEW_OBJ_PBINM_IMMVAL(opcode) return cnxvector(0, (mval << (LEFT_POS)), imm_val, opcode);
#define RETURN_NEW_OBJ_PBIN_IMMVAL(opcode) return cnxvector(0, (other.mval << (LEFT_POS)), imm_val, opcode);


cnxvector cnxvector::operator+(cnxvector other) {RETURN_NEW_OBJ_BINM(_ADD)};
//cnxvector cnxvector::operator+(UINT_PARAM imm_val) { if (imm_val==1) return cnxvector(0, (mval << (LEFT_POS)), NO_IMVAL_MARKER, _INC);
  //                                                   else RETURN_NEW_OBJ_PBINM_IMMVAL(_ADD);}

void cnxvector::operator+=(cnxvector other){ SELF_OP(_ADD);}
void cnxvector::operator+=(UINT_PARAM pseudoVal) {};
void cnxvector::operator++(int val) {};

cnxvector cnxvector::operator-(cnxvector other) {RETURN_NEW_OBJ_BINM(_SUB)};
cnxvector cnxvector::operator-(UINT_PARAM imm_val) {RETURN_NEW_OBJ_PBINM_IMMVAL(_SUB)};
void cnxvector::operator-=(cnxvector other) {SELF_OP(_SUB);}


/* Keep NOP in MULT: the marker of multiplication is the MULT register usage in operator =.
    This way, in case of R5 = R6 * R7, a nop will be generated instead.
    Legal use is:
    MULT = Rx * Ry // I had to use "=" to force compilator evaluate Rx.*(Ry)
    wait a few clock cycles
    Ra = _HI(MULT);
    Rb = _LO(MULT);
*/
cnxvector cnxvector::operator*(cnxvector other) {RETURN_NEW_OBJ_BINM(_MULT)};
cnxvector cnxvector::operator*(UINT_PARAM imm_val) {RETURN_NEW_OBJ_PBINM_IMMVAL(_MULT)};

cnxvector cnxvector::multlo(cnxvector other)
{
    if (other.mval != MULTIPLICATION_MARKER)
        cnxvectorError(ERR_MULT_LO_HI_PARAM);

    return cnxvector(0, 0, _MULT_LO);
};

cnxvector cnxvector::multhi(cnxvector other)
{
    if (other.mval != MULTIPLICATION_MARKER)
        cnxvectorError(ERR_MULT_LO_HI_PARAM);

    return cnxvector(0, 0, _MULT_HI);
};

cnxvector cnxvector::operator|(cnxvector other) {RETURN_NEW_OBJ_BINM(_OR)};
cnxvector cnxvector::operator|(UINT_PARAM imm_val) {RETURN_NEW_OBJ_PBINM_IMMVAL(_OR)};
void cnxvector::operator|=(cnxvector other) {SELF_OP(_OR);};

cnxvector cnxvector::operator&(cnxvector other) {RETURN_NEW_OBJ_BINM(_AND)};
cnxvector cnxvector::operator&(UINT_PARAM imm_val) {RETURN_NEW_OBJ_PBINM_IMMVAL(_AND)};
void cnxvector::operator&=(cnxvector other) {SELF_OP(_AND);};

cnxvector cnxvector::operator^(cnxvector other) {RETURN_NEW_OBJ_BINM(_XOR)};
cnxvector cnxvector::operator^(UINT_PARAM imm_val) {RETURN_NEW_OBJ_PBINM_IMMVAL(_XOR)};
void cnxvector::operator^=(cnxvector other) {SELF_OP(_XOR);};

cnxvector cnxvector::operator==(cnxvector other) {RETURN_NEW_OBJ_BINM(_EQ)};
cnxvector cnxvector::operator==(UINT_PARAM imm_val) {RETURN_NEW_OBJ_PBINM_IMMVAL(_EQ)};

cnxvector cnxvector::operator<(cnxvector other) {RETURN_NEW_OBJ_BINM(_LT)};
cnxvector cnxvector::operator<(UINT_PARAM imm_val) {RETURN_NEW_OBJ_PBINM_IMMVAL(_LT)};

cnxvector cnxvector::operator~() {RETURN_NEW_OBJ_UNM(_NOT)};

cnxvector cnxvector::operator<<(cnxvector other) {RETURN_NEW_OBJ_BINM(_SHL)};
cnxvector cnxvector::operator>>(cnxvector other) {RETURN_NEW_OBJ_BINM(_SHR)};

cnxvector cnxvector::operator<<(UINT_PARAM imm_val)
{
    if (imm_val > IMM_SHIFT_VAL_MAX) cnxvectorError(ERR_SHIFT_OUT_OF_RANGE);
    RETURN_NEW_OBJ_BINM_IMMVAL(_ISHL)
};

cnxvector cnxvector::operator>>(UINT_PARAM imm_val)
{
    if (imm_val > IMM_SHIFT_VAL_MAX) cnxvectorError(ERR_SHIFT_OUT_OF_RANGE);
    RETURN_NEW_OBJ_BINM_IMMVAL(_ISHR)
};

cnxvector cnxvector::operator[](cnxvector other) // to use in read / write with LOCALSTORE_REG
{
    if (mval != LOCALSTORE_MARKER) cnxvectorError(ERR_NOT_LOCALSTORE);
    imval = NO_IMVAL_MARKER; // force avoid collision with LS[immediate_value]
    ival = other.mval << (RIGHT_POS); // for write
    return cnxvector(mval, ival, imval, _NOP); // for read
    /* keep nop by default as it might be read or write */
};

cnxvector cnxvector::operator[](UINT_PARAM imm_val) // to use in read / write with LOCALSTORE_REG
{
    if (mval != LOCALSTORE_MARKER) cnxvectorError(ERR_NOT_LOCALSTORE);
    if (imm_val > IMM_VAL_MAX) cnxvectorError(ERR_SUBSCRIPT_OUT_OF_RANGE);
    imval = 1; // for write
    ival = imm_val << IMMEDIATE_VALUE_POS; // for write
    return cnxvector(mval, ival, imval, _NOP); // for read
    /* keep nop by default as it might be read or write */
};


cnxvector cnxvector::shra(cnxvector other_left, cnxvector other_right)  {RETURN_NEW_OBJ_BIN(_SHRA)};
cnxvector cnxvector::ishra(cnxvector other, UINT_PARAM imm_val) {RETURN_NEW_OBJ_BIN_IMMVAL(_ISHRA)};

cnxvector cnxvector::ult(cnxvector other_left, cnxvector other_right) {RETURN_NEW_OBJ_BIN(_ULT)};
cnxvector cnxvector::ult(cnxvector other, UINT_PARAM imm_val) {RETURN_NEW_OBJ_PBIN_IMMVAL(_ULT)};

cnxvector cnxvector::condsub(cnxvector other_left, cnxvector other_right) {RETURN_NEW_OBJ_BIN(_CONDSUB)};

//cnxvector cnxvector::inc(cnxvector other_left) {RETURN_NEW_OBJ_UNM(_INC)};
cnxvector cnxvector::addc(cnxvector other_left, cnxvector other_right) {RETURN_NEW_OBJ_BIN(_ADDC)};
cnxvector cnxvector::addc(cnxvector other, UINT_PARAM imm_val) {RETURN_NEW_OBJ_PBIN_IMMVAL(_ADDC)};

//cnxvector cnxvector::subc(cnxvector other_left, cnxvector other_right) {RETURN_NEW_OBJ_BIN(_SUBC)};
//cnxvector cnxvector::subc(cnxvector other, UINT_PARAM imm_val) {RETURN_NEW_OBJ_PBIN_IMMVAL(_SUBC)};

void cnxvector::cellshl(cnxvector other_left, cnxvector other_right)
{
    appendInstruction((_CELL_SHL << OPCODE_9BITS_POS) + (other_left.mval << LEFT_POS) + (other_right.mval << RIGHT_POS));
};

void cnxvector::cellshr(cnxvector other_left, cnxvector other_right)
{
    appendInstruction((_CELL_SHR << OPCODE_9BITS_POS) + (other_left.mval << LEFT_POS) + (other_right.mval << RIGHT_POS));
};

// ldix, ldsh, any other op
void cnxvector::operator=(cnxvector other)
{
    if ((other.mval == INDEX_MARKER) && (other.ival == INDEX_MARKER)) //ldix
    {
        appendInstruction((_LDIX << OPCODE_9BITS_POS) + mval);
    }
    else if ((other.mval == SHIFTREG_MARKER) && (other.ival == SHIFTREG_MARKER)) //ldsh
    {
        appendInstruction((_LDSH << OPCODE_9BITS_POS) + mval);
    }
    else if (other.mval == LOCALSTORE_MARKER) //read from local store. "other" is cnxvector_localstore[cnxvector rX] !
    {
        if (other.imval == NO_IMVAL_MARKER) // LS[REG], non immediate
            appendInstruction((_READ << OPCODE_9BITS_POS) + other.ival + mval);
        else // LS[value], immediate
            appendInstruction((_IREAD << OPCODE_6BITS_POS) + other.ival + mval);
    }
    else if (mval == LOCALSTORE_MARKER) //write in local sstore. "other" a cnxvector rX !
    {
        if (imval == NO_IMVAL_MARKER) // LS[REG], non immediate
            appendInstruction((_WRITE << OPCODE_9BITS_POS) + ival + (other.mval << LEFT_POS));
        else
            appendInstruction((_IWRITE << OPCODE_6BITS_POS) + ival + (other.mval << LEFT_POS));
    }
    else if (mval == MULTIPLICATION_MARKER) // MULT = Rx * Ry
    {
        appendInstruction((_MULT << OPCODE_9BITS_POS) + other.ival);
    }
    // MULT from R3 = R1 * R2 or from R3 = R1 * 5.
    else if (other.opcode == _MULT)
    {
        if (other.imval == NO_IMVAL_MARKER)
        {
            // MULT = R1 * R2; R3 = _LO(MULT)
            appendInstruction((_MULT << OPCODE_9BITS_POS) + other.ival);
            appendInstruction((_MULT_LO << OPCODE_9BITS_POS) + mval);
        }
        else
        {
            // R3 = 5; MULT = R1 * R3; R3 = _LO(MULT)
            appendInstruction((_VLOAD << OPCODE_6BITS_POS) + ((other.imval << IMMEDIATE_VALUE_POS)  + mval));
            appendInstruction((other.opcode << OPCODE_9BITS_POS) + (mval << RIGHT_POS) + (other.ival + mval));
            appendInstruction((_MULT_LO << OPCODE_9BITS_POS) + mval);

            other.imval = NO_IMVAL_MARKER;
            imval = NO_IMVAL_MARKER;
        }
        opcode = _NOP;
        other.opcode = _NOP;
    }
    else // including MULT_LO / HI
    {
        if (other.imval == NO_IMVAL_MARKER)
        {
            // no intermediate value
            if (other.ival == NO_IVAL_MARKER)
                appendInstruction((_ISHR << OPCODE_9BITS_POS) + (other.mval << LEFT_POS) + mval);
            else
                appendInstruction((other.opcode << OPCODE_9BITS_POS) + (other.ival + mval));
        }
        else
        {
            //printf("A: %d\n", other.imval);
            appendInstruction((_VLOAD << OPCODE_6BITS_POS) + ((other.imval << IMMEDIATE_VALUE_POS)  + mval));
            appendInstruction((other.opcode << OPCODE_9BITS_POS) + (mval << RIGHT_POS) + (other.ival + mval));
            other.imval = NO_IMVAL_MARKER;
            imval = NO_IMVAL_MARKER;
        }
        opcode = _NOP;
    }
}

// vload, iread, iwrite
void cnxvector::operator=(UINT_PARAM imm_val)
{
    if (imm_val > IMM_VAL_MAX) cnxvectorError(ERR_IMM_VALUE_OUT_OF_RANGE);
    //if (imm_val < IMM_VAL_SIGNED_MIN) cnxvectorError(ERR_IMM_VALUE_OUT_OF_RANGE);
    //imm_val = imm_val & IMMEDIATE_VALUE_MASK;
    appendInstruction((_VLOAD << OPCODE_6BITS_POS) + ((imm_val << IMMEDIATE_VALUE_POS)  + mval)); // vload
}

void cnxvector::reduce(cnxvector other_left)
{
    appendInstruction((_REDUCE << OPCODE_9BITS_POS) + (other_left.mval << LEFT_POS));
}

void cnxvector::onlyOpcode(UINT_INSTRUCTION opcode)
{
    appendInstruction((opcode << OPCODE_9BITS_POS));
}

void cnxvector::nop() {cnxvector::onlyOpcode(_NOP);}
void cnxvector::WhereCry() {cnxvector::onlyOpcode(_WHERE_CRY);}
void cnxvector::WhereEq() {onlyOpcode(_WHERE_EQ);}
void cnxvector::WhereLt() {onlyOpcode(_WHERE_LT);}
void cnxvector::EndWhere() {onlyOpcode(_END_WHERE);}

int cnxvector::initialize()
{
    int result = PASS;
    int BatchIndex;
    for (BatchIndex = 0; BatchIndex < NUMBER_OF_BATCHES; BatchIndex++)
                                dwBatch[BatchIndex] = NULL;
    return result;
}

int cnxvector::deinitialize()
{
    int result = PASS;
    return result;
}

void cnxvector::setBatchIndex(UINT16 BI)
{
    if (BI >= NUMBER_OF_BATCHES)
    {
        cnxvectorError(ERR_TOO_MANY_BATCHES);
        return;
    }

    dwBatchIndex = BI;
    dwInBatchCounter[BI] = 0;
}

UINT_INSTRUCTION cnxvector::getBatchInstruction(UINT16 BI, UINT32 index)
{
    return dwBatch[BI][index];
}

UINT_INSTRUCTION* cnxvector::getBatch(UINT16 BI)
{
    return dwBatch[BI];
}

void cnxvector::setBatch(UINT16 BI, UINT_INSTRUCTION* Batch)
{
    dwBatch[BI]= Batch;
}


UINT32 cnxvector::getInBatchCounter(UINT16 BI)
{
    return dwInBatchCounter[BI];
}

void cnxvector::setInBatchCounter(UINT16 BI, UINT32 InBatchCounter)
{
    dwInBatchCounter[BI] = InBatchCounter;
}


UINT_RED_REG_VAL cnxvector::executeBatch(UINT16 dwBatchNumber)
{
    write(pipe_write_32, dwBatch[dwBatchNumber], 4*dwInBatchCounter[dwBatchNumber]);
    write(pipe_write_32, NULL, 0);//flush
    return PASS;//compatibility with c_simulator function
}

UINT_RED_REG_VAL cnxvector::executeBatchRed(UINT16 dwBatchNumber)
{
    UINT_RED_REG_VAL data_read;
    executeBatch(dwBatchNumber);

    read(pipe_read_32, &data_read,4);
    return data_read;
}

UINT32 cnxvector::getMultiRedResult(UINT_RED_REG_VAL* RedResults, UINT32 Limit)
{
    //return number of actually read bytes
    if (Limit == 0)
    {
        return read(pipe_read_32,RedResults,MAX_MULTIRED_DWORDS);
    }
    else
    {
        /*
        int totalReadBytes = 0;
        int readBytes = 0;
        do
        {
            readBytes = read(pipe_read_32,RedResults + totalReadBytes,Limit - totalReadBytes);
            if (readBytes > 0) totalReadBytes+= readBytes;
        }
        while (readBytes != -1);
        */
        return read(pipe_read_32,RedResults, Limit);

    }
}

void cnxvector::jmp(int mode, int Loops)
{
    if (mode == JMP_MODE_SET_LABEL)
    {
        if (bEstimationMode == 0)
        {
           cnxvector::dwLastJmpLabel = getInBatchCounter(dwBatchIndex);
           nop();//add a nop here, will be overwritten
        }
        else
        {
            nop();
        }
    }
    else
    {
        if (bEstimationMode == 0)
        {
            int DeltaJump = getInBatchCounter(dwBatchIndex) - cnxvector::dwLastJmpLabel - 1;//do not count set lc label
            if (Loops > LOOPS_VAL_MAX) {cnxvectorError(ERR_LOOPS_TIMES_OUT_OF_RANGE); printf(" %d \n",Loops); }
            if (DeltaJump > DELTAJMP_VAL_MAX) {cnxvectorError(ERR_LOOP_LENGTH_OUT_OF_RANGE); printf(" %d \n",DeltaJump); }
            replaceInstruction((_SETLC << OPCODE_6BITS_POS) + (Loops << IMMEDIATE_VALUE_POS), cnxvector::dwLastJmpLabel);
            appendInstruction((_IJMPNZ << OPCODE_6BITS_POS) + (DeltaJump << IMMEDIATE_VALUE_POS));
        }
        else nop();
    }

    //if (imm_val < IMM_VAL_SIGNED_MIN) cnxvectorError(ERR_IMM_VALUE_OUT_OF_RANGE);
    //imm_val = imm_val & IMMEDIATE_VALUE_MASK;


}

