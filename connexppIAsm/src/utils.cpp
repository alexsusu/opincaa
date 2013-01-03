#include "../include/utils.h"
#include "../include/core/vector.h"
#include "../include/c_simu/c_simulator.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int (*EXECUTE_KERNEL)(UINT16 dwBatchNumber);
int (*EXECUTE_KERNEL_RED)(UINT16 dwBatchNumber);

int (*IO_WRITE_NOW)(void*);
int (*IO_READ_NOW)(void*);

static UINT8 RunMode = REAL_HARDWARE_MODE;
int deinitialize()
{
    int result = PASS;
    switch(RunMode)
    {
        case VERILOG_SIMULATION_MODE:
            {
                close(vector::pipe_read_32);
                close(vector::pipe_write_32);
                break;
            }
        case REAL_HARDWARE_MODE:
            {
                close(vector::pipe_read_32);
                close(vector::pipe_write_32);
                break;
            }
        case C_SIMULATION_MODE:
            {
                c_simulator::deinitialize();
                break;
            }
    }
    vector::deinitialize();
    return result;
}
int initialize(UINT8 RunningMode)
{
    int result = PASS;
    RunMode = RunningMode;

    if(RunningMode == VERILOG_SIMULATION_MODE)
    {
        //open and close files to make sure they exist
        FILE * file;
        file = fopen("program.data","w");
        fclose(file);
        file = fopen("reduction.data","w");
        fclose(file);

        vector::pipe_read_32 = open ("reduction.data",O_RDONLY);
        vector::pipe_write_32 = open ("program.data",O_WRONLY);
        EXECUTE_KERNEL = vector::executeKernel;
        EXECUTE_KERNEL_RED = vector::executeKernelRed;
        IO_WRITE_NOW = io_unit::vwrite;
        IO_READ_NOW = io_unit::vread;
    }
    else if (RunningMode == REAL_HARDWARE_MODE)
    {
        vector::pipe_read_32 = open ("/dev/xillybus_read_array2arm_32",O_RDONLY);
        vector::pipe_write_32 = open ("/dev/xillybus_write_arm2array_32",O_WRONLY);
        EXECUTE_KERNEL = vector::executeKernel;
        EXECUTE_KERNEL_RED = vector::executeKernelRed;
        IO_WRITE_NOW = io_unit::vwrite;
        IO_READ_NOW = io_unit::vread;
    }
    else if (RunningMode == C_SIMULATION_MODE)
    {
        EXECUTE_KERNEL = c_simulator::executeDeasmKernel;
        EXECUTE_KERNEL_RED = c_simulator::executeDeasmKernel;
        IO_WRITE_NOW = c_simulator::vwrite;
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
        if (vector::pipe_read_32 == -1)
        {
            perror("Failed to open the read pipe");
            result = FAIL;
        }


        if (vector::pipe_write_32 == -1)
        {
            perror("Failed to open the write pipe");
            result = FAIL;
        }
    }
    vector::initialize();
    return result;
}

/*

    if (close(pipe_read_32)  == -1)
    {
        perror("Failed to open the read pipe");
        result = FAIL;
    }

    if (close(pipe_write_32) == -1)
    {
        perror("Failed to open the write pipe");
        result = FAIL;
    }

*/
