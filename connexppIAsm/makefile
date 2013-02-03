#------------------------------------------------------------------------------#
# Linux Makefile for OPINCAA API                                               #
#------------------------------------------------------------------------------#


WORKDIR = `pwd`

CXX = g++

INC   = -Iinclude -Iinclude/api/opincaa
BUILD_DIR = build
SRC_DIR = src
LIB_DIR = libs
TEST_DIR = test
TEST_BUILD_DIR = test_build

CFLAGS = -fPIC
LDFLAGS_DEBUG =  $(LDFLAGS)

OBJ = $(BUILD_DIR)/Instruction.o  $(BUILD_DIR)/Kernel.o $(BUILD_DIR)/ConnexMachine.o $(BUILD_DIR)/Operand.o
TEST_OBJ = $(TEST_BUILD_DIR)/api_test

lib: make_paths build_lib

test: lib build_tests run_tests

build_tests:
	$(CXX) -O3 -L$(LIB_DIR) -lopincaa $(INC) $(TEST_DIR)/api_test.cpp -o $(TEST_BUILD_DIR)/api_test

run_tests:
	./$(TEST_BUILD_DIR)/api_test
	
make_paths: 
	test -d $(BUILD_DIR) || mkdir -p $(BUILD_DIR)
	test -d $(LIB_DIR) || mkdir -p $(LIB_DIR)
	test -d $(TEST_BUILD_DIR) || mkdir -p $(TEST_BUILD_DIR)

build_lib: $(OBJ)
	$(CXX) $(CFLAGS) $(INC) $(OBJ) -lc -shared -Wl,-soname,libopincaa.so -o $(LIB_DIR)/libopincaa.so

$(BUILD_DIR)/Instruction.o: $(SRC_DIR)/api/opincaa/Instruction.cpp
	$(CXX) $(CFLAGS) $(INC) -c $(SRC_DIR)/api/opincaa/Instruction.cpp -o $(BUILD_DIR)/Instruction.o

$(BUILD_DIR)/Kernel.o: $(SRC_DIR)/api/opincaa/Kernel.cpp
	$(CXX) $(CFLAGS) $(INC) -c $(SRC_DIR)/api/opincaa/Kernel.cpp -o $(BUILD_DIR)/Kernel.o

$(BUILD_DIR)/Image.o: $(SRC_DIR)/api/wrapper/Image.cpp
	$(CXX) $(CFLAGS) $(INC) -c $(SRC_DIR)/api/wrapper/Image.cpp -o $(BUILD_DIR)/Image.o

$(BUILD_DIR)/Operand.o: $(SRC_DIR)/api/opincaa/Operand.cpp
	$(CXX) $(CFLAGS) $(INC) -c $(SRC_DIR)/api/opincaa/Operand.cpp -o $(BUILD_DIR)/Operand.o

$(BUILD_DIR)/ConnexMachine.o: $(SRC_DIR)/api/opincaa/ConnexMachine.cpp
	$(CXX) $(CFLAGS) $(INC) -c $(SRC_DIR)/api/opincaa/ConnexMachine.cpp -o $(BUILD_DIR)/ConnexMachine.o

clean: 
	rm -rf $(BUILD_DIR)
	rm -rf $(TEST_BUILD_DIR)
	rm -rf $(LIB_DIR)


