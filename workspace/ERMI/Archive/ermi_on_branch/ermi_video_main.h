/*
 * ermi_video_main.h - ERMI Video Main header file
 * Ported from original - eda.h
 *
 * January 2004, Ashok Bhaskar: July-2008, Neeraj Khurana
 *
 * Copyright (c) 2004-2008 by cisco Systems, Inc.
 * All rights reserved.
 */
#ifndef __ERMI_VIDEO_MAIN_H__
#define __ERMI_VIDEO_MAIN_H__

#include COMP_INC(rfgw/sup, ../src/qam/qam.h)
#include COMP_INC(rfgw/sup, ../src/qam/qam.h)
#include INTERNAL_INC(sup/src/vclient/video_sg.h)

#include "ermi_video_dummy.h" // Temp

#include "ermi_c_master.h"
#include "ermi_c_rtsp_def.h"
// #include "vcp_vrep_types.h"

#define EDA_MAX_SVC_GROUPS 64
#define EDA_MAX_SLOTS 8
#define EDA_MAX_QAMS_PER_SLOT 24
#define EDA_MAX_QAMS_IN_BOX (EDA_MAX_SLOTS * EDA_MAX_QAMS_PER_SLOT) 
#define EDA_MAX_PROGRAMS_PER_QAM 20
#define EDA_MAX_SESSIONS (EDA_MAX_QAMS_IN_BOX * EDA_MAX_PROGRAMS_PER_QAM) 

#define EDA_CHECK_IPADDR_INTERVAL     2000 /* 2 seconds */
#define EDA_OPEN_VREP_RETRY_INTERVAL  15000 /* 15 seconds */


typedef enum {
    EDA_SVCGRP_FLAG_CP_ENABLED  = 0x01, // SG is control plane enabled
} eda_sg_flag_t;

#define EDA_SIZE_SVC_GRP_HASH_TBL  128

typedef enum {
    /* session control block status */
    EDA_SCB_STATUS_FREE = 0,
    EDA_SCB_STATUS_BUSY
} eda_scb_status;

typedef enum {
    /* EDA message types - has more variations than the standard edcp 
     * messages 
     */
    ERMI_MSG_TYPE_SETUP_RESPONSE_WITH_UDP = 0,
    ERMI_MSG_TYPE_SETUP_RESPONSE,
    ERMI_MSG_TYPE_SET_PARAMETER,
    ERMI_MSG_TYPE_RESPONSE,
    ERMI_MSG_TYPE_ANNOUNCE,
} eda_msg_type_t;

typedef enum {
    /* return codes for EDA internal use */
    ERMI_RET_NOT_USED = 0,  
    ERMI_RET_OK,  
    ERMI_RET_INVALID_PARAMETER,
    ERMI_RET_NOT_ENOUGH_BANDWIDTH,
    ERMI_RET_SESSION_NOT_FOUND,
    ERMI_RET_INTERNAL_SERVER_ERROR,
    ERMI_RET_SERVICE_UNAVAILABLE
} ermi_return_code_t;

typedef enum {
    ERMI_UDP_INCLUDE,
    ERMI_UDP_EXCLUDE
} ermi_udp_op;

typedef enum {
    ERMI_SESS_ID_INCLUDE,
    ERMI_SESS_ID_EXCLUDE
} ermi_session_id_op;


typedef struct { /* entry in the service group hash table */
    /*
     * sgid is a string that holds the name of the service group.  It MUST
     * be the first field in this structure, as it is the hash key.
     */
    char sgid[MAX_SGID_LEN]; 
    list_header qam_id_list; /* list of qams in this service group */
    ipaddrtype erm_ip; /* ip addr of the ERM managing this svc group */
    ipaddrtype erm_ip_mask; /* mask for the erm_ip address */
    /*
     * flags is a bit mapped field. 
     */
    eda_sg_flag_t flags;
} svcgrp_t;

typedef struct {
    int16 ermi_enabled;
    int16 reserved;
    uint32 domain;      /* my resource domain */
    uint32 vrep_id;     /* unique vrep id in the address domain */
    ipaddrtype my_ip;   /* eda's IP address*/
    ipaddrtype erm_ip;  /* ip addr of the erm that controls this eda */
} ermi_config_t;

typedef struct { /* entry in the qam id list in the sg hash table */
    ushort ts_id;  /* qam ts id */
} qam_id_entry_t;

typedef struct {
    /*
     * Holds all information needed by the EDA to manage the linecards.
     */
    uint32 num_cards;
    // vn_mgr *card_info[MAX_SLOTS];
} eda_lc_info_t;

typedef struct {
    /* EDA control block */
    ushort num_cards;
    // vrep_handle_t *vrep_handle;  /* handle to VREP */
    // mgd_timer vrep_open_timer;   /* reopen timer */
} eda_cb_t;

/*
 * Derive the qam state to be reported to the ERM.
 * Inputs:
 *    cc_qam_state = state of the qam as seen by the config controller
 * Returns:
 *    The qam state to be reported to the ERM.  One of ermi_qam_status_t.
 */
#ifdef ERMI_R_VIDEO_LATER
static inline ermi_qam_status_t eda_derive_erm_qam_state 
                                 (qam_state_t cc_qam_state)
{
    switch (cc_qam_state) {
    case LCC_QAM_STATE_INIT:
        return(ERMI_QAM_STATUS_UNKNOWN);
    case LCC_QAM_STATE_UP:
        return(ERMI_QAM_STATUS_GOOD);
    case LCC_QAM_STATE_MAINTENANCE:
        return(ERMI_QAM_STATUS_MAINTENANCE);
    default:
        return(ERMI_QAM_STATUS_BAD);
    }
}
#endif

/*
 * Prototypes
 */
ermi_status eda_send_announce_msg(rfgw_qam_t *qam);
udp_port_map *eda_assign_udp_port_num(rfgw_qam_t *qam, ushort *udp_port_ptr);
udp_port_map *eda_setup_udp_port(rfgw_qam_t *qam, ushort udp_port_num,
				 ushort prog_num);
svcgrp_t *eda_create_svcgrp(char *sgid);
ermi_status ermi_video_start(void);
ermi_status ermi_video_stop(void);
ermi_status eda_add_svcgrp(svcgrp_t *svcgrp);
ermi_status eda_del_svcgrp(svcgrp_t *svcgrp);
svcgrp_t *eda_find_svcgrp(char *sgid);
ermi_status eda_add_qam_to_svcgrp(rfgw_qam_t *qam, svcgrp_t *svcgrp);
void eda_edcp_msg_handler(rtsp_session_t *session, void *msg);
void eda_init_qam_udp_port_numbers(rfgw_qam_t *qam);
ermi_status eda_free_udp_port_num(rfgw_qam_t *qam, ushort udp_port_num);
void eda_all_sessions_over_on_qam(rfgw_qam_t *qam);
void eda_set_qam_cp_disabled(rfgw_qam_t *qam);
void eda_send_qam_state_to_erm(rfgw_qam_t *qam);
void eda_check_ipaddr(mgd_timer *the_timer, void *context);
extern boolean  ermi_sg_create ( rfgw_video_server_group *video_sg);
extern boolean  ermi_sg_delete ( rfgw_video_server_group *video_sg);
// void eda_retry_vrep_open(mgd_timer *the_timer, void *context);

/* return the chunk back to its pool */
// void free_eda_update_msg(ermi_vrep_msg_t *vrep_msg);

/* handler for vrep error */
void eda_handle_vrep_error(int error_code, ipaddrtype peer_ip);
/* init as vrep speaker */
// void eda_init_vrep(vrep_error_func_t err_fn);

ermi_status eda_process_rtsp_setup_msg(rtsp_session_t *session, rtsp_msg_t *msg);
void ermi_c_rtsp_msg_handler(rtsp_session_t *session, rtsp_msg_t *msg);


/* 
 * Externs
 */
extern vcp_hash_table_t *eda_sg_htbl; 
extern list_header eda_sg_list; 
extern ermi_config_t ermi_config;

#endif __ERMI_VIDEO_MAIN_H__
