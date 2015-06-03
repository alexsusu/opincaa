/*
 * File:   ConnexMachine.cpp
 *
 * This is the class used to communicate
 * with the Connex Array
 *
 */

#include <iostream>
#include "ConnexMachine.h"
#include "Architecture.h"
#include <string>
#include <fcntl.h>
#include <sys/mman.h>

#define DEFAULT_DISTRIBUTION_FIFO   "/dev/xillybus_connex_instruction_32"
#define DEFAULT_REDUCTION_FIFO      "/dev/xillybus_connex_reduction_32"
#define DEFAULT_IO_WRITE_FIFO       "/dev/xillybus_connex_iowrite_32"
#define DEFAULT_IO_READ_FIFO        "/dev/xillybus_connex_ioread_32"
#define DEFAULT_REGISTER_FILE       "/dev/uio0"

/***************************************************************************************************
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

/***************************************************************************************************
 * The static kernel map
 */
map<string, Kernel*> ConnexMachine::kernels;

/***************************************************************************************************
 * The mutex used to sync the kernel map operations
 */
mutex ConnexMachine::mapMutex;

/***************************************************************************************************
 * The name of the architecture for which OPINCAA was compiled
 */
string ConnexMachine::targetArchitecture = TARGET_ARCH;

/***************************************************************************************************
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

/***************************************************************************************************
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


/***************************************************************************************************
 * Disassembles the specified kernel.
 */
string ConnexMachine::disassembleKernel(string kernelName)
{
        cout<<kernelName<<endl;
	if (!kernels.count(kernelName)){
		throw string("Kernel ") + kernelName +
			string(" not found in ConnexMachine::disassembleKernel!");
        }
	Kernel *kernel = kernels.find(kernelName)->second;

	return kernel->disassemble();
}

/***************************************************************************************************
 * Reads byteCount bytes from descriptor and places the 
 * result in destination. It blocks until all byteCount bytes
 * have been read.
 */
unsigned ConnexMachine::readFromPipe(int descriptor, void* destination, unsigned byteCount)
{
    char* dest = (char*)destination;
    unsigned totalBytesRead = 0;
    do{
        totalBytesRead += read(descriptor, dest + totalBytesRead, byteCount - totalBytesRead);
    }while(byteCount != totalBytesRead);
    
    return byteCount;
}

/***************************************************************************************************
 * Constructor for creating a new ConnexMachine
 *
 * @param  distributionDescriptorPath the file descriptor of the distribution FIFO (write only)
 * @param  reductionDescriptorPath the file descriptor of the reduction FIFO (read only)
 * @param  writeDescriptorPath the file descriptor of the IO write FIFO (write only)
 * @param  readDescriptorPath the file descriptor of the IO read FIFO (read only)
 * @param  registerInterfacePath the file descriptor of the FPGA register interface (read only)
 *
 */
ConnexMachine::ConnexMachine(string distributionDescriptorPath = DEFAULT_DISTRIBUTION_FIFO,
                                string reductionDescriptorPath = DEFAULT_REDUCTION_FIFO,
                                string writeDescriptorPath = DEFAULT_IO_WRITE_FIFO,
                                string readDescriptorPath = DEFAULT_IO_READ_FIFO,
                                string registerInterfacePath = DEFAULT_REGISTER_FILE)
{
    const char* distpath = distributionDescriptorPath.c_str();
    const char* redpath = reductionDescriptorPath.c_str();
    const char* wiopath = writeDescriptorPath.c_str();
    const char* riopath = readDescriptorPath.c_str();
    const char* regpath = registerInterfacePath.c_str();
    
    registerFile = open(regpath, O_RDONLY);

    if(registerFile < 0)
    {
        throw string("Unable to access accelerator registers");
    }

    printf("Accelerator revision is %s\n",checkAcceleratorArchitecture().c_str());

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
        throw string("Unable to open one or more accelerator FIFOs");
    }
    connexInstructionsCounter.assign((1 << OPCODE_SIZE), 0);
    printf("ConnexMachine created !\n");
    fflush(stdout);
}

/***************************************************************************************************
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

/***************************************************************************************************
 * Executes the kernel on the current ConnexMachine
 *
 * @param kernelName the name of the kernel to execute
 *
 * @throws string if the kernel is not found
 */
void ConnexMachine::executeKernel(string kernelName)
{
    threadMutexIR.lock();
    vector<int> h;
    if(kernels.count(kernelName) == 0)
    {
	threadMutexIR.unlock();
        throw string("Kernel ") + kernelName + string(" not found in ConnexMachine::executeKernel!");
    }
    Kernel *kernel = kernels.find(kernelName)->second;
    h = kernel->getInstructionsCounter();
    for(int i=0; i<(1 <<OPCODE_SIZE); i++){
	connexInstructionsCounter[i] += h[i];
    }

    kernel->writeTo(distributionFifo);
    threadMutexIR.unlock();
}

/***************************************************************************************************
* Writes the specified buffer to the array IO write FIFO
*
* @param buffer the buffer to be written to the FIFO, it should
*   contain at least 2 * CONNEX_VECTOR_LENGTH * vectorCount bytes
* @param vectorCount the number of vectors to fill
* @param startVectorIndex the vector with which to start the writing operation
*
* @return number of bytes written or -1 in case of error
*/
int ConnexMachine::writeDataToArray(const void *buffer, unsigned vectorCount, unsigned startVectorIndex)
{
	threadMutex.lock();

    connex_io_descriptor.type = IO_WRITE_OPERATION;
    /* Use IO_LS_ADDRESS macro to mask the least significant 10 bits */
    connex_io_descriptor.lsAddress = IO_LS_ADDRESS(startVectorIndex);
    /* Use IO_VECTOR_COUNT macro to mask the least significant 10 bits */
    connex_io_descriptor.vectorCount = IO_VECTOR_COUNT(vectorCount);

    /* Issue the command */
    write(ioWriteFifo, &connex_io_descriptor, sizeof(connex_io_descriptor));

    /* Write the data */
    int bytesWritten = write(ioWriteFifo, buffer, vectorCount * CONNEX_VECTOR_LENGTH * 2);

    /* Flush the descriptor */
    write(ioWriteFifo, NULL, 0);

    /* Read the ACK, NOTE:this is blocking */
    int response;
    readFromPipe(ioReadFifo, &response, sizeof(int));

    //TODO: verify response

	threadMutex.unlock();

    return bytesWritten;
}

/***************************************************************************************************
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
        buffer = new short[vectorCount * CONNEX_VECTOR_LENGTH];
    }

	threadMutex.lock();

    connex_io_descriptor.type = IO_READ_OPERATION;
    /* Use IO_LS_ADDRESS macro to mask the least significant 10 bits */
    connex_io_descriptor.lsAddress = IO_LS_ADDRESS(vectorIndex);
    /* Use IO_VECTOR_COUNT macro to mask the least significant 10 bits */
    connex_io_descriptor.vectorCount = IO_VECTOR_COUNT(vectorCount);

    /* Issue the command */
    write(ioWriteFifo, &connex_io_descriptor, sizeof(connex_io_descriptor));

    /* Flush the descriptor */
    write(ioWriteFifo, NULL, 0);

    /* Read the data */
    if(readFromPipe(ioReadFifo, buffer, vectorCount * CONNEX_VECTOR_LENGTH * 2) < 0)
    {
		threadMutex.unlock();
        throw string("Error reading from memory FIFO");
    }

	threadMutex.unlock();
    return buffer;
}

/***************************************************************************************************
 * Reads one int from the reduction FIFO
 *
 * @return the value read from the reduction FIFO
 */
int ConnexMachine::readReduction()
{
    int result;
    readMultiReduction(1, &result);
    return result;
}

/***************************************************************************************************
 * Reads multiple values from the reduction FIFO
 *
 * @param count the number of int to be read
 * @param buffer the memory area where the results will be put
 */
void ConnexMachine::readMultiReduction(int count, void* buffer)
{
    int result;
    threadMutexIR.lock();

    if(readFromPipe(reductionFifo, buffer, count*sizeof(int)) < 0)
    {
        threadMutexIR.unlock();
        throw string("Error reading from reduction FIFO");
    }

    threadMutexIR.unlock();
}

/***************************************************************************************************
 * Checks the FPGA accelerator architecture against the OPINCAA target architecture
 *
 * @return accelerator revision string
 */
string ConnexMachine::checkAcceleratorArchitecture()
{
    void *map_addr = mmap(NULL, 64, PROT_READ, MAP_SHARED, registerFile, 0);
    
    if(map_addr == MAP_FAILED)
    {
        throw string("Failed to mmap accelerator register interface");
    }
    
    volatile unsigned int *regs = (volatile unsigned int *)map_addr;

    std::string archName = std::string((char*)regs);
    archName = string(archName.rbegin(), archName.rend());
    
    if(archName.compare(targetArchitecture) != 0)
    {
        throw string("Accelerator architecture ("+archName+") does not match OPINCAA target architecture ("+targetArchitecture+")");
    }
    
    std::string accRevision = std::to_string(regs[11])+
                              std::to_string(regs[10])+
                              std::to_string(regs[9])+
                              std::to_string(regs[8])+
                              std::to_string(regs[7])+
                              std::to_string(regs[6]);

    return accRevision;
                           
}


/***************************************************************************************************/
vector<int> ConnexMachine::getConnexInstructionsCounter(){
	
       /* for(int i=0; i<(1 <<OPCODE_SIZE); i++){
	    cout<<connexInstructionsCounter[i]<<endl;
        }*/
	return connexInstructionsCounter;
}

/***************************************************************************************************/
void ConnexMachine::getKernelHistogram(string kernelName){
    Kernel *kernel = kernels.find(kernelName)->second;
    kernel->kernelHistogram();
}



