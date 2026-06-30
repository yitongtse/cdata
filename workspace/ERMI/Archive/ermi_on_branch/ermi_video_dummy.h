/*
 * ermi_video_dummy.h - Dummy Function header file
 * Copyright (c) 2008 by cisco Systems, Inc.
 * All rights reserved.
 */

#ifndef __ERMI_VIDEO_DUMMY_H__
#define __ERMI_VIDEO_DUMMY_H__

typedef struct udp_port_map_ {
    uint16 udp_port;      /* UDP, NEVER put anything above this line */
    uint16 ip_interface;  /* from which ip (in emulation mode) */
    ipaddrtype dst_ip;    /* destination IP address */
    ipaddrtype src_ip;    /* source IP address */
    // in_session *session; /* matching session, set when a session starts */
    rfgw_qam_t *qam;      /* matching qam */
    list_header qam_prog_list; /* list of qam_prog */
    uint16 flags;         /* what kind of port it is? clone, mcast ...*/
} udp_port_map;

#endif __ERMI_VIDEO_DUMMY_H__
