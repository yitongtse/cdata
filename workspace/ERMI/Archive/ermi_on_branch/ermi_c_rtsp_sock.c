/*
 *-------------------------------------------------------------------
 * ermi_c_rtsp_sock.c socket functions to open/close sockets,
 *                 send/receive socket msgs
 *
 * January 2004, Linda Hua: July-2008, Neeraj Khurana
 *
 * Copyright (c) 2004-2009 by cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------------------
 */
#include COMP_INC(cisco, ciscolib.h)
#include COMP_INC(network, address.h)
#include COMP_INC(kernel/memory, chunk.h)
#include IOS_INC(socket/socket.h)
#include IOS_INC(socket/select.h)
#include IOS_INC(socket/sock_internal.h)
#include <sched.h>
#include <assert.h>
#include <logger.h>
#include <errno.h>

#include "ermi_video_debug.h"
#include "ermi_c_global.h"
#include "ermi_c_master.h"
#include "ermi_video_hash.h"
#include "ermi_c_rtsp_def.h"
#include "ermi_c_rtsp_api.h"
#include "msg_ermi_c_vcp.c"

#include INTERNAL_INC(sup/src/vclient/video_sg.h)

/*                                                                         
 * *   DESCRIPTION                                                           
 * *                                                                         
 * *       This function swaps 2 bytes of a 16-bit number. Note that this 
 * *       function will work on both Big Endian and Little Endian 
 * *       architectures. On Big Endian architectures this function will 
 * *       not change the byte ordering.  
 */

/* set the options */
static ermi_status rtsp_sock_set_option (int fd)
{

    /* Set non-blocking mode */
    int optval = 1;

    /* Non-Blocked I/O */
    if (socket_set_option(fd, SOL_SOCKET, SO_NBIO, (void *)&optval,
                          sizeof(optval)) == SO_FAIL) {
        socket_close(fd);
        return ERMI_RTSP_SOCK_ERROR;
    }

    /* set keepalive */
    if (socket_set_option(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&optval,
                          0) == SO_FAIL) {
        errmsg(&msgsym(SWERR, RTSP),"rtsp socket_set_option");        
        socket_close(fd);
        return ERMI_RTSP_SOCK_ERROR;        
    }
    
    /* Reuse-Address : Added for USRM compliance - under test */
    if (socket_set_option(fd, SOL_SOCKET, SO_REUSEADDR, &optval,
                          sizeof(optval)) == SO_FAIL) {
        errmsg(&msgsym(SWERR, RTSP),"rtsp socket_set_option");        
        socket_close(fd);
        return ERMI_RTSP_SOCK_ERROR;        
    }

    return (ERMI_OK);
}

/* open up a listening socket */
int rtsp_svr_sock_open (ipaddrtype local_addr, uint16 port)
{
    int listen_fd, yes;
    int status;
    sockaddr_in sin;

    /*
     * Fill in the connection socket values.
     */
    bzero(&sin, sizeof(sockaddr_in));

    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);

    /*
     * localAddr: zero: server well-known addr
     * Otherwise, listen to a specific addr..
     */
    sin.sin_addr.s_addr = (local_addr) ? local_addr : INADDR_ANY;

    /*
     * Create a listening socket.
     */
    listen_fd = socket_open(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_fd == INVALID_FD) {
        errmsg(&msgsym(SWERR, RTSP), "rtsp server socket_open failed");
        return INVALID_FD;
    }

    ERMI_C_RTSP_SOCK_BUGINF("\nERMI RTSP: %s:Listen on port %d sender fd=%d",
                            __FUNCTION__, sin.sin_port, listen_fd);

    status = socket_bind(listen_fd, (sockaddr *) &sin, sizeof(sin));
    if (status == SO_FAIL) {
        errmsg(&msgsym(SWERR, RTSP),"rtsp server socket_bind failed");
        socket_close(listen_fd);
        return INVALID_FD;
    }

    yes = 1;
    status = socket_set_option(listen_fd, SOL_SOCKET, SO_NBIO,
                               &yes, sizeof (yes));
    if (status == SO_FAIL) {
        socket_close(listen_fd);
        return INVALID_FD;
    }

    status = socket_listen(listen_fd, MAX_ERMI_SERVERS);
    if (status == SO_FAIL) {
        errmsg(&msgsym(SWERR, RTSP),"rtsp server socket_listen failed");
        socket_close(listen_fd);
        return INVALID_FD;
    }

    return listen_fd;
}

/*
 * Validate if Socket Connect is from a known Server Group IP addresses,
 */
boolean ermi_c_sock_validate_erm_sock (rfgw_video_server_group *video_sg,
                                       ipaddrtype erm_addr,
                                       uint32 *svr_index)
{
    boolean erm_validated = FALSE;
    uint index = SERVER_NOT_FOUND;

    if (video_sg) {
        index = rfgw_find_server_in_group_by_ip(video_sg, erm_addr);
        if (index != SERVER_NOT_FOUND) {
            erm_validated = TRUE;
        }
    } else {

        /* Find amongst all active ERMI-Video Server group */
        list_element *sg_elem;
        FOR_ALL_DATA_IN_LIST(&rfgw_video_sg_list, sg_elem, video_sg) {

            if (video_sg->video_protocol == RFGW_VIDEO_PROTOCOL_ERMI) {
                index = rfgw_find_server_in_group_by_ip(video_sg, erm_addr);
                if (index != SERVER_NOT_FOUND) {
                    erm_validated = TRUE;
                    break;
                }
            }
        }
    }

    if (!erm_validated) {
        ERMI_TRACE;
    }

    *svr_index = index;

    return (erm_validated);
}

/* accept the connect request from a client */
int rtsp_svr_sock_accept (int listen_fd)
{
    int str_len;
    int fd;
    ermi_c_conn_t *accept_conn;
    sockaddr_in sin;
    rtsp_conn_t *rtsp_conn;
    ermi_c_peer_t *peer;
    ermi_protocol_info_t *ermi_proto_info;
    rfgw_video_server_group *video_sg;
    ipaddrtype erm_addr;
    uint32 svr_index;

    ermi_proto_info = ermi_c_get_ermi_proto_info(NULL);

    if (!ermi_proto_info) {
        errmsg(&msgsym(SWERR, RTSP), "ERMI Protocol Struct not initialized",
               ermi_proto_info, ".");
        return INVALID_FD;
    }

    video_sg = ermi_proto_info->errp_mynode->erm_param.server_group;
    peer = ermi_proto_info->ermi_c_info;

    ERMI_TRACE;

    if (!peer) {
        errmsg(&msgsym(SWERR, RTSP), "Error: NULL Peer");
        return INVALID_FD;
    }

    str_len = sizeof(sockaddr);
    fd = socket_accept(listen_fd, (sockaddr *) &sin, &str_len);
    if (fd == SO_FAIL) {
        errmsg(&msgsym(SWERR, RTSP), "rtsp server socket_accept failed");
        return INVALID_FD;
    }

    ERMI_TRACE;

    /*
     * Validate IP in Socket Connect against known Server Group IP addresses,
     * Accept a connection only from ERM that is configured in the ServerGrp.
     */
    erm_addr = sin.sin_addr.s_addr;
    if (!ermi_c_sock_validate_erm_sock(video_sg, erm_addr, &svr_index)) {
        ERMI_C_RTSP_SOCK_BUGINF(
           "\nERROR: Socket Connect Req from Un-configured ERM %i. listen_fd %d"
           " SG= %s", erm_addr, listen_fd, (video_sg ? video_sg->sg_name:"") );
        socket_close(fd);
        // Other cleanup ...
        return INVALID_FD;
    }

    // NK_TBD: Add new conn[svr_index] entry if (erm_addr != peer->conn.dest_addr)
    if (peer->conn.state != SOCK_IDLE) {
        if (erm_addr != peer->conn.dest_addr) {
            errmsg(&msgsym(SWERR, RTSP),
                   "rtsp server conn entry %d error", fd);
            socket_close(fd);
            return INVALID_FD;
        }
    }

    if (rtsp_sock_set_option(fd) == SO_FAIL) {
        socket_close(fd);
        return INVALID_FD;
    }

    ERMI_TRACE;

    accept_conn = &peer->conn;
    if (accept_conn->peer == NULL) {

        /* Never initialized ERMI-C socket connection. Copy record */
        memcpy(accept_conn, &ermi_c_svr_conn, sizeof(ermi_c_conn_t));
        accept_conn->peer = peer;
        accept_conn->is_listen_fd = FALSE;
    } else {
        /* Socket reconnect on a pre-existing ERMI-C connection record. */
        accept_conn->type  = RTSP_SVR_CONN;
    }

    accept_conn->state = SOCK_CONNECTED;
    accept_conn->fd = fd;

    ERMI_C_RTSP_SOCK_BUGINF("\nSocket Accept: Addr=%i (SG=%s) listen=%d fd=%d",
                            erm_addr, video_sg->sg_name, listen_fd, fd);

    /* alloc rtsp_conn */
    rtsp_conn = rtsp_new_conn(accept_conn, RTSP_SVR_CONN);
    if (rtsp_conn == NULL) {
        socket_close(fd);
        return (INVALID_FD);
    }

    /* initialize the src-addr. remember the destination */
    accept_conn->src_addr = local_ip; 
    accept_conn->dest_addr = erm_addr;

    ERMI_C_RTSP_SOCK_BUGINF("\n%s:New Conn fd %d (listen_fd %d), rtsp_conn %#X",
                            __FUNCTION__, accept_conn->fd, 
                            listen_fd, rtsp_conn);

    return (fd);
}

static ermi_status rtsp_sock_xmit (rtsp_conn_t *rtsp_conn,
                                   char* buf, int len)
{
    int bytes_sent;
    rtsp_buf_t *rtsp_buf;
    int fd = rtsp_conn->conn->fd;

    ERMI_C_RTSP_SOCK_BUGINF("\nTransmitting:\n%s", buf);
    if (rtsp_conn->conn->type == RTSP_CLIENT_CONN) {
        ERMI_C_RTSP_SOCK_BUGINF("\nsock_xmit for session %d",
                             rtsp_conn->cur_session->index);
    }

    while ( (bytes_sent = socket_send(fd, buf, len, 0)) < len) {
        /* cannot send in one shot */
        if (bytes_sent < 0) {
            /* send failed */
            if (errno != EWOULDBLOCK) {
		/* failed not because sending is blocked, give up,
                   and cry to caller  */
                errmsg(&msgsym(SWERR, RTSP), "socket send for fd", fd, 
                                             "failed");
                ERMI_C_RTSP_SOCK_BUGINF("\nSocket send Failed: %i:%d,fd=%d 
                           errno=%d",
                           rtsp_conn->conn->dest_addr, rtsp_conn->conn->port,
                           fd, errno);
                return (ERMI_RTSP_SOCK_ERROR);
            }
            /* failed because sending is blocked, try again later */
            ERMI_C_RTSP_SOCK_BUGINF("\n(EWOULDBLOCK)enqueue to send-list");
            /* Copy the partial buffer to be sent later */
            rtsp_buf = (rtsp_buf_t *)malloc(sizeof(rtsp_buf_t) + len);
            if (!rtsp_buf) {
                errmsg(&msgsym(RSCERR, RTSP), "failed malloc rtsp_buf");
                return (ERMI_MEM_ERROR);
            }
            rtsp_buf->len = len;
            memcpy(&(rtsp_buf->buf), buf, len);
            requeue(&rtsp_conn->send_list, rtsp_buf);
            rtsp_conn->out_size = 0;
            return (ERMI_RTSP_SOCK_BLOCKED);
        }
        /* part of it sent through, try more */
        buf += bytes_sent;
        len = len - bytes_sent;
        ERMI_C_RTSP_SOCK_BUGINF("\n(remaining:%d) (bytes-sent:%d)",
                             len, bytes_sent);
    }
    ERMI_C_RTSP_SOCK_BUGINF("\n(socket:%d) (bytes-sent:%d)",
                             fd, len);
    rtsp_conn->out_size = 0;
    return (ERMI_OK);
}    

/*
 * send rtsp msg through socket if not backlogged, otherwise, queue it.
 */
ermi_status rtsp_sock_send (rtsp_conn_t *rtsp_conn, char* buf, int len)
{
    rtsp_buf_t *rtsp_buf;

    assert(rtsp_conn != NULL);
    assert(buf != NULL); 
    assert(len > 0);

    /* if we are not ready or backlogged, queue the buffers */
    if (!QUEUEEMPTY(&rtsp_conn->send_list) || 
         rtsp_conn->conn->state == SOCK_CONNECT_PENDING) {
        rtsp_buf = (rtsp_buf_t *)malloc(sizeof(rtsp_buf_t) + len);
        if (!rtsp_buf) {
            errmsg(&msgsym(RSCERR, RTSP), "failed malloc rtsp_buf");
            return (ERMI_MEM_ERROR);
        }
        rtsp_buf->len = len;
        memcpy(&(rtsp_buf->buf), buf, len);
        enqueue(&rtsp_conn->send_list, rtsp_buf);
        ERMI_C_RTSP_SOCK_BUGINF("\nenqueue to send_list, (len: %d)", len);
        return (ERMI_OK);
    }

    return rtsp_sock_xmit(rtsp_conn, buf, len);
}

/* the socket is ready to write, start the sending what's 
   left from last time
*/
ermi_status rtsp_handle_sock_write (rtsp_conn_t *rtsp_conn)
{
    ermi_status status;
    rtsp_buf_t *rtsp_buf;

    /* SOCKET_WRITE_EVENT, but we have nothing to send */
    
    if (rtsp_conn->send_list.count == 0) {
        ERMI_C_RTSP_SOCK_BUGINF("\nSOCKET_WRITE_EVENT fd %d, nothing to send",
                              rtsp_conn->conn->fd);
        rtsp_sock_clear(rtsp_conn->conn->fd, SOCKET_WRITE_EVENT);
        return (ERMI_OK);
    }

    /* send the queued data until all data is out or 
       the socket is blocked */
    while (!QUEUEEMPTY(&rtsp_conn->send_list)) {
        rtsp_buf = dequeue(&rtsp_conn->send_list);
        status = rtsp_sock_xmit(rtsp_conn, (char *)&rtsp_buf->buf, 
                                rtsp_buf->len);
	if (status == ERMI_RTSP_SOCK_BLOCKED) {
	    /* blocked again, no more sending */  
            free(rtsp_buf);
            return (ERMI_OK);
	}
        free(rtsp_buf);
    }
    
    return(ERMI_OK);
}

/* read rtsp msg from socket */
ermi_status rtsp_sock_read (rtsp_conn_t *rtsp_conn)
{
    int nbytes = 0;
    int fd;
    conn_type_e conn_type;
    conn_state_e conn_state;
    

    fd = rtsp_conn->conn->fd;
    conn_state = rtsp_conn->conn->state;
    conn_type = rtsp_conn->conn->type;

    /* always read one byte less to leave space for nul terminator */
    nbytes = socket_recv(fd, &rtsp_conn->in_buf[rtsp_conn->in_size],
                         RTSP_IN_BUF_SIZE - rtsp_conn->in_size - 1, 0);

    if (nbytes < 0) {
        if (errno != EWOULDBLOCK) {
            errmsg(&msgsym(SWERR, RTSP), "socket_recv failed with errno",
                   errno, "");
            return (ERMI_RTSP_SOCK_ERROR);
        } else {        /* errno == EWOULDBLOCK */
            ERMI_C_RTSP_SOCK_BUGINF("\n%s: read from fd %d would block",
                         __FUNCTION__, fd);
            /* this is not an error */
            return (ERMI_OK);
        }
    } else if (nbytes > 0) {
        ERMI_C_RTSP_SOCK_BUGINF("\n%s: read from fd %d nbytes = %d",
                         __FUNCTION__, fd, nbytes);
        ERMI_C_RTSP_SOCK_BUGINF("\n in_text = %s",
                               &rtsp_conn->in_buf[rtsp_conn->in_size]);
        rtsp_conn->in_size += nbytes;
        return (ERMI_OK);
    } else {    /* nbytes = 0 */
        /* EOF received; remote closed the connection, so cleanup  */
        ERMI_C_RTSP_SOCK_BUGINF("\n%s: Socket Received nothing. NBYTES=%d",
                          __FUNCTION__, nbytes);
        return (ERMI_RTSP_SOCK_ERROR);
    }
}


/* client tried to connect to remote RTSP server and timed out
   client should delete all message queued to be sent
*/
void rtsp_handle_sock_conn_timeout (mgd_timer *timer, void *context)
{
    rtsp_conn_t *rtsp_conn = (rtsp_conn_t *)context;
    ermi_c_conn_t *conn = rtsp_conn->conn;

    rtsp_close_conn(rtsp_conn);
    socket_close(conn->fd);
    memset(conn, 0, sizeof(struct ermi_c_conn_));      
}

/* when a socket is connected, both 
   SOCKET_READ_EVENT and SOCKET_WRITE_EVENT are generated
*/
void rtsp_sock_clear (int fd, ulong event)
{
    socktype *sock;

    sock = getsock_by_fd(fd);
    socket_clear_event(sock, event);
}
