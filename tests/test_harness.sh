#!/bin/bash

#make opincaa, simulator and tests
cd ../ && make && cd -
cd ../simulator && make && cd -
make

#start the simulator
#./../simulator/build/simulator &

#run the test
sleep 3
LD_LIBRARY_PATH=../libs/$1
export LD_LIBRARY_PATH

#cd .. ; make && sudo make install ; cd -
make build/test_connex-rc

./build/test_connex-rc

#kill simulator
#killall -9 simulator
