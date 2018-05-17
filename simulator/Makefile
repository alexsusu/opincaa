#------------------------------------------------------------------------------#
# Linux Makefile for CONNEX SIMULATOR                                          #
#------------------------------------------------------------------------------#


WORKDIR = `pwd`

CXX = g++

INC_DIR = include
BUILD_DIR = build
SRC_DIR = src
LIB_DIR = libs

# Alex: added $(CXXFLAGS) that can allow to compile with debugging support
#CXXFLAGS = -g -std=c++0x
CXXFLAGS = -g -std=c++11
CFLAGS = -fpermissive
LDFLAGS_DEBUG =  $(LDFLAGS)

# 2018_02_10
OBJ = $(BUILD_DIR)/ConnexVector.o $(BUILD_DIR)/ConnexSimulator.o $(BUILD_DIR)/Instruction.o $(BUILD_DIR)/Params.o $(BUILD_DIR)/InstructionQueue.o

all: make_paths build

start: all
	./build/simulator &

stop:
	killall -9 simulator
	rm distributionFIFO readFIFO reductionFIFO writeFIFO

make_paths: 
	test -d $(BUILD_DIR) || mkdir -p $(BUILD_DIR)

build: $(OBJ)
	$(CXX) -O3 $(CFLAGS) $(CXXFLAGS) -I$(INC_DIR) -I../arch/connex16-hm-generic/include $(OBJ) $(SRC_DIR)/Main.cpp -o $(BUILD_DIR)/simulator -pthread

$(BUILD_DIR)/Instruction.o: ../arch/connex16-hm-generic/src/Instruction.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -I../arch/connex16-hm-generic/include -c ../arch/connex16-hm-generic/src/Instruction.cpp -o $(BUILD_DIR)/Instruction.o
# 2018_02_10
$(BUILD_DIR)/Params.o: ../arch/connex16-hm-generic/src/Params.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -I../arch/connex16-hm-generic/include -c ../arch/connex16-hm-generic/src/Params.cpp -o $(BUILD_DIR)/Params.o

$(BUILD_DIR)/ConnexVector.o: $(SRC_DIR)/ConnexVector.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -I$(INC_DIR) -I../arch/connex16-hm-generic/include -c $(SRC_DIR)/ConnexVector.cpp -o $(BUILD_DIR)/ConnexVector.o

$(BUILD_DIR)/ConnexSimulator.o: $(SRC_DIR)/ConnexSimulator.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -I$(INC_DIR) -I../arch/connex16-hm-generic/include -I../utils/include -c $(SRC_DIR)/ConnexSimulator.cpp -o $(BUILD_DIR)/ConnexSimulator.o

$(BUILD_DIR)/InstructionQueue.o:  $(SRC_DIR)/InstructionQueue.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -I$(INC_DIR) -I../arch/connex16-hm-generic/include -c  $(SRC_DIR)/InstructionQueue.cpp -o $(BUILD_DIR)/InstructionQueue.o
	
clean: 
	rm -rf $(BUILD_DIR)
