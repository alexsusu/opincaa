#------------------------------------------------------------------------------#
# Linux Makefile for CONNEX TESTS                                              #
#------------------------------------------------------------------------------#


WORKDIR = `pwd`

CXX = g++

INC_DIR = include
BUILD_DIR = build
SRC_DIR = src
LIB_DIR = libs

CFLAGS = -fPIC -std=c++0x
LDFLAGS_DEBUG =  $(LDFLAGS)

OBJ = $(BUILD_DIR)/test.o $(BUILD_DIR)/simple_tests.o $(BUILD_DIR)/simple_io_tests.o $(BUILD_DIR)/controller_tests.o $(BUILD_DIR)/timing.o $(BUILD_DIR)/utils.o

all: make_paths build

make_paths: 
	test -d $(BUILD_DIR) || mkdir -p $(BUILD_DIR)

build: $(OBJ)
	$(CXX) -O3 $(CFLAGS) -I$(INC_DIR) $(OBJ) -L../opincaa/main/libs -o $(BUILD_DIR)/test -lopincaa

$(BUILD_DIR)/test.o: $(SRC_DIR)/test.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -I../opincaa/main/include -I../utils/include -I../arch/include -c $(SRC_DIR)/test.cpp -o $(BUILD_DIR)/test.o

$(BUILD_DIR)/simple_tests.o: $(SRC_DIR)/simple_tests.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -I../opincaa/main/include -I../utils/include -I../arch/include -c $(SRC_DIR)/simple_tests.cpp -o $(BUILD_DIR)/simple_tests.o

$(BUILD_DIR)/controller_tests.o: $(SRC_DIR)/controller_tests.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -I../opincaa/main/include -I../utils/include -I../arch/include -c $(SRC_DIR)/controller_tests.cpp -o $(BUILD_DIR)/controller_tests.o
	
$(BUILD_DIR)/simple_io_tests.o: $(SRC_DIR)/simple_io_tests.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -I../opincaa/main/include -I../utils/include -I../arch/include -c $(SRC_DIR)/simple_io_tests.cpp -o $(BUILD_DIR)/simple_io_tests.o

$(BUILD_DIR)/utils.o: ../utils/src/utils.cpp
	$(CXX) $(CFLAGS) -I../utils/include -c ../utils/src/utils.cpp -o $(BUILD_DIR)/utils.o

$(BUILD_DIR)/timing.o: ../utils/src/timing.cpp
	$(CXX) $(CFLAGS) -I../utils/include -c ../utils/src/timing.cpp -o $(BUILD_DIR)/timing.o

clean: 
	rm -rf $(BUILD_DIR)


