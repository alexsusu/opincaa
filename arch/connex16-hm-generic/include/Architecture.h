/*
 * File: Architecture.h
 *
 * Holds information regarding the machine configuration.
 */

#ifndef ARCHITECTURE_H_INCLUDED
#define ARCHITECTURE_H_INCLUDED

#include <limits.h>
#include <cstdint>

#ifndef CONNEX_REGISTER_SIZE
#define CONNEX_REGISTER_SIZE        16
#endif

extern int CONNEX_VECTOR_LENGTH;
extern int LOG2_CONNEX_VECTOR_LENGTH;


extern void ComputeLog2CVL();

extern int CONNEX_REG_COUNT;

extern int CONNEX_MEM_SIZE;
// TODO: Alex: maybe instead of using module operations, we should do assert(x < CONNEX_MEM_SIZE)
#define IO_LS_ADDRESS(x)            ((x) & (CONNEX_MEM_SIZE - 1))
#define IO_VECTOR_COUNT(x)          (((x) - 1) & (CONNEX_MEM_SIZE - 1))


#define IO_WRITE_OPERATION          0x00000001
#define IO_READ_OPERATION           0x00000000

// Alex: modified the original unsigned short to TYPE_ELEMENT for the type of the vectors
// 2018_05_11
#define TYPE_ELEMENT_I16
#ifdef TYPE_ELEMENT_I16
  //typedef unsigned short TYPE_ELEMENT;
  typedef short TYPE_ELEMENT;
  typedef unsigned short UNSIGNED_TYPE_ELEMENT;

  // See e.g. https://www.tutorialspoint.com/c_standard_library/limits_h.htm
  const TYPE_ELEMENT MIN_TYPE_ELEMENT = SHRT_MIN; // -32768;
  const TYPE_ELEMENT MAX_TYPE_ELEMENT = SHRT_MAX; // 32767;

  #define REG_MAX_VAL                 0xffff
#else
  typedef int TYPE_ELEMENT;
  typedef unsigned int UNSIGNED_TYPE_ELEMENT;

  // See e.g. https://www.tutorialspoint.com/c_standard_library/limits_h.htm
  const TYPE_ELEMENT MIN_TYPE_ELEMENT = INT_MIN;
  const TYPE_ELEMENT MAX_TYPE_ELEMENT = INT_MAX;

  #define REG_MAX_VAL                 0xffffffff
#endif


extern int INSTRUCTION_QUEUE_LENGTH;

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
