/*
 * File:   kernel_acc.h
 *
 * Counterpart for kernel_acc.c
 * Contains class definition.
 *
 */

#ifndef KERNEL_ACC_H
#define KERNEL_ACC_H

//#include <stdlib.h>
//#include <stdio.h>
//#include "../util/utils.h"
//#include <stdlib.h>
//#include "cnxvector_errors.h"

class kernel_acc
{
    public:
        static int loadKernel(char *FileName, UINT32 KernelIndex);
        static int storeKernel(char *FileName, UINT32 KernelIndex);

        // methods: non-static
        kernel_acc();
        virtual ~kernel_acc();
};

#endif // KERNEL_ACC_H
