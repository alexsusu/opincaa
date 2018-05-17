/************************************************************
 * File:   Instruction.h
 *
 * This is the header file for a class containing one Connex
 * Array instruction
 * 
 */

#include "Instruction.h"
#include <string>
#include <stdio.h>
#include <sstream>


/************************************************************
* Returns a string representation of specified opcode
*/
string Instruction::mnemonic(int opcode) {
    switch (opcode) {
        case _PRINT_REG: return string("print_reg"); // Alex
        case _PRINT_CHARS: return string("print_chars"); // Alex
        case _QUIT: return string("quit"); // Alex

        case _ADD:       return string("add");
        case _ADDC:      return string("addc");
        case _SUB:       return string("sub");
        case _SUBC:      return string("subc");

        case _POPCNT:    return string("popcnt");

        case _BIT_REVERSE: return string("bitreverse");
        case _NOT:       return string("not");
        case _OR:        return string("or");
        case _AND:       return string("and");
        case _XOR:       return string("xor");
        case _EQ:        return string("eq");
        case _LT:        return string("lt");
        case _ULT:       return string("ult");
        case _SHL:       return string("shl");
        case _SHR:       return string("shr");
        case _SHRA:      return string("shra");
        case _ISHL:      return string("ishl");
        case _ISHR:      return string("ishr");
        case _ISHRA:     return string("ishra");

        case _LDIX:      return string("ldix");
        case _LDSH:      return string("ldsh");
        case _CELL_SHL:  return string("cellshl");
        case _CELL_SHR:  return string("cellshr");

        case _READ:      return string("read");
        case _WRITE:     return string("write");

        case _MULT:      return string("mult");
        case _MULT_LO:   return string("multlo");
        case _MULT_HI:   return string("multhi");

        case _WHERE_CRY: return string("wherecry");
        case _WHERE_EQ:  return string("whereeq");
        case _WHERE_LT:  return string("wherelt");
        case _END_WHERE: return string("endwhere");
        case _REDUCE:    return string("reduce");
        case _SETLC_REDUCE:    return string("setlc_reduce");
        case _NOP:       return string("nop");

        case _VLOAD:     return string("vload");
        case _IREAD:     return string("iread");
        case _IWRITE:    return string("iwrite");

        case _SETLC:     return string("setlc");
        case _IJMPNZ:    return string("ijmpnz");

        default:
           throw string("Unknown opcode in Instruction::mnemonic");
    }
}

/*************************************************************************
* Returns a string representation of specified register
*/
string Instruction::registerName(int registerIndex) {
    char reg[5];

    /* Alex: TODO: maybe we should check if registerIndex > (1 << DEST_SIZE)
     and give some warning at least - this should happen only for method
     Kernel::genLLVMISelManualCode(), which requires more
     CONNEX_REG_COUNT bigger than 32 (for the SSA-like transformation). */
    sprintf(reg, "R%d", registerIndex);

    return string(reg);
}

/************************************************************
* Constructor for creating a new Instruction
*
* @param instruction the 32 bits used to create the instruction
*
* @throws string if the Instruction's opcode is invalid
*/
Instruction::Instruction(unsigned instruction) {
    /* Type of the instruction (with, or without immediate value) */
    switch ((instruction >> TYPE_BIT_INDEX) & 1) {
        case 0: {
            type = INSTRUCTION_TYPE_NO_VALUE;
            opcode = GET_OPCODE_9BITS(instruction);
            right = GET_RIGHT(instruction);
            break;
        }
        case 1: {
            type = INSTRUCTION_TYPE_WITH_VALUE;
            opcode = GET_OPCODE_6BITS(instruction);
            value = GET_IMM(instruction);
            /* printf("Instruction::Instruction(unsigned instruction): "
                      "type = %d, opcode = %d, value = %d\n",
                      type, opcode, value); */
            // Alex: 2017_08_26: we sign extend the short (i16) to int (i32)
            if (value >= 0x8000)
                value |= 0xFFFF0000;
            break;
        }
    }

    if (type_for_opcode[opcode] != type) {
        throw string("Invalid type in Instruction::Instruction(instruction = " +
                     to_string(instruction) + "): type = " + to_string(type) +
                     " and type_for_opcode[opcode = " + to_string(opcode) +
                     "] = " + to_string(type_for_opcode[opcode]));
    }

    /* Alex: we avoid putting this code here for efficiency reasons:
       if (opcode == ) left = -1;
    */
    left  = GET_LEFT(instruction);
    /* Alex: we avoid putting this code here for efficiency reasons:
       if (opcode == CELL_SH...) dest = -1;
    */
    dest = GET_DEST(instruction);
}

/************************************************************
* Constructor for creating a new Instruction
*
* @param opcode the 9 or 6 bits opcode
* @param rightOrValue the 5 or 16 bits corresponding to right or value operand (only the least significat bits are used)
* @param left the 5 bits corresponding to left operand (only the least significat bits are used)
* @param dest the 5 bits corresponding to dest operand (only the least significat bits are used)
*
* @throws string if the opcode is not valid
*/
Instruction::Instruction(int opcode, int rightOrValue, int left, int dest) {
    type = type_for_opcode[opcode];

    this->opcode = opcode;

    switch (type) {
        case INSTRUCTION_TYPE_NO_VALUE:
            right = rightOrValue & ((1 << RIGHT_SIZE) - 1);
            break;
        case INSTRUCTION_TYPE_WITH_VALUE: {
            //printf("Instruction::Instruction(int opcode,...)\n");
            value = rightOrValue & ((1 << IMMEDIATE_VALUE_SIZE) - 1);
            // Alex: 2017_08_26: we sign extend the short (i16) to int (i32)
            if (value >= 0x8000)
                value |= 0xFFFF0000;
            break;
        }
        default:
            //throw string("Unknown type in Instruction::Instruction(int, int, int, int)");
            throw string("Unknown type in Instruction::Instruction(") +
                         std::to_string(opcode) + string(", ") +
                         std::to_string(rightOrValue) + string(", ") +
                         std::to_string(left) + string(", ") +
                         std::to_string(dest) + ")";
    }

    this->left = left & ((1 << LEFT_SIZE) - 1);
    this->dest = dest & ((1 << DEST_SIZE) - 1);

    /* printf("Instruction::Instruction(opcode = %d): left = %d, right = %d, "
              "dest = %d (type = %d)\n",
              opcode, left, right, dest, type); // Alex
    */
}

/************************************************************
// Alex: we comment in the .cpp file the definition of assemble()
//       and put it in the header because of the inline qualifier.
* Returns the 32bit word representing the assembled instruction
*
* @return the 32bit word representing the assembled instruction

extern inline unsigned Instruction::assemble() {
    unsigned instruction;

    switch(type) {
        case INSTRUCTION_TYPE_NO_VALUE:
            instruction = opcode << OPCODE_9BITS_POS;
            instruction |= right << RIGHT_POS;
            break;
        case INSTRUCTION_TYPE_WITH_VALUE:
            instruction = opcode << OPCODE_6BITS_POS;

            // 2017_08_26
            //instruction |= value << IMMEDIATE_VALUE_POS;
            // Alex: Taking into consideration the fact value is a sign extended int
            instruction |= ((value & 0xFFFF) << IMMEDIATE_VALUE_POS);

            break;
        default:
            throw string("Illegal instruction-type in Instruction::assemble");
    }

    instruction |= left << LEFT_POS;
    instruction |= dest << DEST_POS;

  #ifdef PRINT_DEBUG_INFO_ASSEMBLY_INSTRUCTION
    // Alex: new code
    printf("Instruction::assemble(): instruction = 0x%X\n", instruction);
    //printf("  (mnemonic = %s)\n", (this->mnemonic(opcode)).c_str());
    printf("  which means: %s", (this->disassemble()).c_str());
    switch(type) {
        case INSTRUCTION_TYPE_NO_VALUE:
            printf("  opcode = 0x%X\n", opcode);
            printf("  right = %d\n", right);
            break;
        case INSTRUCTION_TYPE_WITH_VALUE:
            printf("  opcode = %d\n", opcode);
            printf("  value = %d\n", value);
            break;
    }
    printf("  left = %d\n", left);
    printf("  dest = %d\n", dest);
    // Alex: END new code
  #endif

    return instruction;
}
*/


/**
 * Returns the string representing the dissasembled instruction.
 */
string Instruction::disassemble() {
    stringstream stream;

  #ifdef ALEX_DEBUG
    printf("Instruction::disassemble(): %d\n", opcode);
    fflush(stdout);
  #endif

    stream << mnemonic(opcode);

    switch (opcode) {
        case _ADD:
        case _ADDC:
        case _SUB:
        case _SUBC:
        case _OR:
        case _AND:
        case _XOR:
        case _EQ:
        case _LT:
        case _ULT:
        case _SHL:
        case _SHR:
        case _SHRA:
        case _CELL_SHL:
        case _CELL_SHR:
            stream << " " << registerName(dest);
            stream << " " << registerName(left);
            stream << " " << registerName(right);
            break;
        case _BIT_REVERSE:
        case _NOT:
            stream << " " << registerName(dest);
            stream << " " << registerName(left);
            break;
        case _READ:
            stream << " " << registerName(dest);
            stream << " " << registerName(right);
            break;
        case _ISHL:
        case _ISHR:
        case _ISHRA:
            stream << " " << registerName(dest);
            stream << " " << registerName(left);
            stream << " " << right;
            break;
        case _LDIX:
        case _LDSH:
        case _MULT_LO:
        case _MULT_HI:
            stream << " " << registerName(dest);
            break;
        case _PRINT_REG: // Alex
        case _REDUCE:
        case _SETLC_REDUCE:
        case _POPCNT:
            stream << " " << registerName(left);
            break;
        case _WRITE:
        case _MULT:
            stream << " " << registerName(left);
            stream << " " << registerName(right);
            break;
        case _VLOAD:
            stream << " " << registerName(dest);
            stream << ", " << value;
            break;
        case _IREAD:
            stream << " " << registerName(dest);
            stream << " (" << value << ")";
            break;
        case _IWRITE:
            stream << " (" << value << ")";
            stream << " " << registerName(left);
            break;
        case _PRINT_CHARS:
            stream << " (numeric value) " << value; // Alex
            break;
        case _SETLC:
            //stream << " (" << value << ")";
            stream << " " << value;
            break;
            /* fall through */
        case _WHERE_CRY:
        case _WHERE_EQ:
        case _WHERE_LT:
        case _END_WHERE:
        case _IJMPNZ:
        case _NOP:
        case _QUIT:
            break;
        default:
            throw string("Invalid instruction opcode!");
    }

    stream << ";" << endl;

    return stream.str();
}

/************************************************************
 * Returns the string representing the dumped instruction in
 *   OPINCAA format.
 *
 * @return string representing the dumped instruction
 */
string Instruction::dump() {
    stringstream stream;

    switch (opcode) {
        case _PRINT_REG: // Alex
            stream << "PRINTREG(" << registerName(left) << ")";
            break;
        case _PRINT_CHARS: // Alex
            stream << "PRINTCHARS((numeric value)" << value << ")";
            break;
        case _QUIT: // Alex
            stream << "QUIT";
            break;
             // We print nothing in the end
             //return stream.str(); break;
        case _ADD:
            stream << registerName(dest) << " = " << registerName(left)
                   << " + " << registerName(right);
            break;
        case _ADDC:
            stream << registerName(dest) << " = ADDC(" << registerName(left)
                   << ", " << registerName(right) << ")";
            break;
        case _SUB:
            stream << registerName(dest) << " = " << registerName(left)
                   << " - " << registerName(right);
            break;
        case _SUBC:
            stream << registerName(dest) << " = SUBC(" << registerName(left)
                   << ", " << registerName(right) << ")";
            break;
        case _NOT:
            stream << registerName(dest) << " = ~" << registerName(left);
            break;
        case _BIT_REVERSE:
            stream << registerName(dest) << " = BITREVERSE("
                   << registerName(left) << ")";
            break;
        case _OR:
            stream << registerName(dest) << " = " << registerName(left)
                   << " | " << registerName(right);
            break;
        case _AND:
            stream << registerName(dest) << " = " << registerName(left)
                   << " & " << registerName(right);
            break;
        case _XOR:
            stream << registerName(dest) << " = " << registerName(left)
                   << " ^ " << registerName(right);
            break;
        case _EQ:
            stream << registerName(dest) << " = " << registerName(left)
                   << " == " << registerName(right);
            break;
        case _LT:
            stream << registerName(dest) << " = " << registerName(left)
                   << " < " << registerName(right);
            break;
        case _ULT:
            stream << registerName(dest) << " = ULT(" << registerName(left)
                   << ", " << registerName(right) << ")";
            break;
        case _SHL:
            stream << registerName(dest) << " = " << registerName(left)
                   << " << " << registerName(right);
            break;
        case _SHR:
            stream << registerName(dest) << " = " << registerName(left)
                   << " >> " << registerName(right);
            break;
        case _SHRA:
            stream << registerName(dest) << " = SHRA(" << registerName(left)
                   << ", " << registerName(right) << ")";
            break;
        case _ISHL:
            stream << registerName(dest) << " = " << registerName(left)
                   << " << " << right;
            break;
        case _ISHR:
            stream << registerName(dest) << " = " << registerName(left)
                   << " >> " << right;
            break;
        case _ISHRA:
            stream << registerName(dest) << " = ISHRA(" << registerName(left)
                   << ", " << right << ")";
            break;
        case _LDIX:
            stream << registerName(dest) << " = INDEX";
            break;

        case _LDSH:
            stream << registerName(dest) << " = SHIFT_REG";
            break;
        case _CELL_SHL:
            stream << "CELL_SHL(" << registerName(left) << ", "
                   << registerName(right) << ")";
            break;
        case _CELL_SHR:
            stream << "CELL_SHR(" << registerName(left) << ", "
                   << registerName(right) << ")";
            break;

        case _READ:
            stream << registerName(dest) << " = LS["
                   << registerName(right) << "]";
            break;
        case _WRITE:
            stream << "LS[" << registerName(right) << "] = "
                   << registerName(left);
            break;
        case _MULT:
            stream << registerName(left) << " * " << registerName(right);
            break;
        case _MULT_LO:
            stream << registerName(dest) << " = MULT_LOW()";
            break;
        case _MULT_HI:
            stream << registerName(dest) << " = MULT_HIGH()";
            break;
        case _WHERE_CRY:
            stream << "WHERE_CRY";
            break;
        case _WHERE_EQ:
            stream << "WHERE_EQ";
            break;
        case _WHERE_LT:
            stream << "WHERE_LT";
            break;
        case _END_WHERE:
            stream << "END_WHERE";
            break;

        case _POPCNT:
            stream << registerName(dest) << " = POPCNT("
                   << registerName(left) << ")";
            break;

        case _VLOAD:
            stream << registerName(dest) << " = " << value;
            break;

        case _IREAD:
            stream << registerName(dest) << " = LS[" << value << "]";
            break;
        case _IWRITE:
            stream << "LS[" << value << "] = " << registerName(left);
            break;

        case _REDUCE:
            stream << "REDUCE(" << registerName(left) << ")";
            break;
        case _SETLC_REDUCE:
            stream << "REPEAT_REDUCE(" << registerName(left) << ")";
            break;

        case _NOP:
            stream << "NOP";
            break;

        case _SETLC:
            stream << "REPEAT(" << value << ")";
            break;
        case _IJMPNZ:
            stream << "END_REPEAT";
            break;
        default:
            throw string("Invalid instruction opcode!");
    }

    stream << ";" << endl;

    return stream.str();
}

/************************************************************
* Returns a string representation of this instruction
*
* @return the 32bit word representing the assembled instruction
*/
string Instruction::toString() {
    char desc[100];

    switch(type) {
        case INSTRUCTION_TYPE_NO_VALUE:
            sprintf(desc, "(0x%x), Right=%d, Left=%d, Dest=%d",
                          opcode, right, left, dest);
            break;
        case INSTRUCTION_TYPE_WITH_VALUE:
            sprintf(desc, "(0x%x), Value=%d, Left=%d, Dest=%d",
                          opcode, value, left, dest);
            break;
    }

    return mnemonic(opcode) + string(desc);
}

/************************************************************
 * Getter for type
 */
int Instruction::getType()
{
    return type;
}

/************************************************************
 * Getter for opcode
 */
int Instruction::getOpcode()
{
    return opcode;
}
/************************************************************
 * Getter for left register operand
 */
int Instruction::getLeft() {
    // Alex
    if (opcode == _NOP ||
        opcode == _IREAD ||
        opcode == _READ ||
        opcode == _VLOAD ||
        opcode == _LDIX ||
        opcode == _END_WHERE ||
        opcode == _WHERE_CRY ||
        opcode == _WHERE_EQ ||
        opcode == _WHERE_LT ||
        opcode == _MULT_LO ||
        opcode == _MULT_HI ||
        opcode == _LDSH ||
        opcode == _SETLC ||
        opcode == _IJMPNZ)
        return -1;

    return left;
}

/************************************************************
 * Getter for right register operand
 */
int Instruction::getRight() {
    // Alex
    if (opcode == _NOP ||
        opcode == _REDUCE ||
        opcode == _IWRITE ||
        opcode == _IREAD ||
        opcode == _VLOAD ||
        opcode == _LDIX ||
        opcode == _END_WHERE ||
        opcode == _WHERE_CRY ||
        opcode == _WHERE_EQ ||
        opcode == _WHERE_LT ||
        opcode == _MULT_LO ||
        opcode == _MULT_HI ||
        opcode == _LDSH ||
        opcode == _POPCNT ||
        opcode == _NOT ||
        opcode == _SETLC ||
        opcode == _IJMPNZ)
        return -1;

    return right;
}

/************************************************************
 * Getter for dest register operand
 */
int Instruction::getDest() {
    // Alex
    if (opcode == _NOP ||
        opcode == _REDUCE ||
        opcode == _IWRITE ||
        opcode == _WRITE ||
        opcode == _END_WHERE ||
        opcode == _WHERE_CRY ||
        opcode == _WHERE_EQ ||
        opcode == _WHERE_LT ||
        opcode == _MULT ||
        opcode == _CELL_SHL ||
        opcode == _CELL_SHR ||
        opcode == _SETLC ||
        opcode == _IJMPNZ)
        return -1;

    return dest;
}

/************************************************************
 * Getter for value
 */
// Alex: 2017_08_26
//short Instruction::getValue() {
int Instruction::getValue() {
    return value;
}

/************************************************************
 * Setter for left
 */
void Instruction::setLeft(int left)
{
    this->left = left;
}

/************************************************************
 * Setter for opcode
 */
void Instruction::setRight(int right)
{
    this->right = right;
}

/************************************************************
 * Setter for opcode
 */
void Instruction::setDest(int dest)
{
    this->dest = dest;
}

/************************************************************
 * Setter for opcode
 */
void Instruction::setValue(int value)
{
    this->value = value;
}

