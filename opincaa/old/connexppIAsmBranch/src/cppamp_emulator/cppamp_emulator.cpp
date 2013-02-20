/*
 *
 *
 *
 *
 */

using namespace std;
#include <iostream>
#include <assert.h>


#include "../../include/core/io_unit.h"
#include "../../include/core/cnxvector.h"
#include "../../include/core/opcodes.h"

#include "../../include/cppamp_emu/cppamp_emulator.h"

#ifdef _MSC_VER //MS C++ compiler
	UINT32 cppamp_emulator::EmuInstructions[MAX_INSTRUCTIONS];
	UINT32 cppamp_emulator::EmuRegs[NUMBER_OF_MACHINES*REGISTER_FILE_SIZE];
	UINT32 cppamp_emulator::EmuLocalStore[NUMBER_OF_MACHINES*LOCAL_STORE_SIZE];
	UINT32 cppamp_emulator::EmuActiveFlags[NUMBER_OF_MACHINES];
	UINT32 cppamp_emulator::EmuCarryFlags[NUMBER_OF_MACHINES];
	UINT32 cppamp_emulator::EmuEqFlags[NUMBER_OF_MACHINES];
	UINT32 cppamp_emulator::EmuLtFlags[NUMBER_OF_MACHINES];
	UINT32 cppamp_emulator::EmuIndexRegs[NUMBER_OF_MACHINES];
	UINT32 cppamp_emulator::EmuShiftRegs[NUMBER_OF_MACHINES];
	UINT32 cppamp_emulator::EmuMultRegs[NUMBER_OF_MACHINES];
	UINT32 cppamp_emulator::EmuRotationMagnitude[NUMBER_OF_MACHINES];
	UINT32 cppamp_emulator::EmuRed[C_SIMU_RED_MAX];
	UINT32 cppamp_emulator::EmuRedCnt;
	UINT32 cppamp_emulator::EmuVWriteCounter;

#endif

#ifdef _MSC_VER //MS C++ compiler
#include <amp.h>
using namespace concurrency;
int cppamp_emulator::deinitialize()
{
    return PASS;
};

int cppamp_emulator::initialize()
{
    int RegNr;
    int LsCnt;
    for (int MACHINE = 0; MACHINE < NUMBER_OF_MACHINES; MACHINE++)
	{
                     for(RegNr = 0; RegNr < REGISTER_FILE_SIZE; RegNr++) EmuRegs[MACHINE * REGISTER_FILE_SIZE + RegNr] = 0;
                     for(LsCnt = 0; LsCnt < LOCAL_STORE_SIZE; LsCnt++) EmuLocalStore[MACHINE*LOCAL_STORE_SIZE + LsCnt] = 0;
                     EmuActiveFlags[MACHINE] = _ACTIVE;
                     EmuCarryFlags[MACHINE] = 0;
                     EmuEqFlags[MACHINE] = 0;
                     EmuLtFlags[MACHINE] = 0;
                     EmuShiftRegs[MACHINE] = 0;
					 EmuMultRegs[MACHINE] = 0;
					 EmuRotationMagnitude[MACHINE] = 0;
	}
    return PASS;
}
/*
void cppamp_emulator::printSHIFT_REGS()
{
    FOR_ALL_MACHINES( cout << " Machine "<<MACHINE<<": " << " SHIFT_REG = "<<  CSimuShiftRegs[MACHINE] << endl; );
}

void cppamp_emulator::printLS(int address)
{
    FOR_ALL_MACHINES( cout << dec<<" Machine "<<MACHINE<<": " << " LS["<<address<<"]= "<<  CppAmpLocalStore[MACHINE][address]
                     <<hex <<"(" << CppAmpLocalStore[MACHINE][address] << ")" <<endl; );
}

void cppamp_emulator::printLS(int address, int machine)
{
    cout <<dec<< " Machine "<<machine<<": " << " LS["<<address<<"]= "<<  CppAmpLocalStore[machine][address]
                     <<hex <<"(" << CppAmpLocalStore[machine][address] << ")" <<endl;
}

void cppamp_emulator::printREG(int address)
{
    FOR_ALL_MACHINES( cout <<dec<< " Machine "<<MACHINE<<": " << " R["<<address<<"]= "<<CppAmpRegs[MACHINE][address]
                            <<hex<<" (0x"<<CppAmpRegs[MACHINE][address] <<")"<<endl; );
}

void cppamp_emulator::printREG(int address, int machine)
{
    cout <<dec<< " Machine "<<machine<<": " << " R["<<address<<"]= "<<CppAmpRegs[machine][address]
                            <<hex<<" (0x"<<CppAmpRegs[machine][address] <<")"<<endl;
}

void cppamp_emulator::printACTIVE()
{
    FOR_ALL_MACHINES( cout << " Machine "<<MACHINE<<": " << " A = "<< CSimuActiveFlags[MACHINE] << endl; );
}
*/

/* opcodes */
struct OpCodeDeAsm
{
    UINT_INSTRUCTION opcode;
    const char* opcodeName;
};

static OpCodeDeAsm OpCodeDeAsms[] =
{
    {_ADD,"+"},
    {_ADDC,"+ CARRY +"},
    {_SUB,"-"},
    {_SUBC,"- CARRY -"},
    {_NOT,"~"},
    {_OR,"|"},
    {_AND,"&"},
    {_XOR,"^"},
    {_EQ,"=="},
    {_LT,"<"},
    {_SHL,"<<"},
    {_SHR,">>"},
    {_SHRA,">>>"},
    {_ULT,"ULT"},
    {_ISHL,"<<"},
    {_ISHR,">>"},
    {_ISHRA,">>>"},
    {_LDIX,"LDIX"},
    {_LDSH,"LDSH"},
    {_CELL_SHL,"CELL_SHL"},
    {_CELL_SHR,"CELL_SHR"},
    {_READ,"READ"},
    {_WRITE,"WRITE"},
    {_MULT,"*"},
    {_MULT_LO,"_LO (MULT)"},
    {_MULT_HI,"_HI (MULT)"},
    {_VLOAD,"VLOAD"},
    {_IREAD,"IREAD"},
    {_IWRITE,"IWRITE"},
    {_WHERE_CRY,"SET_ACTIVE(WHERE_CARRY)"},
    {_WHERE_EQ,"SET_ACTIVE(WHERE_EQUAL)"},
    {_WHERE_LT,"SET_ACTIVE(WHERE_LESS_THAN)"},
    {_END_WHERE,"SET_ACTIVE(ALL)"},
    {_REDUCE,"reduce"},
    {_NOP,"nop"}
};

int cppamp_emulator::verifyBatchInstruction(UINT_INSTRUCTION CurrentInstruction)
{
    UINT16 index;
    if  ( (GET_OPCODE_6BITS(CurrentInstruction) == _VLOAD) ||
          (GET_OPCODE_6BITS(CurrentInstruction) == _IREAD) ||
          (GET_OPCODE_6BITS(CurrentInstruction) == _IWRITE)
         )
        return PASS;

    for (index = 0; index < sizeof(OpCodeDeAsms) / sizeof (OpCodeDeAsm); index++)
        if (GET_OPCODE_9BITS(CurrentInstruction) == OpCodeDeAsms[index].opcode) return PASS;

    perror (" Instruction's opcode is not recognized. This might be a sort of ... crappy code ;)");
    return FAIL;
}

int cppamp_emulator::verifyBatch(UINT16 dwBatchNumber)
{
    // verify opcodes
    UINT32 InstructionIndex = 0;
    UINT32 InstructionIndexMax = cnxvector::getInBatchCounter(dwBatchNumber);
    for (InstructionIndex = 0; InstructionIndex < InstructionIndexMax; InstructionIndex++)
                    if (FAIL == verifyBatchInstruction(cnxvector::getBatchInstruction(dwBatchNumber,InstructionIndex))) return FAIL;

    return PASS;
}

UINT32 cppamp_emulator::getMultiRedResult(UINT_RED_REG_VAL* RedResults, UINT32 Limit)
{
    if (Limit >= EmuRedCnt) Limit = EmuRedCnt;
    for(UINT32 i = 0; i < Limit; i++) RedResults[i] = EmuRed[i];

    //try to simulate same behaviour as in named pipes (see cnxvector::getMultiRedResult)
    // please note that this is not a FIFO like the cnxvector class has !
    EmuRedCnt = EmuRedCnt-Limit;
    return Limit*4;
}

UINT_RED_REG_VAL cppamp_emulator::executeBatchOneReduce(UINT16 dwBatchNumber)
{
    cppamp_emulator::DeAsmBatch(dwBatchNumber);
    return EmuRed[0]; // return first result
}

UINT_RED_REG_VAL* cppamp_emulator::executeBatchMultipleReduce(UINT16 dwBatchNumber)
{
    cppamp_emulator::DeAsmBatch(dwBatchNumber);
    return EmuRed;
}

int cppamp_emulator::DeAsmBatch(UINT16 dwBatchNumber)
{
    long timeStart, timeEnd;

	int result = 0;
    EmuRedCnt = 0;
	UINT32 InstructionIndexMax = cnxvector::getInBatchCounter(dwBatchNumber);

	timeStart = GetTickCount();
	//cnxvector::getInBatchCounter(dwBatchNumber)

	array_view<const UINT32, 1>		CppAmpMaxInstruction(1, &InstructionIndexMax);

	array_view<const UINT32, 1>	CppAmpInstructions(MAX_INSTRUCTIONS, cppamp_emulator::EmuInstructions);
	array_view<UINT32, 2>	CppAmpRegs(NUMBER_OF_MACHINES, 32, cppamp_emulator::EmuRegs);
	array_view<UINT32, 2>	CppAmpLocalStore(NUMBER_OF_MACHINES,1024, cppamp_emulator::EmuLocalStore);
	array_view<UINT32, 1>	CppAmpActiveFlags(NUMBER_OF_MACHINES, cppamp_emulator::EmuActiveFlags);
	array_view<UINT32, 1>	CppAmpCarryFlags(NUMBER_OF_MACHINES, cppamp_emulator::EmuCarryFlags);
	array_view<UINT32, 1>	CppAmpEqFlags(NUMBER_OF_MACHINES, cppamp_emulator::EmuEqFlags);
	array_view<UINT32, 1>	CppAmpLtFlags(NUMBER_OF_MACHINES, cppamp_emulator::EmuLtFlags);
	array_view<UINT32, 1>	CppAmpShiftRegs(NUMBER_OF_MACHINES, cppamp_emulator::EmuShiftRegs);
	array_view<UINT32, 1>	CppAmpMultRegs(NUMBER_OF_MACHINES, cppamp_emulator::EmuMultRegs);
	array_view<UINT32, 1>	CppAmpRed(C_SIMU_RED_MAX, cppamp_emulator::EmuRed);
	array_view<UINT32, 1>	CppAmpRedCnt(1, &cppamp_emulator::EmuRedCnt);


    CppAmpRegs.discard_data();
	CppAmpLocalStore.discard_data();
	CppAmpActiveFlags.discard_data();
	CppAmpCarryFlags.discard_data();
	CppAmpEqFlags.discard_data();
	CppAmpLtFlags.discard_data();
	CppAmpMultRegs.discard_data();
	CppAmpRed.discard_data();
	CppAmpShiftRegs.discard_data();
	//CppAmpRedCnt.discard_data();
	//CppAmpInstructions.discard_data();

	timeEnd = GetTickCount();
	cout << "Creating C++ AMP objects took "<<timeEnd - timeStart <<" ms"<<endl;

	timeStart = GetTickCount();

    parallel_for_each(
        // Define the compute domain, which is the set of threads that are created.
        CppAmpActiveFlags.extent,
        // Define the code to run on each thread on the accelerator.
        [=](index<1> idx) restrict(amp)
    {
		UINT32 CI; //current instruction
		UINT32 InstructionIndexMax = CppAmpMaxInstruction[0];
		UINT32 MACHINE = idx[0];


		for (UINT32 InstructionIndex = 0; InstructionIndex < InstructionIndexMax; InstructionIndex++)
		{
			CI = CppAmpInstructions[InstructionIndex];
			switch (((CI) >> OPCODE_6BITS_POS) & ((1 << OPCODE_6BITS_SIZE)-1))
				{
			case _VLOAD: { if (CppAmpActiveFlags(MACHINE)==0) {CppAmpRegs(MACHINE, GET_DEST(CI)) = GET_IMM(CI);};continue;}
			case _IREAD: {IF_MACHINE_IS_ACTIVE {CppAmpRegs(MACHINE,GET_DEST(CI)) = CppAmpLocalStore(MACHINE,GET_IMM(CI));};continue;}
			case _IWRITE: {IF_MACHINE_IS_ACTIVE{ CppAmpLocalStore(MACHINE,GET_IMM(CI)) = CppAmpRegs(MACHINE,GET_LEFT(CI));}continue;}
				}
			switch (((CI) >> OPCODE_9BITS_POS) & ((1 << OPCODE_9BITS_SIZE)-1))
				{
					case _ADD:{IF_MACHINE_IS_ACTIVE{
									CppAmpRegs(MACHINE,GET_DEST(CI)) = CppAmpRegs(MACHINE,GET_LEFT(CI)) + CppAmpRegs(MACHINE,GET_RIGHT(CI));
									if (CppAmpRegs(MACHINE,GET_LEFT(CI)) + CppAmpRegs(MACHINE,GET_RIGHT(CI)) > UINT_REGVALUE_TOP)
										 CppAmpCarryFlags(MACHINE) = 1;
									else CppAmpCarryFlags(MACHINE) = 0;
							  };
									continue;}

					case _ADDC:{IF_MACHINE_IS_ACTIVE
									CppAmpRegs(MACHINE,GET_DEST(CI)) = CppAmpRegs(MACHINE,GET_LEFT(CI)) +
																			CppAmpRegs(MACHINE,GET_RIGHT(CI)) +
																				CppAmpCarryFlags(MACHINE);
									if (CppAmpRegs(MACHINE,GET_LEFT(CI)) +
											CppAmpRegs(MACHINE,GET_RIGHT(CI)) +
												CppAmpCarryFlags(MACHINE) > UINT_REGVALUE_TOP)
										 CppAmpCarryFlags(MACHINE) = 1;
									else CppAmpCarryFlags(MACHINE) = 0;

													   continue;}

					case _SUB:{IF_MACHINE_IS_ACTIVE {CppAmpRegs(MACHINE,GET_DEST(CI)) =
														CppAmpRegs(MACHINE,GET_LEFT(CI)) - CppAmpRegs(MACHINE,GET_RIGHT(CI));

													if (CppAmpRegs(MACHINE,GET_LEFT(CI)) < CppAmpRegs(MACHINE,GET_RIGHT(CI)))
														 CppAmpCarryFlags(MACHINE) = 1;
													else CppAmpCarryFlags(MACHINE) = 0;

													};
														continue;}

					case _SUBC:{IF_MACHINE_IS_ACTIVE{CppAmpRegs(MACHINE,GET_DEST(CI)) =
														CppAmpRegs(MACHINE,GET_LEFT(CI)) - CppAmpRegs(MACHINE,GET_RIGHT(CI))
														- CppAmpCarryFlags(MACHINE);};
										   continue;
										if (CppAmpRegs(MACHINE,GET_LEFT(CI)) <
											CppAmpRegs(MACHINE,GET_RIGHT(CI)) + CppAmpCarryFlags(MACHINE))
											 CppAmpCarryFlags(MACHINE) = 1;
										else CppAmpCarryFlags(MACHINE) = 0;

							   }

					case _OR:{IF_MACHINE_IS_ACTIVE(CppAmpRegs(MACHINE,GET_DEST(CI)) =
														CppAmpRegs(MACHINE,GET_LEFT(CI)) | CppAmpRegs(MACHINE,GET_RIGHT(CI)));
													   continue;}

					case _AND:{IF_MACHINE_IS_ACTIVE(CppAmpRegs(MACHINE,GET_DEST(CI)) =
														CppAmpRegs(MACHINE,GET_LEFT(CI)) & CppAmpRegs(MACHINE,GET_RIGHT(CI)));
													   continue;}

					case _XOR:{IF_MACHINE_IS_ACTIVE(CppAmpRegs(MACHINE,GET_DEST(CI)) =
														CppAmpRegs(MACHINE,GET_LEFT(CI)) ^ CppAmpRegs(MACHINE,GET_RIGHT(CI)));
													   continue;}
					case _LT:
						{
							IF_MACHINE_IS_ACTIVE { if(CppAmpRegs(MACHINE,GET_LEFT(CI)) < CppAmpRegs(MACHINE,GET_RIGHT(CI)))
														{ CppAmpRegs(MACHINE,GET_DEST(CI)) = 1; CppAmpLtFlags(MACHINE) = 1; }
							else { CppAmpRegs(MACHINE,GET_DEST(CI)) = 0; CppAmpLtFlags(MACHINE) = 0; };  };
							continue;}
					case _ULT:
						{
							IF_MACHINE_IS_ACTIVE{ if(CppAmpRegs(MACHINE,GET_LEFT(CI)) < CppAmpRegs(MACHINE,GET_RIGHT(CI)))
														{ CppAmpRegs(MACHINE,GET_DEST(CI)) = 1; CppAmpLtFlags(MACHINE) = 1; }
													else { CppAmpRegs(MACHINE,GET_DEST(CI)) = 0; CppAmpLtFlags(MACHINE) = 0; };  };
							continue;}
					case _SHL:
						{
							IF_MACHINE_IS_ACTIVE {CppAmpRegs(MACHINE,GET_DEST(CI)) =
													CppAmpRegs(MACHINE,GET_LEFT(CI)) << CppAmpRegs(MACHINE,GET_RIGHT(CI));};
							continue;
						}
					case _SHR:
						{
							IF_MACHINE_IS_ACTIVE {CppAmpRegs(MACHINE,GET_DEST(CI)) =
													CppAmpRegs(MACHINE,GET_LEFT(CI)) >> CppAmpRegs(MACHINE,GET_RIGHT(CI));};
							continue;
						}
					case _SHRA:
						{
							//IF_MACHINE_IS_ACTIVE {CppAmpRegs(MACHINE,GET_DEST(CI)) =
								//				(INT_REGVALUE)(CppAmpRegs(MACHINE,GET_LEFT(CI))) >> (CppAmpRegs(MACHINE,GET_RIGHT(CI)));};
							continue;
						}

					case _EQ:
						{
							IF_MACHINE_IS_ACTIVE { if (CppAmpRegs(MACHINE,GET_LEFT(CI)) == CppAmpRegs(MACHINE,GET_RIGHT(CI)))
														{ CppAmpRegs(MACHINE,GET_DEST(CI)) = 1; CppAmpEqFlags(MACHINE) = 1; }
													else { CppAmpRegs(MACHINE,GET_DEST(CI)) = 0; CppAmpEqFlags(MACHINE) = 0; };
												}
							continue;}

					case _NOT: {IF_MACHINE_IS_ACTIVE(CppAmpRegs(MACHINE,GET_DEST(CI)) = ~CppAmpRegs(MACHINE,GET_LEFT(CI))); continue;}
					case _ISHL:
						{
							IF_MACHINE_IS_ACTIVE(CppAmpRegs(MACHINE,GET_DEST(CI)) =
														CppAmpRegs(MACHINE,GET_LEFT(CI)) << GET_RIGHT(CI));
							continue;
						}
					case _ISHR:
						{
							IF_MACHINE_IS_ACTIVE(CppAmpRegs(MACHINE,GET_DEST(CI)) =
														CppAmpRegs(MACHINE,GET_LEFT(CI)) >> GET_RIGHT(CI));
							continue;
						}
					case _ISHRA:
						{
							//IF_MACHINE_IS_ACTIVE(CppAmpRegs(MACHINE,GET_DEST(CI)) =
								//						((INT_REGVALUE)CppAmpRegs(MACHINE,GET_LEFT(CI))) >> GET_RIGHT(CI));
							continue;
						}
					case _LDIX:{IF_MACHINE_IS_ACTIVE(CppAmpRegs(MACHINE,GET_DEST(CI)) = MACHINE);continue;}
					case _LDSH:{IF_MACHINE_IS_ACTIVE(CppAmpRegs(MACHINE,GET_DEST(CI)) = CppAmpShiftRegs(MACHINE));continue;}

	//void printCELL_SHLR(UINT_INSTRUCTION instr){   printf("SHIFT_REG = R%d then %s by R%d positions \n", GET_LEFT(instr),getOpCodeName(instr),GET_RIGHT(instr));}
	// any contraints regarding ACTIVE ?
	// any constraints regarding
		/*
					case _CELL_SHL: {
										//step 1: load shift reg with R(LEFT).
										bool rotation_existed;
										{ CppAmpShiftRegs(MACHINE) = CppAmpRegs(MACHINE,GET_LEFT(CI));}

										//step 2: copy number of positions to rotate
										{ CSimuRotationMagnitude(MACHINE) = CppAmpRegs(MACHINE,GET_RIGHT(CI)) % NUMBER_OF_MACHINES;}

										//step 3: rotate one position at a time
										do
										{
											rotation_existed = false;

															if ( CSimuRotationMagnitude(MACHINE) > 0)
															{
																UINT32 man;
																rotation_existed = true;

																if (MACHINE == 0) man = CppAmpShiftRegs(NUMBER_OF_MACHINES -1);
																CppAmpShiftRegs((MACHINE + NUMBER_OF_MACHINES -1)% NUMBER_OF_MACHINES) = CSimuShiftRegs(MACHINE);
																if (MACHINE == NUMBER_OF_MACHINES - 1) CSimuShiftRegs(NUMBER_OF_MACHINES - 2) = man;

																CSimuRotationMagnitude(MACHINE)--;
															};
										} while (rotation_existed == true);
										continue;}

					case _CELL_SHR: {
										//step 1: load shift reg with R(LEFT).
										bool rotation_existed;
										{ CppAmpShiftRegs(MACHINE) = CppAmpRegs(MACHINE,GET_LEFT(CI));}

										//step 2: copy number of positions to rotate
										{ CSimuRotationMagnitude(MACHINE) = CppAmpRegs(MACHINE,GET_RIGHT(CI)) % NUMBER_OF_MACHINES;}

										//step 3: rotate one position at a time
										do
										{
											rotation_existed = false;
												if ( CSimuRotationMagnitude(MACHINE) > 0)
												{
													UINT32 man;
													rotation_existed = true;

													if (MACHINE == NUMBER_OF_MACHINES -2) man = CppAmpShiftRegs(NUMBER_OF_MACHINES-1);
													CppAmpShiftRegs((NUMBER_OF_MACHINES - MACHINE)% NUMBER_OF_MACHINES) = CSimuShiftRegs(NUMBER_OF_MACHINES - MACHINE -1);
													if (MACHINE == NUMBER_OF_MACHINES - 1) CSimuShiftRegs(0) = man;

													CSimuRotationMagnitude(MACHINE)--;
												}
										} while (rotation_existed == true);
										continue;}
				*/
					case _READ:  {IF_MACHINE_IS_ACTIVE {CppAmpRegs(MACHINE,GET_DEST(CI)) = CppAmpLocalStore(MACHINE,CppAmpRegs(MACHINE,GET_RIGHT(CI)));};continue;}
					case _WRITE: {IF_MACHINE_IS_ACTIVE {CppAmpLocalStore(MACHINE,CppAmpRegs(MACHINE,GET_RIGHT(CI))) = CppAmpRegs(MACHINE,GET_LEFT(CI));};continue;}

					case _MULT:  {IF_MACHINE_IS_ACTIVE {CppAmpMultRegs(MACHINE) = CppAmpRegs(MACHINE,GET_RIGHT(CI)) * CppAmpRegs(MACHINE,GET_LEFT(CI));};continue;}
					case _MULT_HI:{IF_MACHINE_IS_ACTIVE{CppAmpRegs(MACHINE,GET_DEST(CI)) = CppAmpMultRegs(MACHINE) >> 16;}; continue;}
					case _MULT_LO:{IF_MACHINE_IS_ACTIVE{CppAmpRegs(MACHINE,GET_DEST(CI)) = CppAmpMultRegs(MACHINE) & 0xFFFF;} continue;}

					case _WHERE_CRY:{if (CppAmpCarryFlags(MACHINE)==1) CppAmpActiveFlags(MACHINE)= _ACTIVE;else CppAmpActiveFlags(MACHINE)=_INACTIVE;continue;}
					case _WHERE_EQ:{if (CppAmpEqFlags(MACHINE)==1) CppAmpActiveFlags(MACHINE)= _ACTIVE;else CppAmpActiveFlags(MACHINE)=_INACTIVE;continue;}
					case _WHERE_LT:{if (CppAmpLtFlags(MACHINE)==1) CppAmpActiveFlags(MACHINE)= _ACTIVE;else CppAmpActiveFlags(MACHINE)=_INACTIVE;continue;}

					//case _WHERE_CRY:{IF_MACHINE_IS_ACTIVE(if (CppAmpCarryFlags(MACHINE)==1) CSimuActiveFlags(MACHINE)= _ACTIVE;else CSimuActiveFlags(MACHINE)=_INACTIVE);continue;}
					//case _WHERE_EQ:{IF_MACHINE_IS_ACTIVE(if (CSimuEqFlags(MACHINE)==1) CSimuActiveFlags(MACHINE)= _ACTIVE;else CSimuActiveFlags(MACHINE)=_INACTIVE);continue;}
					//case _WHERE_LT:{IF_MACHINE_IS_ACTIVE(if (CppAmpLtFlags(MACHINE)==1) CSimuActiveFlags(MACHINE)= _ACTIVE;else CSimuActiveFlags(MACHINE)=_INACTIVE);continue;}

					case _END_WHERE:{ CppAmpActiveFlags(MACHINE)= _ACTIVE; continue;}
					case _NOP:      continue;
					case _REDUCE:   continue;
									/*
									{UINT32 sum = 0; IF_MACHINE_IS_ACTIVE(sum += CppAmpRegs(MACHINE,GET_LEFT(CI)));
									if (CppAmpRedCnt[0] < C_SIMU_RED_MAX) CppAmpRed(CppAmpRedCnt[0]++) = sum & REDUCTION_SIZE_MASK; continue;}
									*/
					//default: result= FAIL; break;
				}//eo switch
		}//eo for

	}
    );
	timeEnd = GetTickCount();
	cout << "Running kernel took "<<timeEnd - timeStart <<" ms"<<endl;
    return result;
}



int cppamp_emulator::vwrite(void* Iou)
{
    int result = vwriteNonBlocking(Iou);
    vwriteWaitEnd();
    return result;
}

int cppamp_emulator::vwriteNonBlocking(void* Iou)
{
    UINT32 cnxvectors;
    IO_UNIT_CORE* pIOUC = ((io_unit*)Iou)->getIO_UNIT_CORE();
    UINT32 LSaddress = (pIOUC->Descriptor).LsAddress;

    //cout << "Trying to write " << ((io_unit*)Iou)->getSize() << "bytes "<<endl;
    /*
	for (cnxvectors = 0; cnxvectors < pIOUC->Descriptor.NumOfCnxvectors + 1; cnxvectors++)
    {

        FOR_ALL_MACHINES(
                          if (MACHINE % 2 == 0)
                            CppAmpLocalStore[MACHINE][LSaddress + cnxvectors] = pIOUC->Content[cnxvectors * CNXVECTOR_SIZE_IN_DWORDS + MACHINE/2] & REGISTER_SIZE_MASK;
                          else
                            CppAmpLocalStore[MACHINE][LSaddress + cnxvectors] = pIOUC->Content[cnxvectors * CNXVECTOR_SIZE_IN_DWORDS + MACHINE/2] >> REGISTER_SIZE;
                        );
    }
    EmuVWriteCounter++;
	*/
    return PASS;
}

int cppamp_emulator::vwriteIsEnded()
{
    EmuVWriteCounter--;//allow going under zero: simulate as if io_unit is read too many times
    assert(EmuVWriteCounter >= 0);

    if (EmuVWriteCounter == 0)
        return PASS;
    else return FAIL;
}

void cppamp_emulator::vwriteWaitEnd()
{
    while (PASS != vwriteIsEnded())
    ;//wait
}

#define IO_MODE_POS             0
#define IO_LS_ADDRESS_POS       1
#define IO_NUM_OF_cnxvectorS_POS   2
#define IO_CONTENT_POS          3
/*
int cppamp_emulator::vwrite(void* Iou)
{
    UINT32 cnxvectors;
    UINT32* pIOUC = (UINT32*)(((io_unit*)Iou)->getIO_UNIT_CORE());

    UINT32 IoMode = *(pIOUC + IO_MODE_POS);
    UINT32 LSaddress = *(pIOUC + IO_LS_ADDRESS_POS);
    UINT32 NumOfCnxvectors = *(pIOUC + IO_NUM_OF_cnxvectorS_POS);
    UINT32 *Content = pIOUC + IO_CONTENT_POS;

    for (cnxvectors = 0; cnxvectors < NumOfCnxvectors; cnxvectors++)
    {

        FOR_ALL_MACHINES(
                          if (MACHINE % 2 == 0)
                            CppAmpLocalStore[MACHINE][LSaddress + cnxvectors] = *((UINT32*)(Content + cnxvectors * CNXVECTOR_SIZE_IN_DWORDS + MACHINE/2)) & REGISTER_SIZE_MASK;
                          else
                            CppAmpLocalStore[MACHINE][LSaddress + cnxvectors] = *((UINT32*)(Content + cnxvectors * CNXVECTOR_SIZE_IN_DWORDS + MACHINE/2)) >> REGISTER_SIZE;
                        );
    }
    return PASS;
}
*/

int cppamp_emulator::vread(void* Iou)
{
    UINT32 cnxvectors;
    IO_UNIT_CORE* pIOUC = ((io_unit*)Iou)->getIO_UNIT_CORE();
    UINT32 LSaddress = (pIOUC->Descriptor).LsAddress;
    assert(EmuVWriteCounter ==0);
    for (cnxvectors = 0; cnxvectors < pIOUC->Descriptor.NumOfCnxvectors + 1; cnxvectors++)
    {
        int MACHINE;
		/*
        for (MACHINE = 0; MACHINE < NUMBER_OF_MACHINES; MACHINE+=2)
            pIOUC->Content[cnxvectors * CNXVECTOR_SIZE_IN_DWORDS + MACHINE/2] =
                (CppAmpLocalStore[MACHINE + 1][LSaddress + cnxvectors] << REGISTER_SIZE) +
                                            CppAmpLocalStore[MACHINE][LSaddress + cnxvectors];
		*/
    }
    return PASS;
}

#else

int cppamp_emulator::deinitialize(){    return FAIL;};
int cppamp_emulator::initialize(){    return FAIL;}
int cppamp_emulator::verifyBatchInstruction(UINT_INSTRUCTION CurrentInstruction) {return FAIL;}
int cppamp_emulator::verifyBatch(UINT16 dwBatchNumber)  {    return FAIL;}
UINT32 cppamp_emulator::getMultiRedResult(UINT_RED_REG_VAL* RedResults, UINT32 Limit) {return 0;}
UINT_RED_REG_VAL cppamp_emulator::executeBatchOneReduce(UINT16 dwBatchNumber){ return 0;}
UINT_RED_REG_VAL* cppamp_emulator::executeBatchMultipleReduce(UINT16 dwBatchNumber){    return NULL;}
int cppamp_emulator::DeAsmBatch(UINT16 dwBatchNumber) {    return FAIL;}
int cppamp_emulator::vwrite(void* Iou){    return FAIL;}
int cppamp_emulator::vwriteNonBlocking(void* Iou) {    return FAIL;}
int cppamp_emulator::vwriteIsEnded() {return FAIL;}
void cppamp_emulator::vwriteWaitEnd(){}

#define IO_MODE_POS             0
#define IO_LS_ADDRESS_POS       1
#define IO_NUM_OF_cnxvectorS_POS   2
#define IO_CONTENT_POS          3
int cppamp_emulator::vread(void* Iou){    return FAIL;}


#endif

