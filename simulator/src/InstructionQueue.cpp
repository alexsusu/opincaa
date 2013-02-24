/*
 * File:   InstructionQueue.cpp
 *
 * Header file for the Connex Array Instruction Queue
 */
 

#include "InstructionQueue.h"
#include "Instruction.h"
#include "Architecture.h"
#include <string>

InstructionQueue::InstructionQueue(unsigned short instructionCount)
{
	queue = new Instruction*[instructionCount];
	maxInstructionCount = instructionCount;
	instructionsQueued = 0;
	readPointer = 0;
	writePointer = 0;
}

InstructionQueue::~InstructionQueue()
{
	delete queue;
}

unsigned short InstructionQueue::getInstructionQueued()
{
	return instructionsQueued;
}

bool InstructionQueue::isFull()
{
	return instructionsQueued == maxInstructionCount;
}

void InstructionQueue::displaceReadPointer(unsigned short instructionCount)
{
	if(instructionsQueued < instructionCount + 1)
	{
		throw string("Not enough instruction queued for specified jump.");
	}

	readPointer = (writePointer + maxInstructionCount - instructionCount - 1) % maxInstructionCount;
}

void InstructionQueue::push(Instruction *instruction)
{
	if(isFull())
	{
		delete queue[writePointer];
	}
	else
	{
		instructionsQueued++;
	}
	queue[writePointer] = instruction;
	writePointer = (writePointer + 1) % maxInstructionCount;
}

Instruction* InstructionQueue::read()
{
	Instruction *instruction = queue[readPointer++];
	readPointer %= maxInstructionCount;
	return instruction;
}




