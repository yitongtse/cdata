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
all:	ermi1_test

ermi1_test: ermi1_test.c ermi1_msg_gen.c ermi1_msg_parse.c rfgw_errp_util.c
	gcc $(CFLAGS) ermi1_msg_gen.c ermi1_msg_parse.c ermi1_test.c \
	    rfgw_errp_util.c -o ermi1_test

clean:
	rm -f *.o *.a ermi1_test

