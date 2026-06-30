.KEEP_STATE:
.SUFFIXES: .C .c .o

COMDIR  = /users/ytse/Proj/NEW
INCS    = -I$(COMDIR)
LIBS    = $(COMDIR)/mpeg.a $(COMDIR)/util.a
CFLAGS  = -g -Wimplicit -DUNIX $(INCS)


#  Suffix rules
#
.c.o:
	gcc -c $(CFLAGS) $<

.c:
	gcc $(CFLAGS) -o $(@:%.o=%) $< $(LIBS)

clean:
	rm -f *.o


# Specific makes
#
# pb_in_fifo: pb_in_fifo.c ipdat_1x24.c
# 	gcc $(CFLAGS) ipdat_1x24.o pb_in_fifo.c -o pb_in_fifo


