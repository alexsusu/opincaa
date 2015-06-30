#!/bin/bash

#make opincaa, simulator and tests
cd ../ && make && cd -
cd ../simulator && make && cd -
#make

#start the simulator
./../simulator/build/simulator &

#run the test
sleep 3
LD_LIBRARY_PATH=../libs/$1
export LD_LIBRARY_PATH

#cd .. ; make && sudo make install ; cd -
#make build/test_connex-rc_sim && ./build/test_connex-rc_sim
#make -f makeFile.comb build/test_connex-rc_comb && ./build/test_connex-rc_comb
#make -f makeFile.firssd build/fir_and_ssd && ./build/fir_and_ssd
make build/test_connex-rc && ./build/test_connex-rc


#kill simulator
#killall -9 simulator
