/*
 * File:   Instruction.h
 *
 * This is the header file for a class containing one Connex
 * Array instruction
 * 
 */

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <string>

using namespace std;

class Instruction
{
    public:
        /*
         * Returns a string representation of specified opcode
         */
        static string mnemonic(int opcode);
        
        /*
         * Returns a string representation of specified register
         */
        static string registerName(int register_index);
        
        /*
        * Constructor for creating a new Instruction
        * 
        * @param instruction the 32 bits used to create the instruction
        * 
        * @throws string if the instruction is invalid
        */ 
        Instruction(unsigned int instruction);
        
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
        Instruction(int opcode, int rightOrValue, int left, int dest);
        
       
        /*
         * Returns the 32bit word representing the assembled instruction
         * 
         * @return the 32bit word representing the assembled instruction
         */
        unsigned assemble();
        

        /*
         * Returns a string representation of this instruction
         * 
         * @return the 32bit word representing the assembled instruction
         */
        string toString();
        
        /*
         * Getter for type
         */
        int getType();
        
        /*
         * Getter for opcode
         */
        int getOpcode();
        
        /*
         * Getter for left
         */
        int getLeft();
        
        /*
         * Getter for right
         */
        int getRight();
        
        /*
         * Getter for dest
         */
        int getDest();
        
        /*
         * Getter for value
         */
        int getValue();
        
        /*
         * Setter for left
         */
        void setLeft(int left);
        
        /*
         * Setter for opcode
         */
        void setRight(int right);
        
        /*
         * Setter for opcode
         */
        void setDest(int dest);
        
        /*
         * Setter for opcode
         */
        void setValue(int value);
    private:
        /*
         * The type of this instruction (INSTRUCTION_TYPE_NO_VALUE or
         * INSTRUCTION_TYPE_WITH_VALUE)
         */
        int type;
        
        /*
         * The opcode
         */
        unsigned opcode;
        
        /*
         * The left operand
         */
        unsigned left;
        
        /*
         * The right operand
         */
        unsigned right;
        
        /*
         * The dest operand
         */
        unsigned dest;
        
        /*
         * The value operand
         */
        int value;
        
};

#endif // INSTRUCTION_H
