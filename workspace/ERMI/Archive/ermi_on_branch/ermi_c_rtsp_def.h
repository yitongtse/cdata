/*
 *-------------------------------------------------------------------
 * ermi_c_rtsp_def.h 
 * - Ported from vcp_rtsp_def.h
 *
 * Definitions for ERMI-2 Interface (RTSP based)
 *
 * September 2004
 * Copyright (c) 2004-2009 by Cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------------------
 */

#ifndef _ERMI_C_RTSP_DEF_H_
#define _ERMI_C_RTSP_DEF_H_

#include <os/list_private.h>
#include "ermi_video_hash.h"
#include "ermi_c_master.h"

#define DEFAULT_RTSP_PORT           554 
#define TCP_PORT_RTSP_VOD           8540 

#define RTSP_INITIAL_CSEQ              100   /*initial sequence number*/
#define RTSP_SESSION_TIMEOUT	       60000 /*session timeout in 60 sec*/
#define RTSP_SES_RESP_TIMEOUT	       5000  /* wait 5sec for response */   
#define RTSP_METHOD_LEN                40   /*rtsp method len*/
#define RTSP_IN_BUF_SIZE               1024 /*rtsp read buf size todo*/
#define RTSP_OUT_BUF_SIZE              1024 /*rtsp send buf size todo*/
#define RTSP_MAX_NUM_SESSIONS          3000 /*up to 3000 sessions 
                                              simultaneously*/

// #define RTSP_SESSION_TABLE_LEN         3000 /* max sessions per conn */

#define RTSP_DATE_LEN                  40
#define MAX_NUM_SOCKET_SEND            5  /* call socket_send up to 5 times */

#define MAX_CONTROL_LOC_LEN 	       128 /* "ControlLocation" hdr max len */
#define MAX_SESSGRP_LIST_SIZE 	       20
#define MAX_SESSGRP_LEN 	       128

#define RTSP_URL_PREFIX                "RTSP://"

#define RTSP_SRV_SESSID_LEN	      10 /* length of session_id that our 
					  * rtsp server generates.
					  * Must be less than MAX_SESSID_LEN
					  */
#define ERMI_C_MAX_CONTENT_LEN  	      1024 /* arbitrary limit  */

#define ERMI_C_CONN_TIMEOUT              900  /* seconds */

#define ERMI_C_RTSP_CONN_TIMEOUT         (3 * ONEHOUR)  /* 3 hours */
#define ERMI_C_RTSP_CONN_TIMER_RETRY_COUNT   3  /* 3 times */

/*rtsp session states*/
typedef enum {
    RTSP_SESSION_IDLE,
    RTSP_SESSION_READY
} session_state_e;

typedef struct rtsp_buf_ {
    struct rtsp_buf_ *next;
    int     len;
    uchar   *buf;
} rtsp_buf_t;

/*the rtsp session*/
struct rtsp_session_ {
    uint32 session_id;                    /* unique ID for this session */
    char   clientsession_id[MAX_SESSID_LEN]; /* assigned by RTSP client */
    session_state_e state;           /* INIT_STATE, READY_STATE */
    ermi_c_peer_t  *peer_erm;            /* the peer */
    int    index;                    /* session index, 1 - 3000 */
    char   url[MAX_URL_LEN];         /* url */
    char   method[RTSP_METHOD_LEN];  /* rtsp method */
    ushort sent_method;              /* which request was sent last */
    uint32 recv_cseq;                /* received sequence number */
    uint32 send_cseq;                /* sent sequence number */
    boolean request_pending;
    mgd_timer resp_timer;            /* response has to come back before this
					timer goes off */
    uint32 qam_tsid;                 /* Tsid of QAM on which session created */
    char   session_group[MAX_SESSGRP_LEN];  /* assigned by client */ 

    struct rtsp_msg_ *rtsp_resp_msg;
    // mgd_timer keepalive_timer;    /* Keepalive timer for RTSP client */
};

/* rtsp protocol connection info */
typedef struct rtsp_conn_ {
    ermi_c_conn_t *conn;             /* the conn */

    vcp_hash_table_t *req_table;     /* sessions have pending request */
    int            in_size;          /* how much data is in in_buf */
    char          *read_pointer;     /* where we are in the in_buf */
    char           in_buf[RTSP_IN_BUF_SIZE]; /* buffer to carry incoming data */
    uint32         send_cseq;        /* cseq is updated everytime a request is
                                        sent for a client, or a request is
                                        received by a server */
    int            out_size;         /* how much data is in out_buf */
    char           out_buf[RTSP_OUT_BUF_SIZE]; /*buffer to hold out-going data*/
    queuetype      send_list;        /* data queued to be sent */
    rtsp_session_t *cur_session;     /* the current session */
    mgd_timer      conn_timer;       /* sock connect timer */
    int            remlen;	     /* number of content bytes pending */
    struct rtsp_msg_ *saved_rtsp_msg;

    ulong          connection_timeout;
    ulong          session_keepalive_timeout;

    mgd_timer      keepalive_timer;    /* Keepalive timer for RTSP connection */
    ushort         timer_retry_count;

} rtsp_conn_t; 

/*control protocol types*/
typedef enum {
    RTSP    
} rp_control_e;

typedef enum {
    METHOD_SETUP = 0x01,
    METHOD_TEARDOWN = 0x02,
    METHOD_SETPARAMETER = 0x03,
    METHOD_RESPONSE = 0x04,
    METHOD_ANNOUNCE = 0x05,
    METHOD_GETPARAMETER = 0x06,
    EVENT_TIMEOUT = 0x07,
    EVENT_CONN_ERROR = 0x08,
    MSG_TYPE_LAST
} prot_msg_event_e;


/*rtsp/edcp msg type between app and protocol*/
typedef enum {
    RTSP_SETUP = PROT_RTSP|METHOD_SETUP,               /* 0x11 */
    RTSP_TEARDOWN = PROT_RTSP|METHOD_TEARDOWN,         /* 0x12 */
    RTSP_SETPARAMETER = PROT_RTSP|METHOD_SETPARAMETER, /* 0x13 */
    RTSP_RESPONSE = PROT_RTSP|METHOD_RESPONSE,         /* 0x14 */
    RTSP_ANNOUNCE = PROT_RTSP|METHOD_ANNOUNCE,         /* 0x15 */
    RTSP_GETPARAMETER = PROT_RTSP|METHOD_GETPARAMETER, /* 0x16 */

    RTSP_RESP_TIMEOUT = PROT_RTSP|EVENT_TIMEOUT,       /* 0x17 */
    RTSP_CONN_ERROR = PROT_RTSP|EVENT_CONN_ERROR,      /* 0x18 */
} rp_msg_type_e;

/* macros to get the protocol and method */
#define ERMI_C_GET_PROT(type)   ( type & 0xF0)
#define ERMI_C_GET_METHOD(type) ( type & 0x0F)


#define VALID_FIELD_NBYTES 4

/* max number of "Transport" headers supported per message */ 
#define NUM_TRANS_HDR      5

typedef enum {
    FTAG_UNRECOGNIZED = 0,
    FTAG_ERMI = 1,
} require_t;

typedef struct rtsp_content_ {
    uchar	type;	/* text, SDP, etc. */
    ushort	len;	/* value indicated in the rtsp_msg */
    ushort	curlen; /* current length. maybe waiting for the rest...*/
    uchar       *ptr;   /* points to malloced buffer */
} rtsp_content_t;

#define RTSP_NOTICE_EVENTDATE_LEN 30

typedef struct {
    ushort code;
    char   event_date[RTSP_NOTICE_EVENTDATE_LEN];
    ulong  npt;
} notice_t;

typedef enum {
    ERMI_MPTS_MODE_UNKNOWN = 0,
    ERMI_MPTS_MODE_PASSTHROUGH,
    ERMI_MPTS_MODE_MULTIPLEX
} ermi_mpts_mode_t;


#define MAX_QAM_NAME_LEN 60

/* holds the contents of a "Transport" header 
 */
typedef struct transhdr_ {
    ulong         tvalid; /* indicates valid fields in transport header */
    transport_t	  type; /* UDP or QAM */
    boolean 	  unicast; 
    ulong	  bit_rate;
    ushort	  dest_port;
    ushort	  src_port;
    ulong	  tsid;
    ulong	  qam_frequency;
    ushort	  qam_prognum;
    ipaddrtype	  dest;
    ipaddrtype	  src;
    ipaddrtype	  mcast_address;
    ushort	  rank;
    ushort	  mpts_program;
    char	  qam_name[MAX_QAM_NAME_LEN];
} transhdr_t;

/* holds the contents of an rtsp msg
 */
struct rtsp_msg_ {

    /* Pointer to next request - Needed as NULL for enqueue */
    struct rtsp_msg_ *next;

    rp_msg_type_e msg_type;                  /* SETUP, TEARDOWN, ANNOUNCE,.. */
    ulong    	  valid;                     /* indicates valid fields */
    char     	  url[MAX_URL_LEN];
    uchar         num_trans_hdrs;	     /* # of Transport hdrs in msg */
    transhdr_t    transhdr[NUM_TRANS_HDR];
    uint32   	  cseq;                      /* current seq */
    char          date[RTSP_DATE_LEN];
    ipaddrtype    ip_in_url;
    ushort        port_in_url;

    require_t 	  ftag;
    rtsp_content_t content;

    rtsp_session_t *session_ptr;
    uint32  	  sessionID;
    char  	  clientsessionID[MAX_SESSID_LEN];
    ulong 	  timeout;
    ushort	  reason;
    ushort 	  resp_retcode;
    char   	  session_group[MAX_SESSGRP_LEN];
    char   	  *session_group_list;
    ulong  	  conn_timeout;
    notice_t 	  notice;
    char         *sessionlist;	/* points to malloced buffer */

    ushort  	  pid_remap;
    ermi_mpts_mode_t mpts_mode;
};

typedef enum {

    /* Bits in the "valid" bitmap of a transport header */
    RP_BIT_TRANS_STREAM_TYPE	= 0,
    RP_BIT_TRANS_BIT_RATE 	= 1,
    RP_BIT_TRANS_DEST_PORT    	= 2,
    RP_BIT_TRANS_SRC_PORT 	= 3,
    RP_BIT_TRANS_QAM_TSID 	= 4,
    RP_BIT_TRANS_QAM_NAME 	= 5,
    RP_BIT_TRANS_FREQUENCY 	= 6,
    RP_BIT_TRANS_PROGNUM 	= 7,
    RP_BIT_TRANS_DEST 		= 8,
    RP_BIT_TRANS_SRC 		= 9,
    RP_BIT_TRANS_MCAST_ADDR 	= 10,
    RP_BIT_TRANS_RANK 		= 11,
    RP_BIT_TRANS_MPTS_PROGRAM 	= 12,
    /* .... */
    RP_BIT_TRANS_STREAM_JOINED 	= 30, /* Marked when Xport session created */
    /* Since these must fit in 4 bytes, the max enumeration is 31 */

    /* Bits in the "valid" bitmap of the message */
    RP_BIT_URL                  = 0,
    RP_BIT_DATE                 = 1,
    RP_BIT_IP_IN_URL            = 2,
    RP_BIT_PORT_IN_URL          = 3,
    RP_BIT_FTAG                 = 4,
    RP_BIT_CSEQ                 = 5,
    RP_BIT_SESSIONID            = 6,
    RP_BIT_CLIENTSESSIONID      = 7,
    RP_BIT_RESP_RETCODE         = 8,
    RP_BIT_RESP_RETPHRASE       = 9,
    RP_BIT_CONTROL_LOC          = 10,
    RP_BIT_REASON               = 11,
    RP_BIT_NOTICE               = 12,
    RP_BIT_NOTICE_CODE          = 13,
    RP_BIT_TIMEOUT              = 14,
    RP_BIT_SESSGRP              = 15,
    RP_BIT_CONN_TIMEOUT         = 16,
    RP_BIT_SESSIONLIST          = 17,
    RP_BIT_CONTENT              = 18,
    RP_BIT_SESSGRP_LIST         = 19,

    RP_BIT_PIDREMAP             = 21,
    RP_BIT_MPTS_MODE            = 22,
    
    /* Note: Don't define more bits than can fit in VALID_FIELD_NBYTES.
     * For VALID_FIELD_NBYTES==4, max enumerated bits is 31
     */

} rp_bit_position_t;


/*These macros work for multi-byte bitmaps 
 */
#define RPM_SETBIT(field, bit)   ( field |= (1 << bit) )
#define RPM_TESTBIT(field, bit)  ( field &  (1 << bit) )
#define RPM_CLEARBIT(field, bit) ( field &= ~(1<< bit) )

/* RESPONSE Return Codes */
typedef enum {
    RTSP_RET_OK                         = 200,
    RTSP_RET_NOT_FOUND		        = 404,
    RTSP_RET_METHOD_NOT_ALLOWED		= 405,
    RTSP_RET_REQUEST_TIMEOUT		= 408,
    RTSP_RET_INVALID_PARAMETER          = 451,
    RTSP_RET_NOT_ENOUGH_BANDWIDTH       = 453,
    RTSP_RET_SESSION_NOT_FOUND          = 454,
    RTSP_RET_INVALID_RANGE		= 457,
    RTSP_RET_UNSUPPORTED_TRANSPORT      = 461,
    RTSP_RET_DESTINATION_UNREACHABLE	= 462,
    RTSP_RET_INTERNAL_SERVER_ERROR      = 500,
    RTSP_RET_SERVICE_UNAVAILABLE        = 503,
    RTSP_RET_VERSION_NOT_SUPPORTED      = 505,
    RTSP_RET_INVALID                    = 0xffff
} rtsp_ret_code_e;

extern char *ermi_c_rtsp_respcode2text(ushort resp_code);

/*
 * ERMI-2 RTSP TearDown Reason Codes
 *
 * 200	User stopped
 * 201	N/A
 * 202	N/A
 * 203	N/A
 * 204	No user activity
 * 205	Set-top capability mismatch
 * 206	Insufficient priority
 * 207	Network delivery failure
 * 400	Fail to tune
 * 401	Loss of tune
 * 402	Loss of tune
 * 403	RTSP failure
 * 404	Channel failure
 * 405	No RTSP server
 * 406	N/A
 * 407	N/A
 * 408	Unknown
 * 409	Network Resource Failure
 * 420 	Settop Heartbeat Timeout
 * 421 	Settop Inactivity Timeout
 * 422 	Content Unavailable
 * 423 	Streaming Failure
 * 424 	QAM Failure
 * 425 	Volume Failure
 * 426 	Stream Control Error
 * 427 	Stream Control Timeout
 * 428 	Session List Mismatch
 * 550	Session timeout
 */
typedef enum {
    RTSP_REASON_USER_STOP		= 200,
    RTSP_REASON_NO_USER_ACTIVITY        =  204,
    RTSP_REASON_SETTOP_CAPABILITY_MISMATCH = 205,
    RTSP_REASON_INSUFFICIENT_PRIORITY   = 206,
    RTSP_REASON_NETWORK_DELIVERY_FAILURE= 207,
    RTSP_REASON_FAIL_TO_TUNE		= 400,
    RTSP_REASON_LOSS_OF_TUNE		= 401,
    RTSP_REASON_LOSS_OF_TUNE1		= 402,
    RTSP_REASON_RTSP_FAILURE		= 403,
    RTSP_REASON_CHANNEL_FAILURE		= 404,
    RTSP_REASON_NO_RTSP_SERVER		= 405,
    RTSP_REASON_UNKNOWN			= 408,
    RTSP_REASON_NETWORK_RESOURCE_FAILURE= 409,
    RTSP_REASON_SETTOP_HEARTBEAT_TIMEOUT= 420,
    RTSP_REASON_SETTOP_INACTIVITY_TIMEOUT= 421,
    RTSP_REASON_CONTENT_UNAVAILABLE     =  422,
    RTSP_REASON_STREAMING_FAILURE       =  423,
    RTSP_REASON_QAM_FAILURE             =  424,
    RTSP_REASON_VOLUME_FAILURE          =  425,
    RTSP_REASON_STREAM_CONTROL_ERROR    =  426,
    RTSP_REASON_STREAM_CONTROL_TIMEOUT  =  427,
    RTSP_REASON_SESSION_LIST_MISMATCH   =  428,
    RTSP_REASON_RTSP_REASON_SESSION_TIMEOUT= 550

} ermi_c_teardown_reason_code_t;


/*
 * ERMI-2 RTSP ANNOUNCE Codes
 *
 * 2105	Delivery succeeded
 * 5401	Downstream Failure
 * 5200	Server Resource Unavailable
 * 5404	Unable to Join
 * 5405	Input Port Failure
 * 5406	Failover to Redundant Source
 * 4400	Error Reading Content Data - PID Conflict
 * 5502	Internal Server Error 
 * 5602	Bandwidth Exceeded Limit
 * 5700	Session in Progress
 * 5701	Reclaim Session
 */

typedef enum {
    RTSP_NOTICE_CODE_NONE                   = 0,
    RTSP_NOTICE_CODE_DELIVERY_SUCCEEDED     = 2104,
    RTSP_NOTICE_CODE_ERROR_CONTENT_PID_CONFLICT = 4400,
    RTSP_NOTICE_CODE_SERVER_RESOURCE_UNAVAILABLE= 5200,
    RTSP_NOTICE_CODE_DOWNSTREAM_FAILURE	    = 5401,
    RTSP_NOTICE_CODE_UNABLE_TO_JOIN         = 5404,
    RTSP_NOTICE_CODE_INPUT_PORT_FAILURE     = 5405,
    RTSP_NOTICE_CODE_FAILOVER_TO_RED_SOURCE = 5406,
    RTSP_NOTICE_CODE_INTERNAL_SERVER_ERROR  = 5502,
    RTSP_NOTICE_CODE_STREAM_MARKER_MISMATCH = 5601,
    RTSP_NOTICE_CODE_BANDWIDTH_EXCEEDED_LIMIT = 5602,
    RTSP_NOTICE_CODE_SESSION_IN_PROGRESS    = 5700,
    RTSP_NOTICE_CODE_RECLAIM_SESSION        = 5701
} rtsp_announce_code_t;


/*
 * ERMI-2 (ERMI-C) protocol specific statistics - per message processing.
 */
typedef struct rfgw_ermi_c_statistics_ {

    uint32 recv_req;
    uint32 fail_req;
    uint32 succ_req;

    uint32 send_rsp;
    uint32 fail_rsp;
    uint32 succ_rsp;

} rfgw_ermi_c_statistics_t;


typedef struct rfgw_ermi_c_stats_tab_ {

    rfgw_ermi_c_statistics_t stats[MSG_TYPE_LAST]; /*Stats per ERMI-C Msg type*/

} rfgw_ermi_c_stats_tab_t;

extern rfgw_ermi_c_stats_tab_t rfgw_ermi_c_stats;

#define ERMI_STATS_TABLE  rfgw_ermi_c_stats.stats

/*
 * Stats Incr/handling Functions. Invoke them @ Msg Parser/Processing Points.
 */
static inline void ermi_c_stats_init (void)
{
    memset(&rfgw_ermi_c_stats, 0, sizeof(rfgw_ermi_c_stats_tab_t));
}

static inline void ermi_c_stats_incr_rcvd (prot_msg_event_e msg_type)
{
    if (msg_type <= MSG_TYPE_LAST) ERMI_STATS_TABLE[msg_type].recv_req++;
}

static inline void ermi_c_stats_incr_rcvd_err (prot_msg_event_e msg_type)
{
    if (msg_type <= MSG_TYPE_LAST) ERMI_STATS_TABLE[msg_type].fail_req++;
}

static inline void ermi_c_stats_incr_rcvd_success (prot_msg_event_e msg_type)
{
    if (msg_type <= MSG_TYPE_LAST) ERMI_STATS_TABLE[msg_type].succ_req++;
}

static inline void ermi_c_stats_incr_send (prot_msg_event_e msg_type)
{
    if (msg_type <= MSG_TYPE_LAST) ERMI_STATS_TABLE[msg_type].send_rsp++;
}

static inline void ermi_c_stats_incr_send_err (prot_msg_event_e msg_type)
{
    if (msg_type <= MSG_TYPE_LAST) ERMI_STATS_TABLE[msg_type].fail_rsp++;
}

static inline void ermi_c_stats_incr_send_success (prot_msg_event_e msg_type)
{
    if (msg_type <= MSG_TYPE_LAST) ERMI_STATS_TABLE[msg_type].succ_rsp++;
}

/* Functions Definitions */
extern char *ermi_c_rtsp_announcecode2text(ushort announce_code);

/* global rtsp data */
extern chunk_type *rtsp_conn_chunk;
extern chunk_type *rtsp_session_chunk;
extern chunk_type *rtsp_msg_chunk;
extern int test_rtsp;


/* 
 *  create a new rtsp_conn based on the connection type
 */
rtsp_conn_t *rtsp_new_conn(ermi_c_conn_t *conn, conn_type_e type);


#endif /* #ifndef _ERMI_C_RTSP_DEF_H_ */

