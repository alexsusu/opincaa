/*
 * File:   cppamp_emulator.h
 *
 * Counterpart for cppamp_emulator.cpp
 * Contains class definition and some very useful macros.
 */

#ifndef CPPAMP_EMULATOR_H
#define CPPAMP_EMULATOR_H

#ifndef _MSC_VER //MS C++ compiler
	#include "../util/utils.h"
	#include "../core/io_unit.h"
#else
	#include "../util/utils.h"
	#include "../core/io_unit.h"
#endif

#define MAX_INSTRUCTIONS 1024*500
#define UINT_REGVALUE   UINT16
#define UINT_REGVALUE_TOP 0xffff
#define INT_REGVALUE    INT16
#define UINT_MULTVALUE  UINT32
#define C_SIMU_RED_MAX (8*1024* 1024) // can hold 8MiB reductions

#define LOCAL_STORE_SIZE 1024
#define REGISTER_FILE_SIZE 32

//#define VERIFY_BATCH(Batch)         cppamp_emulator::verifyBatch(Batch)
//#define DEASM_BATCH(Batch)          cppamp_emulator::printDeAsmBatch(Batch)
//#define PRINT_SHIFT_REGS()           cppamp_emulator::printSHIFT_REGS()
//#define FOUND_ERROR()                cppamp_emulator::foundError()
//#define GET_NUM_ERRORS()             cppamp_emulator::getNumErrors()

#define _ACTIVE 0
#define _INACTIVE 1

#ifndef _MSC_VER //MS C++ compiler
#define DECLARE_STATIC_C_EMU_VARS\
		UINT_INSTRUCTION cppamp_emulator::EmuInstructions[MAX_INSTRUCTIONS];\
		UINT_REGVALUE cppamp_emulator::EmuRegs[NUMBER_OF_MACHINES*REGISTER_FILE_SIZE];\
        UINT_REGVALUE cppamp_emulator::EmuLocalStore[NUMBER_OF_MACHINES*LOCAL_STORE_SIZE];\
        UINT8 cppamp_emulator::EmuActiveFlags[NUMBER_OF_MACHINES];\
        UINT8 cppamp_emulator::EmuCarryFlags[NUMBER_OF_MACHINES];\
        UINT8 cppamp_emulator::EmuEqFlags[NUMBER_OF_MACHINES];\
        UINT8 cppamp_emulator::EmuLtFlags[NUMBER_OF_MACHINES];\
        UINT16 cppamp_emulator::EmuIndexRegs[NUMBER_OF_MACHINES];\
        UINT_REGVALUE cppamp_emulator::EmuShiftRegs[NUMBER_OF_MACHINES];\
        UINT_MULTVALUE cppamp_emulator::EmuMultRegs[NUMBER_OF_MACHINES];\
		UINT16 cppamp_emulator::EmuRotationMagnitude[NUMBER_OF_MACHINES];\
		UINT_RED_REG_VAL cppamp_emulator::EmuRed[C_SIMU_RED_MAX];\
        UINT32 cppamp_emulator::EmuRedCnt;\
        INT32 cppamp_emulator::EmuVWriteCounter;


#else
#define DECLARE_STATIC_C_EMU_VARS
#endif

#define IF_MACHINE_IS_ACTIVE if (CppAmpActiveFlags[MACHINE]==0)

class cppamp_emulator
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
        static void printLS(int address, int machine);
        static void printREG(int address);
        static void printREG(int address, int machine);
        static void printACTIVE();

        static int vread(void* Iou);
        static int vwrite(void* Iou);
        static int vwriteNonBlocking(void* Iou);
        static void vwriteWaitEnd();
        static int vwriteIsEnded();

        virtual ~cppamp_emulator();
		static UINT32 EmuRed[C_SIMU_RED_MAX];
		static UINT32 EmuRedCnt;


    private:
        //vars
		static UINT32 EmuInstructions[MAX_INSTRUCTIONS];
        static UINT32 EmuLocalStore[NUMBER_OF_MACHINES*LOCAL_STORE_SIZE];
        static UINT32 EmuRegs[NUMBER_OF_MACHINES*REGISTER_FILE_SIZE];
        static UINT32 EmuActiveFlags[NUMBER_OF_MACHINES];
        static UINT32 EmuCarryFlags[NUMBER_OF_MACHINES];
        static UINT32 EmuEqFlags[NUMBER_OF_MACHINES];
        static UINT32 EmuLtFlags[NUMBER_OF_MACHINES];
        static UINT32 EmuIndexRegs[NUMBER_OF_MACHINES];
        static UINT32 EmuShiftRegs[NUMBER_OF_MACHINES];
		static UINT32 EmuMultRegs[NUMBER_OF_MACHINES];
		static UINT32 EmuRotationMagnitude[NUMBER_OF_MACHINES];
		static UINT32 EmuVWriteCounter;

};

#endif // CPPAMP_EMULATOR_H
