#------------------------------------------------------------------------------#
# Linux Makefile for CONNEX TESTS                                              #
#------------------------------------------------------------------------------#


WORKDIR = `pwd`

CXX = g++
#CXXFLAGS = -g

INC_DIR = include
BUILD_DIR = build
SRC_DIR = src
LIB_DIR = libs

# Alex: added $(CXXFLAGS) that can allow to compile with debugging support
CFLAGS = -fPIC -std=c++0x $(CXXFLAGS)
LDFLAGS_DEBUG =  $(LDFLAGS)

OBJ = $(BUILD_DIR)/test.o $(BUILD_DIR)/simple_tests.o $(BUILD_DIR)/simple_io_tests.o $(BUILD_DIR)/controller_tests.o

ARCH = connex16-hm-generic

all: make_paths build

make_paths: 
	test -d $(BUILD_DIR) || mkdir -p $(BUILD_DIR)

build: $(OBJ)
	$(CXX) -O3 $(CFLAGS) -I$(INC_DIR) $(OBJ) -L../libs/$(ARCH) -o $(BUILD_DIR)/test -lopincaa -fopenmp

$(BUILD_DIR)/test.o: $(SRC_DIR)/test.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -I../include -I../arch/$(ARCH)/include -c $(SRC_DIR)/test.cpp -o $(BUILD_DIR)/test.o

$(BUILD_DIR)/simple_tests.o: $(SRC_DIR)/simple_tests.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -I../include -I../arch/$(ARCH)/include -c $(SRC_DIR)/simple_tests.cpp -o $(BUILD_DIR)/simple_tests.o

$(BUILD_DIR)/controller_tests.o: $(SRC_DIR)/controller_tests.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -I../include -I../arch/$(ARCH)/include -c $(SRC_DIR)/controller_tests.cpp -o $(BUILD_DIR)/controller_tests.o
	
$(BUILD_DIR)/simple_io_tests.o: $(SRC_DIR)/simple_io_tests.cpp
	$(CXX) $(CFLAGS) -I$(INC_DIR) -I../include -I../arch/$(ARCH)/include -c $(SRC_DIR)/simple_io_tests.cpp -o $(BUILD_DIR)/simple_io_tests.o

clean: 
	rm -rf $(BUILD_DIR)


