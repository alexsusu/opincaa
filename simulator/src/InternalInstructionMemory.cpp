/************************************************************
 * File:   InternalInstructionMemory.cpp
 *
 * Implementation file for the Connex Array Instruction Queue
 */

#include "InternalInstructionMemory.h"
#include <assert.h>
#include <string>

/************************************************************
 * Creates a new InternalInstructionMemory object.
 *
 * @param instructionCount the maximum number of instructions
 *   stored in this queue
 */
InternalInstructionMemory::InternalInstructionMemory(unsigned short instructionCount) {
    queue = new Instruction*[instructionCount];
    maxInstructionCount = instructionCount;
    instructionsQueued = 0;
    readPointer = 0;
    writePointer = 0;
}

/************************************************************
 * Deletes this InternalInstructionMemory object and releases all
 * associated resources.
 */
InternalInstructionMemory::~InternalInstructionMemory() {
    delete queue;
}

/************************************************************
 * Returns the number of queued instruction
 *
 * @return the number of queued instruction
 */
unsigned short InternalInstructionMemory::getNumInstructionsQueued() {
    return instructionsQueued;
}

/************************************************************
 * Returns true if the queue is full
 *
 * @return true if the queue is full.
 */
bool InternalInstructionMemory::isFull() {
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
void InternalInstructionMemory::displaceReadPointer(unsigned short instructionCount) {
    if (instructionsQueued < instructionCount + 1) {
        // 2017_11_05:
        throw string(
                 "Not enough instructions queued for specified jump - "
                 "this happens normally when the body of a REPEAT loop "
                 "contains more than INTERNAL_INSTRUCTION_MEMORY_SIZE - 2 instructions."
                 " (Note: The number of instructions in the loop body is "
                 "instructionCount = " +
                 std::to_string(instructionCount) + ")");
    }

  #if 0
    printf("Entered InternalInstructionMemory::displaceReadPointer()\n");
    printf("  readPointer = %d\n", readPointer);
    printf("  writePointer = %d\n", writePointer);
    printf("  instructionCount = %d\n", instructionCount);
    fflush(stdout);
  #endif

  #if 0
    for (int i = readPointer; i < writePointer; i++) {
        Instruction *compiledInstruction = queue[i];
        printf("   queue[%d] = %s\n", i, compiledInstruction->dump().c_str());
        fflush(stdout);
    }
  #endif


  #if 0
    // 2020_04_20
    int readPointerOld = readPointer;
    readPointer = (readPointer - instructionCount + maxInstructionCount - 1) %
                    maxInstructionCount;
    int delta = writePointer - readPointer;
    if (delta < 0 || delta < instructionCount + 2)
        readPointer = (writePointer + maxInstructionCount - instructionCount - 1) %
                        maxInstructionCount;
  #endif

    /* NOTE: readPointer is unsigned short so we need to do first
     *   + maxInstructionCount to avoid underflow. */
    readPointer = (readPointer + maxInstructionCount - instructionCount - 1) %
                    maxInstructionCount;

  #ifdef ORIG_CODE
    readPointer = (writePointer + maxInstructionCount - instructionCount - 1) %
                    maxInstructionCount;
  #endif
  #ifdef DEBUG
    printf("  After adjusting: readPointer = %d\n",
           readPointer);
  #endif

  #if 0
    for (int i = readPointer; i < writePointer; i++) {
        Instruction *compiledInstruction = queue[i];
        printf("   queue[%d] = %s\n", i, compiledInstruction->dump().c_str());
        fflush(stdout);
    }
  #endif
}

/************************************************************
 * Pushed the specified instruction in the queue. If the queue is full,
 * the oldest instruction is deleted and its allocated memory released.
 * Also increments the write pointer.
 *
 * @param instruction the instruction to push
 */
//void InternalInstructionMemory::pushAndConsiderConsumed(Instruction *instruction) {
void InternalInstructionMemory::push(Instruction *instruction) {
   #if 0
    printf("Entered InternalInstructionMemory::push()\n");
    fflush(stdout);
   #endif

    if (isFull()) {
        /* This case is often encountered.
         *   We need to overwrite the oldest element.
         */
       #if 0
        printf("InternalInstructionMemory::push(): case isFull()\n");
        printf("  readPointer = %d\n", readPointer);
        printf("  writePointer = %d\n", writePointer);
        fflush(stdout);
       #endif

        delete queue[writePointer];
    }
    else {
        instructionsQueued++;
    }

    queue[writePointer] = instruction;
    writePointer = (writePointer + 1) % maxInstructionCount;

    readPointer = writePointer;
}

/************************************************************
 * Reads the instruction at the read pointer and increments it.
 * The queue contents is unaffected.
 *
 * @return the instruction read from the read pointer.
 */
Instruction* InternalInstructionMemory::read() {
    Instruction *instruction = queue[readPointer];
    readPointer = (readPointer + 1) % maxInstructionCount;

    return instruction;
}

