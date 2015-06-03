/*
 * File: Architecture.h
 *
 * Holds information regarding the machine configuration.
 */

#include <cstdint>

#ifndef CONNEX_REGISTER_SIZE
#define CONNEX_REGISTER_SIZE 		16
#endif

#ifndef CONNEX_VECTOR_LENGTH
#define CONNEX_VECTOR_LENGTH 		128
#endif

#ifndef CONNEX_REG_COUNT
#define CONNEX_REG_COUNT 			32
#endif

#ifndef CONNEX_MEM_SIZE
#define CONNEX_MEM_SIZE 			1024
#endif

#define IO_WRITE_OPERATION          0x00000001
#define IO_READ_OPERATION           0x00000000

#define REG_MAX_VAL 		    0xffff

#define INSTRUCTION_QUEUE_LENGTH	1024

#define UINT_INSTRUCTION            uint32_t
#define UINT_RED_REG_VAL            uint32_t
#define UINT_REGISTER_VAL           uint16_t
#define UINT_PARAM                  uint16_t

#define REDUCTION_SIZE              (7 + CONNEX_REGISTER_SIZE)
#define REDUCTION_SIZE_MASK         ((1 << REDUCTION_SIZE) -1)

#define REGISTER_SIZE_MASK          ((1 << CONNEX_REGISTER_SIZE) -1)

#define IO_WRITE_OPERATION          0x00000001
#define IO_READ_OPERATION           0x00000000
#define IO_LS_ADDRESS(x)               ((x) & 0x000003FF)
#define IO_VECTOR_COUNT(x)             (((x) - 1) & 0x000003FF)


