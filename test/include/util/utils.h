#ifndef UTILS_H
#define UTILS_H

#define _BEGIN_KERNEL(x) BEGIN_KERNEL("simpleTest_" + to_string(x))
#define _END_KERNEL(x) END_KERNEL("simpleTest_" + to_string(x))

using namespace std;
#ifdef __MINGW32__
#include <iostream>
std::string to_string(int val);
#endif // __MINGW32__

#ifndef UINT8
#define UINT8 unsigned char
#endif

#ifndef UINT16
#define UINT16 unsigned short int
#endif

#ifndef INT16
#define INT16 short int
#endif

#ifndef UINT32
#define UINT32 unsigned int
#endif

#ifndef INT32
#define INT32  int
#endif

#ifndef INT64
#define UINT64 unsigned long long int
#endif

#ifndef INT64
#define INT64  long long int
#endif

#define UINT_INSTRUCTION UINT32 // how long an instruction is
#define UINT_RED_REG_VAL UINT32 //actually 16+ log2(128) = 23 bits are enough for RedAdd
#define UINT_REGISTER_VAL UINT16
#define UINT_PARAM UINT16
#define BYTES_IN_DWORD 4

#define REDUCTION_SIZE      (7 + REGISTER_SIZE) // 7 = log2(NUMBER_OF_MACHINES)
#define REDUCTION_SIZE_MASK ((1 << REDUCTION_SIZE) -1)

#define REGISTER_SIZE       16  //bits
#define REGISTER_SIZE_MASK ((1 << REGISTER_SIZE) -1)

#define PASS 0
#define FAIL -1

#define CPPAMP_EMULATION_MODE	3
#define C_SIMULATION_MODE       2
#define VERILOG_SIMULATION_MODE 1
#define REAL_HARDWARE_MODE      0
#define INVALID_MODE           -1

void initRand();
INT32 randPar(INT32 limit);
INT32 SumRedOfFirstXnumbers(UINT32 numbers, UINT32 start);
void eatRand(int times);

#endif // TYPES_H
