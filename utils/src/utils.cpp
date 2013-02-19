
#include "utils.h"
#include "timing.h"
using namespace std;
#include <iostream>
#include <stdlib.h>

#ifdef __MINGW32__
#include <sstream>
#include <string>

std::string to_string(int val)
{
    std::stringstream ss;
    ss << val;
    std::string s(ss.str());
    return s;
}
#endif // __MINGW32__

void initRand()
{
    int seed =  GetMilliCount();
    srand ( seed );
    printf("\n Running with seed = %d\n",seed);
}
INT32 randPar(INT32 limit)
{
    INT64 result = limit;
    result = result * rand() / ( RAND_MAX + 1.0 );
    return (INT32)result;
}

void eatRand(int times)
{
	int i;
	for (i=0; i< times; i++)
		printf(" Eating up %d-th rand number, %d\n",i,rand());


}

//INT64 randPar(INT64 limit)
//{
//	static int randTimes = 0;
//	randTimes++;
//	//printf(" Getting rand number for the %d-th time \n", randTimes);
//	return (INT64)( limit * rand() / ( RAND_MAX + 1.0 ) );
//}

/**
    Computes sum of first "numbers" consecutive nubers starting with start.
    Numbers are forced to UINT16.
    Sum is forced to UINT16 + log2(NUMBER_OF_MACHINES)

    Eg.
    SumRedofFirstXnumbers(1, 145) = 145
    SumRedofFirstXnumbers(2, 14) = 14 + 15 = 29
    SumRedofFirstXnumbers(1, 5) = 1+2+3+4+5 = 15

*/
INT32 SumRedOfFirstXnumbers(UINT32 numbers, UINT32 start)
{
    UINT32 x;
    UINT32 sum = 0;
    for (x = start; x < start + numbers; x++ ) sum += x;
    return sum & REDUCTION_SIZE_MASK;
}
