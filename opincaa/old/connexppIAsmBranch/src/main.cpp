/*
 * File:   main.cpp
 * Author: Calin (bcalin1984@yahoo.com)
 *
 * OPode INjection for Connex+Arm Architecture ;)
 * O.P.I.N.C.A.A
 * OPINCAA is an kind of asm for connex-arm system, but with c-like compiler interface.
 *
 * Kernel = just like in CUDA, kernel is the function that runs in parallel on connex cnxvector-machine.
 * Batch = a sequence of (logically) grouped instructions, that contain no more than one REDUCTION instruction
 *
 * Concept is:
 *   a) create batch file with instructions you want to crunch with connex at start-up time.
 *   b) launch the kernels on different sets of data got via IO system,
 *
 *
 * History:
 *
 *   v0.1 - proof of concept: two instructions and the two main functions:
 *   v0.2 - added more instructions and de-asm module for checkup
 *   v0.3 - "final draft" implementation: has all instructions; added error checks and functions
 *   v0.4 - added io_unit module (for transfers via IO). UNTESTED.
 *   v0.4.1 - Lucian: fixed subc test
 *   v0.4.2 - Radu & Lucian: fixed simpletests.
 *   v0.5   - Radu: added verilog-simulation mode
 *   v0.5.1 - code clean-up
 *   v0.5.2 - added "estimation mode" for BatchInit functions. See BEGIN_BATCH and END_BATCH.
 *   v0.6   - added c-simulator. Incomplete (CELL_SHL/SHR. Nothing with IO yet)
 *   v0.6.1  - added c-simulator CELL_SHL and SHR. Still nothing with IO yet.
 *   v0.6.2  - added c-simulator IO capabilities. Seems to work for write.
 *   v0.6.3  - Lucian: added more simple tests
 *   v0.6.4  - added IO tests.
 *   v0.6.5  - changed macros for *where*. Now they are EXECUTE_IN_ALL(), EXECUTE_WHERE_*()
 *   v0.6.6  - added utils.cpp support for IO under vsimulation
 *			 - bugfix: reduction is on log2(NUMBER_OF_MACHINES) + REGISTER_SIZE ( 23 bits for 128 UINT16 machines )
 *   v0.6.7  - added some speed tests. Tested on csimulator.
 *   v0.6.8  - speed tests were downsized to run in a reasonable time on arm
 *   v0.7.0  - added 12+5 pseudo-instructions and simple tests for them
 *               Rx = Ry * Rz is done as MULT = Ry * Rz; Rx = _LO(MULT);
 *               Rx = Ry OP number, where OP is +,ADDC,-,SUBC,|,&,^,==,<,ULT,* is done as Rx = number; Rx = Ry OP Rx
 *               Rx OP= Ry,      Rx OP= Ry, where OP is +,-,|,&,^ is done as Rx = Rx OP Ry;
 *
 *               Note that Rx OP= number   is NOT supported (by hw)
 *               Note that Rx = number OP Ry is NOT supported for now (compiler error)
 *   v0.7.1  - bugfix in IO read function. Refactored tests.
 *   TODO: add parameters in kernel-init functions.
 *	 v0.7.2  - bugfix IO write test. Added random tests for IO.
 *   v0.8.0  - added algo for basic matching.
 * 			 - added support for running batch with multiple reduction ops.
 *			 - renamed KERNEL macros to BATCH (eg. EXECUTE_BATCH)
 *			 - moved simpleClearLS and simplePrintLS to utils
 *           - renamed _LO and _HI to _LOW / _HIGH (as in _LO(MULT)): there was a collision with MS VC++ compiler
 *           - changed names of some internal simulator variables (coding guideline) - not finished
 *           - Rx register (x= 0... 31) can also be addressed via R[x]
 *
 *  v0.8.1   - updated io_unit and simulator to comply with new ConnexIOSpec
 *             (confirmation of write, size of written/read cnxvectors is (cnxvector Count + 1)
 *              NB: The io_unit modifications occured in setIOParams().  Calls to vwrite and vread remain backward compatible !
 *           - added new functions in io_unit/c_simulator
 *                          ::vwriteNonBlocking(void* Iou), ::vwriteIsEnded(), ::vwriteWaitEnd()
 *              and matching macros: IO_WRITE_BEGIN(); IO_WRITE_IS_ENDED(); IO_WRITE_WAIT_END();
 *  v0.8.2   - bugfix? randPar() modified: x86 was compiling and running ok, arm was not.
 *  v0.8.3   - bugfix: pseudo-instructions can now use immediate value of 0.
 *           - feature extension: pMov pseudo instruction ( R1 = R0 gets automatically translated to R1 = R0 >> 0 )
 *			 - added new construction with only one param, the MainVal (therefore, Rx were redefined)
 *			 - added simpleTests to check for bugfix and new feature correct operation
 *
 *  v0.8.4
 *			 - bugfix in c_simulator::DeAsmBatch at _CELL_SHL.
 *			 - added more functions: c_simulator::printREG, printACTIVE()
 *			 - added macro: ENABLE_ALL
 *		     - added simpleTest to check for rotation or shift, max test to find maximum UINT16 in a vector.
 *
 * Created on December 19, 2012, 3:32 PM
 *
 */

#include <iostream>
#include <limits>
#include <string.h>

#include "../include/core/cnxvector_registers.h"

#include "../include/util/utils.h"
#include "../include/util/timing.h"

#include "../include/test/simple_tests.h"
#include "../include/test/icc_simple_tests.h"
#include "../include/test/speed_tests.h"
#include "../include/test/simple_io_tests.h"
#include "../include/test/basic_match_tests.h"
#include "../include/test/basic_match_tests_sad_connex.h"
#include "../include/test/basic_match_tests_neon.h"
#include "../include/test/max_tests.h"
#include "../include/test/crypto/aes/aes_tests.h"
#include "../include/test/crypto/aes/aes_cpu_tests.h"
//#include "../include/test/crypto/aes/aes_cpu_neon_sse_tests.h"

//#include "../include/test/crypto/bsDES/bs_des_tests.h"

using namespace std;

int main(int argc, char *argv[])
{
    int i = 1;
    int run_mode = INVALID_MODE;
    //look for running option in arguments
    if (argc >= i+1)
    {
        if(strcmp(argv[i],"--vsimulation") == 0)
        {
            cout << "Running in verilog simulation mode" << endl;
            run_mode = VERILOG_SIMULATION_MODE;
        }
        else if(strcmp(argv[i],"--csimulation") == 0)
        {
            cout << "Running in c-simulation mode" << endl;
            run_mode = C_SIMULATION_MODE;
        }
        else if(strcmp(argv[i],"--hemulation") == 0)
		{
		    cout << "Running in cppamp hardware emulation mode" << endl;
			run_mode = CPPAMP_EMULATION_MODE;
		}
		else if(strcmp(argv[i],"--hardware") == 0)
        {
            cout << "Running in hardware mode" << endl;
            run_mode = REAL_HARDWARE_MODE;
        }
        else
        {
            INIT(C_SIMULATION_MODE);
            //bsDesTest();
            DEINIT();
            return 0;
        }
    }

     if (run_mode == INVALID_MODE)
        {
             cout<< "ERROR: No running mode selected."<<endl;
             cout<<"    Choose one of the following: "<<endl;
             cout<<"        --vsimulation (for Verilog Simulation)" << endl;
             cout<<"        --csimulation (for C++ Simulation)" << endl;
             cout<<"        --hemulation (for Hardware emulation)" << endl;
			 cout<<"        --hardware (for Hardware )" << endl;
             return -1;
        }

//    CountMilliTime();
    INIT(run_mode);
	if (argc > i + 1) // check for super stress
	if (0 == strcmp(argv[i+1],"--superstress"))
	{
	 cout<<"Starting superstress"<<endl;
	 while (1)
		{
			initRand();
			//    test_Simple_All(true);
			//    test_Speed_All();
            test_Simple_IO_All(true);
		}
	}
	//else:
	initRand();

	if (argc > i + 1) // check for super stress
    {
        if (0 == strcmp(argv[i+1],"--icctests"))
        {
            //icc_test_Simple_All(true);
            DEINIT();

            cout << "Press ENTER to continue...";
            cin.ignore( numeric_limits <streamsize> ::max(), '\n' );
        }
    }

    test_Simple_All(false);
    //test_ExtendedSimpleAll();
	//test_Max_All(true);
    //test_Speed_All();
    //test_Simple_IO_All(true);
	//test_BasicMatching_SAD_All();
    test_BasicMatching_All();
    test_BasicMatching_All_SAD();
    //MainNeonSSE();
    //test_BasicMatching_All_NeonSSE("data/adam2_big.png.key", "data/adam1_big.png.key");
    //test_BasicMatching_All_NeonSSE("data/adam1.key", "data/adam2.key");
    //test_BasicMatching_All_NeonSSE((char*)"data/img1.png.key", (char*)"data/img3.png.key");
    //test_BasicMatching_All_NeonSSE("data/img1_siftpp.key", "data/img3_siftpp.key");
    //test_BasicMatching_All_NeonSSE("data/img1.key", "data/img3.key");
	//test_AES_All();
	//test_AES_CPU_All();
	//test_AES_CPU_NEON_SSE_All();
    //MainNeonSSE();
    DEINIT();

    cout << "Press ENTER to continue...";
    cin.ignore( numeric_limits <streamsize> ::max(), '\n' );

    return 0;
}

