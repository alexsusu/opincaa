/*
 * File:   vector.cpp
 * Author: Calin
 *
 * Created on December 19, 2012, 3:32 PM
 */

#include "../include/vector.h"
#include "../include/opcodes.h"

vector::vector(UINT_INSTRUCTION main_val, UINT_INSTRUCTION intermediate_val)
{
    mval = main_val;
    ival = intermediate_val;
    imval = 0;
    opcode = _NOP;
}

vector::vector(UINT_INSTRUCTION main_val, UINT_INSTRUCTION intermediate_val, UINT_INSTRUCTION op_code)
{
    mval = main_val;
    ival = intermediate_val;
    imval = 0;
    opcode = op_code;
}

vector::~vector()
{
    //dtor
}

/****************************************************************************************************************************\
 *********************************************          OPERATORS           *************************************************
 \***************************************************************************************************************************/

/* BINM = binary operators: for use with binary operators that use two operands but not self */
/* These functions will be static, to highlight non-use of self */
#define RETURN_NEW_OBJ_BIN(opcode) return vector(0, (other_left.mval << (LEFT_POS)) + (other_right.mval << (RIGHT_POS)), opcode);

/* BINM = binary operators modificated: for use with binary operators that use self and another operand */
#define RETURN_NEW_OBJ_BINM(opcode) return vector(0, (other.mval << (RIGHT_POS)) + (mval << (LEFT_POS)), opcode);
#define RETURN_NEW_OBJ_UNM(opcode) return vector(0, (mval << (LEFT_POS)), opcode);
// uses immediate value
#define RETURN_NEW_OBJ_BINM_IMMVAL(opcode) return vector(0, (imm_val << (IMMEDIATE_VALUE_POS)) + (mval << (LEFT_POS)), opcode);
#define RETURN_NEW_OBJ_BIN_IMMVAL(opcode) return vector(0, (imm_val << (IMMEDIATE_VALUE_POS)) + (other.mval << (LEFT_POS)), opcode);

vector vector::operator+(vector other) {RETURN_NEW_OBJ_BINM(_ADD)};
vector vector::operator-(vector other) {RETURN_NEW_OBJ_BINM(_SUB)};

//!!!
//vector vector::operator!(vector other) {CREATE_OBJ_BINM;  SET_OBJ_BINM_OPCODE(_LNOT);    RETURN_OBJ_BINM};

vector vector::operator|(vector other) {RETURN_NEW_OBJ_BINM(_OR)};
vector vector::operator&(vector other) {RETURN_NEW_OBJ_BINM(_AND)};
vector vector::operator^(vector other) {RETURN_NEW_OBJ_BINM(_XOR)};
vector vector::operator==(vector other) {RETURN_NEW_OBJ_BINM(_EQ)};
vector vector::operator<(vector other) {RETURN_NEW_OBJ_BINM(_LT)};
vector vector::operator~() {RETURN_NEW_OBJ_UNM(_NOT)};

vector vector::operator<<(vector other) {RETURN_NEW_OBJ_BINM(_SHL)};
vector vector::operator>>(vector other) {RETURN_NEW_OBJ_BINM(_SHR)};

vector vector::operator<<(UINT_PARAM imm_val) {RETURN_NEW_OBJ_BINM_IMMVAL(_ISHL)};
vector vector::operator>>(UINT_PARAM imm_val) {RETURN_NEW_OBJ_BINM_IMMVAL(_ISHR)};

vector vector::shra(vector other_left, vector other_right)  {RETURN_NEW_OBJ_BIN(_SHRA)};
vector vector::ishra(vector other, UINT_PARAM imm_val) {RETURN_NEW_OBJ_BIN_IMMVAL(_ISHRA)};

vector vector::ult(vector other_left, vector other_right) {RETURN_NEW_OBJ_BIN(_ULT)};
vector vector::addc(vector other_left, vector other_right) {RETURN_NEW_OBJ_BIN(_ADDC)};
vector vector::subc(vector other_left, vector other_right) {RETURN_NEW_OBJ_BIN(_SUBC)};

//vector vector::iwr(int val);
//



// ldix, ldsh, any other op
vector vector::operator=(vector other)
{
    if ((other.mval == INDEX_MARKER) && (other.ival == INDEX_MARKER)) //ldix
    {
        dwBatch[dwBatchIndex][dwInBatchCounter[dwBatchIndex]++] = (_LDIX << OPCODE_9BITS_POS) + mval;
    }
    else if ((other.mval == SHIFTREG_MARKER) && (other.ival == SHIFTREG_MARKER)) //ldsh
    {
        dwBatch[dwBatchIndex][dwInBatchCounter[dwBatchIndex]++] = (_LDSH << OPCODE_9BITS_POS) + mval;
    }
    else
    {
        dwBatch[dwBatchIndex][dwInBatchCounter[dwBatchIndex]++] = (other.opcode << OPCODE_9BITS_POS) + (other.ival + mval);
        opcode = _NOP;
    }

    return vector(mval, 0);
}

//read
//vector vector::LocalStore(vector other)

//write

// vload
vector vector::operator=(UINT_PARAM value)
{
    dwBatch[dwBatchIndex][dwInBatchCounter[dwBatchIndex]++]  = (_VLOAD << OPCODE_6BITS_POS) + ((value << IMMEDIATE_VALUE_POS)  + mval);
    return vector(mval, 0);
}

void vector::reduce(vector other_left)
{
    dwBatch[dwBatchIndex][dwInBatchCounter[dwBatchIndex]++] = (_REDUCE << OPCODE_9BITS_POS) + (other_left.mval << LEFT_POS);
}

void vector::onlyOpcode(UINT_INSTRUCTION opcode)
{
    dwBatch[dwBatchIndex][dwInBatchCounter[dwBatchIndex]++] = (opcode << OPCODE_9BITS_POS);
}

void vector::nop() {vector::onlyOpcode(_NOP);}
void vector::WhereCry() {vector::onlyOpcode(_WHERE_CRY);}
void vector::WhereEq() {onlyOpcode(_WHERE_EQ);}
void vector::WhereLt() {onlyOpcode(_WHERE_LT);}
void vector::EndWhere() {onlyOpcode(_END_WHERE);}


int vector::initialize()
{
    if ((pipe_read_32 = fopen ("/dev/xillybus_read_mem2arm_32","rb")) == NULL)
    {
        perror("Failed to open the read pipe");
        return -1;
    }


    if ((pipe_write_32 = fopen ("/dev/xillybus_write_arm2mem_32","wb")) == NULL)
    {
        perror("Failed to open the write pipe");
        return -1;
    }
}

void vector::setBatchIndex(UINT16 BI)
{
    dwBatchIndex = BI;
    dwInBatchCounter[BI] = 0;
}

void vector::executeKernel(UINT16 dwBatchNumber)
{
    fwrite(dwBatch[dwBatchNumber], 1, 4*dwInBatchCounter[dwBatchNumber], pipe_write_32);
}

int vector::executeKernelRed(UINT16 dwBatchNumber)
{
    int data_read;
    executeKernel(dwBatchNumber);
    if (fread(&data_read, 1, 4, pipe_read_32) == 4) return data_read;
    else
    {
        perror("Failed to read from pipe !");
        return 0;
    }
}

