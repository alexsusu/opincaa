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

CFLAGS = -fPIC -std=c++0x
LDFLAGS_DEBUG =  $(LDFLAGS)

OBJ = $(BUILD_DIR)/Instruction.o  $(BUILD_DIR)/Kernel.o $(BUILD_DIR)/ConnexMachine.o $(BUILD_DIR)/Operand.o
TEST_OBJ = $(TEST_BUILD_DIR)/api_test

lib: make_paths build_lib

make_paths:
	test -d $(BUILD_DIR) || mkdir -p $(BUILD_DIR)
	test -d $(LIB_DIR) || mkdir -p $(LIB_DIR)

build_lib: $(OBJ)
	$(CXX) $(CFLAGS) $(OBJ) -lc -shared -Wl,-soname,libopincaa.so -o $(LIB_DIR)/libopincaa.so

$(BUILD_DIR)/Instruction.o: ../../arch/src/Instruction.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -c ../../arch/src/Instruction.cpp -o $(BUILD_DIR)/Instruction.o

$(BUILD_DIR)/Kernel.o: $(SRC_DIR)/Kernel.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -I../../utils/include -c $(SRC_DIR)/Kernel.cpp -o $(BUILD_DIR)/Kernel.o

$(BUILD_DIR)/Operand.o: $(SRC_DIR)/Operand.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -c $(SRC_DIR)/Operand.cpp -o $(BUILD_DIR)/Operand.o

$(BUILD_DIR)/ConnexMachine.o: $(SRC_DIR)/ConnexMachine.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -I../../utils/include -c $(SRC_DIR)/ConnexMachine.cpp -o $(BUILD_DIR)/ConnexMachine.o

$(BUILD_DIR)/NamedPipes.o: ../../utils/src/NamedPipes.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -c ../../utils/src/NamedPipes.cpp -o $(BUILD_DIR)/NamedPipes.o

install: lib
	cp $(LIB_DIR)/libopincaa.so /usr/local/lib
	chmod 0755 /usr/local/lib/libopincaa.so
	ldconfig

clean: 
	rm -rf $(BUILD_DIR)
	rm -rf $(LIB_DIR)


