/* $Id: socket.c,v 1.1 2010/10/01 17:41:32 ytse Exp ytse $
 * $Source: /Users/ytse/work/ermi/ios/RCS/socket.c,v $
 *------------------------------------------------------------------
 * Generic socket functions
 *
 * July 1996, Jenny Yuan
 *
 * Copyright (c) 1996-2009 by cisco Systems, Inc.
 *
 * All rights reserved.
 *------------------------------------------------------------------
 * $Log: socket.c,v $
 * Revision 1.1  2010/10/01 17:41:32  ytse
 * Initial revision
 *
 * Revision 1.1  1996/08/27  18:46:26  jenny
 * Placeholder for Colorado.
 *
 *------------------------------------------------------------------
 * $Endlog$
 */


#include "master.h"
#include <string.h>
#include <ctype.h>
#include COMP_INC(cisco, ciscolib.h)
#include <errno.h>
#include <limits.h>
#include COMP_INC(kernel/sched, sched.h)
#include "config.h"
#include COMP_INC(kernel, subsys.h)
#include "parser.h"
#include "interface_private.h"
#include "select.h"
#include "socket.h"
#include "sockio.h"
#include <posix/include/assert.h>
#include COMP_INC(ip, ip.h)
#include COMP_INC(ip, ip_registry.h)
#include "../ip/ipaddr_iter.h"
#include COMP_INC(ipv6, ipv6.h)
#include <ipv6_addressing/include/ipv6_address.h>
#include <ipv6_forwarding/include/ipv6_forwarding.h>
#include "name.h"
#include "../sys_reg/sys_name_registry.h"
#include <network/include/sys_proto_registry.h>

#include "sock_internal.h"
#include "sock_debug.h"


/*
 * socket_create
 *    pr_type - SOCK_STREAM or SOCK_DGRAM
 *    protocol - transport layer protocol, ie, TCP.
 *
 * Create a new socket and give it an fd,  returns a pointer to a socket 
 * or NULL if a new socket can not be created.
 */
socktype * 
socket_create (int family, int pr_type, int protocol)
{
    socktype *soc = NULL;
    int status;

    soc = malloc(sizeof(socktype));
    if (soc == NULL) {
	SOCK_DEBUG((" SOCKET: Out of memory"));	
	return(NULL);
    }
    soc->watched_bits = create_watched_bitfield("TCP Socket events", 
						(ulong ) soc);
    if (soc->watched_bits == NULL) {
	free(soc);
	return(NULL);
    }
    process_clear_bitfield(soc->watched_bits, 0xFFFF, FALSE);
    soc->so_family = family;
    soc->so_type = pr_type;
    soc->so_proto = protocol;
    soc->direct_wakeup = FALSE;
    soc->tos = ipdefaultparams.tos;
    soc->so_memberships = NULL;
    process_get_pid(&soc->so_pid);
    /*
     * Default behavior is Posix (SO_IOS_TASK_ID flag not set).  However, since
     * legacy IOS apps want legacy behavior, we set this flag here, and have
     * "Posix" apps clear the flag again.
     */
    soc->so_options |= SO_IOS_TASK_ID;

    /* add soc to proc_soc, and give it an fd */
    status = add_sock_for_proc(soc, soc->so_pid); 
    if (status == SO_FAIL) {
	SOCK_DEBUG(("\nSOCKET: Can't add new socket 0x%x to process %d",
		   soc, soc->so_pid));
	delete_watched_bitfield(&soc->watched_bits);
	free(soc);
	return(NULL);
    }
    soc->refcnt = 1;
    SOCK_DEBUG(("\nSOCKET: Added new socket 0x%x to process %d",
		soc, soc->so_pid));
    return(soc);
}


/*
 * socket_open
 *     family - AF_INET | AF_INET6
 *     pr_type - SOCK_STREAM or SOCK_DGRAM
 *     protocol - TCP or UDP
 *
 * This is equivalent to socket open function. It returns a interger
 * as a simulated file descriptor for the socket upon success of the
 * openning of a new socket.
 */
int
socket_open (int family, int pr_type, int protocol)
{
    socktype *soc = NULL; 
    int status;

    if (family != AF_INET && family != AF_INET6) {
	errno = EAFNOSUPPORT;
	return(INVALID_FD);
    }

    if ((pr_type != SOCK_STREAM) && (pr_type != SOCK_DGRAM)) {
	errno = ESOCKTNOSUPPORT;
	return(INVALID_FD) ;
    }

    if (protocol == 0L) {
	/* Select the apropriate default protocol based on socket type */
	protocol = (pr_type == SOCK_STREAM) ? IPPROTO_TCP : IPPROTO_UDP;
    }

    soc = socket_create(family, pr_type, protocol);

    if (soc == NULL) {
	errno = EIO;
	return(INVALID_FD);
    }

    status = SO_FAIL; 
    if ((protocol == IPPROTO_TCP) && (pr_type == SOCK_STREAM)) 
	status = sock_create_tcp(soc, family);
    else if ((protocol == IPPROTO_UDP || 
	     protocol == 0L) && (pr_type == SOCK_DGRAM))
	status = sock_create_udp(soc, family);
    else if (protocol == IPPROTO_PGM && pr_type == SOCK_STREAM)
	status = sock_create_pgm(soc);
    else 
	errno = EPROTONOSUPPORT;

    if (status == SO_FAIL) {
	/*
	 * The socket needs to be freed now since the application 
	 * isn't expected to do a close on the socket if open fails.
	 */ 
	sock_conn_dead(soc);
	return(INVALID_FD);
    } else {
	return(soc->so_fd);
    }
}


/*
 * socket_listen
 *    so_fd - socket file descriptor
 *    backlog - number of connections that can be queued on the socket
 *
 */
int
socket_listen (int fd, int backlog)
{
    int status;
    socktype *soc=NULL;

    soc = getsock_by_fd(fd);
    status= sanity_check_sock(soc);
    if (status == SO_FAIL) {
	SOCK_DEBUG(("\nSOCKET: Listen failed: socket 0x%x is not sane", soc));
	errno = EBADF;
        return(SO_FAIL);
    }

    if (!(soc->so_stateflags & SS_ISBOUND)) { 
	SOCK_DEBUG(("\nSOCKET: Listen failed: socket 0x%x is not bound", soc));
	errno = EACCES;
        return(SO_FAIL); 
    }

    if (soc->so_proto == IPPROTO_TCP) {
	status = sock_listen_tcp(soc, backlog);
	return(status);
    } else if (soc->so_proto == IPPROTO_PGM) {
	status = sock_listen_pgm (soc, backlog);
	return(status);
    } else {
	SOCK_DEBUG(("\nSOCKET: Listen failed: "));
	SOCK_DEBUG(("socket 0x%x doesn't support protocol %d", soc, 
		    soc->so_proto));
	errno = EPROTONOSUPPORT;
	return(SO_FAIL);
    }
}


/*
 * socket_bind
 *    fd - file descriptor for the binding socket
 *    name - point to binding socket address
 *    namelen - length of binding address
 *
 * returns SO_FAIL if bind fails, otherwise returns 1.
 */
int
socket_bind (int fd, const struct sockaddr *name, socklen_t namelen)
{
    socktype *soc;
    int status;

    soc = getsock_by_fd(fd);
    status= sanity_check_sock(soc);
    if (status == SO_FAIL) {
	SOCK_DEBUG(("\nSOCKET: Bind failed: socket  0x%x is not sane", soc));
	errno = EBADF;
        return(SO_FAIL);
    }
    if (name == NULL || namelen == 0) {
	SOCK_DEBUG(("\nSOCKET: Bind failed: name 0x%x or namelen %d zero", 
		    name, namelen));
        errno = EINVAL; 
	return(SO_FAIL);
    }

    if (name->sa_family != AF_INET && name->sa_family != AF_INET6) {
	SOCK_DEBUG(("\nSOCKET: Bind failed: family %d not support", 
		    name->sa_family));
	errno = EAFNOSUPPORT;
	return(SO_FAIL);
    }

    /* The family of the socket and the passed address should match */
    if (soc->so_family != name->sa_family) {
	SOCK_DEBUG((
        "\nSOCKET: Bind failed: socket family %d and addr family %d not same",
		    soc->so_family, name->sa_family));
	errno = EAFNOSUPPORT;
	return(SO_FAIL);
    }

    /* Validate namelen */
    if ((name->sa_family == AF_INET) && (namelen != sizeof(sockaddr_in)) || 
        (name->sa_family == AF_INET6) && (namelen != sizeof(sockaddr_in6)) ) {
	SOCK_DEBUG(("\nSOCKET: Bind failed: Incorrect address length %d ", 
		    namelen));
	errno = EINVAL;
	return(SO_FAIL);
    }
    
    if (soc->so_stateflags & SS_ISBOUND) {
	SOCK_DEBUG(("\nSOCKET: Bind failed: socket 0x%x is already bound", 
		    soc));
	sock_state2string(soc->so_stateflags);
	errno = EISCONN;
	return(SO_FAIL);
    }

    status = SO_FAIL;
    if (soc->so_proto == IPPROTO_TCP) {
	status = sock_bind_tcp(soc, (struct sockaddr *)name);
    } else if  (soc->so_proto == IPPROTO_UDP) {
	status = sock_bind_udp(soc, (struct sockaddr *)name);
    } else if (soc->so_proto == IPPROTO_PGM) {
	status = sock_bind_pgm(soc, (struct sockaddr *)name);
    } else {
	SOCK_DEBUG(("\nSOCKET: Bind failed: protocol %d not supported", 
		    soc->so_proto));
	errno = EPROTONOSUPPORT;
    }
    return(status);
}


/*
 * socket_accept
 *    listen_fd - file descriptor for the listening socket 
 *    addr - not used. 
 *    addrlen - not used. 
 *
 * Returns a file descriptor to a new connection.
 */  
int 
socket_accept (int listen_fd, sockaddr *addr, int *addrlen)
{
    socktype *listen_soc;
    int new_fd;
    int status;

    listen_soc = getsock_by_fd(listen_fd);
    status= sanity_check_sock(listen_soc);
    if (status == SO_FAIL) {
	SOCK_DEBUG(("\nSOCKET: Accept failed: socket 0x%x is not sane", 
		    listen_soc));
	errno = EBADF;
        return(SO_FAIL);
    }
    /* 
     * Make sure a listen() has been done.
     */
    if ((listen_soc->so_options & SO_ACCEPTCONN) == 0) {
	SOCK_DEBUG(("\nSOCKET: Accept failed: "));
	SOCK_DEBUG(("accepting socket 0x%x is not a listener", listen_soc));
	errno = EOPNOTSUPP;
	return(INVALID_FD);
    }
	
    if (listen_soc->so_proto == IPPROTO_TCP) {
	new_fd = sock_accept_tcp(listen_soc, addr, addrlen);
        if (!sock_readable(listen_soc)) {
           socket_clear_event(listen_soc, SOCKET_READ_EVENT);
	} 
	return(new_fd);
    } else if (listen_soc->so_proto == IPPROTO_PGM) {
	new_fd = sock_accept_pgm (listen_soc, addr, addrlen);
        if (!sock_readable(listen_soc)) {
           socket_clear_event(listen_soc, SOCKET_READ_EVENT);
	}
	return(new_fd);
    } else {
	SOCK_DEBUG(("\nSOCKET: Accept failed: protocol %d not supported",
		   listen_soc->so_proto));
	errno = EPROTONOSUPPORT;
	return(INVALID_FD);
    } 
}


/*
 * socket_recvfrom

 * This library function is for UDP connections only.
 * Reads data from a socket referenced by the file descriptor.
 *
 * If from is not a NULL pointer, the  source  address  of  the
 * message  is filled in.  fromlen is a value-result parameter,
 * initialized to the size of the buffer associated with  from,
 * and  modified  on  return to indicate the actual size of the
 * address  stored  there.
 *
 * Returns the bytes read or -1 if an error had occured during
 * reading.
 */
ssize_t 
socket_recvfrom (int fd, void *buf, size_t len, int flags, struct sockaddr *from, 
		 socklen_t *fromlen)
{
    int cnt;
    socktype *soc;
    int status;

    soc = getsock_by_fd(fd);
    status= sanity_check_sock(soc);
    if (status == SO_FAIL) {
	SOCK_DEBUG(("\nSOCKET: Recvfrom failed: socket 0x%x is not sane", soc));
	errno = EBADF;
        return(SO_FAIL);
    }
    if ((soc->so_proto != IPPROTO_UDP) && (soc->so_proto != IPPROTO_TCP)) {
	SOCK_DEBUG(("\nSOCKET: Recvfrom failed: protocol %d not supported", 
		    soc->so_proto));
	errno = EPROTONOSUPPORT;
	return(SO_FAIL);
    }

    if ((soc->so_proto == IPPROTO_TCP) && !(soc->so_stateflags & SS_ISCONNECTED)) {
        SOCK_DEBUG(("\nSOCKET: Recvfrom failed: socket 0x%x is not connected",
                     soc));
        errno = ENOTCONN;
        return (SO_FAIL);
    }

    if (soc->so_stateflags & SS_CANTRCVMORE) {
	SOCK_DEBUG(("\nSOCKET: Recvfrom failed: socket 0x%x can't read "
		    "anymore", soc));
	if (soc->so_error) {
	    errno = soc->so_error;
	} else {
	    errno = EPIPE;
	}
	return (SO_FAIL);
    }

    if (from) {
      if (fromlen) {
          switch (soc->so_family) {
          case AF_INET:
              if (*fromlen < sizeof( sockaddr_in )) {
                  SOCK_DEBUG(("\nSOCKET: socket 0x%x recvfrom failed: invalid fromlen %d",
                              soc, *fromlen));
                  errno = EINVAL;
                  return(SO_FAIL);
              }
              break;
          case AF_INET6:
              if (*fromlen < sizeof( sockaddr_in6 )) {
                  SOCK_DEBUG(("\nSOCKET: socket 0x%x recvfrom failed: invalid fromlen %d",
                              soc, *fromlen));
                  errno = EINVAL;
                  return(SO_FAIL);
              }
              break;
          default:
              SOCK_DEBUG(("\nSOCKET: recvfrom failed: invalid family %d", soc->so_family));
              errno = EINVAL;
              return(SO_FAIL);
              break;
          }
      } else {
          SOCK_DEBUG(("\nSOCKET: socket 0x%x recvfrom failed: invalid fromlen NULL", soc));
          errno = EINVAL;
          return(SO_FAIL);
      }
    }

    if (!sock_readable(soc)) {
            socket_clear_event(soc, SOCKET_READ_EVENT);
	if (flags & MSG_PEEK) {
	    return(0);
	} else if (soc->so_options & SO_NBIO) {
	    /*
	     * Non-blocking socket. Return EWOULDBLOCK if reading
	     * will be blocked.
	     */ 
	    errno = EWOULDBLOCK;
            SOCK_DEBUG(("\nSOCKET: Recvfrom failed: soc %x blocking", soc));
	    return(SO_FAIL);
	}
    }	
    
    if (soc->so_proto == IPPROTO_TCP) {
        cnt = sock_recv_tcp(soc, buf, len, flags);
        /*
         * Update the from address if valid data has been received and a valid buffer has been
         * passed i.e., when either cnt is greater than 0 or cnt is 0  and we reached end of file
         */
        if (((cnt == 0) && (soc->so_stateflags |= SS_CANTRCVMORE)) || (cnt > 0)) {
            if (from) {
                status = socket_get_peername(fd, from, fromlen);
            }
        }
    } else {
        cnt = sock_recvfrom_udp(soc, buf, len, from, fromlen, flags);
    }
    if (!sock_readable(soc) || (cnt <= 0))
        socket_clear_event(soc, SOCKET_READ_EVENT);
    return(cnt);
}


/*
 * socket_recvmsg
 *
 * This library function is for UDP connections only. Reads data from a 
 * socket referenced by the file descriptor. Returns the bytes read or -1 
 * if an error occurs during reading.
 */

int 
socket_recvmsg (int fd, struct msghdr *msg, int flags )
{
    int cnt;
    socktype *soc;
    int status;

    /* Map the file descriptor to a socket structure */
    soc = getsock_by_fd(fd);
    status= sanity_check_sock(soc);
    if (status == SO_FAIL) {
	SOCK_DEBUG(("\nSOCKET: Recvmsg failed: socket 0x%x is not sane", soc));
	errno = EBADF;
        return(SO_FAIL);
    }

    /* Recvmsg() only works with UDP sockets */
    if (soc->so_proto != IPPROTO_UDP) {
	SOCK_DEBUG(("\nSOCKET: Recvmsg failed: protocol %d not supported", 
		    soc->so_proto));
	errno = EPROTONOSUPPORT;
	return(SO_FAIL);
    }

    if (soc->so_stateflags & SS_CANTRCVMORE) {
	SOCK_DEBUG(("\nSOCKET: Recvmsg failed: socket 0x%x can't read anymore",
		    soc));
	if (soc->so_error) {
	    errno = soc->so_error;
	} else {
	    errno = EPIPE;
	}
	return (SO_FAIL);
    }

    /* Validate the message header */
    if (msg == NULL || msg->msg_iov == NULL || msg->msg_iovlen == 0 ) {
	SOCK_DEBUG(("\nSOCKET: Recvmsg failed: invalid msghdr structure" ));
	errno = EFAULT;
	return(SO_FAIL);
    }

    switch (soc->so_family) {
    case AF_INET:
      /* IPv4, same size as sockaddr_in */
      if (msg->msg_name && (msg->msg_namelen < (int)sizeof( sockaddr_in ))) {
          SOCK_DEBUG(("\nSOCKET: Recvmsg failed: invalid IPv4 sockaddr len %d",
                msg->msg_namelen));
          errno = EINVAL;
          return(SO_FAIL);
      }
      break;
    case AF_INET6:
      /* For IPv6, check msg_namelen size */
      if (msg->msg_name && (msg->msg_namelen < (int)sizeof( sockaddr_in6 ))) {
          SOCK_DEBUG(("\nSOCKET: Recvmsg failed: invalid IPv6 sockaddr len %d",
                msg->msg_namelen));
          errno = EINVAL;
          return(SO_FAIL);
      }
      break;
    default:
      SOCK_DEBUG(("\nSOCKET: Recvmsg failed: invalid family %d", soc->so_family));
      errno = EINVAL;
      return(SO_FAIL);
      break;
    }

    /* Is there any data in the receive buffer ? */
    if (!sock_readable(soc)) {
        socket_clear_event(soc, SOCKET_READ_EVENT);
	if (flags & MSG_PEEK) {
	    return(0);
	} else if (soc->so_options & SO_NBIO) {
	    /*
	     * Non-blocking socket. Return EWOULDBLOCK if reading
	     * will be blocked.
	     */ 
	    errno = EWOULDBLOCK;
	    return(SO_FAIL);
	}
    }	

    /* Read data from the socket */
    cnt = sock_recvmsg_udp(soc, msg, flags);
    if (!sock_readable(soc) || (cnt <= 0))
        socket_clear_event(soc, SOCKET_READ_EVENT);
    return(cnt);
}



/*
 * socket_recvpacket
 *
 * This library function is for UDP connections only. Punts  packet(s)
 * referenced by the file descriptor to the application. Returns the bytes 
 * read or -1 if an error occurs.
 */

int 
socket_recvpacket (int fd, void **pak, struct msghdr *msg)
{
    int cnt;
    socktype *soc;
    int status;
    
    /* The caller must provide a packet queue */
    if (pak == NULL)
        goto invalid_arg;
        
    /* Verify message flags */
    if (msg->msg_flags & ~(MSG_PUNTALL | MSG_PEEK))
        goto invalid_arg;

    /* Map the file descriptor to a socket structure */
    soc = getsock_by_fd(fd);
    status= sanity_check_sock(soc);
    if (status == SO_FAIL) {
        SOCK_DEBUG(("\nSOCKET: Recvpacket failed: socket 0x%x is not sane", soc));
        errno = EBADF;
        return(SO_FAIL);
    }

    /* recvpacket() only works with UDP sockets */
    if (soc->so_proto != IPPROTO_UDP) {
        SOCK_DEBUG(("\nSOCKET: Recvpacket failed: protocol %d not supported",
                    soc->so_proto));
        errno = EPROTONOSUPPORT;
        return(SO_FAIL);
    }

    if (soc->so_stateflags & SS_CANTRCVMORE) {
	SOCK_DEBUG(("\nSOCKET: Recvpacket failed: socket 0x%x can't read "
		    "anymore", soc));
	if (soc->so_error) {
	    errno = soc->so_error;
	} else {
	    errno = EPIPE;
	}
	return (SO_FAIL);
    }

    /* Is there any data in the receive buffer ? */
    if (!sock_readable(soc)) {
        socket_clear_event(soc, SOCKET_READ_EVENT);
        if (soc->so_options & SO_NBIO) {
            /*
             * Non-blocking socket. Return EWOULDBLOCK if reading
             * will be blocked.
             */
            errno = EWOULDBLOCK;
            return(SO_FAIL);
        }
    }
    /* Read data from the socket */
    cnt = sock_recvpacket_udp(soc, msg, pak);
    return(cnt);

invalid_arg:
    /* Invalid argument */
    errno = EINVAL;
    return(-1);

}


/*
 * socket_sendto
 * 
 * This library function is for UDP connections only.
 * Writes data to a socket referenced by the file descriptor.
 * The address of the target is given by to with tolen specifying
 * its size. The length of the message is given by len. 
 *
 * Returns the bytes written or -1 if an error had occured during
 * writing.
 */
ssize_t 
socket_sendto (int fd, const void *msg, size_t len, int flags, const struct sockaddr *to, 
	       socklen_t tolen)
{
    int cnt;
    socktype *soc;
    int status;

    soc = getsock_by_fd(fd);
    status= sanity_check_sock(soc);
    if (status == SO_FAIL) {
	SOCK_DEBUG(("\nSOCKET: Sendto failed: socket 0x%x is not sane", soc));
	errno = EBADF; 
        return(SO_FAIL);
    }
    if ((soc->so_proto != IPPROTO_UDP) && (soc->so_proto != IPPROTO_TCP)) {
	SOCK_DEBUG(("\nSOCKET: Sendto failed: protocol %d not supported", 
		    soc->so_proto));
	errno = EPROTONOSUPPORT;
	return(SO_FAIL);
    }
    
    if ((soc->so_proto == IPPROTO_TCP) && 
        !(soc->so_stateflags & SS_ISCONNECTED)) {
        SOCK_DEBUG(("\nSOCKET: Sendto failed: socket 0x%x is not connected",soc));
        errno = ENOTCONN;
        return(SO_FAIL);
    }

    if (soc->so_stateflags & SS_CANTSENDMORE) {
      SOCK_DEBUG(("\nSOCKET: Sendto failed: socket 0x%x can't write anymore",
                  soc));
      if (soc->so_error) {
          errno = soc->so_error;
      } else {
          errno = EPIPE;
      }
      return (SO_FAIL);
    }

    if (to) {
      switch (soc->so_family) {
      case AF_INET:
          if (tolen != (int)sizeof( sockaddr_in )) {
              SOCK_DEBUG(("\nSOCKET: Sendto failed: v4 invalid tolen %d", tolen ));
              errno = EINVAL;
              return(SO_FAIL);
          }
          break;
      case AF_INET6:
          if (tolen != (int)sizeof( sockaddr_in6 )) {
              SOCK_DEBUG(("\nSOCKET: Sendto failed: v6 invalid tolen %d", tolen));
              errno = EINVAL;
              return(SO_FAIL);
          }
          break;
      default:
          SOCK_DEBUG(("\nSOCKET: Sendto failed: invalid family %d", soc->so_family));
          errno = EINVAL;
          return(SO_FAIL);
          break;
      }
    }
    
    /* 
     * we have either a TCP or UDP socket
     */
    if (soc->so_proto == IPPROTO_TCP) {
        if (soc->so_options & SO_NBIO) {
            /*
             * Make sure that the TCP didn't close underneath us.
             */
            if (sock_tcp_closed(soc)) {
                errno = EPIPE;
                SOCK_DEBUG(("\nSOCKET: Send failed: soc %x tcp connection
                           closed", soc));
                return(SO_FAIL);
            }
            /*
             * Non-blocking socket. Return EWOULDBLOCK if writing
             * will be blocked.
             */
            if (!sock_writeable(soc)) {
                errno = EWOULDBLOCK;
                SOCK_DEBUG(("\nSOCKET: Send failed: soc %x blocking", soc));
                return(SO_FAIL);
            }
        }
        /*
         * For TCP sockets ignore the to address provided
         */
        cnt = sock_send_tcp(soc, (void *)msg, len);
    } else {
        cnt = sock_sendto_udp(soc, (void *)msg, len, (struct sockaddr *)to, tolen);
    }
    if (!sock_writeable(soc))
        socket_clear_event(soc, SOCKET_WRITE_EVENT);
    return(cnt);
}


/*
 * socket_sendmsg
 * 
 * This library function is for UDP connections only. Writes data to a socket 
 * referenced by the file descriptor. Returns the bytes written or -1 if an 
 * error occurs during writing.
 */

ssize_t 
socket_sendmsg (int fd, const struct msghdr *msg, int flags)
{
    int cnt;
    socktype *soc;
    int status;

    /* Map the file descriptor to a socket structure */
    soc = getsock_by_fd(fd);
    status= sanity_check_sock(soc);
    if (status == SO_FAIL) {
	SOCK_DEBUG(("\nSOCKET: Sendmsg failed: socket 0x%x is not sane", soc));
	errno = EBADF;
        return(SO_FAIL);
    }

    /* Sendmsg() only works with UDP sockets */
    if (soc->so_proto != IPPROTO_UDP) {
	SOCK_DEBUG(("\nSOCKET: Sendmsg failed: protocol %d not supported", 
		    soc->so_proto));
	errno = EPROTONOSUPPORT;
	return(SO_FAIL);
    }

    if (soc->so_stateflags & SS_CANTSENDMORE) {
	SOCK_DEBUG(("\nSOCKET: Sendmsg failed: socket 0x%x can't "
		    "write anymore", soc));
	if (soc->so_error) {
	    errno = soc->so_error;
	} else {
	    errno = EPIPE;
	}
	return (SO_FAIL);
    }

    /* Validate the message header */
    if (msg == NULL || msg->msg_iov == NULL || msg->msg_iovlen == 0) { 
	SOCK_DEBUG(("\nSOCKET: Sendmsg failed: invalid msghdr structure" ));
	errno = EFAULT;
	return(SO_FAIL);
    }

    /* Validate address length */
    switch (soc->so_family) {
    case AF_INET:
      if (msg->msg_name && (msg->msg_namelen != (int)sizeof( sockaddr_in)) ) {
          SOCK_DEBUG(("\nSOCKET: Sendmsg failed: invalid IPv4 sockaddr len %d",
                msg->msg_namelen));
          errno = EINVAL;
          return(SO_FAIL);
      }
      break;
    case AF_INET6:
      if (msg->msg_name && (msg->msg_namelen != (int)sizeof( sockaddr_in6 )) ) {
          SOCK_DEBUG(("\nSOCKET: Sendmsg failed: invalid IPv6 sockaddr len %d",
                msg->msg_namelen));
          errno = EINVAL;
          return(SO_FAIL);
      }
      break;
    default:
      SOCK_DEBUG(("\nSOCKET: Sendmsg failed: invalid family %d", soc->so_family));
      errno = EINVAL;
      return(SO_FAIL);
      break;
    }

    /* Write data to the socket */
    cnt = sock_sendmsg_udp(soc, (struct msghdr *)msg, flags);
    if (!sock_writeable(soc))
        socket_clear_event(soc, SOCKET_WRITE_EVENT);
    return(cnt);
}

/*
 * socket_read
 * Reads data from a socket (referenced by the fd) into a buffer.
 * Returns the amount of data read or -1 in case of an error.
 */
ssize_t
socket_read (int fd, void *buf, size_t count)
{
  return(socket_recv(fd, buf, count, 0));
}

/*
 * socket_readv
 */
ssize_t
socket_readv (int fd, const struct iovec *vector, int count)
{
    struct msghdr msg;

    memset(&msg, 0, sizeof(struct msghdr));
    msg.msg_iov = ( struct iovec *)vector;
    msg.msg_iovlen = count;

    return (socket_recvmsg(fd, &msg, 0));
}

/*
 * socket_write
 */
ssize_t
socket_write (int fd, const void *buf, size_t count)
{
    return (socket_send(fd, buf, count, 0));
}

/*
 * socket_writev
 */
ssize_t
socket_writev (int fd, const struct iovec *vector, int count)
{
    struct msghdr msg;

    memset(&msg, 0, sizeof(struct msghdr));
    msg.msg_iov = (struct iovec *) vector;
    msg.msg_iovlen = count;

    return (socket_sendmsg(fd, &msg, 0));
}


/*
 * socket_recv
 *
 * This library function is for TCP connections only.
 * Reads data from a socket referenced by the file descriptor.
 * Returns the bytes read or -1 if an error had occured during
 * reading.
 */
ssize_t
socket_recv (int fd, void *buf, size_t len, int flags)
{
    int cnt;
    socktype *soc;
    int status;

    soc = getsock_by_fd(fd);
    status= sanity_check_sock(soc);
    if (status == SO_FAIL) {
	SOCK_DEBUG(("\nSOCKET: Recv failed: socket 0x%x is not sane", soc)); 
	errno = EBADF;
        return(SO_FAIL);
    }

    if (! (soc->so_stateflags & SS_ISCONNECTED)) {
	SOCK_DEBUG(("\nSOCKET: Recv failed: socket 0x%x is not connected", 
		    soc));
	sock_state2string(soc->so_stateflags);
	errno = ENOTCONN;
	goto error;
    }

    if (soc->so_stateflags & SS_CANTRCVMORE) {
	SOCK_DEBUG(("\nSOCKET: Recv failed: socket 0x%x can't read anymore", 
		    soc));
	if (soc->so_error)
	    errno = soc->so_error;
	else 
	    errno = EPIPE;
        goto error;
    }

    if ((soc->so_proto != IPPROTO_TCP) && (soc->so_proto != IPPROTO_UDP) && (soc->so_proto != IPPROTO_PGM))  {
	SOCK_DEBUG(("\nSOCKET: Recv failed: protocol %d not supported", 
		    soc->so_proto));
	errno = EPROTONOSUPPORT;
	return(INVALID_FD);
    } 
    if (!sock_readable(soc)) {
        socket_clear_event(soc, SOCKET_READ_EVENT);
	if ((flags & MSG_PEEK) && (len == 0)) {
	    return(0);
        }
	if (soc->so_options & SO_NBIO) {
	    /*
	     * Non-blocking socket. Return EWOULDBLOCK if reading
	     * will be blocked.
	     */ 
	    errno = EWOULDBLOCK;
            SOCK_DEBUG(("\nSOCKET: Recv failed: soc %x blocking", soc));
	    goto error;
	}
    }	

    /*
     * Validate len = 0 if flag is not MSG_PEEK
     */ 
    if (!(flags & MSG_PEEK) && (len <= 0)) {
        errno = EINVAL;
        return(SO_FAIL);
    }

    /* once we get this far we have a PGM or TCP or UDP socket */
    if (soc->so_proto == IPPROTO_TCP){
	cnt = sock_recv_tcp(soc, buf, len, flags);
    } else
    if  (soc->so_proto == IPPROTO_UDP){
       cnt = sock_recvfrom_udp(soc, buf, len, NULL, 0, flags);
    } else {
	/* must be PGM */
	if (flags && (flags != MSG_PEEK)) {
	    /* The only option supported by PGM is MSG_PEEK */
	    errno = EOPNOTSUPP;
	    goto error;
	}
	cnt = sock_recv_pgm(soc, buf, len, flags);
    }

    if (!sock_readable(soc) || (cnt <= 0))
        socket_clear_event(soc, SOCKET_READ_EVENT);
    return(cnt);

error:
    socket_clear_event(soc, SOCKET_READ_EVENT);
    return(SO_FAIL);
}



/*
 * socket_send
 *
 * This library function is for TCP connections only.
 * socket_send() support for UDP is added.
 * Writes data to a socket referenced by the file descriptor.
 * Returns the bytes written or -1 if an error had occured during
 * writing.
 */
ssize_t 
socket_send (int fd, const void *buf, size_t len, int flags)
{
    int cnt;
    socktype *soc;
    int status;

    soc = getsock_by_fd(fd);
    status= sanity_check_sock(soc);
    if (status == SO_FAIL) {
	SOCK_DEBUG(("\nSOCKET: Send failed: socket 0x%x is not sane", soc));
	errno = EBADF;
        return(SO_FAIL);
    }

    if (!(soc->so_stateflags & SS_ISCONNECTED)) {
	SOCK_DEBUG(("\nSOCKET: Send failed: socket 0x%x is not connected", 
		    soc));
	sock_state2string(soc->so_stateflags);
	errno = ENOTCONN;
	return(SO_FAIL);
    }

    if (soc->so_stateflags & SS_CANTSENDMORE) {
	SOCK_DEBUG(("\nSOCKET: Send failed: socket 0x%x can't send anymore", 
		    soc));
	if (soc->so_error)
	    errno = soc->so_error;
	else 
	    errno = EPIPE;
	return(SO_FAIL);
    }
    
    if ((soc->so_proto == IPPROTO_TCP) || (soc->so_proto == IPPROTO_PGM)) {

	if (soc->so_options & SO_NBIO) {
            /*
             * Make sure that the TCP didn't close underneath us.
             */
            if (sock_tcp_closed(soc)) {
                errno = EPIPE;
                SOCK_DEBUG(("\nSOCKET: Send failed: soc %x tcp connection
                           closed", soc));
                return(SO_FAIL);
            }

	    /*
	     * Non-blocking socket. Return EWOULDBLOCK if writing
	     * will be blocked.
	     */ 
	    if (!sock_writeable(soc)) {
		errno = EWOULDBLOCK;
                SOCK_DEBUG(("\nSOCKET: Send failed: soc %x blocking", soc));
		return(SO_FAIL);
	    }
	}	
	if (soc->so_proto == IPPROTO_TCP)
	    cnt = sock_send_tcp(soc, (void *)buf, len);
	else
	    cnt = sock_send_pgm(soc, (void *)buf, len);
        if (!sock_writeable(soc))
            socket_clear_event(soc, SOCKET_WRITE_EVENT);
	return(cnt);
    }
    if (soc->so_proto == IPPROTO_UDP) {
	cnt = sock_sendto_udp(soc, (void *)buf, len, NULL, 0);
	return(cnt);
     }
    else {
	SOCK_DEBUG(("\nSOCKET: Send failed: protocol %d not supported", 
		    soc->so_proto));
	errno = EPROTONOSUPPORT;
	return(INVALID_FD);
    }
}


/*
 * socket_connect
 */
int 
socket_connect (int fd, const struct sockaddr *name, socklen_t name_len)
{
    socktype *soc;
    sockaddr_in *tsin;
    int status;
    sockaddr_in6 *sin6;

    soc = getsock_by_fd(fd);
    status= sanity_check_sock(soc);
    if (status == SO_FAIL) {
	SOCK_DEBUG(("\nSOCKET: Connect failed: socket 0x%x is not sane", soc));
	errno = EBADF;
        return(SO_FAIL);
    }

    /* The family of the socket and the passed address should match */
    if (soc->so_family != name->sa_family) {
	SOCK_DEBUG((
        "\nSOCKET: Connect failed: socket family %d and addr family %d not same",
		    soc->so_family, name->sa_family));
	errno = EAFNOSUPPORT;
	return(SO_FAIL);
    }

    /* Validate name_len */
    if ((name->sa_family == AF_INET) && (name_len != sizeof(sockaddr_in)) || 
        (name->sa_family == AF_INET6) && (name_len != sizeof(sockaddr_in6)) ) {
	SOCK_DEBUG(("\nSOCKET: Connect failed: Incorrect address length %d ", 
		    name_len));
	errno = EINVAL;
	return(SO_FAIL);
    }

    switch (name->sa_family) {
    case AF_INET:
        tsin = (sockaddr_in *) name;
        if (tsin == NULL || (tsin->sin_addr.s_addr == INADDR_ANY)) {
            SOCK_DEBUG(("\nSOCKET: Connect failed: no foreign address"));
            errno = EDESTADDRREQ;
            return(SO_FAIL);
        }        
        break;
    case AF_INET6:
        sin6 = (sockaddr_in6 *) name;
        if (sin6 == NULL || IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr)) {
            SOCK_DEBUG(("\nSOCKET: Connect failed: no foreign address"));
            errno = EDESTADDRREQ;
            return(SO_FAIL);
        }                                     
        break;
    default:
        SOCK_DEBUG(("\nSOCKET: Connect failed: Invalid family"));
        return(SO_FAIL);                     
    }     
  
    status = SO_FAIL;
    if ((soc->so_proto == IPPROTO_TCP) || (soc->so_proto == IPPROTO_PGM)) {
        if (soc->so_stateflags & SS_ISCONNECTED) {
            SOCK_DEBUG(("\nSOCKET: Connect Succeeded: "));
            SOCK_DEBUG(("connection established for socket 0x%x", soc));
            errno = EISCONN;
	} else if (soc->so_stateflags & SS_ISCONNECTING) {
	    SOCK_DEBUG(("\nSOCKET: Connect failed: "));
	    SOCK_DEBUG(("connecting in progress for socket 0x%x", soc));
	    errno = EALREADY;
	} else {
	    if (soc->so_proto == IPPROTO_TCP)
		status = sock_connect_tcp(soc, (const sockaddr *)name);
	    else
		status = sock_connect_pgm(soc, (sockaddr_in*)name, name_len);
	}
    } else if (soc->so_proto == IPPROTO_UDP) {
	status = sock_connect_udp(soc, (const sockaddr *)name);
    } else {
	SOCK_DEBUG(("\nSOCKET: Connect failed: protocol %d not supported", 
		   soc->so_proto));
	errno = EPROTONOSUPPORT;
    }
    return(status);
}
 

/*
 * socket_get_localname
 *
 * Retrives the local address bound to the specified socket and places
 * it in the buffer passed in. This is useful when the local address and
 * local port was selected by TCP during an implicit bind or when the 
 * user aprocess sepcified a wildcard address during an explicit call 
 * to bind.
 */
int
socket_get_localname (int fd, sockaddr *local_name, int *name_len)
{
    int status;
    socktype *soc;
    sockaddr_in *tsin; 
    sockaddr_in6 *sin6; 

    soc = getsock_by_fd(fd);
    status= sanity_check_sock(soc);
    if (status == SO_FAIL) {
	SOCK_DEBUG(("\nSOCKET: Get socket name failed: "));
	SOCK_DEBUG(("socket 0x%x is not sane", soc)); 
	errno = EBADF;
        return(SO_FAIL);
    }

    if (!(soc->so_options & SO_IOS_TASK_ID) &&
        (soc->so_stateflags & (SS_CANTRCVMORE | SS_CANTSENDMORE))) {
	/* Posix socket has been shutdown() */
	errno = EINVAL;
        return (SO_FAIL);
    }

    if (local_name && name_len) {
            switch (soc->so_family) {
            case AF_INET:
                if (*name_len < (int)sizeof(sockaddr_in)) {
                    SOCK_DEBUG(("\nSOCKET: Get local name 0 len"));
                    *name_len = 0;
                    break;
                }
                tsin = (sockaddr_in *)local_name;
                tsin->sin_len = sizeof(sockaddr_in);
                tsin->sin_family = AF_INET;
                tsin->sin_port = soc->so_lport;
                tsin->sin_addr.s_addr = soc->so_laddr.ip_addr;
                 *name_len = sizeof(sockaddr_in);
                break;
            case AF_INET6:
                if (*name_len < (int)sizeof(sockaddr_in6)) {
                    SOCK_DEBUG(("\nSOCKET: Get local name 0 len"));
                    *name_len = 0;
                    break;
                }
                sin6 = (sockaddr_in6 *)local_name;
                sin6->sin6_len = sizeof(sockaddr_in6);
                sin6->sin6_family = AF_INET6;
                sin6->sin6_port = soc->so_lport;
                sin6->sin6_addr = soc->so_laddr.ipv6_addr;
                *name_len = sizeof(sockaddr_in6);
                break;
            default:
		errno = EACCES;
                SOCK_DEBUG(("\nSOCKET: socket_get_localname(), invalid family"));
                return(SO_FAIL);
            }
         } else {
	     SOCK_DEBUG(("\nSOCKET: Get local name 0 len"));
             *name_len = 0;
         }
    return(SO_SUCCESS);
}
 

/*
 * socket_get_peername
 *
 * UNIX getpeername() equivalent.
 * Returns the address of the remote end of the connection associated
 * with the specified socket.
 */
int
socket_get_peername (int fd, sockaddr *peer_name, int *name_len)
{

    int status;
    socktype *soc;
    sockaddr_in *tsin;
    sockaddr_in6 *sin6;

    soc = getsock_by_fd(fd);
    status= sanity_check_sock(soc);
    if (status == SO_FAIL) {
	SOCK_DEBUG(("\nSOCKET: Get peer name failed: socket 0x%x is not sane", 
		    soc));
	errno = EBADF;
        return(SO_FAIL);
    }

    if ((soc->so_proto == IPPROTO_TCP) || (soc->so_proto == IPPROTO_PGM)) {
	if (!(soc->so_stateflags & SS_ISCONNECTED)) {
	    SOCK_DEBUG(("\nSOCKET: Get peer name failed: "));
	    SOCK_DEBUG(("socket 0x%x is not connected", soc));
	    sock_state2string(soc->so_stateflags);
	    errno = ENOTCONN;
	    return(SO_FAIL);
	}
    }

    if (!(soc->so_options & SO_IOS_TASK_ID) &&
        (soc->so_stateflags & (SS_CANTRCVMORE | SS_CANTSENDMORE))) {
	/* Posix socket has been shutdown() */
	errno = EINVAL;
        return (SO_FAIL);
    }

    if (peer_name && name_len) {
            switch (soc->so_family) {
            case AF_INET:
                if (*name_len < (int)sizeof(sockaddr_in)) {
                    SOCK_DEBUG(("\nSOCKET: Get peer name 0 len"));
                    *name_len = 0;
                    break;
                }
                tsin = (sockaddr_in *)peer_name;
                tsin->sin_len = sizeof(sockaddr_in);
                tsin->sin_family = AF_INET;
                tsin->sin_port = soc->so_fport;
                tsin->sin_addr.s_addr = soc->so_faddr.ip_addr;
                *name_len = sizeof(sockaddr_in);
                break;
            case AF_INET6:
                if (*name_len < (int)sizeof(sockaddr_in6)) {
                    SOCK_DEBUG(("\nSOCKET: Get peer name 0 len"));
                    *name_len = 0;
                    break;
                }
                sin6 = (sockaddr_in6 *)peer_name;
                sin6->sin6_len = sizeof(sockaddr_in6);
                sin6->sin6_family = AF_INET6;
                sin6->sin6_port = soc->so_fport;
                sin6->sin6_addr = soc->so_faddr.ipv6_addr;
                *name_len = sizeof(sockaddr_in6);
                break;
            default:
		errno = EACCES;
                SOCK_DEBUG(("\nSOCKET: socket_get_peername(), invalid family"));
                return(SO_FAIL);
            }
        } else {
	   SOCK_DEBUG(("\nSOCKET: Get peer name 0 len"));
           *name_len = 0;
        }
    return(SO_SUCCESS);
}
    

/*
 * socket_shutdown
 * Close the read-half, write-half, or both halves of a connection.
 */
int
socket_shutdown (int fd, int how)
{
    socktype *soc;

    if (fd == INVALID_FD) {
	SOCK_DEBUG(("\nSOCKET: Shutdown failed: invalid file descriptor"));
	errno = EINVAL;
	return(SO_FAIL);
    }

    soc = getsock_by_fd(fd);
    if (soc == NULL) {
        SOCK_DEBUG(("\nSOCKET: Shutdown failed: no socket for file descriptor"));
	errno = EBADF;
	return(SO_FAIL);
    }
    
    if (!(how & (READ_CLOSE | WRITE_CLOSE))) {
	errno = EINVAL;
	return (SO_FAIL);
    }

    if (soc->so_proto == IPPROTO_TCP) {
	return (sock_shutdown_tcp(soc, how));
    } else if (soc->so_proto == IPPROTO_UDP) {
	return (sock_shutdown_udp(soc, how));
    } else {
        SOCK_DEBUG(("\nSOCKET: Shutdown failed: protocol %d not supported", 
		    soc->so_proto));
	errno = EAFNOSUPPORT;
        return(SO_FAIL);
    }
}


/*
 * socket_close
 * Close the socket and remove the socket from the process socket list.
 */
int
socket_close (int fd)
{
    socktype *soc;

    if (fd == INVALID_FD) {
	SOCK_DEBUG(("\nSOCKET: Close failed: invalid file descriptor %d", fd));
	return(SO_FAIL);
    }

    soc = getsock_by_fd(fd);
    if (soc == NULL) {
	SOCK_DEBUG(("\nSOCKET: Close failed: "));
	SOCK_DEBUG(("no socket for file descriptor %d", fd));
	return(SO_FAIL);
    }
    soc->refcnt--;
    if (soc->refcnt > 0)
	return(SO_SUCCESS);
	
    soc->so_stateflags |= (SS_CANTRCVMORE | SS_CANTSENDMORE);
    if (!soc->so_data) {
	/*
	 * Either the transport layer hasn't been connected or something
	 * very bad happened. Blow away the socket and return.
	 */
	socket_clear_event(soc, SOCKET_ALL_EVENTS);
	sock_conn_dead(soc);
	return(SO_SUCCESS);
    }

    switch (soc->so_proto) {
      case IPPROTO_TCP: 
	sock_close_tcp(soc);
	break;
      case IPPROTO_UDP:
	sock_close_udp(soc);
	break;
      case IPPROTO_PGM:
	sock_close_pgm(soc);
	break;
      default:
	SOCK_DEBUG(("\nSOCKET: Close failed: protocol %d not supported", 
		    soc->so_proto));
	break;
    }
    /*
     * Clear event if there is any one pending on the event list.
     */
    socket_clear_event(soc, SOCKET_ALL_EVENTS);
    sock_conn_dead(soc);
    return(SO_SUCCESS);
}




/* 
 * socket_set_option
 * 
 * Set option for SOCKET, TCP, UDP, or IP layer. 
 */
int
socket_set_option (int fd, int level, int optname, const void *optval, socklen_t optlen)
{
    socktype *soc;
    int status;

    soc = getsock_by_fd(fd);
    if (soc == NULL) {
	errno = EBADF;
        return(SO_FAIL);
    }

    switch( level ) {
    case SOL_SOCKET:
	/* Option is for socket layer */
	status = sock_set_socket_option(soc, optname, (void *)optval, optlen);
	/* errno is already set by sock_set_socket_option */
	if (status == SO_FAIL) {
	    SOCK_DEBUG(("\nSOCKET: set Socket option failed, errno %d, "
		"option 0x%X, fd %d, socket 0x%X",
	       	errno, optname, fd, soc));
	} else {
	    SOCK_DEBUG(("\nSOCKET: set Socket option success, option 0x%X, "
		"fd %d, socket 0x%X",
	       	optname, fd, soc));
	}
        break;

    case SOL_TCP:
	/* Process option for TCP */
	if (soc->so_proto != IPPROTO_TCP)  {
	    errno = EINVAL;
	    return (SO_FAIL);
	}
	status = sock_set_tcp_option(soc, optname, (void *)optval, optlen);
	/* errno is already set by sock_set_tcp_option */
	if (status == SO_FAIL) {
	    SOCK_DEBUG(("\nSOCKET: set TCP option failed, errno %d, "
		"option 0x%X, fd %d, socket 0x%X",
	       	errno, optname, fd, soc));
	} else {
	    SOCK_DEBUG(("\nSOCKET: set TCP option success, option 0x%X, "
		"fd %d, socket 0x%X",
	       	optname, fd, soc));
	}
        break;

    case SOL_UDP:
	/* Process option for UDP */
	if (soc->so_proto != IPPROTO_UDP)  {
	    errno = EINVAL;
	    return (SO_FAIL);
	}
	status = sock_set_udp_option(soc, optname, (void *)optval, optlen);
	/* errno is already set by sock_set_udp_option */
	if (status == SO_FAIL) {
	    SOCK_DEBUG(("\nSOCKET: set UDP option failed, errno %d, "
		"option 0x%X, fd %d, socket 0x%X",
	       	errno, optname, fd, soc));
	} else {
	    SOCK_DEBUG(("\nSOCKET: set UDP option success, option 0x%X, "
		"fd %d, socket 0x%X",
	       	optname, fd, soc));
	}
        break;

    case SOL_IP:
	/* Process option for IP */
	if (soc->so_family != AF_INET) {
	    errno = EINVAL;
	    return (SO_FAIL);
	}
	status = sock_set_ip_option(soc, optname, (void *)optval, optlen);
	/* errno is already set by sock_set_ip_option */
	if (status == SO_FAIL) {
	    SOCK_DEBUG(("\nSOCKET: set IP option failed, errno %d, "
		"option 0x%X, fd %d, socket 0x%X",
	       	errno, optname, fd, soc));
	} else {
	    SOCK_DEBUG(("\nSOCKET: set IP option success, option 0x%X, "
		"fd %d, socket 0x%X",
	       	optname, fd, soc));
	}
        break;

    case SOL_PGM:
	/* Process option for PGM */
	status = sock_set_pgm_option(soc, optname, (void *)optval, optlen);
        break;

    case SOL_IPV6:
        /* Process option for IPv6 */
	if (soc->so_family != AF_INET6) {
	    errno = EINVAL;
	    return (SO_FAIL);
	}
        status = sock_set_ipv6_option(soc, optname, (void *)optval, optlen);
	/* errno is already set by sock_set_ipv6_option */
	if (status == SO_FAIL) {
	    SOCK_DEBUG(("\nSOCKET: set IPv6 option failed, errno %d, "
		"option 0x%X, fd %d, socket 0x%X",
	       	errno, optname, fd, soc));
	} else {
	    SOCK_DEBUG(("\nSOCKET: set IPv6 option success, option 0x%X, "
		"fd %d, socket 0x%X",
	       	optname, fd, soc));
	}
        break;

    default:
	/* Option for other protocol are not yet supported */
	errno = EPROTONOSUPPORT;
	status = SO_FAIL;
	break;
    }

    return(status);
}
	

/*
 * socket_get_option
 *
 * Get option for SOCKET, TCP, or IP layer. 
 */
int
socket_get_option (int fd, int level, int optname, void *optval, int *optlen)
{
    socktype *soc;
    int status;

    soc = getsock_by_fd(fd);
    if (soc == NULL) {
        errno = EBADF;
        return(SO_FAIL);
    }

    switch( level ) {
    case SOL_SOCKET:
	/* Option is for socket layer */
	status = sock_get_socket_option(soc, optname, optval, optlen);
	/* errno is already set by sock_get_socket_option */
	if (status == SO_FAIL) {
	    SOCK_DEBUG(("\nSOCKET: get Socket option failed, errno %d, "
		"option 0x%X, fd %d, socket 0x%X",
	       	errno, optname, fd, soc));
	} else {
	    SOCK_DEBUG(("\nSOCKET: get Socket option success, option 0x%X, "
		"fd %d, socket 0x%X",
	       	optname, fd, soc));
	}
        break;

    case SOL_TCP:
	/* Process option for TCP */
	if (soc->so_proto != IPPROTO_TCP)  {
	    errno = EINVAL;
	    return (SO_FAIL);
	}
	status = sock_get_tcp_option(soc, optname, optval, optlen);
	/* errno is already set by sock_get_tcp_option */
	if (status == SO_FAIL) {
	    SOCK_DEBUG(("\nSOCKET: get TCP option failed, errno %d, "
		"option 0x%X, fd %d, socket 0x%X",
	       	errno, optname, fd, soc));
	} else {
	    SOCK_DEBUG(("\nSOCKET: get TCP option success, option 0x%X, "
		"fd %d, socket 0x%X",
	       	optname, fd, soc));
	}
        break;

    case SOL_UDP:
	/* Process option for UDP */
	if (soc->so_proto != IPPROTO_UDP)  {
	    errno = EINVAL;
	    return (SO_FAIL);
	}
	status = sock_get_udp_option(soc, optname, optval, optlen);
	/* errno is already set by sock_get_udp_option */
	if (status == SO_FAIL) {
	    SOCK_DEBUG(("\nSOCKET: get UDP option failed, errno %d, "
		"option 0x%X, fd %d, socket 0x%X",
	       	errno, optname, fd, soc));
	} else {
	    SOCK_DEBUG(("\nSOCKET: get UDP option success, option 0x%X, "
		"fd %d, socket 0x%X",
	       	optname, fd, soc));
	}
        break;

    case SOL_IP:
	/* Process option for IP */
	if (soc->so_family != AF_INET) {
	    errno = EINVAL;
	    return (SO_FAIL);
	}
	status = sock_get_ip_option(soc, optname, optval, optlen);
	/* errno is already set by sock_get_ip_option */
	if (status == SO_FAIL) {
	    SOCK_DEBUG(("\nSOCKET: get IP option failed, errno %d, "
		"option 0x%X, fd %d, socket 0x%X",
	       	errno, optname, fd, soc));
	} else {
	    SOCK_DEBUG(("\nSOCKET: get IP option success, option 0x%X, "
		"fd %d, socket 0x%X",
	       	optname, fd, soc));
	}
        break;

    case SOL_PGM:
	/* Process option for PGM */
	status = sock_get_pgm_option(soc, optname, optval, optlen);
        break;

    case SOL_IPV6:
        /* Process option for IPv6 */
	if (soc->so_family != AF_INET6) {
	    errno = EINVAL;
	    return (SO_FAIL);
	}
        status = sock_get_ipv6_option(soc, optname, optval, optlen);
	/* errno is already set by sock_get_ipv6_option */
	if (status == SO_FAIL) {
	    SOCK_DEBUG(("\nSOCKET: get IPv6 option failed, errno %d, "
		"option 0x%X, fd %d, socket 0x%X",
	       	errno, optname, fd, soc));
	} else {
	    SOCK_DEBUG(("\nSOCKET: get IPv6 option success, option 0x%X, "
		"fd %d, socket 0x%X",
	       	optname, fd, soc));
	}
        break;
        
    default:
	/* Option for other protocol are not yet supported */
	errno = EPROTONOSUPPORT;
	status = SO_FAIL;
	break;
    }
    return(status);
}



/*
 * socket_gethostbyname
 */
hostent *
socket_gethostbyname (const char *name)
{
    hostent_buf *hostbuf_ptr = sock_host_getnext();

    if (! parse_hostname((char *)name, &hostbuf_ptr->ipaddress)) {
        printf("\n%% Bad IP address for host %s", name);
        return(NULL);
    }

    /* set the array of pointers up for returning to the calling routine */
    hostbuf_ptr->h_addr_ptrs[0] = &hostbuf_ptr->ipaddress;
    hostbuf_ptr->h_addr_ptrs[1] = NULL;

    /* fill in the hostent buffer with the correct information */
    hostbuf_ptr->host.h_name = (char *)name;
    hostbuf_ptr->host.h_addr_list = (char **)hostbuf_ptr->h_addr_ptrs;
    hostbuf_ptr->host.h_length = sizeof(ipaddrtype);
    hostbuf_ptr->host.h_addrtype = AF_INET;

    return(&hostbuf_ptr->host);

}


/*
 * socket_name2addr
 *
 * Internal function to look up the IPv4 or IPv6 address corresponding to a
 * hostname
 */
static int
socket_name2addr (int family, const char *name, addrtype *address)
{
    switch (family) {
    case AF_INET:
	if (!parse_hostname((char *)name, &address->ip_addr)) {
	    return (EAI_NONAME);
	}
	address->type = ADDR_IP;
	address->length = ADDRLEN_IP;
	break;
    case AF_INET6: {
	nametype *name_ptr;
	hostaddr *haddr;
	int dummy;
	name_ptr = name_lookup((char *)name, &dummy, NAM_IPV6);
	if (name_ptr &&
	    (haddr = name_first_addr_type(name_ptr, ADDR_IPV6)) != NULL) {
	    address_copy(address, &haddr->address);
	} else {
	    return (EAI_NONAME);
	}
	address->type = ADDR_IPV6;
	address->length = ADDRLEN_IPV6;
	break;
    }
    default:
	return (EAI_FAMILY);
    }
    return (0);
}


/*
 * socket_name2addr
 *
 * Internal function to look up the hostname corresponding to a IPv4 or IPv6
 * address
 */
static int
socket_addr2name (const addrtype *address, char *name, uint namelen)
{
    nametype *ptr;

    ptr = reg_invoke_name_lookup_number(address->type, (addrtype *)address);
    if (!ptr) {
	return (EAI_NONAME);
    }
    sstrncpy(name, ptr->name, namelen);
    return (0);
}


/*
 * socket_inet_pton()
 *
 * IOS equivalent of Posix inet_pton(), as documented at
 * http://www.opengroup.org/onlinepubs/009695399/functions/inet_pton.html
 */
int
socket_inet_pton (int af, const char *src, void *dst)
{
    addrtype address;
    switch (af) {
    case AF_INET:
	if (reg_invoke_parse_address(ADDR_IP, (char *)src, &address)) {
	    *(ipaddrtype *)dst = address.ip_addr;
	    return (1);
	}
	break;
    case AF_INET6:
	if (reg_invoke_parse_address(ADDR_IPV6, (char *)src, &address)) {
	    *(struct in6_addr *)dst = address.ipv6_addr;
	    return (1);
	}
	break;
    default:
	errno = EAFNOSUPPORT;
	return (-1);
    }
    return (0);
}


/*
 * socket_inet_pton()
 *
 * IOS equivalent of Posix inet_pton(), as documented at
 * http://www.opengroup.org/onlinepubs/009695399/functions/gethostbyaddr.html
 */
hostent *
socket_gethostbyaddr (const void *addr, socklen_t len, int type)
{
    addrtype address;
    hostent_buf *hostbuf_ptr = sock_host_getnext();

    /* sanity check params.  Only IPv4 addresses allowed */
    if (!addr || (len != ADDRLEN_IP) || (type != AF_INET)) {
	return (NULL);
    }

    address.type = ADDR_IP;
    address.length = ADDRLEN_IP;
    memcpy(&address.ip_addr, addr, len);
    hostbuf_ptr->host.h_name = hostbuf_ptr->name;
    if (socket_addr2name(&address, hostbuf_ptr->host.h_name, NI_MAXHOST)
	!= 0) {
	return (NULL);
    }

    /* set the array of pointers up for returning to the calling routine */
    hostbuf_ptr->h_addr_ptrs[0] = &address.ip_addr;
    /* don't try to get any secondary addresses for now */
    hostbuf_ptr->h_addr_ptrs[1] = NULL;

    /* fill in the hostent buffer with the correct information */
    hostbuf_ptr->host.h_addr_list = (char **)hostbuf_ptr->h_addr_ptrs;
    hostbuf_ptr->host.h_length = sizeof(ipaddrtype);
    hostbuf_ptr->host.h_addrtype = AF_INET;
    /* don't bother with aliases for now */
    hostbuf_ptr->host.h_aliases = NULL;

    return (&hostbuf_ptr->host);
}


/*
 * struct addrinfo_buf
 * This stucture containing addrinfo, sockaddr and char[NI_MAXHOST] is
 * allocated by socket_getaddrinfo(), instead of just addrinfo.  This way,
 * socket_freeaddrinfo() will just have to free just the addrinfo_buf.  This
 * will work because &addrinfo_buf will be the same as &addrinfo_buf.aib_ai,
 * since aib_ai is the 1st member
 */
struct addrinfo_buf {
    struct addrinfo aib_ai; /* must be in 1st position */
    char aib_sa[max(sizeof(struct sockaddr_in), sizeof(struct sockaddr_in6))];
    char aib_name[NI_MAXHOST];
};


/*
 * socket_getaddrinfo()
 *
 * IOS equivalent of Posix getaddrinfo(), as documented at
 * http://www.opengroup.org/onlinepubs/009695399/functions/getaddrinfo.html
 *
 * Exceptions to this are as follows:
 * - Only one addrinfo will be returned in the res pointer, and not a linked
 *     list
 * - Only AF_INET and AF_INET6 address families are supported
 * - No translation of service names, such as "ssh" or "http".  Only numerical
 *     strings will be translated, e.g. "23" or "80"
 * - If a hints structure is passed, the ai_socktype, and ai_protocol members
 *     will be ignored
 * - The AI_V4MAPPED and AI_ALL flags will be ignored
 */
int
socket_getaddrinfo (const char *nodename, const char *servname,
		    const struct addrinfo *hints, struct addrinfo **res)
{
    struct addrinfo_buf *ai_buf;
    struct addrinfo *ai_ptr;
    sockaddr_in *sin = (sockaddr_in *)&ai_buf->aib_sa;
    sockaddr_in6 *sin6 = (sockaddr_in6 *)&ai_buf->aib_sa;
    addrtype address;
    int family = AF_INET; /* assume IPv4 unless we find out otherwise */
    int flags = 0;
    ushort *port;
    int rv;
    ipaddr_cont_ref cref;
    idbtype *idb;

    /* check for sane arguments */
    if (!res || !(nodename || servname)) {
	errno = EINVAL;
	return (EAI_SYSTEM);
    }

    /* set address family and flags to non-default values, if necessary */
    if (hints) {
	if (hints->ai_family != AF_UNSPEC) {
	    family = hints->ai_family;
	}
	flags = hints->ai_flags;
	/* ignore ai_socktype and ai_protocol hints for now */
    }

    /* check for unsupported flags */
    if (flags &
	~(AI_PASSIVE |
	  AI_CANONNAME |
	  AI_NUMERICHOST |
	  AI_ADDRCONFIG |
	  AI_NUMERICSERV)) {
	/* Note AI_V4MAPPED and AI_ALL are not supported */
	return (EAI_BADFLAGS);
    }

    /* allocate the addrinfo buffer, including sockaddr and host string */
    ai_buf = malloc(sizeof(*ai_buf));
    if (!ai_buf) {
	return (EAI_MEMORY);
    }
    ai_ptr = &ai_buf->aib_ai;
    /* sanity check addrinfo is at start of addrinfo_buf */
    assert(ai_ptr == (struct addrinfo *)ai_buf);
    if (ai_ptr != (struct addrinfo *)ai_buf) {
	free(ai_buf);
	return (EAI_FAIL);
    }

    /* populate sockaddr with everything excluding address and port */
    switch (family) {
    case AF_INET: {
	int addr_count = 0;
	ipaddr_tabletype *entry;
	sin = (sockaddr_in *)&ai_buf->aib_sa;
	sin->sin_family = AF_INET;
	port = &sin->sin_port; /* get ptr to port in sockaddr struct */
	ai_ptr->ai_addr = (sockaddr *)sin;
	ai_ptr->ai_addrlen = sizeof(sockaddr_in);
	/* check if IPv4 addr is configured on any interface */
	if (flags & AI_ADDRCONFIG) {
	    FOR_NUMBERED_ENTRIES(entry, IPROUTING_ALL_TABLEID, cref) {
		addr_count++;
		/* we have at least 1 IPv4 addr configured */
		break;
	    }
	    if (!addr_count) {
		free(ai_buf);
		return (EAI_NONAME);
	    }
	}
	break;
    }
    case AF_INET6:
	sin6 = (sockaddr_in6 *)&ai_buf->aib_sa;
	sin6->sin6_family = AF_INET6;
	port = &sin6->sin6_port; /* get ptr to port in sockaddr struct */
	ai_ptr->ai_addr = (sockaddr *)sin6;
	ai_ptr->ai_addrlen = sizeof(sockaddr_in6);
	/* check if IPv6 addr is configured on any interface */
	if ((flags & AI_ADDRCONFIG) &&
	    !reg_invoke_ipv6_forwarding_is_active()) {
	    free(ai_buf);
	    return (EAI_NONAME);
	}
	break;
    default:
	free(ai_buf);
	return (EAI_FAMILY);
    }

    /* Do hostname translations */
    if (nodename) {
	/* sanity check hostname length */
	if ((strlen(nodename) + 1) > NI_MAXHOST) {
	    free(ai_buf);
	    return (EAI_NONAME);
	}

	if (flags & AI_NUMERICHOST) {
	    /* numeric translation only, without any cache or DNS lookup */
	    if (socket_inet_pton(family, nodename, &address.addr) != 1) {
		free(ai_buf);
		return (EAI_NONAME);
	    }
	} else {
	    /* translate name to address */
	    /* Check cache first then do DNS lookup if necessary */
	    rv = socket_name2addr(family, nodename, &address);
	    if (rv != 0) {
		free(ai_buf);
		return (rv);
	    }
	}
	/* marshall addrtype to sockaddr */
	switch (family) {
	case AF_INET:
	    sin->sin_addr.s_addr = address.ip_addr;
	    break;
	case AF_INET6:
	    sin6->sin6_addr = address.ipv6_addr;
	    break;
	default:
	    free(ai_buf);
	    return (EAI_FAMILY);
	}

	if (flags & AI_CANONNAME) {
	    /* attempt to reverse lookup canonical name */
	    if (socket_addr2name(&address, hostname, NI_MAXHOST - 1) < 0) {
		/* if that fails, just copy out the passed name */
		sstrncpy(ai_buf->aib_name, hostname, NI_MAXHOST - 1);
	    }
	    ai_ptr->ai_canonname = ai_buf->aib_name;
	} else {
	    ai_ptr->ai_canonname = NULL;
	}
    } else { /* nodename == NULL */
	if (flags & AI_PASSIVE) {
	    /* set up sockaddr for bind() for passive connection */
	    switch (family) {
	    case AF_INET:
		sin->sin_addr.s_addr = INADDR_ANY;
		break;
	    case AF_INET6:
		sin6->sin6_addr = (struct in6_addr)IN6ADDR_ANY_INIT;
		break;
	    default:
		free(ai_buf);
		return (EAI_FAMILY);
	    }
	} else {
	    /* set up sockaddr for active connect() - use loopback addr */
	    boolean loopback_found = FALSE;
	    /* search for 1st loopback configured with given address family */
	    /* not sure if we should search all VRFs here */
	    FOR_NUMBERED_SWIDBS(idb, IPROUTING_ALL_TABLEID, cref) {
		if (interface_up(idb) && idb->hwptr->type == IDBTYPE_LB) {
		    switch (family) {
		    case AF_INET:
			sin->sin_addr.s_addr = ip_address_ours(idb);
			if (address.ip_addr != 0L) {
			    loopback_found = TRUE;
			    break;
			}
		    case AF_INET6: {
			ipv6_addr_block_t addr_block;
			addr_block.total = 0;
			if (reg_invoke_ipv6_address_get_addresses(idb,
								  &addr_block)
			    && (addr_block.total > 0)) {
			    sin6->sin6_addr = addr_block.address[0];
			    loopback_found = TRUE;
			    break;
			}
		    }
		    default:
			free(ai_buf);
			return (EAI_FAMILY);
		    }
		}
		if (loopback_found) {
		    break;
		}
	    }
	    if (!loopback_found) {
		free(ai_buf);
		return (EAI_NONAME);
	    }
	}
	/* we won't find a canonical name for loopback or wildcard */
	ai_ptr->ai_canonname = NULL;
    }

    /* Do servicename translations */
    if (servname) {
	/* sanity check servicename length */
	if ((strlen(servname) + 1) > NI_MAXSERV) {
	    free(ai_buf);
	    return (EAI_SERVICE);
	}

	/* Ignore AI_NUMERICSERV - numeric service translation only */
	if (sscanf(servname, "%hu", port) != 1) { /* "%hu" == ushort */
	    free(ai_buf);
	    return (EAI_SERVICE);
	}
    }

    ai_ptr->ai_family = family;
    ai_ptr->ai_next = NULL; /* return a list of just one element */

    *res = ai_ptr;
    return (0);
}


/*
 * socket_freeaddrinfo()
 *
 * IOS equivalent of Posix freeaddrinfo(), as documented at
 * http://www.opengroup.org/onlinepubs/009695399/functions/freeaddrinfo.html
 *
 * Frees the addrinfo and assoctiated sockname and hostname data that was 
 * allocated by socket_getaddrinfo()
 * Additionally, if a list of addrinfo structures is linked together using the
 * ai_next pointer, the whole list will be freed if the head of the list is
 * passed
 */
void
socket_freeaddrinfo (struct addrinfo *ai)
{
    struct addrinfo_buf *ai_buf;

    assert(ai);

    while (ai) {
	ai_buf = (struct addrinfo_buf *)ai;
	assert(ai == &ai_buf->aib_ai);
	ai = ai->ai_next;
	free(ai_buf);
    }
}


/*
 * socket_getnameinfo()
 *
 * IOS equivalent of Posix getnameinfo(), as documented at
 * http://www.opengroup.org/onlinepubs/009695399/functions/getnameinfo.html
 *
 * Exceptions to this are as follows:
 * - Only AF_INET and AF_INET6 address families are supported
 * - No translation of service names, such as "ssh" or "http".  Only numerical
 *     strings will be translated, e.g. "23" or "80"
 * - IPv4-mapped IPv6 addresses or IPv4-compatible IPv6 addresses will not be
 *     extracted to IPv4 addresses before translation
 * - The NI_DGRAM flag is not supported
 */
int
socket_getnameinfo (const sockaddr *sa, socklen_t salen,
		    char *node, socklen_t nodelen,
		    char *service, socklen_t servicelen, int flags)
{
    sockaddr_in *sin = (sockaddr_in *)sa;
    sockaddr_in6 *sin6 = (sockaddr_in6 *)sa;
    addrtype address;
    ushort port;
    uint32_t scope_id;
    int rv;

    /* sanity check arguments */
    if (!sa) {
	errno = EINVAL;
	return (EAI_SYSTEM);
    }
    if (!node && !service) {
	return (EAI_NONAME);
    }

    /* check for unsupported flags */
    if (flags &
	~(NI_NOFQDN |
	  NI_NUMERICHOST |
	  NI_NAMEREQD |
	  NI_NUMERICSERV |
	  NI_NUMERICSCOPE)) {
	/* Note NI_DGRAM is not supported */
	return (EAI_BADFLAGS);
    }

    /* validate salen, pull out necessary info from sockaddr */
    switch (sa->sa_family) {
    case AF_INET:
	if (salen < sizeof(sockaddr_in)) {
	    errno = EINVAL;
	    return (EAI_SYSTEM);
	}
	address.type = ADDR_IP;
	address.length = ADDRLEN_IP;
	address.ip_addr = sin->sin_addr.s_addr;
	port = sin->sin_port;
	break;
    case AF_INET6:
	if (salen < sizeof(sockaddr_in6)) {
	    errno = EINVAL;
	    return (EAI_SYSTEM);
	}
	address.type = ADDR_IPV6;
	address.length = ADDRLEN_IPV6;
	address.ipv6_addr = sin6->sin6_addr;
	port = sin6->sin6_port;
	scope_id = sin6->sin6_scope_id;
	break;
    default:
	return (EAI_FAMILY);
    }

    /* do address to name translation */
    if (node && (nodelen > 0)) {
	/* sanity check hostname length */
	if (nodelen > NI_MAXHOST) {
	    return (EAI_NONAME);
	}
	if ((sa->sa_family == AF_INET6) && (flags & NI_NUMERICSCOPE)) {
	    /* IPv6 scope id is required instead of hostname */
	    if (snprintf(node, nodelen - 1, "%u", scope_id) != 0) {
		return (EAI_NONAME);
	    }
	} else if (flags & NI_NUMERICHOST) {
	    /* return numeric string representation of address only */
	    if (!inet_ntop(sa->sa_family, &address.addr, node, nodelen - 1)) {
		return (EAI_NONAME);
	    }
	} else {
	    /* do reverse lookup */
	    rv = socket_addr2name(&address, node, nodelen - 1);
	    if (rv == EAI_NONAME) {
		/* lookup failed */
		if (flags & NI_NAMEREQD) {
		    return (EAI_NONAME);
		} else {
		    /* attempt numeric string representation of addr instead */
		    if (!inet_ntop(sa->sa_family, &address.addr,
				   node, nodelen - 1)) {
			return (EAI_NONAME);
		    }
		}
	    } else if (rv != 0) {
		/* some other error */
		return (rv);
	    }
	    /* We don't want the whole FQDN, just the hostname */
	    if (flags & NI_NOFQDN) {
		char *dot;
		dot = strchr(node, '.');
		if (dot) {
		    *dot = 0; /* terminate the name at the 1st dot */
		}
	    }
	}
    }

    /* do service to name translation */
    if (service && (servicelen > 0)) {
	/* sanity check serive name length */
	if (servicelen > NI_MAXSERV) {
	    return (EAI_SERVICE);
	}
	/*
	 * We have no facility to translate e.g. 22 to "ssh", so we always
	 * return numeric string representation of service only.
	 * NI_NUMERICSERV is ignored
	 */
	snprintf(service, servicelen - 1, "%u", port);
    }

    return (0);
}


/*
 * socket_inet_ntoa
 * Returns a pointer to a string in the base 256 notation ``d.d.d.d.''.
 */
char  *
socket_inet_ntoa (struct in_addr in)
{
    char *buf;
    struct {
        union {
	    struct { u_char s_b1,s_b2,s_b3,s_b4; } S_un_b;
	    struct { u_short s_w1,s_w2; } S_un_w;
	    u_long S_addr;
        } S_un;
    } in_addr_dissect;

    /* transfer the address to the temp buffer for dissection */
    in_addr_dissect.S_un.S_addr = in.s_addr;

    buf = sock_string_getnext();
    sprintf(buf,"%d.%d.%d.%d",
                in_addr_dissect.S_un.S_un_b.s_b1, 
		in_addr_dissect.S_un.S_un_b.s_b2,
	        in_addr_dissect.S_un.S_un_b.s_b3, 
	        in_addr_dissect.S_un.S_un_b.s_b4);
    return(buf);
}

/*
 * socket_inet_addr
 * Interpret character strings representing numbers expressed in the
 * Internet standard `.' notation, returning numbers suitable for use
 * as Internet address. 
 */
uint32_t
socket_inet_addr (const char *cp)
{
    ipaddrtype addr;

    if (parse_ip_address((char *)cp, &addr))
	return((uint32_t)addr);
    else 
	return(SO_FAIL);
}
    
/*
 * socket_inet_network
 * Interpert character strings representing numbers expressed in the
 * Internet standard `.' notation, returning numbers suitable for use
 * as Internet address. 
 */
ulong
socket_inet_network (const char *cp)
{
   ipaddrtype network;
   ipaddrtype addr;

   addr = socket_inet_addr(cp);
   if (addr == (ulong)SO_FAIL)
	return(SO_FAIL);
   network = get_majornet(addr);
   if (network == 0L)
	return(SO_FAIL);
   else
 	return((ulong)network);
}


/*
 * socket_allocbuf_recvfrom

 * This library function is for UDP connections only.
 * allocates buffer memory and reads data from a socket referenced by the 
 * file descriptor.
 *
 * If from is not a NULL pointer, the  source  address  of  the
 * message  is filled in.  fromlen is a value-result parameter,
 * initialized to the size of the buffer associated with  from,
 * and  modified  on  return to indicate the actual size of the
 * address  stored  there.
 *
 * Returns the bytes read or -1 if an error had occured during
 * reading.
 *
 * Note: If this function call fails with errno set to ENOMEM, the user should use 
 * the regular sock_recvfrom.
 */
int 
socket_allocbuf_recvfrom (int fd, char **buf, sockaddr *from, int *fromlen)
{
    int cnt;
    socktype *soc;
    int status;

    soc = getsock_by_fd(fd);
    status= sanity_check_sock(soc);
    if (status == SO_FAIL) {
        SOCK_DEBUG(("\nSOCKET: Allocbuf_Recvfrom failed: socket 0x%x is not sane", soc));
        errno = EBADF;
        return(SO_FAIL);
    }
    if (soc->so_proto != IPPROTO_UDP) {
        SOCK_DEBUG(("\nSOCKET: Allocbuf_Recvfrom failed: protocol %d not supported", 
                    soc->so_proto));
        errno = EPROTONOSUPPORT;
        return(SO_FAIL);
    }
    if (!sock_readable(soc)) {
            socket_clear_event(soc, SOCKET_READ_EVENT);
        if (soc->so_options & SO_NBIO) {
            /*
             * Non-blocking socket. Return EWOULDBLOCK if reading
             * will be blocked.
             */ 
            errno = EWOULDBLOCK;
            SOCK_DEBUG(("\nSOCKET: Allocbuf_Recvfrom failed: soc %x blocking", soc));
            return(SO_FAIL);
        }
    }   
    cnt = sock_allocbuf_recvfrom_udp(soc, buf, from, fromlen);
    if (!sock_readable(soc) || (cnt <= 0 && (errno != ENOMEM)))
        socket_clear_event(soc, SOCKET_READ_EVENT);
    return(cnt);
}

