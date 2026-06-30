##############################################################################
#    Copyright (c) 2003 by Cisco Systems, Inc.
#    All rights reserved.
#
#    File name : makefile
#
#    Date      Who	Modification
#    --------  --------	------------------------------------------------------
#    03-26-03  ytse	Created.
#
#    This is the makefile for the Clock Recovery Simulation
##############################################################################

.KEEP_STATE:
.SUFFIXES: .c .o

CFLAGS	= -g -Wall -DUNIX
PROGS	= uqam_sim

# Source code search path
VPATH = 

INCDIR	= 


#  Suffix rules
#
.c.o:
	gcc -c $(CFLAGS) $(INCDIR) $<

.c:
	gcc $(CFLAGS) $(INCDIR) -o $(@:%.o=%) $<


# Specific builds
#
all:	$(PROGS)

tsgen: tsgen.c param.o
	gcc $(CFLAGS) $(INCDIR) -o tsgen tsgen.c param.o -lm

clkrec: clkrec.c param.o
	gcc $(CFLAGS) $(INCDIR) -o clkrec clkrec.c param.o -lm

tmp: tmp.c param.o
	gcc $(CFLAGS) $(INCDIR) -o tmp tmp.c param.o -lm

clean:
	rm -f *.o *.a $(PROGS)

