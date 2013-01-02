#ifndef TYPES_H
#define TYPES_H

#define UINT8 unsigned char
#define UINT16 unsigned short int
#define INT16 short int
#define UINT32 unsigned int
#define UINT64 unsigned long long int
#define INT64  long long int
#define UINT_INSTRUCTION UINT32
#define NUMBER_OF_MACHINES 128LL

#define PASS 0
#define FAIL -1

#define C_SIMULATION_MODE       2
#define VERILOG_SIMULATION_MODE 1
#define REAL_HARDWARE_MODE      0

#define INIT(x) initialize(x)
#define DEINIT() deinitialize()

int initialize(UINT8);
int deinitialize();
extern int (*EXECUTE_KERNEL)(UINT16 dwBatchNumber);
extern int (*EXECUTE_KERNEL_RED)(UINT16 dwBatchNumber);

#endif // TYPES_H