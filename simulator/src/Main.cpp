

#include "ConnexSimulator.h"
#include <string>
#include <iostream>
#include <system_error>

using namespace std;

int main()
{

	try
	{
        ConnexSimulator simulator("distributionFIFO", "reductionFIFO", "writeFIFO", "readFIFO", "regFile");
        simulator.waitFinish();
	}
	catch(string ex)
	{
		cout << "Exception occured: " << ex << endl;
	}
	catch(system_error syserr)
	{
		cout << "System error:: " << syserr.what() << endl;
	}
	return 0;
}
