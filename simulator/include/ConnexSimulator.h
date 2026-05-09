/*
 * File:   ConnexSimulator.h
 *
 * Header file for a class encapsulating the Connex Array Simulation engine
 */

#ifndef CONNEXSIMULATOR_H
#define CONNEXSIMULATOR_H

#include "ConnexVector.h"
#include "Instruction.h"
#include "InternalInstructionMemory.h"
#include "Architecture.h"

#include <assert.h>
#include <string>
#include <thread>

#include <unistd.h>

using namespace std;

/*
 *  Structure holding information for a IO transfer
 */
struct ConnexIoDescriptor;


//extern bool stopSimAtExecuteKernel; // 2020_03_29: Used to retrieve kernel->size()

class ConnexSimulator {
    public:

        /*
         *  Constructor for class ConnexSimulator
         *
         * @param distributionDescriptorPath the path to the distribution FIFO on the file system
         * @param reductionDescriptorPath the path to the reduction FIFO on the file system
         * @param writeDescriptorPath the path to the write FIFO (ARM -> ARRAY) on the file system
         * @param readDescriptorPath the path to the read FIFO (ARRAY -> ARM) on the file system
         */
        ConnexSimulator(string distributionDescriptorPath,
                        string reductionDescriptorPath,
                        string writeDescriptorPath,
                        string readDescriptorPath,
                        string accInfoPath);

        /*
         * Destructor for class ConnexSimulator
         */
        ~ConnexSimulator();

        /* Waits for the threads for program and data transfer to end.
         * NB: They don't stop unless killed.
         */
        void waitFinish();

    private:

        // Added this for debug support
        void printRegister(int index);


        /*
         * The register file for this simulator
         */
        // 2018_02_10
        ConnexVector *registerFile;

        /*
         * The local store for this simulator
         */
        // 2018_02_10
        ConnexVector *localStore;

        /*
         * The controller instruction internal memory buffer/queue
         */
        InternalInstructionMemory *internalInstructionMemory;

        /*
         * 0 if not in a Repeat loop, 1 for a single Repeat,
         *   2 for 2 Nested Repeats, etc
         *
         * Note: We start couting from 1 in a loop nest (0 is NOT used) since we,
         *   as LLVM, consider a simple loop has a depth of 1 (depth 0 doesn't exist).
         */
        int loopNestDepth;
        //
        /*
         * codeIn2ndLoopIterationForNestDepth[i] = for the ith nested loop of the
         *   current loop nest, this is a flag which is true when the array is
         *   executing a loop from the 2nd iteration, meaning the instructions
         *   are read from the local queue, rather than the Named Pipe.
         *
         * Note: We start from index 1 (index 0 is NOT used) since we, as LLVM,
         *   consider a simple loop has a depth of 1 (depth 0 doesn't exist).
         */
        bool codeIn2ndLoopIterationForNestDepth[MAX_DEPTH_LOOP_NESTING + 1]; // 2020_04_20
        //
        /*
         * repeatCounterForLoopOfNestDepth[i] = the controller loop repeat Counter for
         *    the ith nested loop of the current loop nest.
         *
         * Note: We start from index 1 (index 0 is NOT used) since we, as LLVM,
         *   consider a simple loop has a depth of 1 (depth 0 doesn't exist).
         */
        unsigned short repeatCounterForLoopOfNestDepth[MAX_DEPTH_LOOP_NESTING + 1];
        //
        /* startReadPointerForLoopOfNestDepth[i] = the start instruction in
         *   internalInstructionMemory for the nested repeat loop of depth i in the
         *   current loop nest.
         */
        int startReadPointerForLoopOfNestDepth[MAX_DEPTH_LOOP_NESTING + 1];

        /*
         * The distribution FIFO descriptor
         */
        int distributionDescriptor;

        /*
         * The reduction FIFO descriptor
         */
        int reductionDescriptor;

        /*
         * The read FIFO descriptor
         */
        int readDescriptor;

        /*
         * The write FIFO descriptor
         */
        int writeDescriptor;

        thread ioThread;
        thread coreThread;

        /*
         * Opens the FIFO at the specified path in the specified mode
         *
         * @param path the path to the FIFO
         * @param mode the mode of the FIFO
         * @return the FIFO descriptor
         */
        int openPipe(string path, int mode);

        /*
         * Creates a file with the identification of the simulated accelerator
         *
         * @param path the path to the FIFO
         */
        void setupAccInfo(string infoPath);

        /*
         * Starts the threads for program and data transfer.
         * NB: They don't stop unless killed.
         */
        void initiateThreads();

        /*
         * Handler method for IO transfers
         */
        void ioThreadHandler();

        /*
         * Handler method execution and reduction
         */
        void coreThreadHandler();

        /*
         * Performs an IO operation specified by the IO descriptor
         *
         * @param ioDescriptor the descriptor for the IO operation
         */
        void performIO(ConnexIoDescriptor ioDescriptor);

        /*
         * Executes the specified instruction on all active cells
         *
         * @param instruction the instruction to execute
         */
        void executeInstruction(Instruction instruction);

        /*
         * Executes a shit instruction on all active cells
         *
         * @param instruction the shift instruction to execute
         */
        void handleShift(Instruction instruction);

        /*
         * Executes a reduction instruction
         *
         * @param instruction the reduction instruction to execute
         */
        void handleReduction(Instruction instruction);

        /*
         * Executes a reduction instruction with unsigned operands
         *
         * @param instruction the reduction instruction to execute
         */
        void handleReduction_u(Instruction instruction);

        /*
         * Executes a reduction instruction with unsigned operands
         *
         * @param instruction the reduction instruction to execute
         */
        void handleScan(Instruction instruction);

        /*
         * Executes a local (cell) instruction on all active cells
         *
         * @param instruction the instruction to execute
         */
        void handleLocalInstruction(Instruction instruction);
};

#endif // CONNEXSIMULATOR_H
