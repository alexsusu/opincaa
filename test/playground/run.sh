#!/bin/bash

pushd ../../opincaa/main &> /dev/null
make
popd &> /dev/null

pushd ../../simulator &> /dev/null
make
popd &> /dev/null

# Use something like 'make NUM_ITER=2' to increase the size of the vector.
# You should also force a recompilation before by
# uncommenting the following line:
#make clean
make

../../simulator/build/simulator &

sleep 3
export LD_LIBRARY_PATH=../../opincaa/main/libs; ./test

killall -9 simulator

rm -f *FIFO
