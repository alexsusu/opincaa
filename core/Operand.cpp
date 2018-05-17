/*
 * File:   Operand.cpp
 *
 * This is the header file for a class used to simulate
 * a Connex operand. Used by the OPINCAA library.
 */

#include "Operand.h"
#include "Architecture.h"

/*
 * Constructor for creating a new Operand
 *
 * @param type the type of this operand (reg or local store)
 * @param index the index of the register or local store array that is
 *   represented by this object
 * @param localStoreIndexImmediate Only applies for operands of type
 *   TYPE_LOCAL_STORE and it specifies
 *   if the index is the actual index in the localStore (true) or if
 *   it is the register storing the index (false)
 * @param kernel the kernel for which this operand is used
 * @throws string if the index is out of bounds or if the associated kernel is NULL
 */
Operand::Operand(int type, unsigned short index, bool localStoreIndexImmediate, Kernel *kernel)
{
    if (kernel == NULL)
    {
        throw string("Invalid kernel reference in Operand constructor");
    }

    switch (type)
    {
        case TYPE_REGISTER:
            if(index >= CONNEX_REG_COUNT)
            {
                throw string("Invalid register index in Operand constructor");
            }
            break;
        case TYPE_LOCAL_STORE:
            if(index >= CONNEX_MEM_SIZE)
            {
                throw string("Invalid local store index in Operand constructor");
            }
            break;
        case TYPE_INDEX_REG: /* fall through */
        case TYPE_SHIFT_REG:
        case TYPE_LS_DESCRIPTOR:
            break;
        default:
            throw string("Unknown operand type in Operand constructor");
    }

    this->type = type;
    this->index = index;
    this->kernel = kernel;
    this->localStoreIndexImmediate = localStoreIndexImmediate;
}

/*
 * Constructor for creating a new Operand with default localStoreIndexImmediate == false
 *
 * @param type the type of this operand (reg, etc)
 * @param index the index of the register or local store array that is
 *   represented by this object
 * @param kernel the kernel for which this operand is used
 * @throws string if the index is out of bounds or if the associated kernel is NULL
 */
Operand::Operand(int type, unsigned short index, Kernel *kernel)
{
    if(kernel == NULL)
    {
        throw string("Invalid kernel reference in Operand constructor");
    }

    switch(type)
    {
        case TYPE_REGISTER:
            if(index >= CONNEX_REG_COUNT)
            {
                throw string("Invalid register index in Operand constructor");
            }
            break;
        case TYPE_INDEX_REG: /* fall through */
        case TYPE_SHIFT_REG:
        case TYPE_LS_DESCRIPTOR:
            break;
        default:
            throw string("Unknown operand type in Operand constructor");
    }

    this->type = type;
    this->index = index;
    this->kernel = kernel;
    this->localStoreIndexImmediate = false;
}

/*
// IMPORTANT: Do NOT create a destructor for Operand because it will give
//                      Segfault, e.g. when using operator=().
Operand::~Operand() {
    kernel = NULL; // To avoid dangling pointers
}
*/

/***********************************************************
* Start of overloaded operators
***********************************************************/

/* Addition */
//-----------------------------------------------------------
Instruction Operand::operator+(Operand op)
{
    if(op.type != TYPE_REGISTER || type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for + operator");
    }
    return Instruction(_ADD, index, op.index, 0);
}
//-----------------------------------------------------------
Instruction Operand::operator+(unsigned short value)
{
    // Alex: TODO: try to print more info as from where it gives error: We can do this at disassembly time either in simulator or client for Opincaa-lib
    throw string("Unsupported operation +value ");
}
//-----------------------------------------------------------
void Operand::operator+=(Operand op)
{
    if(op.type != TYPE_REGISTER || type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for + operator");
    }
    kernel->append(Instruction(_ADD, index, op.index, index));
}

/* Subtraction */
//-----------------------------------------------------------
Instruction Operand::operator-(Operand op)
{
    if(op.type != TYPE_REGISTER || type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for - operator");
    }
    return Instruction(_SUB, op.index, index, 0);
}
//-----------------------------------------------------------
Instruction Operand::operator-(unsigned short value)
{
    throw string("Unsupported operation -value ");
}
//-----------------------------------------------------------
void Operand::operator-=(Operand op)
{
    if(op.type != TYPE_REGISTER || type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for - operator");
    }
    kernel->append(Instruction(_SUB, op.index, index, index));
}

/* Multiplication */
//-----------------------------------------------------------
Instruction Operand::operator*(Operand op)
{
    if(op.type != TYPE_REGISTER || type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for * operator");
    }
    Instruction instruction = Instruction(_MULT, op.index, index, 0);
    kernel->append(instruction);
    return instruction;
}
//-----------------------------------------------------------
Instruction Operand::operator*(unsigned short value)
{
    throw string("Unsupported operation *value ");
}

/* Assignment */
//-----------------------------------------------------------
void Operand::operator=(Operand op)
{
    /* Destination = register */
    if(type == TYPE_REGISTER)
    {
        switch(op.type)
        {
            case TYPE_REGISTER:
                /* Use an ISHL operation with 0 shift to simulate move */
                kernel->append(Instruction(_ISHL, 0, op.index, index));
                break;
            case TYPE_LOCAL_STORE:
                kernel->append(Instruction(op.localStoreIndexImmediate ? _IREAD : _READ,
                                            op.index,
                                            0,
                                            index));
                break;
            case TYPE_INDEX_REG:
                kernel->append(Instruction(_LDIX, 0, 0, index));
                break;
            case TYPE_SHIFT_REG:
                kernel->append(Instruction(_LDSH, 0, 0, index));
                break;
            case TYPE_LS_DESCRIPTOR:
            default:
                throw string("Unknown source type for operand= for register destination");
        }
    }
    /* Destination = local store */
    else if(type == TYPE_LOCAL_STORE)
    {
        switch(op.type)
        {
            case TYPE_REGISTER:
                kernel->append(Instruction(localStoreIndexImmediate ? _IWRITE : _WRITE,
                                          index,
                                          op.index,
                                          0));
                break;
            case TYPE_LOCAL_STORE:
            case TYPE_INDEX_REG:
            case TYPE_SHIFT_REG:
            case TYPE_LS_DESCRIPTOR:
            default:
                throw string("Unknown source type for operand= for local store destination");
        }
    }
    else
    {
        throw string("Unknown destination type for operand=");
    }
}
//-----------------------------------------------------------
/* Alex: 2017_08_26:
   we have signed short (i16) immediate operands, NOT unsigned:
  void Operand::operator=(unsigned short value) {
*/
void Operand::operator=(TYPE_ELEMENT value) {
    kernel->append(Instruction(_VLOAD, value, 0, index));
}
//-----------------------------------------------------------
void Operand::operator=(Instruction insn)
{
    if(insn.getOpcode() == _MULT)
    {
        kernel->append(Instruction(_MULT_LO, 0, 0, index));
    }
    else
    {
        insn.setDest(index);
        kernel->append(insn);
    }

}

/* Logical */
//-----------------------------------------------------------
Instruction Operand::operator~()
{
    if(type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for ~ operator");
    }
    return Instruction(_NOT, 0, index, 0);
}
//-----------------------------------------------------------
Instruction Operand::operator|(Operand op)
{
    if(op.type != TYPE_REGISTER || type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for | operator");
    }
    return Instruction(_OR, op.index, index, 0);
}
//-----------------------------------------------------------
Instruction Operand::operator|(unsigned short value)
{
    throw string("Unsupported operation |value ");
}
//-----------------------------------------------------------
void Operand::operator|=(Operand op)
{
    if(op.type != TYPE_REGISTER || type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for |= operator");
    }
    kernel->append(Instruction(_OR, op.index, index, index));
}
//-----------------------------------------------------------
Instruction Operand::operator&(Operand op)
{
    if(op.type != TYPE_REGISTER || type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for & operator");
    }
    return Instruction(_AND, op.index, index, 0);
}
//-----------------------------------------------------------
Instruction Operand::operator&(unsigned short value)
{
    throw string("Unsupported operation &value ");
}
//-----------------------------------------------------------
void Operand::operator&=(Operand op)
{
    if(op.type != TYPE_REGISTER || type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for &= operator");
    }
    kernel->append(Instruction(_AND, op.index, index, index));
}
//-----------------------------------------------------------
Instruction Operand::operator==(Operand op)
{
    if(op.type != TYPE_REGISTER || type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for == operator");
    }
    return Instruction(_EQ, op.index, index, 0);
}
//-----------------------------------------------------------
Instruction Operand::operator==(unsigned short value)
{
    throw string("Unsupported operation ==value ");
}
//-----------------------------------------------------------
Instruction Operand::operator<(Operand op)
{
    if(op.type != TYPE_REGISTER || type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for < operator");
    }
    return Instruction(_LT, op.index, index, 0);
}
//-----------------------------------------------------------
Instruction Operand::operator<(unsigned short value)
{
    throw string("Unsupported operation <value ");
}
//-----------------------------------------------------------
Instruction Operand::operator^(Operand op)
{
    if(op.type != TYPE_REGISTER || type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for ^ operator");
    }
    return Instruction(_XOR, op.index, index, 0);
}
//-----------------------------------------------------------
Instruction Operand::operator^(unsigned short value)
{
    throw string("Unsupported operation ^value ");
}
//-----------------------------------------------------------
void Operand::operator^=(Operand op)
{
    if(op.type != TYPE_REGISTER || type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for ^= operator");
    }
    kernel->append(Instruction(_XOR, op.index, index, index));
}
//-----------------------------------------------------------
Operand Operand::operator[](Operand op) {
    if(type != TYPE_LS_DESCRIPTOR) {
        throw string("You can only apply the [] operator to the "
                     "special LS descriptor");
    }
    return Operand(TYPE_LOCAL_STORE, op.index, false /*immediate*/, kernel);
}
//-----------------------------------------------------------
Operand Operand::operator[](unsigned short value) {
    if(type != TYPE_LS_DESCRIPTOR) {
        throw string("You can only apply the [] operator to "
                     "the special LS descriptor");
    }
    if(value < 0 || value >= CONNEX_MEM_SIZE) {
        throw string("Address value outside memory range "
                     "in [value] operator");
    }
    return Operand(TYPE_LOCAL_STORE, value, true /*immediate*/, kernel);
}
//-----------------------------------------------------------
Instruction Operand::operator<<(Operand op)
{
    if(op.type != TYPE_REGISTER || type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for << operator");
    }
    return Instruction(_SHL, op.index, index, 0);
}
//-----------------------------------------------------------
Instruction Operand::operator<<(unsigned short value)
{
    if(type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for << value operator");
    }
    return Instruction(_ISHL, value, index, 0);
}
//-----------------------------------------------------------
void Operand::operator<<=(unsigned short value) {
    kernel->append(Instruction(_ISHL, value, index, index));
}
//-----------------------------------------------------------
void Operand::operator<<=(Operand op) {
    if(op.type != TYPE_REGISTER || type != TYPE_REGISTER) {
        throw string("Invalid operand type for >> operator");
    }
    kernel->append(Instruction(_SHL, op.index, index, index));
}
//-----------------------------------------------------------
Instruction Operand::operator>>(Operand op)
{
    if(op.type != TYPE_REGISTER || type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for >> operator");
    }
    return Instruction(_SHR, op.index, index, 0);
}
//-----------------------------------------------------------
Instruction Operand::operator>>(unsigned short value)
{
    if(type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for >> value operator");
    }
    return Instruction(_ISHR, value, index, 0);
}
//-----------------------------------------------------------
void Operand::operator>>=(unsigned short value) {
    kernel->append(Instruction(_ISHR, value, index, index));
}
//-----------------------------------------------------------
void Operand::operator>>=(Operand op) {
    if(op.type != TYPE_REGISTER || type != TYPE_REGISTER) {
        throw string("Invalid operand type for >> operator");
    }
    kernel->append(Instruction(_SHR, op.index, index, index));
}
//-----------------------------------------------------------
Instruction Operand::addc(Operand op1, Operand op2)
{
    if(op1.type != TYPE_REGISTER || op2.type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for addc operation");
    }
    return Instruction(_ADDC, op1.index, op2.index, 0);
}
//-----------------------------------------------------------
Instruction Operand::addc(Operand op1, unsigned short value)
{
    throw string("Unsupported operation addc(operand, value) ");
}
//-----------------------------------------------------------
Instruction Operand::subc(Operand op1, Operand op2)
{
    if(op1.type != TYPE_REGISTER || op2.type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for subc operator");
    }
    return Instruction(_SUBC, op2.index, op1.index, 0);
}
//-----------------------------------------------------------
Instruction Operand::subc(Operand op1, unsigned short value)
{
    throw string("Unsupported operation subc(operand, value) ");
}
//-----------------------------------------------------------
Instruction Operand::shra(Operand op1, Operand op2)
{
    if(op1.type != TYPE_REGISTER || op2.type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for shra operator");
    }
    return Instruction(_SHRA, op2.index, op1.index, 0);
}
//-----------------------------------------------------------
Instruction Operand::ishra(Operand op1, unsigned short value)
{
    if(op1.type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for ishra operator");
    }
    return Instruction(_ISHRA, value, op1.index, 0);
}
//-----------------------------------------------------------
Instruction Operand::multhi()
{
    return Instruction(_MULT_HI, 0, 0, 0);
}
//-----------------------------------------------------------
Instruction Operand::multlo()
{
    return Instruction(_MULT_LO, 0, 0, 0);
}
//-----------------------------------------------------------
void Operand::cellshl(Operand op1, Operand op2)
{
    if(op1.type != TYPE_REGISTER || op2.type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for cellshl operator");
    }
    op1.kernel->append(Instruction(_CELL_SHL, op2.index, op1.index, 0));
}
//-----------------------------------------------------------
void Operand::cellshr(Operand op1, Operand op2)
{
    if(op1.type != TYPE_REGISTER || op2.type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for cellshr operator");
    }
    op1.kernel->append(Instruction(_CELL_SHR, op2.index, op1.index, 0));
}
//-----------------------------------------------------------
Instruction Operand::ult(Operand op1, Operand op2)
{
    if(op1.type != TYPE_REGISTER || op2.type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for cellshr operator");
    }
    return Instruction(_ULT, op2.index, op1.index, 0);
}
//-----------------------------------------------------------
Instruction Operand::ult(Operand op1, unsigned short value)
{
    throw string("Unsupported operation ult (operand, value)");
}
//-----------------------------------------------------------
Instruction Operand::popcnt(Operand op)
{
    if(op.type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for popcnt operator");
    }
    return Instruction(_POPCNT, 0, op.index, 0);
}
//-----------------------------------------------------------
Instruction Operand::bitreverse(Operand op) {
    if(op.type != TYPE_REGISTER) {
        throw string("Invalid operand type for popcnt operator");
    }
    return Instruction(_BIT_REVERSE, 0, op.index, 0);
}
//-----------------------------------------------------------
void Operand::reduce(Operand op)
{
    if(op.type != TYPE_REGISTER)
    {
        throw string("Invalid operand type for reduce operator");
    }
    op.kernel->append(Instruction(_REDUCE, 0, op.index, 0));
}
