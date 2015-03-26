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
string Instruction::mnemonic(int opcode)
{
    switch(opcode)
    {
	case _NOP: 		return string("nop");
 	case _SETLC:		return string("setlc");        
 	case _IJMPNZDEC:	return string("ijmpnzdec");    
 	case _RED:   		return string("red");
 	case _WHERE_LT: 	return string("wherelt");   
 	case _WHERE_EQ: 	return string("whereeq"); 
 	case _WHERE_CRY:	return string("wherecry");  
 	case _END_WHERE:	return string("endwhere"); 
 	case _LDSH:    		return string("ldsh"); 
 	case _LDIX:    		return string("ldix");    
 	case _MULT_LO:  	return string("multlo");  
 	case _MULT_HI:  	return string("multhi");    

 	case _CELL_SHR: 	return string("cellshr");  
 	case _ICELL_SHR: 	return string("icellshr"); 

 	case _CELL_SHL:  	return string("cellshl");   	
 	case _ICELL_SHL: 	return string("icellshl"); 

 	case _WRITE:  		return string("write");  
 	case _IWRITE:		return string("iwrite");    

 	case _READ:  		return string("read");   
 	case _IREAD: 		return string("iread");    

 	case _VLOAD:  		return string("vload");   
 	case _IVLOAD: 		return string("ivload");    

 	case _EQ:     		return string("eq"); 
	case _IEQ:   		return string("ieq");   

 	case _ULT:     		return string("ult");  
 	case _IULT:  		return string("iult");   

 	case _LT:      		return string("lt");   
 	case _ILT:    		return string("ilt");    

 	case _SHL:     		return string("shl");   
 	case _ISHL:   		return string("ishl");     

 	case _SHR:      	return string("shr");  
 	case _ISHR:    	        return string("ishr");   

 	case _SHRA:     	return string("shra");   
 	case _ISHRA:    	return string("ishra");    

 	case _MULT:     	return string("mult");    
 	case _IMULT:    	return string("imult");    

 	case _ADD:       	return string("add");  
 	case _IADD:     	return string("iadd");   

 	case _SUB:      	return string("sub");  
 	case _ISUB:     	return string("isub");   

 	case _ADDC:     	return string("addc");  
 	case _IADDC:		return string("iaddc");
      
 	case _SUBC:		return string("subc");
 	case _ISUBC:    	return string("isubc");    

 	case _NOT:      	return string("not");   

 	case _OR:       	return string("or");   
 	case _IOR:      	return string("ior");   

 	case _AND:      	return string("and");  	
 	case _IAND:     	return string("iand");    	

 	case _XOR:      	return string("xor"); 
 	case _IXOR:     	return string("ixor");   

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
        
/************************************************************
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
    opcode = GET_OPCODE(instruction);
    left  = GET_LEFT(instruction);
    dest = GET_DEST(instruction);
}
        
/************************************************************
* Constructor for creating a new Instruction
* 
* @param opcode 6 bits opcode
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
        
/************************************************************
* Returns the 32bit word representing the assembled instruction
* 
* @return the 32bit word representing the assembled instruction
*/
unsigned Instruction::assemble()
{
    unsigned instruction;
    instruction = opcode << OPCODE_POS;
    switch(type)
    {
        case INSTRUCTION_TYPE_NO_VALUE: 
            instruction |= right << RIGHT_POS;
            break;
        case INSTRUCTION_TYPE_WITH_VALUE:    
            instruction |= value << IMMEDIATE_VALUE_POS;
            break;
        default:
            throw string("Illegal instruction-type in Instruction::assemble");
    }

    instruction |= left << LEFT_POS;
    instruction |= dest << DEST_POS;
    
    return instruction;
}

/**
 * Returns the string representing the dissasembled instruction.
 */
string Instruction::disassemble()
{
        stringstream stream;

        stream << mnemonic(opcode);
        switch(opcode) {
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
	case _IADD:        
	case _IADDC:        
        case _IAND:           
        case _IEQ:         
        case _ILT:              
        case _IOR:            
        case _ISHL:        
        case _ISHR:      
        case _ISHRA:       
        case _ISUB:        
        case _ISUBC:        
        case _IULT:           
        case _IXOR:
   	case _ICELL_SHL:  
	case _ICELL_SHR:
                stream << " " << registerName(dest);
                stream << " " << registerName(left);
                stream << " (" << value << ")";
                break;
        case _NOT:
        	stream << registerName(dest);
        	stream << " " << registerName(left);
        	break;
        case _READ:
	case _VLOAD: 
                stream << registerName(dest);
                stream << " " << registerName(right);
                break;
        case _IREAD:
	case _IVLOAD:
                stream << registerName(dest);
		stream << " (" << value << ")";
                break;
        case _LDIX:
        case _LDSH:
        case _MULT_LO:
        case _MULT_HI:
                stream << " " << registerName(dest);
                break;
        case _RED:
                stream << " " << registerName(left);
                break;
        case _WRITE:
        case _MULT:
                stream << " " << registerName(left);
                stream << " " << registerName(right);
                break;
	case _IWRITE:
        case _IMULT:  
                stream << " " << registerName(left);
                stream << " (" << value << ")";
                break;
        case _SETLC:
                stream << " (" << value << ")";
                /* fall through */
        case _WHERE_CRY:
        case _WHERE_EQ:
        case _WHERE_LT:
        case _END_WHERE:
        case _IJMPNZDEC:
        case _NOP:
                break;
        default: throw string("Invalid instruction opcode!");
        }

        stream << ";" << endl;

        return stream.str();
}

/************************************************************
 * Returns the string representing the dumped instruction in 
 * OPINCAA format
 * 
 * @return string representing the dumped instruction
 */
string Instruction::dump()
{
    stringstream stream;
    
    switch(opcode)
    {
        case _ADD:   stream << registerName(dest) << " = " << registerName(left) << " + " << registerName(right);            break;
	case _IADD:  stream << registerName(dest) << " = " << registerName(left) << " + " << value;                          break;
        case _ADDC:  stream << registerName(dest) << " = ADDC(" << registerName(left) << ", " << registerName(right) << ")"; break;
	case _IADDC: stream << registerName(dest) << " = IADDC(" << registerName(left) << ", " << value << ")";              break;
	case _SUB:   stream << registerName(dest) << " = " << registerName(left) << " - " << registerName(right);            break;
	case _ISUB:  stream << registerName(dest) << " = " << registerName(left) << " - " << value;                          break;
        case _SUBC:  stream << registerName(dest) << " = SUBC(" << registerName(left) << ", " << registerName(right) << ")"; break;
        case _ISUBC: stream << registerName(dest) << " = ISUBC(" << registerName(left) << ", " << value << ")";              break;
	case _NOT:   stream << registerName(dest) << " = ~" << registerName(left);                                           break;
        case _OR:    stream << registerName(dest) << " = " << registerName(left) << " | " << registerName(right);            break;
	case _IOR:   stream << registerName(dest) << " = " << registerName(left) << " | " << value;                          break;
        case _AND:   stream << registerName(dest) << " = " << registerName(left) << " & " << registerName(right);            break;
	case _IAND:  stream << registerName(dest) << " = " << registerName(left) << " & " << value;                          break;
        case _XOR:   stream << registerName(dest) << " = " << registerName(left) << " ^ " << registerName(right);            break;
        case _IXOR:  stream << registerName(dest) << " = " << registerName(left) << " ^ " << value; 			     break;
	case _EQ:    stream << registerName(dest) << " = " << registerName(left) << " == " << registerName(right);           break;
	case _IEQ:   stream << registerName(dest) << " = " << registerName(left) << " == " << value;                         break;
        case _LT:    stream << registerName(dest) << " = " << registerName(left) << " < " << registerName(right);            break;
	case _ILT:   stream << registerName(dest) << " = " << registerName(left) << " < " << value;                          break;
        case _ULT:   stream << registerName(dest) << " = ULT(" << registerName(left) << ", " << registerName(right) << ")";  break;
	case _IULT:  stream << registerName(dest) << " = IULT(" << registerName(left) << ", " << value << ")";               break;
        case _SHL:   stream << registerName(dest) << " = " << registerName(left) << " << " << registerName(right); 	     break;
	case _ISHL:  stream << registerName(dest) << " = " << registerName(left) << " << " << value;                         break;
        case _SHR:   stream << registerName(dest) << " = " << registerName(left) << " >> " << registerName(right);           break;
	case _ISHR:  stream << registerName(dest) << " = " << registerName(left) << " >> " << value; 			     break;
        case _SHRA:  stream << registerName(dest) << " = SHRA(" << registerName(left) << ", " << registerName(right) << ")"; break;
        case _ISHRA: stream << registerName(dest) << " = ISHRA(" << registerName(left) << ", " << value << ")";              break;
        case _LDIX:  stream << registerName(dest) << " = INDEX";                                                             break;
        case _LDSH:  stream << registerName(dest) << " = SHIFT_REG";                                                         break;
        case _CELL_SHL:  stream << registerName(dest) << " = CELL_SHL(" << registerName(left) << ", " << registerName(right) << ")";
                         break;
	case _ICELL_SHL: stream << registerName(dest) << " = ICELL_SHL(" << registerName(left) << ", " << value << ")";
                         break;
        case _CELL_SHR:  stream << registerName(dest) << " = CELL_SHR(" << registerName(left) << ", " << registerName(right) << ")"; 
                         break;
	case _ICELL_SHR: stream << registerName(dest) << " = ICELL_SHR(" << registerName(left) << ", " << value << ")"; 
                         break;
	case _READ:      stream << registerName(dest) << " = LS[" << registerName(right) << "]";                             break;
        case _IREAD:     stream << registerName(dest) << " = LS[" << value << "]";                                           break;
	case _WRITE:     stream << "LS[" << registerName(right) << "] = " << registerName(left);                             break;
	case _IWRITE:    stream << "LS[" << value << "] = " << registerName(left);                                           break;
        case _MULT:      stream << registerName(left) << " * " << registerName(right);                                       break;
	case _IMULT:     stream << registerName(left) << " * " << value;                                                     break;
        case _MULT_LO:   stream << registerName(dest) << " = LOW()";                                                         break;
        case _MULT_HI:   stream << registerName(dest) << " = HIGH()";                                                        break;
        case _WHERE_CRY: stream << "WHERE_CRY";                                                                              break;
        case _WHERE_EQ:  stream << "WHERE_EQ";                                                                               break;
        case _WHERE_LT:  stream << "WHERE_LT";                                                                               break; 
        case _END_WHERE: stream << "END_WHERE";                                                                              break;
        case _RED:       stream << "RED(" << registerName(left) << ")";                                                      break;
        case _NOP:       stream << "NOP";                                                                                    break;
        case _VLOAD:     stream << registerName(dest) << " = " << registerName(right);                                       break;
	case _IVLOAD:    stream << registerName(dest) << " = " << value;                                                     break;
        case _SETLC:     stream << "REPEAT(" << value << ")";                                                                break;
        case _IJMPNZDEC: stream << "END_REPEAT";                                                                             break;
        default:   throw string("Invalid instruction opcode!");
    }
    
    stream << ";" << endl;
    
    return stream.str();
}

/************************************************************
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
 * Getter for left
 */
int Instruction::getLeft()
{
    return left;
}

/************************************************************
 * Getter for right
 */
int Instruction::getRight()
{
    return right;
}

/************************************************************
 * Getter for dest
 */
int Instruction::getDest()
{
    return dest;
}

/************************************************************
 * Getter for value
 */
int Instruction::getValue()
{
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
