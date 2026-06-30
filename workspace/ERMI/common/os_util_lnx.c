///  Copyright (c) 2011 by Cisco Systems, Inc.
///  All rights reserved.
///
///  @file      os_util_lnx.c
///  @brief     Linux OS Utilities
///  @author    Yi Tong Tse


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <semaphore.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "common.h"
#include "os_util_lnx.h"


#define DEBUG(fmt, str...)        \
    printf(fmt"\n", ##str);
//    NULL


int io_connect (const struct sockaddr *addr)
{
    int fd;             // socket descriptor

    DEBUG("Connecting...");

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return -1;
    }

    if (connect(fd, addr, sizeof(*addr)) < 0) {
        perror("connect");
        return -1;
    }

    return fd;
}


void io_disconnect (int fd)
{
    DEBUG("Disconnecting...");
    close(fd);
}


int io_send (int fd, uint8 *buf, int msg_size)
{
    int cnt;

#if 0
    printf("Msg to send: %d bytes\n", msg_size);
    print_hex(buf, msg_size);
#endif

    cnt = write(fd, buf, msg_size);
    if (cnt == 0) {
        DEBUG("io_send error: EOF");
        return 0;
    }
    if (cnt < 0) {
        perror("io_send");
        return 0;
    }
    DEBUG("I/O sent %d out of %d bytes", cnt, msg_size);  
    return(cnt);
}


int io_recv (int fd, uint8 *buf, int buf_size)
{
    int len = read(fd, buf, buf_size);
    if (len == -1) {
        return -errno;
    }

#if 0
    printf("Msg received: %d bytes\n", len);
    print_hex(buf, len);
#endif

    return len;
}


int timer_start (TIMER_T timer, uint32 timeout)
{
    struct itimerspec its;
    its.it_value.tv_sec = timeout;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    return timer_settime(timer, 0, &its, NULL);
}


int timer_stop (TIMER_T timer)
{
    struct itimerspec its;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    return timer_settime(timer, 0, &its, NULL);
}


void sema_wait (SEMA_T *sema)
{
    sem_wait(sema);
}


void sema_post (SEMA_T *sema)
{
    sem_post(sema);
}

