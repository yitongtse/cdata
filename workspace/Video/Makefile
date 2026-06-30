.KEEP_STATE:
.SUFFIXES: .C .c .o

CFLAGS = -g -Wimplicit -DUNIX

INCLUDES =                      \
        -I../util               \
        -I../video_common       \
        -I../ng_video           \
        -I../../../cdman/infra/include  \
        -I../../../cdman/infra/src      \

LIBS = -llibc

DEFINES =                       \
        -D_BIG_ENDIAN=0         \
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

.c:     $@
	gcc $(CFLAGS) $(INCLUDES) $(DEFINES) -o $(@:%.o=%) $<


# Specific builds
#
all: ts_viewer pat_viewer pmt_viewer

clean:
	rm -f *.o
	rm -f ts_viewer
	rm -f pat_viewer
	rm -f pmt_viewer


TS_VIEWER_SRC =                 \
	param.c                 \
	ts_viewer.c             \

PAT_VIEWER_SRC =                \
	param.c                 \
	pat_viewer.c            \

PMT_VIEWER_SRC =                \
	param.c                 \
	pmt_viewer.c            \


ts_viewer: $(TS_VIEWER_SRC)
	gcc $(CFLAGS) $(INCLUDES) $(DEFINES) $(TS_VIEWER_SRC) \
            -o ts_viewer

pat_viewer: $(PAT_VIEWER_SRC)
	gcc $(CFLAGS) $(INCLUDES) $(DEFINES) $(PAT_VIEWER_SRC) \
            -o pat_viewer

pmt_viewer: $(PMT_VIEWER_SRC)
	gcc $(CFLAGS) $(INCLUDES) $(DEFINES) $(PMT_VIEWER_SRC) \
            -o pmt_viewer

test1: test1.c
	gcc $(CFLAGS) $(INCLUDES) test1.c -o test1

pthread_test: pthread_test.c
	gcc $(CFLAGS) $(INCLUDES) pthread_test.c -o pthread_test -lrt

sem_test: sem_test.c
	gcc $(CFLAGS) $(INCLUDES) sem_test.c -o sem_test -lrt

mutex_test: mutex_test.c
	gcc $(CFLAGS) $(INCLUDES) mutex_test.c -o mutex_test -lrt


