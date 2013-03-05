#include <iostream>
#include "ConnexMachine.h"
#include "simple_tests.h"
#include "icc_simple_tests.h"
#include "simple_io_tests.h"
#include "controller_tests.h"

using namespace std;

void initKernelDemo(int val, int shift)
{
    BEGIN_KERNEL("test_"+ to_string(val)+"_"+ to_string(shift));
    EXECUTE_IN_ALL(
                    R0 = val;
                    R1 = R0 << shift;
                    REDUCE(R1);
                    )
    END_KERNEL("test_" + to_string(val)+"_" + to_string(shift));
}

int RunDemo()
{
    try
    {
        /* Pass the opened file descriptor as the distribution FIFO.
         * All kernels to be executed will be written to that file.
         */
        ConnexMachine *connex = new ConnexMachine("/dev/xillybus_write_arm2array_32", "/dev/xillybus_read_array2arm_32", "/dev/xillybus_write_mem2array_32", "/dev/xillybus_read_array2mem_32");
        cout << "Success in opening the connex machine" << endl;

        for(int i=0;i<7;i++)
            for(int j=0;j<7;j++)
            {
                initKernelDemo(i,j);
                connex->executeKernel("test_" + to_string(i)+"_" + to_string(j));
                //cout << "Expected " << 128*(i<<j) << " got " << connex->readReduction() << endl;
                if (connex->readReduction() != CONNEX_MAX_REGS*(i << j))
                    cout<<"Demo failed with params "<<i<<" "<<j<<endl;
            }

        delete connex;
    }

    catch(string err)
    {
        cout << err << endl;
    }

    return 0;

}

int RunAll(bool stress)
{
    int result = 1;//fail
    try
    {
        /* Pass the opened file descriptor as the distribution FIFO.
         * All kernels to be executed will be written to that file.
         */
        
	ConnexMachine *connex = new ConnexMachine("/dev/xillybus_write_arm2array_32",
                                              "/dev/xillybus_read_array2arm_32",
                                              "/dev/xillybus_write_mem2array_32",
                                              "/dev/xillybus_read_array2mem_32");
        /*

    ConnexMachine *connex = new ConnexMachine("distributionFIFO",
                                                "reductionFIFO",
                                                "writeFIFO",
                                                "readFIFO");
		*/

        //result = test_Simple_All(connex, stress);
        //result += test_Simple_IO_All(connex, stress);
        jump_test(5,connex);
        //result += icc_test_Simple_All(connex, stress);

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

