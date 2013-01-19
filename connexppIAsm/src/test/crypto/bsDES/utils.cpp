/*
 * Set the bit length variables.
 */
#include "../../../../include/test/crypto/bsDES/utils.h"

static UINT8 bitlength;
static UINT8 bitlength_log2;

void set_bitlength (void)
{
	unsigned long	x = ~0UL;

	bitlength = 0;
	for (x = ~0UL; x != 0; x >>= 1)
	    bitlength++;

	printf ("%d-bit machine\n", bitlength);

	if (bitlength == 64)
	    bitlength_log2 = 6;
	else if (bitlength == 32)
	    bitlength_log2 = 5;
	else {
	    fprintf (stderr, "Cannot handle %d-bit machines\n", bitlength);
	    exit (1);
	}
}

static    clock_t start_time,end_time;
void START_COUNTER()
{
    start_time = clock();
}

void STOP_COUNTER()
{
    end_time = clock();
}

void PRINT_TIME(char *text)
{
    printf("Time = %d (ms) %s\n",(int)(end_time - start_time),text);
}

UINT8 getMachineType()
{
    UINT16 i;
    volatile uint8_t a;
    volatile uint16_t b;
    volatile uint32_t c;
    volatile uint64_t d;

    START_COUNTER();
    for (i=0; i< LOOPS; i++)
    {
        a = 0;
        while ((a++) != 0xFF);
    }
    STOP_COUNTER();
    PRINT_TIME(" 1M UINT8+");

    START_COUNTER();
    for (i=0; i< LOOPS; i++)
    {
        b = 0xFF00;
        while ((b++) != 0xFFFF);
    }
    STOP_COUNTER();
    PRINT_TIME(" 1M UINT16+");

    START_COUNTER();
    for (i=0; i< LOOPS; i++)
    {
        c = 0xFFFFFF00;
        while ((c++) != 0xFFFFFFFF);
    }
    STOP_COUNTER();
    PRINT_TIME(" 1M UINT32+");

    START_COUNTER();
    for (i=0; i< LOOPS; i++)
    {
        d = 0xFFFFFFFFFFFFFF00;
        while ((d++) != 0xFFFFFFFFFFFFFFFF);
    }
    STOP_COUNTER();
    PRINT_TIME(" 1M UINT64+");
}
