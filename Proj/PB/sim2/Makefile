.KEEP_STATE:
.SUFFIXES: .C .c .o

COMDIR	= /users/ytse/Proj/NEW
INCS	= -I$(COMDIR)
LIBS	= $(COMDIR)/util.a
CFLAGS  = -g -Wimplicit -DUNIX $(INCS)


#  Suffix rules
#
.c.o:
	gcc -c $(CFLAGS) $<

.c:
	gcc $(CFLAGS) -o $(@:%.o=%) $<


# Specific makes
#
all:	pbsim

pbsim1:
	gcc $(CFLAGS) -o pbsim1 pbsim1.c $(LIBS)

pbsim:
	gcc $(CFLAGS) -o pbsim pbsim.c $(LIBS)

clean:
	rm -f *.o

