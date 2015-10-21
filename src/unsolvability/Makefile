#add your path to the cudd package here
CUDD_DIR=$(HOME)/progs/cudd

CXX = g++
CXXFLAGS = -g -O3 -m32 -std=c++11
CUDD_LIBS = obj cudd mtr st util epd dddmp
CUDD_CXXFLAGS = -I$(CUDD_DIR)/include
CUDD_LDFLAGS = -static $(foreach lib,$(CUDD_LIBS),-L$(CUDD_DIR)/$(lib) -l$(lib))

CXXFLAGS += $(CUDD_CXXFLAGS)

DEPEND = $(CXX) -MM

HEADERS = \
          global_funcs.h \
	  task.h \
          certificate.h \
          simple_certificate.h \

SOURCES = verify.cc $(HEADERS:%.h=%.cc)
OBJ = $(SOURCES:%.cc=%.o)

all: verify

$(OBJ): %.o: %.cc
	$(CXX) $(CXXFLAGS)  -c $< -o $@

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
	rm -f Makefile.depend

Makefile.depend: $(SOURCES) $(HEADERS)
	rm -f Makefile.depend
	for source in $(SOURCES) ; do \
	    $(DEPEND) $(CXXFLAGS) $$source > Makefile.temp0; \
	    objfile=$${source%%.cc}.o; \
	    sed -i -e "s@^[^:]*:@$$objfile:@" Makefile.temp0; \
	    cat Makefile.temp0 >> Makefile.depend; \
	done
	rm -f Makefile.temp0 #Makefile.depend
#	sed -e "s@\(.*\)\.o:\(.*\)@.obj/\1$(OBJECT_SUFFIX_RELEASE).o:\2@" Makefile.temp >> Makefile.depend
#	sed -e "s@\(.*\)\.o:\(.*\)@.obj/\1$(OBJECT_SUFFIX_DEBUG).o:\2@" Makefile.temp >> Makefile.depend
#	sed -e "s@\(.*\)\.o:\(.*\)@.obj/\1$(OBJECT_SUFFIX_PROFILE).o:\2@" Makefile.temp >> Makefile.depend
#	rm -f Makefile.temp

ifneq ($(MAKECMDGOALS),clean)
    ifneq ($(MAKECMDGOALS),distclean)
        include Makefile.depend
    endif
endif
