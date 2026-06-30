///  Copyright (c) 2011 by Cisco Systems, Inc.
///  All rights reserved.
///
///  @file      os_util.h
///  @brief     Header file for OS-Dependent utilities
///  @author    Yi Tong Tse


#ifndef __OS_UTIL_H__
#define __OS_UTIL_H__


#include <sys/socket.h>


/// Open Connection
/// @param addr   socket address to be connected to
/// @return       socket descriptor if connetion is successfully made,
///               otherwise -1
int io_connect(struct sockaddr *addr); 


/// Close Connection
/// @param fd   socket descriptor of connection to disconnect
void io_disconnect(int fd);


/// Send Message
/// @param fd        socket descriptor
/// @param buf       message buffer pointer
/// @param msg_size  message size in bytes
/// @return     number of bytes sent, or 0 if error
int io_send(int fd, uint8 *buf, int msg_size);


/// Receive Message
/// @param fd           socket descriptor
/// @param buf          message buffer pointer
/// @param buf_size     message buffer size
/// @return             number of bytes received
///   - number of bytes reeived
///   - 0 if connection is closed
///   - negative of errno for other errors
int io_recv(int fd, uint8 *buf, int buf_size);


/// Start a one-shot timer
/// @param timer    timer ID
/// @param timeout  timeout period in second
/// @return  0 if successful, or other error code
int timer_start(TIMER_T timer, uint32 timeout);


/// Stop a one-shot timer
/// @param timer        timer ID
/// @return  0 if successful, or other error code
int timer_stop(TIMER_T timer);


/// Wait for semaphore
/// @param sema         Semaphore for blocking
/// @note This is a blocking call
void sema_wait(SEMA_T *sema);


/// Post to semaphore
/// @param sema         Semaphore to post
void sema_post(SEMA_T *sema);


#endif  // __OS_UTIL_H__
