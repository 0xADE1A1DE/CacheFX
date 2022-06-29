SRCS=\
	FilledCacheProfiling.cpp \
	FilledCacheProfilingGroupElim.cpp \
	FilledCacheProfilingLRU.cpp \
	FilledCacheProfilingProbabilistic.cpp \
	FilledCacheProfilingSingleHold.cpp
	
OBJS=$(addsuffix .o, $(SRCS))

%.cpp.o: %.cpp
	$(CXX) ${CPPFLAGS} -c $< -o $@
	
all: ${OBJS}

clean: 
	rm -f *.0
	
include Makefile.dep