/*
 * Test the automatically-generated DES implementation.
 *
 * Key should be - 5e d9 20 4f ec e0 b9 67
 *
 * Written by Matthew Kwan - April 1997.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#define UINT64 unsigned long int
#define UINT32 unsigned int
#define UINT16 unsigned short int
#define UINT8  unsigned char
#define UINTX UINT32

#define LOOPS (1000*100)
//#define LOOPS 1

void START_COUNTER();
void STOP_COUNTER();
void PRINT_TIME(char *);
void set_bitlength ();
