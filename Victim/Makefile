SRCS=\
	AESVictim.cpp \
	SingleAccessVictim.cpp \
	SquareMultVictim.cpp \
	BinaryVictim.cpp

OBJS=$(addsuffix .o, $(SRCS))

%.cpp.o: %.cpp
	$(CXX) ${CPPFLAGS} -c $< -o $@
	
all: ${OBJS}

clean: 
	rm -f *.0
	
include Makefile.dep