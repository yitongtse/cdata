.KEEP_STATE:
.SUFFIXES: .C .c .o


CFLAGS = -g -Wimplicit -DUNIX


#  Suffix rules
#
.c.o:
	gcc -c $(CFLAGS) $<

.C.o:
	g++ -c $(CFLAGS) $<

.c:
	gcc $(CFLAGS) $(MPEG_FLAGS) -o $(@:%.o=%) $<


# Specific builds
#
extract: extract.c
	gcc $(CFLAGS) extract.c -o extract -lsocket


clean:
	rm -f *.o *.a

