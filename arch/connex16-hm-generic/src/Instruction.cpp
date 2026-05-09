#include "Instruction.h"
#include <assert.h>
#include <string>
#include <stdio.h>
#include <sstream>


/************************************************************
* Returns a string representation of specified opcode
*/
string Instruction::mnemonic(int opcode) {
    switch (opcode) {
        case _PRINT_REG: return string("print_reg");
        case _PRINT_CHARS: return string("print_chars");
        case _QUIT:      return string("quit"); // Alex

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
        case _MULT_U:    return string("mult_u");
        case _MULT_LO:   return string("multlo");
        case _MULT_HI:   return string("multhi");
        //case _MULT_HI_U: return string("multhi_u");

        case _WHERE_CRY: return string("wherecry");
        case _WHERE_EQ:  return string("whereeq");
        case _WHERE_LT:  return string("wherelt");
        case _END_WHERE: return string("endwhere");
        case _REDUCE:    return string("red");
        case _REDUCE_U:  return string("red_u");
        case _SCAN:      return string("scan");
        case _NOP:       return string("nop");

        case _VLOAD:     return string("vload");
        case _IREAD:     return string("iread");
        case _IWRITE:    return string("iwrite");

        case _SETLC:     return string("setlc");
        case _SETLC_REDUCE:  return string("setlc_reduce");
        //case _SETLC_REDUCE_NOTNULL: return string("setlc_reduce_notnull");
        case _IJMPNZ:    return string("ijmpnz");
        case _IJMPNZ_RED:return string("ijmpnz_red");


        case _DISABLE_CELL:     return string("disableCell");
        case _ENABLE_ALL_CELLS: return string("enableAllCells");

        default:
           throw string("Unknown opcode in Instruction::mnemonic");
    }
}

/*************************************************************************
* Returns a string representation of specified register
*/
string Instruction::registerName(int registerIndex) {
    char reg[10];

    /* small-TODO: maybe we should check if registerIndex > (1 << DEST_SIZE)
     and give some warning at least - this should happen only for method
     Kernel::genLLVMISelManualCode(), which requires
     CONNEX_REG_COUNT bigger than 32 (for the SSA-like transformation)
       - anyhow for genLLVMISelManualCode() it is safe to have
         CONNEX_REG_COUNT > (1 << DEST_SIZE). */

    //sprintf(reg, "R%d", registerIndex);
    sprintf(reg, "R%02d", registerIndex);

    return string(reg);
}

/************************************************************
* Constructor for creating a new Instruction
*
* @param instruction the 32 bits used to create the instruction
*
* @throws string if the Instruction's opcode is invalid
*/
Instruction::Instruction(InstructionType instruction) {
    /* IMPORTANT NOTE: Due to special ISA design, the instructions
     *   _ISHL, _ISHR, _ISHRA are of type 1 (INSTRUCTION_TYPE_NO_IMMEDIATE),
     *   but they pass the immediate operand as right register index,
     *        with possible values 0..31, which is enough.
     *
     *   We could treat this special case here, but Instruction::Instruction()
     *      is performance critical (used by the runtime assembler) so we
     *      treat this case in getRight() and getValue().
     */

    /* Type of the instruction (with, or without immediate value) */
    switch ((instruction >> TYPE_BIT_INDEX) & 1) {
        case 0: {
            type = INSTRUCTION_TYPE_NO_IMMEDIATE;
            opcode = GET_OPCODE_9BITS(instruction);

            // For efficiency this is NOT executed
            //value = -1000000;

            right = GET_RIGHT(instruction);
            break;
        }
        case 1: {
            type = INSTRUCTION_TYPE_WITH_IMMEDIATE;
            opcode = GET_OPCODE_6BITS(instruction);

            // For efficiency this instruction could be (in principle) removed
            //right = NO_REG_INDEX; // 2018_06_04 - I think this is safe
            /* Note: Instructions with imm operand can have left register -
               e.g., ISHR/L, IREAD/IWRITE */

            value = GET_IMM(instruction);
            // We sign extend the short (i16) to int (i32)
            if (value >= 0x8000) {
              #ifdef CONNEX_REG_COUNT_512
                value |= 0xFFFFFFFFFFFF0000;
              #else
                value |= 0xFFFF0000;
              #endif
            }
            break;
        }
    }

    /* We avoid putting this code here for efficiency reasons:
       if (opcode == ) left = -1;
    */
    left  = GET_LEFT(instruction);

    /* We avoid putting this code here for efficiency reasons:
       if (opcode == CELL_SH...) dest = -1;
    */
    dest = GET_DEST(instruction);

  #ifdef DEBUG_OPINCAA_INSTRUCTION_ASSEMBLE
   #ifdef CONNEX_REG_COUNT_512
    printf("Instruction::Instruction(InstructionType instruction = 0x%016X)\n",
              instruction);
    fflush(stdout);
    printf("Instruction::Instruction(InstructionType instruction): "
           "instruction = 0x%016X, type = %d, opcode = %d, dest = %d, value = %d, left = %d, right = %d\n",
              instruction, type, opcode, dest, value, left, right);
    fflush(stdout);
   #else
    printf("Instruction::Instruction(InstructionType instruction = 0x%08X)\n",
              instruction);
    fflush(stdout);
    printf("Instruction::Instruction(InstructionType instruction): "
           "instruction = 0x%08X, type = %d, opcode = %d, dest = %d, value = %d, left = %d, right = %d\n",
              instruction, type, opcode, dest, value, left, right);
    fflush(stdout);
   #endif
  #endif

    if (type_for_opcode[opcode] != type) {
        throw string("Invalid type in Instruction::Instruction(instruction = " +
                     std::to_string(instruction) + "): type = " + std::to_string(type) +
                     " and type_for_opcode[opcode = " + std::to_string(opcode) +
                     "] = " + std::to_string(type_for_opcode[opcode]));
    }
}

/************************************************************
* IMPORTANT: This is a performance critical method, but its implementation
*   is in a sense very efficient - but we gain from caching the kernel
*   and running it e.g. 100 times in a row.
*
* Constructor for creating a new Instruction
*
* @param opcode the 9 or 6 bits opcode
* @param rightOrValue the 5 or 16 bits corresponding to right or value operand (only the least significant bits are used)
* @param left the 5 bits corresponding to left operand (only the least significant bits are used)
* @param dest the 5 bits corresponding to dest operand (only the least significant bits are used)
*
* @throws string if the opcode is not valid
*/
Instruction::Instruction(int anOpcode, int rightOrValue, int aLeft, int aDest) {
    type = type_for_opcode[anOpcode];

    this->opcode = anOpcode;

    switch (type) {
        case INSTRUCTION_TYPE_NO_IMMEDIATE:
            // 2018_08_19: right = rightOrValue & ((1 << RIGHT_SIZE) - 1);
            this->right = rightOrValue;
            assert(rightOrValue >= 0 && rightOrValue < (1UL << RIGHT_SIZE));
            break;
        case INSTRUCTION_TYPE_WITH_IMMEDIATE: {
            //printf("Instruction::Instruction(int opcode,...)\n");

            this->right = NO_REG_INDEX; // 2018_06_04: I think this is safe
            /* Note: Instructions with imm operand can have left register -
               e.g., ISHR/L, IREAD/IWRITE */

            /*
            value = rightOrValue;
            assert(value >= -(1 << (IMMEDIATE_VALUE_SIZE - 1)) &&
                   value < ((1 << IMMEDIATE_VALUE_SIZE) - 1));
            */
            value = rightOrValue & ((1 << IMMEDIATE_VALUE_SIZE) - 1);
            // We sign extend the short (i16) to int (i32)
            if (value >= 0x8000) {
              #ifdef CONNEX_REG_COUNT_512
                value |= 0xFFFFFFFFFFFF0000;
              #else
                value |= 0xFFFF0000;
              #endif
            }

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

    /*
    printf("Instruction::Instruction(InstructionType instruction): "
              "type = %d, opcode = %d, value = %d\n",
              type, opcode, value);
    */

    // 2018_08_19: this->left = left & ((1 << LEFT_SIZE) - 1);
    this->left = aLeft;
    /*
    printf("Instruction::Instruction(opcode = %d (mnemonic = %s)): aLeft = %d, "
              "value = %ld (type = %d)\n",
              anOpcode, this->mnemonic(opcode).c_str(), aLeft, value, type);
    printf("getValue() = %ld\n", this->getValue());
    fflush(stdout);
    */
    assert(aLeft >= 0 && aLeft < (1UL << LEFT_SIZE));
    //
    // 2018_08_19: this->dest = dest & ((1 << DEST_SIZE) - 1);
    this->dest = aDest;
    assert(aDest >= 0 && aDest < (1UL << DEST_SIZE));

    /*
    printf("Instruction::Instruction(opcode = %d): left = %d, right = %d, "
              "dest = %d, value = %d (type = %d)\n",
              opcode, left, right, dest, value, type);
    printf("getValue() = %ld\n", this->getValue());
    */
}


/**
 * Returns the string representing the disassembled instruction.
 */
string Instruction::disassemble() {
    stringstream stream;

  #ifdef DEBUG_OPINCAA_EXTRA
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
            stream << " " << registerName(dest);
            stream << " " << registerName(left);
            stream << " " << registerName(right);
            break;
        case _CELL_SHL:
        case _CELL_SHR:
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
        case _PRINT_REG:
        case _REDUCE:
        case _REDUCE_U:
        case _SCAN:
        case _SETLC_REDUCE:
        //case _SETLC_REDUCE_NOTNULL:
        case _POPCNT:
            stream << " " << registerName(left);
            break;
        case _WRITE:
        case _MULT:
        case _MULT_U:
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
            stream << " (numeric value) 0x" << std::hex << value << std::dec;
            break;
        case _SETLC:
            //stream << " (" << value << ")";
            stream << " " << value;
            break;
        case _WHERE_CRY:
        case _WHERE_EQ:
        case _WHERE_LT:
        case _END_WHERE:
        case _IJMPNZ:
        case _NOP:
        case _QUIT:
            break;

        case _IJMPNZ_RED:
            stream << " " << registerName(left);
            break;

        case _DISABLE_CELL:
        case _ENABLE_ALL_CELLS:
            //stream << "disableCell";
            break;

        default:
            throw string("Instruction::disassemble(): Invalid instruction opcode ") +
                    std::to_string(opcode) + string("!");
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
        case _PRINT_REG:
            stream << "PRINTREG(" << registerName(left) << ")";
            break;
        case _PRINT_CHARS:
            stream << "PRINTCHARS((numeric value)0x" << std::hex << value << std::dec << ")";
            break;
        case _QUIT:
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
        case _MULT_U:
            stream << "MULT_U(" << registerName(left) << ", " << registerName(right) << ")";
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
            stream << registerName(dest) << " = " << (int)value;
            break;

        case _IREAD:
            stream << registerName(dest) << " = LS[" << (int)value << "]";
            break;
        case _IWRITE:
            stream << "LS[" << (int)value << "] = " << registerName(left);
            break;

        case _REDUCE:
            stream << "RED(" << registerName(left) << ")";
            break;
        case _REDUCE_U:
            stream << "RED_U(" << registerName(left) << ")";
            break;
        case _SCAN:
            stream << "SCAN(" << registerName(left) << ")";
            break;
        case _SETLC_REDUCE:
            stream << "REPEAT_RED(" << registerName(left) << ")";
            break;
        //case _SETLC_REDUCE_NOTNULL:
        //    stream << "REPEAT_RED_NOTNULL(" << registerName(left) << ")";
        //    break;

        case _NOP:
            stream << "NOP";
            break;

        case _SETLC:
            stream << "REPEAT(" << (int)value << ")";
            break;
        case _IJMPNZ:
            stream << "END_REPEAT";
            break;
        case _IJMPNZ_RED:
            stream << "END_REPEAT_RED_NOT_ZERO";
            break;

        case _DISABLE_CELL:
            stream << "DISABLE_CELL";
            break;
        case _ENABLE_ALL_CELLS:
            stream << "ENABLE_ALL_CELLS";
            break;

        default:
            throw string("Instruction::dump(): Invalid instruction opcode ") +
                    std::to_string(opcode) + string("!");
    }

    stream << ";"; // << endl;

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
        case INSTRUCTION_TYPE_NO_IMMEDIATE:
          #ifdef CONNEX_REG_COUNT_512
            sprintf(desc, "(0x%016lx), Right=%lu, Left=%lu, Dest=%lu",
                          opcode, right, left, dest);
          #else
            sprintf(desc, "(0x%08x), Right=%u, Left=%u, Dest=%u",
                          opcode, right, left, dest);
          #endif
            break;
        case INSTRUCTION_TYPE_WITH_IMMEDIATE:
          #ifdef CONNEX_REG_COUNT_512
            sprintf(desc, "(0x%016lx), Value=%ld, Left=%lu, Dest=%lu",
                          opcode, value, left, dest);
          #else
            sprintf(desc, "(0x%08x), Value=%d, Left=%u, Dest=%u",
                          opcode, value, left, dest);
          #endif
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

/* IMPORTANT Note: these functions, getRight(), getLeft(), getDest(),
     getValue() should NOT be used by the (runtime)
     assembler, which is a very performance-sensitive component.
  They are meant to be used by Kernel::genLLVMISelManualCode() and
     by the Connex simulator (including trace printing).
*/
/************************************************************
 * Getter for the index of the left register operand
 */
int Instruction::getLeft() {
    if (opcode == _NOP ||
        opcode == _IREAD ||
        opcode == _READ ||
        opcode == _VLOAD ||
        opcode == _LDIX ||
        opcode == _WHERE_CRY ||
        opcode == _WHERE_EQ ||
        opcode == _WHERE_LT ||
        opcode == _END_WHERE ||
        opcode == _DISABLE_CELL ||
        opcode == _ENABLE_ALL_CELLS ||
        opcode == _MULT_LO ||
        opcode == _MULT_HI ||
        opcode == _LDSH ||
        opcode == _SETLC ||
        opcode == _IJMPNZ ||
        opcode == _PRINT_CHARS)
        return NO_REG_INDEX;

    return left;
}

/************************************************************
 * Getter for the index of the right register operand.
 *  IMPORTANT NOTE: Due to special ISA design the instructions
 *    _ISHL, _ISHR, _ISHRA are of type INSTRUCTION_TYPE_NO_IMMEDIATE,
 *      but they pass the immediate operand as right register index,
 *        with possible values 0..31, which is enough.
 */
int Instruction::getRight() {
    /* Note: we can avoid some of these checks
        (since right is set to NO_REG_INDEX), but we keep them for complete
        treatment.
    */
    if (opcode == _NOP ||
        opcode == _REDUCE ||
        opcode == _REDUCE_U ||
        opcode == _SCAN ||
        opcode == _IWRITE ||
        opcode == _IREAD ||
        opcode == _VLOAD ||
        opcode == _LDIX ||
        opcode == _WHERE_CRY ||
        opcode == _WHERE_EQ ||
        opcode == _WHERE_LT ||
        opcode == _END_WHERE ||
        opcode == _DISABLE_CELL ||
        opcode == _ENABLE_ALL_CELLS ||
        opcode == _MULT_LO ||
        opcode == _MULT_HI ||
        opcode == _LDSH ||
        opcode == _POPCNT ||
        opcode == _NOT ||
        /*
        // NOTE: These instructions actually return immediate operand put in
        //  the place of right operand - so they
        //   are of type INSTRUCTION_TYPE_NO_IMMEDIATE - their immediate
        //   operand needs to be only in interval 0..16.
        */
        opcode == _ISHL ||
        opcode == _ISHR ||
        opcode == _ISHRA ||
        opcode == _SETLC ||
        opcode == _IJMPNZ ||
        opcode == _PRINT_CHARS ||
        opcode == _PRINT_REG)
        return NO_REG_INDEX;

    return right;
}

/************************************************************
 * Getter for the index of the dest register operand
 */
int Instruction::getDest() {
    if (opcode == _NOP ||
        opcode == _REDUCE ||
        opcode == _REDUCE_U ||
        opcode == _SCAN ||
        opcode == _IWRITE ||
        opcode == _WRITE ||
        opcode == _WHERE_CRY ||
        opcode == _WHERE_EQ ||
        opcode == _WHERE_LT ||
        opcode == _END_WHERE ||
        opcode == _DISABLE_CELL ||
        opcode == _ENABLE_ALL_CELLS ||
        opcode == _MULT ||
        opcode == _MULT_U ||
        opcode == _CELL_SHL ||
        opcode == _CELL_SHR ||
        opcode == _SETLC ||
        opcode == _IJMPNZ ||
        opcode == _PRINT_CHARS ||
        opcode == _PRINT_REG)
        return NO_REG_INDEX;

    return dest;
}

/************************************************************
 * Getter for value.
 */
//short Instruction::getValue() {
ValueType Instruction::getValue() {
    /* IMPORTANT NOTE: Due to special ISA design, the instructions
     * _ISHL, _ISHR, _ISHRA are of type INSTRUCTION_TYPE_NO_IMMEDIATE,
     *   but they pass the immediate operand as right register index,
     *        with possible values 0..31, which is enough.
     *
     *    We "correct" this feature here.
     */
    if (opcode == _ISHL ||
        opcode == _ISHR ||
        opcode == _ISHRA) {
        return right;
    }

    return value;
}

// IMPORTANT: The setLeft/Dest/Right() methods should NOT be performance-critical
/************************************************************
 * Setter for opcode
 */
void Instruction::setOpcode(int anOpcode) {
    //assert(anOpcode >= 0 && anOpcode < ...);
    this->opcode = anOpcode;
}

/************************************************************
 * Setter for type
 */
void Instruction::setType(int aType) {
    assert(aType >= 0 && aType < INSTRUCTION_TYPE_WITH_IMMEDIATE);
    this->type = aType;
}

/************************************************************
 * Setter for left
 */
void Instruction::setLeft(int aLeft) {
    assert(aLeft >= 0 && aLeft < (1UL << LEFT_SIZE));
    this->left = aLeft;
}

/************************************************************
 * Setter for opcode
 */
void Instruction::setRight(int aRight) {
    assert(aRight >= 0 && aRight < (1UL << RIGHT_SIZE));
    this->right = aRight;
}

/************************************************************
 * Setter for opcode
 */
void Instruction::setDest(int aDest) {
    assert(aDest >= 0 && aDest < (1UL << DEST_SIZE));
    this->dest = aDest;
}

/************************************************************
 * Setter for opcode
 */
void Instruction::setValue(int value) {
    this->value = value;
}

