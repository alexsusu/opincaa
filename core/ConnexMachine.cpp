/*
 * File:   ConnexMachine.cpp
 *
 * This is the class used to communicate
 * with the Connex Array
 *
 */

#include "ConnexMachine.h"
#include <fcntl.h>

#define     VECTOR_LENGTH   128

#define DEFAULT_DISTRIBUTION_FIFO   "/dev/xillybus_write_arm2array_32"
#define DEFAULT_REDUCTION_FIFO      "/dev/xillybus_read_array2arm_32"
#define DEFAULT_IO_WRITE_FIFO       "/dev/xillybus_write_mem2array_32"
#define DEFAULT_IO_READ_FIFO        "/dev/xillybus_read_array2mem_32"

#define IO_WRITE_OPERATION          0x00000001
#define IO_READ_OPERATION           0x00000000
#define LS_ADDRESS(x)               ((x) & 0x000003FF)
#define VECTOR_COUNT(x)             (((x) - 1) & 0x000003FF)

/*
 * This descriptor is written to the IO_WRITE_FIFO in
 * order to initiate a transfer.
 *
 * Specification can be found in ConnexIOSpec.docx
 */
static struct
{
    unsigned type;
    unsigned lsAddress;
    unsigned vectorCount;
} connex_io_descriptor;

/*
 * The static kernel map
 */
map<string, Kernel*> ConnexMachine::kernels;

/*
 * The mutex used to sync the kernel map operations
 */
mutex ConnexMachine::mapMutex;

/*
 * Adds a kernel to the static kernel map.
 *
 * @param kernel the new kernel to add
 *
 * @throws string if the kernel already exists
 */
void ConnexMachine::addKernel(Kernel *kernel)
{
    string name = kernel->getName();
    const string allowOverwrite = "allowOverwrite";
	mapMutex.lock();
    if(kernels.count(name) > 0)
    {
        if (name.find(allowOverwrite) == std::string::npos) //no explicit request for overwrite
        {
            mapMutex.unlock();
            throw string("Kernel ") + name + string(" already exists in ConnexMachine::addKernel");
        }
        else
        {
            kernels.erase(name);
        }
    }

    kernels.insert(map<string, Kernel*>::value_type(name, kernel));
	mapMutex.unlock();
}

/*
 * Dumps the specified kernel
 *
 * @param kernelName the kernel to dump
 *
 * @return the string representing the dumped kernel,
 * one instruction per line
 */
string ConnexMachine::dumpKernel(string kernelName)
{
    if(kernels.count(kernelName) == 0)
    {
        throw string("Kernel ") + kernelName + string(" not found in ConnexMachine::dumpKernel!");
    }
    Kernel *kernel = kernels.find(kernelName)->second;
    return kernel->dump();
}


/*
 * Disassembles the specified kernel.
 */
string ConnexMachine::disassembleKernel(string kernelName)
{
	if (!kernels.count(kernelName))
		throw string("Kernel ") + kernelName +
			string(" not found in ConnexMachine::disassembleKernel!");

	Kernel *kernel = kernels.find(kernelName)->second;

	return kernel->disassemble();
}

/*
 * Reads byteCount bytes from descriptor and places the 
 * result in destination. It blocks until all byteCount bytes
 * have been read.
 */
unsigned readFromPipe(int descriptor, void* destination, unsigned byteCount)
{
    char* dest = (char*)destination;
    unsigned totalBytesRead = 0;
    do{
        totalBytesRead += read(descriptor, dest + totalBytesRead, byteCount - totalBytesRead);
    }while(byteCount != totalBytesRead);
    
    return byteCount;
}

/*
 * Constructor for creating a new ConnexMachine
 *
 * @param  distributionDescriptorPath the file descriptor of the distribution FIFO (write only)
 * @param  reductionDescriptorPath the file descriptor of the reduction FIFO (read only)
 * @param  writeDescriptorPath the file descriptor of the IO write FIFO (write only)
 * @param  readDescriptorPath the file descriptor of the IO read FIFO (read only)
 *
 */
ConnexMachine::ConnexMachine(string distributionDescriptorPath = DEFAULT_DISTRIBUTION_FIFO,
                                string reductionDescriptorPath = DEFAULT_REDUCTION_FIFO,
                                string writeDescriptorPath = DEFAULT_IO_WRITE_FIFO,
                                string readDescriptorPath = DEFAULT_IO_READ_FIFO)
{
    const char* distpath = distributionDescriptorPath.c_str();
    const char* redpath = reductionDescriptorPath.c_str();
    const char* wiopath = writeDescriptorPath.c_str();
    const char* riopath = readDescriptorPath.c_str();

    distributionFifo = open(distpath, O_WRONLY);
    reductionFifo = open(redpath, O_RDONLY);
    ioWriteFifo = open(wiopath, O_WRONLY);
    ioReadFifo = open(riopath, O_RDONLY);

    if(distributionFifo < 0 ||
        reductionFifo  <0 ||
        ioWriteFifo < 0 ||
        ioReadFifo < 0
        )
    {
        throw string("Unable to open file descriptor");
    }

    printf("ConnexMachine created !\n");
    fflush(stdout);
}

/*
 * Constructor for creating a new ConnexMachine
 *
 * @param  distributionFifo the file descriptor of the distribution FIFO (write only)
 * @param  reductionFifo the file descriptor of the reduction FIFO (read only)
 * @param  ioWriteFifo the file descriptor of the IO write FIFO (write only)
 * @param  ioReadFifo the file descriptor of the IO read FIFO (read only)
 *
 */
ConnexMachine::ConnexMachine(int distributionFifo, int reductionFifo, int ioWriteFifo, int ioReadFifo)
{
    this->distributionFifo = distributionFifo;
    this->reductionFifo = reductionFifo;
    this->ioWriteFifo = ioWriteFifo;
    this->ioReadFifo = ioReadFifo;
}

/*
 * Destructor for the ConnexMachine class
 *
 * Disposes of the kernel map and closes the associated file
 * descriptors
 */
ConnexMachine::~ConnexMachine()
{
    if(distributionFifo > 0) close(distributionFifo);
    if(reductionFifo > 0) close(reductionFifo);
    if(ioWriteFifo > 0) close(ioWriteFifo);
    if(ioReadFifo > 0) close(ioReadFifo);
}

/*
 * Executes the kernel on the current ConnexMachine
 *
 * @param kernelName the name of the kernel to execute
 *
 * @throws string if the kernel is not found
 */
void ConnexMachine::executeKernel(string kernelName)
{
	threadMutexIR.lock();
    if(kernels.count(kernelName) == 0)
    {
		threadMutexIR.unlock();
        throw string("Kernel ") + kernelName + string(" not found in ConnexMachine::executeKernel!");
    }
    Kernel *kernel = kernels.find(kernelName)->second;
    kernel->writeTo(distributionFifo);
	threadMutexIR.unlock();
}

/*
* Writes the specified buffer to the array IO write FIFO
*
* @param buffer the buffer to be written to the FIFO, it should
*   contain at least 2 * VECTOR_LENGTH * vectorCount bytes
* @param vectorCount the number of vectors to fill
* @param startVectorIndex the vector with which to start the writing operation
*
* @return number of bytes written or -1 in case of error
*/
int ConnexMachine::writeDataToArray(void *buffer, unsigned vectorCount, unsigned startVectorIndex)
{
	threadMutex.lock();

    connex_io_descriptor.type = IO_WRITE_OPERATION;
    /* Use LS_ADDRESS macro to mask the least significant 10 bits */
    connex_io_descriptor.lsAddress = LS_ADDRESS(startVectorIndex);
    /* Use VECTOR_COUNT macro to mask the least significant 10 bits */
    connex_io_descriptor.vectorCount = VECTOR_COUNT(vectorCount);

    /* Issue the command */
    write(ioWriteFifo, &connex_io_descriptor, sizeof(connex_io_descriptor));

    /* Write the data */
    int bytesWritten = write(ioWriteFifo, buffer, vectorCount * VECTOR_LENGTH * 2);

    /* Flush the descriptor */
    write(ioWriteFifo, NULL, 0);

    /* Read the ACK, NOTE:this is blocking */
    int response;
    readFromPipe(ioReadFifo, &response, sizeof(int));

    //TODO: verify response

	threadMutex.unlock();

    return bytesWritten;
}

/*
* Reads the specified amounf of bytes to the specified buffer
* from the array IO read FIFO
*
* @param buffer the buffer to write the data to (if NULL, it will be created)
* @param bufferSize the amount of bytes to read
*
* @return the buffer which the data was written to
*
* @throws string if unable to read
*/
void* ConnexMachine::readDataFromArray(void *buffer, unsigned vectorCount, unsigned vectorIndex)
{
    if(buffer == NULL){
        buffer = new short[vectorCount * VECTOR_LENGTH];
    }

	threadMutex.lock();

    connex_io_descriptor.type = IO_READ_OPERATION;
    /* Use LS_ADDRESS macro to mask the least significant 10 bits */
    connex_io_descriptor.lsAddress = LS_ADDRESS(vectorIndex);
    /* Use VECTOR_COUNT macro to mask the least significant 10 bits */
    connex_io_descriptor.vectorCount = VECTOR_COUNT(vectorCount);

    /* Issue the command */
    write(ioWriteFifo, &connex_io_descriptor, sizeof(connex_io_descriptor));

    /* Flush the descriptor */
    write(ioWriteFifo, NULL, 0);

    /* Read the data */
    if(readFromPipe(ioReadFifo, buffer, vectorCount * VECTOR_LENGTH * 2) < 0)
    {
		threadMutex.unlock();
        throw string("Error reading from memory FIFO");
    }

	threadMutex.unlock();
    return buffer;
}

/*
 * Reads one int from the reduction FIFO
 *
 * @return the value read from the reduction FIFO
 */
int ConnexMachine::readReduction()
{
	threadMutexIR.lock();
    int result;
    if(readFromPipe(reductionFifo, &result, sizeof(int)) < 0)
    {
		threadMutexIR.unlock();
        throw string("Error reading from reduction FIFO");
    }

    threadMutexIR.unlock();
    return result;
}