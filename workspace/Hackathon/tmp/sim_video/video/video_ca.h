///  Copyright (c) 2011-2015 by Cisco Systems, Inc.
///  All rights reserved.
///
///  @file      video_ca.h
///  @brief     Video encryption support header file
///  @author    Yi Tong Tse

#ifndef __VIDEO_CA_H__
#define __VIDEO_CA_H__

#include "common.h"
#include "que.h"
#include "video.h"
//#include <vdman_veman_port/vdman_veman_port.h>
//#include "scs_messages.h"


#define CA_DATA_SIZE_MAX        1024
#define INVALID_CA_INFO_IDX     0xFFFF

#define ECM_INFO_SIZE           20
#define ECM_PKT_SIZE            (256 * 4)  // The packets are aligned on 256 byte boundaries
#define MAX_ECM                 (3840 * 5)

#define ECM_PERIOD_DEFAULT      (100 * 45)


// CA test command types
// Note: These command types are used for ng_cli (or sim_cli) testing only!!
//       Their purpose is equivalent to the counterpart message type without
//       "VIDEO_CA", defined in <vdman_veman_port/vdman_veman_port.h>.
//
enum {
    MSG_TYPE_VIDEO_CA_SET_CW_INDEX = 301,
    MSG_TYPE_VIDEO_CA_START_ECM_PLAYOUT,
    MSG_TYPE_VIDEO_CA_REMOVE_ECM_PLAYOUT,
    MSG_TYPE_VIDEO_CA_ADD_CA_DESCRIPTOR,
    MSG_TYPE_VIDEO_CA_REMOVE_CA_DESCRIPTOR,
    MSG_TYPE_VIDEO_CA_GET_SID,
    MSG_TYPE_VIDEO_CA_GET_PID,
    MSG_TYPE_VIDEO_CA_SET_PID_SCRAMBLED,
    MSG_TYPE_VIDEO_CA_RESERVE_PID,
    MSG_TYPE_VIDEO_CA_REQUEST_PID,
    MSG_TYPE_VIDEO_CA_SID_ECM_PID,
    MSG_TYPE_VIDEO_CA_RELEASE_PID
};

struct pmt_info_;
struct crsl_insert_;
struct out_prog_;
struct out_session_;
#if 0
typedef struct {
    que_elem_t link;
    powerkey_ca_info_t ca_info_hdr;
    char ca_data[CA_DATA_SIZE_MAX];
} ca_info_t;
#endif
// ECM info (MUST agree with what's defined by SCS)
typedef struct ecm_info_ {
    uint32 ecm_pkt_offset;      // byte offset of ECM pkt in ECM packet buffer
    uint32 num_tp;              // number of TPs in ECM
    uint32 insert_period;       // ECM insert period (in 45 kHz ticks)
	uint8	junk; //Static ECM packet location (SCS Virtual Address)
	uint32	junk2;		//pointer to the hint bit ecm packet (null if not used)
} ecm_info_t;

extern uint64 *ecm_index_buf;
extern uint8 *ecm_info_buf;
extern uint8 *ecm_pkt_buf;

extern off_t ecm_pkt_offset;

static inline
ecm_info_t* get_ecm_info (int idx)
{
    intptr_t ecm_info_offset = ecm_index_buf[idx];
    if (ecm_info_offset == (intptr_t)NULL)
    	// There is nothing to play out at this time
    	return NULL;  
    if (ecm_info_offset > (MAX_ECM - 1) * ECM_INFO_SIZE) {
        //VIDEO_DEBUG("Bad ecm info offset %" PRIxPTR "d\n", ecm_info_offset);
        return NULL;
    }
    return (ecm_info_t*)&ecm_info_buf[ecm_info_offset];
}


void video_ca_init_ipc(void);
int video_ca_init_shm(void);
int video_ca_send_scg_provision(uint16 out_ses_id);
int video_ca_send_scg_deprovision(uint16 out_ses_id);

int video_ca_send_update_service(uint16 qam_id, uint16 prog_num);
int video_ca_send_add_pid(uint16 qam_id, uint16 pid);
int video_ca_send_delete_pid(uint16 qam_id, uint16 pid);
int video_ca_send_sid_exists(uint16 qam_id, uint16 prog_num, uint64 p_object);
int video_ca_send_no_sid(uint16 qam_id, uint16 prog_num, uint64 p_object);
int video_ca_send_pid_exists(uint16 qam_id, uint16 pid, uint64 p_object);
int video_ca_send_no_pid(uint16 qam_id, uint16 pid, uint64 p_object);
int video_ca_send_pid_provisioned(uint16 qam_id, uint16 pid, uint64 p_object);

boolean video_ca_msg_handler(ipc_message *ipc_msg, ipc_message *rsp_msg,
                             boolean *rsp_flag);
void video_ca_report_qam_change(uint16 qam_id, boolean old_mode, 
                                uint16 old_onid, uint16 old_tsid); 

int video_ca_delete_pid(pmt_change_t *pmt_chg, uint16 pid, uint16 stream_type);
void video_ca_delete_pmt_es_pids(pmt_change_t *pmt_chg, struct pmt_info_ *pmt);
int video_ca_add_pid(pmt_change_t *pmt_chg, uint16 pid, uint16 stream_type);
void video_ca_add_pmt_es_pids(pmt_change_t *pmt_chg, struct pmt_info_ *pmt);
int video_ca_report_pmt_change(uint16 qam_id, pmt_change_t *pmt_chg);
void ecm_info_update(struct crsl_insert_ *insert);
int ecm_insert_schedule(struct crsl_insert_ *insert, uint32 qam_id,
                        uint32 flow_id, uint32 offset, uint16 pid);

void video_ca_set_cw_index_handler(ipc_message* ipc_msg);
void video_ca_add_ecm_handler(ipc_message* ipc_msg);
void video_ca_delete_ecm_handler(ipc_message* ipc_msg);
void video_ca_add_ca_descriptor_handler(ipc_message* ipc_msg);
void video_ca_delete_ca_descriptor_handler(ipc_message* ipc_msg);
void video_ca_get_sid_handler(ipc_message* ipc_msg);
void video_ca_get_pid_handler(ipc_message* ipc_msg);
void video_ca_reserve_pid_handler(ipc_message* ipc_msg);
void video_ca_request_pid_handler(ipc_message* ipc_msg);
void video_ca_sid_ecm_pid_handler(ipc_message* ipc_msg);
void video_ca_release_pid_handler(ipc_message* ipc_msg);
void video_ca_report_add_ts(void);

struct cli_control_block_;
boolean video_ca_cli_handler(struct cli_control_block_ *ccb);
void video_ca_apply_desc (struct out_prog_ *prog, uint32 qam_id);
void video_ca_block_update(struct out_session_ *ses, struct out_prog_ *prog);

#endif  // __VIDEO_CA_H__

