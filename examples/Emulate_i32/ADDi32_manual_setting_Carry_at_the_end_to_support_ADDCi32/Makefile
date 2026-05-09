#/home/asusu/Connex/OPINCAA_2015_10/opincaa-master-d49993900aaf33580858c74e090a77bd67217702/
#OPINCAA_HOME=/home/alarm/OpincaaLLVM/opincaalib
#OPINCAA_HOME=/home/asusu/Connex/OPINCAA_2015_10/opincaa-master-d49993900aaf33580858c74e090a77bd67217702/

#CFLAGS=-g

all:
#	FOR SOME strange reason this does NOT work, so we split it in 2 below: g++ -std=c++11 -I. -I$(OPINCAA_HOME)/tests/include -I$(OPINCAA_HOME)/include -I$(OPINCAA_HOME)/arch/connex16-hm-generic/include -L$(OPINCAA_HOME)/libs/connex16-hm-generic/ -lopincaa -lm -fopenmp -fPIC -pthread output_main.cpp
	g++ -c -std=c++0x -I. -I$(OPINCAA_HOME)/tests/include -I$(OPINCAA_HOME)/include -I$(OPINCAA_HOME)/arch/connex16-hm-generic/include -L$(OPINCAA_HOME)/libs/connex16-hm-generic/ output_main.cpp
	g++ -std=c++0x -I. -I$(OPINCAA_HOME)/tests/include -I$(OPINCAA_HOME)/include -I$(OPINCAA_HOME)/arch/connex16-hm-generic/include -L$(OPINCAA_HOME)/libs/connex16-hm-generic/ output_main.o -lopincaa -lm -fopenmp -fPIC -pthread

CodeGen_ISel:
	g++ -DLLVM_ISEL_CODEGEN -o output_main_codegen.o -c -std=c++0x -I. -I$(OPINCAA_HOME)/tests/include -I$(OPINCAA_HOME)/include -I$(OPINCAA_HOME)/arch/connex16-hm-generic/include -L$(OPINCAA_HOME)/libs/connex16-hm-generic/ output_main.cpp
	g++ -o a_CodeGen.out -std=c++0x -I. -I$(OPINCAA_HOME)/tests/include -I$(OPINCAA_HOME)/include -I$(OPINCAA_HOME)/arch/connex16-hm-generic/include -L$(OPINCAA_HOME)/libs/connex16-hm-generic/ output_main_codegen.o -lopincaa -lm -fopenmp -fPIC -pthread


cpu_x86:
	g++ -O3 -DRUN_ON_ZEDBOARD -DSTANDALONE_APP -DTEST_OPINCAA_CONNEX_KERNELS -I$(OPINCAA_HOME)/include -otest_CPU_1000 test.c


cpu_arm:
#       This requires to set LD_LIBRARY_PATH when executing the binary: g++ -O3 -mfpu=neon -DRUN_ON_ZEDBOARD -DSTANDALONE_APP -DTEST_OPINCAA_CONNEX_KERNELS -I$(OPINCAA_HOME)/include -L$(OPINCAA_HOME)/libs/connex16-hm-generic/ -otest_CPU_1000 test.c -lopincaa
#       g++ -O3 -mfpu=neon -DRUN_ON_ZEDBOARD -DSTANDALONE_APP -DTEST_OPINCAA_CONNEX_KERNELS -I$(OPINCAA_HOME)/include -otest_CPU_1000 test.c
# Maybe use: -fsimd-cost-model=
# From man gcc:
#       -fvect-cost-model=model
#           Alter the cost model used for vectorization.  The model argument should be one of unlimited, dynamic or cheap.  With the unlimited model the vectorized code-path is assumed to be
#           profitable while with the dynamic model a runtime check guards the vectorized code-path to enable it only for iteration counts that will likely execute faster than when executing the
#           original scalar loop.  The cheap model disables vectorization of loops where doing so would be cost prohibitive for example due to required runtime checks for data dependence or alignment
#           but otherwise is equal to the dynamic model.  The default cost model depends on other optimization flags and is either dynamic or cheap.
	g++ -O3 -mfpu=neon -fvect-cost-model=unlimited -fdump-tree-all -ftree-vectorizer-verbose=5 -ftree-vectorize -DRUN_ON_ZEDBOARD -DSTANDALONE_APP -DTEST_OPINCAA_CONNEX_KERNELS -I$(OPINCAA_HOME)/include -otest_CPU_1000 test.c

cpu_arm_wo_neon:
#       This requires to set LD_LIBRARY_PATH when executing the binary: g++ -O3 -DRUN_ON_ZEDBOARD -DSTANDALONE_APP -DTEST_OPINCAA_CONNEX_KERNELS -I$(OPINCAA_HOME)/include -L$(OPINCAA_HOME)/libs/connex16-hm-generic/ -otest_CPU_wo_neon_1000 test.c -lopincaa
	g++ -O3 -DRUN_ON_ZEDBOARD -DSTANDALONE_APP -DTEST_OPINCAA_CONNEX_KERNELS -I$(OPINCAA_HOME)/include -otest_CPU_wo_neon_1000 test.c
