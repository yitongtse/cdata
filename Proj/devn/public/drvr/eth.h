/*
 * Copyright 2003, QNX Software Systems Ltd. All Rights Reserved.
 *
 * This source code may contain confidential information of QNX Software
 * Systems Ltd.  (QSSL) and its licensors. Any use, reproduction,
 * modification, disclosure, distribution or transfer of this software,
 * or any software which includes or is based upon any of this code, is
 * prohibited unless expressly authorized by QSSL by written agreement. For
 * more information (including whether this source code file has been
 * published) please email licensing@qnx.com.
 */




#ifndef NIC_ETH_H_INCLUDED
#define NIC_ETH_H_INCLUDED

#define ETH_MAC_LEN			6				/* 6 bytes dst & src addrs */
#define ETH_MIN_DATA_LEN  	46				/* min = 64 - addrs - crc */
#define ETH_MAX_DATA_LEN	1500			/* max 1500 bytes data */
#define ETH_MIN_PKT_LEN		60				/* min bytes + crc=4 => 64 */
#define ETH_MAX_PKT_LEN		1514			/* max = 1500 + addrs + crc */
#define ETH_CRC_LEN			4				/* 4 byte crc append by 8390 */

#define ETH_MCAST_MAX_ADDR  64              /* Max mcast address to track */
#define ETH_MCAST_CRC_MASK  0x3F

typedef struct _ether_header ether_header_t;
struct _ether_header {
	unsigned char   dst_addr[ETH_MAC_LEN];
	unsigned char   src_addr[ETH_MAC_LEN];
	unsigned short  ether_type;
};

#endif
