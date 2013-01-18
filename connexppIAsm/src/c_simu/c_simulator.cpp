/*
 * File:   vector.cpp
 *
 * OPINCAA's core implementation file.
 * Contains "vector" class where the usual operators (+, -, [], ^, &,* ....) are overloaded.
 *
 * Unlike usual systems where operators perform the math operation, this class does
 *  instruction assembly according to spec ConnexISA.docx
 *
 *
 *
 *
 */

using namespace std;
#include <iostream>
#include <assert.h>

#include "../../include/core/io_unit.h"
#include "../../include/c_simu/c_simulator.h"

#define ALLOW_DE_ASM_CPP
#include "de_asm.cpp"

int c_simulator::deinitialize()
{
    return PASS;
};

int c_simulator::initialize()
{
    int RegNr;
    int LsCnt;
    FOR_ALL_MACHINES(
                     for(RegNr = 0; RegNr < REGISTER_FILE_SIZE; RegNr++) CSimuRegs[MACHINE][RegNr] = 0;
                     for(LsCnt = 0; LsCnt < LOCAL_STORE_SIZE; LsCnt++) CSimuLocalStore[MACHINE][LsCnt] = 0;
                     CSimuActiveFlags[MACHINE] = _ACTIVE;
                     CSimuCarryFlags[MACHINE] = 0;
                     CSimuEqFlags[MACHINE] = 0;
                     CSimuLtFlags[MACHINE] = 0;
                     CSimuShiftRegs[MACHINE] = 0;
					 CSimuMultRegs[MACHINE] = 0;
					 CSimuRotationMagnitude[MACHINE] = 0;
                );
    return PASS;
}

void c_simulator::printSHIFT_REGS()
{
    FOR_ALL_MACHINES( cout << " Machine "<<MACHINE<<": " << " SHIFT_REG = "<<  CSimuShiftRegs[MACHINE] << endl; );
}

void c_simulator::printLS(int address)
{
    FOR_ALL_MACHINES( cout << " Machine "<<MACHINE<<": " << " LS["<<address<<"]= "<<  CSimuLocalStore[MACHINE][address] << endl; );
}


int c_simulator::vwrite(void* Iou)
{
    int result = vwriteNonBlocking(Iou);
    vwriteWaitEnd();
    return result;
}

int c_simulator::vwriteNonBlocking(void* Iou)
{
    UINT32 vectors;
    IO_UNIT_CORE* pIOUC = ((io_unit*)Iou)->getIO_UNIT_CORE();
    UINT32 LSaddress = (pIOUC->Descriptor).LsAddress;

    //cout << "Trying to write " << ((io_unit*)Iou)->getSize() << "bytes "<<endl;
    for (vectors = 0; vectors < pIOUC->Descriptor.NumOfVectors + 1; vectors++)
    {

        FOR_ALL_MACHINES(
                          if (MACHINE % 2 == 0)
                            CSimuLocalStore[MACHINE][LSaddress + vectors] = pIOUC->Content[vectors * VECTOR_SIZE_IN_DWORDS + MACHINE/2] & REGISTER_SIZE_MASK;
                          else
                            CSimuLocalStore[MACHINE][LSaddress + vectors] = pIOUC->Content[vectors * VECTOR_SIZE_IN_DWORDS + MACHINE/2] >> REGISTER_SIZE;
                        );
    }
    CSimulatorVWriteCounter++;
    return PASS;
}

int c_simulator::vwriteIsEnded()
{
    CSimulatorVWriteCounter--;//allow going under zero: simulate as if io_unit is read too many times
    assert(CSimulatorVWriteCounter >= 0);

    if (CSimulatorVWriteCounter == 0)
        return PASS;
    else return FAIL;
}

void c_simulator::vwriteWaitEnd()
{
    while (PASS != vwriteIsEnded())
    ;//wait
}

#define IO_MODE_POS             0
#define IO_LS_ADDRESS_POS       1
#define IO_NUM_OF_VECTORS_POS   2
#define IO_CONTENT_POS          3
/*
int c_simulator::vwrite(void* Iou)
{
    UINT32 vectors;
    UINT32* pIOUC = (UINT32*)(((io_unit*)Iou)->getIO_UNIT_CORE());

    UINT32 IoMode = *(pIOUC + IO_MODE_POS);
    UINT32 LSaddress = *(pIOUC + IO_LS_ADDRESS_POS);
    UINT32 NumOfVectors = *(pIOUC + IO_NUM_OF_VECTORS_POS);
    UINT32 *Content = pIOUC + IO_CONTENT_POS;

    for (vectors = 0; vectors < NumOfVectors; vectors++)
    {

        FOR_ALL_MACHINES(
                          if (MACHINE % 2 == 0)
                            CSimuLocalStore[MACHINE][LSaddress + vectors] = *((UINT32*)(Content + vectors * VECTOR_SIZE_IN_DWORDS + MACHINE/2)) & REGISTER_SIZE_MASK;
                          else
                            CSimuLocalStore[MACHINE][LSaddress + vectors] = *((UINT32*)(Content + vectors * VECTOR_SIZE_IN_DWORDS + MACHINE/2)) >> REGISTER_SIZE;
                        );
    }
    return PASS;
}
*/

int c_simulator::vread(void* Iou)
{
    UINT32 vectors;
    IO_UNIT_CORE* pIOUC = ((io_unit*)Iou)->getIO_UNIT_CORE();
    UINT32 LSaddress = (pIOUC->Descriptor).LsAddress;
    assert(CSimulatorVWriteCounter ==0);
    for (vectors = 0; vectors < pIOUC->Descriptor.NumOfVectors + 1; vectors++)
    {
        int MACHINE;
        for (MACHINE = 0; MACHINE < NUMBER_OF_MACHINES; MACHINE+=2)
            pIOUC->Content[vectors * VECTOR_SIZE_IN_DWORDS + MACHINE/2] =
                (CSimuLocalStore[MACHINE + 1][LSaddress + vectors] << REGISTER_SIZE) +
                                            CSimuLocalStore[MACHINE][LSaddress + vectors];

    }
    return PASS;
}

