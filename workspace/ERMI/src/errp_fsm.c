///  Copyright (c) 2008-2011 by Cisco Systems, Inc.
///  All rights reserved.
///
///  @file      errp_fsm.c
///  @brief     ERRP (ERMI-1) finite state machine
///  @author    Dean Chen, Yi Tong Tse


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "common.h"
#include "errp.h"
#include "fsm.h"
#include "errp_fsm.h"

/* 
 * ERRP FSM
 *
 * States:
 *   S1  - S_IDLE
 *   S2  - S_CONNECT
 *   S3  - S_ACTIVE
 *   S4  - S_OPENSENT
 *   S5  - S_OPENCONFIRM
 *   S6  - S_ESTABLISHED
 *
 * Events:
 *   E1  - E_START
 *   E2  - E_STOP
 *   E3  - E_CONN_OPENED
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
 *   A5  - A_RETRY_LATER
 *   A6  - A_RETRY_NOW
 *   A7  - A_SEND_KEEPALIVE
 *   A8  - A_RESTART_HOLDTIMER
 *   A9  - A_SEND_UPDATE
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
errp_fsm_decode(void *event_desc, int32 major, int32 minor);
/*
 * --------------------------------------------------------------------------
 *  FSM DECODER ROUTINE(s)
 * --------------------------------------------------------------------------
 */
#define ERRP_FSM_DEC FSM_DECODE_ROUTINE(errp_fsm_decode)

/* FSM action routines forward declarations */
static uint16 a_init_conn (errp_fsm_event_desc_t *event_desc, 
                           void const *param);
static uint16 a_close_conn (errp_fsm_event_desc_t *event_desc, 
                            void const *param);
static uint16 a_send_open (errp_fsm_event_desc_t *event_desc, void const *param);
static uint16 a_send_notification (errp_fsm_event_desc_t *event_desc, void const *param);
static uint16 a_retry_later (errp_fsm_event_desc_t *event_desc, void const *param);
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
#define A_RETRY_LATER         FSM_ACTION_ROUTINE(a_retry_later)      
#define A_RETRY_NOW           FSM_ACTION_ROUTINE(a_retry_now)      
#define A_SEND_KEEPALIVE      FSM_ACTION_ROUTINE(a_send_keepalive)
#define A_RESTART_HOLDTIMER   FSM_ACTION_ROUTINE(a_restart_holdtimer)
#define A_SEND_UPDATE         FSM_ACTION_ROUTINE(a_send_update)          

/*
 * --------------------------------------------------------------------------
 *  STATE ENTRY of FSM TABLE
 * -------------------------------------------------------------------------- 
 */

/* ENTRY for S1: S_IDLE STATE */
const static struct fsm_state errp_idle_se[] = {
/*
 *------------------------------------------------------------------------------
 * Action               Argument   Next State        Decoded Event
 *------------------------------------------------------------------------------
 */                                                                            
{A_INIT_CONN,           FSM_NOARG, S_CONNECT},       /* START */           
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* STOP */
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* CONN_OPENED */      
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
const static struct fsm_state errp_connect_se[] = {
/*
 *------------------------------------------------------------------------------
 * Action               Argument   Next State        Decoded Event
 *------------------------------------------------------------------------------
 */                                                                            
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* START */    
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* STOP */       
{A_SEND_OPEN,           FSM_NOARG, S_OPENSENT},      /* CONN_OPENED */      
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* CONN_CLOSED */ 
{A_RETRY_LATER,         FSM_NOARG, S_ACTIVE},        /* CONN_OPEN_FAILED */ 
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
const static struct fsm_state errp_active_se[] = {
/*
 *------------------------------------------------------------------------------
 * Action               Argument   Next State        Decoded Event
 *------------------------------------------------------------------------------
 */                                                                            
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* START */           
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* STOP */
{A_SEND_OPEN,           FSM_NOARG, S_OPENSENT},      /* CONN_OPENED */      
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
const static struct fsm_state errp_opensent_se[] = {
/*
 *------------------------------------------------------------------------------
 * Action               Argument   Next State        Decoded Event
 *------------------------------------------------------------------------------
 */                                                                            
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* START */           
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* STOP, EC: "CEASE" */
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* CONN_OPENED, EC: "FSM Error" */      
{A_RETRY_LATER,         FSM_NOARG, S_ACTIVE},        /* CONN_CLOSED */ 
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* CONN_OPEN_FAILED, EC: "FSM Error" */        
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* CONN_FATAL, EC: "FSM Error" */      
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* CONN_RETRY_TIMEOUT, EC: "FSM Error" */    
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* HOLD_TIMER_EXP, EC: "Hold Timer Expired" */      
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* KEEPALIVE_TMR_EXP, EC: "FSM Error" */ 
{A_SEND_KEEPALIVE,      (void*)TRUE, S_OPENCONFIRM}, /* OPEN_RCVD */      
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* KEEPALIVE_RCVD, EC: "FSM Error" */
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* UPDATE_RCVD, EC: "FSM Error" */           
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* NOTIF_RCVD */      
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* SEND_UPDATE */ 
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* MSG_ERROR */      
};

/* ENTRY for S5: S_OPENCONFIRM STATE */
const static struct fsm_state errp_openconfirm_se[] = {
/*
 *------------------------------------------------------------------------------
 * Action               Argument   Next State        Decoded Event
 *------------------------------------------------------------------------------
 */                                                                            
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* START */           
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* STOP, EC: "CEASE" */
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* CONN_OPENED, EC: "FSM Error" */      
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* CONN_CLOSED, EC: "FSM Error" */ 
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* CONN_OPEN_FAILED, EC: "FSM Error" */        
{A_RETRY_LATER,         FSM_NOARG, S_ACTIVE},        /* CONN_FATAL */      
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* CONN_RETRY_TIMEOUT, EC: "FSM Error" */    
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* HOLD_TIMER_EXP, EC: "Hold Timer Expired" */ 
{A_SEND_KEEPALIVE,      FSM_NOARG, S_OPENCONFIRM},   /* KEEPALIVE_TMR_EXP */      
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* OPEN_RCVD, EC: "FSM Error" */      
{FSM_NOACTION,          FSM_NOARG, S_ESTABLISHED},   /* KEEPALIVE_RCVD */
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* UPDATE_RCVD, EC: "FSM Error" */           
{A_CLOSE_CONN,          FSM_NOARG, S_IDLE},          /* NOTIF_RCVD */      
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* SEND_UPDATE */ 
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* MSG_ERROR */      
};

/* ENTRY for S6: S_ESTABLISHED STATE */
const static struct fsm_state errp_established_se[] = {
/*
 *------------------------------------------------------------------------------
 * Action               Argument   Next State        Decoded Event
 *------------------------------------------------------------------------------
 */                                                                            
{FSM_NOACTION,          FSM_NOARG, FSM_NOCHANGE},    /* START */           
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* STOP, EC: "CEASE" */
{A_SEND_NOTIFICATION,   FSM_NOARG, S_IDLE},          /* CONN_OPENED */      
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
const static fsm_t errp_fsm_table[] = {
/*--------------------------------------------------------------------
 * State                 Decoder      
 *--------------------------------------------------------------------*/
{errp_idle_se,          ERRP_FSM_DEC},     
{errp_connect_se,       ERRP_FSM_DEC}, 
{errp_active_se,        ERRP_FSM_DEC},
{errp_opensent_se,      ERRP_FSM_DEC},
{errp_openconfirm_se,   ERRP_FSM_DEC},
{errp_established_se,   ERRP_FSM_DEC},
};

/* utilities */

char* errp_fsm_state_txt (uint32 state)
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

char* errp_fsm_event_txt (uint32 event)
{
    switch (event) {
    case E_START:                 return("START");
    case E_CONN_FATAL:            return("CONNECTION FATAL ERROR");
    case E_CONN_OPENED:           return("CONNECTION OPENED");
    case E_CONN_OPEN_FAILED:      return("CONNECTION OPEN FAILED");
    case E_CONN_RETRY_TIMER_EXP:  return("CONNECT RETRY TIMER EXPIRED");
    case E_CONN_CLOSED:           return("CONNECTION CLOSED");
    case E_OPEN_RCVD:             return("OPEN RECEIVED");
    case E_HOLD_TIMER_EXP:        return("HOLD TIMER EXPIRED");
    case E_STOP:                  return("STOP");
    case E_KEEPALIVE_RCVD:        return("KEEPALIVE RECEIVED");
    case E_NOTIF_RCVD:            return("NOTIFICATION RECEIVED");
    case E_KEEPALIVE_TIMER_EXP:   return("KEEPALIVE TIMER EXPIRED");
    case E_UPDATE_RCVD:           return("UPDATE RECEIVED");
    case E_SEND_UPDATE:           return("SEND UPDATE");
    case E_MSG_ERROR:             return("MESSAGE ERROR");
    default:                      return("UNKNOWN");    
    }
}


/* decode function */
static int32
errp_fsm_decode (void *event_arg, int32 major, int32 minor) 
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
    errp_conn_t* conn = event_desc->conn;

    ERRP_FSM_LOG("Action: init_conn");

    /* Ignore if transport already established, e.g. from unsolicited I/O */
    if (conn->connected) {
        return 0;
    }

    conn->fd = io_connect((struct sockaddr *)&conn->addr);
    if (conn->fd != -1) {
        conn->connected = TRUE;
    }

    timer_start(conn->connect_timer_id, conn->connect_time);
    sema_post(&conn->conn_sema);

    return 0;
}


static uint16 
a_close_conn (errp_fsm_event_desc_t *event_desc, void const *param)
{
    ERRP_FSM_LOG("Action: close_conn");
    errp_conn_t* conn = event_desc->conn;
    
    if (!conn->connected) {
        return 0;
    }

    // Disconnect
    io_disconnect(conn->fd);
    conn->connected = FALSE;

    // Stop timers
    timer_stop(conn->connect_timer_id);
    timer_stop(conn->hold_timer_id);
    timer_stop(conn->keepalive_timer_id);

    return 0;
}


static uint16 
a_send_open (errp_fsm_event_desc_t *event_desc, void const *param)
{
    uint32 msg_len;
    char msg_buf[1024];
    char* ptr = msg_buf;
    errp_conn_t* conn = event_desc->conn;

    ERRP_FSM_LOG("Action: send_open");

    // Stop ConnectRetry timer
    timer_stop(conn->connect_timer_id);

    // Send OPEN message
    msg_len = errp_put_open_msg(&ptr, conn->node);
    io_send(conn->fd, msg_buf, msg_len);

    return 0;
}


static uint16
a_send_update (errp_fsm_event_desc_t *event_desc, void const *param)
{
    uint32 msg_len;
    char msg_buf[1024];
    char* ptr = msg_buf;
    errp_conn_t* conn = event_desc->conn;
    errp_resource_t* rsc = (errp_resource_t*)event_desc->event_arg;

    ERRP_FSM_LOG("Action: send_update");

    // Send UPDATE message
    msg_len = errp_put_update_msg(&ptr, rsc);
    io_send(conn->fd, msg_buf, msg_len);

    // Restart Keepalive timer
    timer_start(conn->keepalive_timer_id, conn->keepalive_time);

    return 0;
}


static void
errp_timer_setup (errp_conn_t *conn)
{
    // If either one does not want KEEPALIVE, turn it off
    if ((conn->node->hold_time & conn->peer->hold_time) == 0) {
        conn->hold_time = conn->keepalive_time = 0;
        printf("KEEPALIVE off\n");
        return;
    }

    // Pick larger hold time as negotiated value
    conn->hold_time = conn->node->hold_time;
    if (conn->peer->hold_time > conn->hold_time) {
        conn->hold_time = conn->peer->hold_time;
    }
    conn->keepalive_time = conn->hold_time / 3;

    printf("Hold time: node %d, peer %d -> %d;  Keepalive: %d\n",
           conn->node->hold_time, conn->peer->hold_time,
           conn->hold_time, conn->keepalive_time);
}


static uint16 
a_send_keepalive (errp_fsm_event_desc_t *event_desc, void const *param)
{
    uint32 msg_len;
    char msg_buf[4];
    char* ptr = msg_buf;
    errp_conn_t* conn = event_desc->conn;

    ERRP_FSM_LOG("Action: send_keepalive");

    if (param) {
        errp_timer_setup(conn);
    }

    msg_len = errp_put_keepalive_msg(&ptr);

    io_send(conn->fd, msg_buf, msg_len);

    timer_start(conn->keepalive_timer_id, conn->keepalive_time);

    return 0;
}


static uint16 
a_send_notification (errp_fsm_event_desc_t *event_desc, void const *param)
{
    uint32 msg_len;
    char msg_buf[ERRP_MAX_ERR_LEN];
    char* ptr = msg_buf;
    errp_conn_t* conn = event_desc->conn;
    const errp_error_report_t *err_rpt
                    = (errp_error_report_t*)event_desc->event_arg;

    ERRP_FSM_LOG("Action: send_notification");

    if (err_rpt) {
        msg_len = errp_put_notification_msg(&ptr, err_rpt);
        io_send(conn->fd, msg_buf, msg_len);
    }

    io_disconnect(conn->fd);
    conn->connected = FALSE;

    // Stop timers
    timer_stop(conn->connect_timer_id);
    timer_stop(conn->hold_timer_id);
    timer_stop(conn->keepalive_timer_id);

    return 0;
}


static uint16 
a_retry_later (errp_fsm_event_desc_t *event_desc, void const *param)
{
    errp_conn_t* conn = event_desc->conn;

    ERRP_FSM_LOG("Action: retry_later");

    timer_start(conn->connect_timer_id, conn->connect_time);
    return 0;
}


static uint16 
a_retry_now (errp_fsm_event_desc_t *event_desc, void const *param)
{
    errp_conn_t* conn = event_desc->conn;

    ERRP_FSM_LOG("Action: retry_now");

    io_disconnect(conn->fd);
    conn->connected = FALSE;

    conn->fd = io_connect((struct sockaddr *)&conn->addr);

    if (conn->fd == -1) {
        timer_start(conn->connect_timer_id, conn->connect_time);
    } else {
        conn->connected = TRUE;
    }

    sema_post(&conn->conn_sema);

    return 0;
}


static uint16 
a_restart_holdtimer (errp_fsm_event_desc_t *event_desc, void const *param)
{
    errp_conn_t* conn = event_desc->conn;

    ERRP_FSM_LOG("Action: restart_holdtimer");

    timer_start(conn->hold_timer_id, conn->hold_time);
    return 0;
}


void errp_event (errp_conn_t *conn, uint32 event, void *param)
{
    int16 rc;
    errp_fsm_event_desc_t event_desc;

    ERRP_FSM_LOG("state %s, event %s",
           errp_fsm_state_txt(conn->fsm_state), errp_fsm_event_txt(event));

    event_desc.event = event;
    event_desc.conn = conn;
    event_desc.event_arg = param;

    rc = fsm_execute("ERRP_FSM",
                     errp_fsm_table,
                     (int *)&((event_desc.conn)->fsm_state),
                     S_MAX,
                     &event_desc,
                     event_desc.event,
                     0,
                     TRUE);

    ERRP_FSM_LOG("new state %s\n", errp_fsm_state_txt(conn->fsm_state));
}

