#!/bin/bash

CURRENT_PATH=$(pwd)
USELESS=$CURRENT_PATH/.useless_debug

echo "======================"
echo "OPINCAA compiling..."
echo "======================"
pushd ../.. &> /dev/null
make &> $USELESS
popd &> /dev/null

echo "======================"
echo "Simulator compiling..."
echo "======================"
pushd ../../simulator &> /dev/null
make &>> $USELESS
popd &> /dev/null

run_test()
{
	if [ $# -ne 3 ]; then
		echo "Error when calling function $FUNCNAME."
		echo "usage: $FUNCNAME  NUM_ITER  SORT_TYPE  TEST_NAME"
		exit 1
	fi

	echo "======================"
	echo $3
	echo "======================"

	# Use something like 'make NUM_ITER=2' to increase the size of the vector.
	make NUM_ITER=$1 &>> $USELESS

	# Run simulator.
	../../simulator/build/simulator  &>> $USELESS &
	sleep 3

	# Run test.
	export LD_LIBRARY_PATH=../../libs; ./test $2

	# Clean-up.
	killall -9 simulator  &>> $USELESS
	rm -f *FIFO &>> $USELESS
	make clean &>> $USELESS
	echo ""
}

#run_test 1 0 "Sorting 128 elements"
run_test 100 1 "Sorting by merging chunks on host"


