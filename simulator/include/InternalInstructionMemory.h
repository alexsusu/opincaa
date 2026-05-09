/*
 * File:   InternalInstructionMemory.h
 *
 * Header file for a class encapsulating the Connex Array Simulation engine
 */

#ifndef INSTRUCTIONQUEUE_H
#define INSTRUCTIONQUEUE_H

#include "Architecture.h"
#include "Instruction.h"

/*
 * Class implementing an instruction queue for use with the Connex Simulator.
 * VERY IMPORTANT: this is NOT exactly a standard queue (FIFO) because
 *    it is used also to store loops and the loop body is pushed once
 *    and read from our queue several times while reading instructions in the
 *    body for each iteration.
 *    This is more exactly a buffer in which we the simulator stores the
 *      instructions that it reads from the instruction FIFO. The buffer is
 *      circular, allowing to retrieve previously read/consumed elements.
 *
 *   In the Verilog code of Connex this queue/buffer is called
 *     Internal Instruction Memory (unit iim in controller.v).
 *
 *   TODO: readPointer and writePointer with Alex's modification in push() are now the same value - so use just one variable.
 */
class InternalInstructionMemory {
  public:
    /*
     * Creates a new InternalInstructionMemory object.
     *
     * @param instructionCount the maximum number of instructions
     *   stored in this queue
     */
    InternalInstructionMemory(unsigned short instructionCount);

    /*
     * Deletes this InternalInstructionMemory object and releases all
     * associated resources.
     */
    ~InternalInstructionMemory();

    /*
     * Returns the number of queued instruction
     *
     * @return the number of queued instruction
     */
    unsigned short getNumInstructionsQueued();

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
     * Reads the instruction at the read pointer and increments the pointer.
     * The queue contents is unaffected.
     *
     * @return the instruction read from the read pointer.
     */
    Instruction *read();

    // 2020_04_20
    /*
     * Returns readPointer
     *
     * @return readPointer
     */
    unsigned short getReadPointer() {
        return readPointer;
    }

    // 2020_04_20
    /*
     * Sets readPointer
     */
    void setReadPointer(unsigned short aVal) {
        readPointer = aVal;
    }

  private:

    /*
     * The instruction queue
     */
    Instruction **queue;

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

