#### filename: makefile

#### Program Defines
CPLUS      = CC
CPLUSFLAGS = -O $(INCL)
INCL       = -I. -I$(HOME)/Lib/incl -I$(RESSW)/incl
LDPATH     = -L$(RESSW)/lib -L$(HOME)/Lib/lib.$(ARCH)
LDFLAGS    = -O $(LDPATH)
LIBS       = -lressw -lpeh -lm
AR         = ar
ARFLAGS    = cqv

#### File Defines
SRCS = timing.cc
OBJS = $(SRCS:.cc=.o)
TARGETS = acq timing

#### Rules
.KEEP_STATE:
.SUFFIXES: .o .cc
.cc.o:
	$(CPLUS) $(CPLUSFLAGS) -c $<

all: $(TARGETS)
	@echo $(TARGETS) up to date.

install: all

timing: $(OBJS)
	$(CPLUS) $(LDFLAGS) $(OBJS) $(LIBS) -o timing

acq: acq.o
	$(CPLUS) $(LDFLAGS) acq.o $(LIBS) -o acq

## CenterLine stuff
octiming: $(OBJS)
	#setopt load_flags $(INCL) $(LDPATH)
	#load $(LIBS)
	#load $(OBJS)
	#link
	#print "Done with octiming."

ocacq: acq.o
	#setopt load_flags $(INCL) $(LDPATH)
	#load $(LIBS)
	#load acq.o
	#link
	#print "Done with ocacq."


clean:
	sccs clean
	/bin/rm -f a.out core ~* *.o
