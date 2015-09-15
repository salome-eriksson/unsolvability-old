#add your path to the cudd package here
CUDD_DIR=$(HOME)/progs/cudd

CXX = g++
CXXFLAGS = -g -O3 -m32
CUDD_LIBS = obj cudd mtr st util epd dddmp
CUDD_CXXFLAGS = -I$(CUDD_DIR)/include
CUDD_LDFLAGS += -static $(foreach lib,$(CUDD_LIBS),-L$(CUDD_DIR)/$(lib) -l$(lib))

all: test

test: testobj.cc
	$(CXX) $(CXXFLAGS) testobj.cc -o test $(CUDD_CXXFLAGS) $(CUDD_LDFLAGS)

clean-test:
	rm test
