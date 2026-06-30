# Comment out the CC below to use the default gcc compiler for the host
# CC = powerpc-linux-gnu-gcc

CFLAGS = -g -Wimplicit
LDLIBS = -lrt

TARGETS = test

TEST_OBJS = param.o \
	    lnx_test.o \

all: $(TARGETS)

test: $(TEST_OBJS)
	$(CC) $(CFLAGS) -o lnx_test $(TEST_OBJS) $(LDLIBS)


clean:
	rm $(TARGETS)
	rm *.o

