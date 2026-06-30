# @(#)makefile	1.1  07/15/98  @(#)
############################################################################
#
INCDIR    = .
LIBDIR    = .

#
#  OBJS names the objects files necessary to make other targets
#
OBJS  = es.o pes.o que.o tp.o utils.o \
	audTools.o audTpProc.o que.o \
        audPidBufSync.o audTpParse.o mpegTsTools.o

#
#  INCS names the header files necessary to make other targets
#
INCS  = es.h mydefs.h pes.h que.h tp.h utils.h \
	audio.h pidbuf.h

#
#   -D option listings:
#	DEBUG0: add debugging symbol flag and turn to minimal optimizer flag
#	DEBUG1: debugging flag and optimizer flag
#
FLAGS   = -DDEBUG -DWAYNE
DEBUG   = -g -O0

#
#  CCOPTS is used to specify optional arguments to the compiler when it
#  is used to generate object files from C source.

CCOPTS  = -I$(INCDIR) $(FLAGS) $(DEBUG)
LDOPTS  =

#
#
CC 	= gcc
ASM	=
AR	= ar -ru
LD	= ld

#
#   linker archived library
#
USRLIB  = usr.a

#
#   The following suffixes are conventional file extension
#
.SUFFIXES: .c .o .a .x

#
#  Rules ...
#
.c.o:
	$(CC) -c $(CCOPTS) $<

.o.x:
	$(LD) $*.o -o $*.x -l$(USRLIB)

.c.x:
	$(CC) $(CCOPTS) $*.o -o $*.x $(USRLIB)

#
#  Targets ...
#
all: $(USRLIB)

$(USRLIB): $(OBJS) $(INCS)
	$(AR) $(USRLIB) $(OBJS)
	ranlib	$(USRLIB)

std: std.o $(USRLIB)
	$(CC) $(CCOPTS) std.o -o std $(USRLIB)

clean:
	rm -f *.x *.o *.a std

#
#  Dependencies ...
#
