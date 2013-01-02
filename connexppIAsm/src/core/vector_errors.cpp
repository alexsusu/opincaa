/*
 * File:   vector_errors.h
 *
 * Error methods implementation.
 *
 */

#include "../include/vector.h"

void vector::vectorError(const char *Msg)
{
    printf("Error: %s \n", Msg);
    dwErrorCounter++;
}

int vector::foundError()
{
    if (dwErrorCounter !=0) return -1;
        else return 0;
}

int vector::getNumErrors()
{
    return dwErrorCounter;
}
