#include <iostream>

#include "ConnexMachine.h"
#include "utils.h"

using namespace std;

//#define NUM_ITER	CONNEX_MAX_MEMORY
#ifndef NUM_ITER
#define NUM_ITER	1
#endif

static UINT16 *data;

#define SORT_ODD	"bubble_sort_odd"
#define SORT_EVEN	"bubble_sort_even"

static ConnexMachine *connex = NULL;

static void bubble_sort_even(INT32 param1)
{
	BEGIN_KERNEL(SORT_EVEN + to_string(param1));
		EXECUTE_IN_ALL(
			R0 = INDEX;
			R1 = LS[param1];
			R2 = 1;
			R3 = R0 & R2;
			R3 = R2 - R3;
			R6 = 0;
			R4 = (R0 == R6);
			NOP;
		)
		EXECUTE_WHERE_EQ(
			R3 = 0;
		)
		EXECUTE_IN_ALL(
			R5 = R1;
			R4 = (R3 == R2);
			NOP;
		)
		EXECUTE_WHERE_EQ(
			CELL_SHR(R1, R2);
			NOP;
			R1 = SHIFT_REG;
		)
		EXECUTE_IN_ALL(
			R4 = (R5 < R1);
			NOP;
		)
		EXECUTE_WHERE_LT(
			R6 = 1; // Exchange
			LS[param1] = R1;
		)

		// Move selection.
		EXECUTE_IN_ALL(
			R3 = R3 & R6;
			CELL_SHL(R3, R2);
			NOP;
			R3 = SHIFT_REG;
			R4 = (R3 == R2);
			NOP;
		)
		EXECUTE_WHERE_EQ(
			CELL_SHL(R5, R2);
			NOP;
			R5 = SHIFT_REG;
			LS[param1] = R5;
		)

		EXECUTE_IN_ALL(
			REDUCE(R6);
		)

	END_KERNEL(SORT_EVEN + to_string(param1));
}

static void bubble_sort_odd(INT32 param1)
{
	BEGIN_KERNEL(SORT_ODD + to_string(param1));
		EXECUTE_IN_ALL(
			R0 = INDEX;
			R1 = LS[param1];
			R2 = 1;
			R3 = R0 & R2;
			R6 = 0;
			R5 = R1;
			R4 = (R3 == R2);
			NOP;
		)

		EXECUTE_WHERE_EQ(
			CELL_SHR(R1, R2);
			NOP;
			R1 = SHIFT_REG;
		)
		EXECUTE_IN_ALL(
			R4 = (R5 < R1);
			NOP;
		)
		EXECUTE_WHERE_LT(
			R6 = 1; // Exchange
			LS[param1] = R1;
		)

		// Move selection.
		EXECUTE_IN_ALL(
			R3 = R3 & R6;
			CELL_SHR(R3, R2);
			NOP;
			R3 = SHIFT_REG;
			R4 = (R3 == R2);
			NOP;
		)
		EXECUTE_WHERE_EQ(
			CELL_SHL(R5, R2);
			NOP;
			R5 = SHIFT_REG;
			LS[param1] = R5;
		)

		EXECUTE_IN_ALL(
			REDUCE(R6);
		)

	END_KERNEL(SORT_ODD + to_string(param1));
}

static void send_data_to_LS(void)
{
	UINT32 iter;

	for (iter = 0; iter < NUMBER_OF_MACHINES * NUM_ITER; iter++)
		data[iter] = NUMBER_OF_MACHINES * NUM_ITER - iter - 1;

	connex->writeDataToArray(data, NUM_ITER, 0);
}

static void get_data_from_LS(void)
{
	UINT32 iter;
	
	connex->readDataFromArray(data, NUM_ITER, 0);
}

static void print_sorted_array(void)
{
	UINT32 iter;

	for (iter = 0; iter < NUMBER_OF_MACHINES * NUM_ITER; iter++)
		cout << data[iter] << " ";
	cout << endl;
}

static UINT16 get_minimum(UINT8 *indexes)
{
	UINT16 min = ~0x0;
	UINT32 iter, min_iter = -1;

	for (iter = 0; iter < NUM_ITER; iter++) {
		if (indexes[iter] >= NUMBER_OF_MACHINES)
			continue;
		if (data[iter * NUMBER_OF_MACHINES + indexes[iter]] < min) {
			min = data[iter * NUMBER_OF_MACHINES + indexes[iter]];
			min_iter = iter;
		}
	}

	indexes[min_iter]++;

	return min;
}

static void merge_sort_on_host(void)
{
	UINT16 *temp = new UINT16[NUM_ITER * NUMBER_OF_MACHINES];
	UINT8 *indexes = new UINT8[NUM_ITER]();
	UINT32 iter, index = 0;

	while (index < NUM_ITER * NUMBER_OF_MACHINES) {
		temp[index] = get_minimum(indexes);
		index++;
	}

	/* Copy data. */
	for (iter = 0; iter < NUM_ITER * NUMBER_OF_MACHINES; iter++)
		data[iter] = temp[iter];

	delete temp;
	delete indexes;
}

static void test_sort_chunks(ConnexMachine *connex)
{
	UINT32 iter;
	int val = 1;

	for (iter = 0; iter < NUM_ITER; iter++) {
		bubble_sort_odd(iter);
		bubble_sort_even(iter);
	}

	while (val) {
		val = 0;

		for (iter = 0; iter < NUM_ITER; iter++) {
			connex->executeKernel(SORT_ODD + to_string(iter));
			//cout << connex->disassembleKernel(SORT_ODD + to_string(iter));
			val += connex->readReduction();
		}

		for (iter = 0; iter < NUM_ITER; iter++) {
			connex->executeKernel(SORT_EVEN + to_string(iter));
			val += connex->readReduction();
		}
	}

}

int main(int argc, char **argv)
{
	int ret = 1;
	int type = 0;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s ID\n", argv[0]);
		return ret;
	}

	type = argv[1][0] - '0';

	try {
		connex = new ConnexMachine("distributionFIFO",
					   "reductionFIFO",
					   "writeFIFO",
					   "readFIFO");

		cout << "Start testing..." << endl;

		data = new UINT16[NUMBER_OF_MACHINES * NUM_ITER];
		send_data_to_LS();

		switch (type) {
		case 0:
			/* Sort 128 elements. */
			test_sort_chunks(connex);
			get_data_from_LS();
			break;
		case 1:
			/* Sort chunks + mergesort on host machine. */
			test_sort_chunks(connex);
			get_data_from_LS();
			merge_sort_on_host();
			break;
		case 2:
			/* TODO: bubblesort carry between blocks */
		case 3:
			/* TODO: modify kernels to sort end values */
		case 4:
			/* TODO: other algorithm */
		default:
			fprintf(stderr, "No such sorting type\n");
			return ret;
		}

		print_sorted_array();
		delete data;

		cout << "================" << endl;

		ret = 0;
		delete connex;

	} catch(string err) {
		cout << err << endl;
	}

	return ret;
}
