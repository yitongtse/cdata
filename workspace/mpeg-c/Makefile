# Makefile for offline tools
# Copyright (c) 2014 by Cisco Systems, Inc.
# All rights reserved
#

.KEEP_STATE:
.SUFFIXES: .C .c .o

CFLAGS = -g -Wimplicit -DUNIX

INCLUDES =                      \
        -I../util               \
        -I../video_common       \
        -I../ng_video           \
        -I../../../cdman/infra/include  \
        -I../../../cdman/infra/src      \

LIBS = -libc

DEFINES =                       \
        -D__LITTLE_ENDIAN__     \
        -DBENCH_POSIX           \
        -DNUM_QAMS=48           \
        -DFLOWS_PER_QAM=32      \
        -DNUM_INPUT_PORTS=1     \
        -DECM_INSERT_CNT=0      \
        -DSES_HASH_SLOTS=997    \
        -DMIN_LATENCY=40        \
        -DMAX_JITTER=200        \
        -DBUF_HEADROOM=5        \
        -DOUT_CMD_PER_FLOW=2048 \
        -DOUT_WIN_SIZE_MS=20    \

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
#all: timerec_view timerec_analy ts_viewer psi_viewer
all: ts_viewer psi_viewer ts_extract


clean:
	rm -f *.o
	rm -f timerec_view
	rm -f timerec_analy
	rm -f clkrec_sim
	rm -f ts_viewer
	rm -f psi_viewer
	rm -f ts_extract


TIMEREC_VIEW_SRC =              \
        ../util/param.c         \
        timerec.c               \
        timerec_view.c          \

timerec_view: $(TIMEREC_VIEW_SRC)
	gcc $(CFLAGS) $(INCLUDES) $(DEFINES) $(TIMEREC_VIEW_SRC) \
            -o timerec_view -lrt


TIMEREC_ANALY_SRC =             \
        ../util/param.c         \
        timerec.c               \
        timerec_analy.c         \

timerec_analy: $(TIMEREC_ANALY_SRC)
	gcc $(CFLAGS) $(INCLUDES) $(DEFINES) $(TIMEREC_ANALY_SRC) \
            -o timerec_analy -lrt


CLKREC_SIM_SRC =                \
        ../util/param.c         \
        timerec.c               \
        clkrec_sim.c            \

clkrec_sim: $(CLKREC_SIM_SRC)
	gcc $(CFLAGS) $(INCLUDES) $(DEFINES) -DMAX_MB_SLOT_ID=1 $(CLKREC_SIM_SRC) \
            -o clkrec_sim -lrt


TS_VIEWER_SRC =                 \
        ../util/param.c         \
        ts_viewer.c             \

ts_viewer: $(TS_VIEWER_SRC)
	gcc $(CFLAGS) $(INCLUDES) $(DEFINES) $(TS_VIEWER_SRC) \
            -o ts_viewer -lrt


PSI_VIEWER_SRC =                \
        ../util/param.c         \
        psi_viewer.c            \

psi_viewer: $(PSI_VIEWER_SRC)
	gcc $(CFLAGS) $(INCLUDES) $(DEFINES) $(PSI_VIEWER_SRC) \
            -o psi_viewer -lrt


TS_EXTRACT_SRC =                \
        ../util/param.c         \
        ts_extract.c            \

ts_extract: $(TS_EXTRACT_SRC)
	gcc $(CFLAGS) $(INCLUDES) $(DEFINES) $(TS_EXTRACT_SRC) \
            -o ts_extract -lrt

