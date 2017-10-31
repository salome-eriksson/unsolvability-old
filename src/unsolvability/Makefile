#add your path to the cudd package here
CUDD_DIR=$(DOWNWARD_CUDD_ROOT)

CXX = g++
CXXFLAGS = -g --debug -m32 -std=c++11
CUDD_CXXFLAGS = -I$(CUDD_DIR)/include
CUDD_LDFLAGS = -static -L$(CUDD_DIR)/lib -lcudd

CXXFLAGS += $(CUDD_CXXFLAGS)

DEPEND = $(CXX) -MM

HEADERS = \
          global_funcs.h \
          task.h \
	  timer.h \
	  knowledgebase.h \
	  rulechecker.h \
	  statementchecker.h \
	  statementcheckerbdd.h \
	  statementcheckerhorn.h \
	  stateset.h \

SOURCES = verify.cc $(HEADERS:%.h=%.cc)
OBJ = $(SOURCES:%.cc=%.o)

TESTSOURCES = testverify.cc $(HEADERS:%.h=%.cc)
TESTOBJ = $(TESTSOURCES:%cc.=%.o)

all: verify

$(OBJ): %.o: %.cc
	$(CXX) $(CXXFLAGS)  -c $< -o $@

verify: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(CUDD_LDFLAGS)

testverify: $(TESTOBJ)
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
