.KEEP_STATE:
.SUFFIXES: .C .c .o


CFLAGS = -g -DUNIX -W


#  Suffix rules
#
.c.o:
	gcc -c $(CFLAGS) $<

.C.o:
	g++ -c $(CFLAGS) $<

.c:
	gcc $(CFLAGS) $(MPEG_FLAGS) -o $(@:%.o=%) $<

all: endian

prog4:	prog4.c
	gcc $(CFLAGS) -o prog4 prog4.c

shmem:   shmem.c
	gcc $(CFLAGS) -o shmem shmem.c -lrt

ip:     ip.c
	gcc $(CFLAGS) -o ip ip.c

readwrite: readwrite.c
	gcc $(CFLAGS) -o readwrite readwrite.c -lrt

mq: mq.c
	gcc $(CFLAGS) -o mq mq.c -lrt

mq_srv: mq_srv.c
	gcc $(CFLAGS) -o mq_srv mq_srv.c -lrt

timer_test: timer_test.c
	gcc $(CFLAGS) -o timer_test timer_test.c -lrt

