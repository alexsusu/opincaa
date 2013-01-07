#ifndef UTILS_H
#define UTILS_H

#define UINT8 unsigned char
#define UINT16 unsigned short int
#define INT16 short int
#define UINT32 unsigned int
#define INT32  int
#define UINT64 unsigned long long int
#define INT64  long long int
#define UINT_INSTRUCTION UINT32

#define NUMBER_OF_MACHINES 128LL
#define REDUCTION_SIZE      (7 + REGISTER_SIZE) // 7 = log2(NUMBER_OF_MACHINES)
#define REDUCTION_SIZE_MASK ((1 << REDUCTION_SIZE) -1)

#define REGISTER_SIZE       16  //bits
#define REGISTER_SIZE_MASK ((1 << REGISTER_SIZE) -1)

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
extern int (*IO_WRITE_NOW)(void*);
extern int (*IO_READ_NOW)(void*);

#endif // TYPES_H
