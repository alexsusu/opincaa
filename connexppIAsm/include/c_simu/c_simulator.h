/*
 * File:   vector.h
 *
 * Counterpart for vector.c
 * Contains class definition and some very useful macros.
 *
 */

#ifndef C_SIMULATOR_H
#define C_SIMULATOR_H

//#include <stdlib.h>
//#include <stdio.h>
#include "../utils.h"
//#include <stdlib.h>
//#include "vector_errors.h"

#define UINT_REGVALUE   UINT16
#define UINT_REGVALUE_TOP 0xffff
#define INT_REGVALUE    INT16
#define UINT_MULTVALUE  UINT32

#define VERIFY_KERNEL(Batch)         c_simulator::verifyKernel(Batch)
#define DEASM_KERNEL(Batch)          c_simulator::printDeasmKernel(Batch)
#define PRINT_SHIFT_REGS()           c_simulator::printSHIFT_REGS()
//#define FOUND_ERROR()                c_simulator::foundError()
//#define GET_NUM_ERRORS()             c_simulator::getNumErrors()

#define _ACTIVE 0
#define _INACTIVE 1

#define DECLARE_STATIC_C_SIMU_VARS extern UINT_REGVALUE c_simulator::C_SIMU_REGS[NUMBER_OF_MACHINES][32];\
        extern UINT_REGVALUE c_simulator::C_SIMU_LS[NUMBER_OF_MACHINES][1024];\
        extern UINT8 c_simulator::C_SIMU_ACTIVE[NUMBER_OF_MACHINES];\
        extern UINT8 c_simulator::C_SIMU_CARRY[NUMBER_OF_MACHINES];\
        extern UINT8 c_simulator::C_SIMU_EQ[NUMBER_OF_MACHINES];\
        extern UINT8 c_simulator::C_SIMU_LT[NUMBER_OF_MACHINES];\
        extern UINT8 c_simulator::C_SIMU_INDEX[NUMBER_OF_MACHINES];\
        extern UINT_REGVALUE c_simulator::C_SIMU_SH[NUMBER_OF_MACHINES];\
        extern UINT_REGVALUE c_simulator::C_SIMU_ROTATION_MAGNITUDE[NUMBER_OF_MACHINES];\
        extern UINT_MULTVALUE c_simulator::C_SIMU_MULTREGS[NUMBER_OF_MACHINES];\

#define FOR_ALL_ACTIVE_MACHINES(x) int MACHINE;\
                                    for(MACHINE = 0; MACHINE < NUMBER_OF_MACHINES; MACHINE++)\
                                        if (C_SIMU_ACTIVE[MACHINE]==0) {x;};
#define FOR_ALL_MACHINES(x) int MACHINE;\
                                    for(MACHINE = 0; MACHINE < NUMBER_OF_MACHINES; MACHINE++)\
                                        {x;}

class c_simulator
{
    public:
        static int initialize();
        static int deinitialize();
        static int verifyKernel(UINT16 dwBatchNumber);
        static int verifyKernelInstruction(UINT_INSTRUCTION Instruction);
        static int printDeasmKernel(UINT16 dwBatchNumber);
        static int executeDeasmKernel(UINT16 dwBatchNumber);
        static void printSHIFT_REGS();

        virtual ~c_simulator();

    private:
        //vars
        static UINT_REGVALUE C_SIMU_REGS[NUMBER_OF_MACHINES][32];
        static UINT_REGVALUE C_SIMU_LS[NUMBER_OF_MACHINES][1024];
        static UINT8 C_SIMU_ACTIVE[NUMBER_OF_MACHINES];
        static UINT8 C_SIMU_CARRY[NUMBER_OF_MACHINES];
        static UINT8 C_SIMU_EQ[NUMBER_OF_MACHINES];
        static UINT8 C_SIMU_LT[NUMBER_OF_MACHINES];
        static UINT8 C_SIMU_INDEX[NUMBER_OF_MACHINES];
        static UINT_REGVALUE C_SIMU_SH[NUMBER_OF_MACHINES];
        static UINT_REGVALUE C_SIMU_ROTATION_MAGNITUDE[NUMBER_OF_MACHINES];
        static UINT_MULTVALUE C_SIMU_MULTREGS[NUMBER_OF_MACHINES];
};

#endif // C_SIMULATOR_H
