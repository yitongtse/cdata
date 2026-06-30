QNX_HOST = /vob/qnx/sdk/host/solaris-qnx6.2.1/sparc/
QNX_TARGET = /vob/qnx/sdk/target/qnx621
TOOL_PATH = $(QNX_HOST)/usr/bin
CC      = $(TOOL_PATH)/qcc -V gcc_ntoppcbe
OBJDUMP = $(TOOL_PATH)/objdump


%.o: %.c
	$(CC) -g -c $<

mb_bench:	mb_bench.c
	$(CC) -g -c -O mb_bench.c
	$(OBJDUMP) -S mb_bench.o > mb_bench.lst

test:	test.c
	$(CC) -g -c -O test.c

clear:
	rm *.o
