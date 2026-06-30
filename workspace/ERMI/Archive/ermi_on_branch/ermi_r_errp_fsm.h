/*
 *------------------------------------------------------------------
 * ermi_r_errp_fsm.h
 *
 * Finite state machine definitions for ERMI-1 protocol
 *
 * June 2008, Dean Chen
 *
 * Copyright (c) 2008 by cisco Systems, Inc.
 * All rights reserved.
 *
 *------------------------------------------------------------------
 */

/* 
 * --------------------------------------------------------------------------
 *  STATES
 *  Note that the enum starts at 1. The fsm_execute() routine assumes that 
 *  all module states are normalized to 1.
 * --------------------------------------------------------------------------
 */
typedef enum {
    S_IDLE = 1,
    S_CONNECT,
    S_ACTIVE,
    S_OPENSENT,
    S_OPENCONFIRM,
    S_ESTABLISHED,
    S_MAX
} errp_state;

typedef enum {
    E_START,                 /* cli; cmd to connect */
    E_STOP,                  /* cli; cmd to disconnect */
    E_CONN_OPEN,             /* socket event; tcp connection open */
    E_CONN_CLOSED,           /* socket close/event; tcp connection close */
    E_CONN_OPEN_FAILED,      /* socket open; open connection failed */
    E_CONN_FATAL,            /* socket event; tcp fatal error */
    E_CONN_RETRY_TIMER_EXP,  /* timer event; connection retry timer exp */
    E_HOLD_TIMER_EXP,        /* timer event; hold timer exp */
    E_KEEPALIVE_TIMER_EXP,   /* timer event; keepalive timer exp */
    E_OPEN_RCVD,             /* socket event; parsed open msg */
    E_KEEPALIVE_RCVD,        /* socket event; parsed keepalive msg */
    E_UPDATE_RCVD,           /* socket event; parsed notification msg */
    E_NOTIF_RCVD,            /* socket event; parsed notification msg */
    E_SEND_UPDATE,           /* platform event; trigger update send */
    E_MSG_ERROR,             /* socket event; error parsing msg */
} errp_event;

typedef enum {
    SENDER_FSM = 1,
    SERVER_FSM,
} errp_fsm_type;

typedef struct errp_fsm_event_desc_ {
    uint32 slotindex;
    uint32 event;
    uint32 event_arg;
    ermi_r_neighbor_t *nbr;
} errp_fsm_event_desc_t;

typedef void (*errp_fsm_done_callback_t)(uint32 slotindex, 
                                         uint32 ret_code);

void ermi_r_fsm_exec(errp_fsm_event_desc_t event_desc);

extern boolean 
errp_format_open(ermi_r_neighbor_t *nbr, uchar *output, 
                 uint *len);
extern boolean 
errp_format_keepalive(ermi_r_neighbor_t *nbr, uchar *output, 
                      uint *len);
extern void ermi_r_event(ermi_r_neighbor_t *nbr, uint32 event, 
                        void *param);
