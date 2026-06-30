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
b2a: b2a.c utils.c
	gcc $(CFLAGS) b2a.c utils.c -o b2a

clean:
	rm -f *.o

