/*
 *------------------------------------------------------------------
 * ermi_r_main.h
 *
 * Header file for main IOS process handling an ERMI-1 connection
 *
 * Dec 2008, Dean Chen
 *
 * Copyright (c) 2008 by cisco Systems, Inc.
 * All rights reserved.
 *
 *------------------------------------------------------------------
 */
enum {
  ERMI_R_MAIN_START,
  ERMI_R_MAIN_STOP,
  ERMI_R_MAIN_RESTART,
  ERMI_R_MAIN_CONNECT,
  ERMI_R_MAIN_DISCONNECT,

  ERMI_R_MAIN_SEND_OPEN,
  ERMI_R_MAIN_SEND_UPDATE,
  ERMI_R_MAIN_SEND_NOTIFICATION,
  ERMI_R_MAIN_SEND_KEEPALIVE,
};

extern process ermi_r_main_process(void);
extern ermi_r_neighbor_t * ermi_r_create_nbr(ipaddrtype addr, boolean server);
extern void ermi_r_parse_dispatch_msg(ermi_r_neighbor_t *nbr, uchar *buf, uint len);
extern void ermi_r_fsm_init(ermi_r_neighbor_t *nbr, uint32 type);
