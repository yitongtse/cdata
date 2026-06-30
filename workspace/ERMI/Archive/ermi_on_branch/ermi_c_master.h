/*
 *-------------------------------------------------------------------
 * ermi_c_master.h - Declares data structures for ERMI-Video control
 * process to handle socket events, timer events and queue events.
 *
 * December 2003 Linda Hua
 * Copyright (c) 2003-2009 by cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------------------
 */
#ifndef _ERMI_C_MASTER_H_
#define _ERMI_C_MASTER_H_

#include <socket/socket.h>
#include INTERNAL_INC(sup/src/vclient/video_sg.h)
#include "ermi_video_hash.h"
#include "ermi_c_global.h"

#define PROT_RTSP         0x10         /* protocol type is RTSP */
#define PROT_VREP         0x20         /* protocol type is VREP */

#define SVR_CONN          0x01         /* local end is server */
#define CLIENT_CONN       0x02         /* local end is client */

#ifdef NK_NOT_USED
#define ERMI_C_MAX_NUM_SVRS  5         /* Max number of local servers
					* count each listening port
					*/

#define ERMI_C_MAX_NUM_PEERS  10       /* Max number of remote peers */
#endif // NK_NOT_USED

#define RTSP_SESSION_TABLE_LEN         3000 /* Max sessions per ERMI server */

#define SOCK_CONN_TIMEOUT  200         /* 200ms for client to conn to server */

/*cli test msg from CLI to master process */
typedef struct cli_test_msg_ {
    ipaddrtype server_ip;
    ushort     port;
    int        ses_index;
    int        burst_size;
    int	       erm_msg_id;
    int        erm_msg_status;
    int        erm_intp;
    int        erm_index;
    int        erm_freq;
    int        erm_prog_no;
    int        erm_udp;
    char       erm_txtp[MAX_SGID_LEN];
    int        ccp_bprog_id;
    void*      ccp_ses_id; /* NEED to fix - SS */

} cli_test_msg_t;

/* this describe the type of a socket connection */
typedef enum {
    RTSP_SVR_CONN    = PROT_RTSP|SVR_CONN,    /* 0x11 RTSP server */
    RTSP_CLIENT_CONN = PROT_RTSP|CLIENT_CONN, /* 0x12 RTSP client */
    VREP_SVR_CONN    = PROT_VREP|SVR_CONN,    /* 0x21 VREP server */
    VREP_CLIENT_CONN = PROT_VREP|CLIENT_CONN, /* 0x22 VREP client */
} conn_type_e;

#define ERMI_C_GET_END_TYPE(type) ( type & 0x0F)

/* some unexpected socket event */
typedef enum {
    SOCKET_ERROR,       /* connect, accept, send or receive failed */
    SOCKET_TIMEOUT      /* socket transaction timed out */
} conn_event_e;

typedef struct ermi_c_conn_ ermi_c_conn_t;
typedef struct rtsp_msg_    rtsp_msg_t;
typedef struct rtsp_session_ rtsp_session_t;

/* the proto type of function to handle unexpected events */
typedef void (*evt_func_t)(ermi_c_conn_t *conn, conn_event_e event);

/* the proto type of function to handle socket read/write */
typedef void (*msg_func_t)(ermi_c_conn_t *conn, ulong events);

/* the proto type of application API to handle a protocol
   msg received from a socket, prot_msg can be rtsp_msg or edcp_msg
   */
typedef void (*app_func_t)(void *ses, void *prot_msg);

/* the proto type of callnack to handle rtsp/edcp msg received 
   from the watched queue */
typedef void (*que_func_t)(ermi_c_conn_t *conn, void *prot_msg);


/* socket connection state */
typedef enum {
    SOCK_IDLE=0,           /* not inuse                       */
    SOCK_CONNECTED,        /* connection made                 */
    SOCK_CONNECT_PENDING,  /* connection is pending           */
    SOCK_FAILED            /* failed                          */
} conn_state_e;

/* socket connection */
struct ermi_c_conn_ {

  struct ermi_c_peer_ *peer;

  rtsp_session_t *ses_tbl[RTSP_SESSION_TABLE_LEN]; /* sessions on this server */
  list_header    ses_index_list;   /* session indices */

  int         fd;            /* socket descriptor */
  evt_func_t  prot_evt_func; /* callback for a event happened to this socket
                                socket broken, timer expired */
  msg_func_t  prot_msg_func; /* callback for a msg coming to this socket */
  app_func_t  app_msg_func;  /* pass msg to app */
  que_func_t  que_msg_func;  /* callback for que msg */ 
  conn_state_e state;        /* connection state */
  conn_type_e type;          /* RTSP/VREP */
  boolean     is_listen_fd;  /* is this a well-known listening port */
  void       *prot_info;     /* protocol conn. data structure */
                             /* allocated and filled by tcp_msg_func */
  ushort      port;          /* we may also need to know the port */
  ipaddrtype  src_addr;      /* src IP addr */
  ipaddrtype  dest_addr;     /* dest IP addr */
  mgd_timer   conn_timer;    /* connect to svr within some time limit (3 min)*/
};

/* the local/remote TCP based rtsp/edcp server identity */
typedef struct ermi_c_peer_ {
    int         port;
    ipaddrtype  addr;

    /* Per server-group info: Socket connections, etc. */
    struct ermi_protocol_info_ *erm_info; /* Containing ERM Server-Group Info */
    ermi_c_conn_t conn;
} ermi_c_peer_t;

/* in order to let the control process act like an RTSP, or VREP server,
 * an api status_e xxxx_init_server(ipaddrtype local_addr,
 *                         ushort listen_port)
 * is provided by the each protocol component, the following is an example of
 * RTSP.
 * A listening socket is created with the well-known listen port, a bunch of
 * handlers are registered in the socket connection table.
 * If a "connect" event is received from the listening socket,
 * a new socket is created for the new connection, all socket event handlers
 * registered at the listening socket are copied to the new socket to
 * handle socket read/write events, and pass them to corresponding
 * application component.
 * prot_evt_func( ) handles unexpected socket event, such as broken connection
 * timeout, it's provided by a protocol component.
 * prot_msg_func( ) handles socket read/write messages. It's provided by a
 * protocol component,
 * app_msg_func( ) handles the socket message after it is received and parsed
 * by the protocol component, RTSP or VREP. It's provided by ERM or SBM.
 * since a socket msg identifies the connection by fd, we use the table driven
 * mechanism to handle all socket events.
 * please take a look at the example ermi_c_rtsp_api.c: rtsp_init_server( ) 
 */

/* in order to let a control process to act like an RTSP client, an api
 * xxxx_client_open_session( list_header *peer_list,
 *                           ipaddrtype ipa, ushort port
 *                           app_func_t *app_fn)
 * is provided by the protocol component to add an entry in the socket
 * connection table. A bunch of handlers are registered in the socket
 * connection table for the rtsp client to handle socket read/write events.
 * AN example for RTSP is in rtsp_api.c.
 */
 
/* the type of timer events
 * If a new type of timer needs to be handle by the control process,
 * add a type here and also add a corresponding timer_func_t to handle it
 * by calling ermi_c_timer_init( )
*/
typedef enum {
    RTSP_CONNECT_TIMER,     /* socket connect timer */
    RTSP_RESPONSE_TIMER,    /* rtsp response timer */
    RTSP_KEEPALIVE_TIMER,   /* keepalive timer for RTSP Client */
    VREP_KEEPALIVE_TIMER,   /* VREP KEEPALIVE timer */
    EDA_VREP_OPEN_TIMER,    /* eda retry vrep open timer */
    RTSP_CONN_KEEPALIVE_TIMER, /* keepalive timer for RTSP Connection */
    LAST_TIMER_TYPE         /* the limit of timer type */
} timer_type_e;


/* proto type of timer event handler,
 * context will be typecasted according to the timer type
 */
typedef void (*timer_func_t)(mgd_timer* the_timer, void *context);

typedef struct timer_ {
    timer_type_e type;      /* the type of timer event */
    timer_func_t timer_fn;  /* corresponding timer event handler */
} ermi_c_timer_t;

/* table of timers, one per process */
// extern ermi_c_timer_t timer_table[LAST_TIMER_TYPE];

#define RTSP_GET_SESSION_CONN(rtsp_ses_ptr) \
             (rtsp_conn_t *)rtsp_ses_ptr->peer_erm->conn.prot_info

/* RTSP, VREP, ... should call this to add an entry in timer_table */
ermi_status ermi_c_timer_init(timer_type_e type,   /* the type of the timer */
                              timer_func_t timer_fn);  /* the handler for this
                                                          type of timer */

/* RTSP, VREP, ... should call this to remove an entry from timer_table */
void ermi_c_timer_remove(timer_type_e type);   /* the type of the timer */

/* all inter-component messages are sent to the central watched queue,
 * control process displatches the queued msgs based on the type,
 * all msgs sent to the queue bears the format of vcm_msg_t.
 */
typedef enum {
    VREP_ERM_TO_EDA,         /* VREP msg sent from ERM to EDA */
    VREP_EDA_TO_ERM,         /* VREP msg sent from EDA to ERM */
    RTSP_ERM_TO_EDA,
    RTSP_EDA_TO_ERM,
    LAST_ERMI_C_QUEMSG_TYPE /* the upper limit of the type */ 
} ermi_c_que_msg_e;

typedef struct ermi_c_msg_ {
    struct ermi_c_msg_ *next;/* so that it can be put to a queue */
    ermi_c_que_msg_e type; /* type shows the destination of the msg */
    int       fd;       /* index into the conn table */
    ushort    size;     /* msg size (not including type */
    void      *msg;     /* this can be a pointer to rtsp_msg_t or edcp_msg_t */
} ermi_c_que_msg_t;

/* another component send a message to master process to start/stop 
 * a component message: the corresponding CLI command: ENABLE/DISABLE ...
 */
void ermi_c_notify_master(int message);

struct ermi_protocol_info_;
struct rfgw_video_server_group_;

/* add this peer to the ERMI table peer_table when a new connection
   is establiched to this remote peer. 
   So when upper application calls xxxx_open_client_session( ),
   this table will be searched to find if there is an existing connection
   to the specified destination.
   So when upper application calls xxxx_init_server( ),
   this table will be searched to find if there is an existing server, 
   do nothing, otherwise, init a server.

   ipa: the src/dest IP addr of the peer
   port: what TCP port we connect to
   fd:   what fd was assigned to this connection
   return: a pointer to the ermi_c_peer_t
*/
ermi_c_peer_t *ermi_c_add_peer(struct rfgw_video_server_group_ *video_sg,
                               ipaddrtype server_addr,
                               ipaddrtype ipa, ushort port);

struct ermi_protocol_info_ * \
      ermi_c_get_ermi_proto_info(struct rfgw_video_server_group_ *);
ermi_c_peer_t *ermi_c_get_peer_erm_info(struct rfgw_video_server_group_ *);

#ifdef NK_NOT_USED
/* Delete a peer from SG table
   ipa:  the destination IP addr
   port: the TCP port
*/
boolean ermi_c_del_peer(struct rfgw_video_server_group_ *video_sg,
                        ipaddrtype ipa, ushort port);
#endif // NK_NOT_USED

boolean ermi_c_stop_svr(ipaddrtype ipa, ushort listen_port);

extern ermi_c_conn_t ermi_c_svr_conn;

extern ipaddrtype  local_ip;  /* master's local ip */

/* connection table for local peers */
extern mgd_timer  ermi_c_parent_timer;           

extern app_func_t rtsps_app_func;

/* svrs that are initialized by ERM and ECMG etc */
extern boolean eda_configed;           /* indicate whether eda is enabled 
                                           from CLI */
boolean is_eda_configed(void);	       /* is eda enabled? */
boolean ermi_c_is_local_ip(ipaddrtype ipa);

#endif /* _ERMI_C_MASTER_H_ */

