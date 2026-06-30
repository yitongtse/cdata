CC = gcc

VPATH = ../common       \
        ../ios          \
        test            \

INCLUDES = -I../common  \
           -I../ios     \
           -I.          \

CFLAGS = -g -Wall -Wimplicit -Werror $(INCLUDES)

LDLIBS = -lrt


TARGETS = ermi1_erm     \
	  ermi1_eqam    \
	  ermi2_erm     \
	  ermi2_eqam    \
          msg_test      \


all: $(TARGETS)

CLI_OBJS = cli.o con_stty.o

ERRP_OBJS = errp_util.o errp_msg_put.o errp_msg_get.o errp_fsm.o \
	    fsm.o util.o os_util_lnx.o

RTSP_OBJS = rtsp_util.o rtsp_msg_get.o rtsp_msg_put.o util.o os_util_lnx.o

ermi1_erm: ermi1_erm.o $(ERRP_OBJS) $(CLI_OBJS)
	$(CC) $(CFLAGS) -o ermi1_erm ermi1_erm.o $(ERRP_OBJS) $(CLI_OBJS) \
	      $(LDLIBS)

ermi1_eqam: ermi1_eqam.o $(ERRP_OBJS) $(CLI_OBJS)
	$(CC) $(CFLAGS) -o ermi1_eqam ermi1_eqam.o $(ERRP_OBJS) $(CLI_OBJS) \
	      $(LDLIBS)

ermi2_erm: ermi2_erm.o $(RTSP_OBJS) $(CLI_OBJS)
	gcc $(CFLAGS) -o ermi2_erm ermi2_erm.o $(RTSP_OBJS) $(CLI_OBJS) \
	    $(LDLIBS)

ermi2_eqam: ermi2_eqam.o $(RTSP_OBJS) $(CLI_OBJS)
	gcc $(CFLAGS) -o ermi2_eqam ermi2_eqam.o $(RTSP_OBJS) $(CLI_OBJS) \
	    $(LDLIBS)

msg_test: msg_test.o $(RTSP_OBJS)
	gcc $(CFLAGS) -o msg_test msg_test.o $(RTSP_OBJS) $(LDLIBS)

clean:
	rm *.o
	rm $(TARGETS)
