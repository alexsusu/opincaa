/*
 * File:   InstructionQueue.h
 *
 * Header file for a class encapsulating the Connex Array Simulation engine
 */
 
#ifndef INSTRUCTIONQUEUE_H
#define INSTRUCTIONQUEUE_H

#include "Instruction.h"
#include "Architecture.h"
#include <string>
#include <thread>

class InstructionQueue
{
public:
	InstructionQueue(unsigned short instructionCount);
	
	~InstructionQueue();
	
	unsigned short getInstructionQueued();
	
	bool isFull();
	
	void displaceReadPointer(unsigned short instructionCount);

	void push(Instruction *instruction);
	
	Instruction* read();

private:

	Instruction* *queue;
	
	unsigned short readPointer;
	
	unsigned short writePointer;
	
	unsigned short instructionsQueued;
	
	unsigned short maxInstructionCount;
	
};

#endif
