
#include <iostream>
#include <fstream>
#include <thread>
#include <fcntl.h>
#include <sys/stat.h>
#include "ConnexSimulator.h"

/****************************************************************************
 *	Structure holding information for a IO transfer
 */
struct ConnexIoDescriptor
{
	unsigned type;
    unsigned lsAddress;
    unsigned vectorCount;
};


/****************************************************************************
 *	Constructor for class ConnexSimulator
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
                        string accInfoPath)
{
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
ConnexSimulator::~ConnexSimulator()
{
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

	cout << "FIFO " << pipePath << " succesfully opened!" << endl<<flush;
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
 * They don't stop unless killed.
 */
void ConnexSimulator::initiateThreads()
{
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
	while(1)
	{

		//cout<<"Simu: Waiting for receive "<<endl<<flush;
		read(writeDescriptor, &ioDescriptor, sizeof(ioDescriptor));
#if 0
                printf("ioDescriptor:\n");
                printf("              type = %d\n", ioDescriptor.type);
                printf("              lsAddress = %d\n", ioDescriptor.lsAddress);
                printf("              vectorCount = %d\n", ioDescriptor.vectorCount);
#endif
		//cout<<"Simu: Received "<<sizeof(ioDescriptor)<<" Bytes"<<endl<<flush;
		performIO(ioDescriptor);
		//cout<<"Simu: Perform IO "<<endl<<flush;
	}
}

/****************************************************************************
 * Handler method execution and reduction
 */
void ConnexSimulator::coreThreadHandler()
{
	cout << "Starting Core Thread..." << endl << flush;
	int instruction;
	while(1)
	{
		if(codeInLoop)
		{
		    Instruction *compiledInstruction = (instructionQueue->read());
			executeInstruction(*compiledInstruction);
			//cout<<" Loop running "<< compiledInstruction->toString()<<endl;
			//cout <<" R0 ="<<this->registerFile->cells[0]<<endl;
		}
		else
		{
			read(distributionDescriptor, &instruction, sizeof(instruction));
			Instruction *compiledInstruction = new Instruction(instruction);
			instructionQueue->push(compiledInstruction);
			executeInstruction(*compiledInstruction);
			//cout<<" Running "<< compiledInstruction->toString()<<endl;
			//cout <<" R0 ="<<this->registerFile->cells[0]<<endl;
		}
	}
}


ssize_t
force_all_io(ssize_t (*io)(int, void*, size_t), int fd, void *buf, size_t count)
{
        ssize_t total = 0;

        do {
                total += io(fd, (char *)buf + total, count - total);
        } while (total != count);

        return total;
}

/****************************************************************************
 * Performs an IO operation specified by the IO descriptor
 *
 * @param ioDescriptor the descriptor for the IO operation
 */
void ConnexSimulator::performIO(ConnexIoDescriptor ioDescriptor)
{
	unsigned short connexVectors[CONNEX_VECTOR_LENGTH * (ioDescriptor.vectorCount + 1)];
	switch(ioDescriptor.type)
	{
		case IO_WRITE_OPERATION:
			force_all_io(read, writeDescriptor, connexVectors, sizeof(connexVectors));
			for(unsigned int i=0; i<ioDescriptor.vectorCount + 1; i++)
			{
				localStore[ioDescriptor.lsAddress + i].write(connexVectors + i * CONNEX_VECTOR_LENGTH);
			}

			// TODO: Write the ACK with the correct data
			write(readDescriptor, connexVectors, 4);
			write(readDescriptor, NULL, 0);
			break;

		case IO_READ_OPERATION:
			for(unsigned int i=0; i<ioDescriptor.vectorCount + 1; i++)
			{
				force_all_io(write, readDescriptor,
					  localStore[ioDescriptor.lsAddress + i].read(),
					  2 * CONNEX_VECTOR_LENGTH);
			}
			write(readDescriptor, NULL, 0);
			break;

		default:
			throw string("Unknown IO operation type in ConnexSimulator::performIO");
	}
}

/****************************************************************************
 * Executes the specified instruction on all active cells
 *
 * @param instruction the instruction to execute
 */
void ConnexSimulator::executeInstruction(Instruction instruction)
{
	// cout << instruction.disassemble() << flush;
	switch(instruction.getOpcode())
    {
        case _ADD:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] + registerFile[instruction.getRight()];
			return;
        case _ADDC:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] + registerFile[instruction.getRight()] + ConnexVector::carryFlag;
			return;
        case _SUB:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] - registerFile[instruction.getRight()];
			return;
        case _SUBC:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] - registerFile[instruction.getRight()] - ConnexVector::carryFlag;
			return;
        case _POPCNT:
            registerFile[instruction.getDest()] = registerFile[instruction.getLeft()].popcount();
            return;
        case _NOT:
			registerFile[instruction.getDest()] = ~registerFile[instruction.getLeft()];
			return;
        case _OR:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] | registerFile[instruction.getRight()];
			return;
        case _AND:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] & registerFile[instruction.getRight()];
			return;
        case _XOR:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] ^ registerFile[instruction.getRight()];
			return;
        case _EQ:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] == registerFile[instruction.getRight()];
			return;
        case _LT:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] < registerFile[instruction.getRight()];
			return;
        case _ULT:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()].ult(registerFile[instruction.getRight()]);
			return;
        case _SHL:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] << registerFile[instruction.getRight()];
			return;
        case _SHR:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()].shr(registerFile[instruction.getRight()]);
			return;
        case _SHRA:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] >> registerFile[instruction.getRight()];
			return;
        case _ISHL:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] << instruction.getRight();
			return;
        case _ISHR:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()] >> instruction.getRight();
			return;
        case _ISHRA:
			registerFile[instruction.getDest()] = registerFile[instruction.getLeft()].ishra(instruction.getRight());
			return;
        case _LDIX:
			registerFile[instruction.getDest()].loadIndex();
			return;
        case _LDSH:
			registerFile[instruction.getDest()] = ConnexVector::shiftReg;
			return;
		case _WHERE_CRY:
			ConnexVector::Unconditioned_Setactive(ConnexVector::carryFlag);
			return;
        case _WHERE_EQ:
			ConnexVector::Unconditioned_Setactive(ConnexVector::eqFlag);
			return;
        case _WHERE_LT:
			ConnexVector::Unconditioned_Setactive(ConnexVector::ltFlag);
			return;
        case _END_WHERE:
			ConnexVector::Unconditioned_Setactive(1);
			return;
        case _CELL_SHL:
        case _CELL_SHR:
			handleShift(instruction);
			return;
        case _READ:
            registerFile[instruction.getDest()].loadFrom(localStore, registerFile[instruction.getRight()]);
			return;
        case _WRITE:
            registerFile[instruction.getLeft()].storeTo(localStore, registerFile[instruction.getRight()]);
			return;
        case _MULT:
			registerFile[instruction.getLeft()] * registerFile[instruction.getRight()];
			return;
        case _MULT_LO:
			registerFile[instruction.getDest()] = ConnexVector::multLow;
			return;
        case _MULT_HI:
			registerFile[instruction.getDest()] = ConnexVector::multHigh;
			return;
        case _REDUCE:
			handleReduction(instruction);
			return;
        case _NOP:
			return;
        case _VLOAD:
			registerFile[instruction.getDest()] = (unsigned short int)instruction.getValue();
			return;
        case _IREAD:
			registerFile[instruction.getDest()] = localStore[instruction.getValue()];
			return;
        case _IWRITE:
			localStore[instruction.getValue()] = registerFile[instruction.getLeft()];
			return;
		case _SETLC:
			repeatCounter = instruction.getValue();
			return;
        case _IJMPNZ:
			if(!repeatCounter)
			{
				codeInLoop = false;
			}
			else
			{
				repeatCounter--;
				codeInLoop = true;
				instructionQueue->displaceReadPointer(instruction.getValue());
			}
			return;
        default: throw string("Invalid instruction opcode!");
    }
}

/****************************************************************************
 * Executes a shift instruction on all active cells
 *
 * @param instruction the shift instruction to execute
 */
void ConnexSimulator::handleShift(Instruction instruction)
{
	ConnexVector::shiftReg.copyFrom(registerFile[instruction.getLeft()]);
	ConnexVector::shiftCountReg.copyFrom(registerFile[instruction.getRight()]);

    ConnexVector::shiftReg.shift(instruction.getOpcode() == _CELL_SHL ? 1 : -1);
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

