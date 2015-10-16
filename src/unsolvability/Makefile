#add your path to the cudd package here
CUDD_DIR=$(HOME)/progs/cudd

CXX = g++
CXXFLAGS = -g -O0 -m32
CUDD_LIBS = obj cudd mtr st util epd dddmp
CUDD_CXXFLAGS = -I$(CUDD_DIR)/include
CUDD_LDFLAGS += -static $(foreach lib,$(CUDD_LIBS),-L$(CUDD_DIR)/$(lib) -l$(lib))


HEADERS = \
          global_funcs.h \
	  task.h \
          certificate.h \
          simple_certificate.h \

SOURCES = verify.cc $(HEADERS:%.h=%.cc)
OBJ = $(SOURCES:%.cc=%.o)

all: verify

%.o: %.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) $(CUDD_CXXFLAGS) -c $< -o $@

verify: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(CUDD_LDFLAGS)

test: testobj.cc
	$(CXX) -m32  testobj.cc -o test $(CUDD_CXXFLAGS) $(CUDD_LDFLAGS)

clean-obj:
	rm -f *.o *~

clean:
	rm -f *.o *~ 
	rm -f verify
	rm -f test
