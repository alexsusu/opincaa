#include <iostream>
#include "ConnexMachine.h"
#include "simple_tests.h"
#include "icc_simple_tests.h"
#include "simple_io_tests.h"
#include "controller_tests.h"
#include "basic_match_tests.h"

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


#ifdef _WIN32
    #include <direct.h>
    #define GetCurrentDir _getcwd
    #define SLASHES '\\'
#else
    #include <unistd.h>
    #define GetCurrentDir getcwd
    #define SLASHES '/'
#endif

char cCurrentPath[FILENAME_MAX];
char* GetDataFilePath(char *FileName)
{
    if (!GetCurrentDir(cCurrentPath, sizeof(cCurrentPath)))
        return "";

    cCurrentPath[sizeof(cCurrentPath) - 1] = '\0'; /* not really required */
    //printf ("The current working directory is %s", cCurrentPath);
    std::string path(cCurrentPath);
    path = path.substr(0, path.find_last_of(SLASHES));
    path = path + SLASHES + "test" + SLASHES + "data" + SLASHES;

    return (char*)((path + std::string(FileName)).c_str());
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

        result = test_Simple_All(connex, stress);
        result += test_Simple_IO_All(connex, stress);
        jump_test(5,connex);

        char *LogFileName = "RunScores.log";
        FILE *logfile = fopen(LogFileName, "w");
        char *fn1 ="data/adam1.key";
        char *fn2 ="data/adam2.key";

        //result += icc_test_Simple_All(connex, stress);
        result += test_BasicMatching_All_SSD(connex,
                                        GetDataFilePath(fn1),
                                        GetDataFilePath(fn2),
                                        logfile
                                         );

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

