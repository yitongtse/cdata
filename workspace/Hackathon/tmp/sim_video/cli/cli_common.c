/*
 * Copyright (c) 2014-2015 by Cisco Systems, Inc.
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
#include <stdarg.h>

#include "common.h"
#include "cli_common.h"

int gen_peer_flag;
static char tty_name[16];

int cli_peer_init (void)
{
    int rc = ttyname_r(STDOUT_FILENO, tty_name, 16);
    if (rc != 0) {
        perror(NULL);
        return errno;
    }
    gen_peer_flag = 0;
    return EOK;
}

int cli_printf (cli_control_block *cc, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int err = vfprintf(cc->fp, fmt, ap);
    va_end(ap);
    return err;
}

int cli_prepare_cc (cli_control_block *cc)
{
    int i;
    for (i = 0; i < MAX_INPUT_ELEMS; i++) {
        if (cc->tokens[i]) {
            cc->tokens[i] += (uintptr_t)cc->input_tokens;
        }
    }
    return 0;
}


// Parse an non-negative integer range in the format of
//     <start>[-<stop>[*<inc>]]
// Returns -1 on error.
//
int cli_parse_range_int (char* range, int* start, int* stop, int* inc)
{
    char* token;

    if ((*range < '0') || (*range > '9')) {
        return -1;
    }

    token = strtok(range, "-");
    *start = atoi(token);
    *stop = *start;
    *inc = 1;

    token = strtok(NULL, "*");
    if (!token) {
        return 0;
    }
    *stop = atoi(token);
    if (*start >= *stop) {
        return -1;
    }

    token = strtok(NULL, "");
    if (!token) {
        return 0;
    }
    *inc = atoi(token);
    if (*inc < 2) {
        return -1;
    }
    return 0;
}


// Parse an IP address and determine its address family (through af)
//   Returns 1 if successful, 0 on error.
//
int cli_parse_ip_addr (char* token, int* af, uint32 ip_addr[])
{
    *af = AF_INET;
    if (inet_pton(*af, token, ip_addr))  return 1;

    *af = AF_INET6;
    if (inet_pton(*af, token, ip_addr))  return 1;

    return 0;
}


// Parse an IP address range in the format of <start_ip>[-<stop_ip>[*<inc>]]
//   Returns -1 on error.
int cli_parse_range_ip_addr (char* range, int* af, uint32 start_ip[],
                             uint32 stop_ip[], int* inc)
{
    char* token = strtok(range, "-");

    if (cli_parse_ip_addr(token, af, start_ip) == 0) {
        return -1;
    }

    *stop_ip = *start_ip;
    *inc = 1;

    token = strtok(NULL, "*");
    if (!token) {
        return 0;
    }
    if (inet_pton(*af, token, stop_ip) == 0) {
        return -1;
    }

    if (*af == AF_INET) {
        if (start_ip[0] >= stop_ip[0]) {
            return -1;
        }
    } else {
        if (start_ip[0] > stop_ip[0]) {
            return -1;
        } else if (start_ip[0] == stop_ip[0]) {
            if (start_ip[1] > stop_ip[1]) {
                return -1;
            } else if (start_ip[1] == stop_ip[1]) {
                if (start_ip[2] > stop_ip[2]) {
                    return -1;
                } else if (start_ip[2] == stop_ip[2]) {
                    if (start_ip[3] >= stop_ip[3]) {
                        return -1;
                    }
                }
            }
        }
    }

    token = strtok(NULL, "");
    if (!token) {
        return 0;
    }
    *inc = atoi(token);
    if (*inc < 2) {
        return -1;
    }
    return 0;
}


// Parse a comma-separated list of integers
//   Returns the number of list elements found
int cli_parse_list_int (char* list_string, uint16* list, int max_list_size)
{
    int n = 0;
    char *token;

    token = strtok(list_string, ",");
    if (!token) {
        return 0;
    }
    list[n] = atoi(token);

    for (n++; n<max_list_size; n++) {
        token = strtok(NULL, ",");
        if (!token) {
            break;
        }
        list[n] = atoi(token);
    }
    return n;
}

