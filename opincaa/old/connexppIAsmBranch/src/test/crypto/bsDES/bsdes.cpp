/*
 * Test the automatically-generated DES implementation.
 *
 * Key should be - 5e d9 20 4f ec e0 b9 67
 *
 * Based on Matthew Kwan - April 1997.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

/* External functions */
//extern unsigned long	deseval (const unsigned long *p,
//			const unsigned long *c, const unsigned long *k);

/*
 * Entry point.
 */


extern int trial2a_test();
extern int trial2b_test();

int bsDesTest()
{
    //getMachineType();
    //trial1_test();
    //trial1b_test();
    //trial1c_test();
    //trial2_test();
    trial2a_test();
    trial2b_test();
}
