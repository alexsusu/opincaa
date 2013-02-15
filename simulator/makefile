#------------------------------------------------------------------------------#
# Linux Makefile for CONNEX SIMULATOR                                          #
#------------------------------------------------------------------------------#


WORKDIR = `pwd`

CXX = g++

INC_DIR = include
BUILD_DIR = build
SRC_DIR = src
LIB_DIR = libs

CFLAGS = -fPIC -std=c++0x
LDFLAGS_DEBUG =  $(LDFLAGS)

OBJ = $(BUILD_DIR)/Cell.o  $(BUILD_DIR)/ConnexSimulator.o	$(BUILD_DIR)/NamedPipes.o

all: make_paths build

start: all
	./build/simulator &

stop:
	killall -9 simulator

make_paths: 
	test -d $(BUILD_DIR) || mkdir -p $(BUILD_DIR)

build: $(OBJ)
	$(CXX) -O3 $(CFLAGS) -I$(INC_DIR) -I../opincaa/include $(OBJ) -lopincaa $(SRC_DIR)/Main.cpp -o $(BUILD_DIR)/simulator -pthread

$(BUILD_DIR)/Cell.o: $(SRC_DIR)/Cell.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -I../opincaa/include -c $(SRC_DIR)/Cell.cpp -o $(BUILD_DIR)/Cell.o

$(BUILD_DIR)/ConnexSimulator.o: $(SRC_DIR)/ConnexSimulator.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -I../opincaa/include -c $(SRC_DIR)/ConnexSimulator.cpp -o $(BUILD_DIR)/ConnexSimulator.o

$(BUILD_DIR)/ConnexSimulator.o: $(SRC_DIR)/NamedPipes.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -I../opincaa/include -c $(SRC_DIR)/NamedPipes.cpp -o $(BUILD_DIR)/NamedPipes.o
	
clean: 
	rm -rf $(BUILD_DIR)


