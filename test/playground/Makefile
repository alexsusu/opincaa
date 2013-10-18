#------------------------------------------------------------------------------#
# Linux Makefile for CONNEX TESTS                                              #
#------------------------------------------------------------------------------#


WORKDIR = `pwd`
NUM_ITER ?= 1

CXX = g++

INC_DIR = include
BUILD_DIR = build
SRC_DIR = src
LIB_DIR = libs

CFLAGS = -fPIC -std=c++0x
LDFLAGS_DEBUG =  $(LDFLAGS)

test: test.o
	$(CXX) -O3 $(CFLAGS)  $^ -L../../opincaa/main/libs -o $@ -lopincaa -fopenmp

test.o: test.cpp
	$(CXX) $(CFLAGS) -I../../opincaa/main/include -I../../utils/include -I../../arch/include -c $^ -o $@ -DNUM_ITER=$(NUM_ITER)

clean: 
	rm -rf *.o test


