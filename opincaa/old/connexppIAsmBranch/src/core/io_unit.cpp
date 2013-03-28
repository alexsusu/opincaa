#ifndef IO_SYSTEM_H
#define IO_SYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void io_unit::setIOParams(int mode, int LsAddress, int NumOfCnxvectors)
{
    Iouc.Descriptor.Mode = mode;
    Iouc.Descriptor.LsAddress = LsAddress;
    Iouc.Descriptor.NumOfCnxvectors = NumOfCnxvectors - 1;

    Size = NumOfCnxvectors * CNXVECTOR_SIZE_IN_BYTES;
    if (mode == WRITE_MODE) Size += DESCRIPTOR_SIZE_IN_BYTES;
}

void io_unit::preWritecnxvectors(UINT16 destAddress, UINT16 *srcAddress, UINT16 numcnxvectors)
{
    setIOParams(WRITE_MODE, destAddress,numcnxvectors);
    memcpy(Iouc.Content, srcAddress, numcnxvectors*CNXVECTOR_SIZE_IN_BYTES);
}
/*
void io_unit::preWritecnxvectorsAppend(UINT16 destAddress, UINT16 *srcAddress, UINT16 numcnxvectors)
{
    setIOParams(WRITE_MODE, destAddress,numcnxvectors);
    UINT32 *buff = Iouc.Content + (Iouc.Descriptor.NumOfCnxvectors) * CNXVECTOR_SIZE_IN_DWORDS);
    UINT32 *dwsrcAddress = (UINT32*)srcAddress;
    UINT32 *buffstop = buff + numcnxvectors*CNXVECTOR_SIZE_IN_DWORDS;

    while (buff < buffstop)
        *buff++ = *dwsrcAddress++;

    Iouc.Descriptor.NumOfCnxvectors += numcnxvectors;
}
*/

/**
    Blocking write to IO.
    Writes descriptor and data to IO pipe, flushes the pipe and waits for recevive confirmation
*/
int io_unit::vwrite(void *_iou)
{
    int result = vwriteNonBlocking(_iou);
    vwriteWaitEnd();
    return result;
}

/**
    Non-blocking write to IO.
    Writes descriptor and data to IO pipe and flushes the pipe.
    It does NOT wait for recevive confirmation.
    To make sure the stream got to destination, call vwriteWaitEnd

    Keep in mind to read the confirmation
    (by calling once waitVwriteComplete()) before any vread call) !
*/
int io_unit::vwriteNonBlocking(void *_iou)
{
    int result;
    io_unit *iou = (io_unit*)_iou;
    //printf("Trying to write %ld bytes \n", (long int)iou->getSize());
    if (iou->getSize() == write(vpipe_write_32,iou->getIO_UNIT_CORE(),iou->getSize()))
        result = PASS;
     else
        result = FAIL;

    //printf("Write done \n");
	return result;
}

void io_unit::preReadcnxvectors(UINT16 srcAddress,UINT16 numcnxvectors)
{
    setIOParams(READ_MODE, srcAddress, numcnxvectors);
}

int io_unit::vread(void *_iou)
{
    io_unit *iou = (io_unit*)_iou;
    //printf("Writing vread descriptor \n");
    if (DESCRIPTOR_SIZE_IN_BYTES == write(vpipe_write_32, &(iou->getIO_UNIT_CORE())->Descriptor, DESCRIPTOR_SIZE_IN_BYTES))
    {
        write(vpipe_write_32, NULL, 0); //flush
        //printf("Waiting for %d bytes \n", iou->getSize());
        if (iou->getSize() == read(vpipe_read_32, &(iou->getIO_UNIT_CORE())->Content, iou->getSize()))
        {
            //printf("Waiting for is over \n");
            return PASS;
        }
    }

    return FAIL;
}

int io_unit::vwriteIsEnded()
{
    UINT32 dummy;
    if (BYTES_IN_DWORD == read(vpipe_read_32, &dummy, BYTES_IN_DWORD))
        return PASS;
    else return FAIL;
}

void io_unit::vwriteWaitEnd()
{
    write(vpipe_write_32, NULL, 0); //flush
    //printf("Waiting for vwrite-end \n");
    while (PASS != vwriteIsEnded())
    ;//wait
    //printf("Waiting is over \n");
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

