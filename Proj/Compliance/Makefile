.KEEP_STATE:
.SUFFIXES: .C .c .o


CFLAGS = -g -Wimplicit -DUNIX


#  Suffix rules
#
.c.o:
	gcc -c $(CFLAGS) $<

.c:
	gcc $(CFLAGS) -o $(@:%.o=%) $<


# Specific builds
#
all:	mpeg2test

mpeg2test: mpeg2test.c mpeg2util.c param.c
	gcc $(CFLAGS) -o mpeg2test mpeg2test.c mpeg2util.c param.c

