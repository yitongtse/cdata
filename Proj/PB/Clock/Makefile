.KEEP_STATE:
.SUFFIXES: .C .c .o

COMDIR	= /users/ytse/Proj/NEW
INCS	= -I$(COMDIR)
LIBS	= $(COMDIR)/mpeg.a $(COMDIR)/util.a
CFLAGS	= -g -Wimplicit -DUNIX $(INCS)


#  Suffix rules
#
.c.o:
	gcc -c $(CFLAGS) $<

.c:
	gcc $(CFLAGS) -o $(@:%.o=%) $< $(LIBS)


#  Specific make
#
sim: sim.c
	gcc $(CFLAGS) sim.c -o sim -lm

timing: timing.c
	gcc $(CFLAGS) timing.c $(LIBS) -o timing -lm

timing1: timing1.c
	gcc $(CFLAGS) timing1.c $(LIBS) -o timing1 -lm

timing2: timing2.c
	gcc $(CFLAGS) timing2.c $(LIBS) -o timing2 -lm

timing3: timing3.c
	gcc $(CFLAGS) timing3.c $(LIBS) -o timing3 -lm

clean:
	rm -f *.o

