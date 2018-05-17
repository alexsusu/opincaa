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
#include <vector>
#include <mutex>

#include <unistd.h>


using namespace std;

class Kernel;

class ConnexMachine
{
    public:
        /**
         * Adds a kernel to the static kernel map.
         */
        static void addKernel(Kernel *kernel);

        /**
         * Disassembles the specified kernel
         *
         * @param kernelName the kernel to disassemble
         *
         * @return the string representing the disassembled kernel,
         * one instruction per line
         */
        static string disassembleKernel(string kernelName);

        /**
         * Dump the specified kernel.
         */
        static string dumpKernel(string kernelName);

        static Kernel *getKernel(string kernelName);
        static string genLLVMISelManualCode(string kernelName);

        /**
         * Reads byteCount bytes from descriptor and places the
         * result in destination. It blocks until all byteCount bytes
         * have been read.
         */
        unsigned readFromPipe(int descriptor, void* destination,
                              unsigned byteCount);

        /**
        * Constructor for creating a new ConnexMachine
        *
        * @param  distributionDescriptorPath the file descriptor of the distribution FIFO (write only)
        * @param  reductionDescriptorPath the file descriptor of the reduction FIFO (read only)
        * @param  writeDescriptorPath the file descriptor of the IO write FIFO (write only)
        * @param  readDescriptorPath the file descriptor of the IO read FIFO (read only)
        * @param  registerInterfacePath the file descriptor of the FPGA register interface (read only)
        *
        */
        ConnexMachine(string distributionDescriptorPath,
                      string reductionDescriptorPath,
                      string writeDescriptorPath,
                      string readDescriptorPath,
                      string registerInterfacePath);

        /**
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

        /**
         * Destructor for the ConnexMachine class
         *
         * Disposes of the batch map and closes the associated file
         * descriptors
         */
        ~ConnexMachine();

        /**
         * Executes the kernel on the current ConnexMachine
         *
         * @param kernelName the name of the kernel to execute
         *
         * @throws string if the kernel is not found
         */
        void executeKernel(string kernelName);

        /**
         * Writes the specified buffer to the array IO write FIFO
         *
         * @param buffer the buffer to be written to the FIFO, it should
         *   contain at least 2 * VECTOR_LENGTH * vectorCount bytes
         * @param vectorCount the number of vectors to fill
         * @param vectorIndex the vector with which to start the writing operation
         *
         * @return number of bytes written or -1 in case of error
         */
        int writeDataToConnex(const void *buffer, unsigned vectorCount,
                              unsigned vectorIndex);

        int writeDataToConnexPartial(const void *buffer, unsigned vectorCount,
                                     unsigned numElemCount, unsigned vectorIndex);

        /**
         * Reads the specified amounf of bytes to the specified buffer
         * from the array IO read FIFO
         *
         * @param buffer the buffer to write the data to (if NULL, it will be created)
         * @param vectorCount the amount of vectors to read --> bytes to read is vectorCount * CONNEX_VECTOR_LENGTH * sizeof(short)
         *
         * @return the buffer which the data was written to
         */
        void* readDataFromConnex(void *buffer, unsigned vectorCount,
                                 unsigned vectorIndex);

        /**
         * Reads the specified amounf of bytes, numElemCount, to the specified buffer
         * from the array IO read FIFO
         *
         * We use this function only for vector loop + residual loop
         * (i.e., when numElemCount is NOT multiple of CONNEX_VECTOR_LENGTH),
         * that is for less than a vector.
         *
         * @param buffer the buffer to write the data to (if NULL, it will be created)
         * @param numElemCount the amount of bytes to read
         *
         * @return the buffer which the data was written to
         */
        void* readDataFromConnexPartial(void *buffer, unsigned vectorCount,
                                        unsigned numElemCount, unsigned vectorIndex);

        /**
         * Reads one int from the reduction FIFO
         *
         * @return the value read from the reduction FIFO
         */
        int readReduction();

        /**
        * Reads multiple values from the reduction FIFO
        *
        * @param count the number of int to be read
        * @param buffer the memory area where the results will be put
        */
        //void readMultiReduction(int count, void* buffer);
        // Alex:
    #ifdef SIMULATOR_MODE
        int readMultiReduction(int count, void *buffer) {
            int result;

            /* Alex: In simulator mode it blocks if reading multiple ints, so we read
               each one separated.
               Note: GCC seems NOT to complain about defining here this method and
                having also another definition in core/ConnexMachine.cpp,
                int ConnexMachine::readMultiReduction(int count, void* buffer).

               TODO: We can make more efficient the read for the simulator, but it
                should not matter that much, because the I/O performance is not supposed
                to reflect the performance of a real system, such as Zynq ZedBoard.
            */
            printf("Alex: executing simulator mode readMultiReduction!\n");
            printf("Alex: count = %d\n", count);
            fflush(stdout);

            int countActual = 1;

            threadMutexIR.lock();

            for (int i = 0; i < count; i++) {
                /*
                printf("  i = %d\n", i);
                fflush(stdout);
                */
                //threadMutexIR.lock();

                if (readFromPipe(reductionFifo, ((int *)buffer) + i,
                                 countActual * sizeof(int)) < 0) {
                    threadMutexIR.unlock();
                    throw string("Error reading from reduction FIFO");
                    // TODO
                    return -1;
                }

                //threadMutexIR.unlock();
            }

            threadMutexIR.unlock();

            return 0;
        }
    #else
        int readMultiReduction(int count, void *buffer);
    #endif

        /*
         * Write the numResults reduction results from the FIFO in the
            possibly smaller or bigger array bufferRes.
            Eventually sign extend the result.
         * @param count the number of int to be read
         * @param bufferRes the memory area where the correct results will be put
         * @param signExtend if true, perform sign extension (Connex does not do it)
         */
        int readCorrectReductionResults(int count, void *bufferRes,
                                        int sizeOfPtr, bool signExtend = false);


        /**
         * Writes the specified command to the instruction FIFO (use with caution).
         *
         * @param command the command to write.
         */
        void writeCommand(unsigned command);

        /**
         * Writes the specified commands to the instruction FIFO (use with caution).
         *
         * @param commands the commands to write.
         */
        void writeCommands(std::vector<unsigned> commands);

    private:

        /**
        * Checks the FPGA accelerator architecture against the OPINCAA target architecture
        *
        * @return accelerator revision string
        */
        string checkAcceleratorArchitecture();

        /**
         * The file descriptor of the distribution FIFO (write only)
         */
        int distributionFifo;

        /**
         * The file descriptor of the reduction FIFO (read only)
         */
        int reductionFifo;

        /**
         * The file descriptor of the IO write FIFO (write only)
         */
        int ioWriteFifo;

        /**
         * The file descriptor of the IO read FIFO (read only)
         */
        int ioReadFifo;

        /**
         * The file descriptor of the FPGA register interface (read only)
         */
        int registerFile;

        /**
         * The map of the kernels available in the system
         */
        static map<string, Kernel*> kernels;

        /**
         * The mutex used to sync the IO operations.
         */
         mutex threadMutex;
         mutex threadMutexIR;

        /**
         * The mutex used to sync the kernel map operations
         */
        static mutex mapMutex;

        /**
         * The name of the architecture for which OPINCAA was compiled
         */
        static string targetArchitecture;
};

#endif // CONNEX_INTERFACE_H
