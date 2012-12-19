#include "..\include\vector.h"

#define LEFT_POS        5
#define DEST_POS        0
#define RIGHT_POS       10
#define IMMEDIATE_VALUE 10
#define OPCODE_9BITS_POS    23
#define OPCODE_6BITS_POS    26

#define _ADD     0b101000100
#define _ADDC    0b101100100
#define _SUB     0b101010100
#define _SUBC    0b101110100

#define _LNOT    0b101001100
#define _LOR     0b101011100
#define _LAND    0b101101100
#define _LXOR    0b101111100
#define _EQ      0b101001000
#define _LT      0b101011000
#define _ULT     0b101101000
#define _SHL     0b101000000
#define _SHR     0b101010000
#define _SHRA    0b101100000
#define _ISHL    0b101000001
#define _ISHR    0b101010001
#define _ISHRA   0b101100001

#define _LDIX    0b100100000
#define _LDSH    0b100110000
#define _CELL_SHL 0b100010010
#define _CELL_SHR 0b100010001
#define _READ    0b100100100
#define _WRITE   0b100010100
#define _MULT    0b100001000
#define _MULTL   0b100101000
#define _MULTH   0b100111000

#define _WHERE_CRY 0b100011100
#define _WHERE_EQ  0b100011101
#define _WHERE_LT  0b100011110
#define _END_WHERE 0b100011111
#define _REDUCE    0b100000000
#define _NOP       0b000000000

/* 6-bit opcodes (instruction will have immediate value) */
#define _VLOAD   0b110101
#define _IRD     0b110100
#define _IWR     0b110010


vector::vector(int main_val, int intermediate_val)
{
    mval = main_val;
    ival = intermediate_val;
    imval = 0;
    opcode = _NOP;
}

vector::vector(int main_val, int intermediate_val, int op_code)
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
// uses immediate value
#define RETURN_NEW_OBJ_BINM_IMMVAL(opcode) return vector(0, (imm_val << (IMMEDIATE_VALUE)) + (mval << (LEFT_POS)), opcode);
#define RETURN_NEW_OBJ_BIN_IMMVAL(opcode) return vector(0, (imm_val << (IMMEDIATE_VALUE)) + (other.mval << (LEFT_POS)), opcode);

vector vector::operator+(vector other) {RETURN_NEW_OBJ_BINM(_ADD)};
vector vector::operator-(vector other) {RETURN_NEW_OBJ_BINM(_SUB)};

//!!!
//vector vector::operator!(vector other) {CREATE_OBJ_BINM;  SET_OBJ_BINM_OPCODE(_LNOT);    RETURN_OBJ_BINM};

vector vector::operator||(vector other) {RETURN_NEW_OBJ_BINM(_LOR)};
vector vector::operator&&(vector other) {RETURN_NEW_OBJ_BINM(_LAND)};
vector vector::operator^(vector other) {RETURN_NEW_OBJ_BINM(_LXOR)};
vector vector::operator==(vector other) {RETURN_NEW_OBJ_BINM(_EQ)};
vector vector::operator<(vector other) {RETURN_NEW_OBJ_BINM(_LT)};

vector vector::operator<<(vector other) {RETURN_NEW_OBJ_BINM(_SHL)};
vector vector::operator>>(vector other) {RETURN_NEW_OBJ_BINM(_SHR)};

vector vector::operator<<(int imm_val) {RETURN_NEW_OBJ_BINM_IMMVAL(_SHL)};
vector vector::operator>>(int imm_val) {RETURN_NEW_OBJ_BINM_IMMVAL(_SHR)};

vector vector::shra(vector other_left, vector other_right)  {RETURN_NEW_OBJ_BIN(_SHRA)};
vector vector::ishra(vector other, int imm_val) {RETURN_NEW_OBJ_BIN_IMMVAL(_ISHRA)};

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
        dwBatch[dwBatchIndex][dwInBatchCounter[dwBatchIndex]++] = (opcode << OPCODE_9BITS_POS) + (other.ival + mval);
        opcode = _NOP;
    }

    return vector(mval, 0);
}

//read
//vector vector::LocalStore(vector other)

//write

// vload
vector vector::operator=(int value)
{
    dwBatch[dwBatchIndex][dwInBatchCounter[dwBatchIndex]++]  = (_VLOAD << OPCODE_6BITS_POS) + ((value << IMMEDIATE_VALUE)  + mval);
    return vector(mval, 0);
}

void vector::reduce(vector other_left)
{
    dwBatch[dwBatchIndex][dwInBatchCounter[dwBatchIndex]++] = (_REDUCE << OPCODE_9BITS_POS) + (other_left.mval << LEFT_POS);
}

void vector::onlyOpcode(int opcode)
{
    dwBatch[dwBatchIndex][dwInBatchCounter[dwBatchIndex]++] = (_NOP << OPCODE_9BITS_POS);
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

void vector::setBatchIndex(int BI)
{
    dwBatchIndex = BI;
    dwInBatchCounter[BI] = 0;
}

void vector::executeKernel(int dwBatchNumber)
{
    fwrite(dwBatch[dwBatchNumber], 1, 4*dwInBatchCounter[dwBatchNumber], pipe_write_32);
}

int vector::executeKernelRed(int dwBatchNumber)
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

/* opcodes */
int opcodes[] = { _ADD, _ADDC, _SUB, _SUBC, _LNOT, _LOR, _LAND, _LXOR, _EQ, _LT, _ULT, _SHL, _SHR, _SHRA, _ISHL, _ISHR, _ISHRA,
                    _LDIX, _LDSH, _CELL_SHL,_CELL_SHR, _READ, _WRITE, _MULT, _MULTL, _MULTH, _VLOAD, _IRD, _IWR,
                    _WHERE_CRY, _WHERE_EQ, _WHERE_LT, _END_WHERE, _REDUCE, _NOP };

int vector::verifyKernel(int dwBatchNumber)
{
    // verify opcodes
    fwrite(dwBatch[dwBatchNumber], 1, 4*dwInBatchCounter[dwBatchNumber], pipe_write_32);
    int index;
    int* CurrentInstruction;
    for (CurrentInstruction = &dwBatch[dwBatchNumber][0];
            CurrentInstruction < (dwBatch[dwBatchNumber] + dwInBatchCounter[dwBatchNumber]);
                CurrentInstruction++)
    {
        if ((((*CurrentInstruction) >> OPCODE_6BITS_POS) == _VLOAD) ||
            (((*CurrentInstruction) >> OPCODE_6BITS_POS) == _IRD) ||
             (((*CurrentInstruction) >> OPCODE_6BITS_POS) == _IWR)) return PASS;

        for (index = 0; index < sizeof(opcodes); index++)
            if (((*CurrentInstruction) >> OPCODE_9BITS_POS) == opcodes[index]) return PASS;

        perror (" Instruction's opcode is not recognized. This might be a sort of ... crappy code ;)");
        return FAIL;
    }
}
