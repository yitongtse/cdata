/*
 *-------------------------------------------------------------------
 * exec_show_rtsp.h - RTSP  show CLI module 
 *
 * 03/27/04, Xiaomei Liu
 *
 * Copyright (c) 2004-2009 by cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------------------
 */

/* client */
EOLS(rtsp_show_client_eol, rtsp_show_command, RTSP_SHOW_CLIENT);
NUMBER(rtsp_show_client_port, rtsp_show_client_eol, no_alt,
        OBJ(int, 2), 0, 0xffff, "rtsp client port number");
IPADDR(rtsp_show_client_ip, rtsp_show_client_port, no_alt,
        OBJ(paddr, 1), "destination ip address");
KEYWORD(rtsp_show_client, rtsp_show_client_ip, no_alt,
        "client", "show client information", PRIV_CONF);

/* server -> message -> stats */
EOLS(rtsp_show_server_msg_stats_eol, rtsp_show_command,
     RTSP_SHOW_SERVER_MSG_STATS);
KEYWORD(rtsp_show_server_msg_stats, rtsp_show_server_msg_stats_eol, no_alt,
        "stats", "show rtsp server message information", PRIV_CONF);

/* server -> message */
KEYWORD(rtsp_show_server_msg, rtsp_show_server_msg_stats, no_alt,
        "message", "show rtsp server message information", PRIV_CONF);

/* server -> port */
EOLS(rtsp_show_server_eol, rtsp_show_command, RTSP_SHOW_SERVER);
NUMBER(rtsp_show_server_port, rtsp_show_server_eol, rtsp_show_server_msg,
        OBJ(int, 1), 0, 0xffff, "rtsp server port number");

/* server */
KEYWORD(rtsp_show_server, rtsp_show_server_port, rtsp_show_client,
        "server", "show rtsp server information", PRIV_CONF);
KEYWORD(rtsp_show_top, rtsp_show_server, ALTERNATE,
        "rtsp", "show rtsp information", PRIV_CONF);

#undef ALTERNATE
#define ALTERNATE     rtsp_show_top 
