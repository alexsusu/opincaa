/*
 * File:   Kernel.cpp
 *
 * This is the  class containing a kernel
 * (a vector of Instructions) for executing on the Connex Array
 *
 */


#include "Architecture.h"
#include "Kernel.h"
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


/************************************************************
* Constructor for creating a new Kernel
*
* @param name the name of the new kernel\
*
* @throws string if the name is invalid (NULL or empty)
*/
Kernel::Kernel(string aName) {
    printf("Entered Kernel::Kernel(): name = %s\n", aName.c_str());
    fflush(stdout);

    // 2018_04_03
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
    printf("Entered Kernel::~Kernel(): name = %s\n", name.c_str());
    printf("  Kernel::~Kernel(): sdNodeVarNameRegDef = %p\n", sdNodeVarNameRegDef);
    fflush(stdout);

    // 2018_04_03
    // Gets automatically deallocated since it is a member variable: instructions.clear();
    // Gets automatically deallocated since it is a member variable: name.clear();

    // 2018_04_03
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
void Kernel::append(Instruction instruction)
{
    instructions.push_back(instruction.assemble());
    loopDestination++;
}

/* Copy from the myBinaryData array (e.g. from a preassembled kernel)
 *   to the instructions member of the Kernel class. */
void Kernel::copyBinaryKernel(unsigned int *myBinaryData, int numInstructions) {
    // See http://www.cplusplus.com/reference/vector/vector/assign/
    instructions.assign(myBinaryData, myBinaryData + numInstructions);
}


/************************************************************
* Writes the kernel to a memory location
*
* @param buffer the memory location to write the kernel to
*/
void Kernel::writeTo(void * buffer)
{
    memcpy(buffer, instructions.data(), instructions.size() * sizeof(unsigned));
}

/************************************************************
* Writes the kernel to a file descriptor
* @param fileDescriptor the file descriptor to write the kernel to
*/
void Kernel::writeTo(int fileDescriptor) {
    int res;

    res = write(fileDescriptor, instructions.data(), instructions.size() * sizeof(unsigned));

    /* Flush the descriptor */
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
    string kernel;

    printf("Kernel::dump(): Num instructions = %d\n", this->size());

    for (vector<unsigned>::iterator element = instructions.begin(); element != instructions.end(); element++) {
        kernel += Instruction(*element).dump();
    }

    return kernel;
}

/*
 * Returns a string representing the disassembled kernel.
 * One instruction per line.
 */
string Kernel::disassemble() {
    string kernel;

    printf("Kernel::disassemble(): Num instructions = %d\n", this->size());

    for (vector<unsigned>::iterator element = instructions.begin();
         element != instructions.end();
         element++) {
        kernel += Instruction(*element).disassemble();
    }

    return kernel;
}

// We check if instrCrt already has a true (RAW) data-dependence with instrSucc
inline bool isRequiredGlueOrChainOutput(Instruction &instrCrt, Instruction &instrSucc) {
    if (instrCrt.getDest() != -1 &&
            ((instrSucc.getLeft() != -1 &&
                    instrCrt.getDest() == instrSucc.getLeft()) ||
             (instrCrt.getType() == INSTRUCTION_TYPE_NO_VALUE &&
              (instrSucc.getRight() != -1 &&
                instrCrt.getDest() == instrSucc.getRight()))
             )) {
        return false;
    }

    return true;
}

//#define instructionsSimple (instructions.begin() + offsetKernelToStartCodegenFrom)
//#define instructionsSimple instructions
// TODO: defining the array with this macro with iterator is kind-of abusive, so try to avoid it 
#define instructionsSimple (partlySSAInstrs.begin() - offsetKernelToStartCodegenFrom)

string GenerateIfRequiredCopyToReg(int &iInstr,
                                    Instruction &instrCrt,
                                    int &numInstructionsToCodegen,
                                    int &offsetKernelToStartCodegenFrom,
                                    /*
                                    //vector<unsigned> &instructionsSimple,

                                    A bit complicated but it works (adjust
                                     accordingly if something changes in the
                                     future */
                                    //Instruction *instructionsSimple,
                                    vector<Instruction> &partlySSAInstrs,
                                    string &varName,
                                    int *countInstr,
                                    int *virtRegVarNameIdRegDef) {

    #define ID_GET_COPYTOREG ((1UL << OPCODE_9BITS_SIZE) + 1)
    #define ID_VIRTREG ((1UL << OPCODE_9BITS_SIZE) + 2)

    /*
    printf("Alex: countInstr = %p in GenerateIfRequiredCopyToReg()\n", countInstr);
    printf("Alex: virtRegVarNameIdRegDef = %p in GenerateIfRequiredCopyToReg()\n", virtRegVarNameIdRegDef);
    */
    stringstream ss2;

    /* We codegen a getCopyToReg() if we have after instrCrt a def to the
       same register as the dest register of instrCrt. */
    for (int iInstrAfter = iInstr + 1;
          iInstrAfter < offsetKernelToStartCodegenFrom + numInstructionsToCodegen;
          iInstrAfter++) {

        Instruction instrAfter(instructionsSimple[iInstrAfter]);

        if (instrAfter.getDest() != -1 &&
                instrCrt.getDest() == instrAfter.getDest()) {

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

            virtRegVarNameIdRegDef[instrCrt.getDest()] = idInstrVR;

            int idInstrCTR = countInstr[ID_GET_COPYTOREG];
            countInstr[ID_GET_COPYTOREG]++;
            string varNameCTR = "copyToReg" + to_string(idInstrCTR);

            // MAYBE check also if the instruction has input the dest reg
            //      "                   SDValue(" << varName << ", " << numInputsInstr[indexInstr] << "),\n"
            ss2 << "unsigned " << varNameVR << " = RegInfo->createVirtualRegister(&Connex::MSA128HRegClass);\n";
            ss2 << "SDValue " << varNameCTR << " = CurDAG->getCopyToReg(\n" \
                  "                   // glue (or chain) input edge\n" \
                  "                   SDValue(" << varName << ", 1),\n" \
                  "                   DL,\n" \
                  "                   " << varNameVR << ",\n" \
                  "                   // Value copied to register\n" \
                  "                   SDValue(" << varName << ", 0)\n" \
                  "                   // Glue\n" \
                  "                  );\n\n";

            /* IMPORTANT NOTE: CopyToReg SDNode does NOT have chain or
                 glue output edges.
            predVarName = varNameCTR; */
            break;
        }
    }

    //return string(""); //std::to_string(0);
    return ss2.str();
} // END GenerateIfRequiredCopyToReg()


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

    //printf("unsigned table[%d] = {\n", kernelSize);
    ss << "#define PREASSEMBLED_BINARY_KERNEL_SIZE " << kernelSize << "\n";
    //ss << "unsigned int preassembledBinaryKernel[" << kernelSize << "] = {\n";
    ss << "unsigned int preassembledBinaryKernel[PREASSEMBLED_BINARY_KERNEL_SIZE] = {\n";

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

/* TODO TODO TODO TODO: in case a _SPECIAL instr inside a WHERE block does not
      have an UNpredicated predecessor instruction we should give a warning.

   We generate C++ (manual) ISel code for an intrinsic.
   IMPORTANT: We advise the kernel to have registers allocated from last one
       (CONNEX_REG_COUNT) downwards.
    we ~require to have ASM code in "partly SSA-form" i.e., every register
          SHOULD be assigned only once.
     If it is assigned twice (which currently happens only for WHERE blocks)
      it means we have a special constraint and for this we use MachineSDNodes that
      are defined (in TableGen) with tied-to operand constraints.
   NOTE: we do not use any explicit symbolic operands in our Connex ASM code.

   We do not rely on predication, which seems it is not working in the
       Instruction Selection (ISel) pass, where it would have been the most
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
      need to propagate (they have inputs and outputs that have tied registers)
      some values.
       IMPORTANT: they really need to feed the output registers to the
         following nodes, otherwise the "REWRITING TWO-ADDR INSTRS" pass
         will generate for them COPY instructions to copy registers and the
         "SIMPLE REGISTER COALESCING" pass will not remove these basically
         useless instructions). (although WHEREEQ does NOT continue propagating
            its result to following nodes since the value is NOT used anymore
            by no one, END_WHERE really needs to propagate it for the sake
            of generating 1 less COPY instruction because the value it takes
            as input and output is further used).
    It seems a bit complicated, but to be part of the dataflow WHERE and
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
*/
string Kernel::genLLVMISelManualCode() {
    stringstream ss;
    //string res;
    int i;

    assert(numInstructionsToCodegen != -1 &&
            "You need to initialize numInstructionsToCodegen");
    assert(offsetKernelToStartCodegenFrom != -1 &&
            "You need to initialize offsetKernelToStartCodegenFrom");


    ss << "// Code auto-generated by method Kernel::genLLVMISelManualCode() from Opincaa lib.\n";
    //cout << "ss = " << ss.str();
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
    bool isUsedOverall[CONNEX_REG_COUNT];
    for (i = 0; i < CONNEX_REG_COUNT; i++) {
        isUsedOverall[i] = false;
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
            SDNode *ldsh0 = CurDAG->getMachineNode(...);
            unsigned virtReg0 = RegInfo->createVirtualRegister(&Connex::MSA128HRegClass);
            SDValue copyToReg0 = CurDAG->getCopyToReg(...);
          and later want to use this virtReg0 in:
            SDNode *sub0 = CurDAG->getMachineNode(
                                    Connex::SUBV_SPECIAL_H,
                                    DL,
                                    TYPE_VECTOR_I16,
                                    MVT::Other,
                                    SDValue(vload0, 0),
                                    SDValue(ldsh0, 0),
                                    CurDAG->getRegister(virtReg0, TYPE_VECTOR_I16),
                                    SDValue(whereeq0, 1)
                                    );
        we need to remember the id/number of virtReg0 for this SDNode above (sub0) we print.
    */
    int virtRegVarNameIdRegDef[CONNEX_REG_COUNT];
    for (i = 0; i < CONNEX_REG_COUNT; i++) {
        virtRegVarNameIdRegDef[i] = -1;
    }

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

    bool kernelHasInputRegistersSpecified = false;
    for (i = 0; i < CONNEX_REG_COUNT; i++) {
        if (sdNodeVarNameRegDef[i].empty() == true) {
            sdNodeVarNameRegDef[i] = "[NOT_INITIALIZED]";
        }
        else {
            printf("Alex: sdNodeVarNameRegDef[%d] = %s --> initializing isDefined, etc\n",
                    i, sdNodeVarNameRegDef[i].c_str());

            // We initialize as isDefined the input registers to this kernel
            isDefined[i] = true;

            //inputRegistersKernel.push_back(i);
            inputRegistersKernelAndSDNodeVarName[i] = sdNodeVarNameRegDef[i];
            lastInputRegisterKernelSDNodeVarName = sdNodeVarNameRegDef[i];

            kernelHasInputRegistersSpecified = true;
        }
    }
    // We check if we specified input registers to this kernel
    assert(kernelHasInputRegistersSpecified == true && "NO input registers specified to the kernel");

    /* Represents the offset in the kernel of the 1st instruction of our region
     *  to codegen: */
    //offsetKernelToStartCodegenFrom = 12;


    int N = numInstructionsToCodegen;
    assert(this->size() >= offsetKernelToStartCodegenFrom +
                            numInstructionsToCodegen);


    int iInstr;


    printf("Kernel::genLLVMISelManualCode(): Num instructions = %d\n", N);
    fflush(stdout);

    cout << "SDNode *ConnexDAGToDAGISel::Select___(SDNode *Node) { //...TO PUT MORE\n";
    cout << "    SDLoc DL(Node);\n";
    cout << "// We make the following assumptions: inputs are ...!!!!\n";


#define CONVERT_PARTLY_SSA_FORM

#ifdef CONVERT_PARTLY_SSA_FORM
    bool isDefined2[CONNEX_REG_COUNT];
    for (i = 0; i < CONNEX_REG_COUNT; i++) {
        isDefined2[i] = isDefined[i];
    }

    /*
    void ConvertInPartlySSAForm() {
    }
    */
    vector<Instruction> partlySSAInstrs;

    // We do ConvertInPartlySSAForm
    // First we initialize the isUsedOverall for the given kernel
    for (iInstr = offsetKernelToStartCodegenFrom;
            iInstr < offsetKernelToStartCodegenFrom + numInstructionsToCodegen;
            iInstr++) {
        Instruction instr(instructions[iInstr]);
        if (instr.getDest() != -1) {
            isUsedOverall[instr.getDest()] = true;
        }
    }

    bool isInWhereBlock = false;
    printf("Automatically generating 'partly SSA form'...\n");
    /* Constructing vector partlySSAInstrs - we need to mutate it and it's the
         only way since instructions is a vector<unsigned> from whose
         elements we build each time normally an Instruction. */
    for (iInstr = offsetKernelToStartCodegenFrom;
            iInstr < offsetKernelToStartCodegenFrom + numInstructionsToCodegen;
            iInstr++) {
        Instruction instrCrt(instructions[iInstr]);
        partlySSAInstrs.push_back(instrCrt);
    }
    assert(partlySSAInstrs.size() == numInstructionsToCodegen);
    // Doing the actual transformation
    for (iInstr = offsetKernelToStartCodegenFrom;
            iInstr < offsetKernelToStartCodegenFrom + numInstructionsToCodegen;
            iInstr++) {
        /*
        The algorithm we use is:
            if NOT instr is in WHERE block
               if isInstrWithDest()
                    if isDefined2(instr.getDest())
                        take new register name
                        replace subsequent uses with this new reg name
        */
        //Instruction instrCrt(instructions[iInstr]);

        int indexInstr = iInstr - offsetKernelToStartCodegenFrom;

        #define instrCrt partlySSAInstrs[indexInstr]

        cout << "indexInstr = " << indexInstr
             << ": instrCrt.dump() = " << instrCrt.dump();
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

        if (instrCrt.getDest() != -1) {
            int instrCrtDest = instrCrt.getDest();
            if (isDefined2[instrCrtDest]) {
                if (isInWhereBlock) {
                    printf("For 'partly SSA form': we do NOT change dest reg since it (probably) must update the register\n");
                }
                else {
                    // We assign an unused register for the destination register
                    for (i = 0; i < CONNEX_REG_COUNT; i++) {
                        if (isUsedOverall[i] == false) {
                            isUsedOverall[i] = true;
                            isDefined2[i] = true;
                            break;
                        }
                    }
                    assert(i < CONNEX_REG_COUNT);

                    printf("For 'partly SSA form': changing dest reg from %d to %d\n",
                            instrCrtDest, i);
                    instrCrt.setDest(i);
                    //cout << "   -->  instrCrt.dump() = " << instrCrt.dump();

                    bool isInstrAfterInWhereBlock = false;
                    // Updating the register changed for all following instructions
                    for (int indexInstrAfter = indexInstr + 1;
                            indexInstrAfter < numInstructionsToCodegen;
                            indexInstrAfter++) {
                        //Instruction instrAfter(instructions[iInstrAfter]);
                        #define instrAfter partlySSAInstrs[indexInstrAfter]


                        /* We change for all stmts in this EXECUTE_ALL block and the following WHERE
                         block, if any, BUT NOT more than that. */
                        int instrAfterOpcode = instrAfter.getOpcode();

                        if ((instrAfterOpcode == _WHERE_CRY) ||
                            (instrAfterOpcode == _WHERE_EQ) ||
                            (instrAfterOpcode == _WHERE_LT)) {
                            isInstrAfterInWhereBlock = true;
                        }
                        else
                        if (instrAfterOpcode == _END_WHERE) {
                            if (isInstrAfterInWhereBlock == true)
                                break;
                            isInstrAfterInWhereBlock = false;
                        }

                        if (instrAfter.getDest() != -1 &&
                                instrAfter.getDest() == instrCrtDest) {
                            instrAfter.setDest(i);
                        }
/*
*/

                        if (instrAfter.getLeft() == instrCrtDest)
                            instrAfter.setLeft(i);

                        if (instrAfter.getType() == INSTRUCTION_TYPE_NO_VALUE &&
                                instrAfter.getRight() == instrCrtDest) {
                            instrAfter.setRight(i);
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
    printf("Cleaning up after autogen 'partly SSA form'\n");
    for (i = 0; i < CONNEX_REG_COUNT; i++) {
        if (sdNodeVarNameRegDef[i].empty() == true) {
            isDefined[i] = false;
        }
        else {
            printf("Alex: sdNodeVarNameRegDef[%d] = %s --> initializing isDefined\n",
                    i, sdNodeVarNameRegDef[i].c_str());

            // We initialize as isDefined the input registers to this kernel
            isDefined[i] = true;
        }
    }
    */
#endif // END CONVERT_PARTLY_SSA_FORM


    unsigned *predInstr = NULL;
    string predVarName = "[NONE_SO_REPLACE]";
    string varName = "[NONE_SO_REPLACE]";

    // The variable names of the MachineSDNodes
    vector<string> varNameInstr;

    // The num of real inputs (not chain or glue) of the MachineSDNodes
    vector<int> numInputsInstr;
    // The num of real outputs of the MachineSDNodes
    vector<int> numOutputsInstr;
    vector<bool> requireGlueOrChainPred;


    // Adding CopyToReg, if required, for the inputs of the Opincaa kernel
    string strCopyToReg;
    iInstr = offsetKernelToStartCodegenFrom;
    //int reg = 0;
    //for (auto reg: inputRegistersKernelAndSDNodeVarName.keys()) {
    for (auto iterMap: inputRegistersKernelAndSDNodeVarName) {
        int reg = (iterMap).first;
        //varName = "nodeOpSrcCast1";
        varName = (iterMap).second; //inputRegistersKernelAndSDNodeVarName[reg];

        /* We create a bogus instruction with destination reg
        R28 = LS[R0] (the dest reg will be changed) */
        Instruction instrCrt(0x9200001C);
        instrCrt.setDest(reg);
        assert(instrCrt.getDest() == reg);

        strCopyToReg = GenerateIfRequiredCopyToReg(iInstr,
                                              instrCrt,
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


    /* TODO TODO TODO: DONE: if the consumer of dest register set by current
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

/* TODO DONE TODO DONE TODO DONE: We have issues in MUL_i32 with the wrong usage of
    registers... - I have to put more constraints between registers, not only related to updates as I currently did... */

        cout << "indexInstrCrt = " << indexInstrCrt << ": instrCrt.dump() = "
             << instrCrt.dump(); // << "\n";
        fflush(stdout);

        int instrCrtOpcode = instrCrt.getOpcode();

        cout << "  instrCrt.getDest() = " << instrCrt.getDest() << "\n";
        cout << "  instrCrt.getLeft() = " << instrCrt.getLeft() << "\n";
        fflush(stdout);
      #ifdef NNNNNNO
        if (isInstrWithDest(instrCrtOpcode)) {
            // We test that our kernel is in "SSA" form
            assert(isDefined[instrCrt.getDest()] == false);
        }
      #endif


        int idInstrCrt = countInstr[instrCrtOpcode];
        countInstr[instrCrtOpcode]++;


        switch (instrCrtOpcode) {
            case _VLOAD: {
                int idGetConstant = countInstr[ID_GET_CONSTANT];
                //printf("idGetConstant = %d\n", idGetConstant);
                //fflush(stdout);

                countInstr[ID_GET_CONSTANT]++;
                string ctVarName = "ct" + to_string(idGetConstant);
                ss <<
                      "SDValue " << ctVarName
                      << " = CurDAG->getConstant(" << instrCrt.getValue()
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
                    assert(instrPredDestReg != -1);
                    inputsStr = inputsStr + ",\n" +
                        "                                    SDValue(" + sdNodeVarNameRegDef[instrPredDestReg] + ", 0)";
                }
                else {
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
                      << " = CurDAG->getConstant(" << instrCrt.getRight()
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
            case _MULT: {
                isdName = "MULT_H";

                varName = "mult" + to_string(idInstrCrt);

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
                int idGetConstant = countInstr[ID_GET_CONSTANT];
                countInstr[ID_GET_CONSTANT]++;
                string ctVarName = "ct" + to_string(idGetConstant);
                ss <<
                      "SDValue " << ctVarName
                      << " = CurDAG->getConstant(1 /* Num of cycles to NOP */"
                      << ", DL, MVT::i16, true, false);\n";

                varName = "nop" + to_string(idInstrCrt);

                inputsStr = ctVarName;
                UPDATE_GLUESTR;

                numInputsInstr.push_back(1);
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
            case _WHERE_CRY:
            case _WHERE_EQ:
            case _WHERE_LT: {
                string varNamePrefix;

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

                inputsStr = "SDValue(" + *varNameInstrProducer + ", " +
                              to_string(portInput) + ")";
                UPDATE_GLUESTR;

                numInputsInstr.push_back(1);
                numOutputsInstr.push_back(1);

                break;
            }
            case _END_WHERE: {
                isdName = "END_WHERE";

                varName = "endwhere" + to_string(idInstrCrt);

                Instruction instrPred(instructionsSimple[iInstr - 1]);

                // Asserting the predecessor instr of END_WHERE has a dest. register
                assert(instrPred.getDest() != -1);

                inputsStr = "SDValue(" + predVarName + ", " +
                              to_string(0) + ")";

                /* IMPORTANT: Applying Rule #3:
                   We make END_WHERE propagate its value if used anymore to
                     avoid (if possible) generating a useless COPY instruction,
                     as specified in Rule #3 above.
                */
                sdNodeVarNameRegDef[instrPred.getDest()] = varName;

                UPDATE_GLUESTR;

                numInputsInstr.push_back(1);
                numOutputsInstr.push_back(1);

                break;
            }
            case _POPCNT: { // also for _READ, _WRITE, _NOT
                string varNamePrefix;
                if (instrCrtOpcode == _POPCNT) {
                    isdName = "POPCNT_H";
                    varNamePrefix = "popcnt";
                }

                varName = varNamePrefix + to_string(idInstrCrt);

                UPDATE_INPUTSSTR_1_INPUT_OPERAND;
                UPDATE_GLUESTR;

                numInputsInstr.push_back(1);
                numOutputsInstr.push_back(1);

                break;
            }
            case _READ:
            case _WRITE:
            //
            case _IREAD:
            case _IWRITE:
            //
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


        //printf("Alex: instrCrt.getDest() = %d\n", instrCrt.getDest());
        if (instrCrt.getDest() != -1)
            printf("  isDefined[instrCrt.getDest()] = %d\n",
                    isDefined[instrCrt.getDest()]);

        /* Handling "non-SSA" cases (where we assign a register more than once)
             with _SPECIAL_H instructions */
        if (instrCrt.getDest() != -1 &&
                isDefined[instrCrt.getDest()]) {
            printf("    Since this physical dest reg is assigned more than once " \
                    "(normally due to predication): " \
                    "we use SPECIAL_H!\n");
            // SSA form not really violated since we use tied-to constraint

            cout << "instrCrt.getDest() = " << instrCrt.getDest() << "\n";
            cout << "sdNodeVarNameRegDef[instrCrt.getDest()] = "
                 << sdNodeVarNameRegDef[instrCrt.getDest()] << "\n";
            cout << "virtRegVarNameIdRegDef[instrCrt.getDest()] = "
                 << virtRegVarNameIdRegDef[instrCrt.getDest()] << "\n";

            // WRONG: int idInstrVR = countInstr[ID_VIRTREG] - 1;
            int idInstrVR = virtRegVarNameIdRegDef[instrCrt.getDest()];

            string varNameVR = "virtReg" + to_string(idInstrVR);

            isdName = isdName.substr(0, isdName.size() - 2) + "_SPECIAL_H";
            if (inputsStr.size() != 0)
                inputsStr += ",\n" \
                              "                                    ";
            inputsStr += "CurDAG->getRegister(" +
                varNameVR + ", TYPE_VECTOR_I16)";
        }



        //predInstr = &instr;
        predVarName = varName; //sdNodeVarNameRegDef[instr.getDest()];
        varNameInstr.push_back(varName);


        string strCopyToReg;
        if (instrCrt.getDest() != -1) {
            sdNodeVarNameRegDef[instrCrt.getDest()] = varName;

            isDefined[instrCrt.getDest()] = true;

            /*
            printf("Alex: countInstr = %p\n", countInstr);
            printf("Alex: virtRegVarNameIdRegDef = %p\n", virtRegVarNameIdRegDef);
            */
            strCopyToReg = GenerateIfRequiredCopyToReg(iInstr,
                                              instrCrt,
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

        ss << "// " << instrCrt.dump(); // NOT required: << "\n";
        ss <<
          "SDNode *" << varName << " = CurDAG->getMachineNode(\n" \
                      "                                    Connex::" << isdName << ",\n" \
                      "                                    DL,\n";
        if (numOutputsInstr[indexInstrCrt] != 0)
            ss <<     "                                    TYPE_VECTOR_I16,\n";
        if (requireGlueOrChain ||
            strCopyToReg.size() != 0 /* We put this because ...*/ ) //.str().size() != 0)
          if (useGlue == 1) {
            ss <<     "                                    MVT::Glue,\n";
          }
          else {
            ss <<     "                                    MVT::Other,\n";
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

        ss <<         "                                    );\n";
        ss << strCopyToReg; //.str();
        ss <<         "\n";


        // Some silly validation tests
        if (instrCrtOpcode != _WHERE_CRY &&
                instrCrtOpcode != _WHERE_EQ &&
                instrCrtOpcode != _WHERE_LT &&
                instrCrtOpcode != _END_WHERE &&
                //instrCrtOpcode != _IJMPNZDEC &&
                instrCrt.getDest() == -1)
            assert(numOutputsInstr[indexInstrCrt] == 0);
    }

    FILE *fout = fopen("DumpISel_OpincaaCodeGen.cpp", "wt");
    fputs(ss.str().c_str(), fout);
    fclose(fout);

    return ss.str();
    //return res;
} // END Kernel::genLLVMISelManualCode()

/************************************************************
 * Resets the loop size counter so each appended instruction
 * after this one increments it with 1. It is used to determine
 * where the jump needs to be made
 */
void Kernel::resetLoopDestination()
{
    loopDestination = 0;
}

/************************************************************
 * This will append the jump instruction to the kernel by
 * using the loop destination
 */
void Kernel::appendLoopInstruction()
{
    append(Instruction(_IJMPNZ, loopDestination, 0, 0));
}

vector<unsigned> &Kernel::getInstructions() {
    return instructions;
}

