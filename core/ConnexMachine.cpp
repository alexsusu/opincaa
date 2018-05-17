/*
 * File:   ConnexMachine.cpp
 *
 * This is the class used to communicate
 * with the Connex Array
 *
 */

#include "ConnexMachine.h"
#include "Architecture.h"
#include <assert.h>
#include <string>
#include <string.h> // for memcpy()
#include <fcntl.h>
#include <sys/mman.h>

//#define DEBUG_OPINCAA

#define DEFAULT_DISTRIBUTION_FIFO   "/dev/xillybus_connex_instruction_32"
#define DEFAULT_REDUCTION_FIFO      "/dev/xillybus_connex_reduction_32"
#define DEFAULT_IO_WRITE_FIFO       "/dev/xillybus_connex_iowrite_32"
#define DEFAULT_IO_READ_FIFO        "/dev/xillybus_connex_ioread_32"
#define DEFAULT_REGISTER_FILE       "/dev/uio0"


/* Alex: at least libc's write() function has specified to complain about
         the unused result; using (void)write(...) no longer help in later
         versions of GCC, etc because (void) gets compiled away.

Inspired from igr(x) from
  https://stackoverflow.com/questions/7271939/warning-ignoring-return-value-of-scanf-declared-with-attribute-warn-unused-r
*/
//#define IGNORE_RESULT(x) {__typeof__(x) __attribute__((unused)) d=(x);}
#define IGNORE_RESULT(x) {int __attribute__((unused)) resUnused = (x);}

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
 * The name of the architecture for which OPINCAA was compiled
 */
string ConnexMachine::targetArchitecture = TARGET_ARCH;

/*
 * Adds a kernel to the static kernel map.
 *
 * @param kernel the new kernel to add
 *
 * @throws string if the kernel already exists
 *
 * Alex: here we must not put static qualifier for definition, which would mean
 *   the method has static (non-external linkage).
 */
void ConnexMachine::addKernel(Kernel *kernel) {
    string name = kernel->getName();
    const string allowOverwrite = "allowOverwrite";

    mapMutex.lock();

    if (kernels.count(name) > 0) {
        // No explicit request for overwrite
        if (name.find(allowOverwrite) == std::string::npos) {
            mapMutex.unlock();
            throw string("Kernel ") + name +
                         string(" already exists in ConnexMachine::addKernel");
        }
        else {
            kernels.erase(name);
        }
    }

    /*
    Lucian said on Jun 26th, 2017 that it is required to run initially the
      END_WHERE to enable all cells.

    // Alex: erasing 1st instruction from the kernel, which should be an
    // END_WHERE, proved to be a BAD idea, since it is required to activate all
    // cells when the kernel starts.
    vector<unsigned> &kernelInstrs = kernel->getInstructions();
    assert(Instruction(kernelInstrs[0]).getOpcode() == _END_WHERE);
    // See http://www.cplusplus.com/reference/vector/vector/erase/
    kernelInstrs.erase(kernelInstrs.begin());
    */

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
string ConnexMachine::dumpKernel(string kernelName) {
    if (kernels.count(kernelName) == 0) {
        throw string("Kernel ") + kernelName +
                     string(" not found in ConnexMachine::dumpKernel!");
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

Kernel *ConnexMachine::getKernel(string kernelName) {
    // 2018_05_09
    // Inspired from http://www.cplusplus.com/reference/map/map/find/
    std::map<string, Kernel *>::iterator it;

    it = kernels.find(kernelName);
    if (it == kernels.end())
        return NULL;

    Kernel *kernel = kernels.find(kernelName)->second;
    return kernel;
}

/*
 *
 */
string ConnexMachine::genLLVMISelManualCode(string kernelName)
{
    if (!kernels.count(kernelName))
        throw string("Kernel ") + kernelName +
            string(" not found in ConnexMachine::genLLVMISelManualCode()!");

    Kernel *kernel = kernels.find(kernelName)->second;

    return kernel->genLLVMISelManualCode();
}

/*
 * Reads byteCount bytes from descriptor and places the
 * result in destination. It blocks until all byteCount bytes
 * have been read.
 */
unsigned ConnexMachine::readFromPipe(int descriptor, void *destination,
                                     unsigned byteCount) {
    char *dest = (char *)destination;
    unsigned totalBytesRead = 0;
    do {
        totalBytesRead += read(descriptor, dest + totalBytesRead,
                               byteCount - totalBytesRead);

      #ifdef MY_DEBUG
        printf("ConnexMachine::readFromPipe(): totalBytesRead = %d\n",
               totalBytesRead);
        fflush(stdout);
      #endif
    } while (byteCount != totalBytesRead);

    return byteCount;
}

/*
 * Constructor for creating a new ConnexMachine
 *
 * @param  distributionDescriptorPath the file descriptor of the distribution FIFO (write only)
 * @param  reductionDescriptorPath the file descriptor of the reduction FIFO (read only)
 * @param  writeDescriptorPath the file descriptor of the IO write FIFO (write only)
 * @param  readDescriptorPath the file descriptor of the IO read FIFO (read only)
 * @param  registerInterfacePath the file descriptor of the FPGA register interface (read only)
 *
 */
ConnexMachine::ConnexMachine(
                string distributionDescriptorPath = DEFAULT_DISTRIBUTION_FIFO,
                string reductionDescriptorPath = DEFAULT_REDUCTION_FIFO,
                string writeDescriptorPath = DEFAULT_IO_WRITE_FIFO,
                string readDescriptorPath = DEFAULT_IO_READ_FIFO,
                string registerInterfacePath = DEFAULT_REGISTER_FILE) {
    const char* distpath = distributionDescriptorPath.c_str();
    const char* redpath = reductionDescriptorPath.c_str();
    const char* wiopath = writeDescriptorPath.c_str();
    const char* riopath = readDescriptorPath.c_str();
    const char* regpath = registerInterfacePath.c_str();

    registerFile = open(regpath, O_RDONLY);

    if (registerFile < 0) {
        throw string("Unable to access accelerator registers");
    }

    printf("Accelerator revision is %s\n",
           checkAcceleratorArchitecture().c_str());

    distributionFifo = open(distpath, O_WRONLY);
    reductionFifo = open(redpath, O_RDONLY);
    ioWriteFifo = open(wiopath, O_WRONLY);
    ioReadFifo = open(riopath, O_RDONLY);

    if (distributionFifo < 0 ||
        reductionFifo  < 0 ||
        ioWriteFifo < 0 ||
        ioReadFifo < 0
        ) {
        throw string("Unable to open one or more accelerator FIFOs");
    }

    printf("ConnexMachine created !\n");
    fflush(stdout);
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
ConnexMachine::ConnexMachine(
                string distributionDescriptorPath = DEFAULT_DISTRIBUTION_FIFO,
                string reductionDescriptorPath = DEFAULT_REDUCTION_FIFO,
                string writeDescriptorPath = DEFAULT_IO_WRITE_FIFO,
                string readDescriptorPath = DEFAULT_IO_READ_FIFO) {
    const char* distpath = distributionDescriptorPath.c_str();
    const char* redpath = reductionDescriptorPath.c_str();
    const char* wiopath = writeDescriptorPath.c_str();
    const char* riopath = readDescriptorPath.c_str();

    distributionFifo = open(distpath, O_WRONLY);
    reductionFifo = open(redpath, O_RDONLY);
    ioWriteFifo = open(wiopath, O_WRONLY);
    ioReadFifo = open(riopath, O_RDONLY);

    if (distributionFifo < 0 ||
        reductionFifo  < 0 ||
        ioWriteFifo < 0 ||
        ioReadFifo < 0
        ) {
        throw string("Unable to open one or more accelerator FIFOs");
    }

    printf("ConnexMachine created !\n");
    fflush(stdout);
}

/*
 * Destructor for the ConnexMachine class
 *
 * Disposes of the kernel map and closes the associated file
 * descriptors
 */
ConnexMachine::~ConnexMachine() {
    // 2018_04_03
    for (auto &aPair : kernels) {
        printf("ConnexMachine::~ConnexMachine(): freeing "
               "kernel %s (with ptr value = %p)\n",
               aPair.first.c_str(), aPair.second);
        fflush(stdout);

        delete aPair.second;
    }
    // kernels gets automatically deallocated since it is a member variable.
    kernels.clear();

    if (distributionFifo > 0)
        close(distributionFifo);
    if (reductionFifo > 0)
        close(reductionFifo);
    if (ioWriteFifo > 0)
        close(ioWriteFifo);
    if (ioReadFifo > 0)
        close(ioReadFifo);
}

/*
 * Executes the kernel on the current ConnexMachine
 *
 * @param kernelName the name of the kernel to execute
 *
 * @throws string if the kernel is not found
 */
void ConnexMachine::executeKernel(string kernelName) {
    threadMutexIR.lock();
    if (kernels.count(kernelName) == 0) {
        threadMutexIR.unlock();
        throw string("Kernel ") + kernelName +
                        string(" not found in ConnexMachine::executeKernel!");
    }

    Kernel *kernel = kernels.find(kernelName)->second;

  //#ifdef DEBUG_OPINCAA
    printf("Alex: executeKernel(%s): kernel->size() = %d\n",
                                 kernelName.c_str(), kernel->size());
    fflush(stdout);
  //#endif

    kernel->writeTo(distributionFifo);
    threadMutexIR.unlock();
}

/*
* Writes the specified buffer to the array IO write FIFO
*
* @param buffer the buffer to be written to the FIFO, it should
*   contain at least 2 * CONNEX_VECTOR_LENGTH * vectorCount bytes
* @param vectorCount the number of vectors to fill
* @param startVectorIndex the vector with which to start the writing operation
*
* @return number of bytes written or -1 in case of error
*/
int ConnexMachine::writeDataToConnex(const void *buffer, unsigned vectorCount,
                                                  unsigned startVectorIndex) {
    assert(vectorCount > 0 &&
           "if vectorCount == 0, it crashes Connex on Zedboard"); // Alex
    assert(startVectorIndex >= 0 &&
           "startVectorIndex < 0 --> writeDataToConnex() will write outside "
           "the LS memory"); // Alex

  #ifdef DEBUG_OPINCAA
    printf("Entered ConnexMachine::writeDataToConnex(): "
           "before threadMutex.lock();\n");
    fflush(stdout);
  #endif
    threadMutex.lock();

    connex_io_descriptor.type = IO_WRITE_OPERATION;
    /* Use IO_LS_ADDRESS macro to keep only the least significant
       meaningful bits */
    connex_io_descriptor.lsAddress = IO_LS_ADDRESS(startVectorIndex);
    /* Use IO_VECTOR_COUNT macro to keep only the least significant
       meaningful bits */
    connex_io_descriptor.vectorCount = IO_VECTOR_COUNT(vectorCount);

    assert(startVectorIndex + vectorCount <= CONNEX_MEM_SIZE &&
           "startVectorIndex + vectorCount > CONNEX_MEM_SIZE --> "
           "writeDataToConnex() will write outside the LS memory");

  #ifdef DEBUG_OPINCAA
    printf("ConnexMachine::writeDataToConnex(): before write()...\n");
    fflush(stdout);
  #endif
    /* Issue the command */
    IGNORE_RESULT( write(ioWriteFifo, &connex_io_descriptor,
                   sizeof(connex_io_descriptor)) );

  #ifdef DEBUG_OPINCAA
    printf("ConnexMachine::writeDataToConnex(): before 2nd write()...\n");
    fflush(stdout);
  #endif
    /* Write the data */
    int bytesWritten = write(ioWriteFifo, buffer,
                             vectorCount * CONNEX_VECTOR_LENGTH *
                             sizeof(TYPE_ELEMENT));

  #ifdef DEBUG_OPINCAA
    printf("ConnexMachine::writeDataToConnex(): before flushing write()...\n");
    fflush(stdout);
  #endif
    /* Flush the descriptor */
    IGNORE_RESULT( write(ioWriteFifo, NULL, 0) );

  #ifdef DEBUG_OPINCAA
    printf("ConnexMachine::writeDataToConnex(): before readFromPipe()...\n");
    fflush(stdout);
  #endif
    /* Read the ACK. NOTE: this is blocking */
    int response;
    readFromPipe(ioReadFifo, &response, sizeof(int));

    //TODO: verify response

  #ifdef DEBUG_OPINCAA
    printf("ConnexMachine::writeDataToConnex(): before threadMutex.unlock()...\n");
    fflush(stdout);
  #endif
    threadMutex.unlock();

    return bytesWritten;
}

/* We use this function for both vector loop + residual loop.
 * But, for the sake of efficiency, we could use this function only for
 *   residual loop
 *   (i.e., when numElemCount is NOT multiple of CONNEX_VECTOR_LENGTH),
 *   that is for less than the size of a vector.
 */
int ConnexMachine::writeDataToConnexPartial(const void *buffer,
                                            unsigned vectorCount,
                                            unsigned numElemCount,
                                            unsigned startVectorIndex) {
  #ifdef DEBUG_OPINCAA
    printf("Entered ConnexMachine::writeDataToConnexPartial(): buffer = %p, "
            "vectorCount = %d, numElemCount = %d, startVectorIndex = %d\n",
            buffer, vectorCount, numElemCount, startVectorIndex);
    //printf("    numElemCount = %d\n", numElemCount);
    //printf("    startVectorIndex = %d\n", startVectorIndex);
    fflush(stdout);
  #endif

    if (numElemCount == vectorCount * CONNEX_VECTOR_LENGTH) { // TODO: use power strength reduction: << instead of *
        // We use the standard procedure
       #ifdef DEBUG_OPINCAA
        printf("writeDataToConnexPartial(): calling standard writeDataToConnex()\n");
        fflush(stdout);
       #endif
        return writeDataToConnex(buffer, vectorCount, startVectorIndex);
    }

    //assert(0);

    //assert(vectorCount == 1);
    assert(numElemCount < vectorCount * CONNEX_VECTOR_LENGTH);

    int numTotalElemCount = vectorCount * CONNEX_VECTOR_LENGTH;

  #ifdef DEBUG_OPINCAA
    printf("AlexNEW: numTotalElemCount = %d\n", numTotalElemCount);
    printf("AlexNEW: numElemCount = %d\n", numElemCount);
    fflush(stdout);
  #endif

    void *bufferTmp = malloc(numTotalElemCount * sizeof(TYPE_ELEMENT));

    memcpy(bufferTmp, buffer, numElemCount * sizeof(TYPE_ELEMENT));
  #ifdef INTRODUCE_COMMUNICATION_ERROR
    * ((char *)bufferTmp) = 0xAF;
  #endif

    /* We pad the end of bufferTmp.
      More exactly, we do initialization especially for various reduction
     cases: 0 for sum reduction, (in case Connex supports the following in
     hardware do them also):
        - 1 for multiply reduction,
        - 0 for xor-reduction,
        - -inf for max reduction
        - +inf for min reduction
        etc
    */

  #ifdef DEBUG_OPINCAA
    printf("AlexNEW: Before memset()\n");
    fflush(stdout);
  #endif
    memset(((char *)bufferTmp) + numElemCount * sizeof(TYPE_ELEMENT),
            0, (numTotalElemCount - numElemCount) * sizeof(TYPE_ELEMENT));
  #ifdef DEBUG_OPINCAA
    printf("After memset()\n");
    fflush(stdout);
  #endif

    threadMutex.lock();
  #ifdef DEBUG_OPINCAA
    printf("After threadMutex.lock()\n");
    fflush(stdout);
  #endif

    connex_io_descriptor.type = IO_WRITE_OPERATION;
    /* Use IO_LS_ADDRESS macro to mask the least significant 10 bits */
    connex_io_descriptor.lsAddress = IO_LS_ADDRESS(startVectorIndex);
    /* Use IO_VECTOR_COUNT macro to mask the least significant 10 bits */
    connex_io_descriptor.vectorCount = IO_VECTOR_COUNT(vectorCount);

    /* Issue the command */
    IGNORE_RESULT( write(ioWriteFifo, &connex_io_descriptor,
                         sizeof(connex_io_descriptor)) );
  #ifdef DEBUG_OPINCAA
    printf("After write()\n");
    fflush(stdout);
  #endif

    /* Write the data */
    int bytesWritten = write(ioWriteFifo, bufferTmp,
                             numTotalElemCount * sizeof(TYPE_ELEMENT));

    /* Flush the descriptor */
    IGNORE_RESULT( write(ioWriteFifo, NULL, 0) );
  #ifdef DEBUG_OPINCAA
    printf("After 2nd write()\n");
    fflush(stdout);
  #endif

    /* Read the ACK, NOTE:this is blocking */
    int response;
    readFromPipe(ioReadFifo, &response, sizeof(int));
  #ifdef DEBUG_OPINCAA
    printf("After readFromPipe()\n");
    fflush(stdout);
  #endif

    //TODO: verify response

    free(bufferTmp);

  #ifdef DEBUG_OPINCAA
    printf("After free()\n");
    fflush(stdout);
  #endif

    threadMutex.unlock();
  #ifdef DEBUG_OPINCAA
    printf("After unlock()\n");
    fflush(stdout);
  #endif

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
void *ConnexMachine::readDataFromConnex(void *buffer, unsigned vectorCount, unsigned startVectorIndex) {
    assert(vectorCount > 0 &&
           "if vectorCount == 0, it crashes Connex on Zedboard"); // Alex
    assert(startVectorIndex >= 0 &&
           "startVectorIndex < 0 --> readDataFromConnex() will read outside "
           "the LS memory"); // Alex

    // 2018_04_03
    /*
    if (buffer == NULL) {
        buffer = new TYPE_ELEMENT[vectorCount * CONNEX_VECTOR_LENGTH];
    }
    */
    assert(buffer != NULL);

    threadMutex.lock();

    connex_io_descriptor.type = IO_READ_OPERATION;
    /* Use IO_LS_ADDRESS macro to mask the least significant 10 bits */
    connex_io_descriptor.lsAddress = IO_LS_ADDRESS(startVectorIndex);
    /* Use IO_VECTOR_COUNT macro to mask the least significant 10 bits */
    connex_io_descriptor.vectorCount = IO_VECTOR_COUNT(vectorCount);

    assert(startVectorIndex + vectorCount <= CONNEX_MEM_SIZE &&
           "startVectorIndex + vectorCount > CONNEX_MEM_SIZE --> "
           "readDataFromConnex() will read outside the LS memory");

    /* Issue the command */
    IGNORE_RESULT( write(ioWriteFifo, &connex_io_descriptor,
                         sizeof(connex_io_descriptor)) );

    /* Flush the descriptor */
    IGNORE_RESULT( write(ioWriteFifo, NULL, 0) );

    /* Read the data */
    if(readFromPipe(ioReadFifo, buffer,
                    vectorCount * CONNEX_VECTOR_LENGTH *
                    sizeof(TYPE_ELEMENT)) < 0) {
        threadMutex.unlock();
        throw string("Error reading from memory FIFO");
    }

    threadMutex.unlock();
    return buffer;
}


/* We use this function for both vector loop + residual loop.
 * But, for the sake of SILLY efficiency, we could use this function only for residual loop
 *   (i.e., when numElemCount is NOT multiple of CONNEX_VECTOR_LENGTH),
 *   that is for less than a size of a vector.
 *
 * THE reason we use this function is that we want to prevent a buffer overflow on (void *) buffer.
 */
void *ConnexMachine::readDataFromConnexPartial(void *buffer, unsigned vectorCount, unsigned numElemCount, unsigned startVectorIndex) {
    printf("Entered readDataFromConnexPartial(): buffer = %p, "
               "vectorCount = %d, numElemCount = %d, startVectorIndex = %d\n",
               buffer, vectorCount, numElemCount, startVectorIndex);
    fflush(stdout);

    if (numElemCount == vectorCount * CONNEX_VECTOR_LENGTH) { // TODO: use power strength reduction: << instead of *
        printf("readDataFromConnexPartial(): calling standard readDataFromConnex()\n");
        // We use the standard procedure
        return readDataFromConnex(buffer, vectorCount, startVectorIndex);
    }

    //assert(vectorCount == 1);
    assert(numElemCount < vectorCount * CONNEX_VECTOR_LENGTH);

    void *bufferTmp = malloc(vectorCount * CONNEX_VECTOR_LENGTH * sizeof(TYPE_ELEMENT));

    // 2018_04_03
    /*
    if (buffer == NULL) {
        buffer = new TYPE_ELEMENT[vectorCount * CONNEX_VECTOR_LENGTH];
    }
    */
    assert(buffer != NULL);

    threadMutex.lock();

    connex_io_descriptor.type = IO_READ_OPERATION;

    /* Use IO_LS_ADDRESS macro to mask the least significant 10 bits */
    connex_io_descriptor.lsAddress = IO_LS_ADDRESS(startVectorIndex);

    /* Use IO_VECTOR_COUNT macro to mask the least significant 10 bits */
    connex_io_descriptor.vectorCount = IO_VECTOR_COUNT(vectorCount);

    /* Issue the command */
    IGNORE_RESULT( write(ioWriteFifo, &connex_io_descriptor,
                         sizeof(connex_io_descriptor)) );

    /* Flush the descriptor */
    IGNORE_RESULT( write(ioWriteFifo, NULL, 0) );

    /* Read the data */
    if (readFromPipe(ioReadFifo, bufferTmp,
                     vectorCount * CONNEX_VECTOR_LENGTH *
                     sizeof(TYPE_ELEMENT)) < 0) {
        threadMutex.unlock();
        free(bufferTmp);
        throw string("Error reading from memory FIFO");
    }

    memcpy(buffer, bufferTmp, numElemCount * sizeof(TYPE_ELEMENT));
    /* We do NOT pad with 0 (or something else) elements
         numElemCount.. (CONNEX_VECTOR_LENGTH - 1) of
         buffer because we might experience a buffer overflow. */

    free(bufferTmp);

    threadMutex.unlock();

    return buffer;
}

/*
 * Reads one int from the reduction FIFO
 *
 * @return the value read from the reduction FIFO
 */
int ConnexMachine::readReduction() {
    int result;

    readMultiReduction(1, &result);
    return result;
}

/*
 * Reads multiple values from the reduction FIFO
 *
 * @param count the number of int to be read
 * @param buffer the memory area where the results will be put
 */
int ConnexMachine::readMultiReduction(int count, void* buffer) {
    int result;
    threadMutexIR.lock();

    if (readFromPipe(reductionFifo, buffer, count * sizeof(int)) < 0) {
        threadMutexIR.unlock();
        throw string("Error reading from reduction FIFO");
        // TODO
        return -1;
    }

    threadMutexIR.unlock();

    return 0;
}


/*
 * Write the numResults reduction results from the FIFO in the
     possibly smaller or bigger array bufferRes.
     Eventually sign extend the result.
 * @param * @param * @param count the number of int to be read
 * @param bufferRes the memory area where the correct results will be put
 * @param sizeOfBufferResElement size in bytes of element in bufferRes
 * @param signExtend if true, perform sign extension (Connex does not do it)
 */
int ConnexMachine::readCorrectReductionResults(int count, void *bufferRes,
                                  int sizeOfBufferResElement, bool signExtend) {
    //assert(sizeOfBufferResElement == 2);
    //int countOrig = count;

    int numRedResultsToReadActually;
    if (signExtend) {
        numRedResultsToReadActually = count;
    }
    else {
        // We implement a RED i32 by reading 2 RED i16 results and combining them
        numRedResultsToReadActually = count * sizeOfBufferResElement /
                                      sizeof(TYPE_ELEMENT);
    }

  #ifdef DEBUG_ALEX
    printf("numRedResultsToReadActually = %d\n", numRedResultsToReadActually);
  #endif

    // The Connex processor returns a reduction result only on i32
    int *buffer = (int *)malloc(numRedResultsToReadActually * sizeof(int));
    assert(buffer != NULL);

  #ifdef DEBUG_ALEX
    printf("Calling readMultiReduction()\n");
    fflush(stdout);
  #endif

    int res = readMultiReduction(numRedResultsToReadActually, buffer);
    assert(res == 0);

    if (sizeOfBufferResElement == sizeof(TYPE_ELEMENT)) {
        // Return red. results in an 16-bit int array (e.g., for MatMul.i16)
        for (int i = 0; i < numRedResultsToReadActually; i++) {
            //printf("i = %d : calling readReduction()\n", i);
            //fflush(stdout);
            *((TYPE_ELEMENT *)(bufferRes) + i) = buffer[i] & REG_MAX_VAL;
        }
    }
    else
    if (sizeOfBufferResElement == sizeof(int)) {
        if (signExtend) {
            // Perform sign extension for standard reduction results
            assert(CONNEX_VECTOR_LENGTH == 128);
            /* NOTE: Connex 128 returns a result on 23 bits
               (among which 1 sign bit),
               which needs to be sign extended to i32
                  if it is actually a negative number. */

            for (int i = 0; i < count; i++) {
                if (res > (1UL << (16 + LOG2_CONNEX_VECTOR_LENGTH - 1))) {
                    ((int *)bufferRes)[i] = buffer[i] | 0xFF800000;
                }
                else {
                    ((int *)bufferRes)[i] = buffer[i];
                }
            }
            /*
            printf("%d\n", (res > (1UL << (16 + LOG2_CONNEX_VECTOR_LENGTH - 1))));

            // res |= 0xFF800000; // for CONNEX_VECTOR_LENGTH = 128
            for (i = 16 + LOG2_CONNEX_VECTOR_LENGTH; i < 32; i++) {
                res |= (1 << i);
            }
            */
        }
        else {
            // Processing red. results coming from my STANDARD RED i32 emulation
            int bufferIndex = 0;
            for (int i = 0; i < count; i++) {
                /* The RED i32 operation I emulate on Connex returns 2 results:
                 *   - one with result of RED i16 for lower 16 bits of the i32s
                 *   - one with result of RED i16 for higher 16 bits of the i32s.
                 */
                *((int *)(bufferRes) + i) = buffer[bufferIndex++] +
                                            (buffer[bufferIndex++] << 16);
            }
        }
    }

    free(buffer);

    return 0;
}


/*
 * Checks the FPGA accelerator architecture against the OPINCAA target architecture
 *
 * @return accelerator revision string
 */
string ConnexMachine::checkAcceleratorArchitecture() {
    void *map_addr = mmap(NULL, 64, PROT_READ, MAP_SHARED, registerFile, 0);

    if(map_addr == MAP_FAILED) {
        throw string("Failed to mmap accelerator register interface");
    }

    volatile unsigned int *regs = (volatile unsigned int *)map_addr;

    std::string archName = std::string((char*)regs);
    archName = string(archName.rbegin(), archName.rend());

    if (archName.compare(targetArchitecture) != 0) {
        printf("Alex: archName = %s, targetArchitecture = %s. "
               "NOT stopping though.\n",
               archName.c_str(), targetArchitecture.c_str());
        //std::cout << "Alex: (NOT stopping though) Accelerator architecture (" + archName + ") does not match OPINCAA target architecture (" + targetArchitecture + ")");

        // Alex: we should uncomment this
        /* throw string("Accelerator architecture (" + archName +
                        ") does not match OPINCAA target architecture (" +
                        targetArchitecture+")"); */
    }

    std::string accRevision = std::to_string(regs[11]) +
                              std::to_string(regs[10]) +
                              std::to_string(regs[9]) +
                              std::to_string(regs[8]) +
                              std::to_string(regs[7]) +
                              std::to_string(regs[6]);

    return accRevision;

}

/**
 * Writes the specified command to the instruction FIFO (use with caution).
 *
 * @param command the command to write.
 */
void ConnexMachine::writeCommand(unsigned command) {
    if (write(distributionFifo, &command, sizeof command) != sizeof command) {
        throw string("Unable to write command to instruction FIFO.");
    }
}

/**
 * Writes the specified command to the instruction FIFO (use with caution).
 *
 * @param command the command to write.
 */
void ConnexMachine::writeCommands(vector<unsigned> commands){
    if (write(distributionFifo, &commands[0],
              commands.size() * sizeof(unsigned)) != commands.size() *
                                                     sizeof(unsigned)) {
        throw string("Unable to write command to instruction FIFO.");
    }
}

