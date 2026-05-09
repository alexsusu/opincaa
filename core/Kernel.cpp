/*
 * File:   Kernel.cpp
 *
 * This is the  class containing a kernel
 * (a vector of Instructions) for executing on the Connex Array
 *
 */


#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <iostream>
//#include <ostream>
#include <sstream>
//#include <iostream>
#include <iomanip> // for setw, setfill

#include <unordered_map>

#include "Architecture.h"
#include "ConnexLLVM.h"
#include "Kernel.h"
//
#include "CheckDataHazard.h"


/* This macro, most importantly, removes the getCopyToReg(), leaving the
   generated code the description of a pure-dataflow SelectionDAG.
   If we disable this macro (before), the generated code used
     getCopyToReg() for predicated instructions, but this is NOT required since
     the tied-to constraints can do the job very nicely themselves using only
     SDValue obtained from getMachineNode().
*/
#define NEW2018_08_10

//#define CONVERT_PARTLY_SSA_FORM

/************************************************************
* Constructor for creating a new Kernel
*
* @param name the name of the new kernel\
*
* @throws string if the name is invalid (NULL or empty)
*/
Kernel::Kernel(string aName) {
  #ifdef DEBUG_OPINCAA
    printf("Entered Kernel::Kernel(): name = %s.\n", aName.c_str());
    fflush(stdout);
  #endif

    /* We start from index 1 (index 0 is NOT used) since we, as LLVM,
       consider a simple loop has a depth of 1 (depth 0 doesn't exist). */
    loopNestDepth = 0;

    //sdNodeVarNameRegDef = (string *)malloc(sizeof(string) * CONNEX_REG_COUNT);
    sdNodeVarNameRegDef = new string[CONNEX_REG_COUNT];
    assert(sdNodeVarNameRegDef != NULL);
    //printf("  Kernel::Kernel(): sdNodeVarNameRegDef = %p\n", sdNodeVarNameRegDef);
    //fflush(stdout);

    if (aName.length() == 0) {
        throw new string("Invalid kernel name");
    }
    this->name = aName;
}

/************************************************************
* Destructor for the Kernel class
*
*/
Kernel::~Kernel() {
  #ifdef DEBUG_OPINCAA
    printf("Entered Kernel::~Kernel(): name = %s\n", name.c_str());
    printf("  Kernel::~Kernel(): sdNodeVarNameRegDef = %p\n", sdNodeVarNameRegDef);
    fflush(stdout);
  #endif

    // Gets automatically deallocated since it is a member variable: instructions.clear();
    // Gets automatically deallocated since it is a member variable: name.clear();

    assert(sdNodeVarNameRegDef != NULL);
    // It seems all these strings get automatically deallocated before entering the destructor
    /*
    for (int i = 0; i < CONNEX_REG_COUNT; i++) {
        sdNodeVarNameRegDef[i].clear();
    }
    */

    /* This is a rather weird bug - using any of the following 2 instructions gives error <<free(): invalid pointer>>:
      // Because I don't use delete[] while I should since sdNodeVarNameRegDef is an array:
      //delete sdNodeVarNameRegDef;
      // Because I can't mix new and free()
      //free(sdNodeVarNameRegDef);
    */
    // This works well:
    delete[] sdNodeVarNameRegDef;

    sdNodeVarNameRegDef = NULL;

    //printf("Exiting Kernel::~Kernel()\n");
    //fflush(stdout);
}



/************************************************************
* Appends an existing instruction to the kernel
*
* @param instruction the instruction to add
*/
void Kernel::append(Instruction instruction) {
    //printf("checkForDataHazards = %d\n", checkForDataHazards);
    if (checkForDataHazards && instructions.size() > 0) {
        // From http://www.cplusplus.com/reference/vector/vector/back/
        //InstructionType prevInstr = instructions.back();
        Instruction prevInstr = Instruction(instructions.back());

        int instructionsSize = instructions.size();

        bool res = CheckDataHazard(instruction, prevInstr);
        if (res) {
            // We now add a NOP, since it is required

            printf("We now add a NOP, since CheckDataHazard() says it is "
                   "required (instructionsSize = %d)\n\n",
                   instructionsSize - 1);

            Instruction nopInstr = Instruction(_NOP, 0, 0, 0);
            instructions.push_back(nopInstr.assemble());

            instructionsSize = instructions.size();

            //jumpTargetForLoopOfNestDepth[loopNestDepth]++;
            // Note: We start from index 1 (index 0 is NOT used) since we, as LLVM,
            //  consider a simple loop has a depth of 1 (depth 0 doesn't exist).
            for (int i = 1; i <= loopNestDepth; i++)
                jumpTargetForLoopOfNestDepth[i]++;
        }
      #ifdef PERFORMANCE_NOT_PARAMOUNT
        else
        // We now check for unnecessary NOPs
        if (prevInstr.getOpcode() == _NOP && instructionsSize >= 2) {
            Instruction prev2Instr = Instruction(instructions[instructionsSize - 2]);

            if (prev2Instr.getOpcode() != _IJMPNZ) { // Currently END_REPEAT is _IJMPNZ and _NOP
                bool res2 = CheckDataHazard(instruction, prev2Instr, false);
                if (res2 == false) {
                    printf("Found an unnecessary NOP %s at index = %d in the program "
                            "- there's no data hazard "
                            "between its previous instruction %s and its next one, %s "
                            "(although the NOP can be between CELL_SH* and SHIFT_REG).\n",
                            prevInstr.dump().c_str(),
                            instructionsSize - 1,
                            prev2Instr.dump().c_str(),
                            instruction.dump().c_str());

                  #ifdef TO_IMPLEMENT_PERFECTLY
                    // We take out the unnecessary NOP
                    // MEGA-TODO: handle cases between CELL_SH* and SHIFT_REG - although it seems difficult...
                    instructions.pop_back();
                  #endif
                }
            }
        }
      #endif
    }

    //InstructionType crtInstr = instruction.assemble();
    instructions.push_back(instruction.assemble());

    // Note: We start from index 1 (index 0 is NOT used) since we, as LLVM,
    //  consider a simple loop has a depth of 1 (depth 0 doesn't exist).
    for (int i = 1; i <= loopNestDepth; i++)
        jumpTargetForLoopOfNestDepth[i]++;
}

/* Copy from the myBinaryData array (e.g. from a preassembled kernel)
 *   to the instructions member of the Kernel class. */
void Kernel::copyBinaryKernel(InstructionType *myBinaryData, int numInstructions) {
    // See http://www.cplusplus.com/reference/vector/vector/assign/
    instructions.assign(myBinaryData, myBinaryData + numInstructions);
}


/************************************************************
* Writes the kernel to a memory location
*
* @param buffer the memory location to write the kernel to
*/
void Kernel::writeTo(void *buffer) {
    memcpy(buffer, instructions.data(), instructions.size() * sizeof(InstructionType));
}

/************************************************************
* Writes the kernel to a file descriptor
* @param fileDescriptor the file descriptor to write the kernel to
*/
void Kernel::writeTo(int fileDescriptor) {
    int res;

    /* This write() is NON-blocking for Zedboard (something explained by the
     *    fact it writes to a block device, /dev/xillybus_connex_instruction_32,
     *    that is supported by the Xillybus protocol using DMA).
     *   But in the OPINCAA simulator it is blocking since writes to pipes are
     *     blocking until all the data to be written is put in the internal
     *     buffer of the pipe, from where the consumer can later read the data.
     */
    res = write(fileDescriptor, instructions.data(), instructions.size() * sizeof(InstructionType));

    /* Flush the descriptor - it is NON-blocking for both Zedboard and OPINCAA
     *    simulator.
     *
     *  On Zedboard, this increases the performance of kernel execution
     *    (so it doesn't help for correctness).
     *    For example, for MatMul-128.i16 we have a performance increase of
     *       about 4x when we flush.
     *  For the OPINCAA simulator this does not have any effect.
     *  */
    res = write(fileDescriptor, NULL, 0);
}

/************************************************************
 * Returns the number of instructions in this kernel
 *
 * @return the number of instructions in this kernel
 */
unsigned Kernel::size() {
    return instructions.size();
}

/************************************************************
 * Returns the name of this kernel
 *
 * @return the name of this kernel
 */
string Kernel::getName()
{
    return name;
}

/************************************************************
 * Returns a string representing the dumped kernel, one
 * instruction per line.
 *
 * @return the disassembled kernel
 */
string Kernel::dump() {
    string kernelStr;

    printf("Kernel::dump(): Num instructions = %d\n", this->size());

    //for (vector<InstructionType>::iterator element = instructions.begin(); element != instructions.end(); element++) {
    //    kernelStr += Instruction(*element).dump() + "\n";
    for (auto crtElement: instructions) {
        kernelStr += Instruction(crtElement).dump() + "\n";
    }

    return kernelStr;
}

/*
 * Returns a string representing the disassembled kernel.
 * One instruction per line.
 */
string Kernel::disassemble() {
    string kernel;

    printf("Kernel::disassemble(): Num instructions = %d\n", this->size());

    for (vector<InstructionType>::iterator element = instructions.begin();
         element != instructions.end();
         element++) {
        kernel += Instruction(*element).disassemble();
    }

    return kernel;
}

// We check if instrCrt already has a true (RAW) data-dependence with instrSucc
inline bool isRequiredGlueOrChainOutput(Instruction &instrCrt,
                                        Instruction &instrSucc) {
    /*
    if (instrCrt.getDest() != NO_REG_INDEX &&
            ((instrSucc.getLeft() != NO_REG_INDEX &&
                    instrCrt.getDest() == instrSucc.getLeft()) ||
             (instrCrt.getType() == INSTRUCTION_TYPE_NO_IMMEDIATE &&
              (instrSucc.getRight() != NO_REG_INDEX &&
                instrCrt.getDest() == instrSucc.getRight()))
             )) {
        return false;
    }
    */

    return true;

  #ifdef EXPERIMENT_WITH_CSE_IN_SELECTIONDAG_AND_DAGCOMBINER
    /* This allows SelectionDAG (and DAGCombiner) to perform CSE and other
        optimizations on the DAG we generate. */
    return false;
  #endif
}


/*
To avoid the reasonably big overhead of assembling the kernel at runtime,
given by the rather complex C++ framework of Opincaa (classes Operand,
Instruction, etc) we can do the following:
    - since the OpincaaLLVM compiler generates vector code by defining
        an Opincaa kernel inside the vectorized loop(s) it is quite difficult
        to run all the (registered) kernels at the very beginning of the
        execution of the Opincaa program we do the following:
        - we run the Opincaa program once before the actual run to generate
          precomputed tables with the assembled kernels, with the instructions
          of the kernel.
*/
string Kernel::genPrecomputedKernel() {
    int i;
    string res;
    stringstream ss;
    int kernelSize = size();

    //printf("InstructionType table[%d] = {\n", kernelSize);
    ss << "#define PREASSEMBLED_BINARY_KERNEL_SIZE " << kernelSize << "\n";
    //ss << "InstructionType preassembledBinaryKernel[" << kernelSize << "] = {\n";
    ss << "InstructionType preassembledBinaryKernel[PREASSEMBLED_BINARY_KERNEL_SIZE] = {\n";

    for (i = 0; i < kernelSize; i++) {
        //printf("0x%08x%s", instructions[i], (i == kernelSize - 1) ? "": ", ");
        ss << "0x" << std::setfill('0') << std::setw(8) << std::hex
           << instructions[i];
        if (i != kernelSize - 1)
            ss << ", ";
    }
    //printf("};\n");
    ss << "};\n";

    //std::cout << ss.str();
    //return res;

    return ss.str();
}


void Kernel::DetermineDataToSaveForFission(int idxInstrBegin, int idxInstrEnd) {
    printf("Entered DetermineDataToSaveForFission(idxInstrBegin = %d, idxInstrEnd = %d)\n",
           idxInstrBegin, idxInstrEnd);

    #define INF_INDEX_INSTR 100000000
    int updatesUsedMin = INF_INDEX_INSTR;
    int updatesUsedMinIndex = -1;

    for (int idxInstr = idxInstrBegin; idxInstr < idxInstrEnd; idxInstr++) {
        /* If we fission/partition the REPEAT loop at idxInstr
         *  we need to check
         *   if the instructions idxInstr..idxInstrEnd are using HOW MANY updated (i.e. NOT loop invariant??)
         *   registers from interval idxInstrBegin..idxInstr.
         *
         *   For example:
         *     Racc = Racc + Rx; // this is update
         *     Ra = 1; // this is NOT update - should be loop invariant
         */

        Instruction instrCrt(instructions[idxInstr]);

        printf(" Handling idxInstr = %d (instrCrt = %s - analyzing fissioning just before it)\n",
               idxInstr,
               instrCrt.dump().c_str()
               );

        int updatesUsed = 0;

        #define MAX_NUM_REGISTERS 64
        assert(MAX_NUM_REGISTERS >= CONNEX_REG_COUNT);
        int registerUpdated[MAX_NUM_REGISTERS];
        //
        bool registerUsed[MAX_NUM_REGISTERS];
        for (int idxReg = 0; idxReg < CONNEX_REG_COUNT; idxReg++) {
            registerUpdated[idxReg] = INF_INDEX_INSTR;
            registerUsed[idxReg] = false;
        }

        for (int idxInstrA = idxInstrBegin; idxInstrA < idxInstr; idxInstrA++) {
            //printf("idxInstrA = %d\n", idxInstrA);

            Instruction instrCrt(instructions[idxInstrA]);

            if (instrCrt.getDest() != NO_REG_INDEX) {
                registerUpdated[instrCrt.getDest()] = idxInstrA;
            }
        }
        printf("    registers defined:");
        for (int idxReg = 0; idxReg < CONNEX_REG_COUNT; idxReg++) {
            if (registerUpdated[idxReg] < INF_INDEX_INSTR) {
                printf(" R(%d), ", idxReg);
            }
        }
        printf("\n");

        for (int idxInstrB = idxInstr; idxInstrB <= idxInstrEnd; idxInstrB++) {
            //printf("idxInstrB = %d\n", idxInstrB);

            Instruction instrCrt(instructions[idxInstrB]);

            int regLeft = instrCrt.getLeft();
            if (regLeft != NO_REG_INDEX) {
                if (registerUpdated[regLeft] < idxInstr) {
                    Instruction instrTmp(instructions[registerUpdated[regLeft]]);

                    printf("    We reuse left (reg %d, from %s (index = %d)) of instrCrt = %s\n",
                           regLeft,
                           //instrTmp.toString().c_str(),
                           //instrTmp.disassemble().c_str(),
                           instrTmp.dump().c_str(),
                           registerUpdated[regLeft],
                           instrCrt.dump().c_str());
                    registerUsed[regLeft] = true;

                    if (instrTmp.getOpcode() == _VLOAD ||
                        instrTmp.getOpcode() == _OR
                        // MEGA-TODO TODO: check that instrTmp is an index increment instruction ||
                        ) { // && instrTmp.getLeft() == instrTmp.getRight() && def[instrTmp.getLeft()] == _VLOAD))
                        printf("      But we can rematerialize directly since instrCrt is VLOAD... (MAYBE to check better if VLOAD doesn't have symbolic operand, although it shouldn't matter if we fission the REPEAT loop)\n");
                    }
                    // MEGA-TODO: check if regLeft is assigned a constant value (that doesn't come
                    //     from a symbolic operand, which has potential to change)
                }
            }

            int regRight = instrCrt.getRight();
            if (regRight != NO_REG_INDEX) {
                if (registerUpdated[regRight] < idxInstr) {
                    Instruction instrTmp(instructions[registerUpdated[regRight]]);

                    printf("    We reuse right (reg %d, from %s (index = %d)) of instrCrt = %s\n",
                           regRight,
                           instrTmp.dump().c_str(),
                           registerUpdated[regRight],
                           instrCrt.dump().c_str());
                    registerUsed[regRight] = true;
                }
            }

            // IMPORTANT: We update registerUsed after we checked left and right operands
            if (instrCrt.getDest() != NO_REG_INDEX) {
                registerUpdated[instrCrt.getDest()] = idxInstrB;
            }
        }

        printf("    registers used:");
        for (int idxReg = 0; idxReg < CONNEX_REG_COUNT; idxReg++) {
            if (registerUsed[idxReg]) {
                printf(" R(%d), ", idxReg);
                updatesUsed++;
            }
        }
        printf("\n");

        if (idxInstr - idxInstrBegin < INTERNAL_INSTRUCTION_MEMORY_SIZE &&
            idxInstrEnd - idxInstr < INTERNAL_INSTRUCTION_MEMORY_SIZE &&
                updatesUsedMin > updatesUsed) {
            updatesUsedMin = updatesUsed;
            updatesUsedMinIndex = idxInstr;
        }

        printf("    idxInstr = %d, updatesUsed = %d\n", idxInstr, updatesUsed);
    }

    printf("  updatesUsedMin = %d\n", updatesUsedMin);
    printf("    updatesUsedMinIndex = %d\n", updatesUsedMinIndex);
} // END Kernel::DetermineDataToSaveForFission()


// 2020_04_22
//  We look at the nested Repeat loops and read numMaxNestedHwLoops
void Kernel::validateAndOptimizeRepeatLoops() {
    printf("Entered Kernel::validateAndOptimizeRepeatLoops()\n");
    printf("  INTERNAL_INSTRUCTION_MEMORY_SIZE = %d\n", INTERNAL_INSTRUCTION_MEMORY_SIZE);
    printf("  kernel->size() = %d\n", this->size());
    printf("  numMaxNestedHwLoops = %d\n", numMaxNestedHwLoops);
    fflush(stdout);

    // Find Repeat loops with problems

    int numRepeatLoops = 0;
    int crtDepthLoop = 0;
    // For each loop in the kernel
    #define NMAX 100000
    int startInstrIndexLoop[NMAX];
    int endInstrIndexLoop[NMAX];
    int depthLoop[NMAX];
    int dadLoop[NMAX];
    int tripCountLoop[NMAX];

    vector<int> stackLoopNumber;

    //for (auto crtElement: instructions) {
    for (int idx = 0; idx < instructions.size(); idx++) {
        Instruction instrCrt(instructions[idx]);

        if (instrCrt.getOpcode() == _IJMPNZ) {
            int loopNum = stackLoopNumber.back();

//            endInstrIndexLoop[numRepeatLoops] = idx;
            endInstrIndexLoop[loopNum] = idx;
//            depthLoop[numRepeatLoops] = crtDepthLoop / 2;
//            depthLoop[loopNum] = crtDepthLoop / 2;
            depthLoop[loopNum] = crtDepthLoop;

//            numRepeatLoops++;
            crtDepthLoop--;

            stackLoopNumber.pop_back();
//            stackLoopNumber.pop_back();
        }
        else
        if (instrCrt.getOpcode() == _SETLC) {
            // We generate two consecutive SETLC (hw workaround)
            if ((numRepeatLoops % 2) == 1) {
                int loopNum = numRepeatLoops / 2;
                //int loopNum = numRepeatLoops;
                /*
                printf("  loopNum = %d\n", loopNum);
                fflush(stdout);
                */

                if (stackLoopNumber.empty() == true)
                    dadLoop[loopNum] = -1;
                else
                    dadLoop[loopNum] = stackLoopNumber.back();

                stackLoopNumber.push_back(loopNum);

                startInstrIndexLoop[loopNum] = idx;
                crtDepthLoop += 1;

                tripCountLoop[loopNum] = instrCrt.getValue() + 1;

            //numRepeatLoops += 1;
            }

            numRepeatLoops++; // numRepeatLoops is 2x bigger because we have 2 consecutive SETLC instructions
        }
    }

    numRepeatLoops /= 2; // numRepeatLoops is 2x bigger because we have 2 consecutive SETLC instructions
    printf("  numRepeatLoops = %d\n", numRepeatLoops);
    int i;
   #if 0
    for (i = 0; i < std::min(4, numRepeatLoops); i++) {
        printf("Repeat loop #%d\n", i);
        printf("  startInstrIndexLoop[%d] = %d\n", i, startInstrIndexLoop[i]);
        printf("  endInstrIndexLoop[%d] = %d\n", i, endInstrIndexLoop[i]);
        printf("    --> loopBodySize = %d\n", endInstrIndexLoop[i] - startInstrIndexLoop[i] - 1);
        printf("  tripCountLoop = %d\n", tripCountLoop[i]);
        printf("  depthLoop[%d] = %d\n", i, depthLoop[i]);
        printf("  dadLoop[%d] = %d\n", i, dadLoop[i]);
    }
    fflush(stdout);
   #endif

    int repeatsLeftToUseInCurrentLoopNest = numMaxNestedHwLoops + 1;

    //for (int i = 0; i < numRepeatLoops; i++)
    /* We traverse the repeat loops found in reverse order to:
     *    - process first the innermost loops and then the outermost ones
     *    - preserve the repeatLoopStart/EndInstrIndex arrays
     *
     *  NOTE: Traversing the list of repeat loops in reverse order, allows
     *    us to traverse from innermost to outermost loops, from the bottom to
     *    the top of the OPINCAAA vector kernel (or C program).
     */
    for (i = numRepeatLoops - 1; i >= 0; i--) {

printf("  repeatsLeftToUseInCurrentLoopNest = %d\n\n", repeatsLeftToUseInCurrentLoopNest);
assert(repeatsLeftToUseInCurrentLoopNest >= 0 &&
        //"We can't put more repeats in this loop nest..."
        "I'm using more repeats in a loop nest than allowed by repeatsLeftToUseInCurrentLoopNest");

        int loopBodySize = endInstrIndexLoop[i] - startInstrIndexLoop[i] - 1;

        printf("Repeat loop #%04d\n", i);
        printf("  startInstrIndexLoop[%d] = %d\n", i, startInstrIndexLoop[i]);
        printf("  endInstrIndexLoop[%d] = %d\n", i, endInstrIndexLoop[i]);
        printf("    --> loopBodySize = %d\n", loopBodySize);
        printf("  tripCountLoop = %d\n", tripCountLoop[i]);
        printf("  depthLoop[%d] = %d\n", i, depthLoop[i]);
        printf("  dadLoop[%d] = %d\n", i, dadLoop[i]);

        bool unrollDueToLackNestedRepeats = (repeatsLeftToUseInCurrentLoopNest == 0);
        bool unrollDueToBigLoopBodySize = (loopBodySize > INTERNAL_INSTRUCTION_MEMORY_SIZE - (4+1));
        if ( (tripCountLoop[i] > 1) &&
             //
                (unrollDueToLackNestedRepeats || unrollDueToBigLoopBodySize)
              //(loopBodySize > INTERNAL_INSTRUCTION_MEMORY_SIZE - (4+1))
             )
            //(loopBodySize > 10) ) // > 5
            {
            /* We found a REPEAT loop with a body that doesn't fit the IIM
             *    --> we need to completely-unroll the loop (like our C++ host
             *                                               side for loops do)
             */


            printf("We need to completely-unroll repeat loop #%d\n", i);
            //
            if (unrollDueToLackNestedRepeats) {
                printf("  because loopBodySize = %d\n", loopBodySize);
            }
            else
            if (unrollDueToBigLoopBodySize) {
                printf("  because repeatsLeftToUseInCurrentLoopNest = %d\n", repeatsLeftToUseInCurrentLoopNest);
            }
            //
            printf("  The loop is: repeat loop #%04d\n", i);
            printf("   startInstrIndexLoop[%d] = %d\n", i, startInstrIndexLoop[i]);
            printf("   endInstrIndexLoop[%d] = %d\n", i, endInstrIndexLoop[i]);
            printf("    --> loopBodySize = %d\n", endInstrIndexLoop[i] - startInstrIndexLoop[i] - 1);
            printf("   tripCountLoop = %d\n", tripCountLoop[i]);
            printf("   depthLoop[%d] = %d\n", i, depthLoop[i]);
            printf("   dadLoop[%d] = %d\n", i, dadLoop[i]);
            fflush(stdout);


            int tmp = loopBodySize * tripCountLoop[i];
            std::vector<InstructionType> unrolledBody(tmp);

            //instructions.resize(instructions.size() + tmp);
            for (int ind = 0; ind < tripCountLoop[i]; ind++) {
                std::copy(
                    instructions.begin() + (startInstrIndexLoop[i] + 1),
                    instructions.begin() + (endInstrIndexLoop[i]),
                    unrolledBody.begin() + ind * loopBodySize);
            }
            printf("After unroll, unrolledBody.size() = %lu\n", unrolledBody.size());

            string kernelStr;
            for (auto crtElementU: unrolledBody) {
                kernelStr += Instruction(crtElementU).dump() + "\n";
            }
            printf("  unrolledBody = %s\n", kernelStr.c_str());

            printf("Original kernel is:\n");
            printf("  kernel->size() = %d\n", this->size());
            printf("  instructions.size() = %lu\n", instructions.size());
//            printf("  kernel = %s\n", this->dump().c_str());
            fflush(stdout);

            int origInstrSize = instructions.size();

            printf("tmp = %d\n", tmp);
            instructions.resize(instructions.size() + tmp-4);
               // - 4 because: 2 for setlc, 1 for ijmpnz, 1 for NOP

            printf("After making space, kernel is:\n");
            printf("  kernel->size() = %d\n", this->size());
            printf("  instructions.size() = %lu\n", instructions.size());
//            printf("  kernel = %s\n", this->dump().c_str());
            fflush(stdout);

            // We make space for unrolledBody in the kernel
            // See http://www.cplusplus.com/reference/algorithm/copy/
            std::copy(instructions.begin() + (endInstrIndexLoop[i] + 2),
                                  // endInstrIndexLoop[i] points to END_REPEAT, then NOP, then useful instructions
                      //loopBodySize,
                      instructions.begin() + origInstrSize, //instructions.end(),
                      instructions.begin() +
                        (startInstrIndexLoop[i] - 1 + tmp)
                     );

            printf("After moving code after making space, kernel is:\n");
            printf("  kernel->size() = %d\n", this->size());
//            printf("  kernel = %s\n", this->dump().c_str());
            fflush(stdout);

            // We copy unrolledBody in the kernel
            std::copy(
                unrolledBody.begin(), // + (tmp-4) * loopBodySize,
                unrolledBody.end(),
                instructions.begin() + (startInstrIndexLoop[i] - 1)
                );

            printf("After unroll, kernel is:\n");
            printf("  kernel->size() = %d\n", this->size());
//            printf("  kernel = %s\n", this->dump().c_str());
            fflush(stdout);

        }
//            break;
//

repeatsLeftToUseInCurrentLoopNest--;
        if (depthLoop[i] == 1) {
            /* We just finish processing the outermost loop of the current loop
             * nest, so we adjust accordingly/
             */
            repeatsLeftToUseInCurrentLoopNest = numMaxNestedHwLoops + 1;
        }
    }
    /*
    for (auto crtElement: instructions) {
        Instruction instrCrt(crtElement);
        int instrCrtOpcode = instrCrt.getOpcode();

        switch (instrCrtOpcode) {
            //case _IJMPNZ:
            case _SETLC: {

                break;
            }
        }
    }
    */

    //  Try to fix them

    /*
     * The problem of optimal REPEAT loop allocation
     *     s.t. we allow at most k nested repeat loops
     *     all the rest being completely unrolled
     *   seems to be an NP-hard problem because
     *   it SEEMS we can't apply the suboptimality principal
     *     because the subproblems are interdependent in the sense that
     *     if we choose e.g. a leaf to NOT-unroll then this influences the
     *     parent loops in the nest.
     *
     * IMPORTANT NOTE: the loop nests of a program can form a tree.
     *  Example:
     *    REPEAT
     *      REPEAT
     *      ...
     *      END_REPEAT
     *      ...
     *      REPEAT
     *        REPEAT
     *        ...
     *        END_REPEAT
     *        ...
     *      END_REPEAT
     *    END_REPEAT
     *
     */

}

// 2019_08_22
/*
  This should be called before executeKernel()

  If there are REPEAT loops in the kernel with the
    loop body size > INTERNAL_INSTRUCTION_MEMORY_SIZE code size then we are
    forced to perform loop fission, eventually also loop tiling.
  Inputs:
    - FISSION_MEM_SIZE - number of vector lines where we can save vectors
    - INTERNAL_INSTRUCTION_MEMORY_SIZE
    - kernelName

  It will process the kernel:
    - we need to check

*/
#define FISSION_MEM_SIZE 100
//void ConnexMachine::fissionLoops(string kernelName)
// Ex usage: ConnexMachine::getKernel("MatMul")->makeLoopsValid()
//
//void Kernel::fissionLoops()
void Kernel::makeRepeatLoopsValidByUsingFissionAndTiling() {
    /* MEGA-TODO: it seems in order to do SERIOUS/proper program analysis on this
        Connex assembler code we require to use instead of for-loops which force
        unrolling at assemble time, to use a sort of REPEAT_UNROLL statements which
        allow us to distinguish between unrolling (host-side for loops) and 
        standard REPEAT() loops.
    */
    printf("Entered Kernel::makeLoopsValid()\n");
    printf("this->size() = %d\n", this->size());

    for (int idxInstr = 0; idxInstr < this->size(); idxInstr++) {
        Instruction instrCrt(instructions[idxInstr]);

        //int indexInstrCrt = idxInstr - offsetKernelToStartCodegenFrom;
        int instrCrtOpcode = instrCrt.getOpcode();

        if (instrCrtOpcode == _SETLC) {
            printf("Found _SETLC at index idxInstr = %d\n", idxInstr);

            for (int idxInstr2 = idxInstr; idxInstr2 < this->size(); idxInstr2++) {
                Instruction instrCrt2(instructions[idxInstr2]);
                int instrCrtOpcode2 = instrCrt2.getOpcode();

                if (instrCrtOpcode2 == _IJMPNZ) {
                    printf("Found _IJMPNZ at index idxInstr2 = %d\n", idxInstr2);

                    int bodyLoopSize = idxInstr2 - idxInstr;

                    printf("bodyLoopSize = %d\n", bodyLoopSize);

                    //if (bodyLoopSize > INTERNAL_INSTRUCTION_MEMORY_SIZE)
                    if (bodyLoopSize > 2) {
                        printf("This loop is invalid!! "
                               "(since bodyLoopSize > INTERNAL_INSTRUCTION_MEMORY_SIZE)\n");


// Finding the right instruction inside this loop where to perform fission
// Need to determine how much data need to save at each point we investigate to
//     perform fission
DetermineDataToSaveForFission(idxInstr, idxInstr2);
return;
#if 0 // MEGA-TODO
                        // Applying loop fission:
- It is problematic to do fission on assembly because we will have some register iterators
        which we normally save, BUT we should recompute them in the 2nd fissioned loop part directly
    - if we fission on LLVM IR then we can:
        - estimate how many Connex ASM instrs takes 1 LLVM vector IR instr
        - insert in 1st part save to LS mem instructions and in
            the 2nd part load from LS mem instructions
Instruction aNewInstr(_IJMPNZ, -1, -1, -1);

// idxInstr2
instructions.insert(instructions.begin() + idxInstr2, aNewInstr.getValue());
#endif
                    }

                    idxInstr = idxInstr2 + 1;
                    break;
                }
            }
        }

    }
    printf("Exiting Kernel::makeLoopsValid()\n");
} // END Kernel::makeLoopsValid()


//#define instructionsSimple (instructions.begin() + offsetKernelToStartCodegenFrom)
//#define instructionsSimple instructions
// TODO: defining the array with this macro with iterator is kind-of abusive, so try to avoid it
#define instructionsSimple (partlySSAInstrs.begin() - offsetKernelToStartCodegenFrom)

// This is used only during pretty-printing of genLLVMISelManualCode()
string Kernel::GenerateIfRequiredCopyToReg(int &iInstr,
                                           Instruction &instrCrt,
                                         #ifndef CONVERT_PARTLY_SSA_FORM
                                           bool isInstrInsideWhereBlock,
                                         #endif
                                           int &numInstructionsToCodegen,
                                           int &offsetKernelToStartCodegenFrom,
                                           /*
                                           //vector<InstructionType> &instructionsSimple,

                                           A bit complicated but it works (adjust
                                            accordingly if something changes in the
                                            future */
                                           //Instruction *instructionsSimple,
                                           vector<Instruction> &partlySSAInstrs,
                                           string &varName,
                                           int *countInstr,
                                        #ifdef NEW2018_08_10
                                           string *virtRegVarNameIdRegDef
                                        #else
                                           int *virtRegVarNameIdRegDef
                                        #endif
                                           ) {

    #define ID_GET_COPYTOREG ((1UL << OPCODE_9BITS_SIZE) + 1)
    #define ID_VIRTREG ((1UL << OPCODE_9BITS_SIZE) + 2)


    /*
    printf("countInstr = %p in GenerateIfRequiredCopyToReg()\n", countInstr);
    printf("virtRegVarNameIdRegDef = %p in GenerateIfRequiredCopyToReg()\n",
           virtRegVarNameIdRegDef);
    */
    stringstream ss2;


  #ifndef CONVERT_PARTLY_SSA_FORM
    bool isInstrAfterInsideWhereBlock = false;
    bool reachesDefinition = true;

   /* IMPORTANT: We recommend to enable this macro
    *  - this macro makes the treatment of the _SPECIAL_H instructions
    *  in the Kernel::genLLVMISelManualCode() method
    *  to use for the tied-to constraint the most
     recent definition of a register, even if it is in a WHERE block, instead
     of using the FIRST definition encountered of a register that is still
     reaching (even on certain lanes due to predicated instructions between
     the 1st and instrCrt).
   */
   #define USE_THE_MOST_RECENT_DEFINITION_OF_A_REGISTER_FOR_TIED_TO_CONSTRAINTS
   #ifdef USE_THE_MOST_RECENT_DEFINITION_OF_A_REGISTER_FOR_TIED_TO_CONSTRAINTS
    if (instrCrt.getDest() != NO_REG_INDEX)
        virtRegVarNameIdRegDef[instrCrt.getDest()] = varName;
   #endif

    if (isInstrInsideWhereBlock) {
      #ifndef USE_THE_MOST_RECENT_DEFINITION_OF_A_REGISTER_FOR_TIED_TO_CONSTRAINTS
        if (virtRegVarNameIdRegDef[instrCrt.getDest()].empty())
            virtRegVarNameIdRegDef[instrCrt.getDest()] = varName;
      #endif
        return ss2.str();
    }
  #endif

    /* We codegen a getCopyToReg() if we have after instrCrt a def to the
       same register as the dest register of instrCrt. */
    for (int iInstrAfter = iInstr + 1;
          iInstrAfter < offsetKernelToStartCodegenFrom + numInstructionsToCodegen;
          iInstrAfter++) {

        Instruction instrAfter(instructionsSimple[iInstrAfter]);

    #ifndef CONVERT_PARTLY_SSA_FORM
        int instrAfterOpcode = instrAfter.getOpcode();
        if (instrAfterOpcode == _WHERE_CRY ||
              instrAfterOpcode == _WHERE_EQ ||
              instrAfterOpcode == _WHERE_LT)
            isInstrAfterInsideWhereBlock = true;
        else
        if (instrAfterOpcode == _END_WHERE)
            isInstrAfterInsideWhereBlock = false;

        if (instrAfter.getDest() != NO_REG_INDEX &&
                instrCrt.getDest() == instrAfter.getDest() &&
                isInstrAfterInsideWhereBlock == false)
            reachesDefinition = false;
    #endif

        if (instrAfter.getDest() != NO_REG_INDEX &&
                instrCrt.getDest() == instrAfter.getDest()
              #ifndef CONVERT_PARTLY_SSA_FORM
                &&
                isInstrAfterInsideWhereBlock == true &&
                reachesDefinition
              #endif
                ) {

            printf("  WAW (output) dependency: Since instrAfter's dest reg is assigned also before " \
                     "(more than once): we create CopyToReg for use for _SPECIAL_H SDNode " \
                     "(which has tied-to operands constraint)!\n");
            printf("    iInstrAfter = %d\n", iInstrAfter);
            printf("    instrAfter = %s", instrAfter.dump().c_str());
            printf("    (doing it only once for instrCrt)\n");
            //printf("  instrCrt.getDest() = %d\n", instrCrt.getDest());

            // NOTE: we copy to register for use for _SPECIAL_H SDNode
            int idInstrVR = countInstr[ID_VIRTREG];
            countInstr[ID_VIRTREG]++;
            string varNameVR = "virtReg" + to_string(idInstrVR);

          #ifdef NEW2018_08_10
            virtRegVarNameIdRegDef[instrCrt.getDest()] = varName;
          #else
            virtRegVarNameIdRegDef[instrCrt.getDest()] = idInstrVR;
          #endif

            int idInstrCTR = countInstr[ID_GET_COPYTOREG];
            countInstr[ID_GET_COPYTOREG]++;
            string varNameCTR = "copyToReg" + to_string(idInstrCTR);


            /*
             * VERY IMPORTANT:
             *  From http://llvm.org/docs/doxygen/html/classllvm_1_1SelectionDAG.html:
             *   SDValue getCopyToReg (SDValue Chain, SDLoc dl, unsigned Reg, SDValue N, SDValue Glue)
             */

            // MAYBE check also if the instruction has input the dest reg
            //      "                   SDValue(" << varName << ", " << numInputsInstr[indexInstr] << "),\n"
            ss2 << "unsigned " << varNameVR
                << " = RegInfo->createVirtualRegister(&Connex::MSA128HRegClass);\n";
            ss2 << "SDValue " << varNameCTR << " = CrtDAG->getCopyToReg(\n" \
                   "                   // VERY IMPORTANT: Chain input edge (it seems it works to put a glue edge also)\n";

            if (useGlue == 0) {
                ss2 <<
                      "                   SDValue(" << varName << ", 1),\n";
            }
            else {
                ss2 <<
                      "                   CrtDAG->getEntryNode(),\n";
            }

            ss2 <<    "                   DL,\n" \
                      "                   " << varNameVR << ",\n" \
                      "                   // Value copied to register\n" \
                      "                   SDValue(" << varName << ", 0)"
                <<
                     ((useGlue == 1) ? ",\n" : "\n");

            if (useGlue == 0) {
                ss2 << "                   // VERY IMPORTANT: Glue - NONE\n";
            }
            else {
                ss2 <<
                      "                   // VERY IMPORTANT: Glue input edge\n" \
                      "                   SDValue(" << varName << ", 1)\n";
                /*
                ss2 << "                   // Glue\n" \
                    << "                   // \n";
                */
            }

            ss2 <<    "                  );\n\n";

            /* IMPORTANT NOTE: CopyToReg SDNode does NOT have chain or
                 glue output edges.
            predVarName = varNameCTR; */
            break;
        }
    }

  #ifdef NEW2018_08_10
    return string(""); //std::to_string(0);
  #else
    return ss2.str();
  #endif
} // END GenerateIfRequiredCopyToReg()


/* We return/update in partlySSAInstrs the instructions, with register renaming
 *   following an SSA convention, except for the reassignments inside WHERE
 *   blocks that are difficult to be changed into SSA, so we keep redefinitions.
 */
void Kernel::ConvertInPartlySSAForm(vector<Instruction> &partlySSAInstrs,
                                    bool *isDefined, bool *isDefinedOverall) {
    int i;
    int iInstr;
    /* isDefined has the elements set to true for the input registers of the
     *   kernel.
     *     We use isDefined2 to update the registers defined while we iterate
     *       over the part of the kernel for codegen.
     *     We don't use isDefined because it is being used in
     *       genLLVMISelManualCode().
     */
    bool isDefined2[CONNEX_REG_COUNT];

    for (i = 0; i < CONNEX_REG_COUNT; i++) {
        isDefined2[i] = isDefined[i];
    }

    printf("Entered Kernel::ConvertInPartlySSAForm()\n");
    printf("offsetKernelToStartCodegenFrom = %d\n",
            offsetKernelToStartCodegenFrom);
    printf("numInstructionsToCodegen = %d\n",
            numInstructionsToCodegen);

    /* First we initialize the isDefinedOverall for the given kernel:
     *    isDefinedOverall[i] = true iff if there is an instruction defining
     *                          register index i in the part of the kernel
     *                          for which we do codegen.
     */
    for (iInstr = offsetKernelToStartCodegenFrom;
            iInstr < offsetKernelToStartCodegenFrom + numInstructionsToCodegen;
            iInstr++) {
        Instruction instr(instructions[iInstr]);
        if (instr.getDest() != NO_REG_INDEX) {
            isDefinedOverall[instr.getDest()] = true;
        }
    }

    //printf("isDefinedOverall[23] = %d\n", isDefinedOverall[23]);

    bool isInWhereBlock = false;
    printf("Automatically generating 'partly SSA form'...\n");

    /* Constructing vector partlySSAInstrs - we need to mutate it and it's the
         only way since instructions is a vector<InstructionType> whose
         elements we use to build each time normally an Instruction.
       Note that we put in it only the instructions we need to codegen
          (from offsetKernelToStartCodegenFrom onwards).
    */
    for (iInstr = offsetKernelToStartCodegenFrom;
            iInstr < offsetKernelToStartCodegenFrom + numInstructionsToCodegen;
            iInstr++) {
        Instruction instrCrt(instructions[iInstr]);
        partlySSAInstrs.push_back(instrCrt);
    }
    assert(partlySSAInstrs.size() == numInstructionsToCodegen);

    /*
    Doing the actual renaming transformation
    The algorithm we use is:
        if NOT(instr is in WHERE block)
           if instr has dest reg
                if isDefined2(instr.getDest())
                    take new register name
                    replace subsequent (until next def of same register outside 
                        WHERE block) uses with this new reg name
                    update isDefined2 for new register name
                else
                    isDefined2[instr.getDest()] = true

    The standard definition of register renaming is a pass that can be
      implemented in a compiler (for loop unrolling - see
      [Patterson_COD2013, page 338], etc) or hardware.
    We have 2 phases of this register renaming scheme of ours (used for the sake
      of being compatible with LLVM's SelectionDAG with virtual registers, which
      use SSA (static single assignment) form):
        - a detection of a reassignment - we detect a false dependence (WAW or WAR)
        - the actual renaming of a register in all subsequent uses (reads) and
          defs (write)

    So we do NOT perform detection of reassignment (phase 1 of register renaming)
      for the destination register of an instruction inside a WHERE block
      (if that register has already been assigned, which normally should be
      the case), but we can alter destination registers of instructions inside
      WHERE blocks if the register renaming comes from an instruction outside a
      WHERE block.

    Example of how register renaming works:
      // This is outside WHERE blocks
      Rdst = ...; // This is 1st def of Rdst.
      // This is also outside WHERE blocks
      Rdst = ...; // We need to perform register renaming to Rdst2

      ....
      WHERE
         Rdst = ...; // We do NOT do a new renaming: we change Rdst to Rdst2
         ... = ...Rdst; // Rdst -> Rdst2
      END_WHERE;

      ... = ...Rdst; // Rdst -> Rdst2
      ....
      Rdst = ...; // Rdst -> Rdst3
      ....
      Rdst = ...; // Rdst -> Rdst4
      ....
      WHERE
         Rdst = ...; // Rdst -> Rdst4
         ... = ...Rdst; // Rdst -> Rdst4
      END_WHERE;
      ...
      Rdst = ...; // Rdst -> Rdst5
      ....
      // END of kernel
    */
    for (iInstr = offsetKernelToStartCodegenFrom;
            iInstr < offsetKernelToStartCodegenFrom + numInstructionsToCodegen;
            iInstr++) {
        //Instruction instrCrt(instructions[iInstr]);

        int indexInstr = iInstr - offsetKernelToStartCodegenFrom;

        #define instrCrt partlySSAInstrs[indexInstr]

        cout << "indexInstr = " << indexInstr
             << ": instrCrt.dump() = " << instrCrt.dump();
        Instruction instrOrig(instructions[iInstr]);
        cout << "  orig instruction = "
             << instrOrig.dump();
        fflush(stdout);

        int instrCrtOpcode = instrCrt.getOpcode();

        if ((instrCrtOpcode == _WHERE_CRY) ||
            (instrCrtOpcode == _WHERE_EQ) ||
            (instrCrtOpcode == _WHERE_LT)) {
            isInWhereBlock = true;
        }
        else
        if (instrCrtOpcode == _END_WHERE)
            isInWhereBlock = false;

        if (instrCrt.getDest() != NO_REG_INDEX) {
            int instrCrtDest = instrCrt.getDest();
            if (isDefined2[instrCrtDest]) {
                if (isInWhereBlock) {
                    printf("For 'partly SSA form': we do NOT change dest reg "
                            "since it must update (as long as the predicate of "
                            "WHERE has some true values) the register\n");

                }
                else {
                    // We find an unused register for the destination register
                    for (i = 0; i < CONNEX_REG_COUNT; i++) {
                        if (isDefinedOverall[i] == false) {
                            isDefinedOverall[i] = true;
                            isDefined2[i] = true;
                            break;
                        }
                    }
                    assert(i < CONNEX_REG_COUNT);

                    printf("For 'partly SSA form': changing dest reg from %d to %d\n",
                           instrCrtDest, i);
                    // We rename the destination register to i
                    instrCrt.setDest(i);
                    //cout << "   -->  instrCrt.dump() = " << instrCrt.dump();

                    /* Updating the register renamed for ALL following
                         instructions of instrCrt, called instrAfter.

                       IMPORTANT NOTE: it is OK to do this for ALL instrAfter
                         instructions (including the ones in WHERE block) because what we
                         want to avoid is the ISel pass in llc to assign a
                         different register than the "normal" one to an
                         instruction in a WHERE block, which would happen if we
                         change the destination register of an instruction
                         inside a WHERE block w.r.t. its previous definition
                         outside (before) the WHERE block thus changing program
                         semantics.

                       Do NOT mistake: register renaming done here with the
                         register allocation performed by the back end (llc)
                         of LLVM.
                    */
                    for (int indexInstrAfter = indexInstr + 1;
                            indexInstrAfter < numInstructionsToCodegen;
                            indexInstrAfter++) {
                        //Instruction instrAfter(instructions[iInstrAfter]);
                        #define instrAfter partlySSAInstrs[indexInstrAfter]


                        /* We apply the initiated register renaming for all
                             stmts instrAfter.
                        */
                        int instrAfterOpcode = instrAfter.getOpcode();

                        if (instrAfter.getLeft() == instrCrtDest)
                            instrAfter.setLeft(i);

                        //if (instrAfter.getType() == INSTRUCTION_TYPE_NO_IMMEDIATE &&
                        if (instrAfter.getRight() != NO_REG_INDEX &&
                                instrAfter.getRight() == instrCrtDest) {
                            instrAfter.setRight(i);
                        }

                        if (instrAfter.getDest() != NO_REG_INDEX &&
                                instrAfter.getDest() == instrCrtDest) {
                            instrAfter.setDest(i);
                        }
                        #undef instrAfter
                    }
                }
            }
            else {
                isDefined2[instrCrtDest] = true;
            }
        }

        #undef instrCrt
        //partlySSAInstrs.push_back(instrCrt);
    }

    assert(partlySSAInstrs.size() == numInstructionsToCodegen);
    printf("The automatically generated 'partly SSA form' is:\n");
    for (iInstr = 0; iInstr < numInstructionsToCodegen; iInstr++) {
        cout << "iInstr = " << iInstr
             << ": instr.dump() = " << partlySSAInstrs[iInstr].dump();
    }

    // Cleaning up after autogen 'partly SSA form'
    /*
    // No need to cleanup isDefined since we use above isDefined2
    //
    //
    printf("Cleaning up after autogen 'partly SSA form'\n");
    for (i = 0; i < CONNEX_REG_COUNT; i++) {
        if (sdNodeVarNameRegDef[i].empty() == true) {
            isDefined[i] = false;
        }
        else {
            printf("sdNodeVarNameRegDef[%d] = %s --> initializing isDefined\n",
                    i, sdNodeVarNameRegDef[i].c_str());

            // We initialize as isDefined the input registers to this kernel
            isDefined[i] = true;
        }
    }
    */
} // END Kernel::ConvertInPartlySSAForm()



/*
   We generate C++ (manual) ISel code for an intrinsic.
   IMPORTANT: We advise the kernel to have registers allocated from last one
       (register index CONNEX_REG_COUNT - 1) downwards.

   We require to have ASM code in "partly SSA-form" i.e., every register
          SHOULD be assigned only once (unless it is inside a WHERE block in
          order not to mess up the predication).
     If a (destination) register is assigned a second time or more:
       - if it is inside a WHERE block we leave it like that because
           it means we have a special constraint and for this we use
           MachineSDNodes that are defined (in TableGen) with tied-to operand
           constraints.
       - else, we assign an unused register (from the register file) for the
          destination register.

   NOTE: we do not use any explicit symbolic operands in our Connex ASM code.

   Note: We do not rely on LLVM's predication, which seems it is not working in
       the Instruction Selection (ISel) pass, where it would have been the most
       convenient to use. (However, we could use PredicateInstruction() method
       to predicate MachineInstr after scheduling, but we do not bother).
   We do not use bundles, because it is a bit more complex than the current
     solution - we would have to bundle instructions after ISel.


   Some important rules:
    Rule #1 is:
      To be selected nodes (except CopyToReg at least that does not have output
         chain or glue edges), need to be "chained" in the graph in the sense that
         they need to have a successor.

    Rule #3:
     To avoid using Bundles (or any form of predication that could help
        instruction selection and scheduling that seems to be lacking for LLVM)
      I "integrated" in the dataflow the WHERE and END_WHERE instructions that
      need to propagate (they have inputs and outputs that have tied-to registers)
      some values.
       IMPORTANT: they really need to feed the output registers to the
         following nodes, otherwise the "REWRITING TWO-ADDR INSTRS" pass
         will generate for them COPY instructions to copy registers and the
         RegisterCoalescer.cpp ("SIMPLE REGISTER COALESCING") pass will not remove these basically
         useless instructions). (although WHEREEQ does NOT normally continue propagating
            its result to following nodes since the value is normally NOT used anymore
            by no one, END_WHERE really needs to propagate it for the sake
            of generating 1 COPY instruction less because the value it takes
            as input and output is further used).
    It seems a bit complicated, but to be part of the data flow WHERE and
       END_WHERE need to have at least 1 input and 1 output register and I
       obviously had to tie the registers for these input and output.

    Rule #4:
       From http://llvm.org/docs/CodeGenerator.html#introduction-to-selectiondags:
       << All nodes that have side effects should take a token chain as input and produce a new one as output.
         By convention, token chain inputs are always operand #0, and chain results are always the last value produced by an operation.
         However, after instruction selection, the machine nodes have their chain after the instruction’s operands, and may be followed by glue nodes.>>
       This means:
         - getMachineNode() takes as inputs first the instruction operands and
             then the eventual chain and glue inputs

    Rule #5:
         - for MachineSDNode msdn the SDValue(msdn, i) represents all the
           possible ports of the MachineSDNode. These follow the following rule:
            - first comes the result(s) and the chain and glue return values
            - then come the inputs.

  Done: In case a _SPECIAL instr inside a WHERE block does not
    have an UNpredicated predecessor instruction we give a warning
    ("Warning: instrCrt.getDest() = ... register not initialized before updated in WHERE")
    and use a MachineSDNode without tied-to constraint.
*/
string Kernel::genLLVMISelManualCode() {
    stringstream ss;
    //string res;
    int i;
    bool instrCrtInWhereBlock = false;
    bool instrCrtInRepeatBlock = false; // 2020_04_23
    bool instrCrtInLaneGatingRegion = false;
    int numInstrsInWhereBlock = 0;
    int numInstrsInRepeatBlock = 0; // 2020_04_23
    int numWhereInstrs = 0;
    int numEndWhereInstrs = 0;

    assert(numInstructionsToCodegen != -1 &&
           "You need to initialize numInstructionsToCodegen");
    assert(offsetKernelToStartCodegenFrom != -1 &&
           "You need to initialize offsetKernelToStartCodegenFrom");

    if (useLaneGatingOnConnex)
        offsetKernelToStartCodegenFrom++; // We account for the ENABLE_ALL_CELLS instruction added automatically at the beginning of the Opincaa kernel

    ss << "// Code auto-generated by method Kernel::genLLVMISelManualCode()\n";
    //cout << "ss = " << ss.str();
    ss << "//   from the OPINCAA lib, from kernel " << getName() << ".\n";

  #ifdef NEW2018_08_10
    ss << "// You should include this code in the Select() method of the [Target]SelectionDAGISel\n"
       << "//   class of your back end (or MAYBE in the ISelLowering pass).\n";
  #else
    ss << "// It is important to put this code in the Select() method of the\n"
       << "//   SelectionDAGISel class of your back end, after the ISelLowering pass,\n"
       << "//   which contains the DAG Combiner, because the DAG Combiner can remove\n"
       << "//   the getCopyToReg() we create, which can lead to the following llc error:\n"
       << "//   <<Register use before def!>> assertion failed.\n";
  #endif

    ss << "// Number of instructions generated: "
       << numInstructionsToCodegen << ".\n\n";

    //std::vector<bool>
    /*
    std::unordered_map<std::string, int> symTab;
       From
     http://www.cplusplus.com/reference/unordered_map/unordered_map/count/:
     symTab.count("blabla"); */

    #define NUM_SDNODE_OPS ((1UL << OPCODE_9BITS_SIZE) + 100)
    #define ID_GET_CONSTANT ((1UL << OPCODE_9BITS_SIZE) + 0)

    int countInstr[NUM_SDNODE_OPS];
    bool isDefined[CONNEX_REG_COUNT];
    for (i = 0; i < CONNEX_REG_COUNT; i++) {
        isDefined[i] = false;
    }
    bool isDefinedOverall[CONNEX_REG_COUNT];
    for (i = 0; i < CONNEX_REG_COUNT; i++) {
        isDefinedOverall[i] = false;
    }

    for (i = 0; i < NUM_SDNODE_OPS; i++) {
        countInstr[i] = 0;
    }

    /*
    #define BUFFER_LEN 32768
    static char buffer[BUFFER_LEN];
    //std::stringbuf *pbuf = ss.rdbuf();
    // From http://www.cplusplus.com/reference/streambuf/streambuf/pubsetbuf/
    ss.rdbuf()->pubsetbuf(buffer, BUFFER_LEN);
    //ss.rdbuf()->pubseekpos(0);
    // From https://stackoverflow.com/questions/1494182/setting-the-internal-buffer-used-by-a-standard-stream-pubsetbuf
    //ostreambuf<char> ostreamBuffer(buffer, size);
    //std::ostream messageStream(&ostreamBuffer);
    */

    /* IMPORTANT: the kernel auto-generated here for ISel takes
       input SDNodes
       (with names like "nodeOpSrcCast1", "nodeOpSrcCast2", or "nodeOpSrcCast",
       etc) that have as result the <CONNEX_VECTOR_LENGTH x i16> values to be
       used for the arithmetic operator intrinsic we want to select. */

    /* Used to "update" a virtual register.
       More exactly, we use this to map a register index to an Id of the
         virtual register was created.
         For example, when we generate instr-selection code like:
            SDNode *ldsh0 = CrtDAG->getMachineNode(...);
            unsigned virtReg0 = RegInfo->createVirtualRegister(&Connex::MSA128HRegClass);
            SDValue copyToReg0 = CrtDAG->getCopyToReg(...);
          and later want to use this virtReg0 in:
            SDNode *sub0 = CrtDAG->getMachineNode(
                                    Connex::SUBV_SPECIAL_H,
                                    DL,
                                    TYPE_VECTOR_I16,
                                    MVT::Other,
                                    SDValue(vload0, 0),
                                    SDValue(ldsh0, 0),
                                    CrtDAG->getRegister(virtReg0, TYPE_VECTOR_I16),
                                    SDValue(whereeq0, 1)
                                    );
        we need to remember the id/number of virtReg0 for this SDNode above (sub0) we print.
    */
  #ifdef NEW2018_08_10
    string virtRegVarNameIdRegDef[CONNEX_REG_COUNT];
    for (i = 0; i < CONNEX_REG_COUNT; i++) {
        virtRegVarNameIdRegDef[i] = "";
    }
  #else
    int virtRegVarNameIdRegDef[CONNEX_REG_COUNT];
    for (i = 0; i < CONNEX_REG_COUNT; i++) {
        virtRegVarNameIdRegDef[i] = -1;
    }
  #endif

    /* In the code doing ISel for this piece of kernel,
       we define some SDNodes manually which handle the input vector registers
       for this piece of kernel - the SDNodes use "bogus" instructions like
       NOP_BITCONVERT_HW/WH.
     We keep track of these register and their associated SDNode variable
       names.
    */
    unordered_map<int, string> inputRegistersKernelAndSDNodeVarName;
    string lastInputRegisterKernelSDNodeVarName;

    /* sdNodeVarNameRegDef is the var-name of the SDNode we generate
         that updated last the reg regDef.
       It keeps track of the dataflow between instructions */
    //string sdNodeVarNameRegDef[CONNEX_REG_COUNT];

    #define STR_NOT_INIT "[NOT_INITIALIZED]"
    bool kernelHasInputRegistersSpecified = false;
    for (i = 0; i < CONNEX_REG_COUNT; i++) {
        if (sdNodeVarNameRegDef[i].empty() == true) {
            sdNodeVarNameRegDef[i] = STR_NOT_INIT;
        }
        else {
            printf("sdNodeVarNameRegDef[%d] = %s --> initializing isDefined, etc\n",
                    i, sdNodeVarNameRegDef[i].c_str());

            // We initialize as isDefined the input registers to this kernel
            isDefined[i] = true;

            isDefinedOverall[i] = true;

            //inputRegistersKernel.push_back(i);
            inputRegistersKernelAndSDNodeVarName[i] = sdNodeVarNameRegDef[i];
            lastInputRegisterKernelSDNodeVarName = sdNodeVarNameRegDef[i];

            kernelHasInputRegistersSpecified = true;
        }
    }

    // We check if we specified input registers to this kernel
    assert(kernelHasInputRegistersSpecified == true &&
            "NO input registers specified to the kernel");

    /* Represents the offset in the kernel of the 1st instruction of our region
     *  to codegen: */
    //offsetKernelToStartCodegenFrom = 12;


    int N = numInstructionsToCodegen;
    printf("this->size() = %d\n", this->size());
    printf("  offsetKernelToStartCodegenFrom (%d) + numInstructionsToCodegen (%d) = %d\n",
           offsetKernelToStartCodegenFrom,
           numInstructionsToCodegen,
           offsetKernelToStartCodegenFrom + numInstructionsToCodegen);
    assert(this->size() >= offsetKernelToStartCodegenFrom +
                            numInstructionsToCodegen);


    int iInstr;


    printf("Kernel::genLLVMISelManualCode(): Num instructions = %d\n", N);
    fflush(stdout);

    //cout << "SDNode *ConnexDAGToDAGISel::Select___(SDNode *Node) { //...TO PUT MORE\n";
    //cout << "    SDLoc DL(Node);\n";
    //cout << "// We make the following assumptions: inputs are ...!!!!\n";


  #ifdef CONVERT_PARTLY_SSA_FORM
    vector<Instruction> partlySSAInstrs;
    ConvertInPartlySSAForm(partlySSAInstrs, isDefined, isDefinedOverall);
  #else
    vector<Instruction> partlySSAInstrs;
    for (i = offsetKernelToStartCodegenFrom;
            i < offsetKernelToStartCodegenFrom + numInstructionsToCodegen;
            i++) {
        Instruction instrCrt(instructions[i]);
        partlySSAInstrs.push_back(instrCrt);
    }
    assert(partlySSAInstrs.size() == numInstructionsToCodegen);
  #endif // END CONVERT_PARTLY_SSA_FORM



    // Now we start pretty-printing the code


    InstructionType *predInstr = NULL;
    string predVarName = "[NONE_SO_REPLACE]";
    string varName = "[NONE_SO_REPLACE]";

    // The variable names of the MachineSDNodes
    vector<string> varNameInstr;

    // The num of real inputs (not chain, glue nor constants) of the MachineSDNodes
    vector<int> numInputsInstr;

    // The num of real outputs of the MachineSDNodes
    vector<int> numOutputsInstr;
    vector<bool> requireGlueOrChainPred;


    // Adding CopyToReg, if required, for the inputs of the Opincaa kernel
    string strCopyToReg;
    iInstr = offsetKernelToStartCodegenFrom;
    //int reg = 0;
    //for (auto reg: inputRegistersKernelAndSDNodeVarName.keys())
    for (auto iterMap: inputRegistersKernelAndSDNodeVarName) {
        int reg = (iterMap).first;
        //varName = "nodeOpSrcCast1";
        varName = (iterMap).second; //inputRegistersKernelAndSDNodeVarName[reg];

        /* We create a bogus instruction with destination reg:
            R28 = LS[R0] (the dest reg will be changed) */
        //Instruction instrCrt(0x9200001C);
        Instruction instrCrt(0);
        instrCrt.setType(INSTRUCTION_TYPE_NO_IMMEDIATE);
        instrCrt.setOpcode(_READ);

        printf("reg = %d\n", reg);
        instrCrt.setDest(reg);
        printf("instrCrt.getDest() = %d\n", instrCrt.getDest());
        fflush(stdout);
        assert(instrCrt.getDest() == reg);

        // We pretty-print (quite often...)
        strCopyToReg = GenerateIfRequiredCopyToReg(iInstr,
                                              instrCrt,
                                            #ifndef CONVERT_PARTLY_SSA_FORM
                                              false,
                                            #endif
                                              numInstructionsToCodegen,
                                              offsetKernelToStartCodegenFrom,

                                              //instructionsSimple.data(),
                                              //&(instructionsSimple[0]),
                                              partlySSAInstrs,
                                              varName, // Should contain [NONE_TO_REPLACE]
                                              countInstr,
                                              virtRegVarNameIdRegDef);
        ss << strCopyToReg << "\n";
    }
    ss << "\n";


    /* We now generate the LLVM MachineSDNode instruction for the current
          instruction, instrCtr.

       Note: if the consumer of dest register set by current
       instruction is the next instruction, don't glue (or chain) to feed it
       to the next node - anyhow the order is enforced by the data dependence.
       */
    //for (iInstr = 0; iInstr < N; iInstr++)
    for (iInstr = offsetKernelToStartCodegenFrom;
            iInstr < offsetKernelToStartCodegenFrom + numInstructionsToCodegen;
            iInstr++) {
        int indexInstrCrt = iInstr - offsetKernelToStartCodegenFrom;

        Instruction instrCrt(instructionsSimple[iInstr]);

        string glueOrChainStr = "";
        string inputsStr = "";
        string isdName;

        /*
        #define UPDATE_GLUESTR glueOrChainStr = "SDValue(" + predVarName + ", " + \
                                  to_string(indexInstr >= 1 ? numInputsInstr[indexInstr - 1] : -1) + \
                                  ")";
                                                                    to_string(numInputsInstr[indexInstr - 1] + numOutputsInstr[indexInstr - 1] + 1) + ")" ) : \
        */
        /* We also chain (glue) SDNode nodeOpSrcCast2
         with the first SDNode generated
         automatically - because the result of
        nodeOpSrcCast2 is read normally later.*/
        #define UPDATE_GLUESTR if (indexInstrCrt >= 1 && \
                                   requireGlueOrChainPred[indexInstrCrt - 1]) \
                                      glueOrChainStr = "SDValue(" + \
                                               predVarName + ", " + to_string(numOutputsInstr[indexInstrCrt - 1]) + ")"; \
                               else if (indexInstrCrt == 0) \
                                    glueOrChainStr = "SDValue(" + lastInputRegisterKernelSDNodeVarName + ", 1)"; \
                               else glueOrChainStr = "";

        #define UPDATE_INPUTSSTR_1_INPUT_OPERAND inputsStr = "SDValue(" + sdNodeVarNameRegDef[instrCrt.getLeft()] + ", 0)";

        #define UPDATE_INPUTSSTR_2_INPUT_OPERANDS inputsStr = "SDValue(" + sdNodeVarNameRegDef[instrCrt.getLeft()] + ", 0),\n" + \
                      "                                    SDValue(" + sdNodeVarNameRegDef[instrCrt.getRight()] + ", 0)";
        /*
           I use this macrodef because the ASM code that results from llc reads
              nicer like this.
           But beware of using it wrong - wrong semantics (exchanging operands
           does not work for non-commutative operators like SUB, etc)
        */
        #define UPDATE_INPUTSSTR_2_INPUT_OPERANDS_MIRRORED inputsStr = "SDValue(" + sdNodeVarNameRegDef[instrCrt.getRight()] + ", 0),\n" + \
                        "                                    SDValue(" + sdNodeVarNameRegDef[instrCrt.getLeft()] + ", 0)";

        /* DONE DONE (maybe TODO?) DONE: We have issues in MUL_i32 with the wrong usage of
          registers... - I have to put more constraints between registers, not only related to updates as I currently did... */

        cout << "indexInstrCrt = " << indexInstrCrt << ": instrCrt.dump() = "
             << instrCrt.dump(); // << "\n";
        fflush(stdout);

        int instrCrtOpcode = instrCrt.getOpcode();

        cout << "  instrCrt.getDest() = " << instrCrt.getDest() << "\n";
        cout << "  instrCrt.getLeft() = " << instrCrt.getLeft() << "\n";
        fflush(stdout);
      #if 0
        if (isInstrWithDest(instrCrtOpcode)) {
            // We test that our kernel is in "SSA" form
            assert(isDefined[instrCrt.getDest()] == false);
        }
      #endif

        int idInstrCrt = countInstr[instrCrtOpcode];
        countInstr[instrCrtOpcode]++;

        /* if (instrCrtInWhereBlock)
            numInstrsInWhereBlock++;
        */

        switch (instrCrtOpcode) {
            case _VLOAD: {
                int idGetConstant = countInstr[ID_GET_CONSTANT];
                //printf("idGetConstant = %d\n", idGetConstant);
                //fflush(stdout);

                countInstr[ID_GET_CONSTANT]++;
                string ctVarName = "ct" + to_string(idGetConstant);
                ss << "SDValue " << ctVarName
                   << " = CrtDAG->getSignedConstant(" << (int)instrCrt.getValue()
                   << ", DL, MVT::i16, true, false);\n";

                varName = "vload" + to_string(idInstrCrt);

                isdName = "VLOAD_H";
                inputsStr = ctVarName;

                UPDATE_GLUESTR;

                numInputsInstr.push_back(1);
                numOutputsInstr.push_back(1);

                break;
            }
            case _XOR:
            case _AND:
            case _OR:
            case _SHL:
            case _SHR:
            case _SHRA:
            case _ADD:
            case _ADDC:
            case _SUB:
            case _SUBC: {
            // We could put these here also but we might treat them separately in the future
            //case _EQ:
            //case _LT:
            //case _ULT:
                string varNamePrefix;
                if (instrCrtOpcode == _XOR) {
                    isdName = "XORV_H";
                    varNamePrefix = "xor";
                }
                else
                if (instrCrtOpcode == _AND) {
                    isdName = "ANDV_H";
                    varNamePrefix = "and";
                }
                else
                if (instrCrtOpcode == _OR) {
                    isdName = "ORV_H";
                    varNamePrefix = "or";
                }
                else
                if (instrCrtOpcode == _SHL) {
                    isdName = "SHLV_H";
                    varNamePrefix = "shl";
                }
                else
                if (instrCrtOpcode == _SHR) {
                    isdName = "SHRV_H";
                    varNamePrefix = "shr";
                }
                else
                if (instrCrtOpcode == _SHRA) {
                    isdName = "SHRAV_H";
                    varNamePrefix = "shra";
                }
                if (instrCrtOpcode == _ADD) {
                    isdName = "ADDV_H";
                    varNamePrefix = "add";
                }
                else
                if (instrCrtOpcode == _ADDC) {
                    isdName = "ADDCV_H";
                    varNamePrefix = "addc";
                }
                else
                if (instrCrtOpcode == _SUB) {
                    isdName = "SUBV_H";
                    varNamePrefix = "sub";
                }
                else
                if (instrCrtOpcode == _SUBC) {
                    isdName = "SUBCV_H";
                    varNamePrefix = "subc";
                }


                varName = varNamePrefix + to_string(idInstrCrt);

                bool specialTreatmentTakingResultOfPredecessorInstr = false;

                if (instrCrtOpcode == _XOR ||
                        instrCrtOpcode == _AND ||
                        instrCrtOpcode == _OR ||
                        instrCrtOpcode == _ADD
                        //|| instrCrtOpcode == _ADDC
                   ) {
                    UPDATE_INPUTSSTR_2_INPUT_OPERANDS_MIRRORED;
                }
                else
                if (instrCrtOpcode == _ADDC ||
                    instrCrtOpcode == _SUBC) {
                    // NOTE: this covers also the SUBCV_SPECIAL_H and ADDCV_SPECIAL_H cases
                    assert(indexInstrCrt > 0);

                    specialTreatmentTakingResultOfPredecessorInstr = true;

                    UPDATE_INPUTSSTR_2_INPUT_OPERANDS;
                    //inputsStr = "SDValue(" + sdNodeVarNameRegDef[instrCrt.getLeft()] + ", 0),\n" + \
                    //  "                                    SDValue(" + sdNodeVarNameRegDef[instrCrt.getRight()] + ", 0)";

                    /* Keep in mind ADDCV/SUBCV[_SPECIAL]_H take a bogus input
                     from the previous instruction to avoid the scheduler
                     changing their relative order.
                     Also, to avoid other instructions be scheduled before
                       ADDCV/SUBCV (which could mess up the Carry flags) we
                       SHOULD put a glue edge betwen this instruction and its
                       predecesor. */
                    int iInstrPred = iInstr - 1;
                    Instruction instrPred(instructionsSimple[iInstrPred]);
                    //
                    int instrPredDestReg = instrPred.getDest();
                    assert(instrPredDestReg != NO_REG_INDEX && "ADDC(SUBC) should have as predecessor ADD(SUB)");
                    inputsStr = inputsStr + ",\n" +
                        "                                    SDValue(" + sdNodeVarNameRegDef[instrPredDestReg] + ", 0)";
                }
                else {
                    cout << "  We are in the normal case.\n";
                    UPDATE_INPUTSSTR_2_INPUT_OPERANDS;
                }

                if (specialTreatmentTakingResultOfPredecessorInstr == false) {
                    /* //instrCrtOpcode != _ADDC && instrCrtOpcode != _SUBC)
                     ADDC and SUBC take as input the result of the previous
                     instruction */
                    UPDATE_GLUESTR;
                }

                numInputsInstr.push_back(2);
                numOutputsInstr.push_back(1);

                break;
            }
            case _ISHL:
            case _ISHR:
            case _ISHRA: {
                string varNamePrefix;
                if (instrCrtOpcode == _ISHL) {
                    isdName = "ISHLV_H";
                    varNamePrefix = "ishl";
                }
                else
                if (instrCrtOpcode == _ISHR) {
                    isdName = "ISHRV_H";
                    varNamePrefix = "ishr";
                }
                else
                if (instrCrtOpcode == _ISHRA) {
                    isdName = "ISHRAV_H";
                    varNamePrefix = "ishra";
                }

                UPDATE_INPUTSSTR_1_INPUT_OPERAND;

                int idGetConstant = countInstr[ID_GET_CONSTANT];
                countInstr[ID_GET_CONSTANT]++;
                string ctVarName = "ct" + to_string(idGetConstant);
                ss <<
                      "SDValue " << ctVarName
                      //<< " = CrtDAG->getConstant(" << instrCrt.getRight()
                      << " = CrtDAG->getSignedConstant(" << (int)instrCrt.getValue()
                      << ", DL, MVT::i16, true, false);\n";

                varName = varNamePrefix + to_string(idInstrCrt);

                inputsStr = inputsStr + ",\n" +
                                "                                    " +
                                ctVarName;

                UPDATE_GLUESTR;

                numInputsInstr.push_back(2);
                numOutputsInstr.push_back(1);

                break;
            }
            case _MULT_LO:
            //case _MULT_HI_U:
            case _MULT_HI: {
                string varNamePrefix;
                if (instrCrtOpcode == _MULT_LO) {
                    isdName = "MULTLO_H";
                    varNamePrefix = "multlo";
                }
                else
                if (instrCrtOpcode == _MULT_HI) {
                    isdName = "MULTHI_H";
                    varNamePrefix = "multhi";
                }
                /*
                else
                if (instrCrtOpcode == _MULT_HI_U) {
                    isdName = "MULTHI_H_U";
                    varNamePrefix = "multhi_u";
                }
                */

                varName = varNamePrefix + to_string(idInstrCrt);

                inputsStr = "";
                UPDATE_GLUESTR;

                numInputsInstr.push_back(0);
                numOutputsInstr.push_back(1);

                break;
            }
            case _LDSH:
            case _LDIX: {
                string varNamePrefix;
                if (instrCrtOpcode == _LDSH) {
                    isdName = "LDSH_H";
                    varNamePrefix = "ldsh";
                }
                else
                if (instrCrtOpcode == _LDIX) {
                    isdName = "LDIX_H";
                    varNamePrefix = "ldix";
                }

                varName = varNamePrefix + to_string(idInstrCrt);

                inputsStr = "";
                UPDATE_GLUESTR;

                numInputsInstr.push_back(0);
                numOutputsInstr.push_back(1);

                break;
            }
            case _REDUCE: {
                isdName = "RED_H";

                varName = "sumRed" + to_string(idInstrCrt);

                UPDATE_INPUTSSTR_1_INPUT_OPERAND;
                UPDATE_GLUESTR;

                numInputsInstr.push_back(1);
                numOutputsInstr.push_back(0);

                break;
            }
            case _REDUCE_U: {
                isdName = "RED_U_H";

                varName = "sumRedU" + to_string(idInstrCrt);

                UPDATE_INPUTSSTR_1_INPUT_OPERAND;
                UPDATE_GLUESTR;

                numInputsInstr.push_back(1);
                numOutputsInstr.push_back(0);

                break;
            }
            case _MULT: {
                isdName = "MULT_H";

                varName = "mult" + to_string(idInstrCrt);

                UPDATE_INPUTSSTR_2_INPUT_OPERANDS;
                UPDATE_GLUESTR;

                numInputsInstr.push_back(2);
                numOutputsInstr.push_back(0);

                break;
            }
            case _MULT_U: {
                isdName = "MULT_U_H";

                varName = "mult_u" + to_string(idInstrCrt);

                UPDATE_INPUTSSTR_2_INPUT_OPERANDS;
                UPDATE_GLUESTR;

                numInputsInstr.push_back(2);
                numOutputsInstr.push_back(0);

                break;
            }
            case _CELL_SHL:
            case _CELL_SHR: {
                string varNamePrefix;
                if (instrCrtOpcode == _CELL_SHR) {
                    isdName = "CELLSHR_H";
                    varNamePrefix = "cellshr";
                }
                else
                if (instrCrtOpcode == _CELL_SHL) {
                    isdName = "CELLSHL_H";
                    varNamePrefix = "cellshl";
                }

                varName = varNamePrefix + to_string(idInstrCrt);

                UPDATE_INPUTSSTR_2_INPUT_OPERANDS;
                UPDATE_GLUESTR;

                numInputsInstr.push_back(2);
                numOutputsInstr.push_back(0);

                break;
            }
            case _NOP: {
                isdName = "NOP_BPF";
                varName = "nop" + to_string(idInstrCrt);

                int idGetConstant = countInstr[ID_GET_CONSTANT];
                countInstr[ID_GET_CONSTANT]++;
                string ctVarName = "ct" + to_string(idGetConstant);
                ss <<
                      "SDValue " << ctVarName
                      << " = CrtDAG->getSignedConstant(1 /* Num of cycles to NOP */"
                      << ", DL, MVT::i16, true, false);\n";
                inputsStr = ctVarName;
                UPDATE_GLUESTR;

                numInputsInstr.push_back(0); //1);
                numOutputsInstr.push_back(0);

                break;
            }
            case _EQ:
            case _LT:
            case _ULT: {
                string varNamePrefix;

                if (instrCrtOpcode == _EQ) {
                    isdName = "EQ_H";
                    varNamePrefix = "eq";
                }
                else
                if (instrCrtOpcode == _LT) {
                    isdName = "LT_H";
                    varNamePrefix = "lt";
                }
                else
                if (instrCrtOpcode == _ULT) {
                    isdName = "ULT_H";
                    varNamePrefix = "ult";
                }

                varName = varNamePrefix + to_string(idInstrCrt);

                UPDATE_INPUTSSTR_2_INPUT_OPERANDS;
                UPDATE_GLUESTR;

                numInputsInstr.push_back(2);
                numOutputsInstr.push_back(1);

                break;
            }
            case _SETLC: {
//                assert(0 && "Use instead of instrCrtInWhereBlock var named instrCrtInRepeatBlock");
                instrCrtInRepeatBlock = true;

                isdName = "SETLC_H";
                varName = "setlc" + to_string(idInstrCrt);


                int idGetConstant = countInstr[ID_GET_CONSTANT];
                countInstr[ID_GET_CONSTANT]++;
                string ctVarName = "ct" + to_string(idGetConstant);
                ss << "SDValue " << ctVarName
                   << " = CrtDAG->getSignedConstant("
                   << instrCrt.getValue()
                   << " /* trip count */"
                   << ", DL, MVT::i16, true, false);\n";
                inputsStr = ctVarName;



                UPDATE_GLUESTR;

                numInputsInstr.push_back(0);
                numOutputsInstr.push_back(0);

                break;
            }
            case _IJMPNZ: {
                instrCrtInRepeatBlock = false;

                isdName = "IJMPNZ_H";
                varName = "ijmpnz" + to_string(idInstrCrt);

                UPDATE_GLUESTR;

                numInputsInstr.push_back(0);
                numOutputsInstr.push_back(0);

                break;
            }
            case _WHERE_CRY:
            case _WHERE_EQ:
            case _WHERE_LT: {
                string varNamePrefix;

                instrCrtInWhereBlock = true;
                numInstrsInWhereBlock--;
                numWhereInstrs++;
                if (instrCrtOpcode == _WHERE_CRY) {
                    isdName = "WHERECRY";
                    varNamePrefix = "wherecry";
                }
                else
                if (instrCrtOpcode == _WHERE_EQ) {
                    isdName = "WHEREEQ";
                    varNamePrefix = "whereeq";
                }
                else
                if (instrCrtOpcode == _WHERE_LT) {
                    isdName = "WHERELT";
                    varNamePrefix = "wherelt";
                }

                //cout << "predInstr = " << predInstr->dump();
                //Instruction *producerInstr;
                string *varNameInstrProducer = NULL;

                assert(indexInstrCrt >= 1);

                int portInput;
                if (Instruction(instructionsSimple[iInstr - 1]).getOpcode() == _NOP) {
                    printf("genLLVMISelManualCode(): case _WHERE* - we have _NOP before\n");
                    fflush(stdout);
                    assert(indexInstrCrt >= 2);
                    //producerInstr = &Instruction(instructionsSimple[indexInstrCrt - 2]);
                    varNameInstrProducer = &varNameInstr[indexInstrCrt - 2];
                    portInput = 0;
                }
                else {
                    varNameInstrProducer = &varNameInstr[indexInstrCrt - 1];
                    portInput = 0;
                }

                varName = varNamePrefix + to_string(idInstrCrt);

                /*
                // 2020_12_01: Commenting these lines, since we do NOT want
                //              the WHERE instructions to have intput and
                //              output operands.
                inputsStr = "SDValue(" +
                              *varNameInstrProducer + ", " +
                              to_string(portInput) +
                             ")";
                */
                UPDATE_GLUESTR;

                numInputsInstr.push_back(0); //1);
                // 2020_12_01: numOutputsInstr.push_back(1);
                numOutputsInstr.push_back(0); // 2020_12_01

                break;
            }
            case _END_WHERE: {
                instrCrtInWhereBlock = false;

                numEndWhereInstrs++;

              //#define END_WHERE_HAS_OUTPUT
              #ifdef END_WHERE_HAS_OUTPUT
                isdName = "END_WHERE";
              #else
                //isdName = "END_WHERE_0OPNDS";
                isdName = "END_WHERE";
              #endif

                varName = "endwhere" + to_string(idInstrCrt);

              #ifdef END_WHERE_HAS_OUTPUT
                Instruction instrPred(instructionsSimple[iInstr - 1]);

                /* Asserting the predecessor instr of END_WHERE has a dest
                      register.

                 Note: For an older red.f16 kernel, I had a REDUCE before the
                   END_WHERE, and the assertion was violated.
                   But, so far no (final) kernel violated the assertion.
                   Also, for mult.f16 we have DISABLE_CELL before END_WHERE.
                */
                assert(instrPred.getDest() != NO_REG_INDEX);

                inputsStr = "SDValue(" + predVarName + ", " +
                              to_string(0) + ")";

                /* IMPORTANT: Applying Rule #3:
                   We make END_WHERE propagate its value if used anymore to
                     avoid (if possible) generating a useless COPY instruction,
                     as specified in Rule #3 above.
                */
                sdNodeVarNameRegDef[instrPred.getDest()] = varName;
              #endif

                UPDATE_GLUESTR;

                numInputsInstr.push_back(0); //1);
              #ifdef END_WHERE_HAS_OUTPUT
                numOutputsInstr.push_back(1);
              #else
                numOutputsInstr.push_back(0);
              #endif
                break;
            }
            case _ENABLE_ALL_CELLS:
            case _DISABLE_CELL: {
                if (instrCrtOpcode == _DISABLE_CELL) {
                    isdName = "DISABLE_CELL_H";
                    varName = "disableCell" + to_string(idInstrCrt);
                    instrCrtInLaneGatingRegion = true; // 2018_10_06
                }
                else {
                    isdName = "ENABLE_ALL_CELLS_H";
                    varName = "enableAllCells" + to_string(idInstrCrt);
                    instrCrtInLaneGatingRegion = false; // 2018_10_06
                }
                UPDATE_GLUESTR;

                numInputsInstr.push_back(0);
                numOutputsInstr.push_back(0);

                break;
            }
            // Note: The mem instructions should not be less encountered in kernels
            case _READ:
            case _WRITE:
            case _POPCNT:
            case _NOT: {
                string varNamePrefix;
                if (instrCrtOpcode == _POPCNT) {
                    isdName = "POPCNT_H";
                    varNamePrefix = "popcnt";
                }
                else
                if (instrCrtOpcode == _NOT) {
                    isdName = "NOT_H";
                    varNamePrefix = "not";
                }
                else
                if (instrCrtOpcode == _READ) {
                    isdName = "LD_INDIRECT_H";
                    varNamePrefix = "read";
                }
                else
                if (instrCrtOpcode == _WRITE) {
                    isdName = "ST_INDIRECT_H";
                    varNamePrefix = "write";
                }

                varName = varNamePrefix + to_string(idInstrCrt);

                if (instrCrtOpcode == _READ || instrCrtOpcode == _WRITE) {
                    inputsStr = "SDValue(" + sdNodeVarNameRegDef[instrCrt.getRight()] + ", 0)";
                }
                else
                if (instrCrtOpcode == _POPCNT || instrCrtOpcode == _NOT) {
                    UPDATE_INPUTSSTR_1_INPUT_OPERAND;
                }
                UPDATE_GLUESTR;

                numInputsInstr.push_back(1);
                numOutputsInstr.push_back(1);

                break;
            }
            //
            case _IREAD: {
                int idGetConstant = countInstr[ID_GET_CONSTANT];
                //printf("idGetConstant = %d\n", idGetConstant);
                //fflush(stdout);

                countInstr[ID_GET_CONSTANT]++;
                string ctVarName = "ct" + to_string(idGetConstant);
                ss <<
                      "SDValue " << ctVarName
                      << " = CrtDAG->getSignedConstant(" << (int)instrCrt.getValue()
                      << ", DL, MVT::i16, true, false);\n";

                varName = "iread" + to_string(idInstrCrt);

                isdName = "LD_H";
                inputsStr = ctVarName;

                UPDATE_GLUESTR;

                numInputsInstr.push_back(1);
                numOutputsInstr.push_back(1);

                break;
            }
            case _IWRITE: {
                int idGetConstant = countInstr[ID_GET_CONSTANT];
                //printf("idGetConstant = %d\n", idGetConstant);
                //fflush(stdout);

                countInstr[ID_GET_CONSTANT]++;
                string ctVarName = "ct" + to_string(idGetConstant);
                ss <<
                      "SDValue " << ctVarName
                      << " = CrtDAG->getSignedConstant(" << (int)instrCrt.getValue()
                      << ", DL, MVT::i16, true, false);\n";

                varName = "iwrite" + to_string(idInstrCrt);

                isdName = "ST_H";
                inputsStr = ctVarName;

                UPDATE_GLUESTR;

                numInputsInstr.push_back(1);
                numOutputsInstr.push_back(0);

                break;
            }
            case _PRINT_REG: {
                assert(0 && "NOT IMPLEMENTED");
                break;
            }
            default: {
                isdName = "[DEFAULT]";

                varName = "[default]" + to_string(idInstrCrt);

                UPDATE_GLUESTR;

                //UPDATE_INPUTSSTR_2_INPUT_OPERANDS;
                inputsStr = ""; //"SDValue(" + *varNameInstrProducer + ", " + portInput;

                numInputsInstr.push_back(1);
                numOutputsInstr.push_back(1); // most instructions have 1 output

                break;
             }
        }

        Instruction instrSucc(instructionsSimple[iInstr + 1]);
        bool requireGlueOrChain = isRequiredGlueOrChainOutput(instrCrt, instrSucc);
        if (requireGlueOrChain == false)
            printf("    requireGlueOrChain = false --> " \
                   "instrCrt does NOT require a glue/chain output since its successor consumes its output\n");
                   //"instrCrt does NOT require a glue/chain predecessor since it consumes its output\n");
        requireGlueOrChainPred.push_back(requireGlueOrChain);


        //printf("instrCrt.getDest() = %d\n", instrCrt.getDest());
        if (instrCrt.getDest() != NO_REG_INDEX)
            printf("  isDefined[instrCrt.getDest()] = %d\n",
                   isDefined[instrCrt.getDest()]);

        /* Handling "non-SSA" cases (where we assign a register more than once)
             with _SPECIAL_H instructions */
        if (instrCrt.getDest() != NO_REG_INDEX
                // 2018_09_05: // All instruction in a WHERE block should have SPECIAL_H (which have hasSideEffects = 1) to prevent CSE:
                // 2018_10_27:                && isDefined[instrCrt.getDest()] // IMPORTANT Note: this generates invalid SPECIAL_H SDNodes with empty tied-to constraints
              #ifndef CONVERT_PARTLY_SSA_FORM
                && (instrCrtInWhereBlock ||
                    instrCrtInRepeatBlock || // 2020_04_23
                    instrCrtInLaneGatingRegion) // 2018_10_06
              #endif
    /*
    */
                ) {
            if (isDefined[instrCrt.getDest()]) {
                printf("    Since the physical dest reg is assigned more than once " \
                    "(normally due to predication): " \
                    "we use SPECIAL_H!\n");
            }
            else {
                // MEGA-TODO: // 2018_10_27: add automatically when isDefined[instrCrt.getDest()] == false
                //   the VLOAD_BOGUS_H nodes (as we did manually in Select_SHRAi32_OpincaaCodeGen.cpp)
                //   at the beginning of the generated C++ code.

                Instruction instrCrtOrigTmp(instructions[iInstr]);
                cout << "Warning: instrCrt.getDest() = " << instrCrt.getDest()
                     << " - register not initialized before updated in WHERE"
                     << " - maybe wrong semantics."
                     << " instrCrtOrig = " << instrCrtOrigTmp.dump()
                     << "\n";
                fflush(stdout);

// 2021_02_28b:                assert(0 && "generate a corresponding VLOAD_BOGUS_H node which actually is NOT generating (due to _BOGUS) an actual vload instruction");
            }

            // Note: SSA form not really violated since we use tied-to constraint

            cout << "instrCrt.getDest() = " << instrCrt.getDest() << "\n";
            cout << "sdNodeVarNameRegDef[instrCrt.getDest()] = "
                 << sdNodeVarNameRegDef[instrCrt.getDest()] << "\n";
            cout << "virtRegVarNameIdRegDef[instrCrt.getDest()] = "
                 << virtRegVarNameIdRegDef[instrCrt.getDest()] << "\n";

          #ifdef NEW2018_08_10
            string varNameVR = virtRegVarNameIdRegDef[instrCrt.getDest()];
          #else
            // WRONG: int idInstrVR = countInstr[ID_VIRTREG] - 1;
            int idInstrVR = virtRegVarNameIdRegDef[instrCrt.getDest()];
            string varNameVR = "virtReg" + to_string(idInstrVR);
          #endif


            isdName = isdName.substr(0, isdName.size() - 2) + "_SPECIAL_H";
            if (inputsStr.size() != 0) {
                inputsStr += ",\n" \
                              "                                    ";
            }
          #ifdef NEW2018_08_10
            inputsStr += "SDValue(" + varNameVR + ", 0)";
          #else
            inputsStr += "CrtDAG->getRegister(" +
                varNameVR + ", TYPE_VECTOR_I16)";
          #endif
        }
        else {
            Instruction instrCrtOrigTmp(instructions[iInstr]);
            if ( (instrCrtInWhereBlock ||
                  instrCrtInRepeatBlock || // 2020_04_23
                  instrCrtInLaneGatingRegion) // 2018_10_06
                   && instrCrt.getDest() != NO_REG_INDEX) {
            // MEGA-TODO: when CONVERT_PARTLY_SSA_FORM commented, this check should always be false
                assert(isDefined[instrCrt.getDest()] == false);
                cout << "Warning: instrCrt.getDest() = " << instrCrt.getDest()
                     << " - register not initialized before updated in WHERE"
                     << " - maybe wrong semantics."
                     << " instrCrtOrig = " << instrCrtOrigTmp.dump()
                     << "\n";
                fflush(stdout);

            // MEGA-MEGA-TODO: do NOT disallow, but use SPECIAL_SIDE_EFFECTS_H instructions.
                // 2018_09_05
                assert(0 && "Destination register NOT initialized. "
                            "This should NOT be allowed because it will result "
                            "in generating a normal instruction instead of "
                            "SPECIAL_H and this implies it will not have "
                            "'flag hasSideEffects = 1' set, which might result "
                            "in the instruction being CSEd or LICMed.");


          #ifdef COPY_REGISTER_IMPLEMENTED_WITH_ORV_H
            assert(instrCrtOpcode != _OR &&
                    "We strongly advise to avoid using OR instructions inside "
                    "WHERE blocks with the destination register being "
                    "uninitialized since this upsets the PassAfterPostRAScheduler "
                    "pass in ConnexTargetMachine.cpp of OpincaaLLVM - the "
                    "instruction can/will be confused as a COPY (especially if it "
                    "has both operands identical, in which case you can use "
                    "SHL/R 0 in the Opincaa program");
          #endif
            }
        }

        if (inputsStr.find(STR_NOT_INIT) != std::string::npos) {
            Instruction instrCrtOrig(instructions[iInstr]);
            cout << "The following instruction has UNinitialized input(s):\n"
                 << inputsStr.c_str()
                 << "\n    instrCrtOrig = " << instrCrtOrig.dump();
            assert(0 && "instrCrt has UNinitialized input(s)");
        }


        //predInstr = &instr;
        predVarName = varName; //sdNodeVarNameRegDef[instr.getDest()];
        varNameInstr.push_back(varName);


        string strCopyToReg;
        if (instrCrt.getDest() != NO_REG_INDEX
            /*
            #ifndef CONVERT_PARTLY_SSA_FORM
              // WRONG: && (isInstrInsideWhereBlock == false)
            #endif
            */
                ) {
            /* VERY IMPORTANT: this is where we keep track of the
                 reaching definitions register defs */
            sdNodeVarNameRegDef[instrCrt.getDest()] = varName;

            isDefined[instrCrt.getDest()] = true;

            /*
            printf("countInstr = %p\n", countInstr);
            printf("virtRegVarNameIdRegDef = %p\n", virtRegVarNameIdRegDef);
            */
            strCopyToReg = GenerateIfRequiredCopyToReg(iInstr,
                                              instrCrt,
                                              instrCrtInWhereBlock ||
                                              instrCrtInRepeatBlock, // 2020_04_23
                                               /*
                                            #ifndef CONVERT_PARTLY_SSA_FORM
                                              isInstrInsideWhereBlock,
                                            #endif
                                              */
                                              numInstructionsToCodegen,
                                              offsetKernelToStartCodegenFrom,
                                              //
                                              //instructionsSimple.data(),
                                              //&(instructionsSimple[0]),
                                              partlySSAInstrs,
                                              //
                                              varName,
                                              countInstr,
                                              virtRegVarNameIdRegDef);
        }


        // 2021_02_28:
        bool instrCrtInWhereBlockWith4Opnds;
        // This special case has 4 operands: 2 standard, 1 tied-to, 1 glue (or chain)
        instrCrtInWhereBlockWith4Opnds = instrCrtInWhereBlock &&
                                         (numInputsInstr[indexInstrCrt] == 2) &&
                                         (numOutputsInstr[indexInstrCrt] == 1);
        // MULT and MULT_U don't achieve 4 operands since they don't have
        //      result, hence NO tied-to input either.
        //
        // 2021_02_28:
        bool instrCrtWith2Res = (numOutputsInstr[indexInstrCrt] != 0) && requireGlueOrChain;
        // If instrCrtInWhereBlockWith4Opnds is true (has 4 operands, then it
        //   does NOT hold instrCrtWith2Res == true (has 2 results)
        //   at least for MULT[U].



        cout << "Pretty printing the SDNode...\n";
        //

      #ifdef CONVERT_PARTLY_SSA_FORM
      //#ifdef DO_NOT_PRINT_ORIGINAL_INSTR_BEFORE_RENAMING
        // We print also the original instruction to see how register renaming
        //   affected (ConvertInPartlySSAForm()) affected the instructions
        Instruction instrCrtOrig(instructions[iInstr]); // NOT required: - offsetKernelToStartCodegenFrom]);

        ss << "// " << instrCrtOrig.dump(); // NOT required: << "\n";
        ss << "// Instr "
           << "#" << iInstr - offsetKernelToStartCodegenFrom << "\n";
        /*
        ss << "// SSA renamed instr  (after reg-renaming)"
           << instrCrt.dump();
        */

        /*
        ss << "// Original instr "
           << "#" << iInstr - offsetKernelToStartCodegenFrom << " "
           << "before reg-renaming: " << instrCrtOrig.dump();
        */
        //
      //#endif
      #else
        ss << "// " << instrCrt.dump(); // NOT required: << "\n";
        ss << "// Instr "
           << "#" << iInstr - offsetKernelToStartCodegenFrom << "\n";
      #endif

        ss <<
          "SDNode *" << varName << " = CrtDAG->getMachineNode(\n" \
                      "                                    Connex::" << isdName << ",\n" \
                      "                                    DL,\n";

        // 2021_02_28
        if (instrCrtInWhereBlockWith4Opnds && instrCrtWith2Res)
            ss << "                                    CrtDAG->getVTList(\n";

        if (numOutputsInstr[indexInstrCrt] != 0)
            ss <<     "                                    TYPE_VECTOR_I16,\n";

        if (requireGlueOrChain ||
            strCopyToReg.size() != 0 /* For instrCrt requiring CopyToReg */ ) {

            /*
             MEGA-TODO: use as many Glue as possible, at least for WHERE blocks
                and their previous predicate instr + NOP.
                  - we should adapt isRequiredGlueOrChainOutput() to have
                     more Glue outputs.

             TODO: we should actually understand better why using too many Glue
                   gives errors.

          #ifdef CODE_WITH_CONCEPTUAL_ERROR
            if (numInputsInstr[numInputsInstr.size() - 1] == 2)
                if (instrCrt.getRight() == instrCrt.getLeft())
                    printf("Note: instrCrt has 2 input instructions that are "
                           "the SAME (%d).",
                           instrCrt.getRight());

            if (useGlue == 1 && numInputsInstr[numInputsInstr.size() - 1] < 2)
          #endif
            */
            if (useGlue == 1) {
                // 2021_02_28
                if (instrCrtInWhereBlockWith4Opnds)
                    ss <<     "                                    MVT::Glue\n";
                else
                    // Printing comma char as well
                    ss <<     "                                    MVT::Glue,\n";
            }
            else {
                // 2021_02_28
                if (instrCrtInWhereBlockWith4Opnds)
                    ss <<     "                                    MVT::Other\n";
                else
                    // Printing comma char as well
                    ss <<     "                                    MVT::Other,\n";
            }
        }

        // 2021_02_28
        if (instrCrtInWhereBlockWith4Opnds && instrCrtWith2Res)
            ss << "                                    ),\n"; // Ending CrtDAG->getVTList()

        // 2021_02_28
        // In case we print 4 input operands (2 standard, 1 tied-to, 1
        //      glueOrChain)
        if (instrCrtInWhereBlockWith4Opnds) {
            assert(numInputsInstr.size() - 1 == indexInstrCrt);
            //numOutputsInstr[indexInstrCrt]
            ss << "                                    {\n";
        }


        if (inputsStr.size() != 0)
            ss <<     "                                    " << inputsStr;
        if (glueOrChainStr.size() != 0) {
            if (inputsStr.size() != 0)
                ss << ",\n";
            ss //<<     "\n"
               <<     "                                    // glue (or chain) input edge\n" \
                      "                                    " << glueOrChainStr << "\n";
        }
        else {
            ss <<     "\n"
               <<     "                                    // no need for glue or chain input (since it normally consumes the output of the predecessor)\n";
        }


        //instrCrtInWhereBlockWith4Opnds = false;
        // 2021_02_28
        // In case we print 4 input operands (2 standard, 1 tied-to, 1
        //      glueOrChain)
        if (instrCrtInWhereBlockWith4Opnds) {
            assert(numInputsInstr.size() - 1 == indexInstrCrt);
            //numOutputsInstr[indexInstrCrt]
            ss << "                                    }\n";
        }


        ss <<         "                                    );\n";
        ss << strCopyToReg; //.str();
        ss <<         "\n";

        if (instrCrtInWhereBlock)
            numInstrsInWhereBlock++;
        if (instrCrtInRepeatBlock) // 2020_04_23
            numInstrsInRepeatBlock++;

        cout << "Again: indexInstrCrt = " << indexInstrCrt << ": instrCrt.dump() = "
             << instrCrt.dump() << "\n";
        printf("numInputsInstr[%d] = %d\n", indexInstrCrt,
               numInputsInstr[indexInstrCrt]);
        printf("  numInputsInstr.size() = %lu\n",
               numInputsInstr.size());
        printf("numOutputsInstr[%d] = %d\n", indexInstrCrt,
               numOutputsInstr[indexInstrCrt]);
        printf("  numOutputsInstr.size() = %lu\n",
               numOutputsInstr.size());
        fflush(stdout);

        // Some silly validation tests
        if (instrCrt.getDest() == NO_REG_INDEX &&
                instrCrtOpcode != _WHERE_CRY &&
                instrCrtOpcode != _WHERE_EQ &&
                instrCrtOpcode != _WHERE_LT &&
                instrCrtOpcode != _END_WHERE &&
                instrCrtOpcode != _IWRITE &&
                instrCrtOpcode != _SETLC &&
                instrCrtOpcode != _IJMPNZ
                ) {
            assert(numOutputsInstr[indexInstrCrt] == 0);
        }
    } // end for (iInstr)


    printf("Kernel::genLLVMISelManualCode(): numInstrsInWhereBlock = %d, numWhereInstrs = %d, numEndWhereInstrs = %d (out of %d)\n",
           numInstrsInWhereBlock, numWhereInstrs, numEndWhereInstrs, numInstructionsToCodegen);
    fflush(stdout);
// MEGA-TODO: check better for this directly to see if a WHERE has an END_WHERE following
    assert(numWhereInstrs == numEndWhereInstrs &&
           "It seems Where blocks are ill-formed.");

    printf("Exiting Kernel::genLLVMISelManualCode().\n");

    FILE *fout = fopen("DumpISel_OpincaaCodeGen.cpp", "wt");
    fputs(ss.str().c_str(), fout);
    fclose(fout);

    return ss.str();
    //return res;
} // END Kernel::genLLVMISelManualCode()

/************************************************************
 * Resets the loop size counter so each appended instruction
 * after this one increments it with 1. It is used to determine
 * where the jump needs to be made.
 */
void Kernel::resetLoopJumpTarget() {
    loopNestDepth++;
    dprintf("Entered Kernel::resetLoopJumpTarget(): made loopNestDepth = %d\n",
            loopNestDepth);
    assert(loopNestDepth < MAX_DEPTH_LOOP_NESTING + 1);

    jumpTargetForLoopOfNestDepth[loopNestDepth] = 0;
}

/******************************************************************
 * This will append the IJMPNZDEC jump instruction to the kernel by
 *   using the loop jump target.
 *
 * The assembler programmer does NOT need to compute himself the target of
 *   the JMP instruction - OPINCAA itself will compute all the targets
 *   for the corresponding REPEAT..END_REPEAT loops.
 */
void Kernel::appendLoopInstruction(int jmpOpcode, int redReg) {
    printf("Kernel::appendLoopInstruction(): "
           "jumpTargetForLoopOfNestDepth[loopNestDepth = %d] = %d\n",
           loopNestDepth, jumpTargetForLoopOfNestDepth[loopNestDepth]);
    printf("Kernel::appendLoopInstruction(): jmpOpcode = %d, redReg = %d\n",
           jmpOpcode, redReg);
    fflush(stdout);

    //append(Instruction(_IJMPNZ, jumpTargetForLoopOfNestDepth[loopNestDepth], 0, 0));
    append(Instruction(jmpOpcode, jumpTargetForLoopOfNestDepth[loopNestDepth],
           redReg, 0)); // 2021_08_08

    /*
    // This can be made to work but it PREVENTS to compute kernel->size().
    //   We can let the OPINCAA simulator do this check.
    // 2020_03_29
    if (jumpTargetForLoopOfNestDepth[loopNestDepth] > INTERNAL_INSTRUCTION_MEMORY_SIZE) {
        printf("Kernel::appendLoopInstruction(): "
               "IIM is too small to run this code\n",
               loopNestDepth, jumpTargetForLoopOfNestDepth[loopNestDepth]);
        exit(-1);
    }
    */

    loopNestDepth--;
}

vector<InstructionType> &Kernel::getInstructions() {
    return instructions;
}

