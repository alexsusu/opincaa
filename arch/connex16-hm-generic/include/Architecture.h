/*
 * File: Architecture.h
 *
 * Holds information regarding the machine configuration.
 */

#ifndef ARCHITECTURE_H_INCLUDED
#define ARCHITECTURE_H_INCLUDED

#include <limits.h>
#include <cstdint>


/*
// 2018_11_28: This is a fix for Cygwin, which doesn't normally have std::to_string().
#include <sstream>
// From https://stackoverflow.com/questions/12975341/to-string-is-not-a-member-of-std-says-g-mingw
namespace std {
    static std::string to_string(size_t n) {
        std::ostringstream s;
        s << n;
        return s.str();
    }
}
*/

// Following are the debug activation macros
////#define DEBUG_OPINCAA
//
// Used in Instruction::assemble() and Instruction::Instruction()
//// #define DEBUG_OPINCAA_INSTRUCTION_ASSEMBLE
//
// Used in ConnexVector, etc
//#define DEBUG_OPINCAA_EXTRA
//#define DEBUG_OPINCAA_EXTRA_ACTIVE
//
// Other macros
// Print extra-debug info (what instruction gets executed) when executing simulator
// #define DEBUG_OPINCAA_PRINT_SIM
// This macro allows to see total number of cycles and estimated energy consumed
// #define DEBUG_OPINCAA_PRINT_SIM_CYCLES
// #define DEBUG_OPINCAA_PRINT_SIM_MORE // Shows the executed instructions
//
//
/* Activate the print trace functionality when executing instructions in the
    simulator - useful for debugging code compiled by OpincaaLLVM and
     compare it with correct manual code to see where the bug is.
      - I used vimdiff on a correct and a buggy trace to find bugs
         - vimdiff's auto alignment helps.
*/
//#define DEBUG_OPINCAA_PRINT_TRACE_REG_VALUES


#define MAX_DEPTH_LOOP_NESTING 10





#ifndef CONNEX_REGISTER_SIZE
#define CONNEX_REGISTER_SIZE        16
#endif

extern int CONNEX_VECTOR_LENGTH;
extern int LOG2_CONNEX_VECTOR_LENGTH;


extern void ComputeLog2CVL();

extern int CONNEX_REG_COUNT;


// Note: we could define also CONNEX_MEM_SIZE == (CONNEX_MEM_NUM_ROWS + CONNEX_MEM_NUM_ROWS_EXTRA) * CONNEX_VECTOR_LENGTH * sizeof(ConnexVectorElementType)

// Extra LS memory for spills and LUTs for div/sqrt.f16, etc
#define CONNEX_MEM_NUM_ROWS_EXTRA 200
#define CONNEX_MEM_NUM_ROWS_EXTRA_FOR_SPILL 50
#define CONNEX_MEM_SPILL_START_OFFSET (CONNEX_MEM_NUM_ROWS + CONNEX_MEM_NUM_ROWS_EXTRA_FOR_SPILL)

extern int CONNEX_MEM_NUM_ROWS;
// TODO: maybe instead of using modulo operations, we should do assert(x < CONNEX_MEM_NUM_ROWS)
#define IO_LS_ADDRESS(x)            ((x) & (CONNEX_MEM_NUM_ROWS - 1))
#define IO_VECTOR_COUNT(x)          (((x) - 1) & (CONNEX_MEM_NUM_ROWS - 1))


#define IO_WRITE_OPERATION          0x00000001
#define IO_READ_OPERATION           0x00000000

// 2018_05_11
#define CONNEX_VECTOR_ELEMENT_TYPE_I16
#ifdef CONNEX_VECTOR_ELEMENT_TYPE_I16
  //typedef short ConnexVectorElementType;
  typedef int16_t ConnexVectorElementType;
  typedef uint16_t UnsignedConnexVectorElementType;

  // See e.g. https://www.tutorialspoint.com/c_standard_library/limits_h.htm
  const ConnexVectorElementType MIN_CONNEX_VECTOR_ELEMENT_TYPE = SHRT_MIN; // -32768;
  const ConnexVectorElementType MAX_CONNEX_VECTOR_ELEMENT_TYPE = SHRT_MAX; // 32767;

  #define REG_MAX_VAL                 0xffff
#else
  typedef int ConnexVectorElementType;
  typedef unsigned int UnsignedConnexVectorElementType;

  // See e.g. https://www.tutorialspoint.com/c_standard_library/limits_h.htm
  const ConnexVectorElementType MIN_CONNEX_VECTOR_ELEMENT_TYPE = INT_MIN;
  const ConnexVectorElementType MAX_CONNEX_VECTOR_ELEMENT_TYPE = INT_MAX;

  #define REG_MAX_VAL                 0xffffffff
#endif


extern int INTERNAL_INSTRUCTION_MEMORY_SIZE;

typedef uint32_t UIntInstruction;
typedef uint32_t UIntRedRegVal;
typedef uint16_t UIntRegisterVal;
typedef uint16_t UIntParam;


#define REDUCTION_SIZE              (7 + CONNEX_REGISTER_SIZE)
#define REDUCTION_SIZE_MASK         ((1 << REDUCTION_SIZE) - 1)

#define REGISTER_SIZE_MASK          ((1 << CONNEX_REGISTER_SIZE) - 1)

#define IO_WRITE_OPERATION          0x00000001
#define IO_READ_OPERATION           0x00000000


extern bool checkForDataHazards;
extern bool useLaneGatingOnConnex;
extern int numMaxNestedHwLoops; // 2020_04_20
extern bool dontExecuteKernel; // 2020_03_29: Used to retrieve kernel->size()

#endif // ARCHITECTURE_H_INCLUDED
