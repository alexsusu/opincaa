#include "controller_tests.h"
#include "ConnexMachine.h"

void jump_kernel_init(int n){
    BEGIN_KERNEL("jump_test")
        EXECUTE_IN_ALL(
            R0 = 0;
            R1 = 1;
            REPEAT(n)
                //R0 = R0 + R1;
                REDUCE(R1);
            END_REPEAT
            NOP;
        )
    END_KERNEL("jump_test")
}

int jump_test(int n, ConnexMachine * connex){

    int i;

    printf("Initializing jump kernel\n");
    jump_kernel_init(n);

    printf("Running jump kernel\n");
    connex->executeKernel("jump_test");

    for(i=0;i<n;i++){
        printf("%x\n",connex->readReduction());
    }
}
