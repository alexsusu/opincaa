/*
 * File:   InstructionQueue.h
 *
 * Header file for a class encapsulating the Connex Array Simulation engine
 */
 
#ifndef INSTRUCTIONQUEUE_H
#define INSTRUCTIONQUEUE_H

#include "Instruction.h"

/*
 * Class implementing an instruction queue for use with
 * the Connex Simulator
 */
class InstructionQueue
{
public:
	/*
	 * Creates a new InstructionQueue object.
	 *
	 * @param instructionCount the maximum number of instructions
	 *   stored in this queue
	 */
	InstructionQueue(unsigned short instructionCount);
	
	/*
	 * Deletes this InstructionQueue object and releases all
	 * associated resources.
	 */
	~InstructionQueue();

	/*
	 * Returns the number of queued instruction
	 *
	 * @return the number of queued instruction
	 */
	unsigned short getInstructionQueued();
	
	/*
	 * Returns true if the queue is full
	 *
	 * @return true if the queue is full.
	 */
	bool isFull();
	
	/*
	 * Moves the read pointed back with the specified amount of instructions. 
	 * This is used for reading loop instructions
	 *
	 * @param instructionCount the amount of instructions the read pointer
	 *     is displaced with.
	 *
	 * @throws string if there are not enough instructions in the queue
	 */
	void displaceReadPointer(unsigned short instructionCount);

	/*
	 * Pushed the specified instruction in the queue. If the queue is full,
	 * the oldest instruction is deleted and its allocated memory released.
	 * Also increments the write pointer.
	 *
	 * @param instruction the instruction to push
	 */
	void push(Instruction *instruction);
	
	/*
	 * Reads the instruction at the read pointer and increments it.
	 * The queue contents is unaffected.
	 *
	 * @return the instruction read from the read pointer.
	 */
	Instruction* read();

private:

	/*
	 * The instruction queue
	 */
	Instruction* *queue;
	
	/*
	 * The queue read pointer
	 */
	unsigned short readPointer;
	
	/*
	 * The queue write pointer
	 */
	unsigned short writePointer;
	
	/*
	 * The number of instructions queued
	 */
	unsigned short instructionsQueued;
	
	/*
	 * The maximum number of instructions this queue can hold
	 */
	unsigned short maxInstructionCount;
	
};

#endif
