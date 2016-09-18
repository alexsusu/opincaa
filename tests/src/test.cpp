#include <iostream>

#include "ConnexMachine.h"
#include "test.h"

#include "simple_tests.h"
#include "simple_io_tests.h"
#include "controller_tests.h"
#include "basic_match_tests.h"

using namespace std;

void initRand(int seed)
{
    srand (seed);
}

int32_t randPar(int32_t limit)
{
    int64_t result = limit;
    result = result * rand() / ( RAND_MAX + 1.0 );
    return (int32_t)result;
}

void eatRand(int times)
{
	int i;
	for (i=0; i< times; i++)
		printf(" Eating up %d-th rand number, %d\n",i,rand());

}

int RunAll(bool stress, ConnexMachine *connex)
{
    int result = FAIL;

    result = test_Simple_All(connex, stress);
    result += test_Simple_IO_All(connex, stress);
    jump_test(5,connex);

    return result;

}

int main(int argc, char *argv[]){

    if(argc < 6)
    {
        printf("Usage: %s insn red iowr iord regs\n",argv[0]);
        return 0;
    }

    try
    {
        ConnexMachine *connex = new ConnexMachine(argv[1],argv[2],argv[3],argv[4],argv[5]);
		
        RunAll(true,connex);

        delete connex;
    }

    catch(string err)
    {
        cout << err << endl;
    }

}

