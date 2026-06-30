CC = gcc

VPATH = ../common       \
        ../ios          \

INCLUDES = -I../common  \
           -I../ios     \

CFLAGS = -g -Wall -Wimplicit -Werror $(INCLUDES)

LDLIBS = -lrt

TARGETS =               \
          msg_test      \
          ermi2_erm     \
          ermi2_eqam    \


all: $(TARGETS)

CLI_OBJS = cli.o con_stty.o

RTSP_OBJS = rtsp_util.o rtsp_msg_get.o rtsp_msg_put.o util.o os_util_lnx.o

msg_test: msg_test.o $(RTSP_OBJS)
	gcc $(CFLAGS) -o msg_test msg_test.o $(RTSP_OBJS) $(LDLIBS)

ermi2_erm: ermi2_erm.o $(RTSP_OBJS) $(CLI_OBJS)
	gcc $(CFLAGS) -o ermi2_erm ermi2_erm.o $(RTSP_OBJS) $(CLI_OBJS) \
	    $(LDLIBS)

ermi2_eqam: ermi2_eqam.o $(RTSP_OBJS) $(CLI_OBJS)
	gcc $(CFLAGS) -o ermi2_eqam ermi2_eqam.o $(RTSP_OBJS) $(CLI_OBJS) \
	    $(LDLIBS)

clean:
	rm *.o
	rm $(TARGETS)
