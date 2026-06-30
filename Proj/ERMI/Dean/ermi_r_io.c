/*
 *------------------------------------------------------------------
 * ermi_r_io.c
 *
 * I/O layer used by ERMI-1
 *
 * Oct 2008, Dean Chen
 *
 * Copyright (c) 2008 by cisco Systems, Inc.
 * All rights reserved.
 *
 * ermi_r_io_connect
 *    - opens socket connect to given peer
 * ermi_r_io_disconnect
 *    - closes socket to given peer
 * ermi_r_io_send()
 *    - sends msg buffer down the socket to peer
 * ermi_r_io_recv()
 *    - receives bytes from the socket, reads in one message worth
 *
 *------------------------------------------------------------------
 */
#include COMP_INC(target-cpu, posix_types.h) 
#include COMP_INC(target-cpu, cpu_types.h) 
#include COMP_INC(kernel, ios_macros.h) 
#include COMP_INC(kernel, queue.h) 
#include COMP_INC(posix, errno.h)
#include COMP_INC(kernel, mgd_timers.h)
#include COMP_INC(idb, interface.h)
#include IOS_INC(tcp/tcp.h)
#include IOS_INC(socket/socket.h)
#include IOS_INC(h/logger.h)
#include "ermi_r_protocol_def.h"
#include "ermi_r_errp_fsm.h"

void ermi_r_io_connect (ermi_r_neighbor_t *nbr) {
    sockaddr_in sinhim;
    sockaddr_in sinme;
    int fd, res, optval = 1;

    if (nbr->fd > 0) {
        socket_close(nbr->fd);
        nbr->fd = -1;
    }

    fd = socket_open(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1) {
        return;
    }
    nbr->fd = fd;

    if (nbr->timer_start_func) {
        nbr->timer_start_func(nbr, ERRP_CONNECT_TIMER_TYPE,
                              nbr->master->connect_time);
    }
                          
    sinme.sin_family = AF_INET;
    sinme.sin_addr.s_addr = INADDR_ANY;
    sinme.sin_len = sizeof(sockaddr_in);
    sinme.sin_port = 0;

    socket_set_option(fd, SOL_SOCKET, SO_NBIO, (void *)&optval, 
                      sizeof(optval)) ;
    socket_set_option(fd, SOL_TCP, TCP_NO_DELAY,
                      (char*)&optval, (size_t)sizeof(optval));
    optval = ERRP_SOC_WINDOW;
    /* Set the socket send and receive buffer sizes */
    socket_set_option(fd, SOL_SOCKET, SO_SNDBUF,
                      &optval, sizeof(optval));
    socket_set_option(fd, SOL_SOCKET, SO_RCVBUF,
                      &optval, sizeof(optval));

    sinhim.sin_family = AF_INET;
    sinhim.sin_addr.s_addr =  htonl(nbr->address);
    sinhim.sin_port = htons(ERRP_PORT);
    sinhim.sin_len = sizeof(sockaddr_in);
    ERMI_R_DEBUG("ERMI-1 I/O connect to %x port %d\n", 
                nbr->address, ERRP_PORT);

    res = socket_connect(fd, (sockaddr *) &sinhim, 
                         sizeof(sockaddr_in));

    if (res == -1) {
        /*
         * Add the check for writability for this socket.
         * When the connect completes we will get a wakeup call.
         */
        if (errno == EWOULDBLOCK) {
            nbr->fd = fd;
            nbr->connected = FALSE;
            optval = 1;
            socket_set_option(fd, SOL_SOCKET, SO_NBIO, (void *)&optval, 
                              sizeof(optval)) ;
            (void)process_set_socket_interest(fd, SOCKET_READ_EVENT,
                                              ENABLE);
            return;
        } else {
            nbr->connected = FALSE;
            /* connection failed */
            ermi_r_event(nbr, E_CONN_OPEN_FAILED, NULL);
            return;
        }
    }

    nbr->connected = TRUE;
    optval = 1;
    socket_set_option(fd, SOL_SOCKET, SO_NBIO, (void *)&optval, 
                      sizeof(optval)) ;
    (void)process_set_socket_interest(fd, SOCKET_READ_EVENT, 
                                      ENABLE);
    /* connection succeeded */
    ermi_r_event(nbr, E_CONN_OPEN, NULL);
    return;    
}

void ermi_r_io_disconnect (ermi_r_neighbor_t *nbr) {
    socket_close(nbr->fd);
    nbr->connected = FALSE;
}

/*
 * returns number of bytes sent down to socket,
 */
int ermi_r_io_send (ermi_r_neighbor_t *nbr, uchar *buf, int len)
{
    int count = 0;

    if (len <= 0) {
        count = 0;
    } else {
        count = socket_send(nbr->fd, buf, len, 0);
        
        if (count <= 0) {
            if (errno != EAGAIN) {
                ERMI_R_DEBUG("Error in ERMI-1 socket send\n");
            } else {
                ERMI_R_DEBUG("ERMI-1 socket is full\n");
            }
        } else if (count < len) { /* wrote partially */
            ERMI_R_DEBUG("ERMI-1 socket send completed partially\n");
        }
    }
    
    ERMI_R_DEBUG("ERMI-1 I/O sent %d bytes\n", count);  
    return(count);
}

/*
 * returns number of bytes received over socket,
 * the received bytes are placed into the buffer pointed to
 * by buf.
 */
int ermi_r_io_recv (ermi_r_neighbor_t *nbr, uchar *buf)
{
    int count = 0;
    short len = 0;

    count = socket_recv(nbr->fd, buf, ERRP_HEADERBYTES, 
                        MSG_PEEK);
    ERMI_R_DEBUG("ERMI-1 peeked %d bytes\n", count);
    if (count >= ERRP_HEADERBYTES) {
        len = ERRP_GETSHORT(buf);
        count = socket_recv(nbr->fd, buf, len,  0);
        ERMI_R_DEBUG("ERMI-1 read %d of %d bytes\n", count, len);
    } else {
        ERMI_R_DEBUG("ERMI-1 received less than header length bytes\n");
        count = 0;
    }
    return(count);
} 
