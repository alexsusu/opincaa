/************************************************************
 * File:   InstructionQueue.cpp
 *
 * Implementation file for the Connex Array Instruction Queue
 */

#include "InstructionQueue.h"
#include <string>

/************************************************************
 * Creates a new InstructionQueue object.
 *
 * @param instructionCount the maximum number of instructions
 *   stored in this queue
 */
InstructionQueue::InstructionQueue(unsigned short instructionCount) {
    queue = new Instruction*[instructionCount];
    maxInstructionCount = instructionCount;
    instructionsQueued = 0;
    readPointer = 0;
    writePointer = 0;
}

/************************************************************
 * Deletes this InstructionQueue object and releases all
 * associated resources.
 */
InstructionQueue::~InstructionQueue() {
    delete queue;
}

/************************************************************
 * Returns the number of queued instruction
 *
 * @return the number of queued instruction
 */
unsigned short InstructionQueue::getInstructionQueued() {
    return instructionsQueued;
}

/************************************************************
 * Returns true if the queue is full
 *
 * @return true if the queue is full.
 */
bool InstructionQueue::isFull() {
    return instructionsQueued == maxInstructionCount;
}

/************************************************************
 * Moves the read pointed back with the specified amount of instructions.
 * This is used for reading loop instructions
 *
 * @param instructionCount the amount of instructions the read pointer
 *     is displaced with.
 *
 * @throws string if there are not enough instructions in the queue
 */
void InstructionQueue::displaceReadPointer(unsigned short instructionCount) {
    if (instructionsQueued < instructionCount + 1) {
        // 2017_11_05:
        throw string(
                 "Not enough instructions queued for specified jump - "
                 "this happens normally when the body of a REPEAT loop "
                 "contains more than INSTRUCTION_QUEUE_LENGTH - 2 instructions."
                 " (Note: The number of instructions in the loop body "
                 "is instructionCount = " +
                 std::to_string(instructionCount) + ")");
    }

    readPointer = (writePointer + maxInstructionCount - instructionCount - 1) %
                    maxInstructionCount;
}

/************************************************************
 * Pushed the specified instruction in the queue. If the queue is full,
 * the oldest instruction is deleted and its allocated memory released.
 * Also increments the write pointer.
 *
 * @param instruction the instruction to push
 */
void InstructionQueue::push(Instruction *instruction) {
    if (isFull()) {
        delete queue[writePointer];
    }
    else {
        instructionsQueued++;
    }

    queue[writePointer] = instruction;
    writePointer = (writePointer + 1) % maxInstructionCount;
}

/************************************************************
 * Reads the instruction at the read pointer and increments it.
 * The queue contents is unaffected.
 *
 * @return the instruction read from the read pointer.
 */
Instruction* InstructionQueue::read() {
    Instruction *instruction = queue[readPointer++];
    readPointer %= maxInstructionCount;
    return instruction;
}

