# Makefile for video data plane simulation model
# Copyright (c) 2014-2015 by Cisco Systems, Inc.
# All rights reserved

.KEEP_STATE:
.SUFFIXES: .C .c .o

CFLAGS = -g -Wimplicit -DUNIX -Wformat -Wformat-nonliteral -Wformat-extra-args \
         -Werror

INCLUDES =              \
        -Iutil          \
        -Icli           \
        -Ivideo		\
        -Isim     	\

VPATH =              	\
        ./util          \
        ./cli           \
        ./video		\
        ./sim     	\

LIBS = -libc

DEFINES =                               \
        -D__LITTLE_ENDIAN__             \
        -DCLI=1                         \
        -DLOGGER=2                      \
        -DTRACE                         \
        -DDEBUG_QUE=1                   \
        -DCBR                           \
        -DSIM_ERRMSG=1                  \
        -DBENCH_POSIX                   \



#  Suffix rules
#
.c.o:
	gcc -c $(CFLAGS) $(INCLUDES) $(DEFINES) $<

.C.o:
	g++ -c $(CFLAGS) $(INCLUDES) $(DEFINES) $<

.c:
	gcc $(CFLAGS) $(INCLUDES) $(DEFINES) -o $(@:%.o=%) $<


# Specific builds
#
#all: sim_video sim_video_srv sim_cli
all: analyzer sim_cli sim_video_srv pid_stat2.cgi prog_stat2.cgi psi_table.cgi

upload:
	cp *.cgi /home/pi/Hackathon/Site/cgi-bin/

clean:
	rm -f analyzer sim_video_srv sim_cli *.cgi


ANALYZER_SRC =                 	\
        sim/analyzer.c         	\
        sim/sim_json.c         	\
        sim/sim_video_msg.c     \
        sim/sim_video_event.c   \
        sim/sim_video_cli.c     \
        sim/sim_video_util.c    \
	sim/sim_video_lcred.c	\
        cli/con_stty.c          \
        cli/cli_common.c        \
        util/crc.c              \
        util/hash.c             \
        util/logger.c           \
        util/param.c            \
        util/phy_mem.c          \
        util/pool.c             \
        util/prio_que.c         \
        util/que.c              \
        util/trace.c            \
        util/util.c             \
        util/thread_stat.c      \
        video/video.c           \
        video/video_msg.c       \
        video/video_event.c     \
        video/video_cli.c       \
        video/video_clkrec.c    \
        video/video_config.c    \
        video/video_demux.c     \
        video/video_input.c     \
        video/video_monitor.c   \
        video/video_mux.c       \
        video/video_output.c    \
        video/video_psi.c       \
        video/video_psi_out.c   \
        video/video_record.c    \
        video/video_psi_record.c\
        video/video_util.c      \
        video/video_ca.c        \
        video/video_config_db.c \

analyzer: $(ANALYZER_SRC)
	gcc $(CFLAGS) $(INCLUDES) $(DEFINES) $(ANALYZER_SRC)   \
            -o analyzer -lpthread -lrt


SIM_VIDEO_SRC =                 \
        sim/sim_video.c         \
        sim/sim_video_msg.c     \
        sim/sim_video_event.c   \
        sim/sim_video_cli.c     \
        sim/sim_video_util.c    \
	sim/sim_video_lcred.c	\
        cli/con_stty.c          \
        cli/cli_common.c        \
        util/crc.c              \
        util/hash.c             \
        util/logger.c           \
        util/param.c            \
        util/phy_mem.c          \
        util/pool.c             \
        util/prio_que.c         \
        util/que.c              \
        util/trace.c            \
        util/util.c             \
        util/thread_stat.c      \
        video/video.c           \
        video/video_msg.c       \
        video/video_event.c     \
        video/video_cli.c       \
        video/video_clkrec.c    \
        video/video_config.c    \
        video/video_demux.c     \
        video/video_input.c     \
        video/video_monitor.c   \
        video/video_mux.c       \
        video/video_output.c    \
        video/video_psi.c       \
        video/video_psi_out.c   \
        video/video_record.c    \
        video/video_psi_record.c\
        video/video_util.c      \
        video/video_ca.c        \
        video/video_config_db.c \

sim_video: $(SIM_VIDEO_SRC)
	gcc $(CFLAGS) $(INCLUDES) $(DEFINES) $(SIM_VIDEO_SRC)   \
            -o sim_video -lpthread -lrt


SIM_VIDEO_SRV_SRC =             \
        sim/sim_video_srv.c     \
        util/util.c        	\
        util/param.c       	\

sim_video_srv: $(SIM_VIDEO_SRV_SRC)
	gcc $(CFLAGS) $(INCLUDES) $(DEFINES) $(SIM_VIDEO_SRV_SRC)       \
            -o sim_video_srv -lpthread -lrt


SIM_CLI_SRC =                   \
        util/logger.c           \
        util/util.c             \
        cli/generic_cli.c      	\
        cli/con_stty.c         	\
        cli/cli_common.c       	\
        sim/sim_cli.c           \
        sim/sim_cli_video.c     \


sim_cli: $(SIM_CLI_SRC)
	gcc $(CFLAGS) $(INCLUDES) $(DEFINES) $(SIM_CLI_SRC)     \
            -o sim_cli -lpthread -lrt


PID_STAT_SRC =                  \
        cgi/cgi_util.c     	\
        cgi/pid_stat.c     	\

pid_stat.cgi: $(PID_STAT_SRC)
	gcc $(CFLAGS) $(INCLUDES) $(DEFINES) $(PID_STAT_SRC)     \
            -o pid_stat.cgi -lpthread -lrt


PID_STAT2_SRC =                 \
        cgi/cgi_util.c     	\
        cgi/pid_stat2.c     	\

pid_stat2.cgi: $(PID_STAT2_SRC)
	gcc $(CFLAGS) $(INCLUDES) $(DEFINES) $(PID_STAT2_SRC)     \
            -o pid_stat2.cgi -lpthread -lrt


PROG_STAT_SRC =                 \
        cgi/cgi_util.c     	\
        cgi/prog_stat.c     	\

prog_stat.cgi: $(PROG_STAT_SRC)
	gcc $(CFLAGS) $(INCLUDES) $(DEFINES) $(PROG_STAT_SRC)     \
            -o prog_stat.cgi -lpthread -lrt


PROG_STAT2_SRC =                 \
        cgi/cgi_util.c     	\
        cgi/prog_stat2.c     	\

prog_stat2.cgi: $(PROG_STAT2_SRC)
	gcc $(CFLAGS) $(INCLUDES) $(DEFINES) $(PROG_STAT2_SRC)     \
            -o prog_stat2.cgi -lpthread -lrt


PSI_TABLE_SRC =                  \
        cgi/cgi_util.c     	\
        cgi/psi_table.c     	\

psi_table.cgi: $(PID_STAT_SRC)
	gcc $(CFLAGS) $(INCLUDES) $(DEFINES) $(PSI_TABLE_SRC)     \
            -o psi_table.cgi -lpthread -lrt


