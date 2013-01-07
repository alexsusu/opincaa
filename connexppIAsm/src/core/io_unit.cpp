#ifndef IO_SYSTEM_H
#define IO_SYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "../../include/core/io_unit.h"

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
    Iouc.Descriptor.Mode = mode;
    Iouc.Descriptor.LsAddress = LsAddress;
    Iouc.Descriptor.NumOfVectors = NumOfVectors;

    Size = NumOfVectors * 2;
    if (mode == WRITE_MODE) Size += DESCRIPTOR_SIZE;
}

void io_unit::preWriteVectors(UINT16 destAddress, UINT16 *srcAddress, UINT16 numVectors)
{
    setIOParams(WRITE_MODE, destAddress,numVectors);
    UINT32 *buff = Iouc.Content;
    UINT32 *dwsrcAddress = (UINT32*)srcAddress;
    UINT32 *buffstop = buff + numVectors*VECTOR_SIZE_IN_DWORDS;
    while (buff < buffstop)
        *buff++ = *dwsrcAddress++;
}

int io_unit::vwrite(void *_iou)
{
    io_unit *iou = (io_unit*)_iou;
    if (iou->getSize() == write(vpipe_write_32,iou->getIO_UNIT_CORE(),iou->getSize()))
        return PASS;
    else return FAIL;
}

void io_unit::preReadVectors(UINT16 srcAddress,UINT16 numVectors)
{
    setIOParams(READ_MODE, srcAddress,numVectors);
}

int io_unit::vread(void *_iou)
{
    io_unit *iou = (io_unit*)_iou;
    if (DESCRIPTOR_SIZE == write(vpipe_write_32, &(iou->getIO_UNIT_CORE())->Descriptor, DESCRIPTOR_SIZE))
    {
        if (iou->getSize() == read(vpipe_read_32, &(iou->getIO_UNIT_CORE())->Content, iou->getSize()))
            return PASS;
    }

    return FAIL;
}

int io_unit::initialize()
{
    int result = PASS;
    return result;
}

int io_unit::deinitialize()
{
    int result = PASS;
    return result;
}

IO_UNIT_CORE* io_unit::getIO_UNIT_CORE()
{
    return &Iouc;
}

INT32 io_unit::getSize()
{
    return Size;
}

#endif // IO_SYSTEM_H

