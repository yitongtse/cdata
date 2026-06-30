/*
 *-------------------------------------------------------------------
 * ermi_c_rtsp_sock.h - File to define various ermi_c_rtsp_sock.c related
 *                     structs, macros and functions.
 *
 * July 2004, Linda Hua: July-2008, Neeraj Khurana
 *
 * Copyright (c) 2004-2009 by cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------------------
 */
/*
 * RTSP (Real Time Streaming Protocol) socket interfaces
 *
 */

#ifndef _ERMI_C_RTSP_SOCK_H_
#define _ERMI_C_RTSP_SOCK_H_

/* handle socket partial sends and sending failure due 
   to blocking */
typedef struct send_buf_ {
    struct send_buf_ *next;
    int    len;
    char   *buf;
} send_buf_t;

extern list_header rtsp_peer_list;

/* open up a listening socket */
int rtsp_svr_sock_open(ipaddrtype peer_addr, uint16 port);

/* accept the connect request from a client */
int rtsp_svr_sock_accept (int listen_fd);

/* connect to the remote server to well-known port 
   dest_addr: server's IP addr
   dest_port: the well-known port
   return: the fd, INVALID_FD if failed 
*/

/* client tried to connect to remote RTSP server and timed out
   client should delete all message queued to be sent
*/
void rtsp_handle_sock_conn_timeout(mgd_timer *timer, void *rtsp_conn);

/* the socket was blocked for writing, now, it's ready for write, 
   start sending what's left from last time.
*/
ermi_status rtsp_handle_sock_write(rtsp_conn_t *rtsp_conn);

/* send rtsp msg through socket if not backlogged, 
   otherwise, queue it to rtsp_conn->send_list
   rtsp_conn: the rtsp connection
   buf: data to be sent
   len: how many bytes
   return: ERMI_OK if successful
           ERMI_RTSP_SOCK_BLOCKED if socked is blocked.  
*/
ermi_status rtsp_sock_send(rtsp_conn_t *rtsp_conn, char* buf, int len);

/* read data from the socket, then hand it to parser and app
*/
ermi_status rtsp_sock_read(rtsp_conn_t * rtsp_conn);

/* when a socket is connected, both 
   SOCKET_READ_EVENT and SOCKET_WRITE_EVENT are generated
*/
void rtsp_sock_clear(int fd, ulong event);

#endif /* _ERMI_C_RTSP_SOCK_H_ */


