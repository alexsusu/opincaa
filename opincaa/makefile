#------------------------------------------------------------------------------#
# Linux Makefile for OPINCAA API                                               #
#------------------------------------------------------------------------------#


WORKDIR = `pwd`

CXX = g++

INC_DIR = include
BUILD_DIR = build
SRC_DIR = src
LIB_DIR = libs
TEST_DIR = test
TEST_BUILD_DIR = test_build

CFLAGS = -fPIC -std=c++11
LDFLAGS_DEBUG =  $(LDFLAGS)

OBJ = $(BUILD_DIR)/Instruction.o  $(BUILD_DIR)/Kernel.o $(BUILD_DIR)/ConnexMachine.o $(BUILD_DIR)/Operand.o
TEST_OBJ = $(TEST_BUILD_DIR)/api_test

lib: make_paths build_lib

test: lib build_tests run_tests

build_tests:
	$(CXX) -O3 -std=c++11 $(TEST_DIR)/api_test.cpp -o $(TEST_BUILD_DIR)/api_test -L$(LIB_DIR) -lopincaa -I$(INC_DIR)

run_tests:
	./$(TEST_BUILD_DIR)/api_test
	
make_paths: 
	test -d $(BUILD_DIR) || mkdir -p $(BUILD_DIR)
	test -d $(LIB_DIR) || mkdir -p $(LIB_DIR)
	test -d $(TEST_BUILD_DIR) || mkdir -p $(TEST_BUILD_DIR)

build_lib: $(OBJ)
	$(CXX) $(CFLAGS) -I$(INC_DIR) $(OBJ) -lc -shared -Wl,-soname,libopincaa.so -o $(LIB_DIR)/libopincaa.so

$(BUILD_DIR)/Instruction.o: $(SRC_DIR)/Instruction.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -c $(SRC_DIR)/Instruction.cpp -o $(BUILD_DIR)/Instruction.o

$(BUILD_DIR)/Kernel.o: $(SRC_DIR)/Kernel.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -c $(SRC_DIR)/Kernel.cpp -o $(BUILD_DIR)/Kernel.o

$(BUILD_DIR)/Image.o: $(SRC_DIR)/Image.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -c $(SRC_DIR)/Image.cpp -o $(BUILD_DIR)/Image.o

$(BUILD_DIR)/Operand.o: $(SRC_DIR)/Operand.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -c $(SRC_DIR)/Operand.cpp -o $(BUILD_DIR)/Operand.o

$(BUILD_DIR)/ConnexMachine.o: $(SRC_DIR)/ConnexMachine.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -c $(SRC_DIR)/ConnexMachine.cpp -o $(BUILD_DIR)/ConnexMachine.o

install: lib
	cp $(LIB_DIR)/libopincaa.so /usr/local/lib
	chmod 0755 /usr/local/lib/libopincaa.so
	ldconfig

clean: 
	rm -rf $(BUILD_DIR)
	rm -rf $(TEST_BUILD_DIR)
	rm -rf $(LIB_DIR)


