VPATH = test

INCLUDES = -Itest

CFLAGS = -g -Wimplicit $(INCLUDES)

LDLIBS = -lrt


TARGETS = cli_test



all: $(TARGETS)

CLI_OBJS = con_stty.o cli.o

cli_test: cli_test.o $(CLI_OBJS)
	gcc $(CFLAGS) -o cli_test cli_test.o $(CLI_OBJS) $(LDLIBS)

clean:
	rm *.o
	rm $(TARGETS)

