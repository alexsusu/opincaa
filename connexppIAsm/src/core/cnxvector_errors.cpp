/*
 * File:   cnxvector_errors.h
 *
 * Error methods implementation.
 *
 */

#include "../../include/core/cnxvector.h"

void cnxvector::cnxvectorError(const char *Msg)
{
    printf("Error: %s \n", Msg);
    dwErrorCounter++;
}

int cnxvector::foundError()
{
    if (dwErrorCounter !=0) return -1;
        else return 0;
}

int cnxvector::getNumErrors()
{
    return dwErrorCounter;
}
