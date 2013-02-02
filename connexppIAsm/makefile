#------------------------------------------------------------------------------#
# Linux Makefile for OPINCAA API                                               #
#------------------------------------------------------------------------------#


WORKDIR = `pwd`

CXX = g++

INC   = -Iinclude -Iinclude/api/wrapper -Iinclude/api/opincaa
BUILD_DIR = build
SRC_DIR = src

CFLAGS =
LDFLAGS_DEBUG =  $(LDFLAGS)

OBJ = $(BUILD_DIR)/Instruction.o $(BUILD_DIR)/Image.o $(BUILD_DIR)/Kernel.o $(BUILD_DIR)/ConnexMachine.o $(BUILD_DIR)/Operand.o

all: make_paths build

make_paths: 
	test -d $(BUILD_DIR) || mkdir -p $(BUILD_DIR)


build: $(OBJ)
	$(CXX) $(INC) $(OBJ) $(SRC_DIR)/api_test.cpp -o api_test

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


