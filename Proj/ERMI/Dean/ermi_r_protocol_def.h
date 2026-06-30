/*
 *-------------------------------------------------------------------
 * ermi_r_protocol_def.h
 * - Protocol related definitions for ERMI-1 (ERMI-Registration) protocol
 *
 * October 2008
 * Copyright (c) 2008 by Cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------------------
 */

#ifndef _ERMI_R_PROTOCOL_DEF_H__
#define _ERMI_R_PROTOCOL_DEF_H__

// #include <os/list_private.h>
#include COMP_INC(rfgw, rfgw_common.h)
#include COMP_INC(cisco, bitlogic.h)
// #include "../video_sg.h"
// #include "ermi_c_master.h"

#define ERMI_SG_NAME "ermi-sg" 

#define RFGW_QAM_GRP_NAME_LEN  40

#define ERRP_MAXBYTES             4096 
#define ERRP_HEADERBYTES 	  3   
#define ERRP_MAX_NAME_LEN         256
#define ERRP_DFL_CONN_TIME        10 // 120 /* seconds */
#define ERRP_DFL_HOLD_TIME        9   /* seconds */
#define ERRP_DFL_KEEPALIVE_TIME   3   /* seconds */
#define ERRP_PORT                 6069
#define ERRP_SOC_WINDOW           32768
#define ERRP_VERSION              3
#define ERRP_ADDR_DOMAIN_ANY      0
#define ERRP_ADDR_FAMILY          32769 /* RTSP URL */
#define ERRP_APP_PROTOCOL_STATIC  32766 
#define ERRP_APP_PROTOCOL         32768 
#define ERRP_APP_PROTOCOL_PROV    32770 
#define ERRP_VERSION_CAP          1
#define ERRP_MAX_NBRS             256

#define ERRP_MAX_ACCEPT_ATTEMPTS  5 
#define ERRP_READ_ATTEMPTS        5

#define ERRP_PUTSHORT             PUTSHORT
#define ERRP_PUTLONG              PUTLONG
#define ERRP_GETSHORT             GETSHORT
#define ERRP_GETLONG              GETLONG
#define ERMI_R_DEBUG              buginf

typedef enum {
    ERRP_OPEN = 1,
    ERRP_UPDATE,
    ERRP_NOTIFICATION,
    ERRP_KEEPALIVE,
} errp_msg_type_t;

typedef enum {
    ERRP_CAPABILITY_INFORMATION = 1,
    ERRP_STREAMING_ZONE,
    ERRP_COMPONENT_NAME,
    ERRP_VENDOR_SPECIFIC_STRING,
} errp_opt_param_t;

typedef enum {
    ERRP_CAP_CODE_ROUTE_TYPE_SUPPORTED  = 1,
    ERRP_CAP_CODE_SEND_RECEIVE = 2,
    ERRP_CAP_CODE_ERRP_VERSION = 32768,
} errp_cap_code_t;

typedef enum {
    ERRP_SEND_RCV_CAP_BOTH = 1,
    ERRP_SEND_RCV_CAP_SEND_ONLY,
    ERRP_SEND_RCV_CAP_RCV_ONLY,
} errp_send_rcv_cap_t;

typedef enum {
    ERRP_KEEPALIVE_TIMER_TYPE,
    ERRP_HOLD_TIMER_TYPE,
    ERRP_CONNECT_TIMER_TYPE,
} errp_timer_type_t;

/* this enum need to be kept in sync with indices of errp_attrib_type[] */
typedef enum {
    ERRP_WITHDRAWN_ROUTE_BIT,
    ERRP_REACHABLE_ROUTE_BIT,
    ERRP_NEXT_HOP_SERVER_BIT,
    ERRP_QAM_NAMES_BIT,
    ERRP_CAS_CAPABILITY_BIT,
    ERRP_TOTAL_BANDWIDTH_BIT,
    ERRP_AVAILABLE_BANDWIDTH_BIT,
    ERRP_COST_BIT,
    ERRP_EDGE_INPUT_BIT,
    ERRP_QAM_CHANNEL_CONFIGURATION_BIT,
    ERRP_UDP_MAP_BIT,
    ERRP_SERVICE_STATUS_BIT,
    ERRP_MAX_MPEG_FLOWS_BIT,
    ERRP_NEXT_HOP_ALTERNATE_BIT,
    ERRP_PORT_ID_BIT,
    ERRP_FIBER_NODE_BIT,
    ERRP_QAM_CAPABILITY_BIT,
    ERRP_INPUT_MAP_BIT,
    ERRP_ATTRIB_MAX_BIT = ERRP_INPUT_MAP_BIT,
} errp_attrib_bitmask_list;

typedef BITMASK_DEFINITION(errp_attrib_bitmask_t, ERRP_ATTRIB_MAX_BIT);

typedef enum {
    ERRP_WITHDRAWN_ROUTE = 1,
    ERRP_REACHABLE_ROUTE = 2,
    ERRP_NEXT_HOP_SERVER = 3,
    ERRP_QAM_NAMES = 232,
    ERRP_CAS_CAPABILITY = 233,
    ERRP_TOTAL_BANDWIDTH = 234,
    ERRP_AVAILABLE_BANDWIDTH = 235,
    ERRP_COST = 236,
    ERRP_EDGE_INPUT = 237,
    ERRP_QAM_CHANNEL_CONFIGURATION = 238,
    ERRP_UDP_MAP = 239,
    ERRP_SERVICE_STATUS = 241,
    ERRP_MAX_MPEG_FLOWS = 242,
    ERRP_NEXT_HOP_ALTERNATE = 243,
    ERRP_PORT_ID = 244,
    ERRP_FIBER_NODE = 245,
    ERRP_QAM_CAPABILITY = 247,
    ERRP_INPUT_MAP = 249,
} errp_attrib_t;

/* Forward declaration */
struct rfgw_video_server_group_;             /* video_sg.h */
struct ermi_c_peer_;                         /* ermi_c_master.h */

/*
 * Structure to hold the qam group
 * The qam_bitmap holds one bit for each QAM ID
 * The index of the array is the QAM block to which the abs_qam_id belongs to
 * Configured under the "cable video qam-group" CLI
 */

typedef ulonglong qam_bitmap_t;

typedef struct qam_group {
    list_element   list_el;
    char           group_name[RFGW_QAM_GRP_NAME_LEN]; 
    qam_bitmap_t   qam_bitmap[NUMB_MB_QAM_BLOCK_PER_CHASSIS]; /* QAM bitmap */
} rfgw_ermi_qam_grp_t;

/*
 * Adding and storing qam-group under here. From existing VClient implementation
 * we already have struct  rfgw_video_server_group  to hold the ERM IP address
 * and other ERM server-group related info, so we leverage that for ERM
 * server-group info.
 *
 * It is instantiated one ERRP node per ERM.
 */


typedef struct errp_params_ {
    struct rfgw_video_server_group_  *server_group;
    list_header qam_grp_list;
    // other parameters
} errp_erm_params_t;


typedef struct ermi_r_master_ {
    mgd_timer              master_timer;
    list_header            nbr_list;
    int                    server_fd;
    boolean                initialized;
    boolean                running;

    /* own node info */
    uint32                 errp_id;
    uint8                  version;
    uint32                 addr_domain;
    uint16                 addr_family;
    uint16                 app_protocol;
    uint32                 send_receive_cap;
    uint32                 version_cap;
    char                   streaming_zone[ERRP_MAX_NAME_LEN]; // TBD, make this a vector
    char                   component_name[ERRP_MAX_NAME_LEN]; // TBD, make this a vector
    char                   vendor_str[ERRP_MAX_NAME_LEN];
    uint32                 connect_time;
    uint32                 hold_time;
    uint32                 keep_alive_time;
    ipaddrtype             signalling_ip; // TBD, make this a vector
    ipaddrtype             alt_signalling_ip; // TBD, make this a vector
} ermi_r_master_t;

typedef struct ermi_r_neighbor_ ermi_r_neighbor_t;
typedef void(*io_connect_func_t)(ermi_r_neighbor_t *nbr);
typedef void(*io_disconnect_func_t)(ermi_r_neighbor_t *nbr);
typedef int(*io_send_func_t)(ermi_r_neighbor_t *nbr, uchar *buf, int len);
typedef int(*io_recv_func_t)(ermi_r_neighbor_t *nbr, uchar *buf);
typedef void(*timer_start_func_t)(ermi_r_neighbor_t *nbr, 
                                   ushort timer_type, ulong timeout);
typedef void(*timer_stop_func_t)(ermi_r_neighbor_t *nbr, 
                                  ushort timer_type);

/* 
 * RFGW-10 as an ERRP node can interface with multiple ERMs.
 * For each of the peer nodes, a neighbor node data structure is used to hold
 * the peer node id, version, transport handle, as well as the send queue for
 * ERRP updates, state of the ERRP session and timers. The neighbor nodes will
 * be kept in a list.
 *
 * It is instantiated one ERRP node per ERM.
 */
struct ermi_r_neighbor_ {
    struct errp_params_    erm_param;
    struct ermi_r_master_   *master; /* back pointer to master */

    /* IO */
    ipaddrtype             address;
    int                    fd;
    boolean                connected;
    io_connect_func_t      io_connect_func;
    io_disconnect_func_t   io_disconnect_func;
    io_send_func_t         io_send_func;
    io_recv_func_t         io_recv_func;

    /* ERRP FSM */
    uint32                 fsm_type;
    uint32                 fsm_state;
    struct finite_state_machine *fsm_table;
    mgd_timer	           keepalive_timer;
    mgd_timer	           hold_timer;
    mgd_timer	           connect_timer;
    timer_start_func_t     timer_start_func;
    timer_stop_func_t      timer_stop_func;

    /* ERRP NBR NODE */
    uint32                 errp_id;
    uint8                  version;
    uint32                 addr_domain;
    uint16                 addr_family;
    uint16                 addr_protocol;
    uint32                 send_receive_cap;
    uint32                 version_cap;
    char                   streaming_zone[ERRP_MAX_NAME_LEN];
    char                   component_name[ERRP_MAX_NAME_LEN];
    char                   vendor_str[ERRP_MAX_NAME_LEN];
    uint32                 connect_time;
    uint32                 hold_time;
    uint32                 keep_alive_time;

    /* ERRP PARAM */
    struct rfgw_video_server_group_ *server_group;
    errp_attrib_bitmask_t  attrib_bitmask;
    // void * (*errp_get_qam_param)(uint param_type, struct ermi_r_neighbor_ *nbr, void **param_value);
    // void * (*errp_set_qam_param)(uint param_type, struct ermi_r_neighbor_ *nbr, void *param_value);

    /* STATS */

    /* TRANSIENT PRAMETERS */
    /* slot/port/chan of QAM interface in question during an operation */ 
    uint32                 qam_slot; 
    uint32                 qam_port;
    uint32                 qam_chan;
    /* temporary workspace */ 
    uchar                  workspace[ERRP_MAXBYTES];
};

/* Stucture to hold ERMI specific pointers in the generic video_sg structure */
typedef struct ermi_protocol_info_ {
    ermi_r_neighbor_t *errp_mynode ;  // Stores ERMI-R specific info for an SG
    struct ermi_c_peer_ *ermi_c_info; // Stores ERMI-C specific info for an SG
} ermi_protocol_info_t;

typedef struct errp_specific_info {
    ermi_r_neighbor_t *nbr;
} errp_specific_info_t;

#endif /* #ifndef _ERMI_R_PROTOCOL_DEF_H__ */








