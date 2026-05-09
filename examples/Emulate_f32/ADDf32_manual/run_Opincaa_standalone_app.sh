# Inspired from https://serverfault.com/questions/16204/how-to-make-bash-scripts-print-out-every-command-before-it-executes
set -x
# set -v # This prints comments from the bash script also


# Normally we should use the environment variable OPINCAA_HOME set instead of setting it here
OPINCAA_HOME=/home/asusu/Connex/opincaalib
#OPINCAA_HOME=/root/OpincaaLLVM/opincaalib

LD_LIBRARY_PATH=$OPINCAA_HOME/libs/connex16-hm-generic
export LD_LIBRARY_PATH

killall -9 simulator
killall -9 simulator

# Usage: ./simulator CVL CONNEX_MEM_NUM_ROWS CONNEX_REG_COUNT INTERNAL_INSTRUCTION_MEMORY_SIZE
#     Note: all these parameters should be power of 2
# valgrind $OPINCAA_HOME/simulator/build/simulator 128 1024 32 1024 &
$OPINCAA_HOME/simulator/build/simulator 128 1024 32 1024 &
# Although not really required Give some time to the simulator to create the FIFOs, etc
sleep 1

# Usage: ./a.out CONNEX_VECTOR_LENGTH CONNEX_MEM_NUM_ROWS CONNEX_REG_COUNT INTERNAL_INSTRUCTION_MEMORY_SIZE checkForDataHazards useLaneGatingOnConnex numMaxNestedHwLoops dontExecuteKernel
#     Note: all these parameters should be power of 2
##valgrind ./a.out 128 1024 32 1024
./a.out 128 1024 32 1024
#./a.out 128 1024 32 2048
#gdb ./a.out

# We leave some time for simulator to finish - we experienced (at least for 25k_int32_map_add test we got WRONG results because simulator got killed before it finished - don't understand why the client program does not wait until readReduction() - maybe the reduction pipe already had some data from previous runs??)
sleep 2

killall -9 simulator
killall -9 simulator
