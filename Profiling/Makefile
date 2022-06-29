SRCS=\
	EvictionSetSizeProfiling.cpp \

OBJS=$(addsuffix .o, $(SRCS))

%.cpp.o: %.cpp
	$(CXX) ${CPPFLAGS} -c $< -o $@
	
all: ${OBJS}
	$(MAKE) -C FilledCacheProfiling

clean: 
	rm -f *.0
	
include Makefile.dep