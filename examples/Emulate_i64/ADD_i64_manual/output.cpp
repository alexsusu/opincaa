#include <algorithm>
#include <iostream>
#include <vector>
#include <limits>
#include <random>
using namespace std;

typedef int64_t TYPE;
typedef uint64_t UTYPE;


void AddI64(int n, TYPE* a, TYPE* b, TYPE* c) {
    cerr << "AddI64(): n = " << n << endl;

// We just put 1 vector line of each operand:
    //int lanes = n * sizeof(TYPE) / sizeof(short);
    //int b_offset = (lanes + CONNEX_VECTOR_LENGTH - 1) / CONNEX_VECTOR_LENGTH;
    // Correction to lanes: lanes = b_offset;
    int lanes = 1;
    int b_offset = 1;
    cerr << "[code] lanes: " << lanes << endl;
    cerr << "[code] b_offset: " << b_offset << endl;


    static string kernelName;
    kernelName = "Add.i64";


// For iterating through memory
#define REG_ITER_A 0
#define REG_ITER_B 1
#define REG_ITER_C 2

// For computing additions
#define REG_SRC_A 23
#define REG_SRC_B 24
#define REG_DST 25

// Carry bits
#define REG_CRY 26

// Constants
#define REG_CT_0 30
#define REG_CT_1 31
#define REG_CT_3 29

// Used for indexes
#define REG_IDX 20

    BEGIN_KERNEL(kernelName);
      EXECUTE_IN_ALL(

        R(REG_ITER_A) = 0;
        R(REG_ITER_B) = b_offset;
        R(REG_ITER_C) = 2 * b_offset;

        //REPEAT_X_TIMES(b_offset);
// We just put 1 vector line of each operand:        for (int idRepeat = 0; idRepeat < b_offset; idRepeat++) {

          R(REG_SRC_A) = LS[R(REG_ITER_A)];
          R(REG_SRC_B) = LS[R(REG_ITER_B)];


        // Set constants
        R(REG_CT_0) = 0;
        R(REG_CT_1) = 1;
        R(REG_CT_3) = 3;

          R(REG_DST) = R(REG_SRC_A) + R(REG_SRC_B);
      )

          for (int i = 0; i < 3; ++i) {
//      EXECUTE_IN_ALL(
            R(REG_CRY) = ADDC(R(REG_CT_0), R(REG_CT_0));
            R(REG_IDX) = INDEX;
            R(REG_IDX) = R(REG_IDX) & R(REG_CT_3);
            R(REG_IDX) = R(REG_IDX) == R(REG_CT_3);
            NOP;
//      )
      EXECUTE_WHERE_EQ(
            R(REG_CRY) = 0;
      );
      EXECUTE_IN_ALL(
            CELL_SHR(R(REG_CRY), R(REG_CT_1));
            NOP;
            R(REG_CRY) = SHIFT_REG;
            R(REG_DST) = R(REG_DST) + R(REG_CRY);
      )
          } // END for (i) loop
      EXECUTE_IN_ALL(
          NOP;



          LS[R(REG_ITER_C)] = R(REG_DST);
          R(REG_ITER_A) = R(REG_ITER_A) + R(REG_CT_1);
          R(REG_ITER_B) = R(REG_ITER_B) + R(REG_CT_1);
          R(REG_ITER_C) = R(REG_ITER_C) + R(REG_CT_1);

//        } // END for (idRepeat) loop
        //END_REPEAT;

        REDUCE(R(0));
      );
    END_KERNEL(kernelName);





    connexGlobal->writeDataToConnexPartial(a, lanes, 0);
    connexGlobal->writeDataToConnexPartial(b, lanes, b_offset);

#ifdef LLVM_ISEL_CODEGEN
    //string kernelName = "";
    Kernel *kernel = connexGlobal->getKernel(kernelName);
    kernel->sdNodeVarNameRegDef[REG_SRC_A] = "nodeOpSrcCast1";
    kernel->sdNodeVarNameRegDef[REG_SRC_B] = "nodeOpSrcCast2";
    //
    // For ADD.i64:
    kernel->offsetKernelToStartCodegenFrom = 5 + 1; // +1 for the END_WHERE instruction Opincaa adds automatically to each kernel
    kernel->numInstructionsToCodegen = kernel->size()
                                           - 7 /*num instruction we remove from end of kernel */
                                           - kernel->offsetKernelToStartCodegenFrom;
    //
    // We use chain, since with glue we get a lot or weird scheduling errors:
    //kernel->useGlue = 0;
    kernel->useGlue = 1;
    /* IMPORTANT: to convert in 'partly SSA form' we require ~64 (usually more
                   than 32) registers. */
//    assert(CONNEX_REG_COUNT != 32);

    printf("Calling connexGlobal->genLLVMISelManualCode()\n");
    fflush(stdout);
    //string resGenLLVM = connexGlobal->genLLVMISelManualCode(TEST_PREFIX + to_string((long long int)BatchNumberGlobal));
    string resGenLLVM = connexGlobal->genLLVMISelManualCode(kernelName);
    //printf("resGenLLVM = \n%s\n", resGenLLVM.c_str());
    //fflush(stdout);


    printf("Calling connexGlobal->dumpKernel()\n");
    fflush(stdout);
    string resDump = connexGlobal->dumpKernel(kernelName);
    printf("resDump = %s\n", resDump.c_str());
    fflush(stdout);

    printf("Calling connexGlobal->disassembleKernel()\n");
    fflush(stdout);
    string resDis = connexGlobal->disassembleKernel(kernelName);
    printf("resDis = %s\n", resDis.c_str());
    fflush(stdout);
#endif

    connexGlobal->executeKernel(kernelName);
    connexGlobal->readReduction();
    connexGlobal->readDataFromConnexPartial(c, lanes, 2 * b_offset);
}


void Test() {
    mt19937 rnd{random_device()()};
    auto dist = uniform_int_distribution<TYPE>(
        numeric_limits<TYPE>::min(),
        numeric_limits<TYPE>::max()
    );

    //int n = 114;
    int n = CONNEX_VECTOR_LENGTH / (sizeof(int64_t) / sizeof(short));
    vector<TYPE> a(n), b(n), c(n);
    for (int i = 0; i < n; ++i) {
        a[i] = dist(rnd);
        b[i] = dist(rnd);
    }

    AddI64(n, a.data(), b.data(), c.data());
    for (int i = 0; i < n; ++i) {
        TYPE result = (UTYPE) a[i] + (UTYPE) b[i];
        if (result != c[i]) {
            cerr << "Results differ on position " << i << endl;
            cerr << "Got: " << c[i] << " expected: " << result << endl;
        }
        assert(result == c[i]);
    }
    cout << "[code] OK!" << endl;
}

