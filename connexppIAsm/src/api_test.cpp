#include <fcntl.h>
#include <unistd.h>
#include <iostream>

#include "Kernel.h"
#include "ConnexMachine.h"


void initKernel()
{
    BEGIN_KERNEL("test");
    R2 = LS[0];
    R1 = LS[1];
    R1 = R2 + R1;
    NOP;
    REDUCE(R1);
    END_KERNEL("test");
}

int main()
{
    
    initKernel();
    
    int file;
    
    if((file = open("test.dump", O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR)) < 0)
    {
        cout << "Error opening!" << endl;
    }
    
    try
    {
        /* Pass the opened file descriptor as the distribution FIFO.
         * All kernels to be executed will be written to that file.
         */
        ConnexMachine *connex = new ConnexMachine(file, 0, 0, 0);
        cout << "Success in opening the connex machine" << endl;
        connex->executeKernel("test");
        connex->executeKernel("test");
        cout << "Execute complete twice (kernel written to file descriptor)" << endl;
        delete connex;
    }
    catch(string err)
    {
        cout << err << endl;
    }
    
}