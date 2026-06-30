CC = gcc

VPATH = ../common       \
        ../ios          \

INCLUDES = -I../common  \
           -I../ios     \

CFLAGS = -g -Wall -Wimplicit -Werror $(INCLUDES)

LDLIBS = -lrt


TARGETS = ermi1_erm  \
	  ermi1_eqam \


all: $(TARGETS)

CLI_OBJS = cli.o con_stty.o

ERRP_OBJS = errp_util.o errp_msg_put.o errp_msg_get.o errp_fsm.o \
	    fsm.o util.o os_util_lnx.o

ermi1_erm: ermi1_erm.o $(ERRP_OBJS) $(CLI_OBJS)
	$(CC) $(CFLAGS) -o ermi1_erm ermi1_erm.o $(ERRP_OBJS) $(CLI_OBJS) \
	      $(LDLIBS)

ermi1_eqam: ermi1_eqam.o $(ERRP_OBJS) $(CLI_OBJS)
	$(CC) $(CFLAGS) -o ermi1_eqam ermi1_eqam.o $(ERRP_OBJS) $(CLI_OBJS) \
	      $(LDLIBS)

clean:
	rm *.o
	rm $(TARGETS)
