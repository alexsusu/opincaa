#include <iostream>
#include <stdlib.h>
#include <iomanip>
#include <cstdint>

#include "ConnexMachine.h"
#include "Architecture.h"


using namespace std;
//none
static void Kernel0()
{
	BEGIN_KERNEL("Test_kernel_0");
		EXECUTE_IN_ALL(
			LS[0] = R1;	
			REDUCE(R0);
			)
	END_KERNEL("Test_kernel_0");
}

//arithmetic only
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

//logic only
static void Kernel2()
{
	BEGIN_KERNEL("Test_kernel_2");
		EXECUTE_IN_ALL(
			R0 = LS[0];
			R0 = R0 | 1;
			LS[0] = R0;
			REDUCE(R0);
			)
	END_KERNEL("Test_kernel_2");
}

//shift only
static void Kernel3()
{
	BEGIN_KERNEL("Test_kernel_3");
		EXECUTE_IN_ALL(
			R0 = LS[0];
			R0 = R0 << 1;
			LS[0] = R0;
			REDUCE(R0);
			)
	END_KERNEL("Test_kernel_3");
}

//multiply only
static void Kernel4()
{
	BEGIN_KERNEL("Test_kernel_4");
		EXECUTE_IN_ALL(
			R0 = LS[0];
			R0 * 2;
			R0 = MULT_LOW();
			LS[0] = R0;
			REDUCE(R0);
			)
	END_KERNEL("Test_kernel_4");
}

//compare only
static void Kernel5()
{
	BEGIN_KERNEL("Test_kernel_5");
		EXECUTE_IN_ALL(
			R0 = LS[0];
			R0 = R0 < 2;
			LS[0] = R0;
			REDUCE(R0);
		)
	END_KERNEL("Test_kernel_5");
}

//arith + logic
static void Kernel6()
{
	BEGIN_KERNEL("Test_kernel_6");
		EXECUTE_IN_ALL(
			R0 = LS[0];
			R1 = R0 + 2;
			R2 = R1 & 0XFFFF;
			LS[0] = R2;
			REDUCE(R2);
			)
	END_KERNEL("Test_kernel_6");
}

//arith + shift
static void Kernel7()
{
	BEGIN_KERNEL("Test_kernel_7");
		EXECUTE_IN_ALL(
			R0 = LS[0];
			R1 = R0 + 2;
			R2 = R1 << 2;
			LS[0] = R2;
			REDUCE(R2);
			)
	END_KERNEL("Test_kernel_7");
}

//arith + mult
static void Kernel8()
{
	BEGIN_KERNEL("Test_kernel_8");
		EXECUTE_IN_ALL(
			R0 = LS[0];
			R1 = R0 + 2;
			R1 * 3;
			R2 = MULT_LOW();
			LS[0] = R2;
			REDUCE(R2);
                        )
	END_KERNEL("Test_kernel_8");
}

//arith + compare
static void Kernel9()
{
	BEGIN_KERNEL("Test_kernel_9");
		EXECUTE_IN_ALL(
			R0 = LS[0];
             		R1 = R0 + 2;
            		R2 = R1 < 3;
             		LS[0] = R2;
             		REDUCE(R2);
			)
	END_KERNEL("Test_kernel_9");
}

//logic + shift
static void Kernel10()
{
	BEGIN_KERNEL("Test_kernel_10");
		EXECUTE_IN_ALL(
			R0 = LS[0];
             		R1 = R0 | 1;
            		R2 = R1 << 3;
             		LS[0] = R2;
             		REDUCE(R2);
			)
	END_KERNEL("Test_kernel_10");
}

//logic + mult
static void Kernel11()
{
	BEGIN_KERNEL("Test_kernel_11");
		EXECUTE_IN_ALL(
			R0 = LS[0];
			R1 = R0 | 1;
			R1 * 3;
			R2 = MULT_LOW();
			LS[0] = R2;
			REDUCE(R2);
			)
	END_KERNEL("Test_kernel_11");
}

//logic + compare
static void Kernel12()
{
	BEGIN_KERNEL("Test_kernel_12");
		EXECUTE_IN_ALL(
			R0 = LS[0];
             		R1 = R0 | 1;
            		R2 = R1 < 3;
             		LS[0] = R2;
             		REDUCE(R2); 
                        )
	END_KERNEL("Test_kernel_12");
}

//shift + mult
static void Kernel13()
{
	BEGIN_KERNEL("Test_kernel_13");
		EXECUTE_IN_ALL(
			R0 = LS[0];
            		R1 = R0 << 3;
			R1 * 3;
			R2 = MULT_LOW();
             		LS[0] = R2;
             		REDUCE(R2);
                        )
	END_KERNEL("Test_kernel_13");
}

//shift + compare
static void Kernel14()
{
	BEGIN_KERNEL("Test_kernel_14");
		EXECUTE_IN_ALL(
			R0 = LS[0];
            		R1 = R0 << 3;
			R2 = R1 < 3;
             		LS[0] = R2;
             		REDUCE(R2);
			)	
	END_KERNEL("Test_kernel_14");
}

//mult + compare
static void Kernel15()
{
	BEGIN_KERNEL("Test_kernel_15");
		EXECUTE_IN_ALL(
			R0 = LS[0];
            		R1 = R0 < 0;
                        R1 * 3;
			R2 = MULT_LOW();
             		LS[0] = R2;
             		REDUCE(R2);
			)
	END_KERNEL("Test_kernel_15");
}

//arith + logic + shift
static void Kernel16()
{
	BEGIN_KERNEL("Test_kernel_16");
		EXECUTE_IN_ALL(
			R0 = LS[0];
            		R1 = R0 + 2;
                        R2 = R1 & 1;
			R3 = R2 << 2;
             		LS[0] = R3;
             		REDUCE(R3);
	
		)
	END_KERNEL("Test_kernel_16");
}

//arith + logic + mult
static void Kernel17()
{
	BEGIN_KERNEL("Test_kernel_17");
		EXECUTE_IN_ALL(
			R0 = LS[0];
            		R1 = R0 + 2;
                        R2 = R1 & 1;
			R2 * 2;
			R3 = MULT_LOW();
             		LS[0] = R3;
             		REDUCE(R3);
			)
	END_KERNEL("Test_kernel_17");
}


//arith + logic + comp
static void Kernel18()
{
	BEGIN_KERNEL("Test_kernel_18");
		EXECUTE_IN_ALL(
			R0 = LS[0];
            		R1 = R0 + 2;
                        R2 = R1 & 1;
			R3 = R2 < 3;
             		LS[0] = R3;
             		REDUCE(R3);
			)
	END_KERNEL("Test_kernel_18");
}

//arith + shift + mult 
static void Kernel19()
{
	BEGIN_KERNEL("Test_kernel_19");
		EXECUTE_IN_ALL(
			R0 = LS[0];
            		R1 = R0 + 2;
                        R2 = R1 << 1;
			R2 * 2;
			R3 = MULT_LOW();
             		LS[0] = R3;
             		REDUCE(R3);	
			)
	END_KERNEL("Test_kernel_19");
}

//arith + shift + comp
static void Kernel20()
{
	BEGIN_KERNEL("Test_kernel_20");
		EXECUTE_IN_ALL(
			R0 = LS[0];
            		R1 = R0 + 2;
                        R2 = R1 << 1;
			R3 = R2 < 3;
             		LS[0] = R3;
             		REDUCE(R3);
			)
	END_KERNEL("Test_kernel_20");
}

//logic + shift + mult
static void Kernel21()
{
	BEGIN_KERNEL("Test_kernel_21");
		EXECUTE_IN_ALL(
			R0 = LS[0];
            		R1 = R0 | 2;
                        R2 = R1 << 1;
			R2 * 2;
			R3 = MULT_LOW();
             		LS[0] = R3;
             		REDUCE(R3);
			)
	END_KERNEL("Test_kernel_21");
}

//logic + shift + comp
static void Kernel22()
{
	BEGIN_KERNEL("Test_kernel_22");
		EXECUTE_IN_ALL(
			R0 = LS[0];
            		R1 = R0 | 2;
                        R2 = R1 << 1;
			R3 = R2 < 3;
             		LS[0] = R3;
             		REDUCE(R3);
			)
	END_KERNEL("Test_kernel_22");
}

//logic + mult + comp
static void Kernel23()
{
	BEGIN_KERNEL("Test_kernel_23");
		EXECUTE_IN_ALL(
			R0 = LS[0];
            		R1 = R0 | 2;
                        R2 = R1 < 10;
			R2 * 2;
			R3 = MULT_LOW();
             		LS[0] = R3;
             		REDUCE(R3);
			)
	END_KERNEL("Test_kernel_23");
}

//shift + mult + comp 
static void Kernel24()
{
	BEGIN_KERNEL("Test_kernel_24");
		EXECUTE_IN_ALL(
			R0 = LS[0];
            		R1 = R0 << 2;
                        R2 = R1 < 10;
			R2 * 2;
			R3 = MULT_LOW();
             		LS[0] = R3;
             		REDUCE(R3);
			)
	END_KERNEL("Test_kernel_24");
}

//arith + mult + comp
static void Kernel25()
{
	BEGIN_KERNEL("Test_kernel_25");
		EXECUTE_IN_ALL(
			R0 = LS[0];
            		R1 = R0 + 2;
                        R2 = R1 < 10;
			R2 * 2;
			R3 = MULT_LOW();
             		LS[0] = R3;
             		REDUCE(R3);
			)
	END_KERNEL("Test_kernel_25");
}

//arith + logic + shift + mult
static void Kernel26()
{
	BEGIN_KERNEL("Test_kernel_26");
		EXECUTE_IN_ALL(
			R0 = LS[0];
            		R1 = R0 + 2;
                        R2 = R1 & 1;
			R3 = R2 << 2;
			R3 * 2;
			R4 = MULT_LOW();
             		LS[0] = R4;
             		REDUCE(R4);
			)
	END_KERNEL("Test_kernel_26");
}

//arith + logic + shift + comp
static void Kernel27()
{
	BEGIN_KERNEL("Test_kernel_27");
		EXECUTE_IN_ALL(
			R0 = LS[0];
            		R1 = R0 + 2;
                        R2 = R1 & 1;
			R3 = R2 << 2;
			R4 = R3 < 10;
             		LS[0] = R4;
             		REDUCE(R4);
			)
	END_KERNEL("Test_kernel_27");
}

//arith + logic + mult + comp
static void Kernel28()
{
	BEGIN_KERNEL("Test_kernel_28");
		EXECUTE_IN_ALL(
			R0 = LS[0];
            		R1 = R0 + 2;
                        R2 = R1 | 1;
			R3 = R2 < 10;
			R3 * 2;
			R4 = MULT_LOW();
             		LS[0] = R4;
             		REDUCE(R4);
			)
	END_KERNEL("Test_kernel_28");
}

//arith + shift + mult + comp
static void Kernel29()
{
	BEGIN_KERNEL("Test_kernel_29");
		EXECUTE_IN_ALL(
			R0 = LS[0];
            		R1 = R0 + 2;
                        R2 = R1 << 1;
			R3 = R2 < 10;
			R3 * 2;
			R4 = MULT_LOW();
             		LS[0] = R4;
             		REDUCE(R4);
			)
	END_KERNEL("Test_kernel_29");
}

//logic + shift + mult + comp
static void Kernel30()
{
	BEGIN_KERNEL("Test_kernel_30");
		EXECUTE_IN_ALL(
			R0 = LS[0];
            		R1 = R0 | 2;
                        R2 = R1 << 1;
			R3 = R2 < 10;
			R3 * 2;
			R4 = MULT_LOW();
             		LS[0] = R4;
             		REDUCE(R4);
			)	
	END_KERNEL("Test_kernel_30");
}

//arith + logic + shift + mult + comp
static void Kernel31()
{
	BEGIN_KERNEL("Test_kernel_31");
		EXECUTE_IN_ALL(
			R0 = LS[0];
            		R1 = R0 + 2;
                        R2 = R1 << 1;
			R3 = R2 < 10;
			R4 = R4 | 0X00FF;
			R4 * 2;
			R5 = MULT_LOW();
             		LS[0] = R5;
             		REDUCE(R5);
			)
	END_KERNEL("Test_kernel_31");
}


void test_kernel(ConnexMachine *connex){
     	vector<int> h;
     	(*connex).setEnableMachineHistogram(true);
	cout<<ConnexMachine::dumpKernel("Test_kernel_0")<<endl<<endl;	
}

int main(){
	ConnexMachine *connex = new ConnexMachine("/home/vpopescu/Documents/git/opincaa/simulator/build/distributionFIFO",
                                                  "/home/vpopescu/Documents/git/opincaa/simulator/build/reductionFIFO",
                                                  "/home/vpopescu/Documents/git/opincaa/simulator/build/writeFIFO",
                                                  "/home/vpopescu/Documents/git/opincaa/simulator/build/readFIFO",
                                                  "regFile");


	try{
		Kernel0();
 		Kernel1();
		Kernel2();
		Kernel3();
		Kernel4();
		Kernel5();
		Kernel6();
		Kernel7();
		Kernel8();
		Kernel9();
		Kernel10();
		Kernel11();
		Kernel12();
		Kernel13();
		Kernel14();
		Kernel15();
		Kernel16();
		Kernel17();
		Kernel18();
		Kernel19();
		Kernel20();
		Kernel21();
		Kernel22();
		Kernel23();
		Kernel24();
		Kernel25();
		Kernel26();
		Kernel27();
		Kernel28();
		Kernel29();
		Kernel30();
		Kernel31();

		test_kernel(connex);
	}catch(std::string ex){
		cout << "Exception thrown: " << ex << endl;
	}
	return 0;
}

