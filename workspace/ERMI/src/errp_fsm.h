///  Copyright (c) 2008-2011 by cisco Systems, Inc.
///  All rights reserved.
///
///  @file      errp_fsm.h
///  @brief     Finite state machine definitions for ERRP (ERMI-1) protocol
///  @author    Dean Chen, Yi Tong Tse
///  @date      Jan 2011

#ifndef __ERRP_FSM_H__
#define __ERRP_FSM_H__


#define ERRP_FSM_LOG(fmt, str...)        \
    printf("ERRP FSM: "fmt"\n", ##str);
//    NULL


/// ERRP States
/// @note
/// The enum starts at 1, since fsm_execute() assumes that all module states
/// are normalized to 1.
//
typedef enum {
    S_IDLE = 1,                 ///< Initial state
    S_CONNECT,                  ///< TCP connection pending or open
    S_ACTIVE,                   ///< Listening for connection from remote node
    S_OPENSENT,                 ///< OPEN has been sent
    S_OPENCONFIRM,              ///< Waiting for response to OPEN
    S_ESTABLISHED,              ///< Connection ready for use
    S_MAX
} errp_state_t;


/// ERRP Events
//
typedef enum {
    /// NOTE: the following events are from the ERMI spec
    E_START,                    ///< instructed to open connection
    E_STOP,                     ///< instructed to end session
    E_CONN_OPENED,              ///< TCP connection created
    E_CONN_CLOSED,              ///< TCP connection closed
    E_CONN_OPEN_FAILED,         ///< TCP connection open failed
    E_CONN_FATAL,               ///< established TCP connection terminated
    E_CONN_RETRY_TIMER_EXP,     ///< ConnectRetry timer expired
    E_HOLD_TIMER_EXP,           ///< Hold timer expired
    E_KEEPALIVE_TIMER_EXP,      ///< Keepalive timer expired
    E_OPEN_RCVD,                ///< OPEN message received
    E_KEEPALIVE_RCVD,           ///< KEEPALIVE message received
    E_UPDATE_RCVD,              ///< UPDATE message received
    E_NOTIF_RCVD,               ///< NOTIFICATION message received

    /// NOTE: the following events are NOT from the ERMI spec
    E_SEND_UPDATE,              ///< instructed to send UPDATE message
    E_MSG_ERROR,                ///< error parsing message
} errp_event_t;


/// ERRP FSM Event Descriptor
typedef struct errp_fsm_event_desc_ {
    uint32 event;               ///< event ID
    void *event_arg;            ///< event argument
    errp_conn_t *conn;          ///< ERRP connection
} errp_fsm_event_desc_t;


// Prototypes
void errp_fsm_exec(errp_fsm_event_desc_t event_desc);
void errp_event(errp_conn_t *conn, uint32 event, void *param);


#endif  // __ERRP_FSM_H__
