#include <iostream>
#include <stdlib.h>
#include <iomanip>
#include <cstdint>

#include "ConnexMachine.h"
#include "Architecture.h"


using namespace std;

static void Kernel1()
{
    BEGIN_KERNEL("Test_kernel");
        EXECUTE_IN_ALL(//endwhere
                //logic
		R0 = R1 ^ R2;
                R0 = R1 ^ 2;
                R0 ^= R1;
                R0 ^= 2;
                R0 = R1 & R2;
                R0 = R1 & 2;
                R0 &= R1;
                R0 &= 2;
                R0 = R1 | R2;
                R0 = R1 | 2;
                R0 |= R1;
                R0 |= 2;
		R0 = ~R1;
                //arithmetic
		R0 = R1 + R2;
		R0 = R1 + 3;
		R0 += 3;
		R0 += R1;
                R0 = R1 - R2;
                R0 = R1 - 3;
                R0 -= 3;
		R0 -= R1;
		R0 = ADDC(R0,R1);
                R0 = ADDC(R0,3);
		R0 = SUBC(R0,R1);
		R0 = SUBC(R0,3);
		//multiplication
                R0*R1;
                R0*6;
		R0 = MULT_LOW();
                R1 = MULT_HIGH();
                //shift unit
                R0 = SHRA(R2, R1);
                R0 = SHRA(R2, 7);
                R0 = R5 >> R8;
  		R0 = R11 >> 14;
                R25 = R16 << R18;
		R24 = R17 << 11;
                //compare unit
		R0 = R0 < R1;
                R0 = R0 < 3;
                R0 = R1 == R2;
                R0 = R1 == 18;
                R0 = ULT(R1, R15); 
	        R0 = ULT(R1, 15);
		R0 = R16;
                R0 = 5;
                //rest of instructions
                CELL_SHR(R0, R1);
                CELL_SHL(R1,R2);
		CELL_SHR(R0, 5);
		CELL_SHL(R0, 5);
                R0 = SHIFT_REG;
		R0 = INDEX;
		NOP;
                REDUCE(R0);
                REPEAT_X_TIMES(2);
			R0 = R1 + R2;
                END_REPEAT;
		R0 = LS[R1];
                R0 = LS[15];
		LS[R1] = R0;
		LS[15] = R0;
                        )
    END_KERNEL("Test_kernel");
}

void test_kernel(){
     //vector<int> h;
     cout<<ConnexMachine::dumpKernel("Test_kernel")<<endl;
     cout<<"Disassamble version"<<endl; 
     cout<<ConnexMachine::disassembleKernel("Test_kernel")<<endl;
     //cout<<endl<<endl<<"Instructions counter"<<endl;
     //h = ConnexMachine::getConnexInstructionsCounter(); 
}

int main(){
	try{
		Kernel1();
		test_kernel();
	}catch(std::string ex){
		cout << "Exception thrown: " << ex << endl;
	}
	return 0;
}

