SRCS=\
	EvictionAttacker.cpp \
	OccupancyAttacker.cpp \
	Attacker.cpp
	# SquareMultAttacker.cpp

OBJS=$(addsuffix .o, $(SRCS))

%.cpp.o: %.cpp
	$(CXX) ${CPPFLAGS} -c $< -o $@
	
all: ${OBJS}

clean: 
	rm -f *.0
	
include Makefile.dep