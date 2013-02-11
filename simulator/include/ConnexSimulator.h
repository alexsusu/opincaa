/*
 * File:   ConnexSimulator.h
 *
 * Header file for a class encapsulating the Connex Array Simulation engine
 */

#ifndef CONNEXSIMULATOR_H
#define CONNEXSIMULATOR_H

#include "Cell.h"
#include "Instruction.h"
#include "Architecture.h"
#include <string>

using namespace std;

/*
 *	Structure holding information for a IO transfer
 */
struct ConnexIoDescriptor;

class ConnexSimulator
{
	public:
		
		/*
		 *	Constructor for class ConnexSimulator
		 *
		 * @param distributionDescriptorPath the path to the distribution FIFO on the file system
		 * @param reductionDescriptorPath the path to the reduction FIFO on the file system
		 * @param writeDescriptorPath the path to the write FIFO (ARM -> ARRAY) on the file system
		 * @param readDescriptorPath the path to the read FIFO (ARRAY -> ARM) on the file system
		 */
		ConnexSimulator(string distributionDescriptorPath, 
						string reductionDescriptorPath, 
						string writeDescriptorPath, 
						string readDescriptorPath);
		
		/*
		 * Destructor for class ConnexSimulator
		 */
		~ConnexSimulator();
		
		
	private:
		
		/*
		 * The Array of Cells that make up this Connex Machine
		 */
		Cell* cells[CONNEX_VECTOR_LENGTH];
		
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

		/*
		 * Opens the FIFO at the specified path in the specified mode
		 *
		 * @param path the path to the FIFO
		 * @param mode the mode of the FIFO
		 * @return the FIFO descriptor
		 */
		int openPipe(string path, int mode);
		
		/*
		 * Starts the threads for program and data transfer.
		 * They don't stop unless killed.
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
		 * Executes a local (cell) instruction on all active cells
		 * 
		 * @param instruction the instruction to execute
		 */
		void handleLocalInstruction(Instruction instruction);
};

#endif // CONNEXSIMULATOR_H
