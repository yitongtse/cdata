/* 
 *------------------------------------------------------------------
 * socket.h -- data structures and definitions for SOCKET 
 *
 * July 1996, Jenny Yuan
 *
 * Copyright (c) 1996-2008 by cisco Systems, Inc.
 *
 * All rights reserved.
 *------------------------------------------------------------------
 */

#ifndef __SOCKET_H__
#define __SOCKET_H__

#if 0
#include "master.h"
#include COMP_INC(kernel/sched, sched.h)
#include "../socket/socket.h"
#include COMP_INC(arch-ios, sys/socketvar.h)
#include COMP_INC(posix, sys/uio.h)
#include COMP_INC(posix, netinet/in.h)
#include COMP_INC(posix, netinet/udp.h)
#include COMP_INC(posix, netdb.h)
#include COMP_INC(posix, arpa/inet.h)
#include COMP_INC(posix, net/if.h)
#include "../socket/select.h"
#include "../socket/sock_in.h"

#else
#include "sys/socket.h"
#endif


typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;

typedef struct sockaddr_in6 sockaddr_in6;

#if defined(__cplusplus)
extern "C" {
#endif


/* EVENT CHANGES */
/* socket_event_mask */
#define SOCKET_READ_EVENT       0x001    /* Socket has READ events */
#define SOCKET_WRITE_EVENT      0x002    /* Socket has WRITE events */
#define SOCKET_EXCEPTION_EVENT  0x004    /* Socket has EXCEPTION pending */
#define SOCKET_ALL_EVENTS       (~0)    /* Socket has all events */

/* Selective READ/WRITE changes */
#define SOCKET_SELECT_READ_EVENT	0x008
#define SOCKET_SELECT_WRITE_EVENT	0x010
#define SOCKET_SELECT_EXCEPTION_EVENT	0x020

/* various masks */
#define SOCKET_EVENT_MASK		0x007
#define SOCKET_SELECTIVE_EVENT_MASK	0x038
#define SOCKET_EVENT_BITS               3

/*
 * Maximum number of sockets per process supported when
 * using socket EVENT api. Maximum number of sockets with
 * select() is limited by MAX_SELECT_LIMIT.
 */
#define MAX_SOCKETS_PER_PROCESS 2048 


/*
 * Additional socket options, not kept in so_options. These are
 * protocol specific and should not be placed in <sys/socket.h>
 */
#define SO_TRANSDATA	        0x1010	
#define SO_STRICT_ADDR_BIND     0x1011	  /* Accept only those packets that
                                           * have been sent to the address
					   * that this socket is bound to */
#define SO_UDPCHECKSUM          0x1012	  /* zero udp checksum */
#define SO_SRC_DYN              0x1013    /* Dynamically maintain source
                                           * with each write */
#define SO_UDP_BCASTCHECKSUM    UDP_BCASTCHECKSUM
#define SO_SRC_SPECIFIED        0x1015    /* Specified Source addr to be used*/

#define READ_CLOSE      1   /* for shuting down read half */
#define WRITE_CLOSE     2   /* for shuting down write half */

/*
 * socket.c
 */
extern int socket_listen(int, int);
extern int  socket_bind(int, const struct sockaddr *, socklen_t);

extern int socket_open(int, int, int);
extern int socket_accept(int, sockaddr *, int *);
extern ssize_t socket_recv(int, void *, size_t, int);
extern ssize_t socket_send(int, const void *, size_t, int);
extern int     socket_connect(int, const struct sockaddr *, socklen_t);

extern int socket_get_localname(int, sockaddr *, int *);
extern int socket_get_peername(int, sockaddr *, int *);
extern int socket_shutdown(int, int);
extern int socket_set_option(int, int, int, const void *, socklen_t);
extern int socket_get_option (int, int, int, void *, int *);
extern int socket_close(int);

extern void socket_strerror(char *);
extern ssize_t socket_recvfrom(int, void *, size_t, int, struct sockaddr *, socklen_t *);
extern int socket_allocbuf_recvfrom(int, char **, sockaddr *, int *); 
extern ssize_t socket_sendto(int, const void *, size_t, int, const struct sockaddr *, 
               socklen_t);
extern int socket_recvmsg (int, struct msghdr *, int );
extern int socket_recvpacket (int, void **pak, struct msghdr *);
extern ssize_t socket_sendmsg(int, const struct msghdr *, int);
extern int socket_ioctl( int, int, char *);

extern hostent *socket_gethostbyname(const char *);
extern char *socket_inet_ntoa(struct in_addr);
extern const char *inet_ntop(int af, const void *src, char *dst, size_t size);
extern ulong socket_inet_addr(const char *);
extern ulong socket_inet_network(const char *);

extern char *if_indextoname(unsigned int, unsigned int, char *);
// extern int if_outputindex(sockaddr *, unsigned int *, unsigned int *, void **, tableid_t);
// extern void if_getwaninfo (struct in_waninfo_ *,  paktype *);


/*
 * sock_util.c
 */
extern int socket_inherit_fds(pid_t, pid_t);
extern int socket_inherit_fd(int, int*, pid_t, pid_t);
extern int socket_share_fds(pid_t, pid_t);
extern int socket_watch_other_events(boolean);
extern boolean process_get_socket_event(int *fd, ulong *event_mask);
// extern void process_watch_socket_event(ulong, ENABLESTATE, ONESHOTTYPE);
// extern boolean process_set_socket_interest(int fd, ulong event_mask, ENABLESTATE enable);
// extern boolean process_set_socket_interest_all (ulong event_mask, ENABLESTATE enable);


#if defined(__cplusplus)
}
#endif

#endif /* __SOCKET_H__ */
