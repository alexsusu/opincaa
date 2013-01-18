/*
 * File:   vector.h
 *
 * Counterpart for vector.c
 * Contains class definition and some very useful macros.
 *
 */

#ifndef C_SIMULATOR_H
#define C_SIMULATOR_H

#ifndef _MSC_VER //MS C++ compiler
	#include "../util/utils.h"
	#include "../core/io_unit.h"
#else
	#include "../util/utils.h"
	#include "../core/io_unit.h"
#endif

#define UINT_REGVALUE   UINT16
#define UINT_REGVALUE_TOP 0xffff
#define INT_REGVALUE    INT16
#define UINT_MULTVALUE  UINT32
#define C_SIMU_RED_MAX (8*1024* 1024) // can hold 8MiB reductions

#define LOCAL_STORE_SIZE 1024
#define REGISTER_FILE_SIZE 32

#define VERIFY_BATCH(Batch)         c_simulator::verifyBatch(Batch)
#define DEASM_BATCH(Batch)          c_simulator::printDeAsmBatch(Batch)
#define PRINT_SHIFT_REGS()           c_simulator::printSHIFT_REGS()
//#define FOUND_ERROR()                c_simulator::foundError()
//#define GET_NUM_ERRORS()             c_simulator::getNumErrors()

#define _ACTIVE 0
#define _INACTIVE 1

#ifndef _MSC_VER //MS C++ compiler
#define DECLARE_STATIC_C_SIMU_VARS\
		UINT_REGVALUE c_simulator::CSimuRegs[NUMBER_OF_MACHINES][32];\
        UINT_REGVALUE c_simulator::CSimuLocalStore[NUMBER_OF_MACHINES][1024];\
        UINT8 c_simulator::CSimuActiveFlags[NUMBER_OF_MACHINES];\
        UINT8 c_simulator::CSimuCarryFlags[NUMBER_OF_MACHINES];\
        UINT8 c_simulator::CSimuEqFlags[NUMBER_OF_MACHINES];\
        UINT8 c_simulator::CSimuLtFlags[NUMBER_OF_MACHINES];\
        UINT16 c_simulator::CSimuIndexRegs[NUMBER_OF_MACHINES];\
        UINT_REGVALUE c_simulator::CSimuShiftRegs[NUMBER_OF_MACHINES];\
        UINT_MULTVALUE c_simulator::CSimuMultRegs[NUMBER_OF_MACHINES];\
		UINT16 c_simulator::CSimuRotationMagnitude[NUMBER_OF_MACHINES];\
		UINT_RED_REG_VAL c_simulator::CSimuRed[C_SIMU_RED_MAX];\
        UINT32 c_simulator::CSimuRedCnt;


#else
#define DECLARE_STATIC_C_SIMU_VARS
#endif

#define FOR_ALL_ACTIVE_MACHINES(x) \
									int MACHINE;\
                                    for(MACHINE = 0; MACHINE < NUMBER_OF_MACHINES; MACHINE++)\
                                        if (CSimuActiveFlags[MACHINE]==0) {x;};
#define FOR_ALL_MACHINES(x) \
									int MACHINE;\
                                    for(MACHINE = 0; MACHINE < NUMBER_OF_MACHINES; MACHINE++)\
                                        {x;}

class c_simulator
{
    public:
        static int initialize();
        static int deinitialize();

        static int verifyBatch(UINT16 dwBatchNumber);
        static int verifyBatchInstruction(UINT_INSTRUCTION Instruction);

        static int printDeAsmBatch(UINT16 dwBatchNumber);

        static UINT_RED_REG_VAL executeBatchOneReduce(UINT16 dwBatchNumber);
        static UINT_RED_REG_VAL* executeBatchMultipleReduce(UINT16 dwBatchNumber);
        static UINT32 getMultiRedResult(UINT_RED_REG_VAL* RedResults, UINT32 Limit);

        static int DeAsmBatch(UINT16 dwBatchNumber);

        static void printSHIFT_REGS();
        static void printLS(int address);
        static int vwrite();
        static int vread();
        static int vwrite(void* Iou);
        static int vread(void* Iou);

        virtual ~c_simulator();
		static UINT_RED_REG_VAL CSimuRed[C_SIMU_RED_MAX];
		static UINT32 CSimuRedCnt;
    private:
        //vars
        static UINT_REGVALUE CSimuRegs[NUMBER_OF_MACHINES][REGISTER_FILE_SIZE];
        static UINT_REGVALUE CSimuLocalStore[NUMBER_OF_MACHINES][LOCAL_STORE_SIZE];
        static UINT8 CSimuActiveFlags[NUMBER_OF_MACHINES];
        static UINT8 CSimuCarryFlags[NUMBER_OF_MACHINES];
        static UINT8 CSimuEqFlags[NUMBER_OF_MACHINES];
        static UINT8 CSimuLtFlags[NUMBER_OF_MACHINES];
        static UINT16 CSimuIndexRegs[NUMBER_OF_MACHINES];
        static UINT_REGVALUE CSimuShiftRegs[NUMBER_OF_MACHINES];
		static UINT_MULTVALUE CSimuMultRegs[NUMBER_OF_MACHINES];
		static UINT16 CSimuRotationMagnitude[NUMBER_OF_MACHINES];

};

#endif // C_SIMULATOR_H
