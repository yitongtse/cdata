/*
 *------------------------------------------------------------------
 * ermi_r_errp_fsm.c
 *
 * Finite state machine for ERMI-1 protocol
 *
 * June 2008, Dean Chen
 *
 * Copyright (c) 2008 by cisco Systems, Inc.
 * All rights reserved.
 *
 *------------------------------------------------------------------
 */


#include COMP_INC(kernel, list.h)
#include COMP_INC(kernel, mgd_timers.h)
#include COMP_INC(idb, interface.h)
#include IOS_INC(util/fsm.h)
#include IOS_INC(h/logger.h)
#include "ermi_r_protocol_def.h"
#include "ermi_r_errp_fsm.h"
#include "ermi_r_io.h"

/* 
 * ERMI-1 FSM
 *
 * Events:
 *   E1  - E_START
 *   E2  - E_STOP
 *   E3  - E_CONN_OPEN
 *   E4  - E_CONN_CLOSED
 *   E5  - E_CONN_OPEN_FAILED
 *   E6  - E_CONN_FATAL
 *   E7  - E_CONN_RETRY_TIMER_EXP
 *   E8  - E_HOLD_TIMER_EXP
 *   E9  - E_KEEPALIVE_TIMER_EXP
 *   E10 - E_OPEN_RCVD
 *   E11 - E_KEEPALIVE_RCVD
 *   E12 - E_UPDATE_RCVD
 *   E13 - E_NOTIF_RCVD
 *   E14 - E_SEND_UPDATE
 *   E15 - E_MSG_ERROR
 *   
 * Actions:
 *   A1  - A_INIT_CONN
 *   A2  - A_CLOSE_CONN
 *   A3  - A_SEND_OPEN
 *   A4  - A_SEND_NOTIFICATION
 *   A5  - A_RETRY_TIMER
 *   A6  - A_RETRY_NOW
 *   A7  - A_SEND_KEEPALIVE
 *   A8  - A_RESTART_HOLDTIMER
 *   A9  - A_SEND_UPDATE
 *
 * States:
 *   S1  - S_IDLE
 *   S2  - S_CONNECT
 *   S3  - S_ACTIVE
 *   S4  - S_OPENSENT
 *   S5  - S_OPENCONFIRM
 *   S6  - S_ESTABLISHED
 *
 *   Notions:
 *     -  : (FSM_NOACTION, FSM_NOCHANGE), ignored.
 *     /  : (FSM_NOACTION, FSM_INVALID), invalid event in this state.
 *          will be logged an error when debug and ignored at runtime.
 *   ____________________________________
 *   |    | S1 | S2 | S3 | S4 | S5 | S6 |
 *   |====|====|====|====|====|=========|
 *   | E1 | A1 | -  | -  | -  | -  | -  |
 *   |    | S2 |    |    |    |    |    |
 *   |____|____|____|____|____|____|____|
 *   | E2 | -  | A2 | A2 | A4 | A4 | A4 |
 *   |    |    | S1 | S1 | S1 | S1 | S1 |
 *   |____|____|____|____|____|____|____|
 *   | E3 | -  | A3 | A3 | A4 | A4 | A4 |
 *   |    |    | S4 | S4 | S1 | S1 | S1 |
 *   |____|____|____|____|____|____|____|
 *   | E4 | -  | A2 | A2 | A5 | A4 | A4 |
 *   |    |    | S1 | S1 | S3 | S1 | S1 |
 *   |____|____|____|____|____|____|____|
 *   | E5 | -  | A5 | -  | A4 | A4 | A4 |
 *   |    |    | S3 |    | S1 | S1 | S1 |
 *   |____|____|____|____|____|____|____|
 *   | E6 | -  | A2 | A2 | A2 | A5 | A2 |
 *   |    |    | S1 | S1 | S1 | S3 | S1 |
 *   |____|____|____|____|____|____|____|
 *   | E7 | -  | A6 | A6 | A4 | A4 | A4 |
 *   |    |    | S2 | S2 | S1 | S1 | S1 |
 *   |____|____|____|____|____|____|____|
 *   | E8 | -  | A2 | A2 | A4 | A4 | A4 |
 *   |    |    | S1 | S1 | S1 | S1 | S1 |
 *   |____|____|____|____|____|____|____|
 *   | E9 | -  | A2 | A2 | A4 | A7 | A7 |
 *   |    |    | S1 | S1 | S1 | S5 | S6 |
 *   |____|____|____|____|____|____|____|
 *   | E10| -  | A2 | A2 | A7 | A4 | A4 |
 *   |    |    | S1 | S1 | S5 | S1 | S1 |
 *   |____|____|____|____|____|____|____|
 *   | E11| -  | A2 | A2 | A4 |NOP | A8 |
 *   |    |    | S1 | S1 | S1 | S6 | S6 |
 *   |____|____|____|____|____|____|____|
 *   | E12| -  | A2 | A2 | A4 | A4 | A8 |
 *   |    |    | S1 | S1 | S1 | S1 | S6 |
 *   |____|____|____|____|____|____|____|
 *   | E13| -  | A2 | A2 | A2 | A2 | A2 |
 *   |    |    | S1 | S1 | S1 | S1 | S1 |
 *   |____|____|____|____|____|____|____|
 *   | E14| -  | -  | -  | -  | -  | A9 |
 *   |    |    |    |    |    |    | S6 |
 *   |____|____|____|____|____|____|____|
 *   | E15| -  | A2 | A2 | A4 | A4 | A4 |
 *   |    |    | S1 | S1 | S1 | S1 | S1 |
 *   |____|____|____|____|____|____|____|
 */ 


/* FSM decoder routine forward declaration */
static int32
ermi_r_fsm_decode(void *event_desc, int32 major, int32 minor);
/*
 * --------------------------------------------------------------------------
 *  FSM DECODER ROUTINE(s)
 * --------------------------------------------------------------------------
 */
#define ERMI_R_FSM_DEC FSM_DECODE_ROUTINE(ermi_r_fsm_decode)

/* FSM action routines forward declarations */
static uint16 a_init_conn (errp_fsm_event_desc_t *event_desc, 
                           void const *param);
static uint16 a_close_conn (errp_fsm_event_desc_t *event_desc, 
                            void const *param);
static uint16 a_send_open (errp_fsm_event_desc_t *event_desc, void const *param);
static uint16 a_send_notification (errp_fsm_event_desc_t *event_desc, void const *param);
static uint16 a_retry_timer (errp_fsm_event_desc_t *event_desc, void const *param);
static uint16 a_retry_now (errp_fsm_event_desc_t *event_desc, void const *param);
static uint16 a_send_keepalive (errp_fsm_event_desc_t *event_desc, 
                                void const *param);
static uint16 a_restart_holdtimer (errp_fsm_event_desc_t *event_desc, void const *param);
static uint16 a_send_update (errp_fsm_event_desc_t *event_desc, 
                             void const *param);
/*
 * --------------------------------------------------------------------------
 *  FSM ACTION ROUTINES
 * --------------------------------------------------------------------------
 */
#define A_INIT_CONN           FSM_ACTION_ROUTINE(a_init_conn)  
#define A_CLOSE_CONN          FSM_ACTION_ROUTINE(a_close_conn)     
#define A_SEND_OPEN           FSM_ACTION_ROUTINE(a_send_open)      
#define A_SEND_NOTIFICATION   FSM_ACTION_ROUTINE(a_send_notification)     
#define A_RETRY_TIMER         FSM_ACTION_ROUTINE(a_retry_timer)      
#define A_RETRY_NOW              FSM_ACTION_ROUTINE(a_retry_now)      
#define A_SEND_KEEPALIVE      FSM_ACTION_ROUTINE(a_send_keepalive)
#define A_RESTART_HOLDTIMER   FSM_ACTION_ROUTINE(a_restart_holdtimer)
#define A_SEND_UPDATE         FSM_ACTION_ROUTINE(a_send_update)          

/*
 * --------------------------------------------------------------------------
 *  STATE ENTRY of FSM TABLE
 * -------------------------------------------------------------------------- 
 */

/* ENTRY for S1: S_IDLE STATE */
const static struct fsm_state ermi_r_idle_se[] = {
/*
 *------------------------------------------------------------------------------
 * Action               Argument   Next State        Decoded Event
 *------------------------------------------------------------------------------
 */                                                                            
{A_INIT_CONN,           FSM_NOARG, S_CONNECT},       /* START */           
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* STOP */
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* CONN_OPEN */      
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* CONN_CLOSED */ 
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* CONN_OPEN_FAILED */        
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* CONN_FATAL*/      
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* CONN_RETRY_TIMEOUT */    
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* HOLD_TIMER_EXP */      
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* KEEPALIVE_TMR_EXP */ 
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* OPEN_RCVD */      
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* KEEPALIVE_RCVD */
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* UPDATE_RCVD */           
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* NOTIF_RCVD */      
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* SEND_UPDATE */ 
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* MSG_ERROR */ 
};

/* ENTRY for S2: S_CONNECT STATE */
const static struct fsm_state ermi_r_connect_se[] = {
/*
 *------------------------------------------------------------------------------
 * Action               Argument   Next State        Decoded Event
 *------------------------------------------------------------------------------
 */                                                                            
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* START */    
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* STOP */       
{A_SEND_OPEN,           FSM_NOARG, S_OPENSENT},      /* CONN_OPEN */      
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* CONN_CLOSED */ 
{A_RETRY_TIMER,         FSM_NOARG, S_ACTIVE},        /* CONN_OPEN_FAILED */ 
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* CONN_FATAL*/             
{A_RETRY_NOW,           FSM_NOARG, S_CONNECT},       /* CONN_RETRY_TIMEOUT */    
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* HOLD_TIMER_EXP */      
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* KEEPALIVE_TMR_EXP */ 
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* OPEN_RCVD */      
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* KEEPALIVE_RCVD */
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* UPDATE_RCVD */           
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* NOTIF_RCVD */      
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* SEND_UPDATE */ 
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* MSG_ERROR */      
};

/* ENTRY for S3: S_ACTIVE STATE */
const static struct fsm_state ermi_r_active_se[] = {
/*
 *------------------------------------------------------------------------------
 * Action               Argument   Next State        Decoded Event
 *------------------------------------------------------------------------------
 */                                                                            
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* START */           
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* STOP */
{A_SEND_OPEN,           FSM_NOARG, S_OPENSENT},      /* CONN_OPEN */      
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* CONN_CLOSED */ 
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* CONN_OPEN_FAILED */        
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* CONN_FATAL */      
{A_RETRY_NOW,           FSM_NOARG, S_CONNECT},       /* CONN_RETRY_TIMEOUT */    
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* HOLD_TIMER_EXP */      
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* KEEPALIVE_TMR_EXP */ 
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* OPEN_RCVD */      
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* KEEPALIVE_RCVD */
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* UPDATE_RCVD */           
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* NOTIF_RCVD */      
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* SEND_UPDATE */ 
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* MSG_ERROR */      
};

/* ENTRY for S4: S_OPENSENT STATE */
const static struct fsm_state ermi_r_opensent_se[] = {
/*
 *------------------------------------------------------------------------------
 * Action               Argument   Next State        Decoded Event
 *------------------------------------------------------------------------------
 */                                                                            
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* START */           
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* STOP, EC: "CEASE" */
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* CONN_OPEN, EC: "FSM Error" */      
{A_RETRY_TIMER,         FSM_NOARG, S_ACTIVE},        /* CONN_CLOSED */ 
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* CONN_OPEN_FAILED, EC: "FSM Error" */        
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* CONN_FATAL, EC: "FSM Error" */      
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* CONN_RETRY_TIMEOUT, EC: "FSM Error" */    
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* HOLD_TIMER_EXP, EC: "Hold Timer Expired" */      
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* KEEPALIVE_TMR_EXP, EC: "FSM Error" */ 
{A_SEND_KEEPALIVE,      FSM_NOARG, S_OPENCONFIRM},   /* OPEN_RCVD */      
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* KEEPALIVE_RCVD, EC: "FSM Error" */
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* UPDATE_RCVD, EC: "FSM Error" */           
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* NOTIF_RCVD */      
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* SEND_UPDATE */ 
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* MSG_ERROR */      
};

/* ENTRY for S5: S_OPENCONFIRM STATE */
const static struct fsm_state ermi_r_openconfirm_se[] = {
/*
 *------------------------------------------------------------------------------
 * Action               Argument   Next State        Decoded Event
 *------------------------------------------------------------------------------
 */                                                                            
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* START */           
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* STOP, EC: "CEASE" */
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* CONN_OPEN, EC: "FSM Error" */      
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* CONN_CLOSED, EC: "FSM Error" */ 
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* CONN_OPEN_FAILED, EC: "FSM Error" */        
{A_RETRY_TIMER,         FSM_NOARG, S_ACTIVE},        /* CONN_FATAL */      
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* CONN_RETRY_TIMEOUT, EC: "FSM Error" */    
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* HOLD_TIMER_EXP, EC: "Hold Timer Expired" */ 
{A_SEND_KEEPALIVE,      FSM_NOARG, S_OPENCONFIRM},   /* KEEPALIVE_TMR_EXP */      
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* OPEN_RCVD, EC: "FSM Error" */      
{FSM_NOACTION,          FSM_NOARG, S_ESTABLISHED},   /* KEEPALIVE_RCVD */
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* UPDATE_RCVD, EC: "FSM Error" */           
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* NOTIF_RCVD */      
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* SEND_UPDATE */ 
{A_SEND_NOTIFICATION,  FSM_NOARG, S_IDLE},          /* MSG_ERROR */      
};

/* ENTRY for S6: S_ESTABLISHED STATE */
const static struct fsm_state ermi_r_established_se[] = {
/*
 *------------------------------------------------------------------------------
 * Action               Argument   Next State        Decoded Event
 *------------------------------------------------------------------------------
 */                                                                            
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* START */           
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* STOP, EC: "CEASE" */
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* CONN_OPEN */      
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* CONN_CLOSED */ 
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* CONN_OPEN_FAILED */        
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* CONN_FATAL */      
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* CONN_RETRY_TIMEOUT */    
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* HOLD_TIMER_EXP */      
{A_SEND_KEEPALIVE,      FSM_NOARG, S_ESTABLISHED},   /* KEEPALIVE_TMR_EXP */ 
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* OPEN_RCVD */      
{A_RESTART_HOLDTIMER,   FSM_NOARG, S_ESTABLISHED},   /* KEEPALIVE_RCVD */
{A_RESTART_HOLDTIMER,   FSM_NOARG, S_ESTABLISHED},   /* UPDATE_RCVD */           
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* NOTIF_RCVD */      
{A_SEND_UPDATE,         FSM_NOARG, S_ESTABLISHED},   /* SEND_UPDATE */ 
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* MSG_ERROR */      
};

/*
 * --------------------------------------------------------------------------
 *  STATE MACHINE TABLE
 * --------------------------------------------------------------------------
 */ 
const static fsm_t ermi_r_fsm_table[] = {
/*--------------------------------------------------------------------
 * State                 Decoder      
 *--------------------------------------------------------------------*/
{ermi_r_idle_se,          ERMI_R_FSM_DEC},     
{ermi_r_connect_se,       ERMI_R_FSM_DEC}, 
{ermi_r_active_se,        ERMI_R_FSM_DEC},
{ermi_r_opensent_se,      ERMI_R_FSM_DEC},
{ermi_r_openconfirm_se,   ERMI_R_FSM_DEC},
{ermi_r_established_se,   ERMI_R_FSM_DEC},
};

/* utilities */

int8 *
ermi_r_fsm_state_str (int32 state)
{
    switch (state) {
    case S_IDLE:          return("IDLE");
    case S_CONNECT:       return("CONNECT");
    case S_ACTIVE:        return("ACTIVE");
    case S_OPENSENT:      return("OPEN SENT");
    case S_OPENCONFIRM:   return("OPEN CONFIRM");
    case S_ESTABLISHED:   return("ESTABLISHED");
    default:              return("UNKNOWN");
    }
}

int8 *
ermi_r_fsm_event_str (int32 event)
{
    switch (event) {
    case E_START:                 return("START");
    case E_CONN_FATAL:            return("CONNECTION FATAL ERROR");
    case E_CONN_OPEN:             return("CONNECTION OPENED");
    case E_CONN_OPEN_FAILED:      return("CONNECTION OPEN FAILED");
    case E_CONN_RETRY_TIMER_EXP:  return("CONNECTION RETRY TIMER EXPIRED");
    case E_CONN_CLOSED:           return("CONNECTION CLOSED");
    case E_OPEN_RCVD:             return("OPEN RECEIVED");
    case E_HOLD_TIMER_EXP:        return("HOLD TIMER EXPIRED");
    case E_STOP:                  return("STOP");
    case E_KEEPALIVE_RCVD:        return("KEEPALIVE RECEIVED");
    case E_NOTIF_RCVD:            return("NOTIFICATION RECEIVED");
    case E_KEEPALIVE_TIMER_EXP:   return("KEEPALIVE TIMER EXPIRED");
    case E_UPDATE_RCVD:           return("UPDATE RECEIVED");
    case E_SEND_UPDATE:           return("SEND UPDATE");
    case E_MSG_ERROR:             return("MESSAGE PARSING ERROR");
    default:                      return("UNKNOWN");    
    }
}

/* return the corresponding FSM table, given the FSM type */
static fsm_t *
get_ermi_r_fsm_table (errp_fsm_type fsm_type) 
{
    switch (fsm_type) {
        case SENDER_FSM:
            return ((fsm_t *)ermi_r_fsm_table);
            break;
        default:
            return ((fsm_t *)ermi_r_fsm_table);
            break;            
    }
}
/* decode function */
static int32
ermi_r_fsm_decode (void *event_arg, int32 major, int32 minor) 
{
    return (major);
}

/* action functions */

/*
 * start connect_retry timer (120 seconds)
 * start/listen to TCP connection 
 */
static uint16 
a_init_conn (errp_fsm_event_desc_t *event_desc, void const *param)
{
    ermi_r_neighbor_t *nbr;

    nbr = event_desc->nbr;

    if (nbr->connected) {
        /* transport already established, e.g. from unsolicited I/O */
        return 0;
    } else {
        if (nbr->io_connect_func) {
            nbr->io_connect_func(nbr);
            if (nbr->timer_start_func) {
                nbr->timer_start_func(nbr, ERRP_CONNECT_TIMER_TYPE,
                                      nbr->connect_time);
            }
        }
        return 0;
    }
}

static uint16 
a_close_conn (errp_fsm_event_desc_t *event_desc, void const *param)
{
    ermi_r_neighbor_t *nbr;

    nbr = event_desc->nbr;
    
    if (nbr->connected) {
        if (nbr->io_disconnect_func) {
            nbr->io_disconnect_func(nbr);
        }
    }
    return 0;
}

static uint16 
a_send_open (errp_fsm_event_desc_t *event_desc, void const *param)
{
    uint i, len = 0;
    ermi_r_neighbor_t *nbr;

    nbr = event_desc->nbr;
    if (errp_format_open(nbr, nbr->workspace, &len)) {
        if (nbr->io_send_func) {
            nbr->io_send_func(nbr, nbr->workspace, len);
            for (i = 0; i < len; i++) {
                ERMI_R_DEBUG("0x%x ", nbr->workspace[i]);
            }        
        }
    }

    /* stop connect retry timer */
    if (nbr->timer_stop_func) {
        nbr->timer_stop_func(nbr, ERRP_CONNECT_TIMER_TYPE);
    }   
    return len;
}

static uint16 
a_send_notification (errp_fsm_event_desc_t *event_desc, void const *param)
{
    // TBD
#if 0
    uint i, len = 0;
    ermi_r_neighbor_t *nbr;

    nbr = event_desc->nbr;
    if (errp_format_notification(nbr, nbr->workspace, &len)) {
        if (nbr->io_send_func) {
            nbr->io_send_func(nbr, nbr->workspace, len);
            for (i = 0; i < len; i++) {
                ERMI_R_DEBUG("0x%x ", nbr->workspace[i]);
            }        
            return len;
        }
    } 
#endif
    return 0;
}

static uint16 
a_retry_timer (errp_fsm_event_desc_t *event_desc, void const *param)
{
    ermi_r_neighbor_t *nbr;

    nbr = event_desc->nbr;

    if (nbr->timer_start_func) {
        nbr->timer_start_func(nbr, ERRP_CONNECT_TIMER_TYPE,
                              nbr->connect_time);
    }
    return 0;
}

static uint16 
a_retry_now (errp_fsm_event_desc_t *event_desc, void const *param)
{
    ermi_r_neighbor_t *nbr;

    nbr = event_desc->nbr;

    if (nbr->io_disconnect_func) {
        nbr->io_disconnect_func(nbr);
    }
    if (nbr->io_connect_func) {
        nbr->io_connect_func(nbr);
        if (nbr->timer_start_func) {
            nbr->timer_start_func(nbr, ERRP_CONNECT_TIMER_TYPE,
                                  nbr->connect_time);
        }
    }
    return 0;
}


static uint16 
a_send_keepalive (errp_fsm_event_desc_t *event_desc, 
                  void const *param)
{
    uint i, len = 0;
    ermi_r_neighbor_t *nbr;

    nbr = event_desc->nbr;
    if (errp_format_keepalive(nbr, nbr->workspace, &len)) {
        nbr->io_send_func(nbr, nbr->workspace, len);
        for (i = 0; i < len; i++) {
            ERMI_R_DEBUG("0x%x ", nbr->workspace[i]);
        }  

        if (nbr->timer_stop_func && nbr->timer_start_func) {
            nbr->timer_stop_func(nbr, ERRP_KEEPALIVE_TIMER_TYPE);
            nbr->timer_start_func(nbr, ERRP_KEEPALIVE_TIMER_TYPE,
                                  nbr->keep_alive_time);
        }
        return len;
    } 
    return 0;
}

static uint16 
a_restart_holdtimer (errp_fsm_event_desc_t *event_desc, void const *param)
{
    ermi_r_neighbor_t *nbr;

    nbr = event_desc->nbr;
    if (nbr->timer_stop_func && nbr->timer_start_func) {
        nbr->timer_stop_func(nbr, ERRP_HOLD_TIMER_TYPE);
        nbr->timer_start_func(nbr, ERRP_HOLD_TIMER_TYPE,
                              nbr->hold_time);
    }   

    return 0;
}

static uint16 a_send_update (errp_fsm_event_desc_t *event_desc, 
                             void const *param)
{
    // TBD
    /* send update */
    /* restart keepalive timer */
    return 0;
}

void
ermi_r_event (ermi_r_neighbor_t *nbr, uint32 event, void *param)
{
    errp_fsm_event_desc_t event_desc;

    event_desc.event = event;
    event_desc.nbr = nbr;
    
    ERMI_R_DEBUG("ERMI-1 driving %s event into %s state\n",
                ermi_r_fsm_event_str(event),
                ermi_r_fsm_state_str(nbr->fsm_state));
    ermi_r_fsm_exec(event_desc);
}

void
ermi_r_fsm_exec (errp_fsm_event_desc_t event_desc)
{
    uint32 event = 0;
    short rc;
    ermi_r_neighbor_t *nbr;

    event = event_desc.event;
    nbr = event_desc.nbr;
    rc = fsm_execute("ERMI_R_FSM", 
                     nbr->fsm_table,
                     (int *)&((event_desc.nbr)->fsm_state),
                     S_MAX,
                     &event_desc,
                     event,
                     0,
                     TRUE);
    ERMI_R_DEBUG("ERMI-1 processed %s event, current state %s\n",
                ermi_r_fsm_event_str(event),
                ermi_r_fsm_state_str(nbr->fsm_state));
}

void
ermi_r_fsm_init (ermi_r_neighbor_t *nbr, uint32 type)
{
    nbr->fsm_type = type;
    nbr->fsm_state = S_IDLE;
    nbr->fsm_table = get_ermi_r_fsm_table(type);
}
