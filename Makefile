.PHONY: default
default: all

PROJ=cachefx
PROJGUI=gui
QMAKE:=$(word 1,$(wildcard /usr/lib/qt5/bin/qmake /usr/lib64/qt5/bin/qmake $(shell which false)))

export ROOTDIR = $(CURDIR)
export INCLDIR = $(ROOTDIR)/include
export CPPFLAGS=$(MARCH) -O2 -std=c++14 -I$(ROOTDIR)/pugixml/src -I$(INCLDIR)
export LDFLAGS=-O2 $(MARCH)

COMPONENTS = \
	Attacker \
	Cache \
	crypto \
	Victim \
	MMU \
	Profiling \
	PlaintextKeyPairGenerator
	
SRC_FILES = \
	CacheContext.cpp \
	CacheFactory.cpp \
	clargs.cpp \
	Controller.cpp \
	AttackEfficiencyController.cpp \
	EntropyLoss.cpp \
	main.cpp \
	ProfilingEvaluation.cpp \
	Random.cpp \
	pugixml/src/pugixml.cpp
     
.PHONY: components $(COMPONENTS)

components: $(COMPONENTS)

$(COMPONENTS):
		$(MAKE) -C $@
		
%.cpp.o: %.cpp
	$(CXX) ${CPPFLAGS} -c $< -o $@
	
OBJS=$(addsuffix .o, $(SRC_FILES))

.PHONY: ${PROJGUI}
${PROJGUI}: xmlgui/Makefile
	cd xmlgui && $(MAKE)
	mv xmlgui/xmlgui ${PROJGUI}

xmlgui/Makefile: xmlgui/xmlgui.pro
	cd xmlgui && ${QMAKE}
	
$(PROJ): components $(OBJS)
	$(CXX) -o ${PROJ} ${LDFLAGS} $(shell find . -type f -name "*.o" ! -path "./xmlgui/*") ${LDLIBS}

all: $(PROJ) $(PROJGUI)

clean:
	rm -f $(shell find . -name "*.o")
	
include Makefile.dep
