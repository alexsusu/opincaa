#include "..\include\vector.h"

#define LEFT_POS        5
#define DEST_POS        0
#define RIGHT_POS       10
#define IMMEDIATE_VALUE 10
#define OPCODE_9BITS_POS    23
#define OPCODE_6BITS_POS    26

/* 9 bit opcodes */


#define _ADD     0b101000100
#define _ADDC    0b101100100
#define _SUB     0b101010100
#define _SUBC    0b101110100
#define _REDUCE  0b100000000

#define NOP     0

vector::vector(int main_val, int intermediate_val)
{
    mval = main_val;
    ival = intermediate_val;
    imval = 0;
    opcode = NOP;
}

vector::~vector()
{
    //dtor
}

void vector::setBatchIndex(int BI)
{
    dwBatchIndex = BI;
    dwInBatchCounter[BI] = 0;
}

//left.+(right)
vector vector::operator+(vector other)
{
    opcode = _ADD;
    return vector(0, (other.mval << RIGHT_POS) + (mval << LEFT_POS));
}

//left.+(right)
vector vector::addc(vector other_left, vector other_right)
{
    vector a(0, (other_left.mval << LEFT_POS) + (other_right.mval << RIGHT_POS));
    a.opcode = _ADDC;
    return a;
}

//
void vector::reduce(vector other_left)
{
    dwBatch[dwBatchIndex][dwInBatchCounter[dwBatchIndex]++] = (_REDUCE << OPCODE_9BITS_POS) + (other_left.mval << LEFT_POS);
}

//left.-(right)
vector vector::operator-(vector other)
{
    opcode = _SUB;
    return vector(0, (other.mval << RIGHT_POS) + (mval << LEFT_POS));
}

vector vector::operator=(vector other)
{
    vector::dwBatch[dwBatchIndex][dwInBatchCounter[dwBatchIndex]++] = (opcode << OPCODE_9BITS_POS) + (other.ival + mval);
    opcode = 0;
    return vector(mval, 0);
}

vector vector::operator=(int value)
{
    dwBatch[dwBatchIndex][dwInBatchCounter[dwBatchIndex]++]  = (opcode << OPCODE_6BITS_POS) + ((value << IMMEDIATE_VALUE)  + mval);
    opcode = 0;
    return vector(mval, 0);
}
