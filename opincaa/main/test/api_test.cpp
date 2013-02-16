#include <fcntl.h>
#include <unistd.h>
#include <iostream>

#include "ConnexMachine.h"


void initKernel(int val, int shift)
{
    BEGIN_KERNEL("test_"+std::to_string(val)+"_"+std::to_string(shift));
    EXECUTE_IN_ALL(
	R0 = val;
	R1 = R0 << shift;
	REDUCE(R1);
    )
    END_KERNEL("test_"+std::to_string(val)+"_"+std::to_string(shift));
}

int main(int argc, char ** argv)
{
    
    int i,j;
    int dist,red,iowr,iord;
    
    if((dist = open(argv[1], O_WRONLY)) < 0)
    {
        cout << argv[1] << endl;
        cout << "Error opening!" << endl;
        return 0;
    }
    
    if((red = open(argv[2], O_RDONLY)) < 0)
    {
            cout << argv[2] << endl;
        cout << "Error opening!" << endl;
        return 0;
    }
    
    if((iowr = open(argv[3], O_WRONLY)) < 0)
    {
            cout << argv[3] << endl;
        cout << "Error opening!" << endl;
        return 0;
    }

    if((iord = open(argv[4], O_RDONLY)) < 0)
    {
            cout << argv[4] << endl;
        cout << "Error opening!" << endl;
        return 0;
    }
    
    try
    {
        /* Pass the opened file descriptor as the distribution FIFO.
         * All kernels to be executed will be written to that file.
         */
        ConnexMachine *connex = new ConnexMachine(dist,red,iowr,iord);
        cout << "Success in opening the connex machine" << endl;
        
	for(i=1;i<5;i++){
	    //for(j=0;j<8;j++){
		j=0;
		initKernel(i,j);
		connex->executeKernel("test_"+std::to_string(i)+"_"+std::to_string(j));
		cout << "Expected " << 128*(i<<j) << " got " << connex->readReduction() << endl;
	    //}
	}

	delete connex;
    }
    catch(string err)
    {
        cout << err << endl;
    }
    
}