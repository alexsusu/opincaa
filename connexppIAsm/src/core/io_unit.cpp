#ifndef IO_SYSTEM_H
#define IO_SYSTEM_H

#include <stdio.h>
#include <stdlib.h>

#ifndef _MSC_VER //MS C++ compiler
	#include <unistd.h>
#else
	#include "../../ms_visual_c/fake_unistd.h"
#endif

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

    Size = NumOfVectors * VECTOR_SIZE_IN_BYTES;
    if (mode == WRITE_MODE) Size += DESCRIPTOR_SIZE_IN_BYTES;
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

void io_unit::preWriteVectorsAppend(UINT16 destAddress, UINT16 *srcAddress, UINT16 numVectors)
{
    setIOParams(WRITE_MODE, destAddress,numVectors);
    UINT32 *buff = Iouc.Content + (Iouc.Descriptor.NumOfVectors * VECTOR_SIZE_IN_DWORDS);
    UINT32 *dwsrcAddress = (UINT32*)srcAddress;
    UINT32 *buffstop = buff + numVectors*VECTOR_SIZE_IN_DWORDS;

    while (buff < buffstop)
        *buff++ = *dwsrcAddress++;

    Iouc.Descriptor.NumOfVectors += numVectors;
}


int io_unit::vwrite(void *_iou)
{
    int result;
    io_unit *iou = (io_unit*)_iou;
    //printf("Trying to write %ld bytes \n", (long int)iou->getSize());
    if (iou->getSize() == write(vpipe_write_32,iou->getIO_UNIT_CORE(),iou->getSize()))
        result = PASS;
     else
        result = FAIL;

	write(vpipe_write_32, NULL, 0); //flush
	return result;
}

void io_unit::preReadVectors(UINT16 srcAddress,UINT16 numVectors)
{
    setIOParams(READ_MODE, srcAddress,numVectors);
}

int io_unit::vread(void *_iou)
{
    io_unit *iou = (io_unit*)_iou;
    if (DESCRIPTOR_SIZE_IN_BYTES == write(vpipe_write_32, &(iou->getIO_UNIT_CORE())->Descriptor, DESCRIPTOR_SIZE_IN_BYTES))
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

