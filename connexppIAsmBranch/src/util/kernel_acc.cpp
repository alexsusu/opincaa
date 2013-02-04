/*
 * File:   kernel_acc.cpp
 *
 * OPINCAA's database implementation file.
 *
 *
 *
 *
 *
 */

#include "../../include/core/cnxvector.h"
#include "../../include/util/kernel_acc.h"
#include "../../include/util/utils.h"

//#include "../../include/core/cnxvector_errors.h"
//#include "../../include/c_simu/c_simulator.h"

#include <iostream>
#include <fstream>
#include <iomanip>
using namespace std;

//#include "../../include/core/opcodes.h"
#include <fcntl.h>
//#include <stdio.h>
//#include <stdlib.h>

#ifndef _MSC_VER //MS C++ compiler
	#include <unistd.h>
#else
	#include "../../ms_visual_c/fake_unistd.h"
#endif
/*
int kernel_acc::loadKernel(char *FileName, UINT32 KernelIndex)
{
    int read_32 = open(FileName, O_RDONLY | O_BINARY);
    if (read_32 != -1)
    {
        UINT_INSTRUCTION* Kernel = cnxvector::getBatch(KernelIndex);
        UINT32 KernelSize;// = cnxvector::getInBatchCounter(KernelIndex);

        if (sizeof(KernelSize) == read(read_32, &KernelSize, KernelSize))
        {
            cout<<"Alloc mem for kernel size of "<<KernelSize;
            if (Kernel != NULL) free(Kernel);
            Kernel = (UINT_INSTRUCTION*)malloc(sizeof(UINT_INSTRUCTION) * KernelSize);

            if (Kernel == NULL)
            {
                cnxvector::cnxvectorError(ERR_NOT_ENOUGH_MEMORY);
                return FAIL;
            }

            if (KernelSize * sizeof(UINT_INSTRUCTION) == read(read_32, Kernel, KernelSize * sizeof(UINT_INSTRUCTION)))
            {
                cnxvector::setInBatchCounter(KernelIndex, KernelSize);
                cnxvector::setBatch(KernelIndex, Kernel);

                close(read_32);
                return PASS;
            }
            return FAIL_ON_READ;
        }
        return FAIL_ON_READ;
    }
    return FAIL_ON_OPEN;
}

int kernel_acc::storeKernel(char *FileName, UINT32 KernelIndex)
{

    //open(FileName, O_CREAT);//create file if it dows not exist
    //int write_32 = open(FileName, O_WRONLY | O_TRUNC | O_BINARY);
    int write_32 = open(FileName, O_TRUNC|O_WRONLY );
    if (write_32 != -1)
    {
        UINT_INSTRUCTION* Kernel = cnxvector::getBatch(KernelIndex);
        UINT32 KernelSize = cnxvector::getInBatchCounter(KernelIndex);
        if (sizeof(UINT32) != write(write_32, &KernelSize, sizeof(UINT32))) return FAIL_ON_WRITE;
        if (KernelSize*sizeof(UINT_INSTRUCTION) ==
                write(write_32, Kernel, KernelSize * sizeof(UINT_INSTRUCTION)))
        {
            //write(write_32, NULL, 0); //flush
            close(write_32);
            return PASS;
        }
        return FAIL_ON_WRITE;
    }
    cout<<"errno = "<<errno<<endl;
    return FAIL_ON_OPEN;
}
*/

int kernel_acc::loadKernel(char *FileName, UINT32 KernelIndex)
{
    ifstream file(FileName, ios::in | ios::binary);
    if (file.is_open())
    {
        UINT_INSTRUCTION* Kernel = cnxvector::getBatch(KernelIndex);
        UINT32 KernelSize;// = cnxvector::getInBatchCounter(KernelIndex);

        file.read ((char*)&KernelSize, sizeof(UINT32));
            //cout<<"Alloc mem for kernel size of "<<KernelSize;
            if (Kernel != NULL) free(Kernel);
            Kernel = (UINT_INSTRUCTION*)malloc(sizeof(UINT_INSTRUCTION) * KernelSize);

            if (Kernel == NULL)
            {
                cnxvector::cnxvectorError(ERR_NOT_ENOUGH_MEMORY);
                return FAIL;
            }

            //if (KernelSize * sizeof(UINT_INSTRUCTION) == read(read_32, Kernel, KernelSize * sizeof(UINT_INSTRUCTION)))
            file.read((char*)Kernel, KernelSize * sizeof(UINT_INSTRUCTION));
            cnxvector::setInBatchCounter(KernelIndex, KernelSize);
            cnxvector::setBatch(KernelIndex, Kernel);
            file.close();
            return PASS;
    }
    return FAIL_ON_OPEN;
}

int kernel_acc::storeKernel(char *FileName, UINT32 KernelIndex)
{

    //open(FileName, O_CREAT);//create file if it dows not exist
    //int write_32 = open(FileName, O_WRONLY | O_TRUNC | O_BINARY);
    ofstream file(FileName, ios::out | ios::binary);
    if (file.is_open())
    {
        UINT_INSTRUCTION* Kernel = cnxvector::getBatch(KernelIndex);
        UINT32 KernelSize = cnxvector::getInBatchCounter(KernelIndex);

        file.write((char*)&KernelSize, sizeof(KernelSize));
        file.write((char*)Kernel, KernelSize * sizeof(UINT_INSTRUCTION));
        file.flush();
        file.close();
        return PASS;
    }
    return FAIL_ON_OPEN;
}
