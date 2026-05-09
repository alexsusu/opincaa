#include <iostream>
#include <fstream>
#include <thread>
#include <fcntl.h>
//#include <stdioh>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "ConnexSimulator.h"
//
#include "CheckDataHazard.h"
#include "Kernel.h" // 2020_04_24

extern ConnexState connexStateObj;


// Alex: changed connexVectors from unsigned short to ConnexVectorElementType

/****************************************************************************
 *  Structure holding information for a IO transfer
 */
struct ConnexIoDescriptor
{
    unsigned type;
    unsigned lsAddress;
    unsigned vectorCount;
};


/****************************************************************************
 *  Constructor for class ConnexSimulator
 *
 * @param distributionDescriptorPath the path to the distribution FIFO on the file system
 * @param reductionDescriptorPath the path to the reduction FIFO on the file system
 * @param writeDescriptorPath the path to the write FIFO (ARM -> ARRAY) on the file system
 * @param readDescriptorPath the path to the read FIFO (ARRAY -> ARM) on the file system
 * @param accInfoPath the path to the accelerator identification file on the file system
 */
ConnexSimulator::ConnexSimulator(string distributionDescriptorPath,
                                 string reductionDescriptorPath,
                                 string writeDescriptorPath,
                                 string readDescriptorPath,
                                 string accInfoPath) {
    int res;

    printf("ConnexSimulator::ConnexSimulator(): CONNEX_MEM_NUM_ROWS + CONNEX_MEM_NUM_ROWS_EXTRA = %d\n",
           CONNEX_MEM_NUM_ROWS + CONNEX_MEM_NUM_ROWS_EXTRA);
    printf("ConnexSimulator::ConnexSimulator(): CONNEX_MEM_SPILL_START_OFFSET = %d\n",
           CONNEX_MEM_SPILL_START_OFFSET);
    printf("ConnexSimulator::ConnexSimulator(): CONNEX_REG_COUNT = %d\n",
           CONNEX_REG_COUNT);

    localStore = new ConnexVector[CONNEX_MEM_NUM_ROWS + CONNEX_MEM_NUM_ROWS_EXTRA];
    printf("ConnexSimulator::ConnexSimulator(): Allocating registerFile\n");
    registerFile = new ConnexVector[CONNEX_REG_COUNT];

    /* VERY IMPORTANT: The ONLY thing that matters is the size of the buffer
     *   for reductionDescriptor, because sending the kernel for OPINCAA sim to
     *   distributionDescriptor is BLOCKING, so we need to make it as large as
     *   required the size of the reductionDescriptor pipe/FIFO to accomodate
     *   all results created while sending/executing the kernel.
     *  For e.g. MatMul-256.f16 we receive: 256*256 * 4 * 9 = 2359296 bytes.
     *    Note that we can specify for reductionDescriptor a smaller size than
     *      the total reduction results read because in output_main.cpp
     *      the executeKernel() will return while the kernel has e.g. 1M bytes
     *      of instructions to execute, so readReductionResults() will start
     *      receiving results much longer than before the kernel has finished,
     *      preventing a stall due to reductionDescriptor becoming full.
     * To allow such big pipes we also need to edit (it seems) the following
     *    file /proc/sys/fs/pipe-max-size with e.g.:
     *      sudo vi /proc/sys/fs/pipe-max-size
     *    and write e.g. 4194304.
     *
     *  Setting to values much larger than 1 MB (or 2 or 4 MB by changing also
     *     the value in /proc/sys/fs/pipe-max-size) such as 8 MB
     *     results in error, which seems to set the value actually to 64 KB.
     */

    // IMPORTANT: an instr is normally 4 bytes
    // This is to send the machine code instructions to the distribution network, which distributes the instr to each Connex lane
    cout << "Opening simu:" << distributionDescriptorPath << endl;
    distributionDescriptor = openPipe(distributionDescriptorPath, O_RDWR);
    // Using F_SETLEASE instead of F_SETPIPE_SZ seems to act the same: res = fcntl(distributionDescriptor, F_SETLEASE | F_GETFL | F_SETFL, 1048576); // 2020_04_25: Set a lease of 1 MB for this queue file
    res = fcntl(distributionDescriptor, F_SETPIPE_SZ | F_GETFL | F_SETFL, 1048576); // 2020_04_25: Set a lease of 1 MB for this queue file
    printf("For res = fcntl(distributionDescriptor, F_SETPIPE_SZ ...), res = %d\n", res);

    cout << "Opening simu:" << writeDescriptorPath << endl;
    writeDescriptor = openPipe(writeDescriptorPath, O_RDWR);

    //printf("EPERM = %d\n", EPERM);

    cout << "Opening simu:" << reductionDescriptorPath << endl;
    reductionDescriptor = openPipe(reductionDescriptorPath, O_RDWR);
    //printf("F_SETLEASE | F_GETFL | F_SETFL = %d\n", F_SETLEASE | F_GETFL | F_SETFL); // prints 1031
    // IMPORTANT: Normally the reduction results pipe size is 16*4 KB
    // Using F_SETLEASE instead of F_SETPIPE_SZ seems to act the same: res = fcntl(reductionDescriptor, F_SETLEASE | F_GETFL | F_SETFL, 3 * 1048576); // 2020_04_25: Set a lease of 2 MB for this queue file, as said by Andrei Popa on Hangouts on Aug 3 2019
    // Also good for MatMul-256.f16: res = fcntl(reductionDescriptor, F_SETPIPE_SZ | F_GETFL | F_SETFL, 2 * 1048576);
    //
    //#define RED_PIPE_SIZE 3 * 1048576
    // BAD: #define RED_PIPE_SIZE 8 * 1048576 // res = -1
    #define RED_PIPE_SIZE 4 * 1048576
    res = fcntl(reductionDescriptor, F_SETPIPE_SZ | F_GETFL | F_SETFL, RED_PIPE_SIZE);
    printf("For res = fcntl(reductionDescriptor, F_SETPIPE_SZ ...), res = %d\n", res);
    /*res = fcntl(reductionDescriptor, F_SETLEASE | F_GETFL | F_SETFL, 3 * 1048576);
    printf("For res = fcntl(reductionDescriptor, F_SETLEASE ...), res = %d\n", res);*/


    cout << "Opening simu:" << readDescriptorPath << endl;
    readDescriptor = openPipe(readDescriptorPath, O_RDWR);

    setupAccInfo(accInfoPath);

    internalInstructionMemory = new InternalInstructionMemory(INTERNAL_INSTRUCTION_MEMORY_SIZE);

    loopNestDepth = 0;
    for (int i = 0; i < MAX_DEPTH_LOOP_NESTING + 1; i++) {
        repeatCounterForLoopOfNestDepth[i] = 0;
        codeIn2ndLoopIterationForNestDepth[i] = false;
    }
    repeatCounterForLoopOfNestDepth[0] = -1;

    initiateThreads();
}

/****************************************************************************
 * Destructor for class ConnexSimulator
 */
ConnexSimulator::~ConnexSimulator() {
    // 2018_02_10
    delete localStore;
    delete registerFile;

    delete internalInstructionMemory;
}

/****************************************************************************
 * Opens the FIFO at the specified path in the specified mode
 *
 * @param path the path to the FIFO
 * @param mode the mode of the FIFO
 * @return the FIFO descriptor
 */
int ConnexSimulator::openPipe(string pipePath, int mode)
{
    int fifoDescriptor;
    const char* path = pipePath.c_str();
    /* Try and create the pipe, if it already exists, this will return
     * -1, but we don't care
     */
    mkfifo(path, 0666);

    /* Try and attach to it */
    if((fifoDescriptor = open(path, mode)) < 0)
    {
        throw string("Unable to open FIFO ") + path;
    }

    cout << "FIFO " << pipePath << " succesfully opened!" << endl << flush;
    return fifoDescriptor;
}

/****************************************************************************
 * Creates a file with the identification of the simulated accelerator
 *
 * @param path the path to the FIFO
 */
void ConnexSimulator::setupAccInfo(string infoPath)
{
    ofstream infoFile;

    infoFile.open(infoPath);
    string archName("connex16-hm-generic");
    archName = string(archName.rbegin(), archName.rend());

    infoFile << archName << '\0';
    for(int i=infoFile.tellp(); i<48; i++) infoFile << " ";
    infoFile.close();
}

/****************************************************************************
 * Starts the threads for program and data transfer.
 * They don't stop unless killed. (Alex: e.g. with killall from bash;
 *   the coreThreadHandler() loops forever).
 */
void ConnexSimulator::initiateThreads() {
    ioThread = thread(&ConnexSimulator::ioThreadHandler, this);
    coreThread = thread(&ConnexSimulator::coreThreadHandler, this);
}

void ConnexSimulator::waitFinish()
{
    ioThread.join();
    coreThread.join();
}

/****************************************************************************
 * Handler method for IO transfers
 */
void ConnexSimulator::ioThreadHandler()
{
    cout << "Starting IO Thread..." << endl << flush;
    ConnexIoDescriptor ioDescriptor;

    try {
        for (;;) {
            cout << "Simu: Waiting for receive " << endl << flush;
            ssize_t unusedRes = read(writeDescriptor, &ioDescriptor, sizeof(ioDescriptor));

          //#if 0
          #if 1
            printf("ioDescriptor:\n");
            printf("            type = %d\n", ioDescriptor.type);
            printf("            lsAddress = %d\n", ioDescriptor.lsAddress);
            printf("            vectorCount = %d\n", ioDescriptor.vectorCount);
            fflush(stdout);
          #endif

            //cout << "Simu: Received " << sizeof(ioDescriptor)
            //     << " Bytes" << endl << flush;
            performIO(ioDescriptor);
            //cout << "Simu: Perform IO " << endl << flush;
        }
    }
    // 2017_11_05
    catch(string ex) {
        cout << "Exception occured in ConnexSimulator::ioThreadHandler(): "
             << ex << endl;
        exit(-1);
    }
}

/****************************************************************************
 * Handler method execution and reduction
 */

// Printing the instruction trace getting executed
bool printDebugTrace = true; //false;

long numSimCycles = 0; // Current number of cycles simulated already
// The precision is too small s.t. after reaching 0.25 it gets stuck there (at least in certain cases): float totalEnergyConsumed = 0.0;
double totalEnergyConsumed = 0.0; // 0.25; //0.0;

// The value returned is in Joules
//  Note: We compute normally the dynamic energy consumption,
//    but we can in principle compute the total energy (with leakage and short
//    circuit).
double GetEnergyConsumptionInstruction(Instruction *instr) {
    // For CONNEX_VECTOR_LENGTH = 128:
    #define AVG_ENERGY_CONSUMPTION_INSTRUCTION 1.20990557e-8
    // Taken from 1.20990557e-8 = 0.30973585 / (32000 * 800 + 2), where 0.309736 is from /home/alarm/900_profile_energy/1ProfileInstrs/0ALL/3OR/STD_profileEngCons_002_OR_processed

    // For CONNEX_VECTOR_LENGTH = 32:
    //#define AVG_ENERGY_CONSUMPTION_INSTRUCTION (1.99650097809 * 0.38332205304 * 1.20990557e-8)

    // For CONNEX_VECTOR_LENGTH = 64:
    //#define AVG_ENERGY_CONSUMPTION_INSTRUCTION ((0.022218 - 0.00052566596 * 4) / 1312510)

    //float powerConsumptionInstruction;
//    float energyConsumptionInstruction = AVG_ENERGY_CONSUMPTION_INSTRUCTION;
//    float energyConsumptionInstructionGated = 0.7 * AVG_ENERGY_CONSUMPTION_INSTRUCTION;
    //float energyConsumptionInstruction[];

    double res;

    switch (instr->getOpcode()) {
        case _ADD:
            res = 12.10 * 1e-9;
            break;
        case _SUB:
            res = 12.20 * 1e-9;
            break;

        case _ADDC:
            res = 12.20 * 1e-9;
            break;
        case _SUBC:
            res = 12.20 * 1e-9;
            break;

        case _MULT:
        case _MULT_U:
            res = 12.60 * 1e-9;
            break;
        case _MULT_LO:
            res = 0.50 * 1e-9;
            break;
        case _MULT_HI:
            res = 0.50 * 1e-9;
            break;

        case _NOT:
            res = 7.50 * 1e-9;
            break;
        case _OR:
            res = 11.50 * 1e-9;
            break;
        case _AND:
            res = 11.80 * 1e-9;
            break;
        case _XOR:
            res = 12.50 * 1e-9;
            break;

        case _SHL:
            res = 12.00 * 1e-9;
            break;
        case _ISHL:
            res = 12.10 * 1e-9;
            break;
        case _SHR:
            res = 12.00 * 1e-9;
            break;
        case _ISHR:
            res = 12.00 * 1e-9;
            break;
        case _SHRA:
            res = 12.40 * 1e-9; // MEGA-TODO: check if correct (takedn from ISHRA)
            break;
        case _ISHRA:
            res = 12.40 * 1e-9;
            break;

        case _POPCNT:
            res = 7.40 * 1e-9;
            break;


        case _EQ:
            res = 10.50 * 1e-9;
            break;
        case _LT:
            res = 10.80 * 1e-9;
            break;
        case _ULT:
            res = 10.80 * 1e-9;
            break;

        case _IREAD:
            res = 2.60 * 1e-9;
            break;
        case _READ:
            //res = 7.04 * 1e-9;
            res = 5.60 * 1e-9;
            break;

        case _IWRITE:
            res = 9.90 * 1e-9;
            break;
        case _WRITE:
            res = 12.80 * 1e-9;
            break;

        case _LDIX:
            res = 0.50 * 1e-9;
            break;
        case _VLOAD:
            res = 8.40 * 1e-9;
            break;

        case _NOP:
            //res = 0.021 * 1e-9;
            //res = 0.031 * 1e-9;
            res = 0.1 * 1e-9;
            break;

        case _CELL_SHL:
            //res = 0.09 * 1e-9;
            res = 0.2 * 1e-9;
            break;
        case _CELL_SHR:
            //res = 0.06 * 1e-9;
            res = 0.2 * 1e-9;
            break;
        case _LDSH: // SHIFT_REG:
            //res = 0.06 * 1e-9;
            res = 0.2 * 1e-9;
            break;

        case _RED:
        case _RED_U:
            //res = 22.766471844 * 1e-9;
            res = 55.5 * 1e-9;
            break;

        case _SCAN:
            res = 2 * 55.5 * 1e-9; // MEGA TODO: compute energy consumption
            break;


        case _IJMPNZ:
            //res = 0.163 * 1e-9;
            res = 0.15 * 1e-9;
            break;
        case _SETLC:
            //res = 0.163 * 1e-9;
            res = 0.15 * 1e-9;
            break;
        case _SETLC_REDUCE:
            //res = 0.163 * 1e-9;
            res = 0.15 * 1e-9; // MEGA TODO: compute energy consumption
            break;
        /*
        case _SETLC_REDUCE_NOTNULL:
            //res = 0.163 * 1e-9;
            res = 0.15 * 1e-9; // MEGA TODO: compute energy consumption
            break;
        */
        case _IJMPNZ_RED:
            //res = 0.163 * 1e-9;
            res = 0.15 * 1e-9; // MEGA TODO: compute energy consumption
            break;

        case _WHERE_CRY:
        case _WHERE_EQ:
        case _WHERE_LT:
        case _END_WHERE:
            //res = 0.09 * 1e-9;
            res = 0.10 * 1e-9;
            break;

        case _PRINT_REG:
        case _PRINT_CHARS:
            res = 0.0;
            break;

        default:
            printf("Executing unmodeled opcode %d (%s)\n",
                   instr->getOpcode(), instr->dump().c_str());
            fflush(stdout);
            assert(0);
    }


    //res = 1.33085e-08; // 1.45638331e-8; // For CVL = 128
    //res = 1.0751e-08; // For CVL = 64
    //res = 9.74908e-09; //1.02303074e-8; // For CVL = 32

   //#define PRINT_DEBUG_ENG_PROFILE
   #ifdef PRINT_DEBUG_ENG_PROFILE
    printf("GetEnergyConsumptionInstruction(): instr = %s, res = %g\n", //%.12f\n",
           instr->dump().c_str(), res);
   #endif

    return res;
}

void ConnexSimulator::coreThreadHandler() {
    InstructionType instruction;

    cout << "Starting Core Thread..." << endl << flush;

    // IMPORTANT: For all lanes we initialize cellDisabled to 0
    ConnexVector::Unconditioned_Set(connexStateObj.cellDisabled, 0);

    Instruction prevInstruction((InstructionType)_NOP);

    /*
    printf("At the beginning number of cycles simulated = %d, "
           "totalEnergyConsumed = %.12f\n",
           numSimCycles,
           totalEnergyConsumed);
    fflush(stdout);
    */

    try {
        /*
        We now read the instructions in the loop once (for 1 iteration)
          from the distributionFIFO pipe (with a simple read() command)
          and stores them for the next iterations by executing:
            internalInstructionMemory->push(compiledInstruction)
            (to be able to reexecute the loop).

          Note: it seems RaduH & CalinB want to emulate the Verilog IIM with this internalInstructionMemory.
            We could also have implemented the instruction memory as an unlimited
             vector<InstructioType> and use an IP pointer for the current instruction.

        Then we make codeIn2ndLoopIterationForNestDepth = true at the 2nd iteration of the loop
            when the loop instructions are in the internalInstructionMemory and it does.
        */
        for (;;) {
            /*
            printf("  loopNestDepth = %d\n", loopNestDepth);
            fflush(stdout);
            */

            Instruction *compiledInstruction;

            // Note: we set codeIn2ndLoopIterationForNestDepth when we encounter IJMPNZ instruction and repeateCounter != 0
            //
            // MEGA-TODO: We need to check that the current codeIn2ndLoopIterationForNestDepth[loopNestDepth] == true
            //    OR any codeIn2ndLoopIterationForNestDepth[i] == true where 1 <= i < loopNestDepth
            if (codeIn2ndLoopIterationForNestDepth[loopNestDepth] == true ||
                // TODO: do for (int i = 0; i < MAX_DEPTH_LOOP_NESTING + 1; i++) codeIn2ndLoopIterationForNestDepth[i] == true ||
                codeIn2ndLoopIterationForNestDepth[3] == true ||
                codeIn2ndLoopIterationForNestDepth[2] == true ||
                codeIn2ndLoopIterationForNestDepth[1] == true) {
                //printf("At the beginning of: if (codeIn2ndLoopIterationForNestDepth == true)...\n");
                //fflush(stdout);

                compiledInstruction = internalInstructionMemory->read();
                dprintf("  compiledInstruction = %p\n", compiledInstruction);
                dfflush(stdout);

                CheckDataHazard(*compiledInstruction, prevInstruction,
                                true, true);
                prevInstruction = *compiledInstruction;

                if (printDebugTrace) {
                  #ifdef DEBUG_OPINCAA_PRINT_SIM_MORE
                     if (compiledInstruction->mnemonic(
                           compiledInstruction->getOpcode()) != "print_chars") {
                         cout << " Running " << compiledInstruction->dump() // toString() // 2019_09_26
                              << endl;
                         fflush(stdout);
                     }
                  #endif
                }

                if (printDebugTrace) {
                  // 2017_11_05
                  #ifdef DEBUG_OPINCAA_PRINT_SIM
                    if (compiledInstruction->getOpcode() == _IJMPNZ) {
                        cout << "Preparing to jump from loop iter #"
                             << repeatCounterForLoopOfNestDepth[loopNestDepth] << endl;
                        fflush(stdout);
                    }

                    //cout << " Loop running "
                    //     << compiledInstruction->toString() << endl;
                    if (compiledInstruction->mnemonic(
                          compiledInstruction->getOpcode()) != "print_chars") {
                        // cout << "  instr " << compiledInstruction->dump()
                        //      << "" << endl;
                        printf("  instr = %s\n",
                               compiledInstruction->dump().c_str());
                        fflush(stdout);
                    }
                  #endif
                    /*
                    cout << " R0 = " << this->registerFile->getCellValue(0)
                         << endl;
                    cout << " R0(elem 0) = "
                         << this->registerFile[0].getCellValue(0) << endl;
                    // Alex: new code
                    cout << " R1(elem 0) = "
                         << this->registerFile[1].getCellValue(1) << endl;
                    */
                }

                executeInstruction(*compiledInstruction);

                /*
                cout << " Loop running "
                     << compiledInstruction->toString() << endl;
                cout <<" R0 =" << this->registerFile->cells[0] << endl;
                */
                if (printDebugTrace) {
                  // 2017_11_05
                  #ifdef DEBUG_OPINCAA_PRINT_SIM_CYCLES
                    if (compiledInstruction->getOpcode() == _RED ||
                        compiledInstruction->getOpcode() == _RED_U) {
                        printf("At RED[_U]: "
                               "numSimCycles = %ld, "
                               "totalEnergyConsumed = %.9f\n",
                               numSimCycles + 1, totalEnergyConsumed);
                        //fflush(stdout);
                    }
                  #endif
                }
            } // END if (codeIn2ndLoopIterationForNestDepth[] == true)
            else {
                ssize_t unusedRes = read(distributionDescriptor, &instruction, sizeof(instruction));

                compiledInstruction = new Instruction((InstructionType)instruction);

              #if 0 // 2019_05_26
                if (compiledInstruction->getOpcode() == _SETLC) {
                    printf("ConnexSimulator::coreThreadHandler(): removing 1 duplicate _SETLC\n");

                    read(distributionDescriptor, &instruction, sizeof(instruction));

                    compiledInstruction = new Instruction((InstructionType)instruction);
                    assert(compiledInstruction->getOpcode() == _SETLC);
                }
              #endif

                CheckDataHazard(*compiledInstruction, prevInstruction,
                                true, true);
                prevInstruction = *compiledInstruction;

                internalInstructionMemory->push(compiledInstruction);

                if (printDebugTrace) {
                  #ifdef DEBUG_OPINCAA_PRINT_SIM_MORE
                    if (compiledInstruction->mnemonic(
                            compiledInstruction->getOpcode()) != "print_chars") {
                       cout << " Running " << compiledInstruction->dump() // toString() // 2020_04_20
                            << endl;
                    }
                  #endif
                }

                //assert(compiledInstruction->getOpcode() != _IJMPNZ);
                if (printDebugTrace) {
                  #ifdef DEBUG_OPINCAA_PRINT_SIM
                    if (compiledInstruction->getOpcode() == _IJMPNZ) {
                        cout << "Preparing to jump from loop iter #"
                             << repeatCounterForLoopOfNestDepth[loopNestDepth] << endl;
                    }
                  #endif
                }

                executeInstruction(*compiledInstruction);

                if (printDebugTrace) {
                  // 2017_11_05
                  #ifdef DEBUG_OPINCAA_PRINT_SIM
                    if (compiledInstruction->mnemonic(
                            compiledInstruction->getOpcode()) != "print_chars")
                        cout << " Finished running "
                             << compiledInstruction->dump()
                             << endl << std::flush;
                  #endif
                    // cout << " R0 =" << this->registerFile->cells[0] << endl;
                    // cout << " R0(elem 0) = "
                    //      << this->registerFile[0].getCellValue(0) << endl;
                    /*
                    printRegister(22);
                    printRegister(20);
                    printRegister(23);
                    */
                    // Alex: new code
                    // cout << " R1(elem 1) = "
                    //      << this->registerFile[1].getCellValue(1) << endl;
                }
                // cout << " Running "
                //      << compiledInstruction->toString() << endl;
                // cout << " R0 =" << this->registerFile->cells[0] << endl;

                if (printDebugTrace) {
                  // 2017_11_05
                  #ifdef DEBUG_OPINCAA_PRINT_SIM_CYCLES
                    if (compiledInstruction->getOpcode() == _RED ||
                        compiledInstruction->getOpcode() == _RED_U) {
                        printf("At RED[_U] out of REPEAT loop: "
                               "numSimCycles = %ld, "
                               "totalEnergyConsumed = %.9f\n",
                               numSimCycles + 1, totalEnergyConsumed);
                        fflush(stdout);
                    }
                  #endif
                }
            } // END else if (codeIn2ndLoopIterationForNestDepth == true)

            numSimCycles++;


            int numActiveLanes = connexStateObj.active.getNumberActiveLanes();
    //assert(numActiveLanes == CONNEX_VECTOR_LENGTH);
    /*
    printf("numActiveLanes = %d\n", numActiveLanes);
    printf("CONNEX_VECTOR_LENGTH = %d\n", CONNEX_VECTOR_LENGTH);
    */
            double energyConsumptionInstruction128 = GetEnergyConsumptionInstruction(compiledInstruction);
            //double energyConsumptionInstructionGated128 = 0.7 * energyConsumptionInstruction128;
            //double energyConsumptionInstructionGated128 = 0.8 * energyConsumptionInstruction128;
            double energyConsumptionInstructionGated128 = 1.0 * energyConsumptionInstruction128; // The hacker seems to prevent/forbid lane getting for WHERE instructions
    //printf("... = %.20f\n", (energyConsumptionInstruction128 / CONNEX_VECTOR_LENGTH) * numActiveLanes);
            totalEnergyConsumed += numActiveLanes *
                                    (energyConsumptionInstruction128 / CONNEX_VECTOR_LENGTH) +
                                    (CONNEX_VECTOR_LENGTH - numActiveLanes) *
                                      (energyConsumptionInstructionGated128 / CONNEX_VECTOR_LENGTH);
           #ifdef PRINT_DEBUG_ENG_PROFILE
            printf("numSimCycles = %ld: ran %s (totalEnergyConsumed = %.12f)\n",
                   numSimCycles,
                   compiledInstruction->dump().c_str(),
                   totalEnergyConsumed);
            /*
            printf("  Number of cycles simulated = %d, "
                     "numActiveLanes = %d, "
                     "totalEnergyConsumed = %.12f\n",
                   numSimCycles,
                   numActiveLanes,
                   totalEnergyConsumed);
            */
            fflush(stdout);
           #endif
        } // END forever loop
    }
    // 2017_11_05
    catch (string ex) {
        cout << "Exception occured in ConnexSimulator::coreThreadHandler(): "
             << ex << endl;
        exit(-1);
    }
}


ssize_t
force_all_io(ssize_t (*io)(int, void*, size_t), int fd, void *buf,
                           size_t count) {
    ssize_t total = 0;

    do {
        printf("ConnexSimulator::force_all_io(): count = %lu, total = %ld\n",
               count, total);
        fflush(stdout);

        total += io(fd, (char *)buf + total, count - total);
    } while (total != count);

    return total;
}

/****************************************************************************
 * Performs an IO operation specified by the IO descriptor
 *
 * @param ioDescriptor the descriptor for the IO operation
 */
void ConnexSimulator::performIO(ConnexIoDescriptor ioDescriptor) {
    printf("Entered ConnexSimulator::performIO(): "
           "ioDescriptor.type = %d, ioDescriptor.lsAddress = %d, ioDescriptor.vectorCount = %d\n",
             ioDescriptor.type, ioDescriptor.lsAddress, ioDescriptor.vectorCount);
    fflush(stdout);


    switch (ioDescriptor.type) {
        /* Write (from IO FIFO/pipe, normally data coming from CPU RAM) to
             Connex LS memory */
        case IO_WRITE_OPERATION: {
            /* From https://www.quora.com/How-can-I-use-variable-length-array-in-C++:
               (May 2016)
               "C++ doesn't support variable length array like C.
                Variable length array is C99 feature but it is not officially
                  part  of C++ so far.
                But compilers like g++ & clang++ allows Variable Length Arrays
                  (VLA) as an extension."
            //ConnexVectorElementType connexVectors[CONNEX_VECTOR_LENGTH * (ioDescriptor.vectorCount + 1)];
            */
            ConnexVectorElementType *connexVectors = new ConnexVectorElementType[
                                                  CONNEX_VECTOR_LENGTH *
                                                    (ioDescriptor.vectorCount + 1)];

            printf("  ConnexSimulator::performIO(): Before force_all_io()\n");
            fflush(stdout);

            //force_all_io(read, writeDescriptor, connexVectors, sizeof(connexVectors));
            force_all_io(read, writeDescriptor, connexVectors,
                         sizeof(ConnexVectorElementType) * CONNEX_VECTOR_LENGTH *
                           (ioDescriptor.vectorCount + 1));

            for (unsigned int i = 0; i < ioDescriptor.vectorCount + 1; i++) {
                localStore[ioDescriptor.lsAddress + i].write(connexVectors +
                                                             i * CONNEX_VECTOR_LENGTH);
            }

            // TODO: Write the ACK with the correct data
            ssize_t unusedRes = write(readDescriptor, connexVectors, 4);
            unusedRes = write(readDescriptor, NULL, 0);

            delete connexVectors;
            break;
        }

        // Read from Connex LS memory (and put in IO FIFO/pipe, normally data for CPU RAM)
        case IO_READ_OPERATION: {
            for (unsigned int i = 0; i < ioDescriptor.vectorCount + 1; i++) {
                force_all_io( (ssize_t (*)(int, void *, size_t)) write,
                              readDescriptor,
                              localStore[ioDescriptor.lsAddress + i].read(),
                              sizeof(ConnexVectorElementType) * CONNEX_VECTOR_LENGTH);
            }
            ssize_t unusedRes = write(readDescriptor, NULL, 0);
            break;
        }

        default:
            throw string("Unknown IO operation type in "
                         "ConnexSimulator::performIO");
    }

}

//bool stopSimAtExecuteKernel = false; // 2020_03_29
/****************************************************************************
 * Executes the specified instruction on all active cells
 *
 * @param instruction the instruction to execute
 */
void ConnexSimulator::executeInstruction(Instruction instruction) {
    /*
    // 2020_03_29
    if (stopSimAtExecuteKernel == true) {
        if (numSimCycles >= 2000) {
            // Simulating only first 2000 cycles to execute Init_DIVf16 and
            //   other similar kernels, but not the main kernels.
            throw string("ConnexSimulator::executeInstruction(): "
                         "Request to NOT execute kernel --> halting.");
        }
    }
    */

  #ifdef DEBUG_OPINCAA_PRINT_SIM_EXTRA_USELESS
    printf("Entered ConnexSimulator::executeInstruction(): "
           "instruction.getOpcode() = %d\n", instruction.getOpcode());
    fflush(stdout);
    cout << instruction.disassemble() << flush;
  #endif

  #ifdef DEBUG_OPINCAA_PRINT_TRACE_REG_VALUES
    static char strDestBefore[10000];
    strDestBefore[0] = 0;

    if (instruction.getDest() != NO_REG_INDEX) {
        sprintf(strDestBefore, " Before: dest = ");
        for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
            sprintf(strDestBefore + strlen(strDestBefore), "%04hx ",
                    registerFile[instruction.getDest()].getCellValue(i));
        }
        sprintf(strDestBefore + strlen(strDestBefore), "\n");
    }
  #endif

  #ifdef DEBUG_OPINCAA_PRINT_SIM_EXTRA_USELESS
    //printf("instruction (0x%08x):\n", instruction.assemble());
    printf("instruction:\n");
    printf("  opcode = %d\n", instruction.getOpcode());
    printf("  dest = %d\n", instruction.getDest());
    printf("  left = %d\n", instruction.getLeft());
    printf("  right = %d\n", instruction.getRight());
    printf("  value = %d\n", instruction.getValue());
    fflush(stdout);
  #endif

    switch (instruction.getOpcode()) {
        // Alex: adding support for debugging:
        case _PRINT_REG:
            printRegister(instruction.getLeft());
            break;

        case _PRINT_CHARS:
            printf("%c%c",
                     (char)(instruction.getValue() >> 8),
                     (char)(instruction.getValue() & 0xFF));
            break;

        case _ADD:
            registerFile[instruction.getDest()] =
                registerFile[instruction.getLeft()] +
                registerFile[instruction.getRight()];
            break;

        case _ADDC:
            /* 2018_03_26: Alex - making it work well with the initial value of
               the Carry flags (before I was adding the Carry after the add
               with the ADDC operands and this was resetting the initial value
               of the Carry flags).
            */
            registerFile[instruction.getDest()] =
                (connexStateObj.carryFlag + registerFile[instruction.getLeft()]) +
                    registerFile[instruction.getRight()];
            break;

        case _SUB:
            registerFile[instruction.getDest()] =
                registerFile[instruction.getLeft()] -
                registerFile[instruction.getRight()];
            break;

        case _SUBC:
            /* 2018_04_30: Alex - making it work well with the initial value of
               the Carry flags (before I was subtracting the Carry after the
               sub with the SUBC operands and this was resetting the initial
               value of the Carry flags).
            */
            registerFile[instruction.getDest()] =
                registerFile[instruction.getLeft()] - connexStateObj.carryFlag -
                    registerFile[instruction.getRight()];
            break;

        case _POPCNT:
            registerFile[instruction.getDest()] =
                registerFile[instruction.getLeft()].popcount();
            break;

        case _BIT_REVERSE:
            registerFile[instruction.getDest()] =
                registerFile[instruction.getLeft()].bitreverse();
            break;

        case _NOT:
            registerFile[instruction.getDest()] = ~registerFile[instruction.getLeft()];
            break;

        case _OR:
            registerFile[instruction.getDest()] =
                registerFile[instruction.getLeft()] | registerFile[instruction.getRight()];
            break;

        case _AND:
            registerFile[instruction.getDest()] =
                registerFile[instruction.getLeft()] & registerFile[instruction.getRight()];
            break;

        case _XOR:
            registerFile[instruction.getDest()] =
                registerFile[instruction.getLeft()] ^ registerFile[instruction.getRight()];
            break;

        case _EQ:
            registerFile[instruction.getDest()] =
                registerFile[instruction.getLeft()] == registerFile[instruction.getRight()];
            break;

        case _LT:
            registerFile[instruction.getDest()] =
                registerFile[instruction.getLeft()] < registerFile[instruction.getRight()];
            break;

        case _ULT:
            registerFile[instruction.getDest()] =
                registerFile[instruction.getLeft()].ult(registerFile[instruction.getRight()]);
            break;

        case _SHL:
            registerFile[instruction.getDest()] =
                //registerFile[instruction.getLeft()] << registerFile[instruction.getRight()];
                registerFile[instruction.getLeft()].shl(registerFile[instruction.getRight()]);
            break;

        case _SHR:
            registerFile[instruction.getDest()] =
                registerFile[instruction.getLeft()].shr(registerFile[instruction.getRight()]);
            break;

        case _SHRA:
            registerFile[instruction.getDest()] =
                //registerFile[instruction.getLeft()] >> registerFile[instruction.getRight()];
                registerFile[instruction.getLeft()].shra(registerFile[instruction.getRight()]);
            break;

        /* Alex: VERY IMPORTANT: even if the ISHL/R instructions are
           immediate, they are of type INSTRUCTION_TYPE_NO_IMMEDIATE
            - the immediate value is stored in the 5 bits of the
            right register, since it is enough for the delta operand
            of SHIFT operations (normally values 0-16 is enough). */
        case _ISHL:
            registerFile[instruction.getDest()] =
                registerFile[instruction.getLeft()] << instruction.getValue();
            break;

        case _ISHR:
            registerFile[instruction.getDest()] =
                registerFile[instruction.getLeft()] >> instruction.getValue();
            break;

        case _ISHRA:
            registerFile[instruction.getDest()] =
                registerFile[instruction.getLeft()].ishra(instruction.getValue());
            break;

        case _LDIX:
            registerFile[instruction.getDest()].loadIndex();
            break;

        case _LDSH: {
            // 2018_03_27:
            registerFile[instruction.getDest()] = connexStateObj.shiftReg;
            break;
        }

        case _WHERE_CRY:
            ConnexVector::Unconditioned_Set(connexStateObj.active, connexStateObj.carryFlag);
            break;

        case _WHERE_EQ:
            ConnexVector::Unconditioned_Set(connexStateObj.active, connexStateObj.eqFlag);
            break;

        case _WHERE_LT:
            ConnexVector::Unconditioned_Set(connexStateObj.active, connexStateObj.ltFlag);
            break;

        case _END_WHERE:
            ConnexVector::Unconditioned_Set(connexStateObj.active, 1);

          #ifdef DEBUG_OPINCAA_EXTRA_ACTIVE
            printf("connexStateObj.active = ");
            connexStateObj.active.Print();
            printf("connexStateObj.cellDisabled = ");
            connexStateObj.cellDisabled.Print();
          #endif

            break;

        case _CELL_SHL:
        case _CELL_SHR:
            handleShift(instruction);
            break;

        case _READ:
            registerFile[instruction.getDest()].loadFrom(localStore,
                                        registerFile[instruction.getRight()]);
            break;

        case _WRITE:
            registerFile[instruction.getLeft()].storeTo(localStore,
                                        registerFile[instruction.getRight()]);
            break;

        case _MULT:
            registerFile[instruction.getLeft()] * registerFile[instruction.getRight()];
            break;

        case _MULT_U:
            registerFile[instruction.getLeft()].mult_u( registerFile[instruction.getRight()] );
            break;

        case _MULT_LO:
            registerFile[instruction.getDest()] = connexStateObj.multLow;
            break;

        case _MULT_HI:
            registerFile[instruction.getDest()] = connexStateObj.multHigh;
            break;

        case _RED:
            handleReduction(instruction);
            break;

        case _RED_U:
            handleReduction_u(instruction);
            break;

        case _SCAN:
            handleScan(instruction);
            break;

        case _NOP:
            break;

        case _QUIT:
            exit(0);

        case _VLOAD:
            registerFile[instruction.getDest()] =
                                        (ConnexVectorElementType)instruction.getValue();
            break;

        case _IREAD: {
            //ConnexVectorElementType addr = instruction.getValue();
            unsigned int addr = instruction.getValue();
            addr &= 0xFFFF;
            //printf("addr = %u\n", addr);

            // Alex: checking for out-of-bounds case
            if ( !(addr >= 0 && addr < CONNEX_MEM_NUM_ROWS + CONNEX_MEM_NUM_ROWS_EXTRA))
                printf("iread access outside of bounds of Connex LS memory: "
                       "addr = %d\n", addr);
            assert(addr >= 0 && addr < CONNEX_MEM_NUM_ROWS + CONNEX_MEM_NUM_ROWS_EXTRA &&
                   "iread access outside of bounds of Connex LS memory");

          #ifdef DEBUG_OPINCAA_EXTRA_ACTIVE
            printf("connexStateObj.active = ");
            connexStateObj.active.Print();
            printf("connexStateObj.cellDisabled = ");
            connexStateObj.cellDisabled.Print();
          #endif

            registerFile[instruction.getDest()] = localStore[addr];
            break;
        }

        case _IWRITE: {
            //ConnexVectorElementType addr = instruction.getValue();
            unsigned int addr = instruction.getValue();
            //printf("addr = %u\n", addr); fflush(stdout);
            addr &= 0xFFFF;
            //printf("addr = %u\n", addr); fflush(stdout);

            // Alex: checking for out-of-bounds case
            if ( !(addr >= 0 && addr < CONNEX_MEM_NUM_ROWS + CONNEX_MEM_NUM_ROWS_EXTRA)) {
                printf("iwrite access outside of bounds of Connex LS memory: "
                       "addr = %d\n", addr);
                fflush(stdout);
            }
            assert(addr >= 0 && addr < CONNEX_MEM_NUM_ROWS + CONNEX_MEM_NUM_ROWS_EXTRA &&
                   "iwrite access outside of bounds of Connex LS memory");

            localStore[addr] = registerFile[instruction.getLeft()];
            break;
        }

        case _SETLC: {
            static int countSetlc = 0; // It seems the simulator jumps the 1st REPEAT???

//            if (codeIn2ndLoopIterationForNestDepth == true) {
            cout << "Running " << instruction.dump() // toString() // 2020_04_20
                 << " [END]" << endl;
            fflush(stdout);

            printf("  ConnexSimulator::executeInstruction(): tracking 1st duplicate _SETLC: "
                  "countSetlc = %d\n", countSetlc);

            if (countSetlc == 0) {
                printf("  Executing loopNestDepth++\n");
                loopNestDepth++;
            }

            printf("  ConnexSimulator::executeInstruction(): case _SETLC: "
                   "loopNestDepth = %d, countSetlc = %d, "
                   "codeIn2ndLoopIterationForNestDepth[loopNestDepth] = %d\n",
                   loopNestDepth, countSetlc,
                   codeIn2ndLoopIterationForNestDepth[loopNestDepth]);
            fflush(stdout);
            assert(loopNestDepth >= 0 && loopNestDepth < MAX_DEPTH_LOOP_NESTING);

            countSetlc++;
            if (countSetlc == 2)
                countSetlc = 0;

            repeatCounterForLoopOfNestDepth[loopNestDepth] = instruction.getValue();

            if (loopNestDepth <= 1)
                // Initializing startReadPointerForLoopOfNestDepth[] array once per loop nest
                //for (int i = 0; i < MAX_DEPTH_LOOP_NESTING + 1; i++)
                for (int i = 1; i < MAX_DEPTH_LOOP_NESTING + 1; i++) { // 2020_04_26
                    startReadPointerForLoopOfNestDepth[i] = -1; // 2020_04_20
                }

            //startReadPointerForLoopOfNestDepth[loopNestDepth] = internalInstructionMemory->getReadPointer(); // 2020_04_20
//            }

            printf("  ConnexSimulator::executeInstruction(): case _SETLC: "
                   "set repeatCounterForLoopOfNestDepth[loopNestDepth = %d] = %d\n",
                   loopNestDepth, repeatCounterForLoopOfNestDepth[loopNestDepth]);
            fflush(stdout);
            break;
        }
        case _SETLC_REDUCE: {
            // Alex: Experimental: We perform Max-reduce on the vector getLeft()
            ConnexVectorElementType numIterations;
            numIterations = MIN_CONNEX_VECTOR_ELEMENT_TYPE;
            for (int idxMaxRed = 0; idxMaxRed < CONNEX_VECTOR_LENGTH; idxMaxRed++) {
                ConnexVectorElementType valCrt = registerFile[instruction.getLeft()].getCellValue(idxMaxRed);
                if (numIterations < valCrt)
                    numIterations = valCrt;
            }

            assert(numIterations >= 0);
            repeatCounterForLoopOfNestDepth[loopNestDepth] = numIterations;

            printf("ConnexSimulator::executeInstruction(): case _SETLC_REDUCE: "
                   "set repeatCounterForLoopOfNestDepth[loopNestDepth = %d] = %d\n",
                   loopNestDepth, numIterations);

            // We set also R0 to numIterations
            registerFile[0] = numIterations;

            //return;
            break;
        }
        /*
        case _SETLC_REDUCE_NOTNULL: {
            // Alex: Experimental: We perform Max-reduce on the vector getLeft()
            ConnexVectorElementType numIterations;
            numIterations = MIN_CONNEX_VECTOR_ELEMENT_TYPE;
            for (int idxMaxRed = 0; idxMaxRed < CONNEX_VECTOR_LENGTH; idxMaxRed++) {
                ConnexVectorElementType valCrt = registerFile[instruction.getLeft()].getCellValue(idxMaxRed);
                if (numIterations < valCrt)
                    numIterations = valCrt;
            }

            //assert(numIterations >= 0);
            repeatCounterForLoopOfNestDepth[loopNestDepth] = numIterations;

            printf("ConnexSimulator::executeInstruction(): case _SETLC_REDUCE_NOTNULL: "
                   "set repeatCounterForLoopOfNestDepth[loopNestDepth = %d] = %d\n",
                   loopNestDepth, numIterations);

            // We set also R0 to numIterations
            registerFile[0] = numIterations;

            //return;
            break;
        }
        */
        case _IJMPNZ: {
            if (repeatCounterForLoopOfNestDepth[loopNestDepth] == 0) {
                codeIn2ndLoopIterationForNestDepth[loopNestDepth] = false; // maybe-TODO: should check loopNestDepth > 0
                printf("executeInstruction(): IJMPNZ case zero: before dec loopNestDepth = %d\n",
                       loopNestDepth);
                loopNestDepth--;

                numSimCycles += 2;
                printf("executeInstruction(): Accounting for 2 cycles overhead for end of REPEAT loop.\n");
                //fflush(stdout);
            }
            else {
                printf("executeInstruction(): IJMPNZ case != 0: before dec repeatCounterForLoopOfNestDepth[loopNestDepth = %d] = %d\n",
                       loopNestDepth, repeatCounterForLoopOfNestDepth[loopNestDepth]);
                repeatCounterForLoopOfNestDepth[loopNestDepth]--;
                codeIn2ndLoopIterationForNestDepth[loopNestDepth] = true;

                dprintf("executeInstruction(): IJMPNZ case != 0: displaceReadPointer instruction.getValue() = %ld\n",
                        instruction.getValue());

                // 2020_04_20:
                dprintf("executeInstruction(): startReadPointerForLoopOfNestDepth[%d] = %d\n",
                        loopNestDepth, startReadPointerForLoopOfNestDepth[loopNestDepth]);
                dfflush(stdout);
                if (startReadPointerForLoopOfNestDepth[loopNestDepth] == -1) {
                    internalInstructionMemory->displaceReadPointer(instruction.getValue());

//                    startReadPointerForLoopOfNestDepth[loopNestDepth] = internalInstructionMemory->getReadPointer(); // 2020_04_20
                    dprintf("executeInstruction(): after setting it, startReadPointerForLoopOfNestDepth[%d] = %d\n",
                            loopNestDepth,
                            startReadPointerForLoopOfNestDepth[loopNestDepth]);
                    dfflush(stdout);
                }
                else {
                    internalInstructionMemory->setReadPointer(startReadPointerForLoopOfNestDepth[loopNestDepth]);
                }
            }
            //return;
            break;
        }
        case _IJMPNZ_RED: {
            printf("_IJMPNZ_RED: instruction.getLeft() = %d\n", instruction.getLeft());
            printf("_IJMPNZ_RED: loopNestDepth = %d\n", loopNestDepth);
            fflush(stdout);

loopNestDepth = 1;
/*
startReadPointerForLoopOfNestDepth[loopNestDepth] = -1; // MEGA-TODO: need to make it -1 - don't understand why
*/
            printf("_IJMPNZ_RED: after adjusting, loopNestDepth = %d\n", loopNestDepth);
            fflush(stdout);

            // Alex: Experimental: We perform sum-reduce on the vector getLeft()
            ConnexVectorElementType sumRedRes;
            sumRedRes = 0;
            for (int idxRed = 0; idxRed < CONNEX_VECTOR_LENGTH; idxRed++) {
                ConnexVectorElementType valCrt = registerFile[instruction.getLeft()].getCellValue(idxRed);
                sumRedRes += valCrt;
            }

            if (sumRedRes == 0) {
                codeIn2ndLoopIterationForNestDepth[loopNestDepth] = false; // maybe-TODO: should check loopNestDepth > 0
                printf("executeInstruction(): IJMPNZ_RED case zero: before dec loopNestDepth = %d\n",
                       loopNestDepth);
                loopNestDepth--;

                numSimCycles += 2;
                printf("executeInstruction(): Accounting for 2 cycles overhead for end of REPEAT loop.\n");
                //fflush(stdout);
            }
            else {
                printf("executeInstruction(): IJMPNZ_RED case sumRedRes != 0\n");
                fflush(stdout);
                //repeatCounterForLoopOfNestDepth[loopNestDepth]--;
                codeIn2ndLoopIterationForNestDepth[loopNestDepth] = true;

                printf("executeInstruction(): IJMPNZ_RED case != 0: displaceReadPointer instruction.getValue() = %ld\n",
                       instruction.getValue());
                fflush(stdout);

                // 2020_04_20:
                printf("executeInstruction(): startReadPointerForLoopOfNestDepth[%d] = %d\n",
                       loopNestDepth, startReadPointerForLoopOfNestDepth[loopNestDepth]);
                fflush(stdout);
//                if (startReadPointerForLoopOfNestDepth[loopNestDepth] == -1) {
                    internalInstructionMemory->displaceReadPointer(instruction.getValue());

                    //startReadPointerForLoopOfNestDepth[loopNestDepth] = internalInstructionMemory->getReadPointer(); // 2020_04_20
                    printf("executeInstruction(): after setting it, startReadPointerForLoopOfNestDepth[%d] = %d\n",
                           loopNestDepth,
                           startReadPointerForLoopOfNestDepth[loopNestDepth]);
                    fflush(stdout);
/*
                }
                else {
                    internalInstructionMemory->setReadPointer(startReadPointerForLoopOfNestDepth[loopNestDepth]);
                }
*/
            }
            //return;
            break;
        }

        case _DISABLE_CELL: {
            /*
            for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++)
                connexStateObj.cellDisabled.cells[i] = 0;
            */
            //ConnexVector::Unconditioned_Set(connexStateObj.cellDisabled, connexStateObj.active);

            // IMPORTANT: Only for the active lanes we set cellDisabled to 1
            connexStateObj.cellDisabled = connexStateObj.active;

          #ifdef DEBUG_OPINCAA_EXTRA_ACTIVE
            printf("connexStateObj.cellDisabled = ");
            connexStateObj.cellDisabled.Print();
          #endif

            break;
        }
        case _ENABLE_ALL_CELLS:
            // IMPORTANT: For all lanes we set cellDisabled to 0
            ConnexVector::Unconditioned_Set(connexStateObj.cellDisabled, 0);

          #ifdef DEBUG_OPINCAA_EXTRA_ACTIVE
            printf("connexStateObj.cellDisabled = ");
            connexStateObj.cellDisabled.Print();
          #endif

            //return;
            break;

        default:
           #ifdef DEBUG_OPINCAA
            printf("Executing invalid opcode %d\n", instruction.getOpcode());
            fflush(stdout);
           #endif
            throw string("ConnexSimulator::executeInstruction(): Invalid instruction opcode ") +
                  to_string(instruction.getOpcode()) + string("!");
            //break;
    }

  #ifdef DEBUG_OPINCAA_PRINT_TRACE_REG_VALUES
    /* Generating traces is very useful to check where the OpincaaLLVM compiler
        might fail to generate correct code - this happened e.g. at div.i16
        emulation, where post-RA scheduler and ...  were the culprits. */
    FILE *fout = fopen("trace.txt", "at");
    assert(fout != NULL);

    fprintf(fout, "Ran %s", instruction.dump().c_str());
    //fflush(fout);

    if (instruction.getDest() != NO_REG_INDEX) {
      fprintf(fout, "%s", strDestBefore);
      fprintf(fout, " dest = ");
      for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
          fprintf(fout, "%04hx ",
                  registerFile[instruction.getDest()].getCellValue(i));
      }
      fprintf(fout, "\n");
    }
    //fflush(fout);

    if (instruction.getLeft() != NO_REG_INDEX) {
      fprintf(fout, " left = ");
      if (instruction.getDest() != NO_REG_INDEX &&
          instruction.getLeft() == instruction.getDest()) {
          fprintf(fout, " (register is updated by this instr so read value for reg dest BEFORE)\n");
      }
      else {
          for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
              fprintf(fout, "%04hx ",
                  registerFile[instruction.getLeft()].getCellValue(i));
          }
          fprintf(fout, "\n");
      }
    }
    //fflush(fout);

    if (instruction.getRight() != NO_REG_INDEX &&
        (instruction.getOpcode() != _ISHL &&
         instruction.getOpcode() != _ISHR &&
         instruction.getOpcode() != _ISHRA)) {
      fprintf(fout, " right = ");
      if (instruction.getDest() != NO_REG_INDEX &&
          instruction.getRight() == instruction.getDest()) {
          fprintf(fout, " (register is updated by this instr so read value for reg dest BEFORE)\n");
      }
      else {
          for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
              fprintf(fout, "%04hx ",
                  registerFile[instruction.getRight()].getCellValue(i));
          }
          fprintf(fout, "\n");
      }
    }
    //fflush(fout);

    fclose(fout);
  #endif

  #ifdef DEBUG_OPINCAA_PRINT_SIM_EXTRA_USELESS
    printf("Exiting ConnexSimulator::executeInstruction()\n");
    fflush(stdout);
  #endif
}

/****************************************************************************
 * Executes a shift instruction on all active cells
 *
 * @param instruction the shift instruction to execute
 */
void ConnexSimulator::handleShift(Instruction instruction)
{
    connexStateObj.shiftReg.copyFrom(registerFile[instruction.getLeft()]);
    connexStateObj.shiftCountReg.copyFrom(registerFile[instruction.getRight()]);

    connexStateObj.shiftReg.shift(
                                instruction.getOpcode() == _CELL_SHL ? 1 : -1);
}

/****************************************************************************
 * Executes a reduction instruction
 *
 * @param instruction the reduction instruction to execute
 */
void ConnexSimulator::handleReduction(Instruction instruction) {
    dprintf("Entered ConnexSimulator::handleReduction()\n");
    dfflush(stdout);

    int sum = registerFile[instruction.getLeft()].reduce();
    dprintf("ConnexSimulator::handleReduction(): before 1st write\n");
    dfflush(stdout);
    ssize_t unusedRes = write(reductionDescriptor, &sum, sizeof(sum));
    dprintf("ConnexSimulator::handleReduction(): before 2nd write\n");
    dfflush(stdout);
    unusedRes = write(reductionDescriptor, NULL, 0);

    dprintf("Exiting ConnexSimulator::handleReduction()\n");
    dfflush(stdout);
}

/****************************************************************************
 * Executes a reduction instruction with unsigned operands
 *
 * @param instruction the reduction instruction to execute
 */
void ConnexSimulator::handleReduction_u(Instruction instruction) {
    dprintf("Entered ConnexSimulator::handleReduction_u()\n");
    dfflush(stdout);

    int sum = registerFile[instruction.getLeft()].reduce_u();
    ssize_t unusedRes = write(reductionDescriptor, &sum, sizeof(sum));
    unusedRes = write(reductionDescriptor, NULL, 0);
}


/****************************************************************************
 * Executes a sum-scan/prefix instruction with unsigned operands
 *
 * @param instruction the reduction instruction to execute
 */
void ConnexSimulator::handleScan(Instruction instruction) {
    dprintf("Entered ConnexSimulator::handleScan()\n");
    dfflush(stdout);

    ConnexVector res = registerFile[instruction.getLeft()].scan();

    ConnexVectorElementType *buffer = (ConnexVectorElementType *)malloc(CONNEX_VECTOR_LENGTH * sizeof(ConnexVectorElementType));
    assert(buffer != NULL);
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        buffer[i] = res.getCellValue(i);
        printf("handleScan(): buffer[%d] = %d\n", i, buffer[i]);
        fflush(stdout);
    }

    ssize_t unusedRes = write(reductionDescriptor, buffer,
                              CONNEX_VECTOR_LENGTH * sizeof(ConnexVectorElementType));
    unusedRes = write(reductionDescriptor, NULL, 0);

    printf("Exiting handleScan()\n");
    fflush(stdout);

    free(buffer);
}


void ConnexSimulator::printRegister(int index) {
    assert(index >= 0 && index < CONNEX_REG_COUNT);

    //printf("R[%02d] (starting from index 0) = ", index);
    printf("R[%2d] (starting from index 0) = ", index);
    for (int i = 0; i < CONNEX_VECTOR_LENGTH; i++) {
        //printf("%hd ", registerFile[index].getCellValue(i));

        /* We need to use unsigned to avoid at least on x86 to sign extend
           if the number is negative as i16. */
        printf("%04x ", (unsigned short)registerFile[index].getCellValue(i));
        // printf("%d:%04x ", i, (unsigned short)registerFile[index].getCellValue(i));
    }
    printf("[END]\n");
    fflush(stdout);
}

