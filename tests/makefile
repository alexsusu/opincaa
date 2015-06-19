#------------------------------------------------------------------------------#
# Linux Makefile for CONNEX TESTS                                              #
#------------------------------------------------------------------------------#


WORKDIR = `pwd`

CXX = g++

INC_DIR = include
BUILD_DIR = build
SRC_DIR = src
LIB_DIR = libs

CFLAGS = -fPIC -std=c++0x -g
LDFLAGS_DEBUG =  $(LDFLAGS)

OBJ = $(BUILD_DIR)/test.o $(BUILD_DIR)/simple_tests.o $(BUILD_DIR)/simple_io_tests.o $(BUILD_DIR)/controller_tests.o

ARCH = connex-rc

.PHONY: all
all: make_paths build

.PHONY: make_paths
make_paths: 
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/test_connex-rc_sim: $(SRC_DIR)/test_connex-rc_sim.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -I../include -I../arch/$(ARCH)/include $(SRC_DIR)/test_connex-rc_sim.cpp -o $(BUILD_DIR)/test_connex-rc_sim -lopincaa 

#build: $(OBJ)
#	$(CXX) -O3 $(CFLAGS) -I$(INC_DIR) $(OBJ) -L../libs/$(ARCH) -o $(BUILD_DIR)/test -lopincaa -fopenmp
 

#$(BUILD_DIR)/simple_tests.o: $(SRC_DIR)/simple_tests.cpp
#	$(CXX) $(CFLAGS) -I$(INC_DIR) -I../include -I../arch/$(ARCH)/include -c $(SRC_DIR)/simple_tests.cpp -o $(BUILD_DIR)/simple_tests.o

#$(BUILD_DIR)/controller_tests.o: $(SRC_DIR)/controller_tests.cpp
#	$(CXX) $(CFLAGS) -I$(INC_DIR) -I../include -I../arch/$(ARCH)/include -c $(SRC_DIR)/controller_tests.cpp -o $(BUILD_DIR)/controller_tests.o
	
#$(BUILD_DIR)/simple_io_tests.o: $(SRC_DIR)/simple_io_tests.cpp
#	$(CXX) $(CFLAGS) -I$(INC_DIR) -I../include -I../arch/$(ARCH)/include -c $(SRC_DIR)/simple_io_tests.cpp -o $(BUILD_DIR)/simple_io_tests.o

.PHONY: clean
clean: 
	rm -rf $(BUILD_DIR)


