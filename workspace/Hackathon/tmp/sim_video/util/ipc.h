/*
 * Copyright (c) 2005-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#ifndef __IPC_H__
#define __IPC_H__


// Message_header->flags bit definitons
#define IPC_FLAG_BOOTSTRAP     0x0001   /* This is a bootstrap message */
#define IPC_FLAG_ACK           0x0002   /* This is an ACK message */
#define IPC_FLAG_IS_RPC        0x0004   /* This is an RPC request. */
#define IPC_FLAG_IS_RPC_REPLY  0x0008   /* This is an RPC reply. */
#define IPC_FLAG_IS_FRAGMENT   0x0010   /* This is a fragmented message */
#define IPC_FLAG_LAST_FRAGMENT 0x0020   /* Last fragment */
#define IPC_FLAG_DO_NOT_ACK    0x0040   /* Do not ack this message */
#define IPC_FLAG_SEQ_NO_ACK    0x0080   /* Check sequence numbers, but do not
                                           ack */
#define IPC_FLAG_NACK          0x0100   /* This is a NACK message */
#define IPC_FLAG_CONTROL       0x0200   /* This is a control message */
#define IPC_FLAG_RELIABLE      0x0400   /* This is a RELIABLE IPC message */
#define IPC_FLAG_SEQ_SYNC      0x0800   /* Sync seq # in seq window */
#define IPC_FLAG_ISSUMSG       0x2000   /* This is an ISSU negotiation message*/
#define IPC_FLAG_MULTICAST     0x4000   /* This is a multicast message */
#define IPC_FLAG_MCAST_OOB     0x8000
#define IPC_FRAG_PAGE_MASK     0xff00


// IPC message (simulated)
typedef uintptr_t* ipc_message;


// IPC message header (simulated)
typedef struct ipc_message_header_ {
    uint16_t      type;
    uint16_t      size;
    uint32_t      flags;
} ipc_message_header;


typedef uint32_t ipc_error_code;


// ipc_msg should be the beginning or message buffer
#define IPC_HEADER(ipc_msg)     ((ipc_message_header*)ipc_msg)
#define IPC_DATA(ipc_msg)       (void *)(((uintptr_t)ipc_msg)    \
                                        + sizeof(ipc_message_header))

#endif // __IPC_H__
