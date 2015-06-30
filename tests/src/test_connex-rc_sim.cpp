#include <iostream>
#include <stdlib.h>
#include <iomanip>
#include <cstdint>
#include <stdio.h>
#include <string.h>
#include "ConnexMachine.h"
#include "Architecture.h"


using namespace std;
//test cell_shr
static void Kernel0()
{
    BEGIN_KERNEL("Test_kernel");
        EXECUTE_IN_ALL(
		R0 = LS[0];
                R1 = 3;
                CELL_SHR(R0, R1);
                R0 = SHIFT_REG;
                LS[0] = R0;	
                REDUCE(R0);
                      )
    END_KERNEL("Test_kernel");
}

//test repeat
static void Kernel1()
{
    BEGIN_KERNEL("Test_kernel_1");
        EXECUTE_IN_ALL(
                R0 = LS[0];
                REPEAT_X_TIMES(2);
			R0 = R0 + 1;
                END_REPEAT;
                LS[0] = R0;
                REDUCE(R0);
                      )
    END_KERNEL("Test_kernel_1");
}

//test carry
static void Kernel2()
{
    BEGIN_KERNEL("Test_kernel_2");
	EXECUTE_IN_ALL(
		R0 = 0XFFFF;
		R1 = 1;
		R0 = R0 + R1;
		R1 = ADDC(R1, 1);
		LS[0] = R1;
		REDUCE(R1);
                        )
    END_KERNEL("Test_kernel_2");
}

//test carry (borrow)
static void Kernel3()
{
    BEGIN_KERNEL("Test_kernel_3");
        EXECUTE_IN_ALL(
		R0 = 0;
		R1 = 2;
		R0 = R0 - R1;
		R1 = SUBC(R1, 1);
		LS[0] = R0;
		REDUCE(R1);
                        )
    END_KERNEL("Test_kernel_3");
}

//test lt
static void Kernel4()
{
    BEGIN_KERNEL("Test_kernel_4");
        EXECUTE_IN_ALL(
		R0 = 0;
		R1 = 1;
		R0 = R0 < R1;
		LS[0] = R0;
		REDUCE(R0);
                        )
    END_KERNEL("Test_kernel_4");
}

//test lt
static void Kernel5()
{
    BEGIN_KERNEL("Test_kernel_5");
        EXECUTE_IN_ALL(
		R0 = 1;
		R1 = 1;
		R0 = R0 < R1;
		LS[0] = R0;
		REDUCE(R0);
                        )
    END_KERNEL("Test_kernel_5");
}

//test ult instr, lt flag
static void Kernel6()
{
    BEGIN_KERNEL("Test_kernel_6");
        EXECUTE_IN_ALL(
		R0 = 6;
		R1 = 0;
		R1 = R1 - 1;
		R0 = ULT(R0, R1);
		LS[0] = R0;
		REDUCE(R0);
                        )
    END_KERNEL("Test_kernel_6");
}

//test ult instr, eg flag
static void Kernel7()
{
    BEGIN_KERNEL("Test_kernel_7");
	EXECUTE_IN_ALL(
		R0 = 0xffff;
		R1 = 0;
		R1 = R1 - 1;
		R0 = ULT(R0, R1);
		LS[0] = R1;
		REDUCE(R0);
                        )
    END_KERNEL("Test_kernel_7");
}

//test eq instr
static void Kernel8()
{
    BEGIN_KERNEL("Test_kernel_8");
        EXECUTE_IN_ALL(
		R0 = 0;
		R1 = 0;
		R0 = R0 == R1;
		LS[0] = R0;
		REDUCE(R0);
                        )
    END_KERNEL("Test_kernel_8");
}

//test vload instr, eq flag
static void Kernel9()
{
    BEGIN_KERNEL("Test_kernel_9");
        EXECUTE_IN_ALL(
		R0 = 0;
		REDUCE(R0);
                        )
    END_KERNEL("Test_kernel_9");
}

//test vload instr, lt flag
static void Kernel10()
{
    BEGIN_KERNEL("Test_kernel_10");
        EXECUTE_IN_ALL(
		R1 = 0;
		R0 = 0;
		R1 = R1 - 1;
		R0 = R1;
		LS[0] = R0;
		REDUCE(R0);
                        )
    END_KERNEL("Test_kernel_10");
}

//test where_cry functionality
static void Kernel11()
{
    BEGIN_KERNEL("Test_kernel_11");
        EXECUTE_IN_ALL(
		R0 = LS[0];
		R2 = 0;
		R0 = R0 + 2;
		EXECUTE_WHERE_CRY(
			R2 = R2 + 2;
                                 )
		LS[0] = R2;
		REDUCE(R0);
                        )
    END_KERNEL("Test_kernel_11");
}

//test where_lt functionality
static void Kernel12()
{
    BEGIN_KERNEL("Test_kernel_12");
        EXECUTE_IN_ALL(
		R0 = LS[0];
		R2 = 0;
		R0 = R0 < 1;
		EXECUTE_WHERE_LT(
			R2 = R2 + 2;
                                )
		LS[0] = R2;
		REDUCE(R0);
                        )
    END_KERNEL("Test_kernel_12");
}

//test where_eq functionality
static void Kernel13()
{
    BEGIN_KERNEL("Test_kernel_13");
        EXECUTE_IN_ALL(
		R0 = LS[0];
		R2 = 0;
		R0 = R0 == 0;
		EXECUTE_WHERE_EQ(
			R2 = R2 + 2;
                              )
		LS[0] = R2;
		REDUCE(R0);
                        )
    END_KERNEL("Test_kernel_13");
}

void test_kernel(ConnexMachine *connex){
	vector<int> h;
	short data[CONNEX_VECTOR_LENGTH];
	(*connex).setEnableMachineHistogram(true);
     	//cout<<connex->dumpKernel("Test_kernel")<<endl;
     	//cout<<"Disassamble version"<<endl; 
     	//cout<<connex->disassembleKernel("Test_kernel")<<endl;
     	//cout<<endl<<endl<<"Instructions counter"<<endl;
     
     	//h = (*connex).getConnexInstructionsCounter();
     	//for(int i=0; i<h.size(); i++){
	//cout<<h[i]<<endl;
     	//} 
     	//ConnexMachine::getKernelHistogram("Test_kernel");
     	//cout<<"enableMachineHistogram: "<<(*connex).getEnableMachineHistogram()<<endl;
     	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++){
     		if (i%2 == 0) data[i] = 0x0fff;
		else data[i]=0;
     	}  
     	connex->writeDataToArray(data, 1, 0); 
     	for(int i=0; i<CONNEX_VECTOR_LENGTH; i++){
     		data[i] = 0;
     	}
     	connex->executeKernel("Test_kernel_1");
     	int result = connex->readReduction();
     	cout << "Result is: " <<result << dec << endl;
     	connex->readDataFromArray(data, 1, 0);
     	/*for(int i=0; i<CONNEX_VECTOR_LENGTH; i++){
     	cout<<data[i]<<"  ";
     	}*/

	
}

int main(){
	ConnexMachine *connex = new ConnexMachine("/home/vpopescu/Documents/git/opincaa/simulator/build/distributionFIFO",
                                                  "/home/vpopescu/Documents/git/opincaa/simulator/build/reductionFIFO",
                                                  "/home/vpopescu/Documents/git/opincaa/simulator/build/writeFIFO",
                                                  "/home/vpopescu/Documents/git/opincaa/simulator/build/readFIFO",
                                                  "regFile");

	
	try{
		Kernel1();
		test_kernel(connex);
	}catch(std::string ex){
		cout << "Exception thrown: " << ex << endl;
	}
	return 0;
}

