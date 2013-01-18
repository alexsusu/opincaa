/*
 * File:   vector.cpp
 *
 * OPINCAA's core implementation file.
 * Contains "vector" class where the usual operators (+, -, [], ^, &,* ....) are overloaded.
 *
 * Unlike usual systems where operators perform the math operation, this class does
 *  instruction assembly according to spec ConnexISA.docx
 *
 *
 *
 *
 */

#include "../../include/core/vector.h"
#include "../../include/core/vector_errors.h"
#include "../../include/core/opcodes.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef _MSC_VER //MS C++ compiler
	#include <unistd.h>
#else
	#include "../../ms_visual_c/fake_unistd.h"
#endif

    UINT_INSTRUCTION* vector::dwBatch[NUMBER_OF_BATCHES];
    UINT32 vector::dwInBatchCounter[NUMBER_OF_BATCHES];
    UINT16 vector::dwBatchIndex;
    int vector::pipe_read_32;
    int vector::pipe_write_32;
    int vector::dwErrorCounter;
    unsigned char vector::bEstimationMode;

vector::vector(UINT_INSTRUCTION MainVal, UINT_INSTRUCTION intermediate_val)
{
    mval = MainVal;
    ival = intermediate_val;
    imval = 0;
    opcode = _NOP;
}

vector::vector(UINT_INSTRUCTION MainVal, UINT_INSTRUCTION intermediate_val, UINT_INSTRUCTION op_code)
{
    mval = MainVal;
    ival = intermediate_val;
    imval = 0;
    opcode = op_code;
}

vector::vector(UINT_INSTRUCTION MainVal, UINT_INSTRUCTION intermediate_val, UINT16 imm_val, UINT_INSTRUCTION op_code)
{
    mval = MainVal;
    ival = intermediate_val;
    imval = imm_val;
    opcode = op_code;
}

vector::~vector()
{
    //dtor
}

void vector::appendInstruction(UINT_INSTRUCTION instr)
{
    if (bEstimationMode == 0)
        dwBatch[dwBatchIndex][dwInBatchCounter[dwBatchIndex]++] = instr;
    else dwInBatchCounter[dwBatchIndex]++;
    //if ((dwInBatchCounter[dwBatchIndex] % 1000) ==0)
      //  printf("%d ",dwInBatchCounter[dwBatchIndex]);
}


/****************************************************************************************************************************\
 *********************************************          OPERATORS           *************************************************
 \***************************************************************************************************************************/



/* BIN = binary operators: for use with binary operators that use two operands but not self */
/* These functions will be static, to highlight non-use of self */
#define RETURN_NEW_OBJ_BIN(opcode) return vector(0, (other_left.mval << (LEFT_POS)) + (other_right.mval << (RIGHT_POS)), opcode);

/* BINM = binary operators modificated: for use with binary operators that use self and another operand */
#define RETURN_NEW_OBJ_BINM(opcode) return vector(0, (other.mval << (RIGHT_POS)) + (mval << (LEFT_POS)), opcode);
#define RETURN_NEW_OBJ_UNM(opcode) return vector(0, (mval << (LEFT_POS)), opcode);

#define SELF_OP(opcodeee) vector self(0, (other.mval << (RIGHT_POS)) + (mval << (LEFT_POS)), opcodeee);\
                        appendInstruction((self.opcode << OPCODE_9BITS_POS) + (self.ival + mval));

// uses immediate value
#define RETURN_NEW_OBJ_BINM_IMMVAL(opcode) return vector(0, (imm_val << (IMMEDIATE_VALUE_POS)) + (mval << (LEFT_POS)), opcode);
#define RETURN_NEW_OBJ_BIN_IMMVAL(opcode) return vector(0, (imm_val << (IMMEDIATE_VALUE_POS)) + (other.mval << (LEFT_POS)), opcode);

// for pseudo instructions
#define RETURN_NEW_OBJ_PBINM_IMMVAL(opcode) return vector(0, (mval << (LEFT_POS)), imm_val, opcode);
#define RETURN_NEW_OBJ_PBIN_IMMVAL(opcode) return vector(0, (other.mval << (LEFT_POS)), imm_val, opcode);


vector vector::operator+(vector other) {RETURN_NEW_OBJ_BINM(_ADD)};
vector vector::operator+(UINT_PARAM imm_val) {RETURN_NEW_OBJ_PBINM_IMMVAL(_ADD);};
void vector::operator+=(vector other){ SELF_OP(_ADD);}

vector vector::operator-(vector other) {RETURN_NEW_OBJ_BINM(_SUB)};
vector vector::operator-(UINT_PARAM imm_val) {RETURN_NEW_OBJ_PBINM_IMMVAL(_SUB)};
void vector::operator-=(vector other) {SELF_OP(_SUB);}


/* Keep NOP in MULT: the marker of multiplication is the MULT register usage in operator =.
    This way, in case of R5 = R6 * R7, a nop will be generated instead.
    Legal use is:
    MULT = Rx * Ry // I had to use "=" to force compilator evaluate Rx.*(Ry)
    wait a few clock cycles
    Ra = _HI(MULT);
    Rb = _LO(MULT);
*/
vector vector::operator*(vector other) {RETURN_NEW_OBJ_BINM(_MULT)};
vector vector::operator*(UINT_PARAM imm_val) {RETURN_NEW_OBJ_PBINM_IMMVAL(_MULT)};

vector vector::multlo(vector other)
{
    if (other.mval != MULTIPLICATION_MARKER)
        vectorError(ERR_MULT_LO_HI_PARAM);

    return vector(0, 0, _MULT_LO);
};

vector vector::multhi(vector other)
{
    if (other.mval != MULTIPLICATION_MARKER)
        vectorError(ERR_MULT_LO_HI_PARAM);

    return vector(0, 0, _MULT_HI);
};

vector vector::operator|(vector other) {RETURN_NEW_OBJ_BINM(_OR)};
vector vector::operator|(UINT_PARAM imm_val) {RETURN_NEW_OBJ_PBINM_IMMVAL(_OR)};
void vector::operator|=(vector other) {SELF_OP(_OR);};

vector vector::operator&(vector other) {RETURN_NEW_OBJ_BINM(_AND)};
vector vector::operator&(UINT_PARAM imm_val) {RETURN_NEW_OBJ_PBINM_IMMVAL(_AND)};
void vector::operator&=(vector other) {SELF_OP(_AND);};

vector vector::operator^(vector other) {RETURN_NEW_OBJ_BINM(_XOR)};
vector vector::operator^(UINT_PARAM imm_val) {RETURN_NEW_OBJ_PBINM_IMMVAL(_XOR)};
void vector::operator^=(vector other) {SELF_OP(_XOR);};

vector vector::operator==(vector other) {RETURN_NEW_OBJ_BINM(_EQ)};
vector vector::operator==(UINT_PARAM imm_val) {RETURN_NEW_OBJ_PBINM_IMMVAL(_EQ)};

vector vector::operator<(vector other) {RETURN_NEW_OBJ_BINM(_LT)};
vector vector::operator<(UINT_PARAM imm_val) {RETURN_NEW_OBJ_PBINM_IMMVAL(_LT)};

vector vector::operator~() {RETURN_NEW_OBJ_UNM(_NOT)};

vector vector::operator<<(vector other) {RETURN_NEW_OBJ_BINM(_SHL)};
vector vector::operator>>(vector other) {RETURN_NEW_OBJ_BINM(_SHR)};

vector vector::operator<<(UINT_PARAM imm_val)
{
    if (imm_val > IMM_SHIFT_VAL_MAX) vectorError(ERR_SHIFT_OUT_OF_RANGE);
    RETURN_NEW_OBJ_BINM_IMMVAL(_ISHL)
};

vector vector::operator>>(UINT_PARAM imm_val)
{
    if (imm_val > IMM_SHIFT_VAL_MAX) vectorError(ERR_SHIFT_OUT_OF_RANGE);
    RETURN_NEW_OBJ_BINM_IMMVAL(_ISHR)
};

vector vector::operator[](vector other) // to use in read / write with LOCALSTORE_REG
{
    if (mval != LOCALSTORE_MARKER) vectorError(ERR_NOT_LOCALSTORE);
    imval = 0; // force avoid collision with LS[immediate_value]
    ival = other.mval << (RIGHT_POS); // for write
    return vector(mval, ival, imval, _NOP); // for read
    /* keep nop by default as it might be read or write */
};

vector vector::operator[](UINT_PARAM imm_val) // to use in read / write with LOCALSTORE_REG
{
    if (mval != LOCALSTORE_MARKER) vectorError(ERR_NOT_LOCALSTORE);
    if (imm_val > IMM_VAL_MAX) vectorError(ERR_SUBSCRIPT_OUT_OF_RANGE);
    imval = 1; // for write
    ival = imm_val << IMMEDIATE_VALUE_POS; // for write
    return vector(mval, ival, imval, _NOP); // for read
    /* keep nop by default as it might be read or write */
};


vector vector::shra(vector other_left, vector other_right)  {RETURN_NEW_OBJ_BIN(_SHRA)};
vector vector::ishra(vector other, UINT_PARAM imm_val) {RETURN_NEW_OBJ_BIN_IMMVAL(_ISHRA)};

vector vector::ult(vector other_left, vector other_right) {RETURN_NEW_OBJ_BIN(_ULT)};
vector vector::ult(vector other, UINT_PARAM imm_val) {RETURN_NEW_OBJ_PBIN_IMMVAL(_ULT)};

vector vector::addc(vector other_left, vector other_right) {RETURN_NEW_OBJ_BIN(_ADDC)};
vector vector::addc(vector other, UINT_PARAM imm_val) {RETURN_NEW_OBJ_PBIN_IMMVAL(_ADDC)};

vector vector::subc(vector other_left, vector other_right) {RETURN_NEW_OBJ_BIN(_SUBC)};
vector vector::subc(vector other, UINT_PARAM imm_val) {RETURN_NEW_OBJ_PBIN_IMMVAL(_SUBC)};

void vector::cellshl(vector other_left, vector other_right)
{
    appendInstruction((_CELL_SHL << OPCODE_9BITS_POS) + (other_left.mval << LEFT_POS) + (other_right.mval << RIGHT_POS));
};

void vector::cellshr(vector other_left, vector other_right)
{
    appendInstruction((_CELL_SHR << OPCODE_9BITS_POS) + (other_left.mval << LEFT_POS) + (other_right.mval << RIGHT_POS));
};

// ldix, ldsh, any other op
void vector::operator=(vector other)
{
    if ((other.mval == INDEX_MARKER) && (other.ival == INDEX_MARKER)) //ldix
    {
        appendInstruction((_LDIX << OPCODE_9BITS_POS) + mval);
    }
    else if ((other.mval == SHIFTREG_MARKER) && (other.ival == SHIFTREG_MARKER)) //ldsh
    {
        appendInstruction((_LDSH << OPCODE_9BITS_POS) + mval);
    }
    else if (other.mval == LOCALSTORE_MARKER) //read from local sstore. "other" is vector_localstore[vector rX] !
    {
        if (other.imval == 0) // LS[REG], non immediate
            appendInstruction((_READ << OPCODE_9BITS_POS) + other.ival + mval);
        else // LS[value], immediate
            appendInstruction((_IREAD << OPCODE_6BITS_POS) + other.ival + mval);
    }
    else if (mval == LOCALSTORE_MARKER) //write in local sstore. "other" a vector rX !
    {
        if (imval == 0) // LS[REG], non immediate
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
        if (other.imval == 0)
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

            other.imval = 0;
            imval = 0;
        }
        opcode = _NOP;
        other.opcode = _NOP;
    }
    else // including MULT_LO / HI
    {
        if (other.imval == 0)
            appendInstruction((other.opcode << OPCODE_9BITS_POS) + (other.ival + mval));
        else
        {
            //printf("A: %d\n", other.imval);
            appendInstruction((_VLOAD << OPCODE_6BITS_POS) + ((other.imval << IMMEDIATE_VALUE_POS)  + mval));
            appendInstruction((other.opcode << OPCODE_9BITS_POS) + (mval << RIGHT_POS) + (other.ival + mval));
            other.imval = 0;
            imval = 0;
        }
        opcode = _NOP;
    }
}

// vload, iread, iwrite
void vector::operator=(UINT_PARAM imm_val)
{
    if (imm_val > IMM_VAL_MAX) vectorError(ERR_IMM_VALUE_OUT_OF_RANGE);
    //if (imm_val < IMM_VAL_SIGNED_MIN) vectorError(ERR_IMM_VALUE_OUT_OF_RANGE);
    //imm_val = imm_val & IMMEDIATE_VALUE_MASK;
    appendInstruction((_VLOAD << OPCODE_6BITS_POS) + ((imm_val << IMMEDIATE_VALUE_POS)  + mval)); // vload
}

void vector::reduce(vector other_left)
{
    appendInstruction((_REDUCE << OPCODE_9BITS_POS) + (other_left.mval << LEFT_POS));
}

void vector::onlyOpcode(UINT_INSTRUCTION opcode)
{
    appendInstruction((opcode << OPCODE_9BITS_POS));
}

void vector::nop() {vector::onlyOpcode(_NOP);}
void vector::WhereCry() {vector::onlyOpcode(_WHERE_CRY);}
void vector::WhereEq() {onlyOpcode(_WHERE_EQ);}
void vector::WhereLt() {onlyOpcode(_WHERE_LT);}
void vector::EndWhere() {onlyOpcode(_END_WHERE);}

int vector::initialize()
{
    int result = PASS;
    int BatchIndex;
    for (BatchIndex = 0; BatchIndex < NUMBER_OF_BATCHES; BatchIndex++)
                                dwBatch[BatchIndex] = NULL;
    return result;
}

int vector::deinitialize()
{
    int result = PASS;
    return result;
}

void vector::setBatchIndex(UINT16 BI)
{
    if (BI >= NUMBER_OF_BATCHES)
    {
        vectorError(ERR_TOO_MANY_BATCHES);
        return;
    }

    dwBatchIndex = BI;
    dwInBatchCounter[BI] = 0;
}

UINT_INSTRUCTION vector::getBatchInstruction(UINT16 BI, UINT32 index)
{
    return dwBatch[BI][index];
}

UINT32 vector::getInBatchCounter(UINT16 BI)
{
    return dwInBatchCounter[BI];
}

UINT_RED_REG_VAL vector::executeBatch(UINT16 dwBatchNumber)
{
    write(pipe_write_32, dwBatch[dwBatchNumber], 4*dwInBatchCounter[dwBatchNumber]);
    write(pipe_write_32, NULL, 0);//flush
    return PASS;//compatibility with c_simulator function
}

UINT_RED_REG_VAL vector::executeBatchRed(UINT16 dwBatchNumber)
{
    unsigned char data_read[4];
    executeBatch(dwBatchNumber);

    read(pipe_read_32,data_read,1);
    read(pipe_read_32,data_read+1,1);
    read(pipe_read_32,data_read+2,1);
    read(pipe_read_32,data_read+3,1);

    return *((UINT_RED_REG_VAL *)data_read);
}

UINT32 vector::getMultiRedResult(UINT_RED_REG_VAL* RedResults, UINT32 Limit)
{
    //return number of actually read bytes
    if (Limit == 0)
        return read(pipe_read_32,RedResults,MAX_MULTIRED_DWORDS);
    else
        return read(pipe_read_32,RedResults,Limit);
}

