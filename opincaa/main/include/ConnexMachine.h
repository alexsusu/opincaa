/*
 * File:   ConnexMachine.h
 *
 * This is the header file for a class used to communicate
 * with the Connex Array
 *
 */

#ifndef CONNEX_INTERFACE_H
#define CONNEX_INTERFACE_H

#include "Operand.h"
#include "Kernel.h"
#include <string>
#include <map>
#include <mutex>

/* to be moved in architecture.h */
#define CONNEX_MAX_REGS     32
#define CONNEX_MAX_MEMORY   1024
#define NUMBER_OF_MACHINES  128
//#define CONNEX_VECTOR_SIZE_IN_BYTES (2 * NUMBER_OF_MACHINES)
//#define CNXVECTOR_SIZE_IN_WORDS

using namespace std;

class Kernel;

class ConnexMachine
{
    public:
        /*
         * Adds a kernel to the static kernel map.
         */
        static void addKernel(Kernel *kernel);

        /*
         * Disassembles the specified kernel
         *
         * @param kernelName the kernel to disassemble
         *
         * @return the string representing the disassembled kernel,
         * one instruction per line
         */
        static string disassembleKernel(string kernelName);

	/*
	 * Dump the specified kernel.
	 */
	static string dumpKernel(string kernelName);



        /*
         * Constructor for creating a new ConnexMachine
         *
         * @param  distributionDescriptorPath the file of the distribution FIFO (write only)
         * @param  reductionDescriptorPath the file of the reduction FIFO (read only)
         * @param  writeDescriptorPath the file of the IO write FIFO (write only)
         * @param  readDescriptorPath the file of the IO read FIFO (read only)
         *
         */
        ConnexMachine(string distributionDescriptorPath ,
                                string reductionDescriptorPath,
                                string writeDescriptorPath,
                                string readDescriptorPath);



        /*
         * Constructor for creating a new ConnexMachine
         *
         * @param  distributionFifo the file descriptor of the distribution FIFO (write only)
         * @param  reductionFifo the file descriptor of the reduction FIFO (read only)
         * @param  ioWriteFifo the file descriptor of the IO write FIFO (write only)
         * @param  ioReadFifo the file descriptor of the IO read FIFO (read only)
         *
         */
        ConnexMachine(int distributionFifo, int reductionFifo, int ioWriteFifo, int ioReadFifo);

        /*
         * Destructor for the ConnexMachine class
         *
         * Disposes of the batch map and closes the associated file
         * descriptors
         */
        ~ConnexMachine();

        /*
         * Executes the kernel on the current ConnexMachine
         *
         * @param kernelName the name of the kernel to execute
         *
         * @throws string if the kernel is not found
         */
        void executeKernel(string kernelName);

        /*
         * Writes the specified buffer to the array IO write FIFO
         *
         * @param buffer the buffer to be written to the FIFO, it should
         *   contain at least 2 * VECTOR_LENGTH * vectorCount bytes
         * @param vectorCount the number of vectors to fill
         * @param vectorIndex the vector with which to start the writing operation
         *
         * @return number of bytes written or -1 in case of error
         */
        int writeDataToArray(void *buffer, unsigned vectorCount, unsigned vectorIndex);

        /*
         * Reads the specified amounf of bytes to the specified buffer
         * from the array IO read FIFO
         *
         * @param buffer the buffer to write the data to (if NULL, it will be created)
         * @param bufferSize the amount of bytes to read
         *
         * @return the buffer which the data was written to
         */
        void* readDataFromArray(void *buffer, unsigned vectorCount, unsigned vectorIndex);

        /*
         * Reads one int from the reduction FIFO
         *
         * @return the value read from the reduction FIFO
         */
        int readReduction();
    private:

        /*
         * The file descriptor of the distribution FIFO (write only)
         */
        int distributionFifo;

        /*
         * The file descriptor of the reduction FIFO (read only)
         */
        int reductionFifo;

        /*
         * The file descriptor of the IO write FIFO (write only)
         */
        int ioWriteFifo;

        /*
         * The file descriptor of the IO read FIFO (read only)
         */
        int ioReadFifo;

        /*
         * The map of the kernels available in the system
         */
        static map<string, Kernel*> kernels;

		/*
         * The mutex used to sync the IO operations.
         */
		mutex threadMutex;
		mutex threadMutexIR;

		/*
         * The mutex used to sync the kernel map operations
         */
		static mutex mapMutex;
};

#endif // CONNEX_INTERFACE_H
