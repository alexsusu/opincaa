/*
 * File:   Instruction.h
 *
 * This is the header file for a class containing one Connex
 * Array instruction
 * 
 */

#include "opcodes.h"
#include "Instruction.h"
#include <string>
#include <stdio.h>

/*
* Returns a string representation of specified opcode
*/
string Instruction::mnemonic(int opcode)
{
    switch(opcode)
    {
        case _ADD:      return string("add");
        case _ADDC:     return string("addc");
        case _SUB:      return string("sub");      
        case _SUBC:     return string("subc");
        
        case _NOT:      return string("not");
        case _OR:       return string("or");
        case _AND:      return string("and");
        case _XOR:      return string("xor");
        case _EQ:       return string("eq");
        case _LT:       return string("lt");
        case _ULT:      return string("ult");
        case _SHL:      return string("shl");
        case _SHR:      return string("shr");
        case _SHRA:     return string("shra");
        case _ISHL:     return string("ishl");
        case _ISHR:     return string("ishr");
        case _ISHRA:    return string("ishra");
        
        case _LDIX:     return string("ldix");
        case _LDSH:     return string("ldsh");
        case _CELL_SHL: return string("cellshl");
        case _CELL_SHR: return string("cellshr");
        
        case _READ:     return string("read");
        case _WRITE:    return string("write");
        
        case _MULT:     return string("mult");
        case _MULT_LO:  return string("multlo");
        case _MULT_HI:  return string("multhi");
        
        case _WHERE_CRY:return string("wherecry");
        case _WHERE_EQ: return string("whereeq");
        case _WHERE_LT: return string("wherelt");
        case _END_WHERE:return string("endwhere");
        case _REDUCE:   return string("reduce");
        case _NOP:      return string("nop");
        
        case _VLOAD:    return string("vload");
        case _IREAD:    return string("iread");
        case _IWRITE:   return string("iwrite");
        
        default: throw string("Unknown opcode in Instruction::mnemonic");
    }
}
        
/*************************************************************************
* Returns a string representation of specified register
*/
string Instruction::registerName(int register_index)
{
    char reg[4];
    sprintf(reg, "R%d", (register_index & ((1 << DEST_SIZE) - 1)));
    return string(reg);
}
        
/*
* Constructor for creating a new Instruction
* 
* @param instruction the 32 bits used to create the instruction
* 
* @throws string if the Instruction's opcode is invalid
*/ 
Instruction::Instruction(unsigned instruction)
{
    /* Type of the instruction (with, or without immediate value) */
    switch((instruction >> TYPE_BIT_INDEX) & 1)
    {
        case 0: 
            type = INSTRUCTION_TYPE_NO_VALUE;
            opcode = GET_OPCODE_9BITS(instruction);
            right = GET_RIGHT(instruction);
            break;
        case 1:
            type = INSTRUCTION_TYPE_WITH_VALUE;
            value = GET_IMM(instruction);
            break;
    }
    
    if(type_for_opcode[opcode] != type)
    {
        throw string("Invalid type in Instruction::Instruction(unsigned)");
    }
    
    left  = GET_LEFT(instruction);
    dest = GET_DEST(instruction);
}
        
/*
* Constructor for creating a new Instruction
* 
* @param opcode the 9 or 6 bits opcode
* @param rightOrValue the 5 or 16 bits corresponding to right or value operand (only the least significat bits are used)
* @param left the 5 bits corresponding to left operand (only the least significat bits are used)
* @param dest the 5 bits corresponding to dest operand (only the least significat bits are used)
* 
* @throws string if the opcode is not valid
*/ 
Instruction::Instruction(int opcode, int rightOrValue, int left, int dest)
{
    type = type_for_opcode[opcode];
    this->opcode = opcode;
    switch(type)
    {
        case INSTRUCTION_TYPE_NO_VALUE: 
            right = rightOrValue & ((1 << RIGHT_SIZE) - 1);
            break;
        case INSTRUCTION_TYPE_WITH_VALUE:
            value = rightOrValue & ((1 << IMMEDIATE_VALUE_SIZE) - 1);
            break;
        default:
            throw string("Unknown type in Instruction::Instruction(int, int, int, int)");
    }

    this->left = left & ((1 << LEFT_SIZE) - 1);
    this->dest = dest & ((1 << DEST_SIZE) - 1);
}
        
/*
* Returns the 32bit word representing the assembled instruction
* 
* @return the 32bit word representing the assembled instruction
*/
unsigned Instruction::assemble()
{
    unsigned instruction;
    switch(type)
    {
        case INSTRUCTION_TYPE_NO_VALUE: 
            instruction = opcode << OPCODE_9BITS_POS;
            instruction |= right << RIGHT_POS;
            break;
        case INSTRUCTION_TYPE_WITH_VALUE:
            instruction = opcode << OPCODE_9BITS_POS;
            instruction |= value << IMMEDIATE_VALUE_POS;
            break;
    }
    
    instruction |= left << LEFT_POS;
    instruction |= dest << DEST_POS;
    
    return instruction;
}
        
/*
* Returns a string representation of this instruction
* 
* @return the 32bit word representing the assembled instruction
*/
string Instruction::toString()
{
    char desc[100];
    switch(type)
    {
        case INSTRUCTION_TYPE_NO_VALUE: 
            sprintf(desc, "(0x%x), Right=%d, Left=%d, Dest=%d", opcode, right, left, dest);
            break;
        case INSTRUCTION_TYPE_WITH_VALUE: 
            sprintf(desc, "(0x%x), Value=%d, Left=%d, Dest=%d", opcode, value, left, dest);
            break;
    }
    
    return mnemonic(opcode) + string(desc);
}

/*
 * Getter for type
 */
int Instruction::getType()
{
    return type;
}

/*
 * Getter for opcode
 */
int Instruction::getOpcode()
{
    return opcode;
}
/*
 * Getter for left
 */
int Instruction::getLeft()
{
    return left;
}

/*
 * Getter for right
 */
int Instruction::getRight()
{
    return right;
}

/*
 * Getter for dest
 */
int Instruction::getDest()
{
    return dest;
}

/*
 * Getter for value
 */
int Instruction::getValue()
{
    return value;
}

/*
 * Setter for left
 */
void Instruction::setLeft(int left)
{
    this->left = left;
}

/*
 * Setter for opcode
 */
void Instruction::setRight(int right)
{
    this->right = right;
}

/*
 * Setter for opcode
 */
void Instruction::setDest(int dest)
{
    this->dest = dest;
}

/*
 * Setter for opcode
 */
void Instruction::setValue(int value)
{
    this->value = value;
}
