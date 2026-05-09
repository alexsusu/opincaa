#/home/asusu/Connex/OPINCAA_2015_10/opincaa-master-d49993900aaf33580858c74e090a77bd67217702/
#OPINCAA_HOME=/root/OpincaaLLVM/opincaalib
#OPINCAA_HOME=/home/asusu/Connex/OPINCAA_2015_10/opincaa-master-d49993900aaf33580858c74e090a77bd67217702/

#CFLAGS=-g

all:
	#FOR SOME strange reason this does NOT work, so we split it in 2 below: g++ -std=c++11 -I. -I$(OPINCAA_HOME)/tests/include -I$(OPINCAA_HOME)/include -I$(OPINCAA_HOME)/arch/connex16-hm-generic/include -L$(OPINCAA_HOME)/libs/connex16-hm-generic/ -lopincaa -lm -fopenmp -fPIC -pthread output_main.cpp

        # Useful to find bugs reported unclear (due to use of EXECUTE_*() macrodefs)
	#g++ -E -std=c++0x -I. -I$(OPINCAA_HOME)/tests/include -I$(OPINCAA_HOME)/include -I$(OPINCAA_HOME)/arch/connex16-hm-generic/include -L$(OPINCAA_HOME)/libs/connex16-hm-generic/ output_main.cpp >Opincaa_preproc.cpp

	#g++ -O3 -mfpu=neon -c -std=c++0x -I. -I$(OPINCAA_HOME)/tests/include -I$(OPINCAA_HOME)/include -I$(OPINCAA_HOME)/arch/connex16-hm-generic/include -L$(OPINCAA_HOME)/libs/connex16-hm-generic/ output_main.cpp
	g++ -O3 -c -std=c++0x -I. -I$(OPINCAA_HOME)/tests/include -I$(OPINCAA_HOME)/include -I$(OPINCAA_HOME)/arch/connex16-hm-generic/include -L$(OPINCAA_HOME)/libs/connex16-hm-generic/ output_main.cpp
	g++ -std=c++0x -I. -I$(OPINCAA_HOME)/tests/include -I$(OPINCAA_HOME)/include -I$(OPINCAA_HOME)/arch/connex16-hm-generic/include -L$(OPINCAA_HOME)/libs/connex16-hm-generic/ output_main.o -lopincaa -lm -fopenmp -fPIC -pthread


#cpu_sim:
#	g++ -O3 -mfpu=neon -DSTANDALONE_APP -DTEST_OPINCAA_CONNEX_KERNELS -otest_CPU_100 output_main.cpp

cpu:
	g++ -O3 -mfpu=neon -DSTANDALONE_APP -DTEST_OPINCAA_CONNEX_KERNELS -otest_CPU_100 test.c

cpu_wo_neon:
	g++ -O3 -DSTANDALONE_APP -DTEST_OPINCAA_CONNEX_KERNELS -otest_CPU_wo_neon_100 test.c
