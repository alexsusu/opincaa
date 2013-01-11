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

//#include "../include/vector.h"
//#include "../include/vector_errors.h"
//#include "../include/opcodes.h"
//#include <fcntl.h>
#include <iostream>
//#include <stdlib.h>
//#include <unistd.h>
#include "../../include/core/io_unit.h"

using namespace std;

#include "../../include/c_simu/c_simulator.h"

int c_simulator::deinitialize()
{
    return PASS;
};

int c_simulator::initialize()
{
    int RegNr;
    int LsCnt;
    FOR_ALL_MACHINES(
                     for(RegNr = 0; RegNr < 32; RegNr++) C_SIMU_REGS[MACHINE][RegNr] = 0;
                     for(LsCnt = 0; LsCnt < 1024; LsCnt++) C_SIMU_LS[MACHINE][LsCnt] = 0;
                     C_SIMU_ACTIVE[MACHINE] = _ACTIVE;
                     C_SIMU_CARRY[MACHINE] = 0;
                     C_SIMU_EQ[MACHINE] = 0;
                     C_SIMU_LT[MACHINE] = 0;
                     C_SIMU_SH[MACHINE] = 0;
                     C_SIMU_ROTATION_MAGNITUDE[MACHINE] = 0;// keeps the magnitude of the rotation
                     C_SIMU_MULTREGS[MACHINE] = 0;
                );
    return PASS;
}

void c_simulator::printSHIFT_REGS()
{
    FOR_ALL_MACHINES( cout << " Machine "<<MACHINE<<": " << " SHIFT_REG = "<<  C_SIMU_SH[MACHINE] << endl; );
}

void c_simulator::printLS(int address)
{
    FOR_ALL_MACHINES( cout << " Machine "<<MACHINE<<": " << " LS["<<address<<"]= "<<  C_SIMU_LS[MACHINE][address] << endl; );
}

int c_simulator::vwrite(void* Iou)
{
    UINT32 vectors;
    IO_UNIT_CORE* pIOUC = ((io_unit*)Iou)->getIO_UNIT_CORE();
    UINT32 LSaddress = (pIOUC->Descriptor).LsAddress;

    //cout << "Trying to write " << ((io_unit*)Iou)->getSize() << "bytes "<<endl;
    for (vectors = 0; vectors < pIOUC->Descriptor.NumOfVectors; vectors++)
    {

        FOR_ALL_MACHINES(
                          if (MACHINE % 2 == 0)
                            C_SIMU_LS[MACHINE][LSaddress + vectors] = pIOUC->Content[vectors * VECTOR_SIZE_IN_DWORDS + MACHINE/2] & REGISTER_SIZE_MASK;
                          else
                            C_SIMU_LS[MACHINE][LSaddress + vectors] = pIOUC->Content[vectors * VECTOR_SIZE_IN_DWORDS + MACHINE/2] >> REGISTER_SIZE;
                        );
    }
    return PASS;
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
                            C_SIMU_LS[MACHINE][LSaddress + vectors] = *((UINT32*)(Content + vectors * VECTOR_SIZE_IN_DWORDS + MACHINE/2)) & REGISTER_SIZE_MASK;
                          else
                            C_SIMU_LS[MACHINE][LSaddress + vectors] = *((UINT32*)(Content + vectors * VECTOR_SIZE_IN_DWORDS + MACHINE/2)) >> REGISTER_SIZE;
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
    for (vectors = 0; vectors < pIOUC->Descriptor.NumOfVectors; vectors++)
    {
        int MACHINE;
        for (MACHINE = 0; MACHINE < NUMBER_OF_MACHINES; MACHINE+=2)
            pIOUC->Content[vectors * VECTOR_SIZE_IN_DWORDS + MACHINE/2] = (C_SIMU_LS[MACHINE + 1][LSaddress + vectors] << REGISTER_SIZE) +
                                                                            C_SIMU_LS[MACHINE][LSaddress + vectors];

    }
    return PASS;
}

