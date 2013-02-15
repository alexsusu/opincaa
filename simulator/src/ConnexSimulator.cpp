
#include <iostream>
#include <thread>
#include "ConnexSimulator.h"
#include "NamedPipes.h"

/*
 *	Structure holding information for a IO transfer
 */
struct ConnexIoDescriptor
{
	unsigned type;
    unsigned lsAddress;
    unsigned vectorCount;
};


/*
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
	distributionDescriptor = openPipe(distributionDescriptorPath, RDONLY);
	writeDescriptor = openPipe(writeDescriptorPath, RDONLY);

    reductionDescriptor = openPipe(reductionDescriptorPath, WRONLY);
	readDescriptor = openPipe(readDescriptorPath, WRONLY);

	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
	{
		cells[i] = new Cell(i);
	}

	cells[0]->connectLeft(cells[CONNEX_VECTOR_LENGTH - 1]);
	cells[0]->connectRight(cells[1]);
	for(int i=1; i<CONNEX_VECTOR_LENGTH - 1; i++)
	{
		cells[i]->connectLeft(cells[i - 1]);
		cells[i]->connectRight(cells[i + 1]);
	}
	cells[CONNEX_VECTOR_LENGTH - 1]->connectLeft(cells[CONNEX_VECTOR_LENGTH - 2]);
	cells[CONNEX_VECTOR_LENGTH - 1]->connectRight(cells[0]);

	initiateThreads();
}

/*
 * Destructor for class ConnexSimulator
 */
ConnexSimulator::~ConnexSimulator()
{
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
	{
		delete cells[i];
	}
}

/*
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

	cout << "FIFO " << pipePath << " succesfully opened!" << endl;
	return fifoDescriptor;
}

/*
 * Starts the threads for program and data transfer.
 * They don't stop unless killed.
 */
void ConnexSimulator::initiateThreads()
{
    thread ioThread = thread(&ConnexSimulator::ioThreadHandler, this);
    thread coreThread = thread(&ConnexSimulator::coreThreadHandler, this);
    ioThread.join();
    coreThread.join();
}

/*
 * Handler method for IO transfers
 */
void ConnexSimulator::ioThreadHandler()
{
	cout << "Starting IO Thread..." <<  endl;
	ConnexIoDescriptor ioDescriptor;
	while(1)
	{
        pread(writeDescriptor, &ioDescriptor, sizeof(ioDescriptor));
		performIO(ioDescriptor);
	}
}

/*
 * Handler method execution and reduction
 */
void ConnexSimulator::coreThreadHandler()
{
	cout << "Starting Core Thread..." <<  endl;
	int instruction;
	while(1)
	{
		pread(distributionDescriptor, &instruction, sizeof(instruction));
		executeInstruction(Instruction(instruction));
	}
}

/*
 * Performs an IO operation specified by the IO descriptor
 *
 * @param ioDescriptor the descriptor for the IO operation
 */
void ConnexSimulator::performIO(ConnexIoDescriptor ioDescriptor)
{
	short connexVector[CONNEX_VECTOR_LENGTH];
	switch(ioDescriptor.type)
	{
		case IO_WRITE_OPERATION:
			for(int i=0; i<ioDescriptor.vectorCount + 1; i++)
			{
				pread(writeDescriptor, connexVector, 2 * CONNEX_VECTOR_LENGTH);
				for(int j=0; j<CONNEX_VECTOR_LENGTH; j++)
				{
					cells[j]->write(connexVector[j], ioDescriptor.lsAddress + i);
				}
			}

			// TODO: Write the ACK with the correct data
			pwrite(readDescriptor, connexVector, 4);
			pwrite(readDescriptor, NULL, 0);
			break;
		case IO_READ_OPERATION:
			for(int i=0; i<ioDescriptor.vectorCount + 1; i++)
			{
				for(int j=0; j<CONNEX_VECTOR_LENGTH; j++)
				{
					connexVector[j] = cells[j]->read(ioDescriptor.lsAddress + i);
				}
				pwrite(readDescriptor, connexVector, 2 * CONNEX_VECTOR_LENGTH);
			}
			pwrite(readDescriptor, NULL, 0);
			break;
		default:
			throw string("Unknown IO operation type in ConnexSimulator::performIO");
	}
}

/*
 * Executes the specified instruction on all active cells
 *
 * @param instruction the instruction to execute
 */
void ConnexSimulator::executeInstruction(Instruction instruction)
{
	switch(instruction.getOpcode())
	{
		case _REDUCE:
			handleReduction(instruction);
			break;
		case _CELL_SHL:
        case _CELL_SHR:
			handleShift(instruction);
			break;
		default:
			handleLocalInstruction(instruction);
	}
}

/*
 * Executes a shit instruction on all active cells
 *
 * @param instruction the shift instruction to execute
 */
void ConnexSimulator::handleShift(Instruction instruction)
{
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
	{
		cells[i]->shiftInit(instruction.getLeft(), instruction.getRight());
	}

	bool done;
	do
	{
		for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
		{
			if(instruction.getOpcode() == _CELL_SHL)
			{
				cells[i]->shiftLeftStepStart();
			}
			else if(instruction.getOpcode() == _CELL_SHR)
			{
				cells[i]->shiftRightStepStart();
			}
			else
			{
				throw string("Unknown shift instruction!");
			}
		}
		done = true;
		for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
		{
			done = cells[i]->shiftStepFinish() & done;
		}
	}
	while(!done);
}

/*
 * Executes a reduction instruction
 *
 * @param instruction the reduction instruction to execute
 */
void ConnexSimulator::handleReduction(Instruction instruction)
{
	int sum = 0;
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
	{
		sum += cells[i]->readRegister(instruction.getLeft());
	}

	pwrite(reductionDescriptor, &sum, sizeof(sum));
	pwrite(reductionDescriptor, NULL, 0);
}

/*
 * Executes a local (cell) instruction on all active cells
 *
 * @param instruction the instruction to execute
 */
void ConnexSimulator::handleLocalInstruction(Instruction instruction)
{
	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++)
	{
		cells[i]->execute(instruction);
	}
}
