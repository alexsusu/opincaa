/*
 * File: Architecture.h
 *
 * Holds information regarding the machine configuration.
 */

#ifndef ARCHITECTURE_H_INCLUDED
#define ARCHITECTURE_H_INCLUDED

#include <cstdint>

#ifndef CONNEX_REGISTER_SIZE
#define CONNEX_REGISTER_SIZE        16
#endif

#ifndef CONNEX_VECTOR_LENGTH
 #define CONNEX_VECTOR_LENGTH_128
 //#define CONNEX_VECTOR_LENGTH_64

 #ifdef CONNEX_VECTOR_LENGTH_128
  #define LOG2_CONNEX_VECTOR_LENGTH      7
  #define CONNEX_VECTOR_LENGTH      128
 #endif
 #ifdef CONNEX_VECTOR_LENGTH_64
  #define LOG2_CONNEX_VECTOR_LENGTH      6
  #define CONNEX_VECTOR_LENGTH      64
 #endif
  //#define CONNEX_VECTOR_LENGTH      256
  //#define CONNEX_VECTOR_LENGTH        512
  //#define CONNEX_VECTOR_LENGTH      8
#endif

#ifndef CONNEX_REG_COUNT
  //#define CONNEX_REG_COUNT            32

  // This we use normally (only) for the Kernel::genLLVMISelManualCode() method:
  #define CONNEX_REG_COUNT            64
#endif

#ifndef CONNEX_MEM_SIZE
 //#define CONNEX_MEM_1024_LINES
 #define CONNEX_MEM_2048_LINES

 #ifdef CONNEX_MEM_1024_LINES
   #define CONNEX_MEM_SIZE             1024
   #define IO_LS_ADDRESS(x)            ((x) & 0x000003FF)
   #define IO_VECTOR_COUNT(x)             (((x) - 1) & 0x000003FF)
 #endif
 #ifdef CONNEX_MEM_2048_LINES
   #define CONNEX_MEM_SIZE             2048
   #define IO_LS_ADDRESS(x)            ((x) & 0x000007FF)
   #define IO_VECTOR_COUNT(x)          (((x) - 1) & 0x000007FF)
 #endif
#endif

#define IO_WRITE_OPERATION          0x00000001
#define IO_READ_OPERATION           0x00000000

#define REG_MAX_VAL                 0xffff

#define INSTRUCTION_QUEUE_LENGTH    1024

#define UINT_INSTRUCTION            uint32_t
#define UINT_RED_REG_VAL            uint32_t
#define UINT_REGISTER_VAL           uint16_t
#define UINT_PARAM                  uint16_t

#define REDUCTION_SIZE              (7 + CONNEX_REGISTER_SIZE)
#define REDUCTION_SIZE_MASK         ((1 << REDUCTION_SIZE) - 1)

#define REGISTER_SIZE_MASK          ((1 << CONNEX_REGISTER_SIZE) - 1)

#define IO_WRITE_OPERATION          0x00000001
#define IO_READ_OPERATION           0x00000000


#endif // ARCHITECTURE_H_INCLUDED
