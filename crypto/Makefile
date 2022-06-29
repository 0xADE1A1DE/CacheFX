SRCS=\
	qarma64.c \
	speck.cpp

OBJS=$(addsuffix .o, $(SRCS))

%.cpp.o: %.cpp
	$(CXX) ${CPPFLAGS} -c $< -o $@

%.c.o: %.c
	$(CXX) ${CPPFLAGS} -c $< -o $@  
	
all: ${OBJS}

clean: 
	rm -f *.0

include Makefile.dep