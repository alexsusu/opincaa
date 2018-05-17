#include <iostream>
#include <fstream>
#include <thread>
#include <fcntl.h>
#include <sys/stat.h>
#include "ConnexSimulator.h"


extern ConnexState connexStateObj;


// Alex: changed connexVectors from unsigned short to TYPE_ELEMENT

/****************************************************************************
 *  Structure holding information for a IO transfer
 */
struct ConnexIoDescriptor
{
    unsigned type;
    unsigned lsAddress;
    unsigned vectorCount;
};


/****************************************************************************
 *  Constructor for class ConnexSimulator
 *
 * @param distributionDescriptorPath the path to the distribution FIFO on the file system
 * @param reductionDescriptorPath the path to the reduction FIFO on the file system
 * @param writeDescriptorPath the path to the write FIFO (ARM -> ARRAY) on the file system
 * @param readDescriptorPath the path to the read FIFO (ARRAY -> ARM) on the file system
 * @param accInfoPath the path to the accelerator identification file on the file system
 */
ConnexSimulator::ConnexSimulator(string distributionDescriptorPath,
                        string reductionDescriptorPath,
                        string writeDescriptorPath,
                        string readDescriptorPath,
                        string accInfoPath) {
    // 2018_02_10
    printf("ConnexSimulator::ConnexSimulator(): CONNEX_MEM_SIZE = %d\n",
		   CONNEX_MEM_SIZE);
    printf("ConnexSimulator::ConnexSimulator(): CONNEX_REG_COUNT = %d\n",
           CONNEX_REG_COUNT);

    localStore = new ConnexVector[CONNEX_MEM_SIZE];
    printf("ConnexSimulator::ConnexSimulator(): Allocating registerFile\n");
    registerFile = new ConnexVector[CONNEX_REG_COUNT];

    //cout<<"Opening simu:"<<distributionDescriptorPath<<endl;
    distributionDescriptor = openPipe(distributionDescriptorPath, O_RDWR);

    //cout<<"Opening simu:"<<writeDescriptorPath<<endl;
    writeDescriptor = openPipe(writeDescriptorPath, O_RDWR);

    //cout<<"Opening simu:"<<reductionDescriptorPath<<endl;
    reductionDescriptor = openPipe(reductionDescriptorPath, O_RDWR);

    //cout<<"Opening simu:"<<readDescriptorPath<<endl;
    readDescriptor = openPipe(readDescriptorPath, O_RDWR);

    setupAccInfo(accInfoPath);

    instructionQueue = new InstructionQueue(INSTRUCTION_QUEUE_LENGTH);

    codeInLoop = false;
    repeatCounter = 0;

    initiateThreads();
}

/****************************************************************************
 * Destructor for class ConnexSimulator
 */
ConnexSimulator::~ConnexSimulator() {
    // 2018_02_10
    delete localStore;
    delete registerFile;

    delete instructionQueue;
}

/****************************************************************************
 * Opens the FIFO at the specified path in the specified mode
 *
 * @param path the path to the FIFO
 * @param mode the mode of the FIFO
 * @return the FIFO descriptor
 */
int ConnexSimulator::openPipe(string pipePath, int mode)
{
    int fifoDescriptor;
    const char* path = pipePath.c_str();
    /* Try and create the pipe, if it already exists, this will return
     * -1, but we don't care
     */
    mkfifo(path, 0666);

    /* Try and attach to it */
    if((fifoDescriptor = open(path, mode)) < 0)
    {
        throw string("Unable to open FIFO ") + path;
    }

    cout << "FIFO " << pipePath << " succesfully opened!" << endl << flush;
    return fifoDescriptor;
}

/****************************************************************************
 * Creates a file with the identification of the simulated accelerator
 *
 * @param path the path to the FIFO
 */
void ConnexSimulator::setupAccInfo(string infoPath)
{
    ofstream infoFile;

    infoFile.open(infoPath);
    string archName("connex16-hm-generic");
    archName = string(archName.rbegin(), archName.rend());

    infoFile << archName << '\0';
    for(int i=infoFile.tellp(); i<48; i++) infoFile << " ";
    infoFile.close();
}

/****************************************************************************
 * Starts the threads for program and data transfer.
 * They don't stop unless killed. (Alex: e.g. with killall from bash;
 *   the coreThreadHandler() loops forever).
 */
void ConnexSimulator::initiateThreads() {
    ioThread = thread(&ConnexSimulator::ioThreadHandler, this);
    coreThread = thread(&ConnexSimulator::coreThreadHandler, this);
}

void ConnexSimulator::waitFinish()
{
    ioThread.join();
    coreThread.join();
}

/****************************************************************************
 * Handler method for IO transfers
 */
void ConnexSimulator::ioThreadHandler()
{
    cout << "Starting IO Thread..." << endl << flush;
    ConnexIoDescriptor ioDescriptor;

    try {
        for (;;) {
            cout << "Simu: Waiting for receive " << endl << flush;
            read(writeDescriptor, &ioDescriptor, sizeof(ioDescriptor));

          //#if 0
          #if 1
            printf("ioDescriptor:\n");
            printf("            type = %d\n", ioDescriptor.type);
            printf("            lsAddress = %d\n", ioDescriptor.lsAddress);
            printf("            vectorCount = %d\n", ioDescriptor.vectorCount);
            fflush(stdout);
          #endif

            //cout << "Simu: Received " << sizeof(ioDescriptor)
            //     << " Bytes" << endl << flush;
            performIO(ioDescriptor);
            //cout << "Simu: Perform IO " << endl << flush;
        }
    }
    // 2017_11_05
    catch(string ex) {
        cout << "Exception occured in ConnexSimulator::ioThreadHandler(): "
			 << ex << endl;
        exit(-1);
    }
}

/****************************************************************************
 * Handler method execution and reduction
 */

// Alex: printing the instruction trace getting executed
bool printDebugTrace = true; //false;

// Alex:
int cycleSim = 0;
float totalEnergyConsumed = 0.0;
//float powerConsumptionInstruction[];

void ConnexSimulator::coreThreadHandler() {
    int instruction;

    cout << "Starting Core Thread..." << endl << flush;

    try {
        for (;;) {
            if (codeInLoop) {
                Instruction *compiledInstruction = (instructionQueue->read());

                if (printDebugTrace) {
                  #ifdef PRINT_DEBUG_SIM
                     cout << " Running " << compiledInstruction->toString()
						  << endl;
                  #endif
                }

                executeInstruction(*compiledInstruction);

                if (printDebugTrace) {
                  // 2017_11_05
                  #define PRINT_DEBUG_SIM
                  #ifdef PRINT_DEBUG_SIM

                    if (compiledInstruction->getOpcode() == _IJMPNZ) {
                        cout << "Preparing to jump from loop iter #"
							 << repeatCounter << endl;
                    }

                    //cout << " Loop running "
					// 	   << compiledInstruction->toString() << endl;
                    if (compiledInstruction->mnemonic(
						  compiledInstruction->getOpcode()) != "print_chars") {
                        // cout << "  instr " << compiledInstruction->dump()
						//      << "" << endl;
                        printf("  instr = %s\n",
							   compiledInstruction->dump().c_str());
                    }
                  #endif
                    /*
					cout << " R0 = " << this->registerFile->getCellValue(0)
						 << endl;
                    cout << " R0(elem 0) = "
						 << this->registerFile[0].getCellValue(0) << endl;
                    // Alex: new code
                    cout << " R1(elem 0) = "
					     << this->registerFile[1].getCellValue(1) << endl;
					*/
                }

                /*
				cout << " Loop running "
					 << compiledInstruction->toString() << endl;
                cout <<" R0 =" << this->registerFile->cells[0] << endl;
				*/
                if (printDebugTrace) {
                  // 2017_11_05
                  #ifdef PRINT_DEBUG_SIM
                    if (compiledInstruction->getOpcode() == _REDUCE) {
                        printf("Before (final?) REDUCE in loop: number of "
                               "cycles simulated = %d "
							   "(add 1 more for the REDUCE)\n", cycleSim);
                        fflush(stdout);
                    }
                  #endif
                }
            }
            else {
                read(distributionDescriptor, &instruction, sizeof(instruction));

                Instruction *compiledInstruction = new Instruction(instruction);
                instructionQueue->push(compiledInstruction);

                if (printDebugTrace) {
                  #ifdef PRINT_DEBUG_SIM_MORE
                     cout << " Running " << compiledInstruction->toString()
						  << endl;
                  #endif
                }

                executeInstruction(*compiledInstruction);

                if (printDebugTrace) {
                  // 2017_11_05
                  #ifdef PRINT_DEBUG_SIM
                    if (compiledInstruction->mnemonic(
							compiledInstruction->getOpcode()) != "print_chars")
                        cout << " Finished running "
							 << compiledInstruction->dump() << ""
							 << endl << std::flush;
                  #endif
                    // cout << " R0 =" << this->registerFile->cells[0] << endl;
                    // cout << " R0(elem 0) = "
					//      << this->registerFile[0].getCellValue(0) << endl;
                    /*
                    printRegister(22);
                    printRegister(20);
                    printRegister(23);
                    */
                    // Alex: new code
                    // cout << " R1(elem 1) = "
                    //      << this->registerFile[1].getCellValue(1) << endl;
                }
                // cout << " Running "
				//      << compiledInstruction->toString() << endl;
                // cout << " R0 =" << this->registerFile->cells[0] << endl;

                if (printDebugTrace) {
                  // 2017_11_05
                  #ifdef PRINT_DEBUG_SIM
                    if (compiledInstruction->getOpcode() == _REDUCE) {
                        printf("Before (final?) REDUCE: number of cycles "
                               "simulated = %d (add 1 more for the REDUCE)\n",
							   cycleSim);
                        fflush(stdout);
                    }
                  #endif
                }
            }
            cycleSim++;
        }
    }
    // 2017_11_05
    catch (string ex) {
        cout << "Exception occured in ConnexSimulator::coreThreadHandler(): "
			 << ex << endl;
        exit(-1);
    }
}


ssize_t
force_all_io(ssize_t (*io)(int, void*, size_t), int fd, void *buf,
					       size_t count) {
    ssize_t total = 0;

    do {
        printf("ConnexSimulator::force_all_io(): count = %lu, total = %ld\n",
               count, total);
        fflush(stdout);

        total += io(fd, (char *)buf + total, count - total);
    } while (total != count);

    return total;
}

/****************************************************************************
 * Performs an IO operation specified by the IO descriptor
 *
 * @param ioDescriptor the descriptor for the IO operation
 */
void ConnexSimulator::performIO(ConnexIoDescriptor ioDescriptor) {
    printf("Entered ConnexSimulator::performIO(): "
		   "ioDescriptor.lsAddress = %d, ioDescriptor.vectorCount = %d\n",
             ioDescriptor.lsAddress, ioDescriptor.vectorCount);
    fflush(stdout);


    switch (ioDescriptor.type) {
        /* Write (from IO FIFO/pipe, normally data coming from CPU RAM) to
             Connex LS memory */
        case IO_WRITE_OPERATION: {
            /* From https://www.quora.com/How-can-I-use-variable-length-array-in-C++:
               (May 2016)
               "C++ doesn't support variable length array like C.
                Variable length array is C99 feature but it is not officially
                  part  of C++ so far.
                But compilers like g++ & clang++ allows Variable Length Arrays
                  (VLA) as an extension."
            //TYPE_ELEMENT connexVectors[CONNEX_VECTOR_LENGTH * (ioDescriptor.vectorCount + 1)];
            */
            TYPE_ELEMENT *connexVectors = new TYPE_ELEMENT[
                                                  CONNEX_VECTOR_LENGTH *
                                                    (ioDescriptor.vectorCount + 1)];

            printf("  ConnexSimulator::performIO(): Before force_all_io()\n");
            fflush(stdout);

            //force_all_io(read, writeDescriptor, connexVectors, sizeof(connexVectors));
            force_all_io(read, writeDescriptor, connexVectors,
                         sizeof(TYPE_ELEMENT) * CONNEX_VECTOR_LENGTH *
                           (ioDescriptor.vectorCount + 1));

            for (unsigned int i = 0; i < ioDescriptor.vectorCount + 1; i++) {
                localStore[ioDescriptor.lsAddress + i].write(connexVectors +
                                                             i * CONNEX_VECTOR_LENGTH);
            }

            // TODO: Write the ACK with the correct data
            write(readDescriptor, connexVectors, 4);
            write(readDescriptor, NULL, 0);

            delete connexVectors;
            break;
        }

        // Read from Connex LS memory (and put in IO FIFO/pipe, normally data for CPU RAM)
        case IO_READ_OPERATION:
            for (unsigned int i = 0; i < ioDescriptor.vectorCount + 1; i++) {
                force_all_io( (ssize_t (*)(int, void *, size_t)) write,
                              readDescriptor,
                              localStore[ioDescriptor.lsAddress + i].read(),
                              sizeof(TYPE_ELEMENT) * CONNEX_VECTOR_LENGTH);
            }
            write(readDescriptor, NULL, 0);
            break;

        default:
            throw string("Unknown IO operation type in "
                         "ConnexSimulator::performIO");
    }

}

/****************************************************************************
 * Executes the specified instruction on all active cells
 *
 * @param instruction the instruction to execute
 */
void ConnexSimulator::executeInstruction(Instruction instruction) {
  #ifdef ALEX_DEBUG
    printf("Entered ConnexSimulator::executeInstruction(): "
           "instruction.getOpcode() = %d\n", instruction.getOpcode());
    fflush(stdout);
    cout << instruction.disassemble() << flush;
  #endif


    switch (instruction.getOpcode()) {
        // Alex: adding support for debugging:
        case _PRINT_REG:
            printRegister(instruction.getLeft());
            break;

        case _PRINT_CHARS:
            printf("%c%c",
                     instruction.getValue() >> 8,
                     instruction.getValue() & 0xFF);
            break;

        case _ADD:
            registerFile[instruction.getDest()] =
				registerFile[instruction.getLeft()] +
                registerFile[instruction.getRight()];
            break;

        case _ADDC:
            /* 2018_03_26: Alex - making it work well with the initial value of
               the Carry flags (before I was adding the Carry after the add
               with the ADDC operands and this was resetting the initial value
               of the Carry flags).
            */
            registerFile[instruction.getDest()] =
				(connexStateObj.carryFlag + registerFile[instruction.getLeft()]) +
					registerFile[instruction.getRight()];
            break;

        case _SUB:
            registerFile[instruction.getDest()] =
                registerFile[instruction.getLeft()] -
                registerFile[instruction.getRight()];
            break;

        case _SUBC:
            /* 2018_04_30: Alex - making it work well with the initial value of
               the Carry flags (before I was subtracting the Carry after the
               sub with the SUBC operands and this was resetting the initial
               value of the Carry flags).
            */
            registerFile[instruction.getDest()] =
				registerFile[instruction.getLeft()] - connexStateObj.carryFlag -
					registerFile[instruction.getRight()];
            break;

        case _POPCNT:
            registerFile[instruction.getDest()] =
				registerFile[instruction.getLeft()].popcount();
            break;

        case _BIT_REVERSE:
            registerFile[instruction.getDest()] =
				registerFile[instruction.getLeft()].bitreverse();
            break;

        case _NOT:
            registerFile[instruction.getDest()] = ~registerFile[instruction.getLeft()];
            break;

        case _OR:
            registerFile[instruction.getDest()] =
				registerFile[instruction.getLeft()] | registerFile[instruction.getRight()];
            break;

        case _AND:
            registerFile[instruction.getDest()] =
				registerFile[instruction.getLeft()] & registerFile[instruction.getRight()];
            break;

        case _XOR:
            registerFile[instruction.getDest()] =
				registerFile[instruction.getLeft()] ^ registerFile[instruction.getRight()];
            break;

        case _EQ:
            registerFile[instruction.getDest()] =
				registerFile[instruction.getLeft()] == registerFile[instruction.getRight()];
            break;

        case _LT:
            registerFile[instruction.getDest()] =
 				registerFile[instruction.getLeft()] < registerFile[instruction.getRight()];
            break;

        case _ULT:
            registerFile[instruction.getDest()] =
				registerFile[instruction.getLeft()].ult(registerFile[instruction.getRight()]);
            break;

        case _SHL:
            registerFile[instruction.getDest()] =
				registerFile[instruction.getLeft()] << registerFile[instruction.getRight()];
            break;

        case _SHR:
            registerFile[instruction.getDest()] =
				registerFile[instruction.getLeft()].shr(registerFile[instruction.getRight()]);
            break;

        case _SHRA:
            registerFile[instruction.getDest()] =
				registerFile[instruction.getLeft()] >> registerFile[instruction.getRight()];
            break;

        case _ISHL:
            registerFile[instruction.getDest()] =
				registerFile[instruction.getLeft()] << instruction.getRight();
            break;

        case _ISHR:
            registerFile[instruction.getDest()] =
				registerFile[instruction.getLeft()] >> instruction.getRight();
            break;

        case _ISHRA:
            registerFile[instruction.getDest()] =
				registerFile[instruction.getLeft()].ishra(instruction.getRight());
            break;

        case _LDIX:
            registerFile[instruction.getDest()].loadIndex();
            break;

        case _LDSH: {
            // 2018_03_27:
            registerFile[instruction.getDest()] = connexStateObj.shiftReg;
            break;
        }

        case _WHERE_CRY:
            ConnexVector::Unconditioned_Setactive(connexStateObj.carryFlag);
            break;

        case _WHERE_EQ:
            ConnexVector::Unconditioned_Setactive(connexStateObj.eqFlag);
            break;

        case _WHERE_LT:
            ConnexVector::Unconditioned_Setactive(connexStateObj.ltFlag);
            break;

        case _END_WHERE:
            ConnexVector::Unconditioned_Setactive(1);
            break;

        case _CELL_SHL:
        case _CELL_SHR:
            handleShift(instruction);
            break;

        case _READ:
            registerFile[instruction.getDest()].loadFrom(localStore,
										registerFile[instruction.getRight()]);
            break;

        case _WRITE:
            registerFile[instruction.getLeft()].storeTo(localStore,
										registerFile[instruction.getRight()]);
            break;

        case _MULT:
            registerFile[instruction.getLeft()] * registerFile[instruction.getRight()];
            break;

        case _MULT_LO:
            registerFile[instruction.getDest()] = connexStateObj.multLow;
            break;

        case _MULT_HI:
            registerFile[instruction.getDest()] = connexStateObj.multHigh;
            break;

        case _REDUCE:
            handleReduction(instruction);
            break;

        case _NOP:
            break;

        case _QUIT:
            exit(0);

        case _VLOAD:
            registerFile[instruction.getDest()] =
										(TYPE_ELEMENT)instruction.getValue();
            break;

        case _IREAD: {
            TYPE_ELEMENT addr = instruction.getValue();

            // Alex: checking for out-of-bounds case
            if ( !(addr >= 0 && addr < CONNEX_MEM_SIZE))
                printf("iread access outside of bounds of Connex LS memory: "
                       "addr = %d\n", addr);
            assert(addr >= 0 && addr < CONNEX_MEM_SIZE &&
                   "iread access outside of bounds of Connex LS memory");

            registerFile[instruction.getDest()] = localStore[addr];
            break;
        }

        case _IWRITE: {
            TYPE_ELEMENT addr = instruction.getLeft();

            // Alex: checking for out-of-bounds case
            if ( !(addr >= 0 && addr < CONNEX_MEM_SIZE))
                printf("iwrite access outside of bounds of Connex LS memory: "
                       "addr = %d\n", addr);
            assert(addr >= 0 && addr < CONNEX_MEM_SIZE &&
                   "iwrite access outside of bounds of Connex LS memory");

            localStore[instruction.getValue()] = registerFile[addr];
            break;
        }

        case _SETLC:
            repeatCounter = instruction.getValue();
            break;

        case _SETLC_REDUCE: {
            // Alex: Experimental: We perform Max-reduce on the vector getLeft()
            TYPE_ELEMENT numIterations;
            numIterations = MIN_TYPE_ELEMENT;
            for (int idxMaxRed = 0; idxMaxRed < CONNEX_VECTOR_LENGTH; idxMaxRed++) {
                TYPE_ELEMENT valCrt = registerFile[instruction.getLeft()].getCellValue(idxMaxRed);
                if (numIterations < valCrt)
                    numIterations = valCrt;
            }
            assert(numIterations >= 0);
            repeatCounter = numIterations;

            printf("ConnexSimulator::executeInstruction(): case _SETLC_REDUCE: "
                   "set repeatCounter = %d\n", numIterations);

            // We set also R0 to numIterations
            registerFile[0] = numIterations;

            //return;
            break;
        }

        case _IJMPNZ:
            if (!repeatCounter) {
                codeInLoop = false;
            }
            else {
                repeatCounter--;
                codeInLoop = true;
                instructionQueue->displaceReadPointer(instruction.getValue());
            }
            //return;
            break;

        default:
           #ifdef ALEX_DEBUG
            printf("Executing invalid opcode %d\n", instruction.getOpcode());
            fflush(stdout);
           #endif
            throw string("Invalid instruction opcode!");
            //break;
    }

  #ifdef PRINT_TRACE
    FILE *fout = fopen("trace.txt", "at");
    assert(fout != NULL);

    fprintf(fout, "running %s", instruction.dump().c_str());
    if (instruction.getDest() != -1) {
      fprintf(fout, " dest = ");
      for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
          fprintf(fout, "%04hx ",
                  registerFile[instruction.getDest()].getCellValue(i));
      }
      fprintf(fout, "\n");
    }

    fclose(fout);
  #endif
}

/****************************************************************************
 * Executes a shift instruction on all active cells
 *
 * @param instruction the shift instruction to execute
 */
void ConnexSimulator::handleShift(Instruction instruction)
{
    connexStateObj.shiftReg.copyFrom(registerFile[instruction.getLeft()]);
    connexStateObj.shiftCountReg.copyFrom(registerFile[instruction.getRight()]);

    connexStateObj.shiftReg.shift(
                                instruction.getOpcode() == _CELL_SHL ? 1 : -1);
}

/****************************************************************************
 * Executes a reduction instruction
 *
 * @param instruction the reduction instruction to execute
 */
void ConnexSimulator::handleReduction(Instruction instruction)
{
    int sum = registerFile[instruction.getLeft()].reduce();
    write(reductionDescriptor, &sum, sizeof(sum));
    write(reductionDescriptor, NULL, 0);
}


void ConnexSimulator::printRegister(int index) {
    assert(index >= 0 && index < CONNEX_REG_COUNT);

    printf("R[%02d] (left is index 0) = ", index);
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        //printf("%hd ", registerFile[index].getCellValue(i));

        /* We need to use unsigned to avoid at least on x86 to sign extend
           if the number is negative as i16. */
        printf("%04x ", (unsigned short)registerFile[index].getCellValue(i));
    }
    printf("[END]\n");
    fflush(stdout);
}

