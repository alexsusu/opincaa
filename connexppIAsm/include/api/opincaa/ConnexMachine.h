/*
 * File:   ConnexMachine.h
 *
 * This is the header file for a class used to communicate 
 * with the Connex Array
 * 
 */

#ifndef CONNEX_INTERFACE_H
#define CONNEX_INTERFACE_H

#include "Instruction.h"
#include "Kernel.h"
#include <string>
#include <map>

#define CONNEX_MAX_REGS     16
#define CONNEX_MAX_MEMORY   1024

using namespace std;

class ConnexMachine
{
    public:
        /*
         * Constructor for creating a new ConnexMachine with the default FIFO
         * descriptors
         * 
         * @throws string if the descriptors can't be opened
         */ 
        ConnexMachine();
        
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
         * Adds a kernel to the static kernel map. 
         */
        static void addKernel(Kernel *kernel);
        
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

};

#endif // CONNEX_INTERFACE_H
