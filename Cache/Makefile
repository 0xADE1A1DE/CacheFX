SRCS=\
	CEASERCache.cpp \
	CEASERSCache.cpp \
	NewCache.cpp \
	PhantomCache.cpp \
	PLcache.cpp \
	ScatterCache.cpp \
	SetAssocCache.cpp \
	WayPartitionCache.cpp \
	AssocCache.cpp \
	CacheHierarchy.cpp

OBJS=$(addsuffix .o, $(SRCS))

%.cpp.o: %.cpp
	$(CXX) ${CPPFLAGS} -c $< -o $@
	
all: ${OBJS}

clean: 
	rm -f *.0
	
include Makefile.dep
	