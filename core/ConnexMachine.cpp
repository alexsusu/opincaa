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

#define DEFAULT_DISTRIBUTION_FIFO   "/dev/xillybus_connex_instruction_32"
#define DEFAULT_REDUCTION_FIFO      "/dev/xillybus_connex_reduction_32"
#define DEFAULT_IO_WRITE_FIFO       "/dev/xillybus_connex_iowrite_32"
#define DEFAULT_IO_READ_FIFO        "/dev/xillybus_connex_ioread_32"
#define DEFAULT_REGISTER_FILE       "/dev/uio0"


/* At least libc's write() function has specified to complain about
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
static struct {
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
 * Here we must not put static qualifier for definition (non-external linkage).
 */
void ConnexMachine::addKernel(Kernel *kernel) {
    string name = kernel->getName();
    const string allowOverwrite = "allowOverwrite";
    const string allowRedefine = "allowRedefine";

    mapMutex.lock();

    if (kernels.count(name) > 0) {
        // No explicit request for overwrite
        if (name.find(allowOverwrite) == std::string::npos &&
            name.find(allowRedefine) == std::string::npos) {
            mapMutex.unlock();
            throw string("Kernel ") + name +
                         string(" already exists in ConnexMachine::addKernel");
        }
        else {
            kernels.erase(name);
        }
    }

    /*
    LucianP said on Jun 26th, 2017 that it is required to run initially the
      END_WHERE to enable all cells.

    // Erasing 1st instruction from the kernel, which should be an
    // END_WHERE, proved to be a BAD idea, since it is required to activate all
    // cells when the kernel starts.
    vector<InstructionType> &kernelInstrs = kernel->getInstructions();
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
string ConnexMachine::disassembleKernel(string kernelName) {
    if (!kernels.count(kernelName)) {
        throw string("Kernel ") + kernelName +
            string(" not found in ConnexMachine::disassembleKernel!");
    }

    Kernel *kernel = kernels.find(kernelName)->second;

    return kernel->disassemble();
}

Kernel *ConnexMachine::getKernel(string kernelName) {
    return kernels.count(kernelName) > 0 ? kernels[kernelName] : nullptr;
}


/*
 *
 */
string ConnexMachine::genLLVMISelManualCode(string kernelName) {
    if (!kernels.count(kernelName)) {
        throw string("Kernel ") + kernelName +
            string(" not found in ConnexMachine::genLLVMISelManualCode()!");
    }

    return kernels[kernelName]->genLLVMISelManualCode();
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

      #ifdef DEBUG_OPINCAA
        printf("ConnexMachine::readFromPipe(): totalBytesRead = %d\n",
               totalBytesRead);
        fflush(stdout);
      #endif
    } while (byteCount != totalBytesRead);

    return byteCount;
}

void ConnexMachine::InitFPTables() {
    // For SQRT.f16 emulation
    // From SoftFloat library, s_approxRecipSqrt_1Ks.c (related also to f16_sqrt.c)
    const uint16_t softfloat_approxRecipSqrt_1k0s[16] = {
        0xB4C9, 0xFFAB, 0xAA7D, 0xF11C, 0xA1C5, 0xE4C7, 0x9A43, 0xDA29,
        0x93B5, 0xD0E5, 0x8DED, 0xC8B7, 0x88C6, 0xC16D, 0x8424, 0xBAE1
    };
    const uint16_t softfloat_approxRecipSqrt_1k1s[16] = {
        0xA5A5, 0xEA42, 0x8C21, 0xC62D, 0x788F, 0xAA7F, 0x6928, 0x94B6,
        0x5CC7, 0x8335, 0x52A6, 0x74E2, 0x4A3E, 0x68FE, 0x432B, 0x5EFD
    };


    // For DIV.f16 emulation
    // From SoftFloat library, s_approxRecip_1Ks.c (related also to f16_div.c)
    const uint16_t softfloat_approxRecip_1k0s[16] = {
        0xFFC4, 0xF0BE, 0xE363, 0xD76F, 0xCCAD, 0xC2F0, 0xBA16, 0xB201,
        0xAA97, 0xA3C6, 0x9D7A, 0x97A6, 0x923C, 0x8D32, 0x887E, 0x8417
    };

    const uint16_t softfloat_approxRecip_1k1s[16] = {
        0xF0F1, 0xD62C, 0xBFA1, 0xAC77, 0x9C0A, 0x8DDB, 0x8185, 0x76BA,
        0x6D3B, 0x64D4, 0x5D5C, 0x56B1, 0x50B6, 0x4B55, 0x4679, 0x4211
    };


    BEGIN_KERNEL("Init_DIVf16");
      EXECUTE_IN_ALL(
        #define AUX             4

        int i;

        for (i = 0; i < 16; i++) {
            R(AUX) = softfloat_approxRecip_1k0s[i];
            NOP;
            LS[LS_ADDRESS_softfloat_approxRecip_1k0s + i] = R(AUX);

            R(AUX) = softfloat_approxRecip_1k1s[i];
            NOP;
            LS[LS_ADDRESS_softfloat_approxRecip_1k1s + i] = R(AUX);
        }

        for (i = 0; i < 16; i++) {
            R(AUX) = softfloat_approxRecipSqrt_1k0s[i];
            NOP;
            LS[LS_ADDRESS_softfloat_approxRecipSqrt_1k0s + i] = R(AUX);

            R(AUX) = softfloat_approxRecipSqrt_1k1s[i];
            NOP;
            LS[LS_ADDRESS_softfloat_approxRecipSqrt_1k1s + i] = R(AUX);
        }

        REDUCE R(AUX); // For sync with the CPU
      )
    END_KERNEL("Init_DIVf16");

    executeKernel("Init_DIVf16");
    printf("ConnexMachine::InitFPTables(): Returned from executeKernel().\n");
    fflush(stdout);

    readReduction();
    printf("ConnexMachine::InitFPTables(): Returned from readReduction().\n");
    fflush(stdout);
}


/*
 * Destructor for the ConnexMachine class
 *
 * Disposes of the kernel map and closes the associated file
 * descriptors
 */
ConnexMachine::~ConnexMachine() {
    for (auto &aPair : kernels) {
      #ifdef DEBUG_OPINCAA
        printf("ConnexMachine::~ConnexMachine(): freeing "
               "kernel %s (with ptr value = %p)\n",
               aPair.first.c_str(), aPair.second);
        fflush(stdout);
      #endif

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
    const char *distpath = distributionDescriptorPath.c_str();
    const char *redpath = reductionDescriptorPath.c_str();
    const char *wiopath = writeDescriptorPath.c_str();
    const char *riopath = readDescriptorPath.c_str();
    const char *regpath = registerInterfacePath.c_str();

    registerFile = open(regpath, O_RDONLY);

    if (registerFile < 0) {
        throw string("Unable to access accelerator registers");
    }

    printf("Accelerator revision is %s\n",
           checkAcceleratorArchitecture().c_str());

    distributionFifo = open(distpath, O_WRONLY);
    // NOT sure if this helps
    fcntl(distributionFifo, F_SETLEASE | F_GETFL | F_SETFL, 1048576); // 2020_04_24: 2019_09_17: Set a lease of 1 MB for this queue file to work with SSD.f16 and SAD.f16

    reductionFifo = open(redpath, O_RDONLY);
    // NOT sure if this helps
    fcntl(reductionFifo, F_SETLEASE | F_GETFL | F_SETFL, 1048576); // 2020_04_24: 2019_09_17: Set a lease of 1 MB for this queue file to work with SSD.f16 and SAD.f16

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

    InitFPTables();
}

#if 0
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
    const char *distpath = distributionDescriptorPath.c_str();
    const char *redpath = reductionDescriptorPath.c_str();
    const char *wiopath = writeDescriptorPath.c_str();
    const char *riopath = readDescriptorPath.c_str();

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
#endif

/*
 * Executes the kernel on the current ConnexMachine
 *
 * @param kernelName the name of the kernel to execute
 *
 * @throws string if the kernel is not found
 */
void ConnexMachine::executeKernel(string kernelName) {
    if (dontExecuteKernel == false)
        threadMutexIR.lock();

    if (kernels.count(kernelName) == 0) {
        threadMutexIR.unlock();
        throw string("Kernel ") + kernelName +
                        string(" not found in ConnexMachine::executeKernel!");
    }

    Kernel *kernel = kernels.find(kernelName)->second;

  //#ifdef DEBUG_OPINCAA
    printf("executeKernel(%s): kernel->size() = %d\n",
                                 kernelName.c_str(), kernel->size());
    fflush(stdout);
  //#endif

    if (dontExecuteKernel == true) { // 2020_03_29
        if (kernel->size() > 1000) {
            printf("OPINCAA program: ConnexMachine::executeKernel(): "
                "Request to NOT execute kernel --> halting.\n");
            fflush(stdout);

            exit(-1);
        }

        printf("OPINCAA program: ConnexMachine::executeKernel(): "
            "Request to NOT execute kernel --> returning.\n");
        fflush(stdout);

        return;
    }

    /* This writeTo() is NON-blocking for Zedboard (something explained by the
     *  fact it writes to a block device, e.g. /dev/xillybus_connex_instruction_32,
     *  that is supported by the Xillybus protocol using DMA).
     * But in the OPINCAA simulator it is blocking since writes to pipes are
     *     blocking until all the data to be written is put in the internal
     *     buffer of the pipe, from where the consumer can later read the data.
     */
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
           "if vectorCount == 0, it crashes Connex on Zedboard");
    assert(startVectorIndex >= 0 &&
           "startVectorIndex < 0 --> writeDataToConnex() will write outside "
           "the LS memory");

    if (dontExecuteKernel == true) { // 2020_03_29
        printf("OPINCAA program: ConnexMachine::writeDataToConnex(): "
               "Request to NOT execute kernel --> returning.\n");
        fflush(stdout);

        return vectorCount * CONNEX_VECTOR_LENGTH *
                             sizeof(ConnexVectorElementType);
    }

  #ifdef DEBUG_OPINCAA
    printf("Entered ConnexMachine::writeDataToConnex(): "
           "before threadMutex.lock();\n");
    fflush(stdout);
  #endif
    threadMutex.lock();

    connex_io_descriptor.type = IO_WRITE_OPERATION;
    /* Use IO_LS_ADDRESS macro to mask the least significant 10 bits */
    connex_io_descriptor.lsAddress = IO_LS_ADDRESS(startVectorIndex);
    /* Use IO_VECTOR_COUNT macro to keep only the least significant 10 bits */
    connex_io_descriptor.vectorCount = IO_VECTOR_COUNT(vectorCount);

    assert(startVectorIndex + vectorCount <= CONNEX_MEM_NUM_ROWS &&
           "startVectorIndex + vectorCount > CONNEX_MEM_NUM_ROWS --> "
           "writeDataToConnex() will write outside the LS memory");

  #ifdef DEBUG_OPINCAA
    printf("ConnexMachine::writeDataToConnex(): before write().\n");
    fflush(stdout);
  #endif
    /* Issue the command */
    IGNORE_RESULT( write(ioWriteFifo, &connex_io_descriptor,
                   sizeof(connex_io_descriptor)) );

  #ifdef DEBUG_OPINCAA
    printf("ConnexMachine::writeDataToConnex(): before 2nd write().\n");
    fflush(stdout);
  #endif
    /* Write the data */
    int bytesWritten = write(ioWriteFifo, buffer,
                             vectorCount * CONNEX_VECTOR_LENGTH *
                             sizeof(ConnexVectorElementType));

  #ifdef DEBUG_OPINCAA
    printf("ConnexMachine::writeDataToConnex(): before flushing write().\n");
    fflush(stdout);
  #endif
    /* Flush the descriptor */
    IGNORE_RESULT( write(ioWriteFifo, NULL, 0) );

  #ifdef DEBUG_OPINCAA
    printf("ConnexMachine::writeDataToConnex(): before readFromPipe().\n");
    fflush(stdout);
  #endif
    /* Read the ACK. NOTE: this is blocking */
    int response;
    readFromPipe(ioReadFifo, &response, sizeof(int));

    //TODO: verify response

  #ifdef DEBUG_OPINCAA
    printf("ConnexMachine::writeDataToConnex(): before threadMutex.unlock().\n");
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
                                            unsigned numElemCount,
                                            unsigned startVectorIndex) {
    int remainderCVL = numElemCount % CONNEX_VECTOR_LENGTH;
    unsigned vectorCount = numElemCount / CONNEX_VECTOR_LENGTH +
                            (remainderCVL > 0);

  #ifdef DEBUG_OPINCAA
    printf("Entered ConnexMachine::writeDataToConnexPartial(): buffer = %p, "
            "numElemCount = %d, startVectorIndex = %d (vectorCount = %d).\n",
            buffer, numElemCount, startVectorIndex, vectorCount);
    //printf("    numElemCount = %d\n", numElemCount);
    //printf("    startVectorIndex = %d\n", startVectorIndex);
    fflush(stdout);
  #endif

    if (remainderCVL == 0) {
        // We use the standard procedure
       #ifdef DEBUG_OPINCAA
        printf("writeDataToConnexPartial(): calling standard writeDataToConnex()\n");
        fflush(stdout);
       #endif
        return writeDataToConnex(buffer, vectorCount, startVectorIndex);
    }

    //assert(numElemCount < vectorCount * CONNEX_VECTOR_LENGTH);

    int numTotalElemCount = vectorCount * CONNEX_VECTOR_LENGTH;

  #ifdef DEBUG_OPINCAA
    printf("writeDataToConnexPartial(): numTotalElemCount = %d\n", numTotalElemCount);
    printf("writeDataToConnexPartial(): numElemCount = %d\n", numElemCount);
    fflush(stdout);
  #endif

    void *bufferTmp = malloc(numTotalElemCount * sizeof(ConnexVectorElementType));

    memcpy(bufferTmp, buffer, numElemCount * sizeof(ConnexVectorElementType));
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
    printf("writeDataToConnexPartial(): Before memset()\n");
    fflush(stdout);
  #endif
    memset(((char *)bufferTmp) + numElemCount * sizeof(ConnexVectorElementType),
            0, (numTotalElemCount - numElemCount) * sizeof(ConnexVectorElementType));
  #ifdef DEBUG_OPINCAA
    printf("After memset()\n");
    fflush(stdout);
  #endif



    int bytesWritten = writeDataToConnex(bufferTmp, vectorCount,
                                         startVectorIndex);


    free(bufferTmp);
  #ifdef DEBUG_OPINCAA
    printf("After free()\n");
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
void *ConnexMachine::readDataFromConnex(void *buffer, unsigned vectorCount,
                                        unsigned startVectorIndex) {
    assert(vectorCount > 0 &&
           "if vectorCount == 0, it crashes Connex on Zedboard");
    assert(startVectorIndex >= 0 &&
           "startVectorIndex < 0 --> readDataFromConnex() will read outside "
           "the LS memory");

    if (dontExecuteKernel == true) { // 2020_03_29
        printf("OPINCAA program: ConnexMachine::readDataFromConnex(): "
               "Request to NOT execute kernel --> returning.\n");
        fflush(stdout);

        return buffer;
    }

    /*
    if (buffer == NULL) {
        buffer = new ConnexVectorElementType[vectorCount * CONNEX_VECTOR_LENGTH];
    }
    */
    assert(buffer != NULL);

    threadMutex.lock();

    connex_io_descriptor.type = IO_READ_OPERATION;
    /* Use IO_LS_ADDRESS macro to mask the least significant 10 bits */
    connex_io_descriptor.lsAddress = IO_LS_ADDRESS(startVectorIndex);
    /* Use IO_VECTOR_COUNT macro to keep only the least significant 10 bits */
    connex_io_descriptor.vectorCount = IO_VECTOR_COUNT(vectorCount);

    assert(startVectorIndex + vectorCount <= CONNEX_MEM_NUM_ROWS &&
           "startVectorIndex + vectorCount > CONNEX_MEM_NUM_ROWS --> "
           "readDataFromConnex() will read outside the LS memory");

    /* Issue the command */
    IGNORE_RESULT( write(ioWriteFifo, &connex_io_descriptor,
                         sizeof(connex_io_descriptor)) );

    /* Flush the descriptor */
    IGNORE_RESULT( write(ioWriteFifo, NULL, 0) );

    /* Read the data */
    if (readFromPipe(ioReadFifo, buffer,
                    vectorCount * CONNEX_VECTOR_LENGTH *
                    sizeof(ConnexVectorElementType)) < 0) {
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
void *ConnexMachine::readDataFromConnexPartial(void *buffer,
                                               unsigned numElemCount,
                                               unsigned startVectorIndex,
                                               unsigned startOffsetElem
                                               ) {
    int remainderCVL = numElemCount % CONNEX_VECTOR_LENGTH;
    unsigned vectorCount = numElemCount / CONNEX_VECTOR_LENGTH +
                            (remainderCVL > 0);

  #ifdef DEBUG_OPINCAA
    printf("Entered readDataFromConnexPartial(): buffer = %p, "
               "numElemCount = %d, startVectorIndex = %d (vectorCount = %d).\n",
               buffer, numElemCount, startVectorIndex, vectorCount);
    fflush(stdout);
  #endif

    if (remainderCVL == 0) {
      #ifdef DEBUG_OPINCAA
        printf("readDataFromConnexPartial(): calling standard readDataFromConnex()\n");
      #endif
        // We use the standard procedure
        return readDataFromConnex(buffer, vectorCount, startVectorIndex);
    }

    //assert(vectorCount == 1);
    //assert(numElemCount < vectorCount * CONNEX_VECTOR_LENGTH);

    void *bufferTmp = malloc(vectorCount * CONNEX_VECTOR_LENGTH *
                             sizeof(ConnexVectorElementType));

    // 2018_04_03
    /*
    if (buffer == NULL) {
        buffer = new ConnexVectorElementType[vectorCount * CONNEX_VECTOR_LENGTH];
    }
    */
    assert(buffer != NULL);

    void *bTmp = readDataFromConnex(bufferTmp, vectorCount, startVectorIndex);
    // small-TODO: we should catch exception thrown by readDataFromConnex() to do free(bufferTmp) before throwing it further

    memcpy(((char *)buffer) + startOffsetElem,
           ((char *)bufferTmp) + startOffsetElem,
           numElemCount * sizeof(ConnexVectorElementType) - startOffsetElem);
    /* We do not need to pad with 0 (or something else) elements
         numElemCount.. vectorCount * CONNEX_VECTOR_LENGTH of
         buffer - also if we do it we might?? experience a buffer overflow. */

    free(bufferTmp);

    return buffer;
}

/*
 * Reads one int from the reduction FIFO
 *
 * @return the value read from the reduction FIFO
 */
int ConnexMachine::readReduction() {
    int result;

    readReductionResults(&result, 1);
    return result;
}

/*
 * Reads multiple values from the reduction FIFO
 *
 * @param count the number of int to be read
 * @param buffer the memory area where the results will be put
 */
int ConnexMachine::readReductionResults(void *buffer, int count) {
    int result;

    if (dontExecuteKernel == true) { // 2020_03_29
        printf("OPINCAA program: ConnexMachine::readReductionResults(): "
               "Request to NOT execute kernel --> returning.\n");
        fflush(stdout);

        return 0;
    }

    threadMutexIR.lock();

    if (readFromPipe(reductionFifo, buffer, count * sizeof(int)) < 0) {
        threadMutexIR.unlock();
        throw string("Error reading from reduction FIFO");
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
int ConnexMachine::readCorrectReductionResults(void *bufferRes, int count,
                                  int sizeOfBufferResElement, bool signExtend /* Specified in ConnexMachine.h: = false */) {
    //assert(sizeOfBufferResElement == 2);
    //int countOrig = count;

    int numRedResultsToReadActually;
    if (signExtend) {
        numRedResultsToReadActually = count;
    }
    else {
        // We implement a RED i32 by reading 2 RED u16 results and combining them
        numRedResultsToReadActually = count * sizeOfBufferResElement /
                                      sizeof(ConnexVectorElementType);
    }

    dprintf("ConnexMachine::readCorrectReductionResults(): "
            "numRedResultsToReadActually = %d\n", numRedResultsToReadActually);
    dfflush(stdout);

    // The Connex processor returns a reduction result only on i32
    int *bufferRed = (int *)malloc(numRedResultsToReadActually * sizeof(int));
    assert(bufferRed != NULL);

    dprintf("Calling readReductionResults()\n");
    dfflush(stdout);

    int res = readReductionResults(bufferRed, numRedResultsToReadActually);
    assert(res == 0);

    if (sizeOfBufferResElement == sizeof(ConnexVectorElementType)) {
        // Return red. results in an (16-bit) short array (e.g., for MatMul.i16)
        for (int i = 0; i < numRedResultsToReadActually; i++) {
            printf("  i = %d : writing in bufferRes[i] val %d\n", i, bufferRed[i] & REG_MAX_VAL);
            fflush(stdout);
            //*(((ConnexVectorElementType *)bufferRes) + i) = bufferRed[i] & REG_MAX_VAL;
            //*(((ConnexVectorElementType *)bufferRes) + i) = *((ConnexVectorElementType *)&bufferRed[i]); // 2019_10_11
            *(((ConnexVectorElementType *)bufferRes) + i) = *((ConnexVectorElementType *)(bufferRed + i)); // 2020_04_23
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
                    ((int *)bufferRes)[i] = bufferRed[i] | 0xFF800000;
                }
                else {
                    ((int *)bufferRes)[i] = bufferRed[i];
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
                /*
                printf("readCorrectReductionResults(): i = %d: %d %d\n", i, bufferRed[bufferIndex], bufferRed[bufferIndex + 1]);
                fflush(stdout);
                */
                /* The RED i32 operation I emulate on Connex returns 2 results:
                 *   - one with result of RED i16 for lower 16 bits of the i32s
                 *   - one with result of RED i16 for higher 16 bits of the i32s.
                 */
                *((int *)(bufferRes) + i) = bufferRed[bufferIndex++] +
                                            (bufferRed[bufferIndex++] << 16);
            }
        }
    }

    free(bufferRed);

    return 0;
}


// Compute index of highest bit set in a u32
int IHSB_u32_CPU(unsigned int x) {
    int i;
    unsigned int tmp = 1UL << 31;

    for (i = 31; i >= 0; i--) {
        if (x & tmp)
            return i;
        tmp >>= 1;
    }
    return -1;
}

// Compute index of highest bit set in a u64
int IHSB_u64_CPU(uint64_t x) {
    int i;

#define ARM_OR_X86_GCC

#ifdef ARM_OR_X86_GCC
    /*
      Inspired from https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
      (and https://stackoverflow.com/questions/23856596/how-to-count-leading-zeros-in-a-32-bit-unsigned-integer)

      For ARM, CLS (Count Leading Zeroes)
        - see http://infocenter.arm.com/help/topic/com.arm.doc.dui0068b/CIHJGJED.html
        For x86, BSR(Q), Bit Scan Reverse: https://c9x.me/x86/html/file_module_x86_id_20.html
      From https://stackoverflow.com/questions/28423405/counting-the-number-of-leading-zeros-in-a-128-bit-integer:
        <<I would expect gcc to implement each __builtin_clzll using the bsrq instruction
            - bit scan reverse, i.e., most-significant bit position - in conjunction with
            an xor, (msb ^ 63), or sub, (63 - msb), to turn it into a leading zero count.
            gcc might generate lzcnt instructions with the right -march= (architecture) options.>>
    */
    return 63 - __builtin_clzll(x);
#else
  /*
   Other efficient solutions of implementing this funtion are proposed at
     https://stackoverflow.com/questions/23856596/how-to-count-leading-zeros-in-a-32-bit-unsigned-integer:

     - using an OR-prefix/scan operation on the bits of x
       <<
       x = x | (x >> 1);
       x = x | (x >> 2);
       x = x | (x >> 4);
       x = x | (x >> 8);
       x = x | (x >>16);
       return pop(~x);
       [...]
       Most architecture have a 1 cycle pop instruction which can be accessed via compiler
         builtins (eg. gcc's __builtin_pop).>>

     - <<This is probably the optimal way to do it in pure C:
        int clz(uint32_t x)
        {
            static const char debruijn32[32] = {
                0, 31, 9, 30, 3, 8, 13, 29, 2, 5, 7, 21, 12, 24, 28, 19,
                1, 10, 4, 14, 6, 22, 25, 20, 11, 15, 23, 26, 16, 27, 17, 18
            };
            x |= x>>1;
            x |= x>>2;
            x |= x>>4;
            x |= x>>8;
            x |= x>>16;
            x++;
            return debruijn32[x*0x076be629>>27];
        }>>
    */

  //#define START_INDEX 63
  //#define START_INDEX 49
  #define START_INDEX 48
  //#define START_INDEX 50
  //#define START_INDEX 47
  //#define START_INDEX 45
  //#define START_INDEX 44
  //#define START_INDEX 43
  //#define START_INDEX 42
  //#define START_INDEX 41
  // BAD results: #define START_INDEX 40
  //#define START_INDEX 35
  //#define START_INDEX 20
    uint64_t tmp = 1ULL << START_INDEX;

    for (i = START_INDEX; i >= 0; i--) {
        if (x & tmp)
            return i;
        tmp >>= 1;
    }
    return -1;
#endif
}

//#define DEBUG_OPINCAA
int ConnexMachine::readReductionResultsAndComputeF16(void *resultsF16,
                                                      int numElems
                                                      //unsigned short *resultsF16,
                                                      ) {
//#define OLD_WAY_OF_DOING_REDUCTION_FOR_ALL_F16
#ifdef OLD_WAY_OF_DOING_REDUCTION_FOR_ALL_F16
    #define NUM_RED_RESULTS_PER_F16 (3 + 2)
#else
    #define NUM_RED_RESULTS_PER_F16 (3 + 6)
#endif
    //
    #define F16_MANTISSA_BITS 10
    #define F16_EXPONENT_BITS 5
    //
    #define F16_MANTISSA_MASK  0x03FF
    #define F16_EXPONENT_MASK  0x7C00
    #define F16_SIGN_MASK      0x8000
    #define F16_HIDDENBIT_MASK 0x0400
    //
    // "Main" NaN value:
    // This should be a signalling NAN:
    #define F16_NAN 0x7C01
    #define F16_NAN_2 0xFC01
    // This should be a quiet NAN:
    #define F16_NAN_3 0xFC80
    #define F16_NAN_4 0xFE00
    #define F16_NAN_5 0x7E00
    #define F16_NAN_6 0x7C01
    //
    #define F16_INF_POSITIVE 0x7C00
    #define F16_INF_NEGATIVE 0xFC00
    //
    //#define F16_MAX_EXP 0x1F
    #define F16_MAX_EXP ((1UL << F16_EXPONENT_BITS) - 1)

  #define ROUND_TO_NEAREST


    int i;
    int *bufferRed = (int *)malloc(numElems * NUM_RED_RESULTS_PER_F16 *
                                   sizeof(int));
    assert(bufferRed != NULL);

  #ifdef DEBUG_OPINCAA
    //printf("readReductionResultsAndComputeF16(): START_INDEX = %d\n", START_INDEX);
    printf("readReductionResultsAndComputeF16(): "
           "Calling readReductionResults(%d, ...)\n",
            numElems * NUM_RED_RESULTS_PER_F16);
    fflush(stdout);
  #endif
    int resRed = readReductionResults(bufferRed,
                                      numElems * NUM_RED_RESULTS_PER_F16);
    assert(resRed == 0);

    for (i = 0; i < numElems; i++) {
        int numNANs = bufferRed[i * NUM_RED_RESULTS_PER_F16];
        int numPosINFs = bufferRed[i * NUM_RED_RESULTS_PER_F16 + 1];
        int numNegINFs = bufferRed[i * NUM_RED_RESULTS_PER_F16 + 2];

      #ifdef DEBUG_OPINCAA
        printf("numNANs = %d\n", numNANs);
        printf("numPosINFs = %d\n", numPosINFs);
        printf("numNegINFs = %d\n", numNegINFs);
      #endif
        //
        // Get the sum of all mantissas:
  #ifdef OLD_WAY_OF_DOING_REDUCTION_FOR_ALL_F16
        int mantissaRes = bufferRed[i * NUM_RED_RESULTS_PER_F16 + 3];
        int expRes = bufferRed[i * NUM_RED_RESULTS_PER_F16 + 4] / CONNEX_VECTOR_LENGTH;
      #ifdef DEBUG_OPINCAA
        printf("mantissaRes = %d (0x%x)\n", mantissaRes, mantissaRes);
        //printf("mantissaRes >> 7 = %d (0x%x)\n", mantissaRes >> 7, mantissaRes >> 7);
        printf("expRes (adjusted) = %d\n", expRes);
      #endif
  #endif


        unsigned short resF16;

        if (numNANs != 0 || (numPosINFs != 0 && numNegINFs != 0)) {
            resF16 = F16_NAN;
        }
        else if (numPosINFs != 0) {
            resF16 = F16_INF_POSITIVE;
        }
        else if (numNegINFs != 0) {
            resF16 = F16_INF_NEGATIVE;
        }
        else {
            /* IMPORTANT: We now normalize and pack the result of reduction
               in an F16, on the CPU.
            */

        #ifndef OLD_WAY_OF_DOING_REDUCTION_FOR_ALL_F16
            /* This fails on ARM: assert(sizeof(long) == 64 / 8);
               So I use instead type: int64_t.
            */

            int64_t mantissaRes = 0;
            int64_t mantissaResAux;
            int expRes;

            /*
            IMPORTANT: The way I'm doing sum-reduction by storing the sum of
               mantissas on int64_t does NOT lose any precision. This should
               be better than the sequential way of summing up values in an
               f16 on the CPU (on the CPU we could accumulate a lot of error
               if e.g. we start by summing first f16s with big mantissas and
               then, more at the end, with much smaller mantissas).

            On the reduction FIFO we receive the sums of F16 numbers with:
                - exponent in 1..5
                - exponent in 6..10
                - exponent in 11..15
                - exponent in 16..20
                - exponent in 21..25
                - exponent in 26..30

            Supposing that the sum-reduced vector of f16 contains elements with
             the entire exponent range (1..30) the mantissa will have at most
             this number of bits:
               47 = 1 (hidden mantissa bit) +
                    10 (number of mantissa bits for f16) +
                    (30 - 1) (for alignment) +
                    7 (7 = log_2(128) for the overflow).
             These 47 bits might represent a value 2^6 times bigger than what is
               representable on f16, if all 47 are significant.
              (But I expect many cases to actually have the most
               significant bit less than index 47.)
            */
            expRes = 1;
            int expResAux = 0;
            for (int idxRed = 0; idxRed < 6; idxRed++) {
                //expResAux = idxRed * 5 + 1;

                mantissaResAux = bufferRed[i * NUM_RED_RESULTS_PER_F16 + 3 + idxRed];
                mantissaRes += mantissaResAux << expResAux;

              #ifdef DEBUG_OPINCAA
               // Macros found at https://stackoverflow.com/questions/30139983/how-do-i-identify-x86-vs-x86-64-at-compile-time-in-gcc
               #if defined(__x86_64__)
                printf("mantissaRes = %ld (0x%lx)\n",
                       mantissaRes, mantissaRes);
                printf("mantissaResAux = %ld (0x%lx)\n",
                       mantissaResAux, mantissaResAux);
               #else
                printf("mantissaRes = %lld (0x%llx)\n",
                       mantissaRes, mantissaRes);
                printf("mantissaResAux = %lld (0x%llx)\n",
                       mantissaResAux, mantissaResAux);
               #endif

                int idxTmp = i * NUM_RED_RESULTS_PER_F16 + 3 + idxRed;
                printf("  bufferRed[%d] = 0x%x\n",
                       idxTmp, bufferRed[idxTmp]);

                //printf("mantissaRes >> 7 = 0x%lx\n", mantissaRes >> 7);
                printf("expRes = %d\n", expRes);
                printf("expResAux = %d\n", expResAux);
                //printf("mantissaRes = 0x%llx\n", mantissaRes);
              #endif

                expResAux += 5;
            }
        #endif // ! STANDARD_WAY_OF_DOING_REDUCTION_FOR_ALL_F16

            int sgnRes = 0;
            /* If mantissaRes is negative, get absolute value and set sgnRes for
                negative result. */
            if (mantissaRes < 0) {
                sgnRes = F16_SIGN_MASK;

                /* We reverse the sign of mantissa if it is negative (we need to
                     pack positive value of mantissa in the final f16). */
                mantissaRes = -mantissaRes;
            }

          #ifdef DEBUG_OPINCAA
            printf("mantissaRes = %ld (0x%lx) (after making it positive)\n", mantissaRes, mantissaRes);
          #endif

            /* Re-normalization, first stage:
                - find out the index of highest bit set (IHBS) in mnt
            */
          #ifdef BOTHER_TO_DO_ON_CPU_WHAT_YOU_DO_ON_CONNEX_EVEN_ITS_NOT_WORTHY_BECAUSE_ON_CPU_WE_DONT_HAVE_A_BITREVERSE_INSTRUCTION
            int mantissaResRev = BitReverse_CPU_u32(mantissaRes);
            printf("mantissaResRev = %d (0x%x)\n", mantissaResRev, mantissaResRev);
          #else
           #ifdef STANDARD_WAY_OF_DOING_REDUCTION_FOR_ALL_F16
            int mantissaResIHSB = IHSB_u32_CPU(mantissaRes);
           #else
            int mantissaResIHSB = IHSB_u64_CPU(mantissaRes);
           #endif
           #ifdef DEBUG_OPINCAA
            printf("mantissaResIHSB = %d\n", mantissaResIHSB);
           #endif
          #endif


            // Copy-pasting the code from AddF32(), from FloatOps.cpp:
            // VERY IMPORTANT: shrPos can be negative also
            //int shrPos = numBits - (F32_MANTISSA_BITS + 1);
            int shrPos = mantissaResIHSB - F16_MANTISSA_BITS;
           #ifdef DEBUG_OPINCAA
            printf("shrPos = %d\n", shrPos);
           #endif

            /* We now do on mantissaRes a SHR by posSHR positions - this makes mantissaRes
               to have ISHB on F16_MANTISSA_BITS + 1. */



          #ifdef ROUND_TO_NEAREST
            int64_t mantissaGRTAux;
            int shrPosGRTAux = -1;
          #endif

            if (shrPos < 0) {
                mantissaRes <<= (-shrPos);
            }
            else {
              #ifdef ROUND_TO_NEAREST
                mantissaGRTAux = mantissaRes;
                shrPosGRTAux = shrPos;
               #ifdef DEBUG_OPINCAA
                printf("After normalization: mantissaGRTAux = 0x%016lx, shrPosGRTAux = %d\n",
                        mantissaGRTAux, shrPosGRTAux);
               #endif
                assert(shrPosGRTAux >= 0);
              #endif
                mantissaRes >>= shrPos;
            }


            /* We SHR by AUX (F32_MANTISSA_BITS + 1) positions both
               DST_MANTISSA_H and DST_MANTISSA_L together in order
               to keep the significant
                bits of the entire 32-bits (both _L and _H) of result of
                mantissa.
               This makes DST_MANTISSA_L hold F32_MANTISSA_BITS + 2 bits,
                 at most, or less (especially if we have denormals).

               After, this, the mantissa contains, as expected, the hidden bit.
             Note: we change DST_EXPONENT below
            */


            /* Correcting expRes for:
                  - denormals and
                  - mantissa results with 2 "integer" bits.
            */
            expRes += shrPos;



            /*
            if (expRes + shrPos >= 0) {
                expRes += shrPos;

              #ifdef DEBUG_OPINCAA
                printf("expRes (after re-normalization) = %d\n", expRes);
              #endif

              //#define ROUND_TO_NEAREST
              #ifdef ROUND_TO_NEAREST
                mantissaRes >>= shrPos;
              #endif

                mantissaRes >>= shrPos;
            }
            */
          #ifdef DEBUG_OPINCAA
            printf("expRes (after re-normalization) = %d\n", expRes);
          #endif

            /*
            IMPORTANT: we complete the "correction" of negative exponent by
             bringing to 1.
            */
            // We correct the underflows
            // This is underflow (NOT denormal)
            //if (expRes < 1)
            if (expRes < 0) {
                int shrPos = -(expRes - 1);

              //#define ROUND_TO_NEAREST
              #ifdef ROUND_TO_NEAREST
                if (shrPosGRTAux != -1) {
                    shrPosGRTAux += shrPos;
                }
                else {
                    mantissaGRTAux = mantissaRes;
                    shrPosGRTAux = shrPos;
                }

               #ifdef DEBUG_OPINCAA
                printf("When correcting negative expRes: mantissaResGRTAux = 0x%016lx, "
                       "shrPosGRTAux = %d\n",
                        mantissaGRTAux, shrPosGRTAux);
               #endif
                assert(shrPosGRTAux >= 1);
              #endif

                mantissaRes >>= shrPos;
                expRes = 1;
            }



        #ifdef DEBUG_OPINCAA
            printf("After normalization (and correction):\n");
            printf("  expRes = %d (0x%02x)\n", expRes, expRes);
            printf("  mantissaRes = %ld (0x%016lx)\n",
                   mantissaRes, mantissaRes);
        #endif

            // IMPORTANT: Check if the exponent overflows and, if so, declare infinity.
            // Treat overflows: declare INF
            if (expRes >= F16_MAX_EXP) {
                expRes = F16_MAX_EXP;
                mantissaRes = 0; // INF has mantissa 0
            }

            // We correct a denormal: we make exponent 1 be 0:
            if ((mantissaRes <= F16_MANTISSA_MASK) && (expRes == 1)) {
                expRes = 0;
            }

            // We set the result exponent INF if any of the input operands is INF
            //    if (exp1 == F16_MAX_EXP || exp2 == F16_MAX_EXP)
            //        expRes = F16_MAX_EXP;

            assert(mantissaRes <= F16_HIDDENBIT_MASK | F16_MANTISSA_MASK);



            /*
            assert(expRes != 0 || (expRes == 0 && mantissaRes == 0));

            if (expRes == 1) {
                // For the denormal case, the packed exp is 0
                expRes = 0;
            }
            // We correct a denormal: we make exponent 1 be 0:
            if ((mantissaRes <= F16_MANTISSA_MASK) && (expRes == 1)) {
                expRes = 0;
            }
            // else {
                mantissaRes &= F16_MANTISSA_MASK;
            // }
            */



            //assert(expRes != 0 || (expRes == 0 && mantissaRes == 0));

            /*
            if (expRes == 1) {
                // For the denormal case, the packed exp is 0
                expRes = 0;
            }
            else {
                mantissaRes &= F16_MANTISSA_MASK;
            }
            */


           #ifdef DEBUG_OPINCAA
            printf("expRes = %d\n", expRes);

            printf("After adjusting\n");
            //printf("mantissaResL = %hu (0x%hx)\n", mantissaResL, mantissaResL);
            //printf("mantissaResH = %hu (0x%hx)\n", mantissaResH, mantissaResH);
            //
           #ifdef STANDARD_WAY_OF_DOING_REDUCTION_FOR_ALL_F16
            printf("mantissaRes (adjusted to account for mantissa overflow) = %u (0x08%x)\n",
                        mantissaRes, mantissaRes);
           #else
            #if defined(__x86_64__)
            printf("mantissaRes (adjusted to account for mantissa overflow) = %lu (0x08%lx)\n",
                        mantissaRes, mantissaRes);
            #else
            printf("mantissaRes (adjusted to account for mantissa overflow) = %llu (0x08%llx)\n",
                        mantissaRes, mantissaRes);
            #endif
           #endif
          #endif


            // IMPORTANT: Check if the exponent overflows and, if so, declare infinity.
            // Treat overflows: declare INF
            if (expRes >= F16_MAX_EXP) {
                if (sgnRes > 0)
                    resF16 = F16_INF_NEGATIVE;
                else
                    resF16 = F16_INF_POSITIVE;
            }
            else {
              #ifdef DEBUG_OPINCAA
                printf("expRes (adjusted) = %d\n", expRes);
              #endif

                // We pack the result
                resF16 = (expRes << F16_MANTISSA_BITS);
                resF16 |= (mantissaRes & F16_MANTISSA_MASK);
                /* VERY IMPORTANT: We don't add sign bit now because we want to optimize rounding:
                    resF16 |= sgnRes;
                */
            }



          #ifdef ROUND_TO_NEAREST
           #ifdef DEBUG_OPINCAA
            printf("Computing rnd: mantissaGRTAux = 0x%016lx, shrPosGRTAux = %d\n",
                    mantissaGRTAux, shrPosGRTAux);
           #endif

            int L = mantissaRes & 1;

            shrPosGRTAux -= 1;
            // discardedBitsGRTAux keeps the discarded bits after
            //    re-normalization of mantissa.
            uint64_t discardedBitsGRTAux = (1UL << shrPosGRTAux) - 1;
            discardedBitsGRTAux &= mantissaGRTAux;
           #ifdef DEBUG_OPINCAA
            printf("The least significant %d bits of mantissaGRTAux are 0x%016lx\n",
                   shrPosGRTAux, discardedBitsGRTAux);
           #endif
            //
            mantissaGRTAux >>= shrPosGRTAux;
           #ifdef DEBUG_OPINCAA
            printf("mantissaGRTAux after SHR by %d pos is 0x%016lx\n",
                    shrPosGRTAux, mantissaGRTAux);
            printf("  (mantissaRes = 0x%016lx)\n", mantissaRes);
           #endif
            //int T = mantissaGRTAux & 1;
            // T, the sticky bit, is the OR over all the SHR bits of the mantissa
            int T = (discardedBitsGRTAux != 0);

            // This assert is violated if we have INF for result, in which case
            //   we make mantissaRes = 0:
            //     assert((mantissaGRTAux >> 1) == mantissaRes);
            int G = mantissaGRTAux & 1;
            int R = 0;
            //
            //printf("  (mantissaGRTAux = 0x%016lx)\n", mantissaGRTAux);

           #ifdef DEBUG_OPINCAA
            printf("L = %d\n", L);
            printf("G = %d\n", G);
            printf("R = %d\n", R);
            printf("T = %d\n", T);
           #endif

            //int rnd = G;
            int rnd = G & (T | L);
           #ifdef DEBUG_OPINCAA
            printf("rnd = %d\n", rnd);
           #endif

            /* If we overflow the mantissa after rounding (we add 1 to maximum
                              valid mantissa) then:
                  - if expRes <= 30 mantissa becomes 1.0000000000 (base 2).
                      If exprRes was 30 this implies setting to +/- INF.
                      So we need to increment the exponent.
                      But this is automatically done when we add 1 to res, where
                      res contains the packed result.
                  - if expRes == 31 we ca see we set in the instructions immediately
                      above rnd = 0, so we can't overflow mantissa
                        - we don't need to care about this case.
              VERY IMPORTANT: But we need to make sure we don't put in res the
                sign sgnRes, because then if we add rnd it can actually
                subtract it from the mantissa if the f16 is negative. */

            if (expRes != F16_MAX_EXP)
                resF16 += rnd;
          #endif // END ROUND_TO_NEAREST

            /* Only now we can put the sign bit to resF16 (since we can do
                rounding before and we want to optimize it).
            */
            resF16 |= sgnRes;
        } // END else case for non-INF, non-NAN numbers


        *((unsigned short *)resultsF16 + i) = resF16;
    } // END for (i = 0; i < numElems; i++)

    free(bufferRed);

    return 0;
} // END ConnexMachine::readReductionResultsAndComputeF16()
//#undef DEBUG_OPINCAA


/*
 * Checks the FPGA accelerator architecture against the OPINCAA target architecture
 *
 * @return accelerator revision string
 */
string ConnexMachine::checkAcceleratorArchitecture() {
    /*
    void *map_addr = mmap(NULL, 64, PROT_READ, MAP_SHARED, registerFile, 0);

    if (map_addr == MAP_FAILED) {
        throw string("Failed to mmap accelerator register interface");
    }

    volatile unsigned int *regs = (volatile unsigned int *)map_addr;
    */

    assert(registerFile != -1);

    #define LEN_MAX 1024
    int buffer[LEN_MAX];
    int numBytesRead = read(registerFile, buffer, LEN_MAX);
    assert(numBytesRead != 0);

    //std::string archName = std::string((char*)regs);
    std::string archName = std::string((char *)buffer);
    archName = string(archName.rbegin(), archName.rend());

    if (archName.compare(targetArchitecture) != 0) {
        printf("checkAcceleratorArchitecture(): archName = %s, targetArchitecture = %s. "
               "NOT stopping though.\n",
               archName.c_str(), targetArchitecture.c_str());
        /* std::cout << "(NOT stopping though) Accelerator architecture (" +
                 archName + ") does not match OPINCAA target architecture (" +
                 targetArchitecture + ")"); */

        throw string("Accelerator architecture (" + archName +
                        ") does not match OPINCAA target architecture (" +
                        targetArchitecture+")");
    }

    // Returns a string with all decimal values (concatenated) of the ints 11..6 from array regs.
    std::string accRevision = std::to_string(buffer[11]) +
                              std::to_string(buffer[10]) +
                              std::to_string(buffer[9]) +
                              std::to_string(buffer[8]) +
                              std::to_string(buffer[7]) +
                              std::to_string(buffer[6]);

    return accRevision;
}

/**
 * Writes the specified command to the instruction FIFO (use with caution).
 *
 * @param command the command to write.
 */
void ConnexMachine::writeCommand(InstructionType command) {
    if (write(distributionFifo, &command, sizeof(command)) != sizeof(command)) {
        throw string("Unable to write command to instruction FIFO.");
    }
}

/**
 * Writes the specified command to the instruction FIFO (use with caution).
 *
 * @param command the command to write.
 */
void ConnexMachine::writeCommands(vector<InstructionType> commands){
    if (write(distributionFifo, &commands[0],
              commands.size() * sizeof(InstructionType)) != commands.size() *
                                                     sizeof(InstructionType)) {
        throw string("Unable to write command to instruction FIFO.");
    }
}

