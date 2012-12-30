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
//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h>

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
                     C_SIMU_MULTREGS[MACHINE] = 0;
                );
    return PASS;
}
