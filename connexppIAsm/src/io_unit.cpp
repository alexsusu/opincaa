#ifndef IO_SYSTEM_H
#define IO_SYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "../include/io_unit.h"

#define WRITE_MODE 1
#define ADDR_FIELD_MASK ((1 << 11) -1)
#define COUNT_FIELD_MASK (1 << 12) -1)

int io_unit::vpipe_write_32;
int io_unit::vpipe_read_32;

io_unit::io_unit()
{
    //ctor
}

io_unit::~io_unit()
{
    //dtor
}

void io_unit::setIOParams(int mode, int LsAddress, int NumOfVectors)
{
    Iou.Descriptor.Mode = mode;
    Iou.Descriptor.LsAddress = LsAddress;
    Iou.Descriptor.NumOfVectors = NumOfVectors;

    Size = NumOfVectors * 2;
    if (mode == WRITE_MODE) Size += DESCRIPTOR_SIZE;
}

void io_unit::prewriteVectors(UINT16 destAddress, UINT16 *srcAddress, UINT16 numVectors)
{
    setIOParams(WRITE_MODE, destAddress,numVectors);
    UINT32 *buff = Iou.Content;
    UINT32 *buffstop = buff + numVectors*VECTOR_SIZE_IN_DWORDS;
    while (buff < buffstop)
        *buff++ = *srcAddress++;
}

int io_unit::vwrite()
{
    if (Size == write(vpipe_write_32, &Iou,Size))
        return PASS;
    else return FAIL;
}

void io_unit::prepReadVectors(UINT16 srcAddress,UINT16 numVectors)
{
    setIOParams(READ_MODE, srcAddress,numVectors);
}

int io_unit::vread()
{
    if (DESCRIPTOR_SIZE == write(vpipe_write_32, &Iou.Descriptor, DESCRIPTOR_SIZE))
    {
        if (Size == read(vpipe_read_32, Iou.Content, Size))
            return PASS;
    }

    return FAIL;
}

int io_unit::initialize()
{
    int result = PASS;
    if ((vpipe_read_32 = open ("/dev/xillybus_read_mem2arm_32",O_RDONLY)) == -1)
    {
        perror("Failed to open the read pipe");
        result = FAIL;
    }


    if ((vpipe_write_32 = open ("/dev/xillybus_write_arm2mem_32",O_WRONLY)) == -1)
    {
        perror("Failed to open the write pipe");
        result = FAIL;
    }

    return result;
}

int io_unit::deinitialize()
{
    int result = PASS;
    if (close(vpipe_read_32)  == -1)
    {
        perror("Failed to open the read pipe");
        result = FAIL;
    }

    if (close(vpipe_write_32) == -1)
    {
        perror("Failed to open the write pipe");
        result = FAIL;
    }
    return result;
}


#endif // IO_SYSTEM_H

