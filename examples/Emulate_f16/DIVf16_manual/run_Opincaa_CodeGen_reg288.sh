OPINCAADIR=/home/asusu/Connex/OPINCAA_2015_10/opincaa-master-d49993900aaf33580858c74e090a77bd67217702
#OPINCAADIR=/root/OpincaaLLVM/opincaalib

LD_LIBRARY_PATH=$OPINCAADIR/libs/connex16-hm-generic
export LD_LIBRARY_PATH

killall -9 simulator
killall -9 simulator

# Running the Connex simulator with a register file with 64 registers - because we are doing code-generation from the Opincaa program for the Instruction selector
$OPINCAADIR/simulator/build/simulator 128 1024 288 1024 &
# Although not really required Give some time to the simulator to create the FIFOs, etc
sleep 1

# Running the Opincaa program assuming a Connex with a register file with 64 registers
./a_CodeGen.out 128 1024 288 1024
#gdb ./a.out

# We leave some time for simulator to finish - we experienced (at least for 25k_int32_map_add test we got WRONG results because simulator got killed before it finished - don't understand why the client program does not wait until readReduction() - maybe the reduction pipe already had some data from previous runs??)
sleep 2

killall -9 simulator
killall -9 simulator
