///  Copyright (c) 2014-2015 by Cisco Systems, Inc.
///  All rights reserved.
///

#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "video.h"
#include "video_cli.h"
#include "sim_video.h"
#include "generic_cli.h"
#include "sim_cli_video.h"
#include "video_util.h"
#include "scs_messages.h"
#include "video_ca.h"


#define MAX_USED_PROGS                  250

#define FRAGMENT_HEADER                 0xFFFFFFFE;
#define FRAGMENT_ALL                    0xFFFFFFFD;


uint32 transaction_id = 0;
uint8 video_msg_buf[1024];
uint16 in_list[64];

// Default settings
uint32 context_id;
int cr_mode = VIDEO_CR_MODE_UNIFIED_VBR;
uint32 jitter_size = 100;
uint32 delay_target = 70;
int init_timer = 2000;                  // default init timer (in ms)
int idle_timer = 500;                   // default idle timer (in ms)
int off_timer = 30;                     // default off timer (in sec)
boolean encrypt_flag = FALSE;           // default session encryption flag
uint32 owner_id = SIM_CLI_OWNER_ID;     // default owner ID
uint16 cas_sys_id = 0xE000;             // default CAS system ID


static inline
const char* get_cr_mode_text (int cr_mode)
{
    switch (cr_mode) {
        case VIDEO_CR_MODE_UNIFIED_VBR: return "unified-vbr";
        case VIDEO_CR_MODE_UNIFIED_CBR: return "unified-cbr";
        case VIDEO_CR_MODE_MASTER_SLAVE: return "master-slave";
    }
    return "invalid";
}


int cli_video_show_default (cli_control_block *ccb)
{
    printf("Config: %d\n", context_id);
    printf("Clock recovery mode: %s\n", get_cr_mode_text(cr_mode));
    printf("Jitter %d, delay %d\n", jitter_size, delay_target);
    printf("Timer: init %d, idle %d, off %d\n",
           init_timer, idle_timer, off_timer);
    printf("Encrypt flag: %d\n", encrypt_flag);
    printf("Owner ID: %d\n", owner_id);
    printf("CAS System ID: %d\n", cas_sys_id);
    return EOK;
}


static inline
uint16 get_base_pid_by_prog_num (int prog_num)
{
    return 0x30 + ((prog_num - 1) % MAX_USED_PROGS) * MAX_PIDS_PER_PROG;
}


static
int add_session (cli_control_block *ccb, int af, int cast_type, int proc_type,
                 video_pid_range_t pid_range)
{
    int base = 0;

    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header *ipc_hdr = IPC_HEADER(video_msg_buf);
    video_add_session_msg_t *cmd = IPC_DATA(video_msg_buf);

    ipc_hdr->type = MSG_TYPE_VIDEO_ADD_SESSION;
    ipc_hdr->size = sizeof(*cmd);

    cmd->primary_id = context_id;
    cmd->owner_id = owner_id;
    cmd->transaction_id = transaction_id++;
    cmd->resource_id = ccb->data[3].i32;

    // ip addr length
    if (af == AF_INET) {
        cmd->ip_addr_len = IPV4_ADDR_LEN;
    } else {
        cmd->ip_addr_len = IPV4_ADDR_LEN;
    }

    // Input type and parameters
    cmd->input_type = cast_type;

    switch (cast_type) {

    case INPUT_TYPE_UNICAST:
        if (af == AF_INET) {
            cmd->dst_ip_addr[0] = ccb->data[5].ip_addr[0];
        } else {
            cmd->dst_ip_addr[0] = ccb->data[5].ip_addr[0];
            cmd->dst_ip_addr[1] = ccb->data[5].ip_addr[1];
            cmd->dst_ip_addr[2] = ccb->data[5].ip_addr[2];
            cmd->dst_ip_addr[3] = ccb->data[5].ip_addr[3];
        }
        cmd->dst_udp_port = ccb->data[7].i32;
        base = 8;
        break;

    case INPUT_TYPE_MULTICAST_ASM:
        if (af == AF_INET) {
            cmd->dst_ip_addr[0] = ccb->data[5].ip_addr[0];
        } else {
            cmd->dst_ip_addr[0] = ccb->data[5].ip_addr[0];
            cmd->dst_ip_addr[1] = ccb->data[5].ip_addr[1];
            cmd->dst_ip_addr[2] = ccb->data[5].ip_addr[2];
            cmd->dst_ip_addr[3] = ccb->data[5].ip_addr[3];
        }
        base = 6;
        break;

    case INPUT_TYPE_MULTICAST_SSM:
        if (af == AF_INET) {
            cmd->src_ip_addr[0] = ccb->data[5].ip_addr[0];

            cmd->dst_ip_addr[0] = ccb->data[7].ip_addr[0];
        } else {
            cmd->src_ip_addr[0] = ccb->data[5].ip_addr[0];
            cmd->src_ip_addr[1] = ccb->data[5].ip_addr[1];
            cmd->src_ip_addr[2] = ccb->data[5].ip_addr[2];
            cmd->src_ip_addr[3] = ccb->data[5].ip_addr[3];

            cmd->dst_ip_addr[0] = ccb->data[7].ip_addr[0];
            cmd->dst_ip_addr[1] = ccb->data[7].ip_addr[1];
            cmd->dst_ip_addr[2] = ccb->data[7].ip_addr[2];
            cmd->dst_ip_addr[3] = ccb->data[7].ip_addr[3];
        }
        base = 8;
        break;

    default:
        break;
    }

    // From default parameters
    cmd->cbr = cr_mode;
    cmd->jitter_size = jitter_size;
    cmd->delay_target = delay_target;
    cmd->idle_threshold = idle_timer;
    cmd->init_threshold = init_timer;
    cmd->off_timer = off_timer;
    cmd->encrypt_flag = encrypt_flag;

    cmd->bitrate_alloc = ccb->data[base + 1].i32;
    cmd->qam_id = ccb->data[base + 3].i32;

    // Session processing types
    switch (proc_type) {

    case PROCESS_TYPE_REMAP:
        cmd->stream_type = STREAM_TYPE_SPTS;
        cmd->pid_remap = 1;
        cmd->parse_psi = 1;
        cmd->dejitter = 1;
        cmd->prog_num = ccb->data[base + 5].i32;
        cmd->res_pid = pid_range;
        break;

    case PROCESS_TYPE_REMUX:
        cmd->stream_type = STREAM_TYPE_MPTS;
        cmd->pid_remap = 1;
        cmd->parse_psi = 1;
        cmd->dejitter = 1;
        cmd->prog_num = 0;
        break;

    case PROCESS_TYPE_PASSTHRU:
        cmd->stream_type = STREAM_TYPE_MPTS;
        cmd->pid_remap = 0;
        cmd->parse_psi = 1;
        cmd->dejitter = 1;
        cmd->prog_num = 0;
        break;

    case PROCESS_TYPE_DATA:
        cmd->pid_remap = 0;
        cmd->parse_psi = 0;
        cmd->dejitter = 0;
        cmd->prog_num = 0;
        break;

    default:
        break;
    }

    cmd->enable = 1;

    uint32 len = sizeof(ipc_message_header) + sizeof(*cmd);
    uint32 n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }

    return EOK;
}


int cli_video_add_session (cli_control_block *ccb)
{
    int af;                     // address family
    uint32 ip_start[4], ip_stop[4];
    int udp_start, udp_stop;
    int udp_inc, ip_inc;
    int qam_id, qam_id_start, qam_id_stop, qam_id_inc;
    int prog_num, prog_num_start, prog_num_stop, prog_num_inc;
    int base = 0;               // index of CLI keyword "bitrate"
    int rc;
    int first_pid, last_pid, pid_inc;
    video_pid_range_t pid_range = {0, 0};

    cli_prepare_cc(ccb);

    // Decode fooid
    int cast_type = ccb->fooid & 3;
    int proc_type = (ccb->fooid >> 2) & 3;
    int pid_range_flag = (ccb->fooid >> 4) & 1;

    // Classify input types
    switch (cast_type) {

    case INPUT_TYPE_UNICAST:
        base = 8;

        // Parse UDP range
        rc = cli_parse_range_int(ccb->tokens[7], &udp_start, &udp_stop,
                                 &udp_inc);
        if (rc == -1) {
            printf("Error: bad value/range for UDP: %s\n", ccb->tokens[7]);
            return 0;
        }
        ccb->data[7].i32 = udp_start;

        // Parse destination IP
        if (!cli_parse_ip_addr(ccb->tokens[5], &af, ccb->data[5].ip_addr)) {
            printf("Bad IP address: %s\n", ccb->tokens[5]);
            return 0;
        }
        break;

    case INPUT_TYPE_MULTICAST_ASM:
        base = 6;

        // Parse group IP range
        rc = cli_parse_range_ip_addr(ccb->tokens[5], &af, ip_start, ip_stop,
                                     &ip_inc);
        if (rc == -1) {
            printf("Error: bad range for group IP address: %s\n",
                   ccb->tokens[5]);
            return 0;
        }
        ccb->data[5].ip_addr[0] = ip_start[0];
        ccb->data[5].ip_addr[1] = ip_start[1];
        ccb->data[5].ip_addr[2] = ip_start[2];
        ccb->data[5].ip_addr[3] = ip_start[3];
        break;

    case INPUT_TYPE_MULTICAST_SSM:
        base = 8;

        // Parse group IP range
        rc = cli_parse_range_ip_addr(ccb->tokens[7], &af, ip_start, ip_stop,
                                     &ip_inc);
        if (rc == -1) {
            printf("Error: bad range for group IP address: %s\n",
                   ccb->tokens[7]);
            return 0;
        }
        ccb->data[7].ip_addr[0] = ip_start[0];
        ccb->data[7].ip_addr[1] = ip_start[1];
        ccb->data[7].ip_addr[2] = ip_start[2];
        ccb->data[7].ip_addr[3] = ip_start[3];

        // Parse source IP (TODO: check if af matches)
        cli_parse_ip_addr(ccb->tokens[5], &af, ccb->data[5].ip_addr);
        break;

    default:
        printf("Bad cast type: %d\n", cast_type);
        return 0;
    }

    // Parse QAM ID range
    rc = cli_parse_range_int(ccb->tokens[base + 3],
                             &qam_id_start, &qam_id_stop, &qam_id_inc);
    if (rc == -1) {
        printf("Error: bad range for QAM ids: %s\n", ccb->tokens[base + 3]);
        return 0;
    }

    if (proc_type == PROCESS_TYPE_REMAP) {
        // Parse program number range
        rc = cli_parse_range_int(ccb->tokens[base + 5], &prog_num_start,
                                 &prog_num_stop, &prog_num_inc);
        if (rc == -1) {
            printf("Error: bad range for program numbers: %s\n",
                   ccb->tokens[base + 5]);
            return 0;
        }

        if (pid_range_flag) {
            // Parse Program pid range
            rc = cli_parse_range_int(ccb->tokens[base + 7], &first_pid,
                                     &last_pid, &pid_inc);
            if (rc == -1 || pid_inc != 1) {
                printf("Error: bad pid range: %s\n", ccb->tokens[base + 7]);
                return 0;
            }
            pid_range.first = first_pid;
            pid_range.last = last_pid;
        }

    } else {
        prog_num_start = prog_num_stop = 0;
        prog_num_inc = 1;
    }

    // Session creation loop
    for (qam_id = qam_id_start; qam_id <= qam_id_stop; qam_id += qam_id_inc) {
        ccb->data[base + 3].i32 = qam_id;

        for (prog_num = prog_num_start; prog_num <= prog_num_stop;
             prog_num += prog_num_inc) {
            ccb->data[base + 5].i32 = prog_num;

            if (!pid_range_flag) {
                pid_range.first = get_base_pid_by_prog_num(prog_num);
                pid_range.last = pid_range.first + MAX_PIDS_PER_PROG - 1;
            }

            add_session(ccb, af, cast_type, proc_type, pid_range);

            // Update input parameters
            switch (cast_type) {

            case INPUT_TYPE_UNICAST:
                 ccb->data[7].i32 += udp_inc;
                 if (ccb->data[7].i32 > udp_stop) {
                     ccb->data[7].i32 = udp_start;
                 }
                 break;

            case INPUT_TYPE_MULTICAST_ASM:
                 if (af == AF_INET) {
                     ccb->data[5].u32 += ip_inc;
                     if (ccb->data[5].u32 > ip_stop[0]) {
                         ccb->data[5].u32 = ip_start[0];
                     }
                 } else {
                     ccb->data[5].ip_addr[3] += ip_inc;
                     if (ccb->data[5].ip_addr[3] > ip_stop[3]) {
                         ccb->data[5].ip_addr[3] = ip_start[3];
                     }
                 }
                 break;

            case INPUT_TYPE_MULTICAST_SSM:
                 if (af == AF_INET) {
                     ccb->data[7].u32 += ip_inc;
                     if (ccb->data[7].u32 > ip_stop[0]) {
                         ccb->data[7].u32 = ip_start[0];
                     }
                 } else {
                     ccb->data[7].ip_addr[3] += ip_inc;
                     if (ccb->data[7].ip_addr[3] > ip_stop[3]) {
                         ccb->data[7].ip_addr[3] = ip_start[3];
                     }
                 }
                 break;

            default:
                 printf("Unknown input cast type: %d\n", cast_type);
            }

            // Update session ID
            ccb->data[3].i32++;
        }
    }

    return EOK;
}


static
int delete_session (cli_control_block *ccb)
{
    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header *ipc_hdr = IPC_HEADER(video_msg_buf);
    video_delete_session_msg_t *cmd = IPC_DATA(video_msg_buf);

    ipc_hdr->type = MSG_TYPE_VIDEO_DELETE_SESSION;
    ipc_hdr->size = sizeof(*cmd);

    cmd->primary_id = context_id;
    cmd->transaction_id = transaction_id++;
    cmd->resource_id = ccb->data[3].i32;

    uint32_t len = sizeof(ipc_message_header) + sizeof(*cmd);
    uint32_t n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }
    return EOK;
}


int cli_video_delete_session (cli_control_block *ccb)
{
    int vsid, vsid_stop, vsid_inc;
    int err;

    cli_prepare_cc(ccb);

    // Parse out session id range
    err = cli_parse_range_int(ccb->tokens[3], &vsid, &vsid_stop, &vsid_inc);
    if (err == -1) {
        printf("Error: bad range for video session ids\n");
        return 0;
    }

    for ( ; vsid <= vsid_stop; vsid += vsid_inc) {
        ccb->data[3].i32 = vsid;
        err = delete_session(ccb);
    }
    return 0;
}


int cli_video_add_crsl (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header *ipc_hdr = IPC_HEADER(video_msg_buf);
    video_add_crsl_msg_t* cmd = IPC_DATA(video_msg_buf);

    cmd->primary_id = context_id;
    cmd->owner_id = owner_id;
    cmd->transaction_id = transaction_id++;
    cmd->resource_id = ccb->data[3].i32;
    cmd->num_tp = ccb->data[7].i32;
    cmd->insert_count = ccb->data[13].i32;
    cmd->insert_period = ccb->data[15].i32;

    cmd->num_qam = cli_parse_list_int(ccb->tokens[5], cmd->qam_id,
                                      MAX_CRSL_INSERT);

    char* pat_hex = ccb->tokens[11];
    int pat_len = strnlen(pat_hex, 20);
    if ((pat_len & 1) || (pat_len < 2)) {
        printf("Bad payload pattern length\n");
        return EOK;
    }
    pat_len /= 2;

    // Currently just set TP data to 0
    int i, j;
    uint32 data;
    int idx = 0;
    uint8* ptr = cmd->tp_data;

    for (i=0; i<cmd->num_tp; i++, ptr+=TP_SIZE) {
        tp_header_t* tp_hdr = (tp_header_t*)ptr;
        tp_hdr->sync = SYNC_BYTE;
        tp_set_pid(tp_hdr, ccb->data[9].i32);
        ptr[4] = i;

        for (j=5; j<TP_SIZE; j++) {
            int rc = sscanf(pat_hex + idx*2, "%2x", &data);
            ptr[j] = data;
            if (rc != 1) {
                printf("Bad payload pattern\n");
                return EOK;
            }
            if (++idx >= pat_len)  idx = 0;
        }
    }

    ipc_hdr->type = MSG_TYPE_VIDEO_ADD_CAROUSEL;
    ipc_hdr->size = sizeof(*cmd) + cmd->num_tp * TP_SIZE;
    uint32 len = sizeof(ipc_message_header) + ipc_hdr->size;
    uint32 n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }

    return EOK;
}


int cli_video_delete_crsl (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header *ipc_hdr = IPC_HEADER(video_msg_buf);
    video_delete_crsl_msg_t* cmd = IPC_DATA(video_msg_buf);

    cmd->primary_id = context_id;
    cmd->owner_id = owner_id;
    cmd->transaction_id = transaction_id++;
    cmd->resource_id = ccb->data[3].i32;
    cmd->num_qam = (ccb->num_tokens == 4) ? 0 :
                       cli_parse_list_int(ccb->tokens[5], cmd->qam_id,
                                          MAX_CRSL_INSERT);

    ipc_hdr->type = MSG_TYPE_VIDEO_DELETE_CAROUSEL;
    ipc_hdr->size = sizeof(*cmd) + cmd->num_qam * sizeof(uint16);
    uint32 len = sizeof(ipc_message_header) + ipc_hdr->size;
    uint32 n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }

    return EOK;
}


int cli_video_delete_owner (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header *ipc_hdr = IPC_HEADER(video_msg_buf);
    video_delete_owner_msg_t* cmd = IPC_DATA(video_msg_buf);

    cmd->primary_id = context_id;
    cmd->owner_id = ccb->data[3].i32;
    cmd->transaction_id = transaction_id++;

    ipc_hdr->type = MSG_TYPE_VIDEO_DELETE_OWNER;
    ipc_hdr->size = sizeof(video_delete_owner_msg_t);
    uint32 len = sizeof(ipc_message_header) + ipc_hdr->size;
    uint32 n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }

    return EOK;
}

#define VIDEO_QAM_LIST_SIZE             8

int cli_video_query_crsl (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header *ipc_hdr = IPC_HEADER(video_msg_buf);
    video_query_crsl_insert_t *qry = IPC_DATA(video_msg_buf);

    qry->primary_id = context_id;
    qry->transaction_id = transaction_id++;

    uint16 list[VIDEO_QAM_LIST_SIZE];
    qry->num_item = cli_parse_list_int(ccb->tokens[5], list,
                                       VIDEO_QAM_LIST_SIZE);
    int i;
    for (i=0; i<qry->num_item; i++) {
        qry->item[i].resource_id = ccb->data[3].i32;
        qry->item[i].qam_id = list[i];
    }

    ipc_hdr->type = MSG_TYPE_VIDEO_QUERY_CAROUSEL_INSERT;
    ipc_hdr->size = sizeof(*qry) + sizeof(crsl_insert_query_t) * qry->num_item;
    uint32 len = sizeof(ipc_message_header) + ipc_hdr->size;
    uint32 n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }

    n = read(video_msg_fd, video_msg_buf, sizeof(video_msg_buf));
    if (n >= sizeof(video_reply_crsl_insert_t)) {
        video_reply_crsl_insert_t* rsp
                = (video_reply_crsl_insert_t*)IPC_DATA(video_msg_buf);
        printf("Status %d: %d items\n", rsp->status, rsp->num_item);

        for (i=0; i<rsp->num_item; i++) {
            printf("  Resource %d, QAM %d: state %d, count %d\n",
                   rsp->item[i].resource_id, rsp->item[i].qam_id, 
                   rsp->item[i].state, rsp->item[i].insert_cnt); 
        }
    }
    return EOK;
}


static
void show_video_in_stat (video_in_stat_t *my_stat)
{
    printf("  IP: in %u, RTP %d, drop %u\n",
           my_stat->ip_cnt, my_stat->rtp_cnt, my_stat->drop_ip_cnt);
    printf("  TP: in %u, pcr %u, psi %u, null %u\n",
           my_stat->tp_cnt, my_stat->pcr_tp_cnt, my_stat->psi_tp_cnt,
           my_stat->null_tp_cnt);
    printf("  new-PAT %u, new-PMT %u, disc %u, tb-disc %d\n",
           my_stat->new_pat_cnt, my_stat->new_pmt_cnt, my_stat->disc_cnt,
           my_stat->tb_disc_cnt);
    printf("  sync-loss %u, cc-err %u, pcr-jump %u, xover %u, "\
           "underflow %u, overflow %u, block %u\n",
           my_stat->sync_loss_cnt, my_stat->cc_err_cnt, my_stat->pcr_jump_cnt,
           my_stat->pcr_xover_cnt, my_stat->underflow_cnt,
           my_stat->overflow_cnt, my_stat->block_tp_cnt);
    printf("  Life: ");
    print_elapsed_time(stdout, my_stat->start_time);
    printf("  Bitrate: measured %d, PCR %d\n",
           my_stat->measured_bitrate, my_stat->pcr_bitrate);
    printf("  Stay time %d, jitter %d\n",
           my_stat->stay_time, my_stat->jitter);
    printf("  Idle time %d, Idle count %d\n",
           my_stat->idle_time, my_stat->idle_cnt);
}


static
void show_video_out_stat (video_out_stat_t *my_stat)
{
    if ((my_stat->session_state & SESSION_STATE_BLOCKED)) {
        printf("  CONFLICT detected\n");
    }
    printf("  TP: in %u, pcr %u, psi %u\n",
           my_stat->tp_cnt, my_stat->pcr_tp_cnt, my_stat->psi_tp_cnt);
    printf("      drop %u, forward %u, insert %u\n",
           my_stat->drop_tp_cnt, my_stat->forward_tp_cnt,
           my_stat->insert_tp_cnt);
    printf("  new-PAT %u, new-PMT %u\n",
           my_stat->new_pat_cnt, my_stat->new_pmt_cnt);
    printf("  info-overrun %u, info-err %u, block %u, overdue %u\n",
           my_stat->info_overrun_cnt, my_stat->info_err_cnt,
           my_stat->block_tp_cnt, my_stat->overdue_tp_cnt);
    printf("  invalid-rate %u, underflow %u, overflow %u\n",
           my_stat->invalid_rate_cnt, my_stat->underflow_cnt,
           my_stat->overflow_cnt);
    printf("  Life: ");
    print_elapsed_time(stdout, my_stat->start_time);
    printf("  Bitrate %d\n", my_stat->measured_bitrate);
}


int cli_video_query_ses_stat (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header *ipc_hdr = IPC_HEADER(video_msg_buf);
    video_query_stat_msg_t *qry = IPC_DATA(video_msg_buf);

    qry->primary_id = context_id;
    qry->owner_id = owner_id;
    qry->transaction_id = transaction_id++;
    qry->resource_id = ccb->data[3].i32;

    ipc_hdr->type = MSG_TYPE_VIDEO_QUERY_STAT;
    ipc_hdr->size = sizeof(*qry);
    uint32 len = sizeof(ipc_message_header) + ipc_hdr->size;
    uint32 n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }

    n = read(video_msg_fd, video_msg_buf, sizeof(video_msg_buf));
    if (n >= sizeof(video_query_stat_resp_t)) {
        video_query_stat_resp_t* rsp
                = (video_query_stat_resp_t*)IPC_DATA(video_msg_buf);
        printf("Status: %d (%s)\n", rsp->status, strerror(rsp->status));
        if (rsp->status == EOK) {
            printf("Input statistics:\n");
            show_video_in_stat(&rsp->in_stat);
            printf("\nOutput statistics:\n");
            show_video_out_stat(&rsp->out_stat);
        }
    }

    return EOK;
}


// Return the total size of all PAT sections
static
int show_pat (video_psi_info_t* pat)
{
    int tot_size = 0;
    uintptr_t addr = (uintptr_t)pat;

    if (!pat->flags) {
        printf("PAT not available\n");
        return 0;
    }

    int i, j;
    for (i = pat->section_cnt; i > 0; i--) {
        pat_header_t* pat_hdr = (pat_header_t*)pat->psi_section;
        pat_prog_info_t* pat_prog = (pat_prog_info_t*)(pat_hdr + 1);
        int sect_len = pat_get_section_size(pat_hdr);
        int prog_cnt = (sect_len - 12) / 4;

        printf("PAT: ver %d, tsid %d, len %d, section %d/%d\n",
               pat_hdr->ver, pat_get_tsid(pat_hdr), sect_len,
               pat_hdr->sect_num, pat_hdr->last_sect_num);

        for (j=0; j<prog_cnt; j++, pat_prog++) {
            uint16 prog_num = pat_get_prog_num(pat_prog);
            printf("  Prog %d: %s %d\n", prog_num,
                   (prog_num? "PMT" : "NIT"), pat_get_pmt_pid(pat_prog));
        }

        // Update address for next section
        addr += sect_len;
        pat = (video_psi_info_t*)addr;
        tot_size += sect_len;;
    }

    return tot_size;
}


int cli_video_query_ses_in_pat (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header *ipc_hdr = IPC_HEADER(video_msg_buf);
    video_query_pat_msg_t *qry = IPC_DATA(video_msg_buf);

    qry->primary_id = context_id;
    qry->owner_id = owner_id;
    qry->transaction_id = transaction_id++;
    qry->resource_id = ccb->data[3].i32;

    ipc_hdr->type = MSG_TYPE_VIDEO_QUERY_IN_PAT;
    ipc_hdr->size = sizeof(*qry);
    uint32 len = sizeof(ipc_message_header) + ipc_hdr->size;
    uint32 n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }

    n = read(video_msg_fd, video_msg_buf, sizeof(video_msg_buf));
    if (n >= sizeof(video_query_pat_resp_t)) {
        video_query_pat_resp_t* rsp
                = (video_query_pat_resp_t*)IPC_DATA(video_msg_buf);
        printf("Status: %d (%s)\n", rsp->status, strerror(rsp->status));
        if (rsp->status == EOK || rsp->status == E2BIG) {
            show_pat(rsp->section);
        }
    }

    return EOK;
}


int cli_video_query_qam_pat (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);
    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header *ipc_hdr = IPC_HEADER(video_msg_buf);
    video_query_pat_msg_t *qry = IPC_DATA(video_msg_buf);

    qry->primary_id = context_id;
    qry->owner_id = owner_id;
    qry->transaction_id = transaction_id++;
    qry->resource_id = ccb->data[3].i32;

    ipc_hdr->type = MSG_TYPE_VIDEO_QUERY_QAM_PAT;
    ipc_hdr->size = sizeof(*qry);
    uint32 len = sizeof(ipc_message_header) + ipc_hdr->size;
    uint32 n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }

    n = read(video_msg_fd, video_msg_buf, sizeof(video_msg_buf));
    if (n >= sizeof(video_query_pat_resp_t)) {
        video_query_pat_resp_t* rsp
                = (video_query_pat_resp_t*)IPC_DATA(video_msg_buf);
        printf("Status: %d (%s)\n", rsp->status, strerror(rsp->status));
        if (rsp->status == EOK || rsp->status == E2BIG) {
            show_pat(rsp->section);
        }
    }

    return EOK;
}


// Return the number of bytes scanned
static
int show_info_loop (uint8 *ptr, int info_len)
{
    int tot_len = 0;

    while (info_len > 0) {
        descriptor_header_t* desc = (descriptor_header_t*)ptr;
        switch (desc->tag) {

            case DESC_TAG_CA:
            {
                ca_descriptor_header_t *desc2 = (ca_descriptor_header_t*)desc;
                printf(", (ca: sys-id %d, pid %d)",
                       (desc2->sys_id_hi << 8) | desc2->sys_id_lo,
                       (desc2->pid_hi << 8) | desc2->pid_lo);
                break;
            }

            case DESC_TAG_LANG:
            {
                int len = desc->len;
                uint8* ptr2 = (uint8*)(desc + 1);
                printf(", (lang:");

                while (len > 0) {
                    printf(" %c%c%c", ptr2[0], ptr2[1], ptr2[2]);
                    len -= 4;
                    ptr2 += 4;
                }
                printf(")");
                break;
            }

            default:
                printf(", (%d, %d)", desc->tag, desc->len);
        }

        int desc_len = sizeof(descriptor_header_t) + desc->len;
        ptr += desc_len;
        info_len -= desc_len;
        tot_len += desc_len;
    }

    return tot_len;
}


// Return the size of PMT section
static
int show_pmt (video_psi_info_t* pmt)
{
    int len = sizeof(video_psi_info_t);
    uint8* ptr = (uint8*)pmt->psi_section;

    if (!pmt->flags) {
        printf("PMT not available\n");
        return len;
    }

    if (pmt->section_cnt > 1) {
        assert(0);
    }

    int i;
    for (i=0; i<pmt->section_cnt; i++) {
        pmt_header_t* pmt_hdr = (pmt_header_t*)ptr;
        int sect_len = pmt_get_section_size(pmt_hdr);
        uint8* esinfo_end_addr = ptr + sect_len - CRC_SIZE;

        printf("PMT: ver %d, program %d, PCR %d, len %d",
               pmt_hdr->ver, pmt_get_prog_num(pmt_hdr),
               pmt_get_pcr_pid(pmt_hdr), sect_len);

        // Scan descriptors in program info
        ptr += PMT_SECT_HDR_SIZE;
        int info_len = pmt_get_prog_info_len(pmt_hdr);
        if (info_len > 0) {
            int info_len2 = show_info_loop(ptr, info_len);
            ptr += info_len2;
            if (info_len2 != info_len) {
                printf("\nBad program info\n");
            }
        }

        do {
            pmt_es_info_t* es_info = (pmt_es_info_t*)ptr;
            info_len = pmt_get_es_info_len(es_info);
            printf("\n  PID %d: type %d, info_len %d",
                   pmt_get_es_pid(es_info), es_info->es_type, info_len);

            // Scan descriptors in ES info
            ptr = (uint8*)(es_info + 1);
            if (info_len > 0) {
                int info_len2 = show_info_loop(ptr, info_len);
                ptr += info_len2;
                if (info_len2 != info_len) {
                    printf("\nBad ES info\n");
                }
            }
        } while (ptr < esinfo_end_addr);

        printf("\n");
        ptr += CRC_SIZE;    // skip over CRC

        len += sect_len;
        int pad_size = len % 4;
        if (pad_size) {
            len += 4 - pad_size;
            ptr += 4 - pad_size;
        }
    }

    return len;
}


static
void show_all_pmt (video_psi_info_t* pmt, int tot_len)
{
    while (tot_len > 0) {
        int len = show_pmt(pmt);
        tot_len -= len;
        pmt = (video_psi_info_t*)(((uintptr_t)pmt) + len);
    }
}


int cli_video_query_ses_in_pmt (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header *ipc_hdr = IPC_HEADER(video_msg_buf);
    video_query_pmt_msg_t *qry = IPC_DATA(video_msg_buf);

    qry->primary_id = context_id;
    qry->owner_id = owner_id;
    qry->transaction_id = transaction_id++;
    qry->resource_id = ccb->data[3].i32;
    qry->prog_idx = 0;          // start with 1st program

    ipc_hdr->type = MSG_TYPE_VIDEO_QUERY_IN_PMT;
    ipc_hdr->size = sizeof(*qry);
    uint32 len = sizeof(ipc_message_header) + ipc_hdr->size;
    uint32 n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }

    n = read(video_msg_fd, video_msg_buf, sizeof(video_msg_buf));
    if (n >= sizeof(ipc_message_header) + sizeof(video_query_pmt_resp_t)) {
        video_query_pmt_resp_t* rsp
                = (video_query_pmt_resp_t*)IPC_DATA(video_msg_buf);
        printf("Status: %d (%s)\n", rsp->status, strerror(rsp->status));
        if (rsp->status == EOK || rsp->status == E2BIG) {
            printf("PMT section: total %d, first %d, received %d, "\
                   "remaining %d\n", rsp->total_section, rsp->prog_idx,
                   rsp->num_section, rsp->remaining_section);
            show_all_pmt((video_psi_info_t*)rsp->section, rsp->size);
        }
    }

    return EOK;
}


int cli_video_query_ses_out_pmt (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header *ipc_hdr = IPC_HEADER(video_msg_buf);
    video_query_pmt_msg_t *qry = IPC_DATA(video_msg_buf);

    qry->primary_id = context_id;
    qry->owner_id = owner_id;
    qry->transaction_id = transaction_id++;
    qry->resource_id = ccb->data[3].i32;
    qry->prog_idx = 0;          // start with 1st program

    ipc_hdr->type = MSG_TYPE_VIDEO_QUERY_OUT_PMT;
    ipc_hdr->size = sizeof(*qry);
    uint32 len = sizeof(ipc_message_header) + ipc_hdr->size;
    uint32 n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }

    n = read(video_msg_fd, video_msg_buf, sizeof(video_msg_buf));
    if (n >= sizeof(ipc_message_header) + sizeof(video_query_pmt_resp_t)) {
        video_query_pmt_resp_t* rsp
                = (video_query_pmt_resp_t*)IPC_DATA(video_msg_buf);
        printf("Status: %d (%s)\n", rsp->status, strerror(rsp->status));
        if (rsp->status == EOK || rsp->status == E2BIG) {
            printf("PMT section: total %d, first %d, received %d, "\
                   "remaining %d\n", rsp->total_section, rsp->prog_idx,
                   rsp->num_section, rsp->remaining_section);
            show_all_pmt((video_psi_info_t*)rsp->section, rsp->size);
        }
    }

    return EOK;
}


int cli_video_set_filter_pid (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header *ipc_hdr = IPC_HEADER(video_msg_buf);
    video_update_output_msg_t *cmd = IPC_DATA(video_msg_buf);
    ipc_hdr->type = MSG_TYPE_VIDEO_UPDATE_PID_FILTER;

    cmd->primary_id = context_id;
    cmd->oper = ccb->fooid;
    cmd->item_count = cli_parse_list_int(ccb->tokens[7], cmd->list,
                                         VIDEO_PID_LIST_SIZE);
    ipc_hdr->size = sizeof(*cmd) + sizeof(uint16) * cmd->item_count;

    int vsid, vsid_stop, vsid_inc;
    int rc = cli_parse_range_int(ccb->tokens[3], &vsid, &vsid_stop, &vsid_inc);
    if (rc == -1) {
        printf("Error: bad range for video session ids\n");
        return 0;
    }

    int msg_size = sizeof(ipc_message_header) + ipc_hdr->size;
    for ( ; vsid <= vsid_stop; vsid += vsid_inc) {
        cmd->resource_id = vsid;
        cmd->transaction_id = transaction_id++;

        int n = write(video_msg_fd, video_msg_buf, msg_size);
        if (n != msg_size) {
            printf("%s Error: write size mismatch\n", __FUNCTION__);
        }
    }
    return EOK;
}


int cli_video_set_filter_prog (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header *ipc_hdr = IPC_HEADER(video_msg_buf);
    video_update_output_msg_t *cmd = IPC_DATA(video_msg_buf);
    ipc_hdr->type = MSG_TYPE_VIDEO_UPDATE_PROG_FILTER;

    cmd->primary_id = context_id;

    cmd->oper = ccb->fooid;
    cmd->item_count = cli_parse_list_int(ccb->tokens[7], cmd->list,
                                         VIDEO_PROG_LIST_SIZE);

    ipc_hdr->size = sizeof(*cmd) + sizeof(uint16) * cmd->item_count;

    int vsid, vsid_stop, vsid_inc;
    int rc = cli_parse_range_int(ccb->tokens[3], &vsid, &vsid_stop, &vsid_inc);
    if (rc == -1) {
        printf("Error: bad range for video session ids\n");
        return 0;
    }

    int msg_size = sizeof(ipc_message_header) + ipc_hdr->size;
    for ( ; vsid <= vsid_stop; vsid += vsid_inc) {
        cmd->resource_id = vsid;
        cmd->transaction_id = transaction_id++;

        int n = write(video_msg_fd, video_msg_buf, msg_size);
        if (n != msg_size) {
            printf("%s Error: write size mismatch\n", __FUNCTION__);
        }
    }
    return EOK;
}


int cli_video_set_remap_prog (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    int first_pid, last_pid, pid_inc;
    int pid_range_flag = 0;

    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header *ipc_hdr = IPC_HEADER(video_msg_buf);
    video_update_remap_prog_msg_t *cmd = IPC_DATA(video_msg_buf);
    ipc_hdr->type = MSG_TYPE_VIDEO_UPDATE_PROG_REMAP;

    cmd->primary_id = context_id;

    switch (ccb->fooid) {

    case VIDEO_LIST_OPER_ADD | CLI_VIDEO_PID_RANGE:
        pid_range_flag = 1;

        // Parse Program pid range
        int rc = cli_parse_range_int(ccb->tokens[10], &first_pid,
                                     &last_pid, &pid_inc);
        if (rc == -1 || pid_inc != 1) {
            printf("Error: Bad pid range\n");
            return EOK;
        }
        // Intentionally falls through

    case VIDEO_LIST_OPER_ADD:
        cmd->item_count = cli_parse_list_int(ccb->tokens[7], cmd->in_prog_list,
                                             VIDEO_PROG_LIST_SIZE);
        int cnt = cli_parse_list_int(ccb->tokens[8], cmd->out_prog_list,
                                     VIDEO_PROG_LIST_SIZE);
        if (cmd->item_count != cnt) {
            printf("Error: unmatched list sizes %d, %d\n",
                   cmd->item_count, cnt);
            return EOK;
        }

        int i;
        for (i=0; i<cmd->item_count; i++) {
            if (pid_range_flag) {
                // Parse Program pid range
                cmd->pid_range[i].first = first_pid;
                cmd->pid_range[i].last = last_pid;

            } else {
                cmd->pid_range[i].first =
                        get_base_pid_by_prog_num(cmd->out_prog_list[i]);
                cmd->pid_range[i].last = cmd->pid_range[i].first
                                             + MAX_PIDS_PER_PROG - 1;
            }
        }
        cmd->oper = VIDEO_LIST_OPER_ADD;

        break;

    case VIDEO_LIST_OPER_DELETE:
        cmd->item_count = cli_parse_list_int(ccb->tokens[7], cmd->in_prog_list,
                                             VIDEO_PROG_LIST_SIZE);
        cmd->oper = VIDEO_LIST_OPER_DELETE;
        break;

    case VIDEO_LIST_OPER_CLEAR:
        cmd->oper = VIDEO_LIST_OPER_CLEAR;
        break;

    default:
        printf("Unknown list operation %d\n", ccb->fooid);
        return 0;
    }

    ipc_hdr->size = sizeof(*cmd);
    int msg_size = sizeof(ipc_message_header) + ipc_hdr->size;

    int vsid, vsid_stop, vsid_inc;
    int rc = cli_parse_range_int(ccb->tokens[3], &vsid, &vsid_stop, &vsid_inc);
    if (rc == -1) {
        printf("Error: bad range for video session ids\n");
        return 0;
    }

    for ( ; vsid <= vsid_stop; vsid += vsid_inc) {
        cmd->resource_id = vsid;
        cmd->transaction_id = transaction_id++;

        int n = write(video_msg_fd, video_msg_buf, msg_size);
        if (n != msg_size) {
            printf("%s Error: write size mismatch\n", __FUNCTION__);
        }
    }

    return EOK;
}


int cli_video_set_remap_pid (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    int first_pid, last_pid, pid_inc;
    int pid_range_flag = 0;

    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header *ipc_hdr = IPC_HEADER(video_msg_buf);
    video_update_remap_pid_msg_t *cmd = IPC_DATA(video_msg_buf);
    ipc_hdr->type = MSG_TYPE_VIDEO_UPDATE_PID_REMAP;

    cmd->primary_id = context_id;

    switch (ccb->fooid) {

    case VIDEO_LIST_OPER_ADD | CLI_VIDEO_PID_RANGE:
        pid_range_flag = 1;

        // Parse pid range
        int rc = cli_parse_range_int(ccb->tokens[10], &first_pid,
                                     &last_pid, &pid_inc);
        if (rc == -1 || pid_inc != 1) {
            printf("Error: Bad pid range\n");
            return EOK;
        }
        // Intentionally falls through

    case VIDEO_LIST_OPER_ADD:
        cmd->item_count = cli_parse_list_int(ccb->tokens[7], cmd->in_pid_list,
                                             VIDEO_PID_LIST_SIZE);
        int cnt = cli_parse_list_int(ccb->tokens[8], cmd->out_pid_list,
                                     VIDEO_PID_LIST_SIZE);
        if (cmd->item_count != cnt) {
            printf("Error: unmatched list sizes %d, %d\n",
                   cmd->item_count, cnt);
            return EOK;
        }
        cmd->oper = VIDEO_LIST_OPER_ADD;
        break;

    case VIDEO_LIST_OPER_DELETE:
        cmd->item_count = cli_parse_list_int(ccb->tokens[7], cmd->in_pid_list,
                                             VIDEO_PID_LIST_SIZE);
        cmd->oper = VIDEO_LIST_OPER_DELETE;
        break;

    case VIDEO_LIST_OPER_CLEAR:
        cmd->oper = VIDEO_LIST_OPER_CLEAR;
        break;

    default:
        printf("Unknown list operation %d\n", ccb->fooid);
        return EOK;
    }

    ipc_hdr->size = sizeof(*cmd);
    int msg_size = sizeof(ipc_message_header) + ipc_hdr->size;

    int vsid, vsid_stop, vsid_inc;
    int rc = cli_parse_range_int(ccb->tokens[3], &vsid, &vsid_stop, &vsid_inc);
    if (rc == -1) {
        printf("Error: bad range for video session ids\n");
        return EOK;
    }

    for ( ; vsid <= vsid_stop; vsid += vsid_inc) {
        cmd->resource_id = vsid;
        cmd->transaction_id = transaction_id++;

        int n = write(video_msg_fd, video_msg_buf, msg_size);
        if (n != msg_size) {
            printf("%s Error: write size mismatch\n", __FUNCTION__);
        }
    }

    return EOK;
}


static
int set_qam_config (int qam_id, int tsid, int onid, int psi_period,
                    int encryption_flag, int enable_flag, uint16 field_mask)
{
    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header* ipc_hdr = IPC_HEADER(video_msg_buf);
    video_config_qam_msg_t *cmd = IPC_DATA(video_msg_buf);

    ipc_hdr->type = MSG_TYPE_VIDEO_CONFIG_QAM;
    ipc_hdr->size = sizeof(*cmd);

    cmd->primary_id = context_id;
    cmd->transaction_id = transaction_id++;
    cmd->resource_id = qam_id;

    cmd->field_mask = field_mask;
    cmd->tsid = tsid;
    cmd->onid = onid;
    cmd->psi_period = psi_period;
    cmd->encrypt_flag = encryption_flag;
    cmd->enable = enable_flag;

    uint32 len = sizeof(ipc_message_header) + sizeof(*cmd);
    uint32 n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }
    return EOK;
}


int cli_video_set_qam_tsid_onid (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    // Parse QAM ID range
    int qam_id, qam_id_start, qam_id_stop, qam_id_inc;
    int rc = cli_parse_range_int(ccb->tokens[3], &qam_id_start, &qam_id_stop,
                                 &qam_id_inc);
    if (rc == -1) {
        printf("Error: bad range for QAM ids\n");
        return 0;
    }

    // Parse TSID range
    int tsid_start, tsid_stop, tsid_inc;
    rc = cli_parse_range_int(ccb->tokens[5], &tsid_start, &tsid_stop,
                             &tsid_inc);
    if ((rc == -1) || (tsid_start < 0) ||
                         (tsid_start > 65535) || (tsid_stop > 65535)) {
        printf("Error: bad range for TSID\n");
        return 0;
    }

    // Parse ONID range
    int onid_start, onid_stop, onid_inc;
    rc = cli_parse_range_int(ccb->tokens[7], &onid_start, &onid_stop,
                             &onid_inc);
    if ((rc == -1) || (onid_start < 0) ||
                         (onid_start > 65535) || (onid_stop > 65535)) {
        printf("Error: bad range for ONID\n");
        return 0;
    }

    int field_mask = (0x1 << qam_cfg_tsid) | (0x1 << qam_cfg_onid);

    int tsid = tsid_start;
    int onid = onid_start;
    for (qam_id = qam_id_start; qam_id <= qam_id_stop; qam_id += qam_id_inc) {
        set_qam_config(qam_id, tsid, onid, 0, 0, 0, field_mask);
        tsid += tsid_inc;
        onid += onid_inc;
        if (tsid > tsid_stop) {
            tsid = tsid_start;
        }
        if (onid > onid_stop) {
            onid = onid_start;
        }
    }

    return EOK;
}


int cli_video_set_qam_psi_period (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    // Parse QAM ID range
    int qam_id, qam_id_start, qam_id_stop, qam_id_inc;
    int rc = cli_parse_range_int(ccb->tokens[3], &qam_id_start, &qam_id_stop,
                                 &qam_id_inc);
    if (rc == -1) {
        printf("Error: bad range for QAM ids\n");
        return EOK;
    }

    int psi_period = ccb->data[5].u32;
    uint16 field_mask = (1 << qam_cfg_psi_period);

    for (qam_id = qam_id_start; qam_id <= qam_id_stop; qam_id += qam_id_inc) {
        set_qam_config(qam_id, 0, 0, psi_period, 0, 0, field_mask);
    }

    return EOK;
}


int cli_video_set_qam_encrypt (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    // Parse QAM ID range
    int qam_id, qam_id_start, qam_id_stop, qam_id_inc;
    int rc = cli_parse_range_int(ccb->tokens[3], &qam_id_start, &qam_id_stop,
                             &qam_id_inc);
    if (rc == -1) {
        printf("Error: bad range for QAM ids\n");
        return EOK;
    }

    uint16 field_mask = (1 << qam_cfg_encrypt);
    int encrypt_mode = 0;
    if (ccb->fooid == CLI_VIDEO_ON)  encrypt_mode = 1;

    for (qam_id = qam_id_start; qam_id <= qam_id_stop; qam_id += qam_id_inc) {
        set_qam_config(qam_id, 0, 0, 0, encrypt_mode, 0, field_mask);
    }

    return EOK;
}


int cli_video_set_qam_enable (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    // Parse QAM ID range
    int qam_id, qam_id_start, qam_id_stop, qam_id_inc;
    int rc = cli_parse_range_int(ccb->tokens[3], &qam_id_start, &qam_id_stop,
                             &qam_id_inc);
    if (rc == -1) {
        printf("Error: bad range for QAM ids\n");
        return EOK;
    }

    uint16 field_mask = (1 << qam_cfg_enable);
    int enable = 0;
    if (ccb->fooid == CLI_VIDEO_ON)  enable = 1;

    for (qam_id = qam_id_start; qam_id <= qam_id_stop; qam_id += qam_id_inc) {
        set_qam_config(qam_id, 0, 0, 0, 0, enable, field_mask);
    }

    return EOK;
}


int cli_video_set_default_config (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);
    context_id = ccb->data[4].i32;
    return EOK;
}


int cli_video_set_default_cr_mode (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);
    cr_mode = ccb->fooid;
    return EOK;
}


int cli_video_set_default_jitter_delay (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);
    jitter_size = ccb->data[4].i32;
    delay_target = ccb->data[6].i32;
    return EOK;
}


int cli_video_set_default_timer (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);
    init_timer = ccb->data[4].u32;
    idle_timer = ccb->data[5].u32;
    off_timer = ccb->data[6].u32;
    return EOK;
}


int cli_video_set_default_encrypt (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    switch (ccb->fooid) {

    case CLI_VIDEO_ON:
        encrypt_flag = TRUE;
        break;

    case CLI_VIDEO_OFF:
        encrypt_flag = FALSE;
        break;
    }

    printf("Default session encryption flag set to %d\n", encrypt_flag);
    return EOK;
}


int cli_video_set_default_ownerid (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);
    owner_id = ccb->data[4].u32;
    printf("Default owner ID set to %d\n", owner_id);
    return EOK;
}


int cli_video_set_default_cas_sys_id (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);
    cas_sys_id = ccb->data[4].u32;
    printf("Default CAS System ID set to %d\n", cas_sys_id);
    return EOK;
}


int cli_video_show_session_range (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    int id_start, id_stop, id_inc;
    int rc = cli_parse_range_int(ccb->tokens[3], &id_start, &id_stop, &id_inc);
    if (rc == -1 || id_inc != 1 || id_start >= id_stop) {
        printf("Bad session id range: %s\n", ccb->tokens[3]);
        return EOK;
    }

    ccb->data[3].i32 = id_start;
    ccb->data[4].i32 = id_stop;

    cli_sock_send_ccb(ccb, cli_sock_fd[CLI_VIDEO_SOCK]);
    return EOK;
}


int cli_video_ca_add_desc (cli_control_block *ccb)
{
    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header* ipc_hdr = IPC_HEADER(video_msg_buf);
    msg_add_ca_descriptor_t *cmd = IPC_DATA(video_msg_buf);
    ext_ca_descr_header_t *desc = (ext_ca_descr_header_t *)
                                       cmd->descriptor.m_pDescriptor;

    ipc_hdr->type = MSG_TYPE_VIDEO_CA_ADD_CA_DESCRIPTOR;
    ipc_hdr->size = sizeof(*cmd) + sizeof(ext_ca_descr_header_t);

    cli_prepare_cc(ccb);

    cmd->qam_id = ccb->data[5].u32;
    cmd->descriptor.descr_size = 9;

    desc->ca_header.tag = 0x09;
    desc->ca_header.len = 7;
    desc->ca_header.sys_id_hi = cas_sys_id >> 8;
    desc->ca_header.sys_id_lo = cas_sys_id & 0xFF;
    desc->ca_header.pid_hi = (ccb->data[7].u32 >> 8) & 0x1F;
    desc->ca_header.pid_lo = ccb->data[7].u32 & 0xFF;
    desc->priv_data1 = 0x01;
    desc->priv_data2 = 0x01;
    desc->priv_data3 = 0x01;

    // Classify input types
    switch (ccb->fooid) {

        case CLI_VIDEO_CA_DESC_PID:
            cmd->descriptor.m_ulTableIdExt = FRAGMENT_HEADER;
            cmd->descriptor.m_ulLocation = ccb->data[9].u32;
            desc->priv_data3 = ccb->data[11].u32;
            break;

        case CLI_VIDEO_CA_DESC_PROG:
            cmd->descriptor.m_ulTableIdExt = ccb->data[9].u32;
            cmd->descriptor.m_ulLocation = FRAGMENT_HEADER;
            break;

        case CLI_VIDEO_CA_DESC_ALL_ES:
            cmd->descriptor.m_ulTableIdExt = ccb->data[9].u32;
            cmd->descriptor.m_ulLocation = FRAGMENT_ALL;
            break;

        default:
            return -1;
    }

    uint32_t len = sizeof(ipc_message_header) + ipc_hdr->size;
    uint32_t n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }
    return EOK;
}


int cli_video_ca_delete_desc (cli_control_block *ccb)
{
    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header* ipc_hdr = IPC_HEADER(video_msg_buf);
    msg_remove_ca_descriptor_t *cmd = IPC_DATA(video_msg_buf);
    ext_ca_descr_header_t *desc = (ext_ca_descr_header_t *)
                                       cmd->descriptor.m_pDescriptor;

    ipc_hdr->type = MSG_TYPE_VIDEO_CA_REMOVE_CA_DESCRIPTOR;
    ipc_hdr->size = sizeof(*cmd) + sizeof(ca_descriptor_header_t);

    cli_prepare_cc(ccb);

    cmd->qam_id = ccb->data[5].u32;
    cmd->descriptor.descr_size = 9;

    // Classify input types
    switch (ccb->fooid) {

        case CLI_VIDEO_CA_DESC_PID:
            cmd->descriptor.m_ulTableIdExt = FRAGMENT_HEADER;
            cmd->descriptor.m_ulLocation = ccb->data[9].u32;
            break;

        case CLI_VIDEO_CA_DESC_PROG:
            cmd->descriptor.m_ulTableIdExt = ccb->data[9].u32;
            cmd->descriptor.m_ulLocation = FRAGMENT_HEADER;
            break;

        case CLI_VIDEO_CA_DESC_ALL_ES:
            cmd->descriptor.m_ulTableIdExt = ccb->data[9].u32;
            cmd->descriptor.m_ulLocation = FRAGMENT_ALL;
            break;

        default:
            return -1;
    }

    cmd->qam_id = ccb->data[5].u32;
    cmd->descriptor.descr_size = 9;

    desc->ca_header.tag = 0x09;
    desc->ca_header.len = 7;
    desc->ca_header.sys_id_hi = 0xE0;
    desc->ca_header.sys_id_lo = 0x00;
    desc->ca_header.pid_hi = (ccb->data[7].u32 >> 8) & 0x1F;
    desc->ca_header.pid_lo = ccb->data[7].u32 & 0xFF;
    desc->priv_data1 = 0x01;
    desc->priv_data2 = 0x01;
    desc->priv_data3 = 0x01;

    uint32_t len = sizeof(ipc_message_header) + ipc_hdr->size;
    uint32_t n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }
    return EOK;
}


int cli_video_ca_get_pid (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header *ipc_hdr = IPC_HEADER(video_msg_buf);
    msg_get_pid_t* cmd = IPC_DATA(video_msg_buf);

    ipc_hdr->type = MSG_TYPE_VIDEO_CA_GET_PID;
    ipc_hdr->size = sizeof(*cmd);
    cmd->qam_id = ccb->data[3].i32;
    cmd->pid = ccb->data[6].i32;

    uint32 len = sizeof(ipc_message_header) + ipc_hdr->size;
    uint32 n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }

    return EOK;
}


int cli_video_ca_get_sid (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header *ipc_hdr = IPC_HEADER(video_msg_buf);
    msg_get_sid_t* cmd = IPC_DATA(video_msg_buf);

    ipc_hdr->type = MSG_TYPE_VIDEO_CA_GET_SID;
    ipc_hdr->size = sizeof(*cmd);
    cmd->qam_id = ccb->data[3].i32;
    cmd->sid = ccb->data[6].i32;

    uint32 len = sizeof(ipc_message_header) + ipc_hdr->size;
    uint32 n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }

    return EOK;
}


int cli_video_ca_request_ecm_pid (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header* ipc_hdr = IPC_HEADER(video_msg_buf);
    msg_sid_ecm_pid_t *cmd = IPC_DATA(video_msg_buf);

    ipc_hdr->type = MSG_TYPE_VIDEO_CA_SID_ECM_PID;
    ipc_hdr->size = sizeof(*cmd);
    cmd->qam_id = ccb->data[3].i32;
    cmd->sid = ccb->data[7].i32;

    uint32_t len = sizeof(ipc_message_header) + ipc_hdr->size;
    uint32_t n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }

    return EOK;
}


int cli_video_ca_request_pid (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header* ipc_hdr = IPC_HEADER(video_msg_buf);
    msg_request_pid_t *cmd = IPC_DATA(video_msg_buf);

    ipc_hdr->type = MSG_TYPE_VIDEO_CA_REQUEST_PID;
    ipc_hdr->size = sizeof(*cmd);
    cmd->qam_id = ccb->data[3].i32;

    uint32_t len = sizeof(ipc_message_header) + ipc_hdr->size;
    uint32_t n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }

    return EOK;
}


int cli_video_ca_reserve_pid (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header* ipc_hdr = IPC_HEADER(video_msg_buf);
    msg_reserve_pid_t *cmd = IPC_DATA(video_msg_buf);

    ipc_hdr->type = MSG_TYPE_VIDEO_CA_RESERVE_PID;
    ipc_hdr->size = sizeof(*cmd);
    cmd->qam_id = ccb->data[3].i32;
    cmd->pid = ccb->data[6].i32;

    uint32_t len = sizeof(ipc_message_header) + ipc_hdr->size;
    uint32_t n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }

    return EOK;
}


int cli_video_ca_release_pid (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header* ipc_hdr = IPC_HEADER(video_msg_buf);
    msg_release_pid_t *cmd = IPC_DATA(video_msg_buf);

    ipc_hdr->type = MSG_TYPE_VIDEO_CA_RELEASE_PID;
    ipc_hdr->size = sizeof(*cmd);
    cmd->qam_id = ccb->data[3].i32;
    cmd->pid = ccb->data[6].i32;

    uint32_t len = sizeof(ipc_message_header) + ipc_hdr->size;
    uint32_t n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }

    return EOK;
}


int cli_video_ca_set_cw_index (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);

    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header* ipc_hdr = IPC_HEADER(video_msg_buf);
    msg_scs_cw_index_t *cmd = IPC_DATA(video_msg_buf);

    ipc_hdr->type = MSG_TYPE_VIDEO_CA_SET_CW_INDEX;
    ipc_hdr->size = sizeof(*cmd);
    cmd->qam_id = ccb->data[3].i32;
    cmd->pid = ccb->data[8].i32;
    cmd->p_cw = ccb->data[6].i32;

    uint32_t len = sizeof(ipc_message_header) + ipc_hdr->size;
    uint32_t n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }

    return EOK;
}


int cli_video_lcred_mode_role (cli_control_block *ccb)
{
    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header *ipc_hdr = IPC_HEADER(video_msg_buf);
    msg_card_lcred_mode_role_t *cmd = IPC_DATA(video_msg_buf);

    cli_prepare_cc(ccb);

    ipc_hdr->type = MSG_TYPE_VIDEO_LCRED_MODE_ROLE;
    ipc_hdr->size = sizeof(*cmd);

    // Set up lcred message
    switch (ccb->fooid) {

    case CLI_VIDEO_LCRED_NON_REDUNDANT:
        cmd->mode = LCRED_NON_REDUNDANT_MODE;
        cmd->role = LCRED_ACTIVE_ROLE;
        break;

    case CLI_VIDEO_LCRED_PRIMARY_ACTIVE:
        cmd->mode = LCRED_PRIMARY_MODE;
        cmd->role = LCRED_ACTIVE_ROLE;
        break;

    case CLI_VIDEO_LCRED_PRIMARY_STANDBY:
        cmd->mode = LCRED_PRIMARY_MODE;
        cmd->role = LCRED_STANDBY_ROLE;
        break;

    case CLI_VIDEO_LCRED_SECONDARY_ACTIVE:
        cmd->mode = LCRED_SECONDARY_MODE;
        cmd->role = LCRED_ACTIVE_ROLE;
        break;

    case CLI_VIDEO_LCRED_SECONDARY_STANDBY:
        cmd->mode = LCRED_SECONDARY_MODE;
        cmd->role = LCRED_STANDBY_ROLE;
        break;

    default:
        printf("Unknown CLI\n");
    }

    uint32_t len = sizeof(ipc_message_header) + sizeof(*cmd);
    uint32_t n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }
    return EOK;
}


int cli_video_lcred_go_hot (cli_control_block *ccb)
{
    memset(video_msg_buf, 0, sizeof(video_msg_buf));
    ipc_message_header *ipc_hdr = IPC_HEADER(video_msg_buf);
    msg_card_lcred_go_hot_t *cmd = IPC_DATA(video_msg_buf);

    cli_prepare_cc(ccb);

    ipc_hdr->type = MSG_TYPE_VIDEO_LCRED_GO_HOT;
    ipc_hdr->size = sizeof(*cmd);

    cmd->primary_slot = ccb->data[3].i32;
    cmd->group = 0;

    uint32_t len = sizeof(ipc_message_header) + sizeof(*cmd);
    uint32_t n = write(video_msg_fd, video_msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }
    return EOK;
}
