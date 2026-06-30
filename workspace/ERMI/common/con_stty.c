/*
 * Copyright (c) 2006, 2010 by Cisco Systems, Inc.
 * All rights reserved.
 */

#include <stdlib.h>
#include <sys/utsname.h>
#include <sys/types.h>   /* basic system data types */
#include <sys/socket.h>  /* basic socket definitions */
#include <sys/time.h>    /* timeval{} for select() */
#include <time.h>                /* timespec{} for pselect() */
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <errno.h>
#include <fcntl.h>               /* for nonblocking */
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>    /* for S_xxx file mode constants */
#include <sys/uio.h>             /* for iovec{} and readv/writev */
#include <unistd.h>
#include <sys/wait.h>
#include <sys/un.h>              /* for Unix domain sockets */
#include <sys/select.h>  /* for convenience */
#include <termios.h>
#include <strings.h>


#define MAXLINE         4096    /* max text line length */

static struct termios con_stty_tbufsave;
static int con_stty_have_attr = 0;

int con_stty_tc_setraw (void)
{
    struct termios tbuf;
    long disable;
    int i;

    disable = _POSIX_VDISABLE;
    tcgetattr(STDIN_FILENO, &tbuf);
    con_stty_have_attr = 1;
    con_stty_tbufsave = tbuf;
    tbuf.c_cflag &= ~(CSIZE | PARENB);
    tbuf.c_cflag |= CS8;
    tbuf.c_iflag &= ~(INLCR | ICRNL | ISTRIP | INPCK | IXON);
    tbuf.c_lflag &= ~(ICANON | ISIG | IEXTEN | ECHO);
    for (i = 0; i < NCCS; i++)
            tbuf.c_cc[i] = (cc_t)disable;
    tbuf.c_cc[VMIN] = 5;
    tbuf.c_cc[VTIME] = 2;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &tbuf);
    return(0);
}


int con_stty_tc_restore (void)
{
    if (con_stty_have_attr) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &con_stty_tbufsave);
    }
    return(0);
}

char con_stty_getch (void)
{
    char c;

    switch(read(STDIN_FILENO, &c, 1)) {
    default:
            errno = 0;
            /* fall through */
    case -1:
            return(-1);
    case 1:
            break;
    }
    return(c);
}

int con_stty_print_stty_raw_info (void)
{
    setbuf(stdout, NULL);
    printf("Initial attributes:\n");
    system("stty | fold -s -w 60");
    printf("\r\nRaw attributes:\n");
    con_stty_tc_setraw();
    system("stty | fold -s -w 60");
    con_stty_tc_restore();
    printf("\r\nRestored attributes:\n");
    system("stty | fold -s -w 60");
    return(0);
}
