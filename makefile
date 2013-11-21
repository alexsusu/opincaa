#------------------------------------------------------------------------------#
# Linux Makefile for OPINCAA API                                               #
#------------------------------------------------------------------------------#

WORKDIR = $(PWD)

CXX = g++

SRC_DIR = src
INC_DIR = include

BUILD_DIR = build
LIB_DIR = libs
TEST_DIR = test
TEST_BUILD_DIR = test_build
ARCH = connex16-hm-generic

CFLAGS = -fPIC -std=c++0x
LDFLAGS_DEBUG =  $(LDFLAGS)

SRC = $(wildcard core/*.cpp)
OBJ = $(patsubst %.cpp, %.o, $(SRC))

TEST_OBJ = $(TEST_BUILD_DIR)/api_test

lib: make_paths build_arch build_utils build_core build_lib

make_paths:
	@test -d $(BUILD_DIR) || mkdir -p $(BUILD_DIR)
	@test -d $(BUILD_DIR)/$(ARCH) || mkdir -p $(BUILD_DIR)/$(ARCH)
	@test -d $(LIB_DIR) || mkdir -p $(LIB_DIR)

build_arch:
	$(CXX) $(CFLAGS) -Iarch/$(ARCH)/include -c arch/$(ARCH)/src/Instruction.cpp -o $(BUILD_DIR)/$(ARCH)/Instruction.o

build_utils:
	$(CXX) $(CFLAGS) -Iutils/include -c utils/src/NamedPipes.cpp -o $(BUILD_DIR)/$(ARCH)/NamedPipes.o

build_core: $(OBJ)

build_lib:
	$(CXX) $(CFLAGS) $(BUILD_DIR)/$(ARCH)/*.o -lc -shared -Wl,-soname,libopincaa.so -o $(LIB_DIR)/libopincaa.so

$(OBJ): %.o : %.cpp
	$(eval obj = $(shell basename $@))
	$(CXX) $(CFLAGS) -I$(INC_DIR) -Iarch/$(ARCH)/include -Iutils/include -c $< -o $(BUILD_DIR)/$(ARCH)/$(obj)


#install: lib
	cp $(LIB_DIR)/libopincaa.so /usr/local/lib
	chmod 0755 /usr/local/lib/libopincaa.so
	ldconfig

clean: 
	rm -rf $(BUILD_DIR)
	rm -rf $(LIB_DIR)
