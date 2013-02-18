
#include <iostream>
#include <thread>
#include "ConnexSimulator.h"
#include "NamedPipes.h"

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
 */
ConnexSimulator::ConnexSimulator(string distributionDescriptorPath,
						string reductionDescriptorPath,
						string writeDescriptorPath,
						string readDescriptorPath)
{
    //cout<<"Opening simu:"<<distributionDescriptorPath<<endl;
    distributionDescriptor = openPipe(distributionDescriptorPath, O_RDONLY);

	//cout<<"Opening simu:"<<writeDescriptorPath<<endl;
	writeDescriptor = openPipe(writeDescriptorPath, O_RDONLY);

    //cout<<"Opening simu:"<<reductionDescriptorPath<<endl;
    reductionDescriptor = openPipe(reductionDescriptorPath, O_WRONLY);

	//cout<<"Opening simu:"<<readDescriptorPath<<endl;
	readDescriptor = openPipe(readDescriptorPath, O_WRONLY);

	initiateThreads();
}

/****************************************************************************
 * Destructor for class ConnexSimulator
 */
ConnexSimulator::~ConnexSimulator()
{

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
	pmake(path, 0666);

	/* Try and attach to it */
    if((fifoDescriptor = popen(path, mode)) < 0)
	{
		throw string("Unable to open FIFO ") + path;
    }

	cout << "FIFO " << pipePath << " succesfully opened!" << endl<<flush;
	return fifoDescriptor;
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
		pread(writeDescriptor, &ioDescriptor, sizeof(ioDescriptor));
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
		pread(distributionDescriptor, &instruction, sizeof(instruction));
		executeInstruction(Instruction(instruction));
	}
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
			pread(writeDescriptor, connexVectors, sizeof(connexVectors));
			for(unsigned int i=0; i<ioDescriptor.vectorCount + 1; i++)
			{
				localStore[ioDescriptor.lsAddress + i].write(connexVectors + i * CONNEX_VECTOR_LENGTH);
			}

			// TODO: Write the ACK with the correct data
			pwrite(readDescriptor, connexVectors, 4);
			pwrite(readDescriptor, NULL, 0);
			break;

		case IO_READ_OPERATION:
			for(unsigned int i=0; i<ioDescriptor.vectorCount + 1; i++)
			{
				pwrite(readDescriptor,
					  localStore[ioDescriptor.lsAddress + i].read(),
					  2 * CONNEX_VECTOR_LENGTH);
			}
			pwrite(readDescriptor, NULL, 0);
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
	ConnexVector::shiftReg = registerFile[instruction.getLeft()];
	ConnexVector::shiftCountReg = registerFile[instruction.getRight()];

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
	pwrite(reductionDescriptor, &sum, sizeof(sum));
	//printf(" reduced sum = %d\n", sum);
	pwrite(reductionDescriptor, NULL, 0);
}

