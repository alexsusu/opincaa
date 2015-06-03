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

int RunAll(bool stress)
{
    int result = FAIL;
    try
    {
        ConnexMachine *connex = new ConnexMachine("distributionFIFO",
                                                "reductionFIFO",
                                                "writeFIFO",
                                                "readFIFO",
                                                "regFile");
		
        cout<<"Test simple all"<<endl;
	result = test_Simple_All(connex, false);
	cout<<"Test simple IO"<<endl;
        result += test_Simple_IO_All(connex, false);
        cout<<"terminate_Test simple IO"<<endl;
        jump_test(5,connex);
	//cout<<"Kernel_test"<<endl;
	

        delete connex;
    }

    catch(string err)
    {
        cout << err << endl;
    }

    return result;

}

int main(){
    RunAll(true);
}

