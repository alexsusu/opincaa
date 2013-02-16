
#include "../../include/util/utils.h"
#include "../../include/util/timing.h"

#include "../../include/core/cnxvector.h"
#include "../../include/core/cnxvector_registers.h"

#include "../../include/core/io_unit.h"
#include "../../include/c_simu/c_simulator.h"

using namespace std;
#include <iostream>

#ifndef _MSC_VER //MS C++ compiler
	#include <unistd.h>
#else
	#include "../../ms_visual_c/fake_unistd.h"
#endif

#include <fcntl.h>

UINT_RED_REG_VAL (*EXECUTE_BATCH)(UINT16 dwBatchNumber);
UINT_RED_REG_VAL (*EXECUTE_BATCH_RED)(UINT16 dwBatchNumber);
UINT32 (*GET_MULTIRED_RESULT)(UINT_RED_REG_VAL* Results, UINT32 Limit);

int (*IO_WRITE_NOW)(void*);
int (*IO_WRITE_BEGIN)(void*);
int (*IO_WRITE_IS_ENDED)();
void (*IO_WRITE_WAIT_END)();
int (*IO_READ_NOW)(void*);

static UINT8 RunMode = REAL_HARDWARE_MODE;
int deinitialize()
{
    int result = PASS;
    switch(RunMode)
    {
        case VERILOG_SIMULATION_MODE:
            {
                close(cnxvector::pipe_read_32);
                close(cnxvector::pipe_write_32);
                close(io_unit::vpipe_read_32);
                close(io_unit::vpipe_write_32);
                break;
            }
        case REAL_HARDWARE_MODE:
            {
                close(cnxvector::pipe_read_32);
                close(cnxvector::pipe_write_32);
                close(io_unit::vpipe_read_32);
                close(io_unit::vpipe_write_32);
                break;
            }
        case C_SIMULATION_MODE:
            {
                c_simulator::deinitialize();
                break;
            }
    }
    cnxvector::deinitialize();
    io_unit::deinitialize();
    return result;
}

#ifndef S_IRUSR
    #define S_IRUSR 0
#endif // S_IRUSR

#ifndef S_IWUSR
    #define S_IWUSR 0
#endif // S_IWUSR

int initialize(UINT8 RunningMode)
{
    int result = PASS;
    RunMode = RunningMode;

    if(RunningMode == VERILOG_SIMULATION_MODE)
    {
        cnxvector::pipe_read_32 = open ("reduction_fifo_device",O_RDONLY | O_CREAT, S_IRUSR|S_IWUSR);
        cnxvector::pipe_write_32 = open ("program_fifo_device",O_WRONLY | O_CREAT, S_IRUSR|S_IWUSR);

        io_unit::vpipe_read_32 = open ("io_outbound_fifo_device",O_RDONLY | O_CREAT, S_IRUSR|S_IWUSR);
        io_unit::vpipe_write_32 = open ("io_inbound_fifo_device",O_WRONLY | O_CREAT, S_IRUSR|S_IWUSR);

        EXECUTE_BATCH = cnxvector::executeBatch;
        EXECUTE_BATCH_RED = cnxvector::executeBatchRed;
        GET_MULTIRED_RESULT = cnxvector::getMultiRedResult;

        IO_WRITE_NOW = io_unit::vwrite;
        IO_WRITE_BEGIN = io_unit::vwriteNonBlocking;
        IO_WRITE_IS_ENDED = io_unit::vwriteIsEnded;
        IO_WRITE_WAIT_END = io_unit::vwriteWaitEnd;

        IO_READ_NOW = io_unit::vread;
    }
    else if (RunningMode == REAL_HARDWARE_MODE)
    {
        cnxvector::pipe_read_32 = open ("/dev/xillybus_read_array2arm_32",O_RDONLY);
        cnxvector::pipe_write_32 = open ("/dev/xillybus_write_arm2array_32",O_WRONLY);

        io_unit::vpipe_read_32 = open ("/dev/xillybus_read_array2mem_32",O_RDONLY);
        io_unit::vpipe_write_32 = open ("/dev/xillybus_write_mem2array_32",O_WRONLY);

        EXECUTE_BATCH = cnxvector::executeBatch;
        EXECUTE_BATCH_RED = cnxvector::executeBatchRed;
        GET_MULTIRED_RESULT = cnxvector::getMultiRedResult;

        IO_WRITE_NOW = io_unit::vwrite;
        IO_WRITE_BEGIN = io_unit::vwriteNonBlocking;
        IO_WRITE_IS_ENDED = io_unit::vwriteIsEnded;
        IO_WRITE_WAIT_END = io_unit::vwriteWaitEnd;

        IO_READ_NOW = io_unit::vread;
    }
    else if (RunningMode == C_SIMULATION_MODE)
    {
        EXECUTE_BATCH = c_simulator::executeBatchOneReduce;
        EXECUTE_BATCH_RED = c_simulator::executeBatchOneReduce;
        GET_MULTIRED_RESULT = c_simulator::getMultiRedResult;

        IO_WRITE_NOW = c_simulator::vwrite;
        IO_WRITE_BEGIN = c_simulator::vwriteNonBlocking;
        IO_WRITE_IS_ENDED = io_unit::vwriteIsEnded;
        IO_WRITE_WAIT_END = io_unit::vwriteWaitEnd;

        IO_READ_NOW = c_simulator::vread;
        c_simulator::initialize();
    }
    else
    {
        perror("No running mode selected !");
        result = FAIL;
    }

    if ((RunningMode == REAL_HARDWARE_MODE) || (RunningMode == VERILOG_SIMULATION_MODE))
    {
        if (cnxvector::pipe_read_32 == -1)
        {
            perror("Failed to open the cnxvector::read pipe");
            result = FAIL;
        }


        if (cnxvector::pipe_write_32 == -1)
        {
            perror("Failed to open the cnxvector::write pipe");
            result = FAIL;
        }

        if (io_unit::vpipe_read_32 == -1)
        {
            perror("Failed to open the io_unit::read pipe");
            result = FAIL;
        }


        if (io_unit::vpipe_write_32 == -1)
        {
            perror("Failed to open the io_unit::write pipe");
            result = FAIL;
        }
    }
    cnxvector::initialize();
    io_unit::initialize();
    return result;
}

void initRand()
{
    int seed =  GetMilliCount();
    srand ( seed );
    printf("\n Running with seed = %d\n",seed);
}
INT32 randPar(INT32 limit)
{
    INT64 result = limit;
    result = result * rand() / ( RAND_MAX + 1.0 );
    return (INT32)result;
}

void eatRand(int times)
{
	int i;
	for (i=0; i< times; i++)
		printf(" Eating up %d-th rand number, %d\n",i,rand());


}

//INT64 randPar(INT64 limit)
//{
//	static int randTimes = 0;
//	randTimes++;
//	//printf(" Getting rand number for the %d-th time \n", randTimes);
//	return (INT64)( limit * rand() / ( RAND_MAX + 1.0 ) );
//}

/**
    Computes sum of first "numbers" consecutive nubers starting with start.
    Numbers are forced to UINT16.
    Sum is forced to UINT16 + log2(NUMBER_OF_MACHINES)

    Eg.
    SumRedofFirstXnumbers(1, 145) = 145
    SumRedofFirstXnumbers(2, 14) = 14 + 15 = 29
    SumRedofFirstXnumbers(1, 5) = 1+2+3+4+5 = 15

*/
INT32 SumRedOfFirstXnumbers(UINT32 numbers, UINT32 start)
{
    UINT32 x;
    UINT32 sum = 0;
    for (x = start; x < start + numbers; x++ ) sum += x;
    return sum & REDUCTION_SIZE_MASK;
}

void simpleClearLS(int ClearLsBnr)
{
    int BatchNumber = ClearLsBnr;
    int cnxvector_index;
    for (cnxvector_index = 0; cnxvector_index < MAX_CNXVECTORS; cnxvector_index++)
    {
        BEGIN_BATCH(BatchNumber);
            EXECUTE_IN_ALL(
                                R0 = 0;
                                NOP;
                                LS[cnxvector_index] = R0;
                          );
            END_BATCH(BatchNumber);
            EXECUTE_BATCH(BatchNumber);
        }
}

void simplePrintLS(int PrintLsBnr, INT32 LsIndex)
{
    int BatchNumber = PrintLsBnr;
    int cell_index;
    int result;
    for (cell_index = 0; cell_index < NUMBER_OF_MACHINES; cell_index++)
    {
        BEGIN_BATCH(BatchNumber);
            EXECUTE_IN_ALL(
                        R4 = 0;
                        R1 = INDEX;
                        R2 = cell_index;
                        R3 = (R1 == R2);
                      );
            EXECUTE_WHERE_EQ(
                        R4 = LS[LsIndex];
            )
            EXECUTE_IN_ALL(
                        REDUCE(R4);
            )
        END_BATCH(BatchNumber);
        result = EXECUTE_BATCH_RED(BatchNumber);
        if ((cell_index & 2) == 0) cout<< endl;
        cout<<"Mach "<< cell_index <<" : LS["<<LsIndex<<"] = "<<result<<"    ";
    }
    cout<< endl;
}
